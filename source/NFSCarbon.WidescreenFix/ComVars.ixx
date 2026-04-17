module;

#include "stdafx.h"
#include "d3d9.h"

export module ComVars;

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

export namespace cFEngGameInterface
{
    struct FEMouseInfo
    {
        int16_t x;
        int16_t y;
        int16_t unk;
        int16_t buttons;
    };

    void (__stdcall* GetMouseInfo)(FEMouseInfo* info) = nullptr;
}

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

export int SimRate = -1;

export GameRef<HWND> hWnd;
export GameRef<uint32_t> dwWindowedMode;
export GameRef<tagRECT> WindowRect;
export GameRef<int> g_RacingResolution;
export GameRef<IDirect3DDevice9*> Direct3DDevice;
export GameRef<void*> MoviePlayerInstance;

export namespace Sim
{
    namespace Internal
    {
        export GameRef<float> mLastFrameTime;
    }
}

export namespace cFEng
{
    void** pInstance = nullptr;
    void (__fastcall* MakeLoadedPackagesDirty)(void*, void*) = nullptr;
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

export namespace GameFlowManager
{
    bool (*IsPaused)() = nullptr;
}

template<typename... Args>
class ResChange : public WFP::Event<Args...>
{
public:
    using WFP::Event<Args...>::Event;
};

export ResChange<int, int>& onResChange()
{
    static ResChange<int, int> ResChangeEvent;
    return ResChangeEvent;
}