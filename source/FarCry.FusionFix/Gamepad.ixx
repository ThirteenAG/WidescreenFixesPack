module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/PadInput.hpp"
#include <dinput.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>

export module Gamepad;

import Window;

// Native pad play through the engine's own action map. The pad is read through XInput; gameplay actions go straight
// to the action sink with analog look values, menus get a pad-driven cursor plus mouse and key presses in the
// DirectInput data the engine reads.
namespace FarCryPadState
{
#ifdef _WIN64
    constexpr size_t inputMouseSlot = 36, inputManagerSlot = 44;
    constexpr size_t managerMapOffset = 96, managerSinkOffset = 104, managerInvertOffset = 116, managerEnabledOffset = 117;
    constexpr size_t scriptPlayerOffset = 40, playerLocalOffset = 592, shakeDegreeOffset = 416, shakeAmplitudeOffset = 460;
#else
    constexpr size_t inputMouseSlot = 54, inputManagerSlot = 62;
    constexpr size_t managerMapOffset = 48, managerSinkOffset = 52, managerInvertOffset = 60, managerEnabledOffset = 61;
    constexpr size_t scriptPlayerOffset = 20, playerLocalOffset = 556, shakeDegreeOffset = 408, shakeAmplitudeOffset = 452;
#endif
    constexpr size_t inputUpdateSlot = 7, managerSetMapSlot = 9, managerUpdateSlot = 13;
    constexpr size_t mouseSetXSlot = 10, mouseSetYSlot = 11, mouseGetXSlot = 12, mouseGetYSlot = 13;

    template<typename R, typename... A>
    R Virtual(void* object, size_t slot, A... arguments)
    {
#ifdef _WIN64
        using Function = R(__fastcall*)(void*, A...);
#else
        using Function = R(__thiscall*)(void*, A...);
#endif
        return reinterpret_cast<Function>((*static_cast<uintptr_t**>(object))[slot])(object, arguments...);
    }

    template<typename T>
    T& Field(void* object, size_t offset)
    {
        return *reinterpret_cast<T*>(static_cast<uint8_t*>(object) + offset);
    }

    void* VirtualTarget(void* object, size_t slot)
    {
        return reinterpret_cast<void*>((*static_cast<uintptr_t**>(object))[slot]);
    }

    // Game action ids and activation events
    enum Action
    {
        MoveLeft = 1, MoveRight = 2, MoveForward = 3, MoveBackward = 4, Jump = 5, Fire = 7, ScoreBoard = 8,
        Reload = 9, Use = 10, TurnLR = 11, TurnUD = 12, NextWeapon = 14, LeanRight = 16, HoldBreath = 17,
        FireMode = 18, LeanLeft = 19, FireGrenade = 20, CycleGrenade = 30, MoveLR = 33, MoveFB = 34,
        Binoculars = 35, Thermal = 36, ZoomToggle = 45, ZoomIn = 46, ZoomOut = 47, Prone = 49, Flashlight = 57,
        ChangeView = 58, Sprint = 59, CrouchToggle = 63,
    };
    enum Event { Pressing = 0, Holding = 1, Releasing = 2 };

    enum Map : uint8_t { Default = 1, Zoom = 2, Binozoom = 4, Vehicle = 8, Dead = 16, Alive = Default | Zoom | Binozoom | Vehicle };

    // Pad buttons; the triggers get two of the bits XInput leaves free
    constexpr uint32_t LeftTrigger = 0x10000, RightTrigger = 0x20000;

    enum class Mode { Hold, Press, PressRelease };

    struct Binding
    {
        uint32_t button;
        uint8_t maps;
        int tap;
        Mode mode;
        int hold = 0;
    };

