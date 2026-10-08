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
    int32_t* skyTopR = nullptr;
    int32_t* skyTopG = nullptr;
    int32_t* skyTopB = nullptr;
    int32_t* skyBottomR = nullptr;
    int32_t* skyBottomG = nullptr;
    int32_t* skyBottomB = nullptr;
    float* ambientR = nullptr;
    float* ambientG = nullptr;
    float* ambientB = nullptr;
    int32_t* cloudR = nullptr;
    int32_t* cloudG = nullptr;
    int32_t* cloudB = nullptr;
    float* blurR = nullptr;
    float* blurG = nullptr;
    float* blurB = nullptr;
    int32_t* lowCloudR = nullptr;
    int32_t* lowCloudG = nullptr;
    int32_t* lowCloudB = nullptr;
    int32_t* topCloudR = nullptr;
    int32_t* topCloudG = nullptr;
    int32_t* topCloudB = nullptr;
    float* directionalR = nullptr;
    float* directionalG = nullptr;
    float* directionalB = nullptr;

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
    result.ambient = {static_cast<float>(*ambientR), static_cast<float>(*ambientG), static_cast<float>(*ambientB)};
    result.skyTop = {static_cast<float>(*skyTopR) / 255.0f, static_cast<float>(*skyTopG) / 255.0f, static_cast<float>(*skyTopB) / 255.0f};
    result.skyBottom = {static_cast<float>(*skyBottomR) / 255.0f, static_cast<float>(*skyBottomG) / 255.0f, static_cast<float>(*skyBottomB) / 255.0f};
    result.clouds = {static_cast<float>(*cloudR) / 255.0f, static_cast<float>(*cloudG) / 255.0f, static_cast<float>(*cloudB) / 255.0f};
    result.lowClouds = {static_cast<float>(*lowCloudR) / 255.0f, static_cast<float>(*lowCloudG) / 255.0f, static_cast<float>(*lowCloudB) / 255.0f};
    result.topClouds = {static_cast<float>(*topCloudR) / 255.0f, static_cast<float>(*topCloudG) / 255.0f, static_cast<float>(*topCloudB) / 255.0f};
    result.directional = {static_cast<float>(*directionalR), static_cast<float>(*directionalG), static_cast<float>(*directionalB)};
    result.filter1 = {*blurR, *blurG, *blurB, 5.0f};
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
            auto skyTopRAddress = hook::pattern("89 35 ? ? ? ? F3 0F 2A C0 0F B6 04 0F 49 8D 8D 90 15 12 05 F3 0F 2A").get_first<uint8_t>(2);
            auto skyTopGAddress = hook::pattern("44 89 35 ? ? ? ? F3 0F 2A D0 0F B6 04 3A 0F 57 C0 0F 57 C9 F3 0F 2A").get_first<uint8_t>(3);
            auto skyTopBAddress = hook::pattern("44 89 3D ? ? ? ? F3 0F 2A C0 42 0F B6 04 11 49 8D 8D 70 11 12 05 49").get_first<uint8_t>(3);
            auto skyBottomRAddress = hook::pattern("44 89 25 ? ? ? ? F3 0F 2A C0 42 0F B6 04 11 F3 0F 2A C8 42 0F B6 04").get_first<uint8_t>(3);
            auto skyBottomGAddress = hook::pattern("44 89 2D ? ? ? ? F3 0F 2A C0 F3 0F 59 D4 42 0F B6 04 11 0F 57 C9 F3").get_first<uint8_t>(3);
            auto skyBottomBAddress = hook::pattern("89 05 ? ? ? ? 48 8D 05 ? ? ? ? 48 8D 88 E0 09 12 05 48 8D 90 E0").get_first<uint8_t>(2);
            auto ambientRAddress = hook::pattern("F3 0F 10 05 ? ? ? ? F3 0F 10 0D ? ? ? ? F3 0F 11 05 ? ? ? ? F3 0F 10 05 ? ? ? ? F3 0F 11 0D ? ? ? ?").get_first<uint8_t>(4);
            auto ambientGAddress = hook::pattern("F3 0F 10 0D ? ? ? ? F3 0F 11 05 ? ? ? ? F3 0F 10 05 ? ? ? ? F3 0F 11 0D ? ? ? ? F3 0F 10 0D ? ? ? ? F3 0F 11 05 ? ? ? ? F3 0F 10 05 ? ? ? ?").get_first<uint8_t>(4);
            auto ambientBAddress = hook::pattern("F3 0F 10 05 ? ? ? ? F3 0F 11 0D ? ? ? ? F3 0F 10 0D ? ? ? ? F3 0F 11 05 ? ? ? ? F3 0F 10 05 ? ? ? ?").get_first<uint8_t>(4);
            auto cloudRAddress = hook::pattern("89 05 ? ? ? ? 0F B6 04 0F 0F 57 C0 0F 57 C9 0F 57 D2 F3 0F 2A D0 0F").get_first<uint8_t>(2);
            auto cloudGAddress = hook::pattern("89 05 ? ? ? ? 0F B6 04 0F F3 0F 2A D0 0F B6 04 17 F3 0F 2A C0 41 0F B6 04 0A 49 8D 89 90 24").get_first<uint8_t>(2);
            auto cloudBAddress = hook::pattern("89 05 ? ? ? ? 0F B6 04 0F F3 0F 2A C8 0F B6 04 17 F3 0F 2A C0 41 0F").get_first<uint8_t>(2);
            auto blurRAddress = hook::pattern("F3 0F 11 05 ? ? ? ? 0F B6 04 17 0F 57 C0 F3 0F 59 CC F3 0F 2A C0 41 0F B6 04 0A 49 8D 89 60").get_first<uint8_t>(4);
            auto blurGAddress = hook::pattern("F3 0F 11 05 ? ? ? ? 0F 57 C0 F3 0F 2A C0 41 0F B6 04 0A 49 8D 89 10").get_first<uint8_t>(4);
            auto blurBAddress = hook::pattern("F3 0F 11 05 ? ? ? ? 0F 57 C0 F3 0F 2A C0 41 0F B6 04 0A 49 8D 89 C0").get_first<uint8_t>(4);
            auto lowCloudRAddress = hook::pattern("89 05 ? ? ? ? 0F B6 04 0F F3 0F 2A D0 0F B6 04 17 F3 0F 2A C0 41 0F B6 04 0A 49 8D 89 80 1E").get_first<uint8_t>(2);
            auto lowCloudGAddress = hook::pattern("89 05 ? ? ? ? 0F B6 04 0F F3 0F 2A D0 0F B6 04 17 F3 0F 2A C0 41 0F B6 04 0A 49 8D 89 F0 25").get_first<uint8_t>(2);
            auto lowCloudBAddress = hook::pattern("89 05 ? ? ? ? 0F B6 04 0F F3 0F 2A D0 0F B6 04 17 F3 0F 2A C0 41 0F B6 04 0A F3 0F 2A C8 41").get_first<uint8_t>(2);
            auto topCloudRAddress = hook::pattern("89 05 ? ? ? ? 0F 57 C9 0F B6 04 0F F3 0F 2A D0 0F B6 04 17 F3 0F 2A").get_first<uint8_t>(2);
            auto topCloudGAddress = hook::pattern("89 05 ? ? ? ? 0F B6 04 0F F3 0F 2A D0 0F B6 04 17 F3 0F 2A C0 41 0F B6 04 0A 49 8D 89 00 28").get_first<uint8_t>(2);
            auto topCloudBAddress = hook::pattern("89 05 ? ? ? ? 0F B6 04 0F F3 0F 2A D0 0F B6 04 17 F3 0F 2A C0 41 0F B6 04 0A 49 8D 89 30 23").get_first<uint8_t>(2);
            auto directionalRAddress = hook::pattern("F3 0F 10 05 ? ? ? ? F3 0F 10 0D ? ? ? ? F3 0F 11 05 ? ? ? ? F3 0F 10 05 ? ? ? ? F3 0F 11 05 ? ? ? ?").get_first<uint8_t>(4);
            auto directionalGAddress = hook::pattern("F3 0F 10 0D ? ? ? ? F3 0F 11 05 ? ? ? ? F3 0F 10 05 ? ? ? ? F3 0F 11 05 ? ? ? ? F3 0F 11 0D ? ? ? ? 75 ? 48 8D 0D ? ? ?").get_first<uint8_t>(4);
            auto directionalBAddress = hook::pattern("F3 0F 10 05 ? ? ? ? F3 0F 11 05 ? ? ? ? F3 0F 11 0D ? ? ? ? 75 ? 48 8D 0D ? ? ?").get_first<uint8_t>(4);
            const auto update = hook::pattern("48 8B C4 48 89 58 08 48 89 70 10 48 89 78 18 55 41 54 41 55 41 56 41 57 48 8D 6C 24 A0 48 81 EC 60 01 00 00 0F 29 70 C8 0F 29 78 B8 44 0F 29 40 A8 44 0F 29 48 98 44 0F 29 50 88 44 0F 29 98 78 FF FF FF 44 0F 29 A0 68 FF FF FF 44 0F 29 A8 58 FF FF FF 44 0F 29 B0 48 FF FF FF 44 0F 29 B8 38 FF FF FF 48 8B 05 ? ? ? ? 48 33 C4 48 89 45 B0 0F B6 05 ? ? ? ?").get_first();
            if (!update || !skyTopRAddress || !skyTopGAddress || !skyTopBAddress || !skyBottomRAddress || !skyBottomGAddress || !skyBottomBAddress || !ambientRAddress || !ambientGAddress || !ambientBAddress || !cloudRAddress || !cloudGAddress || !cloudBAddress || !blurRAddress || !blurGAddress || !blurBAddress || !lowCloudRAddress || !lowCloudGAddress || !lowCloudBAddress || !topCloudRAddress || !topCloudGAddress || !topCloudBAddress || !directionalRAddress || !directionalGAddress || !directionalBAddress)
                return;
            skyTopR = reinterpret_cast<int32_t*>(injector::ReadRelativeOffset(skyTopRAddress).as_int());
            skyTopG = reinterpret_cast<int32_t*>(injector::ReadRelativeOffset(skyTopGAddress).as_int());
            skyTopB = reinterpret_cast<int32_t*>(injector::ReadRelativeOffset(skyTopBAddress).as_int());
            skyBottomR = reinterpret_cast<int32_t*>(injector::ReadRelativeOffset(skyBottomRAddress).as_int());
            skyBottomG = reinterpret_cast<int32_t*>(injector::ReadRelativeOffset(skyBottomGAddress).as_int());
            skyBottomB = reinterpret_cast<int32_t*>(injector::ReadRelativeOffset(skyBottomBAddress).as_int());
            ambientR = reinterpret_cast<float*>(injector::ReadRelativeOffset(ambientRAddress).as_int());
            ambientG = reinterpret_cast<float*>(injector::ReadRelativeOffset(ambientGAddress).as_int());
            ambientB = reinterpret_cast<float*>(injector::ReadRelativeOffset(ambientBAddress).as_int());
            cloudR = reinterpret_cast<int32_t*>(injector::ReadRelativeOffset(cloudRAddress).as_int());
            cloudG = reinterpret_cast<int32_t*>(injector::ReadRelativeOffset(cloudGAddress).as_int());
            cloudB = reinterpret_cast<int32_t*>(injector::ReadRelativeOffset(cloudBAddress).as_int());
            blurR = reinterpret_cast<float*>(injector::ReadRelativeOffset(blurRAddress).as_int());
            blurG = reinterpret_cast<float*>(injector::ReadRelativeOffset(blurGAddress).as_int());
            blurB = reinterpret_cast<float*>(injector::ReadRelativeOffset(blurBAddress).as_int());
            lowCloudR = reinterpret_cast<int32_t*>(injector::ReadRelativeOffset(lowCloudRAddress).as_int());
            lowCloudG = reinterpret_cast<int32_t*>(injector::ReadRelativeOffset(lowCloudGAddress).as_int());
            lowCloudB = reinterpret_cast<int32_t*>(injector::ReadRelativeOffset(lowCloudBAddress).as_int());
            topCloudR = reinterpret_cast<int32_t*>(injector::ReadRelativeOffset(topCloudRAddress).as_int());
            topCloudG = reinterpret_cast<int32_t*>(injector::ReadRelativeOffset(topCloudGAddress).as_int());
            topCloudB = reinterpret_cast<int32_t*>(injector::ReadRelativeOffset(topCloudBAddress).as_int());
            directionalR = reinterpret_cast<float*>(injector::ReadRelativeOffset(directionalRAddress).as_int());
            directionalG = reinterpret_cast<float*>(injector::ReadRelativeOffset(directionalGAddress).as_int());
            directionalB = reinterpret_cast<float*>(injector::ReadRelativeOffset(directionalBAddress).as_int());
            updateHook = safetyhook::create_inline(update, Update);
            WFP::onGameInitEvent() += []() { weatherReady = false; };
        };
    }
} WeatherModuleInstance;
