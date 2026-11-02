module;

#include <stdafx.h>
#include <d3d9.h>

export module ComVars;

export GameRef<void*> NativeManager;
export GameRef<void*> NativeSymbols;
export GameRef<int> NativeLanguage;

export GameRef<int> NativeWindowed;
export GameRef<float> FrameScaleY;
export GameRef<int32_t> FrameOffset2D;
export GameRef<uint8_t> FrameLetterboxed;
export GameRef<std::array<int32_t, 4>> FrameViewport;
export GameRef<int32_t> FrameTargetWidth;
export GameRef<int32_t> FrameTargetHeight;
export GameRef<IDirect3DDevice9*> GameDevice;
export GameRef<std::array<uint8_t, 256>> KeyBuffer;
export GameRef<float> NativeScaleX;
export GameRef<int> HudLayout;

export GameRef<int32_t> dword_6D3608;
export GameRef<int32_t> dword_6D360C;
export GameRef<int32_t> dword_72DFC4;
export GameRef<std::array<int32_t, 24>> keyboardMap;
export GameRef<int32_t> dword_69B718;
export GameRef<int32_t> dword_681D14;

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
    float fHUDScaleX;
    float fHudOffset;
    float fHudOffsetReal;
};

export ScreenState Screen;


