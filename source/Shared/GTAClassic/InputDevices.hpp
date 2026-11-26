#pragma once
#include "Shared/PadInput.hpp"
#include "Shared/RWGTA/GameThread.hpp"
#include <atomic>
#include <algorithm>
#include <cmath>
#include <utility>

// Keyboard, mouse and XInput controller state for the modern controls, and their
// settings ([INPUT] in the ini). Keys are Windows virtual-key codes.
namespace InputDevices
{
    inline HWND Window = nullptr;                   // the game window; input counts only while it has focus
    inline bool Frontend = true;                    // the menus are showing

    inline float ClearDeadzone(float value)
    {
        return std::isfinite(value) ? std::clamp(value, 0.05f, 0.90f) : 0.20f;
    }
    inline std::pair<float, float> Stick(float x, float y, float deadzone)
    {
        deadzone = ClearDeadzone(deadzone);
        auto length = std::hypot(x, y);
        if (!std::isfinite(length) || length <= deadzone) return {};
        auto magnitude = (std::min(length, 1.0f) - deadzone) / (1.0f - deadzone);
        return {x / length * magnitude, y / length * magnitude};
    }

    inline bool ModernControls = true;
    inline bool Gamepad = true;
    inline bool StickThrottle = false;
    inline float Deadzone = 0.20f;
    inline int MoveUpKey = 'W', MoveDownKey = 'S', MoveLeftKey = 'A', MoveRightKey = 'D';
    inline int FireKey = VK_LBUTTON, EnterVehicleKey = 'F', JumpKey = VK_SPACE;
    inline int SpecialKey = VK_SHIFT, PreviousWeaponKey = 'Q', NextWeaponKey = 'E', PauseKey = 'P';
    inline XINPUT_STATE Pad{};
    inline DWORD LastPoll = 0;
    inline bool PadConnected = false;
    inline DWORD PadSlot = 0;
    inline std::atomic_bool PadLast{ false };                    // the pad was used last: prompts show its buttons
    inline void UsePad(bool pad)
    {
        PadLast = pad;
    }
    inline std::atomic<uint32_t> PendingKeys[8]{};
    inline std::atomic<uint32_t> FrameKeys[8]{};
    inline void KeyboardMessage(UINT message, WPARAM value, LPARAM flags = 0)
    {
        int key = 0;
        if (message == WM_KEYDOWN || message == WM_SYSKEYDOWN) key = int(value);
        else if (message == WM_LBUTTONDOWN) key = VK_LBUTTON;
        else if (message == WM_RBUTTONDOWN) key = VK_RBUTTON;
        else if (message == WM_MBUTTONDOWN) key = VK_MBUTTON;
        else if (message == WM_XBUTTONDOWN) key = GET_XBUTTON_WPARAM(value) == XBUTTON1 ? VK_XBUTTON1 : VK_XBUTTON2;
        if (key > 0 && key < 256) PendingKeys[key / 32].fetch_or(1u << (key % 32));
        const bool repeat = (message == WM_KEYDOWN || message == WM_SYSKEYDOWN) && (flags & (1u << 30));
        if ((key > 0 && !repeat) || message == WM_MOUSEWHEEL || message == WM_MOUSEHWHEEL)
            UsePad(false);
        static POINT origin{};
        static bool haveOrigin = false;
        if (message == WM_MOUSEMOVE)
        {
            POINT now{};
            if (GetCursorPos(&now))
            {
                const bool moved = haveOrigin && std::abs(now.x - origin.x) + std::abs(now.y - origin.y) > 24;
                if (!haveOrigin || moved)
                    origin = now;
                haveOrigin = true;
                if (moved)
                    UsePad(false);
            }
        }
        if (message == WM_KILLFOCUS)
        {
            haveOrigin = false;
            for (int i = 0; i < 8; ++i) { PendingKeys[i] = 0; FrameKeys[i] = 0; }
        }
    }
    inline void BeginInputFrame()
    {
        RWGTA::GameThread::Run();
        for (int i = 0; i < 8; ++i) FrameKeys[i] = PendingKeys[i].exchange(0);
    }
    inline bool Focused()
    {
        if (!Window) return false;
        return GetForegroundWindow() == Window;
    }

    using PadInput::XInput;
    using PadInput::Family;