    // Console-style layout; a button with a hold action runs its tap action on a short press
    constexpr Binding Layout[] =
    {
        { RightTrigger, Default | Zoom | Vehicle | Dead, Fire, Mode::Hold },
        { LeftTrigger, Default | Zoom, ZoomToggle, Mode::PressRelease },
        { XINPUT_GAMEPAD_RIGHT_SHOULDER, Alive, FireGrenade, Mode::Hold },
        { XINPUT_GAMEPAD_LEFT_SHOULDER, Default | Zoom | Binozoom, Binoculars, Mode::Press },
        { XINPUT_GAMEPAD_A, Default | Binozoom | Vehicle, Jump, Mode::Hold },
        { XINPUT_GAMEPAD_A, Zoom, HoldBreath, Mode::Hold },
        { XINPUT_GAMEPAD_A, Dead, Fire, Mode::Hold },
        { XINPUT_GAMEPAD_B, Alive, CrouchToggle, Mode::Press, Prone },
        { XINPUT_GAMEPAD_X, Alive, Reload, Mode::Press, Use },
        { XINPUT_GAMEPAD_Y, Default, NextWeapon, Mode::Press, CycleGrenade },
        { XINPUT_GAMEPAD_Y, Vehicle, ChangeView, Mode::Press, CycleGrenade },
        { XINPUT_GAMEPAD_Y, Zoom | Binozoom, CycleGrenade, Mode::Press },
        { XINPUT_GAMEPAD_DPAD_UP, Default | Vehicle, Flashlight, Mode::Press },
        { XINPUT_GAMEPAD_DPAD_DOWN, Default | Vehicle, Thermal, Mode::Press },
        { XINPUT_GAMEPAD_DPAD_UP, Zoom | Binozoom, ZoomIn, Mode::Press },
        { XINPUT_GAMEPAD_DPAD_DOWN, Zoom | Binozoom, ZoomOut, Mode::Press },
        { XINPUT_GAMEPAD_DPAD_LEFT, Default | Zoom | Binozoom, LeanLeft, Mode::Hold },
        { XINPUT_GAMEPAD_DPAD_RIGHT, Default | Zoom | Binozoom, LeanRight, Mode::Hold },
        { XINPUT_GAMEPAD_RIGHT_THUMB, Alive, FireMode, Mode::Press },
        { XINPUT_GAMEPAD_BACK, Alive, ScoreBoard, Mode::Hold },
    };

    constexpr double HoldTime = 0.3;

    struct Pad
    {
        bool connected = false;
        DWORD slot = 0;
        uint32_t buttons = 0;
        float lx = 0, ly = 0, rx = 0, ry = 0;
    };

    struct ButtonState
    {
        bool active = false;
        bool holdFired = false;
        double since = 0;
    };

    bool (*InputBlocked)() = nullptr;
    Pad Current, Previous;
    uint32_t BlockedButtons = 0;
    std::array<ButtonState, 18> Buttons{};
    bool SprintLatch = false;
    float LookBoostTime = 0;

    void* Input = nullptr;
    void* Manager = nullptr;
    void* Maps[5]{};
    uint8_t CurrentMap = Default;
    uint64_t InputFrame = 0, ManagerFrame = 0;
    bool ManagerEnabled = false, Gameplay = false, AnalogMove = false;
    double ManagerTime = 0, CursorTime = 0, WheelTime = 0;

    // Injected DirectInput state
    bool MouseButton = false, MouseReported = false;
    int MouseWheel = 0;
    std::array<bool, 256> Keys{}, KeysReported{};

    // Rumble
    struct Rumble
    {
        float low = 0, high = 0;
        double until = 0;
    };
    std::array<Rumble, 4> Rumbles{};
    WORD MotorLow = 0, MotorHigh = 0;

    SafetyHookInline CreateInputHook, UpdateHook, ManagerHook, ManagerUpdateHook, SetMapHook, ShakeHook, ShakeLHook;

    double Now()
    {
        static const double frequency = []
        {
            LARGE_INTEGER value{};
            QueryPerformanceFrequency(&value);
            return double(value.QuadPart);
        }();
        LARGE_INTEGER value{};
        QueryPerformanceCounter(&value);
        return double(value.QuadPart) / frequency;
    }

    int ButtonIndex(uint32_t button)
    {
        unsigned long index = 0;
        _BitScanForward(&index, button);
        return int(index);
    }

