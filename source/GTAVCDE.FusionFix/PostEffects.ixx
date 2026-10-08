module;

#include "stdafx.h"

export module PostEffects;

// Colour-only adaptation of the original filters to UE colour grading. The
// native tonemapper and HDR remain active: this is not a pixel-identical port
// of RenderWare's post-tonemap framebuffer blending/clipping or PS2 blur.

import Settings;
import Weather;

namespace
{
    SafetyHookInline applyValuesHook, colourOptionsHook;

    float ToLinear(float value)
    {
        value = std::clamp(value, 0.0f, 1.0f);
        return value <= 0.04045f ? value / 12.92f : std::pow((value + 0.055f) / 1.055f, 2.4f);
    }

    void SetWeatherHue(uint8_t* actor, size_t offset, const std::array<float, 3>& colour)
    {
        auto* target = reinterpret_cast<float*>(actor + offset);
        const std::array<float, 3> linear{ToLinear(colour[0]), ToLinear(colour[1]), ToLinear(colour[2])};
        const auto luminance = [](const auto& rgb) { return rgb[0] * 0.2126f + rgb[1] * 0.7152f + rgb[2] * 0.0722f; };
        const float oldLuminance = luminance(target);
        const float newLuminance = luminance(linear);
        if (!std::isfinite(oldLuminance) || oldLuminance < 0.0f)
            return;
        // Unreal lights use physical intensities. Change their chromaticity
        // without replacing exposure, HDR calibration or DE light attenuation.
        const float scale = newLuminance > 0.000001f ? oldLuminance / newLuminance : 0.0f;
        for (size_t channel = 0; channel < 3; ++channel)
            target[channel] = linear[channel] * scale;
    }

    std::array<float, 3> ColourFilter(const WeatherColours& weather)
    {
        std::array<float, 3> gain{};
        for (size_t channel = 0; channel < 3; ++channel)
        {
            const float colour = std::clamp(weather.filter1[channel] / 255.0f, 0.0f, 1.0f);
            if (Settings.visualStyle == VisualStyle::PS2)
                gain[channel] = 1.0f + colour;
            else
            {
                // VC's PC path adds two coloured passes after the 30/255
                // double-colour blend (SkyGFX vcTrailsPS). Retain the colour
                // contribution without adding temporal trails to DE rendering.
                constexpr float alpha = 30.0f / 255.0f;
                float previous = 1.0f;
                for (int pass = 0; pass < 5; ++pass)
                    previous = (1.0f - alpha) + previous * (std::min(colour * 2.0f, 1.0f) * alpha + colour * 2.0f);
                gain[channel] = previous;
            }
            gain[channel] = std::lerp(1.0f, gain[channel], Settings.colourFilterStrength);
        }
        return gain;
    }

    uintptr_t UpdateColorOptions(uint8_t* volume)
    {
        // Native code rebuilds RGB gain from its baseline on every invocation.
        // Applying to that fresh result avoids accumulating colour each frame.
        const auto result = colourOptionsHook.ccall<uintptr_t>(volume);
        const auto weather = GetWeatherColours();
        if (Settings.visualStyle == VisualStyle::DE || !weather.valid || weather.specialEffect)
            return result;
        const auto gain = ColourFilter(weather);
        auto* colourGain = reinterpret_cast<float*>(volume + 608 + 96);
        for (size_t channel = 0; channel < 3; ++channel)
            colourGain[channel] *= gain[channel];
        volume[608] |= 0x20; // bOverride_ColorGain; other override bits stay native.
        return result;
    }

    uintptr_t ApplyValues(uint8_t* actor)
    {
        const auto weather = GetWeatherColours();
        const bool active = Settings.visualStyle != VisualStyle::DE && weather.valid && !weather.specialEffect;
        if (active)
        {
            if (Settings.restoreWeatherColours)
            {
                // FSkyInterpolationResult starts at 696 on all three latest
                // PC builds. Update before ApplyValues performs sky occlusion,
                // interior blending and uploads the colours to UE components.
                SetWeatherHue(actor, 696, weather.ambient);
                SetWeatherHue(actor, 712, weather.skyBottom);
                SetWeatherHue(actor, 728, weather.skyTop);
                SetWeatherHue(actor, 744, weather.skyBottom);
                SetWeatherHue(actor, 792, weather.clouds);
                SetWeatherHue(actor, 840, weather.lowClouds);
                SetWeatherHue(actor, 856, weather.clouds);
                SetWeatherHue(actor, 872, weather.topClouds);
                SetWeatherHue(actor, 888, weather.clouds);
                std::array<float, 3> fog;
                for (size_t channel = 0; channel < 3; ++channel)
                {
                    const auto top = static_cast<int>(std::lround(weather.skyTop[channel] * 255.0f));
                    const auto bottom = static_cast<int>(std::lround(weather.skyBottom[channel] * 255.0f));
                    fog[channel] = ((top + 2 * bottom) / 3) / 255.0f;
                }
                SetWeatherHue(actor, 920, fog); // Classic Atmosphere fog colour override.
                SetWeatherHue(actor, 976, fog);
                SetWeatherHue(actor, 1040, weather.directional);
            }
            // The weather curve's gain/contrast/saturation/midtone contrast
            // would grade the restored palette a second time. Native user
            // brightness, weapon wheel effects and material blendables remain.
            std::fill_n(reinterpret_cast<float*>(actor + 1076), 4, 1.0f);
        }
        const auto result = applyValuesHook.ccall<uintptr_t>(actor);
        if (active)
        {
            auto* volume = *reinterpret_cast<uint8_t**>(actor + 10032);
            if (volume && !volume[2068])
                UpdateColorOptions(volume);
        }
        return result;
    }
}

class PostEffectsModule
{
public:
    PostEffectsModule()
    {
        WFP::onInitEvent() += []()
        {
            if (Settings.visualStyle == VisualStyle::DE)
                return;
            const auto applyValues = hook::pattern("48 8B C4 55 53 57 41 56 48 8D 68 88 48 81 EC 58 01 00 00 4C 89 60 D8 48").get_first();
            const auto colourOptions = hook::pattern("48 8B C4 55 41 56 48 8D 68 A1 48 81 EC E8 00 00 00 48 83 3D ? ? ? ?").get_first();
            if (!applyValues || !colourOptions)
                return;
            colourOptionsHook = safetyhook::create_inline(colourOptions, UpdateColorOptions);
            if (!colourOptionsHook)
                return;
            applyValuesHook = safetyhook::create_inline(applyValues, ApplyValues);
            if (!applyValuesHook)
                colourOptionsHook = {};
        };
    }
} PostEffectsModuleInstance;
