#pragma once

#include <windows.h>
#include <vector>
#include <atomic>
#include <thread>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <hidusage.h>

#define WM_RAWINPUTMOUSE (WM_APP + 1000)

// Raw mouse input is read on its own thread (a message-only window), a high polling rate mouse sends thousands of WM_INPUT messages
// a second, that would flood the game's message queue. Every report is added to RawMouseDeltaX/Y, none is dropped or merged.
// The game window gets one WM_RAWINPUTMOUSE until it handled it, and at most one a millisecond: a game that empties its queue in
// a loop each frame would otherwise get a new one on every pass while the mouse moves and not finish the frame.
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
    static constexpr ULONGLONG PostIntervalUs = 1000;       // shortest time between two WM_RAWINPUTMOUSE
    static constexpr ULONGLONG AccelerationWindowUs = 7500; // movement compared to the Windows thresholds, about one 125 Hz report

    static inline HWND GameWindow = nullptr;
    static inline WNDPROC DefaultWndProc;
    static inline std::atomic<bool> Pending = false; // WM_RAWINPUTMOUSE posted and not handled yet
    static inline bool Unposted = false;             // movement the game window wasn't told about yet (input thread only)
    static inline ULONGLONG LastPost = 0;
    static inline int MouseAcceleration[3] = { 0 };  // [0]=threshold1, [1]=threshold2, [2]=level (0=off, 1 or 2=on with varying strength)
    static inline int MouseSpeed = 10; // Default to 10 if query fails
    static inline std::atomic<float> Sensitivity = 1.0f;
    static inline bool UseRawData = false;

    static ULONGLONG NowUs()
    {
        static const LONGLONG frequency = []
        {
            LARGE_INTEGER f;
            QueryPerformanceFrequency(&f);
            return f.QuadPart;
        }();
        LARGE_INTEGER t;
        QueryPerformanceCounter(&t);
        return static_cast<ULONGLONG>(t.QuadPart / frequency * 1000000 + t.QuadPart % frequency * 1000000 / frequency);
    }

    static LRESULT CALLBACK GameWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        if (uMsg == WM_RAWINPUTMOUSE)
            Pending = false;
        return CallWindowProc(DefaultWndProc, hWnd, uMsg, wParam, lParam);
    }

    // Input thread: tells the game window about new movement once the last message was handled and the interval passed
    static void Post()
    {
        if (!Unposted || Pending)
            return;
        const auto now = NowUs();
        if (now - LastPost < PostIntervalUs)
            return;
        Unposted = false;
        LastPost = now;
        Pending = true;
        PostMessage(GameWindow, WM_RAWINPUTMOUSE, 0, 0);
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

        for (;;)
        {
            // While movement waits for the game window or for the interval, look again every millisecond
            MsgWaitForMultipleObjectsEx(0, nullptr, Unposted ? 1 : INFINITE, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
            MSG msg;
            while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
            {
                if (msg.message == WM_QUIT)
                    return;
                DispatchMessageW(&msg);
            }
            Post();
        }
    }

    // Windows doubles a movement above its acceleration thresholds. The movement of the last 7.5 ms is compared, so a mouse with
    // a high polling rate (a few counts per report) accelerates like one with 125 Hz.
    static float Accelerate(float d, float recent)
    {
        if (MouseAcceleration[2] > 0 && MouseAcceleration[0] < recent)
            d *= 2.0f;
        if (MouseAcceleration[2] == 2 && MouseAcceleration[1] < recent)
            d *= 2.0f;
        return d;
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
                raw.header.dwType == RIM_TYPEMOUSE && !(raw.data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE) && GetForegroundWindow() == GetAncestor(GameWindow, GA_ROOT))
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
                    if (MouseAcceleration[2] > 0)
                    {
                        struct Report { ULONGLONG time; float x, y; };
                        static std::vector<Report> recent;
                        const auto now = NowUs();
                        std::erase_if(recent, [now](const Report& r) { return now - r.time >= AccelerationWindowUs; });
                        recent.push_back({ now, std::fabsf(dx), std::fabsf(dy) });
                        float sumX = 0.0f, sumY = 0.0f;
                        for (const auto& r : recent)
                        {
                            sumX += r.x;
                            sumY += r.y;
                        }
                        dx = Accelerate(dx, sumX);
                        dy = Accelerate(dy, sumY);
                    }

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

                Unposted = true;
                Post();
            }
        }
        return DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }
};

