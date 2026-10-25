module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "../Shared/PatternCheck.hpp"

export module AimWalk;

// Walking legs for actions that keep the character planted: aiming and firing, the knife, reloading and switching
// weapons (with MAIN.ModernControls).
//
// The character model mixes eight motion slots; a later slot overrides the earlier ones per joint by the joint weights
// in its pose buffer, scaled by the slot's weight. The weapon set's own walk clips (forward, back, strafe left and
// right) play in the last slot, and before every blend the weights of all joints above the hips are cleared, so the
// upper body keeps the aim, shot, reload or switch while the legs walk. The clip's root motion moves the character
// (the game's collision keeps it out of walls); each step is turned to the stick's direction.
// Not in online sessions: the partner's game would not see the legs or the movement in these actions.

namespace
{
    constexpr float Pi = 3.14159265f;

    constexpr uint32_t ChPos = 0x30;
    constexpr uint32_t ChJointCount = 0x314;
    constexpr uint32_t ChJoints = 0x318;
    constexpr uint32_t ChSlots = 0x380;
    constexpr uint32_t ChActionId = 0x10E0;
    constexpr uint32_t ChStateFlags = 0x2D8C;
    constexpr uint32_t ChSlot = 0x2D9C;

    constexpr uint32_t SlotSize = 0xE0;
    constexpr uint32_t SlotMotion = 0x04;
    constexpr uint32_t SlotWeight = 0x14;
    constexpr uint32_t SlotFrame = 0x18;
    constexpr uint32_t SlotFrames = 0x1C;
    constexpr uint32_t SlotSpeed = 0x24;
    constexpr uint32_t SlotNextFrame = 0x2C;
    constexpr uint32_t SlotPose = 0xD8;

    constexpr uint32_t JointSize = 0x90;
    constexpr uint32_t PoseJointSize = 0x60;

    constexpr uint32_t LegSlot = 7;

    SafetyHookInline SetMotionHook;
    bool SetMotion(uintptr_t model, void*, uint32_t slot, uint32_t motion, float inter, float start, float speed, uint32_t flags)
    {
        return SetMotionHook.unsafe_thiscall<bool>(model, slot, motion, inter, start, speed, flags);
    }
    uintptr_t* pSession = nullptr;
    uintptr_t* pPlayers = nullptr; // player manager: the four player characters at +0x24

    // Clips of a weapon set: forward, left, back, right
    constexpr uint8_t AxisClip[4] = { 0x01, 0x0C, 0x07, 0x0D };
    constexpr uint8_t RunClip = 0x04;

    struct Legs
    {
        uintptr_t ch = 0;
        bool active = false;
        int axis = -1;
        uint16_t clip = 0xFFFF;
        uint16_t savedMotion = 0xFFFF;
        float savedFrame = 0.0f;
        float direction = 0.0f;
        float magnitude = 0.0f;
        float lastPos[3] = {};
        bool havePos = false;
        uint32_t jointCount = 0;
        uint8_t upper[256] = {};
        double lowSince = -1.0;
        double lastStick = 0.0;     // the last update from the command builder
        // The game's last requests for the stance slots (3-5) while the legs walk, put back when they stop
        struct Request { bool pending = false; uint32_t motion = 0; float inter = 0.0f, start = 0.0f, speed = 1.0f; uint32_t flags = 0; };
        Request stance[3];
    };
    Legs LegsBySlot[4];

    template<typename T> T& Field(uintptr_t base, uint32_t offset) { return *reinterpret_cast<T*>(base + offset); }

    double Now()
    {
        static LARGE_INTEGER freq{};
        if (!freq.QuadPart)
            QueryPerformanceFrequency(&freq);
        LARGE_INTEGER c;
        QueryPerformanceCounter(&c);
        return double(c.QuadPart) / double(freq.QuadPart);
    }

    float Wrap(float a)
    {
        return std::remainder(a, 2.0f * Pi);
    }

