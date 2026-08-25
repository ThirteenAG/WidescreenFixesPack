#include "stdafx.h"
#include "d3d9.h"

import ComVars;

void Init()
{
    auto pattern = hook::pattern("A1 ? ? ? ? 85 C0 55");
    hWnd.SetAddress(*pattern.get_first<HWND*>(1));

    pattern = hook::pattern("E8 ? ? ? ? ? ? 8D 4C 24 ? 51 8D 54 24");
    static auto MainLoopHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        bPaused = true;

        static uint32_t fps = 0;
        static std::list<int> m_times;
        LARGE_INTEGER frequency;
        LARGE_INTEGER time;
        QueryPerformanceFrequency(&frequency);
        QueryPerformanceCounter(&time);

        if (m_times.size() == 50)
            m_times.pop_front();
        m_times.push_back(static_cast<int>(time.QuadPart));

        if (m_times.size() >= 2)
            fps = static_cast<uint32_t>(0.5f + (static_cast<float>(m_times.size() - 1) *
                                        static_cast<float>(frequency.QuadPart)) / static_cast<float>(m_times.back() - m_times.front()));
        fTimeStep = 1.0f / fps;

        static std::once_flag of;
        std::call_once(of, []()
        {
            WFP::onGameInitEvent().executeAll();
        });

        WFP::onGameProcessEvent().executeAll();
    });

    pattern = hook::pattern("D9 44 24 ? A1 ? ? ? ? D9 18");
    static auto TimingHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        bPaused = false;
    });

    pattern = hook::pattern("B9 ? ? ? ? 8D 7C 24 ? 52");
    static auto BeforeResetHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        WFP::onBeforeReset().executeAll();
    });

    auto dword_8D7DF0 = *hook::get_pattern<int*>("8B 0D ? ? ? ? 8B 44 24 14 8D 54 24 2C", 2);
    BackbufferWidth.SetAddress(dword_8D7DF0 + 2);
    BackbufferHeight.SetAddress(dword_8D7DF0 + 3);

    pattern = hook::pattern("A1 ? ? ? ? ? ? 8D 7B ? 57 6A ? 6A");
    Direct3DDevice.SetAddress(*pattern.get_first<IDirect3DDevice9**>(1));

    pattern = hook::pattern("E8 ? ? ? ? ? ? ? ? ? ? ? 50 FF 92 ? ? ? ? B8 ? ? ? ? E8 ? ? ? ? 8B C5 8D 6F ? BB");
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
