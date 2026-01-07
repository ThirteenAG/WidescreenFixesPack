#include <stdafx.h>

import ComVars;
import Core;
import Engine;
import D3DDrv;
import WinDrv;
import Window;
import Echelon;
import EchelonHUD;
import Xidi;

void Init()
{
    CIniReader iniReader("");
    Screen.Width = iniReader.ReadInteger("MAIN", "ResX", 0);
    Screen.Height = iniReader.ReadInteger("MAIN", "ResY", 0);
    Screen.bDeferredInput = iniReader.ReadInteger("MAIN", "DeferredInput", 1) != 0;
    Screen.fRawInputMouse = std::clamp(iniReader.ReadFloat("MAIN", "RawInputMouse", 1.0f), 0.0f, 5.0f);
    Screen.bRawInputMouseRawData = iniReader.ReadInteger("MAIN", "RawInputMouseRawData", 0) != 0;
    Screen.nFMVWidescreenMode = iniReader.ReadInteger("MAIN", "FMVWidescreenMode", 1);
    Screen.nHudWidescreenMode = iniReader.ReadInteger("MAIN", "HudWidescreenMode", 2);
    Screen.fHudAspectRatioConstraint = ParseWidescreenHudOffset(iniReader.ReadString("MAIN", "HudAspectRatioConstraint", ""));
    bSkipIntro = iniReader.ReadInteger("MAIN", "SkipIntro", 0) != 0;
    bSkipPressStartToContinue = iniReader.ReadInteger("MAIN", "SkipPressStartToContinue", 0) != 0;
    bRestoreCutsceneFOV = iniReader.ReadInteger("MAIN", "RestoreCutsceneFOV", 0) != 0;

    if (!Screen.Width || !Screen.Height)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();

    if (IsEnhanced())
    {
        bSkipIntro = false;
    }

    auto exePath = GetExeModulePath();

    mINI::INIStructure ini;
    mINI::INIFile mIni(iniReader.GetIniPath());
    mIni.read(ini);

    auto userIniPath = exePath / "SplinterCellUser.ini";

    // Read the existing user INI file into a structure
    mINI::INIStructure userIni;
    mINI::INIFile userIniFile(userIniPath);
    userIniFile.read(userIni);

    if (ini.has("Engine.Input"))
    {
        for (auto const& kv : ini["Engine.Input"])
        {
            std::string key = std::get<0>(kv);
            std::string value = std::get<1>(kv);
            userIni["Engine.Input"].setAll(key, value);
        }
    }
    userIni["Engine.EPCGameOptions"]["Resolution"] = std::to_string(Screen.Width) + "x" + std::to_string(Screen.Height);
    userIniFile.generate(userIni);

    auto gameIniPath = exePath / "SplinterCell.ini";
    CIniReader iniWriter(gameIniPath);
    iniWriter.WriteInteger("WinDrv.WindowsClient", "WindowedViewportX", Screen.Width);
    iniWriter.WriteInteger("WinDrv.WindowsClient", "WindowedViewportY", Screen.Height);
    iniWriter.WriteString("WinDrv.WindowsClient", "UseJoystick", "True");
    iniWriter.WriteString("D3DDrv.D3DRenderDevice", "FullScreenVideo", "True");
    iniWriter.WriteInteger("D3DDrv.D3DRenderDevice", "ForceShadowMode", 1);
    iniWriter.WriteInteger("D3DDrv.D3DRenderDevice", "EmulateGF2Mode", 0);
    iniWriter.WriteString("Engine.GameInfo", "UseRumble", "True");

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

    // For SC1, keep 640x480 and 800x600
    ResList[0].second = L"640x480";
    ResList[1].second = L"800x600";

    #ifdef _DEBUG
    // Log the assignments
    for (size_t i = 0; i < ResList.size(); ++i)
    {
        spd::log()->info("{0} : {1}", std::string(ResList[i].first.begin(), ResList[i].first.end()), std::string(ResList[i].second.begin(), ResList[i].second.end()));
    }
    #endif
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallback(Init);
        CallbackHandler::RegisterCallback(L"Engine.dll", InitEngine);
        CallbackHandler::RegisterCallback(L"D3DDrv.dll", InitD3DDrv);
        CallbackHandler::RegisterCallback(L"WinDrv.dll", InitWinDrv);
        CallbackHandler::RegisterCallback(L"Window.dll", InitWindow);
        CallbackHandler::RegisterCallback(L"Core.dll", InitCore);
        CallbackHandler::RegisterCallback(L"Echelon.dll", InitEchelon);
        CallbackHandler::RegisterCallback(L"EchelonHUD.dll", InitEchelonHUD);
        CallbackHandler::RegisterCallback(L"Xidi.32.dll", InitXidi);
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