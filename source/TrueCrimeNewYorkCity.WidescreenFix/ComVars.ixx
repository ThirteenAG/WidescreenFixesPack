module;

#include "stdafx.h"
#include <queue>
#include <functional>
#include <unordered_set>

export module ComVars;

export struct Screen
{
    int32_t Width;
    int32_t Height;
    float fWidth;
    float fHeight;
    float fFieldOfView;
    float fAspectRatio;
    int32_t Width43;
    float fWidth43;
    float fHudOffset;
    std::optional<float> fHudAspectRatioConstraint;
    float fWidescreenHudOffset;
    float fWidescreenHudOffsetPhone;
} Screen;
