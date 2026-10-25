module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "../Shared/PatternPreflight.hpp"
#include "../Shared/SilentHill/Gamepad.hpp"

export module Camera;

// The modern camera: a free third-person camera over Henry's right shoulder outside the apartment, turned with the
// right stick or the mouse, in the style of the Silent Hill 2 remake. Movement follows the camera (the game's own
// third-person movement is already camera-relative), readying a weapon brings the camera closer, and with no enemy
// locked on Henry faces where the camera looks, so the game's lock-on picks the enemies in view. The apartment's
// first-person views, cutscenes, scripted cameras, menus, the item ring and the map stay native.
//
// The game's outdoor camera task keeps running underneath; the fix replaces its result. Conventions of the game:
// about 200 units per metre, Y points down, yaw 0 looks along +Z, matrices are camera-to-world rows right, up,
// forward, eye.

namespace
{
    constexpr float Pi = 3.14159265f;
    constexpr float Metre = 200.0f;

    const int* CameraMode = nullptr;
    const float* Sensitivity = nullptr;
    const int* InvertY = nullptr;
    const int* Distance = nullptr;
    const float* FieldOfView = nullptr;     // degrees, vertical

    // Game data
    uint8_t* Status = nullptr;              // per-frame player summary: +0 position, +0x84 yaw, +0xC4 status code
    uint8_t** Character = nullptr;          // current character: +0x23C lock-on target
    float* Intent = nullptr;                // movement intent: +0x08 desired yaw, +0x50 ready weapon held
    float* CameraMatrix = nullptr;          // the camera task's result, applied next frame
    float* CameraFov = nullptr;             // radians
    int32_t* ThirdPerson = nullptr;
    int32_t* GameMode = nullptr;            // 0 play, 1 event, 2 overlay
    int32_t* SceneFlags = nullptr;          // bit 0: scripted scene
    int32_t* SceneFlag2 = nullptr;
    int32_t* ControlLocked = nullptr;
    int32_t* ScriptedCamera = nullptr;
    int32_t* AutoWalk = nullptr;
    int32_t* CameraOverride = nullptr;
    int32_t* GameOver = nullptr;
    float* BasisLength = nullptr;           // camera-relative movement basis
    float* BasisYaw = nullptr;

    int(__cdecl* OutdoorTask)() = nullptr;
    int(__cdecl* SegmentHit)(float* segment, float* plane) = nullptr;
    int(__cdecl* HitDistance)(float* t, float* segment, float* plane) = nullptr;
    int(__cdecl* SetEye)(float* eye, int smooth) = nullptr;
    int(__cdecl* SetTarget)(float* target, int smooth) = nullptr;
    void(__cdecl* ApplyCamera)() = nullptr; // hands the camera task's result to the renderer
    SafetyHookInline IntentHook;

    bool Active = false;
    bool HaveOrbit = false;
    uint32_t LastFrame = 0, Frame = 0;
    constexpr float RestPitch = -0.12f;
    float Yaw = 0, Pitch = RestPitch;
    float CurrentDistance = 0, CurrentShoulder = 0, CurrentFov = 0, CollisionDistance = 0;
    float RecentreTime = 0;
    float LastEye[4] = {}, LastTarget[4] = {};

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

    template<class T> T& At(uint8_t* base, int offset) { return *reinterpret_cast<T*>(base + offset); }

    bool CanRun()
    {
        if (!CameraMode || *CameraMode != 1)
            return false;
        if (!*ThirdPerson || *GameMode != 0 || (*SceneFlags & 1) || *SceneFlag2 || *ControlLocked || *ScriptedCamera ||
            *AutoWalk || *CameraOverride || *GameOver)
            return false;
        int status = At<int32_t>(Status, 0xC4);
        return status < 6;
    }

    // Free fraction of the segment from -> to
    float FreeFraction(const float* from, const float* to)
    {
        float segment[8] = { from[0], from[1], from[2], 1.0f, to[0], to[1], to[2], 1.0f };
        float plane[4] = {};
        if (!SegmentHit(segment, plane))
            return 1.0f;
        float t = 0;
        if (!HitDistance(&t, segment, plane))
            return 1.0f;
        float dx = to[0] - from[0], dy = to[1] - from[1], dz = to[2] - from[2];
        float length = std::sqrt(dx * dx + dy * dy + dz * dz);
        return length > 1.0f ? std::clamp(t / length, 0.0f, 1.0f) : 1.0f;
    }

