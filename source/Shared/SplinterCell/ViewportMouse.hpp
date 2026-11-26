#pragma once

#include "Shared/BufferedRawMouse.hpp"
#include <rawinput.hpp>
#include <charconv>
#include <deque>
#include <optional>

namespace ViewportMouse
{
    inline BufferedRawMouse movement;
    inline std::atomic<HWND> target{ nullptr };
    inline std::atomic<ULONGLONG> boundAt{ 0 };
    inline std::atomic<float> sensitivity{ 1.0f };
    inline std::atomic<bool> rawData{ false };
    inline HWND reader = nullptr;
    inline HWND candidate = nullptr;
    inline ULONGLONG nextCheck = 0;
    inline bool installed = false;
    inline bool nativeFrame = false;
    inline std::optional<RAWINPUTDEVICE> previous;

    inline void ReadScaledSensitivity(float scale)
    {
        const auto text = FixSettings().Get("MAIN.MouseSensitivity");
        float value = 1.0f;
        if (std::from_chars(text.data(), text.data() + text.size(), value).ec == std::errc{})
            sensitivity.store(value * scale);
    }

    inline void ReadSensitivity()
    {
        ReadScaledSensitivity(1.0f);
    }

    // All filter history belongs to the reader thread.
    inline void Scale(RAWMOUSE& mouse, unsigned generation)
    {
        struct Report { LONGLONG time; double x, y; };
        static std::deque<Report> recent;
        static unsigned lastGeneration = ~0u;
        static double sumX, sumY, carryX, carryY;
        static int thresholds[3]{}, speed = 10;
        if (lastGeneration != generation || (mouse.usFlags & MOUSE_MOVE_ABSOLUTE))
        {
            recent.clear();
            sumX = sumY = carryX = carryY = 0;
            if (lastGeneration != generation)
            {
                SystemParametersInfoW(SPI_GETMOUSE, 0, thresholds, 0);
                SystemParametersInfoW(SPI_GETMOUSESPEED, 0, &speed, 0);
                lastGeneration = generation;
            }
            if (mouse.usFlags & MOUSE_MOVE_ABSOLUTE)
                return;
        }
        double x = mouse.lLastX, y = mouse.lLastY;
        double factor = sensitivity.load();
        if (!rawData.load())
        {
            factor *= speed / 10.0;
            if (thresholds[2])
            {
                static const auto interval = []()
                {
                    LARGE_INTEGER frequency;
                    QueryPerformanceFrequency(&frequency);
                    return frequency.QuadPart * 75 / 10000;
                }();
                LARGE_INTEGER now;
                QueryPerformanceCounter(&now);
                while (!recent.empty() && now.QuadPart - recent.front().time >= interval)
                {
                    sumX -= recent.front().x;
                    sumY -= recent.front().y;
                    recent.pop_front();
                }
                recent.push_back({ now.QuadPart, std::abs(x), std::abs(y) });
                sumX += recent.back().x;
                sumY += recent.back().y;
                auto accelerate = [](double delta, double sum)
                {
                    if (sum > thresholds[0])
                        delta *= 2;
                    if (thresholds[2] == 2 && sum > thresholds[1])
                        delta *= 2;
                    return delta;
                };
                x = accelerate(x, sumX);
                y = accelerate(y, sumY);
            }
        }
        auto scale = [factor](double delta, double& carry)
        {
            const double scaled = delta * factor + carry;
            const double rounded = std::round(scaled);
            carry = scaled - rounded;
            return static_cast<LONG>(std::clamp(rounded, double(LONG_MIN), double(LONG_MAX)));
        };
        mouse.lLastX = scale(x, carryX);
        mouse.lLastY = scale(y, carryY);
    }

    inline LRESULT CALLBACK InputProc(HWND window, UINT message, WPARAM wp, LPARAM lp)
    {
        if (message == WM_INPUT)
        {
            movement.Read(reinterpret_cast<HRAWINPUT>(lp), []()
            {
                const HWND game = target.load();
                const auto now = GetTickCount64();
                const DWORD age = static_cast<DWORD>(now) - static_cast<DWORD>(GetMessageTime());
                return game && game == GetForegroundWindow() && now - boundAt.load() > age;
            }, Scale);
        }
        return DefWindowProcW(window, message, wp, lp);
    }

    inline bool Registration(std::optional<RAWINPUTDEVICE>& mouse)
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
        for (UINT i = 0; i < count; ++i)
        {
            if (devices[i].usUsagePage == 1 && devices[i].usUsage == 2)
                mouse = devices[i];
        }
        return true;
    }

    inline void Bind(HWND game)
    {
        if (game != candidate)
        {
            target.store(nullptr);
            movement.Clear();
            nativeFrame = false;
            candidate = game;
            nextCheck = 0;
        }
        const auto now = GetTickCount64();
        if (now < nextCheck)
            return;
        nextCheck = now + 500;
        std::optional<RAWINPUTDEVICE> current;
        if (!Registration(current))
        {
            target.store(nullptr);
            movement.Clear();
            return;
        }
        if (installed && current && current->hwndTarget == reader && current->dwFlags == RIDEV_INPUTSINK)
        {
            if (game)
            {
                if (!target.load())
                {
                    boundAt.store(now);
                    target.store(game);
                }
                return;
            }
            const auto restore = previous.value_or(RAWINPUTDEVICE{ 1, 2, RIDEV_REMOVE, nullptr });
            if (!RegisterRawInputDevices(&restore, 1, sizeof(restore)))
                return;
        }
        if (target.exchange(nullptr))
            movement.Clear();
        installed = false;
        previous.reset();
        if (!game || (current && (current->hwndTarget || current->dwFlags)))
            return;
        if (!reader)
            reader = CreateRawInputWindow(L"SplinterCellViewportMouse", InputProc);
        if (!reader)
            return;
        const RAWINPUTDEVICE device{ 1, 2, RIDEV_INPUTSINK, reader };
        if (RegisterRawInputDevices(&device, 1, sizeof(device)))
        {
            installed = true;
            previous = current;
            boundAt.store(now);
            target.store(game);
        }
    }
}
