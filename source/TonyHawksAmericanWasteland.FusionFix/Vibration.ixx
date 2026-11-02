module;

#include <stdafx.h>
#include <atomic>

export module Vibration;

import ComVars;

export namespace THAWVibration
{
    inline SafetyHookInline UnmuteHook, StopHook, PulseHook;
    inline const int32_t* Enabled = nullptr;
    inline std::array<std::atomic<uintptr_t>, 4> Devices{};
    inline std::array<std::atomic<uintptr_t>, 4> Pending{};
    inline std::array<std::atomic<bool>, 4> Desired{ true, true, true, true };
    inline bool Applied = true, Initialized = false;

    inline uintptr_t Device(int player)
    {
        const auto manager = VibrationManager.get();
        return manager ? *reinterpret_cast<uintptr_t*>(manager + 4 + player * 0x8c) : 0;
    }

    inline int Find(uintptr_t device)
    {
        for (int i = 0; i < 4; ++i)
            if (Device(i) == device && device) return i;
        return -1;
    }

    inline void Remember(int player, uintptr_t device, bool desired)
    {
        if (player < 0)
        {
            if (!device) return;
            const auto value = device | static_cast<uintptr_t>(desired);
            for (auto& pending : Pending)
            {
                auto current = pending.load(std::memory_order_relaxed);
                if ((current & ~uintptr_t(1)) == device || (!current && pending.compare_exchange_strong(current, value)))
                {
                    pending.store(value, std::memory_order_relaxed);
                    return;
                }
            }
            return;
        }
        Devices[player].store(device, std::memory_order_relaxed);
        Desired[player].store(desired, std::memory_order_relaxed);
    }

    inline void __fastcall Unmute(uintptr_t device, void*)
    {
        Remember(Find(device), device, true);
        if (*Enabled) UnmuteHook.thiscall<void>(device);
        else StopHook.thiscall<void>(device);
    }

    inline void __fastcall Stop(uintptr_t device, void*)
    {
        Remember(Find(device), device, false);
        StopHook.thiscall<void>(device);
    }

    inline void __fastcall Pulse(uintptr_t device, void*)
    {
        const int player = Find(device);
        if (*Enabled)
        {
            PulseHook.thiscall<void>(device);
            Remember(player, device, !*reinterpret_cast<uint8_t*>(device + 0xc4));
        }
        else
        {
            const bool desired = player >= 0 && Desired[player].load(std::memory_order_relaxed);
            StopHook.thiscall<void>(device);
            if (player >= 0) Remember(player, device, desired);
        }
    }

    inline void Frame()
    {
        if (!VibrationManager.is_initialized() || !VibrationManager.get()) return;
        const bool enabled = *Enabled != 0;
        const bool changed = !Initialized || enabled != Applied;
        for (int i = 0; i < 4; ++i)
        {
            const auto device = Device(i);
            const bool fresh = Devices[i].load(std::memory_order_relaxed) != device;
            if (!device)
            {
                Devices[i].store(0, std::memory_order_relaxed);
                continue;
            }
            if (fresh)
            {
                bool desired = !*reinterpret_cast<uint8_t*>(device + 0xc4);
                for (auto& pending : Pending)
                {
                    auto value = pending.load(std::memory_order_relaxed);
                    if ((value & ~uintptr_t(1)) == device && pending.compare_exchange_strong(value, 0))
                    {
                        desired = (value & 1) != 0;
                        break;
                    }
                }
                Remember(i, device, desired);
            }
            if (!enabled && (fresh || changed))
                StopHook.thiscall<void>(device);
            else if (enabled && Initialized && !Applied && changed && Desired[i].load(std::memory_order_relaxed))
                UnmuteHook.thiscall<void>(device);
        }
        Applied = enabled;
        Initialized = true;
    }

    inline void Install(const int32_t* enabled)
    {
        Enabled = enabled;
        auto manager = hook::pattern("8B 3D ? ? ? ? 33 F6 EB ? 8D A4 24 ? ? ? ? 8B FF 8B 0D ? ? ? ? 8A 44 F1 44 84 C0 74 ? 8B 44 24 08 85 C0 8B CF 56 74 ? E8 ? ? ? ? EB ? E8 ? ? ? ? 46 83 FE 04");
        auto unmute = hook::pattern("C6 81 C4 00 00 00 00 C3 CC CC CC CC CC CC CC CC");
        auto stop = hook::pattern("56 8B F1 8A 86 C4 00 00 00 33 C9 3A C1 75 ? 8B 96 D8 00 00 00");
        auto pulse = hook::pattern("56 8B F1 8A 86 C4 00 00 00 84 C0 74 ? C6 86 C4 00 00 00 00 5E E9 ? ? ? ?");
        VibrationManager.SetAddress(*manager.get_first<uintptr_t*>(2));
        UnmuteHook = safetyhook::create_inline(unmute.get_first(), Unmute);
        StopHook = safetyhook::create_inline(stop.get_first(), Stop);
        PulseHook = safetyhook::create_inline(pulse.get_first(), Pulse);
    }
}
