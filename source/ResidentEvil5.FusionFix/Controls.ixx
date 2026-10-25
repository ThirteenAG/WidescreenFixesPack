module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "../Shared/PatternCheck.hpp"

export module Controls;

import AimWalk;

// MAIN.ModernControls (live): a free over-the-shoulder camera and movement relative to it, for the local players.
//
// - Camera: the gameplay camera already orbits the character by its look offsets (QFPS +0x1D0 vertical pose blend,
//   +0x1D4 yaw around the character) for the game's own short look-around. Here the look-around is replaced: the
//   right stick or the mouse turn a camera yaw kept in the character's parent space, and the look yaw is that minus
//   the character's facing every frame. The rig, its wall handling and the controller's smoothing stay the game's.
//   Aiming, looking at the partner and scripted cameras keep the game's own behaviour.
// - Movement: the character's command builder gets the stick straight ahead while the character is steered toward
//   the stick's direction seen from the camera, so walking, running, stopping and their animations stay the game's.
// - Aiming turns the character to the camera first. Pulling toward the camera with A (keyboard: the quick turn key)
//   turns round with the game's own quick turn; the camera swings behind.
// - Aiming with the stick held to a side or back and A (keyboard: quick turn key) dodges with the game's avoid
//   actions (the QTE dodges).
// - Reload with one button outside the aim (pad: X when no X prompt is up; keyboard: the game's reload key already).
// - The legs walk while aiming, using the knife, reloading or switching weapons (AimWalk).
// In online co-op the game sends action changes and the position/facing heartbeats by itself, so the partner sees the
// result; the camera is local.

namespace
{
    constexpr float Pi = 3.14159265f;
    constexpr float DegToRad = Pi / 180.0f;

    // Character (uPlayer) fields
    constexpr uint32_t ChQuat = 0x40;
    constexpr uint32_t ChActionId = 0x10E0;
    constexpr uint32_t ChStateFlags = 0x2D8C;
    constexpr uint32_t ChSlot = 0x2D9C;
    constexpr uint32_t ChNetControl = 0x2DA8; // 0 local, 1 AI, 2 network partner

    // Command builder (cPadEx, controller + 0x220) and its PlayerPad (+0x4E8)
    constexpr uint32_t PadExPlayerPad = 0x4E8;
    constexpr uint32_t CmdTurn180 = 0x0D;
    constexpr uint32_t CmdReload = 0x23;
    constexpr uint32_t PadIndex = 0x10;
    constexpr uint32_t PadOn = 0x34;
    constexpr uint32_t PadTrg = 0x3C;
    constexpr uint32_t PadRAngle = 0x1D0;
    constexpr uint32_t PadRMag = 0x1D8;
    constexpr uint32_t PadMouseTurnX = 0x200;
    constexpr uint32_t PadMouseTurnY = 0x204;
    constexpr uint32_t PadMouseX = 0x20C;
    constexpr uint32_t PadMouseY = 0x210;

    // Canonical buttons in the PlayerPad (the game's layout after the control type remap)
    constexpr uint32_t BtnReady = 0x800;
    constexpr uint32_t BtnKnife = 0x400;
    constexpr uint32_t BtnRun = 0x4000;
    constexpr uint32_t BtnRightStickDirs = 0xF00000;

    // Raw MT pad bits (sPad)
    constexpr uint32_t RawA = 0x4000;
    constexpr uint32_t RawX = 0x8000;

    // QFPS camera (uCameraQFPS)
    constexpr uint32_t CamAimRig = 0x1B1;
    constexpr uint32_t CamLookMode = 0x1B2;
    constexpr uint32_t CamLookAt = 0x1B3;
    constexpr uint32_t CamLookV = 0x1D0;
    constexpr uint32_t CamLookH = 0x1D4;

    constexpr float CameraYawSpeed = 3.0f;   // radians per second at full stick
    constexpr float CameraPitchSpeed = 1.6f; // pose blend per second (+1 looks up)
    constexpr float MouseYawScale = 0.001f; // radians per unit of the game's scaled mouse motion
    constexpr float MousePitchScale = 0.0016f;

