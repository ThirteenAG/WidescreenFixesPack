#pragma once
#include <filesystem>
#include <string>

// The Widescreen Fix of a game became its Fusion Fix. Both patch the same game code, so the Fusion Fix stays off
// while the old <Game>.WidescreenFix.asi is still installed, and says so.
namespace WidescreenFixConflict
{
    // "<Game>.FusionFix.asi" -> "<Game>.WidescreenFix.asi"
    inline std::wstring OldFileName()
    {
        auto name = GetThisModuleName<std::wstring>();
        auto pos = name.rfind(L".FusionFix");
        if (pos == std::wstring::npos)
            return {};
        return name.substr(0, pos) + L".WidescreenFix.asi";
    }

    inline std::filesystem::path Find(const std::wstring& fileName)
    {
        if (auto module = GetModuleHandleW(fileName.c_str()))
            return GetModulePath<std::filesystem::path>(module);
        const auto gamePath = GetExeModulePath();
        for (const auto& directory : { GetThisModulePath(), gamePath, gamePath / L"scripts", gamePath / L"plugins" })
        {
            std::error_code ec;
            if (std::filesystem::is_regular_file(directory / fileName, ec))
                return directory / fileName;
        }
        return {};
    }

    // True when the old Widescreen Fix is installed; the caller then doesn't start
    inline bool Found()
    {
        const auto fileName = OldFileName();
        if (fileName.empty())
            return false;
        const auto path = Find(fileName);
        if (path.empty())
            return false;
        const auto title = GetThisModuleName<std::wstring>();
        const auto text = L"The Widescreen Fix for this game is now part of its Fusion Fix, and both can't work together.\n\n"
            L"Please delete the old Widescreen Fix:\n" + path.wstring() +
            L"\n\nThe Fusion Fix stays off until then.";
        MessageBoxW(nullptr, text.c_str(), title.c_str(), MB_OK | MB_ICONWARNING | MB_TOPMOST | MB_SETFOREGROUND);
        return true;
    }
}
