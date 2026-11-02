module;

#include "stdafx.h"
#include <d3d9.h>
#include "FixSettings.hpp"
#include <Xinput.h>
#ifndef DIRECTINPUT_VERSION
#define DIRECTINPUT_VERSION 0x0800
#endif
#include <dinput.h>


export module Gamepad;

import ComVars;

export namespace DPLPad
{
    SafetyHookInline stickHook, stateHook, effectHook;
    std::array<std::atomic<void*>, 4> systems{};
    IDirectInputDevice8A* previousDevice = nullptr;
    std::atomic<bool> deadzoneChanged{ true }, stopRequested{ true };
    bool deadzoneReady = false, vibrationReady = false, wasBlocked = false;

    template<class T> T& At(void* pointer, size_t offset)
    {
        return *reinterpret_cast<T*>(static_cast<uint8_t*>(pointer) + offset);
    }
    float Deadzone()
    {
        const float value = FixSettings().Float("MAIN.GamepadDeadzone");
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
        return !FixSettings().Bool("MAIN.Vibration") || BlockGameInput ||
            (hWnd.get() && GetForegroundWindow() != hWnd.get());
    }
    DWORD __fastcall SetState(void* self, void*, DWORD index, const XINPUT_VIBRATION* requested)
    {
        if (index < systems.size()) systems[index].store(self);
        if (!requested) return stateHook.unsafe_thiscall<DWORD>(self, index, requested);
        XINPUT_VIBRATION vibration = *requested;
        if (BlockVibration()) vibration = {};
        return stateHook.unsafe_thiscall<DWORD>(self, index, &vibration);
    }
    bool __fastcall Effect(void* self, void*, float left, float right)
    {
        if (BlockVibration())
        {
            auto* effect = At<IDirectInputEffect*>(self, 8);
            return !effect || SUCCEEDED(effect->Stop());
        }
        return effectHook.unsafe_thiscall<bool>(self, left, right);
    }
    void StopEffects()
    {
        const XINPUT_VIBRATION stopped{};
        for (DWORD i = 0; i < systems.size(); ++i)
            if (auto* system = systems[i].load()) stateHook.unsafe_thiscall<DWORD>(system, i, &stopped);
        const uintptr_t manager = dword_70C59C;
        if (!manager || !At<uint8_t>(reinterpret_cast<void*>(manager), 4)) return;
        auto* input = reinterpret_cast<void*>(manager);
        if (At<uint16_t>(input, 1424) == 1)
            if (auto* effect = At<IDirectInputEffect*>(input, 584 + 8)) effect->Stop();
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
        if (!manager || !At<uint8_t>(reinterpret_cast<void*>(manager), 4)) return;
        const bool blocked = BlockVibration();
        if (vibrationReady && (stopRequested.exchange(false) || (blocked && !wasBlocked))) StopEffects();
        wasBlocked = blocked;
        auto* input = reinterpret_cast<void*>(manager);
        auto* device = At<uint16_t>(input, 1424) == 1 ? At<IDirectInputDevice8A*>(input, 584 + 4) : nullptr;
        const bool changed = deadzoneChanged.exchange(false);
        if (deadzoneReady && device && (changed || device != previousDevice))
        {
            DIPROPDWORD property{};
            property.diph.dwSize = sizeof(property);
            property.diph.dwHeaderSize = sizeof(property.diph);
            property.diph.dwHow = DIPH_DEVICE;
            property.dwData = static_cast<DWORD>(std::lround(Deadzone() * 10000));
            if (FAILED(device->SetProperty(DIPROP_DEADZONE, &property.diph)))
                OutputDebugStringA("DriverParallelLines: native controller deadzone update failed.\n");
        }
        previousDevice = device;
    }
    bool HelpPressed()
    {
        if (BlockGameInput) return false;
        const uintptr_t address = dword_70C59C;
        if (!address) return false;
        auto* manager = reinterpret_cast<void*>(address);
        if (!At<uint8_t>(manager, 4)) return false;
        if (At<uint16_t>(manager, 1426) == 1 && At<DWORD>(manager, 956 + 40) == ERROR_SUCCESS &&
            (At<WORD>(manager, 956 + 24) & XINPUT_GAMEPAD_Y)) return true;
        return At<uint16_t>(manager, 1424) == 1 && (At<uint8_t>(manager, 584 + 216 + 3) & 0x80);
    }
    void Install()
    {
        FixSettings().Availability("MAIN.GamepadDeadzone", [](std::string_view) { return deadzoneReady; });
        FixSettings().Availability("MAIN.Vibration", [](std::string_view) { return vibrationReady; });
        auto stick = hook::pattern("F3 0F 10 0D ? ? ? ? F3 0F 10 44 24 04 0F 2F C8 76 23 0F 2F 05 ? ? ? ? 76 12");
        stickHook = safetyhook::create_inline(stick.get_first(), Stick);
        deadzoneReady = static_cast<bool>(stickHook);
        auto state = hook::pattern("8B 81 C0 02 00 00 85 C0 74 02 FF E0 B8 8F 04 00 00 C2 08 00");
        auto effect = hook::pattern("55 8B EC 83 EC 40 56 8B F1 33 C9 39 4E 08 89 4D F8 89 4D FC 75 07 32 C0");
        stateHook = safetyhook::create_inline(state.get_first(), SetState);
        effectHook = safetyhook::create_inline(effect.get_first(), Effect);
        vibrationReady = stateHook && effectHook;
        if (!vibrationReady)
        {
            stateHook.reset(); effectHook.reset();
        }
        FixSettings().OnChange("MAIN.GamepadDeadzone", []() { deadzoneChanged.store(true); });
        FixSettings().OnChange("MAIN.Vibration", []() { stopRequested.store(true); });
    }
}
