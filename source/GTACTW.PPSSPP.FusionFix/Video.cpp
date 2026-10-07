#include "Game.hpp"
#include <cmath>

namespace ctw {
namespace {
injector::hook_back<void(uint32_t**, int, int16_t, int, int16_t, int, int16_t, int, int16_t, int16_t)> movieQuad;

void DrawMovieQuad(uint32_t** context, int x0, int16_t y0, int u0, int16_t v0,
                   int x1, int16_t y1, int u1, int16_t v1, int16_t z) {
    RefreshAspect();
    float horizontal = HorizontalScale();
    float vertical = 1.0f;
    if (horizontal > 1.0f) {
        vertical = 1.0f / horizontal;
        horizontal = 1.0f;
    }
    // Bink's YUV passes use through-mode GE sprites, bypassing the camera and
    // UI projection. Fit their destination rectangle, keeping all six passes
    // together and leaving the native texture coordinates unchanged.
    const auto x = [horizontal](int value) { return int(std::lround(240.0f + (value - 240.0f) * horizontal)); };
    const auto y = [vertical](int value) { return int16_t(std::lround(136.0f + (value - 136.0f) * vertical)); };
    movieQuad.fun(context, x(x0), y(y0), u0, v0, x(x1), y(y1), u1, v1, z);
}
}

void InstallVideo() {
    const uintptr_t sites[] = {Address<0x08860650>(), Address<0x08860870>(), Address<0x08860A14>(),
                              Address<0x08860B1C>(), Address<0x08860CA8>(), Address<0x08860DB0>()};
    for (auto site : sites)
        movieQuad.fun = injector::MakeCALL(site, DrawMovieQuad).get();
}
}
