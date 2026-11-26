module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module Camera;

import ComVars;
import Cutscenes;

class Camera
{
    static inline std::atomic<float> Factor{ 1.0f };
    static inline GameRef<uint8_t*> HudState;

    static bool UsesAuthoredZoom()
    {
        const auto* state = *HudState;
        if (!state)
            return false;

        // The native HUD uses camera mode 3 for the player scope and separate flags for scripted scope views.
        return *reinterpret_cast<const int32_t*>(state + 0x6C) == 3 ||
            *reinterpret_cast<const int32_t*>(state + 0x1650) != 0 ||
            *reinterpret_cast<const int32_t*>(state + 0x1654) != 0;
    }

public:
    // The vertical half-angle the main view is set up with, widened so the horizontal field of view grows by the factor
    static void Fov(SafetyHookContext& ctx)
    {
        const float factor = std::clamp(Factor.load(), 0.5f, 2.0f);
        if (Cutscenes::Cinematic || factor == 1.0f || UsesAuthoredZoom())
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
            WatchMatrixNumber("MAIN.FOVFactor", Factor);
            auto pattern = hook::pattern("A1 ? ? ? ? 8B 48 6C 89 4D ? 83 7D ? 05");
            HudState.SetAddress(*pattern.get_first<uint8_t**>(1));
            // Field of view of the main view
            pattern = hook::pattern("8B ? DC 8B ? 40 ? E8 ? ? ? ? 83 C4 0C 8B ? DC ? E8");
            static auto fovHook = safetyhook::create_mid(pattern.get_first(7), Fov);
        };
    }
} Camera;
