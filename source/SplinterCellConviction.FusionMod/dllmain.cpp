#include "stdafx.h"
#include <LEDEffects.h>
#include <list>

import ComVars;
import Lead3DEngine;
import LeadD3DRender;
import WidescreenFix;
import Window;
import Graphics;
import FileManager;
import BlacklistControls;
import LAN;

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

void Init()
{
    CIniReader iniReader("");
    bSkipIntro = iniReader.ReadInteger("MAIN", "SkipIntro", 1) != 0;
    bWindowedMode = iniReader.ReadInteger("MAIN", "WindowedMode", 0) != 0;
    bDisableNegativeMouseAcceleration = iniReader.ReadInteger("MAIN", "DisableNegativeMouseAcceleration", 1) != 0;
    bSkipSystemDetection = iniReader.ReadInteger("MAIN", "SkipSystemDetection", 1) != 0;
    bForceCPUAffinityToAllCores = iniReader.ReadInteger("MAIN", "ForceCPUAffinityToAllCores", 1) != 0;
    bPartialUltraWideSupport = iniReader.ReadInteger("MAIN", "PartialUltraWideSupport", 1) != 0;
    bDisableBlackAndWhiteFilter = iniReader.ReadInteger("MAIN", "DisableBlackAndWhiteFilter", 0) != 0;
    bBlacklistControlScheme = iniReader.ReadInteger("CONTROLS", "BlacklistControlScheme", 1) != 0;
    auto bUnlockDLC = iniReader.ReadInteger("UNLOCKS", "UnlockDLC", 1) != 0;
    bEnableSplitscreen = iniReader.ReadInteger("2INSTANCESPLITSCREEN", "Enable", 1) != 0;

    if (!bWindowedMode)
        bEnableSplitscreen = false;

    //accept any refresh rate
    {
        auto pattern = hook::pattern("7F 17 89 45 FC");
        injector::MakeNOP(pattern.get_first(), 2);
    }

    //disable uplay
    {
        auto pattern = hook::pattern("74 28 E8 ? ? ? ? 8B 10 8B C8 FF 92 ? ? ? ? 3B C3");
        injector::WriteMemory<uint8_t>(pattern.get_first(), 0xEB, true);
    }

    // UESEQJobUnlockAchievement crash fix
    {
        auto pattern = hook::pattern("56 8B F1 0F B6 46 ? 50 8B 46");
        UESEQJobUnlockAchievement::shsub_4D6DD5 = safetyhook::create_inline(pattern.get_first(), UESEQJobUnlockAchievement::sub_4D6DD5);
    }

    InitFileManager();
    InitWindow();
    InitGraphics();

    if (bDisableNegativeMouseAcceleration)
    {
        auto pattern = hook::pattern("76 05 0F 28 D9 EB 08 0F 2F DA");
        injector::MakeNOP(pattern.get_first(0), 2, true);
        injector::MakeNOP(pattern.get_first(5), 2, true);
        injector::MakeNOP(pattern.get_first(10), 2, true);

        pattern = hook::pattern("77 0D 0F 2F C2 76 05 0F 28 CA EB 03 0F 28 C8 0F 57 C0");
        injector::MakeNOP(pattern.get_first(0), 2, true);
        injector::MakeNOP(pattern.get_first(5), 2, true);
    }

    if (bSkipIntro)
    {
        auto pattern = hook::pattern("55 8D 6C 24 88 81 EC ? ? ? ? A1 ? ? ? ? 33 C5 89 45 74 53 56 57 BE ? ? ? ? 68 ? ? ? ? 8B CE E8 ? ? ? ? E8 ? ? ? ? 50");
        injector::MakeRET(pattern.get_first());
    }

    if (bSkipSystemDetection)
    {
        auto pattern = hook::pattern("75 13 E8 ? ? ? ? 50 E8");
        injector::WriteMemory<uint8_t>(pattern.get_first(), 0xEB, true);
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
            injector::WriteMemory<uint8_t>(pattern.get_first(), 0xEB, true); // UEPECInventory

        pattern = hook::pattern("74 15 FF 74 24 08 8B CE E8 ? ? ? ? 8B 40 04 F7 D0");
        if (!pattern.empty())
            injector::WriteMemory<uint8_t>(pattern.get_first(), 0xEB, true); // UEPECInventory

        pattern = hook::pattern("75 25 FF 75 FC");
        if (!pattern.empty())
            injector::WriteMemory<uint8_t>(pattern.get_first(), 0xEB, true); // MenuUniformSelect

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
}

void InitLED()
{
    CIniReader iniReader("");
    auto bLightSyncRGB = iniReader.ReadInteger("LOGITECH", "LightSyncRGB", 1);

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
        CallbackHandler::RegisterCallback(L"Lead3DEngine.dll", InitLead3DEngine);
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(InitLED, hook::pattern("D9 1C 24 E8 ? ? ? ? D9 5E 0C"));
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
