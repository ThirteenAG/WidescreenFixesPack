#pragma once
#include "Game.hpp"
#include <cmath>
// Full-screen pictures that bypass the UI canvas (loader and loading screens,
// FMVs) are contained at the native 480x272 aspect with black side bars.
namespace essentials {
inline bool containImages = true; // [MAIN] ContainFullscreenImages
// Width scale that keeps native proportions on the physical display (1 = none).
inline float Contain() {
    if (!containImages) return 1;
    const float scale = (480.0f / 272.0f) / Aspect();
    return scale < 0.999f ? scale : 1.0f;
}
inline int ContainX(float x, float scale) { return int(std::lround(240 + (x - 240) * scale)); }
inline uint16_t Mix565(uint16_t a, uint16_t b, unsigned weight) { // weight 0..256 towards b
    const unsigned r = ((a >> 11) * (256 - weight) + (b >> 11) * weight) >> 8;
    const unsigned g = (((a >> 5) & 63) * (256 - weight) + ((b >> 5) & 63) * weight) >> 8;
    const unsigned l = ((a & 31) * (256 - weight) + (b & 31) * weight) >> 8;
    return uint16_t(r << 11 | g << 5 | l);
}
// Resample a 512x272 RGB565 picture in place (480 visible columns).
inline void ContainPicture(uint16_t* pixels) {
    const float scale = Contain();
    if (scale >= 1 || !pixels) return;
    const float left = 240 - 240 * scale;
    const int first = int(std::ceil(left - 0.5f)), last = 480 - first; // [first, last)
    static uint16_t source[480], at[480], weight[480];
    for (int x = first; x < last; ++x) {
        const float position = (x + 0.5f - left) / scale - 0.5f;
        const int index = position <= 0 ? 0 : (position >= 479 ? 479 : int(position));
        const float fraction = position - index;
        at[x] = uint16_t(index);
        weight[x] = uint16_t(fraction <= 0 ? 0 : (fraction >= 1 ? 256 : unsigned(fraction * 256)));
    }
    for (int y = 0; y < 272; ++y) {
        auto line = pixels + y * 512;
        std::memcpy(source, line, sizeof(source));
        for (int x = 0; x < first; ++x) line[x] = 0;
        for (int x = first; x < last; ++x) {
            const int index = at[x];
            line[x] = Mix565(source[index], source[index < 479 ? index + 1 : index], weight[x]);
        }
        for (int x = last; x < 480; ++x) line[x] = 0;
    }
}
}
