module;

#include <stdafx.h>
#include <cmath>
#include <numbers>

export module Camera;

import ComVars;

namespace Camera
{
    struct Vector { float x, y, z; };
    SafetyHookInline shInput, shUpdate, shPlace, shAim;
    uint8_t *pControlFlags, *pInvertLook;
    float *pSensitivityX, *pSensitivityY;
    uintptr_t input, owner, vehicle;
    uint32_t inputFrame, consumedFrame;
    DWORD inputTick, cameraTick;
    float lookX, lookY;
    bool stickX, stickY;
    float dt, yaw, pitch, idleTimer;
    float CameraReturnTimeout, CameraReturnSpeed;
    bool engaged;
    constexpr float pi = std::numbers::pi_v<float>;

    void Reset()
    {
        owner = vehicle = 0;
        engaged = false;
        idleTimer = 0.0f;
        consumedFrame = inputFrame;
    }

    void ReadLook(int action, float& value, bool& stick)
    {
        value = 0.0f;
        stick = false;
        size_t actionsOffset = nGameVersion == MAFIA_1_0_ENG ? 192 : 196;
        auto values = Field<uintptr_t>(input, actionsOffset);
        auto valuesEnd = Field<uintptr_t>(input, actionsOffset + 4);
        if (values && valuesEnd >= values + (action + 1) * sizeof(float))
        {
            float current = Field<float>(values, action * sizeof(float));
            if (std::isfinite(current)) value = std::clamp(current, -1.0f, 1.0f);
        }
        size_t bindingsOffset = nGameVersion == MAFIA_1_0_ENG ? 176 : 180;
        auto bindings = Field<uintptr_t>(input, bindingsOffset);
        auto bindingsEnd = Field<uintptr_t>(input, bindingsOffset + 4);
        if (bindings && bindingsEnd >= bindings + (action + 1) * 12)
            stick = Field<uint16_t>(bindings + action * 12, 4) == 3;
    }

    void __fastcall UpdateInput(uintptr_t _this, void*, uint32_t poll)
    {
        shInput.unsafe_thiscall(_this, poll);
        input = _this;
        // Read the same look actions as the native aiming camera, after their
        // configured mouse/joystick bindings, normalization and deadzones.
        // The game loop polls devices itself and calls this with poll=0.
        ReadLook(18, lookX, stickX);
        ReadLook(19, lookY, stickY);
        inputTick = GetTickCount();
        ++inputFrame;
    }

    void ConsumeLook(float& x, float& y)
    {
        x = y = 0.0f;
        if (consumedFrame == inputFrame) return;
        consumedFrame = inputFrame;
        if (GetTickCount() - inputTick > 250) return;
        // Mouse actions contain deltas; held joystick axes need elapsed time.
        x = lookX * (stickX ? dt * 60.0f : 1.0f);
        y = lookY * (stickY ? dt * 60.0f : 1.0f);
    }

    int __fastcall AimCamera(uintptr_t _this, void*, float x, float y,
        const Vector* forward, const Vector* up, const Vector* right)
    {
        if (owner == _this) ConsumeLook(x, y);
        // Keep Mafia's independent aim angles, sensitivity, inversion, pitch
        // limits and camera matrix update. Orbit fixes the camera's position;
        // aiming rotates the view freely from there instead of at the car.
        return shAim.unsafe_thiscall<int>(_this, x, y, forward, up, right);
    }

    int __fastcall UpdateCamera(uintptr_t _this, void*, uint32_t elapsed)
    {
        DWORD now = GetTickCount();
        auto mode = Field<int>(_this, 16);
        auto car = Field<uintptr_t>(_this, 12);
        if (mode < 7 || mode > 9 || !car || Field<uint8_t>(_this, 32) ||
            (*pControlFlags & 1) || std::abs(Field<float>(_this, 48)) > 0.001f ||
            (cameraTick && now - cameraTick > 250))
            Reset();
        else if (owner != _this || vehicle != car)
        {
            Reset();
            owner = _this;
            vehicle = car;
        }
        cameraTick = now;
        dt = std::min(elapsed, 83u) * 0.001f;
        return shUpdate.unsafe_thiscall<int>(_this, elapsed);
    }

