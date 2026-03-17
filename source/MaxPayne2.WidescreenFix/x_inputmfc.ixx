module;

#include <stdafx.h>
#define DIRECTINPUT_VERSION 0x0700
#include <dinput.h>

export module x_inputmfc;

import ComVars;

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

namespace X_InputControlButton
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
        //JoystickButton Run = Undefined;
        //JoystickButton Walkback = Undefined;
        //JoystickButton StrafeLeft = Undefined;
        //JoystickButton StrafeRight = Undefined;
        JoystickButton Shoot = TriggerRT;
        JoystickButton Attack = ButtonRS;
        JoystickButton Reload = ButtonX;
        JoystickButton Jump = ButtonA;
        JoystickButton Crouch = ButtonLS;
        JoystickButton DodgeLeft = Undefined;
        JoystickButton DodgeRight = Undefined;
        JoystickButton DodgeForward = Undefined;
        JoystickButton DodgeBackward = Undefined;
        JoystickButton DodgeModifier = Undefined;
        JoystickButton Use = ButtonY;
        JoystickButton Slot0 = Undefined;
        JoystickButton Slot1 = Undefined;
        JoystickButton Slot2 = Undefined;
        JoystickButton Slot3 = Undefined;
        JoystickButton Slot4 = Undefined;
        JoystickButton Slot5 = Undefined;
        JoystickButton Slot6 = Undefined;
        JoystickButton Slot7 = Undefined;
        JoystickButton Slot8 = Undefined;
        JoystickButton Slot9 = Undefined;
        JoystickButton Slot10 = Undefined;
        JoystickButton BestWeapon = DpadUp;
        JoystickButton NextWeapon = DpadRight;
        JoystickButton PreviousWeapon = DpadLeft;
        JoystickButton Pause = ButtonBack;
        JoystickButton Painkiller = ButtonB;
        JoystickButton SniperZoom = Undefined;
        JoystickButton SlowMotion = ButtonLB;
        JoystickButton BulletTime = TriggerLT;
    };

    static ControlScheme g_ControlScheme;

    // Helper to get button state based on input control
    void* GetInputPtr(MaxPayne_ConfiguredInput::eControls i)
    {
        constexpr uintptr_t INPUT_STRIDE = 0x13; // Size of each input control structure
        return reinterpret_cast<void*>(*MaxPayne_ConfiguredInput::sm_control + INPUT_STRIDE * i);
    }

    inline JoystickButton GetButtonForInput(void* inputControl)
    {
        if (inputControl == GetInputPtr(MaxPayne_ConfiguredInput::SHOOT))
            return g_ControlScheme.Shoot;
        else if (inputControl == GetInputPtr(MaxPayne_ConfiguredInput::ATTACK))
            return g_ControlScheme.Attack;
        else if (inputControl == GetInputPtr(MaxPayne_ConfiguredInput::RELOAD))
            return g_ControlScheme.Reload;
        else if (inputControl == GetInputPtr(MaxPayne_ConfiguredInput::JUMP))
            return g_ControlScheme.Jump;
        else if (inputControl == GetInputPtr(MaxPayne_ConfiguredInput::CROUCH))
            return g_ControlScheme.Crouch;
        else if (inputControl == GetInputPtr(MaxPayne_ConfiguredInput::DODGELEFT))
            return g_ControlScheme.DodgeLeft;
        else if (inputControl == GetInputPtr(MaxPayne_ConfiguredInput::DODGERIGHT))
            return g_ControlScheme.DodgeRight;
        else if (inputControl == GetInputPtr(MaxPayne_ConfiguredInput::DODGEFORWARD))
            return g_ControlScheme.DodgeForward;
        else if (inputControl == GetInputPtr(MaxPayne_ConfiguredInput::DODGEBACKWARD))
            return g_ControlScheme.DodgeBackward;
        else if (inputControl == GetInputPtr(MaxPayne_ConfiguredInput::DODGEMODIFIER))
            return g_ControlScheme.DodgeModifier;
        else if (inputControl == GetInputPtr(MaxPayne_ConfiguredInput::USE))
            return g_ControlScheme.Use;
        else if (inputControl == GetInputPtr(MaxPayne_ConfiguredInput::SLOT0))
            return g_ControlScheme.Slot0;
        else if (inputControl == GetInputPtr(MaxPayne_ConfiguredInput::SLOT1))
            return g_ControlScheme.Slot1;
        else if (inputControl == GetInputPtr(MaxPayne_ConfiguredInput::SLOT2))
            return g_ControlScheme.Slot2;
        else if (inputControl == GetInputPtr(MaxPayne_ConfiguredInput::SLOT3))
            return g_ControlScheme.Slot3;
        else if (inputControl == GetInputPtr(MaxPayne_ConfiguredInput::SLOT4))
            return g_ControlScheme.Slot4;
        else if (inputControl == GetInputPtr(MaxPayne_ConfiguredInput::SLOT5))
            return g_ControlScheme.Slot5;
        else if (inputControl == GetInputPtr(MaxPayne_ConfiguredInput::SLOT6))
            return g_ControlScheme.Slot6;
        else if (inputControl == GetInputPtr(MaxPayne_ConfiguredInput::SLOT7))
            return g_ControlScheme.Slot7;
        else if (inputControl == GetInputPtr(MaxPayne_ConfiguredInput::SLOT8))
            return g_ControlScheme.Slot8;
        else if (inputControl == GetInputPtr(MaxPayne_ConfiguredInput::SLOT9))
            return g_ControlScheme.Slot9;
        else if (inputControl == GetInputPtr(MaxPayne_ConfiguredInput::SLOT10))
            return g_ControlScheme.Slot10;
        else if (inputControl == GetInputPtr(MaxPayne_ConfiguredInput::BESTWEAPON))
            return g_ControlScheme.BestWeapon;
        else if (inputControl == GetInputPtr(MaxPayne_ConfiguredInput::NEXTWEAPON))
            return g_ControlScheme.NextWeapon;
        else if (inputControl == GetInputPtr(MaxPayne_ConfiguredInput::PREVIOUSWEAPON))
            return g_ControlScheme.PreviousWeapon;
        else if (inputControl == GetInputPtr(MaxPayne_ConfiguredInput::PAUSE))
            return g_ControlScheme.Pause;
        else if (inputControl == GetInputPtr(MaxPayne_ConfiguredInput::PAINKILLER))
            return g_ControlScheme.Painkiller;
        else if (inputControl == GetInputPtr(MaxPayne_ConfiguredInput::SNIPERZOOM))
            return g_ControlScheme.SniperZoom;
        else if (inputControl == GetInputPtr(MaxPayne_ConfiguredInput::SLOWMOTION))
            return g_ControlScheme.SlowMotion;
        else if (inputControl == GetInputPtr(MaxPayne_ConfiguredInput::BULLETTIME))
            return g_ControlScheme.BulletTime;

        return JoystickButton::Undefined;
    }

    inline char GetLeftStickInput(void* inputControl, DIJOYSTATE* joyState)
    {
        const float stickSensitivity = 1.0f;

        if (inputControl == GetInputPtr(MaxPayne_ConfiguredInput::RUN))
        {
            float stickY = ConvertJoystickToMouseDelta(joyState->lY, stickSensitivity);
            return (stickY < 0.0f) ? 1 : 0;
        }
        else if (inputControl == GetInputPtr(MaxPayne_ConfiguredInput::WALKBACK))
        {
            float stickY = ConvertJoystickToMouseDelta(joyState->lY, stickSensitivity);
            return (stickY > 0.0f) ? 1 : 0;
        }
        else if (inputControl == GetInputPtr(MaxPayne_ConfiguredInput::STRAFELEFT))
        {
            float stickX = ConvertJoystickToMouseDelta(joyState->lX, stickSensitivity);
            return (stickX < 0.0f) ? 1 : 0;
        }
        else if (inputControl == GetInputPtr(MaxPayne_ConfiguredInput::STRAFERIGHT))
        {
            float stickX = ConvertJoystickToMouseDelta(joyState->lX, stickSensitivity);
            return (stickX > 0.0f) ? 1 : 0;
        }

        return 0;
    }

    SafetyHookInline shget = {};
    char __fastcall get(void* _this, void* edx)
    {
        auto ret = shget.unsafe_fastcall<char>(_this, edx);

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
                else
                {
                    ret = GetLeftStickInput(_this, joyState);
                }
            }
        }

        return ret;
    }

    SafetyHookInline shgetSingleClicked = {};
    char __fastcall getSingleClicked(void* _this, void* edx)
    {
        auto ret = shgetSingleClicked.unsafe_fastcall<char>(_this, edx);

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
                else
                {
                    char curr = GetLeftStickInput(_this, joyState);
                    char prev = GetLeftStickInput(_this, prevJoyState);
                    ret = (curr && !prev) ? 1 : 0;
                }
            }
        }

        return ret;
    }

    SafetyHookInline shgetReleased = {};
    char __fastcall getReleased(void* _this, void* edx)
    {
        auto ret = shgetReleased.unsafe_fastcall<char>(_this, edx);

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
                else
                {
                    char curr = GetLeftStickInput(_this, joyState);
                    char prev = GetLeftStickInput(_this, prevJoyState);
                    ret = (!curr && prev) ? 1 : 0;
                }
            }
        }

        return ret;
    }

    SafetyHookInline shreadAimUpDown = {};
    float __fastcall readAimUpDown(void* _this, void* edx)
    {
        auto ret = shreadAimUpDown.unsafe_fastcall<float>(_this, edx);

        if (ret == 0.0f && g_pJoystick)
        {
            DIJOYSTATE* joyState = GetJoyState();
            if (joyState)
            {
                const float stickSensitivity = 1.0f;

                float stickY = ConvertJoystickToMouseDelta(joyState->lRy, stickSensitivity);

                auto Mouse = (uintptr_t)X_Input::getMouse();
                bool bInvertY = (*(uint8_t*)(Mouse + 81)) != 0;
                if (bInvertY)
                    stickY = -stickY;

                ret = stickY;
            }
        }

        return ret;
    }

    SafetyHookInline shreadAimLeftRight = {};
    float __fastcall readAimLeftRight(void* _this, void* edx)
    {
        auto ret = shreadAimLeftRight.unsafe_fastcall<float>(_this, edx);

        if (ret == 0.0f && g_pJoystick)
        {
            DIJOYSTATE* joyState = GetJoyState();
            if (joyState)
            {
                const float stickSensitivity = 1.0f;

                float stickX = ConvertJoystickToMouseDelta(joyState->lRx, stickSensitivity);
                ret = stickX;
            }
        }

        return ret;
    }
}

