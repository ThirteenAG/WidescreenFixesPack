module;

#include "stdafx.h"

export module ClassicOrbit;

import Settings;
import Input;
import Controller;

export struct ClassicOrbitSites
{
    std::array<void*, 12> instructions;
    const uint8_t* forceBehind;
    const float* desiredPedPitch;
    float* carPitchHold;
    const float* timeStep;
    bool carCameraInRsi = false;
};

namespace
{
    ClassicOrbitSites sites{};
    std::vector<SafetyHookMid> hooks;
    std::map<uintptr_t, std::array<float, 2>> controllerSpeeds;
    bool MouseOrbitAllowed()
    {
        return IsMouseCameraActive() && !*sites.forceBehind;
    }
    // Original SA filters stick motion; raw mouse orbit bypasses this filter.
    std::optional<float> ControllerTurn(uintptr_t camera, float native, bool horizontal, bool vehicle)
    {
        if (*sites.forceBehind)
            return std::nullopt;
        const auto speed = OriginalControllerOrbit(native, horizontal, *reinterpret_cast<const float*>(camera + 8));
        if (!speed)
            return std::nullopt;
        auto& filtered = controllerSpeeds[camera];
        if (*reinterpret_cast<const bool*>(camera + 37))
            filtered = {};
        const auto step = std::max(0.0f, *sites.timeStep);
        const auto damping = powf(vehicle ? 0.85f : 0.8f, step);
        const auto limit = vehicle ? 0.2f : 0.1f;
        auto& axis = filtered[horizontal ? 0 : 1];
        axis = damping * axis + (1.0f - damping) * std::clamp(*speed * (vehicle ? 0.8f : 1.0f), -limit, limit);
        if (fabsf(axis) < 0.0001f)
            axis = 0.0f;
        return step * axis;
    }
}

// Hook sites were audited against native overwritten ranges for incoming branches.
// Recheck those ranges when changing a signature or hook site; relocation alone
// does not rule out a branch entering the overwritten instruction sequence.
export void InstallClassicOrbit(const ClassicOrbitSites& verified)
{
    if (std::any_of(verified.instructions.begin(), verified.instructions.end(), [](auto p) { return !p; }))
        return;
    sites = verified;
    const auto add = [](size_t index, auto callback)
    {
        hooks.push_back(safetyhook::create_mid(sites.instructions[index], callback));
    };
    // Keep native centering for controllers and explicit camera-behind actions.
    add(0, [](SafetyHookContext& c)
    {
        if (MouseOrbitAllowed())
            c.xmm6.f32[0] = 0.0f;
    });
    add(1, [](SafetyHookContext& c)
    {
        if (MouseOrbitAllowed())
            c.xmm3.f32[0] = *sites.desiredPedPitch;
    });
    // These native mouse branches already apply FOV, inversion and sensitivity.
    // Normalize their different X/Y coefficients to SA's ped orbit gains.
    add(2, [](SafetyHookContext& c)
    {
        if (MouseOrbitAllowed())
            c.xmm6.f32[0] *= 3.125f;
    });
    add(3, [](SafetyHookContext& c)
    {
        if (MouseOrbitAllowed())
            c.xmm0.f32[0] *= 25.0f / 6.0f;
    });
    add(4, [](SafetyHookContext& c) { c.xmm1.f32[0] = Settings.centeringDelay; });
    add(5, [](SafetyHookContext&) { *sites.carPitchHold = Settings.centeringDelay; });
    add(6, [](SafetyHookContext&) { *sites.carPitchHold = Settings.centeringDelay; });
    add(7, [](SafetyHookContext& c)
    {
        if (MouseOrbitAllowed())
        {
            // Eliminate the DE velocity-dependent mouse multiplier.
            c.xmm0.f32[0] = 0.0f;
            c.xmm7.f32[0] *= 2.5f;
        }
        else if (const auto turn = ControllerTurn(sites.carCameraInRsi ? c.rsi : c.rdi, c.xmm7.f32[0], true, true))
        {
            c.xmm0.f32[0] = 0.0f;
            c.xmm7.f32[0] = *turn;
        }
    });
    add(8, [](SafetyHookContext& c)
    {
        if (MouseOrbitAllowed())
            c.xmm6.f32[0] *= 5.0f;
    });
    add(9, [](SafetyHookContext& c)
    {
        if (const auto turn = ControllerTurn(c.rbx, c.xmm6.f32[0] * c.xmm0.f32[0], true, false))
        {
            c.xmm6.f32[0] = *turn;
            c.xmm0.f32[0] = 1.0f;
        }
    });
    add(10, [](SafetyHookContext& c)
    {
        if (const auto turn = ControllerTurn(c.rbx, c.xmm3.f32[0], false, false))
            c.xmm3.f32[0] = *turn;
    });
    add(11, [](SafetyHookContext& c)
    {
        if (const auto turn = ControllerTurn(c.rsi, c.xmm6.f32[0], false, true))
            c.xmm6.f32[0] = *turn;
    });
    if (std::any_of(hooks.begin(), hooks.end(), [](const auto& hook) { return !hook; }))
        hooks.clear();
    WFP::onGameInitEvent() += []() { controllerSpeeds.clear(); };
}