    bool GetOrbit(uintptr_t camera, const Vector& forward, Vector& orbit)
    {
        if (owner != camera || !vehicle || dt <= 0.0f) return false;
        float horizontal = std::hypot(forward.x, forward.z);
        float radius = std::hypot(horizontal, forward.y);
        if (!std::isfinite(radius) || radius < 0.001f) return false;
        float nativeYaw = std::atan2(forward.x, forward.z);
        float nativePitch = std::atan2(-forward.y, horizontal);
        float deltaX = 0.0f, deltaY = 0.0f;
        bool aiming = Field<uint8_t>(camera, 148) != 0;
        if (!aiming)
        {
            ConsumeLook(deltaX, deltaY);
            deltaX *= *pSensitivityX * 3.0f;
            deltaY *= *pSensitivityY * 2.0f;
            if (*pInvertLook) deltaY = -deltaY;
        }
        bool hasInput = std::abs(deltaX) + std::abs(deltaY) > 0.00001f;
        if (!engaged)
        {
            yaw = nativeYaw;
            pitch = nativePitch;
        }
        if (hasInput)
        {
            engaged = true;
            idleTimer = 0.0f;
            yaw = std::remainder(yaw + deltaX, 2.0f * pi);
            pitch = std::clamp(pitch + deltaY, -0.15f, 1.3f);
        }
        else if (!aiming)
        {
            idleTimer += dt;
            // Keep the view while stationary or aiming/firing. Once driving
            // again, return along the shortest arc, independently of FPS.
            if (engaged && idleTimer > CameraReturnTimeout &&
                !Field<uint8_t>(camera, 148) && std::abs(Field<float>(vehicle, 1548)) > 0.5f)
            {
                float blend = 1.0f - std::exp(-CameraReturnSpeed * dt);
                yaw += std::remainder(nativeYaw - yaw, 2.0f * pi) * blend;
                pitch += (nativePitch - pitch) * blend;
                if (std::abs(std::remainder(nativeYaw - yaw, 2.0f * pi)) < 0.0001f &&
                    std::abs(nativePitch - pitch) < 0.0001f)
                    engaged = false;
            }
        }
        if (!engaged) return false;
        float distance = radius * std::cos(pitch);
        orbit = { distance * std::sin(yaw), -radius * std::sin(pitch), distance * std::cos(yaw) };
        return true;
    }

    int __fastcall PlaceCamera(uintptr_t _this, void*, const Vector* target, const Vector* forward)
    {
        Vector orbit;
        bool orbitView = GetOrbit(_this, *forward, orbit);
        if (owner != _this)
            return shPlace.unsafe_thiscall<int>(_this, target, forward);

        // Preserve both the native sphere sweep and its free-aim branch. While
        // aiming, keep the orbit position and route look input to AimCamera.
        // Native weapon targeting raycasts along this independent view direction.
        return shPlace.unsafe_thiscall<int>(_this, target, orbitView ? &orbit : forward);
    }
}

export void InitCamera()
{
    CIniReader ini("");
    if (!ini.ReadInteger("CAMERA", "FreeCamera", 0)) return;
    using namespace Camera;
    CameraReturnTimeout = std::max(ini.ReadFloat("CAMERA", "CameraReturnTimeout", 3.0f), 0.0f);
    CameraReturnSpeed = std::max(ini.ReadFloat("CAMERA", "CameraReturnSpeed", 2.0f), 0.0f);

    auto input = find_pattern("83 EC ? 53 8A 5C 24 ? 55 8B E9 56 57 8B 45 08 85 C0 74 ? 8B 4D 0C 2B C8 B8 EB A0 0E EA");
    auto update = find_pattern("81 EC ? ? ? ? 53 55 56 8B F1 33 ED 57 8B 46 04 8B 7E 1C 3B C5");
    auto place = find_pattern("83 EC 3C 8B 44 24 44 56 8B F1 8B 08 89 4C 24 04 8B 50 04 89 54 24 08");
    auto aim = find_pattern("A0 ? ? ? ? 83 EC 24 A8 01 56 57 8B F9 0F 85");
    if (input.size() != 1 || update.size() != 1 || place.size() != 1 || aim.size() != 1) return;
    auto aimStart = reinterpret_cast<uintptr_t>(aim.get_first());
    auto horizontal = hook::range_pattern(aimStart, aimStart + 0x100,
        "D9 05 ? ? ? ? D8 4C 24 ? DF E0 D8 0D ? ? ? ? F6 C4 05 D8 2D");
    auto vertical = hook::range_pattern(aimStart, aimStart + 0x100,
        "D9 05 ? ? ? ? D8 4C 24 ? A0 ? ? ? ? 84 C0 DF E0 DC C0");
    if (horizontal.size() != 1 || vertical.size() != 1) return;
    pControlFlags = *aim.get_first<uint8_t*>(1);
    pSensitivityX = *horizontal.get_first<float*>(2);
    pSensitivityY = *vertical.get_first<float*>(2);
    pInvertLook = *vertical.get_first<uint8_t*>(11);
    shInput = safetyhook::create_inline(input.get_first(), UpdateInput);
    shUpdate = safetyhook::create_inline(update.get_first(), UpdateCamera);
    shPlace = safetyhook::create_inline(place.get_first(), PlaceCamera);
    shAim = safetyhook::create_inline(aim.get_first(), AimCamera);
    if (!shInput || !shUpdate || !shPlace || !shAim)
    {
        shAim.reset();
        shPlace.reset();
        shUpdate.reset();
        shInput.reset();
    }
}
