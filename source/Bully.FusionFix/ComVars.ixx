module;

#include <stdafx.h>

export module ComVars;

export GameRef<int> DisplayWidth;
export GameRef<int> DisplayHeight;
export GameRef<int32_t> NativeDisplayModeCount;
export GameRef<uint32_t> dword_BCBC68;
export GameRef<uint32_t> dword_20DCD44;
export GameRef<uint32_t> dword_20DCCC8;
export GameRef<bool> bWidescreen;
export GameRef<HWND> hGameWindow;
export GameRef<uint8_t> FocusActive, FocusTimerPaused, FocusFocusMuted, FocusAudioReady, FocusMutexOwned;
export GameRef<HANDLE> FocusStreamingMutex;
export GameRef<std::array<uint8_t, 54>> FocusAudio;
export GameRef<std::array<uintptr_t, 48>> HudComponents;
export GameRef<std::array<uint8_t, 48000>> HudQuads;
export GameRef<int> HudQuadCount;
export GameRef<uint8_t> MenuUp, MenuDown, MenuLeft, MenuRight;
export GameRef<float> MenuScaleX, MenuScaleY, MenuMouseX, MenuMouseY;
export GameRef<int> MenuLanguage;
export GameRef<uint8_t> WindowedMode;
export GameRef<uint8_t*> RendererState;
export GameRef<std::array<uint8_t, 9296>> InputContexts;
export GameRef<std::array<uint8_t, 4>> InputClaims;
export GameRef<int> PrimaryController;

export struct CutsceneManagerState
{
    uint8_t reserved[164];
    int current;
    int previous;
    float progress;
};

export struct CameraManagerState
{
    uint8_t reserved1[0x90];
    int currentMode;
    int previousMode;
    uint8_t reserved2[0xA4 - 0x98];
    int cutscene;
    uint8_t reserved3[0x168 - 0xA8];
    float overrideFov;
    int overridden;
    uint8_t reserved4[0x298 - 0x170];
    int blocked;
};

export GameRef<CutsceneManagerState> CutsceneManager;
export GameRef<float> CutsceneTimeStep;
export GameRef<CameraManagerState> CameraManager;
export GameRef<int32_t> CameraBlending;
export GameRef<uint8_t> MenuDividerColor;

export __declspec(noinline) WFP::Event<>& onResChange()
{
    static WFP::Event<> event;
    return event;
}

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
    float fHudFullWidth;
    float fHudVerticalOffset;
    float fHudVerticalOffsetWide;
} Screen;

