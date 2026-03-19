module;

#include <stdafx.h>
#define DIRECTINPUT_VERSION 0x0700
#include <dinput.h>

export module input;

import ComVars;
import xidi;

#pragma comment(lib, "dinput.lib")
#pragma comment(lib, "dxguid.lib")

// DirectInput 7 interface pointers
static IDirectInput* g_pDI = nullptr;
static IDirectInputDevice* g_pJoystick = nullptr;
static DIJOYSTATE g_LastJoyState = {};
static DIJOYSTATE g_PrevJoyState = {};

// Callback for device enumeration
BOOL CALLBACK EnumJoysticksCallback(const DIDEVICEINSTANCE* pdidInstance, LPVOID pContext)
{
    auto pDI = static_cast<IDirectInput*>(pContext);

    // Create device
    HRESULT hr = pDI->CreateDevice(pdidInstance->guidInstance, &g_pJoystick, nullptr);
    if (FAILED(hr))
        return DIENUM_CONTINUE;

    // Set data format for joystick
    hr = g_pJoystick->SetDataFormat(&c_dfDIJoystick);
    if (FAILED(hr))
    {
        g_pJoystick->Release();
        g_pJoystick = nullptr;
        return DIENUM_CONTINUE;
    }

    // Stop enumeration - we found a joystick
    return DIENUM_STOP;
}

void CreateInput()
{
    HRESULT hr;

    // Create DirectInput interface (DirectInput 7)
    hr = DirectInputCreate(GetModuleHandleA(nullptr), DIRECTINPUT_VERSION, &g_pDI, nullptr);
    if (FAILED(hr))
        return;

    // Enumerate joysticks
    hr = g_pDI->EnumDevices(DIDEVTYPE_JOYSTICK, EnumJoysticksCallback, g_pDI, DIEDFL_ATTACHEDONLY);
    if (FAILED(hr))
    {
        if (g_pDI)
        {
            g_pDI->Release();
            g_pDI = nullptr;
        }
        return;
    }

    // Set cooperative level for the device
    if (g_pJoystick)
    {
        hr = g_pJoystick->SetCooperativeLevel(nullptr, DISCL_BACKGROUND | DISCL_NONEXCLUSIVE);
        if (FAILED(hr))
        {
            g_pJoystick->Release();
            g_pJoystick = nullptr;
            return;
        }

        // Acquire the device
        hr = g_pJoystick->Acquire();
        if (FAILED(hr))
        {
            g_pJoystick->Release();
            g_pJoystick = nullptr;
            return;
        }
    }
}

void UpdateInput()
{
    if (!g_pJoystick)
        return;

    // Save previous state
    g_PrevJoyState = g_LastJoyState;

    // Get device state directly (no Poll() in DI7)
    HRESULT hr = g_pJoystick->GetDeviceState(sizeof(DIJOYSTATE), &g_LastJoyState);

    if (FAILED(hr))
    {
        // Try to reacquire on error
        hr = g_pJoystick->Acquire();
        if (FAILED(hr))
            return;

        // Try getting state again
        hr = g_pJoystick->GetDeviceState(sizeof(DIJOYSTATE), &g_LastJoyState);
        if (FAILED(hr))
            return;
    }
}

void CleanupInput()
{
    if (g_pJoystick)
    {
        g_pJoystick->Unacquire();
        g_pJoystick->Release();
        g_pJoystick = nullptr;
    }

    if (g_pDI)
    {
        g_pDI->Release();
        g_pDI = nullptr;
    }
}

export DIJOYSTATE* GetJoyState()
{
    return &g_LastJoyState;
}

export DIJOYSTATE* GetPrevJoyState()
{
    return &g_PrevJoyState;
}

// Helper to check if a button is currently pressed
inline bool IsButtonPressed(uint8_t buttonState)
{
    return (buttonState & 0x80) != 0;
}

// Helper to check if a button was just pressed (pressed now but wasn't before)
inline bool IsButtonJustPressed(uint8_t currState, uint8_t prevState)
{
    return IsButtonPressed(currState) && !IsButtonPressed(prevState);
}

// Helper to check if a button was just released (not pressed now but was before)
inline bool IsButtonJustReleased(uint8_t currState, uint8_t prevState)
{
    return !IsButtonPressed(currState) && IsButtonPressed(prevState);
}

