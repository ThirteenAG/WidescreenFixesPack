#include "stdafx.h"

void InitGTASASCWin8Component()
{
    const auto module = reinterpret_cast<uintptr_t>(GetModuleHandleW(L"GTASASCWin8Component.dll"));
    if (!module)
        return;
    const auto dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(module);
    const auto nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(module + dos->e_lfanew);
    const auto end = module + nt->OptionalHeader.SizeOfImage;
    auto pattern = hook::range_pattern(module, end, "00 00 63 00 62 00 00 00 14 AE 47 3F 00 00 80 3F");
    if (pattern.size() != 2)
    {
        OutputDebugStringA("GTASA UWP radio test: unsupported vehicle table; feature disabled.\n");
        return;
    }
    const auto match = reinterpret_cast<uintptr_t>(pattern.get(0).get<void*>(0));
    static constexpr auto LANDSTAL = 400;
    static constexpr auto UTILTR1 = 611;
    if (match < module + 0x3840 || match - 0x3840 + 0x1B + UTILTR1 * 36 >= end)
    {
        OutputDebugStringA("GTASA UWP radio test: vehicle table outside module; feature disabled.\n");
        return;
    }
    for (size_t i = LANDSTAL; i <= UTILTR1; ++i)
    {
        injector::WriteMemory<uint8_t>(match - 0x3840 + 0x1B + i * 36, 0, true);
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