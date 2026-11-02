#include "Game.hpp"
#include <cmath>
#include <cstdlib>

namespace ctw {
namespace {
SafetyMipsInline cameraBuilder;
injector::hook_back<void(void*)> buildCamera;
float builtScale = 1.0f;
// Story scenes (application 54) keep the native angle; the FOV factor is for
// gameplay.
float TangentScale() {
    float scale = targetAspect < nativeAspect ? nativeAspect / targetAspect : 1.0f;
    if (!CutsceneApp()) scale *= fovFactor;
    return scale;
}
void BuildCamera(void* object) {
    RefreshAspect();
    const auto camera = reinterpret_cast<uintptr_t>(object);
    // This native camera owns both the GE matrix and fixed-point frustum
    // limits. Changing its Q12 aspect lets the engine update them together.
    // The builder reads it back as a signed 16-bit value (0x088559AC: the
    // int16 cast before "(+272 * aspect) >> 12", stored to a halfword), so
    // 8:1 (32768) wrapped negative. Keep both the aspect and that product in
    // int16 range; +272 holds the last value derived from the unchanged FOV.
    int aspect = int(targetAspect * 4096.0f + 0.5f);
    if (aspect < 1024) aspect = 1024;
    if (aspect > 32767) aspect = 32767;
    const int factor = std::abs(int(at<int16_t>(camera + 272)));
    if (factor > 4096 && aspect > (32767 << 12) / factor) aspect = (32767 << 12) / factor;
    if (at<int>(camera + 88) != aspect) {
        at<int>(camera + 88) = aspect;
        at<uint8_t>(camera + 106) = 1;
    }
    // The vertical half angle is kept as Q12 sine (+80) and cosine (+84); the
    // builder derives the projection and the frustum planes from them. Narrow
    // screens open the vertical angle so the native horizontal view stays
    // (Vert+), and gameplay views get the FOV factor. The native pair is put
    // back after the build, so the game's own zoom values stay unchanged.
    const float tangentScale = TangentScale();
    if (tangentScale != builtScale) {
        builtScale = tangentScale;
        at<uint8_t>(camera + 106) = 1;
    }
    if (tangentScale == 1.0f || !at<uint8_t>(camera + 106)) {
        buildCamera.fun(object);
        return;
    }
    auto& sine = at<int>(camera + 80);
    auto& cosine = at<int>(camera + 84);
    const int nativeSine = sine, nativeCosine = cosine;
    if (nativeCosine > 0) {
        const float tangent = float(nativeSine) / float(nativeCosine) * tangentScale;
        const float scaledCosine = 1.0f / std::sqrt(1.0f + tangent * tangent);
        sine = int(tangent * scaledCosine * 4096.0f + 0.5f);
        cosine = int(scaledCosine * 4096.0f + 0.5f);
    }
    buildCamera.fun(object);
    sine = nativeSine;
    cosine = nativeCosine;
}
}
// Upper bound below 8:1 so the Q12 camera aspect fits a signed halfword.
inline constexpr float maximumAspect = 32767.0f / 4096.0f;
void RefreshAspect() {
    if (automaticAspect) targetAspect = console::bounded(console::portable::Aspect(), 0.5f, maximumAspect, nativeAspect);
}
void InstallAspect() {
    char ratioBuffer[64]{};
    const char* ratio = inireader.ReadString("MAIN", "ForceAspectRatio", "auto", ratioBuffer, sizeof(ratioBuffer));
    char* separator = nullptr;
    const float numerator = std::strtof(ratio, &separator);
    if (separator && *separator == ':') {
        char* end = nullptr;
        const float denominator = std::strtof(separator + 1, &end);
        if (end && !*end && numerator > 0 && denominator > 0) {
            targetAspect = console::bounded(numerator / denominator, 0.5f, maximumAspect, nativeAspect);
            automaticAspect = false;
        }
    }
    RefreshAspect();
    fovFactor = console::bounded(inireader.ReadFloat("MAIN", "FOVFactor", 1.0f), 0.5f, 2.0f, 1.0f);
#ifndef NDEBUG
    logger.WriteF("Camera builder %08X words %08X %08X previous %08X", unsigned(Address<0x088559AC>()), injector::ReadMemory<uint32_t>(Address<0x088559AC>()), injector::ReadMemory<uint32_t>(Address<0x088559AC>()+4), injector::ReadMemory<uint32_t>(Address<0x088559AC>()-4));
#endif
    cameraBuilder = safetymips::create_inline(Address<0x088559AC>(), BuildCamera);
    buildCamera.fun = cameraBuilder.original<void(*)(void*)>();
}
}