    bool Online()
    {
        auto session = pSession ? *pSession : 0;
        auto state = session ? Field<uintptr_t>(session, 0x1042C) : 0;
        return state && Field<uint8_t>(state, 0x56C) != 0;
    }

    uintptr_t SlotAt(uintptr_t ch, uint32_t slot) { return ch + ChSlots + slot * SlotSize; }

    uint32_t JointCount(uintptr_t ch)
    {
        return std::min<uint32_t>(Field<uint32_t>(ch, ChJointCount), 256);
    }

    bool InPlayerList(uintptr_t ch)
    {
        auto players = pPlayers ? *pPlayers : 0;
        if (!ch || !players)
            return false;
        for (int i = 0; i < 4; i++)
        {
            if (Field<uintptr_t>(players, 0x24 + i * 4) == ch)
                return true;
        }
        return false;
    }

    // Actions that keep the legs still: weapon ready/loop/fire/reload/end, weapon switch, the knife states
    bool LegAction(uint32_t action)
    {
        return action >= 0x1B && action <= 0x27;
    }

    // Joints above the hips: everything that isn't the pelvis (joint id 0x0F), one of its descendants or the root
    void BuildMask(Legs& l)
    {
        auto count = JointCount(l.ch);
        auto joints = Field<uintptr_t>(l.ch, ChJoints);
        l.jointCount = 0;
        if (!joints || !count)
            return;
        int pelvis = -1;
        for (uint32_t j = 0; j < count; j++)
        {
            if (Field<uint8_t>(joints, j * JointSize + 7) == 0x0F)
            {
                pelvis = int(j);
                break;
            }
        }
        if (pelvis < 0)
            return;
        for (uint32_t j = 0; j < count; j++)
        {
            bool lower = false;
            uint32_t k = j;
            for (int depth = 0; depth < 64 && k < count; depth++)
            {
                if (int(k) == pelvis)
                {
                    lower = true;
                    break;
                }
                auto parent = Field<uint8_t>(joints, k * JointSize + 5);
                if (parent == 0xFF || parent == k)
                    break;
                k = parent;
            }
            l.upper[j] = lower ? 0 : 1;
        }
        l.jointCount = count;
    }

    void Stop(Legs& l)
    {
        if (!l.active)
            return;
        l.active = false;
        l.havePos = false;
        l.axis = -1;
        SetMotion(l.ch, nullptr, LegSlot, l.savedMotion, 8.0f, l.savedMotion != 0xFFFF ? l.savedFrame : 0.0f, 1.0f, 0);
        l.clip = 0xFFFF;
        for (uint32_t i = 0; i < 3; i++)
        {
            auto& r = l.stance[i];
            if (r.pending)
                SetMotion(l.ch, nullptr, 3 + i, r.motion, 8.0f, r.start, r.speed, r.flags);
            r.pending = false;
        }
    }

    // The legs of a character the command builder stopped updating (an event took over, death, a level change) end:
    // put back as usual while it is still a player, dropped untouched when it is gone (its memory may be reused)
    bool Current(Legs& l)
    {
        if (!l.active)
            return false;
        bool player = InPlayerList(l.ch) && JointCount(l.ch) == l.jointCount;
        if (player && Now() - l.lastStick < 0.2)
            return true;
        if (player)
            Stop(l);
        else
            l = Legs{};
        return false;
    }

