#include "Game.hpp"
#include "Controls.hpp"
#include "../../external/injector/include/ps2/patches.hpp"
extern "C" {
#include "../../external/injector/include/ps2/pcsx2f_api.h"
#include "../../external/injector/include/ps2/inireader.h"
#include "../../external/injector/include/ps2/patterns.h"
#include <cstdio>

int CompatibleCRCList[] = {0x4F32A11F};
char PluginData[MaxIniSize] = {1};
int PCSX2Data[PCSX2Data_Size] = {1};
char KeyboardState[StateNum][StateSize] = {{1}};
CMouseControllerState MouseState[StateNum] = {{1}};
char CheatString[CheatStringLen] = {1};
char OSDText[OSDStringNum][OSDStringSize] = {{1}};
char FrameLimitUnthrottle;
}

namespace vcs {
namespace {
safetymips::GameInline<uint64_t()> gameTick;
SafetyMipsMid littleWillie;
// 60 FPS: the VBlank interrupt handler (sub_370300) only runs its phase-2 frame
// work when a0 == 2 ("bne a0, v0" at 0x370314). NOP-ing that branch (HEAD's
// patch) runs it on every notification. The word is switched from the game
// thread (init / menu), so no plugin code runs inside the interrupt handler.
constexpr uintptr_t sixtyFPSSite = 0x370314;
constexpr uint32_t sixtyFPSOriginal = 0x14820019; // bne $a0, $v0, 0x37037C
pcsx2::Patch sixtyFPSPatch;

float DisplayAspect() {
    switch (PCSX2Data[PCSX2Data_AspectRatioSetting]) {
    case RAuto4_3_3_2: case R4_3: return 4.0f / 3.0f;
    case R16_9: return 16.0f / 9.0f;
    default: break;
    }
    int width = PCSX2Data[PCSX2Data_WindowSizeX];
    int height = PCSX2Data[PCSX2Data_WindowSizeY];
    if (PCSX2Data[PCSX2Data_IsFullscreen] || width <= 0 || height <= 0) {
        width = PCSX2Data[PCSX2Data_DesktopSizeX];
        height = PCSX2Data[PCSX2Data_DesktopSizeY];
    }
    return width > 0 && height > 0
        ? console::bounded(float(width) / float(height), 0.5f, 8.0f, 4.0f / 3.0f)
        : 4.0f / 3.0f;
}
void UpdateViewport() {
    settings.aspect = DisplayAspect();
    settings.baseAspect = settings.aspect >= 16.0f / 9.0f ? 16.0f / 9.0f : 4.0f / 3.0f;
}
uint64_t GameTick() {
    UpdateViewport();
    if (settings.pcControls) UpdateCheats();
    FrameLimitUnthrottle = false;
    if (settings.unthrottle) {
        auto menu = reinterpret_cast<void* (*)()>(0x471400)();
        bool menuActive = reinterpret_cast<int (*)(void*)>(0x3B5130)(menu) != 0;
        FrameLimitUnthrottle = *reinterpret_cast<const float*>(0x486E24) != 0.0f && !menuActive;
    }
    return gameTick.call();
}
void LittleWillieCamera(SafetyMipsContext& regs) {
    auto camera = static_cast<uintptr_t>(regs.s0);
    auto vehicle = *reinterpret_cast<const uint32_t*>(camera + 2040);
    auto distance = reinterpret_cast<float*>(camera + 1992);
    if (vehicle && *distance == 1.0f && *reinterpret_cast<const int16_t*>(vehicle + 86) == 0xAD) {
        *distance = 2.0f;
        regs.f1 = 2.0f;
    }
}
void Rejected(pcsx2_hook_status status) {
    std::snprintf(OSDText[0], OSDStringSize, "VCS fix disabled: patch validation failed (%u)", unsigned(status));
}
}
void ApplyFrameRate() {
    if (sixtyFPSPatch) (void)(settings.sixtyFPS ? sixtyFPSPatch.enable() : sixtyFPSPatch.disable());
}
}

extern "C" void init() {
    using namespace vcs;
    if (injector::InitializeCheckedRuntime(Rejected) != PCSX2_HOOK_OK) return;
    inireader.SetIniPath(PluginData + sizeof(uint32_t), *reinterpret_cast<const uint32_t*>(PluginData));
    settings.skipIntro = inireader.ReadInteger("MAIN", "SkipIntro", 1) != 0;
    settings.widescreen = inireader.ReadInteger("MAIN", "ImprovedWidescreenSupport", 0) != 0;
    settings.sixtyFPS = inireader.ReadInteger("MAIN", "Enable60FPS", 0) != 0;
    settings.unthrottle = inireader.ReadInteger("MAIN", "UnthrottleEmuDuringLoading", 1) != 0;
    settings.modernControls = inireader.ReadInteger("CONTROLS", "ModernControlScheme", 1) != 0;
    settings.pcControls = inireader.ReadInteger("CONTROLS", "PCControlScheme", 0) != 0;
    settings.hudScale = console::bounded(inireader.ReadFloat("HUD", "HudScale", 1.0f), 0.5f, 1.5f, 1.0f);
    settings.mouseSensitivity = console::bounded(inireader.ReadFloat("CONTROLS", "MouseSensitivity", 0.002f), 0.0001f, 0.02f, 0.002f);
    settings.invertMouse = inireader.ReadInteger("CONTROLS", "InvertMouseY", 0) != 0;
    settings.cutsceneBorders = inireader.ReadInteger("DISPLAY", "CutsceneBorders", 1) != 0;
    LoadBindings();
    UpdateViewport();

    if (settings.skipIntro) {
        auto site = pattern.get_first("00 00 00 00 ? ? ? ? 2D 20 00 00 ? ? ? ? 00 00 00 00 ? ? ? ? 00 00 00 00 2D 10 00", -4);
        if (site) injector::MakeNOP(site);
    }
    if (settings.widescreen) {
        InstallCamera();
        InstallDrawing();
        InstallHud();
    }
    InstallControls();
    InstallAim();
    InstallMenu();
    gameTick = safetymips::create_inline_game(0x21E950, GameTick);
    littleWillie = safetymips::create_mid<&LittleWillieCamera>(0x37281C);
    if (injector::FlushCaches() == PCSX2_HOOK_OK) {
        FrameLimitUnthrottle = settings.unthrottle;
        static constexpr uint32_t nop = 0;
        if (*reinterpret_cast<const volatile uint32_t*>(sixtyFPSSite) == sixtyFPSOriginal &&
            sixtyFPSPatch.create(injector::detail::backend, sixtyFPSSite, &nop, 1) == PCSX2_HOOK_OK)
            ApplyFrameRate();
    }
}

extern "C" int main() { return 0; }
