module;

#include "stdafx.h"

export module Settings;


export enum class VisualStyle { DE, PS2, PC };

export struct FusionSettings
{
    VisualStyle visualStyle = VisualStyle::PS2;
    bool restoreWeatherColours = true;
    bool classicLODsOnly = false;
    float colourFilterStrength = 1.0f;
    bool skipIntro = true;
    bool skipMenu = true;
    int saveSlot = 5;
    float hudScale = 0.8f;
    float radarScale = 0.75f;
    bool disableFirstPersonAimForRifles = true;
    bool improveCameraPC = true;
    bool rawMouseInput = true;
    bool restoreOriginalCamera = true;
    float centeringDelay = 5.0f;
};
export FusionSettings Settings;

export void ReadSettings()
{
    CIniReader reader("");
    Settings.skipIntro = reader.ReadInteger("MAIN", "SkipIntro", 1) != 0;
    Settings.skipMenu = reader.ReadInteger("MAIN", "SkipMenu", 1) != 0;
    const auto slot = reader.ReadInteger("MAIN", "SaveSlot", 6);
    Settings.saveSlot = slot >= 1 && slot <= 8 ? slot - 1 : 5;
    const auto validScale = [](float value) { return std::isfinite(value) && value > 0.0f ? value : 1.0f; };
    Settings.hudScale = validScale(reader.ReadFloat("MAIN", "HudScale", 0.8f));
    Settings.radarScale = validScale(reader.ReadFloat("MAIN", "RadarScale", 0.75f));
    Settings.disableFirstPersonAimForRifles = reader.ReadInteger("MAIN", "DisableFirstPersonAimForRifles", 1) != 0;
    Settings.improveCameraPC = reader.ReadInteger("MAIN", "ImproveCameraPC", 1) != 0;
    Settings.restoreOriginalCamera = reader.ReadInteger("MAIN", "RestoreOriginalCamera", 1) != 0;
    Settings.rawMouseInput = reader.ReadInteger("MAIN", "RawMouseInput", 1) != 0;
    const auto delay = reader.ReadFloat("MAIN", "CenteringDelay", 5.0f);
    Settings.centeringDelay = std::isfinite(delay) && delay >= 0.0f ? delay : 5.0f;
    Settings.classicLODsOnly = reader.ReadInteger("GRAPHICS", "ClassicLODsOnly", 0) != 0;
    const auto style = reader.ReadInteger("GRAPHICS", "VisualStyle", 1);
    Settings.visualStyle = style >= 0 && style <= 2 ? static_cast<VisualStyle>(style) : VisualStyle::PS2;
    Settings.restoreWeatherColours = reader.ReadInteger("GRAPHICS", "RestoreWeatherColours", 1) != 0;
    const auto strength = reader.ReadFloat("GRAPHICS", "ColourFilterStrength", 1.0f);
    Settings.colourFilterStrength = std::isfinite(strength) ? std::clamp(strength, 0.0f, 1.0f) : 1.0f;
    WFP::onReadGameConfig().executeAll();
}
