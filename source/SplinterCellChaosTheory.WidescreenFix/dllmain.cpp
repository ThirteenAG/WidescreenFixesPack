#include "stdafx.h"
#include <TlHelp32.h>

import ComVars;
import D3DDrv;
import Engine;
import Core;
import WinDrv;
import GUI;
import Xidi;
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
    Screen.fRawInputMouseForMenu = std::clamp(iniReader.ReadFloat("MAIN", "RawInputMouseForMenu", 1.0f), 0.0f, 5.0f);
    bHudWidescreenMode = iniReader.ReadInteger("MAIN", "HudWidescreenMode", 1) != 0;
    nWidescreenHudOffset = iniReader.ReadInteger("MAIN", "WidescreenHudOffset", 100);
    fWidescreenHudOffset = static_cast<float>(nWidescreenHudOffset);
    bDisableAltTabFix = iniReader.ReadInteger("MAIN", "DisableAltTabFix", 1) != 0;
    nShadowMapResolution = iniReader.ReadInteger("GRAPHICS", "ShadowMapResolution", 0);
    bEnableShadowFiltering = iniReader.ReadInteger("GRAPHICS", "EnableShadowFiltering", 0) != 0;
    auto nFPSLimit = iniReader.ReadInteger("MISC", "FPSLimit", 1000);
    gColor = iniReader.ReadInteger("BONUS", "GogglesLightColor", 0);
    gBlacklistIndicators = iniReader.ReadInteger("BONUS", "BlacklistIndicators", 0);
    bLightSyncRGB = iniReader.ReadInteger("BONUS", "LightSyncRGB", 1);
    auto bSkipIntro = iniReader.ReadInteger("MAIN", "SkipIntro", 0) != 0;
    auto bSkipPressAnyKeyToContinue = iniReader.ReadInteger("MAIN", "SkipPressAnyKeyToContinue", 0) != 0;

    if (!Screen.Width || !Screen.Height)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();

    Screen.fWidth = static_cast<float>(Screen.Width);
    Screen.fHeight = static_cast<float>(Screen.Height);
    Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);

    auto pattern = hook::pattern("68 ? ? ? ? E8 ? ? ? ? 8B 8C 24 20 02 00 00");
    aJoysticks = *pattern.get_first<const wchar_t*>(1);

    pattern = hook::pattern("55 8B EC 83 E4 F8 81 EC 08 02 00 00");
    shsub_10CC8580 = safetyhook::create_inline(pattern.get_first(), sub_10CC8580);

    // Disable safe mode
    pattern = hook::pattern("E8 ? ? ? ? 68 ? ? ? ? 53 68");
    injector::MakeNOP(pattern.get_first(0), 5, true);

    // Unlock all resolutions
    pattern = hook::pattern("75 ? B8 ? ? ? ? 68 ? ? ? ? 50 E8 ? ? ? ? 8B 5C 24 3C");
    injector::MakeJMP(pattern.get_first(0), hook::get_pattern("8B 5C 24 18 8B 43 78 3B C5"), true);

    // Stop mouse sensitivity from affecting gamepad movement in certain states
    pattern = hook::pattern("E8 ? ? ? ? D8 4C 24 20 83 C4 04");
    injector::MakeCALL(pattern.get_first(0), static_cast<float(__cdecl*)(float)>([](float a1) -> float { return 1.0f; }), true);

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

    {
        auto pattern = hook::pattern("FF 15 ? ? ? ? EB 60");
        static auto SetPropWHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            hGameWindow = (HWND)regs.edx;
            if (Screen.fRawInputMouseForMenu > 0.0f)
                RawInputHandler<>::RegisterRawInput(hGameWindow, Screen.Width, Screen.Height, Screen.fRawInputMouseForMenu);
        });

        pattern = hook::pattern("FF 15 ? ? ? ? A1 ? ? ? ? 85 C0 75 ? 68 ? ? ? ? E8 ? ? ? ? 83 C4 04 A3 ? ? ? ? E8 ? ? ? ? A1");
        static auto SetPropAHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            hGameWindow = (HWND)regs.edi;
            if (Screen.fRawInputMouseForMenu > 0.0f)
                RawInputHandler<>::RegisterRawInput(hGameWindow, Screen.Width, Screen.Height, Screen.fRawInputMouseForMenu);
        });
    }

    InitD3DDrv();
    InitEngine();
    InitCore();
    InitWinDrv();
    InitGUI();

#ifdef _DEBUG
    pattern = hook::pattern("8B 88 80 00 00 00 6A 01");
    static auto SkipMenuHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        static char bOnce = 0;
        if (bOnce < 2)
        {
            keybd_event(VK_RETURN, 0x45, KEYEVENTF_EXTENDEDKEY | 0, 0);
            keybd_event(VK_RETURN, 0x45, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
            bOnce++;
        }
    });
