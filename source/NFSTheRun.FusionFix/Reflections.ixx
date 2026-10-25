module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module Reflections;

import ComVars;

// The car reflections come from an environment map rendered around a point that follows the player's car. The game
// moves that point on every physics tick, 30 times a second, while the car itself is drawn between the ticks above
// 30 frames per second, so the reflections jump along the car (issue #2063). On every frame the point is put between
// the last two physics tick positions, the same way the game draws the car between them.

namespace
{
    struct Vec4 { float x, y, z, w; };

    uintptr_t* pEnvmapEntity = nullptr;     // the reflection point: position at +0x20, direction at +0x30

    struct Sample { Vec4 position, direction; };
    Sample Previous = {}, Current = {}, Written = {};
    double TickTime = 0.0;
    double TickInterval = 1.0 / 30.0;
    bool bValid = false;
    uintptr_t LastEntity = 0;

    double Now()
    {
        static LARGE_INTEGER frequency = []() { LARGE_INTEGER f; QueryPerformanceFrequency(&f); return f; }();
        LARGE_INTEGER counter;
        QueryPerformanceCounter(&counter);
        return double(counter.QuadPart) / double(frequency.QuadPart);
    }

    Vec4 Lerp(const Vec4& a, const Vec4& b, float t)
    {
        return { a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, b.w };
    }

    bool Equal(const Sample& a, const Sample& b)
    {
        return memcmp(&a, &b, sizeof(Sample)) == 0;
    }

    void UpdateReflectionPoint()
    {
        auto entity = *pEnvmapEntity;
        if (!entity)
        {
            bValid = false;
            return;
        }

        auto& position = *(Vec4*)(entity + 0x20);
        auto& direction = *(Vec4*)(entity + 0x30);
        Sample game = { position, direction };
        double now = Now();

        // Without the frames between the physics ticks the car is drawn at the tick position, and so is the point
        auto interpolation = fb::Settings::Field<bool>("ClientSettings", "VisualFrameInterpolation");
        if (!interpolation || !*interpolation)
        {
            bValid = false;
            return;
        }

        if (!bValid || entity != LastEntity)
        {
            Previous = Current = Written = game;
            TickTime = now;
            bValid = true;
            LastEntity = entity;
            return;
        }

        // A value that isn't the one written last is the game's new physics tick position
        if (!Equal(game, Written))
        {
            Previous = Current;
            Current = game;
            TickInterval = std::clamp(now - TickTime, 1.0 / 240.0, 0.1);
            TickTime = now;

            // The car was put somewhere else: no gliding there
            float dx = Current.position.x - Previous.position.x;
            float dy = Current.position.y - Previous.position.y;
            float dz = Current.position.z - Previous.position.z;
            if (dx * dx + dy * dy + dz * dz > 20.0f * 20.0f)
                Previous = Current;
        }

        float t = (float)std::clamp((now - TickTime) / TickInterval, 0.0, 1.0);
        Sample out;
        out.position = Lerp(Previous.position, Current.position, t);
        out.direction = Lerp(Previous.direction, Current.direction, t);
        float length = std::sqrt(out.direction.x * out.direction.x + out.direction.y * out.direction.y + out.direction.z * out.direction.z);
        if (length > 0.0001f)
        {
            out.direction.x /= length;
            out.direction.y /= length;
            out.direction.z /= length;
        }
        else
            out.direction = Current.direction;

        position = out.position;
        direction = out.direction;
        Written = out;
    }

    // ClientChassisComponent::onFrameInterpolationUpdate: on every frame, after the physics and before drawing
    SafetyHookInline shFrameInterpolationUpdate = {};
    void __fastcall FrameInterpolationUpdate(void* chassis, void* edx, void* info)
    {
        shFrameInterpolationUpdate.unsafe_fastcall(chassis, edx, info);
        UpdateReflectionPoint();
    }
}

class Reflections
{
public:
    Reflections()
    {
        WFP::onInitEvent() += []()
        {
            if (FixSettings().Int("FRAMERATE.SmoothReflections") == 0)
                return;

            auto pattern = hook::pattern("A1 ? ? ? ? 85 C0 74 3C F3 0F 7E 84 24 80 00 00 00 66 0F D6 40 20");
            auto chassis = hook::pattern("8B 54 24 04 56 8B F1 8B 8E ? ? ? ? 8B 01 8B 40 1C 52 FF D0 8B CE E8 ? ? ? ? 8B CE E8 ? ? ? ? 8B CE E8");
            if (pattern.empty() || chassis.empty())
                return;

            pEnvmapEntity = *pattern.get_first<uintptr_t*>(1);
            shFrameInterpolationUpdate = safetyhook::create_inline(chassis.get_first(), FrameInterpolationUpdate);
        };
    }
} Reflections;
