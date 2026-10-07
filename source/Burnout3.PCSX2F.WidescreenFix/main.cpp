#include "../../external/injector/include/ps2/runtime.hpp"
#include "../../external/injector/include/ps2/safetymips.hpp"
#include "../../external/injector/include/ps2/game_abi.hpp"
#include "../Shared/Console/PS2Display.hpp"
#include <array>
#include <cstdio>
extern "C" {
int CompatibleCRCList[] = {static_cast<int>(0xBEBF8793)};
int PCSX2Data[PCSX2Data_Size] = {1};
char OSDText[OSDStringNum][OSDStringSize] = {{1}};
}

namespace {
struct Layout {
    float aspect = 0, width = 640, hudScale = 1, offset = 0;
    float hudOrigin = 0, normalizedOrigin = 0, menuWidth = 640, rightAnchor = 1, leftAnchor = 0;
} layout;
std::array<SafetyMipsMid, 40> hooks;
size_t hookCount;
pcsx2::GameCallback<void()> hudReset1, hudReset2;

void UpdateLayout() {
    float aspect = console::ps2Aspect(PCSX2Data);
    if (layout.aspect == aspect) return;
    layout.aspect = aspect;
    layout.width = 480.0f * aspect;
    layout.hudScale = (4.0f / 3.0f) / aspect;
    layout.offset = (640.0f - layout.width) * 0.5f;
    layout.hudOrigin = 320.0f * (1.0f - layout.hudScale);
    layout.normalizedOrigin = layout.hudOrigin / 640.0f;
    layout.menuWidth = 640.0f * layout.hudScale;
    float anchorExtension = (layout.width - 640.0f) / 1155.0f;
    layout.rightAnchor = 1.0f + anchorExtension;
    layout.leftAnchor = -anchorExtension;
    injector::WriteMemory<float>(0x4E0A38, 1.0f / layout.hudScale);
    injector::WriteMemory<float>(0x4E0C70, aspect);
    injector::WriteMemory<float>(0x4E0C7C, aspect);
    injector::WriteMemory<float>(0x4E0C80, aspect * 2.0f);
}
void ApplyHud() {
    UpdateLayout();
    for (uintptr_t address : {0x6682B0u, 0x669B30u}) injector::WriteMemory<float>(address, layout.hudOrigin);
    for (uintptr_t address : {0x4B7688u, 0x4B7678u}) injector::WriteMemory<float>(address, layout.rightAnchor);
    for (uintptr_t address : {0x4B7658u, 0x4B7668u}) injector::WriteMemory<float>(address, layout.leftAnchor);
    for (uintptr_t address : {0x4CA660u, 0x4CA640u, 0x4CA650u}) injector::WriteMemory<float>(address, layout.width);
    for (uintptr_t address : {0x4CA638u, 0x4CA658u}) injector::WriteMemory<float>(address, layout.offset);
    injector::WriteMemory<float>(0x4E105C, 1.0f / layout.width);
}
void HudReset1() { reinterpret_cast<void (*)()>(0x4D5350)(); ApplyHud(); }
void HudReset2() { reinterpret_cast<void (*)()>(0x4DB330)(); ApplyHud(); }
void Store(uintptr_t address, float value) { *reinterpret_cast<float*>(address) = value; }
void Rejected(pcsx2_hook_status status) {
    std::snprintf(OSDText[0], OSDStringSize, "Burnout 3 fix disabled: patch validation failed (%u)", unsigned(status));
}
template<class Callback> void Mid(uintptr_t address, Callback callback, bool executeOriginal = true) {
    safetymips::Options options;
    options.execute_original = executeOriginal;
    hooks[hookCount++] = safetymips::create_mid(address, callback, options);
}
}

