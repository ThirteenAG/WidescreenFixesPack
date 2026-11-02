module;

#include <stdafx.h>
#include "common.h"
#include "FixSettings.hpp"
#include "Shared/PadInput.hpp"

export module Pad;

import Main;
import Menu;
import Timer;
import CutsceneMgr;
import Camera;

// XInput pads. The pad's buttons become the game's own controller state (the PlayStation 2 layout the game's pad code
// reads), so every pad feature of the game works as on the console: the controller configurations, vibration, the
// menus. Two layouts:
// - Classic: the buttons by position (A = Cross, B = Circle, ...), the console game's controls.
// - Modern (MAIN.ModernControls): the default pad layout of the Definitive Edition. On foot: LT aims (an
//   over-the-shoulder aim that moves and shoots at once, the game's own mouse aiming driven by the right stick; sniper
//   rifles, the rocket launcher and the camera use their scope, the left stick zooms), RT fires, B attacks, A sprints,
//   X jumps, Y enters, LB takes the next weapon (the PC game has no weapon wheel; d-pad left the previous one), RB is
//   the action button (pickups, the phone, camera behind), L3 crouches, R3 looks behind. In a vehicle: RT accelerates,
//   LT brakes, A fires the vehicle's weapon, RB is the handbrake, X/B look left/right, R3 looks behind, L3 is the horn,
//   d-pad up starts a side mission, d-pad right takes the next radio station (no radio wheel either), d-pad left sets
//   off a car bomb. In aircraft LB/RB are the rudder and B fires the helicopter's gun (the handbrake there).
// The last device used decides the prompts' icons and, for the pad, the console's camera (the mouse camera of the
// PC's standard controls returns with the mouse). The pad's state replaces what the game read from the same pad
// through DirectInput; other DirectInput pads keep working as before.
// With a controller mod loaded that does all of this its own way, none of it is installed.

export struct CControllerState
{
    int16_t LeftStickX, LeftStickY, RightStickX, RightStickY;
    int16_t LeftShoulder1, LeftShoulder2, RightShoulder1, RightShoulder2;
    int16_t DPadUp, DPadDown, DPadLeft, DPadRight;
    int16_t Start, Select, Square, Triangle, Cross, Circle;
    int16_t LeftShock, RightShock, NetworkTalk;
};
static_assert(sizeof(CControllerState) == 0x2A);

export struct CPad
{
    CControllerState NewState;
    CControllerState OldState;
    int16_t SteeringLeftRightBuffer[10];
    int32_t DrunkDrivingBufferUsed;
    CControllerState PCTempKeyState;
    CControllerState PCTempJoyState;
    CControllerState PCTempMouseState;
    int16_t Phase;
    int16_t Mode;
    int16_t ShakeDur;
    uint16_t DisablePlayerControls;
    uint8_t ShakeFreq;
    uint8_t bHornHistory[5];
    uint8_t iCurrHornHistory;
    int8_t JustOutOfFrontend;
    int8_t bApplyBrakes;
    char CheatString[12];
    int32_t LastTimeTouched;
    int32_t AverageWeapon;
    int32_t AverageEntries;
};
static_assert(offsetof(CPad, PCTempKeyState) == 0x6C);
static_assert(offsetof(CPad, PCTempJoyState) == 0x96);
static_assert(offsetof(CPad, Mode) == 0xEC);
static_assert(offsetof(CPad, ShakeDur) == 0xEE);
static_assert(offsetof(CPad, DisablePlayerControls) == 0xF0);
static_assert(offsetof(CPad, ShakeFreq) == 0xF2);
static_assert(offsetof(CPad, LastTimeTouched) == 0x108);
static_assert(sizeof(CPad) == 0x114);

static_assert(offsetof(CCamera, m_fFLOATingFade) == 0x910);

struct CMouseControllerState
{
    bool LMB, RMB, MMB, WHEELUP, WHEELDN, MXB1, MXB2;
    float x, y;
};

export enum class IconStyle : int
{
    Xbox = 1,
    PlayStation = 2,
    Nintendo = 3,
};

namespace
{
    enum Weapon
    {
        WEAPON_UNARMED = 0,
        WEAPON_DETONATOR_GRENADE = 13,
        WEAPON_M4 = 26,
        WEAPON_RUGER = 27,
        WEAPON_SNIPERRIFLE = 28,
        WEAPON_LASERSCOPE = 29,
        WEAPON_ROCKETLAUNCHER = 30,
        WEAPON_M60 = 32,
        WEAPON_DETONATOR = 34,
        WEAPON_CAMERA = 36,
    };

    // The weapons that only aim through their scope
    bool IsScopeWeapon(int weapon)
    {
        return weapon == WEAPON_SNIPERRIFLE || weapon == WEAPON_LASERSCOPE || weapon == WEAPON_ROCKETLAUNCHER || weapon == WEAPON_CAMERA;
    }

