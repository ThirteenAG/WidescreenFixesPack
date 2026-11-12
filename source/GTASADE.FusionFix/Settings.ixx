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
    bool rawMouseInput = true;
    bool restoreOriginalCamera = true;
    float centeringDelay = 5.0f;
    bool aircraftFreeCamera = false;
};
export FusionSettings Settings;

namespace
{
    void ReadValues()
    {
        Settings.skipIntro = FixSettings().Bool("MAIN.SkipIntro");
        Settings.skipMenu = FixSettings().Bool("MAIN.SkipMenu");
        const auto slot = FixSettings().Int("MAIN.SaveSlot");
        Settings.saveSlot = slot >= 1 && slot <= 8 ? slot - 1 : 4;
        const auto validScale = [](float value) { return std::isfinite(value) && value > 0.0f ? value : 1.0f; };
        Settings.hudScale = validScale(FixSettings().Float("MAIN.HudScale"));
        Settings.radarScale = validScale(FixSettings().Float("MAIN.RadarScale"));
        Settings.restoreOriginalCamera = FixSettings().Bool("MAIN.RestoreOriginalCamera");
        Settings.rawMouseInput = FixSettings().Bool("MAIN.RawInputMouse");
        const auto delay = FixSettings().Float("MAIN.CenteringDelay");
        Settings.centeringDelay = std::isfinite(delay) && delay >= 0.0f ? delay : 5.0f;
        Settings.aircraftFreeCamera = FixSettings().Bool("MAIN.AircraftFreeCamera");
        Settings.classicLODsOnly = FixSettings().Bool("GRAPHICS.ClassicLODsOnly");
        const auto style = FixSettings().Int("GRAPHICS.VisualStyle");
        Settings.visualStyle = style >= 0 && style <= 2 ? static_cast<VisualStyle>(style) : VisualStyle::PS2;
        Settings.restoreWeatherColours = FixSettings().Bool("GRAPHICS.RestoreWeatherColours");
        const auto strength = FixSettings().Float("GRAPHICS.ColourFilterStrength");
        Settings.colourFilterStrength = std::isfinite(strength) ? std::clamp(strength, 0.0f, 1.0f) : 1.0f;
    }
}

// Runs after every live change, once the values above are current
export WFP::Event<>& onSettingsChanged()
{
    static WFP::Event<> event;
    return event;
}

export void ReadSettings()
{
    ReadValues();
    for (const auto id : { "MAIN.HudScale", "MAIN.RadarScale",
        "MAIN.RestoreOriginalCamera", "MAIN.RawInputMouse", "MAIN.CenteringDelay", "MAIN.AircraftFreeCamera", "MAIN.SaveSlot",
        "GRAPHICS.VisualStyle",
        "GRAPHICS.RestoreWeatherColours", "GRAPHICS.ColourFilterStrength" })
    {
        FixSettings().OnChange(id, []()
        {
            ReadValues();
            onSettingsChanged().executeAll();
        });
    }
    WFP::onReadGameConfig().executeAll();
}
