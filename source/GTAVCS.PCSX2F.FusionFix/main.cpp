#include "Game.hpp"
#include "Controls.hpp"
#include "Settings.hpp"
#include "../Shared/Console/Ps2PluginConflict.hpp"
PCSX2F_CFG_SETTINGS();
PCSX2F_CFG_PEER_SETTINGS();
PCSX2F_STATE_LOAD_GENERATION();
#include "../../external/injector/include/ps2/patches.hpp"
extern "C" {
#include "../../external/injector/include/ps2/pcsx2f_api.h"
#include "../../external/injector/include/ps2/patterns.h"
#include <cstdio>

int CompatibleCRCList[] = {0x4F32A11F};
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
    settings.aspect = settings.forcedAspect > 0.0f ? settings.forcedAspect : DisplayAspect();
    settings.baseAspect = settings.aspect >= 16.0f / 9.0f ? 16.0f / 9.0f : 4.0f / 3.0f;
}
void ReadSettings() {
    settings.skipIntro = config::Boolean(0, true);
    settings.sixtyFPS = config::Number(1, 30.0f, 30.0f, 60.0f) == 60.0f;
    settings.unthrottle = config::Boolean(2, true);
    settings.modernControls = config::Boolean(3, true);
    settings.pcControls = config::Boolean(4, false);
    settings.hudScale = config::Number(7, 1.0f, 0.5f, 1.5f);
    settings.mouseSensitivity = config::Number(5, 1.0f, 0.05f, 10.0f) * 0.002f;
    settings.invertMouse = config::Boolean(6, false);
    settings.cutsceneBorders = int(config::Number(8, 3.0f, 0.0f, 3.0f));
    settings.fovFactor = config::Number(10, 1.0f, 0.5f, 2.0f);
    settings.hudAspect = console::parse_hud_aspect(config::values[11].value);
    settings.forcedAspect = console::parse_forced_aspect(config::values[12].value);
    settings.gamepadIcons = int(config::Number(13, 0.0f, 0.0f, 3.0f));
    settings.vehicleLights = config::Boolean(14, true);
}
console::ps2settings::StateLoads stateLoads;
void RefreshSettings() {
    // After loading a save state: the CFG wins over the restored values. Skip
    // Intro keeps the value the game started with.
    if (!config::Read()) return;
    ReadSettings();
    ReloadBindings();
    ReloadMenuLanguage();
    ApplyFrameRate();
    ApplyPcControls();
}
uint64_t GameTick() {
    stateLoads.poll(RefreshSettings);
    UpdateViewport();
    if (KeyboardActive()) UpdateCheats();
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
    if (sixtyFPSPatch)
    {
        const auto status = settings.sixtyFPS ? sixtyFPSPatch.enable() : sixtyFPSPatch.disable();
        if (status != PCSX2_HOOK_OK)
        {
            std::snprintf(OSDText[0], OSDStringSize, "VCS 60 FPS patch failed (%u)", unsigned(status));
        }
    }
}
}

extern "C" void init() {
    using namespace vcs;
    static constexpr console::ps2conflict::OldPlugin oldPlugins[] = {
        {"GTAVCS.PCSX2F.WidescreenFix.elf", "MAIN", "ForceAspectRatio"},
        {"GTAVCS.PCSX2F.ImVehLM.elf", "MAIN", "VehicleLights"},
    };
    if (console::ps2conflict::Found(oldPlugins, OSDText)) return;
    if (injector::InitializeCheckedRuntime(Rejected) != PCSX2_HOOK_OK) return;
    config::Load();
    ReadSettings();
    settings.startedSkipIntro = settings.skipIntro;
    LoadBindings();
    UpdateViewport();

    if (settings.skipIntro) {
        auto site = pattern.get_first("00 00 00 00 ? ? ? ? 2D 20 00 00 ? ? ? ? 00 00 00 00 ? ? ? ? 00 00 00 00 2D 10 00", -4);
        if (site)
        {
            injector::MakeNOP(site);
        }
        else
        {
            std::snprintf(OSDText[0], OSDStringSize, "VCS Skip Intro patch not found");
        }
    }
    InstallCamera();
    InstallDrawing();
    InstallHud();
    InstallControls();
    InstallAim();
    InstallMenu();
    InstallVehicleLights();
    gameTick = safetymips::create_inline_game(0x21E950, GameTick);
    littleWillie = safetymips::create_mid<&LittleWillieCamera>(0x37281C);
    if (injector::FlushCaches() == PCSX2_HOOK_OK) {
        FrameLimitUnthrottle = settings.unthrottle;
        static constexpr uint32_t nop = 0;
        const auto status = sixtyFPSPatch.create(injector::detail::backend, sixtyFPSSite, &nop, 1);
        if (status == PCSX2_HOOK_OK)
        {
            ApplyFrameRate();
        }
        else
        {
            std::snprintf(OSDText[0], OSDStringSize, "VCS 60 FPS patch failed (%u)", unsigned(status));
        }
    }
}

extern "C" int main() { return 0; }
