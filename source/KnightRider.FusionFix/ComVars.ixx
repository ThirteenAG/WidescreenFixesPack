module;
#include <stdafx.h>
#include <atomic>

export module ComVars;

export struct Screen
{
    int32_t nWidth;
    int32_t nHeight;
    float fWidth;
    float fHeight;
    float fAspectRatio;
} Screen;

export namespace KnightRiderHud
{
    bool CockpitActive = false;
}

export namespace KnightRiderCamera
{
    GameRef<uint8_t*> Wrapper;
}

export namespace KnightRiderFrame
{
    GameRef<float> NativeCap;
}
