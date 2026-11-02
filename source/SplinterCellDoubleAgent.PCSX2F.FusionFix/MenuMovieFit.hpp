#pragma once
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
namespace scda {
// Refit a 16x16-tiled RGBA movie into the game's 4:3 frame. Fill fits the
// complete video; Original keeps the game's full 4:3 composition.
inline bool FitMovie(const uint32_t* source, uint32_t* destination,
                     unsigned x, unsigned y, unsigned width, unsigned height,
                     unsigned frameWidth, unsigned frameHeight, float aspect, bool fill) {
    if (!source || !destination || !width || !height || !frameWidth || !frameHeight ||
        width > 640 || frameWidth > 640 || frameHeight > 512 ||
        (width & 15) || (height & 15) || (frameWidth & 15) || (frameHeight & 15) ||
        x > frameWidth || y > frameHeight || width > frameWidth - x || height > frameHeight - y ||
        !std::isfinite(aspect) || aspect <= 0) return false;
    constexpr float nativeAspect = 4.0f / 3.0f;
    unsigned outWidth, outHeight, left, top;
    if (fill) {
        const float natural = nativeAspect * float(frameHeight) * float(width) /
                              (float(frameWidth) * float(height));
        const float ratio = natural / aspect;
        outWidth = unsigned(float(frameWidth) * (ratio < 1.0f ? ratio : 1.0f) + 0.5f);
        outHeight = unsigned(float(frameHeight) * (ratio > 1.0f ? 1.0f / ratio : 1.0f) + 0.5f);
        left = (frameWidth - outWidth) / 2;
        top = (frameHeight - outHeight) / 2;
    } else {
        const float ratio = nativeAspect / aspect;
        const float xs = ratio < 1.0f ? ratio : 1.0f;
        const float ys = ratio > 1.0f ? 1.0f / ratio : 1.0f;
        outWidth = unsigned(xs * float(width) + 0.5f);
        outHeight = unsigned(ys * float(height) + 0.5f);
        left = unsigned((1.0f - xs) * float(frameWidth) * 0.5f + xs * float(x) + 0.5f);
        top = unsigned((1.0f - ys) * float(frameHeight) * 0.5f + ys * float(y) + 0.5f);
    }
    if (!outWidth || !outHeight || left > frameWidth - outWidth || top > frameHeight - outHeight) return false;
    for (unsigned i = 0; i < frameWidth * frameHeight; ++i) destination[i] = 0xFF000000;
    if (outWidth == width && frameWidth == width && left == 0) {
        // Full-width movies need only vertical resampling. Copy each 16-pixel
        // tile row as a block instead of recalculating every pixel address.
        for (unsigned py = 0; py < outHeight; ++py) {
            const unsigned sy = ((2 * py + 1) * height) / (2 * outHeight);
            const unsigned dy = top + py;
            const size_t sourceRow = size_t(sy >> 4) * (width >> 4) * 256 + ((sy & 15) << 4);
            const size_t targetRow = size_t(dy >> 4) * (frameWidth >> 4) * 256 + ((dy & 15) << 4);
            for (unsigned tile = 0; tile < width / 16; ++tile)
                std::memcpy(destination + targetRow + tile * 256,
                            source + sourceRow + tile * 256, 16 * sizeof(uint32_t));
        }
        return true;
    }
    unsigned sourceX[640], targetX[640];
    for (unsigned px = 0; px < outWidth; ++px) {
        const unsigned sx = ((2 * px + 1) * width) / (2 * outWidth);
        const unsigned dx = left + px;
        sourceX[px] = (sx >> 4) * 256 + (sx & 15);
        targetX[px] = (dx >> 4) * 256 + (dx & 15);
    }
    for (unsigned py = 0; py < outHeight; ++py) {
        const unsigned sy = ((2 * py + 1) * height) / (2 * outHeight);
        const unsigned dy = top + py;
        const size_t sourceRow = size_t(sy >> 4) * (width >> 4) * 256 + ((sy & 15) << 4);
        const size_t targetRow = size_t(dy >> 4) * (frameWidth >> 4) * 256 + ((dy & 15) << 4);
        for (unsigned px = 0; px < outWidth; ++px)
            destination[targetRow + targetX[px]] = source[sourceRow + sourceX[px]];
    }
    return true;
}
inline void FitMenuBackground(const uint32_t* source, uint32_t* destination,
                              unsigned width, unsigned height, float aspect) {
    FitMovie(source, destination, 0, 0, width, height, width, height, aspect, true);
}
}
