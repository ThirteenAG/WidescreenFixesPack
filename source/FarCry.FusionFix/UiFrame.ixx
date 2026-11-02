module;

#include <stdafx.h>
#include <d3d9.h>
#include "FixSettings.hpp"
#include <algorithm>
#include <cmath>

export module UiFrame;


export namespace FarCryUiFrame
{
    struct Scale
    {
        float x = 1.0f, y = 1.0f;
    };

    inline Scale Fit(float aspectScale)
    {
        if (!std::isfinite(aspectScale) || aspectScale <= 0.0f)
            return {};
        return { std::min(1.0f, 1.0f / aspectScale), std::min(1.0f, aspectScale) };
    }
}
