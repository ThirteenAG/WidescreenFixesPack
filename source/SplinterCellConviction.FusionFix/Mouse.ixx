module;

#include <stdafx.h>

export module Mouse;

import ComVars;

// Mouse look goes through UWindowsViewport's WM_MOUSEMOVE handler (sub_8F199B): while the mouse is captured, it sums the cursor
// movement from the window center over the queued WM_MOUSEMOVE messages, sends it as IK_MouseX/IK_MouseY axis input and puts the
// cursor back to the center with SetCursorPos. The cursor is moved between the last message and SetCursorPos, which gets lost,
// with a high polling rate mouse that happens all the time. The movement also goes through the Windows pointer speed and acceleration.
// The axis input doesn't add up either, the camera only gets the last one before the frame (there are several per frame when
// WM_MOUSEMOVE keeps coming while the messages are pumped).
// With raw input, the handler sends nothing and the raw counts are sent once after the messages were pumped, the cursor is still used to recenter.
namespace RawMouse
{
    using RawInput = RawInputHandler<int32_t>;

    void* viewport = nullptr;
    bool captured = false;
    int32_t cursorX = 0;
    int32_t cursorY = 0;
    int(__fastcall* IsCapturingInput)(void* viewport) = nullptr; // sub_8EFFAA, checked before the axis input

    SafetyHookInline shPumpMessages = {};
    int __fastcall PumpMessages(void* client, void* edx)
    {
        auto ret = shPumpMessages.fastcall<int>(client, edx);

        auto x = RawInput::RawMouseDeltaX.exchange(0);
        auto y = RawInput::RawMouseDeltaY.exchange(0);
        if (viewport && captured && IsCapturingInput(viewport))
        {
            // UWindowsViewport::CauseInputEvent(key, IST_Axis, delta, 0, 0), like the WM_MOUSEMOVE handler
            auto CauseInputEvent = reinterpret_cast<int(__thiscall*)(void*, int, int, int, float, int, int)>((*reinterpret_cast<uintptr_t**>(viewport))[0x78 / 4]);
            auto input = *reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(viewport) + 0x5C);
            if (x)
                CauseInputEvent(viewport, input, 0xE4, 4, static_cast<float>(x), 0, 0); // IK_MouseX
            if (y)
                CauseInputEvent(viewport, input, 0xE5, 4, static_cast<float>(y), 0, 0); // IK_MouseY
        }
        return ret;
    }
}

