module;

#include "stdafx.h"
#include "FixSettings.hpp"

export module Settings;


export enum class VisualStyle { DE, PS2, PC };

export struct FusionSettings
{
    VisualStyle visualStyle = VisualStyle::PS2;
    bool restoreWeatherColours = true;
    bool classicLODsOnly = false;
    float colourFilterStrength = 1.0f;
    bool skipIntro = true;
    bool skipMenu = false;
    int saveSlot = 4;
    float hudScale = 0.8f;
    float radarScale = 0.75f;
    bool improveCameraPC = true;
    bool rawMouseInput = true;
    bool restoreOriginalCamera = true;
    float centeringDelay = 5.0f;
};
export FusionSettings Settings;

export void ReadSettings()
{

    Settings.skipIntro = FixSettings().Bool("MAIN.SkipIntro");
    Settings.skipMenu = FixSettings().Bool("MAIN.SkipMenu");
    const auto slot = FixSettings().Int("MAIN.SaveSlot");
    Settings.saveSlot = slot >= 1 && slot <= 8 ? slot - 1 : 4;
    const auto validScale = [](float value) { return std::isfinite(value) && value > 0.0f ? value : 1.0f; };
    Settings.hudScale = validScale(FixSettings().Float("MAIN.HudScale"));
    Settings.radarScale = validScale(FixSettings().Float("MAIN.RadarScale"));
    Settings.improveCameraPC = FixSettings().Bool("MAIN.ImproveCameraPC");
    Settings.restoreOriginalCamera = FixSettings().Bool("MAIN.RestoreOriginalCamera");
    Settings.rawMouseInput = FixSettings().Bool("MAIN.RawInputMouse");
    const auto delay = FixSettings().Float("MAIN.CenteringDelay");
    Settings.centeringDelay = std::isfinite(delay) && delay >= 0.0f ? delay : 5.0f;
    Settings.classicLODsOnly = FixSettings().Bool("GRAPHICS.ClassicLODsOnly");
    const auto style = FixSettings().Int("GRAPHICS.VisualStyle");
    Settings.visualStyle = style >= 0 && style <= 2 ? static_cast<VisualStyle>(style) : VisualStyle::PS2;
    Settings.restoreWeatherColours = FixSettings().Bool("GRAPHICS.RestoreWeatherColours");
    const auto strength = FixSettings().Float("GRAPHICS.ColourFilterStrength");
    Settings.colourFilterStrength = std::isfinite(strength) ? std::clamp(strength, 0.0f, 1.0f) : 1.0f;
    WFP::onReadGameConfig().executeAll();
}