    inline Family CurrentFamily()
    {
        return PadInput::ResolveFamily(FixSettings().Int("MAIN.GamepadIcons"));
    }

    // The game's fonts have no button symbols.
    inline const wchar_t* ButtonName(WORD button)
    {
        return PadInput::ButtonName(button, CurrentFamily());
    }

    // Rumble: a pulse of a strength 0..1 that fades out over its length
    inline float RumbleStrength = 0.0f;
    inline ULONGLONG RumbleStart = 0, RumbleLength = 0;
    inline DWORD PulseSlot = XUSER_MAX_COUNT;
    inline bool MotorRunning[XUSER_MAX_COUNT]{};
    inline float RumbleNow(ULONGLONG now)
    {
        if (!RumbleLength || now - RumbleStart >= RumbleLength)
            return 0.0f;
        return RumbleStrength * (1.0f - float(now - RumbleStart) / float(RumbleLength));
    }
    inline void Rumble(float strength, ULONGLONG milliseconds)
    {
        if (!FixSettings().Bool("MAIN.Vibration") || !PadConnected || !PadLast)
            return;
        auto now = GetTickCount64();
        if (PadSlot == PulseSlot && strength < RumbleNow(now))
            return;
        RumbleStrength = std::clamp(strength, 0.0f, 1.0f);
        PulseSlot = PadSlot;
        RumbleStart = now;
        RumbleLength = milliseconds;
    }
    // On the game's thread; stop: paused, a menu, no focus or vibration off
    inline void UpdateRumble(bool stop)
    {
        auto setState = XInput().setState;
        if (!setState)
            return;
        if (stop || !PadConnected || PadSlot != PulseSlot || !FixSettings().Bool("MAIN.Vibration"))
            RumbleLength = 0;
        const float strength = RumbleNow(GetTickCount64());
        for (DWORD slot = 0; slot < XUSER_MAX_COUNT; ++slot)
        {
            const float level = slot == PulseSlot ? strength : 0.0f;
            if (level == 0.0f && !MotorRunning[slot])
                continue;
            XINPUT_VIBRATION vibration{ WORD(level * 65535.0f), WORD(level * 0.6f * 65535.0f) };
            const auto result = setState(slot, &vibration);
            if (result == ERROR_SUCCESS || result == ERROR_DEVICE_NOT_CONNECTED)
                MotorRunning[slot] = result == ERROR_SUCCESS && level > 0.0f;
        }
    }

    inline void Read(SettingsReader& ini)
    {
        ModernControls = ini.ReadBoolean("INPUT", "ModernControls", true);
        Gamepad = ini.ReadBoolean("INPUT", "Gamepad", true);
        StickThrottle = ini.ReadBoolean("INPUT", "StickThrottle", false);
        auto value = ini.ReadFloat("INPUT", "StickDeadzone", 0.20f);
        Deadzone = ClearDeadzone(value);
        MoveUpKey = ini.ReadInteger("INPUT", "MoveUpKey", 'W');
        MoveDownKey = ini.ReadInteger("INPUT", "MoveDownKey", 'S');
        MoveLeftKey = ini.ReadInteger("INPUT", "MoveLeftKey", 'A');
        MoveRightKey = ini.ReadInteger("INPUT", "MoveRightKey", 'D');
        FireKey = ini.ReadInteger("INPUT", "FireKey", VK_LBUTTON);
        EnterVehicleKey = ini.ReadInteger("INPUT", "EnterVehicleKey", 'F');
        JumpKey = ini.ReadInteger("INPUT", "JumpKey", VK_SPACE);
        SpecialKey = ini.ReadInteger("INPUT", "SpecialKey", VK_SHIFT);
        PreviousWeaponKey = ini.ReadInteger("INPUT", "PreviousWeaponKey", 'Q');
        NextWeaponKey = ini.ReadInteger("INPUT", "NextWeaponKey", 'E');
        PauseKey = ini.ReadInteger("INPUT", "PauseKey", 'P');
    }
    // Changes from FusionFixHub or a preset apply at once, as from the in-game options
    inline void Watch()
    {
        for (auto id : { "INPUT.ModernControls", "INPUT.Gamepad", "INPUT.StickThrottle", "INPUT.StickDeadzone", "INPUT.MoveUpKey",
            "INPUT.MoveDownKey", "INPUT.MoveLeftKey", "INPUT.MoveRightKey", "INPUT.FireKey", "INPUT.EnterVehicleKey", "INPUT.JumpKey",
            "INPUT.SpecialKey", "INPUT.PreviousWeaponKey", "INPUT.NextWeaponKey", "INPUT.PauseKey" })
            FixSettings().OnChange(id, [] { RWGTA::GameThread::Post([] { SettingsReader ini(""); Read(ini); }); });
    }