    // Actions
    constexpr uint32_t ActTurn180 = 0x09;

    uintptr_t* pSPad = nullptr;
    uintptr_t* pSession = nullptr;
    uintptr_t* pPlayers = nullptr; // player manager: the four player characters at +0x24
    uintptr_t* pActionButtons = nullptr; // context prompts: a manager per player at +0x40 (0x2D0 each)
    using AvoidFn = void(__cdecl*)(int, int, int, int);
    AvoidFn AvoidFront = nullptr, AvoidBack = nullptr, AvoidLeft = nullptr, AvoidRight = nullptr;
    using KeyFn = bool(__stdcall*)(int action, int player);
    KeyFn KeyHeld = nullptr;
    KeyFn KeyPressed = nullptr;

    struct LocalPlayer
    {
        uintptr_t character = 0;
        float cameraYaw = 0.0f;   // world yaw the camera looks along, in the character's parent space
        float cameraPitch = 0.0f; // the rig's vertical pose blend, -1..1
        bool cameraValid = false;
        float inputYaw = 0.0f;    // camera input since the camera last ran
        float inputPitch = 0.0f;
        double lastCamera = 0.0;
        double lastPad = 0.0;
        double lastSteer = 0.0;
        double swingStart = -1.0;
        float swingFrom = 0.0f, swingTo = 0.0f;
        uint32_t lastAction = 0xFFFFFFFF;
        bool aimHeld = false;
        uint32_t lastRaw = 0;
        bool directionLocked = false; // after a quick turn the held stick keeps its world direction
        float lockedYaw = 0.0f, lockedAngle = 0.0f;
        uint32_t steerAction = 0xFFFFFFFF;
        bool mouseView = false;
        double viewFrame = 0.0;
    };
    LocalPlayer Players[4];

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

    template<typename T> T& Field(uintptr_t base, uint32_t offset) { return *reinterpret_cast<T*>(base + offset); }

    float GetYaw(uintptr_t ch)
    {
        auto q = reinterpret_cast<float*>(ch + ChQuat);
        return 2.0f * std::atan2(q[1], q[3]);
    }

    void SetYaw(uintptr_t ch, float yaw)
    {
        auto q = reinterpret_cast<float*>(ch + ChQuat);
        q[0] = 0.0f;
        q[1] = std::sin(yaw * 0.5f);
        q[2] = 0.0f;
        q[3] = std::cos(yaw * 0.5f);
    }

    bool Enabled()
    {
        static auto& modern = FixSettings().Int("MAIN.ModernControls");
        return modern != 0;
    }

    bool InLocomotion(uint32_t action)
    {
        return action <= 8; // wait, relax, walk, walk_l, walk_r, run, back, turn_l, turn_r
    }

    bool InAim(uint32_t action)
    {
        return (action >= 0x1B && action <= 0x27);
    }

    // Reloading and switching weapons outside the aim: the character moves, turns and runs, and the camera stays free
    bool MoveAction(uint32_t action)
    {
        return action == 0x1F || action == 0x20 || action == 0x21;
    }

    LocalPlayer* PlayerFor(uintptr_t ch)
    {
        auto players = pPlayers ? *pPlayers : 0;
        if (!ch || !players)
            return nullptr;
        bool known = false;
        for (int i = 0; i < 4; i++)
            known |= Field<uintptr_t>(players, 0x24 + i * 4) == ch;
        if (!known || Field<uint32_t>(ch, ChNetControl) != 0)
            return nullptr;
        auto slot = Field<uint8_t>(ch, ChSlot);
        if (slot > 3)
            return nullptr;
        auto& p = Players[slot];
        if (p.character != ch)
        {
            p = LocalPlayer{};
            p.character = ch;
        }
        return &p;
    }

