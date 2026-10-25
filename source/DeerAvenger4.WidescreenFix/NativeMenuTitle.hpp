#pragma once

#include "NativeMenuContext.hpp"
#include <array>

namespace DeerMenu
{
    struct TitleContext
    {
        inline static constexpr std::array<int, 6> IDs{1, 2, 1002, 1003, 1000, 1001};
        HWND dialog = nullptr;
        std::array<HWND, 6> controls{};
        DWORD thread = 0;

        static bool Class(HWND window, const wchar_t* name)
        {
            wchar_t actual[32]{};
            return window && GetClassNameW(window, actual, int(std::size(actual))) > 0 &&
                lstrcmpiW(actual, name) == 0;
        }

        bool Alive() const
        {
            DWORD process = 0;
            if (!dialog || !IsWindow(dialog) || !Class(dialog, L"#32770") ||
                GetWindowThreadProcessId(dialog, &process) != thread || process != GetCurrentProcessId() ||
                reinterpret_cast<HMODULE>(GetWindowLongPtrW(dialog, GWLP_HINSTANCE)) != GetModuleHandleW(nullptr))
                return false;
            for (size_t i = 0; i < IDs.size(); ++i)
                if (!controls[i] || GetDlgItem(dialog, IDs[i]) != controls[i] ||
                    GetParent(controls[i]) != dialog || GetDlgCtrlID(controls[i]) != IDs[i] ||
                    !Class(controls[i], i < 3 ? L"Button" : L"ComboBox"))
                    return false;
            return true;
        }

        bool Capture(HWND source)
        {
            dialog = source;
            DWORD process = 0;
            thread = GetWindowThreadProcessId(source, &process);
            for (size_t i = 0; i < IDs.size(); ++i)
                controls[i] = GetDlgItem(source, IDs[i]);
            return thread == GetCurrentThreadId() && process == GetCurrentProcessId() && Alive();
        }

        bool Space(RECT& result) const
        {
            if (!Alive())
                return false;
            RECT client{}, exit{}, modes{};
            if (!GetClientRect(dialog, &client) || !GetWindowRect(controls[2], &exit) ||
                !GetWindowRect(controls[5], &modes))
                return false;
            SetLastError(0);
            if (!MapWindowPoints(nullptr, dialog, reinterpret_cast<POINT*>(&exit), 2) && GetLastError())
                return false;
            SetLastError(0);
            if (!MapWindowPoints(nullptr, dialog, reinterpret_cast<POINT*>(&modes), 2) && GetLastError())
                return false;
            const auto gap = std::max(4L, (exit.bottom - exit.top) / 2);
            result = {std::max(exit.right + gap, client.left), std::max(exit.top, client.top),
                std::min(modes.left - gap, client.right), std::min(exit.bottom, client.bottom)};
            // The native Exit button sits at 11,344 (62x22); the mode combo starts at 250,336.
            // Derive the spare horizontal band from actual child rectangles so
            // window/DPI coordinates stay in the same Win32 native space; keep the new row inside the client.
            return Alive() && result.left >= client.left && result.top >= client.top &&
                result.right <= client.right && result.bottom <= client.bottom &&
                result.right - result.left >= 80 && result.bottom - result.top >= 12 &&
                result.bottom - result.top <= 128;
        }
    };
}
