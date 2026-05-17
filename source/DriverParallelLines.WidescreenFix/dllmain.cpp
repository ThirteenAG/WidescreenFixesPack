#include "stdafx.h"
#include "d3d9.h"

import ComVars;

std::once_flag of;

void MainLoop()
{
    std::call_once(of, []()
    {
        WFP::onGameInitEvent().executeAll();
    });

    WFP::onGameProcessEvent().executeAll();

    //static bool oldMenuState = GameFlowManager::IsPaused();
    //bool curMenuState = GameFlowManager::IsPaused();
    //if (curMenuState != oldMenuState)
    //{
    //    if (curMenuState)
    //        WFP::onMenuEnterEvent().executeAll();
    //    else
    //        WFP::onMenuExitEvent().executeAll();
    //}
    //oldMenuState = curMenuState;
    //if (curMenuState)
    //    WFP::onMenuDrawingEvent().executeAll();
}

void Init()
{
    auto pattern = hook::pattern("A1 ? ? ? ? 85 C0 8D 7E");
    hWnd.SetAddress(*pattern.get_first<HWND*>(1));

    pattern = hook::pattern("38 05 ? ? ? ? 75 ? 39 81 ? ? ? ? 74");
    dwWindowedMode.SetAddress(*pattern.get_first<uint32_t*>(2));

    pattern = hook::pattern("8B 45 ? 8B 48 ? 57");
    static auto MainLoopHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        MainLoop();
    });

    pCurrentMovieStatus.SetAddress(*hook::get_pattern<int32_t*>("BE ? ? ? ? 33 C0 B9 20 01 00 00 8B FE F3 AB", 1) + 0x1D);

    pattern = hook::pattern("8D 55 ? 8D B3");
    static auto BeforeResetHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        WFP::onBeforeReset().executeAll();
    });

    pattern = hook::pattern("A1 ? ? ? ? 8B 08 6A 00 50 FF 91 ? ? ? ? 8D 4E 18");
    Direct3DDevice.SetAddress(*pattern.get_first<IDirect3DDevice9**>(1));

    auto ptr = (int*)Direct3DDevice.get_ptr();
    BackbufferWidth.SetAddress(ptr + 2);
    BackbufferHeight.SetAddress(ptr + 3);

    pattern = hook::pattern("C7 45 ? ? ? ? ? 8D BB");
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
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("55 8B EC 83 EC 60 53 56 57"));
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
