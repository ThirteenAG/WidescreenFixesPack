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
export GameRef<int32_t> nGameState;
export GameRef<void*> pCurrentMoviePlayer;

export float actualDeltaTime = 0.0f;

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
    int** pRaceCoordinator = nullptr;
    bool IsPaused()
    {
        if (pRaceCoordinator && *pRaceCoordinator && (**pRaceCoordinator == 7 || **pRaceCoordinator == 11))
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

export void* (*CreateResourceFile)(const char* filename, int32_t type, int flags, int file_offset, int file_size) = nullptr;
export void(__fastcall* ResourceFileBeginLoading)(void* ResourceFile, void* callback, void* callback_param) = nullptr;
export void (__cdecl* ServiceResourceLoading)() = nullptr;

export void (__cdecl* FEngSetInvisible)(const char* pkg_name, unsigned int obj_hash) = nullptr;
export unsigned int (__cdecl* FEHashUpper)(const char* String) = nullptr;
export unsigned int (__cdecl* FEPkgMgr_FindPackage)(const char* pkg_name) = nullptr;
export FEObject* (__fastcall* FEngFindObject)(const char* pkg_name, unsigned int obj_hash) = nullptr;