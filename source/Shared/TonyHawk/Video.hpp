#pragma once

namespace TonyVideo
{
    struct Box
    {
        float x, y, width, height;
    };

    inline Box Layout(float width, float height, float movieWidth, float movieHeight, int mode)
    {
        if (mode == 2 || movieWidth <= 0.0f || movieHeight <= 0.0f)
            return { 0.0f, 0.0f, width, height };
        const float scale = mode == 0 ? std::min(width / movieWidth, height / movieHeight) :
            std::max(width / movieWidth, height / movieHeight);
        const float boxWidth = movieWidth * scale;
        const float boxHeight = movieHeight * scale;
        return { (width - boxWidth) * 0.5f, (height - boxHeight) * 0.5f, boxWidth, boxHeight };
    }
}
