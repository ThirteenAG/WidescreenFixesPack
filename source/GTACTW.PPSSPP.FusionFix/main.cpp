#include "../Shared/Console/PatchNotice.hpp"
#include "Game.hpp"
#include "../Shared/Console/PortableCfg.hpp"
extern "C" { PSP_MODULE_INFO("GTACTW.FusionFix", PSP_MODULE_USER, 2, 0); }
namespace ctw {
namespace {
SafetyMipsInline radio;
SafetyMipsMid loading, loaded;
injector::hook_back<void()> boot;
SceIoStat observedCfg{};
unsigned lastSettingsPoll;
bool cfgObserved;
int Radio() { return 1; }
void Boot() { boot.fun(); Unthrottle(false); }
// Settings that apply without a restart.
void ReadSettings() {
    ReadAspectSettings();
    ReadHudSettings();
    modernControls = Option("ModernControls", true);
    fastLoading = Option("UnthrottleEmuDuringLoading", true);
    if (!fastLoading) Unthrottle(false);
    char icons[16]{};
    SelectIcons(inireader.ReadString("MAIN", "GamepadIcons", "Auto", icons, sizeof(icons)));
}
int Install() {
    if (!BeginWithNotice()) return -1;
    if (!InitializeAddresses())
    {
        PatchNotice("Fusion Fix: unsupported game code; the fix is not installed. Use a supported game version.\n");
        return -1;
    }
    const bool radioEnabled = Option("RadioInAllVehicles", true);
    constexpr auto radioCyclePattern = "01 00 A5 24 2A 20 85 00";
    uintptr_t radioCycle = 0;
    if (radioEnabled)
    {
        radioCycle = pattern.get_first(radioCyclePattern, 8);
        if (!radioCycle || pattern.get(1, radioCyclePattern, 8) ||
            pattern.text_size < 20 || radioCycle < pattern.text_addr ||
            radioCycle - pattern.text_addr > pattern.text_size - 20)
        {
            radioCycle = 0;
            PatchRejected(PSP_HOOK_UNSUPPORTED_INSTRUCTION);
        }
    }
    // Keep movie setup and the intervening loading/notice initialization. The
    // native completion call can stop the startup movie as soon as loading ends.
    if (Option("SkipIntro", true))
        injector::WriteMemory<uint32_t>(Address<0x088D0D4C>(), 0x34050001u); // li a1, 1
    InstallAspect();
#ifndef NDEBUG
    logger.WriteF("Aspect patches: %u", unsigned(injector::detail::startup.error));
#endif
    InstallInput();
    InstallCamera();
    InstallReplay();
    if (InstallHud()) InstallVideo();
#ifndef NDEBUG
    logger.WriteF("HUD patches: %u", unsigned(injector::detail::startup.error));
#endif
    InstallIcons();
    ReadSettings();
    if (radioCycle) {
        radio = safetymips::create_inline(Address<0x0884F52C>(), Radio);
        // Remove the two conditional skip tests, retaining their delay slots.
        injector::MakeNOP(radioCycle); injector::MakeNOP(radioCycle + 16);
    }
    boot.fun = injector::MakeCALL(Address<0x088CFC1C>(), Boot).get();
    // Run before each virtual loading notification. The game still calls
    // its original method with its own this adjustment and return value.
    loading = safetymips::create_mid(Address<0x08992490>(), [](SafetyMipsContext&) { if (fastLoading) Unthrottle(true); });
    loaded = safetymips::create_mid(Address<0x089924B0>(), [](SafetyMipsContext&) { Unthrottle(false); });
    const int result = Finish();
    if (result == 0 && fastLoading) Unthrottle(true);
    return result;
}
}
// Changes from FusionFixHub or the website apply during gameplay: the cfg is
// read again when it changes, but not while the PDA or a menu is open.
void PollSettings() {
    const unsigned now = sceKernelGetSystemTimeLow();
    if (now - lastSettingsPoll < 1000000u) return;
    lastSettingsPoll = now;
    if (!GameplayApp()) return;
    SceIoStat current{};
    if (sceIoGetstat(cfgPath, &current) < 0 || current.st_size <= 0) return;
    if (cfgObserved && current.st_size == observedCfg.st_size &&
        !std::memcmp(&current.sce_st_mtime, &observedCfg.sce_st_mtime, sizeof(current.sce_st_mtime))) return;
    const bool first = !cfgObserved;
    observedCfg = current;
    cfgObserved = true;
    if (first) return;
    inireader.SetIniPath(cfgPath);
    ReadSettings();
}
}
extern "C" int module_start(SceSize, void*) {
    if (!console::portable::StartReported("CTW", ctw::cfgPath,
                   "ms0:/PSP/PLUGINS/GTACTW.PPSSPP.FusionFix/GTACTW.PPSSPP.FusionFix.log")) return 0;
    // Only CFG is opened; the old fix INI is never read or migrated.
    console::portable::IniValue values[] = {
        {"MAIN", "SkipIntro", "1"},
        {"MAIN", "ForceAspectRatio", "Auto"},
        {"MAIN", "ModernControls", "1"},
        {"MAIN", "GamepadIcons", "Auto"},
        {"MAIN", "Enable3rdPersonCamera", "1"},
        {"MAIN", "RadioInAllVehicles", "1"},
        {"MAIN", "UnthrottleEmuDuringLoading", "1"},
        {"MAIN", "UnlockXinMissionsInReplayBoard", "0"},
        {"MAIN", "HudScale", "1.0"},
        {"MAIN", "HudAspectRatioConstraint", "Auto"},
        {"MAIN", "FOVFactor", "1.0"},
        {"MAIN", "CutsceneBorders", "3"},
    };
    (void)console::portable::CompleteCfg(ctw::cfgPath, values);
    return ctw::Install();
}
