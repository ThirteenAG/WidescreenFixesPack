#include "Game.hpp"
extern "C" { PSP_MODULE_INFO("GTACTW.FusionFix", PSP_MODULE_USER, 2, 0); }
namespace ctw {
namespace {
SafetyMipsInline radio;
SafetyMipsMid loading, loaded;
injector::hook_back<void()> boot;
int Radio() { return 1; }
void Boot() { boot.fun(); Unthrottle(false); }
int Install() {
    if (!Begin()) return -1;
    if (!InitializeAddresses()) return -1;
    InstallAspect();
#ifndef NDEBUG
    logger.WriteF("Aspect patches: %u", unsigned(injector::detail::startup.error));
#endif
    InstallInput();
    InstallCamera();
    InstallReplay();
    InstallHud();
    InstallVideo();
#ifndef NDEBUG
    logger.WriteF("HUD patches: %u", unsigned(injector::detail::startup.error));
#endif
    if (inireader.ReadInteger("MAIN", "RadioInAllVehicles", 1)) {
        radio = safetymips::create_inline(Address<0x0884F52C>(), Radio);
        // Remove the two conditional skip tests, retaining their delay slots.
        const auto cycle = pattern.get_first("01 00 A5 24 2A 20 85 00", 8);
        injector::MakeNOP(cycle); injector::MakeNOP(cycle + 16);
    }
    if (inireader.ReadInteger("MAIN", "UnthrottleEmuDuringLoading", 1)) {
        boot.fun = injector::MakeCALL(Address<0x088CFC1C>(), Boot).get();
        // Run before each virtual loading notification. The game still calls
        // its original method with its own this adjustment and return value.
        loading = safetymips::create_mid(Address<0x08992490>(), [](SafetyMipsContext&) { Unthrottle(true); });
        loaded = safetymips::create_mid(Address<0x089924B0>(), [](SafetyMipsContext&) { Unthrottle(false); });
    }
    const int result = Finish();
    if (result == 0 && inireader.ReadInteger("MAIN", "UnthrottleEmuDuringLoading", 1)) Unthrottle(true);
    return result;
}
}
}
extern "C" int module_start(SceSize, void*) {
    if (!ctw::Start("CTW", "ms0:/PSP/PLUGINS/GTACTW.PPSSPP.FusionFix/GTACTW.PPSSPP.FusionFix.ini",
                   "ms0:/PSP/PLUGINS/GTACTW.PPSSPP.FusionFix/GTACTW.PPSSPP.FusionFix.log")) return 0;
    return ctw::Install();
}
