module;

#include <stdafx.h>

export module Reflections;

// The car reflections come from an environment map that is drawn every frame around a point following the player's
// car. The game moves that point only on the physics ticks, 30 times a second, while the car itself is drawn between
// the last two ticks, so above 30 frames per second the reflections jump along the car (issue #2063). On every frame
// the point is put between its last two tick positions, at the frame's place between the ticks, as the car is.

namespace
{
    struct Vec4 { float x, y, z, w; };
    struct Sample { Vec4 position, direction; };    // the point: position at +0x20, direction at +0x30

    uintptr_t* pEnvmapEntity = nullptr;
    Sample Previous = {}, Current = {};             // the point at the last two ticks
    uintptr_t SampledEntity = 0;

    Sample& Point(uintptr_t entity)
    {
        return *(Sample*)(entity + 0x20);
    }

    Vec4 Lerp(const Vec4& a, const Vec4& b, float t)
    {
        return { a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, b.w };
    }

    // ClientVehicleEnvMapFocusEntity: moves the point to the car on a physics tick
    SafetyHookInline shFocusUpdate = {};
    void __fastcall FocusUpdate(void* focus, void* edx, void* info)
    {
        auto entity = *pEnvmapEntity;
        auto before = entity ? Point(entity) : Sample{};
        shFocusUpdate.unsafe_fastcall(focus, edx, info);

        // Only the focus the map follows moves the point
        entity = *pEnvmapEntity;
        if (!entity)
        {
            SampledEntity = 0;
            return;
        }
        auto game = Point(entity);
        if (entity == SampledEntity && memcmp(&game, &before, sizeof(Sample)) == 0)
            return;

        if (entity != SampledEntity)
        {
            Previous = Current = game;
            SampledEntity = entity;
            return;
        }
        Previous = Current;
        Current = game;

        // The car was put somewhere else: no gliding there
        float dx = Current.position.x - Previous.position.x;
        float dy = Current.position.y - Previous.position.y;
        float dz = Current.position.z - Previous.position.z;
        if (dx * dx + dy * dy + dz * dz > 20.0f * 20.0f)
            Previous = Current;
    }

    // The client's update of a frame, after its physics ticks: the third argument is the frame's place between the last
    // two ticks (1 without the frames between the ticks)
    SafetyHookInline shFrameUpdate = {};
    int __fastcall FrameUpdate(void* client, void* edx, float deltaTime, float tickTime, float alpha, float time, int ticks)
    {
        auto result = shFrameUpdate.unsafe_fastcall<int>(client, edx, deltaTime, tickTime, alpha, time, ticks);

        auto entity = *pEnvmapEntity;
        if (!entity || entity != SampledEntity || !(alpha >= 0.0f && alpha <= 1.0f))
            return result;

        Sample out;
        out.position = Lerp(Previous.position, Current.position, alpha);
        out.direction = Lerp(Previous.direction, Current.direction, alpha);
        float length = std::sqrt(out.direction.x * out.direction.x + out.direction.y * out.direction.y + out.direction.z * out.direction.z);
        if (length > 0.0001f)
        {
            out.direction.x /= length;
            out.direction.y /= length;
            out.direction.z /= length;
        }
        else
            out.direction = Current.direction;
        Point(entity) = out;
        return result;
    }
}

class Reflections
{
public:
    Reflections()
    {
        WFP::onInitEvent() += []()
        {
            auto entity = hook::pattern("A1 ? ? ? ? 85 C0 74 3C F3 0F 7E 84 24 80 00 00 00 66 0F D6 40 20");
            auto focus = hook::pattern("55 8B EC 83 E4 F0 81 EC 98 00 00 00 81 3D ? ? ? ? ? ? ? ? 56 57");
            auto frame = hook::pattern("55 8B EC 83 E4 F0 8B 45 18 D9 45 10");

            pEnvmapEntity = *entity.get_first<uintptr_t*>(1);
            shFocusUpdate = safetyhook::create_inline(focus.get_first(), FocusUpdate);
            shFrameUpdate = safetyhook::create_inline(frame.get_first(), FrameUpdate);
        };
    }
} Reflections;