    // The camera modes of the first-person weapons (the pad's sticks swap there in the modern layout)
    bool IsScopeMode(int16_t mode)
    {
        switch (mode)
        {
            case CCam::MODE_SNIPER:
            case CCam::MODE_ROCKETLAUNCHER:
            case CCam::MODE_M16_1STPERSON:
            case CCam::MODE_SNIPER_RUNABOUT:
            case CCam::MODE_ROCKETLAUNCHER_RUNABOUT:
            case CCam::MODE_1STPERSON_RUNABOUT:
            case CCam::MODE_M16_1STPERSON_RUNABOUT:
            case CCam::MODE_HELICANNON_1STPERSON:
            case CCam::MODE_CAMERA:
                return true;
            default:
                return false;
        }
    }

    // The vehicles whose turret the right stick turns
    constexpr int16_t MI_FIRETRUCK = 137, MI_RHINO = 162;

    // The flyable aircraft (automobiles with a flying model)
    bool IsAircraftModel(int model)
    {
        switch (model)
        {
            case 155:   // Hunter
            case 177:   // Sea Sparrow
            case 190:   // Skimmer
            case 194:   // RC Baron
            case 195:   // RC Raider
            case 199:   // Sparrow
            case 217:   // Maverick
            case 218:   // VCN Maverick
            case 227:   // Police Maverick
            case 231:   // RC Goblin
                return true;
            default:
                return false;
        }
    }

    // What a pad button becomes in the game's controller state
    enum class To : uint8_t
    {
        None, L1, L2, R1, R2, DPadUp, DPadDown, DPadLeft, DPadRight, Start, Select, Square, Triangle, Cross, Circle,
        L3, R3, LookBehind
    };

    enum Src { A, B, X, Y, LB, RB, LT, RT, LS, RS, Back, Start, DUp, DDown, DLeft, DRight, SrcCount };

    constexpr To LayoutClassic[SrcCount] = { To::Cross, To::Circle, To::Square, To::Triangle, To::L1, To::R1, To::L2, To::R2,
        To::L3, To::R3, To::Select, To::Start, To::DPadUp, To::DPadDown, To::DPadLeft, To::DPadRight };
    // B goes back in the menus (the PC menus' back button is the Triangle)
    constexpr To LayoutFrontend[SrcCount] = { To::Cross, To::Triangle, To::None, To::None, To::L1, To::R1, To::L2, To::R2,
        To::L3, To::R3, To::Select, To::Start, To::DPadUp, To::DPadDown, To::DPadLeft, To::DPadRight };
    // LT is the aim (handled by itself); LB is the next weapon, d-pad left the previous one (no stats on the pad here)
    constexpr To LayoutModernFoot[SrcCount] = { To::Cross, To::Circle, To::Square, To::Triangle, To::R2, To::L1, To::None, To::Circle,
        To::L3, To::R3, To::Select, To::Start, To::DPadUp, To::DPadDown, To::L2, To::DPadRight };
    // LT and RT are analog (brake, accelerate); the vehicle's weapon also sets off a car bomb (d-pad left)
    constexpr To LayoutModernVehicle[SrcCount] = { To::Circle, To::R2, To::L2, To::Triangle, To::None, To::R1, To::None, To::None,
        To::L3, To::LookBehind, To::Select, To::Start, To::R3, To::None, To::Circle, To::L1 };
    // The look buttons turn a helicopter (and roll the RC plane); the handbrake fires the helicopters' guns
    constexpr To LayoutModernAircraft[SrcCount] = { To::Circle, To::R1, To::None, To::Triangle, To::L2, To::R2, To::None, To::None,
        To::L3, To::None, To::Select, To::Start, To::R3, To::None, To::Circle, To::L1 };

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
    int16_t SavedMode = -1;            // the player's controller configuration while the modern layout needs the first
    ULONGLONG LastShakeTick = 0;
    XINPUT_GAMEPAD Current{};

    // CCamera::m_bUseMouse3rdPerson
    GameRef<bool> UseMouse3rdPerson([]() -> bool*
    {
        auto pattern = hook::pattern("80 3D ? ? ? ? 00 74 0A 66 83 79 0C 04");
        return *pattern.get_first<bool*>(2);
    });

    // CPad::NewMouseControllerState, before its x written by CPad::UpdateMouse
    GameRef<CMouseControllerState> NewMouseState([]() -> CMouseControllerState*
    {
        auto pattern = hook::pattern("D9 1D ? ? ? ? D9 05 ? ? ? ? D9 1D ? ? ? ? 83");
        return reinterpret_cast<CMouseControllerState*>(*pattern.get_first<uint8_t*>(2) - 8);
    });

