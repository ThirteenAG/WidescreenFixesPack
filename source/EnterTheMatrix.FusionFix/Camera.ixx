module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module Camera;

import ComVars;
import Cutscenes;

class Camera
{
public:
    // The vertical half-angle the main view is set up with, widened so the horizontal field of view grows by the factor
    static void Fov(SafetyHookContext& ctx)
    {
        const float factor = std::clamp(FixSettings().Float("MAIN.FOVFactor"), 0.5f, 2.0f);
        if (Cutscenes::Cinematic || factor == 1.0f)
            return;
        auto& halfAngle = *reinterpret_cast<float*>(ctx.esp);
        constexpr float Radians = 3.14159265f / 180.0f;
        const float vertical = std::tan(halfAngle * Radians);
        const float horizontal = std::min(std::atan(vertical * Screen.fAspectRatio) * factor, 75.0f * Radians);
        halfAngle = std::atan(std::tan(horizontal) / Screen.fAspectRatio) / Radians;
    }

    Camera()
    {
        WFP::onInitEvent() += []()
        {
            // Field of view of the main view
            auto pattern = hook::pattern("8B ? DC 8B ? 40 ? E8 ? ? ? ? 83 C4 0C 8B ? DC ? E8");
            static auto fovHook = safetyhook::create_mid(pattern.get_first(7), Fov);
        };
    }
} Camera;
