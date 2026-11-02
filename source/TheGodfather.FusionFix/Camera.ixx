module;

#include <stdafx.h>
#include "CameraMath.hpp"
#include "FixSettings.hpp"
#include "FloatingPoint.hpp"
#include <atomic>
#include <stdexcept>

export module Camera;

import Cinematics;

export namespace GodfatherCamera
{
    SafetyHookInline setupHook, angleHook;
    void* chaseTable = nullptr;
    void* initializationReturn = nullptr;
    std::atomic<float> aspect{ 4.0f / 3.0f }, multiplier{ 1.0f };

    template <typename T> T Read(const void* object, size_t offset)
    {
        return *reinterpret_cast<const T*>(static_cast<const uint8_t*>(object) + offset);
    }

    bool Gameplay(void* manager, int index, float authored)
    {
        if (!manager || index < 0 || index >= Read<int>(manager, 84) || Read<int>(manager, 48))
            return false;
        const auto slots = Read<uint8_t*>(manager, 12);
        if (!slots)
            return false;
        const auto slot = slots + 424 * index;
        const int active = Read<int>(slot, 248);
        if (active < 0 || active >= 12)
            return false;
        for (int i = 0; i < 12; ++i)
        {
            const auto camera = Read<void*>(slot, 104 + 12 * i);
            const auto weight = Read<float>(slot, 108 + 12 * i);
            if (!std::isfinite(weight) || (i == active ? weight < 0.9999f : weight != 0.0f))
                return false;
            if (i == active && (!camera || Read<void*>(camera, 0) != chaseTable))
                return false;
        }
        // The chase camera also owns temporary authored zooms.
        return std::isfinite(authored) && std::abs(authored - 1.0471976f) < 0.0001f;
    }

    float Effective(void* manager, int index, float authored)
    {
        return Horizontal(authored, GodfatherCinematics::Aspect(aspect.load(std::memory_order_relaxed)),
            multiplier.load(std::memory_order_relaxed), !GodfatherCinematics::Active() && Gameplay(manager, index, authored));
    }

    int __fastcall Setup(void* manager, void*, int index, const void* position, const void* angles, float authored)
    {
        Godfather::FloatingPointScope floatingPoint;
        return setupHook.unsafe_thiscall<int>(manager, index, position, angles, Effective(manager, index, authored));
    }

    double __fastcall Angle(void* manager, void*, int index)
    {
        const auto caller = _ReturnAddress();
        Godfather::FloatingPointScope floatingPoint;
        const auto authored = angleHook.unsafe_thiscall<double>(manager, index);
        // Camera construction copies its own unadjusted lens for later interpolation.
        return caller == initializationReturn ? authored : Effective(manager, index, float(authored));
    }

    void Init()
    {
        auto setup = hook::pattern("83 EC 40 56 57 8D 44 24 08 50 8B 44 24 58 8B 10 8B F1 8B 48 08");
        auto angle = hook::pattern("8B 41 30 85 C0 74 0C D9 41 4C D8 0D ? ? ? ? C2 04 00 8B 44 24 04 8B 49 0C 69 C0 A8 01 00 00 D9 44 08 1C C2 04 00");
        auto chase = hook::pattern("8D 4E 78 C7 06 ? ? ? ? E8 ? ? ? ? 33 C0 89 86 B4 01 00 00");
        auto initialize = hook::pattern("E8 ? ? ? ? D9 9E E8 01 00 00 5F 5E 5D 5B C2 04 00");
        chaseTable = *chase.get_first<void**>(5);
        initializationReturn = initialize.get_first(5);
        multiplier.store(FixSettings().Float("MAIN.FOVFactor"), std::memory_order_relaxed);
        FixSettings().OnChange("MAIN.FOVFactor", []()
        {
            multiplier.store(FixSettings().Float("MAIN.FOVFactor"), std::memory_order_relaxed);
        });
        angleHook = safetyhook::create_inline(angle.get_first(), Angle, safetyhook::InlineHook::StartDisabled);
        setupHook = safetyhook::create_inline(setup.get_first(), Setup, safetyhook::InlineHook::StartDisabled);
        if (!angleHook || !setupHook)
        {
            angleHook = {};
            setupHook = {};
            throw std::runtime_error("Camera hooks unavailable");
        }
        if (!angleHook.enable() || !setupHook.enable())
        {
            setupHook = {};
            angleHook = {};
            throw std::runtime_error("Camera hooks could not be enabled");
        }
    }
}
