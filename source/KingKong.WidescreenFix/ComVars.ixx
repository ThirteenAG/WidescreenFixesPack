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

export int bHideUntexturedObjects = false;
export bool bDisableBlur = true;
export bool bVSync = true;
export bool wasUsingLeftStick = false;

export GameRef<float> aMouseX;
export GameRef<float> aMouseY;
export void* pSettings = nullptr;
export uint32_t gCurrentLevel = 0;
export float* fTimeStep = nullptr;