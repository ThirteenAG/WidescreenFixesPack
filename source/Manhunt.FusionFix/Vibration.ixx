module;

#include <stdafx.h>
#include "common.h"
#include "FixSettings.hpp"
#include <atomic>
#include <thread>

export module Vibration;


export namespace MHRumble
{
    using SendFn = bool(*)(short, unsigned short, unsigned short);
    inline std::array<std::atomic<uint64_t>, 2> motors{};
    inline std::atomic<uint32_t> lastSent{ 0 };
    inline std::atomic<bool> enabled{ false }, running{ false }, closing{ false };
    inline SafetyHookInline largeHook, smallHook, stopHook;
    inline bool (*inputActive)() = nullptr;

    inline SendFn Sender()
    {
        auto module = GetModuleHandleW(L"Xidi.32.dll");
        return module ? reinterpret_cast<SendFn>(GetProcAddress(module, "XidiSendVibration")) : nullptr;
    }

    inline bool Focused()
    {
        DWORD process = 0;
        GetWindowThreadProcessId(GetForegroundWindow(), &process);
        return process == GetCurrentProcessId();
    }

    inline void Send(uint16_t low, uint16_t high)
    {
        if (!enabled || closing || !Focused())
            low = high = 0;
        const uint32_t value = uint32_t(low) | (uint32_t(high) << 16);
        if (lastSent.exchange(value) != value)
            if (auto send = Sender())
                send(0, low, high);
    }

    inline void Stop()
    {
        for (auto& motor : motors)
            motor = 0;
        Send(0, 0);
    }

    inline void Worker()
    {
        for (;;)
        {
            const DWORD now = GetTickCount();
            const bool allowed = enabled && !closing && Focused() && (!inputActive || inputActive());
            std::array<uint16_t, 2> strength{};
            for (size_t i = 0; i < motors.size(); ++i)
            {
                auto state = motors[i].load();
                const DWORD deadline = DWORD(state >> 32);
                if (state && (!allowed || int32_t(now - deadline) >= 0))
                {
                    motors[i].compare_exchange_strong(state, 0);
                    state = motors[i].load();
                }
                if (allowed && state && int32_t(now - DWORD(state >> 32)) < 0)
                    strength[i] = uint16_t(state);
            }
            Send(strength[0], strength[1]);
            if (!motors[0] && !motors[1])
            {
                running = false;
                if ((motors[0] || motors[1]) && !running.exchange(true))
                    continue;
                return;
            }
            Sleep(8);
        }
    }

    inline void Request(size_t motor, int16_t duration, uint16_t strength)
    {
        if (!enabled || closing || !Focused() || (inputActive && !inputActive()) || !Sender())
            return;
        const auto milliseconds = DWORD(std::clamp<int>(duration, 0, 5000));
        if (strength && milliseconds)
        {
            const DWORD now = GetTickCount();
            const uint64_t next = (uint64_t(now + milliseconds) << 32) | strength;
            auto state = motors[motor].load();
            do
            {
                if (state && int32_t(DWORD(state >> 32) - now) >= int32_t(milliseconds))
                    return;
            } while (!motors[motor].compare_exchange_weak(state, next));
        }
        else
            motors[motor] = 0;
        if (!running.exchange(true))
        {
            try
            {
                std::thread(Worker).detach();
            }
            catch (...)
            {
                running = false;
                Stop();
                OutputDebugStringA("Manhunt: controller vibration worker unavailable.\n");
            }
        }
    }

    inline uint8_t WINAPI Large(int16_t duration, uint8_t strength)
    {
        const auto result = largeHook.unsafe_stdcall<uint8_t>(duration, strength);
        Request(0, duration, uint16_t(strength) * 257);
        return result;
    }

    inline int16_t WINAPI Small(int16_t duration, uint8_t strength)
    {
        const auto result = smallHook.unsafe_stdcall<int16_t>(duration, strength);
        Request(1, duration, strength ? UINT16_MAX : 0);
        return result;
    }

    inline int __fastcall NativeStop(void* self, void*)
    {
        const auto result = stopHook.unsafe_thiscall<int>(self);
        Stop();
        return result;
    }

    inline void Shutdown()
    {
        closing = true;
        Stop();
    }

    inline void Install()
    {
        auto large = hook::pattern("8A 44 24 08 53 84 C0 66 8B 5C 24 08 75 15 C7 05 ? ? ? ? 00 00 00 00 5B C6 05");
        auto smallMotor = hook::pattern("80 7C 24 08 00 66 8B 44 24 04 75 14 C7 05 ? ? ? ? 00 00 00 00 C2 08 00");
        auto stop = hook::pattern("6A 00 83 C1 04 E8 ? ? ? ? C3 90 90 90 90 90 D9 44 24 04 D8 15");
        largeHook = safetyhook::create_inline(large.get_first(), Large);
        smallHook = safetyhook::create_inline(smallMotor.get_first(), Small);
        stopHook = safetyhook::create_inline(stop.get_first(), NativeStop);
        FixSettings().Availability("MAIN.Vibration", [](std::string_view value)
        {
            return value == "0" || Sender();
        });
        enabled = FixSettings().Bool("MAIN.Vibration");
        FixSettings().OnChange("MAIN.Vibration", []()
        {
            enabled = FixSettings().Bool("MAIN.Vibration");
            if (!enabled)
                Stop();
        });
    }
}
