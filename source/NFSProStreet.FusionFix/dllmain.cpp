#include "stdafx.h"
#include "d3d9.h"
#include "FixSettings.hpp"
#include "Shared/NFS/Cursor.hpp"
#include "Shared/WidescreenFixConflict.hpp"

import ComVars;
import PostFX;

// The settings' runtime interface for menu hosts and editors (AsiSettings_GetApi)
ASI_SETTINGS_EXPORT(FixSettings())

// Per frame: the fix's events, before the game's frame
static void FrameEvents()
{
    static std::once_flag gameInit;
    std::call_once(gameInit, []() { WFP::onGameInitEvent().executeAll(); });
    CursorConfinement::Suspend(DeveloperOverlay::BlocksInput());
    CursorConfinement::Frame(hWnd.get());
    WFP::onGameProcessEvent().executeAll();
}

// The pause menu's events, from the change of the game's paused state
static void MenuEvents()
{
    static bool wasPaused = GameFlowManager::IsPaused();
    bool paused = GameFlowManager::IsPaused();
    if (paused != wasPaused)
        (paused ? WFP::onMenuEnterEvent() : WFP::onMenuExitEvent()).executeAll();
    wasPaused = paused;
    if (paused)
        WFP::onMenuDrawingEvent().executeAll();
}
// The main loop
SafetyHookInline shMainLoop = {};
void __cdecl MainLoop(float TickerDifference)
{
    FrameEvents();
    MenuEvents();
    shMainLoop.unsafe_ccall(TickerDifference);
}

void Init()
{
    NFSCursor::Initialize();

    // Window, resolution and game flow
    auto pattern = hook::pattern("8B 0D ? ? ? ? 50 51 89 54 24");
    hWnd.SetAddress(*pattern.get_first<HWND*>(2));
    pattern = hook::pattern("A3 ? ? ? ? 89 35 ? ? ? ? C6 05");
    ResX.SetAddress(*pattern.get_first<int*>(1));
    ResY.SetAddress(*pattern.get_first<int*>(7));
    // The game flow manager, from a check of its state (cmp CurrentGameFlowState, GAMEFLOW_STATE_RACING)
    pattern = hook::pattern("E8 ? ? ? ? 83 3D ? ? ? ? 06 0F");
    TheGameFlowManager.SetAddress(reinterpret_cast<GameFlowManager*>(*pattern.get_first<uint8_t*>(7) - offsetof(GameFlowManager, CurrentGameFlowState)));
    pattern = hook::pattern("8B 15 ? ? ? ? F7 DA 1B D2 83 E2 ? 3B C3");
    dwWindowedMode.SetAddress(*pattern.get_first<uint32_t*>(2));
    // Loops
    pattern = hook::pattern("6A ? 68 ? ? ? ? 64 A1 ? ? ? ? 50 83 EC ? 56 A1 ? ? ? ? 33 C4 50 8D 44 24 ? 64 A3 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? E8 ? ? ? ? E8");
    shMainLoop = safetyhook::create_inline(pattern.get_first(), MainLoop);
    // The small loop that runs while the game loads
    pattern = hook::pattern("51 E9 ? ? ? ? 50 E8");
    static SafetyHookInline shMiniMainLoop = {};
    shMiniMainLoop = safetyhook::create_inline(pattern.get_first(), static_cast<void(__cdecl*)()>([]() { FrameEvents(); shMiniMainLoop.unsafe_ccall(); }));
    pattern = hook::pattern("E8 ? ? ? ? ? ? ? ? 84 C0 0F 57 C9");
    GameFlowManager::IsPaused = decltype(GameFlowManager::IsPaused)(injector::GetBranchDestination(pattern.get_first()).get());
    pattern = hook::pattern("F3 0F 10 05 ? ? ? ? B8 ? ? ? ? F3 0F 11 44 24");
    Sim::Internal::mLastFrameTime.SetAddress(*pattern.get_first<float*>(4));

    // Direct3D: the device, and the fix's events before a reset and before the end of a frame
    pattern = hook::pattern("A1 ? ? ? ? 8B 38 56 6A 4B 6A 15");
    Direct3DDevice.SetAddress(*pattern.get_first<IDirect3DDevice9***>(1));
    pattern = hook::pattern("81 C7 ? ? ? ? 57 50 FF D2");
    static auto BeforeResetHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext&) { WFP::onBeforeReset().executeAll(); });
    pattern = hook::pattern("8B 91 ? ? ? ? 57 FF D2 ? ? 8B 48");
    static auto BeforeEndSceneHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext&) { WFP::onEndScene().executeAll(); });
    // The fix's modules
    WFP::onInitEvent().executeAll();
    asi_settings::LiveLink::Start();
    // The asynchronous parts of the fix are done before the game's first frame
    static auto futures = WFP::onInitEventAsync().executeAllAsync();
    WFP::onGameInitEvent() += []()
    {
        for (auto& future : futures.get())
            future.wait();
        futures.get().clear();
    };
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        // The old package's NFSProStreet.GenericFix.asi patches the same code
        if (WidescreenFixConflict::Found({ L"NFSProStreet.GenericFix.asi" }))
            return;
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("C7 46 ? ? ? ? ? E8 ? ? ? ? C6 86 ? ? ? ? ? 5E C2 04 00"));
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