    // CCamera::m_fMouseAccelHorzntl and m_fMouseAccelVertical, read by CCam::Process_FollowPedWithMouse
    GameRef<float> MouseAccelHorizontal([]() -> float*
    {
        auto pattern = hook::pattern("D9 05 ? ? ? ? D8 4C 24 14 D8 C9");
        return *pattern.get_first<float*>(2);
    });

    GameRef<float> MouseAccelVertical([]() -> float*
    {
        auto pattern = hook::pattern("D9 05 ? ? ? ? D8 4C 24 18 D8 C9");
        return *pattern.get_first<float*>(2);
    });

    bool MenuActive()
    {
        return FrontendMenuManager.get_ptr() && FrontendMenuManager->m_bMenuActive;
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
        if (NewMouseState.get_ptr())
        {
            const auto& mouse = *NewMouseState.get_ptr();
            if (std::abs(mouse.x) + std::abs(mouse.y) > 2.0f || mouse.LMB || mouse.RMB || mouse.MMB || mouse.WHEELUP || mouse.WHEELDN)
                return true;
        }
        return false;
    }

    int CurrentWeapon()
    {
        auto ped = reinterpret_cast<uint8_t*>(FindPlayerPed ? FindPlayerPed() : nullptr);
        if (!ped)
            return WEAPON_UNARMED;
        auto slot = *reinterpret_cast<int8_t*>(ped + 0x504);
        return *reinterpret_cast<int32_t*>(ped + 0x408 + slot * 0x18);
    }

    CEntity* PlayerVehicle()
    {
        return FindPlayerVehicle ? FindPlayerVehicle() : nullptr;
    }

    int16_t ActiveCameraMode()
    {
        if (!TheCamera.get_ptr())
            return 0;
        return TheCamera->Cams[TheCamera->ActiveCam].Mode;
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
            return IsAircraftModel(vehicle->GetModelIndex()) ? Context::ModernAircraft : Context::ModernVehicle;
        if (IsScopeMode(ActiveCameraMode()))
            return Context::ModernScope;
        return Context::ModernFoot;
    }

    // The free camera turns with the right stick: the game mustn't take it as its own look or lean
    bool FreeCameraTakesRightStick(Context context)
    {
        static auto& freeCamera = FixSettings().Int("MAIN.FreeCamera");
        if (!freeCamera || context == Context::Frontend || context == Context::ModernScope)
            return false;
        if (auto vehicle = PlayerVehicle())
        {
            auto model = vehicle->GetModelIndex();
            return model != MI_RHINO && model != MI_FIRETRUCK;
        }
        return true;
    }

