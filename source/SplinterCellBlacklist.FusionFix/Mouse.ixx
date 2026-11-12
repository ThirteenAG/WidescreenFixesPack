module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module Mouse;

import ComVars;
import DeveloperOverlay;
import Camera;

// Mouse look comes from WM_MOUSEMOVE (UWindowsViewport's WndProc, sub_1AF55C0): the move from the middle of the window is sent as an
// IK_MouseX/IK_MouseY axis input (UWindowsViewport::CauseInputEvent, virtual +78h, action 6) and the cursor is put back in the middle.
// - The input's Axis command assigns the axis (*Axis = Speed * Delta) instead of adding to it, so when there's more than one WM_MOUSEMOVE
//   before a frame (fast moves, high polling rate mice) the camera only gets the last one and the rest of the movement is lost.
// - The cursor moves with the Windows pointer speed and acceleration (Enhance pointer precision), fast moves are multiplied.
// While the game turns the cursor moves into mouse look, the raw mouse movement (WM_INPUT) is sent instead, once per frame from the
// viewport's input update (sub_1AF2880). The cursor is still put back in the middle by the game and the menus use it as before.
namespace Mouse
{
    constexpr int IK_MouseX = 0xE4;
    constexpr int IK_MouseY = 0xE5;
    constexpr int IST_Axis = 6;

    // raw movement since the last frame
    LONG rawX = 0;
    LONG rawY = 0;

    // the last time the game sent a cursor move as mouse look
    ULONGLONG lookTime = 0;
    bool lookThisFrame = false;

    using CauseInputEventFn = int(__fastcall*)(uintptr_t viewport, void* edx, uintptr_t input, int key, int action, float delta, int controller, int a7);
    CauseInputEventFn CauseInputEvent = nullptr;

    bool Enabled()
    {
        return FixSettings().Bool("MAIN.RawInputMouse") && WindowHandle && GetForegroundWindow() == WindowHandle && !DeveloperOverlay::BlocksInput();
    }

    int __fastcall CauseInputEventHook(uintptr_t viewport, void* edx, uintptr_t input, int key, int action, float delta, int controller, int a7)
    {
        if (DeveloperOverlay::BlocksInput())
        {
            DeveloperOverlay::MouseInput(key, action);
            return 0;
        }
        if (Enabled() && action == IST_Axis && (key == IK_MouseX || key == IK_MouseY))
        {
            lookThisFrame = true;
            return 1;
        }
        return CauseInputEvent(viewport, edx, input, key, action, delta, controller, a7);
    }

    BOOL WINAPI SetCursorPosHook(int x, int y)
    {
        return DeveloperOverlay::BlocksInput() ? TRUE : SetCursorPos(x, y);
    }

    WNDPROC GameWndProc = nullptr;
    LRESULT CALLBACK WndProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        if (message == WM_INPUT && Enabled())
        {
            RAWINPUT input = {};
            UINT size = sizeof(input);
            if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, &input, &size, sizeof(RAWINPUTHEADER)) != UINT(-1) &&
                input.header.dwType == RIM_TYPEMOUSE && (input.data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE) == 0)
            {
                rawX += input.data.mouse.lLastX;
                rawY += input.data.mouse.lLastY;
            }
        }
        return CallWindowProcA(GameWndProc, window, message, wParam, lParam);
    }

    // The raw mouse input is registered for the game window the first time there's one
    void Register()
    {
        static HWND registered = nullptr;
        if (!WindowHandle || registered == WindowHandle || !IsWindow(WindowHandle))
            return;
        RAWINPUTDEVICE device = { 0x01, 0x02, 0, WindowHandle }; // generic desktop, mouse; WM_MOUSEMOVE is still sent
        if (!RegisterRawInputDevices(&device, 1, sizeof(device)))
            return;
        GameWndProc = reinterpret_cast<WNDPROC>(SetWindowLongPtrA(WindowHandle, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&WndProc)));
        registered = WindowHandle;
    }
}