    bool KeyboardPlayer(uintptr_t ch)
    {
        auto spad = *pSPad;
        return spad && Field<uint8_t>(spad, 0x5F0) == 0 && Field<int32_t>(spad, 0x614) == Field<uint8_t>(ch, ChSlot);
    }

    uint32_t RawButtons(uintptr_t pad)
    {
        auto spad = *pSPad;
        auto index = Field<uint32_t>(pad, PadIndex);
        if (!spad || index > 3 || Field<uint8_t>(spad, 0x5DC))
            return 0;
        return Field<uint32_t>(spad, 0x190 + index * 0x2C0);
    }

    // The player's camera invert option
    int CameraInvert(uintptr_t ch)
    {
        auto session = pSession ? *pSession : 0;
        auto options = session ? Field<uintptr_t>(session, 0x1042C) : 0;
        auto slot = Field<uint8_t>(ch, ChSlot);
        return options && slot < 4 ? Field<int32_t>(options, (61 * slot + 39) * 4) : 0;
    }

    // A context prompt (door, item, finisher...) that the given raw buttons answer is up for this player
    bool PromptUp(uintptr_t ch, uint32_t buttons)
    {
        auto prompts = *pActionButtons;
        auto slot = Field<uint8_t>(ch, ChSlot);
        if (!prompts || slot > 3)
            return false;
        auto mgr = prompts + 0x40 + slot * 0x2D0;
        for (int i = 0; i < 2; i++)
        {
            auto s = mgr + 0x230 + i * 0x48;
            if (Field<uint8_t>(s, 4) && Field<uint8_t>(s, 5) && (Field<uint32_t>(s, 0x18) & buttons))
                return true;
        }
        return false;
    }

    // The character faces the camera's way when the aim begins
    void TurnToCamera(LocalPlayer& p)
    {
        if (p.cameraValid)
            SetYaw(p.character, p.cameraYaw);
    }

    // After the pad update: camera input, aim start, quick turn and reload buttons
    void OnPadUpdated(uintptr_t padEx, uintptr_t ch)
    {
        if (!Enabled())
            return;
        auto p = PlayerFor(ch);
        if (!p)
            return;
        auto pad = padEx + PadExPlayerPad;
        auto action = Field<uint32_t>(ch, ChActionId);
        bool keyboard = KeyboardPlayer(ch);
        bool aimHeld = (Field<uint32_t>(pad, PadOn) & (BtnReady | BtnKnife)) != 0 || (keyboard && KeyHeld(12, Field<uint8_t>(ch, ChSlot)));
        bool aiming = aimHeld || (InAim(action) && !MoveAction(action));
        double now = Now();
        float dt = p->lastPad > 0.0 ? float(std::min(now - p->lastPad, 0.1)) : 0.0f;
        p->lastPad = now;

        // Camera input, with the game's camera invert option (1 vertical, 2 horizontal, 3 both)
        if (!aiming)
        {
            int invert = CameraInvert(ch);
            float signX = (invert & 2) ? -1.0f : 1.0f;
            float signY = (invert & 1) ? -1.0f : 1.0f;
            if (keyboard)
            {
                p->inputYaw -= Field<float>(pad, PadMouseX) * MouseYawScale * signX;
                p->inputPitch -= Field<float>(pad, PadMouseY) * MousePitchScale * signY;
            }
            else
            {
                float a = Field<float>(pad, PadRAngle) * DegToRad;
                float m = Field<float>(pad, PadRMag);
                p->inputYaw += std::sin(a) * m * CameraYawSpeed * dt * signX;
                p->inputPitch += std::cos(a) * m * CameraPitchSpeed * dt * signY;
            }
            Field<float>(pad, PadRAngle) = 0.0f;
            Field<float>(pad, PadRMag) = 0.0f;
            Field<uint32_t>(pad, PadOn) &= ~BtnRightStickDirs;
            Field<uint32_t>(pad, PadTrg) &= ~BtnRightStickDirs;
            Field<float>(pad, PadMouseTurnX) = 0.0f;
            Field<float>(pad, PadMouseTurnY) = 0.0f;
            Field<float>(pad, PadMouseX) = 0.0f;
            Field<float>(pad, PadMouseY) = 0.0f;
        }

        if (aimHeld && !p->aimHeld && (InLocomotion(action) || action == 0x1D || action == 0x24 || action == ActTurn180))
            TurnToCamera(*p);
        p->aimHeld = aimHeld;
    }

