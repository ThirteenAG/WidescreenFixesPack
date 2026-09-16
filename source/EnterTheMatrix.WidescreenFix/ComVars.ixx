module;

#include "stdafx.h"

export module ComVars;

export struct Screen
{
    int32_t nWidth;
    int32_t nHeight;
    float fWidth;
    float fHeight;
    float fFieldOfView;
    float fAspectRatio;
    float fCustomAspectRatioHor;
    float fCustomAspectRatioVer;
    int32_t nWidth43;
    float fWidth43;
    float fHudScale;
    float fHudOffset;
    float fHudOffsetReal;
    float fCutOffArea;
    float fFMVScale;
    float fFMVOffset;
    std::optional<float> fHudAspectRatioConstraint;
    float fWidescreenHudOffset;
    int32_t nWidescreenHudOffset;
} Screen;