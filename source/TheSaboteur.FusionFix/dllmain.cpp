#include "stdafx.h"

import ComVars;

void Init()
{
    static bool bEndSceneEventFired = false;

    auto pattern = hook::pattern("83 05 ? ? ? ? 01 F7 47 08 00 00 00 80 8B CE");
    static auto BeforeRenderPassHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        if (bEndSceneEventFired)
            return;

        static constexpr std::string_view UIPasses[] = { "ClamberingHUDDrawList", "ClamberingHUDBucket", "Flash Bucket", "Debug" };
        auto szPassName = (const char*)(regs.edi + 0x0C);
        if (std::find(std::begin(UIPasses), std::end(UIPasses), szPassName) == std::end(UIPasses))
            return;

        static constexpr auto nDeviceOffset = 24424;
        Direct3DDevice = *(IDirect3DDevice9**)(regs.esi + nDeviceOffset);
        bEndSceneEventFired = true;
        WFP::onEndScene().executeAll();
    });

    pattern = hook::pattern("8B 91 ? ? ? ? 50 FF D2 8B CE");
    static auto BeforeEndSceneHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        Direct3DDevice = (IDirect3DDevice9*)regs.eax;
        if (!bEndSceneEventFired)
            WFP::onEndScene().executeAll();
        bEndSceneEventFired = false;
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
        WFP::onShutdownEvent().executeAll();
    }
    return TRUE;
}
