module;

#include <stdafx.h>

#include "common.h"
#include "FixSettings.hpp"
#include "Shared/RWGTA/PadOptions.hpp"
#include "Shared/PadInput.hpp"
#include "Shared/RWGTA/PadCheats.hpp"
#include "Shared/RWGTA/MenuPadButtons.hpp"

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
//   previous one), d-pad left is the second weapon; d-pad right and left also answer yes and no. In aircraft LB/RB
//   are the rudder. As a passenger RT fires the drive-by, which aims with the right stick (the console's aiming).
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
        L3, R3, LookBehind, ActionLeft,
        SecondWeapon,       // the vehicle's second weapon, and "no" in conversations
        NextStation         // the next radio station, and "yes" in conversations
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
        To::L3, To::LookBehind, To::Select, To::None, To::R3, To::DPadDown, To::SecondWeapon, To::NextStation };
    constexpr To LayoutModernAircraft[SrcCount] = { To::Circle, To::None, To::None, To::Triangle, To::L2, To::R2, To::None, To::None,
        To::L3, To::None, To::Select, To::None, To::R3, To::DPadDown, To::SecondWeapon, To::NextStation };

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
    std::atomic_bool mouseActivity = false;
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

    // The camera's stick: a camera speed above 1 reaches the game's full turn sooner and goes past it at full tilt
    // (the camera code takes the stick's value as it is)
    int16_t ToCameraAxis(float v, float speed)
    {
        float range = std::max(speed, 1.0f);
        return int16_t(std::clamp(int(std::lround(v * speed * 128.0f)), int(-128 * range), int(127 * range)));
    }

    bool HasInput(const XINPUT_GAMEPAD& now, const XINPUT_GAMEPAD& before)
    {
        if (now.wButtons != before.wButtons)
            return true;
        auto triggerChanged = [](BYTE current, BYTE previous)
        {
            bool down = current > XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
            return down != (previous > XINPUT_GAMEPAD_TRIGGER_THRESHOLD) ||
                (down && std::abs(int(current) - int(previous)) >= 4);
        };
        auto stickChanged = [](SHORT x, SHORT y, SHORT oldX, SHORT oldY, SHORT deadzone)
        {
            float nx, ny, px, py;
            Stick(x, y, deadzone, nx, ny);
            Stick(oldX, oldY, deadzone, px, py);
            return std::abs(ToAxis(nx) - ToAxis(px)) >= 2 || std::abs(ToAxis(ny) - ToAxis(py)) >= 2;
        };
        return triggerChanged(now.bLeftTrigger, before.bLeftTrigger) || triggerChanged(now.bRightTrigger, before.bRightTrigger) ||
            stickChanged(now.sThumbLX, now.sThumbLY, before.sThumbLX, before.sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) ||
            stickChanged(now.sThumbRX, now.sThumbRY, before.sThumbRX, before.sThumbRY, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
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
                slot.previous = state.Gamepad;
            }
            if (ActiveSlot == int(i))
                Current = state.Gamepad;
        }
        if (ActiveSlot < 0)
            Current = {};
        return used;
    }

    bool KeyboardOrMouseUsed(const CPad& pad)
    {
        static CControllerState previousKeys{};
        static CControllerState previousMouse{};
        static unsigned previousButtons = 0;
        bool changed = mouseActivity.exchange(false, std::memory_order_relaxed) ||
                       std::memcmp(&previousKeys, &pad.PCTempKeyState, sizeof(previousKeys)) != 0 ||
                       std::memcmp(&previousMouse, &pad.PCTempMouseState, sizeof(previousMouse)) != 0;
        previousKeys = pad.PCTempKeyState;
        previousMouse = pad.PCTempMouseState;
        if (NewMouseState.is_initialized())
        {
            const auto& mouse = *NewMouseState;
            unsigned buttons = (mouse.LMB != 0) | ((mouse.RMB != 0) << 1) | ((mouse.MMB != 0) << 2);
            changed |= previousButtons != buttons;
            previousButtons = buttons;
            return changed || mouse.X != 0 || mouse.Y != 0 || mouse.WheelUp || mouse.WheelDown;
        }
        return changed;
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
            case To::SecondWeapon: set(s.LeftShoulder1); set(s.DPadLeft); break;
            case To::NextStation: set(s.DPadUp); set(s.DPadRight); break;
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

    enum class Context { Frontend, Classic, ModernFoot, ModernScope, ModernVehicle, ModernAircraft, ModernPassenger };

    // The player rides in a vehicle someone else drives
    bool DrivenByOther(CEntity* vehicle)
    {
        // CVehicle::m_pDriver
        auto driver = *reinterpret_cast<CEntity**>(reinterpret_cast<uint8_t*>(vehicle) + 0x460);
        return FindPlayerPed && driver != FindPlayerPed(-1);
    }

    Context CurrentContext()
    {
        if (MenuActive())
            return Context::Frontend;
        const auto modern = RWGTA::PadOptions::ModernControls.load(std::memory_order_relaxed);
        if (!modern)
            return Context::Classic;
        if (auto vehicle = PlayerVehicle())
        {
            if (DrivenByOther(vehicle))
                return Context::ModernPassenger;
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
        const auto speed = RWGTA::PadOptions::CameraSpeed.load(std::memory_order_relaxed);
        // The PlayStation 2 sticks: down is positive. The camera's stick turns faster or slower by the camera speed.
        // On foot the game's cameras look up on a positive right stick Y; vehicles keep the console's sign (the
        // hydraulics, the guns, flying)
        float look = context == Context::Frontend ? 1.0f : speed;
        float lookUp = RWGTA::PadOptions::LookUp(ry);
        s.LeftStickX = ToAxis(lx);
        s.LeftStickY = ToAxis(-ly);
        s.RightStickX = ToCameraAxis(rx, look);
        s.RightStickY = ToCameraAxis(context == Context::Frontend ? -ry : PlayerVehicle() ? -lookUp : lookUp, look);

        const To* layout = LayoutClassic;
        switch (context)
        {
            case Context::Frontend: layout = LayoutFrontend; break;
            case Context::ModernFoot: layout = LayoutModernFoot; break;
            case Context::ModernScope: layout = LayoutModernFoot; break;
            case Context::ModernVehicle: layout = LayoutModernVehicle; break;
            case Context::ModernAircraft: layout = LayoutModernAircraft; break;
            case Context::ModernPassenger: layout = LayoutModernVehicle; break;
            default: break;
        }
        for (int i = 0; i < SrcCount; ++i)
        {
            if (Down(g, Src(i)))
                Press(s, layout[i]);
        }
        // The map: A sets and clears the target (the console's Circle)
        if (context == Context::Frontend && Down(g, A) && FrontendMenuManager->GetCurrentScreen() == SCREEN_MAP)
            Press(s, To::Circle);

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
            case Context::ModernPassenger:
                // RT fires the drive-by (the console's Circle, as the vehicle's weapon)
                if (Down(g, RT))
                    Press(s, To::Circle);
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
        const auto vibration = RWGTA::PadOptions::Vibration.load(std::memory_order_relaxed);
        WORD motor = 0;
        if (pad.ShakeDur > 0)
        {
            if (PadIsLastDevice && !MenuActive() && vibration)
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
        const auto speed = RWGTA::PadOptions::CameraSpeed.load(std::memory_order_relaxed);
        // Degrees per second at full tilt, eased near the center for fine aiming
        constexpr float TurnRate = 160.0f, PitchRate = 100.0f;
        float dt = CTimer::ms_fTimeStep / 50.0f;
        auto curve = [](float v) { return v * std::abs(v); };
        // The aiming camera turns by mouse units times its sensitivity and the field of view (by 2.5 and 4 times its
        // accelerations, by FOV / 80; the scopes by 3 and 4): the stick's speed is the same whatever the mouse slider
        float fovScale = TheCamera->m_aCams[TheCamera->m_nActiveCam].FOV / 80.0f;
        float perUnitX = std::max(*MouseAccelHorizontal * (context == Context::ModernScope ? 3.0f : 2.5f) * fovScale, 1e-5f);
        float perUnitY = std::max(*MouseAccelVertical * 4.0f * fovScale, 1e-5f);
        NewMouseState->X += curve(rx) * DEGTORAD(TurnRate) * speed * dt / perUnitX;
        NewMouseState->Y += curve(RWGTA::PadOptions::LookUp(ry)) * DEGTORAD(PitchRate) * speed * dt / perUnitY;
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

    // The map: Y shows and hides the legend (the keyboard's L)
    bool LegendWasDown = false;
    void UpdateMapLegend(Context context)
    {
        bool down = ActiveSlot >= 0 && context == Context::Frontend && Down(Current, Y) &&
            FrontendMenuManager->GetCurrentScreen() == SCREEN_MAP;
        if (down && !LegendWasDown)
            FrontendMenuManager->m_bMapLegend = !FrontendMenuManager->m_bMapLegend;
        LegendWasDown = down;
    }

    // Start is Escape: it opens the pause menu and goes back (and out) in the menus
    bool EscapeSet = false;
    void UpdateStartButton()
    {
        if (EscapeSet && TempEscape.is_initialized())
            *TempEscape = 0;
        EscapeSet = false;
        static RWGTA::MenuPadButtons buttons;
        const bool active = FrontendMenuManager->m_bMenuActive;
        const WORD pressed = buttons.Update(Current, active);
        const bool back = active && !FrontendMenuManager->m_bMainMenuSwitch &&
            FrontendMenuManager->GetCurrentScreen() == SCREEN_PAUSE_MENU && (pressed & XINPUT_GAMEPAD_B);
        if (((pressed & XINPUT_GAMEPAD_START) || back) && TempEscape.is_initialized() && !*TempEscape)
        {
            *TempEscape = 1;
            EscapeSet = true;
        }
    }

    void ActivatePadCheat(unsigned int index)
    {
        // PC keyboard and controller cheats share the localized message, statistics and save flag.
        static auto accepted = hook::pattern("38 9E ? ? ? ? 53 53 B9 ? ? ? ? 6A 01 74 07 68 ? ? ? ? EB 05 68 ? ? ? ? E8");
        static const auto active = *accepted.get_first<bool*>(2);
        static const auto text = *accepted.get_first<void*>(9);
        static const auto getText = reinterpret_cast<const char*(__thiscall*)(void*, const char*)>(injector::GetBranchDestination(accepted.get_first(29)).as_int());
        static const auto showHelp = reinterpret_cast<void(__cdecl*)(const char*, bool, bool, bool)>(injector::GetBranchDestination(accepted.get_first(35)).as_int());
        static const auto incrementStat = reinterpret_cast<void(__cdecl*)(unsigned short, float)>(injector::GetBranchDestination(accepted.get_first(53)).as_int());
        static const auto hasCheated = *accepted.get_first<bool*>(72);
        static const auto keyboardHistory = *accepted.get_first<char*>(85);
        static const auto apply = reinterpret_cast<void(__cdecl*)(unsigned int)>(hook::pattern("8B 44 24 04 8B 0C 85 ? ? ? ? 85 C9 74 02 FF E1 8A 88").get_first());

        showHelp(getText(text, active[index] ? "CHEAT8" : "CHEAT1"), true, false, false);
        incrementStat(137, 1.0f);
        *hasCheated = true;
        apply(index);
        *keyboardHistory = '\0';
    }

    void UpdatePadCheats(CPad& pad)
    {
        struct Cheat
        {
            std::string_view buttons;
            unsigned int index;
        };
        static constexpr Cheat cheats[] =
        {
            { "3414LDRULDRU", 0 }, // LXGIWYL
            { "3414LDRULDDL", 1 }, // KJKSZPJ
            { "3414LDRULDDD", 2 }, // UZUMYMW
            { "341XLDRULDRU", 3 }, // HESOYAM
            { "33C4LRLRLR", 4 }, // OSRBLHH
            { "33C4UDUDUD", 5 }, // ASNAEB
            { "4X11222S", 6 }, // AFZLLQLL
            { "4X11222D", 7 }, // ICIKPYH
            { "2DDLSL4SX311", 8 }, // ALNSFMZO
            { "4X11222C", 9 }, // AUIFRVQS
            { "4X11222X", 10 }, // CFVFGMJ
            { "CC1S1SSS1TCT", 11 }, // YSOHNUL
            { "TURD21S", 12 }, // PPGWJHT
            { "TURDS43", 13 }, // LIYOAAY
            { "DLULX4321", 14 }, // AJLOJYQY
            { "DUUUX4322", 15 }, // BAGOWPG
            { "43XTXTUD", 16 }, // FOOOXFT
            { "CC1CCC123TCT", 17 }, // TIMETOKICKASS
            { "D3C22X31LL", 18 }, // CQZIJMB
            { "URR1RUS2", 19 }, // JQNTDMH
            { "3C4R12XXS3", 20 }, // VROCKPOKEY
            { "41CR13RUC4", 21 }, // VPJTQWV
            { "D4D32L31LR", 22 }, // AQTBCODX
            { "4U2LL31CR", 23 }, // KRIJEBR
            { "C3C3LL31CR", 24 }, // UBHYZHQ
            { "C1U32X31CX", 25 }, // RZHSUEW
            { "423124STCT21", 26 }, // CPKTNWT
            { "T1T4S11", 27 }, // XICWMD
            { "T33L3141", 28 }, // PGGOMOY
            { "R2D3LL3121", 29 }, // SZCMAWO
            { "R3U22L3133", 30 }, // ZEIIVG
            { "4C32L3142", 31 }, // YLTEICZ
            { "C1D2LX31RC", 32 }, // LLQPFBN
            { "C2U3LX31LC", 33 }, // IOWDLAC
            { "R4C32S34", 34 }, // Drive on water
            { "4CU1R3RUST", 35 }, // AFSNMSMW
            { "TUULRSCD", 36 }, // BTCDBCB
            { "TUULRSCL", 37 }, // JYSDSOD
            { "TUULRSCR", 38 }, // KVGYZQK
            { "1CT11S2UDL", 39 }, // BLUESUEDESHOES
            { "X1USDX2TD311", 40 }, // BGLUAWML
            { "UUDDSC13TD", 41 }, // CIKGCGX
            { "2U33L334RD", 42 }, // ONLYHOMIESALLOWED
            { "DR31TSSCC", 43 }, // BIFBUZZ
            { "TXXX12LLUU", 44 }, // AFPHULTL
            { "RLTCC3444", 45 }, // BEKKNQV
            { "2R1UX1243111", 46 }, // KKZRPKW
            { "U13URUX2X1", 47 }, // GUSNHDE
            { "UD131RL1L", 48 }, // RIPAZHA
            { "TSCCSCC12234", 49 }, // JHJOECW
            { "TTSCX11DU", 50 }, // JUMPJET
            { "TTSCX12DD", 51 }, // KGGGDKP
            { "122UDDU344", 52 }, // JCNRUAD
            { "LT31USTDC211", 53 }, // COXEFGU
            { "2341XXCCSS", 54 }, // BSXSGGC
            { "4X11222T", 55 }, // XJVSNAJ
            { "LL23RSS12X", 56 }, // OFVIAC
            { "4T4TCCLRUU", 57 }, // MGHXYRM
            { "UD11221234", 58 }, // CWJXUOC
            { "UUTTUULRS44", 60 }, // KANGAROO
            { "TXXCCS1423", 61 }, // BAGUVIX
            { "DL1DD4D2D", 62 }, // CVWKXAM
            { "LR12344UDR1", 63 }, // AIYPWZQP
            { "1234UDLR1234UDLR", 64 }, // YECGAA
            { "CRCRLSTU", 65 }, // AEZAKMI
            { "CRCRLSXD", 66 }, // LJSPQK
            { "ULXT3CCC2", 67 }, // IAVENJQ
            { "UDDLR112334", 68 }, // AEDUWNV
            { "2R1TRR31R111", 69 }, // IOJUFZN
            { "TT1SSCSDC", 70 }, // PRIEBJ
            { "TRD12SS34", 71 }, // MUNASEF
            { "13S3L43LSD11", 72 }, // WANRLTW
            { "XDU4DT1TL", 74 }, // THGLOJ
            { "TLS4U2D1X111", 75 }, // FVTMNBZ
            { "DSU44URRU", 76 }, // SJMAHPE
            { "344344LLRLLR", 77 }, // BMTPWHR
            { "344344LLRRRL", 78 }, // ZSOXFSQ
            { "13TD4X1U2211", 79 }, // OGXSDAG
            { "CTTUC32UT111", 80 }, // EHIBXQS
            { "ULLDCS11234", 81 }, // VKYPQCF
            { "DSXL34LDD111", 82 }, // NCSGDAG
            { "S2X322L3R111", 83 }, // VQIMAHA
            { "CX1CC1C34211", 84 }, // OHDUDE
            { "LLDDUUSCT34", 85 }, // AKJJYGLC
            { "3ULR4URSR211", 86 }, // AMOMHRER
            { "411RRUUX1L", 87 }, // EEGCYXT
            { "CU12D311LLXT", 88 }, // URKQSRK
            { "RU333DTTXC11", 89 }, // AGBDLCID
            { "R22D2UU24", 90 }, // Prostitutes pay you
            { "UXTXTXS4R", 91 }, // Taxi nitro
        };
        static RWGTA::PadCheats sequence;
        const bool blocked = MenuActive() || pad.DisablePlayerControls || CCutsceneMgr::IsRunning();
        if (const auto match = sequence.Find(Current, blocked ? -1 : ActiveSlot, std::span<const Cheat>(cheats)))
            ActivatePadCheat(match->index);
    }

    // Before CPad::Update of the first pad: the keyboard, mouse and DirectInput states are in place
    void BeforePadUpdate(CPad& pad)
    {
        bool padUsed = PollPads();
        UpdatePadCheats(pad);
        UpdateStartButton();
        bool kbmUsed = KeyboardOrMouseUsed(pad);
        if (padUsed)
            PadIsLastDevice = true;
        else if (kbmUsed || ActiveSlot < 0)
            PadIsLastDevice = false;

        auto context = CurrentContext();
        UpdateMapLegend(context);
        UpdateModernAim(pad, context);
        if (ActiveSlot >= 0)
            pad.PCTempJoyState = Compose(context, Current);
        UpdateCameraStyle(context);
        UpdateVibration(pad);
    }
}

export namespace Pad
{
    void ObserveMouseActivity()
    {
        mouseActivity.store(true, std::memory_order_relaxed);
    }

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
        const auto modern = RWGTA::PadOptions::ModernControls.load(std::memory_order_relaxed);
        return Installed && modern != 0;
    }

    IconStyle Style()
    {
        const auto icons = RWGTA::PadOptions::GamepadIcons.load(std::memory_order_relaxed);
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

    // The player rides in a vehicle someone else drives
    bool IsPassenger()
    {
        auto vehicle = PlayerVehicle();
        return vehicle && DrivenByOther(vehicle);
    }
}

class PadModule
{
public:
    PadModule()
    {
        WFP::onGameInitEvent() += []()
        {
            RWGTA::PadOptions::Install(FixSettings(), false);
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
