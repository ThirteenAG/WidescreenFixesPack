module;

#include <stdafx.h>
#include <d3d9.h>
#include <array>

export module ComVars;

export bool bGOG = false;

export struct Screen
{
    int Width;
    int Height;
    float fWidth;
    float fHeight;
    float fFieldOfView;
    float fDynamicScreenFieldOfViewScale;
    float fAspectRatio;
    float fHudScaleX;
    float fHudOffset;
    float fHudOffsetRight;
    float fFMVoffsetStartX;
    float fFMVoffsetEndX;
    float fFMVoffsetStartY;
    float fFMVoffsetEndY;
} Screen;

export union FColor
{
    uint32_t RGBA;
    struct
    {
        uint8_t R, G, B, A;
    };
};

export uintptr_t sub_42B380_addr, sub_42B3A0_addr, sub_42B460_addr, sub_42B610_addr, sub_42B740_addr, sub_42BA20_addr;
export uintptr_t sub_42BBA0_addr, sub_42BD11_addr, sub_42C880_addr, sub_42C8D0_addr, sub_42CA50_addr, sub_42B900_addr;

export int32_t nCutsceneBorders = 3;     // 0 off, 1 letterbox (the original bars), 2 pillarbox, 3 both
export uint32_t nFMVWidescreenMode;
export std::optional<float> fHudAspectRatioConstraint;
export float fWidescreenHudOffset;
export float fFOVFactor = 1.0f;
export GameRef<float> CutsceneBlend;
export bool AnimatedCutsceneBorders = true;
export int32_t nWindowedMode = 0;

export GameRef<std::array<uint8_t, 2>> Paused;
export GameRef<uint8_t> bActive;

export struct PresentParameters
{
    UINT BackBufferWidth, BackBufferHeight;
    D3DFORMAT BackBufferFormat;
    UINT BackBufferCount;
    D3DMULTISAMPLE_TYPE MultiSampleType;
    DWORD MultiSampleQuality;
    D3DSWAPEFFECT SwapEffect;
    HWND hDeviceWindow;
    BOOL Windowed;
    BOOL EnableAutoDepthStencil;
    D3DFORMAT AutoDepthStencilFormat;
    DWORD Flags;
    UINT FullScreen_RefreshRateInHz;
    UINT PresentationInterval;
};
export GameRef<uint32_t> dword_761208;
export GameRef<uint8_t*> dword_820B90;
export GameRef<uint8_t*> dword_81C488;
export GameRef<PresentParameters> dword_8A2620;
export GameRef<const char*> szMenuPageName;

