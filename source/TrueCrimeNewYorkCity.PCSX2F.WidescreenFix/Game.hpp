#pragma once
#include "../../external/injector/include/ps2/runtime.hpp"
#include "../../external/injector/include/ps2/game_abi.hpp"
#include "../Shared/Console/PS2Display.hpp"
#include <cstdint>

extern "C" {
#include "../../external/injector/include/ps2/inireader.h"
extern int PCSX2Data[PCSX2Data_Size];
extern char FrameLimitUnthrottle;
}
namespace tcny {
struct Display {
    float aspect = 4.0f / 3.0f, width = 640.0f;
    float scale = 1.0f, centered = 0, normalized = 0;
    float right = 0, phone = 0, radar = -5.0f;
    float constraint = -1;
    void refresh() {
        aspect = console::ps2Aspect(PCSX2Data);
        scale = (4.0f / 3.0f) / aspect;
        centered = width * (1 - scale) * 0.5f;
        normalized = (aspect / (4.0f / 3.0f) - 1) * 0.5f;
        float constrained = constraint > 0 ? console::bounded(constraint, 4.0f / 3.0f, aspect, aspect) : aspect;
        right = (constrained / (4.0f / 3.0f) - 1) * 0.5f;
        phone = normalized;
        radar = -5.0f + centered - width * (1 - (4.0f / 3.0f) / constrained) * 0.5f;
    }
};
inline Display display;
inline bool enable60 = false, unthrottle = true;
inline int* loading = nullptr;
inline uintptr_t mapCallers[2];
void InstallHud(uintptr_t width, uintptr_t layout, uintptr_t subtitles, uintptr_t models,
                uintptr_t map, uintptr_t blips, uintptr_t radar, uintptr_t radarDisc, uintptr_t scale3D);
void InstallTiming(uintptr_t process, uintptr_t frame, uintptr_t vsyncCall);
}
