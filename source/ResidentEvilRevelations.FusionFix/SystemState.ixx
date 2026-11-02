module;

#include <stdafx.h>
#include <atomic>
#include <cstdint>
#include <iathook.hpp>

export module SystemState;

export namespace RevSystemState
{
    inline std::atomic<decltype(&::SystemParametersInfoA)> NativeParameters{ ::SystemParametersInfoA };
    inline std::atomic<decltype(&::DefWindowProcA)> NativeWindowA{ ::DefWindowProcA };
    inline std::atomic<decltype(&::DefWindowProcW)> NativeWindowW{ ::DefWindowProcW };
    inline std::atomic<uint32_t> BlockedPreferenceWrites{ 0 };
    inline bool Installed = false;

    inline BOOL WINAPI Parameters(UINT action, UINT parameter, PVOID value, UINT flags)
    {
        // The game's focus and shutdown paths must not change the user's preference.
        if (action == SPI_SETSCREENSAVEACTIVE)
        {
            ++BlockedPreferenceWrites;
            return TRUE;
        }
        return NativeParameters.load()(action, parameter, value, flags);
    }

    inline bool ScreenSaverCommand(UINT message, WPARAM parameter, bool foreground, bool owned)
    {
        return message == WM_SYSCOMMAND && (parameter & 0xFFF0) == SC_SCREENSAVE &&
            !(parameter & SCF_ISSECURE) && foreground && owned;
    }

    inline bool Suppress(HWND window, UINT message, WPARAM parameter)
    {
        if (message != WM_SYSCOMMAND || (parameter & 0xFFF0) != SC_SCREENSAVE)
            return false;
        DWORD process = 0;
        GetWindowThreadProcessId(window, &process);
        return ScreenSaverCommand(message, parameter, GetForegroundWindow() == window, process == GetCurrentProcessId());
    }

    inline LRESULT WINAPI WindowA(HWND window, UINT message, WPARAM parameter, LPARAM value)
    {
        const auto error = GetLastError();
        const auto suppress = Suppress(window, message, parameter);
        SetLastError(error);
        return suppress ? 0 : NativeWindowA.load()(window, message, parameter, value);
    }

    inline LRESULT WINAPI WindowW(HWND window, UINT message, WPARAM parameter, LPARAM value)
    {
        const auto error = GetLastError();
        const auto suppress = Suppress(window, message, parameter);
        SetLastError(error);
        return suppress ? 0 : NativeWindowW.load()(window, message, parameter, value);
    }

    inline void Init()
    {
        auto original = IATHook::Replace(GetModuleHandleW(nullptr), "USER32.dll",
            std::forward_as_tuple("SystemParametersInfoA", Parameters),
            std::forward_as_tuple("DefWindowProcA", WindowA),
            std::forward_as_tuple("DefWindowProcW", WindowW));
        if (const auto found = original.find("SystemParametersInfoA"); found != original.end())
        {
            NativeParameters.store(reinterpret_cast<decltype(&::SystemParametersInfoA)>(found->second.get()));
            Installed = true;
        }
        if (const auto found = original.find("DefWindowProcA"); found != original.end())
            NativeWindowA.store(reinterpret_cast<decltype(&::DefWindowProcA)>(found->second.get()));
        if (const auto found = original.find("DefWindowProcW"); found != original.end())
            NativeWindowW.store(reinterpret_cast<decltype(&::DefWindowProcW)>(found->second.get()));
    }
}
