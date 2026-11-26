module;

#include <stdafx.h>
#include "common.h"
#include "FixSettings.hpp"
#include "Shared/RWGTA/PadOptions.hpp"
#include "Shared/PadInput.hpp"
#include "Shared/RWGTA/PadCheats.hpp"
#include "Shared/RWGTA/MenuPadButtons.hpp"

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
        WEAPON_GRENADE = 12,
        WEAPON_DETONATOR_GRENADE = 13,
        WEAPON_COLT45 = 17,
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
    constexpr int16_t MI_FIRETRUCK = 137, MI_VOODOO = 142, MI_RHINO = 162;

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
    bool MouseAiming = false;           // the free camera's aim with the keyboard and mouse
    int16_t SavedMode = -1;            // the player's controller configuration while the modern layout needs the first
    ULONGLONG LastShakeTick = 0;
    XINPUT_GAMEPAD Current{};

    // CPad::Pads, from CPad::UpdatePads
    GameRef<CPad> Pads([]() -> CPad*
    {
        auto pattern = hook::pattern("B9 ? ? ? ? 6A 00 E8 ? ? ? ? BA ? ? ? ? 89 D0");
        return *pattern.get_first<CPad*>(1);
    });

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
                slot.previous = {};
                slot.style = DetectStyle(i);
                slot.motors[0] = slot.motors[1] = 0;
                if (ActiveSlot < 0)
                    ActiveSlot = int(i);
            }
            if (HasInput(state.Gamepad, slot.previous))
            {
                slot.previous = state.Gamepad;
                ActiveSlot = int(i);
                used = true;
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
        bool changed = std::memcmp(&previousKeys, &pad.PCTempKeyState, sizeof(previousKeys)) != 0 ||
                       std::memcmp(&previousMouse, &pad.PCTempMouseState, sizeof(previousMouse)) != 0;
        previousKeys = pad.PCTempKeyState;
        previousMouse = pad.PCTempMouseState;
        if (NewMouseState.get_ptr())
        {
            const auto& mouse = *NewMouseState.get_ptr();
            unsigned buttons = (mouse.LMB != 0) | ((mouse.RMB != 0) << 1) | ((mouse.MMB != 0) << 2);
            changed |= previousButtons != buttons;
            previousButtons = buttons;
            return changed || mouse.x != 0 || mouse.y != 0 || mouse.WHEELUP || mouse.WHEELDN;
        }
        return changed;
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
        const auto modern = RWGTA::PadOptions::ModernControls.load(std::memory_order_relaxed);
        if (!modern)
            return Context::Classic;
        if (auto vehicle = PlayerVehicle())
            return IsAircraftModel(vehicle->GetModelIndex()) ? Context::ModernAircraft : Context::ModernVehicle;
        if (IsScopeMode(ActiveCameraMode()))
            return Context::ModernScope;
        return Context::ModernFoot;
    }

    // The free camera turns with the right stick: the game mustn't take it as its own look or lean, nor as the tank's
    // and the fire truck's turret (which follow the free camera)
    bool FreeCameraTakesRightStick(Context context, CEntity* vehicle)
    {
        const auto freeCamera = RWGTA::PadOptions::FreeCamera.load(std::memory_order_relaxed);
        if (!freeCamera || context == Context::Frontend || context == Context::ModernScope)
            return false;
        if (vehicle)
        {
            auto mode = ActiveCameraMode();
            return mode == CCam::MODE_CAM_ON_A_STRING || mode == CCam::MODE_BEHINDBOAT;
        }
        return true;
    }

    // The free camera with the keyboard and mouse: the player moves freely under the camera, and the target button
    // (with any weapon that aims) or the fire button (with a gun) aims, as the game's mouse controls do: the player
    // turns with the camera, the crosshair shows and the shots go to it
    void UpdateMouseAim(const CPad& pad)
    {
        const auto freeCamera = RWGTA::PadOptions::FreeCamera.load(std::memory_order_relaxed);
        const auto modern = RWGTA::PadOptions::ModernControls.load(std::memory_order_relaxed);
        bool aim = false;
        if ((freeCamera || modern) && !PadIsLastDevice && FrontendMenuManager->m_ControlMethod == 0 && !MenuActive() && !PlayerVehicle() &&
            !pad.DisablePlayerControls && !CCutsceneMgr::IsRunning())
        {
            auto weapon = CurrentWeapon();
            bool target = pad.PCTempKeyState.RightShoulder1 || pad.PCTempMouseState.RightShoulder1;
            bool fire = pad.PCTempKeyState.Circle || pad.PCTempMouseState.Circle;
            bool gun = weapon >= WEAPON_COLT45 && weapon < WEAPON_DETONATOR;
            bool aims = gun || weapon == WEAPON_CAMERA || (weapon >= WEAPON_GRENADE && weapon < WEAPON_COLT45 && weapon != WEAPON_DETONATOR_GRENADE);
            aim = (target && aims) || (fire && gun);
        }
        MouseAiming = aim;
    }

    SafetyHookInline processPlayerWeapon;
    uint8_t* (__cdecl* weaponInfo)(int) = nullptr;
    void (__thiscall* setPointGunAt)(void*, CEntity*) = nullptr;
    void (__thiscall* clearPointGunAt)(void*) = nullptr;
    void (__thiscall* setAimDirection)(void*, float) = nullptr;
    void (__thiscall* setLookDirection)(void*, float, bool, bool) = nullptr;
    float (__thiscall* quickAimPitch)(CCamera*) = nullptr;

    void __fastcall ProcessAimedWeapon(uint8_t* player, void*, CPad* pad)
    {
        processPlayerWeapon.unsafe_thiscall(player, pad);
        constexpr int sniping = 12, attacking = 16, pointing = 22, sprinting = 5;
        constexpr uint8_t canAim = 0x40;
        const int state = *reinterpret_cast<int*>(player + 0x244);
        const auto slot = player[0x504];
        static uint8_t* posingPlayer = nullptr;
        const bool target = ModernAiming || (MouseAiming && pad->NewState.RightShoulder1);
        if (target && state != sniping && !IsScopeMode(ActiveCameraMode()) && slot == player[0x60C] &&
            *reinterpret_cast<int*>(player + 0x24C) != sprinting && !*reinterpret_cast<CEntity**>(player + 0x508))
        {
            const auto type = *reinterpret_cast<int*>(player + 0x408 + slot * 0x18);
            if (!IsScopeWeapon(type) && (weaponInfo(type)[0x5C] & canAim))
            {
                float direction = -TheCamera->Orientation;
                setLookDirection(player, direction, true, true);
                setAimDirection(player, direction);
                *reinterpret_cast<float*>(player + 0x6C0) = quickAimPitch(TheCamera.get_ptr());
                if (state != attacking && state != pointing)
                    setPointGunAt(player, nullptr);
                posingPlayer = player;
                return;
            }
        }
        if (posingPlayer == player && state == pointing && !*reinterpret_cast<CEntity**>(player + 0x508))
            clearPointGunAt(player);
        posingPlayer = nullptr;
    }

    CControllerState Compose(Context context, const XINPUT_GAMEPAD& g)
    {
        CControllerState s{};
        float lx, ly, rx, ry;
        Stick(g.sThumbLX, g.sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE, lx, ly);
        Stick(g.sThumbRX, g.sThumbRY, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE, rx, ry);
        auto vehicle = PlayerVehicle();
        bool hydraulics = vehicle && vehicle->GetModelIndex() == MI_VOODOO;
        // The PlayStation 2 sticks: down is positive. The game's cameras look up on a positive right stick Y; vehicles
        // keep the console's sign (the guns, flying)
        float look = RWGTA::PadOptions::LookUp(ry);
        s.LeftStickX = ToAxis(lx);
        s.LeftStickY = ToAxis(-ly);
        s.RightStickX = ToAxis(rx);
        s.RightStickY = ToAxis(context == Context::Frontend || hydraulics ? -ry : vehicle ? -look : look);

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

        if ((!hydraulics && FreeCameraTakesRightStick(context, vehicle)) || ModernAiming)
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
                s.LeftStickY = ToAxis(look);
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
        // The game shakes the pad only with its vibration preference on, which the PC menus don't show: the setting's
        const auto vibration = RWGTA::PadOptions::Vibration.load(std::memory_order_relaxed);
        FrontendMenuManager->m_PrefsUseVibration = vibration != 0;
        WORD motor = 0;
        if (pad.ShakeDur > 0)
        {
            pad.ShakeDur = int16_t(std::max<int64_t>(0, pad.ShakeDur - int64_t(elapsed)));
            if (pad.ShakeDur == 0)
                pad.ShakeFreq = 0;
            else if (vibration && PadIsLastDevice && !MenuActive())
                motor = WORD(pad.ShakeFreq * 257);
        }
        if (!XInput().setState)
            return;
        for (DWORD i = 0; i < XUSER_MAX_COUNT; ++i)
        {
            auto& slot = Slots[i];
            WORD target = int(i) == ActiveSlot ? motor : 0;
            if (!slot.connected || (slot.motors[0] == target && slot.motors[1] == target))
                continue;
            XINPUT_VIBRATION output{ target, target };
            if (XInput().setState(i, &output) == ERROR_SUCCESS)
                slot.motors[0] = slot.motors[1] = target;
        }
    }

    // The modern layout's aim: the game's mouse aiming, the right stick moving the mouse
    void UpdateModernAim(CPad& pad, Context context)
    {
        bool aim = false;
        if (PadIsLastDevice && context == Context::ModernFoot && Down(Current, LT) && !pad.DisablePlayerControls && !CCutsceneMgr::IsRunning())
        {
            auto weapon = CurrentWeapon();
            aim = !IsScopeWeapon(weapon) && weapon != WEAPON_DETONATOR;
        }
        ModernAiming = aim;
        if (!aim || !NewMouseState.get_ptr() || !MouseAccelHorizontal.get_ptr() || !MouseAccelVertical.get_ptr())
            return;

        float rx, ry;
        Stick(Current.sThumbRX, Current.sThumbRY, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE, rx, ry);
        const auto speed = RWGTA::PadOptions::CameraSpeed.load(std::memory_order_relaxed);
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
        mouse.y += curve(RWGTA::PadOptions::LookUp(ry)) * DEGTORAD(PitchRate) * speed * dt / perUnitY;
    }

    void UpdateCameraStyle(Context context)
    {
        bool standard = FrontendMenuManager->m_ControlMethod == 0;
        if (context == Context::Frontend)
        {
            UseMouse3rdPerson = standard;
            return;
        }
        const auto freeCamera = RWGTA::PadOptions::FreeCamera.load(std::memory_order_relaxed);
        bool orbit = freeCamera && !PlayerVehicle() && ActiveCameraMode() == CCam::MODE_FOLLOWPED;
        bool mouseView = PadIsLastDevice ? ModernAiming : standard && (!orbit || MouseAiming);
        if (orbit && mouseView && !*UseMouse3rdPerson.get_ptr())
        {
            // The native mouse camera measures its horizontal angle towards the camera, opposite to the orbit view.
            auto& cam = TheCamera->Cams[TheCamera->ActiveCam];
            cam.Beta = std::atan2(-cam.Front.y, -cam.Front.x);
            cam.Alpha = std::asin(std::clamp(cam.Front.z, -1.0f, 1.0f));
        }
        UseMouse3rdPerson = mouseView;
    }

    void SmoothAimHeading(SafetyHookContext& regs)
    {
        if (!MouseAiming && !ModernAiming)
            return;

        const auto* player = reinterpret_cast<const uint8_t*>(TheCamera->pTargetEntity);
        const auto current = *reinterpret_cast<const float*>(player + 0x374);
        const auto headingRate = *reinterpret_cast<const float*>(player + 0x37C);
        auto& heading = *reinterpret_cast<float*>(regs.esp + 0x38);
        const auto step = DEGTORAD(headingRate) * CTimer::ms_fTimeStep;
        const auto delta = std::remainder(heading - current, TWOPI);
        heading = step > 0.0f ? current + std::clamp(delta, -step, step) : current;
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
    void UpdateMenuButtons()
    {
        static RWGTA::MenuPadButtons buttons;
        auto& menu = *FrontendMenuManager.get_ptr();
        const WORD pressed = buttons.Update(Current, menu.m_bMenuActive);
        if (!menu.m_bMenuActive)
        {
            if ((pressed & XINPUT_GAMEPAD_START) && TheCamera->m_fFLOATingFade == 0.0f && !TheCamera->m_WideScreenOn)
                menu.m_bStartUpFrontEndRequested = true;
        }
        else if (!menu.m_bGameNotLoaded && ((pressed & XINPUT_GAMEPAD_START) ||
            ((pressed & XINPUT_GAMEPAD_B) && menu.m_nCurrScreen == 32)))
        {
            menu.m_bShutDownFrontEndRequested = true;
        }
    }

    void UpdatePadCheats(CPad& pad)
    {
        static constexpr RWGTA::PadCheat cheats[] =
        {
            { "3414LDRULDRU", "THUGSTOOLS" },
            { "3414LDRULDDL", "PROFESSIONALTOOLS" },
            { "3414LDRULDDD", "NUTTERTOOLS" },
            { "341CLDRULDRU", "ASPIRINE" },
            { "341XLDRULDRU", "PRECIOUSPROTECTION" },
            { "33C4LRLRLR", "YOUWONTTAKEMEALIVE" },
            { "33C4UDUDUD", "LEAVEMEALONE" },
            { "R2D3LL3121", "ICANTTAKEITANYMORE" },
            { "TURDS43", "BOOOOOORING" },
            { "TURD21S", "ONSPEED" },
            { "CC1CCC123TCT", "PANZER" },
            { "R2D22X31CL", "GETTHEREFAST" },
            { "D4D32L31LR", "THELASTRIDE" },
            { "4U2LL31CR", "ROCKANDROLLCAR" },
            { "C3C3LL31CR", "RUBBISHCAR" },
            { "C1U32X31CX", "BETTERTHANWALKING" },
            { "423124STCT21", "BIGBANG" },
            { "C1D2LX31RC", "AHAIRDRESSERSCAR" },
            { "C2U3LX31LC", "IWANTITPAINTEDBLACK" },
            { "R3U22L3133", "GREENLIGHT" },
            { "4X11222D", "ALOVELYDAY" },
            { "4X11222T", "APLEASANTDAY" },
            { "4X11222S", "ABITDRIEG" },
            { "4X11222X", "CANTSEEATHING" },
            { "4X11222C", "CATSANDDOGS" },
            { "43XTXTUD", "OURGODGIVENRIGHTTOBEARARMS" },
            { "URR1RUS2", "TRAVELINSTYLE" },
            { "D3C22X31LL", "GETTHEREQUICKLY" },
            { "DUUUX4322", "NOBODYLIKESME" },
            { "CX114XXCT", "FANNYMAGNET" },
            { "DLULX4321", "FIGHTFIGHTFIGHT" },
            { "T33L3141", "GRIPISEVERYTHING" },
            { "R4C32D13", "COMEFLYWITHME" },
            { "CC1S1SSS1TCT", "LIFEISPASSINGMEBY" },
            { "RRLU12LUDR", "STILLLIKEDRESSINGUP" },
            { "4CU1R3UST", "AIRSHIP" },
            { "4C32L3142", "MIAMITRAFFIC" },
            { "C2LX31X1", "LOOKLIKELANCE" },
            { "R1U21R31X3", "MYSONISALAWYER" },
            { "3C41R31X4", "ILOOKLIKEHILARY" },
            { "D1D2LX31XX", "ROCKANDROLLMAN" },
            { "3241R4LXS1", "WELOVEOURDICK" },
            { "R3U41R31RC", "ONEARMEDBANDIT" },
            { "C1C2LX31XX", "IDONTHAVETHEMONEYSONNY" },
            { "41U1R3RUCT", "FOXYLITTLETHING" },
            { "C4D3LR31X2", "IWANTBIGTITS" },
            { "1234D142", "CHEATSHAVEBEENCRACKED" },
            { "R4C32S34", "SEAWAYS" },
            { "R1C2LX311X", "CHICKSWITHGUNS" },
            { "3XTR4SUDS", "LOADSOFLITTLETHINGS" },
            { "3C4R12XXS3", "GETTHEREVERYFASTINDEED" },
            { "41CR13RUC4", "GETTHEREAMAZINGLYFAST" },
            { "4CU1R3RUST", "CHASESTAT" },
            { "T1T4S11", "WHEELSAREALLINEED" },
        };
        static RWGTA::PadCheats sequence;
        const bool blocked = MenuActive() || pad.DisablePlayerControls || CCutsceneMgr::IsRunning();
        const auto keyword = sequence.Update(Current, blocked ? -1 : ActiveSlot, cheats);
        if (keyword.empty())
            return;
        static auto appendKey = reinterpret_cast<void(__thiscall*)(CPad*, char)>(hook::pattern("53 56 57 55 83 EC 10 89 4C 24 0C BE 1C 00 00 00 8A 86").get_first());
        for (const char key : keyword)
            appendKey(&pad, key);
    }

    // Before CPad::Update of the first pad: the keyboard, mouse and DirectInput states are in place
    void BeforePadUpdate(CPad& pad)
    {
        bool padUsed = PollPads();
        UpdatePadCheats(pad);
        UpdateMenuButtons();
        bool kbmUsed = KeyboardOrMouseUsed(pad);
        if (padUsed)
            PadIsLastDevice = true;
        else if (kbmUsed || ActiveSlot < 0)
            PadIsLastDevice = false;

        auto context = CurrentContext();
        UpdateMode(pad, context);
        UpdateModernAim(pad, context);
        UpdateMouseAim(pad);
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

    const CPad& Player()
    {
        return *Pads.get_ptr();
    }

    // The right stick, -1..1 (up positive, after the invert setting), when the pad drives the game
    void RightStick(float& x, float& y)
    {
        x = y = 0.0f;
        if (!IsPadLastDevice() || MenuActive() || Player().DisablePlayerControls)
            return;
        Stick(Current.sThumbRX, Current.sThumbRY, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE, x, y);
        y = RWGTA::PadOptions::LookUp(y);
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

    // The player's weapon (eWeaponType)
    int PlayerWeapon()
    {
        return CurrentWeapon();
    }

    // The game's actions on the player's pad as its CPad functions read them: the configuration decides the button


    bool ControlsDisabled()
    {
        return Player().DisablePlayerControls != 0;
    }

    bool Fire()
    {
        auto& pad = Player();
        if (pad.DisablePlayerControls)
            return false;
        switch (pad.Mode)
        {
            case 0: case 1: return pad.NewState.Circle != 0;
            case 2: return pad.NewState.Cross != 0;
            case 3: return pad.NewState.RightShoulder1 != 0;
            default: return false;
        }
    }

    bool CameraBehind()
    {
        auto& pad = Player();
        if (pad.DisablePlayerControls)
            return false;
        switch (pad.Mode)
        {
            case 0: case 1: return pad.NewState.LeftShoulder1 != 0;
            case 2: return pad.NewState.Triangle != 0;
            case 3: return pad.NewState.Circle != 0;
            default: return false;
        }
    }

    bool LookLeft()
    {
        auto& pad = Player();
        return !pad.DisablePlayerControls && pad.NewState.LeftShoulder2 && !pad.NewState.RightShoulder2;
    }

    bool LookRight()
    {
        auto& pad = Player();
        return !pad.DisablePlayerControls && pad.NewState.RightShoulder2 && !pad.NewState.LeftShoulder2;
    }

    bool LookBehindForCar()
    {
        auto& pad = Player();
        return !pad.DisablePlayerControls && pad.NewState.LeftShoulder2 && pad.NewState.RightShoulder2;
    }

    bool LookBehindForPed()
    {
        auto& pad = Player();
        return !pad.DisablePlayerControls && pad.NewState.RightShock;
    }

    // The left stick's (or d-pad's) up and down in a vehicle, -128..127, down positive
    int16_t SteeringUpDown()
    {
        auto& pad = Player();
        if (pad.DisablePlayerControls)
            return 0;
        if (pad.Mode == 0 || pad.Mode == 2)
        {
            int16_t axis = pad.NewState.LeftStickY;
            int16_t dpad = int16_t((pad.NewState.DPadDown - pad.NewState.DPadUp) / 2);
            return std::abs(axis) > std::abs(dpad) ? axis : dpad;
        }
        return pad.NewState.LeftStickY;
    }

    // The mouse camera's sensitivity, radians per mouse unit at a field of view of 80
    void MouseAccel(float& horizontal, float& vertical)
    {
        horizontal = MouseAccelHorizontal.get_ptr() ? *MouseAccelHorizontal.get_ptr() : 0.0025f;
        vertical = MouseAccelVertical.get_ptr() ? *MouseAccelVertical.get_ptr() : 0.0015f;
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
            RWGTA::PadOptions::Install(FixSettings(), true);
            GInputLoaded = GetModuleHandleW(L"GInputVC.asi") != nullptr;
            if (GInputLoaded)
                return;
            if (!XInput().getState)
                return;

            auto weapon = hook::pattern("53 56 89 CB 57 55 83 EC 50 8B 6C 24 64 0F BE 93 04 05 00 00 8D 14 52 8B 84 D3 08 04 00 00 50 E8");
            weaponInfo = reinterpret_cast<decltype(weaponInfo)>(injector::GetBranchDestination(weapon.get_first(31)).as_int());
            setPointGunAt = reinterpret_cast<decltype(setPointGunAt)>(hook::pattern("53 89 CB 0F BE 93 04 05 00 00 56 55 8B 74 24 10 8D 14 52").get_first());
            clearPointGunAt = reinterpret_cast<decltype(clearPointGunAt)>(hook::pattern("53 56 55 89 CB E8 ? ? ? ? 89 D9 E8 ? ? ? ? 8A 83 4C 01 00 00 24 F7").get_first());
            setAimDirection = reinterpret_cast<decltype(setAimDirection)>(hook::pattern("53 89 CB 8A 83 4C 01 00 00 24 7F 0C 80 88 83 4C 01 00 00").get_first());
            setLookDirection = reinterpret_cast<decltype(setLookDirection)>(hook::pattern("A1 ? ? ? ? 8B 91 3C 05 00 00 39 C2 72 ? 80 7C 24 0C 00").get_first());
            quickAimPitch = reinterpret_cast<decltype(quickAimPitch)>(hook::pattern("83 EC 08 0F B6 41 76 D9 EE D9 EE D9 EE").get_first());
            processPlayerWeapon = safetyhook::create_inline(weapon.get_first(), ProcessAimedWeapon);

            // FollowPedWithMouse normally assigns the camera angle directly to the player.
            // Keep the camera on target while the player turns at their native heading rate.
            auto heading = hook::pattern("D9 83 68 01 00 00 D9 E0 D9 83 6C 01 00 00 D9 F3 "
                "D9 5C 24 38 D9 44 24 38 A1 ? ? ? ? DD D9 D9 90 74 03 00 00");
            static auto smoothHeading = safetyhook::create_mid(heading.get_first(20), SmoothAimHeading);

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
