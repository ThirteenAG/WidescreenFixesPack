module;

#include <stdafx.h>
#include "rawinput.hpp"
#include "FixSettings.hpp"
#include "Shared/DirectInputRawMouse.hpp"
#include <algorithm>
#include <dinput.h>

export module Misc;

import ComVars;
import Frontend;
import Camera;
import Hud;
import Intro;
import Window;
import Movie;

namespace DeerControls
{
    struct JoystickState
    {
        uint8_t reserved[2];
        uint8_t buttons[8];
        int16_t x, y;
    };

    int (__cdecl* readDevices)(JoystickState*, void*) = nullptr;
    void** joystickDevice = nullptr;
    int* useJoystick = nullptr;
    bool padActive = false;
    bool rawMouse = false;
    bool mouseLost = true;
    SafetyHookInline registrationHook;

    BOOL NativeRegister(PCRAWINPUTDEVICE devices, UINT count, UINT size)
    {
        return registrationHook.stdcall<BOOL>(devices, count, size);
    }

    BOOL WINAPI RegisterDevices(PCRAWINPUTDEVICE devices, UINT count, UINT size)
    {
        return DirectInputRawMouse::RegisterDevices(devices, count, size, NativeRegister);
    }

    // A windowed DirectInput mouse only gets accelerated cursor movement, so its state comes from raw input instead.
    int ReadMouse(JoystickState* joystick, DIMOUSESTATE* mouse)
    {
        const auto result = readDevices(joystick, mouse);
        if (!rawMouse)
            return result;
        if (!DirectInputRawMouse::Active() && DirectInputRawMouse::detail::window.load())
        {
            const RAWINPUTDEVICE device{ 0x01, 0x02, RIDEV_INPUTSINK, nullptr };
            DirectInputRawMouse::RegisterDevices(&device, 1, sizeof(device), NativeRegister);
        }
        if (result != DI_OK || !DirectInputRawMouse::Active())
        {
            mouseLost = true;
            DirectInputRawMouse::Clear();
            return result;
        }
        if (std::exchange(mouseLost, false))
            DirectInputRawMouse::Clear();
        const auto raw = DirectInputRawMouse::Take();
        mouse->lX = raw.x;
        mouse->lY = raw.y;
        mouse->lZ = raw.wheel;
        for (size_t button = 0; button < 4; ++button)
            mouse->rgbButtons[button] = ((raw.down | raw.pressed) & (1u << button)) ? 0x80 : 0;
        return result;
    }

    int __cdecl ReadDevices(JoystickState* joystick, void* mouse)
    {
        const auto state = static_cast<DIMOUSESTATE*>(mouse);
        if (!*joystickDevice)
        {
            if (std::exchange(padActive, false))
                *useJoystick = 0;
            return ReadMouse(joystick, state);
        }

        *joystick = {};
        const auto result = ReadMouse(joystick, state);
        constexpr int deadzone = 100; // The game configures each axis to -1000 through 1000.
        const bool active = std::abs(joystick->x) > deadzone || std::abs(joystick->y) > deadzone ||
            std::ranges::any_of(joystick->buttons, [](uint8_t button) { return button != 0; });
        // Run one neutral sample through the native joystick branch so it can release its buttons.
        const bool wasActive = std::exchange(padActive, active);
        *useJoystick = active || wasActive;
        return result;
    }

    void Install()
    {
        auto dispatch = hook::pattern("83 EC 40 8D 44 24 04 8D 4C 24 14 53 50 51 E8 ? ? ? ? A1 ? ? ? ? 33 DB 83 C4 08 3B C3");
        const auto call = dispatch.get_first(14);
        readDevices = injector::GetBranchDestination(call).get();
        auto devices = hook::pattern("A1 ? ? ? ? 83 EC 50 85 C0 56 0F 84 ? ? ? ? 8B 08 50 FF 51 64");
        joystickDevice = *devices.get_first<void**>(1);
        useJoystick = *dispatch.get_first<int*>(20);
        injector::MakeCALL(call, ReadDevices, true);
        rawMouse = DeerWindow::mode != 0;
        if (rawMouse)
        {
            DirectInputRawMouse::Start();
            registrationHook = safetyhook::create_inline(GetProcAddress(GetModuleHandleW(L"user32.dll"), "RegisterRawInputDevices"), RegisterDevices);
        }
    }
}

void Install()
{
    MessagePumpGuard::Install(GetModuleHandleW(nullptr));
    WindowedModeWrapper::EnableDpiAwareness();
    auto& settings = FixSettings();
    asi_settings::LiveLink::Start();
    (void)DeerMenu::Game();
    DeerMovie::Init();
    DeerWindow::Init(settings.Int("MAIN.WindowMode"));
    DeerControls::Install();
    DeerIntro::Init(settings.Bool("MAIN.SkipIntro"));
    DeerHud::Init();
    DeerCamera::Init();
}

export void InitWithMenu()
{
    WFP::onInitEvent().executeAll();
    HMODULE owner = nullptr;
    if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCWSTR>(&Install), &owner))
        DeerMenu::Init(owner);
}

class Misc
{
public:
    Misc()
    {
        WFP::onInitEvent() += []() { Install(); };
    }
} Misc;

