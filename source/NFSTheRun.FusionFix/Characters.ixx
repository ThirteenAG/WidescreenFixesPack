module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <unordered_map>
#include <shared_mutex>

export module Characters;

import ComVars;

// Animated objects drawn between the 30 Hz physics ticks. Above 30 frames per second the game runs the animation once
// more on every frame between the ticks, while the objects' parents (cars, the cutscene's objects) are already drawn
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
        std::vector<float> previous, current, blended;
        uint32_t previousTick = 0, currentTick = 0, seenTick = 0, blendedFrame = 0;
        uintptr_t source = 0;
        std::mutex blendMutex;
    };

    // Skinned characters: the bone matrices for the renderer, by their address
    std::unordered_map<uintptr_t, Pose> Poses;

    // Objects that aren't skinned (falling trees and rocks, planes, car doors, ...) take their parts' places from the
    // animation's other output, a position and a rotation per bone, through a getter of their own. By the address of
    // the animation's result data.
    std::unordered_map<uintptr_t, Pose> Transforms;

    constexpr uint32_t BoneSize = 48;               // a bone matrix: 3 rows of 4 floats
    constexpr uint32_t TransformSize = 32;          // a bone transform: the position and scale, then the rotation

    // The animation's result data: +0x08 bone matrices, +0x14 bone transforms, +0x24 written by the animation.
    // It's at +0x1C in the animatable.
    constexpr uintptr_t ResultInAnimatable = 0x1C;

    // AnimationManager +0x94 -> skeleton manager, +0x40 -> list of skeletons {next, prev, skeleton}.
    // Skeleton +0x04 bone count, +0x1C size of a slot's bone transforms (32 per bone), +0x20 size of a slot's bone
    // matrices (48 per bone), +0xCC/+0xD0 active slots, +0xE0 blocks of 4 slots: +0x00/+0x04/+0x0C/+0x10 bone
    // transforms, +0x08 bone matrices, +0x20 the result data reading each slot.
    template<typename Fn>
    void ForEachSlot(Fn&& fn)
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
            auto slot = *(int32_t**)(skeleton + 0xCC);
            auto slotEnd = *(int32_t**)(skeleton + 0xD0);
            auto blocks = *(uintptr_t**)(skeleton + 0xE0);
            if (!slot || !blocks)
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
                auto result = readers ? *(uintptr_t*)(readers + index * 12) : 0;
                if (!result || !*(uint8_t*)(result + 0x24))
                    continue;
                fn(skeleton, block, uint32_t(index), result);
            }
        }
    }

    float BlendFactor(const Pose& pose)
    {
        float span = float(pose.currentTick - pose.previousTick);
        return std::clamp((Alpha + float(Tick - pose.currentTick)) / span, 0.0f, 1.0f);
    }

    // Keeps the values of a tick with the ones of the tick before. position: where a bone's position is in its values,
    // stride: the values per bone
    void Snapshot(Pose& pose, uintptr_t source, size_t count, const size_t (&position)[3], size_t stride)
    {
        pose.seenTick = Tick;
        auto values = (const float*)source;
        if (pose.source != source || pose.current.size() != count)
        {
            pose.source = source;
            pose.current.assign(values, values + count);
            pose.previous = pose.current;
            pose.previousTick = pose.currentTick = Tick;
            return;
        }
        if (memcmp(pose.current.data(), values, count * sizeof(float)) == 0)
            return;
        pose.previous.swap(pose.current);
        pose.current.assign(values, values + count);
        pose.previousTick = pose.currentTick;
        pose.currentTick = Tick;

        // Put somewhere else, or not animated for a while: no blending from where it was. A skeleton's first bone is a
        // reference point that moves in steps of 20 m while the character moves on smoothly. Its children are placed
        // relative to it and step back by the same amount in that tick, so a first bone's step with a child that
        // stays in place in the world isn't a move to somewhere else.
        const auto delta = [&](size_t bone, size_t axis)
        {
            return pose.current[bone + position[axis]] - pose.previous[bone + position[axis]];
        };
        const float rx = delta(0, 0), ry = delta(0, 1), rz = delta(0, 2);
        bool moved = rx * rx + ry * ry + rz * rz > 5.0f * 5.0f;
        for (size_t bone = stride; moved && bone + stride <= count; bone += stride)
        {
            const float x = delta(bone, 0) + rx, y = delta(bone, 1) + ry, z = delta(bone, 2) + rz;
            if (x * x + y * y + z * z < 5.0f * 5.0f)
                moved = false;
        }
        if (moved || pose.currentTick - pose.previousTick > 10)
        {
            pose.previous = pose.current;
            pose.previousTick = pose.currentTick;
        }
    }

    // After the animation of a tick: the new poses are kept with the one before
    void SnapshotPoses()
    {
        std::unique_lock lock(PosesMutex);
        ++Tick;
        auto interpolation = fb::Settings::Field<bool>("ClientSettings", "VisualFrameInterpolation");
        bInterpolation = interpolation && *interpolation;
        ForEachSlot([](uintptr_t skeleton, uintptr_t block, uint32_t index, uintptr_t result)
        {
            auto bones = *(uint32_t*)(skeleton + 0x04);
            auto transformsSize = *(uint32_t*)(skeleton + 0x1C);
            auto matricesSize = *(uint32_t*)(skeleton + 0x20);
            if (!bones || bones > 1024)
                return;

            auto matrices = *(uintptr_t*)(result + 0x08);
            if (matrices && matricesSize == bones * BoneSize && matrices == *(uintptr_t*)(block + 0x08) + index * matricesSize)
                Snapshot(Poses[matrices], matrices, matricesSize / sizeof(float), { 3, 7, 11 }, BoneSize / sizeof(float));

            // Only a slot's own bone transforms of this skeleton: exactly its bone count is read and given back
            auto transforms = *(uintptr_t*)(result + 0x14);
            if (!transforms || transformsSize != bones * TransformSize)
                return;
            for (auto buffer : { 0x00, 0x04, 0x0C, 0x10 })
            {
                if (transforms == *(uintptr_t*)(block + buffer) + index * transformsSize)
                {
                    Snapshot(Transforms[result], transforms, transformsSize / sizeof(float), { 0, 1, 2 }, TransformSize / sizeof(float));
                    break;
                }
            }
        });

        // Skeletons and objects that are gone
        std::erase_if(Poses, [](const auto& entry) { return Tick - entry.second.seenTick > 30; });
        std::erase_if(Transforms, [](const auto& entry) { return Tick - entry.second.seenTick > 30; });
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

    // Bone matrices: a straight line between the two tick poses
    void BlendMatrices(const Pose& pose, float f, float* out)
    {
        auto a = pose.previous.data();
        auto b = pose.current.data();
        for (size_t i = 0, n = pose.current.size(); i < n; ++i)
            out[i] = a[i] + (b[i] - a[i]) * f;
    }

    // Bone transforms: the position and scale in a line, the rotation along the shorter way
    void BlendTransforms(const Pose& pose, float f, float* out)
    {
        for (size_t bone = 0, n = pose.current.size(); bone < n; bone += TransformSize / sizeof(float))
        {
            auto a = &pose.previous[bone];
            auto b = &pose.current[bone];
            auto o = &out[bone];
            for (int i = 0; i < 4; ++i)
                o[i] = a[i] + (b[i] - a[i]) * f;
            float sign = (a[4] * b[4] + a[5] * b[5] + a[6] * b[6] + a[7] * b[7]) < 0.0f ? -1.0f : 1.0f;
            float length = 0.0f;
            for (int i = 4; i < 8; ++i)
            {
                o[i] = a[i] + (b[i] * sign - a[i]) * f;
                length += o[i] * o[i];
            }
            length = std::sqrt(length);
            if (length > 0.0f)
                for (int i = 4; i < 8; ++i)
                    o[i] /= length;
        }
    }

    // The values between the last two ticks for this frame, or the game's own when there's nothing to blend
    template<typename Blend>
    uintptr_t Blended(std::unordered_map<uintptr_t, Pose>& poses, uintptr_t key, uintptr_t values, Blend&& blend)
    {
        std::shared_lock lock(PosesMutex);
        auto it = poses.find(key);
        if (it == poses.end())
            return values;
        auto& pose = it->second;
        if (pose.source != values || pose.currentTick == pose.previousTick || pose.previous.size() != pose.current.size())
            return values;

        std::lock_guard blendLock(pose.blendMutex);
        if (pose.blendedFrame != Frame || pose.blended.size() != pose.current.size())
        {
            pose.blended.resize(pose.current.size());
            blend(pose, BlendFactor(pose), pose.blended.data());
            pose.blendedFrame = Frame;
        }
        return (uintptr_t)pose.blended.data();
    }

    // The bone matrices of an animated object, for the renderer (GameAnimatable)
    SafetyHookInline shGetBones = {};
    uintptr_t __fastcall GetBones(void* animatable, void*)
    {
        auto bones = shGetBones.unsafe_thiscall<uintptr_t>(animatable);
        if (!bones || bInTick || !bInterpolation)
            return bones;
        return Blended(Poses, bones, bones, BlendMatrices);
    }

    // The bone transforms of an animated object (GameAnimatable): +0xB4 -> the animation
    SafetyHookInline shGetBoneTransforms = {};
    uintptr_t __fastcall GetBoneTransforms(void* animatable, void*)
    {
        auto transforms = shGetBoneTransforms.unsafe_thiscall<uintptr_t>(animatable);
        if (!transforms || bInTick || !bInterpolation)
            return transforms;
        auto animation = *(uintptr_t*)((uintptr_t)animatable + 0xB4);
        if (!animation)
            return transforms;
        return Blended(Transforms, animation + ResultInAnimatable, transforms, BlendTransforms);
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
            auto getTransforms = hook::pattern("83 B9 88 01 00 00 02 75 08 8A 81 44 02 00 00 EB 0D 8B 81 B4 00 00 00 85 C0 74 11 8A 40 40 84 C0 74 0A 8B 81 B4 00 00 00 8B 40 30 C3");

            pAnimationManager = *tick.get_first<uintptr_t*>(13);

            // A tick advances the animation by a whole tick, not up to the frame's place between the ticks
            injector::MakeNOP(tickDt.get_first(0x16), 4, true);     // subsd: 1 - previous place
            injector::MakeNOP(tickDt.get_first(0x22), 4, true);     // addsd: + place

            shTickAnimation = safetyhook::create_inline(tick.get_first(), TickAnimation);
            shFrameAnimation = safetyhook::create_inline(frame.get_first(), FrameAnimation);
            shGetBones = safetyhook::create_inline(getBones.get_first(), GetBones);
            shGetBoneTransforms = safetyhook::create_inline(getTransforms.get_first(), GetBoneTransforms);
        };
    }
} Characters;
