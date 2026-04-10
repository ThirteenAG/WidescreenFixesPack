module;

#include "stdafx.h"
#include "d3d9.h"

export module ComVars;

export uintptr_t RangeStart, RangeEnd;
export hook::pattern GetPattern(std::string_view pattern)
{
    auto p = hook::pattern(pattern);
    if (p.empty())
        return hook::pattern(RangeStart, RangeEnd, pattern);
    return p;
}

export struct bVector3
{
    float x;
    float y;
    float z;
};

export struct bVector4
{
    float x;
    float y;
    float z;
    float w;
};

export struct bMatrix4
{
    bVector4 v0;
    bVector4 v1;
    bVector4 v2;
    bVector4 v3;
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

export enum GameFlowState : int32_t
{
    GAMEFLOW_STATE_NONE = 0x0,
    GAMEFLOW_STATE_LOADING_FRONTEND = 0x1,
    GAMEFLOW_STATE_UNLOADING_FRONTEND = 0x2,
    GAMEFLOW_STATE_IN_FRONTEND = 0x3,
    GAMEFLOW_STATE_LOADING_REGION = 0x4,
    GAMEFLOW_STATE_LOADING_TRACK = 0x5,
    GAMEFLOW_STATE_RACING = 0x6,
    GAMEFLOW_STATE_UNLOADING_TRACK = 0x7,
    GAMEFLOW_STATE_UNLOADING_REGION = 0x8,
    GAMEFLOW_STATE_EXIT_DEMO_DISC = 0x9,
};

export struct GameFlowManager
{
    char unk[36];
    GameFlowState CurrentGameFlowState;

    static inline bool (*IsPaused)() = nullptr;
};

export bool bHideCursorForMouseLook = false;

export int SimRate = -1;

export uint32_t* Width = nullptr;
export uint32_t* Height = nullptr;

export GameRef<HWND> hWnd;
export GameRef<uint32_t> dwWindowedMode;
export GameRef<IDirect3DDevice9*> Direct3DDevice;

export uint32_t ShadowLevel = 3;

export namespace Sim
{
    namespace Internal
    {
        export GameRef<float> mLastFrameTime;
    }
}

export int bStringHash(const char* str)
{
    if (!str)
        return -1;

    int result = -1;
    unsigned char c = *str;

    while (c != 0)
    {
        result = c + 33 * result;
        c = *(++str);
    }

    return result;
}
