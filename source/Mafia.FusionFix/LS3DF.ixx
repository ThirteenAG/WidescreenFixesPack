module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <cmath>
#include <numbers>
#include <stdexcept>

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
        corrected = std::clamp(corrected * factor, 0.005, std::numbers::pi * (75.0 / 180.0));
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
    struct FovMatch
    {
        uintptr_t load;
        uintptr_t multiply;
        uint8_t cameraRegister;
        uint32_t field;
    };
    std::vector<FovMatch> matches;
    const auto collect = [&](const char* signature, size_t count, size_t multiplyOffset, uint8_t cameraRegister, uint32_t field)
    {
        auto pattern = hook::module_pattern(module, signature);
        if (pattern.size() != count)
            throw std::runtime_error("FOV signatures not found");
        for (size_t i = 0; i < count; ++i)
        {
            auto load = reinterpret_cast<uintptr_t>(pattern.get(i).get<void>());
            matches.push_back({ load, load + multiplyOffset, cameraRegister, field });
        }
    };
    auto oldVersion = hook::module_pattern(module, "D9 85 40 01 00 00 B9 10 00 00 00 D8 0D ? ? ? ? 8D 95 64 03 00 00 33 C0 D9 C0 D9 FF D9 C9 D9 FE");
    if (!oldVersion.empty()) // 1.0 has separate projection sites
    {
        collect("D9 85 40 01 00 00 B9 10 00 00 00 D8 0D ? ? ? ? 8D 95 64 03 00 00 33 C0 D9 C0 D9 FF D9 C9 D9 FE", 1, 11, 5, 0x140);
        collect("D9 83 40 01 00 00 8B F2 D8 0D ? ? ? ? B9 10 00 00 00 33 C0 D9 C0 D9 FF D9 C9 D9 FE", 3, 8, 3, 0x140);
        collect("D9 83 40 01 00 00 33 C0 D8 0D ? ? ? ? D9 C0 D9 FF D9 C9 D9 FE", 2, 8, 3, 0x140);
        collect("D9 82 40 01 00 00 B9 10 00 00 00 D8 0D ? ? ? ? 8B F3 33 C0 D9 C0 D9 FF D9 C9 D9 FE", 2, 11, 2, 0x140);
        collect("D9 85 4C 01 00 00 D8 0D ? ? ? ? D9 C0 D9 FF D9 C9 D9 FE", 1, 6, 5, 0x14C);
        collect("D9 82 4C 01 00 00 D8 0D ? ? ? ? D9 C0 D9 FF D9 C9 D9 FE", 1, 6, 2, 0x14C);
        collect("D9 83 4C 01 00 00 D8 0D ? ? ? ? 5F D9 C0 5E D9 FF D9 C9 D9 FE", 5, 6, 3, 0x14C);
        collect("D9 82 4C 01 00 00 D8 0D ? ? ? ? 5F D9 C0 D9 FF D9 C9 D9 FE", 1, 6, 2, 0x14C);
    }
    else // 1.1 and 1.2 share the projection sites
    {
        collect("D9 82 40 01 00 00 D8 0D ? ? ? ? D9 C0 D9 FF D9 C9 D9 FE", 1, 6, 2, 0x140);
        collect("D9 82 4C 01 00 00 D8 0D ? ? ? ? D9 C0 5F D9 FF D9 C9 D9 FE", 1, 6, 2, 0x14C);
    }
    constexpr auto callbacks = FovCallbacks(std::make_index_sequence<16>{});
    for (size_t i = 0; i < matches.size(); ++i)
    {
        const auto& [load, multiply, cameraRegister, field] = matches[i];
        auto& site = fovSites[i];
        site.cameraRegister = cameraRegister;
        site.field = field;
        site.hook = safetyhook::create_mid(load, callbacks[i]);
        injector::WriteMemory<float*>(multiply + 2, &site.multiplier, true);
    }
    // Called for the active camera on every scene render, before the renderer
    // checks its revision. Projection setters alone miss cutscene transitions.
    auto pCamera = find_module_pattern(module,
        "56 8B F1 8B 86 F4 03 00 00 85 C0 8A 86 AC 00 00 00 74 ? A8 20");
    {
        static auto CameraFovHook = safetyhook::create_mid(pCamera.get_first(), RefreshCameraFov);
    }
}

export void InitMovie()
{
    auto module = GetLS3DF();
    auto pattern = find_module_pattern(module,
        "8B 4C 24 14 8B 54 24 00 8D 41 28 89 10 8B 54 24 04",
        "8B 4C 24 14 8B 54 24 00 8D 41 2C 89 10 8B 54 24 04");
    static auto MoviePositionHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        auto& rect = Field<RECT>(regs.esp, 0);
        const float scaleX = Screen.fWidth / 4.0f;
        const float scaleY = Screen.fHeight / 3.0f;
        const float scale = std::min(scaleX, scaleY);
        const float width = scale * 4.0f, height = scale * 3.0f;
        rect.left = static_cast<LONG>(std::nearbyint((Screen.fWidth - width) * 0.5f));
        rect.top = static_cast<LONG>(std::nearbyint((Screen.fHeight - height) * 0.5f));
        rect.right = static_cast<LONG>(std::nearbyint(width));
        rect.bottom = static_cast<LONG>(std::nearbyint(height));
    });
}