// Turning while moving: the move model (CMoveModel, sub_1D00930) turns the character toward the movement direction by at most
// angular speed * frame time a frame ([ebp-1Ch], up to 179.9 degrees), the angular speed is the current animation's or one set by the game.
// Small direction changes (the camera turned a little) take a while, big ones play a turn animation.
// The model's animation object (+12ECh, sub_6854C0) has the pawn at +BC8h.
namespace TurnSpeed
{
    float multiplier = 1.0f;

    bool IsLocalPlayerModel(uintptr_t model)
    {
        auto controller = *reinterpret_cast<uintptr_t*>(model + 0x12EC);
        if (!controller)
            return false;
        uintptr_t animation = 0;
        if (auto a = *reinterpret_cast<uintptr_t*>(controller + 0x1F0))
            animation = *reinterpret_cast<uintptr_t*>(a + 0xC0);
        else if (auto b = *reinterpret_cast<uintptr_t*>(controller + 0x1F8))
            animation = *reinterpret_cast<uintptr_t*>(b + 0x60);
        if (!animation)
            return false;
        auto pawn = *reinterpret_cast<uintptr_t*>(animation + 0xBC8);
        // APawn::IsLocalPlayer
        return pawn && (*reinterpret_cast<uint32_t*>(pawn + 0x5C) & 0x800000) != 0 && *reinterpret_cast<uint8_t*>(pawn + 0x4D) != 2;
    }
}

// Shoulder swap: the game swaps the camera to the other shoulder only in cover and with a human shield (its own camera modes),
// the controller's left stick click alias (SwitchHandness) does nothing. The swap key puts the camera over Sam's left shoulder
// on foot: CameraImpl::New_ComputeIdealLocation (sub_D4B270) gets the mode's ideal camera offset from ComputeIdealOffset (sub_D46D80)
// in the camera's space (y = to the right) and its side is mirrored. Each camera mode entity (settings at +1Ch, kind at +8, mode at +Ch)
// computes its own location, so going into cover or a human shield blends to the game's own camera as before.
namespace ShoulderSwap
{
    constexpr float SwapTime = 0.25f;

    bool swapped = false;
    float side = 1.0f;
    LARGE_INTEGER last = {};
    LARGE_INTEGER frequency = {};
    bool keyDown = false;

    // modes on foot that have the camera over the right shoulder (CameraImpl::CameraMode2String, sub_D81F10)
    bool Mirrored(int mode)
    {
        switch (mode)
        {
            case 2: case 3: case 4: case 5: case 6: case 7: case 8:   // Walk, _Zoom, _Scope, _HighProfile, _SurvivalSuit, _Zoom_SurvivalSuit, _Arc
            case 9:                                                   // GunPoint
            case 10: case 11: case 12: case 19: case 20: case 21: case 22: // Crouch, _Zoom, _Scope, _HighProfile, _SurvivalSuit, _Zoom_SurvivalSuit, _Arc
            case 30: case 31: case 32: case 33:                       // SlideStand, SlideCrouch (+ _HighProfile)
            case 90: case 91: case 92: case 93: case 94: case 95:     // Run, _Scope, _HighProfile, _HighProfile_AlternateControl, _SurvivalSuit, _HighProfile_SurvivalSuit
            case 96: case 97:                                         // Combat, Combat_Zoom
            case 365: case 366:                                       // Disguised, Disguised_Zoom
            case 367: case 368: case 369: case 370: case 371: case 372: // Hub, Hub_Zoom, HubClose, HubClose_Zoom, Hub_HighProfile, HubClose_HighProfile
                return true;
            default:
                return false;
        }
    }

    bool MirroredEntity(uintptr_t entity)
    {
        return entity && *reinterpret_cast<int*>(entity + 8) == 2 && Mirrored(*reinterpret_cast<int*>(entity + 0xC));
    }

