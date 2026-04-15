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
}

void Init()
{
    auto pattern = hook::pattern("A1 ? ? ? ? 68 ? ? ? ? 50 FF 15");
    hWnd.SetAddress(*pattern.get_first<HWND*>(1));
    WindowRect.SetAddress(*pattern.get_first<tagRECT*>(6));

    pattern = hook::pattern("A1 ? ? ? ? 85 C0 74 ? A1 ? ? ? ? 68");
    dwWindowedMode.SetAddress(*pattern.get_first<uint32_t*>(1));

    pattern = hook::pattern("A1 ? ? ? ? 8B 0C 85 ? ? ? ? 85 C9");
    g_RacingResolution.SetAddress(*pattern.get_first<int*>(1));

    pattern = hook::pattern("A3 ? ? ? ? 39 1D ? ? ? ? 89 15");
    static auto MainLoopHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        MainLoop();
    });

    pattern = hook::pattern("A1 ? ? ? ? 85 C0 74 ? ? ? 83 F8 ? 74 ? 83 F8 ? 75 ? 8A 43");
    GameFlowManager::pRaceCoordinator = *pattern.get_first<int**>(1);

    pattern = hook::pattern("83 3D ? ? ? ? 06 ? ? A1");
    nGameState.SetAddress(*pattern.get_first<int32_t*>(2));

    pattern = hook::pattern("A1 ? ? ? ? 33 C9 3B D3");
    cFEng::pInstance = *pattern.get_first<void**>(1);

    pattern = hook::pattern("E8 ? ? ? ? 5B 5F C3 C1 E6");
    cFEng::MakeLoadedPackagesDirty = (decltype(cFEng::MakeLoadedPackagesDirty))injector::GetBranchDestination(pattern.get_first(0)).as_int();

    pattern = hook::pattern("E8 ? ? ? ? 83 C4 ? C2 ? ? ? ? C2");
    cFEng::PopPackage = (decltype(cFEng::PopPackage))injector::GetBranchDestination(pattern.get_first(0)).as_int();

    pattern = hook::pattern("E8 ? ? ? ? 83 C4 ? 8D 44 24 ? 50 FF D7");
    static auto GetActualDeltaTime = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        actualDeltaTime = *(float*)(regs.esp);
    });

    pattern = hook::pattern("A1 ? ? ? ? ? ? 68 ? ? ? ? 50 FF 51 ? 85 C0 75 ? E8");
    static auto BeforeResetHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        WFP::onBeforeReset().executeAll();
    });

    pattern = hook::pattern("A1 ? ? ? ? ? ? 83 C4 ? 50");
    Direct3DDevice.SetAddress(*pattern.get_first<IDirect3DDevice9**>(1));
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
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("68 20 03 00 00 BE 58 02 00 00"));
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
