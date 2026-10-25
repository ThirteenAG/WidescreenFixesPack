#pragma once
#include <numbers>
#include "Cutscene.hpp"

namespace ScarfaceCamera
{
    SafetyHookMid ProjectionHook, SniperHook, LockHook, AimHook;
    const float* Factor = nullptr;
    const uint8_t* Character = nullptr;
    bool Aiming = false;

    float Adjust(const uint8_t* manager, const uint8_t* source, float fov)
    {
        if (!manager || !source || !std::isfinite(fov) || fov <= 0.0f || fov >= std::numbers::pi_v<float>)
            return fov;
        const int state = *reinterpret_cast<const int*>(manager + 232);
        if ((state != 0 && state != 2 && state != 5 && state != 6) || manager[254])
            return fov;
        auto object = *reinterpret_cast<uint8_t* const*>(manager + 196 + state * 4);
        if (!object || *reinterpret_cast<uint8_t* const*>(object + 132) != source)
            return fov;
        const int type = *reinterpret_cast<const int*>(object + 128);
        if (state == 0 || state == 2)
        {
            if (type != 1 || object != Character || Aiming)
                return fov;
            const float normal = *reinterpret_cast<const float*>(object + (object[564] ? 368 : 356));
            const float degrees = fov * (180.0f / std::numbers::pi_v<float>);
            if (!std::isfinite(normal) || std::abs(degrees - normal) > 0.05f)
                return fov;
        }
        else if (type != 2)
        {
            return fov;
        }
        const float factor = Factor && std::isfinite(*Factor) ? std::clamp(*Factor, 0.5f, 2.0f) : 1.0f;
        if (factor == 1.0f)
            return fov;
        return std::clamp(fov * factor, 5.0f * (std::numbers::pi_v<float> / 180.0f),
            170.0f * (std::numbers::pi_v<float> / 180.0f));
    }

    void ResetHooks()
    {
        ProjectionHook.reset();
        SniperHook.reset();
        LockHook.reset();
        AimHook.reset();
        Character = nullptr;
        Aiming = false;
    }

    void Install(const float& factor, HMODULE module = GetModuleHandle(nullptr))
    {
        auto projection = hook::module_pattern(module, "8B 47 10 8B 8E B0 00 00 00 D9 41 10 89 44 24 10 D9 44 24 10 DA E9 DF E0 F6 C4 44 7B 0F 8B 44 24 10 8B 11 68 00 00 80 3F 50 FF 52 1C");
        auto sniper = hook::module_pattern(module, "57 8B 7C 24 18 8B CF E8 ? ? ? ? 84 C0 74 0B 8B 45 38 89 44 24 10 B3 01");
        auto locked = hook::module_pattern(module, "8B CF E8 ? ? ? ? 84 C0 74 1D 80 BE 34 02 00 00 00 74 08 8B 8E 78 01 00 00 EB 32");
        auto aim = hook::module_pattern(module, "8B CF E8 ? ? ? ? 84 C0 74 1F 80 BE 34 02 00 00 00 74 0C 8B 86 74 01 00 00");
        if (projection.size() != 1 || sniper.size() != 1 || locked.size() != 1 || aim.size() != 1)
        {
            OutputDebugStringA("Scarface: gameplay FOV patterns unavailable.\n");
            return;
        }
        Factor = &factor;
        SniperHook = safetyhook::create_mid(sniper.get_first(12), [](SafetyHookContext& regs)
        {
            Character = reinterpret_cast<const uint8_t*>(regs.esi);
            Aiming = (regs.eax & 0xFF) != 0;
        });
        const auto captureAim = [](SafetyHookContext& regs)
        {
            if (Character == reinterpret_cast<const uint8_t*>(regs.esi) && (regs.eax & 0xFF))
                Aiming = true;
        };
        LockHook = safetyhook::create_mid(locked.get_first(7), captureAim);
        AimHook = safetyhook::create_mid(aim.get_first(7), captureAim);
        ProjectionHook = safetyhook::create_mid(projection.get_first(16), [](SafetyHookContext& regs)
        {
            auto& fov = *reinterpret_cast<float*>(regs.esp + 16);
            const auto manager = reinterpret_cast<const uint8_t*>(regs.esi);
            const auto source = reinterpret_cast<const uint8_t*>(regs.edi);
            fov = ScarfaceCutscene::Adjust(manager, source, Adjust(manager, source, fov));
        });
        if (!ProjectionHook || !SniperHook || !LockHook || !AimHook)
        {
            ResetHooks();
            OutputDebugStringA("Scarface: gameplay FOV hooks unavailable.\n");
        }
    }
}