export void InitInput()
{
    X_Input::getMouse = (void* (*)())GetProcAddress(GetModuleHandle(L"X_Inputmfc"), "?getMouse@X_Input@@SIPAVX_InputDeviceMouse@@XZ");

    auto pattern = hook::pattern("8B 15 ? ? ? ? 50 51 53");
    MaxPayne_ConfiguredInput::sm_control = *pattern.get_first<uintptr_t*>(2);

    pattern = hook::module_pattern(GetModuleHandle(L"X_Inputmfc"), "C7 05 ? ? ? ? ? ? ? ? 8B 4D");
    static auto X_InputconstructHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        CreateInput();
    });

    pattern = hook::module_pattern(GetModuleHandle(L"X_Inputmfc"), "89 1D ? ? ? ? 88 5E ? ? ? 89 5E");
    static auto X_InputdestructHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        CleanupInput();
    });

    pattern = hook::module_pattern(GetModuleHandle(L"X_Inputmfc"), "8B 0D ? ? ? ? 85 C9 74 ? ? ? 56");
    static auto X_InputupdateHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        UpdateInput();
    });

    X_InputControlButton::shget = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"X_Inputmfc"), "?get@X_InputControlButton@@QBE_NXZ"), X_InputControlButton::get);
    X_InputControlButton::shgetSingleClicked = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"X_Inputmfc"), "?getSingleClicked@X_InputControlButton@@QBE_NXZ"), X_InputControlButton::getSingleClicked);
    X_InputControlButton::shgetReleased = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"X_Inputmfc"), "?getReleased@X_InputControlButton@@QBE_NXZ"), X_InputControlButton::getReleased);

    pattern = hook::pattern("83 EC ? 56 8B F1 FF 15 ? ? ? ? 84 C0 74 ? ? ? FF 15 ? ? ? ? 84 C0 74 ? 8D 44 24 ? 50 FF 15 ? ? ? ? 8B C8 FF 15 ? ? ? ? ? ? ? 5E");
    X_InputControlButton::shreadAimUpDown = safetyhook::create_inline(pattern.get_first(), X_InputControlButton::readAimUpDown);

    pattern = hook::pattern("83 EC ? 56 8B F1 FF 15 ? ? ? ? 84 C0 74 ? ? ? FF 15 ? ? ? ? 84 C0 74 ? 8D 44 24 ? 50 FF 15 ? ? ? ? 8B C8 FF 15 ? ? ? ? ? ? 5E");
    X_InputControlButton::shreadAimLeftRight = safetyhook::create_inline(pattern.get_first(), X_InputControlButton::readAimLeftRight);
}