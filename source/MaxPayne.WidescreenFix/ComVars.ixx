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
export safetyhook::MidHook BorderlessWindowedHook = {};

export namespace X_Crosshair
{
    GameRef<bool> sm_bCameraPathRunning;
}

export enum eCurrentGameMode
{
    GameMode,
    MenuMode,
    GraphicNovelMode,
};

export eCurrentGameMode CurrentGameMode = MenuMode;

export namespace MaxPayne_ConfiguredInput
{
    void* ForwardBackward = nullptr;
    void* LeftRight = nullptr;
    void* Shoot = nullptr;
    void* Reload = nullptr;
    void* Jump = nullptr;
    void* Crouch = nullptr;
    void* DodgeLeft = nullptr;
    void* DodgeRight = nullptr;
    void* DodgeForward = nullptr;
    void* DodgeBackward = nullptr;
    void* DodgeModifier = nullptr;
    void* AimUpDown = nullptr;
    void* AimLeftRight = nullptr;
    void* Use = nullptr;
    void* Slot1 = nullptr;
    void* Slot2 = nullptr;
    void* Slot3 = nullptr;
    void* Slot4 = nullptr;
    void* Slot5 = nullptr;
    void* Slot6 = nullptr;
    void* BestWeapon = nullptr;
    void* NextPreviousWeapon = nullptr;
    void* Pause = nullptr;
    void* Painkiller = nullptr;
    void* SniperZoom = nullptr;
    void* SlowMotion = nullptr;
    void* BulletTime = nullptr;
}

export namespace X_Input
{
    void* (*getMouse)() = nullptr;
}