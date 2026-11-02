module;

#include <stdafx.h>

#include "common.h"
#include "FixSettings.hpp"
#include "Shared/PadInput.hpp"

export module Pad;

import Skeleton;
import Menu;
import Timer;
import CutsceneMgr;
import Camera;
import Entity;

// XInput pads. The pad's buttons become the game's own controller state (the PlayStation 2 layout the game's pad code
// reads), so every pad feature of the game works as on the console: the camera, lock-on targeting, the menus. Two
// layouts:
// - Classic: the buttons by position (A = Cross, B = Circle, ...), the console game's controls.
// - Modern (MAIN.ModernControls): the default pad layout of the Definitive Edition. On foot: LT aims (the PC's
//   over-the-shoulder free aim that moves and shoots at once, driven by the right stick; sniper rifle, camera and
//   rocket launchers use their scope, the left stick zooms), RT fires, B attacks, A sprints, X jumps, Y enters, LB
//   takes the next weapon (the PC game has no weapon wheel), RB is the action button (pickups, the phone), L3 crouches,
//   R3 looks behind, d-pad left shows the stats. In a vehicle: RT accelerates, LT brakes, A is the nitro (and the
//   vehicle's weapon), RB the handbrake, X/B look left/right, R3 looks behind, L3 is the horn, d-pad up starts a side
//   mission (and the hydraulics), d-pad right takes the next radio station (no radio wheel either; d-pad down the
//   previous one), d-pad left is the second weapon. In aircraft LB/RB are the rudder.
// The last device used decides the prompts' icons and, for the pad, the console's camera (the mouse camera of the
// PC's standard controls returns with the mouse). Start opens and closes the pause menu, as Escape does. The pad's
// state replaces what the game read from the same pad through DirectInput; other DirectInput pads keep working.
// With a controller mod loaded that does all of this its own way, none of it is installed.

export struct CControllerState
{
    int16_t LeftStickX, LeftStickY, RightStickX, RightStickY;
    int16_t LeftShoulder1, LeftShoulder2, RightShoulder1, RightShoulder2;
    int16_t DPadUp, DPadDown, DPadLeft, DPadRight;
    int16_t Start, Select, Square, Triangle, Cross, Circle;
    int16_t LeftShock, RightShock;
    int16_t ChatIndicated, PedWalk, VehicleMouseLook, RadioTrackSkip;
};
static_assert(sizeof(CControllerState) == 0x30);

export struct CPad
{
    CControllerState NewState;
    CControllerState OldState;
    int16_t SteeringLeftRightBuffer[10];
    int32_t DrunkDrivingBufferUsed;
    CControllerState PCTempKeyState;
    CControllerState PCTempJoyState;
    CControllerState PCTempMouseState;
    uint8_t Phase;
    int16_t Mode;
    int16_t ShakeDur;
    uint16_t DisablePlayerControls;
    uint8_t ShakeFreq;
};
static_assert(offsetof(CPad, PCTempKeyState) == 0x78);
static_assert(offsetof(CPad, PCTempJoyState) == 0xA8);
static_assert(offsetof(CPad, PCTempMouseState) == 0xD8);
static_assert(offsetof(CPad, Mode) == 0x10A);
static_assert(offsetof(CPad, ShakeDur) == 0x10C);
static_assert(offsetof(CPad, DisablePlayerControls) == 0x10E);
static_assert(offsetof(CPad, ShakeFreq) == 0x110);

struct CMouseControllerState
{
    bool LMB, RMB, MMB, WheelUp, WheelDown, MXB1, MXB2;
    float Z, X, Y;
};
static_assert(offsetof(CMouseControllerState, X) == 0xC);

export enum class IconStyle : int
{
    Xbox = 1,
    PlayStation = 2,
    Nintendo = 3,
};

namespace
{
    // The camera modes that look through a scope (the left stick zooms there in the modern layout)
    bool IsScopeMode(int16_t mode)
    {
        switch (mode)
        {
            case 7:     // MODE_SNIPER
            case 8:     // MODE_ROCKETLAUNCHER
            case 34:    // MODE_M16_1STPERSON
            case 39:    // MODE_SNIPER_RUNABOUT
            case 40:    // MODE_ROCKETLAUNCHER_RUNABOUT
            case 41:    // MODE_1STPERSON_RUNABOUT
            case 42:    // MODE_M16_1STPERSON_RUNABOUT
            case 45:    // MODE_HELICANNON_1STPERSON
            case 46:    // MODE_CAMERA
            case 51:    // MODE_ROCKETLAUNCHER_HS
            case 52:    // MODE_ROCKETLAUNCHER_RUNABOUT_HS
                return true;
            default:
                return false;
        }
    }

