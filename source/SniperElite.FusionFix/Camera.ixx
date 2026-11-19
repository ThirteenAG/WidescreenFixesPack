module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module Camera;

import ComVars;
import Hud;

export namespace Camera
{
    constexpr float AspectRatio43 = 4.0f / 3.0f;
    float CinematicAspect = AspectRatio43;
    float NativeHalf = 0.0f, AdjustedHalf = 0.0f;
    SafetyHookMid CutsceneFOVCapture;
    SafetyHookInline CinematicBars;
    void(__cdecl* SetProjection)(float, float) = nullptr;

    void DrawRect(float x0, float y0, float x1, float y1, uint32_t color);

    // The cutscene camera always renders full-width Hor+ from the 4:3 reference; the call site's letterbox scale is unused
    void __cdecl Projection(float fov, float /*scale*/)
    {
        const float half = fov * 0.5f;
        const float adjusted = atan(tan(half) * Screen.fAspectRatio / AspectRatio43);
        NativeHalf = half;
        AdjustedHalf = adjusted;
        SetProjection(adjusted * 2.0f, 1.0f);
    }

    // FOV transitions start from the camera's native angle
    void CaptureCutsceneFOV(injector::reg_pack& regs)
    {
        const auto halfAngle = *reinterpret_cast<float*>(regs.eax + 0x4C);
        static float native;
        native = fabsf(halfAngle - AdjustedHalf) < 1e-5f ? NativeHalf : halfAngle;
        regs.eax = reinterpret_cast<uintptr_t>(&native) - 0x4C;
    }

    int32_t __cdecl DrawCinematicBars()
    {
        const bool mode = FixSettings().Bool("MAIN.CutsceneBorders");
        CinematicAspect = mode ? AspectRatio43 : 0.0f; //4:3 cinematic bars; Off collapses them to null

        const auto result = CinematicBars.unsafe_ccall<int32_t>();
        return result;
    }
}

void Camera::DrawRect(float x0, float y0, float x1, float y1, uint32_t color)
{
    if (!QueueHudQuad || !UI::FlushQuads)
        return;
    HudQuad quad = { { x0, x1, x1, x0 }, { y0, y0, y1, y1 } };
    std::fill(std::begin(quad.color), std::end(quad.color), color);
    quad.texture = -1;
    quad.blend = 1;
    quad.state[0] = quad.state[1] = 1;
    QueueHudQuad(&quad);
    UI::FlushQuads();
}
