module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module Movies;

import ComVars;

class Movies
{
public:
    // The movie's whole picture: in the 4:3 frame as the game shows it, or as large as the screen allows
    static void Fit(SafetyHookContext& ctx)
    {
        const auto movie = *reinterpret_cast<uint8_t**>(ctx.ebp - 0x1C);
        const float movieWidth = static_cast<float>(*reinterpret_cast<int32_t*>(movie + 0x128));
        const float movieHeight = static_cast<float>(*reinterpret_cast<int32_t*>(movie + 0x12C));
        float aspect = movieWidth > 0.0f && movieHeight > 0.0f ? movieWidth / movieHeight : 4.0f / 3.0f;
        // Up to this shape the game shows a movie over the whole 4:3 frame
        const bool fullFrame = aspect <= 1.4285715f;
        if (fullFrame)
            aspect = 4.0f / 3.0f;

        float boxLeft = Screen.fBoxLeft, boxTop = Screen.fBoxTop, boxWidth = Screen.fBoxWidth, boxHeight = Screen.fBoxHeight;
        if (std::clamp(FixSettings().Int("MAIN.FMVWidescreenMode"), 0, 1) == 1)
        {
            boxLeft = 0.0f;
            boxTop = 0.0f;
            boxWidth = Screen.fWidth;
            boxHeight = Screen.fHeight;
        }
        const float width = std::min(boxWidth, boxHeight * aspect);
        const float height = width / aspect;
        const float left = std::round(boxLeft + (boxWidth - width) / 2.0f);
        const float top = std::round(boxTop + (boxHeight - height) / 2.0f);
        const float right = left + std::round(width);
        const float bottom = top + std::round(height);

        auto vertex = reinterpret_cast<float*>(ctx.ebp - 0x90);
        constexpr int Stride = 7;
        vertex[0 * Stride + 0] = left;
        vertex[0 * Stride + 1] = top;
        vertex[1 * Stride + 0] = right;
        vertex[1 * Stride + 1] = top;
        vertex[2 * Stride + 0] = left;
        vertex[2 * Stride + 1] = bottom;
        vertex[3 * Stride + 0] = right;
        vertex[3 * Stride + 1] = bottom;
    }

    Movies()
    {
        WFP::onInitEvent() += []()
        {
            // FMVs
            auto pattern = hook::pattern("6A 1C 8D 85 70 FF FF FF 50 6A 02 6A 05 8B 0D");
            static auto movieHook = safetyhook::create_mid(pattern.get_first(), Fit);
        };
    }
} Movies;
