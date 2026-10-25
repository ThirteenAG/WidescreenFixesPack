module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "../Shared/PatternCheck.hpp"

export module System;

// Start-up and window: no logos at start (MAIN.SkipIntro) and the window of the game's windowed mode (MAIN.WindowMode).

namespace
{
    HWND StyledWindow = nullptr;

    HWND* GameWindow = nullptr;

    void ApplyWindowStyle(HWND window)
    {
        if (window != StyledWindow && GameWindow && *GameWindow == window && IsWindowVisible(window))
        {
            StyledWindow = window;
            WindowedModeWrapper::GameHWND = window;
            WindowedModeWrapper::SetWindowLongA_Hook(window, GWL_STYLE, GetWindowLong(window, GWL_STYLE));
        }
    }

    BOOL WINAPI ShowWindowHook(HWND window, int command)
    {
        const auto result = ShowWindow(window, command);
        if (command != SW_HIDE && IsWindowVisible(window))
            ApplyWindowStyle(window);
        return result;
    }
}

export namespace System
{
    void Init()
    {
        auto& settings = FixSettings();

        if (settings.Bool("MAIN.SkipIntro"))
        {
            // The logo screens (uCapLogo, one boot step each) end as soon as their logo reports it has finished
            auto logo = hook::pattern("56 8B F1 8B 46 0C 80 78 3C 01 75");
            if (PatternUnique(logo, "ResidentEvil5.FusionFix: System"))
            {
                static SafetyHookMid LogoHook = safetyhook::create_mid(logo.get_first(6), [](SafetyHookContext& regs)
                {
                    if (regs.eax)
                        *reinterpret_cast<uint8_t*>(regs.eax + 0x3C) = 1;
                });
            }

            // The logo movie (movie\bh5logo or movie\re5logo, index 0 or 1) is skipped like a finished one (index 3);
            // the title's attract movie (index 2) stays
            auto movie = hook::pattern("83 EC 0C 55 56 8B F1 8B 46 10 33 ED 3B C5 0F 8C");
            if (PatternUnique(movie, "ResidentEvil5.FusionFix: System"))
            {
                static SafetyHookMid MovieHook = safetyhook::create_mid(movie.get_first(7), [](SafetyHookContext& regs)
                {
                    auto& index = *reinterpret_cast<int32_t*>(regs.esi + 0x10);
                    if (index == 0 || index == 1)
                        index = 3;
                });
            }
        }

        // MAIN.WindowMode: the window when the game's own fullscreen option is off; 1 = with a frame, 2 = borderless.
        // A framed window as big as the screen is placed with its frame outside the screen; no rounded corners.
        {
            WindowedModeWrapper::bBorderlessWindowed = settings.Int("MAIN.WindowMode") != 1;
            auto window = hook::pattern("8B 0D ? ? ? ? 89 44 24 20");
            if (PatternAvailable(window, "ResidentEvil5.FusionFix: System"))
            {
                GameWindow = *window.get_first<HWND*>(2);
                if (GameWindow && *GameWindow)
                    ApplyWindowStyle(*GameWindow);
            }

            IATHook::Replace(GetModuleHandleA(NULL), "USER32.DLL",
                std::forward_as_tuple("CreateWindowExA", WindowedModeWrapper::CreateWindowExA_Hook),
                std::forward_as_tuple("CreateWindowExW", WindowedModeWrapper::CreateWindowExW_Hook),
                std::forward_as_tuple("SetWindowLongA", WindowedModeWrapper::SetWindowLongA_Hook),
                std::forward_as_tuple("SetWindowLongW", WindowedModeWrapper::SetWindowLongW_Hook),
                std::forward_as_tuple("AdjustWindowRect", WindowedModeWrapper::AdjustWindowRect_Hook),
                std::forward_as_tuple("AdjustWindowRectEx", WindowedModeWrapper::AdjustWindowRectEx_Hook),
                std::forward_as_tuple("SetWindowPos", WindowedModeWrapper::SetWindowPos_Hook),
                std::forward_as_tuple("ShowWindow", ShowWindowHook)
            );
        }
    }
}
