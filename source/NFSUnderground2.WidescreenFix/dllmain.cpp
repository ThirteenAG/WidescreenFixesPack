#include "stdafx.h"
#include "d3d9.h"
#include "FixSettings.hpp"
#include "../Shared/NFS/Cursor.hpp"

import ComVars;

// The settings' runtime interface for menu hosts and editors (AsiSettings_GetApi)
ASI_SETTINGS_EXPORT(FixSettings())

std::once_flag of;

void MainLoop()
{
    std::call_once(of, []()
    {
        WFP::onGameInitEvent().executeAll();
    });

    CursorConfinement::Frame(hWnd.get());
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
    NFSCursor::Initialize();
    auto pattern = hook::pattern("A1 ? ? ? ? 68 ? ? ? ? 50 FF 15");
    hWnd.SetAddress(*pattern.get_first<HWND*>(1));
    WindowRect.SetAddress(*pattern.get_first<tagRECT*>(6));

    pattern = hook::pattern("A1 ? ? ? ? 33 FF 3B C7 74 ? A1");
    dwWindowedMode.SetAddress(*pattern.get_first<uint32_t*>(1));

    pattern = find_pattern("A1 ? ? ? ? 56 8B F0 75", "A1 ? ? ? ? 83 F8 ? 74 ? 40");
    g_RacingResolution.SetAddress(*pattern.get_first<int*>(1));

    pattern = hook::pattern("E8 ? ? ? ? A1 ? ? ? ? A3 ? ? ? ? E8");
    static auto MainLoopHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        MainLoop();
    });

    pattern = hook::pattern("A1 ? ? ? ? 3B C3 74 ? ? ? 75");
    GameFlowManager::pRaceCoordinator = *pattern.get_first<int**>(1);

    pattern = hook::pattern("83 3D ? ? ? ? 06 ? ? A1");
    nGameState.SetAddress(*pattern.get_first<int32_t*>(2));

    pattern = hook::pattern("A0 ? ? ? ? 84 C0 74 ? C6 05 ? ? ? ? ? E9");
    bExitFEOptionsSystem.SetAddress(*pattern.get_first<uint8_t*>(1));

    pattern = hook::pattern("8B 0D ? ? ? ? 3B CB 74 ? E8 ? ? ? ? 83 FF");
    cFEng::pInstance = *pattern.get_first<void**>(2);
    cFEng::MakeLoadedPackagesDirty = (decltype(cFEng::MakeLoadedPackagesDirty))injector::GetBranchDestination(pattern.get_first(10)).as_int();

    // The frame's time step, after PrepareRealTimestep. The time MainLoop passes to it is measured from the end
    // of the previous wait, so with a frame rate limit it leaves out the wait; PrepareRealTimestep stores the
    // whole step (with the wait, in 1/1800 s units, the remainder kept for the next frame).
    static float* FrameTimeStep = *hook::get_pattern<float*>("D9 05 ? ? ? ? D8 05 ? ? ? ? D9 05 ? ? ? ? D8 C9 E8 ? ? ? ? 85 C0", 2);
    pattern = hook::pattern("E8 ? ? ? ? 83 C4 ? E8 ? ? ? ? B9 ? ? ? ? 8B F8");
    static auto GetActualDeltaTime = safetyhook::create_mid(pattern.get_first(8), [](SafetyHookContext& regs)
    {
        actualDeltaTime = *FrameTimeStep;
    });

    pattern = hook::pattern("E8 ? ? ? ? A1 ? ? ? ? 85 C0 74 ? ? ? 50 FF 51");
    static auto BeforeResetHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        WFP::onBeforeReset().executeAll();
    });

    pattern = hook::pattern("A1 ? ? ? ? ? ? 83 C4 ? 50 FF 92");
    Direct3DDevice.SetAddress(*pattern.get_first<IDirect3DDevice9**>(1));
    static auto BeforeEndSceneHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        WFP::onEndScene().executeAll();
    });

    WFP::onInitEvent().executeAll();

    // Serve the initialized settings to editors while the game runs.
    asi_settings::LiveLink::Start();

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
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("C7 00 80 02 00 00 C7 01 E0 01 00 00"));
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
