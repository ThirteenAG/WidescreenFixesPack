#pragma once

#include <atomic>
#include <limits>
#include "rawinput.hpp"

namespace BullyRawMouse
{
    inline SafetyHookMid MouseHook, SampleHook;
    inline uintptr_t Resume = 0;
    inline std::atomic_bool Enabled = false;
    inline thread_local bool rawSample = false;
    inline std::mutex MotionMutex;
    inline int64_t DeltaX = 0, DeltaY = 0;
    inline bool Relative = false;
    inline std::atomic_uint64_t MotionGeneration{};
    inline std::atomic<HWND> Window{ nullptr };
    inline HWND Reader = nullptr;
    inline std::atomic<HWND> Source{ nullptr };
    inline constexpr wchar_t PreviousProperty[] = L"Bully.FusionFix.RawMouse.Previous";
    inline bool Registered = false;
    inline ULONGLONG NextRegistrationCheck = 0;

    inline void Clear()
    {
        std::lock_guard lock(MotionMutex);
        ++MotionGeneration;
        DeltaX = DeltaY = 0;
        Relative = false;
    }

    enum class RegistrationStatus { Error, Empty, Mouse };

    inline RegistrationStatus Registration(RAWINPUTDEVICE& mouse)
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
            const auto& device = devices[i];
            if (device.usUsagePage == 1 && device.usUsage == 2)
            {
                mouse = device;
                return RegistrationStatus::Mouse;
            }
        }
        return RegistrationStatus::Empty;
    }

    inline void Collect(HWND reader, LPARAM lParam, bool buffered)
    {
        const auto generation = MotionGeneration.load();
        if (Source.load() != reader || !Enabled)
            return;
        static const bool wow64 = []
        {
            BOOL result = FALSE;
            IsWow64Process(GetCurrentProcess(), &result);
            return result != FALSE;
        }();
        const auto alignment = wow64 ? uintptr_t{ 8 } : alignof(RAWINPUT);
        const auto mouseOffset = sizeof(RAWINPUTHEADER) + (wow64 ? 8 : 0);
        const auto game = Window.load();
        int64_t x = 0, y = 0;
        bool sawMouse = false, sawAbsolute = false, relative = true;
        const auto add = [&](const RAWMOUSE& mouse)
        {
            if (!(mouse.usFlags & MOUSE_MOVE_ABSOLUTE) && !mouse.lLastX && !mouse.lLastY)
                return;
            sawMouse = true;
            relative = !(mouse.usFlags & MOUSE_MOVE_ABSOLUTE);
            if (relative)
            {
                x += mouse.lLastX;
                y += mouse.lLastY;
            }
            else
            {
                sawAbsolute = true;
                x = y = 0;
            }
        };

        // The message loop removed the first report before dispatch. Read its handle, then drain the queue.
        RAWINPUT first{};
        UINT size = sizeof(first);
        if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, &first, &size, sizeof(RAWINPUTHEADER)) != UINT(-1) &&
            first.header.dwType == RIM_TYPEMOUSE)
            add(first.data.mouse);

        alignas(8) static unsigned char buffer[64 * 1024];
        while (buffered)
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

        if (sawMouse && game && GetForegroundWindow() == game)
        {
            std::lock_guard lock(MotionMutex);
            if (generation == MotionGeneration.load() && Window.load() == game && Source.load() == reader)
            {
                if (sawAbsolute)
                    DeltaX = DeltaY = 0;
                DeltaX += x;
                DeltaY += y;
                Relative = relative;
            }
        }
    }

    inline LRESULT CALLBACK ReadMouse(HWND reader, UINT message, WPARAM wParam, LPARAM lParam)
    {
        if (message == WM_INPUT)
            Collect(reader, lParam, true);
        return DefWindowProcW(reader, message, wParam, lParam);
    }

    inline LRESULT CALLBACK ObserveMouse(HWND reader, UINT message, WPARAM wParam, LPARAM lParam)
    {
        auto previous = reinterpret_cast<WNDPROC>(GetPropW(reader, PreviousProperty));
        if (message == WM_INPUT)
            Collect(reader, lParam, false);
        const auto result = CallWindowProcW(previous, reader, message, wParam, lParam);
        if (message == WM_NCDESTROY)
            RemovePropW(reader, PreviousProperty);
        return result;
    }

    inline bool Observe(HWND target, HWND game)
    {
        DWORD process{};
        const auto thread = GetWindowThreadProcessId(target, &process);
        if (!thread || process != GetCurrentProcessId() || thread == GetWindowThreadProcessId(game, nullptr))
            return false;
        // DirectInput owns this worker's messages. Observe each dispatched handle and leave its input queue intact.
        const auto current = reinterpret_cast<WNDPROC>(GetWindowLongPtrW(target, GWLP_WNDPROC));
        const auto previous = reinterpret_cast<WNDPROC>(GetPropW(target, PreviousProperty));
        if (current == ObserveMouse || (previous && current != previous))
            return true;
        if (!current || !SetPropW(target, PreviousProperty, reinterpret_cast<HANDLE>(current)))
            return false;
        SetLastError(0);
        if (!SetWindowLongPtrW(target, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(ObserveMouse)) && GetLastError())
        {
            RemovePropW(target, PreviousProperty);
            return false;
        }
        return true;
    }

    inline void Attach(HWND window)
    {
        if (!Enabled || (window && !IsWindow(window)))
            window = nullptr;
        if (Window.exchange(window) != window)
        {
            NextRegistrationCheck = 0;
            Registered = false;
            Source = nullptr;
            Clear();
        }
        if (!window && !Reader)
            return;

        const auto now = GetTickCount64();
        if (now < NextRegistrationCheck)
            return;
        NextRegistrationCheck = now + 250;
        RAWINPUTDEVICE current{};
        const auto status = Registration(current);
        const bool own = status == RegistrationStatus::Mouse && Reader && current.hwndTarget == Reader && current.dwFlags == 0;
        HWND target = nullptr;
        if (window)
        {
            if (own)
                target = Reader;
            else if (status == RegistrationStatus::Mouse && current.hwndTarget && Observe(current.hwndTarget, window))
                target = current.hwndTarget;
            // Existing mouse registrations belong to the game or another plugin. Keyboard input is independent.
            if (status == RegistrationStatus::Empty)
            {
                if (!Reader)
                    Reader = CreateRawInputWindow(L"BullyRawMouseReader", ReadMouse);
                RAWINPUTDEVICE mouse{ 1, 2, 0, Reader };
                if (Reader && RegisterRawInputDevices(&mouse, 1, sizeof(mouse)))
                    target = Reader;
            }
        }
        else if (own)
        {
            RAWINPUTDEVICE mouse{ 1, 2, RIDEV_REMOVE, nullptr };
            RegisterRawInputDevices(&mouse, 1, sizeof(mouse));
        }
        {
            std::lock_guard lock(MotionMutex);
            if (Source.exchange(target) != target)
            {
                ++MotionGeneration;
                DeltaX = DeltaY = 0;
                Relative = false;
            }
        }
        Registered = target != nullptr;
    }

    inline bool Consume(int& x, int& y)
    {
        std::lock_guard lock(MotionMutex);
        const auto dx = std::exchange(DeltaX, int64_t{ 0 });
        const auto dy = std::exchange(DeltaY, int64_t{ 0 });
        // Only movement seen since the last sample replaces the native one: an observer that another window
        // procedure bypasses stops seeing reports, and the native mouse takes over
        if (!std::exchange(Relative, false) || !Registered)
            return false;
        x = static_cast<int>(std::clamp<int64_t>(dx, std::numeric_limits<int>::min(), std::numeric_limits<int>::max()));
        y = static_cast<int>(std::clamp<int64_t>(dy, std::numeric_limits<int>::min(), std::numeric_limits<int>::max()));
        return true;
    }

    inline void Install()
    {
        auto mouse = hook::pattern("8B 06 3B C3 C6 44 24 14 01 74 05 83 C0 FF 89 06 DB 06 D9 E1");

        Resume = reinterpret_cast<uintptr_t>(mouse.get_first(16));
        SampleHook = safetyhook::create_mid(mouse.get_first(), [](SafetyHookContext& regs)
        {
            rawSample = false;
            if (!Enabled)
                return;
            const auto window = Window.load();
            if (!window || GetForegroundWindow() != window)
            {
                Clear();
                return;
            }
            auto axes = reinterpret_cast<int*>(regs.esi);
            rawSample = Consume(axes[0], axes[1]);
        });
        MouseHook = safetyhook::create_mid(mouse.get_first(11), [](SafetyHookContext& regs)
        {
            if (rawSample)
            {
                *reinterpret_cast<int32_t*>(regs.esi) = int32_t(regs.eax);
                regs.eip = Resume;
            }
        });
    }
}
