module;

#include "stdafx.h"

export module ComVars;

export struct Screen
{
    int32_t nWidth;
    int32_t nHeight;
    int32_t nWidth43;
    float fWidth43;
    float fWidth;
    float fHeight;
    float fAspectRatio;
    float fHudScaleX;
    float fHudPosX;
    float fHudOffsetReal;
} Screen;

export GameRef<int32_t> nGameState;