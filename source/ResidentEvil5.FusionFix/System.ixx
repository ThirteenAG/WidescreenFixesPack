module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module System;

import Overlay;

// Start-up and window: no logos, OK-only notices or PRESS ANY KEY at start (MAIN.SkipIntro) and the window of the game's windowed mode (MAIN.WindowMode).

namespace
{
    HWND StyledWindow = nullptr;

    GameRef<HWND> GameWindow;

    void ApplyWindowStyle(HWND window)
    {
        if (window != StyledWindow && GameWindow.is_initialized() && GameWindow == window && IsWindowVisible(window))
        {
            StyledWindow = window;
            WindowedModeWrapper::GameHWND = window;
            WindowedModeWrapper::SetWindowLongA_Hook(window, GWL_STYLE, GetWindowLong(window, GWL_STYLE));
        }
    }

    // MAIN.SkipIntro: the start-up notices that only have an OK button are confirmed at once. The game's alert
    // (uIdAlert00) shows one of 40 messages (+0x40); the alert system's state (+0x3974C) is 2 while it waits for input
    // and 3 while it closes (+0x58 the closing time).
    GameRef<uintptr_t> Alerts;

    // MAIN.PauseOnFocusLoss: the windowed game opens its pause menu when its window loses focus. With the setting off
    // only that check sees the window as active; the game's input still reads nothing while another window has focus.
    bool __fastcall HasFocusForPause(uintptr_t main, void* edx)
    {
        static auto& pause = FixSettings().Int("MAIN.PauseOnFocusLoss");
        return !pause || Overlay::HasFocus(main);
    }

    // The pause check of the game's input: it pauses when the player's device reports its loss, which also happens to
    // the keyboard and mouse while the window is unfocused (the input is blocked then, +0x5DC). With the setting off
    // that report is ignored while the window is unfocused and for a moment after it comes back; the block still
    // stops the input. While the overlay has the input, it is ignored too.
    GameRef<uintptr_t> sMain;
    SafetyHookInline InputBlockedHook;
    bool __fastcall InputBlockedPause(uintptr_t input, void*)
    {
        static auto& pause = FixSettings().Int("MAIN.PauseOnFocusLoss");
        static ULONGLONG unfocused = 0;
        const auto now = GetTickCount64();
        if (Overlay::BlocksInput() || (!pause && sMain.is_initialized() && sMain && !Overlay::HasFocus(sMain)))
            unfocused = now;
        if (unfocused && now - unfocused < 1000)
            return false;
        return InputBlockedHook.unsafe_thiscall<bool>(input);
    }

    // MAIN.SkipIntro: the title's PRESS ANY KEY screen takes the first check for a key as pressed, as a key of
    // the keyboard (a connected pad is still found as the player's device)
    using AnyKeyFn = bool(__fastcall*)(uintptr_t, void*, int, int, int);
    AnyKeyFn AnyKey = nullptr;
    bool __fastcall PressAnyKey(uintptr_t input, void* edx, int a, int b, int c)
    {
        static bool pressed = false;
        if (!pressed)
        {
            pressed = true;
            return true;
        }
        return AnyKey(input, edx, a, b, c);
    }

