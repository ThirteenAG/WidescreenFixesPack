#pragma once

#include <windows.h>
#include <vector>
#include <atomic>
#include <thread>
#include <algorithm>
#include <cmath>
#include <hidusage.h>

#define WM_RAWINPUTMOUSE (WM_APP + 1000)

// Raw mouse input is read on its own thread (a message-only window), a high polling rate mouse sends thousands of WM_INPUT messages
// a second, that would flood the game's message queue. The game window gets one WM_RAWINPUTMOUSE until it handled it.
template<typename T = int16_t>
class RawInputHandler
{
public:
    static inline std::atomic<T> RawMouseCursorX = 0;
    static inline std::atomic<T> RawMouseCursorY = 0;
    static inline std::atomic<T> RawMouseDeltaX = 0;
    static inline std::atomic<T> RawMouseDeltaY = 0;

    static void RegisterRawInput(HWND hWnd, float sensitivity = 1.0f, bool bUseRawData = false)
    {
        Sensitivity = sensitivity;
        UseRawData = bUseRawData;
        SystemParametersInfo(SPI_GETMOUSE, 0, MouseAcceleration, 0);
        SystemParametersInfo(SPI_GETMOUSESPEED, 0, &MouseSpeed, 0);
        GameWindow = hWnd;
        DefaultWndProc = (WNDPROC)SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)GameWndProc);
        std::thread(InputThread).detach();
    }

    static void OnResChange()
    {
        RECT windowRect;
        GetWindowRect(GameWindow, &windowRect);
        RawMouseCursorX = static_cast<T>((windowRect.right - windowRect.left) / 2 + windowRect.left);
        RawMouseCursorY = static_cast<T>((windowRect.bottom - windowRect.top) / 2 + windowRect.top);
    }

    static void SetSensitivity(float sensitivity)
    {
        Sensitivity = sensitivity;
    }

private:
    static inline HWND GameWindow = nullptr;
    static inline WNDPROC DefaultWndProc;
    static inline std::atomic<bool> Pending = false; // WM_RAWINPUTMOUSE posted and not handled yet
    static inline int MouseAcceleration[3] = { 0 };  // [0]=threshold1, [1]=threshold2, [2]=level (0=off, 1 or 2=on with varying strength)
    static inline int MouseSpeed = 10; // Default to 10 if query fails
    static inline std::atomic<float> Sensitivity = 1.0f;
    static inline bool UseRawData = false;

    static LRESULT CALLBACK GameWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        if (uMsg == WM_RAWINPUTMOUSE)
            Pending = false;
        return CallWindowProc(DefaultWndProc, hWnd, uMsg, wParam, lParam);
    }

    static void InputThread()
    {
        WNDCLASSEXW wc = { sizeof(wc) };
        wc.lpfnWndProc = InputWndProc;
        wc.hInstance = GetModuleHandleW(nullptr);
        wc.lpszClassName = L"WFPRawInputHandler";
        RegisterClassExW(&wc);

        auto hWnd = CreateWindowExW(0, wc.lpszClassName, L"", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, wc.hInstance, nullptr);
        RAWINPUTDEVICE rid = { HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_MOUSE, RIDEV_INPUTSINK, hWnd };
        if (!hWnd || !RegisterRawInputDevices(&rid, 1, sizeof(rid)))
            return;

        MSG msg;
        while (GetMessageW(&msg, nullptr, 0, 0) > 0)
            DispatchMessageW(&msg);
    }

    static LRESULT CALLBACK InputWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        static float SubpixelX = 0.0f;          // Carry-over fractions for X
        static float SubpixelY = 0.0f;          // Carry-over fractions for Y

        if (uMsg == WM_INPUT)
        {
            RAWINPUT raw = {};
            UINT dwSize = sizeof(raw);
            if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, &raw, &dwSize, sizeof(RAWINPUTHEADER)) != UINT(-1) &&
                raw.header.dwType == RIM_TYPEMOUSE && !(raw.data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE) && GetForegroundWindow() == GameWindow)
            {
                float dx = static_cast<float>(raw.data.mouse.lLastX);
                float dy = static_cast<float>(raw.data.mouse.lLastY);

                if (UseRawData)
                {
                    // Raw input mode: only apply sensitivity
                    dx *= Sensitivity;
                    dy *= Sensitivity;
                }
                else
                {
                    // Standard mode: apply acceleration, speed, and sensitivity
                    // Apply acceleration (independent for each axis, using original abs values)
                    float abs_dx = std::fabsf(dx);
                    if (MouseAcceleration[2] > 0 && MouseAcceleration[0] < abs_dx)
                        dx *= 2.0f;
                    if (MouseAcceleration[2] == 2 && MouseAcceleration[1] < abs_dx)
                        dx *= 2.0f;

                    float abs_dy = std::fabsf(dy);
                    if (MouseAcceleration[2] > 0 && MouseAcceleration[0] < abs_dy)
                        dy *= 2.0f;
                    if (MouseAcceleration[2] == 2 && MouseAcceleration[1] < abs_dy)
                        dy *= 2.0f;

                    // Apply mouse speed scaling
                    dx *= (static_cast<float>(MouseSpeed) / 10.0f);
                    dy *= (static_cast<float>(MouseSpeed) / 10.0f);

                    // Apply custom sensitivity factor
                    dx *= Sensitivity;
                    dy *= Sensitivity;
                }

                // Add subpixel carry-over, round to int for accumulation, save new fractions
                dx += SubpixelX;
                dy += SubpixelY;
                T int_dx = static_cast<T>(std::roundf(dx));
                T int_dy = static_cast<T>(std::roundf(dy));
                SubpixelX = dx - static_cast<float>(int_dx);
                SubpixelY = dy - static_cast<float>(int_dy);

                RawMouseDeltaX += int_dx;
                RawMouseDeltaY += int_dy;

                RECT clientRect;
                GetClientRect(GameWindow, &clientRect);
                RawMouseCursorX = std::clamp(static_cast<T>(RawMouseCursorX + int_dx), T(0), static_cast<T>(clientRect.right));
                RawMouseCursorY = std::clamp(static_cast<T>(RawMouseCursorY + int_dy), T(0), static_cast<T>(clientRect.bottom));

                if (!Pending.exchange(true))
                    PostMessage(GameWindow, WM_RAWINPUTMOUSE, 0, 0);
            }
        }
        return DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }
};

