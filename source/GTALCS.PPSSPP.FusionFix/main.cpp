#include "Game.hpp"
#include "Addresses.hpp"
#include "AspectRatio.hpp"
#include "Shared/Console/PortableCfg.hpp"
#include "Shared/Console/PspPluginConflict.hpp"
#include <cstdlib>
#include <cstring>
extern "C" {
#include <pspkernel.h>
#include <pspiofilemgr.h>
}
PSP_MODULE_INFO("GTALCS.PPSSPP.FusionFix",PSP_MODULE_USER,2,0);
namespace lcsws {
namespace {
unsigned lastSettingsPoll;
void ReadTextLanguage()
{
    char buffer[32]{};
    const char* value = inireader.ReadString("MAIN", "TextLanguage", "auto", buffer, sizeof(buffer));
    settings.textLanguage = value && !std::strcmp(value, "en") ? TextLanguage::English : TextLanguage::Auto;
}

bool ReloadCfg()
{
    SceIoStat stat{};
    if (sceIoGetstat(iniPath, &stat) < 0 || stat.st_size <= 0 || uint64_t(stat.st_size) >= console::portable::IniCapacity)
        return false;
    inireader.SetIniPath(iniPath);
    return inireader.iniBufSize == stat.st_size;
}

void ReadSettings()
{
    using console::bounded;
    ReadTextLanguage();
    settings.skipIntro=inireader.ReadBoolean("MAIN","SkipIntro",1);
    settings.unthrottle=inireader.ReadBoolean("MAIN","UnthrottleEmuDuringLoading",1);
    settings.fps=inireader.ReadInteger("MAIN","FPSLimit",30)==60;
    settings.modernControls=inireader.ReadBoolean("CONTROLS","ModernControlScheme",1);
    settings.holdVehicleCamera=inireader.ReadBoolean("CONTROLS","HoldVehicleCamera",0);
    settings.pcCheats=inireader.ReadBoolean("CONTROLS","PCCheats",0);
    settings.vehicleLights=inireader.ReadBoolean("MAIN","VehicleLights",1);
    float fov=inireader.ReadFloat("MAIN","FOVFactor",1);
    settings.fov=fov>0 ? bounded(fov,0.5f,2.0f,1) : 1;
    settings.cutsceneBorders=int(bounded(inireader.ReadFloat("MAIN","CutsceneBorders",3),0,3,3));
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
    drawing.multiplayerRadar=bounded(inireader.ReadFloat("MPRADAR","RadarScale",0.63f),0.1f,2,0.63f);
}
}
void PollSettings()
{
    const unsigned now = sceKernelGetSystemTimeLow();
    TickIcons(now);
    if (now - lastSettingsPoll < 1000000u) return;
    lastSettingsPoll = now;
    if (*reinterpret_cast<const uint8_t*>(Address<0x8B8EE20>() + 0x131)) return;
    // File timestamps can stay the same between edits on the memory stick.
    // After loading a state, the live file must also win over restored values.
    if (!ReloadCfg())
        return;
    ReadSettingsPreservingMenuEdits(ReadSettings);
    ApplyFrameRate();
    if (!settings.unthrottle) console::portable::Unthrottle(false);
}
void ReloadTextLanguage()
{
    if (ReloadCfg())
        ReadSettingsPreservingMenuEdits(ReadTextLanguage);
}
}
extern "C" int module_start(SceSize,void*) {
    using namespace lcsws;
    if (!console::portable::StartReported("GTA3",iniPath)) return -1;
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
        {"CONTROLS", "ModernControlScheme", "1"},
        {"CONTROLS", "HoldVehicleCamera", "0"},
        {"CONTROLS", "PCCheats", "0"},
        {"MAIN", "HudScale", "0.75"},
        {"MAIN", "HudAspectRatioConstraint", "Auto"},
        {"RADAR", "RadarScale", "0.83"},
        {"MPRADAR", "RadarScale", "0.63"},
        {"MAIN", "FOVFactor", "1.0"},
        {"MAIN", "CutsceneBorders", "3"},
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
