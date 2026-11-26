module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "DeveloperOverlay.hpp"
#include "Shared/MaxPayne/GamepadButtons.hpp"
#include "Shared/MaxPayne/NativeIcons.hpp"
#include "Shared/MaxPayne/Pad.hpp"
#include "Shared/MaxPayne/Vibration.hpp"
#include <stdexcept>

export module input;

import ComVars;
import xidi;

void UpdateInput()
{
    MaxPaynePad::Update();
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
    return MaxPayne_GameMode::IsSniperZooming() ? X_InputDeviceMouse::GetZoomSensitivity() : 1.0f;
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
        if (MaxPayneOverlay::BlocksInput())
            return 0.0f;
        auto ret = shgetFloat.unsafe_fastcall<float>(_this, edx);
        if (ret != 0.0f)
            return ret;

        uint32_t inputType = *(uint32_t*)((uintptr_t)_this + 0x24);
        switch (inputType)
        {
            case 0: // Movement axes
            {
                auto stick = MaxPaynePad::LeftStick();
                if (_this == MaxPayne_ConfiguredInput::LeftRight)
                    ret = -stick.x;
                else if (_this == MaxPayne_ConfiguredInput::ForwardBackward)
                    ret = -stick.y;
                break;
            }
            case 1: // Right stick X
            {
                ret = MaxPaynePad::RightStick().x * FixSettings().Float("MAIN.GamepadCameraSpeed") * GetAimZoomSensitivity();
                break;
            }
            case 2: // Right stick Y
            {
                float stickY = MaxPaynePad::RightStick().y * FixSettings().Float("MAIN.GamepadCameraSpeed") * GetAimZoomSensitivity();
                if (_this == MaxPayne_ConfiguredInput::AimUpDown)
                {
                    auto Mouse = (uintptr_t)X_Input::getMouse();
                    bool bInvertY = (*(uint8_t*)(Mouse + 81)) != 0;
                    if (bInvertY)
                        stickY = -stickY;
                }
                if (FixSettings().Bool("MAIN.InvertGamepadY"))
                    stickY = -stickY;
                ret = stickY;
                break;
            }
            case 3: // D-pad weapon scrolling
            {
                if (MaxPaynePad::JustPressed(g_ControlScheme.PreviousWeapon))
                    ret = 1.0f;
                else if (MaxPaynePad::JustPressed(g_ControlScheme.NextWeapon))
                    ret = -1.0f;
                break;
            }
            default:
                break;
        }
        return ret;
    }

    SafetyHookInline shgetFloatWithSingleClicks = {};
    float __fastcall getFloatWithSingleClicks(void* _this, void* edx)
    {
        if (MaxPayneOverlay::BlocksInput())
            return 0.0f;
        auto ret = shgetFloatWithSingleClicks.unsafe_fastcall<float>(_this, edx);
        if (ret != 0.0f)
            return ret;

        uint32_t inputType = *(uint32_t*)((uintptr_t)_this + 0x24);
        if (inputType == 3) // D-pad weapon scrolling
        {
            if (MaxPaynePad::JustPressed(g_ControlScheme.PreviousWeapon))
                ret = 1.0f;
            else if (MaxPaynePad::JustPressed(g_ControlScheme.NextWeapon))
                ret = -1.0f;
        }
        return ret;
    }

    SafetyHookInline shgetButton = {};
    char __fastcall getButton(void* _this, void* edx, int a2)
    {
        if (MaxPayneOverlay::BlocksInput())
            return 0;
        auto ret = shgetButton.unsafe_fastcall<char>(_this, edx, a2);
        if (ret == 0)
            ret = MaxPaynePad::Pressed(GetButtonForInput(_this)) ? 1 : 0;
        return ret;
    }

    SafetyHookInline shgetButtonSingleClicked = {};
    char __fastcall getButtonSingleClicked(void* _this, void* edx, int a2)
    {
        if (MaxPayneOverlay::BlocksInput())
            return 0;
        auto ret = shgetButtonSingleClicked.unsafe_fastcall<char>(_this, edx, a2);
        if (ret == 0)
            ret = MaxPaynePad::JustPressed(GetButtonForInput(_this)) ? 1 : 0;
        return ret;
    }

    SafetyHookInline shgetButtonReleased = {};
    char __fastcall getButtonReleased(void* _this, void* edx, int a2)
    {
        if (MaxPayneOverlay::BlocksInput())
            return 0;
        auto ret = shgetButtonReleased.unsafe_fastcall<char>(_this, edx, a2);
        if (ret == 0)
            ret = MaxPaynePad::JustReleased(GetButtonForInput(_this)) ? 1 : 0;
        return ret;
    }

    // Tips name the key of an action by a token, "Press #USE# to ...", replaced with the key's name (the HUD message
    // code's substitution, for every action: this std::string in ecx, the token in edx, the name, a std::string, on
    // the stack). While a controller is in use, the tokens of the actions the fix maps get the controller's button.
    struct GameString
    {
        uint32_t allocator;
        const char* pText;
        uint32_t nLength;
        uint32_t nReserved;
    };

    const char* GetPadName(std::string_view token)
    {
        static const std::pair<std::string_view, JoystickButton ControlScheme::*> Buttons[] =
        {
            { "#SHOOT#", &ControlScheme::Shoot }, { "#RELOAD#", &ControlScheme::Reload }, { "#JUMP#", &ControlScheme::Jump },
            { "#CROUCH#", &ControlScheme::Crouch }, { "#DODGEMODIFIER#", &ControlScheme::DodgeModifier }, { "#USE#", &ControlScheme::Use },
            { "#NEXTWEAPON#", &ControlScheme::NextWeapon }, { "#PREVIOUSWEAPON#", &ControlScheme::PreviousWeapon },
            { "#PAUSE#", &ControlScheme::Pause }, { "#PAINKILLER#", &ControlScheme::Painkiller }, { "#SNIPERZOOM#", &ControlScheme::SniperZoom },
            { "#SLOWMOTION#", &ControlScheme::SlowMotion }, { "#BULLETTIME#", &ControlScheme::BulletTime },
        };

        static auto& nGamepadIcons = FixSettings().Int("MAIN.GamepadIcons");
        if (token == "#RUN#" || token == "#WALKBACK#" || token == "#STRAFELEFT#" || token == "#STRAFERIGHT#")
            return MaxPayneIcons::Glyph(19);
        if (token == "#AIMUP#" || token == "#AIMDOWN#" || token == "#AIMLEFT#" || token == "#AIMRIGHT#")
            return MaxPayneIcons::Glyph(24);
        int family = nGamepadIcons == MaxPayneGamepadButtons::Auto ? MaxPayneGamepadButtons::DetectFamily() : nGamepadIcons;
        for (auto& [name, button] : Buttons)
        {
            if (token == name)
            {
                if (auto icon = MaxPayneIcons::Button(g_ControlScheme.*button))
                    return icon;
                return MaxPayneGamepadButtons::Name(g_ControlScheme.*button, family);
            }
        }
        return nullptr;
    }

    SafetyHookInline shReplaceToken = {};
    char* __fastcall ReplaceToken(void* pText, const char* szToken, GameString* pName)
    {
        GameString padName = {};
        auto string = static_cast<GameString*>(pText);
        auto alternatives = MaxPayneIcons::Alternatives(string->pText ? string->pText : "");
        std::string keyboardName = pName && pName->pText ? pName->pText : "";
        auto szPadName = szToken ? GetPadName(szToken) : nullptr;
        if (szPadName)
        {
            padName = { 0, szPadName, uint32_t(std::strlen(szPadName)), uint32_t(std::strlen(szPadName)) };
            if (MaxPayneGamepadButtons::IsPadInUse())
                pName = &padName;
        }
        auto result = shReplaceToken.unsafe_fastcall<char*>(pText, szToken, pName);
        if (szToken && szPadName)
        {
            MaxPayneIcons::Replace(alternatives.keyboard, szToken, keyboardName);
            MaxPayneIcons::Replace(alternatives.pad, szToken, szPadName);
            MaxPayneIcons::RegisterPrompt(string->pText ? string->pText : "", std::move(alternatives));
        }
        return result;
    }
}

