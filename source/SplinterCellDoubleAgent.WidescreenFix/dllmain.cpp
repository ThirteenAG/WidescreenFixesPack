#include "stdafx.h"
#include <LEDEffects.h>

import ComVars;
import Window;
import Engine;
import D3DDrv;
import EchelonMenus;
import Echelon;
import Core;
import WinDrv;
import Xidi;
import Online;
import Eax;

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
    Screen.fRawInputMouse = std::clamp(iniReader.ReadFloat("MAIN", "RawInputMouse", 1.0f), 0.0f, 5.0f) / 2.0f;
    Screen.bRawInputMouseRawData = iniReader.ReadInteger("MAIN", "RawInputMouseRawData", 0) != 0;
    Screen.nHudWidescreenMode = iniReader.ReadInteger("MAIN", "HudWidescreenMode", 1);
    Screen.fHudAspectRatioConstraint = ParseWidescreenHudOffset(iniReader.ReadString("MAIN", "HudAspectRatioConstraint", ""));
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
        auto pattern = hook::pattern("A1 ? ? ? ? 8B 0D ? ? ? ? 89 45");
        injector::MakeNOP(pattern.get_first(), 5, true);
        static auto DeltaTimeHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            auto ECoopPlayerControllerWithLabel = UObject::GetState(L"ECoopPlayerControllerWithLabel");
            constexpr auto s_CrackSafeTurnLeft = L"s_CrackSafe224";
            constexpr auto s_CrackSafeTurnRight = L"s_CrackSafe248";

            if (ECoopPlayerControllerWithLabel == s_CrackSafeTurnLeft || ECoopPlayerControllerWithLabel == s_CrackSafeTurnRight)
            {
                *(float*)&regs.eax = 1.0f / 30.0f;
                return;
            }

            if (bLoadingScreenActive && *bLoadingScreenActive)
            {
                *(float*)&regs.eax = 1.0f / 9999.0f;
                return;
            }

            *(float*)&regs.eax = fFPSLimit;
        });
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

// Ensure Gear registers the drive containing the game when its 10-prefix limit is reached.
void InitGear()
{
    static std::string gameDrive = GetExeModulePath().root_path().string();

    if (gameDrive.size() != 3 || gameDrive[1] != ':' ||
        (gameDrive[2] != '\\' && gameDrive[2] != '/'))
    {
        return;
    }

    gameDrive[2] = '\\';

    auto pattern = hook::module_pattern(
        GetModuleHandle(L"gear_RD"),
        "8D 9E A4 02 00 00 89 5D FC C7 45 F8 34 04 00 00"
    );

    static auto GearDrivePrefixHook =
        safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        auto* device = reinterpret_cast<uint8_t*>(regs.esi);
        auto* prefixCount = reinterpret_cast<uint32_t*>(device + 0x14);
        auto* prefixes = reinterpret_cast<const char**>(device + 0x18);

        if (*prefixCount == 0 || *prefixCount > 10)
        {
            return;
        }

        for (uint32_t i = 0; i < *prefixCount; ++i)
        {
            if (prefixes[i] != nullptr &&
                _stricmp(prefixes[i], gameDrive.c_str()) == 0)
            {
                return;
            }
        }

        if (*prefixCount < 10)
        {
            prefixes[*prefixCount] = gameDrive.c_str();
            ++(*prefixCount);
        }
        else
        {
            prefixes[9] = gameDrive.c_str();
        }
    });
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("74 ? 68 ? ? ? ? 53 FF D7"));
        CallbackHandler::RegisterCallback(L"Window.dll", InitWindow);
        CallbackHandler::RegisterCallback(L"Engine.dll", InitEngine);
        CallbackHandler::RegisterCallback(L"D3DDrv.dll", InitD3DDrv);
        CallbackHandler::RegisterCallback(L"gear_RD.dll", InitGear);
        CallbackHandler::RegisterCallback(L"EchelonMenus.dll", InitEchelonMenus);
        CallbackHandler::RegisterCallback(L"Echelon.dll", InitEchelon);
        CallbackHandler::RegisterCallback(L"Core.dll", InitCore);
        CallbackHandler::RegisterCallback(L"WinDrv.dll", InitWinDrv);
        CallbackHandler::RegisterCallback(L"Xidi.32.dll", InitXidi);
        CallbackHandler::RegisterCallback(L"eax.dll", InitEAX);

        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(InitOnline, hook::pattern("8B F9 8B 47 30 85 C0 74 18"));
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