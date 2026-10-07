#pragma once
#include "../Shared/Console/PSP.hpp"

namespace mc3 {
using namespace console::portable;

inline constexpr float nativeAspect = 480.0f / 272.0f;
inline constexpr float screenWidth = 480.0f, screenHeight = 272.0f;
inline float targetAspect = nativeAspect;
inline bool automaticAspect = true;
inline float hudScale = 1.0f;

template<class T> inline T& at(uintptr_t address) { return *reinterpret_cast<T*>(address); }
inline bool Guest(uintptr_t address, unsigned size = 4) {
    return address >= 0x08800000 && address <= 0x0A000000 - size;
}
inline void RefreshAspect() { if (automaticAspect) targetAspect = Aspect(); }
// Displayed width of one framebuffer pixel relative to its native width.
inline float Widening() { return targetAspect / nativeAspect; }

// Affine screen-space transform applied to 2D output (framebuffer pixels).
struct ScreenTransform {
    float ax = 1.0f, bx = 0.0f, ay = 1.0f, by = 0.0f;
    bool active = false;
    float x(float value) const { return value * ax + bx; }
    float y(float value) const { return value * ay + by; }
    // Keep proportions while scaling around a native screen anchor point.
    static ScreenTransform around(float anchorX, float anchorY, float scale) {
        const float k = Widening();
        ScreenTransform t;
        t.ax = scale * (k > 1.0f ? 1.0f / k : 1.0f);
        t.ay = scale * (k < 1.0f ? k : 1.0f);
        t.bx = anchorX * (1.0f - t.ax);
        t.by = anchorY * (1.0f - t.ay);
        t.active = t.ax != 1.0f || t.ay != 1.0f;
        return t;
    }
};
inline ScreenTransform screen;

void InstallAspect();
void InstallHud();
void InstallCamera();
}
