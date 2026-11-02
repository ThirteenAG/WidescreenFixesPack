module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <atomic>
#include <cmath>

export module FreeCamera;

import ComVars;
import Cinema;
import DeveloperOverlay;

// Mouse look around the player on foot (normal, fight and stealth mode) and around the car while driving (when the
// game's own Mouse Steering option is off). The mouse turns the camera around the point it looks at; after the return
// delay without mouse movement, or when the right stick moves the camera, the game's camera takes over with a short
// blend. Views the game forces keep priority and end the mouse view: looking back from the car, another camera
// (missions, scenes); afterwards the game's camera stays until the mouse moves again. Guns mode keeps the game's mouse
// aiming.
export namespace TrueCrimeFreeCamera
{
    struct Vector
    {
        float x, y, z;
        Vector operator+(const Vector& o) const { return { x + o.x, y + o.y, z + o.z }; }
        Vector operator-(const Vector& o) const { return { x - o.x, y - o.y, z - o.z }; }
        Vector operator*(float s) const { return { x * s, y * s, z * s }; }
        float Dot(const Vector& o) const { return x * o.x + y * o.y + z * o.z; }
        Vector Cross(const Vector& o) const { return { y * o.z - z * o.y, z * o.x - x * o.z, x * o.y - y * o.x }; }
        float Length() const { return std::sqrt(Dot(*this)); }
        Vector Normalized() const { const float l = Length(); return l > 1e-6f ? *this * (1.0f / l) : Vector{ 0.0f, 0.0f, 1.0f }; }
    };

    // An object's local transform at +16: three axis rows and the position, 16 bytes each; the parent at +88
    struct alignas(16) Transform
    {
        Vector x; float xw; Vector y; float yw; Vector z; float zw; Vector p; float pw;
    };

    inline GameRef<uint8_t*> CurrentCamera;
    inline Transform* (__fastcall* WorldMatrix)(void*, void*, Transform*) = nullptr;
    inline ProtectedGameRef<uint8_t> MouseSteering;
    inline std::atomic<int> PendingMouseSteering{ -1 };
    // The world's ray test: from, to, hit position, hit normal, two objects to ignore, collision mask, closest hit, kind
    inline void* World = nullptr;
    inline void* (__fastcall* CheckRay)(void*, void*, const float*, const float*, float*, float*, void*, void*, int, int, int) = nullptr;
    inline SafetyHookMid RenderHook;

    // Mouse counts gathered by the input hook while the camera has the mouse; the camera also reads the mouse once
    // per drawn frame, so movement arrives every frame whatever the game's update rate
    inline std::atomic<int32_t> CountsX{ 0 }, CountsY{ 0 };
    inline void (*ReadMouse)() = nullptr;
    inline constexpr uint32_t RearView = 0x4000000;
    inline std::atomic_bool StickMoved{ false };

    inline constexpr float BlendSeconds = 0.4f;

    struct State
    {
        bool active = false;
        bool mouseActive = false;
        float idle = 0.0f;
        float blend = 0.0f;         // 1 right after the mouse camera hands back, fading to 0
        int mode = -1;
        // View direction (camera to the point it looks at) as yaw and pitch, the look point's height above the
        // player, the distance, and how the transform's axes relate to the view
        float yaw = 0.0f, pitch = 0.0f, height = 0.0f, distance = 0.0f;
        float reach = 0.0f;         // distance after obstacles, pulled in at once and eased back out
        float forwardSign = 1.0f, rightSign = 1.0f, upSign = 1.0f, yawSign = 1.0f;
        uint8_t* camera = nullptr;
        Transform last{};
    };
    inline State Camera;

    inline int PlayerMode()
    {
        auto player = PlayerObject.get();
        return player ? player[9] : 0;
    }

    // Whether mouse movement goes to the camera instead of the game
    inline bool TakesMouse()
    {
        if (!CurrentCamera.is_initialized() || !CurrentCamera.get() || GameMode.get() >= 2 ||
            TrueCrimeCinema::Active() || TrueCrimeOverlay::BlocksInput())
            return false;
        const int mode = PlayerMode();
        if (mode == 5)
            return !MouseSteering.get();
        // Not during precision aim (the slow-motion crosshair, state 3 at +0xD68)
        return (mode == 1 || mode == 3 || mode == 4) && *reinterpret_cast<int*>(PlayerObject.get() + 0xD68) != 3;
    }

    inline Vector Direction(float yaw, float pitch)
    {
        return { std::cos(pitch) * std::sin(yaw), std::sin(pitch), std::cos(pitch) * std::cos(yaw) };
    }

