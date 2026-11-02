module;

#include "stdafx.h"

export module ComVars;

export struct Screen
{
    int Width;
    int Height;
    float fWidth;
    float fHeight;
    float fFieldOfView;
    float fAspectRatio;
    int Width43;
    float fWidth43;
    float fHudScale;
} Screen;

export GameRef<uint8_t> wideScreen;
export GameRef<uint32_t*> movieDecoder;
