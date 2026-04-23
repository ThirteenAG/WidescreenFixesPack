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
    auto pattern = hook::pattern("A1 ? ? ? ? 52 33 FF");
    if (!pattern.empty())
        hWnd.SetAddress(*pattern.get_first<HWND*>(1));
    else
    {
        pattern = hook::pattern("B9 ? ? ? ? A3 ? ? ? ? E8 ? ? ? ? 8B 15");
        hWnd.SetAddress((HWND*)(*pattern.get_first<uintptr_t>(1) + 0x18));
    }

    pattern = hook::pattern("A1 ? ? ? ? F7 D8 1B C0 83 E0");
    dwWindowedMode.SetAddress(*pattern.get_first<uint32_t*>(1));

    pattern = find_pattern("F3 0F 2A 44 24 ? 56", "F3 0F 2A 44 24 ? F3 0F 59 05 ? ? ? ? F3 0F 59 05 ? ? ? ? 56");
    shMainLoop = safetyhook::create_inline(pattern.get_first(), MainLoop);

    pattern = find_pattern("51 A1 ? ? ? ? 50 E8 ? ? ? ? ? ? ? ? ? ? ? ? 83 C4 ? ? ? ? ? ? ? ? ? ? ? 76", "51 A1 ? ? ? ? 50 E8 ? ? ? ? ? ? ? ? ? ? ? ? ? ? 83 C4", "51 E9 ? ? ? ? 50 E8");
    shMiniMainLoop = safetyhook::create_inline(pattern.get_first(), MiniMainLoop);

    pattern = hook::pattern("E8 ? ? ? ? 84 C0 F3 0F 10 54 24");
    GameFlowManager::IsPaused = decltype(GameFlowManager::IsPaused)(injector::GetBranchDestination(pattern.get_first()).get());

    pattern = hook::pattern("F3 0F 10 05 ? ? ? ? 6A ? 6A ? C7 44 24");
    Sim::Internal::mLastFrameTime.SetAddress(*pattern.get_first<float*>(4));

    pattern = hook::pattern("A1 ? ? ? ? ? ? 8D 96");
    Direct3DDevice.SetAddress(*pattern.get_first<IDirect3DDevice9**>(1));
    static auto BeforeResetHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        WFP::onBeforeReset().executeAll();
    });

    pattern = hook::pattern("8B 91 ? ? ? ? 53 56 57");
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
        //securom compatibility
        auto ModuleStart = (uintptr_t)GetModuleHandle(NULL);
        MEMORY_BASIC_INFORMATION mbi;
        auto pattern = hook::pattern("FF 25"); //jmp
        for (size_t i = 0; i < pattern.size(); i++)
        {
            auto addr = injector::ReadMemory<uintptr_t>(pattern.get(i).get<void>(2), true);
            VirtualQuery((PVOID)addr, &mbi, sizeof(MEMORY_BASIC_INFORMATION));
            if (RangeEnd < addr && mbi.Protect == PAGE_EXECUTE_WRITECOPY && mbi.AllocationBase == (PVOID)ModuleStart)
            {
                RangeStart = (uintptr_t)mbi.BaseAddress;
                RangeEnd = (uintptr_t)mbi.BaseAddress + mbi.RegionSize;
            }
        }

        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::range_pattern(ModuleStart, RangeEnd, "C7 44 24 ? ? ? ? ? FF 15 ? ? ? ? 8D 54 24 0C 52"));
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