    // What a pad button becomes in the game's controller state
    enum class To : uint8_t
    {
        None, L1, L2, R1, R2, DPadUp, DPadDown, DPadLeft, DPadRight, Start, Select, Square, Triangle, Cross, Circle,
        L3, R3, LookBehind, ActionLeft
    };

    enum Src { A, B, X, Y, LB, RB, LT, RT, LS, RS, Back, Start, DUp, DDown, DLeft, DRight, SrcCount };

    // Start is Escape (handled by itself)
    constexpr To LayoutClassic[SrcCount] = { To::Cross, To::Circle, To::Square, To::Triangle, To::L1, To::R1, To::L2, To::R2,
        To::L3, To::R3, To::Select, To::None, To::DPadUp, To::DPadDown, To::DPadLeft, To::DPadRight };
    // B goes back in the menus (the menus' back button is the Triangle)
    constexpr To LayoutFrontend[SrcCount] = { To::Cross, To::Triangle, To::None, To::None, To::L1, To::R1, To::L2, To::R2,
        To::L3, To::R3, To::Select, To::None, To::DPadUp, To::DPadDown, To::DPadLeft, To::DPadRight };
    // LT is the aim (handled by itself)
    constexpr To LayoutModernFoot[SrcCount] = { To::Cross, To::Circle, To::Square, To::Triangle, To::R2, To::L1, To::None, To::Circle,
        To::L3, To::R3, To::Select, To::None, To::DPadUp, To::DPadDown, To::ActionLeft, To::DPadRight };
    // LT and RT are analog (brake, accelerate); the radio's next station is the console's d-pad up
    constexpr To LayoutModernVehicle[SrcCount] = { To::Circle, To::R2, To::L2, To::Triangle, To::None, To::R1, To::None, To::None,
        To::L3, To::LookBehind, To::Select, To::None, To::R3, To::DPadDown, To::L1, To::DPadUp };
    constexpr To LayoutModernAircraft[SrcCount] = { To::Circle, To::None, To::None, To::Triangle, To::L2, To::R2, To::None, To::None,
        To::L3, To::None, To::Select, To::None, To::R3, To::DPadDown, To::L1, To::DPadUp };

    const auto& XInput() { return PadInput::XInput(); }

    struct Slot
    {
        bool connected = false;
        ULONGLONG nextProbe = 0;
        XINPUT_GAMEPAD previous{};
        IconStyle style = IconStyle::Xbox;
        WORD motors[2] = {};
    } Slots[XUSER_MAX_COUNT];

    int ActiveSlot = -1;                // the pad last used, -1 = none connected
    bool PadIsLastDevice = false;
    bool Installed = false;
    bool GInputLoaded = false;
    bool ModernAiming = false;
    ULONGLONG LastShakeTick = 0;
    XINPUT_GAMEPAD Current{};

    GameRef<bool> UseMouse3rdPerson;                    // CCamera::m_bUseMouse3rdPerson
    GameRef<CMouseControllerState> NewMouseState;       // CPad::NewMouseControllerState
    GameRef<float> MouseAccelHorizontal;                // CCamera::m_fMouseAccelHorzntl
    GameRef<float> MouseAccelVertical;                  // CCamera::m_fMouseAccelVertical
    GameRef<int16_t> TempEscape;                        // CPad::TempKeyState.esc

    bool MenuActive()
    {
        return FrontendMenuManager->m_bMenuActive;
    }

    IconStyle DetectStyle(DWORD)
    {
        return IconStyle::Xbox;
    }

    // -1..1 with a radial dead zone, the rest scaled to the full range
    void Stick(SHORT sx, SHORT sy, SHORT deadzone, float& x, float& y)
    {
        float fx = sx / 32767.0f, fy = sy / 32767.0f;
        float length = std::sqrt(fx * fx + fy * fy);
        float dz = deadzone / 32767.0f;
        if (length <= dz)
        {
            x = y = 0.0f;
            return;
        }
        float scaled = std::min((length - dz) / (1.0f - dz), 1.0f);
        x = fx / length * scaled;
        y = fy / length * scaled;
    }