// Helper to convert joystick range [0-65535] to normalized [-1, 1]
// then to mouse-like movement delta
float ConvertJoystickToMouseDelta(LONG joystickValue, float sensitivity)
{
    // DirectInput joystick range: 0-65535, center at 32767
    float normalized = ((float)joystickValue - 32768.0f) / 32767.0f;

    // Apply deadzone (typically 0.15 of the range)
    const float deadzone = 0.15f;
    if (fabsf(normalized) < deadzone)
        return 0.0f;

    // Scale past deadzone
    if (normalized > 0.0f)
        normalized = (normalized - deadzone) / (1.0f - deadzone);
    else
        normalized = (normalized + deadzone) / (1.0f - deadzone);

    // Convert to mouse-like delta with sensitivity
    // Typical mouse movement units: scale by sensitivity factor
    return normalized * sensitivity;
}

namespace MaxPayne_InputControl
{
    enum JoystickButton
    {
        ButtonA = 0,
        ButtonB = 1,
        ButtonX = 2,
        ButtonY = 3,
        ButtonLB = 4,
        ButtonRB = 5,
        TriggerLT = 6,
        TriggerRT = 7,
        ButtonBack = 8,
        ButtonStart = 9,
        ButtonLS = 10,
        ButtonRS = 11,
        DpadUp = 12,
        DpadDown = 13,
        DpadLeft = 14,
        DpadRight = 15,

        Undefined = -1,
    };

    // Control scheme mapping
    struct ControlScheme
    {
        JoystickButton Shoot = TriggerRT;
        JoystickButton Reload = ButtonX;
        JoystickButton Jump = ButtonA;
        JoystickButton Crouch = ButtonLS;
        JoystickButton DodgeLeft = Undefined;
        JoystickButton DodgeRight = Undefined;
        JoystickButton DodgeForward = Undefined;
        JoystickButton DodgeBackward = Undefined;
        JoystickButton DodgeModifier = Undefined;
        JoystickButton Use = ButtonY;
        JoystickButton Slot1 = Undefined;
        JoystickButton Slot2 = Undefined;
        JoystickButton Slot3 = Undefined;
        JoystickButton Slot4 = Undefined;
        JoystickButton Slot5 = Undefined;
        JoystickButton Slot6 = Undefined;
        JoystickButton BestWeapon = Undefined;
        JoystickButton NextPreviousWeapon = Undefined;
        JoystickButton NextWeapon = DpadRight;
        JoystickButton PreviousWeapon = DpadLeft;
        JoystickButton Pause = ButtonBack;
        JoystickButton Painkiller = ButtonB;
        JoystickButton SniperZoom = ButtonRS;
        JoystickButton SlowMotion = ButtonLB;
        JoystickButton BulletTime = TriggerLT;
    };

    static ControlScheme g_ControlScheme;

    // Helper to get button state based on input control
    inline JoystickButton GetButtonForInput(void* inputControl)
    {
        if (inputControl == MaxPayne_ConfiguredInput::Shoot)
            return g_ControlScheme.Shoot;
        else if (inputControl == MaxPayne_ConfiguredInput::Reload)
            return g_ControlScheme.Reload;
        else if (inputControl == MaxPayne_ConfiguredInput::Jump)
            return g_ControlScheme.Jump;
        else if (inputControl == MaxPayne_ConfiguredInput::Crouch)
            return g_ControlScheme.Crouch;
        else if (inputControl == MaxPayne_ConfiguredInput::DodgeLeft)
            return g_ControlScheme.DodgeLeft;
        else if (inputControl == MaxPayne_ConfiguredInput::DodgeRight)
            return g_ControlScheme.DodgeRight;
        else if (inputControl == MaxPayne_ConfiguredInput::DodgeForward)
            return g_ControlScheme.DodgeForward;
        else if (inputControl == MaxPayne_ConfiguredInput::DodgeBackward)
            return g_ControlScheme.DodgeBackward;
        else if (inputControl == MaxPayne_ConfiguredInput::DodgeModifier)
            return g_ControlScheme.DodgeModifier;
        else if (inputControl == MaxPayne_ConfiguredInput::Use)
            return g_ControlScheme.Use;
        else if (inputControl == MaxPayne_ConfiguredInput::Slot1)
            return g_ControlScheme.Slot1;
        else if (inputControl == MaxPayne_ConfiguredInput::Slot2)
            return g_ControlScheme.Slot2;
        else if (inputControl == MaxPayne_ConfiguredInput::Slot3)
            return g_ControlScheme.Slot3;
        else if (inputControl == MaxPayne_ConfiguredInput::Slot4)
            return g_ControlScheme.Slot4;
        else if (inputControl == MaxPayne_ConfiguredInput::Slot5)
            return g_ControlScheme.Slot5;
        else if (inputControl == MaxPayne_ConfiguredInput::Slot6)
            return g_ControlScheme.Slot6;
        else if (inputControl == MaxPayne_ConfiguredInput::BestWeapon)
            return g_ControlScheme.BestWeapon;
        else if (inputControl == MaxPayne_ConfiguredInput::NextPreviousWeapon)
            return g_ControlScheme.NextPreviousWeapon;
        else if (inputControl == MaxPayne_ConfiguredInput::Pause)
            return g_ControlScheme.Pause;
        else if (inputControl == MaxPayne_ConfiguredInput::Painkiller)
            return g_ControlScheme.Painkiller;
        else if (inputControl == MaxPayne_ConfiguredInput::SniperZoom)
            return g_ControlScheme.SniperZoom;
        else if (inputControl == MaxPayne_ConfiguredInput::SlowMotion)
            return g_ControlScheme.SlowMotion;
        else if (inputControl == MaxPayne_ConfiguredInput::BulletTime)
            return g_ControlScheme.BulletTime;

        return JoystickButton::Undefined;
    }

