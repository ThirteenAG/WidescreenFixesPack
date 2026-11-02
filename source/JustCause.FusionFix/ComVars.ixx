module;
#include <stdafx.h>
export module ComVars;

export struct Screen
{
    int32_t nWidth;
    int32_t nHeight;
    float fWidth;
    float fHeight;
    float fFieldOfView;
    float fAspectRatio;
    int32_t nWidth43;
    float fWidth43;
    float fHudScale;
    float fHudOffset;
    float fHudOffsetReal;
    bool bDrawBorders;
    bool bBoxedBackground;
    float fWidescreenOffsetX;
    float fWidescreenOffsetY;
} Screen;

