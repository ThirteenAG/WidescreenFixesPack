module;

#include <stdafx.h>
#include <array>

export module ComVars;

export GameRef<const uint32_t> languageCode;
export GameRef<uint32_t> GameFlags, DisplayFlags;
export GameRef<uintptr_t> Memory;
export GameRef<float> NativeFOV;
export GameRef<std::array<int32_t, 3>> DisplayMode;


export uint32_t off_288, off_290, off_2C0;

export struct Screen
{
    int32_t Width = 640;
    int32_t Height = 480;
    float fWidth = 640.0f;
    float fHeight = 480.0f;
    float fAspectRatio = 4.0f / 3.0f;
    float fFieldOfView;
    float fHUDScaleX;
    float fHudOffset;
    float fHudOffsetReal;
    float fAspectRatioDiff = 1.0f;
} Screen;

