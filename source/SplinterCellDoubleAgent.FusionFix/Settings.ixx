module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module Settings;

import ComVars;

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
            Screen.bRawInputMouse = FixSettings().Bool("MAIN.RawInputMouse");
            FixSettings().OnChange("MAIN.MouseSensitivity", []
            {
                RawInputHandler<>::SetSensitivity(FixSettings().Float("MAIN.MouseSensitivity") * 0.5f);
            });
            Screen.bRawInputMouseRawData = FixSettings().Int("MAIN.RawInputMouseRawData") != 0;
            Screen.fHudAspectRatioConstraint = ParseWidescreenHudOffset(FixSettings().String("MAIN.HudAspectRatioConstraint"));
            Screen.LoadscreenPicture = std::filesystem::path(FixSettings().CfgPath()).replace_extension(L".png");
            gBlacklistIndicators = FixSettings().Int("BONUS.BlacklistIndicators");
            gColor = FixSettings().Int("BONUS.GogglesLightColor");
            bLightSyncRGB = FixSettings().Int("MAIN.LightSyncRGB");

            if (Screen.Width <= 0 || Screen.Height <= 0 || Screen.Width > 16384 || Screen.Height > 16384)
                std::tie(Screen.Width, Screen.Height) = GetDesktopRes();
            if (Screen.Width <= 0 || Screen.Height <= 0)
                std::tie(Screen.Width, Screen.Height) = std::make_tuple(640, 480);

            auto exePath = GetExeModulePath();

            {
                // Preserve the engine defaults and skip files without window settings.
                auto sc4IniPath = exePath / "SplinterCell4.ini";
                CreateInitialIni(exePath / "Default.ini", sc4IniPath, "WinDrv.WindowsClient");

                mINI::INIStructure sc4Ini;
                mINI::INIFile sc4IniFile(sc4IniPath);
                if (sc4IniFile.read(sc4Ini) && sc4Ini.has("WinDrv.WindowsClient"))
                {
                    if (FixSettings().Bool("NATIVE.Enabled"))
                    {
                        if (auto value = FixSettings().String("NATIVE_WINDOW.StartupFullscreen"); !value.empty())
                            sc4Ini["WinDrv.WindowsClient"].set("StartupFullscreen", value);
                        if (auto value = FixSettings().String("NATIVE_WINDOW.TextureDetail"); !value.empty())
                            sc4Ini["WinDrv.WindowsClient"].set("TextureDetail", value);
                        if (auto value = FixSettings().String("NATIVE_GENERAL.DisableHDRTonemapping"); !value.empty())
                            sc4Ini["GENERAL"].set("DisableHDRTonemapping", value);
                        if (auto value = FixSettings().String("NATIVE_GENERAL.LowResTex"); !value.empty())
                            sc4Ini["GENERAL"].set("LowResTex", value);
                        if (auto value = FixSettings().String("NATIVE_GENERAL.SimpleShader"); !value.empty())
                            sc4Ini["GENERAL"].set("SimpleShader", value);
                        if (auto value = FixSettings().String("NATIVE_GENERAL.SimpleEffect"); !value.empty())
                            sc4Ini["GENERAL"].set("SimpleEffect", value);
                        if (auto value = FixSettings().String("NATIVE_GENERAL.HidePieces"); !value.empty())
                            sc4Ini["GENERAL"].set("HidePieces", value);
                        if (auto value = FixSettings().String("NATIVE_GRAPHICS.bEnableHDR"); !value.empty())
                            sc4Ini["Graphics"].set("bEnableHDR", value);
                        if (auto value = FixSettings().String("NATIVE_GRAPHICS.bSpecularLighting"); !value.empty())
                            sc4Ini["Graphics"].set("bSpecularLighting", value);
                        if (auto value = FixSettings().String("NATIVE_GRAPHICS.bHighQualityVisualFX"); !value.empty())
                            sc4Ini["Graphics"].set("bHighQualityVisualFX", value);
                        if (auto value = FixSettings().String("NATIVE_D3D.UseHWShadowMapping"); !value.empty())
                            sc4Ini["D3DDrv.D3DRenderDevice"].set("UseHWShadowMapping", value);
                        if (auto value = FixSettings().String("NATIVE_D3D.ShadowResolutionDegree"); !value.empty())
                            sc4Ini["D3DDrv.D3DRenderDevice"].set("ShadowResolutionDegree", value);
                        if (auto value = FixSettings().String("NATIVE_D3D.ShadowTurnOffDegree"); !value.empty())
                            sc4Ini["D3DDrv.D3DRenderDevice"].set("ShadowTurnOffDegree", value);
                        if (auto value = FixSettings().String("NATIVE_D3D.bUseSmoothShadow"); !value.empty())
                            sc4Ini["D3DDrv.D3DRenderDevice"].set("bUseSmoothShadow", value);
                        if (auto value = FixSettings().String("NATIVE_D3D.FSAADegree"); !value.empty())
                            sc4Ini["D3DDrv.D3DRenderDevice"].set("FSAADegree", value);
                    }

                    sc4Ini["WinDrv.WindowsClient"].set("StartupFullscreen", FixSettings().Int("MAIN.WindowMode") == 0 ? "True" : "False");

                    auto ResX = std::to_string(Screen.Width);
                    auto ResY = std::to_string(Screen.Height);

                    sc4Ini["WinDrv.WindowsClient"].set("WindowedViewportX", ResX);
                    sc4Ini["WinDrv.WindowsClient"].set("WindowedViewportY", ResY);
                    sc4Ini["WinDrv.WindowsClient"].set("FullscreenViewportX", ResX);
                    sc4Ini["WinDrv.WindowsClient"].set("FullscreenViewportY", ResY);

                    sc4IniFile.generate(sc4Ini);
                }
            }

            {
                // Only update the user INI when it exists and could be read; the game creates it on first run,
                // and a file written from scratch here would be missing everything else (bindings etc.)
                mINI::INIStructure userIni;
                mINI::INIFile userIniFile(exePath / "SplinterCell4User.ini");
                if (userIniFile.read(userIni) && userIni.has("Engine.Input"))
                {
                    for (auto key : { "Joy1", "Joy2", "Joy3", "Joy4", "Joy5", "Joy6", "Joy7", "Joy8", "Joy9", "Joy10", "Joy11", "Joy12", "Joy13", "Joy14", "Joy15", "Joy16", "JoyX", "JoyY", "JoyZ", "JoyV", "JoyR", "JoyU", "AnalogUp", "AnalogDown", "AnalogLeft", "AnalogRight", "MouseWheelUp", "MouseWheelDown", "Tilde" })
                        userIni["Engine.Input"].setAll(key, FixSettings().String(std::string("INPUT.") + key));
                    userIniFile.generate(userIni);
                }
            }
        };
    }
} Settings;
