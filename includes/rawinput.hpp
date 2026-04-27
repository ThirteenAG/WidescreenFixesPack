#pragma once

#include <windows.h>
#include <vector>
#include <cmath>
#include <hidusage.h>

#define WM_RAWINPUTMOUSE (WM_APP + 1000)

template<typename T = int16_t>
class RawInputHandler
{
public:
    static inline T RawMouseCursorX = 0;
    static inline T RawMouseCursorY = 0;
    static inline T RawMouseDeltaX = 0;
    static inline T RawMouseDeltaY = 0;

    static void RegisterRawInput(HWND hWnd, float sensitivity = 1.0f, bool bUseRawData = false)
    {
        Sensitivity = sensitivity;
        UseRawData = bUseRawData;
        SystemParametersInfo(SPI_GETMOUSE, 0, MouseAcceleration, 0);
        SystemParametersInfo(SPI_GETMOUSESPEED, 0, &MouseSpeed, 0);
        DefaultWndProc = (WNDPROC)SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)RawInputWndProc);
        rid[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
        rid[0].usUsage = HID_USAGE_GENERIC_MOUSE;
        rid[0].dwFlags = RIDEV_INPUTSINK;
        rid[0].hwndTarget = hWnd;
        RegisterRawInputDevices(rid, 1, sizeof(RAWINPUTDEVICE));
    }

    static void OnResChange()
    {
        RECT windowRect;
        GetWindowRect(rid[0].hwndTarget, &windowRect);
        RawMouseCursorX = static_cast<T>((windowRect.right - windowRect.left) / 2 + windowRect.left);
        RawMouseCursorY = static_cast<T>((windowRect.bottom - windowRect.top) / 2 + windowRect.top);
    }

private:
    static inline WNDPROC DefaultWndProc;
    static inline RAWINPUTDEVICE rid[1];
    static inline int MouseAcceleration[3] = { 0 };  // [0]=threshold1, [1]=threshold2, [2]=level (0=off, 1 or 2=on with varying strength)
    static inline int MouseSpeed = 10; // Default to 10 if query fails
    static inline float Sensitivity = 1.0f;
    static inline bool UseRawData = false;

    static LRESULT CALLBACK RawInputWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        static float SubpixelX = 0.0f;          // Carry-over fractions for X
        static float SubpixelY = 0.0f;          // Carry-over fractions for Y

        if (uMsg == WM_INPUT)
        {
            RECT clientRect;
            GetClientRect(hWnd, &clientRect);

            UINT dwSize = 0;
            GetRawInputData((HRAWINPUT)lParam, RID_INPUT, nullptr, &dwSize, sizeof(RAWINPUTHEADER));
            std::vector<BYTE> buffer(dwSize);
            RAWINPUT* raw = (RAWINPUT*)buffer.data();
            if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, raw, &dwSize, sizeof(RAWINPUTHEADER)) == dwSize)
            {
                if (raw->header.dwType == RIM_TYPEMOUSE)
                {
                    float dx = static_cast<float>(raw->data.mouse.lLastX);
                    float dy = static_cast<float>(raw->data.mouse.lLastY);

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

                    // Accumulate and clamp
                    T oldX = RawMouseCursorX;
                    T oldY = RawMouseCursorY;

                    RawMouseCursorX += int_dx;
                    RawMouseCursorY += int_dy;

                    RawMouseDeltaX += (RawMouseCursorX - oldX);
                    RawMouseDeltaY += (RawMouseCursorY - oldY);

                    RawMouseCursorX = std::max(T(0), std::min(RawMouseCursorX, static_cast<T>(clientRect.right)));
                    RawMouseCursorY = std::max(T(0), std::min(RawMouseCursorY, static_cast<T>(clientRect.bottom)));
                }
            }
            PostMessage(hWnd, WM_RAWINPUTMOUSE, 0, 0);
            return 0;  // Consume the message to avoid game interference.
        }
        return CallWindowProc(DefaultWndProc, hWnd, uMsg, wParam, lParam);
    }

    static void SetSensitivity(float sensitivity)
    {
        Sensitivity = sensitivity;
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