    // a key name of the settings file (A, F5, Space, Mouse.Middle, Mouse.X1) or a virtual-key code (0x05)
    int KeyCode(std::string_view key)
    {
        if (key.empty())
            return 0;
        if (key.size() > 2 && key[0] == '0' && (key[1] == 'x' || key[1] == 'X'))
            return std::strtol(std::string(key).c_str(), nullptr, 16);
        if (key.size() == 1 && std::isalnum(uint8_t(key[0])))
            return std::toupper(uint8_t(key[0]));
        if ((key[0] == 'F' || key[0] == 'f') && key.size() <= 3 && std::all_of(key.begin() + 1, key.end(), ::isdigit))
        {
            auto n = std::atoi(std::string(key.substr(1)).c_str());
            return n >= 1 && n <= 24 ? VK_F1 + n - 1 : 0;
        }
        static const std::pair<std::string_view, int> Names[] =
        {
            { "Space", VK_SPACE }, { "Tab", VK_TAB }, { "Enter", VK_RETURN }, { "Backspace", VK_BACK }, { "Insert", VK_INSERT },
            { "Delete", VK_DELETE }, { "Home", VK_HOME }, { "End", VK_END }, { "PageUp", VK_PRIOR }, { "PageDown", VK_NEXT },
            { "Pause", VK_PAUSE }, { "ScrollLock", VK_SCROLL }, { "CapsLock", VK_CAPITAL }, { "Shift", VK_SHIFT },
            { "Control", VK_CONTROL }, { "Ctrl", VK_CONTROL }, { "Alt", VK_MENU }, { "Left", VK_LEFT }, { "Right", VK_RIGHT },
            { "Up", VK_UP }, { "Down", VK_DOWN }, { "Mouse.Middle", VK_MBUTTON }, { "Mouse.X1", VK_XBUTTON1 },
            { "Mouse.X2", VK_XBUTTON2 }, { "Mouse.Button4", VK_XBUTTON1 }, { "Mouse.Button5", VK_XBUTTON2 },
        };
        for (auto& [name, code] : Names)
        {
            if (name.size() == key.size() && _strnicmp(name.data(), key.data(), key.size()) == 0)
                return code;
        }
        return 0;
    }

    int Key()
    {
        static std::string name;
        static int code = 0;
        const auto& current = FixSettings().String("MAIN.SwapShoulderKey");
        if (current != name)
        {
            name = current;
            code = KeyCode(name);
        }
        return code;
    }

    // once a frame or more: the key toggles the side while the camera is on foot, the side moves to it over SwapTime
    void Update(uintptr_t camera)
    {
        const auto key = Key();
        const bool down = key && WindowHandle && GetForegroundWindow() == WindowHandle && (GetAsyncKeyState(key) & 0x8000) != 0;
        if (down && !keyDown && !NativeMenuOpen && !DeveloperOverlay::BlocksInput() &&
            MirroredEntity(*reinterpret_cast<uintptr_t*>(camera + 0x2C)))
            swapped = !swapped;
        keyDown = down;

        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        const float elapsed = last.QuadPart ? std::min(float(now.QuadPart - last.QuadPart) / float(frequency.QuadPart), 0.1f) : 0.0f;
        last = now;
        const float target = swapped ? -1.0f : 1.0f;
        const float step = 2.0f * elapsed / SwapTime;
        side = side < target ? std::min(side + step, target) : std::max(side - step, target);
    }
}

