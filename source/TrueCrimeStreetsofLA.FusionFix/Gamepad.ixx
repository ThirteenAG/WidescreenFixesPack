module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/PadInput.hpp"
#include <cmath>

export module Gamepad;

import ComVars;
import DeveloperOverlay;
import Window;
import FreeCamera;
import Cinema;

// The PC game builds a console controller state from keyboard and mouse once per update: action flags (held, newly
// pressed), menu buttons and four stick axes. It never reads a gamepad during play, so an XInput pad is added to the
// same state, with a console-style layout.
export namespace TrueCrimeGamepad
{
    struct State
    {
        uint32_t unknown;
        uint32_t held;          // action flags
        uint32_t menu;          // 0x100 confirm, 0x200 back, 0x20 map and pause menu, 0x10 start (skips scenes)
        uint32_t pressed;       // held flags new this update
        int32_t cameraX;        // pan, keyboard +-170
        int32_t cameraY;        // zoom, stored inverted
        int32_t moveX;          // keyboard +-125
        int32_t moveY;          // stored inverted (up is negative)
    };

    namespace Flag
    {
        constexpr uint32_t Jump = 0x11, Kick = 0x402, Punch = 0x804, Grab = 0x48, Frisk = 0x40, Arrest = 0x80;
        constexpr uint32_t Block = 0x20, FlashBadge = 0x200, Cover = 0x4000, Reload = 0x2000, Fire = 0x1000;
        constexpr uint32_t WarningShot = 0x8000100, Commandeer = 0x8000, CenterCamera = 0x10000;
        constexpr uint32_t NormalMode = 0x20000, FightMode = 0x40000, GunMode = 0x80000;
        constexpr uint32_t Accelerate = 0x100000, Brake = 0x200000, Handbrake = 0x400000, Horn = 0x800000, Siren = 0x1000000;
        constexpr uint32_t CarCamera = 0x2000000, RearView = 0x4000000, SkipTrack = 0x10000000;
    }

    inline SafetyHookInline UpdateHook;

    // Stick to the game's range with a radial dead zone
    inline std::pair<int, int> Stick(SHORT x, SHORT y, float deadZone, int range)
    {
        const float fx = float(x) / 32767.0f, fy = float(y) / 32767.0f;
        const float length = std::sqrt(fx * fx + fy * fy);
        const float zone = std::isfinite(deadZone) ? std::clamp(deadZone, 0.0f, 0.5f) : 0.25f;
        if (length <= zone)
            return { 0, 0 };
        const float scale = std::min(1.0f, (length - zone) / (1.0f - zone)) / length;
        return { int(std::lround(fx * scale * range)), int(std::lround(fy * scale * range)) };
    }

