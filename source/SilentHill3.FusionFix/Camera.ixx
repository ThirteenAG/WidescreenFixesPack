module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/SilentHill/Gamepad.hpp"

export module Camera;

import ComVars;

// The modern camera: a free third-person camera over Heather's right shoulder, turned with the right stick or the
// mouse, in the style of the Silent Hill 2 remake. Movement becomes camera-relative, readying a weapon brings the
// camera closer and turns Heather to where the camera looks, and auto-aim prefers the enemy nearest the centre of
// the screen. The game's own camera keeps running underneath: cutscenes, scripted camera moves, item and death
// cameras, menus and the map all use it as before, and the fix only replaces the final camera matrix during free
// gameplay.
//
// Conventions of the game: 500 units = 1 m, Y points down, angles are radians with yaw 0 looking along +Z,
// matrices are row-major with the translation in the last row.

namespace
{
    constexpr float Pi = 3.14159265f;
    constexpr float Metre = 500.0f;

    // Settings
    const int* CameraMode = nullptr;        // 0 classic, 1 modern
    const float* Sensitivity = nullptr;     // 1..16
    const int* InvertY = nullptr;
    const int* Distance = nullptr;          // 0 near, 1 normal, 2 far
    const float* FieldOfView = nullptr;     // degrees, vertical

    float GameProjection = 0;                   // the game's value while the modern camera zooms

    void(__cdecl* SetCameraMatrix)(int parent, const float* matrix) = nullptr;
    void(__cdecl* SetProjection)(float h) = nullptr;
    int*(__cdecl* SegmentQuery)(int* result, void* ignore, const float* from, const float* to, int mask) = nullptr;

    SafetyHookInline PlayerUpdateHook, ControlTypeHook, TargetHook, FacingHook;
    SafetyHookMid CameraHook, HeadingHook;

    // Camera state
    bool PlayerTicked = false;      // the player update ran since the camera's last frame
    bool Active = false;            // the modern camera drives the view this frame
    bool HaveOrbit = false;
    constexpr float RestPitch = -0.1f;    // slightly down
    float Yaw = 0, Pitch = RestPitch;
    float CurrentDistance = 0, CurrentShoulder = 0, CurrentHeight = 0, CurrentZoom = 1;
    float CollisionDistance = 0;
    float RecentreTime = 0;
    int InactiveFrames = 1000;
    float ForwardX = 0, ForwardZ = 1;   // camera forward on the ground, for aiming

    float Wrap(float a)
    {
        while (a > Pi) a -= 2 * Pi;
        while (a < -Pi) a += 2 * Pi;
        return a;
    }

    float Approach(float current, float target, float rate, float dt)
    {
        return target + (current - target) * std::exp(-rate * dt);
    }

    float* Player()
    {
        return pPlayer ? reinterpret_cast<float*>(pPlayer.get()) : nullptr;
    }

    bool Enabled()
    {
        return CameraMode && *CameraMode == 1;
    }

