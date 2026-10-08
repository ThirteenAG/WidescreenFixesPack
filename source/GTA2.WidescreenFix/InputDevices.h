#pragma once
#include <Xinput.h>
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
    inline float Deadzone = 0.20f;
    inline int MoveUpKey = 'W', MoveDownKey = 'S', MoveLeftKey = 'A', MoveRightKey = 'D';
    inline int FireKey = VK_LBUTTON, EnterVehicleKey = 'F', JumpKey = VK_SPACE;
    inline int SpecialKey = VK_SHIFT, PreviousWeaponKey = 'Q', NextWeaponKey = 'E', PauseKey = 'P';
    inline XINPUT_STATE Pad{};
    inline DWORD LastPoll = 0;
    inline bool PadConnected = false;
    inline std::atomic<uint32_t> PendingKeys[8]{};
    inline std::atomic<uint32_t> FrameKeys[8]{};
    inline void KeyboardMessage(UINT message, WPARAM value)
    {
        int key = 0;
        if (message == WM_KEYDOWN) key = int(value);
        else if (message == WM_LBUTTONDOWN) key = VK_LBUTTON;
        else if (message == WM_RBUTTONDOWN) key = VK_RBUTTON;
        if (key > 0 && key < 256) PendingKeys[key / 32].fetch_or(1u << (key % 32));
        if (message == WM_KILLFOCUS)
            for (int i = 0; i < 8; ++i) { PendingKeys[i] = 0; FrameKeys[i] = 0; }
    }
    inline void BeginInputFrame()
    {
        for (int i = 0; i < 8; ++i) FrameKeys[i] = PendingKeys[i].exchange(0);
    }
    inline bool Focused()
    {
        if (!Window) return false;
        return GetForegroundWindow() == Window;
    }

    inline void Read(CIniReader& ini)
    {
        ModernControls = ini.ReadBoolean("INPUT", "ModernControls", true);
        Gamepad = ini.ReadBoolean("INPUT", "Gamepad", true);
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

    inline void Poll()
    {
        if (!Gamepad || !Focused())
        {
            Pad = {};
            PadConnected = false;
            return;
        }
        auto now = GetTickCount();
        if (now == LastPoll) return;
        LastPoll = now;
        Pad = {};
        PadConnected = false;
        // Load system XInput only; no redistributable or import-time dependency.
        static auto getState = []() -> decltype(&XInputGetState)
        {
            for (auto name : { L"xinput1_4.dll", L"xinput9_1_0.dll", L"xinput1_3.dll" })
                if (auto dll = LoadLibraryExW(name, nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32))
                    if (auto fn = GetProcAddress(dll, "XInputGetState"))
                        return reinterpret_cast<decltype(&XInputGetState)>(fn);
            return nullptr;
        }();
        if (!getState) return;
        for (DWORD i = 0; i < XUSER_MAX_COUNT; ++i)
            if (getState(i, &Pad) == ERROR_SUCCESS) { PadConnected = true; break; }
    }

    inline bool Key(int key)
    {
        return key > 0 && key < 256 && Focused() &&
            ((GetAsyncKeyState(key) & 0x8000) || (FrameKeys[key / 32].load() & (1u << (key % 32))));
    }
    inline bool Button(WORD button) { return PadConnected && (Pad.Gamepad.wButtons & button); }
    inline float Axis(SHORT value) { return value / (value < 0 ? 32768.0f : 32767.0f); }
    inline std::pair<float, float> Stick(bool right = false)
    {
        if (!PadConnected) return {};
        float x = Axis(right ? Pad.Gamepad.sThumbRX : Pad.Gamepad.sThumbLX);
        float y = -Axis(right ? Pad.Gamepad.sThumbRY : Pad.Gamepad.sThumbLY);
        return Stick(x, y, Deadzone);
    }

    inline std::pair<float, float> Movement()
    {
        Poll();
        auto [x, y] = Stick();
        if (ModernControls)
        {
            x += Key(MoveRightKey) - Key(MoveLeftKey);
            y += Key(MoveDownKey) - Key(MoveUpKey);
        }
        x += Button(XINPUT_GAMEPAD_DPAD_RIGHT) - Button(XINPUT_GAMEPAD_DPAD_LEFT);
        y += Button(XINPUT_GAMEPAD_DPAD_DOWN) - Button(XINPUT_GAMEPAD_DPAD_UP);
        auto length = std::hypot(x, y);
        if (length > 1.0f) { x /= length; y /= length; }
        return { x, y };
    }
}
