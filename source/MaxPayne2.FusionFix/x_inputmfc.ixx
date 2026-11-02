module;

#include <stdafx.h>
#include <commctrl.h>
#include "FixSettings.hpp"
#include "DeveloperOverlay.hpp"
#include "Shared/MaxPayne/GamepadButtons.hpp"
#include "Shared/MaxPayne/NativeIcons.hpp"
#define DIRECTINPUT_VERSION 0x0700
#include <dinput.h>
#include <stdexcept>
#include "Shared/PadInput.hpp"

export module x_inputmfc;

import ComVars;
import xidi;

#pragma comment(lib, "dinput.lib")
#pragma comment(lib, "dxguid.lib")

// DirectInput 7 interface pointers
static IDirectInput* g_pDI = nullptr;
static IDirectInputDevice* g_pJoystick = nullptr;
static DIJOYSTATE g_LastJoyState = {};
static DIJOYSTATE g_PrevJoyState = {};
static bool g_InputEnabled = false;
static bool g_JoystickDisconnected = false;
static ULONGLONG g_NextJoystickProbe = 0;

DIJOYSTATE NeutralJoystickState()
{
    DIJOYSTATE state{};
    state.lX = state.lY = state.lZ = 32768;
    state.lRx = state.lRy = state.lRz = 32768;
    state.rglSlider[0] = state.rglSlider[1] = 32768;
    for (auto& pov : state.rgdwPOV)
        pov = DWORD(-1);
    return state;
}

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

    hr = g_pJoystick->SetCooperativeLevel(nullptr, DISCL_BACKGROUND | DISCL_NONEXCLUSIVE);
    if (SUCCEEDED(hr))
        hr = g_pJoystick->Acquire();
    if (FAILED(hr))
    {
        g_pJoystick->Release();
        g_pJoystick = nullptr;
        return DIENUM_CONTINUE;
    }

    // Stop enumeration once the device can be used.
    return DIENUM_STOP;
}

void CreateInput()
{
    g_InputEnabled = true;
    g_NextJoystickProbe = GetTickCount64() + 1000;
    if (g_pJoystick)
        return;
    HRESULT hr;

    // Create DirectInput interface (DirectInput 7)
    if (!g_pDI)
    {
        hr = DirectInputCreate(GetModuleHandleA(nullptr), DIRECTINPUT_VERSION, &g_pDI, nullptr);
        if (FAILED(hr))
            return;
    }

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

}

void UpdateOverlayPad()
{
    static constexpr uint16_t buttons[] =
    {
        0x4000, 0x2000, 0x8000, 0x1000, 0x400, 0x800, 0x100, 0x200,
        0x1, 0x8, 0x2, 0x4, 0x10, 0x40, 0x80, 0x20,
    };
    uint16_t held = 0;
    for (size_t i = 0; i < std::size(buttons); ++i)
    {
        if (g_LastJoyState.rgbButtons[i] & 0x80)
            held |= buttons[i];
    }
    auto axis = [](LONG value)
    {
        return static_cast<int8_t>(std::clamp((value - 32768) / 256, LONG(-127), LONG(127)));
    };
    MaxPayneOverlay::Gamepad(held, axis(g_LastJoyState.lX), axis(g_LastJoyState.lY));
}

void UpdateInput()
{
    struct OverlayPadUpdate
    {
        ~OverlayPadUpdate() { UpdateOverlayPad(); }
    } overlayPadUpdate;
    g_JoystickDisconnected = false;
    g_PrevJoyState = g_LastJoyState;
    if (!g_InputEnabled)
        return;
    if (!g_pJoystick && GetTickCount64() >= g_NextJoystickProbe)
        CreateInput();
    if (!g_pJoystick)
    {
        g_LastJoyState = NeutralJoystickState();
        return;
    }

    DIJOYSTATE state{};
    HRESULT hr = g_pJoystick->GetDeviceState(sizeof(state), &state);
    if (FAILED(hr) && SUCCEEDED(g_pJoystick->Acquire()))
        hr = g_pJoystick->GetDeviceState(sizeof(state), &state);
    if (FAILED(hr))
    {
        g_LastJoyState = NeutralJoystickState();
        g_JoystickDisconnected = true;
        g_pJoystick->Unacquire();
        g_pJoystick->Release();
        g_pJoystick = nullptr;
        g_NextJoystickProbe = GetTickCount64() + 1000;
        return;
    }
    g_LastJoyState = state;

    // Held input must not take the device back from a newer keyboard or mouse action.
    auto ChangedAxis = [](LONG value, LONG previous)
    {
        return std::abs(value - 32768) > 4915 && std::abs(value - previous) > 128;
    };
    bool bActive = ChangedAxis(g_LastJoyState.lX, g_PrevJoyState.lX) || ChangedAxis(g_LastJoyState.lY, g_PrevJoyState.lY) ||
        ChangedAxis(g_LastJoyState.lRx, g_PrevJoyState.lRx) || ChangedAxis(g_LastJoyState.lRy, g_PrevJoyState.lRy);
    for (int i = 0; i < 16 && !bActive; ++i)
        bActive = (g_LastJoyState.rgbButtons[i] & 0x80) != 0 && (g_PrevJoyState.rgbButtons[i] & 0x80) == 0;
    if (bActive)
        MaxPayneGamepadButtons::OnPadActivity();
}