    inline float Axis(SHORT value) { return value / (value < 0 ? 32768.0f : 32767.0f); }

    inline bool NewPadActivity(const XINPUT_GAMEPAD& pad, XINPUT_GAMEPAD& previous)
    {
        const auto stickMoved = [](SHORT x, SHORT y, SHORT& lastX, SHORT& lastY)
        {
            const auto outside = [](SHORT a, SHORT b) { return std::hypot(Axis(a), Axis(b)) > Deadzone; };
            if (!outside(x, y))
            {
                lastX = lastY = 0;
                return false;
            }
            if (outside(lastX, lastY) && std::hypot(Axis(x) - Axis(lastX), Axis(y) - Axis(lastY)) < 0.05f)
                return false;
            lastX = x;
            lastY = y;
            return true;
        };
        const bool left = stickMoved(pad.sThumbLX, pad.sThumbLY, previous.sThumbLX, previous.sThumbLY);
        const bool right = stickMoved(pad.sThumbRX, pad.sThumbRY, previous.sThumbRX, previous.sThumbRY);
        const bool pressed = (pad.wButtons & ~previous.wButtons) ||
            (pad.bLeftTrigger > 30 && previous.bLeftTrigger <= 30) || (pad.bRightTrigger > 30 && previous.bRightTrigger <= 30);
        previous.wButtons = pad.wButtons;
        previous.bLeftTrigger = pad.bLeftTrigger;
        previous.bRightTrigger = pad.bRightTrigger;
        return left || right || pressed;
    }

    inline void Poll()
    {
        static XINPUT_GAMEPAD previous{};
        if (!Gamepad || !Focused())
        {
            if (!Gamepad)
                UsePad(false);
            Pad = {};
            previous = {};
            PadConnected = false;
            return;
        }
        auto now = GetTickCount();
        if (now == LastPoll) return;
        LastPoll = now;
        Pad = {};
        const auto previousSlot = PadSlot;
        PadConnected = PadInput::Read(Pad, PadSlot);
        if (!PadConnected)
        {
            previous = {};
            UsePad(false);
            return;
        }
        if (previousSlot != PadSlot)
            previous = {};
        if (NewPadActivity(Pad.Gamepad, previous))
            UsePad(true);
    }

    inline bool Key(int key)
    {
        return key > 0 && key < 256 && Focused() &&
            ((GetAsyncKeyState(key) & 0x8000) || (FrameKeys[key / 32].load() & (1u << (key % 32))));
    }
    inline bool Button(WORD button) { return PadConnected && (Pad.Gamepad.wButtons & button); }

    inline std::pair<float, float> Stick(bool right = false)
    {
        if (!PadConnected) return {};
        float x = Axis(right ? Pad.Gamepad.sThumbRX : Pad.Gamepad.sThumbLX);
        float y = -Axis(right ? Pad.Gamepad.sThumbRY : Pad.Gamepad.sThumbLY);
        return Stick(x, y, Deadzone);
    }

    inline std::pair<float, float> Movement(bool vehicle = false)
    {
        Poll();
        auto [x, y] = Stick();
        if (vehicle && !StickThrottle) y = 0;
        if (ModernControls)
        {
            x += Key(MoveRightKey) - Key(MoveLeftKey);
            y += Key(MoveDownKey) - Key(MoveUpKey);
        }
        x += Button(XINPUT_GAMEPAD_DPAD_RIGHT) - Button(XINPUT_GAMEPAD_DPAD_LEFT);
        if (!vehicle || StickThrottle) y += Button(XINPUT_GAMEPAD_DPAD_DOWN) - Button(XINPUT_GAMEPAD_DPAD_UP);
        auto length = std::hypot(x, y);
        if (length > 1.0f) { x /= length; y /= length; }
        return { x, y };
    }
}
