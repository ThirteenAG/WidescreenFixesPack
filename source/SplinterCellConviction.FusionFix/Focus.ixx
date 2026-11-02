module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/CursorConfinement.hpp"

export module Focus;

import ComVars;

namespace
{
    SafetyHookInline TickHook;
    SafetyHookInline PauseHook;
    std::atomic<uintptr_t> ManagedLevelInfo{};
    std::atomic<uint64_t> PauseChanges{};

    struct PausedWorld
    {
        uintptr_t gameInfo = 0;
        uintptr_t controller = 0;
        uint64_t changes = 0;
    };
    PausedWorld ManagedPause;

    template <typename T = uintptr_t>
    T Read(uintptr_t address)
    {
        return *reinterpret_cast<T*>(address);
    }

    int __fastcall Pause(uintptr_t actor, void* edx, int paused, uintptr_t controller, uint8_t input, int sound)
    {
        if (actor && Read(actor + 0x130) == ManagedLevelInfo.load())
            ++PauseChanges;
        return PauseHook.fastcall<int>(actor, edx, paused, controller, input, sound);
    }

    void UpdatePause(uintptr_t engine, bool wanted)
    {
        const auto level = Read(engine + 0x19C);
        const auto actors = level ? Read(level + 0x28) : 0;
        const auto info = actors && Read<int>(level + 0x2C) > 0 ? Read(actors) : 0;
        const auto gameInfo = info ? Read(info + 0x7FC) : 0;
        const bool valid = gameInfo && Read(gameInfo + 0x130) == info;

        if (ManagedPause.gameInfo)
        {
            if (!valid || ManagedPause.gameInfo != gameInfo || ManagedPause.changes != PauseChanges.load() ||
                Read(info + 0x6D0) != ManagedPause.controller)
            {
                ManagedPause = {};
                ManagedLevelInfo.store(0);
            }
            else if (!wanted)
            {
                PauseHook.unsafe_fastcall<int>(gameInfo, nullptr, 0, uintptr_t{ 0 }, uint8_t{ 1 }, 1);
                ManagedPause = {};
                ManagedLevelInfo.store(0);
            }
        }
        if (!wanted || !valid || ManagedPause.gameInfo || Read(info + 0x6D0))
            return;

        const auto client = Read(engine + 0x44);
        const auto viewports = client ? Read(client + 0x2C) : 0;
        const int count = client ? Read<int>(client + 0x30) : 0;
        if (!viewports || count < 1 || count > 8)
            return;
        for (int index = 0; index < count; ++index)
        {
            const auto viewport = Read(viewports + index * sizeof(uintptr_t));
            const auto controller = viewport ? Read(viewport + 0x30) : 0;
            const auto window = viewport ? Read(viewport + 0x3F4) : 0;
            if (!controller || !window || Read(controller + 0x130) != info || Read<HWND>(window + 4) != hGameWindow)
                continue;
            ManagedLevelInfo.store(info);
            const auto changes = PauseChanges.load();
            PauseHook.unsafe_fastcall<int>(gameInfo, nullptr, 1, controller, uint8_t{ 1 }, 1);
            if (changes == PauseChanges.load() && Read(info + 0x6D0) == controller)
                ManagedPause = { gameInfo, controller, changes };
            else
                ManagedLevelInfo.store(0);
            break;
        }
    }

    int __fastcall Tick(uint8_t* engine, void* edx, float delta)
    {
        FullscreenVideo.store(false, std::memory_order_relaxed);
        if (ProcessWindowResize)
            ProcessWindowResize();
        const bool suspend = hGameWindow && (IsIconic(hGameWindow) ||
            (GetForegroundWindow() != hGameWindow && FixSettings().Bool("MAIN.PauseOnFocusLoss")));
        UpdatePause(reinterpret_cast<uintptr_t>(engine), suspend);
        return TickHook.fastcall<int>(engine, edx, delta);
    }
}

export namespace Focus
{
    void Frame(bool overlay)
    {
        if (SuspendGameMouse)
            SuspendGameMouse(overlay);
        CursorConfinement::Suspend(overlay);
        CursorConfinement::Update(hGameWindow);
        CursorConfinement::Bind();
    }

    void Message(UINT message, WPARAM wParam)
    {
        if (CursorConfinement::Window.load() != hGameWindow)
            CursorConfinement::Update(hGameWindow);
        CursorConfinement::Message(message, wParam);
    }

    void Init()
    {
        CursorConfinement::SetEnabled(FixSettings().Bool("MAIN.ConfineCursor"));
        // This game clips directly from its window input code rather than DirectInput.
        auto slot = CursorConfinement::FindSlot(GetModuleHandleW(nullptr));
        if (slot && *slot && *slot != CursorConfinement::NativeClip)
        {
            CursorConfinement::OriginalClip.store(*slot);
            CursorConfinement::NativeSlot = slot;
            injector::WriteMemory<CursorConfinement::Clip>(slot, CursorConfinement::NativeClip, true);
        }
        FixSettings().OnChange("MAIN.ConfineCursor", []()
        {
            CursorConfinement::SetEnabled(FixSettings().Bool("MAIN.ConfineCursor"));
        });
        auto pause = hook::pattern("55 8B EC 83 7D 08 00 8B 81 30 01 00 00 74 0B 8B 4D 0C 89 88 D0 06 00 00 EB 07 83 A0 D0 06 00 00 00 56 33 F6");
        auto tick = hook::pattern("55 8D 6C 24 8C 81 EC 0C 01 00 00 53 56 57 33 FF 47 57 8B F1");
        PauseHook = safetyhook::create_inline(pause.get_first(), Pause);
        TickHook = safetyhook::create_inline(tick.get_first(), Tick);
    }
}
