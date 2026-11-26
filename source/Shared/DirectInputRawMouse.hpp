#pragma once
#include <windows.h>
#include <atomic>
#include <thread>
#include <unordered_map>
#include <vector>

// Collect the DirectInput mouse registration on a dedicated thread and supply one accumulated state per poll.
namespace DirectInputRawMouse
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
        struct AbsolutePosition
        {
            LONG x, y;
            bool virtualDesktop;
        };

        inline SRWLOCK lock = SRWLOCK_INIT;
        inline SRWLOCK registrationLock = SRWLOCK_INIT;
        inline Totals totals;
        inline std::atomic<HWND> window = nullptr;
        inline std::atomic<bool> registered = false;
        inline std::unordered_map<HANDLE, AbsolutePosition> absolutePositions;
        inline std::unordered_map<HANDLE, BYTE> heldButtons;

        inline void RefreshHeldButtons()
        {
            totals.down = 0;
            for (const auto& entry : heldButtons)
                totals.down |= entry.second;
        }

        inline bool IsMouse(const RAWINPUTDEVICE& device)
        {
            return device.usUsagePage == 0x01 && device.usUsage == 0x02;
        }

        inline void Add(HANDLE device, const RAWMOUSE& m)
        {
            LONG dx = m.lLastX;
            LONG dy = m.lLastY;
            if (m.usFlags & MOUSE_MOVE_ABSOLUTE)
            {
                // Tablets and remote sessions report 0-65535 positions; DirectInput turns them into pixel movement
                const bool desktop = (m.usFlags & MOUSE_VIRTUAL_DESKTOP) != 0;
                const LONG w = GetSystemMetrics(desktop ? SM_CXVIRTUALSCREEN : SM_CXSCREEN);
                const LONG h = GetSystemMetrics(desktop ? SM_CYVIRTUALSCREEN : SM_CYSCREEN);
                const AbsolutePosition position{ MulDiv(m.lLastX, w, 65535), MulDiv(m.lLastY, h, 65535), desktop };
                auto [previous, inserted] = absolutePositions.try_emplace(device, position);
                const bool sameDesktop = !inserted && previous->second.virtualDesktop == desktop;
                dx = sameDesktop ? position.x - previous->second.x : 0;
                dy = sameDesktop ? position.y - previous->second.y : 0;
                previous->second = position;
            }
            else if (!absolutePositions.empty())
                absolutePositions.erase(device);
            totals.x += dx;
            totals.y += dy;
            if (m.usButtonFlags & RI_MOUSE_WHEEL)
                totals.wheel += (SHORT)m.usButtonData;
            if (!(m.usButtonFlags & 0x03FF))
                return;

            BYTE& held = heldButtons[device];
            for (int i = 0; i < 5; i++)
            {
                const BYTE bit = BYTE(1 << i);
                if (m.usButtonFlags & (1 << (i * 2)))
                {
                    held |= bit;
                    totals.pressed |= bit;
                }
                if (m.usButtonFlags & (2 << (i * 2)))
                    held &= BYTE(~bit);
            }
            if (!held)
                heldButtons.erase(device);
            RefreshHeldButtons();
        }

        inline void Drain()
        {
            static const bool wow64 = []()
            {
                BOOL enabled = FALSE;
                return sizeof(void*) == 4 && IsWow64Process(GetCurrentProcess(), &enabled) && enabled;
            }();
            const size_t header = wow64 ? 24 : sizeof(RAWINPUTHEADER);
            const size_t alignment = wow64 ? 8 : sizeof(void*);
            alignas(8) static BYTE buffer[64 * 1024];
            for (;;)
            {
                UINT size = sizeof(buffer);
                UINT count = GetRawInputBuffer((PRAWINPUT)buffer, &size, sizeof(RAWINPUTHEADER));
                if (count == 0 || count == UINT(-1))
                    break;
                AcquireSRWLockExclusive(&lock);
                const BYTE* packet = buffer;
                for (UINT i = 0; i < count; i++)
                {
                    const auto* raw = reinterpret_cast<const RAWINPUTHEADER*>(packet);
                    if (raw->dwType == RIM_TYPEMOUSE)
                        Add(raw->hDevice, *reinterpret_cast<const RAWMOUSE*>(packet + header));
                    packet += (raw->dwSize + alignment - 1) & ~(alignment - 1);
                }
                ReleaseSRWLockExclusive(&lock);
            }
        }

        inline LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
        {
            if (message == WM_INPUT_DEVICE_CHANGE && wParam == GIDC_REMOVAL)
            {
                const auto device = reinterpret_cast<HANDLE>(lParam);
                AcquireSRWLockExclusive(&lock);
                absolutePositions.erase(device);
                if (heldButtons.erase(device))
                    RefreshHeldButtons();
                ReleaseSRWLockExclusive(&lock);
            }
            return DefWindowProcW(hwnd, message, wParam, lParam);
        }

        inline void Thread()
        {
            SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
            WNDCLASSEXW wc{ sizeof(wc) };
            wc.lpfnWndProc = WindowProc;
            wc.hInstance = GetModuleHandleW(nullptr);
            wc.lpszClassName = L"FusionFixRawMouse";
            RegisterClassExW(&wc);
            window = CreateWindowExW(0, wc.lpszClassName, L"", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, wc.hInstance, nullptr);

            HANDLE timer = CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
            for (;;)
            {
                MsgWaitForMultipleObjects(0, nullptr, FALSE, INFINITE, QS_INPUT | QS_POSTMESSAGE | QS_SENDMESSAGE);
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

    inline void Clear()
    {
        AcquireSRWLockExclusive(&detail::lock);
        detail::totals = {};
        detail::absolutePositions.clear();
        detail::heldButtons.clear();
        ReleaseSRWLockExclusive(&detail::lock);
    }

    // Mouse registrations go to the reader's window instead of DirectInput's; everything else stays as asked.
    // Until the reader's window exists, DirectInput keeps reading the mouse itself.
    template<typename Register>
    BOOL RegisterDevices(PCRAWINPUTDEVICE devices, UINT count, UINT size, Register registerDevices)
    {
        if (!devices || size != sizeof(RAWINPUTDEVICE))
            return registerDevices(devices, count, size);

        const RAWINPUTDEVICE* mouse = nullptr;
        for (UINT i = 0; i < count; ++i)
        {
            if (detail::IsMouse(devices[i]))
                mouse = &devices[i];
        }
        if (!mouse)
            return registerDevices(devices, count, size);

        const HWND window = detail::window.load();
        std::vector<RAWINPUTDEVICE> redirected;
        bool changed = false;
        if (window)
        {
            redirected.assign(devices, devices + count);
            for (auto& device : redirected)
            {
                if (detail::IsMouse(device) && !(device.dwFlags & RIDEV_REMOVE) &&
                    (device.hwndTarget != window || !(device.dwFlags & RIDEV_DEVNOTIFY)))
                {
                    device.hwndTarget = window;
                    device.dwFlags |= RIDEV_DEVNOTIFY;
                    changed = true;
                }
            }
        }

        AcquireSRWLockExclusive(&detail::registrationLock);
        BOOL result = changed ? registerDevices(redirected.data(), count, size) : FALSE;
        bool readerTarget = result != FALSE;
        if (!result)
        {
            result = registerDevices(devices, count, size);
            readerTarget = window && mouse->hwndTarget == window;
        }
        const DWORD error = result ? 0 : GetLastError();
        const bool useReader = readerTarget && !(mouse->dwFlags & RIDEV_REMOVE);
        if (result && detail::registered.load() != useReader)
        {
            Clear();
            detail::registered = useReader;
        }
        ReleaseSRWLockExclusive(&detail::registrationLock);
        if (!result)
            SetLastError(error);
        return result;
    }

    inline bool Active()
    {
        return detail::registered;
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