    bool Down(uint32_t button) { return (Current.buttons & button) && !(BlockedButtons & button); }
    bool WasDown(uint32_t button) { return (Previous.buttons & button) && !(BlockedButtons & button); }
    bool Pressed(uint32_t button) { return Down(button) && !(Previous.buttons & button); }
    bool Released(uint32_t button) { return !(Current.buttons & button) && WasDown(button); }

    float Axis(SHORT value)
    {
        return std::max(-1.0f, float(value) / 32767.0f);
    }

    // Radial dead zone with the remaining range stretched back to 0..1
    std::pair<float, float> Stick(float x, float y, float deadZone, float& magnitude)
    {
        const float length = std::sqrt(x * x + y * y);
        if (length <= deadZone)
        {
            magnitude = 0;
            return { 0.0f, 0.0f };
        }
        magnitude = std::min(1.0f, (length - deadZone) / (1.0f - deadZone));
        return { x / length * magnitude, y / length * magnitude };
    }

    bool Focused()
    {
        const auto window = FarCryWindow::Window;
        return window && GetForegroundWindow() == window;
    }

    void Poll(bool focus)
    {
        Previous = Current;
        Current = {};
        XINPUT_STATE state{};
        DWORD slot = 0;
        if (!focus || !PadInput::Read(state, slot))
            return;
        const auto& pad = state.Gamepad;
        Current.connected = true;
        Current.slot = slot;
        Current.buttons = pad.wButtons;
        if (pad.bLeftTrigger > 60)
            Current.buttons |= LeftTrigger;
        if (pad.bRightTrigger > 60)
            Current.buttons |= RightTrigger;
        Current.lx = Axis(pad.sThumbLX);
        Current.ly = Axis(pad.sThumbLY);
        Current.rx = Axis(pad.sThumbRX);
        Current.ry = Axis(pad.sThumbRY);
    }

    // A button held while play and menus swap stays silent until it is let go
    void SwitchMode(bool gameplay)
    {
        if (gameplay == Gameplay)
            return;
        Gameplay = gameplay;
        BlockedButtons = Current.buttons;
        Buttons = {};
        SprintLatch = false;
        LookBoostTime = 0;
    }

    void Notify(int action, float value, Event event)
    {
        if (!Manager || !Field<bool>(Manager, managerEnabledOffset))
            return;
        if (auto sink = Field<void*>(Manager, managerSinkOffset))
            Virtual<void>(sink, 0, action, value, int(event));
    }

