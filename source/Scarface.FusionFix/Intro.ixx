module;

#include "stdafx.h"
#include "FixSettings.hpp"
#include "Frame.hpp"
#include <string_view>

export module Intro;

export namespace ScarfaceIntro
{
    inline SafetyHookInline DialogHook;
    inline void(__thiscall* Close)(void*, bool) = nullptr;
    inline bool(__cdecl* PauseContext)() = nullptr;
    inline bool Skip = true, Startup = true;

    inline bool StartupNotice(std::string_view text, bool system, unsigned options)
    {
        return system && options == 16 &&
            text.ends_with("[SGM_LOADSUCCESS]");
    }

    inline uintptr_t __fastcall Dialog(void* dialog, void*, const char* header, const char* text, bool system,
        unsigned options, const char* first, const char* second, const char* third, const char* fourth)
    {
        const auto result = DialogHook.unsafe_thiscall<uintptr_t>(dialog, header, text, system, options,
            first, second, third, fourth);
        if (Skip && Startup && !PauseContext() && text && StartupNotice(std::string_view(text, strnlen(text, 512)), system, options))
        {
            Startup = false;
            // Complete the native dialog lifecycle and publish its only result to the profile manager.
            Close(dialog, true);
        }
        return result;
    }

    inline void Install()
    {
        Skip = FixSettings().Bool("MAIN.SkipIntro");
        auto state = hook::pattern("E8 ? ? ? ? 84 C0 74 07 68 ? ? ? ? EB 1D 8B 0D");
        PauseContext = reinterpret_cast<decltype(PauseContext)>(injector::GetBranchDestination(state.get_first()).as_int());
        Close = reinterpret_cast<decltype(Close)>(hook::pattern("53 56 8B F1 33 DB F6 46 60 08 0F 84 ? ? ? ? 6A FF 53 53 53").get_first());
        DialogHook = safetyhook::create_inline(hook::pattern("83 EC 10 8B 44 24 24 8B 54 24 1C 53 8B 5C 24 2C 55 8B 6C 24 34 56 57").get_first(), Dialog);
        ScarfaceFrame::Handlers.push_back([](void*)
        {
            // A later load is reached through the pause context and must retain its notice.
            if (PauseContext())
                Startup = false;
        });
    }
}
