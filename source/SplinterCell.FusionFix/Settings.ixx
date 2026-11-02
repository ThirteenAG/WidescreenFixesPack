module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module Settings;

import ComVars;
import Gamepad;

namespace
{
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
            Screen.Width = FixSettings().Int("MAIN.ResX");
            Screen.Height = FixSettings().Int("MAIN.ResY");
            Screen.nShadowBufferResolution = FixSettings().Int("MAIN.ShadowBufferResolution");
            Screen.bRawInputMouse = FixSettings().Bool("MAIN.RawInputMouse");
            FixSettings().OnChange("MAIN.MouseSensitivity", []
            {
                RawInputHandler<int32_t>::SetSensitivity(FixSettings().Float("MAIN.MouseSensitivity"));
            });
            Screen.bRawInputMouseRawData = FixSettings().Bool("MAIN.RawInputMouseRawData");
            Screen.fHudAspectRatioConstraint = ParseWidescreenHudOffset(FixSettings().String("MAIN.HudAspectRatioConstraint"));
            Screen.fGrainScale = std::clamp(FixSettings().Float("MAIN.GrainScale"), 0.0f, 1.0f);
            bSkipIntro = FixSettings().Bool("MAIN.SkipIntro");
            bSkipPressStartToContinue = FixSettings().Bool("MAIN.SkipPressStart");
            
            if (Screen.Width <= 0 || Screen.Height <= 0)
                std::tie(Screen.Width, Screen.Height) = GetDesktopRes();
            if (Screen.Width <= 0 || Screen.Height <= 0)
                std::tie(Screen.Width, Screen.Height) = std::make_tuple(640, 480);

            if (IsEnhanced())
            {
                bSkipIntro = false;
            }

            auto exePath = GetExeModulePath();

            auto userIniPath = exePath / "SplinterCellUser.ini";

            // Start with the engine's defaults so the pad bindings also apply on the first launch.
            CreateInitialIni(exePath / "DefUser.ini", userIniPath, "Engine.Input");
            mINI::INIStructure userIni;
            mINI::INIFile userIniFile(userIniPath);
            if (userIniFile.read(userIni) && userIni.has("Engine.Input"))
            {
                // Only native input commands are copied; the fix's legacy INI is never read.
                for (auto key : { "Joy1", "Joy2", "Joy3", "Joy4", "Joy5", "Joy6", "Joy7", "Joy8", "Joy9", "Joy10", "Joy11", "Joy12", "Joy13", "Joy14", "Joy15", "Joy16", "JoyX", "JoyY" })
                    userIni["Engine.Input"].setAll(key, FixSettings().String(std::string("INPUT.") + key));
                userIni["Engine.Input"].setAll("JoyZ", Gamepad::TurnBinding());
                userIni["Engine.Input"].setAll("JoyV", Gamepad::LookUpBinding());
                userIni["Engine.EPCGameOptions"]["Resolution"] = std::to_string(Screen.Width) + "x" + std::to_string(Screen.Height);
                userIniFile.generate(userIni);
            }

            auto gameIniPath = exePath / "SplinterCell.ini";
            // Preserve the engine defaults and skip files without window settings.
            CreateInitialIni(exePath / "Default.ini", gameIniPath, "WinDrv.WindowsClient");
            mINI::INIStructure gameIni;
            if (mINI::INIFile(gameIniPath).read(gameIni) && gameIni.has("WinDrv.WindowsClient"))
            {
                CIniReader iniWriter(gameIniPath);
                iniWriter.WriteInteger("WinDrv.WindowsClient", "WindowedViewportX", Screen.Width);
                iniWriter.WriteInteger("WinDrv.WindowsClient", "WindowedViewportY", Screen.Height);
                iniWriter.WriteString("WinDrv.WindowsClient", "UseJoystick", "True");
                iniWriter.WriteString("D3DDrv.D3DRenderDevice", "FullScreenVideo", "True");

                auto shadowMode = FixSettings().Bool("MAIN.ForceShadowBufferMode");
                iniWriter.WriteInteger("D3DDrv.D3DRenderDevice", "ForceShadowMode", shadowMode);
                if (shadowMode == 1) // GF2 mode must be off to render shadow buffers
                    iniWriter.WriteInteger("D3DDrv.D3DRenderDevice", "EmulateGF2Mode", 0);

                iniWriter.WriteString("Engine.GameInfo", "UseRumble", "True");
            }

            std::vector<std::string> list;
            GetResolutionsList(list);

            // Add current resolution to list
            auto str = std::format("{}x{}", Screen.Width, Screen.Height);
            if (std::find(list.begin(), list.end(), str) == list.end())
            {
                list.push_back(str);
            }

            // Sort list by resolution size (descending)
            std::sort(list.begin(), list.end(), [](const std::string& lhs, const std::string& rhs)
            {
                int32_t x1, y1, x2, y2;
                sscanf_s(lhs.c_str(), "%dx%d", &x1, &y1);
                sscanf_s(rhs.c_str(), "%dx%d", &x2, &y2);
                return (x1 * y1) > (x2 * y2);
            });

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
            bool currentResInList = std::any_of(ResList.begin(), ResList.end(), [&strWide](const auto& pair)
            {
                return pair.second == strWide;
            });

            if (!currentResInList && !ResList.empty())
            {
                auto it = std::find_if(ResList.begin(), ResList.end(), [&strWide](const auto& pair)
                {
                    return pair.first == strWide;
                });
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
