#pragma once

#include "rawinput.hpp"
#include <commctrl.h>
#pragma comment(lib, "comctl32.lib")
#include <atomic>
#include <cstdint>
#include <limits>
#include <mutex>
#include <optional>
#include <utility>

namespace GtaRawMouse
{
    inline HWND Window = nullptr;
    inline HWND FallbackWindow = nullptr;
    inline HWND (*GetGameWindow)() = nullptr;
    inline constexpr UINT ReleaseReaderMessage = WM_APP + 0x421;
    inline constexpr UINT_PTR ReleaseReaderTimer = 1;
    inline constexpr auto PreviousProperty = L"WFP.RawMouse.Previous";
    inline bool (*Playing)() = nullptr;
    inline void (*MouseActivity)() = nullptr;

    struct Motion
    {
        int64_t x = 0, y = 0;
        bool relative = false, reset = false, activity = false;

        void Add(const RAWMOUSE& mouse)
        {
            activity |= mouse.usButtonFlags != 0;
            if (mouse.usFlags & MOUSE_MOVE_ABSOLUTE)
            {
                reset = true;
                relative = false;
                x = y = 0;
            }
            else if (mouse.lLastX || mouse.lLastY)
            {
                relative = activity = true;
                x += mouse.lLastX;
                y += mouse.lLastY;
            }
        }
    };

    inline std::mutex MotionMutex, RegistrationMutex;
    inline HWND InputTarget = nullptr;
    inline Motion Pending{};
    inline std::atomic_uint64_t MotionGeneration{};

    // Called with MotionMutex held. Discard both accumulated and in-flight motion.
    inline void InvalidateMotion()
    {
        ++MotionGeneration;
        Pending = {};
    }
    inline std::optional<RAWINPUTDEVICE> Original;
    enum class Registration { Error, Empty, Mouse };

    inline Registration Query(RAWINPUTDEVICE& mouse)
    {
        UINT count = 0;
        if (GetRegisteredRawInputDevices(nullptr, &count, sizeof(mouse)) == UINT(-1))
            return Registration::Error;
        if (!count)
            return Registration::Empty;
        static std::vector<RAWINPUTDEVICE> devices;
        devices.resize(count);
        const auto written = GetRegisteredRawInputDevices(devices.data(), &count, sizeof(mouse));
        if (written == UINT(-1))
            return Registration::Error;
        for (UINT i = 0; i < written; ++i)
        {
            if (devices[i].usUsagePage == HID_USAGE_PAGE_GENERIC && devices[i].usUsage == HID_USAGE_GENERIC_MOUSE)
            {
                mouse = devices[i];
                return Registration::Mouse;
            }
        }
        return Registration::Empty;
    }

    inline void SetLegacy(const RAWINPUTDEVICE& mouse, bool legacy)
    {
        if (Original && (mouse.hwndTarget != Original->hwndTarget || mouse.dwFlags != (Original->dwFlags | RIDEV_NOLEGACY)))
            Original.reset();
        if (legacy)
        {
            if (Original && RegisterRawInputDevices(&*Original, 1, sizeof(mouse)))
                Original.reset();
        }
        else if ((mouse.dwFlags & RIDEV_NOLEGACY) != RIDEV_NOLEGACY)
        {
            auto modified = mouse;
            modified.dwFlags |= RIDEV_NOLEGACY;
            if (RegisterRawInputDevices(&modified, 1, sizeof(modified)))
                Original = mouse;
        }
    }

    inline void RestoreLegacy(HWND gameWindow = nullptr)
    {
        std::lock_guard lock(RegistrationMutex);
        if ((gameWindow && gameWindow != Window) || !Original)
            return;
        RAWINPUTDEVICE mouse{};
        const auto state = Query(mouse);
        if (state == Registration::Mouse)
            SetLegacy(mouse, true);
        else if (state == Registration::Empty)
            Original.reset();
    }

    inline bool Front()
    {
        return Window && GetForegroundWindow() == GetAncestor(Window, GA_ROOT);
    }

