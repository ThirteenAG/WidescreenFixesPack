module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module Camera;

import ComVars;
import Hud;

export namespace Camera
{
    constexpr float AspectRatio43 = 4.0f / 3.0f;
    constexpr float LetterboxProbe = 1.0f; //any aspect below the cinematic band keeps the letterbox scale above 1
    float CinematicAspect = 2.35f, EffectAspect = 2.35f;
    float NativeHalf = 0.0f, AdjustedHalf = 0.0f;
    SafetyHookMid CutsceneFOVCapture;
    SafetyHookInline CinematicBars, EffectBars, Subtitle;
    void(__cdecl* SetProjection)(float, float) = nullptr;

    struct Picture { float width, height, fit; };

    // The cinematic picture of the given aspect: the director's frame scaled to fit the screen (fit, its width in
    // pixels); letterbox and pillarbox limit the picture to it, without them the picture fills that side of the screen
    inline Picture Frame(float band)
    {
        const int mode = std::clamp(FixSettings().Int("MAIN.CutsceneBorders"), 0, 3);
        const float fit = std::min(Screen.fWidth, Screen.fHeight * band);
        return { (mode & 2) ? fit : Screen.fWidth, (mode & 1) ? std::min(Screen.fHeight, fit / band) : Screen.fHeight, fit };
    }

    inline void Update()
    {
        const float band = CinematicBand.is_initialized() ? CinematicBand.get() : 2.35f;
        CinematicAspect = band * Frame(band).height / Screen.fHeight; //the native bar height formula then gives our bars
    }

    void DrawRect(float x0, float y0, float x1, float y1, uint32_t color);

    inline void SideBars(float band, uint32_t color)
    {
        const float side = (Screen.fWidth - Frame(band).width) / 2.0f;
        if (side < 0.5f)
            return;
        DrawRect(0.0f, 0.0f, side, Screen.fHeight, color);
        DrawRect(Screen.fWidth - side, 0.0f, Screen.fWidth, Screen.fHeight, color);
    }

    // The cutscene camera: letterboxed scenes keep the director's 2.35 frame across the picture, others are Hor+ from 4:3
    void __cdecl Projection(float fov, float scale)
    {
        const float half = fov * 0.5f;
        float adjusted;
        if (scale > 1.0001f)
        {
            const float band = CinematicBand.is_initialized() ? CinematicBand.get() : 2.35f;
            const int mode = std::clamp(FixSettings().Int("MAIN.CutsceneBorders"), 0, 3);
            const float span = mode ? Frame(band).fit : Screen.fHeight * band; //pixels covered by the director's width
            adjusted = atan(tan(half) * Screen.fWidth / span);
        }
        else
            adjusted = atan(tan(half) * Screen.fAspectRatio / AspectRatio43);
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
        const bool replaced = BarsOverride.is_initialized() && BarsOverride.get(); //a camera effect draws its own bars
        const auto result = CinematicBars.unsafe_ccall<int32_t>();
        if (!replaced)
            SideBars(CinematicBand.is_initialized() ? CinematicBand.get() : 2.35f, 0xFF000000);
        return result;
    }

    // Cutscene subtitles keep the native layout across the cinematic frame instead of the HUD's 4:3 canvas
    int32_t __fastcall DrawSubtitle(void* cutscene, void*, const wchar_t* text)
    {
        UI::SubtitleWidth = Frame(CinematicBand.is_initialized() ? CinematicBand.get() : 2.35f).width;
        const auto result = Subtitle.unsafe_thiscall<int32_t>(cutscene, text);
        UI::SubtitleWidth = 0.0f;
        return result;
    }

    int32_t __cdecl DrawEffectBars()
    {
        float band = 0.0f;
        switch (EffectBand.is_initialized() ? EffectBand.get() : 0)
        {
            case 2: band = 2.35f; break;
            case 3: band = 16.0f / 9.0f; break;
            case 4: band = 1.618034f; break;
        }
        if (band > 0.0f)
            EffectAspect = band * Frame(band).height / Screen.fHeight;
        const auto result = EffectBars.unsafe_ccall<int32_t>();
        if (band > 0.0f)
            SideBars(band, uint32_t(EffectAlpha.is_initialized() ? EffectAlpha.get() : 0xFF) << 24);
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
