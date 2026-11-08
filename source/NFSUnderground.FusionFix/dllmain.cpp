#include "stdafx.h"
#include "d3d9.h"
#include "automation/asi_automation_frames.h"
#include "FixSettings.hpp"
#include "Shared/NFS/Cursor.hpp"
#include "Shared/WidescreenFixConflict.hpp"

// The settings' runtime interface for menu hosts and editors (AsiSettings_GetApi)
ASI_SETTINGS_EXPORT(FixSettings())
ASI_AUTOMATION_FRAMES_EXPORT()

import ComVars;
import PostFX;

void MainLoop()
{
    static std::once_flag gameInit;
    std::call_once(gameInit, []()
    {
        WFP::onGameInitEvent().executeAll();
    });

    CursorConfinement::Suspend(DeveloperOverlay::BlocksInput());
    CursorConfinement::Frame(hWnd.get());
    WFP::onGameProcessEvent().executeAll();

    static bool oldMenuState = GameFlowManager::IsPaused();
    bool curMenuState = GameFlowManager::IsPaused();
    if (curMenuState != oldMenuState)
        (curMenuState ? WFP::onMenuEnterEvent() : WFP::onMenuExitEvent()).executeAll();
    oldMenuState = curMenuState;
    if (curMenuState)
        WFP::onMenuDrawingEvent().executeAll();
}

// The fix supports Speed.exe 1.4 (the US exe and its no-CD versions, and the Russian 1.4)
void Init()
{
    NFSCursor::Initialize();

    auto pattern = hook::pattern("A1 ? ? ? ? 68 ? ? ? ? 50 FF 15");
    hWnd.SetAddress(*pattern.get_first<HWND*>(1));
    WindowRect.SetAddress(*pattern.get_first<tagRECT*>(6));
    // The window mode, the resolution and the main loop
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

    pattern = hook::pattern("A1 ? ? ? ? 85 C0 74 ? BB");
    pCurrentMoviePlayer.SetAddress(*pattern.get_first<void**>(1));

    pattern = hook::pattern("E8 ? ? ? ? 3B C6 74 ? C7 40");
    FEPkgMgr_FindPackage = (decltype(FEPkgMgr_FindPackage))injector::GetBranchDestination(pattern.get_first(0)).as_int();

    pattern = hook::pattern("E8 ? ? ? ? 85 C0 75 ? A1 ? ? ? ? 85 C0 74 ? 8B 40 ? 56 8B CF E8 ? ? ? ? 5E");
    FEngFindObject = (decltype(FEngFindObject))injector::GetBranchDestination(pattern.get_first(0)).as_int();

    pattern = hook::pattern("E8 ? ? ? ? 8A 43 ? 84 C0 74 ? E8");
    FEngSetInvisible = (decltype(FEngSetInvisible))injector::GetBranchDestination(pattern.get_first(0)).as_int();

    pattern = hook::pattern("E8 ? ? ? ? 8B 75 ? 3B F3 74 ? 81 4E");
    FEHashUpper = (decltype(FEHashUpper))injector::GetBranchDestination(pattern.get_first(0)).as_int();
    // EAXSound::Update(RealTimeElapsed) in MainLoop
    pattern = hook::pattern("52 E8 ? ? ? ? 8B 2D ? ? ? ? BF");
    RealTimeElapsed.SetAddress(*pattern.get_first<float*>(8));

    pattern = hook::pattern("8B 44 24 ? 8B 4C 24 ? ? ? 50 8B 44 24 ? 51 50");
    static auto FEPkgMgr_SendMessageToPackageHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs) { onMenuScreenBaseNotify().executeAll((MenuScreen*)regs.esi); });
    // The device's reset and the end of a frame
    pattern = hook::pattern("BE ? ? ? ? ? ? ? ? 50 FF 91");
    static auto BeforeResetHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext&) { WFP::onBeforeReset().executeAll(); });
    pattern = hook::pattern("A1 ? ? ? ? ? ? 83 C4 ? 50");
    Direct3DDevice.SetAddress(*pattern.get_first<IDirect3DDevice9**>(1));
    static auto BeforeEndSceneHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext&) { WFP::onEndScene().executeAll(); });
    // The fix's modules
    WFP::onInitEvent().executeAll();
    // Serve the initialized settings to editors while the game runs.
    asi_settings::LiveLink::Start();
    // The tasks started at init are done before the game's first frame
    static auto futures = WFP::onInitEventAsync().executeAllAsync();
    WFP::onGameInitEvent() += []() { for (auto& f : futures.get()) f.wait(); futures.get().clear(); };
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        if (WidescreenFixConflict::Found())
            return;
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("68 20 03 00 00 BE 58 02 00 00"));
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
