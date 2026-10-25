#pragma once
#include "Frame.hpp"

namespace ScarfacePacing
{
    inline const int* LimitValue = nullptr;
    inline bool Registered = false;

    class Pacer
    {
        LONGLONG frequency = 0, next = 0, remainder = 0;
        int previousFPS = 0;
        HANDLE timer = nullptr;

        static LONGLONG Now()
        {
            LARGE_INTEGER value{};
            QueryPerformanceCounter(&value);
            return value.QuadPart;
        }

        void WaitUntil(LONGLONG target)
        {
            const auto remaining = target - Now();
            const auto spin = std::max<LONGLONG>(1, frequency / 5000);
            if (remaining > spin)
            {
                LARGE_INTEGER due{};
                due.QuadPart = -((remaining - spin) * 10000000 / frequency);
                if (timer && SetWaitableTimer(timer, &due, 0, nullptr, nullptr, FALSE))
                    WaitForSingleObject(timer, INFINITE);
                else
                    Sleep(DWORD((remaining - spin) * 1000 / frequency));
            }
            while (Now() < target)
                YieldProcessor();
        }

    public:
        Pacer()
        {
            LARGE_INTEGER value{};
            if (QueryPerformanceFrequency(&value))
                frequency = value.QuadPart;
            timer = CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
            if (frequency <= 0)
                OutputDebugStringA("Scarface: frame pacing clock unavailable.\n");
        }

        Pacer(const Pacer&) = delete;
        Pacer& operator=(const Pacer&) = delete;

        ~Pacer()
        {
            if (timer)
                CloseHandle(timer);
        }

        void Limit(int fps)
        {
            if (!fps || frequency <= 0)
            {
                next = remainder = 0;
                previousFPS = 0;
                return;
            }
            fps = std::clamp(fps, 30, 1000);
            const auto period = std::max<LONGLONG>(1, frequency / fps);
            const auto now = Now();
            if (fps != previousFPS || !next || now > next + period || now < next - period * 4)
            {
                next = now + period;
                remainder = frequency % fps;
                previousFPS = fps;
                return;
            }
            WaitUntil(next);
            next += period;
            remainder += frequency % fps;
            if (remainder >= fps)
            {
                ++next;
                remainder -= fps;
            }
        }
    };

    inline int MonitorRate(void* display)
    {
        static HMONITOR previous = nullptr;
        static ULONGLONG updated = 0;
        static int rate = 60;
        const auto window = *reinterpret_cast<HWND*>(static_cast<uint8_t*>(display) + 456);
        const auto monitor = MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST);
        const auto now = GetTickCount64();
        if (monitor != previous || !updated || now - updated >= 2000)
        {
            MONITORINFOEXW info{};
            info.cbSize = sizeof(info);
            DEVMODEW mode{};
            mode.dmSize = sizeof(mode);
            rate = GetMonitorInfoW(monitor, &info) && EnumDisplaySettingsW(info.szDevice, ENUM_CURRENT_SETTINGS, &mode)
                && mode.dmDisplayFrequency >= 30 ? std::clamp<int>(mode.dmDisplayFrequency, 30, 1000) : 60;
            previous = monitor;
            updated = now;
        }
        return rate;
    }

    inline bool Install(const int& value)
    {
        LimitValue = &value;
        if (Registered)
            return true;
        if (!ScarfaceFrame::PresentHook)
        {
            OutputDebugStringA("Scarface: frame pacing boundary unavailable.\n");
            return false;
        }
        ScarfaceFrame::Handlers.push_back([](void* display)
        {
            static Pacer pacer;
            const int value = LimitValue ? *LimitValue : 0;
            pacer.Limit(value < 0 ? MonitorRate(display) : value);
        });
        Registered = true;
        return true;
    }
}
