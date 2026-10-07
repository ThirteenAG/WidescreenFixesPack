module;

#include "stdafx.h"
#include <numbers>

export module Camera;

import Build;
import Settings;
import Input;
import Controller;

namespace
{
    struct CameraAngles
    {
        float alpha, alphaSpeed, fov, fovSpeed, beta, betaSpeed;
    };
    static_assert(offsetof(CameraAngles, beta) == 0x10);
    struct PedInput
    {
        CameraAngles* camera = nullptr;
        float x = 0.0f, y = 0.0f;
        bool eligible = false;
        bool controller = false;
        float betaSpeed = 0.0f;
    } ped;
    struct CarInput
    {
        CameraAngles* camera = nullptr;
        float y = 0.0f;
        float hold = 0.0f;
        bool sampled = false, eligible = false, applied = false;
        const float* parameters = nullptr;
    } car;
    SafetyHookInline carBetaHook;
    std::vector<SafetyHookMid> orbitHooks;
    using AimAxis = float(*)(void*, void*, float, bool, bool, int);
    AimAxis aimHorizontal = nullptr;
    const float* carParameters = nullptr;
    float* timeStep = nullptr;
    uint16_t* disabledControls = nullptr;
    int* direction = nullptr;
    bool* scriptAngles = nullptr;

    bool MouseOrbitAllowed()
    {
        return IsMouseCameraActive() && !*disabledControls && !*scriptAngles;
    }
    float FovScale(const CameraAngles& camera)
    {
        return camera.fov * 0.0125f;
    }
    float ComputeCarBeta(CameraAngles* camera, void* vehicle, void* target)
    {
        const auto native = carBetaHook.ccall<float>(camera, vehicle, target);
        if (!*disabledControls && !*scriptAngles && *direction == 3 && car.parameters)
        {
            const auto x = aimHorizontal(reinterpret_cast<uint8_t*>(disabledControls) - 48,
                nullptr, 0.0f, false, false, 0);
            if (const auto speed = OriginalControllerOrbit(x, true, camera->fov))
            {
                const auto velocity = reinterpret_cast<const float*>(vehicle) + 31;
                const auto magnitude = sqrtf(velocity[0] * velocity[0] + velocity[1] * velocity[1] + velocity[2] * velocity[2]);
                const auto following = native - x * (1.0f + magnitude * 1.7f);
                auto difference = following - camera->beta;
                while (difference > std::numbers::pi_v<float>) difference -= 2.0f * std::numbers::pi_v<float>;
                while (difference < -std::numbers::pi_v<float>) difference += 2.0f * std::numbers::pi_v<float>;
                const auto step = std::max(0.0f, *timeStep);
                const auto damping = powf(car.parameters[8], step);
                const auto desired = std::clamp(difference / std::max(1.0f, step)
                    + *speed * car.parameters[12], -car.parameters[9], car.parameters[9]);
                camera->betaSpeed = damping * camera->betaSpeed + (1.0f - damping) * desired;
                if (fabsf(camera->betaSpeed) < 0.0001f)
                    camera->betaSpeed = 0.0f;
                return camera->beta + step * camera->betaSpeed;
            }
        }
        if (!MouseOrbitAllowed() || *direction != 3)
            return native;
        // Keep the native pad's wheel/vehicle/aim gates and inversion/settings.
        const auto x = aimHorizontal(reinterpret_cast<uint8_t*>(disabledControls) - 48,
            nullptr, 0.0f, false, false, 0);
        if (x != 0.0f)
            car.hold = Settings.centeringDelay;
        if (x == 0.0f && car.hold <= 0.0f)
            return native;
        // Original SA uses a two-times mouse delta, with FOV scaling, rather
        // than the DE helper's additional gain from vehicle velocity.
        return camera->beta + x * 2.5f * FovScale(*camera);
    }

