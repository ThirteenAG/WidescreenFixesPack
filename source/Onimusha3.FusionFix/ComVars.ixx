module;
#include <stdafx.h>

export module ComVars;

export struct Screen
{
    int Width;
    int Height;
    float fWidth;
    float fHeight;
    float fFieldOfView;
    float fAspectRatio;
    float fHudOffset1;
    float fHudOffset2;
    float fFMVOffset1;
    float fFMVOffset2;
    float fFMVOffset3;
    float fFMVOffset4;
    int Width43;
    float fWidth43;
    float f3DHorScale;
    float fNegResX;
    float fNegResY;
} Screen;

export GameRef<const uint8_t> GameState;
export namespace OnimushaBorders
{
    GameRef<const int32_t> Script;
    GameRef<uint8_t> Allocator;
}
export namespace OnimushaIntro
{
    GameRef<const uint8_t> Startup;
}
export namespace OnimushaMovies
{
    GameRef<const uint16_t> Entries;
}
export namespace OnimushaMenu
{
    GameRef<uint8_t> NativeLanguage;
    GameRef<uint32_t> Pressed;
    GameRef<uint32_t> Repeated;
}
export namespace OnimushaPad
{
    GameRef<const int> CellWidth;
    GameRef<const int> CellHeight;
}
export namespace OnimushaTiming
{
    GameRef<uint8_t> Ready;
}
export namespace OnimushaWindow
{
    GameRef<uint32_t> NativeWidth;
    GameRef<uint32_t> NativeHeight;
}
