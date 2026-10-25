module;

#include <stdafx.h>
#include "../Shared/SilentHill/Gamepad.hpp"

export module Input;

import Camera;

// Controllers and mouse. Pads come through Xidi's standard gamepad layout (Xidi.ini): buttons 0-11 = A B X Y LB RB
// LT RT Back Start LS RS, right stick on Z/RotZ, d-pad on the POV.
//
// The PC port turns every stick into on/off key presses, so Henry either stands or runs and the camera jerks; the
// game's movement and camera code would take proportional values, and gets them here. Its retail pad bindings were
// made for a PS2 pad on a USB adapter and put the camera on the wrong right stick axes with Xidi; when the bindings
// are still the retail ones they become a modern layout:
//
//   A        Action, Enter              B        Cancel, Run             X       Items
//   Y        Evade                      LB/RB    Step left/right         LT      Camera behind Henry
//   RT       Ready weapon               Back     Map                     Start   Pause, Skip
//   LS click Scrapbook                  RS click Item info               Sticks  Move / camera
//
// The button configuration screen shows the pad's button names, rumble reaches the pad through Xidi (the game drove
// a single on/off effect), and the mouse no longer slows down when moved fast (the game folds large movements back)
// and turns equally in both directions in first person.

namespace
{
    // Pad element codes: (DIJOYSTATE2 offset << 8) | kind, kind 1 = axis negative, 3 = axis positive, 0x80 = button
    constexpr uint32_t Button(int n) { return 0x3080 + n * 0x100; }
    enum PadButton { A, B, X, Y, LB, RB, LT, RT, Back, Start, LS, RS };

    // In the game's file order (input.ini: MoveForward, MoveBackward, MoveLeft, MoveRight, TurnLeft, TurnRight, Up, Down,
    // Left, Right, Enter, Action, Cancel, SkipDemo, Pause, Pose, Evade, RearCamera, Run, ScrapBook, Map, Item, Iteminfo,
    // CameraLeft, CameraRight, CameraUp, CameraDown; the configuration screen)
    constexpr uint32_t RetailFileOrder[27] =
    {
        0x0401, 0x0403, 0x3680, 0x3780, 0x0001, 0x0003, 0x2001, 0x2003, 0x2004, 0x2002, 0x3280, 0x3280, 0x3180, 0x3880,
        0x3880, 0x3580, 0x3A80, 0x3480, 0x3180, 0x3B80, 0x3080, 0x3380, 0x3980, 0x1401, 0x1403, 0x0801, 0x0803,
    };
    constexpr uint32_t ModernFileOrder[27] =
    {
        0x0401, 0x0403, Button(LB), Button(RB), 0x0001, 0x0003, 0x2001, 0x2003, 0x2004, 0x2002,
        Button(A), Button(A), Button(B), Button(Start), Button(Start), Button(RT), Button(Y), Button(LT), Button(B),
        Button(LS), Button(Back), Button(X), Button(RS), 0x0801, 0x0803, 0x1401, 0x1403,
    };
    // The game's own action order: 21 digital, then 8 analog, each by its index in the file order
    constexpr int RuntimeDigital[21] = { 6, 7, 8, 9, 12, 21, 11, 20, 16, 13, 14, 22, 2, 17, 3, 15, 18, 10, 19, -1, -1 };
    constexpr int RuntimeAnalog[8] = { 5, 4, 0, 1, 24, 23, 25, 26 };

    uint32_t* PadDigital = nullptr;     // [21], then the analog [8]
    int32_t* MouseX = nullptr;
    int32_t* MouseY = nullptr;
    SafetyHookInline AnalogHook, DefaultsHook, ReadBindingsHook, VibrationOnHook, VibrationOffHook;

    bool UsesModernAnalogBindings()
    {
        if (!PadDigital)
            return false;
        auto analog = PadDigital + 21;
        for (int i = 0; i < 8; ++i)
        {
            if (analog[i] != ModernFileOrder[RuntimeAnalog[i]])
                return false;
        }
        return true;
    }

