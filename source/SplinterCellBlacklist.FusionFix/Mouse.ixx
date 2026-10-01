module;

#include <stdafx.h>

export module Mouse;

import ComVars;

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

    int __fastcall CauseInputEventHook(uintptr_t viewport, void* edx, uintptr_t input, int key, int action, float delta, int controller, int a7)
    {
        if (action == IST_Axis && (key == IK_MouseX || key == IK_MouseY))
        {
            lookThisFrame = true;
            return 1;
        }
        return CauseInputEvent(viewport, edx, input, key, action, delta, controller, a7);
    }

    WNDPROC GameWndProc = nullptr;
    LRESULT CALLBACK WndProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        if (message == WM_INPUT)
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
        __try
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
        __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
    }
}

export void InitMouse()
{
    CIniReader iniReader("");
    auto bDisableNegativeMouseAcceleration = iniReader.ReadInteger("MAIN", "DisableNegativeMouseAcceleration", 1) != 0;
    TurnSpeed::multiplier = std::clamp(iniReader.ReadFloat("MAIN", "TurnSpeed", 1.0f), 0.5f, 10.0f);

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
    if (bDisableNegativeMouseAcceleration)
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

        // mouse look stays on a little after the last cursor move, the raw input can come before or after the cursor in a frame
        auto now = GetTickCount64();
        if (std::exchange(lookThisFrame, false))
            lookTime = now;
        // the movement of the frame before mouse look turns on is kept for it
        static LONG lastX = 0, lastY = 0;
        auto x = std::exchange(rawX, 0);
        auto y = std::exchange(rawY, 0);
        if (now - lookTime > 250)
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
}
