#pragma once
#include <windows.h>
#include <atomic>
#include <thread>
#include <vector>

// High polling rate mice (4000-8000 Hz): DirectInput reads the mouse through raw input on a worker thread of its own
// that wakes up and copies the data for every single WM_INPUT message, about 30 us per report. At 8000 reports a
// second that thread takes a quarter of a core at the same priority as the game's job threads (the game runs in the
// high priority class), and mouse look hitches while the mouse moves.
// The fix moves the mouse's raw input registration to a thread of its own, which wakes at most once per millisecond
// and reads everything that arrived since with GetRawInputBuffer, then hands the summed movement, wheel and buttons to
// the game when it reads the DirectInput mouse. No report is lost or counted twice; the registration flags DirectInput
// asked for (exclusive or not, legacy messages or not) are kept.
namespace RawMouse
{
    struct Totals
    {
        LONG x = 0;
        LONG y = 0;
        LONG wheel = 0;
        BYTE down = 0;      // buttons held now, bit per button
        BYTE pressed = 0;   // buttons pressed since the last read, so a click shorter than a frame still counts
    };

    namespace detail
    {
        inline SRWLOCK lock = SRWLOCK_INIT;
        inline Totals totals;
        inline std::atomic<HWND> window = nullptr;
        inline std::atomic<bool> registered = false;
        inline bool haveAbsolute = false;
        inline LONG lastAbsoluteX = 0;
        inline LONG lastAbsoluteY = 0;

        inline void Add(const RAWMOUSE& m)
        {
            LONG dx = m.lLastX;
            LONG dy = m.lLastY;
            if (m.usFlags & MOUSE_MOVE_ABSOLUTE)
            {
                // Tablets and remote sessions report 0-65535 positions; DirectInput turns them into pixel movement
                bool desktop = (m.usFlags & MOUSE_VIRTUAL_DESKTOP) != 0;
                LONG w = GetSystemMetrics(desktop ? SM_CXVIRTUALSCREEN : SM_CXSCREEN);
                LONG h = GetSystemMetrics(desktop ? SM_CYVIRTUALSCREEN : SM_CYSCREEN);
                LONG ax = MulDiv(m.lLastX, w, 65535);
                LONG ay = MulDiv(m.lLastY, h, 65535);
                dx = haveAbsolute ? ax - lastAbsoluteX : 0;
                dy = haveAbsolute ? ay - lastAbsoluteY : 0;
                lastAbsoluteX = ax;
                lastAbsoluteY = ay;
                haveAbsolute = true;
            }
            totals.x += dx;
            totals.y += dy;
            if (m.usButtonFlags & RI_MOUSE_WHEEL)
                totals.wheel += (SHORT)m.usButtonData;
            for (int i = 0; i < 5; i++)
            {
                if (m.usButtonFlags & (1 << (i * 2)))
                {
                    totals.down |= BYTE(1 << i);
                    totals.pressed |= BYTE(1 << i);
                }
                if (m.usButtonFlags & (2 << (i * 2)))
                    totals.down &= BYTE(~(1 << i));
            }
        }

        inline void Drain()
        {
            alignas(8) static BYTE buffer[64 * 1024];
            for (;;)
            {
                UINT size = sizeof(buffer);
                UINT count = GetRawInputBuffer((PRAWINPUT)buffer, &size, sizeof(RAWINPUTHEADER));
                if (count == 0 || count == UINT(-1))
                    break;
                AcquireSRWLockExclusive(&lock);
                auto raw = (PRAWINPUT)buffer;
                for (UINT i = 0; i < count; i++)
                {
                    if (raw->header.dwType == RIM_TYPEMOUSE)
                        Add(raw->data.mouse);
                    raw = (PRAWINPUT)(((ULONG_PTR)raw + raw->header.dwSize + 7) & ~ULONG_PTR(7));
                }
                ReleaseSRWLockExclusive(&lock);
            }
        }

        inline void Thread()
        {
            SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
            WNDCLASSEXW wc{ sizeof(wc) };
            wc.lpfnWndProc = DefWindowProcW;
            wc.hInstance = GetModuleHandleW(nullptr);
            wc.lpszClassName = L"FusionFixRawMouse";
            RegisterClassExW(&wc);
            window = CreateWindowExW(0, wc.lpszClassName, L"", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, wc.hInstance, nullptr);

            HANDLE timer = CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
            for (;;)
            {
                MsgWaitForMultipleObjects(0, nullptr, FALSE, INFINITE, QS_RAWINPUT | QS_POSTMESSAGE | QS_SENDMESSAGE);
                // Let up to a millisecond of reports gather, then read them in one go
                LARGE_INTEGER due{};
                due.QuadPart = -10000;
                if (timer && SetWaitableTimer(timer, &due, 0, nullptr, nullptr, FALSE))
                    WaitForSingleObject(timer, INFINITE);
                else
                    Sleep(1);
                Drain();

                MSG msg;
                while (PeekMessageW(&msg, nullptr, 0, WM_INPUT - 1, PM_REMOVE) || PeekMessageW(&msg, nullptr, WM_INPUT + 1, 0xFFFFFFFF, PM_REMOVE))
                    DispatchMessageW(&msg);
            }
        }
    }

    inline void Start()
    {
        std::thread(detail::Thread).detach();
    }

    // Mouse registrations go to the reader's window instead of DirectInput's; everything else stays as asked.
    // Until the reader's window exists, DirectInput keeps reading the mouse itself.
    inline std::vector<RAWINPUTDEVICE> Redirect(PCRAWINPUTDEVICE devices, UINT count)
    {
        std::vector<RAWINPUTDEVICE> list(devices, devices + count);
        HWND window = detail::window;
        for (auto& d : list)
        {
            if (d.usUsagePage != 0x01 || d.usUsage != 0x02)
                continue;
            if (d.dwFlags & RIDEV_REMOVE)
            {
                detail::registered = false;
                continue;
            }
            if (!window)
                continue;
            d.hwndTarget = window;
            AcquireSRWLockExclusive(&detail::lock);
            detail::totals = {};
            detail::haveAbsolute = false;
            ReleaseSRWLockExclusive(&detail::lock);
            detail::registered = true;
        }
        return list;
    }

    inline bool Active()
    {
        return detail::registered;
    }

    // The redirected registration failed: DirectInput registers its own window again and keeps the mouse
    inline void Cancel()
    {
        detail::registered = false;
    }

    // The movement, wheel and buttons since the last call
    inline Totals Take()
    {
        AcquireSRWLockExclusive(&detail::lock);
        Totals t = detail::totals;
        detail::totals.x = detail::totals.y = detail::totals.wheel = 0;
        detail::totals.pressed = 0;
        ReleaseSRWLockExclusive(&detail::lock);
        return t;
    }
}
