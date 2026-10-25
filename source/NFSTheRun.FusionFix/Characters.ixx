module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <unordered_map>
#include <shared_mutex>

export module Characters;

import ComVars;

// Characters drawn between the 30 Hz physics ticks. Above 30 frames per second the game runs the animation once more
// on every frame between the ticks, while the characters' parents (cars, the cutscene's objects) are already drawn
// between the ticks: the pose comes out ahead of where it's placed, and characters in cutscenes jump back and forth.
// The animation's own blending between the ticks would need buffers that only exist when it's on from the start.
// Instead the animation runs on the ticks only, a full tick at a time, and the bones asked for between the ticks are
// put between the last two tick poses, the same way the game draws everything else between the ticks.

namespace
{
    uintptr_t* pAnimationManager = nullptr;

    float Alpha = 1.0f;         // the place of the frame between the last two ticks, 0..1
    uint32_t Tick = 0;
    uint32_t Frame = 0;
    bool bInTick = false;
    bool bInterpolation = false;        // frames between the ticks are drawn at all
    std::shared_mutex PosesMutex;       // the bones are asked for on other threads too

    struct Pose
    {
        std::vector<uint8_t> previous, current, blended;
        uint32_t previousTick = 0, currentTick = 0, seenTick = 0, blendedFrame = 0;
        std::mutex blendMutex;
    };
    std::unordered_map<uintptr_t, Pose> Poses;      // by the address of the skeleton slot's bone matrices

    constexpr uint32_t BoneSize = 48;                 // a bone: 3 rows of 4 floats

    // AnimationManager +0x94 -> skeleton manager, +0x40 -> list of skeletons {next, prev, skeleton}
    template<typename Fn>
    void ForEachPalette(Fn&& fn)
    {
        auto manager = *pAnimationManager;
        if (!manager)
            return;
        auto skeletons = *(uintptr_t*)(manager + 0x94);
        if (!skeletons)
            return;
        auto sentinel = skeletons + 0x40;
        int guard = 0;
        for (auto node = *(uintptr_t*)sentinel; node && node != sentinel && guard < 4096; node = *(uintptr_t*)node, ++guard)
        {
            auto skeleton = *(uintptr_t*)(node + 8);
            if (!skeleton)
                continue;
            auto size = *(uint32_t*)(skeleton + 0x20);
            auto slot = *(int32_t**)(skeleton + 0xCC);
            auto slotEnd = *(int32_t**)(skeleton + 0xD0);
            auto blocks = *(uintptr_t**)(skeleton + 0xE0);
            if (!size || size % BoneSize || !slot || !blocks)
                continue;
            for (; slot < slotEnd; ++slot)
            {
                auto id = *slot;
                if (id < 0)
                    continue;
                auto block = blocks[id >> 24];
                auto index = id & 0xFF;
                if (!block || index > 3)
                    continue;
                auto readers = *(uintptr_t*)(block + 0x20);
                auto reader = readers ? *(uintptr_t*)(readers + index * 12) : 0;
                if (!reader || !*(uint8_t*)(reader + 0x24))
                    continue;
                auto palette = *(uintptr_t*)(block + 0x08);
                if (palette)
                    fn(palette + index * size, size);
            }
        }
    }

    float BlendFactor(const Pose& pose)
    {
        float span = float(pose.currentTick - pose.previousTick);
        return std::clamp((Alpha + float(Tick - pose.currentTick)) / span, 0.0f, 1.0f);
    }

    // After the animation of a tick: the new poses are kept with the one before
    void SnapshotPoses()
    {
        std::unique_lock lock(PosesMutex);
        ++Tick;
        auto interpolation = fb::Settings::Field<bool>("ClientSettings", "VisualFrameInterpolation");
        bInterpolation = interpolation && *interpolation;
        ForEachPalette([](uintptr_t palette, uint32_t size)
        {
            auto& pose = Poses[palette];
            pose.seenTick = Tick;
            auto bytes = (const uint8_t*)palette;
            if (pose.current.size() != size)
            {
                pose.current.assign(bytes, bytes + size);
                pose.previous = pose.current;
                pose.previousTick = pose.currentTick = Tick;
                return;
            }
            if (memcmp(pose.current.data(), bytes, size) == 0)
                return;
            pose.previous.swap(pose.current);
            pose.current.assign(bytes, bytes + size);
            pose.previousTick = pose.currentTick;
            pose.currentTick = Tick;

            // A character put somewhere else, or not animated for a while: no blending from where it was
            float dx = ((float*)pose.current.data())[3] - ((float*)pose.previous.data())[3];
            float dy = ((float*)pose.current.data())[7] - ((float*)pose.previous.data())[7];
            float dz = ((float*)pose.current.data())[11] - ((float*)pose.previous.data())[11];
            if (dx * dx + dy * dy + dz * dz > 5.0f * 5.0f || pose.currentTick - pose.previousTick > 10)
            {
                pose.previous = pose.current;
                pose.previousTick = pose.currentTick;
            }
        });

        // Skeletons that are gone
        for (auto it = Poses.begin(); it != Poses.end();)
            it = (Tick - it->second.seenTick > 30) ? Poses.erase(it) : std::next(it);
    }