    // UI index of a code in the configuration screen's list of pad elements
    int UiIndex(uint32_t code)
    {
        static const uint32_t Fixed[12] = { 0x2001, 0x2002, 0x2003, 0x2004, 0x0401, 0x0403, 0x0001, 0x0003, 0x0801, 0x0803, 0x1401, 0x1403 };
        for (int i = 0; i < 12; ++i)
        {
            if (Fixed[i] == code)
                return i;
        }
        int button = int(code - 0x3080) / 0x100;
        return button < 16 ? 12 + button : 13 + button;
    }

    // The game reading input.ini (at startup and for the configuration screen): retail pad bindings come back as the
    // modern ones, in memory only; keyboard and customised pad bindings stay. The file changes only when the player
    // saves in the configuration screen.
    int __cdecl ReadBindings(int* keyboard, int keyboardCount, int* pad, int padCount)
    {
        int result = ReadBindingsHook.ccall<int>(keyboard, keyboardCount, pad, padCount);
        if (!result || !pad || padCount != 27)
            return result;
        for (int i = 0; i < 27; ++i)
        {
            if (pad[i] != UiIndex(RetailFileOrder[i]))
                return result;
        }
        for (int i = 0; i < 27; ++i)
            pad[i] = UiIndex(ModernFileOrder[i]);
        return result;
    }

    // Proportional sticks and the mouse, read where the game converts its stick words to floats
    void __cdecl AnalogWords(int16_t* turn, int16_t* move, int16_t* cameraX, int16_t* cameraY)
    {
        AnalogHook.ccall<void>(turn, move, cameraX, cameraY);

        auto pad = SHPad::Get();
        bool modern = Camera::IsModernActive();
        if (pad.connected && UsesModernAnalogBindings())
        {
            auto word = [](float v) { return int16_t(std::clamp(v, -1.0f, 1.0f) * 32767.0f); };
            if (pad.lx != 0.0f || pad.ly != 0.0f)
            {
                *turn = word(pad.lx);
                *move = word(-pad.ly);
            }
            if (pad.rx != 0.0f || pad.ry != 0.0f)
            {
                *cameraX = word(pad.rx);
                *cameraY = word(-pad.ry);
            }
        }
        if (modern)
        {
            // The modern camera owns the right stick and the mouse
            *cameraX = *cameraY = 0;
            if (MouseX && MouseY)
                *MouseX = *MouseY = 0;
            return;
        }

        // The game folds mouse movement above 24 counts per frame back down (moving faster turns less): keep the
        // per-frame value in its linear range and carry the rest to the next frames
        if (MouseX && MouseY)
        {
            static int32_t carryX = 0, carryY = 0;
            auto limit = [](int32_t& value, int32_t& carry)
            {
                value += carry;
                int32_t clamped = std::clamp(value, -23, 23);
                carry = std::clamp(value - clamped, -96, 96);
                value = clamped;
            };
            limit(*MouseX, carryX);
            limit(*MouseY, carryY);
        }
    }

    // Button names in the button configuration screen
    using DrawMessageFn = void(__cdecl*)(int, int, int, int, int, int);
    DrawMessageFn DrawMessage = nullptr;
    int32_t* PadMessageIds = nullptr;           // [45] message id of each pad element of the screen
    const char* (__cdecl* TextFromAscii)(const char*) = nullptr;
    void(__cdecl* DrawGameText)(const char*, int, int, int, int) = nullptr;
    void(__cdecl* EndText)(int) = nullptr;

