#include "stdafx.h"
#include "d3d9.h"
#include "FixSettings.hpp"
#include "Shared/NFS/Cursor.hpp"
#include "Shared/WidescreenFixConflict.hpp"
// The settings' runtime interface for menu hosts and editors (AsiSettings_GetApi)
ASI_SETTINGS_EXPORT(FixSettings())

import ComVars;
import PostFX;

// A frame's events. The game's loops while it loads (MiniMainLoop) get them without the menu events.
void FrameEvents(bool menus)
{
    static std::once_flag gameInit;
    std::call_once(gameInit, [] { WFP::onGameInitEvent().executeAll(); });
    // The cursor and the frame
    if (menus)
        CursorConfinement::Suspend(DeveloperOverlay::BlocksInput());
    CursorConfinement::Frame(hWnd.get());
    WFP::onGameProcessEvent().executeAll();
    if (!menus)
        return;
    // Entering and leaving the pause menu
    static bool wasPaused = GameFlowManager::IsPaused();
    bool paused = GameFlowManager::IsPaused();
    if (paused != wasPaused)
        (paused ? WFP::onMenuEnterEvent() : WFP::onMenuExitEvent()).executeAll();
    wasPaused = paused;
    if (paused)
        WFP::onMenuDrawingEvent().executeAll();
}

injector::hook_back<void(__cdecl*)(float)> hbMainLoop;
injector::hook_back<void(__cdecl*)()> hbMiniMainLoop;

void __cdecl MainLoop(float TickerDifference)
{
    FrameEvents(true);
    hbMainLoop.fun(TickerDifference);
}

void __cdecl MiniMainLoop()
{
    FrameEvents(false);
    hbMiniMainLoop.fun();
}

void Init()
{
    NFSCursor::Initialize();
    auto pattern = hook::pattern("A1 ? ? ? ? 68 ? ? ? ? 50 FF 15");
    hWnd.SetAddress(*pattern.get_first<HWND*>(1));
    WindowRect.SetAddress(*pattern.get_first<tagRECT*>(6));

    pattern = hook::pattern("39 1D ? ? ? ? 75 ? 53 FF 15 ? ? ? ? 53 FF 15");
    dwWindowedMode.SetAddress(*pattern.get_first<uint32_t*>(2));

    pattern = hook::pattern("A1 ? ? ? ? 8B 0C 85 ? ? ? ? 85 C9 75");
    g_RacingResolution.SetAddress(*pattern.get_first<int*>(1));

    pattern = hook::pattern("E8 ? ? ? ? A0 ? ? ? ? 83 C4 ? 84 C0 74 ? C6 05");
    hbMainLoop.fun = injector::MakeCALL(pattern.get_first(), MainLoop).get();
    // The loops while the game loads
    pattern = hook::pattern("E8 ? ? ? ? EB ? 8B 0D ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 84 C0 74 ? 8B 0D ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 8B 46 ? 68 ? ? ? ? 68 ? ? ? ? 50 E8 ? ? ? ? 83 C4 ? 50 E8 ? ? ? ? 8B 46");
    hbMiniMainLoop.fun = injector::MakeCALL(pattern.get_first(), MiniMainLoop).get();
    pattern = hook::pattern("E8 ? ? ? ? EB ? 8B 0D ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 84 C0 74 ? 8B 0D ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 8B 46 ? 68 ? ? ? ? 68 ? ? ? ? 50 E8 ? ? ? ? 83 C4 ? 50 E8 ? ? ? ? 83 C4");
    hbMiniMainLoop.fun = injector::MakeCALL(pattern.get_first(), MiniMainLoop).get();
    pattern = hook::pattern("E8 ? ? ? ? B9 ? ? ? ? E8 ? ? ? ? 84 C0 75 ? E8 ? ? ? ? 83 F8");
    hbMiniMainLoop.fun = injector::MakeCALL(pattern.get_first(), MiniMainLoop).get();
    pattern = hook::pattern("E8 ? ? ? ? 6A ? 6A ? B9 ? ? ? ? E8 ? ? ? ? 6A");
    hbMiniMainLoop.fun = injector::MakeCALL(pattern.get_first(), MiniMainLoop).get();
    pattern = hook::pattern("E8 ? ? ? ? 38 1D ? ? ? ? 75 ? 8B 0D");
    hbMiniMainLoop.fun = injector::MakeCALL(pattern.get_first(), MiniMainLoop).get();

    pattern = hook::pattern("E8 ? ? ? ? 84 C0 0F 85 ? ? ? ? 8B 86 ? ? ? ? B9");
    GameFlowManager::IsPaused = decltype(GameFlowManager::IsPaused)(injector::GetBranchDestination(pattern.get_first()).get());

    pattern = hook::pattern("D9 1D ? ? ? ? E8 ? ? ? ? 53 E8 ? ? ? ? D8 05 ? ? ? ? A1 ? ? ? ? 83 C4 04");
    Sim::Internal::mLastFrameTime.SetAddress(*pattern.get_first<float*>(2));

    pattern = hook::pattern("8B 15 ? ? ? ? ? ? ? 8B 41");
    pOptions = *pattern.get_first<void**>(2);

    pattern = hook::pattern("8B 0D ? ? ? ? E8 ? ? ? ? 5F 5E 59");
    cFEng::pInstance = *pattern.get_first<void**>(2);
    cFEng::MakeLoadedPackagesDirty = (decltype(cFEng::MakeLoadedPackagesDirty))injector::GetBranchDestination(pattern.get_first(6)).as_int();

    pattern = hook::pattern("8B 0D ? ? ? ? 83 C4 ? 8D 54 24 ? 52 C6 44 24");
    MoviePlayerInstance.SetAddress(*pattern.get_first<void**>(2));

    pattern = hook::pattern("83 3D ? ? ? ? 06 ? ? A1");
    nGameState.SetAddress(*pattern.get_first<int32_t*>(2));

    pattern = hook::pattern("E8 ? ? ? ? C7 44 24 ? ? ? ? ? 8D 44 24");
    FEPackage::ForAllObjects = (decltype(FEPackage::ForAllObjects))injector::GetBranchDestination(pattern.get_first(0)).as_int();

    pattern = hook::pattern("A1 ? ? ? ? ? ? 68 ? ? ? ? 50 FF 51 ? A1");
    Direct3DDevice.SetAddress(*pattern.get_first<IDirect3DDevice9**>(1));
    static auto BeforeResetHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs) { WFP::onBeforeReset().executeAll(); });

    // Cache the SMAA site before the EndScene hook changes its trailing instructions.
    SMAAHookSite = hook::pattern("E8 ? ? ? ? E8 ? ? ? ? 57 E8 ? ? ? ? A1 ? ? ? ? 8B 10 83 C4 04 50 FF 92 A8 00 00 00").get_first(10);

    pattern = hook::pattern("A1 ? ? ? ? ? ? 83 C4 ? 50 FF 92");
    static auto BeforeEndSceneHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs) { WFP::onEndScene().executeAll(); });

    WFP::onInitEvent().executeAll();
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
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("C7 00 80 02 00 00 C7 01 E0 01 00 00 C2 08 00"));
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
