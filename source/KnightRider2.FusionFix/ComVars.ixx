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

export namespace KnightRider2Hud
{
    bool CockpitActive = false;
}

export namespace KnightRider2Camera
{
    GameRef<uint8_t*> Wrapper;
}

export namespace KnightRider2Window
{
    GameRef<uint8_t*> Application;
}

export namespace KnightRider2Frame
{
    GameRef<float> NativeCap;
}
