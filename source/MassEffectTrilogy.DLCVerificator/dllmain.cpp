#include "stdafx.h"

void Init()
{
    //ME2
    auto pattern = hook::pattern("E9 ? ? ? ? 39 5C 24 18 8B 44 24 14 75 05 B8 ? ? ? ? 8B 0D ? ? ? ? 8B 11 68 ? ? ? ? 53");
    if (!pattern.empty())
        injector::WriteMemory(pattern.get_first(1), *pattern.get_first<uint32_t>(1) - 5, true);

    //ME3
    pattern = hook::pattern("8B 11 8B 42 0C 57 56 FF D0 8B C3 8B 4D F4 64 89 0D 00 00 00 00 59 5F 5E 5B 8B E5 5D C2 08 00");
    if (!pattern.empty())
        injector::WriteMemory<uint16_t>(pattern.get_first(9), 0x01B0, true);

    pattern = hook::pattern("8B 45 0C C7 00 01 00 00 00 5D C2 08 00 8B 4D 0C C7 01 01 00 00 00 5D C2 08 00");
    if (!pattern.empty())
    {
        injector::WriteMemory(pattern.get_first(5), 0, true);
        injector::WriteMemory(pattern.get_first(18), 0, true);
    }

    pattern = hook::pattern("B8 E4 FF FF FF 5B 59 C3");
    if (!pattern.empty())
        injector::WriteMemory(pattern.get_first(1), 0, true);
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallback(Init, hook::pattern("51 68 ? ? ? ? 68 04 01 00 00 52 FF 15 ? ? ? ? 85 C0 ").count_hint(1).empty(), 0x1100);
    });
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {

    }
    return TRUE;
}