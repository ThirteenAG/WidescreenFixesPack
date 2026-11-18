module;

#include <stdafx.h>
#include <minidx8/d3d8.h>

export module ComVars;

export GameRef<uint32_t> NativeLanguage;
export GameRef<void*> NativeSymbols;
export GameRef<void*> NativeRegistry;
export GameRef<void*> NativeManager;
export GameRef<void*> NativeFonts;
export GameRef<uint32_t> NativeFontBits;
export GameRef<uint8_t> NativeFontsLoaded;
export GameRef<float> MovieFrameWidth;
export GameRef<float> MovieFrameHeight;
export GameRef<D3DPRESENT_PARAMETERS> GamePresentation;
export GameRef<IDirect3DDevice8*> GameDevice;
export GameRef<uint8_t> GameInitialized;
export GameRef<uint8_t> GameLost;
export GameRef<int> NativeWindowed;
export GameRef<int> PresentationInterval;

export GameRef<int> CameraLayout;
export GameRef<float> NativeZoomScale;
export GameRef<int32_t> dword_5A0280;
export GameRef<int32_t> dword_5A0284;
export GameRef<int32_t> OffsetY;

export struct ScreenState
{
    int32_t Width;
    int32_t Height;
    float fWidth;
    float fHeight;
    int32_t Width43;
    float fAspectRatio;
    float fAspectRatioDiff;
    float fFieldOfView;
    float fFOVFactor;
    float fHUDScaleX;
    float fHudOffset;
    float fHudOffsetReal;
    float fHudOffsetY;
};

export ScreenState Screen;


export GameRef<std::array<uint8_t, 512>> KeyBuffer;
// Per player (192 bytes each): the selected pad's instance GUID followed by its 22 binding codes
export GameRef<uint8_t> PlayerControls;
export GameRef<uint8_t*> ButtonFont;
export GameRef<uint8_t*> BatchFont;
export GameRef<int32_t> KeyBufferIndex;

// Prompts show pad buttons while the pad was the last device used
export std::atomic<bool> UsingPad = false;
export std::atomic<int> ActivePadPlayer = 0;
