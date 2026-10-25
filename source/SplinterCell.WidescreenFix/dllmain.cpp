#include <stdafx.h>
#include "FixSettings.hpp"
#include "../Shared/SplinterCell/Window.hpp"

ASI_SETTINGS_EXPORT(FixSettings())

import ComVars;
import CrashHandler;
import Core;
import Engine;
import D3DDrv;
import WinDrv;
import Window;
import Echelon;
import EchelonHUD;
import EchelonMenus;
import Xidi;
import Eax;

#include "NativeMenu.hpp"

void Init()
{
    SCWindow::EnableDpi();
    InitCrashHandler();

    SettingsReader iniReader;
    Screen.Width = iniReader.ReadInteger("MAIN", "ResX", 0);
    Screen.Height = iniReader.ReadInteger("MAIN", "ResY", 0);
    Screen.nShadowBufferResolution = iniReader.ReadInteger("MAIN", "ShadowBufferResolution", 0);
    Screen.bDeferredInput = iniReader.ReadBoolean("MAIN", "DeferredInput", true);
    Screen.bRawInputMouse = iniReader.ReadBoolean("MAIN", "RawInputMouse", true);
    FixSettings().OnChange("MAIN.MouseSensitivity", []
    {
        RawInputHandler<int32_t>::SetSensitivity(FixSettings().Float("MAIN.MouseSensitivity"));
    });
    Screen.bRawInputMouseRawData = iniReader.ReadBoolean("MAIN", "RawInputMouseRawData", false);
    Screen.nFMVWidescreenMode = iniReader.ReadInteger("MAIN", "FMVWidescreenMode", 1);
    Screen.nHudWidescreenMode = iniReader.ReadInteger("MAIN", "HudWidescreenMode", 2);
    Screen.fHudAspectRatioConstraint = ParseWidescreenHudOffset(iniReader.ReadString("MAIN", "HudAspectRatioConstraint", ""));
    Screen.fGrainScale = std::clamp(iniReader.ReadFloat("MAIN", "GrainScale", 1.0f), 0.0f, 1.0f);
    auto nFPSLimit = iniReader.ReadInteger("MAIN", "FPSLimit", 1000);
    bSkipIntro = iniReader.ReadBoolean("MAIN", "SkipIntro", false);
    bSkipPressStartToContinue = iniReader.ReadBoolean("MAIN", "SkipPressStart", false);
    bRestoreCutsceneFOV = iniReader.ReadBoolean("MAIN", "RestoreCutsceneFOV", false);
    Screen.nCutsceneBorders = iniReader.ReadInteger("MAIN", "CutsceneBorders", 0);
    
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

    // Only update the user INI when it exists and could be read; the game creates it on first run,
    // and a file written from scratch here would be missing everything else (bindings etc.)
    mINI::INIStructure userIni;
    mINI::INIFile userIniFile(userIniPath);
    if (userIniFile.read(userIni))
    {
        // Only native input commands are copied; the fix's legacy INI is never read.
        for (auto key : { "Joy1", "Joy2", "Joy3", "Joy4", "Joy5", "Joy6", "Joy7", "Joy8", "Joy9", "Joy10", "Joy11", "Joy12", "Joy13", "Joy14", "Joy15", "Joy16", "JoyX", "JoyY", "JoyZ", "JoyV" })
            userIni["Engine.Input"].setAll(key, FixSettings().String(std::string("INPUT.") + key));
        userIni["Engine.EPCGameOptions"]["Resolution"] = std::to_string(Screen.Width) + "x" + std::to_string(Screen.Height);
        userIniFile.generate(userIni);
    }

    auto gameIniPath = exePath / "SplinterCell.ini";
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
        iniWriter.WriteString("D3DDrv.D3DRenderDevice", "FullScreenVideo", "True");

        auto shadowMode = iniReader.ReadBoolean("MAIN", "ForceShadowBufferMode", true);
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

    if (nFPSLimit)
    {
        static float fFPSLimit = 1.0f / static_cast<float>(nFPSLimit);
        auto pattern = hook::pattern("8B 15 ? ? ? ? 89 55 D4 A1 ? ? ? ? 89 45 F8");
        if (!pattern.empty())
            ProtectedGameRef<float*>(pattern.get_first<float*>(2)) = &fFPSLimit;
    }
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        FixSettings();
        asi_settings::LiveLink::Start();
        CallbackHandler::RegisterCallback(Init);
        CallbackHandler::RegisterCallback(L"Engine.dll", InitEngine);
        CallbackHandler::RegisterCallback(L"D3DDrv.dll", InitD3DDrv);
        CallbackHandler::RegisterCallback(L"WinDrv.dll", InitWinDrv);
        CallbackHandler::RegisterCallback(L"Window.dll", InitWindow);
        CallbackHandler::RegisterCallback(L"Core.dll", [] { InitCore(); SCMenu::Init(); });
        CallbackHandler::RegisterCallback(L"Echelon.dll", InitEchelon);
        CallbackHandler::RegisterCallback(L"EchelonHUD.dll", InitEchelonHUD);
        CallbackHandler::RegisterCallback(L"EchelonMenus.dll", InitEchelonMenus);
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