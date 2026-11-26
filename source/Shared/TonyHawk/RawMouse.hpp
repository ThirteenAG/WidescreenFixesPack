#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include <mutex>
#include <utility>
#include <vector>
#include "Shared/AsiSettings/asi_settings.hpp"
#include "rawinput.hpp"

// Buffered mouse input and registration for the skating camera. The game's own DirectInput poll supplies
// the relative axes when it owns the process mouse registration.
namespace TonyRawMouse
{
    inline std::atomic<bool> Enabled{ true }, InvertY{ false }, Gameplay{ false }, Seen{ false };
    inline std::atomic<float> Sensitivity{ 1.0f };
    inline std::atomic<HWND> InputWindow{ nullptr };
    inline std::mutex MotionMutex, RegistrationMutex;
    inline std::array<int64_t, 2> Motion{};
    inline std::atomic<uint32_t> MotionGeneration{};
    inline std::atomic<bool> ReleasePending{ false };
    inline HWND GameWindow = nullptr, DestroyingWindow = nullptr;
    inline HWND Reader = nullptr;
    inline ULONGLONG NextRegistration = 0;
    inline constexpr UINT RetryReleaseMessage = WM_APP + 1;
    inline constexpr UINT_PTR ReleaseTimer = 1;

    inline void Clear()
    {
        std::lock_guard lock(MotionMutex);
        Motion = {};
        MotionGeneration.fetch_add(1, std::memory_order_relaxed);
        Seen.store(false, std::memory_order_release);
    }

    inline void SetRawTarget(HWND target)
    {
        if (InputWindow.exchange(target, std::memory_order_relaxed) != target)
            Clear();
    }

    inline bool ReadMouseRegistration(RAWINPUTDEVICE& mouse)
    {
        UINT count = 0;
        if (GetRegisteredRawInputDevices(nullptr, &count, sizeof(mouse)) == UINT(-1))
            return false;
        std::vector<RAWINPUTDEVICE> devices(count);
        if (count && GetRegisteredRawInputDevices(devices.data(), &count, sizeof(mouse)) == UINT(-1))
            return false;
        mouse = {};
        for (UINT i = 0; i < count; ++i)
        {
            if (devices[i].usUsagePage == 1 && devices[i].usUsage == 2)
            {
                mouse = devices[i];
                break;
            }
        }
        return true;
    }

    inline bool OwnsRegistration(const RAWINPUTDEVICE& mouse)
    {
        return Reader && mouse.hwndTarget == Reader && mouse.dwFlags == RIDEV_INPUTSINK;
    }

    // Caller holds RegistrationMutex.
    inline void TryReleaseRegistration()
    {
        RAWINPUTDEVICE current{};
        if (!ReadMouseRegistration(current))
            return;
        if (OwnsRegistration(current))
        {
            const RAWINPUTDEVICE remove{ 1, 2, RIDEV_REMOVE, nullptr };
            if (!RegisterRawInputDevices(&remove, 1, sizeof(remove)))
                return;
        }
        ReleasePending.store(false, std::memory_order_relaxed);
    }

    // Caller holds RegistrationMutex.
    inline void ReleaseRegistrationLocked()
    {
        SetRawTarget(nullptr);
        if (!Reader)
            return;
        ReleasePending.store(true, std::memory_order_relaxed);
        TryReleaseRegistration();
        if (ReleasePending.load(std::memory_order_relaxed))
            PostMessageW(Reader, RetryReleaseMessage, 0, 0);
    }

    inline void ReleaseRegistration()
    {
        std::lock_guard lock(RegistrationMutex);
        ReleaseRegistrationLocked();
    }

    inline void PublishMotion(int64_t x, int64_t y, uint32_t generation, HWND readerTarget, bool absolute = false)
    {
        if ((!x && !y && !absolute) || !Gameplay.load(std::memory_order_relaxed))
            return;
        std::lock_guard lock(MotionMutex);
        if (generation == MotionGeneration.load(std::memory_order_relaxed) &&
            readerTarget == InputWindow.load(std::memory_order_relaxed) &&
            Enabled.load(std::memory_order_relaxed) && Gameplay.load(std::memory_order_relaxed))
        {
            if (absolute)
            {
                Motion = {};
                Seen.store(false, std::memory_order_release);
            }
            if (x || y)
            {
                Motion[0] += x;
                Motion[1] += y;
                Seen.store(true, std::memory_order_release);
            }
        }
    }

    inline void Message(HWND window, UINT message, LPARAM)
    {
        if (message != WM_NCCREATE && message != WM_KILLFOCUS && message != WM_NCDESTROY)
            return;
        std::lock_guard lock(RegistrationMutex);
        if (message == WM_NCCREATE)
        {
            if (window == DestroyingWindow)
                DestroyingWindow = nullptr;
            return;
        }
        if (window != GameWindow)
            return;
        Gameplay.store(false, std::memory_order_relaxed);
        Clear();
        if (message == WM_NCDESTROY)
        {
            GameWindow = nullptr;
            DestroyingWindow = window;
            ReleaseRegistrationLocked();
        }
    }

    inline LRESULT CALLBACK RawInputProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        if (message == RetryReleaseMessage || (message == WM_TIMER && wParam == ReleaseTimer))
        {
            std::lock_guard lock(RegistrationMutex);
            if (window == Reader)
            {
                if (message == RetryReleaseMessage)
                {
                    if (ReleasePending.load(std::memory_order_relaxed))
                        SetTimer(window, ReleaseTimer, 250, nullptr);
                }
                else
                {
                    if (ReleasePending.load(std::memory_order_relaxed))
                        TryReleaseRegistration();
                    if (!ReleasePending.load(std::memory_order_relaxed))
                        KillTimer(window, ReleaseTimer);
                }
            }
            return 0;
        }
        if (message != WM_INPUT)
            return DefWindowProcW(window, message, wParam, lParam);

