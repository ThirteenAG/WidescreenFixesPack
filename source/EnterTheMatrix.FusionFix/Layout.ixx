module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module Layout;

import ComVars;

// The game draws its 2D in a 640x448 frame that it stretches over the whole screen. Every 2D command is moved into a
// centred 4:3 frame (letterboxed on screens narrower than 4:3), and while the gameplay HUD draws, elements near a
// side of the 4:3 frame move out to that side of the HUD box.
export namespace Layout
{
    constexpr float FrameWidth = 640.0f;
    constexpr float FrameHeight = 448.0f;

    enum class Context { Normal, World, Mask };

    int Raw = 0;                // > 0: the coordinates are already final
    int Group = 0;              // > 0: inside one element (a string, a bar); its parts share one shift
    float Shift = 0.0f;         // pixels the current element moves sideways
    bool InHud = false;
    Context Current = Context::Normal;

    int32_t ToX(float pixels)
    {
        return static_cast<int32_t>(std::lround(pixels * FrameWidth / Screen.fWidth));
    }

    int32_t ToY(float pixels)
    {
        return static_cast<int32_t>(std::lround(pixels * FrameHeight / Screen.fHeight));
    }

    float PixelX(float x, float shift)
    {
        return Screen.fBoxLeft + shift + x * Screen.fBoxWidth / FrameWidth;
    }

    float PixelY(float y)
    {
        return Screen.fBoxTop + y * Screen.fBoxHeight / FrameHeight;
    }

    int32_t X(float x, float shift)
    {
        return ToX(PixelX(x, shift));
    }

    int32_t Y(float y)
    {
        return ToY(PixelY(y));
    }

    int32_t Width(float w)
    {
        return static_cast<int32_t>(std::lround(w * Screen.fBoxWidth / Screen.fWidth));
    }

    int32_t Height(float h)
    {
        return static_cast<int32_t>(std::lround(h * Screen.fBoxHeight / Screen.fHeight));
    }

    // The shift of a HUD element by its centre in the 640-wide frame: the outer thirds belong to the sides
    float Classify(float center)
    {
        if (!InHud || Current != Context::Normal)
            return 0.0f;
        if (center < FrameWidth / 3.0f)
            return -Screen.fHudShift;
        if (center > FrameWidth * 2.0f / 3.0f)
            return Screen.fHudShift;
        return 0.0f;
    }

    void Update()
    {
        Screen.fWidth = static_cast<float>(Screen.nWidth);
        Screen.fHeight = static_cast<float>(Screen.nHeight);
        Screen.fAspectRatio = Screen.fWidth / Screen.fHeight;
        Screen.fBoxWidth = std::min(Screen.fWidth, Screen.fHeight * (4.0f / 3.0f));
        Screen.fBoxHeight = Screen.fBoxWidth * (3.0f / 4.0f);
        Screen.fBoxLeft = (Screen.fWidth - Screen.fBoxWidth) / 2.0f;
        Screen.fBoxTop = (Screen.fHeight - Screen.fBoxHeight) / 2.0f;

        float hudWidth = Screen.fWidth;
        if (auto ratio = ParseWidescreenHudOffset(FixSettings().String("MAIN.HudAspectRatioConstraint")))
            hudWidth = std::min(Screen.fWidth, Screen.fHeight * ClampHudAspectRatio(*ratio, Screen.fAspectRatio));
        Screen.fHudShift = std::max(0.0f, (hudWidth - Screen.fBoxWidth) / 2.0f);
    }
}
