module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/BufferedRawMouse.hpp"
#include <commctrl.h>
#include <optional>

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
    BufferedRawMouse movement;
    std::atomic<bool> rawAllowed{ true };
    std::atomic<HWND> target = nullptr;
    HWND inputWindow = nullptr;
    bool installed = false;
    std::optional<RAWINPUTDEVICE> previous;
    HWND candidateWindow = nullptr;
    ULONGLONG nextAttempt = 0;

    // the last time the game sent a cursor move as mouse look
    ULONGLONG lookTime = 0;
    bool lookThisFrame = false;
    bool usedNativeAxes = false;

    using CauseInputEventFn = int(__fastcall*)(uintptr_t viewport, void* edx, uintptr_t input, int key, int action, float delta, int controller, int a7);
    CauseInputEventFn CauseInputEvent = nullptr;

    bool Enabled()
    {
        const auto window = target.load();
        return window && window == WindowHandle && GetForegroundWindow() == window && !DeveloperOverlay::BlocksInput();
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
            if (!usedNativeAxes && movement.HasRelative())
                return 1;
            usedNativeAxes = true;
        }
        return CauseInputEvent(viewport, edx, input, key, action, delta, controller, a7);
    }

    BOOL WINAPI SetCursorPosHook(int x, int y)
    {
        return DeveloperOverlay::BlocksInput() ? TRUE : SetCursorPos(x, y);
    }

    // The WM_MOUSEMOVE handler puts the cursor back to the window center after every move (sub_1AF55C0), each SetCursorPos makes
    // Windows queue a new move and costs the game thread time, with a high polling rate mouse that's on every pass of the message loop.
    // During raw mouse look the cursor's movement isn't used (the game's axis input is dropped, UGameEngine::MouseDelta only reads
    // the buttons): it only has to stay in the window, so it's put back once it's a quarter of the window away from the center.
    BOOL WINAPI RecenterCursor(int x, int y)
    {
        if (Enabled() && movement.HasRelative() && lookTime && GetTickCount64() - lookTime <= 250)
        {
            POINT cursor;
            RECT client;
            if (GetCursorPos(&cursor) && GetClientRect(WindowHandle, &client) &&
                std::abs(cursor.x - x) < client.right / 4 && std::abs(cursor.y - y) < client.bottom / 4)
                return TRUE;
        }
        return SetCursorPosHook(x, y);
    }

    bool Registration(std::optional<RAWINPUTDEVICE>& mouse)
    {
        UINT count = 0;
        if (GetRegisteredRawInputDevices(nullptr, &count, sizeof(RAWINPUTDEVICE)) == UINT(-1))
            return false;
        std::vector<RAWINPUTDEVICE> devices(count);
        if (count)
        {
            count = GetRegisteredRawInputDevices(devices.data(), &count, sizeof(RAWINPUTDEVICE));
            if (count == UINT(-1))
                return false;
        }
        mouse.reset();
        for (UINT i = 0; i < count; ++i)
        {
            if (devices[i].usUsagePage == 1 && devices[i].usUsage == 2)
            {
                mouse = devices[i];
                break;
            }
        }
        return true;
    }

    bool Owns(const std::optional<RAWINPUTDEVICE>& mouse)
    {
        return mouse && mouse->hwndTarget == inputWindow && mouse->dwFlags == RIDEV_INPUTSINK;
    }

    void Release()
    {
        if (target.exchange(nullptr))
            movement.Clear();
        if (!installed)
            return;

        std::optional<RAWINPUTDEVICE> current;
        if (!Registration(current))
            return;
        if (Owns(current))
        {
            const auto restore = previous.value_or(RAWINPUTDEVICE{ 1, 2, RIDEV_REMOVE, nullptr });
            if (!RegisterRawInputDevices(&restore, 1, sizeof(restore)))
                return;
        }
        installed = false;
        previous.reset();
    }

    LRESULT CALLBACK InputProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        if (message == WM_INPUT)
        {
            movement.Read(reinterpret_cast<HRAWINPUT>(lParam), []()
            {
                const auto game = target.load();
                return game && GetForegroundWindow() == game;
            });
        }
        return DefWindowProcW(window, message, wParam, lParam);
    }

    LRESULT CALLBACK GameProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR)
    {
        if (window == candidateWindow)
        {
            if (message == WM_KILLFOCUS || (message == WM_ACTIVATEAPP && !wParam))
                movement.Clear();
            if (message == WM_NCDESTROY)
            {
                Release();
                candidateWindow = nullptr;
            }
        }
        if (message == WM_NCDESTROY)
            RemoveWindowSubclass(window, GameProc, id);
        return DefSubclassProc(window, message, wParam, lParam);
    }

    void Register()
    {
        const auto game = rawAllowed.load() && IsWindow(WindowHandle) ? WindowHandle : nullptr;
        std::optional<RAWINPUTDEVICE> current;
        const auto now = GetTickCount64();
        if (candidateWindow != game)
        {
            Release();
            if (candidateWindow)
                RemoveWindowSubclass(candidateWindow, GameProc, 0);
            candidateWindow = game;
            nextAttempt = 0;
        }
        // A failed restore must keep the previous registration until it can be retried.
        if (installed && !target.load())
        {
            if (now < nextAttempt)
                return;
            nextAttempt = now + 500;
            Release();
            if (installed)
                return;
        }
        if (!game)
            return;
        if (target.load())
        {
            if (!Registration(current) || Owns(current))
                return;
            Release();
        }
        if (now < nextAttempt)
            return;
        nextAttempt = now + 500;
        if (!SetWindowSubclass(game, GameProc, 0, 0))
            return;
        if (!Registration(current) || (current && (current->hwndTarget || current->dwFlags)))
            return;
        if (!inputWindow)
            inputWindow = CreateRawInputWindow(L"BlacklistRawMouse", InputProc);
        if (!inputWindow)
            return;
        const RAWINPUTDEVICE device{ 1, 2, RIDEV_INPUTSINK, inputWindow };
        if (RegisterRawInputDevices(&device, 1, sizeof(device)))
        {
            previous = current;
            installed = true;
            target.store(game);
        }
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
    std::atomic<int> binding{ 0 };
    int previousKey = 0;
    HWND previousWindow = nullptr;

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

    void RefreshBinding()
    {
        static std::mutex mutex;
        std::lock_guard lock(mutex);
        const int key = KeyCode(FixSettings().Get("MAIN.SwapShoulderKey"));
        if (binding.exchange(key) != key)
            ResetShoulderKey.store(true);
    }

    // once a frame or more: the key toggles the side while the camera is on foot, the side moves to it over SwapTime
    void Update(uintptr_t camera)
    {
        const bool reset = ResetShoulderKey.exchange(false);
        const int key = binding.load();
        const auto window = GetForegroundWindow() == WindowHandle ? WindowHandle : nullptr;
        const bool down = key && window && (GetAsyncKeyState(key) & 0x8000) != 0;
        if (!reset && window == previousWindow && key == previousKey && down && !keyDown && !NativeMenuOpen && !DeveloperOverlay::BlocksInput() &&
            MirroredEntity(*reinterpret_cast<uintptr_t*>(camera + 0x2C)))
            swapped = !swapped;
        previousWindow = window;
        previousKey = key;
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
    // The main loop empties the message queue each frame and the viewport recenters the cursor on every WM_MOUSEMOVE: with a high
    // polling rate mouse each pass would get a new one and the frame wouldn't end
    MessagePumpGuard::Install(GetModuleHandleW(nullptr));
    {
        auto pattern = hook::pattern("51 52 FF 15 ? ? ? ? 83 BD ? ? ? ? 00 74 5E");
        injector::MakeCALL(pattern.get_first(2), Mouse::RecenterCursor, true);
        injector::MakeNOP(pattern.get_first(7), 1, true);
    }
    auto& settings = FixSettings();
    auto publishRaw = []()
    {
        static std::mutex mutex;
        std::lock_guard lock(mutex);
        Mouse::rawAllowed.store(FixSettings().Get("MAIN.RawInputMouse") == "1");
    };
    settings.OnChange("MAIN.RawInputMouse", publishRaw);
    publishRaw();
    settings.OnChange("MAIN.SwapShoulderKey", ShoulderSwap::RefreshBinding);
    ShoulderSwap::RefreshBinding();
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
        static int64_t lastX = 0, lastY = 0;
        const bool enabled = Enabled();
        if (!enabled || enabled != wasEnabled)
        {
            movement.Clear();
            lastX = lastY = 0;
            lookTime = 0;
            lookThisFrame = false;
            usedNativeAxes = false;
            wasEnabled = enabled;
            return;
        }

        // mouse look stays on a little after the last cursor move, the raw input can come before or after the cursor in a frame
        auto now = GetTickCount64();
        if (std::exchange(lookThisFrame, false))
            lookTime = now;
        // the movement of the frame before mouse look turns on is kept for it
        auto [x, y, relative] = movement.Take();
        if (std::exchange(usedNativeAxes, false) || !relative)
        {
            lastX = lastY = 0;
            return;
        }
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
        if (!binding.load(std::memory_order_relaxed) && !swapped && side == 1.0f)
            return;
        const auto camera = *reinterpret_cast<uintptr_t*>(regs.ebp - 4);
        if (!IsFirstPlayerCamera(camera))
            return;
        Update(camera);
        if (side != 1.0f && MirroredEntity(regs.esi - 0x1C))
            *reinterpret_cast<float*>(regs.ebp - 0x10 + 4) *= side;
    });
}
