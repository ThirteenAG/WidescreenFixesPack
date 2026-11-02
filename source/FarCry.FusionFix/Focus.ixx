module;

#include <stdafx.h>
#include <d3d9.h>
#include "FixSettings.hpp"

export module Focus;

import Window;
import Frontend;

export namespace FarCryFocus
{
    inline SafetyHookInline PauseHook;
    inline SafetyHookMid UpdateHook;
    inline void* Owner = nullptr;
    inline bool TimerWasPaused = false, Ready = false;

#ifdef _WIN64
    constexpr size_t systemOffset = 0x40, timersOffset = 0x778, timerPauseOffset = 0x54, entitiesOffset = 0x40;
#else
    constexpr size_t systemOffset = 0x24, timersOffset = 0x3A8, timerPauseOffset = 0x2C, entitiesOffset = 0x20;
#endif

    inline bool Services(void* game, bool pause)
    {
        auto bytes = static_cast<uint8_t*>(game);
        auto system = *reinterpret_cast<void**>(bytes + systemOffset);
        auto timers = *reinterpret_cast<uint8_t**>(bytes + timersOffset);
        if (!system || !timers)
            return false;
        auto entities = *reinterpret_cast<void**>(timers + entitiesOffset);
        if (!entities)
            return false;
        if (pause)
            TimerWasPaused = timers[timerPauseOffset] != 0;
        const bool timerPause = pause || TimerWasPaused;
        injector::WriteMemory<uint8_t>(timers + timerPauseOffset, timerPause, true);
        FarCryMenu::Virtual<void>(entities, 14, timerPause, false);
        if (auto sound = FarCryMenu::Virtual<void*>(system, 27))
            FarCryMenu::Virtual<void>(sound, 15, pause, pause);
        if (auto music = FarCryMenu::Virtual<void*>(system, 28))
            FarCryMenu::Virtual<void>(music, 6, pause);
        return true;
    }

    inline bool Paused(void* game, bool native)
    {
        if (Owner && (Owner != game || native))
            Owner = nullptr;
        const auto window = FarCryWindow::Window;
        const bool wanted = window && GetForegroundWindow() != window && FixSettings().Bool("MAIN.PauseOnFocusLoss") &&
            !FarCryMenu::Virtual<bool>(game, 57);
        if (!wanted || native)
        {
            if (Owner == game && Services(game, false))
                Owner = nullptr;
            return native;
        }
        if (!Owner && Services(game, true))
            Owner = game;
        return native || Owner == game;
    }

#ifndef _WIN64
    inline bool __fastcall Pause(void* game, void*, void* process)
    {
        const auto native = PauseHook.unsafe_fastcall<bool>(game, nullptr, process);
        return Paused(game, native);
    }
#else
    inline void Update(SafetyHookContext& regs)
    {
        const auto pause = Paused(reinterpret_cast<void*>(regs.rbx), (regs.rdi & 0xFF) != 0);
        regs.rdi = (regs.rdi & ~uintptr_t(0xFF)) | uintptr_t(pause);
    }
#endif

    inline void Init()
    {
        const auto module = GetModuleHandleW(L"CryGame.dll");
        if (!module || Ready)
            return;
#ifdef _WIN64
        auto site = hook::module_pattern(module, "40 0F B6 EF 80 7B 28 00 41 0F 45 EF 40 3A 6B 3C 74 ? 40 88 6B 3C");
        auto timers = hook::module_pattern(module, "48 8B 9F 78 07 00 00 C6 43 54 01 48 8B 4B 48");
#else
        auto site = hook::module_pattern(module, "53 56 8B F1 32 DB 38 9E 18 0C 00 00 74 ? 8B 06 FF 90 E4 00 00 00");
        auto timers = hook::module_pattern(module, "53 8B 5C 24 08 56 8B F1 8B 4E 24 88 5E 2C 8B 01 6A 01 FF 50 50");
#endif
#ifdef _WIN64
        UpdateHook = safetyhook::create_mid(site.get_first(), Update);
        Ready = bool(UpdateHook);
#else
        PauseHook = safetyhook::create_inline(site.get_first(), Pause);
        Ready = bool(PauseHook);
#endif
    }
}