export void InitMouse()
{
    IATHook::Replace(GetModuleHandleW(nullptr), "USER32.DLL",
        std::forward_as_tuple("SetCursorPos", Mouse::SetCursorPosHook));
    auto& settings = FixSettings();
    TurnSpeed::multiplier = std::clamp(settings.Float("MAIN.TurnSpeed"), 0.5f, 10.0f);

    if (TurnSpeed::multiplier != 1.0f)
    {
        auto pattern = hook::pattern("E8 ? ? ? ? D9 5D E0 D9 45 E0 D8 4D DC D9 5D E4 D9 05");
        static auto TurnPerFrame = safetyhook::create_mid(pattern.get_first(17), [](SafetyHookContext& regs)
        {
            if (TurnSpeed::IsLocalPlayerModel(regs.esi))
                *reinterpret_cast<float*>(regs.ebp - 0x1C) *= TurnSpeed::multiplier;
        });
    }

    // CameraImpl mouse input (sub_D8E190) is clamped to +-500 * delta time
    {
        auto pattern = hook::pattern("0F 28 CA EB 13 0F 57 ED");
        injector::MakeNOP(pattern.get_first(0), 3, true);

        pattern = hook::pattern("0F 28 C8 F3 0F 10 07");
        injector::MakeNOP(pattern.get_first(0), 3, true);

        pattern = hook::pattern("0F 28 C2 EB 15 0F 57 D2");
        injector::MakeNOP(pattern.get_first(0), 3, true);

        pattern = hook::pattern("0F 28 C3 D9 86");
        injector::MakeNOP(pattern.get_first(0), 3, true);
    }

    // UWindowsViewport vtable, set in the constructor (sub_1AF50F0)
    auto pattern = hook::pattern("C7 06 ? ? ? ? C7 46 28 ? ? ? ? C7 46 2C ? ? ? ? 89 9E 30 08 00 00");
    auto vtable = *pattern.get_first<uintptr_t*>(2);
    Mouse::CauseInputEvent = reinterpret_cast<Mouse::CauseInputEventFn>(vtable[0x78 / 4]);
    injector::WriteMemory(&vtable[0x78 / 4], &Mouse::CauseInputEventHook, true);

    // UWindowsViewport input update, after the DirectInput mouse and before the keys, esi is the viewport.
    // The input is the viewport's +58h, as in WndProc.
    pattern = hook::pattern("83 BE 4C 08 00 00 00 74 10 66 C7 85 FD FE FF FF");
    static auto MouseFrame = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        using namespace Mouse;
        GameViewport = regs.esi;
        Register();

        static bool wasEnabled = false;
        static LONG lastX = 0, lastY = 0;
        const bool enabled = Enabled();
        if (!enabled || enabled != wasEnabled)
        {
            rawX = rawY = lastX = lastY = 0;
            lookTime = 0;
            lookThisFrame = false;
            wasEnabled = enabled;
            return;
        }

        // mouse look stays on a little after the last cursor move, the raw input can come before or after the cursor in a frame
        auto now = GetTickCount64();
        if (std::exchange(lookThisFrame, false))
            lookTime = now;
        // the movement of the frame before mouse look turns on is kept for it
        auto x = std::exchange(rawX, 0);
        auto y = std::exchange(rawY, 0);
        if (!lookTime || now - lookTime > 250)
        {
            lastX = x;
            lastY = y;
            return;
        }
        x += std::exchange(lastX, 0);
        y += std::exchange(lastY, 0);

        auto viewport = regs.esi;
        auto input = *reinterpret_cast<uintptr_t*>(viewport + 0x58);
        if (x)
            CauseInputEvent(viewport, nullptr, input, IK_MouseX, IST_Axis, static_cast<float>(x), 0, 0);
        if (y)
            CauseInputEvent(viewport, nullptr, input, IK_MouseY, IST_Axis, static_cast<float>(y), 0, 0);
    });

    // CameraImpl::New_ComputeIdealLocation right after ComputeIdealOffset: esi is the mode's settings, the offset is at ebp-10h,
    // the camera at ebp-4
    QueryPerformanceFrequency(&ShoulderSwap::frequency);
    pattern = hook::pattern("8B 4D FC 57 8D 81 ? ? 00 00 50 56 8D 55 D8 52 E8");
    static auto IdealOffset = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        using namespace ShoulderSwap;
        const auto camera = *reinterpret_cast<uintptr_t*>(regs.ebp - 4);
        if (!IsFirstPlayerCamera(camera))
            return;
        Update(camera);
        if (side != 1.0f && MirroredEntity(regs.esi - 0x1C))
            *reinterpret_cast<float*>(regs.ebp - 0x10 + 4) *= side;
    });
}
