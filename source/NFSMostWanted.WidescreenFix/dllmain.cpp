#include "stdafx.h"
#include "d3d9.h"

import ComVars;

std::once_flag of;

injector::hook_back<void(__cdecl*)(float)> hbMainLoop;
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

    hbMainLoop.fun(TickerDifference);
}

injector::hook_back<void(__cdecl*)()> hbMiniMainLoop;
void __cdecl MiniMainLoop()
{
    std::call_once(of, []()
    {
        WFP::onGameInitEvent().executeAll();
    });

    WFP::onGameProcessEvent().executeAll();

    hbMiniMainLoop.fun();
}

void Init()
{
    auto hExecutableInstance = (size_t)GetModuleHandle(NULL);
    IMAGE_NT_HEADERS* ntHeader = (IMAGE_NT_HEADERS*)(hExecutableInstance + ((IMAGE_DOS_HEADER*)hExecutableInstance)->e_lfanew);
    SIZE_T size = ntHeader->OptionalHeader.SizeOfImage;
    DWORD oldProtect;
    VirtualProtect((VOID*)hExecutableInstance, size, PAGE_EXECUTE_READWRITE, &oldProtect);

    auto pattern = hook::pattern("A1 ? ? ? ? 68 ? ? ? ? 50 FF 15");
    hWnd.SetAddress(*pattern.get_first<HWND*>(1));
    WindowRect.SetAddress(*pattern.get_first<tagRECT*>(6));

    pattern = hook::pattern("39 1D ? ? ? ? 75 ? 53 FF 15 ? ? ? ? 53 FF 15");
    dwWindowedMode.SetAddress(*pattern.get_first<uint32_t*>(2));

    pattern = hook::pattern("A1 ? ? ? ? 8B 0C 85 ? ? ? ? 85 C9 75");
    g_RacingResolution.SetAddress(*pattern.get_first<int*>(1));

    pattern = hook::pattern("E8 ? ? ? ? A0 ? ? ? ? 83 C4 ? 84 C0 74 ? C6 05");
    hbMainLoop.fun = injector::MakeCALL(pattern.get_first(), MainLoop).get();

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

    pattern = hook::pattern("DB 05 ? ? ? ? 56 D8 25");
    cFEngGameInterface::GetMouseInfo = decltype(cFEngGameInterface::GetMouseInfo)(pattern.get_first());

    pattern = hook::pattern("8B 0D ? ? ? ? 83 C4 ? 8D 54 24 ? 52 C6 44 24");
    MoviePlayerInstance.SetAddress(*pattern.get_first<void**>(2));

    pattern = hook::pattern("A1 ? ? ? ? ? ? 68 ? ? ? ? 50 FF 51 ? A1");
    Direct3DDevice.SetAddress(*pattern.get_first<IDirect3DDevice9**>(1));
    static auto BeforeResetHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        WFP::onBeforeReset().executeAll();
    });

    pattern = hook::pattern("A1 ? ? ? ? ? ? 83 C4 ? 50 FF 92");
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
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("C7 00 80 02 00 00 C7 01 E0 01 00 00 C2 08 00"));
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
