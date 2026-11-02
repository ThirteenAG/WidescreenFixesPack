module;

#include "stdafx.h"
#include <d3d9.h>
#include "FixSettings.hpp"
#ifndef DIRECTINPUT_VERSION
#define DIRECTINPUT_VERSION 0x0800
#endif
#include <dinput.h>


export module Mouse;

import ComVars;

export namespace DPLMouse
{
    struct State
    {
        int32_t x, y, wheel;
        uint8_t buttons[8];
    };
    static_assert(sizeof(State) == 20);
    SafetyHookInline pollHook, cursorHook;
    float remainderX = 0, remainderY = 0;
    std::atomic<bool> changed{ true };
    bool ready = false;
    void (*drainController)(void*) = nullptr;

    template<class T> T& At(void* pointer, size_t offset)
    {
        return *reinterpret_cast<T*>(static_cast<uint8_t*>(pointer) + offset);
    }
    char __fastcall Poll(void* self, void*, int tick)
    {
        auto* device = At<IDirectInputDevice8A*>(self, 4);
        if (!device || !At<uint8_t>(self, 0)) return 0;
        auto& current = At<State>(self, 8);
        auto& previous = At<State>(self, 28);
        previous = current;
        DIMOUSESTATE2 input{};
        const bool success = SUCCEEDED(device->GetDeviceState(sizeof(input), &input));
        if (!success) device->Acquire();
        if (!success || BlockGameInput) input = {};
        current = { static_cast<int32_t>(input.lX), static_cast<int32_t>(input.lY), static_cast<int32_t>(input.lZ), {} };
        std::copy(std::begin(input.rgbButtons), std::end(input.rgbButtons), current.buttons);
        auto& totals = *accumulation.get_ptr();
        if (changed.exchange(false) || !success || BlockGameInput)
        {
            totals.velocityX = totals.velocityY = 0;
            remainderX = remainderY = 0;
        }
        totals.totalX += static_cast<uint32_t>(input.lX);
        totals.totalY += static_cast<uint32_t>(input.lY);
        const float configuredSensitivity = FixSettings().Float("MAIN.MouseSensitivity");
        const float sensitivity = std::isfinite(configuredSensitivity) ? std::clamp(configuredSensitivity, 0.1f, 5.0f) : 1.0f;
        const float verticalSensitivity = FixSettings().Bool("MAIN.InvertMouseY") ? -sensitivity : sensitivity;
        if (FixSettings().Bool("MAIN.RawInputMouse"))
        {
            totals.x += static_cast<float>(input.lX) * sensitivity;
            totals.y += static_cast<float>(input.lY) * verticalSensitivity;
        }
        else
        {
            const float native = coefficient;
            const float smoothing = std::isfinite(native) ? std::clamp(native, 0.0f, 1.0f) : 0.5f;
            totals.velocityX = smoothing * totals.velocityX + (1.0f - smoothing) * input.lX;
            totals.velocityY = smoothing * totals.velocityY + (1.0f - smoothing) * input.lY;
            totals.x += totals.velocityX * sensitivity;
            totals.y += totals.velocityY * verticalSensitivity;
        }
        for (size_t i = 0; i < 3; ++i) totals.buttons[i] = input.rgbButtons[i] != 0;
        if (!success || BlockGameInput) previous = {};
        return success;
    }
    int32_t Pixels(double movement, float& remainder)
    {
        if (!std::isfinite(movement)) { remainder = 0; return 0; }
        const auto pixels = static_cast<int32_t>(std::clamp(movement, static_cast<double>(INT32_MIN), static_cast<double>(INT32_MAX)));
        remainder = movement > INT32_MIN && movement < INT32_MAX ? static_cast<float>(movement - pixels) : 0.0f;
        return pixels;
    }
    char __fastcall Cursor(void* self, void*)
    {
        if (BlockGameInput)
        {
            std::memset(static_cast<uint8_t*>(self) + 24, 0, 512);
            std::memset(static_cast<uint8_t*>(self) + 544, 0, 40);
            if (drainController) drainController(self);
        }
        if (!FixSettings().Bool("MAIN.RawInputMouse")) return cursorHook.unsafe_thiscall<char>(self);
        auto& state = At<State>(self, 544);
        const int32_t x = state.x, y = state.y;
        auto& speed = At<float>(self, 1408);
        const float originalSpeed = speed;
        state.x = Pixels(static_cast<double>(x) * originalSpeed + remainderX, remainderX);
        state.y = Pixels(static_cast<double>(y) * originalSpeed + remainderY, remainderY);
        speed = 1.0f;
        const auto result = cursorHook.unsafe_thiscall<char>(self);
        speed = originalSpeed;
        state.x = x;
        state.y = y;
        return result;
    }
    void Install()
    {
        FixSettings().Availability("MAIN.RawInputMouse", [](std::string_view) { return ready; });
        FixSettings().Availability("MAIN.MouseSensitivity", [](std::string_view) { return ready; });
        FixSettings().Availability("MAIN.InvertMouseY", [](std::string_view) { return ready; });
        auto poll = hook::pattern("55 8B EC 81 EC 84 02 00 00 53 8B D9 8B 43 04 85 C0 56 57 74 50 80 3B 00");
        auto cursor = hook::pattern("55 8B EC 83 EC 0C 56 8B F1 F3 0F 2A 86 20 02 00 00 F3 0F 10 96 80 05 00 00");
        auto globals = hook::pattern("8B 08 01 0E F3 0F 10 05 ? ? ? ? 01 0D ? ? ? ? 0F 28 D1");
        coefficient.SetAddress(*globals.get_first<float*>(8));
        accumulation.SetAddress(*globals.get_first<MouseAccumulation*>(14));
        pollHook = safetyhook::create_inline(poll.get_first(), Poll);
        cursorHook = safetyhook::create_inline(cursor.get_first(), Cursor);
        ready = pollHook && cursorHook;
        if (!ready)
        {
            pollHook.reset();
            cursorHook.reset();
            OutputDebugStringA("DriverParallelLines: native mouse hooks unavailable.\n");
        }
        FixSettings().OnChange("MAIN.RawInputMouse", []() { changed.store(true); });
    }
}
