module;
#include <stdafx.h>
#include "Shared/PadInput.hpp"
#include "FixSettings.hpp"

export module PadLabels;
import Frame;
import DeveloperOverlay;
import PadBindings;


export namespace KnightRiderPadLabels
{
    inline SafetyHookInline NameHook, DestroyHook;
    inline void* (__thiscall* Construct)(void*, const char*) = nullptr;
    inline void (__thiscall* Refresh)(void*) = nullptr;
    inline uintptr_t FirstLabelReturn = 0;
    inline void* CurrentMenu = nullptr;
    inline std::atomic<bool> UsingPad{ false }, RefreshRequested{ false };

    inline const char* Label(int key)
    {
        static constexpr const char* buttons[3][16]
        {
            { "[A]", "[B]", "[X]", "[Y]", "[LB]", "[RB]", "[LT]", "[RT]", "[VIEW]", "[MENU]", "[LS]", "[RS]", "[DPAD UP]", "[DPAD DOWN]", "[DPAD LEFT]", "[DPAD RIGHT]" },
            { "[CROSS]", "[CIRCLE]", "[SQUARE]", "[TRIANGLE]", "[L1]", "[R1]", "[L2]", "[R2]", "[SHARE]", "[OPTIONS]", "[L3]", "[R3]", "[DPAD UP]", "[DPAD DOWN]", "[DPAD LEFT]", "[DPAD RIGHT]" },
            { "[B]", "[A]", "[Y]", "[X]", "[L]", "[R]", "[ZL]", "[ZR]", "[-]", "[+]", "[LS]", "[RS]", "[DPAD UP]", "[DPAD DOWN]", "[DPAD LEFT]", "[DPAD RIGHT]" }
        };
        const auto family = PadInput::ResolveFamily(std::clamp(FixSettings().Int("MAIN.GamepadIcons"), 0, 3));
        if (key >= 7 && key <= 22)
            return buttons[static_cast<int>(family) - 1][key - 7];
        if (key == 3)
            return "[LS LEFT]";
        if (key == 4)
            return "[LS RIGHT]";
        if (key == 2)
            return buttons[static_cast<int>(family) - 1][6];
        if (key == 5 || key == 1)
            return buttons[static_cast<int>(family) - 1][7];
        return nullptr;
    }

    inline void* __fastcall Name(void* menu, void*, void* output, int key, bool keyboard)
    {
        CurrentMenu = menu;
        const char* label = nullptr;
        if (!keyboard)
            label = Label(key);
        else if (UsingPad.load(std::memory_order_relaxed))
        {
            const auto address = reinterpret_cast<uintptr_t>(_ReturnAddress());
            // The native page builds its normal and highlighted labels in fourteen pairs.
            if (address >= FirstLabelReturn && address < FirstLabelReturn + 28 * 57 && (address - FirstLabelReturn) % 57 == 0)
            {
                static constexpr int actions[14]{ 5, 6, 14, 3, 4, 13, 11, 12, 15, 17, 16, 18, 19, 2 };
                label = Label(KnightRiderPadBindings::DefaultKey(actions[(address - FirstLabelReturn) / (2 * 57)]));
            }
        }
        if (!label)
            return NameHook.thiscall<void*>(menu, output, key, keyboard);
        return Construct(output, label);
    }

    inline void Keyboard()
    {
        if (UsingPad.exchange(false))
            RefreshRequested = true;
    }

    inline void Frame()
    {
        XINPUT_STATE state{};
        DWORD slot = 0;
        static DWORD previousPacket = 0;
        static bool connected = false;
        const bool present = GetModuleHandleW(L"Xidi.32.dll") && PadInput::Read(state, slot);
        uint16_t overlayButtons = 0;
        if (present)
        {
            const auto buttons = state.Gamepad.wButtons;
            const auto map = [&](WORD source, uint16_t target)
            {
                if (buttons & source)
                    overlayButtons |= target;
            };
            map(XINPUT_GAMEPAD_BACK, 0x1);
            map(XINPUT_GAMEPAD_START, 0x8);
            map(XINPUT_GAMEPAD_DPAD_UP, 0x10);
            map(XINPUT_GAMEPAD_DPAD_RIGHT, 0x20);
            map(XINPUT_GAMEPAD_DPAD_DOWN, 0x40);
            map(XINPUT_GAMEPAD_DPAD_LEFT, 0x80);
            map(XINPUT_GAMEPAD_LEFT_SHOULDER, 0x400);
            map(XINPUT_GAMEPAD_RIGHT_SHOULDER, 0x800);
            map(XINPUT_GAMEPAD_Y, 0x1000);
            map(XINPUT_GAMEPAD_B, 0x2000);
            map(XINPUT_GAMEPAD_A, 0x4000);
            map(XINPUT_GAMEPAD_X, 0x8000);
        }
        KnightRiderOverlay::Overlay.Gamepad(overlayButtons,
            present ? static_cast<int8_t>(state.Gamepad.sThumbLX / 256) : 0,
            present ? static_cast<int8_t>(-state.Gamepad.sThumbLY / 256) : 0);
        if (present)
        {
            const auto& pad = state.Gamepad;
            const bool active = pad.wButtons || pad.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD
                || pad.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD
                || std::abs(static_cast<int>(pad.sThumbLX)) > 4915
                || std::abs(static_cast<int>(pad.sThumbLY)) > 4915
                || std::abs(static_cast<int>(pad.sThumbRX)) > XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE
                || std::abs(static_cast<int>(pad.sThumbRY)) > XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE;
            if (active && (!connected || previousPacket != state.dwPacketNumber)
                && GetForegroundWindow() == WindowedModeWrapper::GameHWND && !UsingPad.exchange(true))
                RefreshRequested = true;
            previousPacket = state.dwPacketNumber;
        }
        else if (connected)
            Keyboard();
        connected = present;
        if (RefreshRequested.exchange(false) && CurrentMenu)
            Refresh(CurrentMenu);
    }

    inline void* __fastcall Destroy(void* menu, void*)
    {
        if (CurrentMenu == menu)
            CurrentMenu = nullptr;
        return DestroyHook.thiscall<void*>(menu);
    }

    inline void Init()
    {
        auto construct = hook::pattern("8B 54 24 04 56 85 D2 8B F1 74 10 57 8B FA 83 C9 FF");
        Construct = reinterpret_cast<decltype(Construct)>(construct.get_first());
        auto name = hook::pattern("51 56 57 8B F9 8D 4C 24 08 E8 ? ? ? ? 8A 44 24 18 84 C0");
        NameHook = safetyhook::create_inline(name.get_first(), Name);
        auto first = hook::pattern("53 56 57 8B 7C 24 10 8B F1 8B CF E8 ? ? ? ? 66 8B 00 66 3B 05");
        FirstLabelReturn = reinterpret_cast<uintptr_t>(first.get_first(0x33));
        auto refresh = hook::pattern("8B 81 C4 02 00 00 05 E0 00 00 00 50 E8 ? ? ? ? C3");
        Refresh = reinterpret_cast<decltype(Refresh)>(refresh.get_first());
        auto destroy = hook::pattern("53 56 8B F1 57 8D 8E 6C 08 00 00 E8");
        DestroyHook = safetyhook::create_inline(destroy.get_first(), Destroy);
        FixSettings().OnChange("MAIN.GamepadIcons", []() { RefreshRequested = true; });
        KnightRiderFrame::InputFrame = Frame;
        KnightRiderFrame::KeyboardInput = Keyboard;
    }
}