void CleanupInput()
{
    g_InputEnabled = false;
    g_JoystickDisconnected = false;
    g_LastJoyState = NeutralJoystickState();
    g_PrevJoyState = g_LastJoyState;
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

// Buttons that skip videos, pressed on any controller. Read with XInput directly: the intro plays
// before the game sets up its controller, and Xidi's keyboard mapping only starts with that.
WORD GetVideoSkipButtons()
{
    auto pXInputGetState = PadInput::XInput().getState;

    WORD wButtons = 0;
    if (pXInputGetState)
    {
        for (DWORD i = 0; i < XUSER_MAX_COUNT; ++i)
        {
            XINPUT_STATE state = {};
            if (pXInputGetState(i, &state) == ERROR_SUCCESS)
                wButtons |= state.Gamepad.wButtons;
        }
    }

    constexpr WORD wSkipButtons = XINPUT_GAMEPAD_A | XINPUT_GAMEPAD_B | XINPUT_GAMEPAD_X | XINPUT_GAMEPAD_Y | XINPUT_GAMEPAD_START | XINPUT_GAMEPAD_BACK;
    return wButtons & wSkipButtons;
}

// Stands in for PeekMessageA in the loop of the video player, which stops on a key press message.
// A gamepad button press is reported as one.
export BOOL WINAPI VideoPeekMessageA(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg)
{
    using namespace std::chrono_literals;
    using clock = std::chrono::steady_clock;
    static clock::time_point lastPoll;
    static WORD wPrevButtons = 0;

    auto now = clock::now();
    if (now - lastPoll >= 30ms)
    {
        // The loop calls this all the time, a longer gap means another video started. Buttons
        // held from before it, like the one that started a new game, don't skip it.
        bool bNewVideo = now - lastPoll > 500ms;
        WORD wButtons = GetVideoSkipButtons();
        WORD wNewlyPressed = bNewVideo ? 0 : (wButtons & ~wPrevButtons);
        wPrevButtons = wButtons;
        lastPoll = now;

        if (wNewlyPressed)
        {
            *lpMsg = { .hwnd = hWnd, .message = WM_KEYDOWN, .wParam = VK_ESCAPE };
            return TRUE;
        }
    }

    return PeekMessageA(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
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

// The game slows the mouse down by the sniper zoom factor while zooming, the right stick needs the same
float GetAimZoomSensitivity()
{
    return MP_GameMode::IsSniperZooming() ? X_InputDeviceMouse::GetZoomSensitivity() : 1.0f;
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
        return reinterpret_cast<void*>(MaxPayne_ConfiguredInput::sm_control + INPUT_STRIDE * i);
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
        if (MaxPayneOverlay::BlocksInput())
            return 0;
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
        if (MaxPayneOverlay::BlocksInput())
            return 0;
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
        if (MaxPayneOverlay::BlocksInput())
            return 0;
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

    static constexpr float gameHorizontalMultiplier = 1.4f;
    static constexpr float gameHorizontalBias = 0.1f;
    static constexpr float gameVerticalMultiplier = gameHorizontalMultiplier;
    static constexpr float gameVerticalBias = gameHorizontalBias;

    SafetyHookInline shreadAimUpDown = {};
    float __fastcall readAimUpDown(void* _this, void* edx)
    {
        if (MaxPayneOverlay::BlocksInput())
            return 0;
        auto ret = shreadAimUpDown.unsafe_fastcall<float>(_this, edx);

        if (ret == 0.0f && g_pJoystick)
        {
            DIJOYSTATE* joyState = GetJoyState();
            if (joyState)
            {
                auto Mouse = (uintptr_t)X_Input::getMouse();
                bool bInvertY = (*(uint8_t*)(Mouse + 81)) != 0;
                auto fVerSens = *(float*)(Mouse + 87);
                const float stickSensitivity = (gameVerticalMultiplier * fVerSens + gameVerticalBias) * FixSettings().Float("MAIN.GamepadCameraSpeed") * GetAimZoomSensitivity();

                float stickY = ConvertJoystickToMouseDelta(joyState->lRy, stickSensitivity);

                if (bInvertY != FixSettings().Bool("MAIN.InvertGamepadY"))
                    stickY = -stickY;

                ret = stickY;
            }
        }

        return ret;
    }

    SafetyHookInline shreadAimLeftRight = {};
    float __fastcall readAimLeftRight(void* _this, void* edx)
    {
        if (MaxPayneOverlay::BlocksInput())
            return 0;
        auto ret = shreadAimLeftRight.unsafe_fastcall<float>(_this, edx);

        if (ret == 0.0f && g_pJoystick)
        {
            DIJOYSTATE* joyState = GetJoyState();
            if (joyState)
            {
                auto Mouse = (uintptr_t)X_Input::getMouse();
                auto fHorSens = *(float*)(Mouse + 83);
                const float stickSensitivity = (gameHorizontalMultiplier * fHorSens + gameHorizontalBias) * FixSettings().Float("MAIN.GamepadCameraSpeed") * GetAimZoomSensitivity();

                float stickX = ConvertJoystickToMouseDelta(joyState->lRx, stickSensitivity);
                ret = stickX;
            }
        }

        return ret;
    }

    // Tips name the key of an action by a token, "Press #USE# to ...", replaced with the name
    // MaxPayne_ConfiguredInput gives the control (a std::string). While a controller is in use, the controls the fix
    // maps get the controller's button instead.
    struct GameString
    {
        uint32_t allocator;
        char szText[16];
        uint32_t nLength;
        uint32_t nReserved;
    };

    std::string ReadString(const GameString* value)
    {
        if (!value || value->nLength > 65536)
            return {};
        auto text = value->nReserved >= sizeof(value->szText) ? *reinterpret_cast<const char* const*>(value->szText) : value->szText;
        return text ? std::string(text, value->nLength) : std::string();
    }

    struct Substitution
    {
        std::string keyboard;
        std::string pad;
    };
    thread_local Substitution CurrentSubstitution;
    void* (__fastcall* describe)(void* pControl, void* edx) = nullptr;
    void* __fastcall describeHook(void* pControl, void* edx)
    {
        auto original = static_cast<GameString*>(describe(pControl, edx));
        CurrentSubstitution = {};
        CurrentSubstitution.keyboard = ReadString(original);
        static auto& nGamepadIcons = FixSettings().Int("MAIN.GamepadIcons");
        const char* szPadName = nullptr;
        if (pControl == GetInputPtr(MaxPayne_ConfiguredInput::RUN) || pControl == GetInputPtr(MaxPayne_ConfiguredInput::WALKBACK) ||
            pControl == GetInputPtr(MaxPayne_ConfiguredInput::STRAFELEFT) || pControl == GetInputPtr(MaxPayne_ConfiguredInput::STRAFERIGHT))
            szPadName = MaxPayneIcons::Ready ? MaxPayneIcons::Glyph(19) : "Left Stick";
        else
        {
            auto button = GetButtonForInput(pControl);
            szPadName = MaxPayneIcons::Button(button);
            if (!szPadName)
                szPadName = MaxPayneGamepadButtons::Name(button, nGamepadIcons == MaxPayneGamepadButtons::Auto ? MaxPayneGamepadButtons::DetectFamily() : nGamepadIcons);
        }
        if (!szPadName)
            return original;
        CurrentSubstitution.pad = szPadName;
        if (!MaxPayneGamepadButtons::IsPadInUse())
            return original;
        static GameString padName = {};
        padName.nLength = uint32_t(std::min<size_t>(std::strlen(szPadName), sizeof(padName.szText) - 1));
        padName.nReserved = sizeof(padName.szText) - 1;
        std::memcpy(padName.szText, szPadName, padName.nLength);
        padName.szText[padName.nLength] = '\0';
        return &padName;
    }

    using ReplaceString = GameString* (__thiscall*)(GameString*, unsigned, unsigned, const GameString*, unsigned, unsigned);
    ReplaceString OriginalReplace = nullptr;
    GameString* __fastcall ReplacePrompt(GameString* self, void*, unsigned position, unsigned length,
                                          const GameString* name, unsigned start, unsigned count)
    {
        auto before = ReadString(self);
        auto alternatives = MaxPayneIcons::Alternatives(before);
        if (position <= before.size() && !CurrentSubstitution.pad.empty())
        {
            auto token = before.substr(position, length);
            MaxPayneIcons::Replace(alternatives.keyboard, token, CurrentSubstitution.keyboard);
            MaxPayneIcons::Replace(alternatives.pad, token, CurrentSubstitution.pad);
        }
        auto result = OriginalReplace(self, position, length, name, start, count);
        if (MaxPayneIcons::Ready && !CurrentSubstitution.pad.empty())
            MaxPayneIcons::RegisterPrompt(ReadString(self), std::move(alternatives));
        CurrentSubstitution = {};
        return result;
    }

}

export std::string GetControlIcon(std::string_view action)
{
    using namespace X_InputControlButton;
    if (!MaxPayneIcons::Ready)
        return {};
    if (action == "RUN") return MaxPayneIcons::Glyph(22);
    if (action == "WALKBACK") return MaxPayneIcons::Glyph(23);
    if (action == "STRAFELEFT" || action == "STRAFERIGHT") return MaxPayneIcons::Glyph(21);
    struct Binding { std::string_view name; JoystickButton ControlScheme::* button; };
    static constexpr Binding bindings[] = {
        { "SHOOT", &ControlScheme::Shoot }, { "ATTACK", &ControlScheme::Attack },
        { "RELOAD", &ControlScheme::Reload }, { "JUMP", &ControlScheme::Jump },
        { "CROUCH", &ControlScheme::Crouch }, { "USE", &ControlScheme::Use },
        { "PAINKILLER", &ControlScheme::Painkiller }, { "BULLETTIME", &ControlScheme::BulletTime },
        { "SLOWMOTION", &ControlScheme::SlowMotion }, { "PAUSE", &ControlScheme::Pause },
        { "NEXTWEAPON", &ControlScheme::NextWeapon }, { "PREVIOUSWEAPON", &ControlScheme::PreviousWeapon },
        { "BESTWEAPON", &ControlScheme::BestWeapon },
    };
    for (auto& binding : bindings)
        if (binding.name == action)
            if (auto glyph = MaxPayneIcons::Button(g_ControlScheme.*binding.button))
                return glyph;
    return {};
}

bool g_VibrationThisFrame = false;
bool g_VibrationActive = false;
int g_VibrationCurrentStrength = 0;
std::chrono::steady_clock::time_point g_VibrationStart = {};
std::chrono::milliseconds g_VibrationDuration{ 150 };

void Vibrate(int strength, std::chrono::milliseconds duration = std::chrono::milliseconds{ 150 })
{
    const auto sendVibration = XidiSendVibration.load(std::memory_order_acquire);
    if (!sendVibration || !FixSettings().Bool("MAIN.Vibration"))
        return;

    if (g_VibrationThisFrame)
        return;

    if (strength < 0)
        return;

    if (g_VibrationActive && strength < g_VibrationCurrentStrength)
        return;

    g_VibrationThisFrame = true;
    g_VibrationActive = true;
    g_VibrationCurrentStrength = strength;
    g_VibrationStart = std::chrono::steady_clock::now();
    g_VibrationDuration = duration;

    constexpr int RUMBLE_MAX_STRENGTH = 100;
    constexpr WORD RUMBLE_MAX_VALUE = 0xFFFF;

    const WORD motor = static_cast<WORD>(
        static_cast<float>(std::clamp(strength, 0, RUMBLE_MAX_STRENGTH))
        / RUMBLE_MAX_STRENGTH * RUMBLE_MAX_VALUE);

    sendVibration(-1, motor, motor);
}

// Stops a vibration once its time is up. Called every tick and from loading screens, which block the game loop.
export void UpdateVibration()
{
    g_VibrationThisFrame = false;

    if (g_VibrationActive)
    {
        auto elapsed = std::chrono::steady_clock::now() - g_VibrationStart;
        if (elapsed >= g_VibrationDuration || !FixSettings().Bool("MAIN.Vibration"))
        {
            g_VibrationActive = false;
            g_VibrationCurrentStrength = 0;
            if (const auto sendVibration = XidiSendVibration.load(std::memory_order_acquire))
                sendVibration(-1, 0, 0);
        }
    }
}

namespace
{
    std::atomic<bool> Enabled{ false };
    std::atomic<bool> Relative{ false };
    std::atomic<int64_t> DeltaX{ 0 }, DeltaY{ 0 };
    HWND Window = nullptr;
    bool Registered = false;
    bool OwnRegistration = false;
    HWND CandidateWindow = nullptr;
    bool WasEnabled = false;
    bool HavePrevious = false;
    RAWINPUTDEVICE Previous{};
    SafetyHookMid MouseHook;
    SafetyHookInline MouseUpdateHook;
    uint8_t* SuspendedMouse = nullptr;
    constexpr UINT_PTR SubclassId = 0x4D505241;

    void ClearMovement()
    {
        Relative.store(false);
        DeltaX.store(0);
        DeltaY.store(0);
    }

    template<typename T>
    T ReadMouse(uint8_t* address)
    {
        T value;
        std::memcpy(&value, address, sizeof(value));
        return value;
    }

    // X_InputDeviceMouse::update uses the same state offsets as the first game. Keep its
    // accumulated movement and button edges neutral while its exclusive device is released.
    template<typename T>
    void WriteMouse(uint8_t* address, const T& value)
    {
        std::memcpy(address, &value, sizeof(value));
    }

    void NeutralMouse(uint8_t* mouse)
    {
        WriteMouse(mouse + 9, DIMOUSESTATE2{});
        for (auto offset : { 91, 107 })
            WriteMouse(mouse + offset + 8, ReadMouse<std::array<float, 2>>(mouse + offset));
        WriteMouse(mouse + 127, ReadMouse<float>(mouse + 123));
        auto buttons = ReadMouse<uint8_t*>(mouse + 73);
        int count = ReadMouse<int>(mouse + 45);
        if (buttons && count >= 0 && count <= 8)
        {
            for (int i = 0; i < count; ++i)
            {
                WriteMouse(buttons + i * 15, int{});
                WriteMouse(buttons + i * 15 + 8, uint16_t{});
            }
        }
    }

    void __fastcall UpdateMouse(uint8_t* self, void* edx, float delta)
    {
        if (MaxPayneOverlay::BlocksInput())
        {
            if (SuspendedMouse != self)
            {
                auto device = ReadMouse<IDirectInputDeviceA*>(self + 5);
                if (device)
                    device->Unacquire();
                SuspendedMouse = self;
            }
            ClearMovement();
            NeutralMouse(self);
            return;
        }
        if (SuspendedMouse == self)
        {
            SuspendedMouse = nullptr;
            ClearMovement();
        }
        MouseUpdateHook.unsafe_fastcall(self, edx, delta);
    }

    bool MouseRegistration(RAWINPUTDEVICE& mouse)
    {
        UINT count = 0;
        if (GetRegisteredRawInputDevices(nullptr, &count, sizeof(RAWINPUTDEVICE)) == UINT(-1) || !count)
            return false;
        std::vector<RAWINPUTDEVICE> devices(count);
        if (GetRegisteredRawInputDevices(devices.data(), &count, sizeof(RAWINPUTDEVICE)) == UINT(-1))
            return false;
        for (auto& device : devices)
        {
            if (device.usUsagePage == 1 && device.usUsage == 2)
            {
                mouse = device;
                return true;
            }
        }
        return false;
    }

    void RestoreRegistration()
    {
        RAWINPUTDEVICE current{};
        if (OwnRegistration && MouseRegistration(current) && current.hwndTarget == Window && current.dwFlags == 0)
        {
            RAWINPUTDEVICE device = HavePrevious ? Previous : RAWINPUTDEVICE{ 1, 2, RIDEV_REMOVE, nullptr };
            RegisterRawInputDevices(&device, 1, sizeof(device));
        }
        Registered = false;
        OwnRegistration = false;
        ClearMovement();
    }

    LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR)
    {
        if (MaxPayneOverlay::BlocksInput())
            ClearMovement();
        else if (message == WM_INPUT && Registered && Enabled.load() && GetForegroundWindow() == window)
        {
            RAWINPUT input{};
            UINT size = sizeof(input);
            if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, &input, &size, sizeof(RAWINPUTHEADER)) != UINT(-1) &&
                input.header.dwType == RIM_TYPEMOUSE)
            {
                if (input.data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE)
                    ClearMovement();
                else
                {
                    DeltaX.fetch_add(input.data.mouse.lLastX);
                    DeltaY.fetch_add(input.data.mouse.lLastY);
                    Relative.store(true);
                }
            }
        }
        else if (message == WM_KILLFOCUS || (message == WM_ACTIVATEAPP && !wParam))
            ClearMovement();
        else if (message == WM_NCDESTROY)
        {
            RestoreRegistration();
            RemoveWindowSubclass(window, WindowProc, SubclassId);
            Window = nullptr;
            CandidateWindow = nullptr;
        }
        return DefSubclassProc(window, message, wParam, lParam);
    }
}

