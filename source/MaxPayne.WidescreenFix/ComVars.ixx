module;

#include "stdafx.h"
#include "dxsdk\dx8\d3d8.h"

export module ComVars;

export struct Screen
{
    int32_t nWidth;
    int32_t nHeight;
    float fWidth;
    float fHeight;
    float fFOVFactor;
    float fFieldOfView;
    float fAspectRatio;
    float fHudScale;
    float fHudOffset;
    float fHudOffsetReal;
    float fWidescreenHudOffset;
    std::optional<float> fHudAspectRatioConstraint;
    float fBorderOffset;
    float fWidthScale;
    float fHalfWidthScale;
    float f1_fWidthScale;
    float fDouble1_fWidthScale;
    float fHalf1_fWidthScale;
    bool bDrawBorders;
    bool bDrawBordersToFillGap;
    bool bDrawBordersForCameraOverlay;
    bool bGraphicNovelMode;
    bool bIsInGraphicNovel;
} Screen;

export struct TextCoords
{
    float a;
    float b;
    float c;
    float d;
};

export typedef HRESULT(STDMETHODCALLTYPE* EndScene_t)(LPDIRECT3DDEVICE8);
export EndScene_t RealEndScene = NULL;
export SafetyHookInline shDllMainHook = {};
export safetyhook::MidHook EndSceneHook = {};

export namespace X_Crosshair
{
    GameRef<bool> sm_bCameraPathRunning;
}