    SafetyHookInline shgetFloat = {};
    float __fastcall getFloat(void* _this, void* edx)
    {
        auto ret = shgetFloat.unsafe_fastcall<float>(_this, edx);

        if (ret == 0.0f && g_pJoystick)
        {
            DIJOYSTATE* joyState = GetJoyState();
            if (joyState)
            {
                const float stickSensitivity = 1.0f;

                uint32_t inputType = *(uint32_t*)((uintptr_t)_this + 0x24);
                switch (inputType)
                {
                    case 0: // Movement axes
                    {
                        if (_this == MaxPayne_ConfiguredInput::LeftRight)
                        {
                            float stickX = ConvertJoystickToMouseDelta(joyState->lX, stickSensitivity);
                            ret = (stickX > 0.0f) ? -1.0f : (stickX < 0.0f) ? 1.0f : 0.0f;
                        }
                        else if (_this == MaxPayne_ConfiguredInput::ForwardBackward)
                        {
                            float stickY = ConvertJoystickToMouseDelta(joyState->lY, stickSensitivity);
                            ret = (stickY > 0.0f) ? -1.0f : (stickY < 0.0f) ? 1.0f : 0.0f;
                        }
                        break;
                    }
                    case 1: // Right stick X
                    {
                        float stickX = ConvertJoystickToMouseDelta(joyState->lRx, stickSensitivity);
                        ret = stickX;
                        break;
                    }
                    case 2: // Right stick Y
                    {
                        float stickY = ConvertJoystickToMouseDelta(joyState->lRy, stickSensitivity);

                        if (_this == MaxPayne_ConfiguredInput::AimUpDown)
                        {
                            auto Mouse = (uintptr_t)X_Input::getMouse();
                            bool bInvertY = (*(uint8_t*)(Mouse + 81)) != 0;
                            if (bInvertY)
                                stickY = -stickY;
                        }

                        ret = stickY;
                        break;
                    }
                    case 3: // D-pad weapon scrolling
                    {
                        bool bLeftPressed = IsButtonPressed(joyState->rgbButtons[g_ControlScheme.PreviousWeapon]) &&
                            !IsButtonPressed(GetPrevJoyState()->rgbButtons[g_ControlScheme.PreviousWeapon]);
                        bool bRightPressed = IsButtonPressed(joyState->rgbButtons[g_ControlScheme.NextWeapon]) &&
                            !IsButtonPressed(GetPrevJoyState()->rgbButtons[g_ControlScheme.NextWeapon]);

                        if (bLeftPressed)
                        {
                            ret = 1.0f;  // Previous weapon
                        }
                        else if (bRightPressed)
                        {
                            ret = -1.0f;  // Next weapon
                        }
                        else
                        {
                            ret = 0.0f;
                        }
                        break;
                    }
                    default:
                        break;
                }
            }
        }

        return ret;
    }