    // Whether the free camera may run this frame
    bool CanRun()
    {
        if (!Enabled() || !PlayerTicked)
            return false;
        auto player = Player();
        if (!player)
            return false;
        if (nCameraMode != 0 || (nCameraFlags & 3) || nRenderMode == 2 || nEventId != -1 || bPauseActive)
            return false;
        if (*reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(player) + 0x180) <= 0.0f)    // dead
            return false;
        return true;
    }

    // Distance along from->to that is free of walls, as a fraction
    float FreeFraction(const float* from, const float* to)
    {
        int result[10] = {};
        float a[4] = { from[0], from[1], from[2], 1.0f };
        float b[4] = { to[0], to[1], to[2], 1.0f };
        SegmentQuery(result, nullptr, a, b, 0x103E);
        if (result[0] == 0)
            return 1.0f;
        auto hit = reinterpret_cast<const float*>(&result[1]);
        float dx = b[0] - a[0], dy = b[1] - a[1], dz = b[2] - a[2];
        float length = std::sqrt(dx * dx + dy * dy + dz * dz);
        if (length < 1.0f)
            return 1.0f;
        float hx = hit[0] - a[0], hy = hit[1] - a[1], hz = hit[2] - a[2];
        return std::clamp(std::sqrt(hx * hx + hy * hy + hz * hz) / length, 0.0f, 1.0f);
    }

    void BuildMatrix(float pitch, float yaw, const float* eye, float* m)
    {
        float sy = std::sin(yaw), cy = std::cos(yaw), sp = std::sin(pitch), cp = std::cos(pitch);
        float rows[16] =
        {
            cy, 0, -sy, 0,
            sy * sp, cp, cy * sp, 0,
            sy * cp, -sp, cy * cp, 0,
            eye[0], eye[1], eye[2], 1,
        };
        memcpy(m, rows, sizeof(rows));
    }

    void UpdateCamera()
    {
        bool run = CanRun();
        PlayerTicked = false;
        if (!run)
        {
            if (Active)
                SetProjection(GameProjection);
            Active = false;
            SHPad::HideMouseFromGame(false);
            ++InactiveFrames;
            SHPad::TakeMouse();
            return;
        }

        auto player = Player();
        auto bytes = reinterpret_cast<uint8_t*>(player);
        float dt = std::clamp(fPlayerDt.get(), 0.0f, 0.1f);
        float playerYaw = player[5];
        bool aiming = bAiming != 0;

        // Coming back from a cutscene or a long break: start behind Heather
        if (!HaveOrbit || InactiveFrames > 30)
        {
            Yaw = playerYaw;
            Pitch = RestPitch;
            HaveOrbit = true;
            CollisionDistance = 0;
        }
        InactiveFrames = 0;

        // Input
        auto pad = SHPad::Get();
        float speed = 0.8f + 0.2f * (*Sensitivity);                // rad/s at full deflection
        float sx = pad.rx * std::abs(pad.rx), sy = pad.ry * std::abs(pad.ry);
        auto [mouseX, mouseY] = SHPad::TakeMouse();
        float mouseScale = 0.0003f * (*Sensitivity);           // rad per count
        Yaw += sx * speed * dt + float(mouseX) * mouseScale;
        // Stick or mouse down looks down (pitch is positive looking up)
        float pitchInput = sy * speed * 0.75f * dt + float(mouseY) * mouseScale;
        Pitch -= (*InvertY ? -pitchInput : pitchInput);
        Pitch = std::clamp(Pitch, -0.9f, 1.0f);
        Yaw = Wrap(Yaw);

        // Right stick click: back behind Heather
        if (pad.pressed & SHPad::RS)
            RecentreTime = 0.25f;
        if (RecentreTime > 0)
        {
            Yaw = Wrap(Yaw + Wrap(playerYaw - Yaw) * std::min(1.0f, dt / RecentreTime));
            Pitch += (RestPitch - Pitch) * std::min(1.0f, dt / RecentreTime);
            RecentreTime -= dt;
        }

        // Framing
        static const float Distances[] = { 1.3f, 1.75f, 2.4f };
        float targetDistance = (aiming ? 1.0f : Distances[std::clamp(*Distance, 0, 2)]) * Metre;
        float targetShoulder = (aiming ? 0.48f : 0.38f) * Metre;
        float targetHeight = (aiming ? 1.45f : 1.5f) * Metre;
        // The game's view is 2 atan(224 / 448) = 53.13 degrees high; the setting's angle, a little closer when aiming
        float vertical = (*FieldOfView - (aiming ? 10.0f : 0.0f)) * Pi / 180.0f;
        float targetZoom = 0.5f / std::tan(vertical * 0.5f);
        if (CurrentDistance == 0)
        {
            CurrentDistance = targetDistance;
            CurrentShoulder = targetShoulder;
            CurrentHeight = targetHeight;
            CurrentZoom = targetZoom;
        }
        CurrentDistance = Approach(CurrentDistance, targetDistance, 8.0f, dt);
        CurrentShoulder = Approach(CurrentShoulder, targetShoulder, 8.0f, dt);
        CurrentHeight = Approach(CurrentHeight, targetHeight, 8.0f, dt);
        CurrentZoom = Approach(CurrentZoom, targetZoom, 8.0f, dt);

        float sinYaw = std::sin(Yaw), cosYaw = std::cos(Yaw);
        float sinPitch = std::sin(Pitch), cosPitch = std::cos(Pitch);
        float forward[3] = { sinYaw * cosPitch, -sinPitch, cosYaw * cosPitch };
        float right[3] = { cosYaw, 0, -sinYaw };
        ForwardX = sinYaw;
        ForwardZ = cosYaw;

        // Pivot over the shoulder; the shoulder offset fades when looking up or down steeply
        float centre[3] = { player[0], player[1] - CurrentHeight, player[2] };
        float shoulderAmount = CurrentShoulder * (0.6f + 0.4f * cosPitch);
        float shoulder[3] = { centre[0] + right[0] * shoulderAmount, centre[1], centre[2] + right[2] * shoulderAmount };

        // Keep the shoulder point itself out of walls
        float free = FreeFraction(centre, shoulder);
        if (free < 1.0f)
        {
            float keep = std::max(0.0f, free - 0.15f);
            for (int i = 0; i < 3; ++i)
                shoulder[i] = centre[i] + (shoulder[i] - centre[i]) * keep;
        }

        float desired[3];
        for (int i = 0; i < 3; ++i)
            desired[i] = shoulder[i] - forward[i] * CurrentDistance;

        // Wall collision with a few parallel rays around the camera
        constexpr float Radius = 0.12f * Metre;
        float fraction = 1.0f;
        const float offsets[][2] = { { 0, 0 }, { Radius, 0 }, { -Radius, 0 }, { 0, Radius }, { 0, -Radius } };
        for (auto& offset : offsets)
        {
            float a[3], b[3];
            for (int i = 0; i < 3; ++i)
            {
                float side = right[i] * offset[0] + (i == 1 ? offset[1] : 0.0f);
                a[i] = shoulder[i] + side * 0.25f;
                b[i] = desired[i] + side;
            }
            fraction = std::min(fraction, FreeFraction(a, b));
        }
        float allowed = std::max(0.15f * Metre, fraction * CurrentDistance - 0.1f * Metre);
        if (CollisionDistance == 0 || allowed < CollisionDistance)
            CollisionDistance = allowed;    // in front of a wall at once
        else
            CollisionDistance = Approach(CollisionDistance, allowed, 4.0f, dt);     // back out smoothly
        float distance = std::min(CollisionDistance, CurrentDistance);

        float eye[3];
        for (int i = 0; i < 3; ++i)
            eye[i] = shoulder[i] - forward[i] * distance;

        float matrix[16];
        BuildMatrix(Pitch, Yaw, eye, matrix);
        SetCameraMatrix(0, matrix);
        // The zoom is relative to the game's own value, kept while the modern camera runs (the projection has no
        // separate default: scaling the current value would shrink it every frame)
        if (!Active)
            GameProjection = fProjectionDistance;
        SetProjection(GameProjection * CurrentZoom);

        Active = true;
        SHPad::HideMouseFromGame(true);
    }

    // Auto-aim: the game takes the first suitable enemy of its candidate list, which is sorted nearest first.
    // While the modern camera is active the list is ordered by the angle to the camera's view for that pick, so
    // the enemy nearest the centre of the screen wins; enemies far off the centre keep their distance order.
    int __cdecl SelectTarget(float* player, float* aimPoint)
    {
        if (!Active)
            return TargetHook.ccall<int>(player, aimPoint);

        int count = std::clamp(20 - nAimCandidateCount, 0, 20);
        auto list = aAimCandidates->data();
        std::vector<AimCandidate> saved(list, list + count);
        std::vector<std::pair<float, int>> order;
        for (int i = 0; i < count; ++i)
        {
            float key = 1000.0f + float(i);
            auto object = *reinterpret_cast<float**>(list[i].data);
            if (object && *reinterpret_cast<int32_t*>(list[i].data + 4) == 1)
            {
                float dx = object[0] - player[0], dz = object[2] - player[2];
                float length = std::sqrt(dx * dx + dz * dz);
                if (length > 1.0f)
                {
                    float angle = std::acos(std::clamp((dx * ForwardX + dz * ForwardZ) / length, -1.0f, 1.0f));
                    if (angle < 0.45f)
                        key = angle;
                }
            }
            order.emplace_back(key, i);
        }
        std::stable_sort(order.begin(), order.end());
        for (int i = 0; i < count; ++i)
            list[i] = saved[order[i].second];
        int result = TargetHook.ccall<int>(player, aimPoint);
        std::copy(saved.begin(), saved.end(), list);
        return result;
    }

    // Facing while aiming: with no target, Heather turns to where the camera looks
    void __cdecl FaceTarget(int useAimPoint)
    {
        auto player = Player();
        if (Active && player && bAiming && nLockedTarget == 0)
        {
            float target = std::atan2(ForwardX, ForwardZ);
            float dt = fPlayerDt;
            float delta = Wrap(target - player[5]);
            float step = delta * std::min(1.0f, 15.0f * dt);
            player[5] = Wrap(std::abs(delta) < 0.01f ? target : player[5] + step);
            return;
        }
        FacingHook.ccall<void>(useAimPoint);
    }

    template<class T>
    T Resolve(const char* signature, int offset)
    {
        auto pattern = hook::pattern(signature);
        return *pattern.get_first<T>(offset);
    }

    void* Find(const char* signature, int offset = 0)
    {
        auto pattern = hook::pattern(signature);
        return pattern.get_first(offset);
    }
}