    // Rebuild the axes around a new forward axis, keeping the original's handedness
    inline void Orient(Transform& t, const Vector& z, float rightSign, float upSign)
    {
        const Vector up{ 0.0f, 1.0f, 0.0f };
        const Vector x = up.Cross(z).Normalized() * rightSign;
        const Vector y = z.Cross(x).Normalized() * upSign;
        t.x = x; t.y = y; t.z = z;
    }

    inline void Update(uint8_t* camera)
    {
        static LARGE_INTEGER frequency{}, previous{};
        LARGE_INTEGER now{};
        QueryPerformanceCounter(&now);
        if (!frequency.QuadPart)
            QueryPerformanceFrequency(&frequency);
        const float dt = previous.QuadPart ? std::clamp(float(now.QuadPart - previous.QuadPart) / float(frequency.QuadPart), 0.0f, 0.25f) : 0.0f;
        previous = now;

        if (ReadMouse && TakesMouse())
            ReadMouse();
        int32_t dx = CountsX.exchange(0), dy = CountsY.exchange(0);
        const bool stick = StickMoved.exchange(false);
        auto& state = Camera;
        auto& t = *reinterpret_cast<Transform*>(camera + 16);
        const bool usable = !*reinterpret_cast<void**>(camera + 88) && TakesMouse() && PlayerObject.get();
        const int mode = PlayerMode();
        // A view the game forces ends the mouse view; the mouse is ignored while it lasts
        const bool modeChanged = mode != state.mode;
        const bool forced = (mode == 5 && (HeldFlags.get() & RearView)) ||
            ((state.active || state.blend > 0.0f) && camera != state.camera);
        if (forced)
            dx = dy = 0;
        if (!usable || forced || modeChanged)
            state.blend = 0.0f;
        const float configuredDelay = FixSettings().Float("CAMERA.ReturnDelay");
        const float delay = std::isfinite(configuredDelay) ? std::clamp(configuredDelay, 0.1f, 30.0f) : 5.0f;
        if (usable && !stick && (dx || dy))
        {
            state.idle = 0.0f;
            state.mouseActive = true;
        }
        else
            state.idle += dt;
        if (!usable || stick || forced || modeChanged || state.idle >= delay)
            state.mouseActive = false;
        state.mode = mode;

        const bool wanted = usable && state.mouseActive;
        if (state.active && !wanted)
        {
            state.active = false;
            state.blend = usable && !forced && !modeChanged ? 1.0f : 0.0f;
        }
        if (!usable || forced)
            return;

        Transform player{};
        WorldMatrix(PlayerObject.get(), nullptr, &player);
        // The game's follow camera stays close to the player; a camera further away is a fixed or mission view
        if (wanted && !state.active && (player.p - t.p).Length() > 4.0f)
        {
            state.mouseActive = false;
            return;
        }
        if (wanted && !state.active)
        {
            // Start from the game's camera: the point it looks at next to the player, its distance and angles
            const Vector z = t.z.Normalized();
            const float along = (player.p - t.p).Dot(z);
            if (!std::isfinite(along) || std::abs(along) < 0.05f)
            {
                state.mouseActive = false;
                return;
            }
            state.forwardSign = along < 0.0f ? -1.0f : 1.0f;
            const Vector view = z * state.forwardSign;
            state.distance = std::abs(along);
            state.height = (t.p + z * along).y - player.p.y;
            state.pitch = std::asin(std::clamp(view.y, -1.0f, 1.0f));
            state.yaw = std::atan2(view.x, view.z);
            const Vector up{ 0.0f, 1.0f, 0.0f };
            state.rightSign = up.Cross(z).Dot(t.x) < 0.0f ? -1.0f : 1.0f;
            state.upSign = z.Cross(t.x).Dot(t.y) < 0.0f ? -1.0f : 1.0f;
            // Mouse right turns the view towards the transform's x axis
            const Vector turn{ std::cos(state.yaw), 0.0f, -std::sin(state.yaw) };
            state.yawSign = turn.Dot(t.x) < 0.0f ? -1.0f : 1.0f;
            state.camera = camera;
            state.reach = state.distance;
            state.active = true;
            state.blend = 0.0f;
        }

        if (state.active)
        {
            const float configuredSensitivity = FixSettings().Float("MAIN.MouseSensitivity");
            const float sensitivity = std::isfinite(configuredSensitivity) ? std::clamp(configuredSensitivity, 0.1f, 5.0f) : 1.0f;
            const float scale = 0.002f * sensitivity;
            state.yaw += float(dx) * scale * state.yawSign;
            // The world's y axis points down: a positive pitch looks down
            const float highest = mode == 5 ? -0.17f : -0.70f;
            const float lowest = mode == 5 ? 0.70f : 1.20f;
            const float vertical = FixSettings().Bool("MAIN.InvertMouseY") ? -1.0f : 1.0f;
            state.pitch = std::clamp(state.pitch + float(dy) * scale * vertical, highest, lowest);
            const Vector view = Direction(state.yaw, state.pitch);
            const Vector look{ player.p.x, player.p.y + state.height, player.p.z };
            // Like the game's camera, the camera stays in front of walls, cars and the ground between it and the player
            // In a car the test starts behind the car's middle, so the car itself doesn't count
            const float start = mode == 5 ? std::min(2.5f, state.distance * 0.5f) : 0.0f;
            const Vector origin = look - view * start;
            alignas(16) float from[4]{ origin.x, origin.y, origin.z, 1.0f };
            alignas(16) float to[4]{};
            const Vector wanted = look - view * state.distance;
            to[0] = wanted.x; to[1] = wanted.y; to[2] = wanted.z; to[3] = 1.0f;
            alignas(16) float hit[4]{}, normal[4]{};
            float reach = state.distance;
            if (CheckRay(World, nullptr, from, to, hit, normal, PlayerObject.get(), nullptr, 0xFF00, 1, 0))
                reach = std::clamp(start + (Vector{ hit[0], hit[1], hit[2] } - origin).Length() - 0.1f, 0.05f, state.distance);
            if (reach < state.reach)
                state.reach = reach;
            else
                state.reach += (reach - state.reach) * std::min(1.0f, dt * 4.0f);
            t.p = look - view * state.reach;
            // The ground isn't part of the ray test: the camera stays above the player's feet (the world's y axis
            // points down)
            t.p.y = std::min(t.p.y, player.p.y - (mode == 5 ? 0.05f : 0.12f));
            Orient(t, view * state.forwardSign, state.rightSign, state.upSign);
            state.last = t;
        }
        else if (state.blend > 0.0f)
        {
            // Hand back smoothly: from the last mouse view to the game's
            const float b = state.blend;
            const Vector z = (t.z * (1.0f - b) + state.last.z * b).Normalized();
            t.p = t.p * (1.0f - b) + state.last.p * b;
            Orient(t, z, state.rightSign, state.upSign);
            state.blend = std::max(0.0f, state.blend - dt / BlendSeconds);
        }
    }