    int16_t ToAxis(float v)
    {
        return int16_t(std::clamp(int(std::lround(v * 128.0f)), -128, 127));
    }

    bool HasInput(const XINPUT_GAMEPAD& now, const XINPUT_GAMEPAD& before)
    {
        if (now.wButtons != before.wButtons)
            return true;
        if (now.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD || now.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
            return true;
        float x, y;
        Stick(now.sThumbLX, now.sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE, x, y);
        if (x || y)
            return true;
        Stick(now.sThumbRX, now.sThumbRY, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE, x, y);
        return x || y;
    }

    // Reads every pad; returns whether the active one was used this frame
    bool PollPads()
    {
        auto now = GetTickCount64();
        bool used = false;
        for (DWORD i = 0; i < XUSER_MAX_COUNT; ++i)
        {
            auto& slot = Slots[i];
            // A missing pad is slow to ask for: once a second
            if (!slot.connected && now < slot.nextProbe)
                continue;
            XINPUT_STATE state{};
            if (XInput().getState(i, &state) != ERROR_SUCCESS)
            {
                if (slot.connected && ActiveSlot == int(i))
                    ActiveSlot = -1;
                slot.connected = false;
                slot.nextProbe = now + 1000;
                continue;
            }
            if (!slot.connected)
            {
                slot.connected = true;
                slot.previous = state.Gamepad;
                slot.style = DetectStyle(i);
                slot.motors[0] = slot.motors[1] = 0;
                if (ActiveSlot < 0)
                    ActiveSlot = int(i);
            }
            if (HasInput(state.Gamepad, slot.previous))
            {
                ActiveSlot = int(i);
                used = true;
            }
            slot.previous = state.Gamepad;
            if (ActiveSlot == int(i))
                Current = state.Gamepad;
        }
        if (ActiveSlot < 0)
            Current = {};
        return used;
    }

    bool KeyboardOrMouseUsed(const CPad& pad)
    {
        auto any = [](const CControllerState& s)
        {
            auto p = reinterpret_cast<const int16_t*>(&s);
            for (size_t i = 0; i < sizeof(s) / sizeof(int16_t); ++i)
                if (p[i])
                    return true;
            return false;
        };
        if (any(pad.PCTempKeyState) || any(pad.PCTempMouseState))
            return true;
        if (NewMouseState.is_initialized())
        {
            const auto& mouse = *NewMouseState;
            if (std::abs(mouse.X) + std::abs(mouse.Y) > 2.0f || mouse.LMB || mouse.RMB || mouse.MMB || mouse.WheelUp || mouse.WheelDown)
                return true;
        }
        return false;
    }

    CEntity* PlayerVehicle()
    {
        return FindPlayerVehicle ? FindPlayerVehicle(-1, false) : nullptr;
    }

    int16_t ActiveCameraMode()
    {
        return TheCamera->m_aCams[TheCamera->m_nActiveCam].Mode;
    }

    void Press(CControllerState& s, To to, int16_t value = 255)
    {
        auto set = [value](int16_t& field) { field = std::max(field, value); };
        switch (to)
        {
            case To::L1: set(s.LeftShoulder1); break;
            case To::L2: set(s.LeftShoulder2); break;
            case To::R1: set(s.RightShoulder1); break;
            case To::R2: set(s.RightShoulder2); break;
            case To::DPadUp: set(s.DPadUp); break;
            case To::DPadDown: set(s.DPadDown); break;
            case To::DPadLeft: set(s.DPadLeft); break;
            case To::DPadRight: set(s.DPadRight); break;
            case To::Start: set(s.Start); break;
            case To::Select: set(s.Select); break;
            case To::Square: set(s.Square); break;
            case To::Triangle: set(s.Triangle); break;
            case To::Cross: set(s.Cross); break;
            case To::Circle: set(s.Circle); break;
            case To::L3: set(s.LeftShock); break;
            case To::R3: set(s.RightShock); break;
            case To::LookBehind: set(s.LeftShoulder2); set(s.RightShoulder2); break;
            case To::ActionLeft: set(s.DPadLeft); set(s.LeftShoulder1); break;
            default: break;
        }
    }

    bool Down(const XINPUT_GAMEPAD& g, Src src)
    {
        switch (src)
        {
            case A: return g.wButtons & XINPUT_GAMEPAD_A;
            case B: return g.wButtons & XINPUT_GAMEPAD_B;
            case X: return g.wButtons & XINPUT_GAMEPAD_X;
            case Y: return g.wButtons & XINPUT_GAMEPAD_Y;
            case LB: return g.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER;
            case RB: return g.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER;
            case LT: return g.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
            case RT: return g.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
            case LS: return g.wButtons & XINPUT_GAMEPAD_LEFT_THUMB;
            case RS: return g.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB;
            case Back: return g.wButtons & XINPUT_GAMEPAD_BACK;
            case Start: return g.wButtons & XINPUT_GAMEPAD_START;
            case DUp: return g.wButtons & XINPUT_GAMEPAD_DPAD_UP;
            case DDown: return g.wButtons & XINPUT_GAMEPAD_DPAD_DOWN;
            case DLeft: return g.wButtons & XINPUT_GAMEPAD_DPAD_LEFT;
            case DRight: return g.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT;
            default: return false;
        }
    }

    // A trigger's pull past its threshold as 0..255
    int16_t Trigger(BYTE value)
    {
        if (value <= XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
            return 0;
        return int16_t((value - XINPUT_GAMEPAD_TRIGGER_THRESHOLD) * 255 / (255 - XINPUT_GAMEPAD_TRIGGER_THRESHOLD));
    }

    enum class Context { Frontend, Classic, ModernFoot, ModernScope, ModernVehicle, ModernAircraft };

    Context CurrentContext()
    {
        if (MenuActive())
            return Context::Frontend;
        static auto& modern = FixSettings().Int("MAIN.ModernControls");
        if (!modern)
            return Context::Classic;
        if (auto vehicle = PlayerVehicle())
        {
            // CVehicle's subclass: 3 helicopter, 4 plane
            auto subclass = *reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(vehicle) + 0x594);
            return subclass == 3 || subclass == 4 ? Context::ModernAircraft : Context::ModernVehicle;
        }
        if (IsScopeMode(ActiveCameraMode()))
            return Context::ModernScope;
        return Context::ModernFoot;
    }

    CControllerState Compose(Context context, const XINPUT_GAMEPAD& g)
    {
        CControllerState s{};
        float lx, ly, rx, ry;
        Stick(g.sThumbLX, g.sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE, lx, ly);
        Stick(g.sThumbRX, g.sThumbRY, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE, rx, ry);
        static auto& invert = FixSettings().Int("MAIN.InvertGamepadY");
        static auto& speed = FixSettings().Float("MAIN.GamepadCameraSpeed");
        // The PlayStation 2 sticks: down is positive. The camera's stick reaches the game's full turn sooner or later
        // by the camera speed.
        float look = context == Context::Frontend ? 1.0f : speed;
        s.LeftStickX = ToAxis(lx);
        s.LeftStickY = ToAxis(-ly);
        s.RightStickX = ToAxis(rx * look);
        s.RightStickY = ToAxis((invert && context != Context::Frontend ? ry : -ry) * look);

        const To* layout = LayoutClassic;
        switch (context)
        {
            case Context::Frontend: layout = LayoutFrontend; break;
            case Context::ModernFoot: layout = LayoutModernFoot; break;
            case Context::ModernScope: layout = LayoutModernFoot; break;
            case Context::ModernVehicle: layout = LayoutModernVehicle; break;
            case Context::ModernAircraft: layout = LayoutModernAircraft; break;
            default: break;
        }
        for (int i = 0; i < SrcCount; ++i)
        {
            if (Down(g, Src(i)))
                Press(s, layout[i]);
        }

        // The modern aim turns the camera as the mouse does
        if (ModernAiming)
            s.RightStickX = s.RightStickY = 0;

        switch (context)
        {
            case Context::Classic:
            case Context::Frontend:
                break;
            case Context::ModernVehicle:
            case Context::ModernAircraft:
                s.Cross = std::max(s.Cross, Trigger(g.bRightTrigger));
                s.Square = std::max(s.Square, Trigger(g.bLeftTrigger));
                break;
            case Context::ModernScope:
                // The left stick zooms
                s.LeftStickX = s.LeftStickY = 0;
                if (ly > 0.3f)
                    Press(s, To::Square);
                else if (ly < -0.3f)
                    Press(s, To::Cross);
                [[fallthrough]];
            case Context::ModernFoot:
                if (Down(g, LT))
                    Press(s, To::R1);
                break;
        }
        return s;
    }

    void UpdateVibration(CPad& pad)
    {
        auto now = GetTickCount64();
        auto elapsed = LastShakeTick ? now - LastShakeTick : 0;
        LastShakeTick = now;
        if (ActiveSlot < 0 || !XInput().setState)
            return;

        // The PC game sets the shake but never runs it down
        WORD motor = 0;
        if (pad.ShakeDur > 0)
        {
            if (PadIsLastDevice && !MenuActive())
                motor = WORD(pad.ShakeFreq * 257);
            pad.ShakeDur = int16_t(std::max<int64_t>(0, pad.ShakeDur - int64_t(elapsed)));
            if (pad.ShakeDur == 0)
                pad.ShakeFreq = 0;
        }
        auto& slot = Slots[ActiveSlot];
        if (slot.motors[0] != motor || slot.motors[1] != motor)
        {
            XINPUT_VIBRATION vibration{ motor, motor };
            XInput().setState(DWORD(ActiveSlot), &vibration);
            slot.motors[0] = slot.motors[1] = motor;
        }
    }

    // The modern layout's aim: the PC's mouse aiming, the right stick moving the mouse
    void UpdateModernAim(CPad& pad, Context context)
    {
        bool aim = (context == Context::ModernFoot || context == Context::ModernScope) && PadIsLastDevice && Down(Current, LT) &&
            !pad.DisablePlayerControls && !CCutsceneMgr::IsRunning();
        ModernAiming = aim;
        if (!aim || !NewMouseState.is_initialized() || !MouseAccelHorizontal.is_initialized() || !MouseAccelVertical.is_initialized())
            return;

        float rx, ry;
        Stick(Current.sThumbRX, Current.sThumbRY, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE, rx, ry);
        static auto& speed = FixSettings().Float("MAIN.GamepadCameraSpeed");
        static auto& invert = FixSettings().Int("MAIN.InvertGamepadY");
        // Degrees per second at full tilt, eased near the center for fine aiming
        constexpr float TurnRate = 160.0f, PitchRate = 100.0f;
        float dt = CTimer::ms_fTimeStep / 50.0f;
        auto curve = [](float v) { return v * std::abs(v); };
        // The aiming camera turns by mouse units times its sensitivity and the field of view (by 2.5 and 4 times its
        // accelerations, by FOV / 80): the stick's speed is the same whatever the mouse slider
        float fovScale = TheCamera->m_aCams[TheCamera->m_nActiveCam].FOV / 80.0f;
        float perUnitX = std::max(*MouseAccelHorizontal * 2.5f * fovScale, 1e-5f);
        float perUnitY = std::max(*MouseAccelVertical * 4.0f * fovScale, 1e-5f);
        NewMouseState->X += curve(rx) * DEGTORAD(TurnRate) * speed * dt / perUnitX;
        NewMouseState->Y += curve(invert ? ry : -ry) * DEGTORAD(PitchRate) * speed * dt / perUnitY;
    }

    void UpdateCameraStyle(Context context)
    {
        bool standard = FrontendMenuManager->m_ControlMethod == 0;
        if (!PadIsLastDevice || context == Context::Frontend)
        {
            *UseMouse3rdPerson = standard;
            return;
        }
        // On a pad the console's camera, and the mouse's aiming only while the modern layout aims
        *UseMouse3rdPerson = ModernAiming;
    }

    // Start is Escape: it opens the pause menu and goes back (and out) in the menus
    bool StartWasDown = false;
    bool EscapeSet = false;
    void UpdateStartButton()
    {
        if (EscapeSet && TempEscape.is_initialized())
            *TempEscape = 0;
        EscapeSet = false;
        bool down = ActiveSlot >= 0 && (Current.wButtons & XINPUT_GAMEPAD_START);
        bool pressed = down && !StartWasDown;
        StartWasDown = down;
        if (pressed && TempEscape.is_initialized() && !*TempEscape)
        {
            *TempEscape = 1;
            EscapeSet = true;
        }
    }

    // Before CPad::Update of the first pad: the keyboard, mouse and DirectInput states are in place
    void BeforePadUpdate(CPad& pad)
    {
        bool padUsed = PollPads();
        UpdateStartButton();
        bool kbmUsed = KeyboardOrMouseUsed(pad);
        if (padUsed)
            PadIsLastDevice = true;
        else if (kbmUsed || ActiveSlot < 0)
            PadIsLastDevice = false;

        auto context = CurrentContext();
        UpdateModernAim(pad, context);
        if (ActiveSlot >= 0)
            pad.PCTempJoyState = Compose(context, Current);
        UpdateCameraStyle(context);
        UpdateVibration(pad);
    }
}

export namespace Pad
{
    // Whether the fix handles pads at all (not with a controller mod)
    bool IsInstalled()
    {
        return Installed;
    }