    // Final stick values of the command builder: steer toward the stick's direction seen from the camera
    void OnSticks(uintptr_t padEx, uintptr_t ch, float& angle, float& mag)
    {
        if (!Enabled())
        {
            if (AimWalk::Walking(ch))
                AimWalk::SetStick(ch, 0.0f, 0.0f); // switched off mid-walk: the legs stop
            return;
        }
        auto p = PlayerFor(ch);
        if (!p)
            return;
        if (!p->cameraValid)
        {
            AimWalk::SetStick(ch, 0.0f, 0.0f);
            return;
        }
        auto action = Field<uint32_t>(ch, ChActionId);
        auto pad = padEx + PadExPlayerPad;
        double now = Now();
        float dt = p->lastSteer > 0.0 ? float(std::min(now - p->lastSteer, 0.1)) : 0.0f;
        p->lastSteer = now;

        bool keyboard = KeyboardPlayer(ch);
        uint32_t raw = keyboard ? 0 : RawButtons(pad);
        uint32_t rawTrg = raw & ~p->lastRaw;
        p->lastRaw = raw;
        float yaw = GetYaw(ch);
        float wanted = Wrap(p->cameraYaw + angle * DegToRad);
        bool moving = mag > 0.3f;
        if (p->directionLocked)
        {
            if (moving && std::fabs(Wrap((angle - p->lockedAngle) * DegToRad)) < 30.0f * DegToRad)
                wanted = p->lockedYaw;
            else
                p->directionLocked = false;
        }
        else if (action == ActTurn180 && p->steerAction != ActTurn180 && moving)
        {
            // The keyboard's quick turn key: the game turns round and the held keys go on the new way
            p->directionLocked = true;
            p->lockedYaw = Wrap(yaw + Pi);
            p->lockedAngle = angle;
            wanted = p->lockedYaw;
        }
        p->steerAction = action;

        if (MoveAction(action) && !p->aimHeld)
        {
            if (moving)
            {
                float rel = Wrap(wanted - yaw);
                float rate = 540.0f * DegToRad * dt;
                SetYaw(ch, Wrap(yaw + std::clamp(rel, -rate, rate)));
            }
            int slot = Field<uint8_t>(ch, ChSlot);
            bool run = keyboard ? KeyHeld(4, slot) : (Field<uint32_t>(pad, PadOn) & BtnRun) != 0;
            AimWalk::SetStick(ch, 0.0f, moving ? mag : 0.0f, run);
            mag = 0.0f;
            return;
        }

        if (InAim(action))
        {
            // Dodges: aim + A with the stick held (keyboard: aim + quick turn key with a direction)
            bool dodge = moving && (keyboard ? KeyPressed(6, Field<uint8_t>(ch, ChSlot)) : (rawTrg & RawA) != 0);
            if (dodge)
            {
                float rel = Wrap(wanted - yaw);
                int slot = Field<uint8_t>(ch, ChSlot);
                if (std::fabs(rel) < Pi * 0.25f) { if (AvoidFront) AvoidFront(0, slot, 0, 0); }
                else if (std::fabs(rel) > Pi * 0.75f) { if (AvoidBack) AvoidBack(0, slot, 0, 0); }
                else if (rel > 0.0f) { if (AvoidLeft) AvoidLeft(0, slot, 0, 0); }
                else { if (AvoidRight) AvoidRight(0, slot, 0, 0); }
            }
            AimWalk::SetStick(ch, Wrap(wanted - yaw), moving ? mag : 0.0f);
            mag = 0.0f;
            return;
        }

        // One-button reload outside the aim (the keyboard's reload key already works there)
        if ((rawTrg & RawX) && !PromptUp(ch, RawX) && (InLocomotion(action) || action == 0x20 || action == 0x21))
            Field<uint8_t>(padEx, CmdReload) = 1;

        if (!InLocomotion(action))
        {
            AimWalk::SetStick(ch, Wrap(wanted - yaw), moving ? mag : 0.0f);
            return;
        }
        AimWalk::SetStick(ch, 0.0f, 0.0f);

        if (!moving)
            return;

        // Quick turn: A with the stick toward the camera while facing away from it
        float rel = Wrap(wanted - yaw);
        if (!keyboard && (rawTrg & RawA) && std::fabs(rel) > Pi * 0.75f)
        {
            p->directionLocked = true;
            p->lockedYaw = wanted;
            p->lockedAngle = angle;
            Field<uint8_t>(padEx, CmdTurn180) = 1;
            angle = 180.0f;
            return;
        }

        // Steer, faster from standing
        float rate = (action == 5 ? 540.0f : 720.0f) * DegToRad * dt;
        float step = std::clamp(rel, -rate, rate);
        SetYaw(ch, Wrap(yaw + step));
        angle = 0.0f;
    }

