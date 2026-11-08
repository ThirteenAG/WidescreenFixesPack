#include "stdafx.h"
#include "d3d9.h"
#include "FixSettings.hpp"
#include "Shared/NFS/Cursor.hpp"

import ComVars;
import PostFX;
#ifdef _DEBUG
import Speedhack;
#endif
// The settings' runtime interface for menu hosts and editors (AsiSettings_GetApi)
ASI_SETTINGS_EXPORT(FixSettings())

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

// Per frame: the fix's events, before the game's frame (debug builds also set the game's speed)
static void FrameEvents([[maybe_unused]] float debugSpeed)
{
    #ifdef _DEBUG
    SetSpeed(debugSpeed);
    #endif
    static std::once_flag gameInit;
    std::call_once(gameInit, []() { WFP::onGameInitEvent().executeAll(); });
    CursorConfinement::Suspend(DeveloperOverlay::BlocksInput());
    CursorConfinement::Frame(hWnd.get());
    WFP::onGameProcessEvent().executeAll();
}

void Init()
{
    NFSCursor::Initialize();
    #ifdef _DEBUG
    InitSpeedhack();
    #endif

    // Window and loops
    auto pattern = hook::pattern("A1 ? ? ? ? 8D 54 24 14 52 66 0F EF C0 50 89 7C 24 1C 66 0F D6");
    hWnd.SetAddress(*pattern.get_first<HWND*>(1));
    pattern = hook::pattern("A1 ? ? ? ? F7 D8 1B C0 83 E0");
    dwWindowedMode.SetAddress(*pattern.get_first<uint32_t*>(1));
    // MainLoop(int ticks): the ticker difference since the last frame, in 1/65536 ms
    pattern = find_pattern("F3 0F 2A 44 24 ? 56", "F3 0F 2A 44 24 ? F3 0F 59 05 ? ? ? ? F3 0F 59 05 ? ? ? ? 56");
    static SafetyHookInline shMainLoop = {};
    shMainLoop = safetyhook::create_inline(pattern.get_first(), static_cast<void(__cdecl*)(int32_t)>([](int32_t ticks) { FrameEvents(1.0f); MenuEvents(); shMainLoop.unsafe_ccall(ticks); }));
    // The small loop that runs while the game loads
    pattern = find_pattern("51 A1 ? ? ? ? 50 E8 ? ? ? ? ? ? ? ? ? ? ? ? 83 C4 ? ? ? ? ? ? ? ? ? ? ? 76", "51 A1 ? ? ? ? 50 E8 ? ? ? ? ? ? ? ? ? ? ? ? ? ? 83 C4", "51 E9 ? ? ? ? 50 E8");
    static SafetyHookInline shMiniMainLoop = {};
    shMiniMainLoop = safetyhook::create_inline(pattern.get_first(), static_cast<void(__cdecl*)()>([]() { FrameEvents(500.0f); shMiniMainLoop.unsafe_ccall(); }));
    pattern = hook::pattern("E8 ? ? ? ? 84 C0 F3 0F 10 54 24");
    GameFlowManager::IsPaused = decltype(GameFlowManager::IsPaused)(injector::GetBranchDestination(pattern.get_first()).get());
    pattern = hook::pattern("F3 0F 10 05 ? ? ? ? 6A ? 6A ? C7 44 24");
    Sim::Internal::mLastFrameTime.SetAddress(*pattern.get_first<float*>(4));

    // Direct3D: the device, and the fix's events before a reset and before the end of a frame
    pattern = hook::pattern("A1 ? ? ? ? ? ? 8D 96");
    Direct3DDevice.SetAddress(*pattern.get_first<IDirect3DDevice9**>(1));
    static auto BeforeResetHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext&) { WFP::onBeforeReset().executeAll(); });
    pattern = hook::pattern("8B 91 ? ? ? ? 53 56 57");
    static auto BeforeEndSceneHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext&) { WFP::onEndScene().executeAll(); });
    // The fix's modules; the asynchronous parts are done before the game's first frame
    WFP::onInitEvent().executeAll();
    asi_settings::LiveLink::Start();
    static auto futures = WFP::onInitEventAsync().executeAllAsync();
    WFP::onGameInitEvent() += []() { std::ranges::for_each(futures.get(), [](auto& future) { future.wait(); }); futures.get().clear(); };
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        //securom compatibility
        auto ModuleStart = (uintptr_t)GetModuleHandle(NULL);
        MEMORY_BASIC_INFORMATION mbi;
        auto pattern = hook::pattern("FF 25"); //jmp
        for (size_t i = 0; i < pattern.size(); i++)
        {
            auto addr = injector::ReadMemory<uintptr_t>(pattern.get(i).get<void>(2), true);
            if (VirtualQuery((PVOID)addr, &mbi, sizeof(MEMORY_BASIC_INFORMATION)) &&
                RangeEnd < addr && mbi.Protect == PAGE_EXECUTE_WRITECOPY && mbi.AllocationBase == (PVOID)ModuleStart)
            {
                RangeStart = (uintptr_t)mbi.BaseAddress;
                RangeEnd = (uintptr_t)mbi.BaseAddress + mbi.RegionSize;
            }
        }

        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::range_pattern(ModuleStart, RangeEnd, "E8 ? ? ? ? 84 C0 F3 0F 10 54 24"));
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
