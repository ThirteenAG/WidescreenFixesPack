module;

#include "stdafx.h"
#include "FixSettings.hpp"

export module Movies;

import ComVars;

export void InstallMovieLayout()
{
    auto movie = hook::pattern("55 8B EC 83 E4 F8 83 EC 6C 53 56 8B F1 8A 46 24 84 C0 57 0F 85 ? ? ? ? 8A 46 25 84 C0 0F 84 ? ? ? ? A1 ? ? ? ? 50 FF 15");
    auto draw = hook::pattern("8B CB F3 0F 11 6C 24 4C F3 0F 11 5C 24 3C F3 0F 11 44 24 78 F3 0F 11 4C 24 7C E8 ? ? ? ? 5F 5E 5B 8B E5 5D C2 04 00");
    movieDecoder.SetAddress(*movie.get_first<uint32_t**>(37));
    static auto layout = safetyhook::create_mid(draw.get_first(26), [](SafetyHookContext& context)
    {
        const auto video = movieDecoder.get();
        if (!video || !video[0] || !video[1] || Screen.Width <= 0 || Screen.Height <= 0)
            return;
        static const auto& mode = FixSettings().Int("MAIN.FMVWidescreenMode");
        // The whole movie in its own aspect: Original fits it into the game's 4:3 frame, Fill into the whole screen
        const float frameWidth = mode == 1 ? Screen.fWidth : std::min(Screen.fWidth, Screen.fHeight * 4.0f / 3.0f);
        const float frameHeight = mode == 1 ? Screen.fHeight : std::min(Screen.fHeight, Screen.fWidth * 0.75f);
        const float scale = std::min(frameWidth / static_cast<float>(video[0]), frameHeight / static_cast<float>(video[1]));
        const float width = video[0] * scale, height = video[1] * scale;
        const float left = (Screen.fWidth - width) * 0.5f;
        const float top = (Screen.fHeight - height) * 0.5f;
        // The movie player has already drawn the full black background. Preserve
        // its texture, UV coordinates, colour and decoder/completion handling.
        auto points = reinterpret_cast<float**>(context.esp + 4);
        points[0][0] = left;
        points[0][1] = top;
        points[1][0] = left;
        points[1][1] = top + height;
        points[2][0] = left + width;
        points[2][1] = top + height;
        points[3][0] = left + width;
        points[3][1] = top;
    });
}