export void UpdateRawMouseWindow(HWND window)
{
    if (!MouseHook)
        return;
    bool enabled = Enabled.load();
    if (CandidateWindow == window && WasEnabled == enabled)
    {
        RAWINPUTDEVICE current{};
        if (Registered && (!MouseRegistration(current) || current.hwndTarget != Window || (OwnRegistration && current.dwFlags != 0)))
        {
            // A later registration belongs to its caller; keep it and return to DirectInput.
            OwnRegistration = false;
            RestoreRegistration();
            if (Window)
                RemoveWindowSubclass(Window, WindowProc, SubclassId);
            Window = nullptr;
            OutputDebugStringA("Max Payne 2: raw mouse ownership changed; using DirectInput.\n");
        }
        return;
    }
    CandidateWindow = window;
    WasEnabled = enabled;
    RestoreRegistration();
    if (Window)
        RemoveWindowSubclass(Window, WindowProc, SubclassId);
    Window = nullptr;
    if (!enabled || !window || !IsWindow(window))
        return;

    RAWINPUTDEVICE current{};
    HavePrevious = MouseRegistration(current);
    // Another plugin's explicit target keeps ownership of its input.
    if (HavePrevious && current.hwndTarget && current.hwndTarget != window)
    {
        OutputDebugStringA("Max Payne 2: another window owns raw mouse input; using DirectInput.\n");
        return;
    }
    Previous = current;
    Window = window;
    if (!SetWindowSubclass(window, WindowProc, SubclassId, 0))
    {
        Window = nullptr;
        return;
    }
    if (HavePrevious && current.hwndTarget == window)
        Registered = true;
    else
    {
        RAWINPUTDEVICE mouse{ 1, 2, 0, window };
        OwnRegistration = RegisterRawInputDevices(&mouse, 1, sizeof(mouse)) != FALSE;
        Registered = OwnRegistration;
    }
    if (!Registered)
    {
        RemoveWindowSubclass(window, WindowProc, SubclassId);
        Window = nullptr;
        OutputDebugStringA("Max Payne 2: raw mouse registration failed; using DirectInput.\n");
    }
}

