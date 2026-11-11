#pragma once
#include "Addresses.hpp"
#include "../Shared/Console/PortableDrawing.hpp"
namespace lcsws {
struct Game {};
using Drawing = console::portable::StoryDrawing<Game>;
using Anchor = console::portable::DrawAnchor;
enum class TextLanguage { Auto, English };
struct Settings {
    TextLanguage textLanguage = TextLanguage::Auto;
    TextLanguage MenuLanguage() const {
        // English is the only shipped native console menu catalog.
        return textLanguage == TextLanguage::Auto ? TextLanguage::English : textLanguage;
    }
    bool modernControls = false, skipIntro = true, unthrottle = true, pcCheats = false;
    int fps = 0;
    float fov = 1.0f;
    // MAIN.CutsceneBorders: 0 off, 1 letterbox, 2 pillarbox, 3 both.
    int cutsceneBorders = 3;
    int icons = 8;
    // MAIN.VehicleLights (read at startup): lit head, tail, brake and reverse lights on vehicle bodies.
    bool vehicleLights = true;
};
inline Settings settings;
inline bool autoAspect=true;
inline constexpr const char* iconSets[] = {"PSP", "360", "XBO", "PS3", "PS4", "PS5", "SWI", "DEK", "Auto"};
inline constexpr char iniPath[]="ms0:/PSP/PLUGINS/GTALCS.PPSSPP.FusionFix/GTALCS.PPSSPP.FusionFix.cfg";
void ReloadTextLanguage();
void PollSettings();
void ApplyFrameRate();
void InstallMenu();
void TickMenu();
void InstallAim();
void InstallVehicleLights();
void InstallControls();
void InstallCamera();
void InstallMovies();
void InstallIcons();
void TickIcons(unsigned now);
bool IconsAvailable();
bool IconDataValid();
void InstallDrawing();
void InstallTiming();
void TickCheats();
void InstallCheats();
bool CheatsAvailable();
}