#endif

    if (nFPSLimit)
    {
        static float fFPSLimit = 1.0f / static_cast<float>(nFPSLimit);
        auto pattern = hook::pattern("8B 15 ? ? ? ? A1 ? ? ? ? 89 54 24 10 89 44 24 18");
        injector::WriteMemory(pattern.get_first(2), &fFPSLimit, true);
    }

    //Bink high res
    pattern = hook::pattern("68 E0 01 00 00 68 80 02 00 00 50 FF 52 ? EB 18");
    if (!pattern.empty())
    {
        injector::WriteMemory(pattern.get_first(1), 2160, true);
        injector::WriteMemory(pattern.get_first(6), 3840, true);
    }
    
    pattern = hook::pattern("68 E0 01 00 00 68 80 02 00 00 50 FF 51 ? EB 18 8B 10 6A 01 6A 15 6A 00 6A 01 68 E0 01 00 00 68 80 02 00 00 50 FF 52 ? 85 DB");
    if (!pattern.empty())
    {
        injector::WriteMemory(pattern.get_first(1), 2160, true);
        injector::WriteMemory(pattern.get_first(6), 3840, true);
    }
    
    pattern = hook::pattern("68 E0 01 00 00 68 80 02 00 00 50 FF 51 ? EB 18 8B 10 6A 01 6A 15 6A 00 6A 01 68 E0 01 00 00 68 80 02 00 00 50 FF 52 ? 8B AC 24 88 00 00 00");
    if (!pattern.empty())
    {
        injector::WriteMemory(pattern.get_first(1), 2160, true);
        injector::WriteMemory(pattern.get_first(6), 3840, true);
    }
    
    static float h = 1.0f / 2160.0f;
    pattern = hook::pattern("D8 0D ? ? ? ? 8B 0E");
    if (!pattern.empty())
        injector::WriteMemory(pattern.get_first(2), &h, true);
    
    static float w = 1.0f / 3840.0f;
    pattern = hook::pattern("D8 0D ? ? ? ? 8B 54 24 18 8B 44 24 24");
    if (!pattern.empty())
        injector::WriteMemory(pattern.get_first(2), &w, true);

    // Button names
    pattern = hook::pattern("56 E8 ? ? ? ? 83 C4 04 50 8D 8C 24 90 00 00 00");
    static auto ButtonNamesHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        auto sv = std::wstring_view((const wchar_t*)regs.esi);
        
        static const wchar_t* labelStrs[] = {
            L"(A)",
            L"(B)",
            L"(X)",
            L"(Y)",
            L"(LB)",
            L"(RB)",
            L"(LT)",
            L"(RT)",
            L"(Back)",
            L"(Start)",
            L"(LS)",
            L"(RS)",
            L"(D-pad Up)",
            L"(D-pad Down)",
            L"(D-pad Left)",
            L"(D-pad Right)",
            L"(Left Stick X)",
            L"(Left Stick Y)",
            L"(Right Stick X)",
            L"(Right Stick Y)",
        };

        for (auto [index, label] : std::views::enumerate(labelStrs))
        {
            if (sv == (L"Joy ") + std::to_wstring(index + 1))
            {
                regs.esi = (uintptr_t)label;
                break;
            }
        }
    });

    if (bSkipIntro)
    {
        auto pattern = hook::pattern("75 ? 68 ? ? ? ? E8 ? ? ? ? 50 E8 ? ? ? ? 83 C4 08 85 C0 75 ? 68");
        injector::WriteMemory<uint8_t>(pattern.get_first(0), 0xEB, true);
    }

    if (bSkipPressAnyKeyToContinue)
    {
        auto pattern = hook::pattern("6A 0A FF 50 ? 8B 0D");
        injector::WriteMemory<uint8_t>(pattern.get_first(1), 1, true);

        pattern = hook::pattern("6A 08 FF 52 ? 8B 0D ? ? ? ? 3B FE");
        injector::WriteMemory<uint8_t>(pattern.get_first(1), 1, true);
    }
}

DWORD FindProcessId(const std::wstring& processName)
{
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE)
    {
        return 0;
    }

    if (Process32First(hSnapshot, &pe32))
    {
        do
        {
            if (processName == pe32.szExeFile)
            {
                CloseHandle(hSnapshot);
                return pe32.th32ProcessID;
            }
        } while (Process32Next(hSnapshot, &pe32));
    }

    CloseHandle(hSnapshot);
    return 0;
}

void InitGameOverlayRenderer()
{
    auto steamPid = FindProcessId(L"steam.exe");
    if (steamPid)
    {
        auto steamPath = GetModulePath(GetModuleHandleW(L"GameOverlayRenderer.dll")).parent_path();
        auto exePath = steamPath / L"GameOverlayUI.exe";
        std::wstring args = L" -steampid " + std::to_wstring(steamPid) + L" -pid " + std::to_wstring(GetCurrentProcessId()) + L" -gameid " + L"13570" + L" -manuallyclearframes 0";

        SHELLEXECUTEINFOW sei = { sizeof(sei) };
        sei.lpFile = exePath.c_str();
        sei.lpParameters = args.c_str();
        sei.nShow = SW_HIDE;
        sei.fMask = SEE_MASK_NOCLOSEPROCESS;
        ShellExecuteEx(&sei);
    }
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("8D 84 24 34 04 00 00 68 ? ? ? ? 50 E8 ? ? ? ? 83 C4 14"));
        CallbackHandler::RegisterCallback(L"Xidi.32.dll", InitXidi);
        CallbackHandler::RegisterCallback(L"GameOverlayRenderer.dll", InitGameOverlayRenderer);
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