export std::string GetControlIcon(int id)
{
    using namespace MaxPayneIcons;
    using namespace MaxPayne_InputControl;
    auto button =[](JoystickButton value)
    {
        auto icon = Button(value);
        return icon ? std::string(icon) : std::string();
    };
    switch (id)
    {
        case 2: return Glyph(23);
        case 3: return Glyph(22);
        case 4: case 5: return Glyph(21);
        case 6: return button(g_ControlScheme.Crouch);
        case 7: return button(g_ControlScheme.Jump);
        case 8: case 9: case 10: case 11: return std::string(Glyph(6)) + " + " + Glyph(19);
        case 12: return Glyph(6);
        case 13: case 14: return Glyph(26);
        case 15: return button(g_ControlScheme.Shoot);
        case 16: return button(g_ControlScheme.Reload);
        case 17: return button(g_ControlScheme.NextWeapon);
        case 18: return button(g_ControlScheme.PreviousWeapon);
        case 19: return std::string(Glyph(14)) + " / " + Glyph(15);
        case 40: return button(g_ControlScheme.Use);
        case 41: return button(g_ControlScheme.SlowMotion);
        case 42: return button(g_ControlScheme.Pause);
        case 43: return button(g_ControlScheme.Painkiller);
        case 44: return button(g_ControlScheme.SniperZoom);
        case 45: return button(g_ControlScheme.BulletTime);
        default: return id >= 20 && id <= 30 ? std::string(Glyph(14)) + " / " + Glyph(15) : "";
    }
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

void InitInput()
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
        MaxPaynePad::Enable(true);
    });

    pattern = hook::pattern("C7 05 ? ? ? ? ? ? ? ? 8B CF");
    static auto X_InputdestructHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        MaxPaynePad::Enable(false);
    });

    pattern = hook::pattern("8B 0D ? ? ? ? 85 C9 74 ? 8B 01 56 FF 50 ? 5E");
    static auto X_InputupdateHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        UpdateInput();
    });

    auto floatAccessors = hook::pattern("56 8B F1 6A 00 E8 ? ? ? ? 84 C0 74 ? D9 05 ? ? ? ? 5E C3");
    auto buttonAccessors = hook::pattern("56 8B F1 57 8B 7C 24 ? 8D 04 FE 8B CF 03 C8 E8 ? ? ? ? 84 C0");
    if (floatAccessors.size() != 2 || buttonAccessors.size() != 3)
        throw std::runtime_error("Input-control signatures not found");
    MaxPayne_InputControl::shgetFloat = safetyhook::create_inline(floatAccessors.get(0).get<void*>(0), MaxPayne_InputControl::getFloat);
    MaxPayne_InputControl::shgetFloatWithSingleClicks = safetyhook::create_inline(floatAccessors.get(1).get<void*>(0), MaxPayne_InputControl::getFloatWithSingleClicks);
    MaxPayne_InputControl::shgetButton = safetyhook::create_inline(buttonAccessors.get(0).get<void*>(0), MaxPayne_InputControl::getButton);
    MaxPayne_InputControl::shgetButtonSingleClicked = safetyhook::create_inline(buttonAccessors.get(1).get<void*>(0), MaxPayne_InputControl::getButtonSingleClicked);
    MaxPayne_InputControl::shgetButtonReleased = safetyhook::create_inline(buttonAccessors.get(2).get<void*>(0), MaxPayne_InputControl::getButtonReleased);

    // Tips with controller button names, see MaxPayne_InputControl::ReplaceToken
    pattern = hook::pattern("83 EC 0C 53 55 8B E9 56 57 83 C9 FF 33 C0 8B FA F2 AE");
    MaxPayne_InputControl::shReplaceToken = safetyhook::create_inline(pattern.get_first(), MaxPayne_InputControl::ReplaceToken); //0x4CD1A0

    pattern = hook::pattern("FF 15 ? ? ? ? 33 DB 8D 4C 24");
    static auto ApplicationTickHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        UpdateVibration();
    });

    // Sniper scope enter
    pattern = hook::pattern("C7 83 ? ? ? ? ? ? ? ? E9 ? ? ? ? 84 C0");
    static auto MaxPayne_GameModeupdateHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        Vibrate(14, std::chrono::milliseconds{ 100 });
    });

    // Painkiller use
    pattern = hook::pattern("E8 ? ? ? ? 8B CB E8 ? ? ? ? ? ? 8B C8 FF 52");
    static auto X_GlobalPainkillerSettingssendMessagesHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        Vibrate(23, std::chrono::milliseconds{ 200 });
    });

    // Damage taken, X_Character::setHealth runs for every character: esi is the character
    pattern = hook::pattern("E8 ? ? ? ? E9 ? ? ? ? ? ? ? ? ? ? ? ? ? ? DF E0");
    static auto X_CharactersetHealthHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        if ((uint8_t*)regs.esi == MaxPayne_GameMode::GetPlayerCharacter())
            Vibrate(46, std::chrono::milliseconds{ 180 });
    });

    // Shooting, the X_PlayerInputEvaluator states also drive the enemies (MaxPayne_ArtificialPlayerInput):
    // esi is the state, its X_CharacterProperties are at +0x10. The states shoot whatever is in hand,
    // only guns vibrate.
    static auto IsPlayerHoldingGun = [](SafetyHookContext& regs)
    {
        auto pCharacterProperties = *(uint8_t**)(regs.esi + 0x10);
        return X_Character::IsPlayerCharacterProperties(pCharacterProperties) && X_CharacterProperties::IsHoldingGun(pCharacterProperties);
    };

    pattern = hook::pattern("E8 ? ? ? ? 8B 44 24 ? 50 8B CE E8 ? ? ? ? 5E");
    static auto X_CharacterPropertiessetIsShooting1 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        if (IsPlayerHoldingGun(regs))
            Vibrate(80, std::chrono::milliseconds{ 80 });
    });

    pattern = hook::pattern("6A ? 8B CE E8 ? ? ? ? 8B C8 E8 ? ? ? ? 5E 5B C2");
    static auto X_CharacterPropertiessetIsShooting2 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        if (IsPlayerHoldingGun(regs))
            Vibrate(80, std::chrono::milliseconds{ 180 });
    });

    pattern = hook::pattern("E8 ? ? ? ? 53 8B CE E8 ? ? ? ? 5E");
    static auto X_CharacterPropertiessetIsShooting3 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        if (IsPlayerHoldingGun(regs))
            Vibrate(80, std::chrono::milliseconds{ 80 });
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
            Vibrate(100, std::chrono::milliseconds{ 500 });
        CameraPathName.clear();
    });
}

class Input
{
public:
    Input()
    {
        WFP::onInitEvent() += []()
        {
            InitInput();
        };
    }
} Input;