    void __cdecl DrawPadElement(int bank, int message, int x, int y, int align, int flags)
    {
        if (SHPad::Get().connected && PadMessageIds)
        {
            for (int i = 0; i < 45; ++i)
            {
                if (PadMessageIds[i] != message)
                    continue;
                static const char* Fixed[12] = { "D-Pad Up", "D-Pad Right", "D-Pad Down", "D-Pad Left", "LS Up", "LS Down", "LS Left", "LS Right", "RS Left", "RS Right", "RS Up", "RS Down" };
                std::string name;
                if (i < 12)
                    name = Fixed[i];
                else
                {
                    int button = i <= 27 ? i - 12 : (i == 28 ? 15 : i - 13);
                    if (button <= RS)
                        name = SHPad::ButtonName(SHPad::LayoutButton(button == LT ? SHPad::LayoutLT : button == RT ? SHPad::LayoutRT :
                            button == Back ? SHPad::LayoutBack : button == Start ? SHPad::LayoutStart : button == LS ? SHPad::LayoutLS :
                            button == RS ? SHPad::LayoutRS : button));
                    else
                        name = std::format("Button {}", button + 1);
                }
                DrawGameText(TextFromAscii(("\\h" + name).c_str()), x, y, align, flags);
                EndText(0);
                return;
            }
        }
        DrawMessage(bank, message, x, y, align, flags);
    }

    // Rumble: the PS2's two motors arrive here; the PC port played both as one constant force
    bool Motor[3] = {};
    void SendRumble()
    {
        SHPad::Vibrate(Motor[2] ? 1.0f : 0.0f, Motor[1] ? 0.6f : 0.0f);
    }
}

