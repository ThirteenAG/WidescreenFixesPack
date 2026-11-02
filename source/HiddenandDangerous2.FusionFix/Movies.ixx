module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <algorithm>

export module Movies;


export struct MovieRect
{
    int x, y, width, height;
};

export inline MovieRect FitMovie(int screenWidth, int screenHeight, unsigned int videoWidth, unsigned int videoHeight, int mode)
{
    const float width = static_cast<float>(screenWidth);
    const float height = static_cast<float>(screenHeight);
    const float boxWidth = mode == 0 ? std::max(1.0f, std::min(width, height * (4.0f / 3.0f)) - 10.0f) : width;
    const float boxHeight = mode == 0 ? std::max(1.0f, std::min(height, width / (4.0f / 3.0f)) - 10.0f) : height;
    const float aspect = static_cast<float>(videoWidth) / static_cast<float>(videoHeight);
    const float drawWidth = std::min(boxWidth, boxHeight * aspect);
    const float drawHeight = drawWidth / aspect;
    const int roundedWidth = static_cast<int>(drawWidth + 0.5f);
    const int roundedHeight = static_cast<int>(drawHeight + 0.5f);
    return { (screenWidth - roundedWidth) / 2, (screenHeight - roundedHeight) / 2, roundedWidth, roundedHeight };
}