    bool Play(Legs& l, uint16_t clip, float rate)
    {
        auto slot = SlotAt(l.ch, LegSlot);
        if (!l.active)
        {
            BuildMask(l);
            if (!l.jointCount)
                return false;
            l.savedMotion = Field<uint16_t>(slot, SlotMotion);
            l.savedFrame = Field<float>(slot, SlotFrame);
            l.active = true;
            l.havePos = false;
        }
        if (l.clip != clip || Field<uint16_t>(slot, SlotMotion) != clip)
        {
            // A change of direction goes on at the same point of the step
            float phase = -1.0f;
            if (l.clip != 0xFFFF && Field<uint16_t>(slot, SlotMotion) == l.clip && Field<float>(slot, SlotFrames) > 1.0f)
                phase = Field<float>(slot, SlotFrame) / Field<float>(slot, SlotFrames);
            if (!SetMotion(l.ch, nullptr, LegSlot, clip, 8.0f, 0.0f, rate, 0))
            {
                Stop(l);
                return false;
            }
            if (phase >= 0.0f && Field<float>(slot, SlotFrames) > 1.0f)
            {
                float frame = phase * Field<float>(slot, SlotFrames);
                Field<float>(slot, SlotFrame) = frame;
                Field<float>(slot, SlotNextFrame) = frame;
            }
            l.clip = clip;
        }
        Field<float>(slot, SlotSpeed) = rate;
        Field<float>(slot, SlotWeight) = 1.0f;
        return true;
    }

    // While the legs walk, the game's own requests for the last slot (small accents on action changes) and the
    // lower-body stance slots (restarted at every aim turn and shot) would restart the legs: they wait until the legs
    // stop
    bool __fastcall SetMotionDetour(uintptr_t model, void* edx, uint32_t slot, uint32_t motion, float inter, float start, float speed, uint32_t flags)
    {
        if (slot == LegSlot || (slot >= 3 && slot <= 5))
        {
            for (auto& l : LegsBySlot)
            {
                if (!l.active || l.ch != model || !Current(l))
                    continue;
                if (slot == LegSlot)
                {
                    l.savedMotion = uint16_t(motion);
                    l.savedFrame = start;
                }
                else
                    l.stance[slot - 3] = { true, motion, inter, start, speed, flags };
                return true;
            }
        }
        return SetMotionHook.unsafe_thiscall<bool>(model, slot, motion, inter, start, speed, flags);
    }

    // Before each blend of a walking character: only the joints at and below the hips take the walk clip
    SafetyHookInline BlendHook;
    void __fastcall Blend(uintptr_t model, void* edx)
    {
        for (auto& l : LegsBySlot)
        {
            if (!l.active || l.ch != model || !Current(l))
                continue;
            auto pose = Field<uintptr_t>(SlotAt(model, LegSlot), SlotPose);
            if (pose && Field<uint16_t>(SlotAt(model, LegSlot), SlotMotion) == l.clip)
            {
                for (uint32_t j = 0; j < l.jointCount; j++)
                {
                    if (!l.upper[j])
                        continue;
                    Field<float>(pose, j * PoseJointSize + 0x0C) = 0.0f;
                    Field<float>(pose, j * PoseJointSize + 0x1C) = 0.0f;
                    Field<float>(pose, j * PoseJointSize + 0x2C) = 0.0f;
                }
            }
            break;
        }
        BlendHook.unsafe_thiscall(model);
    }
}

export namespace AimWalk
{
    // The legs of this character are walking
    bool Walking(uintptr_t ch)
    {
        auto index = ch ? Field<uint8_t>(ch, ChSlot) : 0xFF;
        return index <= 3 && LegsBySlot[index].active && LegsBySlot[index].ch == ch;
    }