export namespace Input
{
    void Init()
    {
        SHPad::Layout layout;
        layout.rightX = DIJOFS_Z;
        layout.rightY = DIJOFS_RZ;
        layout.ltAxis = layout.rtAxis = -1;
        int buttons[16] = { A, B, X, Y, LB, RB, Back, Start, LS, RS, LT, RT, -1, -1, -1, -1 };
        std::copy(std::begin(buttons), std::end(buttons), layout.button);
        SHPad::Install(layout);

        auto read = hook::pattern("55 8B EC 83 E4 F8 81 EC EC 01 00 00 A1 ? ? ? ? 33 45 04 53 8B 5D 08 56 8B 75 10 57 89 84 24 F4 01 00 00 33 C0 C6 84 24 E8 00 00 00 00");
        if (read.size() == 1)
            ReadBindingsHook = safetyhook::create_inline(read.get_first(), &ReadBindings);

        // The built-in bindings (used without input.ini) and the screen's "Default" become the modern layout
        auto tables = hook::pattern("8B 51 FC 89 90 ? ? ? ? 8B 11 89 90 ? ? ? ? 83 C0 04 83 C1 08 83 F8 4C");
        if (tables.size() == 1)
        {
            PadDigital = *tables.get_first<uint32_t*>(0x0D);
            bool retail = true;
            for (int i = 0; i < 21 && retail; ++i)
                retail = RuntimeDigital[i] < 0 || PadDigital[i] == RetailFileOrder[RuntimeDigital[i]];
            for (int i = 0; i < 8 && retail; ++i)
                retail = PadDigital[21 + i] == RetailFileOrder[RuntimeAnalog[i]];
            if (retail)
            {
                for (int i = 0; i < 21; ++i)
                {
                    if (RuntimeDigital[i] >= 0)
                        injector::WriteMemory(&PadDigital[i], ModernFileOrder[RuntimeDigital[i]], true);
                }
                for (int i = 0; i < 8; ++i)
                    injector::WriteMemory(&PadDigital[21 + i], ModernFileOrder[RuntimeAnalog[i]], true);
            }
        }
        auto defaults = hook::pattern("83 EC 6C 53 55 B8 01 00 00 00 56 57 8B BC 24 80 00 00 00 C7 44 24 10 11 00 00 00 C7 44 24 14 1F");
        if (defaults.size() == 1)
        {
            DefaultsHook = safetyhook::create_inline(defaults.get_first(), static_cast<int(__cdecl*)(int*, int, int*)>([](int* keyboard, int count, int* pad) -> int
            {
                int result = DefaultsHook.ccall<int>(keyboard, count, pad);
                for (int i = 0; i < std::min(count, 27); ++i)
                    pad[i] = UiIndex(ModernFileOrder[i]);
                return result;
            }));
        }

        // Analog sticks and mouse
        auto mouse = hook::pattern("8B 0D ? ? ? ? 33 C0 85 C9 74 05 B8 01 00 00");
        if (mouse.size() == 2)
        {
            MouseX = *mouse.get(0).get<int32_t*>(2);
            MouseY = *mouse.get(1).get<int32_t*>(2);
        }
        auto analog = hook::pattern("E8 ? ? ? ? 66 8B 48 3C 8B 54 24 04 66 89 0A 66 8B 48 3E 8B 54 24 08");
        if (analog.size() == 1)
            AnalogHook = safetyhook::create_inline(analog.get_first(), &AnalogWords);

        // First person: looking up and down responds linearly, like turning (it was squared)
        auto pitch = hook::pattern("D9 C0 D8 C9 DE CA DD D8 D9 16 D8 47 60 D9 5F 60");
        if (pitch.size() == 1)
            injector::WriteMemory<uint16_t>(pitch.get_first(2), 0xE1D9, true);   // fmul st, st(1) -> fabs

        // Pad names in the button configuration screen
        auto names = hook::pattern("8B 2C 85 ? ? ? ? E8 ? ? ? ? 6A 00 6A 00 53 81 C6 A2 01 00 00 56 55 6A 00 E8 ? ? ? ?");
        auto fromAscii = hook::pattern("A1 ? ? ? ? 56 0F B7 35 ? ? ? ? C1 E6 08 81 C6 ? ? ? ? 40 0F");
        auto drawText = hook::pattern("83 EC 50 56 8B 74 24 58 33 C0 3B F0 75 19 A3 ? ? ? ? A3 ? ? ? ?");
        auto endText = hook::pattern("8B 44 24 04 A3 ? ? ? ? C3 90 90 90 90 90 90 83 EC 14 57");
        if (names.size() == 1 && fromAscii.size() == 1 && drawText.size() == 1 && endText.size() == 1)
        {
            PadMessageIds = *names.get_first<int32_t*>(3);
            TextFromAscii = reinterpret_cast<decltype(TextFromAscii)>(fromAscii.get_first());
            DrawGameText = reinterpret_cast<decltype(DrawGameText)>(drawText.get_first());
            EndText = reinterpret_cast<decltype(EndText)>(endText.get_first());
            DrawMessage = reinterpret_cast<DrawMessageFn>(injector::GetBranchDestination(names.get_first(0x1B)).as_int());
            injector::MakeCALL(names.get_first(0x1B), &DrawPadElement, true);
        }

        // Rumble
        auto on = hook::pattern("8B 44 24 08 83 F8 01 75 0A 50 E8 ? ? ? ? 83 C4 04 C3 83 F8 02 75 07 50 E8 ? ? ? ? 59 C3 8B 44 24 08 83 F8 01 75");
        auto off = hook::pattern("8B 44 24 08 83 F8 01 75 0A 50 E8 ? ? ? ? 83 C4 04 C3 83 F8 02 75 07 50 E8 ? ? ? ? 59 C3 56 33 F6 6A 00 6A 00 6A");
        auto stop = hook::pattern("56 33 F6 6A 00 6A 00 6A 00 56 E8 ? ? ? ? 83 C4 10 46 83 FE 04 7C EB");
        if (on.size() == 1 && off.size() == 1 && stop.size() == 1)
        {
            VibrationOnHook = safetyhook::create_inline(on.get_first(), static_cast<int(__cdecl*)(int, int)>([](int pad, int motor) -> int
            {
                if (motor == 1 || motor == 2)
                {
                    Motor[motor] = true;
                    SendRumble();
                }
                return 0;
            }));
            VibrationOffHook = safetyhook::create_inline(off.get_first(), static_cast<int(__cdecl*)(int, int)>([](int pad, int motor) -> int
            {
                if (motor == 1 || motor == 2)
                {
                    Motor[motor] = false;
                    SendRumble();
                }
                return 0;
            }));
            // After its loop (which jumps back into its first instructions), before it stops the game's effect
            static SafetyHookMid StopHook = safetyhook::create_mid(stop.get_first(0x18), [](SafetyHookContext&)
            {
                Motor[1] = Motor[2] = false;
                SendRumble();
            });
        }
    }
}
