#include "stdafx.h"
#include "FixSettings.hpp"
#include "../Shared/SplinterCell/Window.hpp"

ASI_SETTINGS_EXPORT(FixSettings())
#include <LEDEffects.h>
#include <list>

import ComVars;
import LeadD3DRender;
import WidescreenFix;
import Window;
import Graphics;
import FileManager;
import BlacklistControls;
import LAN;
import Mouse;
import Splitscreen;

#include "NativeMenu.hpp"

SafetyHookInline shSetProcessAffinityMask{};
BOOL WINAPI SetProcessAffinityMaskHook(HANDLE hProcess, DWORD_PTR dwProcessAffinityMask)
{
    if (hProcess == GetCurrentProcess())
    {
        DWORD_PTR processAffinityMask;
        DWORD_PTR systemAffinityMask;

        if (GetProcessAffinityMask(GetCurrentProcess(), &processAffinityMask, &systemAffinityMask))
        {
            return shSetProcessAffinityMask.stdcall<BOOL>(hProcess, systemAffinityMask);
        }
    }

    return shSetProcessAffinityMask.stdcall<BOOL>(hProcess, dwProcessAffinityMask);
}

namespace UESEQJobUnlockAchievement
{
    SafetyHookInline shsub_4D6DD5 = {};
    int __fastcall sub_4D6DD5(void* _this, void* edx)
    {
        return 0;
    }
}

void InitLED();

void Init()
{
    SCWindow::EnableDpi();

    bSkipIntro = FixSettings().Bool("MAIN.SkipIntro") != 0;
    bWindowedMode = FixSettings().Int("MAIN.WindowMode") != 0;
    bDisableNegativeMouseAcceleration = FixSettings().Bool("GAMEPLAY.DisableNegativeMouseAcceleration") != 0;
    bSkipSystemDetection = FixSettings().Bool("GENERAL.SkipSystemDetection") != 0;
    bForceCPUAffinityToAllCores = FixSettings().Bool("GENERAL.ForceCPUAffinityToAllCores") != 0;
    bUltraWideSupport = FixSettings().Bool("DISPLAY.UltraWideSupport") != 0;
    bDisableBlackAndWhiteFilter = FixSettings().Bool("GRAPHICS.DisableBlackAndWhiteFilter") != 0;
    bBlacklistControlScheme = FixSettings().Bool("GAMEPLAY.BlacklistControlScheme") != 0;
    auto bUnlockDLC = FixSettings().Bool("UNLOCKS.UnlockDLC") != 0;
    bEnableSplitscreen = FixSettings().Bool("2INSTANCESPLITSCREEN.Enable") != 0;

    static auto nFPSLimit = FixSettings().Int("MAIN.FPSLimit");

    if (bEnableSplitscreen)
        bWindowedMode = true;

    // frame rate limit: Engine.Display MaxFPS in User.ini (120 if it isn't there), 30 to 1000
    if (nFPSLimit > 0)
    {
        auto pattern = hook::pattern("FF 50 04 6A 1E 58 39 45 F8 7C");
        static auto MaxFPSHook = safetyhook::create_mid(pattern.get_first(3), [](SafetyHookContext& regs)
        {
            *reinterpret_cast<int32_t*>(regs.ebp - 8) = nFPSLimit;
        });
    }

    //accept any refresh rate
    {
        auto pattern = hook::pattern("7F 17 89 45 FC");
        injector::MakeNOP(pattern.get_first(), 2);
    }

    //disable uplay
    {
        auto pattern = hook::pattern("74 28 E8 ? ? ? ? 8B 10 8B C8 FF 92 ? ? ? ? 3B C3");
        ProtectedGameRef<uint8_t>(pattern.get_first<uint8_t>()) = 0xEB;
    }

    // UESEQJobUnlockAchievement crash fix
    {
        auto pattern = hook::pattern("56 8B F1 0F B6 46 ? 50 8B 46");
        UESEQJobUnlockAchievement::shsub_4D6DD5 = safetyhook::create_inline(pattern.get_first(), UESEQJobUnlockAchievement::sub_4D6DD5);
    }

    InitFileManager();
    if (!MenuOverridesAvailable())
    {
        SCCMenu::Log("native menu movie overrides unavailable: loader API or file-manager hooks missing");
    }
    InitWindow();
    InitSplitscreen();
    InitLocalSplitscreen();
    SCCMenu::Init();
    InitGraphics();

    InitMouse();

    if (bSkipIntro)
    {
        auto pattern = hook::pattern("55 8D 6C 24 88 81 EC ? ? ? ? A1 ? ? ? ? 33 C5 89 45 74 53 56 57 BE ? ? ? ? 68 ? ? ? ? 8B CE E8 ? ? ? ? E8 ? ? ? ? 50");
        injector::MakeRET(pattern.get_first());

        // Press any key: the title page (OpeningPage) sends ResetPressStart when it opens and SignIn LIVE=0 PRESSSTART=1 on a key press,
        // sign in right away the first time (CFlashMenuPreGameManager command handler sub_D7B89A, the manager at [ebp+68h], virtual +2Ch sends a command)
        pattern = hook::pattern("68 ? ? ? ? 50 E8 ? ? ? ? 85 C0 59 59 74 14 E8 ? ? ? ? 8B 10 8B C8 FF 92 60 02 00 00 E9");
        static auto PressStartHook = safetyhook::create_mid(pattern.get_first(32), [](SafetyHookContext& regs)
        {
            static bool once = false;
            if (std::exchange(once, true))
                return;
            auto manager = *reinterpret_cast<uintptr_t*>(regs.ebp + 0x68);
            reinterpret_cast<int(__thiscall*)(uintptr_t, const char*, const char*, int32_t)>((*reinterpret_cast<uintptr_t**>(manager))[0x2C / 4])(manager, "SignIn", "LIVE=0 PRESSSTART=1", 0);
        });
    }

    if (bSkipSystemDetection)
    {
        auto pattern = hook::pattern("75 13 E8 ? ? ? ? 50 E8");
        ProtectedGameRef<uint8_t>(pattern.get_first<uint8_t>()) = 0xEB;
    }

    if (bForceCPUAffinityToAllCores)
    {
        shSetProcessAffinityMask = safetyhook::create_inline(SetProcessAffinityMask, SetProcessAffinityMaskHook);
    }

    // controller scheme force X360
    {
        auto pattern = hook::pattern("66 81 FF ? ? 75 0E 66 81 FE ? ? 75 13");
        injector::MakeNOP(pattern.get_first(), 14, true);
    }

    InitBlacklistControls();

    if (bUnlockDLC)
    {
        auto pattern = hook::pattern("74 15 FF 74 24 08 8B CE E8 ? ? ? ? 8B 40 08");
        if (!pattern.empty())
            ProtectedGameRef<uint8_t>(pattern.get_first<uint8_t>()) = 0xEB; // UEPECInventory

        pattern = hook::pattern("74 15 FF 74 24 08 8B CE E8 ? ? ? ? 8B 40 04 F7 D0");
        if (!pattern.empty())
            ProtectedGameRef<uint8_t>(pattern.get_first<uint8_t>()) = 0xEB; // UEPECInventory

        pattern = hook::pattern("75 25 FF 75 FC");
        if (!pattern.empty())
            ProtectedGameRef<uint8_t>(pattern.get_first<uint8_t>()) = 0xEB; // MenuUniformSelect

        pattern = hook::pattern("8B 41 7C D1 E8 F7 D0");
        if (!pattern.empty())
        {
            injector::MakeNOP(pattern.get_first(), 10, true);
            static auto UEChallengeProfileExternal1 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs) { regs.eax = 0; });
        }

        pattern = hook::pattern("8B 41 7C C1 E8 03");
        if (!pattern.empty())
        {
            injector::MakeNOP(pattern.get_first(), 11, true);
            static auto UEChallengeProfileExternal2 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs) { regs.eax = 0; });
        }

        // MapConfigurationDLC.xml
        pattern = hook::pattern("0F 85 ? ? ? ? 39 7D EC");
        if (!pattern.empty())
            injector::MakeNOP(pattern.get_first(), 6, true);

        pattern = hook::pattern("0F 84 ? ? ? ? 8D 45 E8 50 8B CB");
        if (!pattern.empty())
            injector::MakeNOP(pattern.get_first(), 6, true);

        pattern = hook::pattern("7E 50 8B 45 E8");
        if (!pattern.empty())
            injector::MakeJMP(pattern.get_first(), hook::pattern("8B 0D ? ? ? ? 8B 01 57 FF 35 ? ? ? ? 57 68 ? ? ? ? FF 50 10 8B D8").get_first(), true);
    }

    InitLAN();
    InitWidescreenFix();
    InitLED();
}

