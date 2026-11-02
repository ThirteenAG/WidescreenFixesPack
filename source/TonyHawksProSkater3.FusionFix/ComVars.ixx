module;

#include <stdafx.h>

export module ComVars;

export GameRef<uint8_t> ModeFlags;
export GameRef<int32_t> dword_851084;
export GameRef<int32_t> dword_851088;
export GameRef<void*> NativeFrontend;
export GameRef<void*> NativeManager;
export GameRef<void*> NativeResources;
export using MovieCallback = int(__cdecl*)(const uint8_t*, int);
export ProtectedGameRef<MovieCallback> NativeCallback;

export struct ScreenState
{
    int32_t Width;
    int32_t Height;
    float fWidth;
    float fHeight;
    int32_t Width43;
    float fAspectRatio;
    float fAspectRatioDiff;
    int32_t FOV;
    float fUnit;
    float fHUDScaleX;
    float fHUDScaleY;
    float fHudOffset;
    float fHudOffsetReal;
    float fHudOffsetY;
};

export ScreenState Screen;