    inline void Init()
    {
        // Before the frame is drawn: the current camera's transform becomes the view
        auto render = hook::pattern("8B 0D ? ? ? ? 8D ? 24 ? ? E8 ? ? ? ? 50 B9 ? ? ? ? E8");
        CurrentCamera.SetAddress(*render.get_first<uint8_t**>(2));
        WorldMatrix = reinterpret_cast<decltype(WorldMatrix)>(injector::GetBranchDestination(render.get_first(11)).as_int());
        // The pedestrian camera's own obstruction test: the world and its ray test
        auto ray = hook::pattern("B9 ? ? ? ? E8 ? ? ? ? 85 C0 74 ? 8B C8");
        World = *ray.get_first<void*>(1);
        CheckRay = reinterpret_cast<decltype(CheckRay)>(injector::GetBranchDestination(ray.get_first(5)).as_int());
        auto steering = hook::pattern("80 3D ? ? ? ? 00 0F 84 ? ? ? ? 83 FF 02 0F 8D");
        if (!steering.empty()) // TrueCrime_.exe
            MouseSteering.SetAddress(*steering.get_first<uint8_t*>(2));
        else // TrueCrimeMB.exe
            MouseSteering.SetAddress(*hook::pattern("A0 ? ? ? ? 84 C0 0F 84 ? ? ? ? 83 FD 02 0F 8D").get_first<uint8_t*>(1));
        FixSettings().OnChange("MAIN.MouseSteering", []()
        {
            PendingMouseSteering.store(FixSettings().Bool("MAIN.MouseSteering") ? 1 : 0, std::memory_order_relaxed);
        });
        WFP::onGameProcessEvent() += []()
        {
            const int requested = PendingMouseSteering.exchange(-1, std::memory_order_relaxed);
            if (requested >= 0)
                MouseSteering = static_cast<uint8_t>(requested);
        };
        RenderHook = safetyhook::create_mid(render.get_first(), [](SafetyHookContext&)
        {
            if (auto camera = CurrentCamera.get())
                Update(camera);
        });
    }
}
