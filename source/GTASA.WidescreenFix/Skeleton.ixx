module;

#include <stdafx.h>
#include <cstdint>
#include "common.h"
#include <d3d9.h>

export module Skeleton;

import Entity;

enum RsInputDeviceType : int
{
    rsKEYBOARD,
    rsMOUSE,
    rsPAD
};

enum RsEventStatus : int
{
    rsEVENTERROR,
    rsEVENTPROCESSED,
    rsEVENTNOTPROCESSED
};

enum RsEvent : int
{
    rsCAMERASIZE,
    rsCOMMANDLINE,
    rsFILELOAD,
    rsINITDEBUG,
    rsINPUTDEVICEATTACH,
    rsLEFTBUTTONDOWN,
    rsLEFTBUTTONUP,
    rsMOUSEMOVE,
    rsMOUSEWHEELMOVE,
    rsPLUGINATTACH,
    rsREGISTERIMAGELOADER,
    rsRIGHTBUTTONDOWN,
    rsRIGHTBUTTONUP,
    _rs_13,
    _rs_14,
    _rs_15,
    _rs_16,
    _rs_17,
    _rs_18,
    _rs_19,
    _rs_20,
    rsRWINITIALIZE,
    rsRWTERMINATE,
    rsSELECTDEVICE,
    rsINITIALIZE,
    rsTERMINATE,
    rsIDLE,
    rsFRONTENDIDLE,
    rsKEYDOWN,
    rsKEYUP,
    rsQUITAPP,
    rsPADBUTTONDOWN,
    rsPADBUTTONUP,
    rsPADANALOGUELEFT,
    rsPADANALOGUELEFTRESET,
    rsPADANALOGUERIGHT,
    rsPADANALOGUERIGHTRESET,
    rsPREINITCOMMANDLINE,
    rsACTIVATE,
};

using RsInputEventHandler = RsEventStatus(*)(RsEvent event, void* param);

struct RsInputDevice
{
    RsInputDeviceType inputDeviceType;
    bool              used;
    RsInputEventHandler inputEventHandler;
};

export struct RsGlobalType
{
    const char* appName;
    union
    {
        int32_t maximumWidth;
        int32_t width;
    };
    union
    {
        int32_t maximumHeight;
        int32_t height;
    };
    int32_t maxFPS;
    bool  quit;
    void* ps;
    RsInputDevice keyboard;
    RsInputDevice mouse;
    RsInputDevice pad;
};

export GameRef<RsGlobalType> RsGlobal([]() -> RsGlobalType*
{
    auto pattern = hook::pattern("A1 ? ? ? ? 51 2B 54 24");
    if (!pattern.empty())
        return *pattern.get_first<RsGlobalType*>(1);
    return nullptr;
});

export namespace CGeneral
{
    unsigned int __cdecl GetRandomNumber()
    {
        return rand();
    }
}

export CEntity* (__cdecl* FindPlayerPed)(int number) = nullptr;
export CEntity* (__cdecl* FindPlayerVehicle)(int playerNum, bool bIncludeRemote) = nullptr;
export void (__cdecl* RwCameraSetNearClipPlane)(RwCamera* camera, float nearClip) = nullptr;

export IDirect3DDevice9** pD3D9Device = nullptr;

class Skeleton
{
public:
    Skeleton()
    {
        WFP::onInitEvent() += []()
        {
            auto pattern = hook::pattern("A1 ? ? ? ? 8B 10 6A 00 8D 7B");
            pD3D9Device = *pattern.get_first<IDirect3DDevice9**>(1);

            pattern = find_pattern("E8 ? ? ? ? 8B 80 ? ? ? ? 83 C4 ? 85 C0 74 ? ? ? EB ? 33 C0 8B 40 ? 83 F8 ? 0F 8E");
            FindPlayerPed = (decltype(FindPlayerPed))injector::GetBranchDestination(pattern.get_first()).as_int();

            pattern = find_pattern("E8 ? ? ? ? 83 C4 ? 85 C0 74 ? 83 C0 ? ? ? 8B 50 ? 8B 40 ? 89 4C 24 ? ? ? ? ? ? ? ? ? ? ? 89 54 24");
            FindPlayerVehicle = (decltype(FindPlayerVehicle))injector::GetBranchDestination(pattern.get_first()).as_int();
        };
    }
} Skeleton;