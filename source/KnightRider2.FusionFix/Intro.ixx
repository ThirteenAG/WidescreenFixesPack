module;
#include <stdafx.h>
#include "FixSettings.hpp"

export module Intro;


export namespace KnightRider2Intro
{
    inline bool StartupMovie = false;
    inline SafetyHookInline LegalHook, VideoHook, SplashHook;
    inline void (__thiscall* AcceptSplash)(void*) = nullptr;

    inline int __fastcall Legal(uint8_t* state, void*, void* frame)
    {
        const auto result = LegalHook.thiscall<int>(state, frame);
        state[20] = 1;
        return result;
    }

    inline int __fastcall Video(uint8_t* state, void*, void* frame)
    {
        if (StartupMovie)
        {
            // Complete only the startup logo/title videos through the owner's normal state transition.
            state[20] = 1;
            return 0;
        }
        return VideoHook.thiscall<int>(state, frame);
    }

    inline int __fastcall Splash(uint8_t* window, void*, void* frame)
    {
        const auto result = SplashHook.thiscall<int>(window, frame);
        if (window[604] && !window[605])
            AcceptSplash(window);
        return result;
    }

    inline void Init()
    {
        if (FixSettings().Bool("MAIN.SkipIntro"))
        {
            auto legal = hook::pattern("83 EC 14 8B 44 24 18 56 8B F1 D9 00 D9 5C 24 04 D9 40 04 8B 40 08 8B 4E 2C 89 44 24 0C 8B 46 10 D9 5C 24 08 48 89 4C 24 10 89 74 24 14 74 16 48 75 31 8D 54 24 04 8B CE 52 E8 ? ? ? ? 5E 83 C4 14 C2 04 00 8B 4E 30 85 C9 74 05 E8 ? ? ? ? 51 8B CE 8B C4 6A 02 C7 00 00 00 00 00 E8 ? ? ? ? 5E 83 C4 14 C2 04 00 90 90 90 90 90 90 56");
            LegalHook = safetyhook::create_inline(legal.get_first(), Legal);
        }
        if (FixSettings().Bool("MAIN.SkipIntro"))
        {
            auto video = hook::pattern("51 53 56 8B F1 8B 46 10 83 F8 04 57 0F 87 8E 01 00 00 FF 24 85 ? ? ? ?");
            VideoHook = safetyhook::create_inline(video.get_first(), Video);
        }
        if (FixSettings().Bool("MAIN.SkipPressStart"))
        {
            auto accept = hook::pattern("56 8B F1 8A 86 5C 02 00 00 C7 86 C8 01 00 00 04 00 00 00 84 C0 74 14 8B 8E 0C 02 00 00");
            AcceptSplash = reinterpret_cast<decltype(AcceptSplash)>(accept.get_first());
            auto splash = hook::pattern("56 57 8B 7C 24 0C 8B F1 57 E8 ? ? ? ? 8A 86 5D 02 00 00 84 C0 0F 85 1F 01 00 00");
            SplashHook = safetyhook::create_inline(splash.get_first(), Splash);
        }
    }
}
