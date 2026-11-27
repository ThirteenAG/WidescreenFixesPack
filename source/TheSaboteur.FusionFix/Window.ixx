module;

#include "stdafx.h"
#include "FixSettings.hpp"
#include "RuntimeBridge.hpp"
#include <dinput.h>

export module Window;

export namespace SaboteurWindow
{
    inline SafetyHookInline ConfigurationHook;
    inline int Mode = 2;
    inline int Width = 0, Height = 0;

    inline std::pair<int, int> MonitorSize()
    {
        MONITORINFO info{ sizeof(info) };
        const auto window = SaboteurRuntime::GetWindow();
        const auto monitor = window ? MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST)
            : MonitorFromPoint(POINT{}, MONITOR_DEFAULTTOPRIMARY);
        if (GetMonitorInfoW(monitor, &info))
            return { info.rcMonitor.right - info.rcMonitor.left, info.rcMonitor.bottom - info.rcMonitor.top };
        return { std::max(GetSystemMetrics(SM_CXSCREEN), 640), std::max(GetSystemMetrics(SM_CYSCREEN), 480) };
    }

    inline int __fastcall Configuration(void* self, void* edx, const char* name)
    {
        if (name)
        {
            if (!_stricmp(name, "Windowed"))
                return Mode != 0;
            const bool width = !_stricmp(name, "ScreenWidth");
            if (width || !_stricmp(name, "ScreenHeight"))
            {
                const auto desktop = MonitorSize();
                const auto requested = width ? Width : Height;
                return Mode != 2 && requested > 0 ? requested : width ? desktop.first : desktop.second;
            }
        }
        return ConfigurationHook.unsafe_fastcall<int>(self, edx, name);
    }

    inline void Install()
    {
        Mode = FixSettings().Int("MAIN.WindowMode");
        Width = FixSettings().Int("MAIN.ResX");
        Height = FixSettings().Int("MAIN.ResY");
        auto pattern = hook::pattern("8B 44 24 04 56 6A 01 8B F1 50 8D 4C 24 10 E8 ? ? ? ? 50 8B CE E8 ? ? ? ? 5E C2 04 00");
        ConfigurationHook = safetyhook::create_inline(pattern.get_first(), Configuration);
        if (Mode != 0)
        {
            // A windowed mouse must reacquire without taking exclusive control of the desktop pointer.
            auto mouse = hook::pattern("A1 ? ? ? ? 8B 15 ? ? ? ? 8B 08 6A 05 52 50 8B 41 34 FF D0 A1 ? ? ? ? 85 C0 74 08 8B 08 8B 51 1C 50 FF D2 A1 ? ? ? ? 56 6A 01 6A 00 68");
            injector::WriteMemory<uint8_t>(mouse.get_first<uint8_t>(14), DISCL_NONEXCLUSIVE | DISCL_FOREGROUND, true);
        }
    }
}
