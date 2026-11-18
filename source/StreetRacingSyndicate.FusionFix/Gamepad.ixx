module;

#include <stdafx.h>
#ifndef DIRECTINPUT_VERSION
#define DIRECTINPUT_VERSION 0x0800
#endif
#include <dinput.h>
#include <optional>

// Out-of-the-box controller support. Player 1 starts on the keyboard, and the game reads a pad only after it's
// chosen under Options > Controls. Every action has a second binding that the controls screen doesn't show; it's
// filled with the bundled Xidi controller, so keyboard and pad both work without setup. Choosing the pad in the menu
// still gives the game's own pad layout.

export module Gamepad;

export namespace SRSPad
{
    // Binding record: device GUID, input code, analog flag, enabled flag, current and previous value
    struct Binding
    {
        GUID device;
        int32_t code;
        uint8_t analog;
        uint8_t enabled;
        uint8_t pad[2];
        float value;
        float previous;
    };
    static_assert(sizeof(Binding) == 32);

    constexpr size_t BindingsOffset = 916;
    constexpr size_t DeviceOffset = 5652;
    constexpr int32_t KeyboardDevice = 0;

    // Codes 0-31 are buttons; from 32 on each axis has a positive and a negative half
    enum Code : int32_t
    {
        PadA = 0, PadB = 1, PadX = 2, PadY = 3, PadLB = 4, PadRB = 5, PadBack = 6, PadLS = 8, PadRS = 9,
        StickRight = 32, StickLeft = 33, StickDown = 34, StickUp = 35, TriggerLeft = 36, TriggerRight = 42,
    };

    enum Action : int32_t
    {
        ChangeView = 1, RearView = 4, SteerLeft = 7, SteerRight = 8, Accelerate = 9, HandBrake = 11, Brake = 13,
        GearUp = 15, GearDown = 16, Nos = 17, Clutch = 18, HighBeam = 19,
        MenuUp = 22, MenuDown = 23, MenuLeft = 24, MenuRight = 25, MenuAccept = 26, MenuOption = 27, MenuBack = 28,
        MenuExtra = 29, ViewMap = 73,
    };

    struct DefaultBinding
    {
        Action action;
        Code code;
    };

    // The layout of the bundled Xidi.ini: triggers on their own half axes, d-pad on the left stick's axes
    constexpr DefaultBinding Defaults[] =
    {
        { Accelerate, TriggerRight }, { Brake, TriggerLeft }, { SteerLeft, StickLeft }, { SteerRight, StickRight },
        { HandBrake, PadX }, { Nos, PadA }, { GearUp, PadRB }, { GearDown, PadLB }, { Clutch, PadLS },
        { ChangeView, PadY }, { RearView, PadB }, { HighBeam, PadRS }, { ViewMap, PadBack },
        { MenuUp, StickUp }, { MenuDown, StickDown }, { MenuLeft, StickLeft }, { MenuRight, StickRight },
        { MenuAccept, PadA }, { MenuBack, PadB }, { MenuOption, PadX }, { MenuExtra, PadY },
    };

    void* (*GetInputAdapter)() = nullptr;
    void* PlayerOne = nullptr;
    SafetyHookInline ControllerHook, KeyboardDefaultsHook;

    // The first Xidi controller among the game's joysticks (devices 2 and up)
    std::optional<GUID> XidiController()
    {
        auto* adapter = static_cast<uint8_t*>(GetInputAdapter());
        if (!adapter)
            return std::nullopt;
        const int32_t devices = *reinterpret_cast<int32_t*>(adapter + 124);
        for (int32_t i = 0; i < devices - 2; ++i)
        {
            auto* device = *reinterpret_cast<IDirectInputDevice8A**>(adapter + 48 + 20 * i);
            if (!device)
                continue;
            DIDEVICEINSTANCEA info{};
            info.dwSize = sizeof(info);
            if (FAILED(device->GetDeviceInfo(&info)))
                continue;
            if (std::string_view(info.tszProductName).find("Xidi") != std::string_view::npos)
                return info.guidInstance;
        }
        return std::nullopt;
    }

    void AddPadBindings(void* controller)
    {
        auto* base = static_cast<uint8_t*>(controller);
        if (controller != PlayerOne || *reinterpret_cast<int32_t*>(base + DeviceOffset) != KeyboardDevice)
            return;
        const auto pad = XidiController();
        if (!pad)
            return;
        auto* bindings = reinterpret_cast<Binding*>(base + BindingsOffset);
        for (const auto& entry : Defaults)
        {
            auto& second = bindings[entry.action * 2 + 1];
            if (second.enabled)
                continue;
            second = {};
            second.device = *pad;
            second.code = entry.code;
            second.analog = entry.code >= StickRight;
            second.enabled = 1;
        }
    }

    void* __fastcall Controller(void* self, void*)
    {
        auto result = ControllerHook.unsafe_thiscall<void*>(self);
        // The first controller the game creates is player 1's
        if (!PlayerOne)
            PlayerOne = self;
        AddPadBindings(self);
        return result;
    }

    void __fastcall KeyboardDefaults(void* self, void*)
    {
        KeyboardDefaultsHook.unsafe_thiscall<void>(self);
        AddPadBindings(self);
    }

    void Init()
    {
        auto adapter = hook::pattern("E8 ? ? ? ? A3 ? ? ? ? A1 ? ? ? ? 39 68 7C");
        GetInputAdapter = reinterpret_cast<void* (*)()>(injector::GetBranchDestination(adapter.get_first()).as_int());
        auto controller = hook::pattern("53 55 56 57 8B F1 E8 ? ? ? ? 8D 7E 1C 8B CF C7 06 ? ? ? ? E8 ? ? ? ? C7 07 ? ? ? ? 8D 86 A8 03 00 00 B9");
        ControllerHook = safetyhook::create_inline(controller.get_first(), Controller);
        auto keyboardDefaults = hook::pattern("83 EC 10 53 55 56 57 8B D9 E8 ? ? ? ? 8B 03 8B CB FF 50 28 A1");
        KeyboardDefaultsHook = safetyhook::create_inline(keyboardDefaults.get_first(), KeyboardDefaults);
    }
}
