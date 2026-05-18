#include "stdafx.h"
#include "d3d9.h"

import ComVars;

void Init()
{
    auto pattern = hook::pattern("A1 ? ? ? ? 85 C0 8D 7E");
    hWnd.SetAddress(*pattern.get_first<HWND*>(1));

    pattern = hook::pattern("38 05 ? ? ? ? 75 ? 39 81 ? ? ? ? 74");
    dwWindowedMode.SetAddress(*pattern.get_first<uint32_t*>(2));

    pattern = hook::pattern("8B 45 ? 8B 48 ? 57");
    static auto MainLoopHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
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

    pCurrentMovieStatus.SetAddress(*hook::get_pattern<int32_t*>("BE ? ? ? ? 33 C0 B9 20 01 00 00 8B FE F3 AB", 1) + 0x1D);

    pattern = hook::pattern("8D 55 ? 8D B3");
    static auto BeforeResetHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        WFP::onBeforeReset().executeAll();
    });

    pattern = hook::pattern("A1 ? ? ? ? 8B 08 6A 00 50 FF 91 ? ? ? ? 8D 4E 18");
    Direct3DDevice.SetAddress(*pattern.get_first<IDirect3DDevice9**>(1));

    auto ptr = (int*)Direct3DDevice.get_ptr();
    BackbufferWidth.SetAddress(ptr + 2);
    BackbufferHeight.SetAddress(ptr + 3);

    pattern = hook::pattern("C7 45 ? ? ? ? ? 8D BB");
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