    inline void ReadInput(HWND target, LPARAM handle, bool buffered)
    {
        const auto generation = MotionGeneration.load();
        Motion batch{};
        RAWINPUT first{};
        UINT size = sizeof(first);
        if (GetRawInputData(reinterpret_cast<HRAWINPUT>(handle), RID_INPUT, &first, &size, sizeof(RAWINPUTHEADER)) != UINT(-1) &&
            first.header.dwType == RIM_TYPEMOUSE)
            batch.Add(first.data.mouse);
        if (buffered)
        {
            // WOW64 widens the buffered header and aligns each report to eight bytes.
            static const bool wow64 = [] { BOOL wow = FALSE; IsWow64Process(GetCurrentProcess(), &wow); return wow != FALSE; }();
            const uintptr_t align = wow64 ? 8 : alignof(RAWINPUT);
            const auto offset = sizeof(RAWINPUTHEADER) + (wow64 ? 8 : 0);
            alignas(8) static unsigned char buffer[64 * 1024];
            for (;;)
            {
                size = sizeof(buffer);
                const auto count = GetRawInputBuffer(reinterpret_cast<RAWINPUT*>(buffer), &size, sizeof(RAWINPUTHEADER));
                if (!count || count == UINT(-1))
                    break;
                auto next = reinterpret_cast<uintptr_t>(buffer);
                for (UINT i = 0; i < count; ++i)
                {
                    const auto& header = *reinterpret_cast<const RAWINPUTHEADER*>(next);
                    if (header.dwType == RIM_TYPEMOUSE)
                        batch.Add(*reinterpret_cast<const RAWMOUSE*>(next + offset));
                    next = (next + header.dwSize + align - 1) & ~(align - 1);
                }
            }
        }
        std::lock_guard lock(MotionMutex);
        if (generation != MotionGeneration.load() || target != InputTarget || !Front())
            return;
        if (batch.reset)
            Pending = {};
        Pending.relative |= batch.relative;
        Pending.x += batch.x;
        Pending.y += batch.y;
        if (batch.activity && MouseActivity)
            MouseActivity();
    }

    enum class ReaderRelease { Keep, Retry, Destroy };

    inline ReaderRelease ReleaseFallback(HWND reader)
    {
        std::lock_guard lock(RegistrationMutex);
        if (Window || reader != FallbackWindow)
            return ReaderRelease::Keep;

        RAWINPUTDEVICE mouse{};
        const auto state = Query(mouse);
        if (state == Registration::Error)
            return ReaderRelease::Retry;
        if (state == Registration::Mouse && mouse.hwndTarget == reader)
        {
            constexpr DWORD readerFlags = RIDEV_INPUTSINK;
            const bool addedNoLegacy = Original && Original->hwndTarget == reader &&
                Original->dwFlags == readerFlags && mouse.dwFlags == (readerFlags | RIDEV_NOLEGACY);
            if (mouse.dwFlags != readerFlags && !addedNoLegacy)
                return ReaderRelease::Keep;
            const RAWINPUTDEVICE remove{ 1, 2, RIDEV_REMOVE, nullptr };
            if (!RegisterRawInputDevices(&remove, 1, sizeof(remove)))
                return ReaderRelease::Retry;
        }
        if (Original && Original->hwndTarget == reader)
            Original.reset();
        FallbackWindow = nullptr;
        return ReaderRelease::Destroy;
    }

