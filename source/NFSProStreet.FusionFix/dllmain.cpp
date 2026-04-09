#include "stdafx.h"
#include "d3d9.h"

import ComVars;

std::once_flag of;

SafetyHookInline shMainLoop = {};
void __cdecl MainLoop(float TickerDifference)
{
    std::call_once(of, []()
    {
        WFP::onGameInitEvent().executeAll();
    });

    WFP::onGameProcessEvent().executeAll();

    static bool oldMenuState = GameFlowManager::IsPaused();
    bool curMenuState = GameFlowManager::IsPaused();
    if (curMenuState != oldMenuState)
    {
        if (curMenuState)
            WFP::onMenuEnterEvent().executeAll();
        else
            WFP::onMenuExitEvent().executeAll();
    }
    oldMenuState = curMenuState;
    if (curMenuState)
        WFP::onMenuDrawingEvent().executeAll();

    shMainLoop.unsafe_ccall(TickerDifference);
}

SafetyHookInline shMiniMainLoop = {};
void __cdecl MiniMainLoop()
{
    std::call_once(of, []()
    {
        WFP::onGameInitEvent().executeAll();
    });

    WFP::onGameProcessEvent().executeAll();

    shMiniMainLoop.unsafe_ccall();
}

void Init()
{
    auto pattern = hook::pattern("8B 0D ? ? ? ? 50 51 89 54 24");
    hWnd.SetAddress(*pattern.get_first<HWND*>(2));

    pattern = hook::pattern("A3 ? ? ? ? 89 35 ? ? ? ? C6 05");

    ResX.SetAddress(*pattern.get_first<int*>(1));
    ResY.SetAddress(*pattern.get_first<int*>(7));

    pattern = hook::pattern("A1 ? ? ? ? 85 C0 75 ? C3 8B 40");
    TheGameFlowManager.SetAddress(*pattern.get_first<GameFlowManager*>(1));

    pattern = hook::pattern("8B 15 ? ? ? ? F7 DA 1B D2 83 E2 ? 3B C3");
    dwWindowedMode.SetAddress(*pattern.get_first<uint32_t*>(2));

    pattern = hook::pattern("6A ? 68 ? ? ? ? 64 A1 ? ? ? ? 50 83 EC ? 56 A1 ? ? ? ? 33 C4 50 8D 44 24 ? 64 A3 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? E8 ? ? ? ? E8");
    shMainLoop = safetyhook::create_inline(pattern.get_first(), MainLoop);

    pattern = hook::pattern("51 E9 ? ? ? ? 50 E8");
    shMiniMainLoop = safetyhook::create_inline(pattern.get_first(), MiniMainLoop);

    pattern = hook::pattern("E8 ? ? ? ? ? ? ? ? 84 C0 0F 57 C9");
    GameFlowManager::IsPaused = decltype(GameFlowManager::IsPaused)(injector::GetBranchDestination(pattern.get_first()).get());

    pattern = hook::pattern("F3 0F 10 05 ? ? ? ? B8 ? ? ? ? F3 0F 11 44 24");
    Sim::Internal::mLastFrameTime.SetAddress(*pattern.get_first<float*>(4));

    pattern = hook::pattern("A1 ? ? ? ? 53 8B 5C 24 08 56 57 8B 38 74 19");
    Direct3DDevice.SetAddress(*pattern.get_first<IDirect3DDevice9***>(1));

    pattern = hook::pattern("81 C7 ? ? ? ? 57 50 FF D2");
    static auto BeforeResetHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        WFP::onBeforeReset().executeAll();
    });

    pattern = hook::pattern("8B 91 ? ? ? ? 57 FF D2 ? ? 8B 48");
    static auto BeforeEndSceneHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        WFP::onEndScene().executeAll();
    });

    WFP::onInitEvent().executeAll();

    static auto futures = WFP::onInitEventAsync().executeAllAsync();

    WFP::onGameInitEvent() += []()
    {
        for (auto& f : futures.get())
            f.wait();
        futures.get().clear();
    };
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("C7 46 ? ? ? ? ? E8 ? ? ? ? C6 86 ? ? ? ? ? 5E C2 04 00"));
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
