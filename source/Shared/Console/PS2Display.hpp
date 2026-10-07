#pragma once
#include "Viewport.hpp"
extern "C" {
#include "../../../external/injector/include/ps2/pcsx2f_api.h"
}
namespace console {
inline float ps2Aspect(const int* data) {
    switch (data[PCSX2Data_AspectRatioSetting]) {
    case RAuto4_3_3_2: case R4_3: return 4.0f / 3.0f;
    case R16_9: return 16.0f / 9.0f;
    default: break;
    }
    int width = data[PCSX2Data_WindowSizeX], height = data[PCSX2Data_WindowSizeY];
    if (data[PCSX2Data_IsFullscreen] || width <= 0 || height <= 0) {
        width = data[PCSX2Data_DesktopSizeX]; height = data[PCSX2Data_DesktopSizeY];
    }
    return width > 0 && height > 0 ? bounded(float(width) / float(height), 0.5f, 8.0f, 4.0f / 3.0f) : 4.0f / 3.0f;
}
}
