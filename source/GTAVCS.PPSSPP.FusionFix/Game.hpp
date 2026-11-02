#pragma once
#include "Addresses.hpp"
#include "../Shared/Console/PortableDrawing.hpp"
namespace vcsws {
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
    int fps = 0, unthrottleMode = 1;
    float fov = 1.0f;
    void SetFovRequest(float value) {
        // MAIN.FOVFactor; zero (the old "auto") renders at the neutral factor.
        fov = value > 0.0f ? console::bounded(value, 0.5f, 2.0f, 1.0f) : 1.0f;
    }
    // MAIN.CutsceneBorders: 0 off, 1 letterbox, 2 pillarbox, 3 both.
    int cutsceneBorders = 3;
    // MAIN.FMVWidescreenMode: 0 original, 1 fill.
    int movieMode = 1;
    // MAIN.VehicleLights (read at startup): lit head, tail, brake and reverse lights on vehicle bodies.
    bool vehicleLights = true;
};
inline Settings settings;
inline bool autoAspect=true;
inline constexpr char cfgPath[]="ms0:/PSP/PLUGINS/GTAVCS.PPSSPP.FusionFix/GTAVCS.PPSSPP.FusionFix.cfg";
void ReloadTextLanguage();
void ApplyFrameRate();
void InstallControls();
void InstallCamera();
void InstallDrawing();
void InstallTiming();
void InstallMenu();
void InstallAim();
void InstallVehicleLights();
void InstallGamepadIcons(const char* set);
bool GamepadIconsAvailable();
unsigned GamepadIconSet();
void SelectGamepadIcons(unsigned set);
void TickCheats();
void InstallCheats();
void InstallMovies();
bool MoviesAvailable();
}
