#include "Game.hpp"
#include <cstdlib>

namespace ctw {
namespace {
SafetyMipsInline cameraBuilder;
injector::hook_back<void(void*)> buildCamera;
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
    buildCamera.fun(object);
}
}
// Upper bound below 8:1 so the Q12 camera aspect fits a signed halfword.
inline constexpr float maximumAspect = 32767.0f / 4096.0f;
void RefreshAspect() {
    if (automaticAspect) targetAspect = console::bounded(console::portable::Aspect(), 0.5f, maximumAspect, nativeAspect);
}
void InstallAspect() {
    char ratio[64];
    inireader.ReadString("MAIN", "ForceAspectRatio", "auto", ratio, sizeof(ratio));
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
#ifndef NDEBUG
    logger.WriteF("Camera builder %08X words %08X %08X previous %08X", unsigned(Address<0x088559AC>()), injector::ReadMemory<uint32_t>(Address<0x088559AC>()), injector::ReadMemory<uint32_t>(Address<0x088559AC>()+4), injector::ReadMemory<uint32_t>(Address<0x088559AC>()-4));
#endif
    cameraBuilder = safetymips::create_inline(Address<0x088559AC>(), BuildCamera);
    buildCamera.fun = cameraBuilder.original<void(*)(void*)>();
}
}
