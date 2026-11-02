module;

#include <stdafx.h>
#include <algorithm>
#include <cmath>
#include <numbers>
#include "FixSettings.hpp"

export module CutsceneFrame;

// Cinematics are framed for a 16:9 picture: the 4:3 view with a 60-pixel bar above and below. MAIN.CutsceneBorders:
// 0 Off: the picture's height fills the screen (narrower screens lose the sides, wider ones show more at the sides)
// 1 Letterbox: below 16:9 the 4:3 width stays and bars cover what the picture didn't show, as the original did
// 2 Pillarbox: above 16:9 bars at the sides keep the picture at 16:9
// 3 Both
export namespace PTCutscene
{
    constexpr float kPictureAspect = 16.0f / 9.0f;

    inline int Mode()
    {
        return std::clamp(FixSettings().Int("MAIN.CutsceneBorders"), 0, 3);
    }

    inline bool Letterbox(float aspect)
    {
        return (Mode() & 1) && aspect < kPictureAspect;
    }

    inline bool Pillarbox(float aspect)
    {
        return (Mode() & 2) && aspect > kPictureAspect;
    }

    // fov: the scene's horizontal field of view for 4:3, in degrees
    inline float FOV(float fov, float aspect)
    {
        if (!(aspect > 0.0f) || !std::isfinite(aspect))
            return fov;
        float half = std::tan(fov * std::numbers::pi_v<float> / 360.0f);
        if (!Letterbox(aspect))
            half *= aspect / kPictureAspect;
        return std::clamp(std::atan(half) * 360.0f / std::numbers::pi_v<float>, 1.0f, 179.0f);
    }

    // Height of each bar on the 640x480 canvas, 0 without bars
    inline float LetterboxHeight(float aspect)
    {
        return Letterbox(aspect) ? std::max(0.0f, (480.0f - 270.0f * aspect) * 0.5f) : 0.0f;
    }

    // Width of each side bar in pixels, 0 without bars
    inline int PillarboxWidth(int width, int height)
    {
        if (width <= 0 || height <= 0 || !Pillarbox(float(width) / float(height)))
            return 0;
        return std::max(0, static_cast<int>(std::floor((float(width) - float(height) * kPictureAspect) * 0.5f)));
    }
}
