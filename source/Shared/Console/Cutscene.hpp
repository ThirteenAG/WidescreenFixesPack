#pragma once
#include "Viewport.hpp"
#include <cstdint>

namespace console {
// Fit the original visible cinematic window. Projection and border visibility
// are independent: hiding the mask exposes scenery instead of changing zoom.
struct CutsceneFrame {
    float aspect = 16.0f / 9.0f;
    Rect window(float viewportAspect, float width, float height) const {
        const float sx = viewportAspect > aspect ? aspect / viewportAspect : 1.0f;
        const float sy = viewportAspect < aspect ? viewportAspect / aspect : 1.0f;
        return {width * (1 - sx) * .5f, height * (1 + sy) * .5f,
            width * (1 + sx) * .5f, height * (1 - sy) * .5f};
    }
    float fovAspect(float viewportAspect) const {
        // Keep horizontal coverage on narrower screens and vertical coverage on
        // wider screens. The projection itself still uses the display aspect.
        return viewportAspect > aspect ? viewportAspect : aspect;
    }
};
struct CutsceneBorderAnimation {
    uint32_t lastMs = 0;
    bool initialized = false;
    float amount = 0;
    float tick(uint32_t now, bool active, bool enabled, bool dark) {
        if (!initialized) { lastMs = now; initialized = true; }
        const uint32_t elapsed = now - lastMs;
        lastMs = now;
        if (!enabled || dark) { amount = 0; return amount; }
        // Pause and loading jumps cannot consume the whole transition at once.
        const float step = float(elapsed < 100 ? elapsed : 100) / 500.0f;
        amount = bounded(amount + (active ? step : -step), 0.0f, 1.0f, 0.0f);
        return amount;
    }
    // MAIN.CutsceneBorders: 0 off, 1 letterbox (top/bottom), 2 pillarbox
    // (left/right), 3 both. The bits select which sides of the window are masked.
    template<class Draw> void draw(const Rect& target, float width, float height, Draw emit, int sides = 3) const {
        if (amount <= 0) return;
        const float pillar = (sides & 2) ? amount : 0.0f, letter = (sides & 1) ? amount : 0.0f;
        const float left = target.left * pillar, top = target.top * letter;
        const float right = (width - target.right) * pillar;
        const float bottom = (height - target.bottom) * letter;
        if (left > 0) emit(Rect{0, height, left, 0});
        if (right > 0) emit(Rect{width-right, height, width, 0});
        if (top > 0) emit(Rect{0, top, width, 0});
        if (bottom > 0) emit(Rect{0, height, width, height-bottom});
    }
};
}
