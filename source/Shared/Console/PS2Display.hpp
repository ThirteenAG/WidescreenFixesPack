#pragma once
#include "Viewport.hpp"
extern "C" {
#include "../../../external/injector/include/ps2/pcsx2f_api.h"
}
// Opt in to receiving the effective display aspect alongside the raw mode.
#define PCSX2F_EFFECTIVE_ASPECT() extern "C" { uint32_t PCSX2FEffectiveAspectVersion = 1; }

namespace console {
// Fixed 10:7 is the next host mode after 16:9 in the display ABI.
inline constexpr int PS2Aspect10By7 = R16_9 + 1;
inline float ps2Aspect(const volatile int* data, float autoFallback = 4.0f / 3.0f) {
    constexpr float nativeAspect = 4.0f / 3.0f, widescreenAspect = 16.0f / 9.0f;
    const auto display = static_cast<uint32_t>(data[PCSX2Data_AspectRatioSetting]);
    const auto mode = display & 0xffu;
    switch (mode) {
    case Stretch:
    case RAuto4_3_3_2: {
        const uint32_t encoded = display >> 8;
        // Keep the nominal ratios exact for native layout thresholds.
        if (encoded == uint32_t(nativeAspect * 65536.0f + 0.5f)) return nativeAspect;
        if (encoded == uint32_t(widescreenAspect * 65536.0f + 0.5f)) return widescreenAspect;
        const float effective = float(encoded) / 65536.0f;
        if (effective >= 0.5f && effective <= 8.0f) return effective;
        if (mode == RAuto4_3_3_2) return autoFallback;
        break;
    }
    case R4_3: return nativeAspect;
    case R16_9: return widescreenAspect;
    case PS2Aspect10By7: return 10.0f / 7.0f;
    default: break;
    }
    int width = data[PCSX2Data_WindowSizeX], height = data[PCSX2Data_WindowSizeY];
    if (width <= 0 || height <= 0) {
        width = data[PCSX2Data_DesktopSizeX]; height = data[PCSX2Data_DesktopSizeY];
    }
    return width > 0 && height > 0 ? bounded(float(width) / float(height), 0.5f, 8.0f, 4.0f / 3.0f) : 4.0f / 3.0f;
}
}
