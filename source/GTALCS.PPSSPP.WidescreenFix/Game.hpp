#pragma once
#include "Addresses.hpp"
#include "../Shared/Console/PortableDrawing.hpp"
namespace lcsws {
struct Game {};
using Drawing = console::portable::StoryDrawing<Game>;
using Anchor = console::portable::DrawAnchor;
struct Settings {
    bool dualAnalog = true, modernControls = false, skipIntro = true, unthrottle = true, pcCheats = false;
    int fps = 0;
    float fov = 1.0f, lod = 0.0f;
    bool restoreCutsceneFov = true, cutsceneBorders = true;
};
inline Settings settings;
inline bool autoAspect=true;
inline constexpr char iniPath[]="ms0:/PSP/PLUGINS/GTALCS.PPSSPP.WidescreenFix/GTALCS.PPSSPP.WidescreenFix.ini";
void ApplyFrameRate();
void InstallMenu();
void TickMenu();
void InstallAim();
void InstallControls();
void InstallCamera();
void InstallDrawing();
void InstallTiming();
void TickCheats();
void InstallCheats();
}
