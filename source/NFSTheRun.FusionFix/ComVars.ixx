module;

#include "stdafx.h"

export module ComVars;

export struct bVector3
{
    float x;
    float y;
    float z;
};

export struct MouseData
{
    int   DeltaX = 0;
    int   DeltaY = 0;
    int   Wheel = 0;
} g_Mouse;

export struct StickData
{
    int   RawX = 0;
    int   RawY = 0;
    float X = 0.0f;
    float Y = 0.0f;
} g_RightStick;

export bool bHideCursorForMouseLook = false;

export GameRef<HWND> hWnd;
export GameRef<void*> pRenderDevice;

export bool bBothSticksClicked = false;

export namespace fb
{
    namespace GameTimer
    {
        float actualDeltaTime = 0.0f;
    }

    GameRef<void**> g_profileOptionsManager;

    namespace ProfileOptions
    {
        int (__thiscall* getInt)(void* ProfileOptions, const char* name, int defaultValue) = nullptr;
    }
}

export std::pair<int, int> GetRes()
{
    auto Width = fb::ProfileOptions::getInt(fb::g_profileOptionsManager + 0x14, "ResolutionWidth", 0);
    auto Height = fb::ProfileOptions::getInt(fb::g_profileOptionsManager + 0x14, "ResolutionHeight", 0);

    if (Width <= 0 || Height <= 0)
    {
        Width = PtrWalkthrough<int>(&pRenderDevice, 0, 0x84, 0x18).value_or(1280);
        Height = PtrWalkthrough<int>(&pRenderDevice, 0, 0x84, 0x20).value_or(720);
    }

    return { Width, Height };
}