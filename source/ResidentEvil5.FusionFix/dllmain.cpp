#include "stdafx.h"

injector::memory_pointer_raw p6D1650;
void __fastcall sub_6D1650(float* _this, void* edx, float a2, float a3, float a4, float a5)
{
    a2 /= 1280.0f / 800.0f;
    return injector::fastcall<void(float*, void*, float, float, float, float)>::call(p6D1650, _this, edx, a2, a3, a4, a5);
}

void Init()
{
    //CIniReader iniReader("");

    // Camera near clip fix
    p6D1650 = injector::GetBranchDestination(hook::get_pattern("E8 ? ? ? ? 83 BE ? ? ? ? ? 8B C7"));
    injector::MakeCALL(hook::get_pattern("E8 ? ? ? ? 83 BE ? ? ? ? ? 8B C7"), sub_6D1650, true);
    injector::MakeCALL(hook::get_pattern("E8 ? ? ? ? 8B C6 5E 8B E5 5D C2 04 00 CC CC CC CC CC CC CC CC CC 55"), sub_6D1650, true);
    injector::MakeCALL(hook::get_pattern("E8 ? ? ? ? 80 BF ? ? ? ? ? 74 0E"), sub_6D1650, true);
    injector::MakeCALL(hook::get_pattern("E8 ? ? ? ? F3 0F 10 AC 24 ? ? ? ? F3 0F 10 84 24 ? ? ? ? F3 0F 10 A4 24"), sub_6D1650, true);
    injector::MakeCALL(hook::get_pattern("E8 ? ? ? ? 8D 97 ? ? ? ? 52 8D 87"), sub_6D1650, true);
    injector::MakeCALL(hook::get_pattern("E8 ? ? ? ? 8B C6 5E 8B E5 5D C2 04 00 CC CC CC CC CC CC CC CC CC 83 EC 28"), sub_6D1650, true);
    injector::MakeCALL(hook::get_pattern("E8 ? ? ? ? F3 0F 10 44 24 ? F3 0F 10 4C 24 ? F3 0F 10 7C 24 ? 0F 28 D1"), sub_6D1650, true);
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallback(Init, hook::pattern("E8 ? ? ? ? 83 BE ? ? ? ? ? 8B C7"));
    });
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        if (!IsUALPresent()) { InitializeASI(); }
    }
    return TRUE;
}
