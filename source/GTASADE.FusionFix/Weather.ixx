module;

#include "stdafx.h"

export module Weather;

import Settings;

// The native timecycle owns interpolation, including VC extra colours and SA
// regional smog, timecycle boxes, tunnels and underwater transitions.
export struct WeatherColours
{
    std::array<float, 3> ambient{}, skyTop{}, skyBottom{}, clouds{}, lowClouds{}, topClouds{}, directional{};
    std::array<float, 4> filter1{}, filter2{};
    bool valid = false;
    bool specialEffect = false;
};

namespace
{
    SafetyHookInline updateHook;
    bool weatherReady = false;
    uint8_t* colours = nullptr;
    uint8_t* nightVision = nullptr;
    uint8_t* infrared = nullptr;
    uint8_t* darkness = nullptr;

    uintptr_t Update()
    {
        const auto result = updateHook.ccall<uintptr_t>();
        weatherReady = true;
        return result;
    }
}

export WeatherColours GetWeatherColours()
{
    WeatherColours result;
    if (!weatherReady)
        return result;
    // CColourSet's original 172-byte prefix precedes the DE sky-curve result.
    // These offsets are verified against the PC constructor, not the mobile ABI.
    const auto floats = reinterpret_cast<const float*>(colours);
    for (size_t channel = 0; channel < 3; ++channel)
    {
        result.ambient[channel] = floats[channel];
        result.skyTop[channel] = reinterpret_cast<const uint16_t*>(colours + 36)[channel] / 255.0f;
        result.skyBottom[channel] = reinterpret_cast<const uint16_t*>(colours + 42)[channel] / 255.0f;
        result.clouds[channel] = reinterpret_cast<const uint16_t*>(colours + 98)[channel] / 255.0f;
    }
    for (size_t channel = 0; channel < 3; ++channel)
    {
        result.lowClouds[channel] = reinterpret_cast<const uint16_t*>(colours + 92)[channel] / 255.0f;
        result.topClouds[channel] = result.clouds[channel];
        result.directional[channel] = reinterpret_cast<const uint16_t*>(colours + 48)[channel] / 255.0f;
    }
    std::copy_n(floats + 30, 4, result.filter1.begin());
    std::copy_n(floats + 34, 4, result.filter2.begin());
    result.specialEffect = *nightVision || *infrared || *darkness;
    result.valid = true;
    for (const auto* values : {&result.ambient, &result.skyTop, &result.skyBottom, &result.clouds, &result.lowClouds, &result.topClouds, &result.directional})
        for (const auto value : *values)
            result.valid &= std::isfinite(value) && value >= 0.0f;
    for (const auto* values : {&result.filter1, &result.filter2})
        for (const auto value : *values)
            result.valid &= std::isfinite(value) && value >= 0.0f;
    return result;
}

class WeatherModule
{
public:
    WeatherModule()
    {
        WFP::onInitEvent() += []()
        {
            if (Settings.visualStyle == VisualStyle::DE)
                return;
            auto coloursAddress = hook::pattern("F3 0F 10 05 ? ? ? ? F3 0F 10 0D ? ? ? ? F3 0F 59 C2 F3 0F 59 CA F3 0F 11 05 ? ? ? ? F3 0F 10 05 ? ? ? ? F3 0F 59 C2 F3 0F 11 0D ? ? ? ? F3 0F 10 0D").get_first<uint8_t>(4);
            auto nightVisionAddress = hook::pattern("44 88 35 ? ? ? ? 44 88 35 ? ? ? ? 48 8B 87 F0 05 00 00 44 88 35").get_first<uint8_t>(3);
            auto infraredAddress = hook::pattern("44 88 35 ? ? ? ? 48 8B 87 F0 05 00 00 44 88 35 ? ? ? ? 44 88 35").get_first<uint8_t>(3);
            auto darknessAddress = hook::pattern("44 88 35 ? ? ? ? 48 8B 48 08 44 88 35 ? ? ? ? 4C 39 71 48 74 ?").get_first<uint8_t>(3);
            const auto update = hook::pattern("4C 8B DC 55 56 49 8D 6B A1 48 81 EC C8 00 00 00 45 0F 29 4B A8 48 8B 05").get_first();
            if (!update || !coloursAddress || !nightVisionAddress || !infraredAddress || !darknessAddress)
                return;
            colours = reinterpret_cast<uint8_t*>(injector::ReadRelativeOffset(coloursAddress).as_int());
            nightVision = reinterpret_cast<uint8_t*>(injector::ReadRelativeOffset(nightVisionAddress).as_int());
            infrared = reinterpret_cast<uint8_t*>(injector::ReadRelativeOffset(infraredAddress).as_int());
            darkness = reinterpret_cast<uint8_t*>(injector::ReadRelativeOffset(darknessAddress).as_int());
            updateHook = safetyhook::create_inline(update, Update);
            WFP::onGameInitEvent() += []() { weatherReady = false; };
        };
    }
} WeatherModuleInstance;