    inline LRESULT CALLBACK FallbackProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        if (message == WM_INPUT)
            ReadInput(window, lParam, true);
        else if (message == ReleaseReaderMessage || (message == WM_TIMER && wParam == ReleaseReaderTimer))
        {
            const auto release = ReleaseFallback(window);
            if (release == ReaderRelease::Retry)
                SetTimer(window, ReleaseReaderTimer, 50, nullptr);
            else
                KillTimer(window, ReleaseReaderTimer);
            if (release == ReaderRelease::Destroy)
                DestroyWindow(window);
            return 0;
        }
        else if (message == WM_NCDESTROY)
            PostQuitMessage(0);
        return DefWindowProcW(window, message, wParam, lParam);
    }

    inline LRESULT CALLBACK InputWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        auto previous = reinterpret_cast<WNDPROC>(GetPropW(window, PreviousProperty));
        if (message == WM_INPUT)
            ReadInput(window, lParam, false);
        auto result = previous ? CallWindowProcW(previous, window, message, wParam, lParam) : DefWindowProcW(window, message, wParam, lParam);
        if (message == WM_NCDESTROY)
            RemovePropW(window, PreviousProperty);
        return result;
    }

    inline bool Observe(HWND target)
    {
        auto current = reinterpret_cast<WNDPROC>(GetWindowLongPtrW(target, GWLP_WNDPROC));
        auto previous = reinterpret_cast<WNDPROC>(GetPropW(target, PreviousProperty));
        if (current == InputWindowProc || (previous && current != previous))
            return true;
        if (!previous && !SetPropW(target, PreviousProperty, reinterpret_cast<HANDLE>(current)))
            return false;
        SetLastError(0);
        if (!SetWindowLongPtrW(target, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(InputWindowProc)) && GetLastError())
        {
            RemovePropW(target, PreviousProperty);
            return false;
        }
        return true;
    }

    inline LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);
    inline constexpr UINT_PTR GameWindowSubclass = 1;

    inline void RefreshGameWindow()
    {
        const auto next = GetGameWindow();
        if (!next || next == Window || !SetWindowSubclass(next, WindowProc, GameWindowSubclass, 0))
            return;
        if (Window)
            RemoveWindowSubclass(Window, WindowProc, GameWindowSubclass);
        std::lock_guard motion(MotionMutex);
        Window = next;
        InputTarget = nullptr;
        InvalidateMotion();
    }

    inline void Sample(int32_t& x, int32_t& y)
    {
        std::lock_guard registration(RegistrationMutex);
        RefreshGameWindow();
        if (!Window)
            return;
        RAWINPUTDEVICE mouse{};
        const auto state = Query(mouse);
        if (state == Registration::Error)
        {
            // The native sample is used this frame. Its movement must not be replayed on recovery.
            std::lock_guard motion(MotionMutex);
            InvalidateMotion();
            return;
        }
        const bool front = Front();
        HWND target = nullptr;
        if (state == Registration::Mouse)
        {
            target = mouse.hwndTarget ? mouse.hwndTarget : Window;
            DWORD owner = 0;
            GetWindowThreadProcessId(target, &owner);
            if (owner != GetCurrentProcessId() || (target != Window && target != FallbackWindow && !Observe(target)))
                target = nullptr;
        }
        else
        {
            Original.reset();
            if (front)
            {
                if (!FallbackWindow)
                    FallbackWindow = CreateRawInputWindow(L"WFPGtaRawMouse", FallbackProc);
                const RAWINPUTDEVICE device{ 1, 2, RIDEV_INPUTSINK, FallbackWindow };
                if (FallbackWindow && RegisterRawInputDevices(&device, 1, sizeof(device)))
                    target = FallbackWindow;
            }
        }
        bool relative = false;
        {
            std::lock_guard motion(MotionMutex);
            if (std::exchange(InputTarget, target) != target || !front || !target)
                InvalidateMotion();
            relative = Pending.relative;
            if (relative)
            {
                const auto axis = [](int64_t value) { return static_cast<int32_t>(std::clamp<int64_t>(value, INT32_MIN, INT32_MAX)); };
                x = axis(std::exchange(Pending.x, int64_t{ 0 }));
                y = axis(std::exchange(Pending.y, int64_t{ 0 }));
            }
        }
        if (state == Registration::Mouse)
            SetLegacy(mouse, !relative || !Playing || !Playing());
    }

    inline LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR)
    {
        if (message == WM_INPUT)
            ReadInput(window, lParam, false);
        if (message == WM_KILLFOCUS || (message == WM_ACTIVATE && LOWORD(wParam) == WA_INACTIVE) ||
            (message == WM_ACTIVATEAPP && !wParam))
        {
            {
                std::lock_guard lock(MotionMutex);
                if (Window == window)
                    InvalidateMotion();
            }
            RestoreLegacy(window);
        }
        if (message == WM_NCDESTROY)
        {
            RemoveWindowSubclass(window, WindowProc, GameWindowSubclass);
            RestoreLegacy(window);
            std::lock_guard registration(RegistrationMutex);
            if (Window == window)
            {
                std::lock_guard motion(MotionMutex);
                Window = InputTarget = nullptr;
                InvalidateMotion();
                if (FallbackWindow)
                    PostMessageW(FallbackWindow, ReleaseReaderMessage, 0, 0);
            }
        }
        return DefSubclassProc(window, message, wParam, lParam);
    }
}
