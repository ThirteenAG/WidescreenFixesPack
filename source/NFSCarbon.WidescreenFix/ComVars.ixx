module;

#include "stdafx.h"

export module ComVars;

export struct Screen
{
    int32_t Width;
    int32_t Height;
    int32_t Width43;
    float fWidth;
    float fHeight;
    float fAspectRatio;
    float fHudScaleX;
    float fHudPosX;
} Screen;

export struct bVector4
{
    float x;
    float y;
    float z;
    float w;
};

export struct bMatrix4
{
    bVector4 v0;
    bVector4 v1;
    bVector4 v2;
    bVector4 v3;
};

export int SimRate = -1;
export bool bHUDWidescreenMode = true;
