module;

#include "stdafx.h"
#include "../Shared/CheckedPattern.hpp"

export module Build;

export void* WindowProcedureAddress()
{
    return wfp::patterns::First("4C 8B DC 55 56 57 49 8D AB 48 FE FF FF 48 81 EC A0 02 00 00 48 8B 05 ?");
}
export void* HudParserAddress()
{
    return wfp::patterns::First("40 55 53 57 41 55 48 8D AC 24 58 FC FF FF 48 81 EC A8 04 00 00 48 8B 05");
}
export void* HudScanCallAddress()
{
    return wfp::patterns::First("E8 ? ? ? ? E9 ? ? ? ? 48 8D 85 90 00 00 00 48 8B CB 48 89 44 24");
}
export void* LegalScreenAddress()
{
    return wfp::patterns::First("40 53 48 83 EC 20 48 8B 42 20 45 33 C9 48 85 C0 49 8B D8 41 0F 95 C1 4C 03 C8 4C 89 4A 20 48 8B 01 FF 90 A0 04 00 00 88");
}
export void* InterfaceUpdateAddress()
{
    return wfp::patterns::First("48 89 5C 24 20 57 48 81 EC A0 00 00 00 48 8B 05 ? ? ? ? 48 33 C4 48");
}
export void* ResumeGameAddress()
{
    return wfp::patterns::First("40 57 48 83 EC 20 48 8B F9 E8 ? ? ? ? 83 3D ? ? ? ? 09 48 8B CF");
}
export void* GameStateAddress()
{
    static const auto address = injector::ReadRelativeOffset(wfp::patterns::First<uint8_t>("83 3D ? ? ? ? 09 0F 84 ? ? ? ? 48 8B 0D ? ? ? ? 83 B9 50 03", 2)).as_int() + 1;
    return reinterpret_cast<void*>(address);
}
export void* LoginFlagsAddress()
{
    const auto operand = wfp::patterns::First<uint8_t>("48 8D 05 ? ? ? ? F6 04 01 02 74 ? 48 8D 05 ? ? ? ? C7 44 24 30", 3);
    if (!operand)
        return nullptr;
    static const auto address = injector::ReadRelativeOffset(operand).as_int();
    return reinterpret_cast<void*>(address);
}
export void* ControllerDevicesAddress()
{
    const auto operand = wfp::patterns::First<uint8_t>("48 8D 35 ? ? ? ? 48 63 05 ? ? ? ? F3 41 0F 5C C2 4C 8B BC 24 A0", 3);
    if (!operand)
        return nullptr;
    static const auto address = injector::ReadRelativeOffset(operand).as_int();
    return reinterpret_cast<void*>(address);
}
export void* ControllerSlotAddress()
{
    const auto operand = wfp::patterns::First<uint8_t>("48 63 05 ? ? ? ? F3 41 0F 5C C2 4C 8B BC 24 A0 00 00 00 45 0F 57 C9", 3);
    if (!operand)
        return nullptr;
    static const auto address = injector::ReadRelativeOffset(operand).as_int();
    return reinterpret_cast<void*>(address);
}

export void* PadUpdateAddress()
{
    return wfp::patterns::First("48 8B C4 53 41 54 48 81 EC A8 00 00 00 44 0F 29 50 98 48 8B D9 48 8B 0D");
}
export void* MouseSampleAddress()
{
    return wfp::patterns::First("F2 0F 11 43 20 0F 54 C1 41 0F 2F C3 73 ? 44 89 7B 20 F3 0F 10 43 24 0F");
}