    SafetyHookInline shgetFloatWithSingleClicks = {};
    float __fastcall getFloatWithSingleClicks(void* _this, void* edx)
    {
        auto ret = shgetFloatWithSingleClicks.unsafe_fastcall<float>(_this, edx);

        if (ret == 0.0f && g_pJoystick)
        {
            DIJOYSTATE* joyState = GetJoyState();
            DIJOYSTATE* prevJoyState = GetPrevJoyState();
            if (joyState && prevJoyState)
            {
                uint32_t inputType = *(uint32_t*)((uintptr_t)_this + 0x24);
                switch (inputType)
                {
                    case 3: // D-pad weapon scrolling with single-click
                    {
                        bool bLeftPressed = IsButtonJustPressed(joyState->rgbButtons[g_ControlScheme.PreviousWeapon],
                                                                     prevJoyState->rgbButtons[g_ControlScheme.PreviousWeapon]);
                        bool bRightPressed = IsButtonJustPressed(joyState->rgbButtons[g_ControlScheme.NextWeapon],
                                                                      prevJoyState->rgbButtons[g_ControlScheme.NextWeapon]);

                        if (bLeftPressed)
                        {
                            ret = 1.0f;  // Previous weapon
                        }
                        else if (bRightPressed)
                        {
                            ret = -1.0f;  // Next weapon
                        }
                        else
                        {
                            ret = 0.0f;
                        }
                        break;
                    }
                    default:
                        break;
                }
            }
        }

        return ret;
    }

    SafetyHookInline shgetButton = {};
    char __fastcall getButton(void* _this, void* edx, int a2)
    {
        auto ret = shgetButton.unsafe_fastcall<char>(_this, edx, a2);

        if (ret == 0 && g_pJoystick)
        {
            DIJOYSTATE* joyState = GetJoyState();
            if (joyState)
            {
                JoystickButton button = GetButtonForInput(_this);
                if (button >= 0 && button < 16)
                {
                    ret = IsButtonPressed(joyState->rgbButtons[button]) ? 1 : 0;
                }
            }
        }

        return ret;
    }

    SafetyHookInline shgetButtonSingleClicked = {};
    char __fastcall getButtonSingleClicked(void* _this, void* edx, int a2)
    {
        auto ret = shgetButtonSingleClicked.unsafe_fastcall<char>(_this, edx, a2);

        if (ret == 0 && g_pJoystick)
        {
            DIJOYSTATE* joyState = GetJoyState();
            DIJOYSTATE* prevJoyState = GetPrevJoyState();

            if (joyState && prevJoyState)
            {
                JoystickButton button = GetButtonForInput(_this);
                if (button >= 0 && button < 16)
                {
                    ret = IsButtonJustPressed(joyState->rgbButtons[button], prevJoyState->rgbButtons[button]) ? 1 : 0;
                }
            }
        }

        return ret;
    }

    SafetyHookInline shgetButtonReleased = {};
    char __fastcall getButtonReleased(void* _this, void* edx, int a2)
    {
        auto ret = shgetButtonReleased.unsafe_fastcall<char>(_this, edx, a2);

        if (ret == 0 && g_pJoystick)
        {
            DIJOYSTATE* joyState = GetJoyState();
            DIJOYSTATE* prevJoyState = GetPrevJoyState();

            if (joyState && prevJoyState)
            {
                JoystickButton button = GetButtonForInput(_this);
                if (button >= 0 && button < 16)
                {
                    ret = IsButtonJustReleased(joyState->rgbButtons[button], prevJoyState->rgbButtons[button]) ? 1 : 0;
                }
            }
        }

        return ret;
    }
}

bool g_VibrationThisFrame = false;
bool g_VibrationActive = false;
std::chrono::steady_clock::time_point g_VibrationStart = {};
constexpr std::chrono::milliseconds VIBRATION_DURATION{ 300 };

void Vibrate(int strength)
{
    if (!XidiSendVibration)
        return;

    if (g_VibrationThisFrame)
        return;

    if (g_VibrationActive)
        return;

    if (strength < 0)
        return;

    g_VibrationThisFrame = true;
    g_VibrationActive = true;
    g_VibrationStart = std::chrono::steady_clock::now();

    constexpr int RUMBLE_MAX_STRENGTH = 100;
    constexpr WORD RUMBLE_MAX_VALUE = 0xFFFF;

    const WORD motor = static_cast<WORD>(
        static_cast<float>(std::clamp(strength, 0, RUMBLE_MAX_STRENGTH))
        / RUMBLE_MAX_STRENGTH * RUMBLE_MAX_VALUE);

    XidiSendVibration(-1, motor, motor);
}

