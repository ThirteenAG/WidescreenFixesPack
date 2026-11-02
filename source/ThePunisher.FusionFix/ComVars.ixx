module;

#include <stdafx.h>

export module ComVars;

export GameRef<int32_t> dword_913250, dword_913254;
export GameRef<uint32_t> dword_73DF80, gTexColor;
export GameRef<uint32_t*> MovieDecoder;
export GameRef<int> MenuFont;
export GameRef<uint8_t> byte_9132CC;
export namespace Cutscene
{
    GameRef<uintptr_t> Data;
    GameRef<float> Slide;
    GameRef<int32_t> FillMode, FillFlags;
}

export struct Screen
{
    int32_t Width;
    int32_t Height;
    float fWidth;
    float fHeight;
    int32_t Width43;
    float fAspectRatio;
    float fAspectRatioDiff;
    float fDynamicScreenFieldOfViewScale;
    float fHUDScaleX;
    float fHudOffset;
    float fHudOffsetReal;
    float fMenuOffsetRealX;
    float fMenuOffsetRealY;
    int32_t PresetWidth;
    int32_t PresetHeight;
    float PresetFactorX;
    float PresetFactorY;
} Screen;