    // The animation update of a physics tick: arg +8 = the place of the frame between the ticks
    SafetyHookInline shTickAnimation = {};
    void __stdcall TickAnimation(float* info)
    {
        Alpha = info[2];
        ++Frame;
        bInTick = true;
        shTickAnimation.unsafe_stdcall(info);
        SnapshotPoses();
        bInTick = false;
    }

    // The animation update of a frame between the ticks: only its place between the ticks (+4) is taken
    SafetyHookInline shFrameAnimation = {};
    void __stdcall FrameAnimation(float* info)
    {
        Alpha = info[1];
        ++Frame;
    }

    // The bone matrices of an animated object, for the renderer (GameAnimatable): outside the ticks it gets the bones
    // between the last two tick poses
    SafetyHookInline shGetBones = {};
    uintptr_t __fastcall GetBones(void* animatable, void*)
    {
        auto bones = shGetBones.unsafe_thiscall<uintptr_t>(animatable);
        if (!bones || bInTick || !bInterpolation)
            return bones;

        std::shared_lock lock(PosesMutex);
        auto it = Poses.find(bones);
        if (it == Poses.end())
            return bones;
        auto& pose = it->second;
        if (pose.currentTick == pose.previousTick || pose.previous.size() != pose.current.size())
            return bones;

        std::lock_guard blendLock(pose.blendMutex);
        if (pose.blendedFrame != Frame || pose.blended.size() != pose.current.size())
        {
            pose.blended.resize(pose.current.size());
            float f = BlendFactor(pose);
            auto a = (const float*)pose.previous.data();
            auto b = (const float*)pose.current.data();
            auto out = (float*)pose.blended.data();
            for (size_t i = 0, n = pose.current.size() / sizeof(float); i < n; ++i)
                out[i] = a[i] + (b[i] - a[i]) * f;
            pose.blendedFrame = Frame;
        }
        return (uintptr_t)pose.blended.data();
    }
}

class Characters
{
public:
    Characters()
    {
        WFP::onInitEvent() += []()
        {
            if (FixSettings().Int("FRAMERATE.UnlockFrameRate") == 0 || FixSettings().Int("FRAMERATE.UnlockCutscenes") == 0)
                return;

            auto tick = hook::pattern("55 8B EC 83 E4 F0 81 EC 94 00 00 00 A1 ? ? ? ? 53 56 57 89 44 24 18 85 C0 0F 84 ? ? ? ? 8B 7D 08 F3 0F 10 47 0C");
            auto tickDt = hook::pattern("F3 0F 10 47 0C F2 0F 10 0D ? ? ? ? 8B 35 ? ? ? ? 0F 5A C0 F2 0F 5C C8 F3 0F 10 47 08 0F 5A C0 F2 0F 58 C8 F3 0F 10 07");
            auto frame = hook::pattern("55 8B EC 83 E4 F0 81 EC C4 00 00 00 53 8B 1D ? ? ? ? 56 57 85 DB 0F 84");
            auto getBones = hook::pattern("83 B9 88 01 00 00 02 75 08 8A 81 44 02 00 00 EB 0D 8B 81 B4 00 00 00 85 C0 74 11 8A 40 40 84 C0 74 0A 8B 81 B4 00 00 00 8B 40 24 C3");
            if (tick.empty() || tickDt.empty() || frame.empty() || getBones.empty())
                return;

            pAnimationManager = *tick.get_first<uintptr_t*>(13);

            // A tick advances the animation by a whole tick, not up to the frame's place between the ticks
            injector::MakeNOP(tickDt.get_first(0x16), 4, true);     // subsd: 1 - previous place
            injector::MakeNOP(tickDt.get_first(0x22), 4, true);     // addsd: + place

            shTickAnimation = safetyhook::create_inline(tick.get_first(), TickAnimation);
            shFrameAnimation = safetyhook::create_inline(frame.get_first(), FrameAnimation);
            shGetBones = safetyhook::create_inline(getBones.get_first(), GetBones);
        };
    }
} Characters;