    CControllerState Compose(Context context, const XINPUT_GAMEPAD& g)
    {
        CControllerState s{};
        float lx, ly, rx, ry;
        Stick(g.sThumbLX, g.sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE, lx, ly);
        Stick(g.sThumbRX, g.sThumbRY, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE, rx, ry);
        static auto& invert = FixSettings().Int("MAIN.InvertGamepadY");
        // The PlayStation 2 sticks: down is positive
        s.LeftStickX = ToAxis(lx);
        s.LeftStickY = ToAxis(-ly);
        s.RightStickX = ToAxis(rx);
        s.RightStickY = ToAxis(invert && context != Context::Frontend ? ry : -ry);

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

        if (FreeCameraTakesRightStick(context) || ModernAiming)
            s.RightStickX = s.RightStickY = 0;

        switch (context)
        {
            case Context::Classic:
            case Context::Frontend:
                // The triggers are digital there; the classic Cross/Square throttle stays on the face buttons
                break;
            case Context::ModernVehicle:
            case Context::ModernAircraft:
                s.Cross = std::max(s.Cross, Trigger(g.bRightTrigger));
                s.Square = std::max(s.Square, Trigger(g.bLeftTrigger));
                break;
            case Context::ModernScope:
            {
                // The scope aims with the right stick, the left stick zooms
                s.LeftStickX = ToAxis(rx);
                s.LeftStickY = ToAxis(invert ? ry : -ry);
                s.RightStickX = s.RightStickY = 0;
                if (ly > 0.3f)
                    Press(s, To::Square);
                else if (ly < -0.3f)
                    Press(s, To::Cross);
                if (Down(g, LT))
                    Press(s, To::R1);
                break;
            }
            case Context::ModernFoot:
            {
                // The scope weapons aim through their scope
                if (Down(g, LT) && IsScopeWeapon(CurrentWeapon()))
                    Press(s, To::R1);
                break;
            }
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

        WORD motor = 0;
        if (pad.ShakeDur > 0)
        {
            if (FrontendMenuManager->m_PrefsUseVibration && PadIsLastDevice && !MenuActive())
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

    // The modern layout's aim: the game's mouse aiming, the right stick moving the mouse
    void UpdateModernAim(CPad& pad, Context context)
    {
        bool aim = false;
        if (context == Context::ModernFoot && Down(Current, LT) && !pad.DisablePlayerControls && !CCutsceneMgr::IsRunning())
        {
            auto weapon = CurrentWeapon();
            aim = !IsScopeWeapon(weapon) && weapon != WEAPON_DETONATOR;
        }
        ModernAiming = aim;
        if (!aim || !NewMouseState.get_ptr() || !MouseAccelHorizontal.get_ptr() || !MouseAccelVertical.get_ptr())
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
        float fovScale = TheCamera->Cams[TheCamera->ActiveCam].FOV / 80.0f;
        float perUnitX = std::max(*MouseAccelHorizontal.get_ptr() * 2.5f * fovScale, 1e-5f);
        float perUnitY = std::max(*MouseAccelVertical.get_ptr() * 4.0f * fovScale, 1e-5f);
        auto& mouse = *NewMouseState.get_ptr();
        mouse.x += curve(rx) * DEGTORAD(TurnRate) * speed * dt / perUnitX;
        mouse.y += curve(invert ? ry : -ry) * DEGTORAD(PitchRate) * speed * dt / perUnitY;
    }

    void UpdateCameraStyle(Context context)
    {
        bool standard = FrontendMenuManager->m_ControlMethod == 0;
        if (!PadIsLastDevice || context == Context::Frontend)
        {
            UseMouse3rdPerson = standard;
            return;
        }
        // On a pad the console's camera, and the mouse's aiming only while the modern layout aims
        UseMouse3rdPerson = ModernAiming;
    }

    void UpdateMode(CPad& pad, Context context)
    {
        // The modern layout is laid out for the game's first configuration
        bool force = PadIsLastDevice && context != Context::Frontend && context != Context::Classic;
        if (force)
        {
            if (SavedMode < 0)
                SavedMode = pad.Mode;
            pad.Mode = 0;
        }
        else if (SavedMode >= 0)
        {
            pad.Mode = SavedMode;
            SavedMode = -1;
        }
    }

    // Start opens and closes the pause menu, which the PC game opens with Escape only: the menu's own requests
    bool StartWasDown = false;
    void UpdateStartButton()
    {
        bool down = ActiveSlot >= 0 && (Current.wButtons & XINPUT_GAMEPAD_START);
        bool pressed = down && !StartWasDown;
        StartWasDown = down;
        if (!pressed || !FrontendMenuManager.get_ptr())
            return;
        auto& menu = *FrontendMenuManager.get_ptr();
        // As Escape: not while the screen fades or a cutscene's borders are on
        if (!menu.m_bMenuActive && TheCamera->m_fFLOATingFade == 0.0f && !TheCamera->m_WideScreenOn)
            menu.m_bStartUpFrontEndRequested = true;
        else if (!menu.m_bGameNotLoaded)
            menu.m_bShutDownFrontEndRequested = true;
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
        UpdateMode(pad, context);
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

    // The right stick, -1..1 (up positive, after the invert setting), when the pad drives the game
    void RightStick(float& x, float& y)
    {
        x = y = 0.0f;
        if (!IsPadLastDevice() || MenuActive())
            return;
        Stick(Current.sThumbRX, Current.sThumbRY, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE, x, y);
        static auto& invert = FixSettings().Int("MAIN.InvertGamepadY");
        if (invert)
            y = -y;
    }

    bool IsModernAiming()
    {
        return ModernAiming;
    }

    enum class Seat { Foot, Vehicle, Aircraft };

    Seat PlayerSeat()
    {
        auto vehicle = PlayerVehicle();
        if (!vehicle)
            return Seat::Foot;
        return IsAircraftModel(vehicle->GetModelIndex()) ? Seat::Aircraft : Seat::Vehicle;
    }

    // This frame's mouse movement, in the game's units
    void MouseDelta(float& x, float& y)
    {
        x = y = 0.0f;
        if (auto mouse = NewMouseState.get_ptr(); mouse && !MenuActive())
        {
            x = mouse->x;
            y = mouse->y;
        }
    }

    // The mouse camera's sensitivity (radians per mouse unit at the default field of view)
    float MouseSensitivity()
    {
        return MouseAccelHorizontal.get_ptr() ? *MouseAccelHorizontal.get_ptr() : 0.0025f;
    }
}

class PadModule
{
public:
    PadModule()
    {
        WFP::onGameInitEvent() += []()
        {
            GInputLoaded = GetModuleHandleW(L"GInputVC.asi") != nullptr;
            if (GInputLoaded)
                return;
            if (!XInput().getState)
                return;

            // CPad::UpdatePads: Pads[0].Update(0)
            auto pattern = hook::pattern("B9 ? ? ? ? 6A 00 E8 ? ? ? ? BA ? ? ? ? 89 D0");
            static auto UpdateHook = safetyhook::create_mid(pattern.get_first(7), [](SafetyHookContext& regs)
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
