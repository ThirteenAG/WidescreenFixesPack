module;

#include "stdafx.h"

export module Build;

// Latest installed PC build: 1.0.112.48699928. Its .text was compared
// byte-for-byte with the 1126680 IDB input before these patterns were selected.
export bool IsSupportedBuild()
{
    const auto base = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    const auto dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    const auto nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(base + dos->e_lfanew);
    return nt->FileHeader.TimeDateStamp == 0x66FC55DB
        && nt->OptionalHeader.SizeOfImage == 0x5936400;
}


export void* WindowProcedureAddress()
{
    return hook::pattern("4C 8B DC 55 56 57 49 8D AB 48 FE FF FF 48 81 EC A0 02 00 00 48 8B 05 ?").get_first();
}
export void* HudParserAddress()
{
    return hook::pattern("40 55 53 57 41 55 48 8D AC 24 58 FC FF FF 48 81 EC A8 04 00 00 48 8B 05").get_first();
}
export void* HudScanCallAddress()
{
    return hook::pattern("E8 ? ? ? ? E9 ? ? ? ? 48 8D 85 90 00 00 00 48 8B CB 48 89 44 24").get_first();
}
export void* LegalScreenAddress()
{
    return hook::pattern("40 53 48 83 EC 20 48 8B 42 20 45 33 C9 48 85 C0 49 8B D8 41 0F 95 C1 4C 03 C8 4C 89 4A 20 48 8B 01 FF 90 A0 04 00 00 88").get_first();
}
export void* InterfaceUpdateAddress()
{
    return hook::pattern("48 89 5C 24 20 57 48 81 EC A0 00 00 00 48 8B 05 ? ? ? ? 48 33 C4 48").get_first();
}
export void* ResumeGameAddress()
{
    return hook::pattern("40 57 48 83 EC 20 48 8B F9 E8 ? ? ? ? 83 3D ? ? ? ? 09 48 8B CF").get_first();
}
export void* GameStateAddress()
{
    static const auto address = injector::ReadRelativeOffset(hook::pattern("8B 0D ? ? ? ? 75 ? 80 3D ? ? ? ? 00 74 ? 83 F9 09 74 ? 48 8B").get_first<uint8_t>(2)).as_int();
    return reinterpret_cast<void*>(address);
}
export void* LoginFlagsAddress()
{
    static const auto address = injector::ReadRelativeOffset(hook::pattern("48 8D 05 ? ? ? ? F6 04 01 02 74 ? 48 8D 05 ? ? ? ? C7 44 24 30").get_first<uint8_t>(3)).as_int();
    return reinterpret_cast<void*>(address);
}
export void* ControllerDevicesAddress()
{
    static const auto address = injector::ReadRelativeOffset(hook::pattern("48 8D 35 ? ? ? ? 48 63 05 ? ? ? ? F3 41 0F 5C C2 4C 8B BC 24 A0").get_first<uint8_t>(3)).as_int();
    return reinterpret_cast<void*>(address);
}
export void* ControllerSlotAddress()
{
    static const auto address = injector::ReadRelativeOffset(hook::pattern("48 63 05 ? ? ? ? F3 41 0F 5C C2 4C 8B BC 24 A0 00 00 00 45 0F 57 C9").get_first<uint8_t>(3)).as_int();
    return reinterpret_cast<void*>(address);
}

export void* PadUpdateAddress()
{
    return hook::pattern("48 8B C4 53 41 54 48 81 EC A8 00 00 00 44 0F 29 50 98 48 8B D9 48 8B 0D").get_first();
}
export void* MouseSampleAddress()
{
    return hook::pattern("F2 0F 11 43 20 0F 54 C1 41 0F 2F C3 73 ? 44 89 7B 20 F3 0F 10 43 24 0F").get_first();
}
