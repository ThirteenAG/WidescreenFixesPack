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
export GameRef<int32_t> nGameState;
export GameRef<void*> pCurrentMoviePlayer;

// Game flow states (nGameState)
export constexpr int32_t GameStateFrontEnd = 3;
export constexpr int32_t GameStateRacing = 6;

// The game's time step of the current frame in seconds (PrepareRealTimestep): the whole frame including the
// frame rate limiter's wait, in steps of 1/3600 s
export GameRef<float> RealTimeElapsed;

export namespace cFEng
{
    void** pInstance = nullptr;
    void (__stdcall* MakeLoadedPackagesDirty)(void*) = nullptr;
    void (__cdecl* PopPackage)(char* a1) = nullptr;
}

export bool MovieIsPlaying()
{
    return pCurrentMoviePlayer != nullptr;
}

export bool IsFrontEnd()
{
    return nGameState == GameStateFrontEnd;
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
        return pRaceCoordinator && *pRaceCoordinator && (**pRaceCoordinator == 7 || **pRaceCoordinator == 11);
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

export struct __declspec(align(4)) MenuScreen
{
    void* __vftable;
    bool mPlaySound;
    unsigned int mDirectionForNextSound;
    const char* PackageFilename;
    ///...
};

template<typename... Args>
class MenuScreenBaseNotify : public WFP::Event<Args...>
{
public:
    using WFP::Event<Args...>::Event;
};

export __declspec(noinline) MenuScreenBaseNotify<MenuScreen*>& onMenuScreenBaseNotify()
{
    static MenuScreenBaseNotify<MenuScreen*> MenuScreenBaseNotifyEvent;
    return MenuScreenBaseNotifyEvent;
}

struct FEMinNode
{
    void* __vftable;
    FEMinNode* next;
    FEMinNode* prev;
};

export enum FEObjType : int32_t
{
    FE_None = 0x0,
    FE_Image = 0x1,
    FE_String = 0x2,
    FE_Model = 0x3,
    FE_List = 0x4,
    FE_Group = 0x5,
    FE_CodeList = 0x6,
    FE_Movie = 0x7,
    FE_Effect = 0x8,
    FE_ColoredImage = 0x9,
    FE_AnimImage = 0xA,
    FE_SimpleImage = 0xB,
    FE_MultiImage = 0xC,
    FE_UserMin = 0x100,
};

export struct FEObject : FEMinNode
{
    uint32_t GUID;
    uint32_t NameHash;
    char* pName;
    FEObjType Type;
    uint32_t Flags;
    uint16_t RenderContext;
    uint16_t ResourceIndex;
    uint32_t Handle;
    uint32_t UserParam;
    uint8_t* pData;
    uint32_t DataSize;
    //...
};

export struct FEImage : FEObject
{
    unsigned int ImageFlags;
};

export void (__cdecl* FEngSetInvisible)(const char* pkg_name, unsigned int obj_hash) = nullptr;
export unsigned int (__cdecl* FEHashUpper)(const char* String) = nullptr;
export unsigned int (__cdecl* FEPkgMgr_FindPackage)(const char* pkg_name) = nullptr;
export FEObject* (__fastcall* FEngFindObject)(const char* pkg_name, unsigned int obj_hash) = nullptr;
