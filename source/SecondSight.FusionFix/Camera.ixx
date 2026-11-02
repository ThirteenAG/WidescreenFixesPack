module;

#include <stdafx.h>
#include <algorithm>
#include <atomic>
#include <bit>
#include <cmath>


export module Camera;


export namespace SecondSightCamera
{
    inline std::atomic<float> Factor{ 1.0f };
    inline SafetyHookMid ProjectionHook, FrustumAngleHook, FrustumArgumentHook;
    inline thread_local float FrustumDegrees = 0.0f, FrustumAspect = 1.0f;
    inline bool (*InCutscene)() = nullptr;

    inline float VerticalFOV(float degrees, float aspect, float factor)
    {
        if (!std::isfinite(degrees) || degrees <= 0.0f || !std::isfinite(aspect) || aspect <= 0.0f)
            return degrees;
        if (!std::isfinite(factor))
            factor = 1.0f;

        constexpr float RadiansPerDegree = 0.017453292519943295f;
        constexpr float Tan75 = 3.7320508075688773f;
        const float maximum = 2.0f * std::atan(Tan75 / aspect) / RadiansPerDegree;
        // Keep the established framing, with the same lens for the matrix and visibility tests.
        return std::min(degrees * 0.8f * std::clamp(factor, 0.5f, 2.0f), maximum);
    }

    // Match the visibility planes to the angle and aspect used by the active projection.
    inline void FrustumAngle(SafetyHookContext& context)
    {
        const auto camera = reinterpret_cast<const uint8_t*>(context.esi);
        const float raw = *reinterpret_cast<const float*>(camera + 0x374);
        const float vertical = *reinterpret_cast<const float*>(camera + 0x36C);
        const float horizontal = *reinterpret_cast<const float*>(camera + 0x370);
        float aspect = vertical > 0.0f ? horizontal / vertical : 1.0f;
        if (!std::isfinite(aspect) || aspect <= 0.0f)
            aspect = 1.0f;
        FrustumAspect = aspect;
        const float factor = InCutscene && InCutscene() ? 1.0f : Factor.load(std::memory_order_relaxed);
        FrustumDegrees = VerticalFOV(raw, aspect, factor);
        *reinterpret_cast<float*>(context.esp + 0x14) = FrustumDegrees * 0.008726646259971648f;
    }

    inline void FrustumArgument(SafetyHookContext& context)
    {
        context.eax = std::bit_cast<uint32_t>(FrustumDegrees);
        *reinterpret_cast<float*>(context.esp) = 1.0f / FrustumAspect;
    }

    inline void Projection(SafetyHookContext& context)
    {
        auto& degrees = *reinterpret_cast<float*>(context.esp + 0x10);
        const auto aspect = *reinterpret_cast<const float*>(context.esp + 0x0C);
        const float factor = InCutscene && InCutscene() ? 1.0f : Factor.load(std::memory_order_relaxed);
        degrees = VerticalFOV(degrees, aspect, factor);
    }
}
