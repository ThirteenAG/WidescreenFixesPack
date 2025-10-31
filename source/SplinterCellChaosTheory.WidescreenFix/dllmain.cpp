#include "stdafx.h"

import ComVars;
import D3DDrv;
import Engine;
import Core;
import WidescreenHUD;

const wchar_t* aJoysticks = nullptr;
SafetyHookInline shsub_10CC8580;
void* __fastcall sub_10CC8580(void* _this, void* edx)
{
    auto ret = shsub_10CC8580.unsafe_fastcall<void*>(_this, edx);

    if (aJoysticks)
    {
        std::filesystem::path aJoysticksDefaultPath = GetExeModulePath() / "JoysticksDefault.ini";
        std::filesystem::path aJoysticksPath = std::wstring(aJoysticks) + L".ini";
        for (auto it : { aJoysticksDefaultPath, aJoysticksPath })
        {
            CIniReader iniReader("");
            mINI::INIStructure ini;
            mINI::INIFile mIni(iniReader.GetIniPath());
            mIni.read(ini);

            // Read the existing user INI file into a structure
            mINI::INIStructure joysticksIni;
            mINI::INIFile joysticksIniFile(it);
            joysticksIniFile.read(joysticksIni);

            for (auto const& sec : ini)
            {
                std::string sectionName = std::get<0>(sec);
                if (sectionName.starts_with("Joystick"))
                {
                    for (auto const& kv : std::get<1>(sec))
                    {
                        std::string key = std::get<0>(kv);
                        std::string value = std::get<1>(kv);
                        joysticksIni[sectionName].set(key, value);
                    }
                }
            }

            joysticksIniFile.generate(joysticksIni);
        }
    }

    return ret;
}

void Init()
{
    CIniReader iniReader("");
    Screen.Width = iniReader.ReadInteger("MAIN", "ResX", 0);
    Screen.Height = iniReader.ReadInteger("MAIN", "ResY", 0);
    bHudWidescreenMode = iniReader.ReadInteger("MAIN", "HudWidescreenMode", 1) != 0;
    nWidescreenHudOffset = iniReader.ReadInteger("MAIN", "WidescreenHudOffset", 100);
    fWidescreenHudOffset = static_cast<float>(nWidescreenHudOffset);
    bDisableAltTabFix = iniReader.ReadInteger("MAIN", "DisableAltTabFix", 1) != 0;
    eGameLang = static_cast<GameLang>(iniReader.ReadInteger("MAIN", "GameLanguage", 0));
    nShadowMapResolution = iniReader.ReadInteger("GRAPHICS", "ShadowMapResolution", 0);
    bEnableShadowFiltering = iniReader.ReadInteger("GRAPHICS", "EnableShadowFiltering", 0) != 0;
    auto nFPSLimit = iniReader.ReadInteger("MISC", "FPSLimit", 1000);
    gColor = iniReader.ReadInteger("BONUS", "GogglesLightColor", 0);
    gBlacklistIndicators = iniReader.ReadInteger("BONUS", "BlacklistIndicators", 0);
    bLightSyncRGB = iniReader.ReadInteger("BONUS", "LightSyncRGB", 1);

    if (!Screen.Width || !Screen.Height)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();

    Screen.fWidth = static_cast<float>(Screen.Width);
    Screen.fHeight = static_cast<float>(Screen.Height);
    Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);

    auto pattern = hook::pattern("68 ? ? ? ? E8 ? ? ? ? 8B 8C 24 20 02 00 00");
    aJoysticks = *pattern.get_first<const wchar_t*>(1);

    pattern = hook::pattern("55 8B EC 83 E4 F8 81 EC 08 02 00 00");
    shsub_10CC8580 = safetyhook::create_inline(pattern.get_first(), sub_10CC8580);

    pattern = hook::pattern("8D 84 24 34 04 00 00 68 ? ? ? ? 50 E8 ? ? ? ? 83 C4 14"); //0x10CD09C5
    struct SetResHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.eax = (regs.esp + 0x434); // lea eax, [esp+434h]

            if (Screen.Width < 640 || Screen.Height < 480)
                return;

            auto pszPath = (const wchar_t*)regs.edi;
            auto ResX = std::to_wstring(Screen.Width);
            auto ResY = std::to_wstring(Screen.Height);
            WritePrivateProfileStringW(L"WinDrv.WindowsClient", L"WindowedViewportX", ResX.c_str(), pszPath);
            WritePrivateProfileStringW(L"WinDrv.WindowsClient", L"WindowedViewportY", ResY.c_str(), pszPath);
            WritePrivateProfileStringW(L"WinDrv.WindowsClient", L"FullscreenViewportX", ResX.c_str(), pszPath);
            WritePrivateProfileStringW(L"WinDrv.WindowsClient", L"FullscreenViewportY", ResY.c_str(), pszPath);
        }
    }; injector::MakeInline<SetResHook>(pattern.get_first(0), pattern.get_first(7));

    InitD3DDrv();
    InitEngine();
    InitCore();

    if (nFPSLimit)
    {
        static float fFPSLimit = 1.0f / static_cast<float>(nFPSLimit);
        auto pattern = hook::pattern("8B 15 ? ? ? ? A1 ? ? ? ? 89 54 24 10 89 44 24 18");
        injector::WriteMemory(pattern.get_first(2), &fFPSLimit, true);
    }
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("8D 84 24 34 04 00 00 68 ? ? ? ? 50 E8 ? ? ? ? 83 C4 14"));
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
