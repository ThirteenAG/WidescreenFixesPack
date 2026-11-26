module;

#include <stdafx.h>
#include "Shared/NFS/BufferedRawMouse.hpp"
#include "Shared/PadInput.hpp"
#include "Shared/SilentHill/Display.hpp"
#include "Shared/SilentHill/Gamepad.hpp"
#include "FixSettings.hpp"

export module Input;

import ComVars;
import Camera;
import ButtonIcons;

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


    constexpr wchar_t MouseWindowClass[] = L"SilentHill4.FusionFix.Mouse";
    using RawMouse = NFSBufferedRawMouse<MouseWindowClass>;
    SafetyHookInline InputFrameHook, AnalogHook, DefaultsHook, ReadBindingsHook, VibrationOnHook, VibrationOffHook;
    bool Motor[3]{};
    int RumbleSlot = -1;

    void SetRumble(int slot, WORD low, WORD high)
    {
        using XidiVibration = bool(__cdecl*)(short, WORD, WORD);
        static const auto xidi = GetModuleHandleW(L"Xidi.32.dll");
        static const auto send = xidi ? reinterpret_cast<XidiVibration>(GetProcAddress(xidi, "XidiSendVibration")) : nullptr;
        if (send)
        {
            send(static_cast<short>(slot), low, high);
            return;
        }
        if (const auto set = PadInput::XInput().setState)
        {
            XINPUT_VIBRATION vibration{ low, high };
            set(static_cast<DWORD>(slot), &vibration);
        }
    }

    void SendRumble()
    {
        if (RumbleSlot >= 0 && RumbleSlot != GameplayPad.slot)
            SetRumble(RumbleSlot, 0, 0);
        RumbleSlot = GameplayPad.slot;
        if (RumbleSlot >= 0)
        {
            // The small motor is high-frequency, and the large motor is low-frequency.
            const WORD low = Motor[2] ? 65535 : 0;
            const WORD high = Motor[1] ? 39321 : 0;
            SetRumble(RumbleSlot, low, high);
        }
    }

    GameplayPadState ReadAxes(const XINPUT_GAMEPAD& pad)
    {
        const auto stick = [](SHORT rawX, SHORT rawY, float deadzone)
        {
            const float x = std::clamp(float(rawX) / 32767.0f, -1.0f, 1.0f);
            const float y = std::clamp(-float(rawY) / 32767.0f, -1.0f, 1.0f);
            const float magnitude = std::sqrt(x * x + y * y);
            const float scale = magnitude <= deadzone ? 0.0f :
                (std::min(1.0f, magnitude) - deadzone) / ((1.0f - deadzone) * magnitude);
            return std::pair{ x * scale, y * scale };
        };
        const auto [lx, ly] = stick(pad.sThumbLX, pad.sThumbLY, 0.24f);
        const auto [rx, ry] = stick(pad.sThumbRX, pad.sThumbRY, 0.20f);
        return { lx, ly, rx, ry, true };
    }

    bool PadElementHeld(const XINPUT_GAMEPAD& pad, uint32_t code)
    {
        constexpr WORD buttons[] = { XINPUT_GAMEPAD_A, XINPUT_GAMEPAD_B, XINPUT_GAMEPAD_X, XINPUT_GAMEPAD_Y,
            XINPUT_GAMEPAD_LEFT_SHOULDER, XINPUT_GAMEPAD_RIGHT_SHOULDER, 0, 0, XINPUT_GAMEPAD_BACK,
            XINPUT_GAMEPAD_START, XINPUT_GAMEPAD_LEFT_THUMB, XINPUT_GAMEPAD_RIGHT_THUMB };
        constexpr WORD directions[] = { XINPUT_GAMEPAD_DPAD_UP, XINPUT_GAMEPAD_DPAD_RIGHT,
            XINPUT_GAMEPAD_DPAD_DOWN, XINPUT_GAMEPAD_DPAD_LEFT };
        const auto kind = code & 0xFF;
        const auto offset = code >> 8;
        if (kind == 0x80 && offset >= 0x30 && offset < 0x30 + std::size(buttons))
        {
            const auto button = offset - 0x30;
            if (button == LT)
                return pad.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
            if (button == RT)
                return pad.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
            return (pad.wButtons & buttons[button]) != 0;
        }
        if (offset == 0x20 && kind >= 1 && kind <= std::size(directions))
            return (pad.wButtons & directions[kind - 1]) != 0;
        int axis;
        switch (offset)
        {
            case 0x00: axis = pad.sThumbLX; break;
            case 0x04: axis = -int(pad.sThumbLY); break;
            case 0x08: axis = pad.sThumbRX; break;
            case 0x14: axis = -int(pad.sThumbRY); break;
            default: return false;
        }
        return (kind == 1 && axis < -16384) || (kind == 3 && axis > 16384);
    }

    void ReadActivePad()
    {
        static DWORD active = XUSER_MAX_COUNT;
        static std::array<bool, XUSER_MAX_COUNT> previousConnected{};
        static std::array<XINPUT_GAMEPAD, XUSER_MAX_COUNT> previous{};
        std::array<bool, XUSER_MAX_COUNT> connected{}, engaged{};
        std::array<XINPUT_GAMEPAD, XUSER_MAX_COUNT> current{};
        std::array<GameplayPadState, XUSER_MAX_COUNT> states{};
        const auto get = PadInput::XInput().getState;
        for (DWORD slot = 0; get && slot < XUSER_MAX_COUNT; ++slot)
        {
            XINPUT_STATE packet{};
            if (get(slot, &packet) != ERROR_SUCCESS)
                continue;
            const auto& pad = current[slot] = packet.Gamepad;
            auto& state = states[slot] = ReadAxes(pad);
            connected[slot] = true;
            engaged[slot] = pad.wButtons || pad.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD ||
                pad.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD || state.lx || state.ly || state.rx || state.ry;
        }
        if (active >= XUSER_MAX_COUNT || !engaged[active])
        {
            DWORD fallback = active < XUSER_MAX_COUNT && connected[active] ? active : XUSER_MAX_COUNT;
            for (DWORD slot = 0; slot < XUSER_MAX_COUNT; ++slot)
            {
                if (engaged[slot])
                {
                    fallback = slot;
                    break;
                }
                if (fallback == XUSER_MAX_COUNT && connected[slot])
                    fallback = slot;
            }
            active = fallback;
        }
        GameplayPad = active < XUSER_MAX_COUNT ? states[active] : GameplayPadState{};
        if (active < XUSER_MAX_COUNT)
            GameplayPad.slot = static_cast<int>(active);
        if (active < XUSER_MAX_COUNT && previousConnected[active])
        {
            constexpr size_t MapAction = 7, RearCameraAction = 13;
            const auto& bindings = *aPadBindings;
            const auto pressed = [&](uint32_t code)
            {
                return PadElementHeld(current[active], code) && !PadElementHeld(previous[active], code);
            };
            GameplayPad.mapPressed = pressed(bindings[MapAction]);
            GameplayPad.recentre = pressed(bindings[RearCameraAction]) || pressed(Button(RS));
        }
        previousConnected = connected;
        previous = current;
        if (RumbleSlot != GameplayPad.slot && (RumbleSlot >= 0 || Motor[1] || Motor[2]))
            SendRumble();
    }

    int __cdecl InputFrame()
    {
        const int result = InputFrameHook.ccall<int>();
        ReadActivePad();
        RawMouse::Update(SHDisplay::Get().window);
        int x = nMouseX, y = nMouseY;
        RawMouse::Consume(x, y);
        if (IsModernCameraActive())
        {
            LookMouse[0] += x;
            LookMouse[1] += y;
            nMouseX = nMouseY = 0;
        }
        else
        {
            LookMouse = {};
            nMouseX = x;
            nMouseY = y;
        }
        return result;
    }

    void LinearizeNativeMouse()
    {
        const auto x = hook::pattern(pattern_str(0xA1, to_bytes(nMouseX.get_ptr())) + "C3").get_first();
        const auto y = hook::pattern(pattern_str(0xA1, to_bytes(nMouseY.get_ptr())) + "C3").get_first();
        auto calls = hook::pattern("E8 ? ? ? ? 89 44 24 ? DB 44 24 ? D8 0D ? ? ? ?");
        size_t patched = 0;
        for (size_t i = 0; i < calls.size(); ++i)
        {
            const auto entry = reinterpret_cast<uintptr_t>(calls.get(i).get<void>());
            const auto target = injector::GetBranchDestination(entry).get<void>();
            if (target != x && target != y)
                continue;
            auto fold = hook::range_pattern(entry, entry + 0x40,
                "99 33 C2 2B C2 89 44 24 ? DB 44 24 ? D9 05 ? ? ? ? D8 D9 DF E0 F6 C4 05 7A ?");
            // Keep the native multiplier and the x87 stack balance while bypassing the fold.
            injector::WriteMemory<uint8_t>(fold.get_first(26), 0xEB, true);
            ++patched;
        }
        if (patched != 10)
            std::terminate();
    }

    // How far the pad pushes a bound analog element (0..1); -1 for buttons and the d-pad, which stay on/off
    float ElementValue(const GameplayPadState& pad, uint32_t code)
    {
        float axis = 0.0f;
        switch (code >> 8)
        {
            case 0x00: axis = pad.lx; break;
            case 0x04: axis = pad.ly; break;
            case 0x08: axis = pad.rx; break;
            case 0x14: axis = pad.ry; break;
            default: return -1.0f;
        }
        switch (code & 0xFF)
        {
            case 1: return std::max(0.0f, -axis);
            case 3: return std::max(0.0f, axis);
            default: return -1.0f;
        }
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

        const auto pad = GameplayPad;
        if (pad.connected && aPadBindings.is_initialized())
        {
            // The game's analog actions in pairs (right/left turn, forward/back, camera right/left, camera up/down),
            // whatever pad elements they are bound to
            auto analog = aPadBindings->data() + 21;
            int16_t* words[4] = { turn, move, cameraX, cameraY };
            for (int i = 0; i < 4; ++i)
            {
                const float positive = ElementValue(pad, analog[i * 2]), negative = ElementValue(pad, analog[i * 2 + 1]);
                if (positive < 0.0f || negative < 0.0f || (positive == 0.0f && negative == 0.0f))
                    continue;
                *words[i] = int16_t(std::clamp(positive - negative, -1.0f, 1.0f) * 32767.0f);
            }
        }
        if (IsModernCameraActive())
            *cameraX = *cameraY = 0;
    }

    // Button names in the button configuration screen
    using DrawMessageFn = void(__cdecl*)(int, int, int, int, int, int);
    DrawMessageFn DrawMessage = nullptr;

    const char* (__cdecl* TextFromAscii)(const char*) = nullptr;
    void(__cdecl* DrawGameText)(const char*, int, int, int, int) = nullptr;
    void(__cdecl* EndText)(int) = nullptr;

    void __cdecl DrawPadElement(int bank, int message, int x, int y, int align, int flags)
    {
        if ((GameplayPad.connected || SH4Icons::Texture) && aPadMessageIds.is_initialized())
        {
            for (int i = 0; i < 45; ++i)
            {
                if ((*aPadMessageIds)[i] != message)
                    continue;
                static const char* Fixed[12] = { "D-Pad Up", "D-Pad Right", "D-Pad Down", "D-Pad Left", "LS Up", "LS Down", "LS Left", "LS Right", "RS Left", "RS Right", "RS Up", "RS Down" };
                std::string name;
                if (i < 12)
                    name = Fixed[i];
                else
                {
                    int button = i <= 27 ? i - 12 : (i == 28 ? 15 : i - 13);
                    if (button >= A && button <= RS && SH4Icons::Texture)
                    {
                        constexpr int images[] = { 0, 1, 2, 3, 4, 5, 6, 7, 10, 11, 8, 9 };
                        SH4Icons::Queue(x, y, images[button]);
                        EndText(0);
                        return;
                    }
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

}

class Input
{
public:
    Input()
    {
        WFP::onInitEvent() += []()
        {
            SH4Icons::Init();
            auto updateIcons = []() { SHPad::SetIconFamily(FixSettingNumber("MAIN.GamepadIcons", 0)); };
            updateIcons();
            FixSettings().OnChange("MAIN.GamepadIcons", updateIcons);

            auto read = hook::pattern("55 8B EC 83 E4 F8 81 EC EC 01 00 00 A1 ? ? ? ? 33 45 04 53 8B 5D 08 56 8B 75 10 57 89 84 24 F4 01 00 00 33 C0 C6 84 24 E8 00 00 00 00");
            ReadBindingsHook = safetyhook::create_inline(read.get_first(), &ReadBindings);

            // The built-in bindings (used without input.ini) and the screen's "Default" become the modern layout
            auto tables = hook::pattern("8B 51 FC 89 90 ? ? ? ? 8B 11 89 90 ? ? ? ? 83 C0 04 83 C1 08 83 F8 4C");
            aPadBindings.SetAddress(*tables.get_first<uint32_t*>(0x0D));
            auto& padBindings = *aPadBindings;
            bool retail = true;
            for (int i = 0; i < 21 && retail; ++i)
                retail = RuntimeDigital[i] < 0 || padBindings[i] == RetailFileOrder[RuntimeDigital[i]];
            for (int i = 0; i < 8 && retail; ++i)
                retail = padBindings[21 + i] == RetailFileOrder[RuntimeAnalog[i]];
            if (retail)
            {
                for (int i = 0; i < 21; ++i)
                {
                    if (RuntimeDigital[i] >= 0)
                        injector::WriteMemory(&padBindings[i], ModernFileOrder[RuntimeDigital[i]], true);
                }
                for (int i = 0; i < 8; ++i)
                    injector::WriteMemory(&padBindings[21 + i], ModernFileOrder[RuntimeAnalog[i]], true);
            }
            auto defaults = hook::pattern("83 EC 6C 53 55 B8 01 00 00 00 56 57 8B BC 24 80 00 00 00 C7 44 24 10 11 00 00 00 C7 44 24 14 1F");
            DefaultsHook = safetyhook::create_inline(defaults.get_first(), static_cast<int(__cdecl*)(int*, int, int*)>([](int* keyboard, int count, int* pad) -> int
            {
                int result = DefaultsHook.ccall<int>(keyboard, count, pad);
                for (int i = 0; i < std::min(count, 27); ++i)
                    pad[i] = UiIndex(ModernFileOrder[i]);
                return result;
            }));

            // Analog sticks and mouse
            auto mouse = hook::pattern("8B 0D ? ? ? ? 33 C0 85 C9 74 05 B8 01 00 00").count(2);
            nMouseX.SetAddress(*mouse.get(0).get<int32_t*>(2));
            nMouseY.SetAddress(*mouse.get(1).get<int32_t*>(2));
            auto inputFrame = hook::pattern("E8 ? ? ? ? 8B C8 E9 ? ? ? ? 90 90 90 90 55 8B EC 83 E4 F8 81 EC 1C 02 00 00");
            InputFrameHook = safetyhook::create_inline(inputFrame.get_first(), &InputFrame);
            RawMouse::Enabled = true;
            SHDisplay::OnWindowMessage().push_back([](HWND window, UINT message, WPARAM wParam, LPARAM lParam, LRESULT&)
            {
                RawMouse::Message(window, message, wParam, lParam);
                return false;
            });
            LinearizeNativeMouse();
            auto analog = hook::pattern("E8 ? ? ? ? 66 8B 48 3C 8B 54 24 04 66 89 0A 66 8B 48 3E 8B 54 24 08");
            AnalogHook = safetyhook::create_inline(analog.get_first(), &AnalogWords);

            // First person: looking up and down responds linearly, like turning (it was squared)
            auto pitch = hook::pattern("D9 C0 D8 C9 DE CA DD D8 D9 16 D8 47 60 D9 5F 60");
            injector::WriteMemory<uint16_t>(pitch.get_first(2), 0xE1D9, true);   // fmul st, st(1) -> fabs

            // Pad names in the button configuration screen
            auto names = hook::pattern("8B 2C 85 ? ? ? ? E8 ? ? ? ? 6A 00 6A 00 53 81 C6 ? 01 00 00 56 55 6A 00 E8 ? ? ? ?");
            auto fromAscii = hook::pattern("A1 ? ? ? ? 56 0F B7 35 ? ? ? ? C1 E6 08 81 C6 ? ? ? ? 40 0F");
            auto drawText = hook::pattern("83 EC 50 56 8B 74 24 58 33 C0 3B F0 75 19 A3 ? ? ? ? A3 ? ? ? ?");
            auto endText = hook::pattern("8B 44 24 04 A3 ? ? ? ? C3 90 90 90 90 90 90 83 EC 14 57");
            aPadMessageIds.SetAddress(*names.get_first<int32_t*>(3));
            TextFromAscii = reinterpret_cast<decltype(TextFromAscii)>(fromAscii.get_first());
            DrawGameText = reinterpret_cast<decltype(DrawGameText)>(drawText.get_first());
            EndText = reinterpret_cast<decltype(EndText)>(endText.get_first());
            DrawMessage = reinterpret_cast<DrawMessageFn>(injector::GetBranchDestination(names.get_first(0x1B)).as_int());
            injector::MakeCALL(names.get_first(0x1B), &DrawPadElement, true);

            // Rumble
            auto on = hook::pattern("8B 44 24 08 83 F8 01 75 0A 50 E8 ? ? ? ? 83 C4 04 C3 83 F8 02 75 07 50 E8 ? ? ? ? 59 C3 8B 44 24 08 83 F8 01 75");
            auto off = hook::pattern("8B 44 24 08 83 F8 01 75 0A 50 E8 ? ? ? ? 83 C4 04 C3 83 F8 02 75 07 50 E8 ? ? ? ? 59 C3 56 33 F6 6A 00 6A 00 6A");
            auto stop = hook::pattern("56 33 F6 6A 00 6A 00 6A 00 56 E8 ? ? ? ? 83 C4 10 46 83 FE 04 7C EB");
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
            WFP::onShutdownEvent() += []()
            {
                if (RumbleSlot >= 0)
                    SetRumble(RumbleSlot, 0, 0);
            };
        };
    }
} Input;
