#include "stdafx.h"

import ComVars;

void Init()
{
    auto pattern = hook::pattern("8B 91 ? ? ? ? 50 FF D2 8B CE");
    static auto BeforeEndSceneHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        Direct3DDevice = (IDirect3DDevice9*)regs.eax;
        WFP::onEndScene().executeAll();
    });

    static auto futures = WFP::onInitEventAsync().executeAllAsync();

    WFP::onGameInitEvent() += []() //todo: add onGameInitEvent hook
    {
        for (auto& f : futures.get())
            f.wait();
        futures.get().clear();
    };

    WFP::onInitEvent().executeAll();
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("D9 44 24 04 56 DC 0D ? ? ? ? 8B F1 D9 5C 24 08"));
    });
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        if (!IsUALPresent()) { InitializeASI(); }
    }
    else if (reason == DLL_PROCESS_DETACH)
    {

    }
    return TRUE;
}
