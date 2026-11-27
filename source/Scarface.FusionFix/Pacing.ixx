module;

#include "stdafx.h"
#include "Frame.hpp"
#include "FixSettings.hpp"
#include <stdexcept>

export module Pacing;

export namespace ScarfacePacing
{
    inline std::atomic<int> LimitValue{ 0 };
    inline bool Registered = false;

    class Pacer
    {
        LONGLONG frequency = 0, next = 0, remainder = 0;
        int previousFPS = 0;
        HANDLE timer = nullptr;
        bool timerAttempted = false;

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
                if (!timerAttempted)
                {
                    timerAttempted = true;
                    timer = CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
                }
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
            if (fps != previousFPS || !next || now < next - period * 4)
            {
                next = now + period;
                remainder = frequency % fps;
                previousFPS = fps;
                return;
            }
            if (now < next)
                WaitUntil(next);
            next = std::max(next, Now()) + period;
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

    inline void Install()
    {
        if (Registered)
            return;
        auto refresh = []()
        {
            static std::mutex settingsMutex;
            std::lock_guard lock(settingsMutex);
            const auto setting = FixSettings().Get("MAIN.FPSLimit");
            int value = 0;
            std::from_chars(setting.data(), setting.data() + setting.size(), value);
            LimitValue.store(value);
        };
        FixSettings().OnChange("MAIN.FPSLimit", refresh);
        refresh();
        ScarfaceFrame::Handlers.push_back([](void* display)
        {
            static Pacer pacer;
            const int value = LimitValue.load();
            pacer.Limit(value < 0 ? MonitorRate(display) : value);
        });
        Registered = true;
    }
}