void InitLED()
{

    auto bLightSyncRGB = FixSettings().Bool("MAIN.LightSyncRGB");

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

            constexpr auto minBrightness = 0.3f;
            constexpr auto maxBrightness = 1.0f;

            fPlayerVisibility = std::clamp(fPlayerVisibility, minBrightness, maxBrightness);

            auto [R, G, B] = LEDEffects::RGBtoPercent(255, 39, 26, gBlacklistIndicators ? fPlayerVisibility : ((maxBrightness + minBrightness) - fPlayerVisibility));
            LEDEffects::SetLighting(R, G, B, false, false, false);
        });
    }
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        FixSettings();
        asi_settings::LiveLink::Start();
        {
            std::error_code ec;
            auto injPath = std::filesystem::path("InjectorCLIx86.exe");
            auto processPath = GetExeModulePath() / injPath;
            auto workingDir = std::filesystem::path(processPath).remove_filename();
            if (std::filesystem::exists(processPath, ec))
            {
                auto cmd = processPath.wstring() + L" pid:" + std::to_wstring(GetCurrentProcessId()) + L" HookLibraryx86.dll";
                HANDLE hJob = CreateJobObject(nullptr, nullptr);
                JOBOBJECT_EXTENDED_LIMIT_INFORMATION info = { };
                info.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
                SetInformationJobObject(hJob, JobObjectExtendedLimitInformation, &info, sizeof(info));
                STARTUPINFO si = { sizeof(si) };
                PROCESS_INFORMATION pi;
                if (CreateProcessInJob(hJob,
                    NULL,
                    const_cast<wchar_t*>(cmd.c_str()),
                    nullptr, nullptr, FALSE, CREATE_NO_WINDOW,
                    nullptr, workingDir.c_str(), &si, &pi))
                {
                    CloseHandle(pi.hProcess);
                    CloseHandle(pi.hThread);
                }
            }
        }

        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("D9 1C 24 E8 ? ? ? ? D9 5E 0C"));
        CallbackHandler::RegisterCallback(L"LeadD3DRender.dll", InitLeadD3DRender);
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