    void InstallMouseOrbit()
    {
        // Patterns resolve the instruction boundaries audited in the latest SA build.
        // Recheck incoming branches across the actual overwritten ranges when
        // changing these sites; successful relocation alone is insufficient.
        // Validate the entire group before changing any camera code.
        auto aimGain = hook::pattern("0F 28 C7 F3 0F 58 07 41 0F 28 CA F3 0F 58 4F 10 F3 0F 11 07 F3 0F 11 4F").get_first();
        auto samplePed = hook::pattern("F3 44 0F 11 9D A0 00 00 00 4D 85 ED 74 ? 49 8B 4D 30 48 85 C9 74 ? 48").get_first();
        auto betaPed = hook::pattern("F3 0F 11 4B 14 0F 54 CB 0F 2F 0D ? ? ? ? 73 ? 89 73 14 66 83 3D ?").get_first();
        auto alphaPed = hook::pattern("F3 0F 58 33 F3 0F 10 7D 88 0F 2F F7 F3 0F 11 33 76 ? F3 0F 11 3B EB ?").get_first();
        auto sourcePed = hook::pattern("F3 0F 10 0D ? ? ? ? F3 0F 10 25 ? ? ? ? 0F 28 C1 F3 0F 10 1D ?").get_first();
        auto controllerPedPitch = hook::pattern("F3 44 0F 58 E7 E8 ? ? ? ? 4C 8B AC 24 68 01 00 00 48 85 C0 74 ? F6").get_first();
        auto beginCar = hook::pattern("83 3D ? ? ? ? 03 41 0F 28 FD 75 ? 48 8B 4C 24 68 33 C0 89 44 24 28").get_first();
        auto sampleCar = hook::pattern("F3 0F 59 3D ? ? ? ? 41 0F 2F FD 76 ? F3 0F 59 3D ? ? ? ? 48 8B").get_first();
        auto taskCar = hook::pattern("F3 41 0F 59 BC 1F ? ? ? ? E8 ? ? ? ? F3 45 0F 10 84 1F ? ? ?").get_first();
        auto alphaCar = hook::pattern("F3 0F 58 36 F3 41 0F 59 C0 41 0F 2F F2 F3 0F 11 36 F3 44 0F 58 F0 76 ?").get_first();
        auto sourceCar = hook::pattern("F3 0F 10 46 10 0F 57 C9 F3 41 0F 10 CA F3 44 0F 11 15 ? ? ? ? E8 ?").get_first();
        auto betaCar = hook::pattern("48 8B C4 48 81 EC A8 00 00 00 83 3D ? ? ? ? 00 44 0F 29 40 C8 45 0F").get_first();
        if (!aimGain || !samplePed || !betaPed || !alphaPed || !sourcePed || !controllerPedPitch || !beginCar
            || !sampleCar || !taskCar || !alphaCar || !sourceCar || !betaCar)
            return;
        aimHorizontal = reinterpret_cast<AimAxis>(hook::pattern("48 89 5C 24 20 55 41 54 41 55 41 56 41 57 48 8B EC 48 81 EC 80 00 00 00").get_first());
        disabledControls = reinterpret_cast<uint16_t*>((injector::ReadRelativeOffset(hook::pattern("66 44 09 15 ? ? ? ? E8 ? ? ? ? 44 38 35 ? ? ? ? 4D 8B D6 4C").get_first<uint8_t>(4)).as_int()));
        timeStep = reinterpret_cast<float*>((injector::ReadRelativeOffset(hook::pattern("F3 44 0F 10 05 ? ? ? ? 48 8D 05 ? ? ? ? F3 0F 10 3D ? ? ? ?").get_first<uint8_t>(5)).as_int()));
        direction = reinterpret_cast<int*>((injector::ReadRelativeOffset(hook::pattern("8B 05 ? ? ? ? 89 47 3C 0F B6 05 ? ? ? ? 48 69 C8 B8 01 00 00 48").get_first<uint8_t>(2)).as_int()));
        scriptAngles = reinterpret_cast<bool*>((injector::ReadRelativeOffset(hook::pattern("C6 05 ? ? ? ? 00 E9 ? ? ? ? 48 8B CF E8 ? ? ? ? E9 ? ? ?").get_first<uint8_t>(2)).as_int() + 1));
        carParameters = reinterpret_cast<const float*>((injector::ReadRelativeOffset(hook::pattern("F3 0F 59 35 ? ? ? ? F3 44 0F 10 44 24 38 F3 44 0F 10 5C 24 34 F3 0F").get_first<uint8_t>(4)).as_int()));
        carBetaHook = safetyhook::create_inline(betaCar, ComputeCarBeta);
        if (!carBetaHook)
            return;
        const auto add = [](void* address, auto callback)
        {
            orbitHooks.push_back(safetyhook::create_mid(address, callback));
        };
        add(aimGain, [](SafetyHookContext& c)
        {
            if (MouseOrbitAllowed())
            {
                // Normal third-person weapon aim uses the free-look mouse
                // gains at its 70-degree reference FOV. Weapon zoom remains
                // visual; it must not introduce an additional mouse slowdown.
                // Dedicated scope cameras do not execute this integration.
                c.xmm7.f32[0] = c.xmm8.f32[0] * (25.0f / 6.0f) * 0.875f;
                c.xmm10.f32[0] = c.xmm12.f32[0] * 3.125f * 0.875f;
            }
        });
        add(samplePed, [](SafetyHookContext& c)
        {
            ped = {reinterpret_cast<CameraAngles*>(c.rbx), c.xmm10.f32[0],
                c.xmm11.f32[0], MouseOrbitAllowed() && *direction == 3};
            if (!*disabledControls && !*scriptAngles && *direction == 3)
            {
                const auto x = OriginalControllerOrbit(ped.x, true, ped.camera->fov);
                const auto y = OriginalControllerOrbit(ped.y, false, ped.camera->fov);
                if (x && y)
                {
                    ped.controller = true;
                    c.xmm10.f32[0] = ped.x = *x;
                    c.xmm11.f32[0] = ped.y = *y;
                }
            }
        });
        add(betaPed, [](SafetyHookContext& c)
        {
            // Task handlers can replace either input axis. Preserve their
            // camera updates instead of treating those values as mouse input.
            ped.eligible = ped.eligible && ped.camera == reinterpret_cast<CameraAngles*>(c.rbx)
                && c.xmm10.f32[0] == ped.x && c.xmm11.f32[0] == ped.y;
            ped.controller = ped.controller && ped.camera == reinterpret_cast<CameraAngles*>(c.rbx)
                && c.xmm10.f32[0] == ped.x && c.xmm11.f32[0] == ped.y;
            if (ped.controller)
            {
                ped.betaSpeed = c.xmm1.f32[0];
                ped.camera->beta += std::max(0.0f, *timeStep) * ped.betaSpeed;
                c.xmm1.f32[0] = c.xmm10.f32[0] = 0.0f;
            }
            if (ped.eligible)
            {
                ped.camera->beta += ped.x * 3.125f * FovScale(*ped.camera);
                c.xmm1.f32[0] = 0.0f;
                c.xmm10.f32[0] = 0.0f; // the forward/look branch must not add it again
            }
        });
        add(alphaPed, [](SafetyHookContext& c)
        {
            if (ped.controller)
                ped.camera->betaSpeed = ped.betaSpeed;
            if (ped.eligible)
            {
                c.xmm6.f32[0] = ped.y * (25.0f / 6.0f) * FovScale(*ped.camera);
                ped.camera->alphaSpeed = 0.0f;
            }
        });
        add(controllerPedPitch, [](SafetyHookContext& c)
        {
            if (ped.controller)
                c.xmm7.f32[0] *= std::max(0.0f, *timeStep);
        });
        add(sourcePed, [](SafetyHookContext& c)
        {
            if (ped.eligible)
                c.xmm12.f32[0] = c.xmm7.f32[0]; // pitch after native angle limits
        });
        add(beginCar, [](SafetyHookContext& c)
        {
            car.camera = reinterpret_cast<CameraAngles*>(c.rsi);
            car.sampled = car.eligible = car.applied = false;
            car.parameters = nullptr;
            car.hold = fmaxf(0.0f, car.hold - fmaxf(0.0f, *timeStep) * 0.02f);
        });
        add(sampleCar, [](SafetyHookContext& c)
        {
            car.y = c.xmm0.f32[0];
            car.sampled = MouseOrbitAllowed() && *direction == 3;
            if (!*disabledControls && !*scriptAngles && *direction == 3)
                if (const auto speed = OriginalControllerOrbit(car.y, false, car.camera->fov))
                    c.xmm0.f32[0] = *speed / 0.7f;
        });
        add(taskCar, [](SafetyHookContext& c)
        {
            const auto base = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
            if (c.rbx == base && c.r15 <= 360 && c.r15 % 60 == 0)
                car.parameters = reinterpret_cast<const float*>(reinterpret_cast<uintptr_t>(carParameters) + c.r15);
            const auto expected = car.y * (car.y > 0.0f ? 0.35f : 0.7f);
            // Native code multiplies by 0.7 and then 0.5; tolerate the
            // rounding difference from the equivalent combined coefficient.
            car.eligible = car.sampled && fabsf(c.xmm7.f32[0] - expected)
                <= std::max(1.0e-8f, fabsf(expected) * 1.0e-6f);
            if (car.eligible && car.y != 0.0f)
                car.hold = Settings.centeringDelay;
        });
        add(alphaCar, [](SafetyHookContext& c)
        {
            // DE fixes controller pitch integration to 1.66 per frame.
            // Original SA uses the actual simulation timestep.
            if (OriginalControllerOrbit(0.0f, false, car.camera->fov))
                c.xmm0.f32[0] = std::max(0.0f, *timeStep);
            car.applied = car.eligible && (car.y != 0.0f || car.hold > 0.0f);
            if (car.applied)
            {
                c.xmm6.f32[0] = car.y * (10.0f / 3.0f) * FovScale(*car.camera);
                c.xmm14.f32[0] = car.camera->alpha + c.xmm6.f32[0];
                c.xmm8.f32[0] = 0.0f; // no timestep-dependent mouse turn speed
                car.camera->alphaSpeed = 0.0f;
            }
        });
        add(sourceCar, [](SafetyHookContext& c)
        {
            if (car.applied)
                c.xmm14.f32[0] = c.xmm10.f32[0];
        });
        if (std::any_of(orbitHooks.begin(), orbitHooks.end(), [](const auto& hook) { return !hook; }))
        {
            orbitHooks.clear();
            carBetaHook = {};
        }
    }
}

class CameraModule
{
public:
    CameraModule()
    {
        WFP::onInitEvent() += []()
        {
            if (Settings.improveCameraPC)
                InstallMouseOrbit();
        };
        WFP::onGameInitEvent() += []() { ped = {}; car = {}; };
    }
} CameraModuleInstance;