    void Update()
    {
        ++Frame;
        bool run = CanRun();
        if (!run)
        {
            if (Active)
            {
                // Hand back: the native follow camera starts from where the modern camera was
                SetEye(LastEye, 0);
                SetTarget(LastTarget, 0);
            }
            Active = false;
            CurrentDistance = 0;
            SHPad::HideMouseFromGame(false);
            SHPad::TakeMouse();
            return;
        }

        // Real time since the previous update (the game's tick rate isn't fixed)
        static LARGE_INTEGER frequency = {}, previous = {};
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        if (!frequency.QuadPart)
            QueryPerformanceFrequency(&frequency);
        float dt = previous.QuadPart ? std::clamp(float(double(now.QuadPart - previous.QuadPart) / double(frequency.QuadPart)), 1.0f / 240.0f, 1.0f / 20.0f) : 1.0f / 60.0f;
        previous = now;
        float playerYaw = At<float>(Status, 0x84);
        int status = At<int32_t>(Status, 0xC4);
        bool aiming = status == 3 || status == 4 || status == 5 || (Intent && Intent[0x50 / 4] != 0.0f);

        // After cutscenes, menus or loading: start behind Henry
        if (!HaveOrbit || Frame - LastFrame > 15)
        {
            Yaw = playerYaw;
            Pitch = RestPitch;
            HaveOrbit = true;
            CollisionDistance = 0;
        }
        LastFrame = Frame;

        auto pad = SHPad::Get();
        float speed = 0.6f + 0.25f * (*Sensitivity);         // rad/s at full deflection
        float sx = pad.rx * std::abs(pad.rx), sy = pad.ry * std::abs(pad.ry);
        auto [mouseX, mouseY] = SHPad::TakeMouse();
        float mouseScale = 0.0004f * (*Sensitivity);        // rad per count
        Yaw = Wrap(Yaw + sx * speed * dt + float(mouseX) * mouseScale);
        float pitchInput = sy * speed * 0.75f * dt + float(mouseY) * mouseScale;
        Pitch = std::clamp(Pitch - (*InvertY ? -pitchInput : pitchInput), -0.9f, 1.0f);

        // The "camera behind" button recentres
        if (pad.pressed & (SHPad::LT | SHPad::RS))
            RecentreTime = 0.25f;
        if (RecentreTime > 0)
        {
            float k = std::min(1.0f, dt / RecentreTime);
            Yaw = Wrap(Yaw + Wrap(playerYaw - Yaw) * k);
            Pitch += (RestPitch - Pitch) * k;
            RecentreTime -= dt;
        }

        static const float Distances[] = { 1.5f, 2.0f, 2.6f };
        float targetDistance = (aiming ? 1.3f : Distances[std::clamp(*Distance, 0, 2)]) * Metre;
        float targetShoulder = (aiming ? 0.55f : 0.45f) * Metre;
        // The setting is the vertical angle; the game's angle is the horizontal one of its 4:3 frame
        float vertical = (*FieldOfView - (aiming ? 10.0f : 0.0f)) * Pi / 180.0f;
        float targetFov = 2.0f * std::atan(std::tan(vertical * 0.5f) / 0.78125f);
        if (CurrentDistance == 0)
        {
            CurrentDistance = targetDistance;
            CurrentShoulder = targetShoulder;
            CurrentFov = targetFov;
        }
        CurrentDistance = Approach(CurrentDistance, targetDistance, 8.0f, dt);
        CurrentShoulder = Approach(CurrentShoulder, targetShoulder, 8.0f, dt);
        CurrentFov = Approach(CurrentFov, targetFov, 8.0f, dt);

        float sinYaw = std::sin(Yaw), cosYaw = std::cos(Yaw), sinPitch = std::sin(Pitch), cosPitch = std::cos(Pitch);
        float forward[3] = { sinYaw * cosPitch, -sinPitch, cosYaw * cosPitch };
        float right[3] = { cosYaw, 0, -sinYaw };

        auto position = reinterpret_cast<float*>(Status);
        float centre[3] = { position[0], position[1] - 1.55f * Metre, position[2] };
        float shoulderAmount = CurrentShoulder * (0.6f + 0.4f * cosPitch);
        float shoulder[3] = { centre[0] + right[0] * shoulderAmount, centre[1], centre[2] + right[2] * shoulderAmount };
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
            CollisionDistance = allowed;
        else
            CollisionDistance = Approach(CollisionDistance, allowed, 4.0f, dt);
        float distance = std::min(CollisionDistance, CurrentDistance);

        float eye[3];
        for (int i = 0; i < 3; ++i)
            eye[i] = shoulder[i] - forward[i] * distance;

        // right, up (world down), forward, eye
        float up[3] = { forward[1] * right[2] - forward[2] * right[1], forward[2] * right[0] - forward[0] * right[2], forward[0] * right[1] - forward[1] * right[0] };
        float matrix[16] =
        {
            right[0], right[1], right[2], 0,
            up[0], up[1], up[2], 0,
            forward[0], forward[1], forward[2], 0,
            eye[0], eye[1], eye[2], 1,
        };
        memcpy(CameraMatrix, matrix, sizeof(matrix));
        *CameraFov = CurrentFov;
        // The renderer took the camera before this task ran: give it this frame's view now, not a frame late
        if (ApplyCamera)
            ApplyCamera();
        for (int i = 0; i < 3; ++i)
        {
            LastEye[i] = eye[i];
            LastTarget[i] = eye[i] + forward[i] * Metre * 3.0f;
        }
        LastEye[3] = LastTarget[3] = 1.0f;

        // Movement relative to the camera from this frame on
        *BasisLength = 1.0f;
        *BasisYaw = Yaw;

        Active = true;
        SHPad::HideMouseFromGame(true);
    }

