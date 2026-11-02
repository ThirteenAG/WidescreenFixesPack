#include "Game.hpp"
#include "Addresses.hpp"
#include "AspectRatio.hpp"
#include "../Shared/Console/PortableCfg.hpp"
#include "../Shared/Console/PspPluginConflict.hpp"
#include <cstdlib>
#include <cstring>
extern "C" {
#include <pspkernel.h>
#include <pspiofilemgr.h>
}
PSP_MODULE_INFO("GTALCS.PPSSPP.FusionFix",PSP_MODULE_USER,2,0);
namespace lcsws {
namespace {
SceIoStat observedCfg{};
unsigned lastSettingsPoll;
bool cfgObserved;
void ReadSettings() {
    using console::bounded;
    ReloadTextLanguage();
    settings.skipIntro=inireader.ReadBoolean("MAIN","SkipIntro",1);
    settings.unthrottle=inireader.ReadBoolean("MAIN","UnthrottleEmuDuringLoading",1);
    settings.fps=inireader.ReadInteger("MAIN","FPSLimit",30)==60;
    settings.modernControls=inireader.ReadBoolean("CONTROLS","ModernControlScheme",0);
    settings.pcCheats=inireader.ReadBoolean("CONTROLS","PCCheats",0);
    settings.vehicleLights=inireader.ReadBoolean("MAIN","VehicleLights",1);
    float fov=inireader.ReadFloat("MAIN","FOVFactor",1);
    settings.fov=fov>0 ? bounded(fov,0.5f,2.0f,1) : 1;
    settings.cutsceneBorders=int(bounded(inireader.ReadFloat("MAIN","CutsceneBorders",3),0,3,3));
    settings.movieMode=int(bounded(inireader.ReadFloat("MAIN","FMVWidescreenMode",1),0,1,1));
    char icons[16]{};
    const char* selectedIcons = inireader.ReadString("MAIN", "GamepadIcons", "Auto", icons, sizeof(icons));
    settings.icons = 8;
    for (unsigned i = 0; i < 9; ++i)
        if (!std::strcmp(selectedIcons, iconSets[i])) { settings.icons = int(i); break; }
    auto& drawing=Drawing::settings;
    drawing.aspect=console::portable::Aspect();
    char ratioBuffer[64];const char* ratio=inireader.ReadString("MAIN","ForceAspectRatio","auto",ratioBuffer,sizeof(ratioBuffer));
    autoAspect = true;
    float forcedAspect = 0.0f;
    if (ParseAspectRatio(ratio, forcedAspect))
    {
        drawing.aspect = forcedAspect;
        autoAspect = false;
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
void PollSettings()
{
    const unsigned now = sceKernelGetSystemTimeLow();
    TickIcons(now);
    if (now - lastSettingsPoll < 1000000u) return;
    lastSettingsPoll = now;
    if (*reinterpret_cast<const uint8_t*>(Address<0x8B8EE20>() + 0x131)) return;
    SceIoStat current{};
    if (sceIoGetstat(iniPath, &current) < 0 || current.st_size <= 0) return;
    if (cfgObserved && current.st_size == observedCfg.st_size &&
        !std::memcmp(&current.sce_st_mtime, &observedCfg.sce_st_mtime, sizeof(current.sce_st_mtime))) return;
    ReadSettings();
    ApplyFrameRate();
    if (!settings.unthrottle) console::portable::Unthrottle(false);
    observedCfg = current;
    cfgObserved = true;
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
    if (!console::portable::StartReported("GTA3",iniPath,"ms0:/PSP/PLUGINS/GTALCS.PPSSPP.FusionFix/GTALCS.PPSSPP.FusionFix.log")) return -1;
    static constexpr const char* oldPlugins[] = {"GTALCS.PPSSPP.WidescreenFix", "GTALCS.PPSSPP.ImVehLM"};
    if (console::portable::conflict::Found(oldPlugins)) return -1;
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
        {"MAIN", "FPSLimit", "30"},
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
        {"MAIN", "FOVFactor", "1"},
        {"MAIN", "CutsceneBorders", "3"},
        {"MAIN", "FMVWidescreenMode", "1"},
        {"MAIN", "GamepadIcons", "Auto"},
        {"MAIN", "VehicleLights", "1"},
    };
    console::portable::CompleteCfg(iniPath, defaults);
    ReadSettings();
    if (settings.skipIntro) injector::MakeNOP(Address<0x89c3814>());
    InstallCheats();
    InstallMovies();
    InstallIcons();
    InstallCamera();InstallDrawing();InstallControls();InstallTiming();InstallMenu();InstallAim();
    if (settings.vehicleLights) InstallVehicleLights();
    if (injector::detail::startup.error != PSP_HOOK_OK)
        logger.WriteF("Fix disabled: patch at %08X rejected (%u).\n",
            unsigned(injector::detail::startup.failed_address), unsigned(injector::detail::startup.error));
    return console::portable::Finish();
}
