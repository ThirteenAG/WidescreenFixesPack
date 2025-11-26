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
    float fFieldOfView;
    float fAspectRatio;
    float fDiffFactor;
    float fHudScale;
    float fHudOffset;
    float fHudOffset2;
    float fHudOffsetReal;
    float fWidescreenHudOffset;
    std::optional<float> fHudAspectRatioConstraint;
    float fWidthScale;
    float fHalfWidthScale;
    float f1_fWidthScale;
    float fDouble1_fWidthScale;
    float fHalf1_fWidthScale;
    float fMirrorFactor;
    float fViewPortSizeX = 640.0f;
    float fViewPortSizeY = 480.0f;
    float fNovelsScale = 0.003125f;
    float fNovelsOffset = -1.0f;
    bool bIsX_QuadRenderer;
    bool bIsSniperZoomOn;
    bool bIsFading;
    bool bDrawBorders;
    bool bDrawBordersForCameraOverlay;
    bool bIs2D;
    bool bIsSkybox;
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
export safetyhook::MidHook BorderlessWindowedHook = {};

export namespace X_Crosshair
{
    GameRef<bool> sm_bCameraPathRunning;
}