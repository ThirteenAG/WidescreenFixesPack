module;

#include "stdafx.h"

export module Controller;

import Build;
import Settings;

// The four native HID slots and their current slot are verified per build.
// Callers pass the native pad result so its menu, task and control gates win.
export std::optional<float> OriginalControllerOrbit(float native, bool horizontal, float fov)
{
    if (!Settings.restoreOriginalCamera || !std::isfinite(native))
        return std::nullopt;
    const auto slot = *reinterpret_cast<const int*>(ControllerSlotAddress());
    if (slot < 0 || slot >= 4)
        return std::nullopt;
    const auto device = reinterpret_cast<void* const*>(ControllerDevicesAddress())[slot];
    if (!device)
        return std::nullopt;
    const auto methods = *reinterpret_cast<uintptr_t* const*>(device);
    using DeviceType = unsigned(*)(void*);
    if (reinterpret_cast<DeviceType>(methods[6])(device) != 1)
        return std::nullopt;
    if (native == 0.0f)
        return 0.0f;
    using ReadAxis = bool(*)(void*, uintptr_t, float*);
    float axis = 0.0f;
    reinterpret_cast<ReadAxis>(methods[4])(device, horizontal ? 28 : 29, &axis);
    if (!std::isfinite(axis) || axis == 0.0f)
        return std::nullopt;
    // SA's integer stick range, 35-unit dead zone and quadratic response.
    // Use the native result's sign to retain the configured inversion.
    const auto stick = static_cast<int>(std::clamp(fabsf(axis), 0.0f, 1.0f) * 128.0f);
    const auto adjusted = stick > 35 ? static_cast<int>((stick - 35) * 1.3763441f) : 0;
    const auto gain = horizontal ? 0.071428575f : 0.042857144f;
    const auto speed = adjusted * adjusted * (fov * 0.0125f) * gain * 0.007f * 0.007f;
    return std::copysign(speed, native);
}