    // Every frame for each local character under the modern controls: the stick in the character's frame (radians,
    // 0 = ahead, + = left) and its magnitude (0 = let go)
    void SetStick(uintptr_t ch, float direction, float magnitude, bool run = false)
    {
        if (!SetMotionHook || !ch)
            return;
        auto index = Field<uint8_t>(ch, ChSlot);
        if (index > 3)
            return;
        auto& l = LegsBySlot[index];
        if (l.ch != ch)
        {
            l = Legs{};
            l.ch = ch;
        }
        double now = Now();
        l.lastStick = now;

        auto action = Field<uint32_t>(ch, ChActionId);
        bool allowed = LegAction(action) && !(Field<uint32_t>(ch, ChStateFlags) & 0x4F00) && !Online();
        if (!allowed)
        {
            Stop(l);
            return;
        }
        if (magnitude < (l.active ? 0.22f : 0.3f))
        {
            if (!l.active)
                return;
            if (l.lowSince < 0.0)
                l.lowSince = now;
            if (now - l.lowSince > 0.1)
            {
                Stop(l);
                return;
            }
            direction = l.direction;
            magnitude = l.magnitude;
        }
        else
            l.lowSince = -1.0;
        l.direction = direction;
        l.magnitude = std::min(magnitude, 1.0f);

        // Nearest axis, with some hysteresis at the diagonals
        static constexpr float AxisAngle[4] = { 0.0f, Pi * 0.5f, Pi, -Pi * 0.5f };
        if (l.axis < 0 || std::fabs(Wrap(direction - AxisAngle[l.axis])) > Pi * 0.3f)
        {
            int best = 0;
            for (int i = 1; i < 4; i++)
                if (std::fabs(Wrap(direction - AxisAngle[i])) < std::fabs(Wrap(direction - AxisAngle[best])))
                    best = i;
            l.axis = best;
        }
        uint16_t stance = Field<uint16_t>(SlotAt(ch, 0), SlotMotion);
        uint16_t bank = (stance == 0xFFFF || action == 0x20 || action == 0x21) ? 0x05 : (stance >> 8);
        // Running (a reload or switch on the run): the weapon's run clip, straight ahead
        bool running = run && l.axis == 0;
        float rate = running ? 1.0f : 0.6f + 0.4f * l.magnitude;
        if (!Play(l, uint16_t((bank << 8) | (running ? RunClip : AxisClip[l.axis])), rate))
            return;

        // Turn the last step toward the stick's direction
        auto pos = reinterpret_cast<float*>(ch + ChPos);
        if (l.havePos)
        {
            float dx = pos[0] - l.lastPos[0], dz = pos[2] - l.lastPos[2];
            float len = std::sqrt(dx * dx + dz * dz);
            if (len > 0.01f && len < 50.0f)
            {
                float turn = Wrap(direction - AxisAngle[l.axis]);
                float c = std::cos(turn), s = std::sin(turn);
                pos[0] = l.lastPos[0] + dx * c + dz * s;
                pos[2] = l.lastPos[2] - dx * s + dz * c;
            }
        }
        std::memcpy(l.lastPos, pos, sizeof(l.lastPos));
        l.havePos = true;
    }

    void Init()
    {
        // The online check and the player list are needed: without them no legs
        auto session = hook::pattern("33 FF 8B F1 89 BE EC 2D 00 00 A1 ? ? ? ?");
        auto players = hook::pattern("8B 44 24 08 8B 0D ? ? ? ? 56 50 E8 ? ? ? ? 8B F0 85 F6 74 ? 8B 86 64 2D 00 00");
        auto pattern = hook::pattern("8B 54 24 04 53 8B D9 3B 93 74 03 00 00 72 06 32 C0 5B C2 18 00");
        if (pattern.size() != 1 || !PatternAvailable(session, "ResidentEvil5.FusionFix: AimWalk") || !PatternAvailable(players, "ResidentEvil5.FusionFix: AimWalk"))
            return;
        pSession = *session.get_first<uintptr_t*>(11);
        pPlayers = *players.get_first<uintptr_t*>(6);
        SetMotionHook = safetyhook::create_inline(pattern.get_first(), SetMotionDetour);

        pattern = hook::pattern("55 8B EC 83 E4 F0 81 EC D4 01 00 00 53 56 57 8B D9 E8");
        if (PatternUnique(pattern, "ResidentEvil5.FusionFix: AimWalk"))
            BlendHook = safetyhook::create_inline(pattern.get_first(), Blend);
    }
}
