module;
#include <stdafx.h>
#include "FixSettings.hpp"

export module Resolution;
import ComVars;
import Window;
import NativeMenuText;


export namespace OnimushaResolution
{
    struct Mode
    {
        int width, height;
        UINT refresh;
    };
    inline std::vector<Mode> Modes;
    inline std::mutex ModesLock;
    inline int (__cdecl* Print)(void*, int16_t, int16_t, char, uint8_t, char, char, const char*, ...) = nullptr;

    inline void Refresh()
    {
        MONITORINFOEXW monitor{};
        monitor.cbSize = sizeof(monitor);
        GetMonitorInfoW(MonitorFromWindow(OnimushaWindow::Window, MONITOR_DEFAULTTOPRIMARY), &monitor);
        std::map<std::pair<int, int>, UINT> unique;
        DEVMODEW mode{};
        mode.dmSize = sizeof(mode);
        for (DWORD index = 0; EnumDisplaySettingsW(monitor.szDevice, index, &mode); ++index)
            if (mode.dmBitsPerPel == 32 && mode.dmPelsWidth >= 640 && mode.dmPelsHeight >= 480)
            {
                auto& rate = unique[{ int(mode.dmPelsWidth), int(mode.dmPelsHeight) }];
                rate = std::max(rate, UINT(mode.dmDisplayFrequency));
            }
        unique.try_emplace(std::make_pair(OnimushaWindow::Width, OnimushaWindow::Height), 0);
        std::vector<Mode> modes;
        for (const auto& [size, rate] : unique)
            modes.push_back({ size.first, size.second, rate });
        std::sort(modes.begin(), modes.end(), [](const Mode& left, const Mode& right)
        {
            return std::make_tuple(int64_t(left.width) * left.height, left.width, left.height) <
                std::make_tuple(int64_t(right.width) * right.height, right.width, right.height);
        });
        const std::lock_guard lock(ModesLock);
        Modes = std::move(modes);
    }

    inline void Move(int direction)
    {
        if (OnimushaWindow::Mode == 2)
            return;
        int width = FixSettings().Int("MAIN.ResX"), height = FixSettings().Int("MAIN.ResY");
        if (width <= 0 || height <= 0)
        {
            width = OnimushaWindow::Width;
            height = OnimushaWindow::Height;
        }
        Mode selected{};
        {
            const std::lock_guard lock(ModesLock);
            if (Modes.empty())
                return;
            const auto current = std::find_if(Modes.begin(), Modes.end(), [&](const Mode& mode)
            {
                return mode.width == width && mode.height == height;
            });
            const auto index = current == Modes.end() ? 0 : int(current - Modes.begin());
            selected = Modes[(index + int(Modes.size()) + direction) % Modes.size()];
        }
        OnimushaWindow::SetResolution(selected.width, selected.height);
    }

    inline int __cdecl Label(void* font, int16_t x, int16_t y, char colour, uint8_t alpha, char width, char height,
        const char*, ...)
    {
        const auto label = std::to_wstring(OnimushaWindow::Width) + L" x " + std::to_wstring(OnimushaWindow::Height);
        const auto encoded = OnimushaMenu::Text::Encode(label, OnimushaMenu::NativeLanguage == 9 || OnimushaMenu::NativeLanguage == 10);
        return Print(font, x, y, colour, alpha, width, height, "%s", encoded.c_str());
    }

    inline void Init()
    {
        OnimushaWindow::DisplayChanged = Refresh;
        OnimushaWindow::RefreshRate = [](int width, int height)
        {
            Refresh();
            const std::lock_guard lock(ModesLock);
            for (const auto& mode : Modes)
                if (mode.width == width && mode.height == height)
                    return mode.refresh;
            return UINT(0);
        };
        auto labels = hook::pattern("68 ? ? ? ? 6A 15 6A 13 68 80 00 00 00 6A 00 68 ? ? ? ? 68 64 01 00 00 68 ? ? ? ? E8");
        labels.count_hint(4);
        Print = reinterpret_cast<decltype(Print)>(injector::GetBranchDestination(hook::pattern(labels).get_first(31)).as_int());
        for (size_t i = 0; i < 4; ++i)
            injector::MakeCALL(labels.get(i).get<void>(31), Label, true);
    }
}
