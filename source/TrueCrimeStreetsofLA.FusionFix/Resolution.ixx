module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module Resolution;

import ComVars;
import Window;
import Device;

// The game's Display options pick from five fixed 4:3 modes and look the choice up in the adapter's mode list (a
// missing mode crashed the game). The options list the monitor's modes instead, any choice is applied while the
// game runs, and the game keeps it in TrueCrime.INI. Borderless always uses the desktop resolution.
export namespace TrueCrimeResolution
{
    // The options menu has room for 16 entries
    inline constexpr int MaximumModes = 16;
    inline int Modes[MaximumModes][2]{};
    inline int ModeCount = 0;
    inline uint8_t Mode[0x40]{};
    inline SafetyHookInline FindModeHook;

    inline void SetScreen(int width, int height)
    {
        Screen.Width = width;
        Screen.Height = height;
        Screen.fWidth = static_cast<float>(width);
        Screen.fHeight = static_cast<float>(height);
        Screen.fAspectRatio = Screen.fWidth / Screen.fHeight;
        Screen.fAspectRatioDiff = Screen.fAspectRatio / (4.0f / 3.0f);
        Screen.Width43 = static_cast<int>(Screen.fHeight * (4.0f / 3.0f));
        Screen.fWidth43 = static_cast<float>(Screen.Width43);
    }

    inline std::tuple<int, int> Desktop()
    {
        auto [width, height] = GetDesktopRes();
        if (width <= 0 || height <= 0)
            return { 640, 480 };
        return { width, height };
    }

    inline bool Listed(int width, int height)
    {
        for (int i = 0; i < ModeCount; ++i)
        {
            if (Modes[i][0] == width && Modes[i][1] == height)
                return true;
        }
        return false;
    }

    // Distinct modes of the primary monitor; with more than the menu holds, those of the desktop's shape and the
    // largest others stay
    inline void BuildModes(int width, int height)
    {
        std::vector<std::pair<int, int>> modes;
        DEVMODEW mode{};
        mode.dmSize = sizeof(mode);
        for (DWORD i = 0; EnumDisplaySettingsW(nullptr, i, &mode); ++i)
        {
            const std::pair<int, int> size{ int(mode.dmPelsWidth), int(mode.dmPelsHeight) };
            if (mode.dmBitsPerPel >= 32 && size.first >= 640 && size.second >= 480 &&
                std::find(modes.begin(), modes.end(), size) == modes.end())
                modes.push_back(size);
        }
        const auto [desktopWidth, desktopHeight] = Desktop();
        for (const auto& size : { std::pair{ desktopWidth, desktopHeight }, std::pair{ width, height } })
        {
            if (std::find(modes.begin(), modes.end(), size) == modes.end())
                modes.push_back(size);
        }
        const auto shape = [&](const std::pair<int, int>& size)
        {
            return std::abs(size.first * desktopHeight - size.second * desktopWidth) * 100 <= desktopWidth * size.second;
        };
        const auto keep = [&](const std::pair<int, int>& size)
        {
            return size == std::pair{ width, height } || size == std::pair{ desktopWidth, desktopHeight };
        };
        std::stable_sort(modes.begin(), modes.end(), [&](const auto& a, const auto& b)
        {
            if (keep(a) != keep(b))
                return keep(a);
            if (shape(a) != shape(b))
                return shape(a);
            return a.first * a.second > b.first * b.second;
        });
        if (modes.size() > MaximumModes)
            modes.resize(MaximumModes);
        std::sort(modes.begin(), modes.end(), [](const auto& a, const auto& b)
        {
            return a.first != b.first ? a.first < b.first : a.second < b.second;
        });
        ModeCount = int(modes.size());
        for (int i = 0; i < ModeCount; ++i)
        {
            Modes[i][0] = modes[i].first;
            Modes[i][1] = modes[i].second;
        }
    }

    // Startup: the fix's resolution settings, else the game's own choice from TrueCrime.INI, else the desktop. 640x480
    // is the game's default for a missing entry and starts at the desktop resolution.
    inline std::tuple<int, int> Startup()
    {
        if (TrueCrimeWindow::Mode == 2)
            return Desktop();
        const int width = FixSettings().Int("MAIN.ResX");
        const int height = FixSettings().Int("MAIN.ResY");
        if (width >= 640 && height >= 480 && width <= 16384 && height <= 16384)
            return { width, height };
        wchar_t path[MAX_PATH]{};
        GetModuleFileNameW(nullptr, path, MAX_PATH);
        const auto ini = (std::filesystem::path(path).parent_path() / L"TrueCrime.INI").wstring();
        const int iniWidth = GetPrivateProfileIntW(L"Renderer", L"ScreenWidth", 0, ini.c_str());
        const int iniHeight = GetPrivateProfileIntW(L"Renderer", L"ScreenHeight", 0, ini.c_str());
        const auto [desktopWidth, desktopHeight] = Desktop();
        if (iniWidth >= 640 && iniHeight >= 480 && !(iniWidth == 640 && iniHeight == 480) &&
            iniWidth <= desktopWidth && iniHeight <= desktopHeight)
            return { iniWidth, iniHeight };
        return { desktopWidth, desktopHeight };
    }

