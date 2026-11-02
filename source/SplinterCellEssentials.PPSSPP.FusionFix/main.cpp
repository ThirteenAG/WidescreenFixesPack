#include "Screens.hpp"
#include "Shared/Console/PatchNotice.hpp"
#include "../Shared/Console/PortableCfg.hpp"
extern "C" {
PSP_MODULE_INFO("SplinterCellPSP.FusionFix", PSP_MODULE_USER, 2, 0);
// module_start runs above the game's threads (default 0x20), so the early
// hooks are published before the game's first loading screens are drawn.
// {count, priority, stack size (0 = default), attributes (0 = default)}
unsigned int module_start_thread_parameter[4] __attribute__((used)) = {3, 0x18, 0, 0};
}
namespace {
using namespace essentials;
constexpr auto ini = "ms0:/PSP/PLUGINS/SplinterCellEssentials.PPSSPP.FusionFix/SplinterCellEssentials.PPSSPP.FusionFix.cfg";
constexpr auto logPath = "ms0:/PSP/PLUGINS/SplinterCellEssentials.PPSSPP.FusionFix/SplinterCellEssentials.PPSSPP.FusionFix.log";
// The game's main thread already runs while plugins start, and it draws the
// loading picture and mission briefing right away. Publish those hooks before
// any file access (which lets the game thread run): they only search the
// start of the executable.
// PPSSPP loads a fresh plugin image for every LoadExec (loader, then game),
// so two "Loaded plugin" messages are expected. A duplicate plugin folder or
// ini would start a second copy in the same stage; it must not hook twice.
void CompleteSettings() {
    console::portable::IniValue defaults[] = {
        {"MAIN", "SkipIntro", "1"},
        {"MAIN", "StickDeadzone", "0.1"},
        {"MAIN", "SpeedStickControl", "1"},
        {"MAIN", "FPSLimit", "60"},
        {"MAIN", "UnthrottleEmuDuringLoading", "1"},
        {"MAIN", "FOVFactor", "1"},
        {"MAIN", "HudAspectRatioConstraint", "Auto"},
        {"MAIN", "HudScale", "1"},
        {"MAIN", "ForceAspectRatio", "Auto"}
    };
    console::portable::CompleteCfg(ini, defaults);
    char text[64]{};
    forced = console::parse_forced_aspect(inireader.ReadString("MAIN", "ForceAspectRatio", "Auto", text, sizeof(text)));
    hudConstraint = console::parse_hud_aspect(inireader.ReadString("MAIN", "HudAspectRatioConstraint", "Auto", text, sizeof(text)));
}

bool AlreadyRunning() {
    const SceUID self = sceKernelGetModuleIdByAddress(reinterpret_cast<const void*>(&AlreadyRunning));
    SceUID modules[64]; int count = 0;
    if (sceKernelGetModuleIdList(modules, sizeof(modules), &count) < 0) return false;
    if (count > 64) count = 64;
    for (int i = 0; i < count; ++i) {
        // Both copies may be loaded before either starts: the first loaded wins.
        if (modules[i] == self || modules[i] > self) continue;
        SceKernelModuleInfo info{}; info.size = sizeof(info);
        if (sceKernelQueryModuleInfo(modules[i], &info) >= 0 && !std::strcmp(info.name, "SplinterCellPSP.FusionFix")) return true;
    }
    return false;
}
bool EarlyGame() {
    if (!Emulator() || injector::InitializeRuntime(Rejected) != PSP_HOOK_OK) return false;
    SceKernelModuleInfo info{};
    if (!FindModule("SplinterCellPSP", info)) return false;
    SceKernelModuleInfo own{}; own.size = sizeof(own);
    if (sceKernelQueryModuleInfo(sceKernelGetModuleIdByAddress(reinterpret_cast<const void*>(&EarlyGame)), &own) >= 0)
        injector::SetModuleBaseAddress(own.text_addr, own.text_size);
    Attach(info);
    if (Begin()) { InstallLoadingScreens(); Finish(); }
    inireader.SetIniPath(ini);
    CompleteSettings();
#ifndef NDEBUG
    logger.SetPath(logPath);
    logger.WriteF("Early loading-screen hooks: %d", int(injector::LastError()));
#endif
    return true;
}
int Install() {
    if (!sites::DetectExecutable())
    {
        PatchRejected(PSP_HOOK_UNSUPPORTED_INSTRUCTION);
        return -1;
    }
    if (!Begin()) return -1;
    InstallInput();
    if (!InstallHud())
    {
        PatchRejected(PSP_HOOK_UNSUPPORTED_INSTRUCTION);
        return -1;
    }
    InstallCamera();
    InstallTiming();
    return Finish();
}
}
extern "C" int module_start(SceSize, void*) {
    if (AlreadyRunning()) return 1;
    if (EarlyGame()) return Install();
    // The loader performs LoadExec; PPSSPP loads a fresh plugin image for the
    // game. Leave its second movie call intact, as required by the original loader.
    if (essentials::Start("PSPLoader", ini, logPath) && essentials::Begin()) {
        CompleteSettings();
        if (inireader.ReadBoolean("MAIN", "SkipIntro", true))
        {
            // Identify the first movie by its position before the second one.
            const auto intro = sites::Unique("01 00 04 3C 25 28 00 00 25 30 00 00 25 38 00 00 ? ? ? ? ? ? 84 24 01 00 04 3C 25 28 00 00 25 30 00 00 25 38 00 00", 16);
            if (intro)
                injector::MakeNOP(intro);
            else
                PatchRejected(PSP_HOOK_UNSUPPORTED_INSTRUCTION);
        }
        essentials::InstallLoaderScreens();
        return essentials::Finish();
    }
    return 0;
}
