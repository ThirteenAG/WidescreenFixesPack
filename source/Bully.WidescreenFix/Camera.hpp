#pragma once
#include "Cutscene.hpp"

namespace GameplayCamera
{
    uint8_t* Manager = nullptr;
    int32_t* Blending = nullptr;

    float Factor(float value)
    {
        if (!Manager || !Blending)
            return 1.0f;
        const int current = *reinterpret_cast<int32_t*>(Manager + 0x90);
        const int previous = *reinterpret_cast<int32_t*>(Manager + 0x94);
        auto gameplay = [](int mode)
        {
            return mode == 1 || mode == 8; // follow and vehicle controllers
        };
        if (!gameplay(current) || (*Blending && !gameplay(previous)) ||
            *reinterpret_cast<int32_t*>(Manager + 0x298) != 0 || *reinterpret_cast<int32_t*>(Manager + 0x16C) != 0)
            return 1.0f;
        return std::isfinite(value) ? std::clamp(value, 0.5f, 2.0f) : 1.0f;
    }

    float Adjust(float original, float aspect, float factor, bool primary, bool overridden)
    {
        const float value = std::isfinite(original) ? std::clamp(original, 1.0f, 179.0f) : 60.0f;
        if (primary && BullyCutscene::Active())
        {
            const float cutscene = BullyCutscene::FOV(value, aspect);
            const int mode = Manager ? *reinterpret_cast<int*>(Manager + 0x90) : 0;
            if (!overridden && Manager && *reinterpret_cast<int*>(Manager + 164) == 0 && (mode == 1 || mode == 8))
            {
                constexpr float radians = 0.017453292519943295f;
                const float base = std::clamp(AdjustFOV(value, aspect), 1.0f, 179.0f);
                const float gameplay = std::clamp(base * Factor(factor), 1.0f, std::max(base, 150.0f));
                const float phase = BullyCutscene::Calculate(aspect).progress;
                const float tangent = std::lerp(std::tan(cutscene * radians * 0.5f), std::tan(gameplay * radians * 0.5f), phase);
                return std::clamp(2.0f * std::atan(tangent) / radians, 1.0f, 179.0f);
            }
            return cutscene;
        }
        if (primary && overridden)
            return value;
        // A larger multiplier stops at 150 degrees: wider views smear the scene and break clipping
        const float base = std::clamp(AdjustFOV(value, aspect), 1.0f, 179.0f);
        const float scaled = base * (primary ? Factor(factor) : 1.0f);
        return std::clamp(scaled, 1.0f, std::max(base, 150.0f));
    }
}
