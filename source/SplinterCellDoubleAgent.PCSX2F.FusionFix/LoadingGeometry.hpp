#pragma once
#include <algorithm>
#include <cstdint>

namespace scda {
// GS XYZ coordinates use 12.4 fixed point around the display centre 0x8000.
inline uint64_t FitLoadingVertex(uint64_t vertex, float aspect) {
    constexpr float nativeAspect = 4.0f / 3.0f;
    const float scaleX = std::min(1.0f, nativeAspect / aspect);
    const float scaleY = std::min(1.0f, aspect / nativeAspect);
    const auto x = uint16_t(vertex), y = uint16_t(vertex >> 16);
    const auto newX = uint16_t(32768.0f + (float(x) - 32768.0f) * scaleX + .5f);
    const auto newY = uint16_t(32768.0f + (float(y) - 32768.0f) * scaleY + .5f);
    return (vertex & 0xFFFFFFFF00000000ull) | uint64_t(newX) | (uint64_t(newY) << 16);
}
}