        static const bool wow64 = []()
        {
            BOOL result = FALSE;
            IsWow64Process(GetCurrentProcess(), &result);
            return result != FALSE;
        }();
        const auto alignment = wow64 ? uintptr_t{ 8 } : alignof(RAWINPUT);
        const auto mouseOffset = sizeof(RAWINPUTHEADER) + (wow64 ? 8 : 0);
        const auto game = InputWindow.load(std::memory_order_relaxed);
        const auto generation = MotionGeneration.load(std::memory_order_relaxed);
        int64_t x = 0, y = 0;
        bool sawAbsolute = false;
        const auto add = [&](const RAWMOUSE& mouse)
        {
            if (mouse.usFlags & MOUSE_MOVE_ABSOLUTE)
            {
                sawAbsolute = true;
                x = y = 0;
            }
            else
            {
                x += mouse.lLastX;
                y += mouse.lLastY;
            }
        };

        // The first message has already been removed from the queue.
        RAWINPUT first{};
        UINT size = sizeof(first);
        if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, &first, &size, sizeof(RAWINPUTHEADER)) != UINT(-1) &&
            first.header.dwType == RIM_TYPEMOUSE)
            add(first.data.mouse);

        alignas(8) static uint8_t buffer[64 * 1024];
        for (;;)
        {
            size = sizeof(buffer);
            const auto count = GetRawInputBuffer(reinterpret_cast<RAWINPUT*>(buffer), &size, sizeof(RAWINPUTHEADER));
            if (count == 0 || count == UINT(-1))
                break;
            auto next = reinterpret_cast<uintptr_t>(buffer);
            for (UINT i = 0; i < count; ++i)
            {
                const auto& header = *reinterpret_cast<const RAWINPUTHEADER*>(next);
                if (header.dwType == RIM_TYPEMOUSE)
                    add(*reinterpret_cast<const RAWMOUSE*>(next + mouseOffset));
                next = (next + header.dwSize + alignment - 1) & ~(alignment - 1);
            }
        }

        if ((x || y || sawAbsolute) && game && GetForegroundWindow() == game)
            PublishMotion(x, y, generation, game, sawAbsolute);
        return DefWindowProcW(window, message, wParam, lParam);
    }

    inline void Frame(HWND window, bool blocked, const wchar_t* readerName, bool (*inGameplay)())
    {
        std::lock_guard lock(RegistrationMutex);
        const bool validWindow = window && window != DestroyingWindow;
        if (validWindow && window != GameWindow)
        {
            Gameplay.store(false, std::memory_order_relaxed);
            InputWindow.store(nullptr, std::memory_order_relaxed);
            Clear();
            GameWindow = window;
            NextRegistration = 0;
        }
        const bool active = Enabled.load(std::memory_order_relaxed) && !blocked && validWindow &&
            GetForegroundWindow() == window && inGameplay();
        if (Gameplay.exchange(active, std::memory_order_relaxed) != active)
            Clear();
        if ((!validWindow || !Enabled.load(std::memory_order_relaxed)) && !ReleasePending.load(std::memory_order_relaxed))
            return;
        const auto now = GetTickCount64();
        if (now < NextRegistration)
            return;
        NextRegistration = now + 250;
        if (ReleasePending.load(std::memory_order_relaxed))
        {
            TryReleaseRegistration();
            if (ReleasePending.load(std::memory_order_relaxed))
                return;
        }
        if (!validWindow || !Enabled.load(std::memory_order_relaxed))
            return;
        RAWINPUTDEVICE device{};
        if (!ReadMouseRegistration(device))
        {
            SetRawTarget(nullptr);
            return;
        }
        if (device.usUsage)
        {
            SetRawTarget(OwnsRegistration(device) ? window : nullptr);
            return;
        }
        if (!Reader)
            Reader = CreateRawInputWindow(readerName, RawInputProc);
        device = { 1, 2, RIDEV_INPUTSINK, Reader };
        SetRawTarget(Reader && RegisterRawInputDevices(&device, 1, sizeof(device)) ? window : nullptr);
    }

    inline void ReadRelativeState(const int32_t* state, HRESULT result)
    {
        // DirectInput already accumulates relative counts when it owns the raw registration.
        if (SUCCEEDED(result) && !InputWindow.load(std::memory_order_relaxed))
        {
            const auto generation = MotionGeneration.load(std::memory_order_relaxed);
            PublishMotion(state[0], state[1], generation, nullptr);
        }
    }

    inline std::array<int64_t, 2> TakeMotion()
    {
        std::lock_guard lock(MotionMutex);
        return std::exchange(Motion, {});
    }

    inline void InstallSettings(asi_settings::Plugin& settings)
    {
        auto updateSettings = [&settings]()
        {
            static std::mutex settingsMutex;
            std::lock_guard lock(settingsMutex);
            const bool enabled = settings.Get("MAIN.RawInputMouse") == "1";
            const bool wasEnabled = Enabled.exchange(enabled, std::memory_order_relaxed);
            InvertY.store(settings.Get("MAIN.InvertMouseY") == "1", std::memory_order_relaxed);
            float sensitivity = 1.0f;
            asi_settings::detail::ParseFloat(settings.Get("MAIN.MouseSensitivity"), sensitivity);
            Sensitivity.store(sensitivity, std::memory_order_relaxed);
            Clear();
            if (wasEnabled && !enabled)
                ReleaseRegistration();
        };
        settings.OnChange("MAIN.RawInputMouse", updateSettings);
        settings.OnChange("MAIN.InvertMouseY", updateSettings);
        settings.OnChange("MAIN.MouseSensitivity", updateSettings);
        updateSettings();
    }
}
