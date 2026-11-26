#pragma once

#include <algorithm>
#include <atomic>
#include <limits>
#include <mutex>
#include <vector>
#include "rawinput.hpp"
#include "Shared/BufferedRawMouse.hpp"

template<const wchar_t* ClassName>
class NFSBufferedRawMouse
{
public:
    static inline std::atomic<bool> Enabled{ false };
    static inline std::atomic<HWND> Window{ nullptr };
    static inline HWND Reader = nullptr;
    static inline std::atomic<bool> Registered{ false };
    static inline ULONGLONG NextRegistrationCheck = 0;
    static inline BufferedRawMouse Movement;
    static inline std::mutex RegistrationMutex;
    static inline bool RemovalPending = false;
    static constexpr UINT ScheduleRemoval = WM_APP + 1;
    static constexpr UINT_PTR RemovalTimer = 1;
    static constexpr UINT RetryInterval = 250;

    static inline void Clear()
    {
        Movement.Clear();
    }

    enum class RegistrationStatus { Error, Empty, Mouse };

    static inline RegistrationStatus Registration(RAWINPUTDEVICE& mouse)
    {
        UINT count = 0;
        if (GetRegisteredRawInputDevices(nullptr, &count, sizeof(mouse)) == UINT(-1) &&
            (GetLastError() != ERROR_INSUFFICIENT_BUFFER || !count))
            return RegistrationStatus::Error;
        if (!count)
            return RegistrationStatus::Empty;

        std::vector<RAWINPUTDEVICE> devices(count);
        const auto written = GetRegisteredRawInputDevices(devices.data(), &count, sizeof(mouse));
        if (written == UINT(-1))
            return RegistrationStatus::Error;
        for (UINT i = 0; i < written; ++i)
        {
            if (devices[i].usUsagePage == 1 && devices[i].usUsage == 2)
            {
                mouse = devices[i];
                return RegistrationStatus::Mouse;
            }
        }
        return RegistrationStatus::Empty;
    }

    static inline bool OwnsRegistration(HWND reader, RegistrationStatus status, const RAWINPUTDEVICE& mouse)
    {
        return reader && status == RegistrationStatus::Mouse && mouse.hwndTarget == reader && mouse.dwFlags == 0;
    }

    static inline bool RemoveIfOwned(HWND reader)
    {
        RAWINPUTDEVICE current{};
        const auto status = Registration(current);
        if (status == RegistrationStatus::Error)
            return false;
        if (!OwnsRegistration(reader, status, current))
            return true;
        const RAWINPUTDEVICE mouse{ 1, 2, RIDEV_REMOVE, nullptr };
        return RegisterRawInputDevices(&mouse, 1, sizeof(mouse)) != FALSE;
    }

    static inline LRESULT CALLBACK ReadMouse(HWND reader, UINT message, WPARAM wParam, LPARAM lParam)
    {
        if (message == WM_INPUT)
        {
            Movement.Read(reinterpret_cast<HRAWINPUT>(lParam), []()
            {
                const auto game = Window.load();
                return Enabled.load() && game && GetForegroundWindow() == game;
            });
        }
        else if (message == ScheduleRemoval)
        {
            const std::lock_guard lock(RegistrationMutex);
            if (RemovalPending && !Window.load() && !SetTimer(reader, RemovalTimer, RetryInterval, nullptr))
                RemovalPending = false;
            return 0;
        }
        else if (message == WM_TIMER && wParam == RemovalTimer)
        {
            const std::lock_guard lock(RegistrationMutex);
            if (!RemovalPending || Window.load() || RemoveIfOwned(reader))
            {
                KillTimer(reader, RemovalTimer);
                RemovalPending = false;
            }
            return 0;
        }
        return DefWindowProcW(reader, message, wParam, lParam);
    }

    static inline void Update(HWND window, HWND destroyed = nullptr)
    {
        const std::lock_guard lock(RegistrationMutex);
        if (destroyed && Window.load() != destroyed)
            return;
        if (!Enabled.load() || (window && !IsWindow(window)))
            window = nullptr;
        if (Window.exchange(window) != window)
        {
            NextRegistrationCheck = 0;
            Registered = false;
            Clear();
        }
        if (!window && (!Reader || RemovalPending))
            return;
        if (window)
            RemovalPending = false;

        const auto now = GetTickCount64();
        if (now < NextRegistrationCheck)
            return;
        NextRegistrationCheck = now + RetryInterval;

        if (!window)
        {
            if (!RemoveIfOwned(Reader))
                RemovalPending = PostMessageW(Reader, ScheduleRemoval, 0, 0) != FALSE;
            return;
        }

        RAWINPUTDEVICE current{};
        const auto status = Registration(current);
        if (status == RegistrationStatus::Error)
            return;
        bool registered = OwnsRegistration(Reader, status, current);
        if (status == RegistrationStatus::Empty)
        {
            if (!Reader)
                Reader = CreateRawInputWindow(ClassName, ReadMouse);
            RAWINPUTDEVICE mouse{ 1, 2, 0, Reader };
            registered = Reader && RegisterRawInputDevices(&mouse, 1, sizeof(mouse));
        }
        if (Registered.exchange(registered) != registered)
            Clear();
    }

    static inline void Message(HWND window, UINT message, WPARAM wParam, LPARAM)
    {
        if (window != Window.load())
            return;
        if (message == WM_KILLFOCUS || (message == WM_ACTIVATEAPP && !wParam))
            Clear();
        else if (message == WM_NCDESTROY)
            Update(nullptr, window);
    }

    static inline bool Consume(int& x, int& y)
    {
        const auto movement = Movement.Take();
        if (!Registered || !Enabled.load() || !movement.relative || GetForegroundWindow() != Window.load())
            return false;
        x = static_cast<int>(std::clamp<int64_t>(movement.x, std::numeric_limits<int>::min(), std::numeric_limits<int>::max()));
        y = static_cast<int>(std::clamp<int64_t>(movement.y, std::numeric_limits<int>::min(), std::numeric_limits<int>::max()));
        return true;
    }
};
