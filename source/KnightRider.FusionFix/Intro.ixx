module;
#include <stdafx.h>
#include "FixSettings.hpp"

export module Intro;


export namespace KnightRiderIntro
{
    inline SafetyHookInline UpdateHook;
    inline SafetyHookInline SplashHook;
    inline void (__thiscall* AcceptSplash)(void*) = nullptr;

    inline void __fastcall Splash(uint8_t* window, void*, void* frame)
    {
        SplashHook.thiscall<void>(window, frame);
        if (window[0x304] && !window[0x305])
            AcceptSplash(window);
    }

    inline int __fastcall Update(uint8_t* state, void*, void* frame)
    {
        const auto result = UpdateHook.thiscall<int>(state, frame);
        // Finish the legal/logo state through its normal transition and destruction path.
        state[20] = 1;
        return result;
    }

    inline void Init()
    {
        if (FixSettings().Bool("MAIN.SkipIntro"))
        {
            auto update = hook::pattern("56 8B F1 8B 4E 30 85 C9 74 ? 8B 54 24 08 8B 01 52 FF 50 0C 8B 46 34");
            UpdateHook = safetyhook::create_inline(update.get_first(), Update);
        }
        if (FixSettings().Bool("MAIN.SkipPressStart"))
        {
            auto accept = hook::pattern("56 8B F1 8A 86 04 03 00 00 C7 86 84 02 00 00 04 00 00 00 84 C0 74 14");
            AcceptSplash = reinterpret_cast<decltype(AcceptSplash)>(accept.get_first());
            auto splash = hook::pattern("56 57 8B 7C 24 0C 8B F1 57 E8 ? ? ? ? 8A 86 05 03 00 00 33 D2");
            SplashHook = safetyhook::create_inline(splash.get_first(), Splash);
        }
    }
}
