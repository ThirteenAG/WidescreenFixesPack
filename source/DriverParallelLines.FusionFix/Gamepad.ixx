module;

#include "stdafx.h"
#include <d3d9.h>
#include "FixSettings.hpp"
#include "Shared/PadInput.hpp"
#ifndef DIRECTINPUT_VERSION
#define DIRECTINPUT_VERSION 0x0800
#endif
#include <dinput.h>


export module Gamepad;

import ComVars;

export namespace DPLPad
{
    SafetyHookInline stickHook, stateHook, effectHook;
    std::mutex vibrationMutex;
    std::array<void*, 4> systems{};
    std::array<bool, 4> pendingMotorStops{};
    IDirectInputEffect* pendingEffect = nullptr;
    IDirectInputDevice8A* previousDevice = nullptr;
    float previousDeadzone = -1.0f;
    bool deadzoneFailed = false;
    std::atomic<bool> vibrationEnabled{ true };
    std::atomic<float> deadzoneSetting{ 0.15f };
    bool wasBlocked = false;
    ULONGLONG nextStopAttempt = 0;

    template<class T> T& At(void* pointer, size_t offset)
    {
        return *reinterpret_cast<T*>(static_cast<uint8_t*>(pointer) + offset);
    }
    float Deadzone()
    {
        const float value = deadzoneSetting.load(std::memory_order_relaxed);
        return std::isfinite(value) ? std::clamp(value, 0.0f, 0.5f) : 0.15f;
    }
    double __stdcall Stick(float value)
    {
        if (!std::isfinite(value)) return 0;
        const float deadzone = Deadzone(), magnitude = std::abs(value);
        if (magnitude <= deadzone) return 0;
        return std::copysign(std::clamp((magnitude - deadzone) / (0.9f - deadzone), 0.0f, 1.0f), value);
    }
    bool BlockVibration()
    {
        return !vibrationEnabled.load(std::memory_order_relaxed) || BlockGameInput ||
            (hWnd.is_initialized() && hWnd.get() && GetForegroundWindow() != hWnd.get());
    }
    DWORD __fastcall SetState(void* self, void*, DWORD index, const XINPUT_VIBRATION* requested)
    {
        const std::lock_guard lock(vibrationMutex);
        if (index < systems.size()) systems[index] = self;
        if (!requested) return stateHook.unsafe_thiscall<DWORD>(self, index, requested);
        const bool blocked = BlockVibration();
        const XINPUT_VIBRATION vibration = blocked ? XINPUT_VIBRATION{} : *requested;
        const auto result = stateHook.unsafe_thiscall<DWORD>(self, index, &vibration);
        if (index < pendingMotorStops.size() && (blocked || result == ERROR_SUCCESS))
            pendingMotorStops[index] = result != ERROR_SUCCESS;
        return result;
    }
    bool __fastcall Effect(void* self, void*, float left, float right)
    {
        const std::lock_guard lock(vibrationMutex);
        auto* effect = At<IDirectInputEffect*>(self, 8);
        if (BlockVibration())
        {
            const bool stopped = !effect || SUCCEEDED(effect->Stop());
            if (!stopped)
                pendingEffect = effect;
            else if (pendingEffect == effect)
                pendingEffect = nullptr;
            return stopped;
        }
        const bool accepted = effectHook.unsafe_thiscall<bool>(self, left, right);
        if (accepted && pendingEffect == effect)
            pendingEffect = nullptr;
        return accepted;
    }
    IDirectInputEffect* CurrentEffect()
    {
        const uintptr_t manager = dword_70C59C;
        auto* input = reinterpret_cast<void*>(manager);
        if (!input || !At<uint8_t>(input, 4) || At<uint16_t>(input, 1424) != 1) return nullptr;
        return At<IDirectInputEffect*>(input, 584 + 8);
    }
    void TickVibration()
    {
        const std::lock_guard lock(vibrationMutex);
        const bool blocked = BlockVibration();
        const auto now = GetTickCount64();
        if (blocked && !wasBlocked)
        {
            for (size_t index = 0; index < systems.size(); ++index)
                pendingMotorStops[index] = systems[index] != nullptr;
            pendingEffect = CurrentEffect();
            nextStopAttempt = now;
        }
        wasBlocked = blocked;
        if (now < nextStopAttempt) return;
        nextStopAttempt = now + 125;
        const XINPUT_VIBRATION stopped{};
        for (DWORD index = 0; index < systems.size(); ++index)
        {
            if (pendingMotorStops[index])
                pendingMotorStops[index] = systems[index] && stateHook.unsafe_thiscall<DWORD>(systems[index], index, &stopped) != ERROR_SUCCESS;
        }
        if (pendingEffect)
        {
            auto* effect = CurrentEffect();
            if (effect != pendingEffect || SUCCEEDED(effect->Stop()))
                pendingEffect = nullptr;
        }
    }
    void Drain(void* manager)
    {
        if (At<uint16_t>(manager, 1424) == 1)
            std::memset(static_cast<uint8_t*>(manager) + 584 + 152, 0, 160);
        if (At<uint16_t>(manager, 1426) == 1)
        {
            auto* controller = static_cast<uint8_t*>(manager) + 956;
            std::memset(controller + 20, 0, 16);
            std::memset(controller + 232, 0, 142);
        }
    }
    void Frame()
    {
        const uintptr_t manager = dword_70C59C;
        auto* input = reinterpret_cast<void*>(manager);
        auto* device = input && At<uint8_t>(input, 4) && At<uint16_t>(input, 1424) == 1 ? At<IDirectInputDevice8A*>(input, 584 + 4) : nullptr;
        const float deadzone = Deadzone();
        if (device && (device != previousDevice || deadzone != previousDeadzone))
        {
            DIPROPDWORD property{};
            property.diph.dwSize = sizeof(property);
            property.diph.dwHeaderSize = sizeof(property.diph);
            property.diph.dwHow = DIPH_DEVICE;
            property.dwData = static_cast<DWORD>(std::lround(deadzone * 10000));
            if (FAILED(device->SetProperty(DIPROP_DEADZONE, &property.diph)))
            {
                if (!std::exchange(deadzoneFailed, true))
                    OutputDebugStringA("DriverParallelLines: native controller deadzone update failed.\n");
                return;
            }
            previousDeadzone = deadzone;
        }
        deadzoneFailed = false;
        previousDevice = device;
    }
    bool HelpPressed(bool acceptInput)
    {
        static std::array<bool, XUSER_MAX_COUNT> held{};
        const auto getState = PadInput::XInput().getState;
        if (!getState)
            return false;
        bool pressed = false;
        for (DWORD slot = 0; slot < XUSER_MAX_COUNT; ++slot)
        {
            XINPUT_STATE state{};
            const bool down = getState(slot, &state) == ERROR_SUCCESS && (state.Gamepad.wButtons & XINPUT_GAMEPAD_Y);
            if (down && !held[slot])
                pressed = true;
            held[slot] = down;
        }
        return acceptInput && !BlockGameInput && pressed;
    }
    // The shoulders as page keys of the Plugins menu: -1 left, +1 right
    bool ShoulderHeld(int side)
    {
        const auto getState = PadInput::XInput().getState;
        if (!getState || BlockGameInput)
            return false;
        const WORD mask = side < 0 ? XINPUT_GAMEPAD_LEFT_SHOULDER : XINPUT_GAMEPAD_RIGHT_SHOULDER;
        for (DWORD slot = 0; slot < XUSER_MAX_COUNT; ++slot)
        {
            XINPUT_STATE state{};
            if (getState(slot, &state) == ERROR_SUCCESS && (state.Gamepad.wButtons & mask))
                return true;
        }
        return false;
    }
    void Install()
    {
        BindLiveSetting("MAIN.GamepadDeadzone", deadzoneSetting);
        BindLiveSetting("MAIN.Vibration", vibrationEnabled);
        auto stick = hook::pattern("F3 0F 10 0D ? ? ? ? F3 0F 10 44 24 04 0F 2F C8 76 23 0F 2F 05 ? ? ? ? 76 12");
        stickHook = safetyhook::create_inline(stick.get_first(), Stick);
        auto state = hook::pattern("8B 81 C0 02 00 00 85 C0 74 02 FF E0 B8 8F 04 00 00 C2 08 00");
        auto effect = hook::pattern("55 8B EC 83 EC 40 56 8B F1 33 C9 39 4E 08 89 4D F8 89 4D FC 75 07 32 C0");
        stateHook = safetyhook::create_inline(state.get_first(), SetState);
        effectHook = safetyhook::create_inline(effect.get_first(), Effect);
    }
}
