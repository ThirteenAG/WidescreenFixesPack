#include "Game.hpp"
#include "Addresses.hpp"
#include <cstdlib>
#include <cstring>
#include <cmath>
#include "../Shared/Console/PortableCfg.hpp"
extern "C" {
#include <pspkernel.h>
}
PSP_MODULE_INFO("GTAVCS.PPSSPP.WidescreenFix",PSP_MODULE_USER,2,0);
namespace vcsws {
namespace {
float ReadCfgFloat(const char* section, const char* key, float fallback) {
    char buffer[64];
    const char* text = inireader.ReadString(section, key, "", buffer, sizeof(buffer));
    char* end = nullptr;
    const float value = std::strtof(text, &end);
    if (end == text || !std::isfinite(value)) return fallback;
    const bool separated = *end == ' ' || *end == '\t';
    while (*end == ' ' || *end == '\t' || *end == '\r') ++end;
    return *end && !(separated && *end == ';') ? fallback : value;
}
void ReadSettings() {
    using console::bounded;
    ReloadTextLanguage();
    settings.skipIntro=inireader.ReadBoolean("MAIN","SkipIntro",1);
    settings.unthrottleMode=inireader.ReadInteger("MAIN","UnthrottleEmuDuringLoading",1);
    if (settings.unthrottleMode < 0 || settings.unthrottleMode > 2) settings.unthrottleMode = 1;
    settings.unthrottle=settings.unthrottleMode!=0;
    settings.fps=inireader.ReadBoolean("MAIN","Enable60FPS",0);
    settings.modernControls=inireader.ReadBoolean("CONTROLS","ModernControlScheme",0);
    settings.pcCheats=inireader.ReadBoolean("CONTROLS","PCCheats",0);
    settings.SetFovRequest(ReadCfgFloat("MAIN","FOVFactor",1));
    settings.restoreCutsceneFov=inireader.ReadBoolean("FOV","RestoreCutsceneFOV",1);
    settings.cutsceneBorders=int(bounded(ReadCfgFloat("MAIN","CutsceneBorders",3),0,3,3));
    auto& drawing=Drawing::settings;
    drawing.aspect=console::portable::Aspect();
    char ratioBuffer[64];const char* ratio=inireader.ReadString("MAIN","ForceAspectRatio","auto",ratioBuffer,sizeof(ratioBuffer));
    char* separator=nullptr;float numerator=std::strtof(ratio,&separator);
    if (separator && *separator==':') {
        char* end=nullptr;float denominator=std::strtof(separator+1,&end);
        if (end && !*end && std::isfinite(numerator) && std::isfinite(denominator) && numerator>0 && denominator>0) {
            drawing.aspect=bounded(numerator/denominator,0.5f,8,drawing.aspect);autoAspect=false;
        }
    }
    char hudAspect[32];
    drawing.hudAspect=console::parse_hud_aspect(inireader.ReadString("MAIN","HudAspectRatioConstraint","Auto",hudAspect,sizeof(hudAspect)));
    float hud=ReadCfgFloat("MAIN","HudScale",0.75f);
    drawing.hud=hud>0 ? bounded(hud,0.1f,2,0.75f) : 0.75f;
    float radar=ReadCfgFloat("RADAR","RadarScale",0.83f);
    drawing.radar=radar>0 ? bounded(radar,0.1f,2,0.83f) : 0.83f;
    drawing.radarX=bounded(ReadCfgFloat("RADAR","RadarPosX",12),-480,480,12);
    drawing.radarY=bounded(ReadCfgFloat("RADAR","RadarPosY",196),-272,272,196);
    drawing.multiplayerRadar=bounded(ReadCfgFloat("MPRADAR","RadarScale",0.63f),0.1f,2,0.63f);
    drawing.multiplayerRadarY=bounded(ReadCfgFloat("MPRADAR","RadarPosY",170.0f),-272,272,170.0f);
}
}
void ReloadTextLanguage() {
    // Refresh the full native CFG buffer only at startup/menu reopening.
    inireader.SetIniPath(cfgPath);
    char buffer[32]{};
    const char* value = inireader.ReadString("MAIN", "TextLanguage", "auto", buffer, sizeof(buffer));
    settings.textLanguage = value && !std::strcmp(value, "en") ? TextLanguage::English : TextLanguage::Auto;
}
}
extern "C" int module_start(SceSize,void*) {
    using namespace vcsws;
    if (!console::portable::StartReported("GTA3",cfgPath,"ms0:/PSP/PLUGINS/GTAVCS.PPSSPP.WidescreenFix/GTAVCS.PPSSPP.WidescreenFix.log")) return -1;
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
        {"RADAR", "RadarPosX", "12.0"},
        {"RADAR", "RadarPosY", "196.0"},
        {"MPRADAR", "RadarScale", "0.63"},
        {"MPRADAR", "RadarPosY", "170.0"},
        {"FOV", "RestoreCutsceneFOV", "1"},
        {"MAIN", "FOVFactor", "1.0"},
        {"MAIN", "CutsceneBorders", "3"},
    };
    (void)console::portable::CompleteCfg(cfgPath, defaults);
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