    void Actions(double now, float dt)
    {
        uint32_t handled = 0;
        for (const auto& binding : Layout)
        {
            if (!(binding.maps & CurrentMap))
                continue;
            const auto button = binding.button;
            if (handled & button)
                continue;
            handled |= button;
            auto& state = Buttons[ButtonIndex(button)];
            if (binding.hold)
            {
                if (Pressed(button))
                    state = { true, false, now };
                if (!state.active)
                    continue;
                if (Down(button) && !state.holdFired && now - state.since >= HoldTime)
                {
                    state.holdFired = true;
                    Notify(binding.hold, 0, Pressing);
                }
                if (!Down(button))
                {
                    if (!state.holdFired)
                        Notify(binding.tap, 0, Pressing);
                    state = {};
                }
                continue;
            }
            switch (binding.mode)
            {
                case Mode::Hold:
                    if (Down(button))
                        Notify(binding.tap, 0, Holding);
                    break;
                case Mode::Press:
                    if (Pressed(button))
                        Notify(binding.tap, 0, Pressing);
                    break;
                case Mode::PressRelease:
                    if (Pressed(button))
                        Notify(binding.tap, 0, Pressing);
                    else if (Released(button))
                        Notify(binding.tap, 0, Releasing);
                    break;
            }
        }

        float moveMagnitude = 0;
        const auto [mx, my] = Stick(Current.lx, Current.ly, 0.24f, moveMagnitude);
        if (CurrentMap == Dead)
        {
            if (mx < -0.5f)
                Notify(MoveLeft, 0, Holding);
            else if (mx > 0.5f)
                Notify(MoveRight, 0, Holding);
            return;
        }
        if (!(CurrentMap & Alive))
            return;

        if (moveMagnitude > 0)
        {
            if (AnalogMove)
            {
                // The game removes its own 0.2 dead zone and rescales, so the stick value is mapped past it
                if (mx != 0)
                    Notify(MoveLR, std::copysign(0.2f + 0.8f * std::abs(mx), mx), Holding);
                if (my != 0)
                    Notify(MoveFB, std::copysign(0.2f + 0.8f * std::abs(my), -my), Holding);
            }
            else
            {
                const float x = mx / moveMagnitude, y = my / moveMagnitude;
                if (x < -0.38f)
                    Notify(MoveLeft, 0, Holding);
                else if (x > 0.38f)
                    Notify(MoveRight, 0, Holding);
                if (y > 0.38f)
                    Notify(MoveForward, 0, Holding);
                else if (y < -0.38f)
                    Notify(MoveBackward, 0, Holding);
            }
        }

        if (Pressed(XINPUT_GAMEPAD_LEFT_THUMB))
            SprintLatch = !SprintLatch;
        if (moveMagnitude < 0.3f)
            SprintLatch = false;
        if (SprintLatch)
            Notify(Sprint, 0, Holding);

        float lookMagnitude = 0;
        const auto [lx, ly] = Stick(Current.rx, Current.ry, 0.2f, lookMagnitude);
        if (lookMagnitude <= 0)
        {
            LookBoostTime = 0;
            return;
        }
        LookBoostTime = lookMagnitude > 0.95f ? LookBoostTime + dt : 0.0f;
        const float boost = 1.0f + 0.75f * std::clamp((LookBoostTime - 0.2f) / 0.35f, 0.0f, 1.0f);
        const float response = lookMagnitude * boost;
        const float speed = std::clamp(FixSettings().Float("MAIN.GamepadCameraSpeed"), 0.25f, 3.0f);
        const bool invert = FixSettings().Bool("MAIN.InvertGamepadY") != Field<bool>(Manager, managerInvertOffset);
        const float yaw = lx * response * 200.0f * speed * dt;
        const float pitch = -ly * response * 135.0f * speed * dt * (invert ? -1.0f : 1.0f);
        if (yaw != 0)
            Notify(TurnLR, yaw, Holding);
        if (pitch != 0)
            Notify(TurnUD, pitch, Holding);
    }

    // Menus: the sticks move the cursor, A clicks, B goes back, the d-pad sends arrow keys, the right stick scrolls
    void Menu(double now)
    {
        const float dt = float(std::clamp(now - CursorTime, 0.0, 0.1));
        CursorTime = now;
        MouseButton = Down(XINPUT_GAMEPAD_A);
        Keys[DIK_ESCAPE] = Down(XINPUT_GAMEPAD_B) || Down(XINPUT_GAMEPAD_START);
        Keys[DIK_UP] = Down(XINPUT_GAMEPAD_DPAD_UP);
        Keys[DIK_DOWN] = Down(XINPUT_GAMEPAD_DPAD_DOWN);
        Keys[DIK_LEFT] = Down(XINPUT_GAMEPAD_DPAD_LEFT);
        Keys[DIK_RIGHT] = Down(XINPUT_GAMEPAD_DPAD_RIGHT);

        float magnitude = 0;
        const auto [x, y] = Stick(Current.lx, Current.ly, 0.2f, magnitude);
        if (magnitude > 0 && Input)
        {
            const float speed = 650.0f * magnitude * dt;
            if (auto mouse = Virtual<void*>(Input, inputMouseSlot))
            {
                const float cursorX = Virtual<float>(mouse, mouseGetXSlot) - 0.5f + x / magnitude * speed;
                const float cursorY = Virtual<float>(mouse, mouseGetYSlot) - 0.5f - y / magnitude * speed;
                Virtual<void>(mouse, mouseSetXSlot, std::clamp(cursorX, 0.0f, 799.0f));
                Virtual<void>(mouse, mouseSetYSlot, std::clamp(cursorY, 0.0f, 599.0f));
            }
        }

        float scroll = 0;
        const auto [sx, sy] = Stick(Current.rx, Current.ry, 0.5f, scroll);
        if (scroll > 0 && std::abs(sy) > std::abs(sx) && now >= WheelTime)
        {
            MouseWheel += sy > 0 ? WHEEL_DELTA : -WHEEL_DELTA;
            WheelTime = now + 0.15;
        }
    }

