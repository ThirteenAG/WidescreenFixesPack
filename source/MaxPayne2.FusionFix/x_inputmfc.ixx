module;

#include <stdafx.h>
#include <commctrl.h>
#include "FixSettings.hpp"
#include "Shared/BufferedRawMouse.hpp"
#include "DeveloperOverlay.hpp"
#include "rawinput.hpp"
#include "Shared/MaxPayne/GamepadButtons.hpp"
#include "Shared/MaxPayne/NativeIcons.hpp"
#define DIRECTINPUT_VERSION 0x0700
#include <dinput.h>
#include <stdexcept>
#include "Shared/MaxPayne/Pad.hpp"
#include "Shared/MaxPayne/Vibration.hpp"

export module x_inputmfc;

import ComVars;
import xidi;

void UpdateInput()
{
    // The game never opens a DirectInput joystick, and Xidi starts reading the controller only when asked for one:
    // without this its menu mapping (keys for the menus, Esc on Start) stayed off until the first vibration
    static bool bXidiStarted = false;
    if (!bXidiStarted)
    {
        if (auto send = XidiSendVibration.load(std::memory_order_acquire))
        {
            send(-1, 0, 0);
            bXidiStarted = true;
        }
    }
    // The menu mappings move the cursor with the right stick
    const auto profile = GamepadProfile.load(std::memory_order_relaxed);
    MaxPaynePad::Update(profile == eGamepadProfile::Menu || profile == eGamepadProfile::GraphicNovel);
    if constexpr (WFP::DeveloperOverlay)
        MaxPaynePad::ToOverlay(MaxPayneOverlay::Gamepad);
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
        WORD wButtons = MaxPaynePad::VideoSkipButtons();
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

    // The left stick as the four movement keys
    inline char GetLeftStickInput(void* inputControl, bool previous)
    {
        auto stick = MaxPaynePad::LeftStick(previous);
        if (inputControl == GetInputPtr(MaxPayne_ConfiguredInput::RUN))
            return MaxPaynePad::Direction(stick, 0.0f, -1.0f);
        else if (inputControl == GetInputPtr(MaxPayne_ConfiguredInput::WALKBACK))
            return MaxPaynePad::Direction(stick, 0.0f, 1.0f);
        else if (inputControl == GetInputPtr(MaxPayne_ConfiguredInput::STRAFELEFT))
            return MaxPaynePad::Direction(stick, -1.0f, 0.0f);
        else if (inputControl == GetInputPtr(MaxPayne_ConfiguredInput::STRAFERIGHT))
            return MaxPaynePad::Direction(stick, 1.0f, 0.0f);
        return 0;
    }

    SafetyHookInline shget = {};
    char __fastcall get(void* _this, void* edx)
    {
        if (MaxPayneOverlay::BlocksInput())
            return 0;
        auto ret = shget.unsafe_fastcall<char>(_this, edx);
        if (ret != 0)
            return ret;

        JoystickButton button = GetButtonForInput(_this);
        if (button != Undefined)
            return MaxPaynePad::Pressed(button) ? 1 : 0;
        return GetLeftStickInput(_this, false);
    }

    SafetyHookInline shgetSingleClicked = {};
    char __fastcall getSingleClicked(void* _this, void* edx)
    {
        if (MaxPayneOverlay::BlocksInput())
            return 0;
        auto ret = shgetSingleClicked.unsafe_fastcall<char>(_this, edx);
        if (ret != 0)
            return ret;

        JoystickButton button = GetButtonForInput(_this);
        if (button != Undefined)
            return MaxPaynePad::JustPressed(button) ? 1 : 0;
        return (GetLeftStickInput(_this, false) && !GetLeftStickInput(_this, true)) ? 1 : 0;
    }

    SafetyHookInline shgetReleased = {};
    char __fastcall getReleased(void* _this, void* edx)
    {
        if (MaxPayneOverlay::BlocksInput())
            return 0;
        auto ret = shgetReleased.unsafe_fastcall<char>(_this, edx);
        if (ret != 0)
            return ret;

        JoystickButton button = GetButtonForInput(_this);
        if (button != Undefined)
            return MaxPaynePad::JustReleased(button) ? 1 : 0;
        return (!GetLeftStickInput(_this, false) && GetLeftStickInput(_this, true)) ? 1 : 0;
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
        if (ret != 0.0f)
            return ret;

        auto Mouse = (uintptr_t)X_Input::getMouse();
        bool bInvertY = (*(uint8_t*)(Mouse + 81)) != 0;
        auto fVerSens = *(float*)(Mouse + 87);
        const float stickSensitivity = (gameVerticalMultiplier * fVerSens + gameVerticalBias) * FixSettings().Float("MAIN.GamepadCameraSpeed") * GetAimZoomSensitivity();
        float stickY = MaxPaynePad::RightStick().y * stickSensitivity;
        if (bInvertY != FixSettings().Bool("MAIN.InvertGamepadY"))
            stickY = -stickY;
        return stickY;
    }

    SafetyHookInline shreadAimLeftRight = {};
    float __fastcall readAimLeftRight(void* _this, void* edx)
    {
        if (MaxPayneOverlay::BlocksInput())
            return 0;
        auto ret = shreadAimLeftRight.unsafe_fastcall<float>(_this, edx);
        if (ret != 0.0f)
            return ret;

        auto Mouse = (uintptr_t)X_Input::getMouse();
        auto fHorSens = *(float*)(Mouse + 83);
        const float stickSensitivity = (gameHorizontalMultiplier * fHorSens + gameHorizontalBias) * FixSettings().Float("MAIN.GamepadCameraSpeed") * GetAimZoomSensitivity();
        return MaxPaynePad::RightStick().x * stickSensitivity;
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
            szPadName = MaxPayneIcons::Glyph(19);
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
        if (!CurrentSubstitution.pad.empty())
            MaxPayneIcons::RegisterPrompt(ReadString(self), std::move(alternatives));
        CurrentSubstitution = {};
        return result;
    }

}

