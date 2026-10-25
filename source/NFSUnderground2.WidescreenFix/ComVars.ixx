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

export GameRef<HWND> hWnd;
export GameRef<uint32_t> dwWindowedMode;
export GameRef<tagRECT> WindowRect;
export GameRef<int> g_RacingResolution;
export GameRef<IDirect3DDevice9*> Direct3DDevice;
export GameRef<uint8_t> bExitFEOptionsSystem;
export GameRef<int32_t> nGameState;

// Game flow states (nGameState)
export constexpr int32_t GameStateFrontEnd = 3;

export bool IsFrontEnd()
{
    return nGameState == GameStateFrontEnd;
}

export float actualDeltaTime = 0.0f;

export namespace cFEng
{
    void** pInstance = nullptr;
    void (__fastcall* MakeLoadedPackagesDirty)(void*, void*) = nullptr;
}

export uint32_t bStringHash(const char* str)
{
    if (!str)
        return 0xFFFFFFFF;

    uint32_t result = 0xFFFFFFFF;
    for (; *str; ++str)
        result = static_cast<uint8_t>(*str) + 33 * result;

    return result;
}

export namespace GameFlowManager
{
    int** pRaceCoordinator = nullptr;
    bool IsPaused()
    {
        if (pRaceCoordinator && *pRaceCoordinator && **pRaceCoordinator == 6)
            return true;

        if (bExitFEOptionsSystem)
            return true;

        return false;
    }
}

template<typename... Args>
class ResChange : public WFP::Event<Args...>
{
public:
    using WFP::Event<Args...>::Event;
};

export __declspec(noinline) ResChange<int, int>& onResChange()
{
    static ResChange<int, int> ResChangeEvent;
    return ResChangeEvent;
}

template<typename... Args>
class WindowMessage : public WFP::Event<Args...>
{
public:
    using WFP::Event<Args...>::Event;
};

// Messages received by the game's window procedure, before the game handles them
export __declspec(noinline) WindowMessage<HWND, UINT, WPARAM, LPARAM>& onWindowMessage()
{
    static WindowMessage<HWND, UINT, WPARAM, LPARAM> WindowMessageEvent;
    return WindowMessageEvent;
}