    void ClearInjected()
    {
        MouseButton = false;
        Keys.fill(false);
    }

    void UpdateRumble(double now)
    {
        float low = 0, high = 0;
        if (Current.connected && Gameplay && FixSettings().Bool("MAIN.Vibration"))
        {
            for (const auto& rumble : Rumbles)
            {
                if (now < rumble.until)
                {
                    low = std::max(low, rumble.low);
                    high = std::max(high, rumble.high);
                }
            }
        }
        const auto motorLow = WORD(std::clamp(low, 0.0f, 1.0f) * 65535.0f);
        const auto motorHigh = WORD(std::clamp(high, 0.0f, 1.0f) * 65535.0f);
        if (motorLow == MotorLow && motorHigh == MotorHigh)
            return;
        auto setState = PadInput::XInput().setState;
        if (!setState)
            return;
        XINPUT_VIBRATION vibration{ motorLow, motorHigh };
        setState(Current.connected ? Current.slot : Previous.slot, &vibration);
        MotorLow = motorLow;
        MotorHigh = motorHigh;
    }

    void AddRumble(float low, float high, double duration)
    {
        const double now = Now();
        auto slot = std::min_element(Rumbles.begin(), Rumbles.end(), [](const Rumble& a, const Rumble& b) { return a.until < b.until; });
        *slot = { low, high, now + duration };
    }

    void Frame()
    {
        ++InputFrame;
        Poll(Focused() && !(InputBlocked && InputBlocked()));
        const bool gameplay = ManagerEnabled && InputFrame - ManagerFrame <= 2;
        SwitchMode(gameplay);
        // A blocked button stays blocked through the frame it is released in
        BlockedButtons &= Previous.buttons | Current.buttons;
        const double now = Now();
        ClearInjected();
        Keys[DIK_ESCAPE] = Down(XINPUT_GAMEPAD_START);
        if (!gameplay)
            Menu(now);
        else
            CursorTime = now;
        UpdateRumble(now);
    }

    void ManagerUpdated()
    {
        ManagerFrame = InputFrame;
        ManagerEnabled = Field<bool>(Manager, managerEnabledOffset);
        const double now = Now();
        const float dt = float(std::clamp(now - ManagerTime, 0.0, 0.1));
        ManagerTime = now;
        if (!Gameplay || !Current.connected || !ManagerEnabled)
            return;
        Actions(now, dt);
    }

    void MapChanged(const char* name)
    {
        static constexpr std::pair<const char*, uint8_t> names[] =
        {
            { "default", Default }, { "zoom", Zoom }, { "binozoom", Binozoom }, { "vehicle", Vehicle }, { "player_dead", Dead },
        };
        CurrentMap = 0;
        for (const auto& [mapName, map] : names)
        {
            if (name && !_stricmp(name, mapName))
                CurrentMap = map;
        }
    }

    void Shaken(void* script, bool weapon)
    {
        auto player = Field<void*>(script, scriptPlayerOffset);
        if (!player || !Field<bool>(player, playerLocalOffset))
            return;
        if (weapon)
        {
            const float amplitude = std::abs(Field<float>(player, shakeAmplitudeOffset));
            AddRumble(0.12f, std::clamp(0.3f + amplitude * 12.0f, 0.3f, 0.8f), 0.09);
        }
        else
        {
            const float degree = std::min(std::abs(Field<float>(player, shakeDegreeOffset)) / 13.0f, 1.0f);
            AddRumble(0.25f + 0.75f * degree, 0.2f + 0.4f * degree, 0.22 + 0.15 * degree);
        }
    }

    void HookManager(void* manager);

#ifdef _WIN64
    void InputUpdate(void* input, bool focus)
    {
        Frame();
        UpdateHook.call<void>(input, focus);
    }

    void* CreateManager(void* input)
    {
        auto manager = ManagerHook.call<void*>(input);
        HookManager(manager);
        return manager;
    }

