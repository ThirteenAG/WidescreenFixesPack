module;

#include <stdafx.h>
#include <chrono>
#include <map>
#include <tuple>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

export module Mouse;

import ComVars;

// The game's mouse state in its input manager, the mouse object is at +0x21C and this at +0x224
struct MouseState
{
    int32_t x; // movement, in mouse counts
    int32_t y;
    int32_t wheel;
    uint8_t buttons[8];
};
static_assert(sizeof(MouseState) == 20);

float CursorSensitivity = 1.0f;

namespace Total // every mouse count so far
{
    int64_t X = 0;
    int64_t Y = 0;
    int64_t Wheel = 0;
}

// Control bindings read the mouse axes whenever their controller updates, every 33-50 ms for on foot look,
// each gets the movement since its own previous read (on foot look itself is driven by Camera.ixx)
namespace Bindings
{
    struct Read
    {
        int64_t total = 0;
        std::chrono::steady_clock::time_point time = {};
    };

    std::map<std::tuple<uintptr_t, uintptr_t, uint32_t>, Read> Reads; // call site, binding, axis

    int32_t GetMovement(uintptr_t callSite, uintptr_t binding, uint32_t axis)
    {
        const int64_t total = axis == Keymap::AxisMouseX ? Total::X : axis == Keymap::AxisMouseY ? Total::Y : Total::Wheel;
        auto& read = Reads[{ callSite, binding, axis }];

        // movement from before it was reading (new, paused, loading) belongs to something else
        const auto now = std::chrono::steady_clock::now();
        const bool stale = now - read.time > std::chrono::milliseconds(250);
        const int32_t movement = stale ? 0 : static_cast<int32_t>(total - read.total);

        read.total = total;
        read.time = now;
        return movement;
    }
}

namespace Cursor
{
    float RemainderX = 0.0f; // mouse movement the cursor has not made yet, in pixels
    float RemainderY = 0.0f;
    float SavedSpeed = 0.0f;
}

class Mouse
{
public:
    Mouse()
    {
        WFP::onInitEvent() += []()
        {
            CIniReader iniReader("");
            CursorSensitivity = iniReader.ReadFloat("MAIN", "CursorSensitivity", 1.0f);

            // The game reads buffered mouse data every 8 ms by GetTickCount, so about every 16 ms, and only 32 events at a time, which delays
            // and drops fast movement. It keeps that movement until the next read, and whatever reads the controls in between gets it again or misses it.
            // Read the immediate state on every input update (once per frame) instead, it has every count since the previous read.
            auto pattern = hook::pattern("8B 6A 0C 8B 52 10 89 69 0C 89 51 10 0F 84"); // the previous state is saved, jz to skip the read
            auto skipRead = pattern.get_first<uint8_t>(12);
            static auto ReadDone = reinterpret_cast<uintptr_t>(skipRead + 6 + *reinterpret_cast<int32_t*>(skipRead + 2)); // returns true
            static auto ReadFailed = reinterpret_cast<uintptr_t>(hook::get_pattern("8B 76 04 8B 06 56 FF 50 1C 33 C9 89 0F")); // acquires again, clears the state, returns false
            static auto MouseReadHook = safetyhook::create_mid(skipRead, [](SafetyHookContext& regs)
            {
                auto device = *reinterpret_cast<IDirectInputDevice8A**>(regs.esi + 4);
                auto state = reinterpret_cast<MouseState*>(regs.esi + 8);

                DIMOUSESTATE2 current = {};
                if (FAILED(device->GetDeviceState(sizeof(current), &current)))
                {
                    MouseRead::X = 0;
                    MouseRead::Y = 0;
                    regs.eip = ReadFailed;
                    return;
                }

                MouseRead::X = current.lX;
                MouseRead::Y = current.lY;
                Total::X += current.lX;
                Total::Y += current.lY;
                Total::Wheel += current.lZ;
                std::copy(std::begin(current.rgbButtons), std::end(current.rgbButtons), state->buttons);
                regs.eip = ReadDone;
            });

            // A control binding reads an axis (ecx axis, edx input manager, the binding in esi at every call site),
            // put its movement where the function reads the mouse axes from
            pattern = hook::pattern("80 7A 04 00 0F 84 ? ? ? ? 83 7C 24 04 01 0F 8D ? ? ? ? 8B C1 83 E8 08");
            static auto AxisReadHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                const uint32_t axis = regs.ecx;
                if (axis < Keymap::AxisMouseX || axis > Keymap::AxisMouseWheel)
                    return;

                const uintptr_t callSite = *reinterpret_cast<uintptr_t*>(regs.esp);
                auto state = reinterpret_cast<MouseState*>(regs.edx + 0x224);
                const int32_t movement = Bindings::GetMovement(callSite, regs.esi, axis);

                if (axis == Keymap::AxisMouseX)
                    state->x = movement;
                else if (axis == Keymap::AxisMouseY)
                    state->y = movement;
                else
                    state->wheel = movement;
            });

            // The menu cursor moves on every input update by (int)(movement * 1.75) pixels, dropping the fraction, so slow movement moves it less than fast movement.
            // Give it the whole pixels of the latest read and keep the fraction for the next one.
            pattern = hook::pattern("E8 ? ? ? ? 5F 5B B0 01 5E 59 C3");
            static auto CursorUpdateHook = safetyhook::create_mid(pattern.get_first(0), [](SafetyHookContext& regs)
            {
                auto state = reinterpret_cast<MouseState*>(regs.esi + 0x224);
                auto speed = reinterpret_cast<float*>(regs.esi + 0x7C4);
                Cursor::SavedSpeed = *speed;

                const float moveX = MouseRead::X * *speed * CursorSensitivity + Cursor::RemainderX;
                const float moveY = MouseRead::Y * *speed * CursorSensitivity + Cursor::RemainderY;
                state->x = static_cast<int32_t>(moveX);
                state->y = static_cast<int32_t>(moveY);
                Cursor::RemainderX = moveX - state->x;
                Cursor::RemainderY = moveY - state->y;
                *speed = 1.0f;
            });

            // the input update is over, a later one may not read the mouse (no device)
            static auto CursorUpdateEndHook = safetyhook::create_mid(pattern.get_first(5), [](SafetyHookContext& regs)
            {
                *reinterpret_cast<float*>(regs.esi + 0x7C4) = Cursor::SavedSpeed;
                MouseRead::X = 0;
                MouseRead::Y = 0;
            });
        };
    }
} Mouse;
