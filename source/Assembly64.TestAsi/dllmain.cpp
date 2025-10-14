#include "stdafx.h"
#include "raw_mem.hpp"

void Init()
{
    auto pattern = hook::pattern("48 8D 15 ? ? ? ? 33 C9 FF 15 ? ? ? ? 45 33 C9");
    struct TestHook
    {
        void operator()(injector::reg_pack& regs)
        {
            static constexpr auto str = "Hello x64!";
            regs.rdx = (uint64_t)(str);
        }
    }; injector::MakeInline<TestHook>(pattern.get_first(0), pattern.get_first(7));
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallback(Init, hook::pattern("48 8D 15 ? ? ? ? 33 C9 FF 15 ? ? ? ? 45 33 C9"));
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
