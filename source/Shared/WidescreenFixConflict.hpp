#pragma once
#include <filesystem>
#include <initializer_list>
#include <string>
#include <string_view>
#include <vector>

// The Widescreen Fix of a game became its Fusion Fix. Both patch the same game code, so the Fusion Fix stays off
// while the old <Game>.WidescreenFix.asi (or another plugin of the old package) is still installed, and says so.
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

    // True when the old Widescreen Fix is installed; the caller then doesn't start. otherFiles: further plugins the old
    // Widescreen Fix package shipped whose work the Fusion Fix now does.
    inline bool Found(std::initializer_list<std::wstring_view> otherFiles = {})
    {
        std::vector<std::wstring> fileNames;
        if (auto fileName = OldFileName(); !fileName.empty())
            fileNames.push_back(std::move(fileName));
        for (auto fileName : otherFiles)
            fileNames.emplace_back(fileName);
        std::wstring paths;
        size_t found = 0;
        for (const auto& fileName : fileNames)
        {
            if (auto path = Find(fileName); !path.empty())
            {
                paths += L"\n" + path.wstring();
                ++found;
            }
        }
        if (!found)
            return false;
        const auto title = GetThisModuleName<std::wstring>();
        auto text = std::wstring(L"The old Widescreen Fix is still installed. It is now part of the Fusion Fix, and both can't run together.\n\n");
        text += found > 1 ? L"Delete these files, then start the game again:" : L"Delete this file, then start the game again:";
        text += paths + L"\n\nClick OK to play now without the Fusion Fix.";
        MessageBoxW(nullptr, text.c_str(), title.c_str(), MB_OK | MB_ICONWARNING | MB_TOPMOST | MB_SETFOREGROUND);
        return true;
    }
}