void InitRawMouse()
{
    auto module = GetModuleHandleW(L"X_Inputmfc.dll");
    auto update = hook::module_pattern(module, "64 A1 00 00 00 00 6A FF 68 ? ? ? ? 50 8B 44 24 10 64 89 25 00 00 00 00 83 EC 1C 57 8B F9 89 47 4D 8B 47 05");
    MouseUpdateHook = safetyhook::create_inline(update.get_first(), UpdateMouse);
    if (!MouseUpdateHook)
        throw std::runtime_error("Mouse update hook unavailable");
    auto pattern = hook::module_pattern(module, "D9 47 53 8B 4F 5F D8 0D ? ? ? ? 8B 97 87 00 00 00 8B 47 5B");
    MouseHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& context)
    {
        if (!Enabled.load() || !Registered || !Relative.load() || GetForegroundWindow() != Window)
            return;
        auto mouse = reinterpret_cast<uint8_t*>(context.edi);
        MaxPayneOverlay::Mouse(mouse + 9);
        constexpr int64_t minimum = std::numeric_limits<LONG>::min();
        constexpr int64_t maximum = std::numeric_limits<LONG>::max();
        WriteMouse(mouse + 9, static_cast<LONG>(std::clamp(DeltaX.exchange(0), minimum, maximum)));
        WriteMouse(mouse + 13, static_cast<LONG>(std::clamp(DeltaY.exchange(0), minimum, maximum)));
    });
    if (!MouseHook)
    {
        MouseUpdateHook.reset();
        throw std::runtime_error("Raw mouse hook unavailable");
    }
    Enabled.store(FixSettings().Bool("MAIN.RawInputMouse"));
    FixSettings().OnChange("MAIN.RawInputMouse", []() { Enabled.store(FixSettings().Bool("MAIN.RawInputMouse")); ClearMovement(); });
}

