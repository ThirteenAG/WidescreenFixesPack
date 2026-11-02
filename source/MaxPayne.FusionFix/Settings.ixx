module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/MaxPayne/SettingsMenu.hpp"

export module Settings;

import ComVars;
import Cinematic;

export int32_t nGraphicNovelModeKey = VK_F2;

// The settings that apply at once, read again whenever one of them changes (in the menu, in an editor through the
// live link, or the graphic novel key)
void ApplySettings()
{
    auto& settings = FixSettings();
    Screen.fHudAspectRatioConstraint = ParseWidescreenHudOffset(settings.String("MAIN.HudAspectRatioConstraint"));
    Screen.fFOVFactor = settings.Float("MAIN.FOVFactor");
    if (Screen.fFOVFactor <= 0.0f) { Screen.fFOVFactor = 1.0f; }
    Screen.bGraphicNovelMode = settings.Int("MAIN.GraphicNovelMode") != 0;
    Cinematic::nCutsceneBorders = std::clamp(settings.Int("MAIN.CutsceneBorders"), (int32_t)Cinematic::Off, (int32_t)Cinematic::Both);
    Cinematic::bNoBorderAnimation = !settings.Bool("MAIN.AnimatedCutsceneBorders");
    nGraphicNovelModeKey = MaxPayneMenu::KeyCode(settings.String("MAIN.GraphicNovelModeKey"));

    // HUD offsets, cameras and the graphic novel page follow the new values
    RecalculateScreen();
}

export void ReadSettings()
{
    ApplySettings();
    for (auto szId : { "MAIN.HudAspectRatioConstraint", "MAIN.FOVFactor", "MAIN.GraphicNovelMode", "MAIN.CutsceneBorders", "MAIN.AnimatedCutsceneBorders", "MAIN.GraphicNovelModeKey" })
        FixSettings().OnChange(szId, ApplySettings);
}