// A game that empties its message queue each frame with a PeekMessage loop gets a new WM_MOUSEMOVE on every pass while the
// mouse moves, and with a high polling rate mouse the loop can go on for many milliseconds: the frame stalls. Install() replaces
// a module's PeekMessage imports: after one WM_MOUSEMOVE the rest of the same loop leaves mouse moves in the queue (Windows
// merges them, the next frame gets the latest position; mouse buttons come with it). Keys and all other messages still come.
class MessagePumpGuard
{
public:
    static void Install(HMODULE module)
    {
        auto base = reinterpret_cast<uint8_t*>(module);
        auto nt = reinterpret_cast<IMAGE_NT_HEADERS*>(base + reinterpret_cast<IMAGE_DOS_HEADER*>(base)->e_lfanew);
        const auto& dir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
        if (!dir.VirtualAddress)
            return;
        for (auto imp = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(base + dir.VirtualAddress); imp->Name; ++imp)
        {
            if (_stricmp(reinterpret_cast<const char*>(base + imp->Name), "user32.dll") || !imp->OriginalFirstThunk)
                continue;
            auto names = reinterpret_cast<IMAGE_THUNK_DATA*>(base + imp->OriginalFirstThunk);
            auto slots = reinterpret_cast<void**>(base + imp->FirstThunk);
            for (size_t i = 0; names[i].u1.AddressOfData; ++i)
            {
                if (IMAGE_SNAP_BY_ORDINAL(names[i].u1.Ordinal))
                    continue;
                auto name = reinterpret_cast<IMAGE_IMPORT_BY_NAME*>(base + names[i].u1.AddressOfData)->Name;
                const bool wide = !strcmp(name, "PeekMessageW");
                if (!wide && strcmp(name, "PeekMessageA"))
                    continue;
                void* hook = wide ? reinterpret_cast<void*>(&PeekW) : reinterpret_cast<void*>(&PeekA);
                if (slots[i] == hook)
                    continue;
                (wide ? OriginalW : OriginalA) = reinterpret_cast<Peek>(slots[i]);
                DWORD protect;
                VirtualProtect(&slots[i], sizeof(void*), PAGE_READWRITE, &protect);
                slots[i] = hook;
                VirtualProtect(&slots[i], sizeof(void*), protect, &protect);
            }
        }
    }

private:
    using Peek = BOOL(WINAPI*)(LPMSG, HWND, UINT, UINT, UINT);
    static inline Peek OriginalW = nullptr;
    static inline Peek OriginalA = nullptr;
    static inline thread_local bool MovedThisPass = false;

    static BOOL Call(Peek original, LPMSG msg, HWND hWnd, UINT min, UINT max, UINT remove)
    {
        // Only the loop's plain peeks for any message are limited, and a window procedure's loop that gathers the queued
        // mouse moves (a peek for WM_MOUSEMOVE alone): it gets the one this pass already took. Other filtered peeks stay as
        // the game made them.
        const bool plain = !min && !max && !(remove & ~(PM_REMOVE | PM_NOYIELD));
        if (MovedThisPass && min == WM_MOUSEMOVE && max == WM_MOUSEMOVE && (remove & PM_REMOVE))
            return FALSE;
        BOOL result;
        if (plain && MovedThisPass)
        {
            // Every message but WM_MOUSEMOVE: a range that leaves it out doesn't make Windows generate it
            result = original(msg, hWnd, 0, WM_MOUSEMOVE - 1, remove);
            if (!result)
                result = original(msg, hWnd, WM_MOUSEMOVE + 1, UINT(-1), remove);
        }
        else
            result = original(msg, hWnd, min, max, remove);
        if (plain && (remove & PM_REMOVE))
        {
            if (!result)
                MovedThisPass = false; // the loop ends here, the next frame takes a mouse move again
            else if (msg->message == WM_MOUSEMOVE)
                MovedThisPass = true;
        }
        return result;
    }

    static BOOL WINAPI PeekW(LPMSG msg, HWND hWnd, UINT min, UINT max, UINT remove)
    {
        return Call(OriginalW, msg, hWnd, min, max, remove);
    }

    static BOOL WINAPI PeekA(LPMSG msg, HWND hWnd, UINT min, UINT max, UINT remove)
    {
        return Call(OriginalA, msg, hWnd, min, max, remove);
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

            // The fraction a whole-number T can't hold is kept for the next update, slow movement isn't lost
            static float RemainderX = 0.0f;
            static float RemainderY = 0.0f;
            float scaledDeltaX = static_cast<float>(deltaX) * Sensitivity + RemainderX;
            float scaledDeltaY = static_cast<float>(deltaY) * Sensitivity + RemainderY;
            RemainderX = scaledDeltaX - static_cast<float>(static_cast<T>(scaledDeltaX));
            RemainderY = scaledDeltaY - static_cast<float>(static_cast<T>(scaledDeltaY));

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