// The modern camera drives the view this frame
export bool IsModernCameraActive()
{
    return Active;
}

class Camera
{
public:
    Camera()
    {
        WFP::onInitEvent() += []()
        {
            CameraMode = &FixSettings().Int("CAMERA.Camera");
            Sensitivity = &FixSettings().Float("CAMERA.CameraSpeed");
            InvertY = &FixSettings().Int("CAMERA.InvertCamera");
            Distance = &FixSettings().Int("CAMERA.CameraDistance");
            FieldOfView = &FixSettings().Float("CAMERA.FieldOfView");

            // The player, camera mode, event, render mode, pause, aiming and projection variables are bound in Init
            fPlayerDt.SetAddress(Resolve<float*>("56 E8 ? ? ? ? A3 ? ? ? ? E8 ? ? ? ? D9 1D ? ? ? ? 8B 74 24 08 6A 00 56 C7 86 14 01 00 00", 18));
            nLockedTarget.SetAddress(Resolve<int32_t*>("A1 ? ? ? ? 83 EC 08 85 C0 0F 84 ? ? ? ? 8A 0D ? ? ? ? 84 C9 0F 84 ? ? ? ?", 1));
            aHeadingSlot.SetAddress(Resolve<std::array<uint8_t, 2>*>("0F B6 0D ? ? ? ? 83 C4 08 D9 1C 8D ? ? ? ? 8B 15 ? ? ? ? 8B 0D ? ? ? ? 8D 41 FE 83 F8 2A", 3));
            aHeadingReference.SetAddress(Resolve<std::array<float, 2>*>("0F B6 0D ? ? ? ? 83 C4 08 D9 1C 8D ? ? ? ? 8B 15 ? ? ? ? 8B 0D ? ? ? ? 8D 41 FE 83 F8 2A", 13));

            auto targeting = Find("55 8B EC 83 E4 F0 83 EC 74 8B 15 ? ? ? ? 83 FA 14 53 56 57 0F 84 ? ? ? ? B8 14 00 00 00");
            nAimCandidateCount.SetAddress(*reinterpret_cast<int32_t**>(reinterpret_cast<uint8_t*>(targeting) + 11));
            aAimCandidates.SetAddress(*reinterpret_cast<std::array<AimCandidate, 20>**>(reinterpret_cast<uint8_t*>(targeting) + 0x36));

            auto projection = Find("85 C0 74 ? E8 ? ? ? ? 51 D9 1C 24 E8 ? ? ? ?");
            auto setter = injector::GetBranchDestination(reinterpret_cast<uint8_t*>(projection) + 13).get<uint8_t>();
            SetProjection = reinterpret_cast<decltype(SetProjection)>(setter);

            SetCameraMatrix = reinterpret_cast<decltype(SetCameraMatrix)>(Find("55 8B EC 8B 4D 0C 8B 45 08 51 68 ? ? ? ? C7 05 ? ? ? ? 00 00 00 00"));
            SegmentQuery = reinterpret_cast<decltype(SegmentQuery)>(Find("83 EC 14 55 8B 6C 24 24 D9 45 00 57 8B 7C 24 2C D8 27 8B 44 24 20 D9 45 04 6A 00"));

            auto playerUpdate = Find("56 8B 74 24 08 8B 46 7C 83 E8 00 74 6F 48 75 7D E8 ? ? ? ? E8 ? ? ? ? 56 E8 ? ? ? ?");
            auto cameraTail = Find("68 ? ? ? ? E8 ? ? ? ? A1 ? ? ? ? 8B 0D ? ? ? ? 8B 15 ? ? ? ? A3 ? ? ? ? A1 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? 89 0D ? ? ? ? 89 15");
            auto controlType = Find("FE C8 24 01 A2 ? ? ? ? E8 ? ? ? ? 0F BE 15 ? ? ? ? 0F B6 C8 3B D1 A1 ? ? ? ? 75 07 25 FF FD FF FF");
            auto heading = Find("0F B6 0D ? ? ? ? 83 C4 08 D9 1C 8D ? ? ? ? 8B 15 ? ? ? ? 8B 0D ? ? ? ? 8D 41 FE 83 F8 2A", 23);
            auto facing = Find("A1 ? ? ? ? 83 EC 08 85 C0 0F 84 ? ? ? ? 8A 0D ? ? ? ? 84 C9 0F 84 ? ? ? ?");

            PlayerUpdateHook = safetyhook::create_inline(playerUpdate, static_cast<void(__cdecl*)(void*)>([](void* object)
            {
                PlayerUpdateHook.ccall<void>(object);
                PlayerTicked = true;
            }));

            CameraHook = safetyhook::create_mid(cameraTail, [](SafetyHookContext&)
            {
                UpdateCamera();
            });

            // Camera-relative ("2D") controls while the modern camera runs, whatever the option says
            auto controlTypeFunction = injector::GetBranchDestination(reinterpret_cast<uint8_t*>(controlType) + 9).get<void>();
            ControlTypeHook = safetyhook::create_inline(controlTypeFunction, static_cast<uint8_t(__cdecl*)()>([]() -> uint8_t
            {
                if (Active)
                    return 1;
                return ControlTypeHook.ccall<uint8_t>();
            }));

            // Movement is relative to the modern camera's yaw
            HeadingHook = safetyhook::create_mid(heading, [](SafetyHookContext&)
            {
                if (!Active)
                    return;
                (*aHeadingReference)[0] = (*aHeadingReference)[1] = Yaw;
                (*aHeadingSlot)[1] = 0;
            });

            TargetHook = safetyhook::create_inline(targeting, &SelectTarget);
            FacingHook = safetyhook::create_inline(facing, &FaceTarget);
        };
    }
} Camera;
