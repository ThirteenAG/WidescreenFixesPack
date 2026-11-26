#include "Game.hpp"
#include "Addresses.hpp"
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <algorithm>
#include "Shared/Console/PortableCfg.hpp"
#include "Shared/Console/PspPluginConflict.hpp"
extern "C" {
#include <pspkernel.h>
#include <pspiofilemgr.h>
}
PSP_MODULE_INFO("GTAVCS.PPSSPP.FusionFix",PSP_MODULE_USER,2,0);
namespace vcsws {
namespace {
unsigned lastSettingsPoll;

bool ReloadCfg()
{
    SceIoStat stat{};
    if (sceIoGetstat(cfgPath, &stat) < 0 || stat.st_size <= 0 || uint64_t(stat.st_size) >= console::portable::IniCapacity)
        return false;
    inireader.SetIniPath(cfgPath);
    return inireader.iniBufSize == stat.st_size;
}

void ReadTextLanguage()
{
    char buffer[32]{};
    const char* value = inireader.ReadString("MAIN", "TextLanguage", "auto", buffer, sizeof(buffer));
    settings.textLanguage = value && !std::strcmp(value, "en") ? TextLanguage::English : TextLanguage::Auto;
}

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
void ReadDisplayRatios() {
    char ratio[64];
    const float forced = console::parse_aspect(
        inireader.ReadString("MAIN", "ForceAspectRatio", "Auto", ratio, sizeof(ratio)), 0.5f, 8.0f);
    autoAspect = forced == 0.0f;
    Drawing::settings.aspect = autoAspect ? console::portable::Aspect() : forced;
    char hudAspect[32];
    Drawing::settings.hudAspect = console::parse_hud_aspect(
        inireader.ReadString("MAIN", "HudAspectRatioConstraint", "Auto", hudAspect, sizeof(hudAspect)));
}
void ReadSettings() {
    using console::bounded;
    ReadTextLanguage();
    settings.skipIntro=inireader.ReadBoolean("MAIN","SkipIntro",1);
    settings.unthrottleMode=inireader.ReadInteger("MAIN","UnthrottleEmuDuringLoading",1);
    if (settings.unthrottleMode < 0 || settings.unthrottleMode > 2) settings.unthrottleMode = 1;
    settings.unthrottle=settings.unthrottleMode!=0;
    settings.fps=inireader.ReadInteger("MAIN","FPSLimit",30)==60;
    settings.modernControls=inireader.ReadBoolean("CONTROLS","ModernControlScheme",1);
    settings.pcCheats=inireader.ReadBoolean("CONTROLS","PCCheats",0);
    settings.SetFovRequest(ReadCfgFloat("MAIN","FOVFactor",1));
    settings.cutsceneBorders=int(bounded(ReadCfgFloat("MAIN","CutsceneBorders",3),0,3,3));
    settings.vehicleLights=inireader.ReadBoolean("MAIN","VehicleLights",1);
    settings.holdVehicleCamera=inireader.ReadBoolean("CONTROLS","HoldVehicleCamera",0);
    ReadDisplayRatios();
    auto& drawing=Drawing::settings;
    float hud=ReadCfgFloat("MAIN","HudScale",0.75f);
    drawing.hud=hud>0 ? bounded(hud,0.1f,2,0.75f) : 0.75f;
    float radar=ReadCfgFloat("RADAR","RadarScale",0.83f);
    drawing.radar=radar>0 ? bounded(radar,0.1f,2,0.83f) : 0.83f;
    drawing.multiplayerRadar=bounded(ReadCfgFloat("MPRADAR","RadarScale",0.63f),0.1f,2,0.63f);
}
}
void PollSettings(bool displayMenu)
{
    const unsigned now = sceKernelGetSystemTimeLow();
    const bool menuActive = *reinterpret_cast<const uint8_t*>(Address<0x8BC9100>() + 0x20) != 0;
    if (menuActive && !displayMenu) return;
    if (!menuActive) SaveMenuSettingsIfDue(now);
    if (now - lastSettingsPoll < 1000000u) return;
    lastSettingsPoll = now;
    if (!ReloadCfg()) return;
    if (menuActive) {
        ReadSettingsPreservingMenuEdits(ReadDisplayRatios);
        return;
    }
    ReadSettingsPreservingMenuEdits(ReadSettings);
    ApplyFrameRate();
    if (!settings.unthrottle) console::portable::Unthrottle(false);
    char icons[16]{};
    const char* selected = inireader.ReadString("MAIN", "GamepadIcons", "Auto", icons, sizeof(icons));
    static constexpr const char* iconSets[] = {"PSP", "360", "XBO", "PS3", "PS4", "PS5", "SWI", "DEK", "Auto"};
    for (unsigned i = 0; i < 9; ++i)
        if (!std::strcmp(selected, iconSets[i])) { SelectConfiguredGamepadIcons(i); break; }
}
void ReloadTextLanguage() {
    if (ReloadCfg()) ReadTextLanguage();
}
}
extern "C" int module_start(SceSize,void*) {
    using namespace vcsws;
    if (!console::portable::StartReported("GTA3",cfgPath)) return -1;
    static constexpr const char* oldPlugins[] = {"GTAVCS.PPSSPP.WidescreenFix", "GTAVCS.PPSSPP.ImVehLM", "GTAVCS.PPSSPP.GamepadIcons"};
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
        {"CONTROLS", "PCCheats", "0"},
        {"CONTROLS", "HoldVehicleCamera", "0"},
        {"MAIN", "HudScale", "0.75"},
        {"MAIN", "HudAspectRatioConstraint", "Auto"},
        {"RADAR", "RadarScale", "0.83"},
        {"MPRADAR", "RadarScale", "0.63"},
        {"MAIN", "FOVFactor", "1.0"},
        {"MAIN", "CutsceneBorders", "3"},
        {"MAIN", "VehicleLights", "1"},
        {"MAIN", "GamepadIcons", "Auto"},
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
    InstallMovies();
    if (settings.vehicleLights) InstallVehicleLights();
    char icons[16]{};
    InstallGamepadIcons(inireader.ReadString("MAIN", "GamepadIcons", "Auto", icons, sizeof(icons)));
#ifndef NDEBUG
    logger.WriteF("Menu patches: %u", unsigned(injector::detail::startup.error));
    for (auto* entry = injector::detail::startup.first; entry; entry = entry->next)
        if (entry->kind == injector::detail::entry_kind::patch && entry->callback &&
            psp_hook_control(entry->value.patch.replacement[1]) != 0)
            logger.WriteF("Invalid call delay slot at %08X: %08X", entry->value.patch.address, entry->value.patch.replacement[1]);
#endif
    return console::portable::Finish();
}
