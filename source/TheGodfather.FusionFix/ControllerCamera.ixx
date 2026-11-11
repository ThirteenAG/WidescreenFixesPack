module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "FloatingPoint.hpp"
#include <stdexcept>

export module ControllerCamera;
import Input;
import ComVars;

export namespace GodfatherControllerCamera
{
    SafetyHookInline updateHook, axesHook, curveHook;
    std::atomic<float> speed{ 1.0f };
    std::atomic<bool> invertY{ false };
    thread_local unsigned updating = 0;

    struct Pending
    {
        float* x{};
        float* y{};
        float multiplier = 1.0f;
    };
    thread_local Pending pending;

    bool Pad(unsigned slot)
    {
        auto input = GodfatherInputSlot.is_initialized() ? GodfatherInputSlot.get() : nullptr;
        if (!input)
            return false;
        const auto bytes = static_cast<const uint8_t*>(input);
        if (slot == 8)
            slot = 0;
        else if (slot < 4 && bytes[56])
            slot = bytes[988 + slot];
        if (slot >= 5)
            return false;
        return bytes[64 + 132 * slot + 124] != 0;
    }

    void Scale(float* x, float* y, float multiplier, bool invert)
    {
        if (!x || !y)
            return;
        if (!std::isfinite(multiplier))
            multiplier = 1.0f;
        multiplier = std::clamp(multiplier, 0.1f, 5.0f);
        const auto horizontal = *x;
        const auto vertical = *y;
        if (std::isfinite(horizontal))
            *x = horizontal * multiplier;
        if (std::isfinite(vertical))
            *y = vertical * (invert ? -multiplier : multiplier);
    }

    bool MouseMotion()
    {
        auto input = GodfatherInputSlot.is_initialized() ? GodfatherInputSlot.get() : nullptr;
        if (!input)
            return false;
        const auto bytes = static_cast<const uint8_t*>(input);
        const auto x = *reinterpret_cast<const float*>(bytes + 1276);
        const auto y = *reinterpret_cast<const float*>(bytes + 1280);
        return bytes[1349] && (x != 0.0f || y != 0.0f);
    }

    void* __fastcall Axes(void* self, void*, int pair, float* x, float* y, float horizontalDeadzone, float verticalDeadzone)
    {
        pending = {};
        const auto result = axesHook.unsafe_thiscall<void*>(self, pair, x, y, horizontalDeadzone, verticalDeadzone);
        const auto multiplier = speed.load(std::memory_order_relaxed);
        const auto invert = invertY.load(std::memory_order_relaxed);
        if (updating && pair == 1 && (multiplier != 1.0f || invert))
        {
            const auto slot = *reinterpret_cast<const int*>(static_cast<const uint8_t*>(self) + 60);
            if (slot >= 0 && slot <= 8 && Pad(unsigned(slot)) && !MouseMotion())
            {
                Godfather::FloatingPointScope floatingPoint;
                if (invert)
                    Scale(x, y, 1.0f, true);
                pending = { x, y, multiplier };
            }
        }
        return result;
    }

    void __stdcall Curve(float* x, float* y, float linear, float power)
    {
        const auto input = pending;
        pending = {};
        curveHook.unsafe_stdcall<void>(x, y, linear, power);
        if (updating && input.x == x && input.y == y && input.multiplier != 1.0f)
        {
            Godfather::FloatingPointScope floatingPoint;
            Scale(x, y, input.multiplier, false);
        }
    }

    int __fastcall Update(void* self, void*)
    {
        struct Scope
        {
            Pending previous = pending;
            Scope() { ++updating; pending = {}; }
            ~Scope() { pending = previous; --updating; }
        } scope;
        return updateHook.unsafe_thiscall<int>(self);
    }

    void Init()
    {
        auto update = hook::pattern("83 EC 08 53 55 56 57 8B F9 D9 87 04 01 00 00 D8 1D ? ? ? ? DF E0 F6 C4 41 75 2B");
        auto axes = hook::pattern("83 EC 08 53 8B D9 83 7B 3C FF 56 8B 35 ? ? ? ? 75 1C 8B 44 24 1C "
            "8B 4C 24 18 5E C7 00 00 00 00 00 C7 01 00 00 00 00 5B 83 C4 08 C2 14 00");
        auto curve = hook::pattern("CC CC 56 8B 74 24 08 D9 06 57 8B 7C 24 10 D9 07 D9 C0 D8 C9 D9 C2 D8 CB DE C1 D9 FA DD DA DD D8");
        updateHook = safetyhook::create_inline(update.get_first(), Update, safetyhook::InlineHook::StartDisabled);
        axesHook = safetyhook::create_inline(axes.get_first(), Axes, safetyhook::InlineHook::StartDisabled);
        curveHook = safetyhook::create_inline(curve.get_first(2), Curve, safetyhook::InlineHook::StartDisabled);
        (void)updateHook.enable();
        (void)axesHook.enable();
        (void)curveHook.enable();
        const auto refresh = []()
        {
            speed.store(FixSettings().Float("MAIN.GamepadCameraSpeed"), std::memory_order_relaxed);
            invertY.store(FixSettings().Bool("MAIN.InvertGamepadY"), std::memory_order_relaxed);
        };
        refresh();
        FixSettings().OnChange("MAIN.GamepadCameraSpeed", refresh);
        FixSettings().OnChange("MAIN.InvertGamepadY", refresh);
    }
}
