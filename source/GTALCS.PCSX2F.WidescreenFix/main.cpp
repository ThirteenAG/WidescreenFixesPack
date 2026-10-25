#include "Game.hpp"
#include "Controls.hpp"
#include "../Shared/Console/PS2Display.hpp"
#include "../../external/injector/include/ps2/patches.hpp"
#include <cstdio>
#include "Settings.hpp"
PCSX2F_CFG_SETTINGS();
extern "C" {
int CompatibleCRCList[] = {static_cast<int>(0x7EA439F5)};
int PCSX2Data[PCSX2Data_Size] = {1};
char OSDText[OSDStringNum][OSDStringSize] = {{1}};
char KeyboardState[StateNum][StateSize] = {{1}};
CMouseControllerState MouseState[StateNum] = {{1}};
char FrameLimitUnthrottle;
}
namespace lcs {
namespace {
pcsx2::Patch blurPatch;
// 60 FPS: the VBlank interrupt handler (sub_2D92B0) only runs its phase-2 frame
// work when a0 == 2 ("bne a0, v0" at 0x2D92C4). NOP-ing that branch (HEAD's
// patch) runs it on every notification. The word is switched from the game
// thread (init / menu), so no plugin code runs inside the interrupt handler.
constexpr uintptr_t sixtyFPSSite = 0x2D92C4;
constexpr uint32_t sixtyFPSOriginal = 0x14820017; // bne $a0, $v0, 0x2D9324
pcsx2::Patch sixtyFPSPatch;
}
void ApplyBlur() {
    if (blurPatch) (void)(settings.disableBlur ? blurPatch.enable() : blurPatch.disable());
}
void ApplyFrameRate() {
    if (sixtyFPSPatch) (void)(settings.sixtyFPS ? sixtyFPSPatch.enable() : sixtyFPSPatch.disable());
}
void UpdateViewport() {
    settings.aspect = console::ps2Aspect(PCSX2Data);
    settings.baseAspect = settings.aspect >= 16.0f / 9.0f ? 16.0f / 9.0f : 4.0f / 3.0f;
}
namespace {
void Rejected(pcsx2_hook_status status) {
    std::snprintf(OSDText[0], OSDStringSize, "LCS fix disabled: patch validation failed (%u)", unsigned(status));
}
}
}
extern "C" void init() {
    using namespace lcs;
    if (injector::InitializeCheckedRuntime(Rejected) != PCSX2_HOOK_OK) return;
    config::Load();
    UpdateViewport();
    settings.hudScale = config::Number(8, 1.0f, 0.5f, 1.5f);
    settings.widescreen = config::Boolean(1, true);
    settings.sixtyFPS = config::Boolean(2, false);
    settings.unthrottle = config::Boolean(3, true);
    settings.pcControls = config::Boolean(5, false);
    settings.mouseSensitivity = config::Number(6, 1.0f, 0.05f, 10.0f) * 0.002f;
    settings.invertMouse = config::Boolean(7, false);
    settings.cutsceneBorders = int(config::Number(9, 3.0f, 0.0f, 3.0f));
    settings.fovFactor = config::Number(11, 1.0f, 0.5f, 2.0f);
    settings.hudAspect = console::parse_hud_aspect(config::values[12].value);
    settings.disableBlur = config::Boolean(4, true);
    LoadBindings();
    InstallAim();
    InstallControls();
    InstallMenu();
    if (config::Boolean(0, true)) injector::MakeNOP(0x1F35EC);
    if (settings.widescreen) {
        InstallCamera();
        InstallDrawing();
        InstallHud();
        InstallFrontend();
    }
    // The blur pass (sub_194178) calls its renderer at 0x1941A0 when the gate
    // gp-0x6094 is set; dropping the call (the delay slot only copies f12 to
    // f13) removes the blur. HEAD's NOPs at 0x194194 / 0x3D0E64 left it drawn.
    static constexpr uint32_t nop = 0;
    (void)blurPatch.create(injector::detail::backend, 0x1941A0, &nop, 1);
    ApplyBlur();
    if (*reinterpret_cast<const volatile uint32_t*>(sixtyFPSSite) == sixtyFPSOriginal &&
        sixtyFPSPatch.create(injector::detail::backend, sixtyFPSSite, &nop, 1) == PCSX2_HOOK_OK)
        ApplyFrameRate();
    injector::FlushCaches();
}
extern "C" int main() { return 0; }
