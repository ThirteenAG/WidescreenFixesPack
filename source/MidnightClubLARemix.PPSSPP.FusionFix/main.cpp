#include "../Shared/Console/PatchNotice.hpp"
#include "Game.hpp"
#include "../Shared/Console/PortableCfg.hpp"
extern "C" { PSP_MODULE_INFO("MCLARemix.FusionFix", PSP_MODULE_USER, 2, 0); }
namespace mc3 {
namespace {
int Install() {
    if (!BeginWithNotice()) return -1;
    IniValue defaults[] = {{"MAIN", "DualAnalogPatch", "1"}, {"MAIN", "ForceAspectRatio", "auto"},
                           {"MAIN", "FixHUD", "1"}, {"MAIN", "HudScale", "1.0"}};
    CompleteCfg("ms0:/PSP/PLUGINS/MidnightClubLARemix.PPSSPP.FusionFix/MidnightClubLARemix.PPSSPP.FusionFix.cfg", defaults);
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
    logger.SetPath("ms0:/PSP/PLUGINS/MidnightClubLARemix.PPSSPP.FusionFix/MidnightClubLARemix.PPSSPP.FusionFix.log");
    if (!mc3::Start("MC3", "ms0:/PSP/PLUGINS/MidnightClubLARemix.PPSSPP.FusionFix/MidnightClubLARemix.PPSSPP.FusionFix.cfg",
        "ms0:/PSP/PLUGINS/MidnightClubLARemix.PPSSPP.FusionFix/MidnightClubLARemix.PPSSPP.FusionFix.log")) return 0;
    return mc3::Install();
}
