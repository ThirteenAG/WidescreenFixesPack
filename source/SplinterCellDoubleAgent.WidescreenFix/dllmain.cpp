#include "stdafx.h"
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib") // needed for timeBeginPeriod()/timeEndPeriod()
#include <LEDEffects.h>

import ComVars;
import Window;
import Engine;
import D3DDrv;
import EchelonMenus;
import Echelon;
import Core;
import Online;

void AmmoInClip()
{
    static std::vector<LogiLed::KeyName> keys = {
        LogiLed::KeyName::F1, LogiLed::KeyName::F2, LogiLed::KeyName::F3,
        LogiLed::KeyName::F4, LogiLed::KeyName::F5, LogiLed::KeyName::F6,
        LogiLed::KeyName::F7, LogiLed::KeyName::F8, LogiLed::KeyName::F9,
        LogiLed::KeyName::F10, LogiLed::KeyName::F11, LogiLed::KeyName::F12,
        LogiLed::KeyName::PRINT_SCREEN, LogiLed::KeyName::SCROLL_LOCK,
        LogiLed::KeyName::PAUSE_BREAK,
    };

    static auto oldAmmoInClip = -1;
    if (curAmmoInClip != oldAmmoInClip)
    {
        auto maxAmmo = curClipCapacity;
        float ammoInClipPercent = ((float)curAmmoInClip / (float)maxAmmo) * 100.0f;

        for (size_t i = 0; i < keys.size(); i++)
        {
            auto indexInPercent = ((float)i / (float)keys.size()) * 100.0f;
            if (ammoInClipPercent > indexInPercent)
                LogiLedSetLightingForKeyWithKeyName(keys[i], 100, 100, 100);
            else
                LogiLedSetLightingForKeyWithKeyName(keys[i], 10, 10, 10);
        }
    }
    oldAmmoInClip = curAmmoInClip;
}

void Init()
{
    CIniReader iniReader("");
    Screen.Width = iniReader.ReadInteger("MAIN", "ResX", 0);
    Screen.Height = iniReader.ReadInteger("MAIN", "ResY", 0);
    bool bForceLL = iniReader.ReadInteger("MAIN", "ForceLL", 1) != 0;
    auto nFPSLimit = iniReader.ReadInteger("MISC", "FPSLimit", 1000);
    Screen.szLoadscPath = iniReader.GetIniPath();
    Screen.szLoadscPath.replace_extension(".png");
    gBlacklistIndicators = iniReader.ReadInteger("BONUS", "BlacklistIndicators", 0);
    gColor = iniReader.ReadInteger("BONUS", "GogglesLightColor", 0);
    bLightSyncRGB = iniReader.ReadInteger("BONUS", "LightSyncRGB", 1);

    if (!Screen.Width || !Screen.Height)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();

    auto exePath = GetExeModulePath();

    {
        mINI::INIStructure ini;
        mINI::INIFile mIni(iniReader.GetIniPath());
        mIni.read(ini);

        // Read the existing user INI file into a structure
        mINI::INIStructure sc4Ini;
        mINI::INIFile sc4IniFile(exePath / "SplinterCell4.ini");
        sc4IniFile.read(sc4Ini);

        for (auto const& sec : ini)
        {
            std::string sectionName = std::get<0>(sec);
            if (sectionName == "GENERAL" || sectionName == "Graphics" || sectionName == "WinDrv.WindowsClient" || sectionName == "D3DDrv.D3DRenderDevice")
            {
                for (auto const& kv : std::get<1>(sec))
                {
                    std::string key = std::get<0>(kv);
                    std::string value = std::get<1>(kv);
                    sc4Ini[sectionName].set(key, value);
                }
            }
        }

        auto ResX = std::to_string(Screen.Width);
        auto ResY = std::to_string(Screen.Height);

        sc4Ini["WinDrv.WindowsClient"].set("WindowedViewportX", ResX);
        sc4Ini["WinDrv.WindowsClient"].set("WindowedViewportY", ResY);
        sc4Ini["WinDrv.WindowsClient"].set("FullscreenViewportX", ResX);
        sc4Ini["WinDrv.WindowsClient"].set("FullscreenViewportY", ResY);

        sc4IniFile.generate(sc4Ini);
    }

    {
        mINI::INIStructure ini;
        mINI::INIFile mIni(iniReader.GetIniPath());
        mIni.read(ini);

        // Read the existing user INI file into a structure
        mINI::INIStructure userIni;
        mINI::INIFile userIniFile(exePath / "SplinterCell4User.ini");
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
        userIniFile.generate(userIni);
    }

    if (bForceLL)
    {
        auto pattern = hook::pattern("74 ? 68 ? ? ? ? 53 FF D7");
        injector::MakeNOP(pattern.get_first(), 2, true);
    }

    if (nFPSLimit)
    {
        static float fFPSLimit = 1.0f / static_cast<float>(nFPSLimit);
        auto pattern = hook::pattern("A1 ? ? ? ? 8B 0D ? ? ? ? 89 45 DC 89 4D C4");
        injector::WriteMemory(pattern.get_first(1), &fFPSLimit, true);
    }

    if (bLightSyncRGB)
    {
        LEDEffects::Inject([]()
        {
            static auto fPlayerVisibility = gVisibility;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            auto gVisCmp = static_cast<float>(static_cast<int>(gVisibility * 10.0f)) / 10.0f;
            auto fPlVisCmp = static_cast<float>(static_cast<int>(fPlayerVisibility * 10.0f)) / 10.0f;

            if (fPlVisCmp > gVisCmp)
                fPlayerVisibility -= 0.05f;
            else if (fPlVisCmp < gVisCmp)
                fPlayerVisibility += 0.05f;

            fPlayerVisibility = std::clamp(fPlayerVisibility, 0.0f, 1.0f);

            auto [R, G, B] = LEDEffects::RGBtoPercent(1, 255, 1, fPlayerVisibility);

            LEDEffects::SetLighting(R, G, B, false, false, true);
            AmmoInClip();
        });
    }

    //for test only (steam version)
    //CallbackHandler::RegisterCallback([]()
    //{
    //	auto pattern = hook::pattern("89 85 D8 61 00 00");
    //	struct StartupHook
    //	{
    //		void operator()(injector::reg_pack& regs)
    //		{
    //			*(uint32_t*)(regs.ebp + 0x61D8) = regs.eax;
    //			MessageBox(0, 0, L"test", 0);
    //		}
    //	}; injector::MakeInline<StartupHook>(pattern.get_first(0), pattern.get_first(6));
    //}, hook::pattern("89 85 D8 61 00 00").count_hint(1).empty(), 0x1100);
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("74 ? 68 ? ? ? ? 53 FF D7"));
        CallbackHandler::RegisterCallback(L"Window.dll", InitWindow);
        CallbackHandler::RegisterCallback(L"Engine.dll", InitEngine);
        CallbackHandler::RegisterCallback(L"D3DDrv.dll", InitD3DDrv);
        CallbackHandler::RegisterCallback(L"EchelonMenus.dll", InitEchelonMenus);
        CallbackHandler::RegisterCallback(L"Echelon.dll", InitEchelon);
        CallbackHandler::RegisterCallback(L"Core.dll", InitCore);

        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(InitOnline, hook::pattern("8B F9 8B 47 30 85 C0 74 18"));
    });
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        if (!IsUALPresent()) { InitializeASI(); }
    }
    else if (reason == DLL_PROCESS_DETACH)
    {
        if (nFrameLimitType == FrameLimiter::FPSLimitMode::FPS_ACCURATE)
            timeEndPeriod(1);
    }
    return TRUE;
}