void UpdateVibration()
{
    g_VibrationThisFrame = false;

    if (g_VibrationActive)
    {
        auto elapsed = std::chrono::steady_clock::now() - g_VibrationStart;
        if (elapsed >= VIBRATION_DURATION)
        {
            g_VibrationActive = false;
            if (XidiSendVibration)
                XidiSendVibration(-1, 0, 0);
        }
    }
}

export void InitInput()
{
    auto pattern = hook::pattern("E8 ? ? ? ? 8B C8 E8 ? ? ? ? D9 83");
    X_Input::getMouse = (void* (*)())injector::GetBranchDestination(pattern.get_first()).as_int();

    pattern = hook::pattern("BE ? ? ? ? 6A 00 8B CE");
    uintptr_t INPUT_BASE = *pattern.get_first<uintptr_t>(1);
    constexpr uintptr_t INPUT_STRIDE = 0x29; // Size of each input control structure

    MaxPayne_ConfiguredInput::ForwardBackward = (void*)(INPUT_BASE + INPUT_STRIDE * 0);
    MaxPayne_ConfiguredInput::LeftRight = (void*)(INPUT_BASE + INPUT_STRIDE * 1);
    MaxPayne_ConfiguredInput::Shoot = (void*)(INPUT_BASE + INPUT_STRIDE * 2);
    MaxPayne_ConfiguredInput::Reload = (void*)(INPUT_BASE + INPUT_STRIDE * 3);
    MaxPayne_ConfiguredInput::Jump = (void*)(INPUT_BASE + INPUT_STRIDE * 4);
    MaxPayne_ConfiguredInput::Crouch = (void*)(INPUT_BASE + INPUT_STRIDE * 5);
    MaxPayne_ConfiguredInput::DodgeLeft = (void*)(INPUT_BASE + INPUT_STRIDE * 6);
    MaxPayne_ConfiguredInput::DodgeRight = (void*)(INPUT_BASE + INPUT_STRIDE * 7);
    MaxPayne_ConfiguredInput::DodgeForward = (void*)(INPUT_BASE + INPUT_STRIDE * 8);
    MaxPayne_ConfiguredInput::DodgeBackward = (void*)(INPUT_BASE + INPUT_STRIDE * 9);
    MaxPayne_ConfiguredInput::DodgeModifier = (void*)(INPUT_BASE + INPUT_STRIDE * 10);
    MaxPayne_ConfiguredInput::AimUpDown = (void*)(INPUT_BASE + INPUT_STRIDE * 11);
    MaxPayne_ConfiguredInput::AimLeftRight = (void*)(INPUT_BASE + INPUT_STRIDE * 12);
    MaxPayne_ConfiguredInput::Use = (void*)(INPUT_BASE + INPUT_STRIDE * 13);
    MaxPayne_ConfiguredInput::Slot1 = (void*)(INPUT_BASE + INPUT_STRIDE * 14);
    MaxPayne_ConfiguredInput::Slot2 = (void*)(INPUT_BASE + INPUT_STRIDE * 15);
    MaxPayne_ConfiguredInput::Slot3 = (void*)(INPUT_BASE + INPUT_STRIDE * 16);
    MaxPayne_ConfiguredInput::Slot4 = (void*)(INPUT_BASE + INPUT_STRIDE * 17);
    MaxPayne_ConfiguredInput::Slot5 = (void*)(INPUT_BASE + INPUT_STRIDE * 18);
    MaxPayne_ConfiguredInput::Slot6 = (void*)(INPUT_BASE + INPUT_STRIDE * 19);
    MaxPayne_ConfiguredInput::BestWeapon = (void*)(INPUT_BASE + INPUT_STRIDE * 25);
    MaxPayne_ConfiguredInput::NextPreviousWeapon = (void*)(INPUT_BASE + INPUT_STRIDE * 26);
    MaxPayne_ConfiguredInput::Pause = (void*)(INPUT_BASE + INPUT_STRIDE * 27);
    MaxPayne_ConfiguredInput::Painkiller = (void*)(INPUT_BASE + INPUT_STRIDE * 28);
    MaxPayne_ConfiguredInput::SniperZoom = (void*)(INPUT_BASE + INPUT_STRIDE * 29);
    MaxPayne_ConfiguredInput::SlowMotion = (void*)(INPUT_BASE + INPUT_STRIDE * 30);
    MaxPayne_ConfiguredInput::BulletTime = (void*)(INPUT_BASE + INPUT_STRIDE * 31);

    pattern = hook::pattern("89 1D ? ? ? ? 8B 4D");
    static auto X_InputconstructHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        CreateInput();
    });

    pattern = hook::pattern("C7 05 ? ? ? ? ? ? ? ? 8B CF");
    static auto X_InputdestructHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        CleanupInput();
    });

    pattern = hook::pattern("8B 0D ? ? ? ? 85 C9 74 ? 8B 01 56 FF 50 ? 5E");
    static auto X_InputupdateHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        UpdateInput();
    });

    pattern = hook::pattern("56 8B F1 6A 00 E8 ? ? ? ? 84 C0 74 ? D9 05 ? ? ? ? 5E C3");
    MaxPayne_InputControl::shgetFloat = safetyhook::create_inline(pattern.count(2).get(0).get<void*>(0), MaxPayne_InputControl::getFloat);
    MaxPayne_InputControl::shgetFloatWithSingleClicks = safetyhook::create_inline(pattern.count(2).get(1).get<void*>(0), MaxPayne_InputControl::getFloatWithSingleClicks);

    pattern = hook::pattern("56 8B F1 57 8B 7C 24 ? 8D 04 FE 8B CF 03 C8 E8 ? ? ? ? 84 C0");
    MaxPayne_InputControl::shgetButton = safetyhook::create_inline(pattern.count(3).get(0).get<void*>(0), MaxPayne_InputControl::getButton);
    MaxPayne_InputControl::shgetButtonSingleClicked = safetyhook::create_inline(pattern.count(3).get(1).get<void*>(0), MaxPayne_InputControl::getButtonSingleClicked);
    MaxPayne_InputControl::shgetButtonReleased = safetyhook::create_inline(pattern.count(3).get(2).get<void*>(0), MaxPayne_InputControl::getButtonReleased);

    pattern = hook::pattern("FF 15 ? ? ? ? 33 DB 8D 4C 24");
    static auto ApplicationTickHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        UpdateVibration();
    });

    // Sniper scope enter
    pattern = hook::pattern("C7 83 ? ? ? ? ? ? ? ? E9 ? ? ? ? 84 C0");
    static auto MaxPayne_GameModeupdateHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        Vibrate(14);
    });

    // Painkiller use
    pattern = hook::pattern("E8 ? ? ? ? 8B CB E8 ? ? ? ? ? ? 8B C8 FF 52");
    static auto X_GlobalPainkillerSettingssendMessagesHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        Vibrate(23);
    });

    // Damage taken
    pattern = hook::pattern("E8 ? ? ? ? E9 ? ? ? ? ? ? ? ? ? ? ? ? ? ? DF E0");
    static auto X_CharactersetHealthHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        Vibrate(46);
    });

    // Shooting
    pattern = hook::pattern("E8 ? ? ? ? 8B 44 24 ? 50 8B CE E8 ? ? ? ? 5E");
    static auto X_CharacterPropertiessetIsShooting1 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        Vibrate(50);
    });

    pattern = hook::pattern("6A ? 8B CE E8 ? ? ? ? 8B C8 E8 ? ? ? ? 5E 5B C2");
    static auto X_CharacterPropertiessetIsShooting2 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        Vibrate(50);
    });

    pattern = hook::pattern("E8 ? ? ? ? 53 8B CE E8 ? ? ? ? 5E");
    static auto X_CharacterPropertiessetIsShooting3 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        Vibrate(50);
    });

    // Explosion
    static std::string CameraPathName;
    pattern = hook::pattern("E8 ? ? ? ? 8D 7B ? 68 ? ? ? ? 8B CF E8 ? ? ? ? 3B 47 ? 89 44 24 ? 74 ? 8D 50 ? B9 ? ? ? ? E8 ? ? ? ? 84 C0 75 ? 8D 44 24 ? EB ? 8D 44 24");
    static auto X_CameraImplementationreceiveAnimateInPlaceHook1 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        CameraPathName = std::string_view(*(char**)(regs.eax + 4));
    });

    pattern = hook::pattern("89 8E ? ? ? ? E8 ? ? ? ? C6 86");
    static auto X_CameraImplementationreceiveAnimateInPlaceHook2 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        if (CameraPathName == "explosion")
            Vibrate(100);
        CameraPathName.clear();
    });
}