    bool IsGInputLoaded()
    {
        return GInputLoaded;
    }

    // The pad was used last: prompts show its buttons
    bool IsPadLastDevice()
    {
        return Installed && PadIsLastDevice && ActiveSlot >= 0;
    }

    bool IsModernLayout()
    {
        static auto& modern = FixSettings().Int("MAIN.ModernControls");
        return Installed && modern != 0;
    }

    IconStyle Style()
    {
        static auto& icons = FixSettings().Int("MAIN.GamepadIcons");
        if (icons >= 1 && icons <= 3)
            return IconStyle(icons);
        if (ActiveSlot >= 0)
            return Slots[ActiveSlot].style;
        return IconStyle::Xbox;
    }

    bool IsModernAiming()
    {
        return ModernAiming;
    }
}

class PadModule
{
public:
    PadModule()
    {
        WFP::onGameInitEvent() += []()
        {
            GInputLoaded = GetModuleHandleW(L"GInputSA.asi") != nullptr;
            if (GInputLoaded)
                return;
            if (!XInput().getState)
                return;

            // CPad::UpdatePads: Pads[0].Update(0), Pads[1].Update(1), then the keyboard states are copied
            auto update = hook::pattern("6A 00 B9 ? ? ? ? E8 ? ? ? ? 6A 01 B9 ? ? ? ? E8 ? ? ? ? B9 9C 00 00 00 BE ? ? ? ? BF ? ? ? ? ? ? B9 9C 00 00 00 BE");
            auto mouse3rdPerson = hook::pattern("A0 ? ? ? ? 84 C0 74 0A 66 83");
            auto mouseState = hook::pattern("A0 ? ? ? ? 84 C0 74 0C A0 2C");
            // CCam::Process_AimWeapon's mouse turn: the sensitivities it reads (another mod may point both at one)
            auto accel = hook::pattern("D8 C9 D8 0D ? ? ? ? D9 44 24 2C D8 0D ? ? ? ? D8 CA D8 0D");
            UseMouse3rdPerson.SetAddress(*mouse3rdPerson.get_first<bool*>(1));
            NewMouseState.SetAddress(*mouseState.get_first<CMouseControllerState*>(1));
            MouseAccelVertical.SetAddress(*accel.get_first<float*>(4));
            MouseAccelHorizontal.SetAddress(*accel.get_first<float*>(22));
            // CKeyboardState::esc of CPad::TempKeyState
            TempEscape.SetAddress(reinterpret_cast<int16_t*>(*update.get_first<uint8_t*>(47) + 0x218));

            static auto UpdateHook = safetyhook::create_mid(update.get_first(7), [](SafetyHookContext& regs)
            {
                BeforePadUpdate(*reinterpret_cast<CPad*>(regs.ecx));
            });
            Installed = true;
        };

        WFP::onShutdownEvent() += []()
        {
            if (Installed && XInput().setState)
            {
                for (DWORD i = 0; i < XUSER_MAX_COUNT; ++i)
                {
                    if (Slots[i].connected)
                    {
                        XINPUT_VIBRATION off{};
                        XInput().setState(i, &off);
                    }
                }
            }
        };
    }
} PadModule;