    int __cdecl ModernOutdoorTask()
    {
        int result = OutdoorTask();
        Update();
        return result;
    }

    // Ready weapon with no enemy locked on and no movement: Henry turns to where the camera looks
    float* __cdecl MoveIntent()
    {
        auto result = IntentHook.ccall<float*>();
        if (Active && Intent && Character && *Character)
        {
            bool ready = Intent[0x50 / 4] != 0.0f;
            bool target = At<int32_t>(*Character, 0x23C) != 0;
            auto pad = SHPad::Get();
            bool moving = pad.lx != 0.0f || pad.ly != 0.0f;
            if (ready && !target && !moving)
                Intent[0x08 / 4] = Yaw;
        }
        return result;
    }

    template<class T>
    T Resolve(const char* signature, int offset, int index = 0)
    {
        auto pattern = hook::pattern(signature);
        if (pattern.size() <= size_t(index))
        {
            PatternPreflight::Report("Silent Hill 4 camera");
            return T{};
        }
        return *pattern.get(index).get<T>(offset);
    }

    void* Find(const char* signature, int index = 0)
    {
        auto pattern = hook::pattern(signature);
        if (pattern.size() <= size_t(index))
        {
            PatternPreflight::Report("Silent Hill 4 camera");
            return nullptr;
        }
        return pattern.get(index).get<void>(0);
    }
}

export namespace Camera
{
    bool IsModernActive()
    {
        return Active;
    }