export std::string GetControlIcon(std::string_view action)
{
    using namespace X_InputControlButton;
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

MaxPayneVibration Vibration;

void Vibrate(int strength, std::chrono::milliseconds duration = std::chrono::milliseconds{ 150 })
{
    Vibration.Pulse(XidiSendVibration.load(std::memory_order_acquire), MaxPayneGamepadButtons::selectedSlot.load(),
        FixSettings().Get("MAIN.Vibration") == "1", strength, duration);
}

// Called during gameplay and from loading screens so pulses can expire in either case.
export void UpdateVibration()
{
    Vibration.Update(XidiSendVibration.load(std::memory_order_acquire), MaxPayneGamepadButtons::selectedSlot.load(),
        FixSettings().Get("MAIN.Vibration") == "1");
}

namespace
{
    std::atomic<bool> Enabled{ false };
    BufferedRawMouse Movement;
    std::atomic<HWND> Window = nullptr;
    std::atomic<bool> Reading{ false };
    HWND InputWindow = nullptr;
    bool Registered = false;
    bool OwnRegistration = false;
    HWND CandidateWindow = nullptr;
    bool WasEnabled = false;
    ULONGLONG nextRegistrationCheck = 0;
    bool HavePrevious = false;
    RAWINPUTDEVICE Previous{};
    SafetyHookMid MouseAcquireHook;
    SafetyHookMid MouseStateHook;
    SafetyHookMid MouseHook;
    SafetyHookInline MouseUpdateHook;
    SafetyHookMid KeyboardStateHook, KeyboardRetryHook;
    uintptr_t KeyboardUpdateReturn = 0;
    uint8_t* SuspendedMouse = nullptr;
    constexpr UINT_PTR SubclassId = 0x4D505241;

    void ClearMovement()
    {
        Movement.Clear();
    }

    bool RawMouseActive()
    {
        return Enabled.load() && Registered && GetForegroundWindow() == Window;
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

    void ClearButtonStates(uint8_t* buttons, int count)
    {
        if (!buttons)
            return;
        for (int i = 0; i < count; ++i)
        {
            WriteMouse(buttons + i * 15, int{});
            WriteMouse(buttons + i * 15 + 8, uint16_t{});
        }
    }

    void NeutralMouse(uint8_t* mouse)
    {
        WriteMouse(mouse + 9, DIMOUSESTATE2{});
        for (auto offset : { 91, 107 })
            WriteMouse(mouse + offset + 8, ReadMouse<std::array<float, 2>>(mouse + offset));
        WriteMouse(mouse + 127, ReadMouse<float>(mouse + 123));
        auto buttons = ReadMouse<uint8_t*>(mouse + 73);
        int count = ReadMouse<int>(mouse + 45);
        if (count >= 0 && count <= 8)
            ClearButtonStates(buttons, count);
    }

    void RecoverKeyboardState(SafetyHookContext& context)
    {
        if (static_cast<HRESULT>(context.eax) != DIERR_NOTACQUIRED)
            return;
        auto keyboard = reinterpret_cast<uint8_t*>(context.esi);
        std::memset(keyboard + 9, 0, 256);
        ClearButtonStates(ReadMouse<uint8_t*>(keyboard + 0x109), 256);
        context.eip = KeyboardUpdateReturn;
    }

    // The game reads the mouse's buffered data only for its buttons, 32 events at a time from a 32 event buffer. A high
    // polling rate mouse fills that buffer with movement before each frame and button presses are dropped: the buffer
    // gets room for a few frames of an 8000 Hz mouse and the game gets only the button events.
    injector::scoped_basic<6> MouseDataCall;
    injector::scoped_write<1> EmptyButtonRetry;
    injector::scoped_write<1> ClearedButtonRetry;
    IDirectInputDeviceA* MouseDevice = nullptr;
    std::vector<DIDEVICEOBJECTDATA_DX3> ButtonEvents;
    std::atomic<bool> discardButtons{ false };
    bool bufferConfigured = false;
    ULONGLONG nextBufferAttempt = 0;
    constexpr DWORD MouseBufferSize = 4096;

    HRESULT WINAPI MouseGetDeviceData(IDirectInputDeviceA* device, DWORD size, DIDEVICEOBJECTDATA_DX3* data, DWORD* count, DWORD flags)
    {
        if (discardButtons.exchange(false))
            ButtonEvents.clear();
        for (;;)
        {
            DIDEVICEOBJECTDATA_DX3 events[256];
            DWORD read = DWORD(std::size(events));
            const HRESULT result = device->GetDeviceData(size, reinterpret_cast<DIDEVICEOBJECTDATA*>(events), &read, flags);
            if (FAILED(result))
            {
                ButtonEvents.clear();
                *count = 0;
                return result;
            }
            for (DWORD i = 0; i < read; ++i)
            {
                if (events[i].dwOfs >= DIMOFS_BUTTON0)
                    ButtonEvents.push_back(events[i]);
            }
            if (read < std::size(events))
                break;
        }
        if (discardButtons.load())
            ButtonEvents.clear();
        const DWORD given = std::min<DWORD>(*count, DWORD(ButtonEvents.size()));
        std::copy_n(ButtonEvents.begin(), given, data);
        ButtonEvents.erase(ButtonEvents.begin(), ButtonEvents.begin() + given);
        *count = given;
        return DI_OK;
    }

    void WatchMouseDevice(IDirectInputDeviceA* device)
    {
        if (device != MouseDevice)
        {
            MouseDevice = device;
            ButtonEvents.clear();
            bufferConfigured = false;
            nextBufferAttempt = 0;
        }
        if (!device || bufferConfigured)
            return;
        const auto now = GetTickCount64();
        if (now < nextBufferAttempt)
            return;
        nextBufferAttempt = now + 500;
        // The native update immediately following this one reacquires the mouse.
        if (FAILED(device->Unacquire()))
            return;
        ButtonEvents.clear();
        DIPROPDWORD buffer{ { sizeof(DIPROPDWORD), sizeof(DIPROPHEADER), 0, DIPH_DEVICE }, MouseBufferSize };
        bufferConfigured = SUCCEEDED(device->SetProperty(DIPROP_BUFFERSIZE, &buffer.diph));
    }

    void __fastcall UpdateMouse(uint8_t* self, void* edx, float delta)
    {
        WatchMouseDevice(ReadMouse<IDirectInputDeviceA*>(self + 5));
        if (MaxPayneOverlay::BlocksInput())
        {
            if (SuspendedMouse != self)
            {
                auto device = ReadMouse<IDirectInputDeviceA*>(self + 5);
                if (device)
                    device->Unacquire();
                ButtonEvents.clear();
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
        if (GetRegisteredRawInputDevices(nullptr, &count, sizeof(RAWINPUTDEVICE)) == UINT(-1))
            return false;
        std::vector<RAWINPUTDEVICE> devices(count);
        const auto found = count ? GetRegisteredRawInputDevices(devices.data(), &count, sizeof(RAWINPUTDEVICE)) : 0;
        if (found == UINT(-1))
            return false;
        mouse = {};
        for (UINT i = 0; i < found; ++i)
        {
            if (devices[i].usUsagePage == 1 && devices[i].usUsage == 2)
            {
                mouse = devices[i];
                break;
            }
        }
        return true;
    }

    void RestoreRegistration()
    {
        Registered = false;
        Reading.store(false);
        ClearMovement();
        if (!OwnRegistration)
            return;

        RAWINPUTDEVICE current{};
        if (!MouseRegistration(current))
            return;
        if (current.hwndTarget == InputWindow && current.dwFlags == RIDEV_INPUTSINK)
        {
            const auto device = HavePrevious ? Previous : RAWINPUTDEVICE{ 1, 2, RIDEV_REMOVE, nullptr };
            if (!RegisterRawInputDevices(&device, 1, sizeof(device)))
                return;
        }
        OwnRegistration = false;
    }

    LRESULT CALLBACK InputWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        if (message == WM_INPUT)
        {
            Movement.Read(reinterpret_cast<HRAWINPUT>(lParam), []()
            {
                const auto target = Window.load();
                return Reading.load() && Enabled.load() && target && GetForegroundWindow() == target;
            });
        }
        return DefWindowProcW(window, message, wParam, lParam);
    }

    LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR)
    {
        if (message == WM_KILLFOCUS || (message == WM_ACTIVATEAPP && !wParam))
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

export void DiscardMouseButtons()
{
    discardButtons.store(true);
}

export void UpdateRawMouseWindow(HWND window)
{
    if (!MouseHook)
        return;
    bool enabled = Enabled.load();
    const auto now = GetTickCount64();
    if (OwnRegistration && !Registered)
    {
        if (now < nextRegistrationCheck)
            return;
        nextRegistrationCheck = now + 500;
        RestoreRegistration();
        if (OwnRegistration)
            return;
    }
    if (CandidateWindow == window && WasEnabled == enabled)
    {
        if (!enabled || !window || (!Registered && now < nextRegistrationCheck))
            return;
        nextRegistrationCheck = now + 500;
        RAWINPUTDEVICE current{};
        if (Registered && !MouseRegistration(current))
            return;
        if (Registered && (current.hwndTarget != InputWindow || current.dwFlags != RIDEV_INPUTSINK))
        {
            // A later registration belongs to its caller; keep it and return to DirectInput.
            OwnRegistration = false;
            RestoreRegistration();
            if (Window.load())
                RemoveWindowSubclass(Window.load(), WindowProc, SubclassId);
            Window = nullptr;
            OutputDebugStringA("Max Payne 2: raw mouse ownership changed; using DirectInput.\n");
        }
        if (Registered)
            return;
    }
    nextRegistrationCheck = now + 500;
    CandidateWindow = window;
    WasEnabled = enabled;
    RestoreRegistration();
    if (Window.load())
        RemoveWindowSubclass(Window.load(), WindowProc, SubclassId);
    Window = nullptr;
    if (OwnRegistration || !enabled || !window || !IsWindow(window))
        return;
    if (!InputWindow)
        InputWindow = CreateRawInputWindow(L"MaxPayne2RawMouse", InputWindowProc);
    if (!InputWindow)
        return;

    RAWINPUTDEVICE current{};
    if (!MouseRegistration(current))
        return;
    HavePrevious = current.usUsage != 0;
    // Another plugin's explicit target keeps ownership of its input.
    if (HavePrevious && (current.hwndTarget || current.dwFlags))
    {
        return;
    }
    Previous = current;
    if (!SetWindowSubclass(window, WindowProc, SubclassId, 0))
        return;
    Window = window;
    RAWINPUTDEVICE mouse{ 1, 2, RIDEV_INPUTSINK, InputWindow };
    OwnRegistration = RegisterRawInputDevices(&mouse, 1, sizeof(mouse)) != FALSE;
    Registered = OwnRegistration;
    Reading.store(Registered);
    if (!Registered)
    {
        RemoveWindowSubclass(window, WindowProc, SubclassId);
        Window = nullptr;
        OutputDebugStringA("Max Payne 2: raw mouse registration failed; using DirectInput.\n");
    }
}

void InitRawMouse()
{
    MessagePumpGuard::Install(GetModuleHandleW(nullptr));
    auto module = GetModuleHandleW(L"X_Inputmfc.dll");
    auto readButtons = hook::module_pattern(module, "89 5C 24 20 8B 10 50 FF 52 28 33 F6 85 C0 8B 47 2D").get_first<uint8_t>(4);
    MouseDataCall.save(readButtons, 6, true);
    injector::WriteMemory<uint8_t>(readButtons, 0x50, true); // push eax (device)
    injector::MakeCALL(readButtons + 1, MouseGetDeviceData, true);
    // Finish the frame after the native button reset if the device stays unacquired.
    auto emptyRetry = hook::module_pattern(module, "85 C0 7E ? 33 ED 8D 9B 00 00 00 00 85 ED").get_first<uint8_t>(3);
    auto clearedRetry = hook::module_pattern(module, "83 C5 0F 3B F0 7C ? EB ? 85 C0").get_first<uint8_t>(8);
    auto eventCount = hook::module_pattern(module, "8B 44 24 10 33 DB 85 C0 76 ? BE").get_first<uint8_t>();
    EmptyButtonRetry.write(emptyRetry, static_cast<int8_t>(eventCount - (emptyRetry + 1)), true);
    ClearedButtonRetry.write(clearedRetry, static_cast<int8_t>(eventCount - (clearedRetry + 1)), true);
    auto update = hook::module_pattern(module, "64 A1 00 00 00 00 6A FF 68 ? ? ? ? 50 8B 44 24 10 64 89 25 00 00 00 00 83 EC 1C 57 8B F9 89 47 4D 8B 47 05");
    MouseUpdateHook = safetyhook::create_inline(update.get_first(), UpdateMouse);
    // Relative motion still comes from the raw worker when DirectInput cannot acquire the mouse.
    auto acquire = hook::module_pattern(module, "89 47 4D 8B 47 05 8B 08 50 FF 51 1C 85 C0 7D ? 68");
    MouseAcquireHook = safetyhook::create_mid(acquire.get_first(12), [](SafetyHookContext& context)
    {
        if (FAILED(static_cast<HRESULT>(context.eax)) && RawMouseActive())
            context.eax = DI_OK;
    });
    auto state = hook::module_pattern(module, "55 8D 6F 09 55 6A 14 50 FF 52 24 85 C0 74 ?");
    MouseStateHook = safetyhook::create_mid(state.get_first(11), [](SafetyHookContext& context)
    {
        if (context.eax != DI_OK && RawMouseActive())
        {
            WriteMouse(reinterpret_cast<uint8_t*>(context.edi) + 9, DIMOUSESTATE2{});
            context.eax = DI_OK;
        }
    });
    auto pattern = hook::module_pattern(module, "D9 47 53 8B 4F 5F D8 0D ? ? ? ? 8B 97 87 00 00 00 8B 47 5B");
    MouseHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& context)
    {
        if (MaxPayneOverlay::BlocksInput())
        {
            ClearMovement();
            return;
        }
        auto mouse = reinterpret_cast<uint8_t*>(context.edi);
        MaxPayneOverlay::Mouse(mouse + 9);
        if (!RawMouseActive())
            return;
        const auto movement = Movement.Take();
        if (!movement.relative)
            return;
        constexpr int64_t minimum = std::numeric_limits<LONG>::min();
        constexpr int64_t maximum = std::numeric_limits<LONG>::max();
        WriteMouse(mouse + 9, static_cast<LONG>(std::clamp(movement.x, minimum, maximum)));
        WriteMouse(mouse + 13, static_cast<LONG>(std::clamp(movement.y, minimum, maximum)));
    });
    Enabled.store(FixSettings().Bool("MAIN.RawInputMouse"));
    FixSettings().OnChange("MAIN.RawInputMouse", []() { Enabled.store(FixSettings().Bool("MAIN.RawInputMouse")); ClearMovement(); });
}

void ShutdownRawMouse()
{
    Enabled.store(false);
    RestoreRegistration();
    if (Window.load())
        RemoveWindowSubclass(Window.load(), WindowProc, SubclassId);
    Window = nullptr;
    CandidateWindow = nullptr;
    MouseHook.reset();
    MouseStateHook.reset();
    MouseAcquireHook.reset();
    MouseUpdateHook.reset();
    KeyboardRetryHook.reset();
    KeyboardStateHook.reset();
    ClearedButtonRetry.restore();
    EmptyButtonRetry.restore();
    MouseDataCall.restore();
    MouseDevice = nullptr;
    ButtonEvents.clear();
    SuspendedMouse = nullptr;
}

export void InitInput()
{
    InitRawMouse();
    // Focus can change between Acquire and GetDeviceState. Retry on the next frame with no held keys.
    const auto input = GetModuleHandleW(L"X_Inputmfc.dll");
    KeyboardUpdateReturn = reinterpret_cast<uintptr_t>(hook::module_pattern(input,
        "5B 5F 5D 8B 4C 24 14 5E 64 89 0D 00 00 00 00 83 C4 1C C2 04 00").get_first(1));
    KeyboardStateHook = safetyhook::create_mid(hook::module_pattern(input,
        "55 68 00 01 00 00 50 FF 52 24 85 C0 0F 84 ? ? ? ? 3D 1E 00 07 80").get_first(10), RecoverKeyboardState);
    KeyboardRetryHook = safetyhook::create_mid(hook::module_pattern(input,
        "55 68 00 01 00 00 50 FF 52 24 8B F8 85 FF 74 ? 8B CE E8").get_first(10), RecoverKeyboardState);
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
        MaxPaynePad::Enable(true);
    });

    pattern = hook::module_pattern(GetModuleHandle(L"X_Inputmfc"), "89 1D ? ? ? ? 88 5E ? ? ? 89 5E");
    static auto X_InputdestructHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        MaxPaynePad::Enable(false);
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
