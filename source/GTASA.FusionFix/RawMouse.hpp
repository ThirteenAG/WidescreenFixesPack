#pragma once
#include "rawinput.hpp"
#include <cwchar>
#include <mutex>

namespace GtaRawMouse
{
    inline HWND Window = nullptr;
    inline HWND FallbackWindow = nullptr;
    inline std::atomic<HWND> InputTarget = nullptr;
    inline WNDPROC Previous = nullptr;
    inline SafetyHookMid MouseHook;
    inline std::once_flag Fallback;
    inline constexpr auto PreviousProperty = L"WFP.RawMouse.Previous";

    inline void Clear()
    {
        RawInputHandler<int32_t>::RawMouseDeltaX.exchange(0);
        RawInputHandler<int32_t>::RawMouseDeltaY.exchange(0);
    }

    inline void ReadInput(HWND window, UINT message, LPARAM lParam)
    {
        if (message != WM_INPUT || window != InputTarget.load() ||
            GetForegroundWindow() != GetAncestor(Window, GA_ROOT))
            return;
        RAWINPUT input{};
        UINT size = sizeof(input);
        if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, &input, &size, sizeof(RAWINPUTHEADER)) != UINT(-1) &&
            input.header.dwType == RIM_TYPEMOUSE && !(input.data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE))
        {
            RawInputHandler<int32_t>::RawMouseDeltaX.fetch_add(input.data.mouse.lLastX);
            RawInputHandler<int32_t>::RawMouseDeltaY.fetch_add(input.data.mouse.lLastY);
        }
    }

    inline LRESULT CALLBACK InputWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        auto previous = reinterpret_cast<WNDPROC>(GetPropW(window, PreviousProperty));
        ReadInput(window, message, lParam);
        auto result = previous ? CallWindowProcW(previous, window, message, wParam, lParam) : DefWindowProcW(window, message, wParam, lParam);
        if (message == WM_NCDESTROY)
            RemovePropW(window, PreviousProperty);
        return result;
    }

    inline bool ObserveInputWindow(HWND window)
    {
        auto current = reinterpret_cast<WNDPROC>(GetWindowLongPtrW(window, GWLP_WNDPROC));
        auto previous = reinterpret_cast<WNDPROC>(GetPropW(window, PreviousProperty));
        if (current == InputWindowProc || (previous && current != previous))
            return true;
        if (!previous && !SetPropW(window, PreviousProperty, reinterpret_cast<HANDLE>(current)))
            return false;
        SetLastError(0);
        if (!SetWindowLongPtrW(window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(InputWindowProc)) && GetLastError())
        {
            RemovePropW(window, PreviousProperty);
            return false;
        }
        return true;
    }

    inline bool InputReady()
    {
        UINT count = 0;
        if (GetRegisteredRawInputDevices(nullptr, &count, sizeof(RAWINPUTDEVICE)) == UINT(-1))
            return false;
        static std::vector<RAWINPUTDEVICE> devices;
        devices.resize(count);
        if (count && GetRegisteredRawInputDevices(devices.data(), &count, sizeof(RAWINPUTDEVICE)) == UINT(-1))
            return false;
        for (UINT i = 0; i < count; ++i)
        {
            const auto& device = devices[i];
            if (device.usUsagePage != HID_USAGE_PAGE_GENERIC || device.usUsage != HID_USAGE_GENERIC_MOUSE)
                continue;
            auto target = device.hwndTarget ? device.hwndTarget : Window;
            DWORD owner = 0;
            GetWindowThreadProcessId(target, &owner);
            if (owner != GetCurrentProcessId())
                return false;
            InputTarget = target;
            wchar_t name[64]{};
            GetClassNameW(target, name, int(std::size(name)));
            if (std::wcscmp(name, L"WFPRawInputHandler") == 0)
            {
                FallbackWindow = target;
                return true;
            }
            // DirectInput can register its own raw-input window when it acquires
            // the mouse. Observe it without taking its buttons or wheel away.
            return target == Window || ObserveInputWindow(target);
        }
        if (GetForegroundWindow() == GetAncestor(Window, GA_ROOT))
        {
            if (FallbackWindow && IsWindow(FallbackWindow))
            {
                RAWINPUTDEVICE device{ HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_MOUSE, RIDEV_INPUTSINK, FallbackWindow };
                RegisterRawInputDevices(&device, 1, sizeof(device));
            }
            else
            {
                std::call_once(Fallback, []() { RawInputHandler<int32_t>::RegisterRawInput(Window, 1.0f, true); });
            }
        }
        return false;
    }

    inline LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        ReadInput(window, message, lParam);
        if (message == WM_KILLFOCUS || (message == WM_ACTIVATEAPP && !wParam))
            Clear();
        return CallWindowProcW(Previous, window, message, wParam, lParam);
    }

    inline void Install(HWND window)
    {
        // Replace the native relative packet before its invert-axis handling.
        // Buttons, wheel, menu cursor and the game's sensitivity remain native.
        auto packet = hook::pattern("8B 54 24 08 0F AF D0 8B 44 24 0C 0F AF C1 8A 4C 24 14 89 54 24 04 8A 54 24 15");
        static constexpr uint8_t x = 0x08;
        static constexpr uint8_t y = 0x0C;
        Window = window;
        Previous = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WindowProc)));
        MouseHook = safetyhook::create_mid(packet.get_first(), [](SafetyHookContext& context)
        {
            if (!InputReady())
            {
                Clear();
                return;
            }
            *reinterpret_cast<int32_t*>(context.esp + x) = RawInputHandler<int32_t>::RawMouseDeltaX.exchange(0);
            *reinterpret_cast<int32_t*>(context.esp + y) = RawInputHandler<int32_t>::RawMouseDeltaY.exchange(0);
        });
    }
}