    void ManagerUpdate(void* manager, unsigned time)
    {
        ManagerUpdateHook.call<void>(manager, time);
        if (manager == Manager)
            ManagerUpdated();
    }

    void SetMap(void* manager, const char* name)
    {
        SetMapHook.call<void>(manager, name);
        if (manager == Manager)
            MapChanged(name);
    }

    intptr_t Shake(void* script, void* handler)
    {
        const auto result = ShakeHook.call<intptr_t>(script, handler);
        Shaken(script, false);
        return result;
    }

    intptr_t ShakeL(void* script, void* handler)
    {
        const auto result = ShakeLHook.call<intptr_t>(script, handler);
        Shaken(script, true);
        return result;
    }
#else
    void __fastcall InputUpdate(void* input, void*, bool focus)
    {
        Frame();
        UpdateHook.thiscall<void>(input, focus);
    }

    void* __fastcall CreateManager(void* input, void*)
    {
        auto manager = ManagerHook.thiscall<void*>(input);
        HookManager(manager);
        return manager;
    }

    void __fastcall ManagerUpdate(void* manager, void*, unsigned time)
    {
        ManagerUpdateHook.thiscall<void>(manager, time);
        if (manager == Manager)
            ManagerUpdated();
    }

    void __fastcall SetMap(void* manager, void*, const char* name)
    {
        SetMapHook.thiscall<void>(manager, name);
        if (manager == Manager)
            MapChanged(name);
    }

    intptr_t __fastcall Shake(void* script, void*, void* handler)
    {
        const auto result = ShakeHook.thiscall<intptr_t>(script, handler);
        Shaken(script, false);
        return result;
    }

    intptr_t __fastcall ShakeL(void* script, void*, void* handler)
    {
        const auto result = ShakeLHook.thiscall<intptr_t>(script, handler);
        Shaken(script, true);
        return result;
    }
#endif

    void HookManager(void* manager)
    {
        if (!manager)
            return;
        Manager = manager;
        if (!ManagerUpdateHook)
        {
            ManagerUpdateHook = safetyhook::create_inline(VirtualTarget(manager, managerUpdateSlot), &ManagerUpdate);
            SetMapHook = safetyhook::create_inline(VirtualTarget(manager, managerSetMapSlot), &SetMap);
        }
    }

    void* __cdecl CreateInput(void* system, void* instance, void* window, bool directInput)
    {
        auto input = CreateInputHook.ccall<void*>(system, instance, window, directInput);
        if (input && !UpdateHook)
        {
            Input = input;
            UpdateHook = safetyhook::create_inline(VirtualTarget(input, inputUpdateSlot), &InputUpdate);
            ManagerHook = safetyhook::create_inline(VirtualTarget(input, inputManagerSlot), &CreateManager);
        }
        return input;
    }
}

export namespace FarCryPad
{
    // Input the developer overlay takes for itself
    inline void SetBlocked(bool (*blocked)())
    {
        FarCryPadState::InputBlocked = blocked;
    }

    inline void InitInput()
    {
        if (FarCryPadState::CreateInputHook)
            return;
        auto module = GetModuleHandleW(L"CryInput.dll");
        FarCryPadState::CreateInputHook = safetyhook::create_inline(GetProcAddress(module, "CreateInput"), &FarCryPadState::CreateInput);
    }

