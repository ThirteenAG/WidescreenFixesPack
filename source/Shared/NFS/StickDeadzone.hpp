#pragma once
#include <algorithm>
#include <cmath>

// Gamepad stick dead zones of the Need for Speed fixes, set in the options menu in percent of the stick's travel
// (10 by default, as in the older versions of the fixes)
namespace WFPStickDeadzone
{
    // A stick of two DirectInput axes from 0 to 65535: a round dead zone around the centre, and the rest of the
    // travel stretched to the full range, so the stick doesn't jump at the edge of the dead zone
    inline void Apply(LONG& x, LONG& y, float percent)
    {
        float deadzone = std::clamp(percent / 100.0f, 0.0f, 0.9f);
        if (deadzone <= 0.0f)
            return;

        float fx = (float(x) - 32767.5f) / 32767.5f;
        float fy = (float(y) - 32767.5f) / 32767.5f;
        float length = std::sqrt(fx * fx + fy * fy);
        float scale = length <= deadzone ? 0.0f : std::min(1.0f, (length - deadzone) / (1.0f - deadzone)) / length;
        x = std::clamp(LONG(std::lround(fx * scale * 32767.5f + 32767.5f)), 0L, 65535L);
        y = std::clamp(LONG(std::lround(fy * scale * 32767.5f + 32767.5f)), 0L, 65535L);
    }

    // The dead zone in DirectInput's DIPROP_DEADZONE units (0 to 10000)
    inline DWORD Property(float percent)
    {
        return DWORD(std::clamp(percent, 0.0f, 90.0f) * 100.0f);
    }
}
