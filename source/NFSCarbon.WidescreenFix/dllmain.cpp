#include "stdafx.h"
#include "d3d9.h"
#include "FixSettings.hpp"
#include "../Shared/NFS/Cursor.hpp"

import ComVars;

// The settings' runtime interface for menu hosts and editors (AsiSettings_GetApi)
ASI_SETTINGS_EXPORT(FixSettings())

std::once_flag of;

SafetyHookInline shMainLoop = {};
void __cdecl MainLoop(float TickerDifference)
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

    shMainLoop.unsafe_ccall(TickerDifference);
}

SafetyHookInline shMiniMainLoop = {};
void __cdecl MiniMainLoop()
{
    std::call_once(of, []()
    {
        WFP::onGameInitEvent().executeAll();
    });

    CursorConfinement::Frame(hWnd.get());
    WFP::onGameProcessEvent().executeAll();

    bInMiniMainLoop = true;
    shMiniMainLoop.unsafe_ccall();
    bInMiniMainLoop = false;
}

void Init()
{
    NFSCursor::Initialize();
    auto pattern = hook::pattern("A1 ? ? ? ? 68 ? ? ? ? 50 FF 15 ? ? ? ? 8B 0D");
    hWnd.SetAddress(*pattern.get_first<HWND*>(1));
    WindowRect.SetAddress(*pattern.get_first<tagRECT*>(6));

    pattern = hook::pattern("8B 0D ? ? ? ? 2B FA");
    dwWindowedMode.SetAddress(*pattern.get_first<uint32_t*>(2));

    pattern = hook::pattern("A1 ? ? ? ? 8B 4C 24 ? 8D A4 24");
    g_RacingResolution.SetAddress(*pattern.get_first<int*>(1));

    pattern = hook::pattern("6A ? 68 ? ? ? ? 64 A1 ? ? ? ? 50 64 89 25 ? ? ? ? 83 EC ? ? ? ? ? 56 ? ? ? ? ? ? ? ? ? ? E8 ? ? ? ? E8");
    shMainLoop = safetyhook::create_inline(pattern.get_first(), MainLoop);

    pattern = hook::pattern("51 6A ? E8 ? ? ? ? A1");
    shMiniMainLoop = safetyhook::create_inline(pattern.get_first(), MiniMainLoop);

    pattern = hook::pattern("E8 ? ? ? ? 84 C0 0F 85 ? ? ? ? 8B 46 ? C7 80");
    GameFlowManager::IsPaused = decltype(GameFlowManager::IsPaused)(injector::GetBranchDestination(pattern.get_first()).get());

    pattern = hook::pattern("A1 ? ? ? ? 6A 01 6A 1C C7 44 24");
    Sim::Internal::mLastFrameTime.SetAddress(*pattern.get_first<float*>(1));

    pattern = hook::pattern("8B 0D ? ? ? ? 85 C9 74 ? E8 ? ? ? ? 84 C0 75 ? 8B CE E8");
    MoviePlayerInstance.SetAddress(*pattern.get_first<void**>(2));

    pattern = hook::pattern("83 3D ? ? ? ? 06 ? ? A1");
    nGameState.SetAddress(*pattern.get_first<int32_t*>(2));

    pattern = hook::pattern("8B 0D ? ? ? ? E8 ? ? ? ? 8B 0D ? ? ? ? 3B CF 5F");
    cFEng::pInstance = *pattern.get_first<void**>(2);
    cFEng::MakeLoadedPackagesDirty = (decltype(cFEng::MakeLoadedPackagesDirty))injector::GetBranchDestination(pattern.get_first(6)).as_int();

    pattern = hook::pattern("E8 ? ? ? ? C7 44 24 ? ? ? ? ? 8D 44 24");
    FEPackage::ForAllObjects = (decltype(FEPackage::ForAllObjects))injector::GetBranchDestination(pattern.get_first(0)).as_int();

    pattern = hook::pattern("A1 ? ? ? ? ? ? 68 ? ? ? ? 50 FF 52 ? A1");
    Direct3DDevice.SetAddress(*pattern.get_first<IDirect3DDevice9**>(1));
    static auto BeforeResetHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        WFP::onBeforeReset().executeAll();
    });

    pattern = hook::pattern("A1 ? ? ? ? ? ? 83 C4 ? 50 FF 91");
    static auto BeforeEndSceneHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        WFP::onEndScene().executeAll();
    });

    WFP::onInitEvent().executeAll();
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
