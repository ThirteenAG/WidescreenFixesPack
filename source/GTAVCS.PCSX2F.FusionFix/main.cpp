#include "Game.hpp"
#include "Controls.hpp"
#include "Settings.hpp"
#include "Shared/Console/Ps2PluginConflict.hpp"
#include "Shared/Console/PS2Display.hpp"
#include "Shared/Console/Ps2TypedCheats.hpp"
PCSX2F_CFG_SETTINGS();
PCSX2F_CFG_PEER_SETTINGS();
PCSX2F_STATE_LOAD_GENERATION();
PCSX2F_EFFECTIVE_ASPECT()
#include "ps2/patches.hpp"
extern "C" {
#include "ps2/pcsx2f_api.h"
#include <cstdio>

int CompatibleCRCList[] = {0x4F32A11F};
int PCSX2Data[PCSX2Data_Size] = {1};
char KeyboardState[StateNum][StateSize] = {{1}};
CMouseControllerState MouseState[StateNum] = {{1}};
console::MouseMotion MouseStateV2{};
console::MouseTransitions MouseStateV3{};
char CheatString[CheatStringLen] = {1};
console::CheatEvents CheatStringV2{};
char OSDText[OSDStringNum][OSDStringSize] = {{1}};
char FrameLimitUnthrottle;
}

namespace vcs {
namespace {
safetymips::GameInline<uint64_t()> gameTick;
SafetyMipsMid littleWillie;
void UpdateViewport() {
    settings.aspect = settings.forcedAspect > 0.0f ? settings.forcedAspect : console::ps2Aspect(PCSX2Data);
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
    settings.holdVehicleCamera = config::Boolean(15, false);
    settings.padCameraSpeed = config::Number(16, 1.0f, 0.25f, 4.0f);
    settings.invertPad = config::Boolean(17, false);
}
bool RefreshSettings()
{
    if (!config::Read() || !ReloadBindings())
        return false;

    ReadSettings();
    ReloadMenuLanguage();
    ApplyFrameRate();
    return true;
}
struct SettingsRefresh
{
    uint32_t appliedGeneration = 0;
    uint32_t attemptedGeneration = 0;
    unsigned retryDelay = 0;

    void Poll()
    {
        const auto generation = *reinterpret_cast<const volatile uint32_t*>(&PCSX2FStateLoadGeneration);
        if (generation == appliedGeneration)
            return;
        if (generation == attemptedGeneration && retryDelay)
        {
            --retryDelay;
            return;
        }

        attemptedGeneration = generation;
        if (RefreshSettings())
        {
            appliedGeneration = generation;
            retryDelay = 0;
        }
        else
        {
            // Retry on the 60th subsequent game tick without waiting for another load.
            retryDelay = 59;
        }
    }
};
SettingsRefresh stateLoads;

uint64_t GameTick() {
    stateLoads.Poll();
    UpdateViewport();
    TickTiming();
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
pcsx2::Patch introPatch;
pcsx2_hook_status InstallSkipIntro(const pcsx2_hook_backend& backend)
{
    settings.startedSkipIntro = false;
    if (!settings.skipIntro)
        return PCSX2_HOOK_OK;

    // The startup call plays the logo and intro movies before opening the game.
    constexpr uintptr_t site = 0x21C8F0;
    constexpr uint32_t originalCall = 0x0C08729C;
    constexpr uint32_t nop = 0;
    uint32_t original = 0;
    if (!backend.read(backend.user, site, &original, sizeof(original)))
        return PCSX2_HOOK_READ_FAILED;
    if (original != originalCall)
        return PCSX2_HOOK_CONFLICT;

    const auto created = introPatch.create(backend, site, &nop, 1);
    if (created != PCSX2_HOOK_OK)
        return created;
    const auto enabled = introPatch.enable();
    settings.startedSkipIntro = introPatch.enabled();
    return enabled;
}

void Rejected(pcsx2_hook_status status) {
    std::snprintf(OSDText[0], OSDStringSize, "VCS fix disabled: patch validation failed (%u)", unsigned(status));
}
}
}

extern "C" void init() {
    using namespace vcs;
    static constexpr const char* oldPlugins[] = {
        "GTAVCS.PCSX2F.WidescreenFix.elf",
        "GTAVCS.PCSX2F.ImVehLM.elf",
    };
    if (console::ps2conflict::Found(oldPlugins, OSDText)) return;
    if (injector::InitializeCheckedRuntime(Rejected) != PCSX2_HOOK_OK) return;
    config::Load();
    ReadSettings();
    LoadBindings();
    UpdateViewport();

    const auto introStatus = InstallSkipIntro(injector::detail::backend);
    if (introStatus != PCSX2_HOOK_OK)
        std::snprintf(OSDText[0], OSDStringSize, "VCS Skip Intro patch failed (%u)", unsigned(introStatus));
    InstallCamera();
    InstallDrawing();
    InstallHud();
    InstallControls();
    InstallAim();
    InstallMenu();
    InstallVehicleLights();
    gameTick = safetymips::create_inline_game(0x21E950, GameTick);
    littleWillie = safetymips::create_mid<&LittleWillieCamera>(0x37281C);
    InstallTiming();
    if (injector::FlushCaches() == PCSX2_HOOK_OK) FrameLimitUnthrottle = settings.unthrottle;
}

extern "C" int main() { return 0; }
