#include "Game.hpp"
#include "Addresses.hpp"
#include <cstdlib>
extern "C" {
#include <pspkernel.h>
}
PSP_MODULE_INFO("GTAVCS.PPSSPP.WidescreenFix",PSP_MODULE_USER,2,0);
namespace vcsws {
namespace {
void ReadSettings() {
    using console::bounded;
    settings.skipIntro=inireader.ReadInteger("MAIN","SkipIntro",1)!=0;
    settings.unthrottleMode=inireader.ReadInteger("MAIN","UnthrottleEmuDuringLoading",1);
    settings.unthrottle=settings.unthrottleMode!=0;
    settings.fps=inireader.ReadInteger("MAIN","Enable60FPS",0)!=0;
    settings.modernControls=inireader.ReadInteger("CONTROLS","ModernControlScheme",0)!=0;
    settings.pcCheats=inireader.ReadInteger("CONTROLS","PCCheats",0)!=0;
    float fov=inireader.ReadFloat("FOV","FOVFactor",1);
    settings.fov=fov>0 ? bounded(fov,0.1f,2.5f,1) : 1;
    settings.restoreCutsceneFov=inireader.ReadInteger("FOV","RestoreCutsceneFOV",1)!=0;
    settings.cutsceneBorders=inireader.ReadInteger("FOV","CutsceneBorders",1)!=0;
    auto& drawing=Drawing::settings;
    drawing.aspect=console::portable::Aspect();
    char ratio[64];inireader.ReadString("MAIN","ForceAspectRatio","auto",ratio,sizeof(ratio));
    char* separator=nullptr;float numerator=std::strtof(ratio,&separator);
    if (separator && *separator==':') {
        char* end=nullptr;float denominator=std::strtof(separator+1,&end);
        if (end && !*end && numerator>0 && denominator>0) {
            drawing.aspect=bounded(numerator/denominator,0.5f,8,drawing.aspect);autoAspect=false;
        }
    }
    float hud=inireader.ReadFloat("HUD","HudScale",1);
    drawing.hud=hud>0 ? bounded(hud,0.1f,2,1) : 1;
    float radar=inireader.ReadFloat("RADAR","RadarScale",1);
    drawing.radar=radar>0 ? bounded(radar,0.1f,2,1) : 1;
    drawing.radarX=bounded(inireader.ReadFloat("RADAR","RadarPosX",12),0,400,12);
    drawing.radarY=bounded(inireader.ReadFloat("RADAR","RadarPosY",196),0,272,196);
    drawing.multiplayerRadar=bounded(inireader.ReadFloat("MPRADAR","RadarScale",1),0.1f,2,1);
    drawing.multiplayerRadarY=bounded(inireader.ReadFloat("MPRADAR","RadarPosY",196.0f),0,272,196.0f);
}
}
}
extern "C" int module_start(SceSize,void*) {
    using namespace vcsws;
    if (!console::portable::Start("GTA3",iniPath,"ms0:/PSP/PLUGINS/GTAVCS.PPSSPP.WidescreenFix/GTAVCS.PPSSPP.WidescreenFix.log")) return -1;
    // Keep the emulator's required island-transition startup delay.
    sceKernelDelayThread(100000);
    if (!console::portable::Begin()) return -1;
    if (!InitializeAddresses()) return -1;
    ReadSettings();
    if (settings.skipIntro) injector::MakeNOP(Address<0x8936efc>());
    InstallCamera();
#ifndef NDEBUG
    logger.WriteF("Camera patches: %u", unsigned(injector::detail::startup.error));
#endif
    InstallDrawing();
#ifndef NDEBUG
    logger.WriteF("Drawing patches: %u", unsigned(injector::detail::startup.error));
#endif
    InstallControls();
#ifndef NDEBUG
    logger.WriteF("Control patches: %u", unsigned(injector::detail::startup.error));
#endif
    InstallTiming();
#ifndef NDEBUG
    logger.WriteF("Timing patches: %u", unsigned(injector::detail::startup.error));
#endif
    InstallMenu();
    InstallAim();
#ifndef NDEBUG
    logger.WriteF("Menu patches: %u", unsigned(injector::detail::startup.error));
    for (auto* entry = injector::detail::startup.first; entry; entry = entry->next)
        if (entry->kind == injector::detail::entry_kind::patch && entry->callback &&
            psp_hook_control(entry->value.patch.replacement[1]) != 0)
            logger.WriteF("Invalid call delay slot at %08X: %08X", entry->value.patch.address, entry->value.patch.replacement[1]);
#endif
    return console::portable::Finish();
}
