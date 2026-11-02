#include <stdafx.h>
#include <map>
#include "ResolutionHooks.hpp"

namespace SecondSightResolution
{
    const std::map<std::pair<int, int>, int>& Modes()
    {
        static const auto modes = []()
        {
            std::map<std::pair<int, int>, int> list;
            DEVMODEW mode{};
            mode.dmSize = sizeof(mode);
            for (DWORD i = 0; EnumDisplaySettingsW(nullptr, i, &mode); ++i)
            {
                if (mode.dmBitsPerPel < 32 || mode.dmPelsWidth < 640 || mode.dmPelsHeight < 480)
                    continue;
                auto& refresh = list[{ int(mode.dmPelsWidth), int(mode.dmPelsHeight) }];
                refresh = (std::max)(refresh, int(mode.dmDisplayFrequency));
            }
            return list;
        }();
        return modes;
    }

    // Replaces the game's mode filter (640x480 to 1600x1200 in 4:3, 60 to 85 Hz)
    BOOL __cdecl Listed(int width, int height, int refresh)
    {
        const auto& modes = Modes();
        const auto mode = modes.find({ width, height });
        if (mode == modes.end())
            return width >= 640 && height >= 480 && modes.empty() && refresh >= 60;
        return refresh == mode->second;
    }

    // The caller keeps the mode in ecx/edx across the call
    __declspec(naked) void ListedPreserving()
    {
        __asm
        {
            push ecx
            push edx
            push dword ptr [esp + 20]
            push dword ptr [esp + 20]
            push dword ptr [esp + 20]
            call Listed
            add esp, 12
            pop edx
            pop ecx
            ret
        }
    }

}
