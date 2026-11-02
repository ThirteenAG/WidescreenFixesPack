#pragma once
#include "Game.hpp"
#include <cmath>
// Full-screen pictures that bypass the UI canvas (loader and loading screens,
// FMVs) are contained at the native 480x272 aspect with black bars.
namespace essentials {
inline float DisplayAspect() { return forced > 0 ? forced : Aspect(); }
// Scales that keep native proportions on the physical display (1 = none).
inline float ContainScaleX() {
    const float scale = nativeAspect / DisplayAspect();
    return scale < 0.999f ? scale : 1.0f;
}
inline float ContainScaleY() {
    const float scale = DisplayAspect() / nativeAspect;
    return scale < 0.999f ? scale : 1.0f;
}
inline int ContainX(float x, float scale) { return int(std::lround(240 + (x - 240) * scale)); }
inline int ContainY(float y, float scale) { return int(std::lround(136 + (y - 136) * scale)); }
inline uint16_t Mix565(uint16_t a, uint16_t b, unsigned weight) { // weight 0..256 towards b
    const unsigned r = ((a >> 11) * (256 - weight) + (b >> 11) * weight) >> 8;
    const unsigned g = (((a >> 5) & 63) * (256 - weight) + ((b >> 5) & 63) * weight) >> 8;
    const unsigned l = ((a & 31) * (256 - weight) + (b & 31) * weight) >> 8;
    return uint16_t(r << 11 | g << 5 | l);
}
// Source index and blend weight (0..256) for destination position i of a
// line of count samples contained at scale around its center.
inline void ContainSample(int i, int count, float scale, int& index, unsigned& weight) {
    const float start = count * 0.5f * (1 - scale);
    const float position = (i + 0.5f - start) / scale - 0.5f;
    index = position <= 0 ? 0 : (position >= count - 1 ? count - 1 : int(position));
    const float fraction = position - index;
    weight = fraction <= 0 ? 0 : (fraction >= 1 ? 256 : unsigned(fraction * 256));
}
// Resample a 512x272 RGB565 picture in place (480 visible columns).
inline void ContainPicture(uint16_t* pixels) {
    const float sx = ContainScaleX(), sy = ContainScaleY();
    if (!pixels || (sx >= 1 && sy >= 1)) return;
    if (sx < 1) {
        const int first = int(std::ceil(240 * (1 - sx) - 0.5f)), last = 480 - first; // [first, last)
        static uint16_t source[480], at[480], weight[480];
        for (int x = first; x < last; ++x) {
            int index; unsigned blend;
            ContainSample(x, 480, sx, index, blend);
            at[x] = uint16_t(index); weight[x] = uint16_t(blend);
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
    if (sy < 1) {
        const int first = int(std::ceil(136 * (1 - sy) - 0.5f)), last = 272 - first; // [first, last)
        // Rows above the center read rows further up, rows below read further
        // down, so each half is resampled from the center outwards in place.
        const auto row = [&](int y) {
            int index; unsigned blend;
            ContainSample(y, 272, sy, index, blend);
            auto line = pixels + y * 512;
            const auto a = pixels + index * 512, b = pixels + (index < 271 ? index + 1 : index) * 512;
            for (int x = 0; x < 480; ++x) line[x] = Mix565(a[x], b[x], blend);
        };
        for (int y = 135; y >= first; --y) row(y);
        for (int y = 136; y < last; ++y) row(y);
        for (int y = 0; y < first; ++y) std::memset(pixels + y * 512, 0, 480 * 2);
        for (int y = last; y < 272; ++y) std::memset(pixels + y * 512, 0, 480 * 2);
    }
}
}