export void InitMouse()
{

    CIniReader iniReader("");
    auto bRawMouseInput = iniReader.ReadInteger("GAMEPLAY", "RawMouseInput", 1) != 0;
    auto bDisableMouseSmoothing = iniReader.ReadInteger("GAMEPLAY", "DisableMouseSmoothing", 1) != 0;

    if (bDisableNegativeMouseAcceleration)
    {
        auto pattern = hook::pattern("76 05 0F 28 D9 EB 08 0F 2F DA");
        injector::MakeNOP(pattern.get_first(0), 2, true);
        injector::MakeNOP(pattern.get_first(5), 2, true);
        injector::MakeNOP(pattern.get_first(10), 2, true);

        pattern = hook::pattern("77 0D 0F 2F C2 76 05 0F 28 CA EB 03 0F 28 C8 0F 57 C0");
        injector::MakeNOP(pattern.get_first(0), 2, true);
        injector::MakeNOP(pattern.get_first(5), 2, true);
    }

    if (bDisableMouseSmoothing)
    {
        // the camera averages the mouse input over 4 frames (0.34 for this frame, 0.22 for each of the 3 before),
        // take the branch the game uses when that's off (it only resets the history)
        auto pattern = hook::pattern("F6 80 04 06 00 00 02 0F 85 ? ? ? ? 8B 86 34 04 00 00");
        auto jnz = pattern.get_first<uint8_t>(7);
        auto target = jnz + 6 + *reinterpret_cast<int32_t*>(jnz + 2);
        injector::WriteMemory<uint8_t>(jnz, 0xE9, true);
        injector::WriteMemory<int32_t>(jnz + 1, int32_t(target - (jnz + 5)), true);
        injector::WriteMemory<uint8_t>(jnz + 5, 0x90, true);
    }

    static auto fGamepadCameraSpeed = iniReader.ReadFloat("GAMEPLAY", "GamepadCameraSpeed", 1.0f);
    if (fGamepadCameraSpeed > 0.0f && fGamepadCameraSpeed != 1.0f)
    {
        // CameraImpl (sub_5CD6FB) turns the camera by the stick input at +424h/+428h (-1..1 with a pad) times the camera mode's
        // yaw and pitch speed (yawSpeed, pitchSpeed in degrees/second, ConvictionCamera.ini), the controller at +20h is in pad mode with +604h & 2
        auto pattern = hook::pattern("8B 4E 28 8B 01 57 8D 5E 30 53 8D 96 28 04 00 00");
        static auto GamepadCameraSpeedHook = safetyhook::create_mid(pattern.get_first(6), [](SafetyHookContext& regs)
        {
            auto controller = *reinterpret_cast<uintptr_t*>(regs.esi + 0x20);
            if (!controller || (*reinterpret_cast<uint32_t*>(controller + 0x604) & 2) == 0)
                return;
            *reinterpret_cast<float*>(regs.esi + 0x424) *= fGamepadCameraSpeed;
            *reinterpret_cast<float*>(regs.esi + 0x428) *= fGamepadCameraSpeed;
        });
    }

    if (bRawMouseInput)
    {
        auto pattern = hook::pattern("55 8B EC 83 EC 20 53 56 8B 35 ? ? ? ? 57 33 FF 33 DB 89 7D FC 43");
        RawMouse::shPumpMessages = safetyhook::create_inline(pattern.get_first(), RawMouse::PumpMessages);

        // captured: after the WM_MOUSEMOVE loop, before the IK_MouseX/IK_MouseY axis input, [esp+48h] and [esp+38h] are the summed cursor movement
        pattern = hook::pattern("8B CB E8 ? ? ? ? 85 C0 74 4A 39 74 24 48");
        RawMouse::IsCapturingInput = reinterpret_cast<decltype(RawMouse::IsCapturingInput)>(injector::GetBranchDestination(pattern.get_first(2)).as_int());
        static auto AxisInputHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            RawMouse::viewport = reinterpret_cast<void*>(regs.ebx);
            if (!std::exchange(RawMouse::captured, true))
            {
                static bool registered = false;
                if (!std::exchange(registered, true))
                    RawMouse::RawInput::RegisterRawInput(*reinterpret_cast<HWND*>(*reinterpret_cast<uintptr_t*>(regs.ebx + 0x3F4) + 4), 1.0f, true);
                RawMouse::RawInput::RawMouseDeltaX = 0; // moved before
                RawMouse::RawInput::RawMouseDeltaY = 0;
            }
            RawMouse::cursorX = std::exchange(*reinterpret_cast<int32_t*>(regs.esp + 0x48), 0);
            RawMouse::cursorY = std::exchange(*reinterpret_cast<int32_t*>(regs.esp + 0x38), 0);
        });

        // the cursor is put back to the center when it moved
        pattern = hook::pattern("39 74 24 50 75 41 39 74 24 48 75 06 39 74 24 38");
        static auto RecenterHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            *reinterpret_cast<int32_t*>(regs.esp + 0x48) = RawMouse::cursorX;
            *reinterpret_cast<int32_t*>(regs.esp + 0x38) = RawMouse::cursorY;
        });

        // not captured (menus)
        pattern = hook::pattern("8D 83 20 04 00 00 50 FF 15 ? ? ? ? 8B 43 18");
        static auto NotCapturedHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            RawMouse::captured = false;
        });
    }
}
