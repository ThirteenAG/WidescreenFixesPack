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
    uint8_t* hours = nullptr;
    uint8_t* minutes = nullptr;
    float* weatherInterpolation = nullptr;
    int16_t* oldWeather = nullptr;
    int16_t* newWeather = nullptr;
    int32_t* lowCloudR = nullptr;
    int32_t* lowCloudG = nullptr;
    int32_t* lowCloudB = nullptr;
    int32_t* topCloudR = nullptr;
    int32_t* topCloudG = nullptr;
    int32_t* topCloudB = nullptr;
    float* directionalR = nullptr;
    float* directionalG = nullptr;
    float* directionalB = nullptr;
    float* blurR = nullptr;
    float* blurG = nullptr;
    float* blurB = nullptr;
    float* blurA = nullptr;

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
    // DE removed III's motion-blur call, but still loads its original four
    // filter tables. Use the same hourly/weather blend as CTimeCycle::Update.
    const int hour = *hours % 24;
    const int next = (hour + 1) % 24;
    const int old = *oldWeather;
    const int current = *newWeather;
    if (old < 0 || old >= 4 || current < 0 || current >= 4)
        return {};
    const float time = std::clamp(*minutes / 60.0f, 0.0f, 1.0f);
    const float weather = std::clamp(*weatherInterpolation, 0.0f, 1.0f);
    const auto blend = [&](const float* table)
    {
        const auto oldColour = std::lerp(table[hour * 4 + old], table[next * 4 + old], time);
        const auto newColour = std::lerp(table[hour * 4 + current], table[next * 4 + current], time);
        return std::lerp(oldColour, newColour, weather);
    };
    result.filter1 = {blend(blurR), blend(blurG), blend(blurB), blend(blurA)};
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
            auto skyTopRAddress = hook::pattern("44 89 3D ? ? ? ? F3 41 0F 2A 94 8E 70 5F 04 05 F3 0F 59 CD F3 41 0F").get_first<uint8_t>(3);
            auto skyTopGAddress = hook::pattern("44 89 25 ? ? ? ? F3 41 0F 2A 94 8E F0 5D 04 05 F3 0F 59 CD F3 41 0F").get_first<uint8_t>(3);
            auto skyTopBAddress = hook::pattern("44 89 2D ? ? ? ? F3 41 0F 2A 94 8E 70 5C 04 05 F3 41 0F 59 D2 F3 0F").get_first<uint8_t>(3);
            auto skyBottomRAddress = hook::pattern("89 15 ? ? ? ? F3 41 0F 2A 94 8E F0 5A 04 05 F3 0F 59 CD F3 41 0F 59").get_first<uint8_t>(2);
            auto skyBottomGAddress = hook::pattern("89 1D ? ? ? ? F3 41 0F 2A 94 8E 70 71 04 05 F3 0F 59 CD F3 41 0F 59").get_first<uint8_t>(2);
            auto skyBottomBAddress = hook::pattern("89 05 ? ? ? ? F3 41 0F 2A 94 8E 70 59 04 05 F3 0F 59 CD F3 41 0F 59").get_first<uint8_t>(2);
            auto ambientRAddress = hook::pattern("F3 0F 10 15 ? ? ? ? F3 0F 10 0D ? ? ? ? F3 0F 10 05 ? ? ? ? F3 0F 10 1D ? ? ? ? F3 0F 11 15 ? ? ? ?").get_first<uint8_t>(4);
            auto ambientGAddress = hook::pattern("F3 0F 10 0D ? ? ? ? F3 0F 10 05 ? ? ? ? F3 0F 10 1D ? ? ? ? F3 0F 11 15 ? ? ? ?").get_first<uint8_t>(4);
            auto ambientBAddress = hook::pattern("F3 0F 10 05 ? ? ? ? F3 0F 10 1D ? ? ? ? F3 0F 11 15 ? ? ? ?").get_first<uint8_t>(4);
            auto cloudRAddress = hook::pattern("89 05 ? ? ? ? F3 41 0F 2A 94 8E B0 7C 04 05 F3 0F 59 CD F3 41 0F 59").get_first<uint8_t>(2);
            auto cloudGAddress = hook::pattern("89 05 ? ? ? ? F3 41 0F 59 D2 F3 0F 58 D0 0F 57 C9 0F 57 C0 F3 43 0F").get_first<uint8_t>(2);
            auto cloudBAddress = hook::pattern("89 05 ? ? ? ? 45 85 F6 74 ? 66 41 0F 6E C6 0F 5B C0 F3 0F 59 05 ?").get_first<uint8_t>(2);
            auto hoursAddress = hook::pattern("44 0F B6 0D ? ? ? ? 44 03 C1 41 83 F8 3C 7C ? B8 89 88 88 88 41 F7").get_first<uint8_t>(4);
            auto minutesAddress = hook::pattern("44 0F B6 05 ? ? ? ? 44 0F B6 0D ? ? ? ? 44 03 C1 41 83 F8 3C 7C").get_first<uint8_t>(4);
            auto weatherInterpolationAddress = hook::pattern("F3 0F 11 05 ? ? ? ? F3 0F 59 05 ? ? ? ? 66 89 05 ? ? ? ? F3 0F 11 0D ? ? ? ? 89").get_first<uint8_t>(4);
            auto oldWeatherAddress = hook::pattern("66 89 05 ? ? ? ? 0F B7 05 ? ? ? ? F3 0F 11 05 ? ? ? ? F3 0F 59 05 ? ? ? ? 66 89 05 ? ? ? ? F3 0F 11").get_first<uint8_t>(3);
            auto newWeatherAddress = hook::pattern("66 89 05 ? ? ? ? F3 0F 11 0D ? ? ? ? 89 0D ? ? ? ? F3 0F 2C").get_first<uint8_t>(3);
            auto lowCloudRAddress = hook::pattern("89 05 ? ? ? ? F3 41 0F 2A 94 8E B0 85 04 05 F3 0F 59 CD F3 41 0F 59").get_first<uint8_t>(2);
            auto lowCloudGAddress = hook::pattern("89 05 ? ? ? ? F3 41 0F 2A 94 8E 30 84 04 05 F3 0F 59 CD F3 41 0F 59").get_first<uint8_t>(2);
            auto lowCloudBAddress = hook::pattern("89 05 ? ? ? ? F3 41 0F 2A 94 8E B0 82 04 05 F3 0F 59 CD F3 41 0F 59").get_first<uint8_t>(2);
            auto topCloudRAddress = hook::pattern("89 05 ? ? ? ? F3 41 0F 2A 94 8E 30 81 04 05 F3 0F 59 CD F3 41 0F 59").get_first<uint8_t>(2);
            auto topCloudGAddress = hook::pattern("89 05 ? ? ? ? F3 41 0F 2A 94 8E B0 7F 04 05 F3 0F 59 CD F3 41 0F 59").get_first<uint8_t>(2);
            auto topCloudBAddress = hook::pattern("89 05 ? ? ? ? F3 41 0F 2A 94 8E 30 7E 04 05 F3 0F 59 CD F3 41 0F 59").get_first<uint8_t>(2);
            auto directionalRAddress = hook::pattern("F3 0F 10 05 ? ? ? ? F3 0F 10 0D ? ? ? ? F3 0F 11 05 ? ? ? ? F3 0F 10 05 ? ? ? ?").get_first<uint8_t>(4);
            auto directionalGAddress = hook::pattern("F3 0F 10 0D ? ? ? ? F3 0F 11 05 ? ? ? ? F3 0F 10 05 ? ? ? ? F3 0F 11 05 ? ? ? ? F3 0F 11 0D ? ? ? ? 75 ? 48 8D 0D ? ? ?").get_first<uint8_t>(4);
            auto directionalBAddress = hook::pattern("F3 0F 10 05 ? ? ? ? F3 0F 11 05 ? ? ? ? F3 0F 11 0D ? ? ? ? 75 ? 48 8D 0D ? ? ?").get_first<uint8_t>(4);
            auto blurRAddress = hook::pattern("F3 41 0F 11 84 8C B0 51 04 05 F3 0F 10 45 14 41 89 84 8C B0 7C 04 05 8B").get_first<uint8_t>(6);
            auto blurGAddress = hook::pattern("F3 41 0F 11 84 8C 30 50 04 05 F3 0F 10 45 18 49 83 C6 04 F3 41 0F 11 84").get_first<uint8_t>(6);
            auto blurBAddress = hook::pattern("F3 41 0F 11 84 8C B0 4E 04 05 F3 0F 10 45 1C F3 41 0F 11 84 8C 30 4D 04").get_first<uint8_t>(6);
            auto blurAAddress = hook::pattern("F3 41 0F 11 84 8C 30 4D 04 05 49 83 FE 60 0F 8C ? ? ? ? 66 FF C6 66").get_first<uint8_t>(6);
            const auto update = hook::pattern("48 8B C4 48 89 58 08 48 89 70 10 48 89 78 18 55 41 54 41 55 41 56 41 57 48 8D 68 A1 48 81 EC C0 00 00 00 0F 29 70 C8 0F").get_first();
            if (!update || !skyTopRAddress || !skyTopGAddress || !skyTopBAddress || !skyBottomRAddress || !skyBottomGAddress || !skyBottomBAddress || !ambientRAddress || !ambientGAddress || !ambientBAddress || !cloudRAddress || !cloudGAddress || !cloudBAddress || !hoursAddress || !minutesAddress || !weatherInterpolationAddress || !oldWeatherAddress || !newWeatherAddress || !lowCloudRAddress || !lowCloudGAddress || !lowCloudBAddress || !topCloudRAddress || !topCloudGAddress || !topCloudBAddress || !directionalRAddress || !directionalGAddress || !directionalBAddress || !blurRAddress || !blurGAddress || !blurBAddress || !blurAAddress)
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
            hours = reinterpret_cast<uint8_t*>(injector::ReadRelativeOffset(hoursAddress).as_int());
            minutes = reinterpret_cast<uint8_t*>(injector::ReadRelativeOffset(minutesAddress).as_int());
            weatherInterpolation = reinterpret_cast<float*>(injector::ReadRelativeOffset(weatherInterpolationAddress).as_int());
            oldWeather = reinterpret_cast<int16_t*>(injector::ReadRelativeOffset(oldWeatherAddress).as_int());
            newWeather = reinterpret_cast<int16_t*>(injector::ReadRelativeOffset(newWeatherAddress).as_int());
            lowCloudR = reinterpret_cast<int32_t*>(injector::ReadRelativeOffset(lowCloudRAddress).as_int());
            lowCloudG = reinterpret_cast<int32_t*>(injector::ReadRelativeOffset(lowCloudGAddress).as_int());
            lowCloudB = reinterpret_cast<int32_t*>(injector::ReadRelativeOffset(lowCloudBAddress).as_int());
            topCloudR = reinterpret_cast<int32_t*>(injector::ReadRelativeOffset(topCloudRAddress).as_int());
            topCloudG = reinterpret_cast<int32_t*>(injector::ReadRelativeOffset(topCloudGAddress).as_int());
            topCloudB = reinterpret_cast<int32_t*>(injector::ReadRelativeOffset(topCloudBAddress).as_int());
            directionalR = reinterpret_cast<float*>(injector::ReadRelativeOffset(directionalRAddress).as_int());
            directionalG = reinterpret_cast<float*>(injector::ReadRelativeOffset(directionalGAddress).as_int());
            directionalB = reinterpret_cast<float*>(injector::ReadRelativeOffset(directionalBAddress).as_int());
            blurR = reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr)) + injector::ReadMemory<uint32_t>(blurRAddress, true));
            blurG = reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr)) + injector::ReadMemory<uint32_t>(blurGAddress, true));
            blurB = reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr)) + injector::ReadMemory<uint32_t>(blurBAddress, true));
            blurA = reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr)) + injector::ReadMemory<uint32_t>(blurAAddress, true));
            updateHook = safetyhook::create_inline(update, Update);
            WFP::onGameInitEvent() += []() { weatherReady = false; };
        };
    }
} WeatherModuleInstance;
