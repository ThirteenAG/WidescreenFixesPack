#include "stdafx.h"

void InitGTASASCWin8Component()
{
    //Radio in all vehicles
    auto m = (uint32_t)GetModuleHandle(L"GTASASCWin8Component");
    auto pattern = hook::range_pattern(m, m + 0xB3DFF0, "00 00 63 00 62 00 00 00 14 AE 47 3F 00 00 80 3F");
    auto offset = (uint32_t)pattern.count(2).get(0).get<void*>(0);
    static constexpr auto LANDSTAL = 400;
    static constexpr auto UTILTR1 = 611;
    for (size_t i = LANDSTAL; i <= UTILTR1; i++)
    {
        injector::WriteMemory<uint8_t>(((offset - 0x3840 + 0x1B) + (i * 36)), 0, true);
    }
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallback(L"GTASASCWin8Component.dll", InitGTASASCWin8Component);
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