    // The adapter's display modes: width, height, bit depth at +0, +4, +12, next at +60. The game uses the returned
    // entry's size for the device; a copy carries the requested size, so any size works in a window.
    inline void* __fastcall FindMode(uint8_t* adapter, void*, int width, int height, int depth)
    {
        if (TrueCrimeWindow::Mode == 2)
            std::tie(width, height) = Desktop();
        uint8_t* found = nullptr;
        for (auto entry = *reinterpret_cast<uint8_t**>(adapter + 0x1C); entry; entry = *reinterpret_cast<uint8_t**>(entry + 0x3C))
        {
            const auto values = reinterpret_cast<int*>(entry);
            if (values[0] == width && values[1] == height && (values[3] == depth || !found))
                found = entry;
        }
        if (!found)
            found = *reinterpret_cast<uint8_t**>(adapter + 0x1C);
        if (!found)
            return nullptr;
        std::memcpy(Mode, found, sizeof(Mode));
        reinterpret_cast<int*>(Mode)[0] = width;
        reinterpret_cast<int*>(Mode)[1] = height;
        return Mode;
    }

    // Every instruction that addresses the game's mode table or its count gets the fix's
    inline void Redirect(uintptr_t from, uintptr_t to)
    {
        char text[16]{};
        std::snprintf(text, sizeof(text), "%02X %02X %02X %02X", unsigned(from & 0xFF), unsigned((from >> 8) & 0xFF),
            unsigned((from >> 16) & 0xFF), unsigned(from >> 24));
        auto references = hook::pattern(text);
        if (references.empty())
        {
            (void)hook::pattern(references).get_first();
            std::abort();
        }
        references.for_each_result([to](hook::pattern_match match)
        {
            injector::WriteMemory<uintptr_t>(match.get<void>(0), to, true);
        });
    }

    inline void Init(int width, int height)
    {
        SetScreen(width, height);
        BuildModes(width, height);

        // Mode index lookup of the options: count, table
        auto lookup = hook::pattern("8B 0D ? ? ? ? 33 C0 85 C9 7E ? 8B 15 ? ? ? ? 56 8B 35 ? ? ? ? 8D A4 24 00 00 00 00 3B 14 C5");
        const auto count = *lookup.get_first<uintptr_t>(2);
        const auto table = *lookup.get_first<uintptr_t>(35);
        Redirect(table, reinterpret_cast<uintptr_t>(&Modes[0][0]));
        Redirect(table + 4, reinterpret_cast<uintptr_t>(&Modes[0][1]));
        Redirect(count, reinterpret_cast<uintptr_t>(&ModeCount));

        FindModeHook = safetyhook::create_inline(hook::pattern("8B 41 1C 85 C0 56 74 21 8B 4C 24 10").get_first(), FindMode);

        // The game reads the resolution from TrueCrime.INI at start: it gets the startup resolution
        auto ini = hook::pattern("8B D8 53 8D ? 24 10 68 ? ? ? ? ? E8");
        struct IniWidth
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.ebx = Screen.Width;
                regs.edx = regs.esp + 0x10;
            }
        }; injector::MakeInline<IniWidth>(ini.count(3).get(1).get<void>(0), ini.count(3).get(1).get<void>(7));
        injector::WriteMemory<uint8_t>(ini.count(3).get(1).get<uint8_t>(5), 0x53, true); // push ebx
        struct IniHeight
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.ebx = Screen.Height;
                regs.ecx = regs.esp + 0x10;
            }
        }; injector::MakeInline<IniHeight>(ini.count(3).get(2).get<void>(0), ini.count(3).get(2).get<void>(7));
        injector::WriteMemory<uint8_t>(ini.count(3).get(2).get<uint8_t>(5), 0x53, true); // push ebx

        // The device's back buffer follows the game's choice
        TrueCrimeDevice::Resolution = [](int newWidth, int newHeight)
        {
            if (TrueCrimeWindow::Mode == 2 || (newWidth == Screen.Width && newHeight == Screen.Height))
                return;
            SetScreen(newWidth, newHeight);
            TrueCrimeWindow::Resize(newWidth, newHeight);
        };
    }
}
