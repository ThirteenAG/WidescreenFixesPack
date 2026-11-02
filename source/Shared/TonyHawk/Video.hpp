#pragma once

namespace TonyVideo
{
    struct Box
    {
        float x, y, width, height;
    };

    inline Box Layout(float width, float height, float movieWidth, float movieHeight, int mode)
    {
        if (movieWidth <= 0.0f || movieHeight <= 0.0f)
            return { 0.0f, 0.0f, width, height };
        // The whole movie between bars: 0 inside the game's centred 4:3 area, otherwise inside the whole screen
        const float areaWidth = mode == 0 ? std::min(width, height * 4.0f / 3.0f) : width;
        const float areaHeight = mode == 0 ? std::min(height, width * 3.0f / 4.0f) : height;
        const float scale = std::min(areaWidth / movieWidth, areaHeight / movieHeight);
        const float boxWidth = movieWidth * scale;
        const float boxHeight = movieHeight * scale;
        return { (width - boxWidth) * 0.5f, (height - boxHeight) * 0.5f, boxWidth, boxHeight };
    }
}
