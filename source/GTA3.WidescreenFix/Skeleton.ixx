module;

#include <stdafx.h>
#include <cstdint>
#include "common.h"

export module Skeleton;

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
    std::int32_t  width;
    std::int32_t  height;
    std::int32_t  maximumWidth;
    std::int32_t  maximumHeight;
    std::int32_t  maxFPS;
    bool          quit;
    void* ps;
    RsInputDevice keyboard;
    RsInputDevice mouse;
    RsInputDevice pad;
};

export GameRef<RsGlobalType> RsGlobal([]() -> RsGlobalType*
{
    auto pattern = hook::pattern("8B 0D ? ? ? ? 6A ? 6A ? 52");
    if (!pattern.empty())
        return *pattern.get_first<RsGlobalType*>(2);
    return nullptr;
});

export namespace CGeneral
{
    unsigned int (__cdecl* GetRandomNumber)() = nullptr;
}

class Skeleton
{
public:
    Skeleton()
    {
        WFP::onGameInitEvent() += []()
        {
            auto pattern = hook::pattern("E8 ? ? ? ? 0F B7 C0 83 E0 ? 83 F8 ? 74");
            CGeneral::GetRandomNumber = (decltype(CGeneral::GetRandomNumber))injector::GetBranchDestination(pattern.get_first(0)).as_int();
        };
    }
} Skeleton;