    // Replaces the game's look-around while the camera is the normal gameplay one
    SafetyHookInline LookAroundHook;
    void __fastcall LookAround(uintptr_t camera, void* edx, uintptr_t ch)
    {
        auto p = Enabled() ? PlayerFor(ch) : nullptr;
        bool normal = p && Field<uint8_t>(camera, CamLookMode) && !Field<uint8_t>(camera, CamAimRig) && !Field<uint8_t>(camera, CamLookAt);
        if (!normal)
        {
            LookAroundHook.unsafe_thiscall(camera, ch);
            if (p)
            {
                // Follow the game's view, so the free camera starts where the game's camera was
                p->cameraYaw = Wrap(GetYaw(ch) + Field<float>(camera, CamLookH));
                p->cameraPitch = 0.0f;
                p->cameraValid = true;
                p->inputYaw = p->inputPitch = 0.0f;
                p->swingStart = -1.0;
            }
            return;
        }

        double now = Now();
        if (!p->cameraValid || now - p->lastCamera > 0.5)
        {
            p->cameraYaw = Wrap(GetYaw(ch) + Field<float>(camera, CamLookH));
            p->cameraPitch = std::clamp(Field<float>(camera, CamLookV), -1.0f, 1.0f);
            p->cameraValid = true;
        }
        p->lastCamera = now;

        // The camera swings behind the character during a quick turn
        auto action = Field<uint32_t>(ch, ChActionId);
        if (action == ActTurn180 && p->lastAction != ActTurn180)
        {
            p->swingStart = now;
            p->swingFrom = p->cameraYaw;
            p->swingTo = Wrap(GetYaw(ch) + Pi);
        }
        p->lastAction = action;
        if (p->swingStart >= 0.0)
        {
            float t = float((now - p->swingStart) / 0.35);
            if (t >= 1.0f)
            {
                t = 1.0f;
                p->swingStart = -1.0;
            }
            t = t * t * (3.0f - 2.0f * t);
            p->cameraYaw = Wrap(p->swingFrom + Wrap(p->swingTo - p->swingFrom) * t);
        }

        p->cameraYaw = Wrap(p->cameraYaw + p->inputYaw);
        p->cameraPitch = std::clamp(p->cameraPitch + p->inputPitch, -1.0f, 1.0f);
        p->inputYaw = p->inputPitch = 0.0f;

        Field<float>(camera, CamLookV) = p->cameraPitch;
        Field<float>(camera, CamLookH) = Wrap(p->cameraYaw - GetYaw(ch));
        p->mouseView = KeyboardPlayer(ch);
        p->viewFrame = now;
    }

