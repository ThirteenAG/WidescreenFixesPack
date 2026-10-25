module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <cmath>
#include <numbers>

export module LS3DF;

import ComVars;
import Cutscene;

struct FovSite
{
    SafetyHookMid hook;
    uint8_t cameraRegister;
    uint32_t field;
    float multiplier = 0.5f;
};
std::array<FovSite, 16> fovSites;

double CameraHalfAngle(uintptr_t camera, double angle)
{
    double corrected = std::atan(std::tan(angle * 0.5) * GetCameraFovScale());
    if (GameplayFovEnabled && camera == GameplayCamera && !IsCutsceneCamera())
    {
        const float value = FixSettings().Float("MAIN.FOVFactor");
        const float factor = std::isfinite(value) ? std::clamp(value, 0.5f, 2.0f) : 1.0f;
        if (factor != 1.0f)
            corrected = std::clamp(corrected * factor, 0.005, (std::numbers::pi - 0.01) * 0.5);
    }
    return corrected;
}

template<size_t I> void CorrectFov(SafetyHookContext& regs)
{
    auto& site = fovSites[I];
    uintptr_t camera = site.cameraRegister == 5 ? regs.ebp : site.cameraRegister == 3 ? regs.ebx : regs.edx;
    double angle = Field<float>(camera, site.field);
    // Change the existing half-angle multiplier before the native FLD. This
    // leaves the x87 stack, camera FOV fields, and projection code intact.
    site.multiplier = angle == 0.0 ? 0.5f : static_cast<float>(
        CameraHalfAngle(camera, angle) / angle);
}

template<size_t... I> constexpr auto FovCallbacks(std::index_sequence<I...>)
{
    return std::array{&CorrectFov<I>...};
}

void RefreshCameraFov(SafetyHookContext& regs)
{
    auto camera = regs.ecx;
    if (Field<uint8_t>(camera, 0x15C)) return; // orthographic camera
    for (auto [field, projection] : { std::pair{0x140u, 0x1A4u}, std::pair{0x14Cu, 0x364u} })
    {
        double angle = Field<float>(camera, field);
        if (!std::isfinite(angle) || angle <= 0.0 || angle >= std::numbers::pi) return;
        double expected = 1.0 / std::tan(CameraHalfAngle(camera, angle));
        double cached = Field<float>(camera, projection);
        if (std::isfinite(cached) && std::abs(cached - expected) <= expected * 0.00001) continue;

        // SetFOV rebuilds both native projection matrices and advances the
        // camera's revision. Update also rebuilds its view matrices and frustum
        // before Render can use caches prepared earlier in the same frame.
        // A gameplay camera prepared during the intro must not retain that
        // cutscene projection after the bars are disabled (or vice versa).
        auto table = *reinterpret_cast<uintptr_t**>(camera);
        using SetFOV = void(__stdcall*)(void*, float);
        reinterpret_cast<SetFOV>(table[0x50 / sizeof(uintptr_t)])(
            reinterpret_cast<void*>(camera), Field<float>(camera, 0x140));
        using Update = void(__thiscall*)(void*);
        reinterpret_cast<Update>(table[0x44 / sizeof(uintptr_t)])(reinterpret_cast<void*>(camera));
        return;
    }
}

export void InitFOV()
{
    auto module = GetLS3DF();
    if (!module) return;
    std::vector<std::pair<uintptr_t, uintptr_t>> matches;
    // 1.0 duplicates projection calculations in eight camera methods; later
    // engines have a shared method. Find both FOV fields in either layout.
    for (auto bytes : {"D9 85 40 01 00 00", "D9 83 40 01 00 00", "D9 82 40 01 00 00",
                       "D9 85 4C 01 00 00", "D9 83 4C 01 00 00", "D9 82 4C 01 00 00"})
    {
        auto pattern = find_module_pattern<UINT32_MAX>(module, bytes);
        for (size_t i = 0; i < pattern.size(); ++i)
        {
            auto load = reinterpret_cast<uintptr_t>(pattern.get(i).get<void>());
            for (size_t offset : {6u, 8u, 11u})
            {
                auto multiply = load + offset;
                if (Field<uint16_t>(multiply, 0) != 0x0DD8) continue;
                auto* constant = Field<float*>(multiply, 2);
                if (*constant != 0.5f) continue;
                bool cosine = false, sine = false;
                for (size_t j = 6; j < 22; ++j)
                {
                    cosine |= Field<uint16_t>(multiply, j) == 0xFFD9;
                    sine |= Field<uint16_t>(multiply, j) == 0xFED9;
                }
                if (cosine && sine) matches.emplace_back(load, multiply);
            }
        }
    }
    if (matches.size() != 2 && matches.size() != 16)
    {
        OutputDebugStringA("Mafia: camera projection patterns unavailable.\n");
        return;
    }
    constexpr auto callbacks = FovCallbacks(std::make_index_sequence<16>{});
    for (size_t i = 0; i < matches.size(); ++i)
    {
        auto [load, multiply] = matches[i];
        auto& site = fovSites[i];
        site.cameraRegister = Field<uint8_t>(load, 1) & 7;
        site.field = Field<uint32_t>(load, 2);
        site.hook = safetyhook::create_mid(load, callbacks[i]);
        if (site.hook) ProtectedGameRef<float*>{reinterpret_cast<float**>(multiply + 2)} = &site.multiplier;
    }
    // Called for the active camera on every scene render, before the renderer
    // checks its revision. Projection setters alone miss cutscene transitions.
    auto pCamera = find_module_pattern(module,
        "56 8B F1 8B 86 F4 03 00 00 85 C0 8A 86 AC 00 00 00 74 ? A8 20");
    if (pCamera.size() == 1)
    {
        static auto CameraFovHook = safetyhook::create_mid(pCamera.get_first(), RefreshCameraFov);
    }
    else
        OutputDebugStringA("Mafia: active camera refresh pattern unavailable.\n");
}

export void InitMovie()
{
    auto module = GetLS3DF();
    if (!module) return;
    auto pattern = find_module_pattern(module,
        "8B 4C 24 14 8B 54 24 00 8D 41 28 89 10 8B 54 24 04",
        "8B 4C 24 14 8B 54 24 00 8D 41 2C 89 10 8B 54 24 04");
    if (pattern.size() != 1)
    {
        OutputDebugStringA("Mafia: movie position pattern unavailable.\n");
        return;
    }
    static auto MoviePositionHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        auto& rect = Field<RECT>(regs.esp, 0);
        float width = Screen.fWidth, height = Screen.fHeight;
        if (nFMVWidescreenMode != 2)
        {
            const float scaleX = Screen.fWidth / 4.0f;
            const float scaleY = Screen.fHeight / 3.0f;
            const float scale = nFMVWidescreenMode == 1 ? std::max(scaleX, scaleY) : std::min(scaleX, scaleY);
            width = scale * 4.0f;
            height = scale * 3.0f;
        }
        rect.left = static_cast<LONG>(std::nearbyint((Screen.fWidth - width) * 0.5f));
        rect.top = static_cast<LONG>(std::nearbyint((Screen.fHeight - height) * 0.5f));
        rect.right = static_cast<LONG>(std::nearbyint(width));
        rect.bottom = static_cast<LONG>(std::nearbyint(height));
    });
}
