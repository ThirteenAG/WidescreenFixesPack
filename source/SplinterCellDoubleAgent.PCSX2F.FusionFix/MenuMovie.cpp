#include "Game.hpp"
#include "MenuMovieFit.hpp"
#include <cmath>
#include <cstdio>

namespace scda {
namespace {
// Each ring may contain references to several video uploads. Its buffers are
// reusable only when the game resets that ring after its DMA synchronization.
constexpr unsigned RingBuffers = 2;
constexpr unsigned MaxPixels = 640 * 512;
alignas(16) uint32_t frames[2][RingBuffers][MaxPixels];
unsigned used[2];
SafetyMipsMid uploadHook, resetHook;
using MovieDraw = int32_t(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
pcsx2::GameFunction<MovieDraw> launcherOriginal;
pcsx2::GameCallback<MovieDraw> launcherCallback;

int32_t DrawLauncherMovie(uint32_t target, uint32_t x, uint32_t y,
                          uint32_t width, uint32_t height, uint32_t source) {
    UpdateSettings(true);
    const unsigned frameWidth = width == 640 ? 640 : 512;
    const unsigned frameHeight = frameWidth == 640 ? 448 : 512;
    if ((width != 640 && width != 512) || !source ||
        !FitMovie(reinterpret_cast<const uint32_t*>(source), frames[0][0], x, y, width, height,
                  frameWidth, frameHeight, Aspect(), videoMode != 0))
        return launcherOriginal(target, x, y, width, height, source);

    // Each regional uploader accepts its own full frame: 640x448 on NTSC,
    // 512x512 on PAL. It waits for its DMA transfer before returning.
    return launcherOriginal(target, 0, 0, frameWidth, frameHeight,
                            reinterpret_cast<uintptr_t>(frames[0][0]));
}

}

void InstallMenuMovie() {
    resetHook = safetymips::create_mid(game.movieRingReset, [](SafetyMipsContext& r) {
        if (r.a0 == game.movieRingBase) used[0] = 0;
        else if (r.a0 == game.movieRingBase + 40) used[1] = 0;
    });
    uploadHook = safetymips::create_mid(game.movieUpload, [](SafetyMipsContext& r) {
        UpdateSettings(true);
        const unsigned width = game.projection == EU.projection ? 512 : 640;
        const unsigned height = width == 512 ? 512 : 448;
        if (r.a1 != 0 || r.a3 != width || !r.t0 || r.t0 > height ||
            (r.t0 & 15) || r.a2 != (height - r.t0) / 2 || !r.t1) return;
        const float aspect = Aspect();
        if (!std::isfinite(aspect) || aspect <= 0 || std::fabs(aspect - 4.0f / 3.0f) < 0.0001f) return;
        const unsigned ring = Read<uint32_t>(game.movieRingIndex);
        if (ring >= 2 || used[ring] >= RingBuffers) {
            static bool warned;
            if (!warned) {
                std::snprintf(OSDText[1], OSDStringSize, "Video buffer unavailable; using the game display.");
                warned = true;
            }
            return;
        }
        uint32_t* destination = frames[ring][used[ring]];
        if (!FitMovie(reinterpret_cast<const uint32_t*>(r.t1), destination, r.a1, r.a2, r.a3, r.t0,
                      width, height, aspect, videoMode != 0)) return;
        ++used[ring];
        r.a1 = 0; r.a2 = 0; r.a3 = width; r.t0 = height;
        r.t1 = reinterpret_cast<uintptr_t>(destination);
    });
}

void InstallLauncherMovies() {
    const auto background = pattern.get_first("? ? 44 8C 31 00 04 3C 2D 38 20 02 2D 40 00 02 ? ? 84 24 2D 28 00 00 2D 30 00 00 88 79 08 0C 2D 48 40 00", 28);
    const auto movie = pattern.get_first("D0 00 A6 8F 31 00 04 3C C0 00 A7 8F 2D 48 40 00 ? ? 84 24 2D 28 00 00 88 79 08 0C 2D 40 C0 03", 24);
    if (!background || !movie)
    {
        std::snprintf(OSDText[1], OSDStringSize, "%s", Notice(2));
        return;
    }
    launcherOriginal.bind(0x21E620);
    launcherCallback.bind(DrawLauncherMovie);
    injector::MakeCALL(background, launcherCallback.address());
    injector::MakeCALL(movie, launcherCallback.address());
}
}