    // The camera controller (the gameplay camera sits in it at +0xE00) trails the camera's view with a spring. With the
    // mouse turning the free camera the controller's view is the camera's own, so the view turns by the full mouse
    // movement in the same frame
    SafetyHookInline ControllerHook;
    void __fastcall ControllerUpdate(uintptr_t controller, void* edx, float dt)
    {
        auto camera = controller + 0xE00;
        // A reload or weapon switch outside the aim raises the gun with the aim camera: the free camera stays
        if (Enabled() && Field<uintptr_t>(controller, 0x1B0) == camera)
        {
            auto player = PlayerFor(Field<uintptr_t>(camera, 0x140));
            if (player && player->cameraValid && !player->aimHeld && MoveAction(Field<uint32_t>(player->character, ChActionId)))
                Field<uint8_t>(camera, CamAimRig) = 0;
        }
        ControllerHook.unsafe_thiscall(controller, dt);
        if (!Enabled() || Field<uintptr_t>(controller, 0x1B0) != camera)
            return;
        auto ch = Field<uintptr_t>(camera, 0x140);
        auto p = PlayerFor(ch);
        if (!p || !p->mouseView || Now() - p->viewFrame > 0.1 || !Field<uint8_t>(camera, CamLookMode) || Field<uint8_t>(camera, CamAimRig) || Field<uint8_t>(camera, CamLookAt))
            return;
        std::memcpy(reinterpret_cast<void*>(controller + 0x30), reinterpret_cast<void*>(camera + 0x170), 12); // eye
        std::memcpy(reinterpret_cast<void*>(controller + 0x40), reinterpret_cast<void*>(camera + 0x180), 12); // up
        std::memcpy(reinterpret_cast<void*>(controller + 0x50), reinterpret_cast<void*>(camera + 0x190), 12); // target
    }
}