template<typename T = int16_t>
class RawCursorHandler
{
public:
    static inline T MouseCursorX = 0;
    static inline T MouseCursorY = 0;
    static inline T MouseDeltaX = 0;
    static inline T MouseDeltaY = 0;

    static void Initialize(HWND hWnd, float sensitivity = 1.0f)
    {
        Sensitivity = sensitivity;
        TargetWindow = hWnd;
        OnResChange();
    }

    static void OnResChange()
    {
        RECT windowRect;
        GetWindowRect(TargetWindow, &windowRect);
        MouseCursorX = static_cast<T>((windowRect.right - windowRect.left) / 2 + windowRect.left);
        MouseCursorY = static_cast<T>((windowRect.bottom - windowRect.top) / 2 + windowRect.top);
        SetCursorPos(static_cast<int>(MouseCursorX), static_cast<int>(MouseCursorY));
    }

    static HWND UpdateMouseInput(bool bIsAbsoluteValue = false)
    {
        if (TargetWindow != GetForegroundWindow())
        {
            if (bIsAbsoluteValue)
                ClipCursor(NULL);

            return TargetWindow;
        }

        RECT windowRect;
        GetWindowRect(TargetWindow, &windowRect);

        if (bIsAbsoluteValue)
        {
            POINT cursorPos;
            ClipCursor(&windowRect);
            GetCursorPos(&cursorPos);
            MouseCursorX = static_cast<T>(cursorPos.x - windowRect.left);
            MouseCursorY = static_cast<T>(cursorPos.y - windowRect.top);
        }
        else
        {
            POINT cursorPos;
            GetCursorPos(&cursorPos);

            T centerX = static_cast<T>((windowRect.right - windowRect.left) / 2 + windowRect.left);
            T centerY = static_cast<T>((windowRect.bottom - windowRect.top) / 2 + windowRect.top);

            T deltaX = static_cast<T>(cursorPos.x - centerX);
            T deltaY = static_cast<T>(cursorPos.y - centerY);

            float scaledDeltaX = static_cast<float>(deltaX) * Sensitivity;
            float scaledDeltaY = static_cast<float>(deltaY) * Sensitivity;

            MouseDeltaX += static_cast<T>(scaledDeltaX);
            MouseDeltaY -= static_cast<T>(scaledDeltaY);

            SetCursorPos(static_cast<int>(centerX), static_cast<int>(centerY));
        }

        return TargetWindow;
    }

    static void SetSensitivity(float sensitivity)
    {
        Sensitivity = sensitivity;
    }

private:
    static inline HWND TargetWindow = nullptr;
    static inline float Sensitivity = 1.0f;
};