    inline void Merge(State& state, uint32_t previous)
    {
        XINPUT_STATE pad{};
        DWORD slot = 0;
        if (TrueCrimeOverlay::BlocksInput() || (TrueCrimeWindow::Window && GetForegroundWindow() != TrueCrimeWindow::Window) || !PadInput::Read(pad, slot))
            return;
        const auto& g = pad.Gamepad;
        const auto down = [&](WORD button) { return (g.wButtons & button) != 0; };
        const bool lt = g.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
        const bool rt = g.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
        auto [moveX, moveY] = Stick(g.sThumbLX, g.sThumbLY, FixSettings().Float("GAMEPAD.LeftStickDeadzone"), 125);
        auto [cameraX, cameraY] = Stick(g.sThumbRX, g.sThumbRY, FixSettings().Float("GAMEPAD.RightStickDeadzone"), 170);
        const float requestedSpeed = FixSettings().Float("MAIN.GamepadCameraSpeed");
        const float cameraSpeed = std::isfinite(requestedSpeed) ? std::clamp(requestedSpeed, 0.1f, 3.0f) : 1.0f;
        cameraX = std::clamp(int(std::lround(cameraX * cameraSpeed)), -170, 170);
        cameraY = std::clamp(int(std::lround(cameraY * cameraSpeed)), -170, 170);
        if (FixSettings().Bool("MAIN.InvertGamepadY"))
            cameraY = -cameraY;
        uint32_t held = 0, menu = 0;
        if (GameMode >= 2)
        {
            // Menus: d-pad or left stick to move, A confirm, B back
            if (down(XINPUT_GAMEPAD_DPAD_UP)) moveY = 125;
            if (down(XINPUT_GAMEPAD_DPAD_DOWN)) moveY = -125;
            if (down(XINPUT_GAMEPAD_DPAD_LEFT)) moveX = -125;
            if (down(XINPUT_GAMEPAD_DPAD_RIGHT)) moveX = 125;
            if (down(XINPUT_GAMEPAD_A)) menu |= 0x100;
            if (down(XINPUT_GAMEPAD_B)) menu |= 0x220;
            if (down(XINPUT_GAMEPAD_START)) menu |= 0x10;
        }
        else if (PlayerObject.get() && PlayerObject.get()[9] == 5)
        {
            // Driving: right trigger accelerates, left trigger brakes and reverses
            if (rt || lt)
                moveY = int(std::lround((float(g.bRightTrigger) - float(g.bLeftTrigger)) / 255.0f * 125.0f));
            // The car reads the pedals as flags, with the stick for their strength
            if (moveY > 30) held |= Flag::Accelerate;
            if (moveY < -30) held |= Flag::Brake;
            if (down(XINPUT_GAMEPAD_A)) held |= Flag::Handbrake;
            if (down(XINPUT_GAMEPAD_B)) held |= Flag::Horn;
            if (down(XINPUT_GAMEPAD_X)) held |= Flag::Siren;
            if (down(XINPUT_GAMEPAD_Y)) held |= Flag::Commandeer;
            if (down(XINPUT_GAMEPAD_RIGHT_SHOULDER)) held |= Flag::Fire;
            if (down(XINPUT_GAMEPAD_LEFT_SHOULDER)) held |= Flag::RearView;
            if (down(XINPUT_GAMEPAD_RIGHT_THUMB)) held |= Flag::CarCamera;
            if (down(XINPUT_GAMEPAD_DPAD_RIGHT)) held |= Flag::SkipTrack;
            if (down(XINPUT_GAMEPAD_DPAD_UP)) held |= Flag::FlashBadge;
        }
        else
        {
            if (down(XINPUT_GAMEPAD_A)) held |= Flag::Jump;
            if (down(XINPUT_GAMEPAD_B)) held |= Flag::Kick;
            if (down(XINPUT_GAMEPAD_X)) held |= Flag::Punch;
            if (down(XINPUT_GAMEPAD_Y)) held |= Flag::Commandeer;
            if (rt) held |= Flag::Fire;
            if (lt) held |= Flag::Block;
            if (down(XINPUT_GAMEPAD_RIGHT_SHOULDER)) held |= Flag::Grab;
            if (down(XINPUT_GAMEPAD_LEFT_SHOULDER)) held |= Flag::Cover;
            if (down(XINPUT_GAMEPAD_LEFT_THUMB)) held |= Flag::Reload;
            if (down(XINPUT_GAMEPAD_RIGHT_THUMB)) held |= Flag::CenterCamera;
            if (down(XINPUT_GAMEPAD_DPAD_UP)) held |= Flag::FlashBadge;
            if (down(XINPUT_GAMEPAD_DPAD_DOWN)) held |= Flag::Arrest;
            if (down(XINPUT_GAMEPAD_DPAD_LEFT)) held |= Flag::FightMode;
            if (down(XINPUT_GAMEPAD_DPAD_RIGHT)) held |= Flag::GunMode;
        }
        if (GameMode < 2)
        {
            // Start: skip a story scene, otherwise the pause menu (as Escape)
            if (down(XINPUT_GAMEPAD_START)) menu |= TrueCrimeCinema::Active() ? 0x10 : 0x220;
            if (down(XINPUT_GAMEPAD_BACK)) menu |= 0x20;
        }
        state.held |= held;
        state.menu |= menu;
        state.pressed = state.held & ~previous;
        // The larger deflection of keyboard/mouse and pad wins
        const auto take = [](int32_t& axis, int value)
        {
            if (std::abs(value) > std::abs(axis))
                axis = value;
        };
        take(state.moveX, moveX);
        if (std::abs(moveY) > std::abs(~state.moveY))
            state.moveY = ~moveY;
        if (cameraX || cameraY)
            TrueCrimeFreeCamera::StickMoved.store(true, std::memory_order_relaxed);
        take(state.cameraX, cameraX);
        if (std::abs(cameraY) > std::abs(~state.cameraY))
            state.cameraY = ~(-cameraY);
    }

    inline int __fastcall Update(State* state, void*)
    {
        const uint32_t previous = state->held;
        const int result = UpdateHook.thiscall<int>(state);
        Merge(*state, previous);
        return result;
    }

    inline void Init()
    {
        auto input = hook::pattern("8B 0D ? ? ? ? 8B ? ? ? ? ? 32 DB");
        PlayerObject.SetAddress(*input.get_first<uint8_t**>(2));
        GameMode.SetAddress(*input.get_first<int*>(8));
        // The update through the game's controller state object
        auto update = hook::pattern("B9 ? ? ? ? E9 ? ? ? ? CC CC CC CC CC CC 8B 54 24 04");
        HeldFlags.SetAddress(reinterpret_cast<uint32_t*>(*update.get_first<uintptr_t>(1) + 4));
        UpdateHook = safetyhook::create_inline(injector::GetBranchDestination(update.get_first(5)).as_int(), Update);
    }
}
