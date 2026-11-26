module;

#include "stdafx.h"
#include <d3d9.h>
#include "FixSettings.hpp"

#ifndef DIRECTINPUT_VERSION
#define DIRECTINPUT_VERSION 0x0800
#endif
#include <dinput.h>

export module Gamepad;

import ComVars;


export namespace Driv3rPad
{
    SafetyHookMid initHook, vibrationHook;
    std::atomic<bool> dirty{ true }, stopRequested{ true };
    struct ControllerState
    {
        IDirectInputDevice8A* device = nullptr;
        DWORD appliedDeadzone = MAXDWORD;
        IDirectInputEffect* effect = nullptr;
        bool pendingStop = false;
    };
    std::array<ControllerState, 8> controllers{};
    bool wasBlocked = false;

    DWORD Deadzone()
    {
        const float value = Driv3rSettings::GamepadDeadzone.load();
        return static_cast<DWORD>(std::lround((std::isfinite(value) ? std::clamp(value, 0.0f, 0.5f) : 0.15f) * 10000));
    }
    bool BlockVibration()
    {
        return !Driv3rSettings::Vibration.load() || BlockGameInput;
    }
    void UpdateEffect(ControllerState& controller, IDirectInputEffect* effect, bool request, bool blocked)
    {
        if (controller.effect != effect)
        {
            controller.effect = effect;
            controller.pendingStop = blocked;
        }
        controller.pendingStop |= request;
        if (effect && controller.pendingStop && SUCCEEDED(effect->Stop()))
            controller.pendingStop = false;
    }

    void StopEffects()
    {
        const uintptr_t manager = InputManager;
        if (!manager || !*reinterpret_cast<const uint8_t*>(manager + 4))
        {
            stopRequested.store(true);
            return;
        }
        auto* joystick = reinterpret_cast<uint8_t*>(manager + 588);
        const bool blocked = BlockVibration();
        for (size_t index = 0; index < controllers.size(); ++index)
        {
            auto* effect = *reinterpret_cast<IDirectInputEffect**>(joystick + index * 144 + 4);
            UpdateEffect(controllers[index], effect, true, blocked);
        }
    }

    void Frame()
    {
        const uintptr_t manager = InputManager;
        if (!manager || !*reinterpret_cast<const uint8_t*>(manager + 4))
            return;
        auto* joystick = reinterpret_cast<uint8_t*>(manager + 588);
        const bool update = dirty.exchange(false), blocked = BlockVibration();
        const bool stop = stopRequested.exchange(false) || (blocked && !wasBlocked);
        const DWORD deadzone = Deadzone();
        *reinterpret_cast<DWORD*>(joystick + 1380) = deadzone;
        for (size_t index = 0; index < controllers.size(); ++index)
        {
            auto& controller = controllers[index];
            auto* device = *reinterpret_cast<IDirectInputDevice8A**>(joystick + index * 144);
            auto* effect = *reinterpret_cast<IDirectInputEffect**>(joystick + index * 144 + 4);
            if (update || controller.device != device)
            {
                controller.device = device;
                controller.appliedDeadzone = MAXDWORD;
            }
            if (device && controller.appliedDeadzone != deadzone)
            {
                DIPROPDWORD property{};
                property.diph.dwSize = sizeof(property);
                property.diph.dwHeaderSize = sizeof(property.diph);
                property.diph.dwHow = DIPH_DEVICE;
                property.dwData = deadzone;
                if (SUCCEEDED(device->SetProperty(DIPROP_DEADZONE, &property.diph)))
                    controller.appliedDeadzone = deadzone;
            }
            UpdateEffect(controller, effect, blocked && stop, blocked);
        }
        wasBlocked = blocked;
    }

    void Install()
    {
        auto init = hook::pattern("C7 86 64 05 00 00 C4 09 00 00 8B 0F 57 FF 51 10");
        auto vibration = hook::pattern("83 EC 48 8D 04 C0 C1 E0 04 53 33 DB 39 5C 08 04 8D 14 08");
        initHook = safetyhook::create_mid(init.get_first(10), [](SafetyHookContext& regs)
        {
            *reinterpret_cast<DWORD*>(regs.esi + 1380) = Deadzone();
            dirty.store(true);
            stopRequested.store(true);
        });
        vibrationHook = safetyhook::create_mid(vibration.get_first(), [](SafetyHookContext& regs)
        {
            if (BlockVibration())
            {
                *reinterpret_cast<float*>(regs.esp + 4) = 0.0f;
                *reinterpret_cast<float*>(regs.esp + 8) = 0.0f;
            }
        });
        FixSettings().OnChange("MAIN.GamepadDeadzone", []() { dirty.store(true); });
        FixSettings().OnChange("MAIN.Vibration", []() { stopRequested.store(true); });
    }
}
