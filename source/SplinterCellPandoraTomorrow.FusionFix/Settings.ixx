module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <cctype>
#include <cwctype>

export module Settings;

import ComVars;
import Gamepad;

namespace
{
    std::filesystem::path InitialUserTemplate(const std::filesystem::path& directory, std::wstring command)
    {
        mINI::INIStructure config;
        if (!mINI::INIFile(directory / "SplinterCell2.ini").read(config))
            mINI::INIFile(directory / "Default.ini").read(config);
        std::string language = config["Engine.Engine"].get("Language");
        std::transform(language.begin(), language.end(), language.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        std::transform(command.begin(), command.end(), command.begin(), [](wchar_t c) { return static_cast<wchar_t>(std::towlower(c)); });
        for (size_t at = 0; (at = command.find(L"language=", at)) != std::wstring::npos; ++at)
        {
            if (at)
            {
                const auto prefix = command[at - 1];
                if (prefix == L'-' || prefix == L'/')
                {
                    if (at > 1 && !std::iswspace(command[at - 2]) && command[at - 2] != L'"')
                        continue;
                }
                else if (!std::iswspace(prefix) && prefix != L'"')
                    continue;
            }
            auto start = at + 9;
            if (start < command.size() && command[start] == L'"')
                ++start;
            auto end = start;
            while (end < command.size() && !std::iswspace(command[end]) && command[end] != L'"')
                ++end;
            language.clear();
            for (auto index = start; index < end; ++index)
            {
                if (command[index] > 0x7F)
                {
                    language.clear();
                    break;
                }
                language.push_back(static_cast<char>(command[index]));
            }
            break;
        }
        const wchar_t* folder = nullptr;
        if (language == "int" || language == "eng") folder = L"English";
        else if (language == "fra") folder = L"French";
        else if (language == "deu") folder = L"German";
        else if (language == "ita") folder = L"Italian";
        else if (language == "esp") folder = L"Spanish";
        if (folder)
        {
            const auto localized = directory / "Localization" / folder / "SplinterCell2User.ini";
            std::error_code ec;
            if (std::filesystem::exists(localized, ec) && !ec)
                return localized;
        }
        return directory / "DefaultUser.ini";
    }

    void CreateInitialIni(const std::filesystem::path& source, const std::filesystem::path& target, const char* requiredSection)
    {
        std::error_code ec;
        if (std::filesystem::exists(target, ec) || ec)
            return;
        mINI::INIStructure defaults;
        if (!mINI::INIFile(source).read(defaults) || !defaults.has(requiredSection))
            return;
        const auto input = CreateFileW(source.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (input == INVALID_HANDLE_VALUE)
            return;
        const auto output = CreateFileW(target.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (output == INVALID_HANDLE_VALUE)
        {
            CloseHandle(input);
            return;
        }
        std::array<char, 4096> buffer{};
        DWORD read = 0;
        bool success = true;
        while (success)
        {
            if (!ReadFile(input, buffer.data(), static_cast<DWORD>(buffer.size()), &read, nullptr))
                success = false;
            else if (!read)
                break;
            else
            {
                DWORD written = 0;
                success = WriteFile(output, buffer.data(), read, &written, nullptr) && written == read;
            }
        }
        CloseHandle(output);
        CloseHandle(input);
        if (!success)
            DeleteFileW(target.c_str());
    }
}

class Settings
{
public:
    Settings()
    {
        WFP::onInitEvent() += []()
        {
            Gamepad::Init();
            auto& settings = FixSettings();
            asi_settings::LiveLink::Start();
            Screen.Width = settings.Int("MAIN.ResX");
            Screen.Height = settings.Int("MAIN.ResY");
            Screen.bRawInputMouse = settings.Int("MAIN.RawInputMouse") != 0;
            settings.OnChange("MAIN.MouseSensitivity", []
            {
                RawInputHandler<int32_t>::SetSensitivity(FixSettings().Float("MAIN.MouseSensitivity"));
            });
            Screen.bRawInputMouseRawData = settings.Int("MAIN.RawInputMouseRawData") != 0;
            Screen.nFMVWidescreenMode = settings.Int("MAIN.FMVWidescreenMode");
            Screen.bScopeWidescreenMode = settings.Int("MAIN.ScopeWidescreenMode") != 0;
            Screen.fHudAspectRatioConstraint = ParseWidescreenHudOffset(settings.Get("MAIN.HudAspectRatioConstraint"));
            Screen.nPostProcessFixedScale = settings.Int("MAIN.PostProcessFixedScale");
            Screen.fGrainScale = std::clamp(settings.Float("MAIN.GrainScale"), 0.0f, 1.0f);
            Screen.nShadowMapResolution = settings.Int("MAIN.ShadowMapResolution");
            Screen.nReflectionsResolution = settings.Int("MAIN.ReflectionsResolution");
            Screen.nBloomResolutionMultiplier = std::clamp(settings.Int("MAIN.BloomResolutionMultiplier"), 0, 4);
            fLightDistanceMultiplier = std::clamp(settings.Float("MAIN.LightDistanceMultiplier"), 1.0f, 8.0f);
            const auto gogglesColor = settings.Int("BONUS.GogglesLightColor");
            gColor.RGBA = gogglesColor == -1 ? settings.Int("BONUS.CustomGogglesLightColor") : gogglesColor;
            bSkipIntro = settings.Int("MAIN.SkipIntro") != 0;
            bSkipPressStartToContinue = settings.Int("MAIN.SkipPressStart") != 0;
            bEnableConsole = settings.Int("MAIN.EnableConsole") != 0;
            bEnableCheckpoints = settings.Int("MAIN.EnableCheckpoints") != 0;

            if (Screen.Width <= 0 || Screen.Height <= 0)
                std::tie(Screen.Width, Screen.Height) = GetDesktopRes();
            if (Screen.Width <= 0 || Screen.Height <= 0)
                std::tie(Screen.Width, Screen.Height) = std::make_tuple(640, 480);

            auto exePath = GetExeModulePath();

            auto userIniPath = exePath / "SplinterCell2User.ini";

            // Start with the game-language defaults so the pad bindings also apply on the first launch.
            CreateInitialIni(InitialUserTemplate(exePath, GetCommandLineW()), userIniPath, "Engine.Input");
            mINI::INIStructure userIni;
            mINI::INIFile userIniFile(userIniPath);
            if (userIniFile.read(userIni) && userIni.has("Engine.Input"))
            {
                for (const auto& [id, key] : std::array{
                    std::pair{ "INPUT.Aliases14", "Aliases[14]" },
                    std::pair{ "INPUT.Aliases15", "Aliases[15]" },
                    std::pair{ "INPUT.Aliases33", "Aliases[33]" },
                    std::pair{ "INPUT.Joy1", "Joy1" },
                    std::pair{ "INPUT.Joy2", "Joy2" },
                    std::pair{ "INPUT.Joy3", "Joy3" },
                    std::pair{ "INPUT.Joy4", "Joy4" },
                    std::pair{ "INPUT.Joy5", "Joy5" },
                    std::pair{ "INPUT.Joy6", "Joy6" },
                    std::pair{ "INPUT.Joy7", "Joy7" },
                    std::pair{ "INPUT.Joy8", "Joy8" },
                    std::pair{ "INPUT.Joy9", "Joy9" },
                    std::pair{ "INPUT.Joy10", "Joy10" },
                    std::pair{ "INPUT.Joy11", "Joy11" },
                    std::pair{ "INPUT.Joy12", "Joy12" },
                    std::pair{ "INPUT.Joy13", "Joy13" },
                    std::pair{ "INPUT.Joy14", "Joy14" },
                    std::pair{ "INPUT.Joy15", "Joy15" },
                    std::pair{ "INPUT.Joy16", "Joy16" },
                    std::pair{ "INPUT.JoyX", "JoyX" },
                    std::pair{ "INPUT.JoyY", "JoyY" },
                    std::pair{ "INPUT.JoyV", "JoyV" },
                    std::pair{ "INPUT.JoyZ", "JoyZ" },
                    std::pair{ "INPUT.JoyR", "JoyR" },
                    std::pair{ "INPUT.JoyU", "JoyU" },
                    std::pair{ "INPUT.AnalogUp", "AnalogUp" },
                    std::pair{ "INPUT.AnalogDown", "AnalogDown" },
                    std::pair{ "INPUT.AnalogLeft", "AnalogLeft" },
                    std::pair{ "INPUT.AnalogRight", "AnalogRight" }
                })
                    userIni["Engine.Input"].setAll(key, settings.Get(id));
                userIni["Engine.Input"].setAll("JoyZ", Gamepad::TurnBinding());
                userIni["Engine.Input"].setAll("JoyV", Gamepad::LookUpBinding());
                userIni["Engine.EPCGameOptions"]["Resolution"] = std::to_string(Screen.Width) + "x" + std::to_string(Screen.Height);
                userIniFile.generate(userIni);
            }

            auto gameIniPath = exePath / "SplinterCell2.ini";
            // Preserve the engine defaults and skip files without window settings.
            CreateInitialIni(exePath / "Default.ini", gameIniPath, "WinDrv.WindowsClient");
            mINI::INIStructure gameIni;
            if (mINI::INIFile(gameIniPath).read(gameIni) && gameIni.has("WinDrv.WindowsClient"))
            {
                CIniReader iniWriter(gameIniPath);
                iniWriter.WriteInteger("WinDrv.WindowsClient", "WindowedViewportX", Screen.Width);
                iniWriter.WriteInteger("WinDrv.WindowsClient", "WindowedViewportY", Screen.Height);
                iniWriter.WriteString("WinDrv.WindowsClient", "UseJoystick", "True");
                iniWriter.WriteString("Engine.GameInfo", "UseRumble", "True");
            }

            auto ESettingIniPath = exePath / "ESetting.ini";
            mINI::INIStructure ESettingIni;
            mINI::INIFile ESettingIniFile(ESettingIniPath);
            if (ESettingIniFile.read(ESettingIni))
            {
                for (auto const& section : ESettingIni)
                {
                    std::string sectionName = std::get<0>(section);
                    if (ESettingIni[sectionName].has("useAimTuning"))
                    {
                        ESettingIni[sectionName]["useAimTuning"] = "v=0";
                    }
                    //if (ESettingIni[sectionName].has("AimSpeedAdjust"))
                    //{
                    //    ESettingIni[sectionName]["AimSpeedAdjust"] = "v=1.0";
                    //}
                }
                ESettingIniFile.write(ESettingIni);
            }

            std::vector<std::string> list;
            GetResolutionsList(list);

            // Add current resolution to list
            auto str = std::format("{}x{}", Screen.Width, Screen.Height);
            if (std::find(list.begin(), list.end(), str) == list.end())
            {
                list.push_back(str);
            }

            NativeDisplayModes = { { 640, 480 }, { 800, 600 } };
            for (const auto& resolution : list)
            {
                int width = 0, height = 0;
                if (sscanf_s(resolution.c_str(), "%dx%d", &width, &height) == 2 &&
                    width >= 640 && height >= 480 && width <= 16384 && height <= 16384)
                    NativeDisplayModes.emplace_back(width, height);
            }
            std::sort(NativeDisplayModes.begin(), NativeDisplayModes.end());
            NativeDisplayModes.erase(std::unique(NativeDisplayModes.begin(), NativeDisplayModes.end()), NativeDisplayModes.end());

            // Sort list by resolution size (descending)
            std::sort(list.begin(), list.end(), [](const std::string& lhs, const std::string& rhs)
            {
                int32_t x1, y1, x2, y2;
                sscanf_s(lhs.c_str(), "%dx%d", &x1, &y1);
                sscanf_s(rhs.c_str(), "%dx%d", &x2, &y2);
                return (x1 * y1) > (x2 * y2);
            });

            // Group resolutions by string length
            std::map<size_t, std::vector<std::string>> lengthToRes;
            for (const auto& res : list)
            {
                lengthToRes[res.length()].push_back(res);
            }

            // Assign to ResList slots in reverse order (largest slots first)
            for (auto it = ResList.rbegin(); it != ResList.rend(); ++it)
            {
                size_t targetLength = it->first.length();
                auto& candidates = lengthToRes[targetLength];
                if (!candidates.empty())
                {
                    // Assign the largest available resolution for this length
                    it->second = std::wstring(candidates.front().begin(), candidates.front().end());
                    candidates.erase(candidates.begin());  // Remove to prevent reuse
                }
                else
                {
                    // No matching length available, use the template resolution
                    it->second = it->first;
                }
            }

            // Ensure current resolution is in the list
            auto strWide = std::wstring(str.begin(), str.end());
            bool currentResInList = std::any_of(ResList.begin(), ResList.end(), [&strWide](const auto& pair) { return pair.second == strWide; });

            if (!currentResInList && !ResList.empty())
            {
                auto it = std::find_if(ResList.begin(), ResList.end(), [&strWide](const auto& pair) { return pair.first == strWide; });
                if (it != ResList.end())
                {
                    it->second = strWide;
                }
                else
                {
                    // Find a slot with matching string length and replace it
                    for (auto& pair : ResList)
                    {
                        if (pair.second.length() == strWide.length() && pair.second != strWide)
                        {
                            pair.second = strWide;
                            break;
                        }
                    }
                }
            }

            // Keep 640x480 and 800x600
            ResList[0].second = L"640x480";
            ResList[1].second = L"800x600";

            #ifdef _DEBUG
            // Log the assignments
            for (size_t i = 0; i < ResList.size(); ++i)
            {
                spd::log()->info("{0} : {1}", std::string(ResList[i].first.begin(), ResList[i].first.end()), std::string(ResList[i].second.begin(), ResList[i].second.end()));
            }
            #endif
        };
    }
} Settings;