    void Init()
    {
        CameraMode = &FixSettings().Int("CAMERA.Camera");
        Sensitivity = &FixSettings().Float("CAMERA.CameraSpeed");
        InvertY = &FixSettings().Int("CAMERA.InvertCamera");
        Distance = &FixSettings().Int("CAMERA.CameraDistance");
        FieldOfView = &FixSettings().Float("CAMERA.FieldOfView");

        Status = Resolve<uint8_t*>("55 8B EC 83 E4 F0 83 EC 14 8B 0D ? ? ? ? A1 ? ? ? ? 8B 15 ? ? ? ? 53 56 89 0D", 0x10);
        Character = Resolve<uint8_t**>("8B 45 08 83 F8 02 7C 02 33 C0 69 C0 20 04 00 00 05", 0x16);
        Intent = Resolve<float*>("B8 ? ? ? ? C3 90 90 90 90 90 90 90 90 90 90 A1 ? ? ? ? C3 90 90 90 90 90 90 90 90 90 90 8B 15", 1);
        ControlLocked = Resolve<int32_t*>("B8 ? ? ? ? C3 90 90 90 90 90 90 90 90 90 90 A1 ? ? ? ? C3 90 90 90 90 90 90 90 90 90 90 8B 15", 17);
        CameraMatrix = Resolve<float*>("68 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? A1 ? ? ? ? 50 E8 ? ? ? ? 83 C4 10 E9", 1);
        CameraFov = Resolve<float*>("68 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? A1 ? ? ? ? 50 E8 ? ? ? ? 83 C4 10 E9", 0x1A);
        ApplyCamera = reinterpret_cast<decltype(ApplyCamera)>(Find("68 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? A1 ? ? ? ? 50 E8 ? ? ? ? 83 C4 10 E9"));
        ThirdPerson = Resolve<int32_t*>("A1 ? ? ? ? 85 C0 75 ? 6A 00 E8 ? ? ? ? 83 C4 04 EB ? E8", 1);
        GameMode = Resolve<int32_t*>("A1 ? ? ? ? 8B 34 85 ? ? ? ? 33 FF", 1);
        SceneFlags = Resolve<int32_t*>("A1 ? ? ? ? 83 E0 01 C3 90 90 90 90 90 90 90 55 8B EC 83 EC 1C", 1);
        SceneFlag2 = Resolve<int32_t*>("A1 ? ? ? ? C3 90 90 90 90 90 90 90 90 90 90 55 8B EC 66 8B 45 08 66", 1);
        AutoWalk = Resolve<int32_t*>("89 0D ? ? ? ? C7 05 ? ? ? ? 01 00 00 00 C3", 8);
        CameraOverride = Resolve<int32_t*>("C7 05 ? ? ? ? 01 00 00 00 D9 00", 2);
        GameOver = Resolve<int32_t*>("E8 ? ? ? ? C7 05 ? ? ? ? 01 00 00 00 C3 C7 05 ? ? ? ? 01 00 00 00", 7);
        auto scripted = hook::pattern("A1 ? ? ? ? C3 90 90 90 90 90 90 90 90 90 90 33 C0 A3 ? ? ? ? A3");
        for (size_t i = 0; i < scripted.size(); ++i)
        {
            // The getter whose next function clears the same flag
            if (*scripted.get(i).get<int32_t*>(1) == *scripted.get(i).get<int32_t*>(19))
            {
                ScriptedCamera = *scripted.get(i).get<int32_t*>(1);
                break;
            }
        }
        auto basis = Find("83 EC 08 8B 4C 24 0C D9 41 08 D9 01 D9 C0 D8 C9 D9 C2 D8 CB DE C1");
        if (basis)
        {
            BasisLength = *reinterpret_cast<float**>(reinterpret_cast<uint8_t*>(basis) + 0x48);
            BasisYaw = *reinterpret_cast<float**>(reinterpret_cast<uint8_t*>(basis) + 0x74);
        }
        SegmentHit = reinterpret_cast<decltype(SegmentHit)>(Find("55 8B EC 83 E4 F0 83 EC 24 53 56 57 E8 ? ? ? ? A1 ? ? ? ? 85 C0 0F 84 F4 00 00 00", 0));
        HitDistance = reinterpret_cast<decltype(HitDistance)>(Find("55 8B EC 83 E4 F0 83 EC 1C 8B 45 0C D9 40 04 56 8B 75 10 D8 4E 04"));
        SetEye = reinterpret_cast<decltype(SetEye)>(Find("55 8B EC 8B 45 08 85 C0 74 23 8B 08 89 0D ? ? ? ? 8B 50 04 89 15 ? ? ? ? 8B 40 08 A3"));
        SetTarget = reinterpret_cast<decltype(SetTarget)>(Find("55 8B EC 8B 45 08 85 C0 74 24 8B 08 89 0D ? ? ? ? 8B 50 04 89 15 ? ? ? ? 8B 48 08 89 0D"));
        auto dispatch = hook::pattern("55 8B EC A1 ? ? ? ? 83 F8 01 74 0C 83 F8 0A 74 07 B8 01 00 00 00 EB 02 33 C0 5D FF 24 85");
        auto intent = Find("55 8B EC 83 E4 F0 83 EC 24 A1 ? ? ? ? 8B 0D ? ? ? ? 53 8B 1D ? ? ? ? 56 8B 35");

        if (!Status || !Character || !Intent || !ControlLocked || !CameraMatrix || !CameraFov || !ThirdPerson || !GameMode ||
            !SceneFlags || !SceneFlag2 || !AutoWalk || !CameraOverride || !GameOver || !ScriptedCamera || !BasisLength ||
            !BasisYaw || !SegmentHit || !HitDistance || !SetEye || !SetTarget || dispatch.size() != 1)
        {
            // A game the free camera doesn't know: classic, whatever the setting says
            static const int Classic = 0;
            CameraMode = &Classic;
            return;
        }

        // The outdoor camera task is called through a table: its entry is replaced
        auto table = *dispatch.get_first<void**>(0x1F);
        OutdoorTask = reinterpret_cast<decltype(OutdoorTask)>(table[1]);
        injector::WriteMemory(&table[1], &ModernOutdoorTask, true);

        if (intent)
            IntentHook = safetyhook::create_inline(intent, &MoveIntent);
    }
}
