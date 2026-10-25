#include "Game.hpp"
#include "Addresses.hpp"
#include "../Shared/Console/PortableCfg.hpp"
#include <cstdlib>
#include <cstring>
extern "C" {
#include <pspkernel.h>
}
PSP_MODULE_INFO("GTALCS.PPSSPP.WidescreenFix",PSP_MODULE_USER,2,0);
namespace lcsws {
namespace {
void ReadSettings() {
    using console::bounded;
    ReloadTextLanguage();
    settings.skipIntro=inireader.ReadBoolean("MAIN","SkipIntro",1);
    settings.unthrottle=inireader.ReadBoolean("MAIN","UnthrottleEmuDuringLoading",1);
    settings.fps=inireader.ReadBoolean("MAIN","Enable60FPS",0);
    settings.modernControls=inireader.ReadBoolean("CONTROLS","ModernControlScheme",0);
    settings.pcCheats=inireader.ReadBoolean("CONTROLS","PCCheats",0);
    float fov=inireader.ReadFloat("MAIN","FOVFactor",1);
    settings.fov=fov>0 ? bounded(fov,0.5f,2.0f,1) : 1;
    settings.restoreCutsceneFov=inireader.ReadBoolean("FOV","RestoreCutsceneFOV",1);
    settings.cutsceneBorders=int(bounded(inireader.ReadFloat("MAIN","CutsceneBorders",3),0,3,3));
    auto& drawing=Drawing::settings;
    drawing.aspect=console::portable::Aspect();
    char ratioBuffer[64];const char* ratio=inireader.ReadString("MAIN","ForceAspectRatio","auto",ratioBuffer,sizeof(ratioBuffer));
    char* separator=nullptr;float numerator=std::strtof(ratio,&separator);
    if (separator && *separator==':') {
        char* end=nullptr;float denominator=std::strtof(separator+1,&end);
        if (end && !*end && numerator>0 && denominator>0) {
            drawing.aspect=bounded(numerator/denominator,0.5f,8,drawing.aspect);autoAspect=false;
        }
    }
    char hudAspect[32];
    drawing.hudAspect=console::parse_hud_aspect(inireader.ReadString("MAIN","HudAspectRatioConstraint","Auto",hudAspect,sizeof(hudAspect)));
    float hud=inireader.ReadFloat("MAIN","HudScale",0.75f);
    drawing.hud=hud>0 ? bounded(hud,0.1f,2,0.75f) : 0.75f;
    float radar=inireader.ReadFloat("RADAR","RadarScale",0.83f);
    drawing.radar=radar>0 ? bounded(radar,0.1f,2,0.83f) : 0.83f;
    drawing.radarX=bounded(inireader.ReadFloat("RADAR","RadarPosX",12),-480,480,12);
    drawing.radarY=bounded(inireader.ReadFloat("RADAR","RadarPosY",196),-272,272,196); // -45 puts the radar on top
    drawing.multiplayerRadar=bounded(inireader.ReadFloat("MPRADAR","RadarScale",0.63f),0.1f,2,0.63f);
    drawing.multiplayerRadarY=bounded(inireader.ReadFloat("MPRADAR","RadarPosY",170.0f),-272,272,170.0f);
}
}
void ReloadTextLanguage() {
    // Refresh the full native CFG buffer only at startup/menu reopening.
    inireader.SetIniPath(iniPath);
    char buffer[32]{};
    const char* value = inireader.ReadString("MAIN", "TextLanguage", "auto", buffer, sizeof(buffer));
    settings.textLanguage = value && !std::strcmp(value, "en") ? TextLanguage::English : TextLanguage::Auto;
}
}
extern "C" int module_start(SceSize,void*) {
    using namespace lcsws;
    if (!console::portable::StartReported("GTA3",iniPath,"ms0:/PSP/PLUGINS/GTALCS.PPSSPP.WidescreenFix/GTALCS.PPSSPP.WidescreenFix.log")) return -1;
    if (!console::portable::BeginReported()) return -1;
    if (!InitializeAddresses())
    {
        logger.Write("Fix disabled: unsupported game code. Use a supported game version.\n");
        return -1;
    }
    console::portable::IniValue defaults[] = {
        {"MAIN", "SkipIntro", "1"},
        {"MAIN", "TextLanguage", "auto"},
        {"MAIN", "ForceAspectRatio", "auto"},
        {"MAIN", "Enable60FPS", "0"},
        {"MAIN", "UnthrottleEmuDuringLoading", "1"},
        {"CONTROLS", "ModernControlScheme", "0"},
        {"CONTROLS", "PCCheats", "0"},
        {"MAIN", "HudScale", "0.75"},
        {"MAIN", "HudAspectRatioConstraint", "Auto"},
        {"RADAR", "RadarScale", "0.83"},
        {"RADAR", "RadarPosX", "12"},
        {"RADAR", "RadarPosY", "196"},
        {"MPRADAR", "RadarScale", "0.63"},
        {"MPRADAR", "RadarPosY", "170"},
        {"FOV", "RestoreCutsceneFOV", "1"},
        {"MAIN", "FOVFactor", "1"},
        {"MAIN", "CutsceneBorders", "3"},
    };
    console::portable::CompleteCfg(iniPath, defaults);
    ReadSettings();
    if (settings.skipIntro) injector::MakeNOP(Address<0x89c3814>());
    InstallCamera();InstallDrawing();InstallControls();InstallTiming();InstallMenu();InstallAim();
#ifndef NDEBUG
    logger.WriteF("Startup patches: %u", unsigned(injector::detail::startup.error));
#endif
    return console::portable::Finish();
}
