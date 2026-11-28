#pragma once
#include <windows.h>
#include <tlhelp32.h>

// Fusion Rain Droplets draws its drops over the finished frame in the games it has no adapter for. A fix tells it while
// a menu of the game is open, and the drops stay off the menu until it closes.
namespace RainDroplets
{
    inline void MenuOpen(bool open)
    {
        static bool last = false;
        if (open == last) return;
        last = open;
        using SetMenuOpen = void (__cdecl*)(int);
        static SetMenuOpen set = nullptr;
        // Looked up when the state changes, until a droplets plugin is found; nothing is called without one
        if (!set)
        {
            const auto snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
            if (snapshot == INVALID_HANDLE_VALUE) return;
            MODULEENTRY32W entry{ sizeof(entry) };
            for (auto found = Module32FirstW(snapshot, &entry); found && !set; found = Module32NextW(snapshot, &entry))
                set = reinterpret_cast<SetMenuOpen>(GetProcAddress(entry.hModule, "FusionRainDroplets_SetMenuOpen"));
            CloseHandle(snapshot);
        }
        if (set) set(open ? 1 : 0);
    }
}
