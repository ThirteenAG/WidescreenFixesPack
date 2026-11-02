#include "Game.hpp"
#include "Controls.hpp"
#include "../Shared/Console/PS2Display.hpp"
#include "../Shared/Console/Ps2PluginConflict.hpp"
#include "../../external/injector/include/ps2/patches.hpp"
#include <cstdio>
#include "Settings.hpp"
PCSX2F_CFG_SETTINGS();
PCSX2F_CFG_PEER_SETTINGS();
PCSX2F_STATE_LOAD_GENERATION();
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
pcsx2::Patch sixtyFPSPatch;
void BlurFailure(pcsx2_hook_status status)
{
    static constexpr const char* languages[] = { "en", "fr", "de", "it", "es" };
    static constexpr const char* messages[] = {
        "LCS blur patch failed (%u)", "LCS echec du filtre de flou (%u)",
        "LCS Unschaerfe-Patch fehlgeschlagen (%u)", "LCS filtro sfocatura non riuscito (%u)",
        "LCS fallo del filtro de desenfoque (%u)"
    };
    unsigned language = 0;
    for (unsigned i = 0; i < 5; ++i)
    {
        if (std::strcmp(config::values[9].value, languages[i]) == 0) language = i;
    }
    std::snprintf(OSDText[0], OSDStringSize, messages[language], unsigned(status));
}
}
void ApplyBlur() {
    if (blurPatch)
    {
        const auto status = settings.disableBlur ? blurPatch.enable() : blurPatch.disable();
        if (status != PCSX2_HOOK_OK) BlurFailure(status);
    }
}
void ApplyFrameRate() {
    if (sixtyFPSPatch)
    {
        const auto status = settings.sixtyFPS ? sixtyFPSPatch.enable() : sixtyFPSPatch.disable();
        if (status != PCSX2_HOOK_OK)
        {
            std::snprintf(OSDText[0], OSDStringSize, "LCS 60 FPS patch failed (%u)", unsigned(status));
        }
    }
}
void UpdateViewport() {
    settings.aspect = settings.forcedAspect > 0.0f ? settings.forcedAspect : console::ps2Aspect(PCSX2Data);
    settings.baseAspect = settings.aspect >= 16.0f / 9.0f ? 16.0f / 9.0f : 4.0f / 3.0f;
}
void ReadSettings() {
    settings.hudScale = config::Number(7, 1.0f, 0.5f, 1.5f);
    settings.sixtyFPS = config::Number(1, 30.0f, 30.0f, 60.0f) == 60.0f;
    settings.unthrottle = config::Boolean(2, true);
    settings.pcControls = config::Boolean(4, false);
    settings.mouseSensitivity = config::Number(5, 1.0f, 0.05f, 10.0f) * 0.002f;
    settings.invertMouse = config::Boolean(6, false);
    settings.cutsceneBorders = int(config::Number(8, 3.0f, 0.0f, 3.0f));
    settings.fovFactor = config::Number(10, 1.0f, 0.5f, 2.0f);
    settings.hudAspect = console::parse_hud_aspect(config::values[11].value);
    settings.forcedAspect = console::parse_forced_aspect(config::values[12].value);
    settings.disableBlur = config::Boolean(3, true);
    settings.skipIntro = config::Boolean(0, true);
    settings.gamepadIcons = int(config::Number(13, 0.0f, 0.0f, 3.0f));
    settings.vehicleLights = config::Boolean(14, true);
}
void RefreshSettings() {
    // Everything the menu changes applies live; Skip Intro keeps the value the
    // game started with.
    if (!config::Read()) return;
    ReadSettings();
    ReloadBindings();
    ReloadMenuLanguage();
    UpdateViewport();
    ApplyBlur();
    ApplyFrameRate();
}
namespace {
void Rejected(pcsx2_hook_status status) {
    std::snprintf(OSDText[0], OSDStringSize, "LCS fix disabled: patch validation failed (%u)", unsigned(status));
}
}
}
extern "C" void init() {
    using namespace lcs;
    static constexpr console::ps2conflict::OldPlugin oldPlugins[] = {
        {"GTALCS.PCSX2F.WidescreenFix.elf", "MAIN", "ForceAspectRatio"},
        {"GTALCS.PCSX2F.ImVehLM.elf", "MAIN", "VehicleLights"},
    };
    if (console::ps2conflict::Found(oldPlugins, OSDText)) return;
    if (injector::InitializeCheckedRuntime(Rejected) != PCSX2_HOOK_OK) return;
    config::Load();
    ReadSettings();
    UpdateViewport();
    LoadBindings();
    InstallAim();
    InstallControls();
    InstallMenu();
    settings.startedSkipIntro = settings.skipIntro;
    if (settings.skipIntro) injector::MakeNOP(0x1F35EC);
    InstallCamera();
    InstallDrawing();
    InstallHud();
    InstallFrontend();
    InstallVehicleLights();
    // The blur pass (sub_194178) calls its renderer at 0x1941A0 when the gate
    // gp-0x6094 is set; dropping the call (the delay slot only copies f12 to
    // f13) removes the blur. HEAD's NOPs at 0x194194 / 0x3D0E64 left it drawn.
    static constexpr uint32_t nop = 0;
    const auto blurStatus = blurPatch.create(injector::detail::backend, 0x1941A0, &nop, 1);
    if (blurStatus != PCSX2_HOOK_OK)
    {
        BlurFailure(blurStatus);
    }
    else
    {
        ApplyBlur();
    }
    const auto status = sixtyFPSPatch.create(injector::detail::backend, sixtyFPSSite, &nop, 1);
    if (status != PCSX2_HOOK_OK)
    {
        std::snprintf(OSDText[0], OSDStringSize, "LCS 60 FPS patch failed (%u)", unsigned(status));
    }
    else
    {
        ApplyFrameRate();
    }
    injector::FlushCaches();
}
extern "C" int main() { return 0; }
