module;

#include "stdafx.h"
#include <charconv>
#include "FixSettings.hpp"

export module Settings;

import RawMouse;


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
    float fovFactor = 1.0f;
    float hudScale = 0.8f;
    float radarScale = 0.75f;
    bool aircraftFreeCamera = false;
    std::atomic_bool rawMouseInput = true;
    bool restoreOriginalCamera = true;
    float centeringDelay = 5.0f;
};
export FusionSettings Settings;

namespace
{
    std::atomic_bool settingsPending = false;

    template<typename T>
    T EffectiveValue(const char* id)
    {
        const auto text = FixSettings().Get(id);
        T value{};
        std::from_chars(text.data(), text.data() + text.size(), value);
        return value;
    }

    void ReadValues()
    {
        const auto validScale = [](float value) { return std::isfinite(value) && value > 0.0f ? value : 1.0f; };
        Settings.fovFactor = EffectiveValue<float>("MAIN.FOVFactor");
        Settings.hudScale = validScale(EffectiveValue<float>("MAIN.HudScale"));
        Settings.radarScale = validScale(EffectiveValue<float>("MAIN.RadarScale"));
        Settings.aircraftFreeCamera = EffectiveValue<int>("MAIN.AircraftFreeCamera") != 0;
        Settings.restoreOriginalCamera = EffectiveValue<int>("MAIN.RestoreOriginalCamera") != 0;
        const bool rawMouseInput = EffectiveValue<int>("MAIN.RawInputMouse") != 0;
        if (Settings.rawMouseInput.exchange(rawMouseInput) != rawMouseInput)
            ResetRawMouse();
        const auto delay = EffectiveValue<float>("MAIN.CenteringDelay");
        Settings.centeringDelay = std::isfinite(delay) && delay >= 0.0f ? delay : 5.0f;
        const auto style = EffectiveValue<int>("GRAPHICS.VisualStyle");
        Settings.visualStyle = style >= 0 && style <= 2 ? static_cast<VisualStyle>(style) : VisualStyle::PS2;
        Settings.restoreWeatherColours = EffectiveValue<int>("GRAPHICS.RestoreWeatherColours") != 0;
        const auto strength = EffectiveValue<float>("GRAPHICS.ColourFilterStrength");
        Settings.colourFilterStrength = std::isfinite(strength) ? std::clamp(strength, 0.0f, 1.0f) : 1.0f;
    }
}

export WFP::Event<>& onWeatherPaletteChanged()
{
    static WFP::Event<> event;
    return event;
}

export WFP::Event<>& onOriginalCameraChanged()
{
    static WFP::Event<> event;
    return event;
}

export void ReadSettings()
{
    Settings.skipIntro = EffectiveValue<int>("MAIN.SkipIntro") != 0;
    Settings.skipMenu = EffectiveValue<int>("MAIN.SkipMenu") != 0;
    const auto slot = EffectiveValue<int>("MAIN.SaveSlot");
    Settings.saveSlot = slot >= 1 && slot <= 8 ? slot - 1 : 4;
    Settings.classicLODsOnly = EffectiveValue<int>("GRAPHICS.ClassicLODsOnly") != 0;
    for (const auto id : { "MAIN.FOVFactor", "MAIN.HudScale", "MAIN.RadarScale", "MAIN.AircraftFreeCamera",
        "MAIN.RestoreOriginalCamera", "MAIN.RawInputMouse", "MAIN.CenteringDelay", "GRAPHICS.VisualStyle",
        "GRAPHICS.RestoreWeatherColours", "GRAPHICS.ColourFilterStrength" })
    {
        FixSettings().OnChange(id, []()
        {
            settingsPending.store(true);
        });
    }
    ReadValues();
    WFP::onGameProcessEvent() += []()
    {
        if (!settingsPending.exchange(false))
            return;
        const auto useClassicPalette = []() { return Settings.visualStyle != VisualStyle::DE && Settings.restoreWeatherColours; };
        const bool wasClassic = useClassicPalette();
        const bool originalCamera = Settings.restoreOriginalCamera;
        ReadValues();
        if (originalCamera != Settings.restoreOriginalCamera)
            onOriginalCameraChanged().executeAll();
        if (wasClassic != useClassicPalette())
            onWeatherPaletteChanged().executeAll();
    };
    WFP::onReadGameConfig().executeAll();
}
