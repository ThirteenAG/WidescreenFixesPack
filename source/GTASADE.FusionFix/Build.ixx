module;

#include "stdafx.h"

export module Build;

namespace
{
    // The target of a RIP-relative operand
    void* RelativeTarget(const char* signature, ptrdiff_t offset)
    {
        return reinterpret_cast<void*>(injector::ReadRelativeOffset(hook::pattern(signature).get_first<uint8_t>(offset)).as_int());
    }
}

export void* WindowProcedureAddress()
{
    static const auto address = hook::pattern("4C 8B DC 55 56 57 49 8D AB 48 FE FF FF 48 81 EC A0 02 00 00 48 8B 05 ?").get_first();
    return address;
}
export void* HudReloadAddress()
{
    static const auto address = hook::pattern("48 89 5C 24 08 48 89 74 24 10 57 48 83 EC 20 45 33 C0 33 D2 33 C9 E8 ? ? ? ? E8 ? ? ? ? 8B D8 E8 ? ? ? ? 8B CB E8 ? ? ? ? 33 F6 48 8D 3D").get_first();
    return address;
}
export void* HelpMessageAddress()
{
    static const auto address = hook::pattern("48 89 5C 24 10 48 89 74 24 18 48 89 7C 24 20 55 41 56 41 57 48 8D AC 24 50 FF FF FF 48 81 EC B0 01 00 00 48 8B 05 ? ? ? ? 48 33 C4 48 89 85 A0 00 00 00 48 63 59 08").get_first();
    return address;
}
export void* TextGetAddress()
{
    static const auto address = hook::pattern("48 89 5C 24 18 55 56 57 41 54 41 55 41 56 41 57 48 8D AC 24 60 ED FF FF B8 A0 13 00 00 E8").get_first();
    return address;
}
export void* HudParserAddress()
{
    static const auto address = hook::pattern("40 55 53 57 41 55 48 8D AC 24 58 FC FF FF 48 81 EC A8 04 00 00 48 8B 05").get_first();
    return address;
}
export void* HudScanCallAddress()
{
    static const auto address = hook::pattern("E8 ? ? ? ? E9 62 01 00 00 48 8D 85 90 00 00 00 48 8B CB 48 89 44 24").get_first();
    return address;
}
export void* LegalScreenAddress()
{
    static const auto address = hook::pattern("40 53 48 83 EC 20 48 8B 42 20 45 33 C9 48 85 C0 49 8B D8 41 0F 95 C1 4C 03 C8 4C 89 4A 20 48 8B 01 FF 90 A0 04 00 00 88").get_first();
    return address;
}
export void* InterfaceUpdateAddress()
{
    static const auto address = hook::pattern("40 53 55 48 81 EC B8 00 00 00 48 8B 05 ? ? ? ? 48 33 C4 48 89 84 24").get_first();
    return address;
}
export void* ResumeGameAddress()
{
    static const auto address = hook::pattern("40 57 48 83 EC 20 48 8B F9 E8 ? ? ? ? 83 3D ? ? ? ? 09 48 8B CF").get_first();
    return address;
}
export void* GameStateAddress()
{
    static const auto address = injector::ReadRelativeOffset(hook::pattern("83 3D ? ? ? ? 09 75 ? E8 ? ? ? ? 8B 05 ? ? ? ? 48 8D 0D ?").get_first<uint8_t>(2)).as_int() + 1;
    return reinterpret_cast<void*>(address);
}
export void* LoginFlagsAddress()
{
    static const auto address = RelativeTarget("48 8D 05 ? ? ? ? F6 04 01 02 74 ? 48 8D 05 ? ? ? ? C7 44 24 30", 3);
    return address;
}
export void* ControllerDevicesAddress()
{
    // Scanned once: the controller camera asks every frame
    static const auto address = RelativeTarget("48 8D 35 ? ? ? ? 48 63 05 ? ? ? ? F3 41 0F 5C C2 4C 8B BC 24 A0", 3);
    return address;
}
export void* ControllerSlotAddress()
{
    // Scanned once: the controller camera asks every frame
    static const auto address = RelativeTarget("48 63 05 ? ? ? ? F3 41 0F 5C C2 4C 8B BC 24 A0 00 00 00 45 0F 57 C9", 3);
    return address;
}

export void* PadUpdateAddress()
{
    static const auto address = hook::pattern("48 8B C4 53 41 54 48 81 EC A8 00 00 00 44 0F 29 50 98 48 8B D9 48 8B 0D").get_first();
    return address;
}
export void* MouseSampleAddress()
{
    static const auto address = hook::pattern("F2 0F 11 43 20 0F 54 C1 41 0F 2F C3 73 ? 44 89 7B 20 F3 0F 10 43 24 0F").get_first();
    return address;
}

export void* HudStoreAddress()
{
    static const auto address = hook::pattern("F3 0F 10 64 24 6C F3 0F 10 4C 24 70 F3 0F 10 54 24 74 F3 0F 10 5C 24 68").get_first();
    return address;
}

export void* HudViewportCallsAddress()
{
    static const auto address = hook::pattern("E8 ? ? ? ? 0F 57 F6 F3 0F 2A F0 E8 ? ? ? ? 41 83 7C 24 08 00").get_first();
    return address;
}

export void* HudBoundsAddress()
{
    static const auto address = hook::pattern("0F 11 87 D0 00 00 00 8B 47 6C 44 0F 28 64 24 60").get_first(7);
    return address;
}
