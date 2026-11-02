module;

#include <stdafx.h>
#include "FixSettings.hpp"

// The game draws its loading screens on a second thread. While the game starts, that thread and the main thread can
// run the executable's protected code at the same time, which crashes now and then before the first menu appears.
// The loading screen thread waits until the main thread has shown its first frame.

export module Startup;

import Renderer;

export namespace NeoStartup
{
    inline SafetyHookInline LoadingScreenHook;
    inline DWORD MainThread = 0;
    inline std::atomic<bool> Started{ false };

    inline void __cdecl LoadingScreen()
    {
        if (Started.load(std::memory_order_acquire))
            LoadingScreenHook.ccall<void>();
    }

    inline void Frame()
    {
        if (GetCurrentThreadId() == MainThread)
            Started.store(true, std::memory_order_release);
    }

    inline void Install()
    {
        MainThread = GetCurrentThreadId();
        NeoRenderer::Started = Frame;
        auto loading = hook::pattern("53 8B 1D ? ? ? ? 85 DB 57 74 ? A1 ? ? ? ? 85 C0 74 ? 85 DB 74 ? 8B 08 F6 C5 20");
        LoadingScreenHook = safetyhook::create_inline(loading.get_first(), LoadingScreen, SafetyHookInline::StartDisabled);
        (void)LoadingScreenHook.enable();
    }
}
