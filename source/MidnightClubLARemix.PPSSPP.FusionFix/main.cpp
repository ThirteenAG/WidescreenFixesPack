#include "Game.hpp"
extern "C" { PSP_MODULE_INFO("MCLARemix.FusionFix", PSP_MODULE_USER, 2, 0); }
namespace mc3 {
namespace {
int Install() {
    if (!Begin()) return -1;
    InstallAspect();
    InstallHud();
    InstallCamera();
#ifndef NDEBUG
    logger.WriteF("Patches: %u aspect %f", unsigned(injector::detail::startup.error), double(targetAspect));
#endif
    return Finish();
}
}
}
extern "C" int module_start(SceSize, void*) {
    if (!mc3::Start("MC3", "ms0:/PSP/PLUGINS/MidnightClubLARemix.PPSSPP.FusionFix/MidnightClubLARemix.PPSSPP.FusionFix.ini",
        "ms0:/PSP/PLUGINS/MidnightClubLARemix.PPSSPP.FusionFix/MidnightClubLARemix.PPSSPP.FusionFix.log")) return 0;
    return mc3::Install();
}
