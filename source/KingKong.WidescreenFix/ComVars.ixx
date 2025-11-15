module;

#include "stdafx.h"

export module ComVars;

export struct Screen
{
    int32_t Width;
    int32_t Height;
    float fWidth;
    float fHeight;
    float fFieldOfView;
    float fAspectRatio;
    float fCustomAspectRatioHor;
    float fCustomAspectRatioVer;
    int32_t Width43;
    float fWidth43;
    float fHudScale;
    float fHudOffset;
    float fCutOffArea;
    float fFMVScale;
    bool bSignatureEdition;
    uintptr_t pBarsPtr;
    float fRawInputMouse;
} Screen;

export bool bHideUntexturedObjects = false;
export bool bDisableBlur = true;