void ShutdownRawMouse()
{
    Enabled.store(false);
    RestoreRegistration();
    if (Window)
        RemoveWindowSubclass(Window, WindowProc, SubclassId);
    Window = nullptr;
    CandidateWindow = nullptr;
    MouseHook.reset();
    MouseUpdateHook.reset();
    SuspendedMouse = nullptr;
}

export void InitInput()
{
    InitRawMouse();
    X_Input::getMouse = (void* (*)())GetProcAddress(GetModuleHandle(L"X_Inputmfc"), "?getMouse@X_Input@@SIPAVX_InputDeviceMouse@@XZ");

    auto pattern = hook::pattern("8B 15 ? ? ? ? 50 51 53");
    MaxPayne_ConfiguredInput::sm_control.SetAddress(*pattern.get_first<uintptr_t*>(2));

    // Mouse smoothing: each frame applies half of the movement not applied yet and carries the rest
    // over to the next one. Switched off, the movement applies in the frame it's made.
    pattern = hook::module_pattern(GetModuleHandle(L"X_Inputmfc"), "8D 74 24 ? E8 ? ? ? ? D9 44 24 ? D8 47 5B"); // X_InputDeviceMouse::update
    static auto pSmoothingCall = (uintptr_t)pattern.get_first(4); //0x10006DCA
    static auto MouseSmoothingHook = safetyhook::create_mid(pSmoothingCall, [](SafetyHookContext& regs)
    {
        static auto& nMouseSmoothing = FixSettings().Int("MAIN.MouseSmoothing");
        if (!nMouseSmoothing)
            regs.eip = pSmoothingCall + 5;
    });

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

    // Tips with controller button names, see X_InputControlButton::describeHook
    pattern = hook::pattern("A1 ? ? ? ? 8B 4C 24 ? 8B 7C 24 ? 03 C8 E8"); // the tip's token substitution
    auto replacement = hook::pattern("6A FF 53 50 57 56 8B CD E8 ? ? ? ? 83 7C 24 ? 10 8B 44 24");
    X_InputControlButton::describe = (decltype(X_InputControlButton::describe))injector::MakeCALL(pattern.get_first(15), X_InputControlButton::describeHook, true).get(); //0x4A2C6F
    X_InputControlButton::OriginalReplace = (X_InputControlButton::ReplaceString)injector::MakeCALL(replacement.get_first(8), X_InputControlButton::ReplacePrompt, true).get();

    pattern = hook::pattern("83 EC ? 56 8B F1 FF 15 ? ? ? ? 84 C0 74 ? ? ? FF 15 ? ? ? ? 84 C0 74 ? 8D 44 24 ? 50 FF 15 ? ? ? ? 8B C8 FF 15 ? ? ? ? ? ? ? 5E");
    X_InputControlButton::shreadAimUpDown = safetyhook::create_inline(pattern.get_first(), X_InputControlButton::readAimUpDown);

    pattern = hook::pattern("83 EC ? 56 8B F1 FF 15 ? ? ? ? 84 C0 74 ? ? ? FF 15 ? ? ? ? 84 C0 74 ? 8D 44 24 ? 50 FF 15 ? ? ? ? 8B C8 FF 15 ? ? ? ? ? ? 5E");
    X_InputControlButton::shreadAimLeftRight = safetyhook::create_inline(pattern.get_first(), X_InputControlButton::readAimLeftRight);

    pattern = hook::pattern("8A 86 ? ? ? ? 84 C0 57 74 ? 8D 8E");
    static auto ApplicationTickHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        UpdateVibration();
    });

    // Sniper scope enter
    pattern = hook::pattern("C7 86 ? ? ? ? ? ? ? ? E9 ? ? ? ? 84 C0");
    static auto MaxPayne_GameModeupdateHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        Vibrate(14, std::chrono::milliseconds{ 100 });
    });

    // Painkiller use
    pattern = hook::pattern("FF 25 ? ? ? ? 5E");
    static auto X_GlobalPainkillerSettingssendMessagesHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        Vibrate(23, std::chrono::milliseconds{ 200 });
    });

    // Damage taken, X_Character::setHealth runs for every character: esi is the character
    pattern = hook::module_pattern(GetModuleHandle(L"X_GameObjectsMFC"), "E8 ? ? ? ? 8B CF E8 ? ? ? ? 48");
    static auto X_CharactersetHealthHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        if ((uint8_t*)regs.esi == MP_GameMode::GetPlayerCharacter())
            Vibrate(46, std::chrono::milliseconds{ 180 });
    });

    // Shooting, the input evaluator states also drive the enemies: esi is the state, its
    // X_CharacterProperties are at +0xF. Only guns vibrate.
    static auto IsPlayerHoldingGun = [](SafetyHookContext& regs)
    {
        auto pCharacterProperties = *(uint8_t**)(regs.esi + 0xF);
        return X_Character::IsPlayerCharacterProperties(pCharacterProperties) && X_CharacterProperties::IsHoldingGun(pCharacterProperties);
    };

    // X_ActionState_DodgeSpecific, shooting while shootdodging
    pattern = hook::module_pattern(GetModuleHandle(L"X_GameObjectsMFC"), "E8 ? ? ? ? 8B 44 24 ? 50 8B CE E8 ? ? ? ? 5E");
    static auto X_CharacterPropertiessetIsShooting1 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        if (IsPlayerHoldingGun(regs))
            Vibrate(80, std::chrono::milliseconds{ 80 });
    });

    // X_ActionState_WeaponSpecific: ebx is the action, CHARANIM_SHOOTEMPTY for shooting the weapon in
    // hand, CHARANIM_ATTACKMELEE for secondary attacks (melee, also on an empty clip, and throwing)
    constexpr uintptr_t ACTION_SHOOT = 430;
    pattern = hook::module_pattern(GetModuleHandle(L"X_GameObjectsMFC"), "E8 ? ? ? ? 8B C8 E8 ? ? ? ? 5E 5B");
    static auto X_CharacterPropertiessetIsShooting2 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        if (regs.ebx == ACTION_SHOOT && IsPlayerHoldingGun(regs))
            Vibrate(80, std::chrono::milliseconds{ 80 });
    });

    pattern = hook::module_pattern(GetModuleHandle(L"X_GameObjectsMFC"), "E8 ? ? ? ? 8B C8 E8 ? ? ? ? 53 8B CE E8 ? ? ? ? 5E");
    static auto X_CharacterPropertiessetIsShooting3 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        if (regs.ebx == ACTION_SHOOT && IsPlayerHoldingGun(regs))
            Vibrate(80, std::chrono::milliseconds{ 80 });
    });

    // Explosion
    static std::string CameraPathName;
    pattern = hook::module_pattern(GetModuleHandle(L"X_GameObjectsMFC"), "FF 15 ? ? ? ? 68 ? ? ? ? 8D 44 24 ? 50 8D 4F ? E8 ? ? ? ? ? ? 3B 47 ? 74 ? 8B 40 ? EB ? 8B 44 24 ? 8B 4E ? 69 C0 ? ? ? ? 03 C8 89 4E ? E8 ? ? ? ? 8B 44 24");
    static auto X_CameraImplementationreceiveAnimateInPlaceHook1 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        CameraPathName = std::string_view((char*)(regs.eax + 4));
    });

    pattern = hook::module_pattern(GetModuleHandle(L"X_GameObjectsMFC"), "89 4E ? E8 ? ? ? ? 8B 44 24");
    static auto X_CameraImplementationreceiveAnimateInPlaceHook2 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        if (CameraPathName == "explosion")
            Vibrate(100, std::chrono::milliseconds{ 500 });
        CameraPathName.clear();
    });
}

class RawMouse
{
public:
    RawMouse()
    {
        WFP::onShutdownEvent() += []()
        {
            ShutdownRawMouse();
        };
    }
} RawMouse;
