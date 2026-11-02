#pragma once
#include "../Shared/Console/PatchNotice.hpp"

namespace mc3 {
using namespace console::portable;

template<class T> inline T& at(uintptr_t address) { return *reinterpret_cast<T*>(address); }
inline constexpr float nativeAspect = 480.0f / 272.0f;
inline constexpr float screenWidth = 480.0f, screenHeight = 272.0f;
inline float targetAspect = nativeAspect;
inline bool automaticAspect = true;
inline float hudScale = 1.0f;
inline float fovFactor = 1.0f;
inline float hudAspect = 0.0f;
inline bool skipIntro = true, dualAnalog = true;
// MAIN.CutsceneBorders: bit 0 letterbox on narrow screens, bit 1 pillarbox on wide screens.
inline int cutsceneBorders = 3;
// Vertical blank count of the last full-screen 3D camera; screens without a
// 3D scene leave stale pixels beside centered 2D layers.
inline unsigned sceneVcount;
// Race director: +4 is the race phase (2 = opening fly-by camera).
inline uintptr_t raceDirector;
inline bool Cutscene()
{
    const auto director = raceDirector ? at<uintptr_t>(raceDirector) : 0;
    return director && at<int>(director + 4) == 2;
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
    static ScreenTransform around(float anchorX, float anchorY, float scale, bool hud = false) {
        const float k = Widening();
        ScreenTransform t;
        t.ax = scale * (k > 1.0f ? 1.0f / k : 1.0f);
        t.ay = scale * (k < 1.0f ? k : 1.0f);
        t.bx = anchorX * (1.0f - t.ax);
        if (hud && (anchorX == 0.0f || anchorX == screenWidth))
            t.bx += console::hud_edge(anchorX, screenWidth, targetAspect, hudAspect) - anchorX;
        t.by = anchorY * (1.0f - t.ay);
        t.active = t.ax != 1.0f || t.ay != 1.0f || t.bx != 0.0f || t.by != 0.0f;
        return t;
    }
};
inline ScreenTransform screen;

void InstallAspect();
inline constexpr unsigned maxHudElementHooks = 40;
void InstallHud();
void InstallCamera();
void InstallIntro();
void InstallNativeMenu();
void ReadMenuOptions();
void MenuFrame();
void RefreshDisplayOptions();
}
