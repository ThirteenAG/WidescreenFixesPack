module;

#include "stdafx.h"
#include "Shared/PadInput.hpp"

export module Controller;

import Build;
import Settings;

export bool ReadActiveController(XINPUT_STATE& state)
{
    const auto slot = *reinterpret_cast<const int*>(ControllerSlotAddress());
    if (slot < 0 || slot >= 4)
        return false;
    const auto device = reinterpret_cast<void* const*>(ControllerDevicesAddress())[slot];
    if (!device)
        return false;
    const auto methods = *reinterpret_cast<uintptr_t* const*>(device);
    using DeviceType = unsigned(*)(void*);
    if (reinterpret_cast<DeviceType>(methods[6])(device) != 1)
        return false;
    const auto getState = PadInput::XInput().getState;
    return getState && getState(slot, &state) == ERROR_SUCCESS;
}

// The four native HID slots and their current slot are verified per build.
// Callers pass the native pad result so its menu, task and control gates win.
export std::optional<float> OriginalControllerOrbit(float native, bool horizontal, float fov)
{
    if (!Settings.restoreOriginalCamera || !std::isfinite(native))
        return std::nullopt;
    XINPUT_STATE state{};
    if (!ReadActiveController(state))
        return std::nullopt;
    if (native == 0.0f)
        return 0.0f;
    const int axis = horizontal ? state.Gamepad.sThumbRX : state.Gamepad.sThumbRY;
    // Use the native sign so the game's configured inversion is preserved.
    const int stick = std::abs(axis) * 128 / (axis < 0 ? 32768 : 32767);
    const auto adjusted = stick > 35 ? static_cast<int>((stick - 35) * 1.3763441f) : 0;
    const auto gain = horizontal ? 0.071428575f : 0.042857144f;
    const auto speed = adjusted * adjusted * (fov * 0.0125f) * gain * 0.007f * 0.007f;
    return std::copysign(speed, native);
}