    SafetyHookInline AlertInputHook;
    void __fastcall AlertInput(uintptr_t alert, void*)
    {
        constexpr int LoadSuccessful = 8;   // "Load successful."
        constexpr int AutoSave = 21;        // "This game includes an auto-save feature."
        auto& state = *reinterpret_cast<int32_t*>(Alerts + 0x3974C);
        const auto message = *reinterpret_cast<int32_t*>(alert + 0x40);
        if (state == 2 && (message == LoadSuccessful || message == AutoSave))
        {
            *reinterpret_cast<float*>(alert + 0x58) = 0.0f;
            state = 3;
        }
        AlertInputHook.unsafe_thiscall(alert);
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
    float FrameDelta()
    {
        return *reinterpret_cast<const float*>(sMain.get() + 0x68);
    }

    void Init()
    {
        auto& settings = FixSettings();

        if (settings.Bool("MAIN.SkipIntro"))
        {
            // The logo screens (uCapLogo, one boot step each) end as soon as their logo reports it has finished
            auto logo = hook::pattern("56 8B F1 8B 46 0C 80 78 3C 01 75");
            static SafetyHookMid LogoHook = safetyhook::create_mid(logo.get_first(6), [](SafetyHookContext& regs)
            {
                if (regs.eax)
                    *reinterpret_cast<uint8_t*>(regs.eax + 0x3C) = 1;
            });

            // The notices with only an OK button on the way to the main menu
            auto alert = hook::pattern("A1 ? ? ? ? 8B 80 4C 97 03 00 83 E8 02 56 8B F1 74");
            Alerts.SetAddress(*alert.get_first<uintptr_t*>(1));
            AlertInputHook = safetyhook::create_inline(alert.get_first(), AlertInput);

            // The logo movie (movie\bh5logo or movie\re5logo, index 0 or 1) is skipped like a finished one (index 3);
            // the title's attract movie (index 2) stays
            auto movie = hook::pattern("83 EC 0C 55 56 8B F1 8B 46 10 33 ED 3B C5 0F 8C");
            static SafetyHookMid MovieHook = safetyhook::create_mid(movie.get_first(7), [](SafetyHookContext& regs)
            {
                auto& index = *reinterpret_cast<int32_t*>(regs.esi + 0x10);
                if (index == 0 || index == 1)
                    index = 3;
            });
        }

        if (settings.Bool("MAIN.SkipIntro"))
        {
            auto press = hook::pattern("8B 0D ? ? ? ? 6A 00 6A 00 6A 00 E8 ? ? ? ? 84 C0 75 4E");
            AnyKey = reinterpret_cast<AnyKeyFn>(injector::GetBranchDestination(press.get_first(12)).as_int());
            injector::MakeCALL(press.get_first(12), PressAnyKey, true);
        }

        // MAIN.PauseOnFocusLoss: the focus checks that pause the game (those after the "pause when unfocused" flag, +0x21
        // of the screen settings, and the pause menu's own); the other focus checks gate the input
        auto main = hook::pattern("8B 15 ? ? ? ? 38 42 21 74 ? 8B 0D ? ? ? ? E8 ? ? ? ? 84 C0");
        sMain.SetAddress(*hook::pattern(main).get_first<uintptr_t*>(13));
        main.for_each_result([](hook::pattern_match match)
        {
            injector::MakeCALL(match.get<uint8_t>(17), HasFocusForPause, true);
        });
        auto settingsFocus = hook::pattern("A1 ? ? ? ? 80 78 21 00 74 ? E8 ? ? ? ? 84 C0 75 ? B3 01 EB");
        auto extendedFocus = hook::pattern("8B 15 ? ? ? ? 80 7A 21 00 0F 84 ? ? ? ? E8 ? ? ? ? 84 C0 0F 85");
        auto titleFocus = hook::pattern("A1 ? ? ? ? 80 78 21 00 74 ? 8B 0D ? ? ? ? E8 ? ? ? ? 84 C0");
        auto movieFocus = hook::pattern("8B 15 ? ? ? ? 80 7A 21 00 74 ? 8B 0D ? ? ? ? E8 ? ? ? ? 84 C0 0F 84");
        injector::MakeCALL(settingsFocus.get_first(11), HasFocusForPause, true);
        injector::MakeCALL(extendedFocus.get_first(16), HasFocusForPause, true);
        injector::MakeCALL(titleFocus.get_first(17), HasFocusForPause, true);
        injector::MakeCALL(movieFocus.get_first(18), HasFocusForPause, true);
        // The pause menu's own check (offline games)
        auto menu = hook::pattern("80 B8 6C 05 00 00 00 75 ? F7 81 34 04 01 00 00 10 00 00 75 ? E8 ? ? ? ? 84 C0");
        injector::MakeCALL(menu.get_first(21), HasFocusForPause, true);
        auto blocked = hook::pattern("8A 81 DC 05 00 00 84 C0 8B 15 ? ? ? ? 75 44 80 B9 F0 05 00 00 01");
        InputBlockedHook = safetyhook::create_inline(blocked.get_first(), InputBlockedPause);

        // The opening movie has a 2.35:1 picture inside its 16:9 frame. On screens wider than 16:9 the game enlarges the
        // frame by 1.3235 so the picture fills the height; between 16:9 and 2.35:1 that cuts the picture's sides. The
        // enlargement is limited to the screen's width, so the whole picture shows (with thin bars above and below).
        auto movieZoom = hook::pattern("F3 0F 5E C2 F3 0F 2A D8 F3 0F 59 D8 F3 0F 2C C3 F3 0F 2A D9");
        static SafetyHookMid MovieZoomHook = safetyhook::create_mid(movieZoom.get_first(4), [](SafetyHookContext& regs)
        {
            // eax = the frame's width in pixels before the zoom (xmm0), xmm1 = the screen's width
            const auto width = float(int32_t(regs.eax));
            if (width > 0.0f && regs.xmm0.f32[0] * width > regs.xmm1.f32[0])
                regs.xmm0.f32[0] = regs.xmm1.f32[0] / width;
        });

        // MAIN.WindowMode: the window when the game's own fullscreen option is off; 1 = with a frame, 2 = borderless.
        // A framed window as big as the screen is placed with its frame outside the screen; no rounded corners.
        {
            WindowedModeWrapper::bBorderlessWindowed = settings.Int("MAIN.WindowMode") != 1;
            auto window = hook::pattern("8B 0D ? ? ? ? 89 44 24 20");
            GameWindow.SetAddress(*window.get_first<HWND*>(2));
            if (GameWindow.is_initialized() && GameWindow)
                ApplyWindowStyle(GameWindow);

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
