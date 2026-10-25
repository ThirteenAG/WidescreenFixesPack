#include "stdafx.h"
#include "FixSettings.hpp"

ASI_SETTINGS_EXPORT(FixSettings())
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
import GUI;
import WidescreenHUD;

#include "NativeMenu.hpp"

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

    Screen.Width = FixSettings().Int("MAIN.ResX");
    Screen.Height = FixSettings().Int("MAIN.ResY");
    Screen.bRawInputMouse = FixSettings().Bool("MAIN.RawInputMouse");
    FixSettings().OnChange("MAIN.MouseSensitivity", []
    {
        RawInputHandler<>::SetSensitivity(FixSettings().Float("MAIN.MouseSensitivity") * 0.5f);
    });
    Screen.bRawInputMouseRawData = FixSettings().Int("MAIN.RawInputMouseRawData") != 0;
    Screen.nHudWidescreenMode = FixSettings().Int("MAIN.HudWidescreenMode");
    Screen.fHudAspectRatioConstraint = ParseWidescreenHudOffset(FixSettings().String("MAIN.HudAspectRatioConstraint"));
    bool bForceLL = FixSettings().Int("MAIN.ForceLL") != 0;
    auto nFPSLimit = FixSettings().Int("MAIN.FPSLimit");
    Screen.szLoadscPath = FixSettings().CfgPath();
    Screen.szLoadscPath.replace_extension(".png");
    gBlacklistIndicators = FixSettings().Int("BONUS.BlacklistIndicators");
    gColor = FixSettings().Int("BONUS.GogglesLightColor");
    bLightSyncRGB = FixSettings().Int("MAIN.LightSyncRGB");

    if (Screen.Width <= 0 || Screen.Height <= 0 || Screen.Width > 16384 || Screen.Height > 16384)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();
    if (Screen.Width <= 0 || Screen.Height <= 0)
        std::tie(Screen.Width, Screen.Height) = std::make_tuple(640, 480);

    auto exePath = GetExeModulePath();

    {
        // The engine creates a missing game INI by copying Default.ini, but only when the file doesn't exist,
        // so do the same here instead of generating a file with only a few keys in it.
        // Only update it when it could be read, otherwise everything else in it would be lost
        auto sc4IniPath = exePath / "SplinterCell4.ini";
        std::error_code ec;
        if (!std::filesystem::exists(sc4IniPath, ec) && !ec)
            CopyFileW((exePath / "Default.ini").c_str(), sc4IniPath.c_str(), TRUE);

        mINI::INIStructure sc4Ini;
        mINI::INIFile sc4IniFile(sc4IniPath);
        if (sc4IniFile.read(sc4Ini))
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
        if (userIniFile.read(userIni))
        {
            for (auto key : { "Joy1", "Joy2", "Joy3", "Joy4", "Joy5", "Joy6", "Joy7", "Joy8", "Joy9", "Joy10", "Joy11", "Joy12", "Joy13", "Joy14", "Joy15", "Joy16", "JoyX", "JoyY", "JoyZ", "JoyV", "JoyR", "JoyU", "AnalogUp", "AnalogDown", "AnalogLeft", "AnalogRight", "MouseWheelUp", "MouseWheelDown", "Tilde" })
                userIni["Engine.Input"].setAll(key, FixSettings().String(std::string("INPUT.") + key));
            userIniFile.generate(userIni);
        }
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
        // Real pixels on scaled desktops: the desktop resolution, and a full screen picture not drawn into a corner
        WindowedModeWrapper::EnableDpiAwareness();
        FixSettings();
        asi_settings::LiveLink::Start();
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("74 ? 68 ? ? ? ? 53 FF D7"));
        CallbackHandler::RegisterCallback(L"Window.dll", InitWindow);
        CallbackHandler::RegisterCallback(L"Engine.dll", InitEngine);
        CallbackHandler::RegisterCallback(L"D3DDrv.dll", InitD3DDrv);
        CallbackHandler::RegisterCallback(L"gear_RD.dll", InitGear);
        CallbackHandler::RegisterCallback(L"EchelonMenus.dll", [] { InitEchelonMenus(); DAMenu::Init(); NativeWindowsMenuHelp = DAMenu::WindowsHelp; });
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
    else if (reason == DLL_PROCESS_DETACH)
    {
        WFP::onShutdownEvent().executeAll();
    }
    return TRUE;
}
