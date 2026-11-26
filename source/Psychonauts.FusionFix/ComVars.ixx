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
    float fHudCanvasWidth;
    float fHudCanvasHeight;
    float fHudOffsetX;
    float fHudOffsetY;
    float fHudOffsetWide;
} Screen;

export GameRef<void*> engine;
export GameRef<uint8_t*> currentMenu;