export namespace Controls
{
    void Init()
    {
        // Every hook below needs these: without them the modern controls and the forced laser stay off
        auto spad = hook::pattern("8B 0D ? ? ? ? 52 F3 0F 11 44 24 44 89 44 24 1C E8");
        auto session = hook::pattern("33 FF 8B F1 89 BE EC 2D 00 00 A1 ? ? ? ?");
        auto players = hook::pattern("8B 44 24 08 8B 0D ? ? ? ? 56 50 E8 ? ? ? ? 8B F0 85 F6 74 ? 8B 86 64 2D 00 00");
        auto actionButtons = hook::pattern("8B 15 ? ? ? ? 80 BA F8 0D 00 00 00");
        auto keyHeld = hook::pattern("6A 00 F3 0F 11 44 24 60 E8 ? ? ? ?");
        auto keyPressed = hook::pattern("8B 0D ? ? ? ? 50 6A 06 E8 ? ? ? ?");
        if (!PatternAvailable(spad, "ResidentEvil5.FusionFix: Controls") || !PatternAvailable(session, "ResidentEvil5.FusionFix: Controls") || !PatternAvailable(players, "ResidentEvil5.FusionFix: Controls") || !PatternAvailable(actionButtons, "ResidentEvil5.FusionFix: Controls") || !PatternAvailable(keyHeld, "ResidentEvil5.FusionFix: Controls") || !PatternAvailable(keyPressed, "ResidentEvil5.FusionFix: Controls"))
            return;
        pSPad = *spad.get_first<uintptr_t*>(2);
        pSession = *session.get_first<uintptr_t*>(11);
        pPlayers = *players.get_first<uintptr_t*>(6);
        pActionButtons = *actionButtons.get_first<uintptr_t*>(2);
        KeyHeld = (KeyFn)injector::GetBranchDestination(keyHeld.get_first(8)).as_int();
        KeyPressed = (KeyFn)injector::GetBranchDestination(keyPressed.get_first(9)).as_int();

        auto avoid = [](uint8_t id) -> AvoidFn
        {
            auto pattern = hook::pattern(std::format("6A 00 6A FF 6A 00 6A 00 6A {:02X} 8B CE FF D0 5F 5E 8B E5 5D C3", id));
            return pattern.size() == 1 ? (AvoidFn)pattern.get_first(-0x105) : nullptr;
        };
        AvoidFront = avoid(0x51);
        AvoidBack = avoid(0x52);
        AvoidLeft = avoid(0x53);
        AvoidRight = avoid(0x54);

        // Command builder: after the PlayerPad update (ebx = builder, esi = character)
        auto pattern = hook::pattern("8D BB E8 04 00 00 8B CF E8 ? ? ? ? F3 0F 10 83 B4 06 00 00");
        if (PatternUnique(pattern, "ResidentEvil5.FusionFix: Controls"))
        {
            static auto PadUpdatedHook = safetyhook::create_mid(pattern.get_first(13), [](SafetyHookContext& regs)
            {
                OnPadUpdated(regs.ebx, regs.esi);
            });
        }

        // Command builder: the final stick values (pad and keyboard joined), before the commands are made
        pattern = hook::pattern("83 BB 0C 07 00 00 00 74 ? C6 83 14 07 00 00 00 83 BE A8 2D 00 00 00");
        if (PatternUnique(pattern, "ResidentEvil5.FusionFix: Controls"))
        {
            static auto SticksHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                auto& angle = *reinterpret_cast<float*>(regs.esp + 0x30);
                auto& mag = *reinterpret_cast<float*>(regs.esp + 0x2C);
                OnSticks(regs.ebx, regs.esi, angle, mag);
            });
        }

        pattern = hook::pattern("55 8B EC 83 E4 F0 83 EC 24 53 56 57 8B F9 80 BF A0 02 00 00 00 0F 84");
        if (PatternUnique(pattern, "ResidentEvil5.FusionFix: Controls"))
            LookAroundHook = safetyhook::create_inline(pattern.get_first(), LookAround);

        pattern = hook::pattern("55 8B EC 83 E4 F0 83 EC 74 53 56 8B F1 80 BE B1 30 00 00 00 57 0F 85");
        if (PatternUnique(pattern, "ResidentEvil5.FusionFix: Controls"))
            ControllerHook = safetyhook::create_inline(pattern.get_first(), ControllerUpdate);

        // MAIN.ForceLaserSight (live): keyboard/mouse aiming looks like the pad's. The aim update decides whether the
        // weapon's laser is drawn (laser +0x2FA; esi = character, edi = laser): drawn for keyboard/mouse too.
        pattern = hook::pattern("E8 ? ? ? ? 88 9F FA 02 00 00 33 DB 33 FF");
        if (PatternUnique(pattern, "ResidentEvil5.FusionFix: Controls"))
        {
            static auto LaserHook = safetyhook::create_mid(pattern.get_first(5), [](SafetyHookContext& regs)
            {
                static auto& forceLaser = FixSettings().Int("MAIN.ForceLaserSight");
                auto ch = regs.esi;
                if (!forceLaser || !PlayerFor(ch) || !KeyboardPlayer(ch))
                    return;
                regs.ebx = (regs.ebx & ~0xFFu) | 1;
            });
        }

        // The keyboard/mouse crosshair: the HUD cockpit (character +0x2640, its character at +0x1A0) shows its reticle
        // when it was asked for one this frame (+0x373), which it copies into its visible flag here (ecx = cockpit).
        // With the laser forced the request is dropped, as with a pad; the sniper scope keeps its reticle.
        pattern = hook::pattern("32 D2 38 91 C3 00 00 00 75 06 88 91 73 03 00 00");
        if (PatternUnique(pattern, "ResidentEvil5.FusionFix: Controls"))
        {
            static auto ReticleHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                static auto& forceLaser = FixSettings().Int("MAIN.ForceLaserSight");
                if (!forceLaser)
                    return;
                auto cockpit = regs.ecx;
                auto ch = Field<uintptr_t>(cockpit, 0x1A0);
                if (!PlayerFor(ch) || Field<uintptr_t>(ch, 0x2640) != cockpit || !KeyboardPlayer(ch) || (Field<uint32_t>(ch, ChStateFlags) & 0x80))
                    return;
                Field<uint8_t>(cockpit, 0x373) = 0;
            });
        }
    }
}
