#include <stdafx.h>
#include "FixSettings.hpp"
#include "../Shared/SplinterCell/Window.hpp"

import ComVars;
import CrashHandler;
import Core;
import Engine;
import D3DDrv;
import Echelon;
import WinDrv;
import Window;
import GUI;
import Xidi;
import Eax;

#include "NativeMenu.hpp"

void Init()
{
    SCWindow::EnableDpi();
    InitCrashHandler();

    auto& settings = FixSettings();
    asi_settings::LiveLink::Start();
    Screen.Width = settings.Int("MAIN.ResX");
    Screen.Height = settings.Int("MAIN.ResY");
    Screen.bDeferredInput = settings.Int("MAIN.DeferredInput") != 0;
    Screen.bRawInputMouse = settings.Int("MAIN.RawInputMouse") != 0;
    settings.OnChange("MAIN.MouseSensitivity", []
    {
        RawInputHandler<int32_t>::SetSensitivity(FixSettings().Float("MAIN.MouseSensitivity"));
    });
    Screen.bRawInputMouseRawData = settings.Int("MAIN.RawInputMouseRawData") != 0;
    Screen.nFMVWidescreenMode = settings.Int("MAIN.FMVWidescreenMode");
    Screen.nHudWidescreenMode = settings.Int("MAIN.HudWidescreenMode");
    Screen.bOpsatWidescreenMode = settings.Int("MAIN.OpsatWidescreenMode") != 0;
    Screen.bScopeWidescreenMode = settings.Int("MAIN.ScopeWidescreenMode") != 0;
    Screen.fHudAspectRatioConstraint = ParseWidescreenHudOffset(settings.Get("MAIN.HudAspectRatioConstraint"));
    Screen.nPostProcessFixedScale = settings.Int("MAIN.PostProcessFixedScale");
    Screen.fGrainScale = std::clamp(settings.Float("MAIN.GrainScale"), 0.0f, 1.0f);
    Screen.nShadowMapResolution = settings.Int("MAIN.ShadowMapResolution");
    Screen.nReflectionsResolution = settings.Int("MAIN.ReflectionsResolution");
    Screen.nBloomResolutionMultiplier = std::clamp(settings.Int("MAIN.BloomResolutionMultiplier"), 0, 4);
    fLightDistanceMultiplier = std::clamp(settings.Float("MAIN.LightDistanceMultiplier"), 1.0f, 8.0f);
    auto nFPSLimit = settings.Int("MAIN.FPSLimit");
    gColor.RGBA = settings.Int("BONUS.GogglesLightColor");
    bSkipIntro = settings.Int("MAIN.SkipIntro") != 0;
    bSkipPressStartToContinue = settings.Int("MAIN.SkipPressStart") != 0;
    bRestoreCutsceneFOV = settings.Int("MAIN.RestoreCutsceneFOV") != 0;
    Screen.nCutsceneBorders = settings.Int("MAIN.CutsceneBorders");
    bEnableConsole = settings.Int("MAIN.EnableConsole") != 0;
    bEnableCheckpoints = settings.Int("MAIN.EnableCheckpoints") != 0;

    if (Screen.Width <= 0 || Screen.Height <= 0)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();
    if (Screen.Width <= 0 || Screen.Height <= 0)
        std::tie(Screen.Width, Screen.Height) = std::make_tuple(640, 480);

    auto exePath = GetExeModulePath();

    auto userIniPath = exePath / "SplinterCell2User.ini";

    // Only update the user INI when it exists and could be read; the game creates it on first run,
    // and a file written from scratch here would be missing everything else (bindings etc.)
    mINI::INIStructure userIni;
    mINI::INIFile userIniFile(userIniPath);
    if (userIniFile.read(userIni))
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
        userIni["Engine.EPCGameOptions"]["Resolution"] = std::to_string(Screen.Width) + "x" + std::to_string(Screen.Height);
        userIniFile.generate(userIni);
    }

    auto gameIniPath = exePath / "SplinterCell2.ini";
    // The engine creates a missing game INI by copying Default.ini, but only when the file doesn't exist,
    // so do the same here instead of letting the writes below create a file with only a few keys in it
    std::error_code ec;
    if (!std::filesystem::exists(gameIniPath, ec) && !ec)
        CopyFileW((exePath / "Default.ini").c_str(), gameIniPath.c_str(), TRUE);

    if (std::filesystem::exists(gameIniPath, ec))
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

    if (nFPSLimit)
    {
        static float fFPSLimit = 1.0f / static_cast<float>(nFPSLimit);
        auto pattern = hook::pattern("F3 0F 10 05 ? ? ? ? F3 0F 11 45 F0 F3 0F 10 05 ? ? ? ? F3 0F 11 45 EC"); // Digital version
        if (!pattern.empty())
            ProtectedGameRef<decltype(&fFPSLimit)>(reinterpret_cast<decltype(&fFPSLimit)*>(pattern.get_first(4))) = &fFPSLimit;
        else
        {
            pattern = hook::pattern("8B 0D ? ? ? ? 89 4D C8 8B 15 ? ? ? ? 89 55 E0"); // Retail version
            if (!pattern.empty())
                ProtectedGameRef<decltype(&fFPSLimit)>(reinterpret_cast<decltype(&fFPSLimit)*>(pattern.get_first(2))) = &fFPSLimit;
        }
    }
}

ASI_SETTINGS_EXPORT(FixSettings())

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallback(Init);
        CallbackHandler::RegisterCallback(L"Core.dll", [] { InitCore(); PTMenu::Init(); });
        CallbackHandler::RegisterCallback(L"Engine.dll", InitEngine);
        CallbackHandler::RegisterCallback(L"D3DDrv.dll", InitD3DDrv);
        CallbackHandler::RegisterCallback(L"Echelon.dll", InitEchelon);
        CallbackHandler::RegisterCallback(L"WinDrv.dll", InitWinDrv);
        CallbackHandler::RegisterCallback(L"GUI.dll", [] { InitGUI(); PTMenu::InitDrawing(); });
        CallbackHandler::RegisterCallback(L"Window.dll", InitWindow);
        CallbackHandler::RegisterCallback(L"Xidi.32.dll", InitXidi);
        CallbackHandler::RegisterCallback(L"eax.dll", InitEAX);
    });
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        if (!IsUALPresent()) { InitializeASI(); }
    }
    return TRUE;
}