    inline void InitGame()
    {
        if (FarCryPadState::ShakeHook)
            return;
        auto module = GetModuleHandleW(L"CryGame.dll");
        // The 32-bit game has analog move actions; the 64-bit build only has the four move directions
#ifdef _WIN64
        FarCryPadState::AnalogMove = false;
        auto shake = hook::module_pattern(module, "48 83 EC 58 48 C7 44 24 30 FE FF FF FF 48 89 5C 24 68 48 89 74 24 70 48 89 7C 24 78 48 8B FA 48 8B F1 48 8B 02 48 8B CA FF 50 18 83 F8 04 74 3A");
        auto shakeL = hook::module_pattern(module, "48 8B C4 48 83 EC 68 48 C7 44 24 30 FE FF FF FF 48 89 58 10 48 89 70 18 48 89 78 20 48 8B FA 48 8B F1 48 8B 02 48 8B CA FF 50 18 83 F8 03 74 3A");
#else
        FarCryPadState::AnalogMove = true;
        auto shake = hook::module_pattern(module, "B8 ? ? ? ? E8 ? ? ? ? 83 EC 18 53 56 8B 75 08 8B 06 8B D9 57 8B CE FF 50 0C 83 F8 04 74 2F");
        auto shakeL = hook::module_pattern(module, "B8 ? ? ? ? E8 ? ? ? ? 83 EC 24 53 56 8B 75 08 8B 06 57 8B F9 8B CE FF 50 0C 83 F8 03 74 2F");
#endif
        FarCryPadState::ShakeHook = safetyhook::create_inline(shake.get_first(), &FarCryPadState::Shake);
        FarCryPadState::ShakeLHook = safetyhook::create_inline(shakeL.get_first(), &FarCryPadState::ShakeL);
    }

    // DirectInput reports game controllers to the engine's own joystick code; the pad is read through XInput instead
    inline bool HideDevices(DWORD type)
    {
        const auto device = type & 0xFF;
        return device == DI8DEVCLASS_GAMECTRL || (device >= DI8DEVTYPE_JOYSTICK && device <= DI8DEVTYPE_SUPPLEMENTAL);
    }

    inline void MouseState(void* data, DWORD size)
    {
        if (!data || (size != sizeof(DIMOUSESTATE) && size != sizeof(DIMOUSESTATE2)))
            return;
        auto state = static_cast<DIMOUSESTATE*>(data);
        if (FarCryPadState::MouseButton)
            state->rgbButtons[0] |= 0x80;
        state->lZ += FarCryPadState::MouseWheel;
        FarCryPadState::MouseWheel = 0;
        FarCryPadState::MouseReported = FarCryPadState::MouseButton;
    }

    inline void KeyboardState(void* data, DWORD size)
    {
        if (!data || size != 256)
            return;
        auto keys = static_cast<uint8_t*>(data);
        for (size_t i = 0; i < FarCryPadState::Keys.size(); ++i)
        {
            if (FarCryPadState::Keys[i])
                keys[i] |= 0x80;
        }
        FarCryPadState::KeysReported = FarCryPadState::Keys;
    }

    inline void Append(void* data, DWORD stride, DWORD* count, DWORD capacity, DWORD offset, DWORD value)
    {
        if (!data || !count || *count >= capacity || stride < sizeof(DIDEVICEOBJECTDATA))
            return;
        auto entry = reinterpret_cast<DIDEVICEOBJECTDATA*>(static_cast<uint8_t*>(data) + size_t(*count) * stride);
        std::memset(entry, 0, stride);
        entry->dwOfs = offset;
        entry->dwData = value;
        entry->dwTimeStamp = GetTickCount();
        ++*count;
    }

    inline void MouseData(void* data, DWORD stride, DWORD* count, DWORD capacity, bool peek)
    {
        if (peek)
            return;
        if (FarCryPadState::MouseButton != FarCryPadState::MouseReported)
        {
            Append(data, stride, count, capacity, DIMOFS_BUTTON0, FarCryPadState::MouseButton ? 0x80 : 0);
            FarCryPadState::MouseReported = FarCryPadState::MouseButton;
        }
        if (FarCryPadState::MouseWheel)
        {
            Append(data, stride, count, capacity, DIMOFS_Z, DWORD(FarCryPadState::MouseWheel));
            FarCryPadState::MouseWheel = 0;
        }
    }

    inline void KeyboardData(void* data, DWORD stride, DWORD* count, DWORD capacity, bool peek)
    {
        if (peek)
            return;
        for (size_t i = 0; i < FarCryPadState::Keys.size(); ++i)
        {
            if (FarCryPadState::Keys[i] != FarCryPadState::KeysReported[i])
            {
                Append(data, stride, count, capacity, DWORD(i), FarCryPadState::Keys[i] ? 0x80 : 0);
                FarCryPadState::KeysReported[i] = FarCryPadState::Keys[i];
            }
        }
    }
}
