#pragma once
#include <algorithm>

namespace scda {
struct CinemaLayout { float zoom, left, right, top, bottom, framePad; bool nativeBars; };
struct CinemaBars {
    float top = 0, bottom = 0, visible = 1;
    bool seenTop = false, seenBottom = false;
    void begin() { seenTop = seenBottom = false; }
    bool observe(bool isTop, float extent, float height) {
        if (isTop) { seenTop = true; top = std::clamp(extent, 0.0f, height); }
        else { seenBottom = true; bottom = std::clamp(extent, 0.0f, height); }
        const bool paired = top > 0 && bottom > 0;
        visible = paired ? std::clamp(1.0f - (top + bottom) / height, 0.01f, 1.0f) : 1.0f;
        return paired;
    }
    void finish() { if (!seenTop || !seenBottom) *this = {}; }
};
// The original full bars leave 254 of 448 lines visible on both regions.
inline CinemaLayout CinemaFit(float aspect, float visible, int mode) {
    constexpr float nativeAspect = 4.0f / 3.0f, nativeVisible = 254.0f / 448.0f;
    visible = std::clamp(visible, 0.01f, 1.0f);
    const float progress = std::clamp((1.0f - visible) / (1.0f - nativeVisible), 0.0f, 1.0f);
    if (mode == 0) return {1, 0, 1, 0, 1, 0, false};
    if (mode == 1) {
        const float height = std::min(1.0f, aspect / nativeAspect);
        const float zoom = 1 + (height - 1) * progress;
        const float padding = (1 - height) * .5f * progress;
        return {zoom, 0, 1, (1 - zoom * visible) * .5f,
                (1 + zoom * visible) * .5f, padding, true};
    }
    if (mode == 3) {
        const float width = std::min(1.0f, nativeAspect / aspect);
        const float height = std::min(1.0f, aspect / nativeAspect);
        const float padding = (1 - height) * .5f * progress;
        const float side = (1 - width) * .5f * progress;
        const float zoom = 1 + (height - 1) * progress;
        return {zoom, side, 1 - side, (1 - zoom * visible) * .5f,
                (1 + zoom * visible) * .5f, padding, true};
    }
    const float zoom = std::min(1.0f / nativeVisible, aspect / nativeAspect);
    const float width = std::min(1.0f, nativeAspect / aspect * zoom);
    const float height = std::min(1.0f, nativeVisible * zoom);
    const float side = (1 - width) * .5f * progress, padding = (1 - height) * .5f * progress;
    return {1 + (zoom - 1) * progress, side, 1 - side, padding, 1 - padding, 0, false};
}
}