extern "C" void init() {
    if (injector::InitializeCheckedRuntime(Rejected) != PCSX2_HOOK_OK) return;
    ApplyHud();
    injector::WriteMemory<uint32_t>(0x228194, 0x24020001); // Select the native widescreen camera.
    hudReset1.bind(HudReset1); hudReset2.bind(HudReset2);
    injector::WriteMemory<uint32_t>(0x4DD840, hudReset1.address());
    injector::WriteMemory<uint32_t>(0x4DD9E4, hudReset2.address());
    Mid(0x1D475C, [](SafetyMipsContext&) { ApplyHud(); });
    // Run after the native call and its SQ delay slot; no fabricated call ABI.
    Mid(0x1D5188, [](SafetyMipsContext&) { ApplyHud(); });

    // Change each value after its native LUI and before its first consumer.
    // All displaced instructions and upper halves of the EE registers survive.
    for (uintptr_t site : {0x3D723Cu, 0x134F30u, 0x38AE3Cu, 0x31D6E8u, 0x31D740u,
                           0x31D7ECu, 0x31D794u, 0x31D844u, 0x31DA24u, 0x3A6988u, 0x3A69CCu})
        Mid(site, [](SafetyMipsContext& regs) { regs.v0 = injector::WordBits(layout.width); });
    Mid(0x31B184, [](SafetyMipsContext& regs) { regs.v1 = injector::WordBits(layout.width); });
    Mid(0x30D7E8, [](SafetyMipsContext& regs) { regs.v1 = injector::WordBits(layout.menuWidth); });
    Mid(0x3D70C0, [](SafetyMipsContext& regs) { regs.f1 = layout.offset; });
    Mid(0x3D72F8, [](SafetyMipsContext& regs) { regs.f2 = layout.offset; });

    // These two sites write width and origin together. Keeping each pair in a
    // single callback avoids overlapping hooks in the adjacent native stores.
    Mid(0x1A1770, [](SafetyMipsContext& regs) {
        regs.v0 = injector::WordBits(layout.width);
        Store(uintptr_t(regs.sp) + 0xA8, layout.offset);
        Store(uintptr_t(regs.sp) + 0xA0, layout.width);
    }, false);
    Mid(0x1A17D4, [](SafetyMipsContext& regs) {
        regs.v0 = injector::WordBits(layout.width);
        Store(uintptr_t(regs.sp) + 0x98, layout.offset);
        Store(uintptr_t(regs.sp) + 0x90, layout.width);
    }, false);
    Mid(0x134F74, [](SafetyMipsContext& regs) { Store(uintptr_t(regs.sp) + 0x168, layout.hudScale * 0.5f); });
    Mid(0x30D834, [](SafetyMipsContext& regs) {
        Store(uintptr_t(regs.a0) + 8, regs.f3);
        Store(uintptr_t(regs.a0), layout.hudOrigin);
        Store(uintptr_t(regs.a0) + 12, regs.f0);
        regs.f3 = layout.hudOrigin;
    }, false);
    Mid(0x4DC71C, [](SafetyMipsContext& regs) { Store(uintptr_t(regs.v1) + 0x1568, layout.offset); });
    Mid(0x4DC73C, [](SafetyMipsContext& regs) { Store(uintptr_t(regs.v1) + 0x1570, layout.width); });
    Mid(0x31B1F4, [](SafetyMipsContext& regs) { regs.f7 = 0.0f; Store(uintptr_t(regs.a0), 0.0f); });
    Mid(0x38AE04, [](SafetyMipsContext& regs) {
        regs.v0 = injector::WordBits(layout.width);
        Store(uintptr_t(regs.sp) + 0x78, layout.normalizedOrigin);
        Store(uintptr_t(regs.sp) + 0x70, layout.width);
    }, false);
    Mid(0x38AE50, [](SafetyMipsContext& regs) { Store(uintptr_t(regs.sp) + 0x68, layout.offset); });
    Mid(0x31D6FC, [](SafetyMipsContext& regs) { Store(uintptr_t(regs.sp) + 0x110, layout.normalizedOrigin); });
    Mid(0x31D754, [](SafetyMipsContext& regs) { Store(uintptr_t(regs.sp) + 0xF8, layout.normalizedOrigin); });
    Mid(0x31D800, [](SafetyMipsContext& regs) { Store(uintptr_t(regs.sp) + 0xC8, layout.normalizedOrigin); });
    Mid(0x31D7A8, [](SafetyMipsContext& regs) { Store(uintptr_t(regs.sp) + 0xE0, layout.normalizedOrigin); });
    Mid(0x31D858, [](SafetyMipsContext& regs) { Store(uintptr_t(regs.sp) + 0xB0, layout.normalizedOrigin); });
    Mid(0x31DA44, [](SafetyMipsContext& regs) { Store(uintptr_t(regs.sp) + 0x1D8, layout.offset); });
    Mid(0x3A699C, [](SafetyMipsContext& regs) { Store(uintptr_t(regs.sp) + 0x78, layout.offset); });
    Mid(0x3A69E0, [](SafetyMipsContext& regs) { Store(uintptr_t(regs.sp) + 0x60, layout.offset); });
    injector::FlushCaches();
}
extern "C" int main() { return 0; }
