#include "stdafx.h"
#include "FixSettings.hpp"
#include "Shared/SplinterCell/Window.hpp"
#include <list>

ASI_SETTINGS_EXPORT(FixSettings())

import ComVars;
import LeadD3DRender;
import WidescreenFix;
import Window;
import FrameRate;
import Focus;
import DeveloperOverlay;
import Gamepad;
import Graphics;
import FileManager;
import BlacklistControls;
import LAN;
import Mouse;
import Splitscreen;
import LED;
import NativeMenu;

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
    bSkipIntro = FixSettings().Bool("MAIN.SkipIntro") != 0;
    bWindowedMode = FixSettings().Int("MAIN.WindowMode") != 0;
    bDisableBlackAndWhiteFilter = FixSettings().Bool("GRAPHICS.DisableBlackAndWhiteFilter") != 0;
    bBlacklistControlScheme = FixSettings().Bool("GAMEPLAY.BlacklistControlScheme") != 0;
    auto bUnlockDLC = FixSettings().Bool("UNLOCKS.UnlockDLC") != 0;
    bEnableSplitscreen = FixSettings().Bool("2INSTANCESPLITSCREEN.Enable") != 0;

    if (bEnableSplitscreen)
        bWindowedMode = true;

    InitFrameRate();

    //accept any refresh rate
    {
        auto pattern = hook::pattern("7F 17 89 45 FC");
        injector::MakeNOP(pattern.get_first(), 2);
    }

    //disable uplay
    {
        auto pattern = hook::pattern("74 28 E8 ? ? ? ? 8B 10 8B C8 FF 92 ? ? ? ? 3B C3");
        injector::WriteMemory<uint8_t>(pattern.get_first<uint8_t>(), 0xEB, true);
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
    Focus::Init();
    InitWindow();
    InitSplitscreen();
    InitLocalSplitscreen();
    SCCMenu::Init();
    NativeMenuHelp = SCCMenu::RequestHelp;
    InitGraphics();

    InitMouse();
    InitGamepad();

    if (bSkipIntro)
    {
        auto pattern = hook::pattern("55 8D 6C 24 88 81 EC ? ? ? ? A1 ? ? ? ? 33 C5 89 45 74 53 56 57 BE ? ? ? ? 68 ? ? ? ? 8B CE E8 ? ? ? ? E8 ? ? ? ? 50");
        injector::MakeRET(pattern.get_first());
    }

    if (FixSettings().Bool("MAIN.SkipPressStart"))
    {
        // Press any key: the title page (OpeningPage) sends ResetPressStart when it opens, then adds its key and mouse listeners (any key
        // becomes START), on START it sends SignIn LIVE=0 PRESSSTART=1 and leaves when the player is ready, removing the listeners.
        // The first time, SignIn is sent here on the menu manager's next update: sent from ResetPressStart the page left before it
        // added the listeners, they stayed and turned every key into START, which launched the Deniable Ops match from its lobby.
        // (CFlashMenuPreGameManager: command handler sub_D7B89A with the manager at [ebp+68h], virtual +2Ch sends a command; update sub_D7931F)
        static uintptr_t pressStartManager = 0;
        auto pattern = hook::pattern("68 ? ? ? ? 50 E8 ? ? ? ? 85 C0 59 59 74 14 E8 ? ? ? ? 8B 10 8B C8 FF 92 60 02 00 00 E9");
        static auto PressStartHook = safetyhook::create_mid(pattern.get_first(32), [](SafetyHookContext& regs)
        {
            static bool once = false;
            if (!std::exchange(once, true))
                pressStartManager = *reinterpret_cast<uintptr_t*>(regs.ebp + 0x68);
        });

        pattern = hook::pattern("55 8B EC D9 45 08 83 EC 48 56 57 51 D9 1C 24 8B F1 E8 ? ? ? ? 8B 8E C4 02 00 00");
        static auto MenuUpdateHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            if (!pressStartManager || regs.ecx != pressStartManager)
                return;
            auto manager = std::exchange(pressStartManager, 0);
            reinterpret_cast<int(__thiscall*)(uintptr_t, const char*, const char*, int32_t)>((*reinterpret_cast<uintptr_t**>(manager))[0x2C / 4])(manager, "SignIn", "LIVE=0 PRESSSTART=1", 0);
        });
    }

    // skip the hardware detection pop-up
    {
        auto pattern = hook::pattern("75 13 E8 ? ? ? ? 50 E8");
        injector::WriteMemory<uint8_t>(pattern.get_first<uint8_t>(), 0xEB, true);
    }

    // run on all CPU cores
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
        injector::WriteMemory<uint8_t>(pattern.get_first<uint8_t>(), 0xEB, true); // UEPECInventory

        pattern = hook::pattern("74 15 FF 74 24 08 8B CE E8 ? ? ? ? 8B 40 04 F7 D0");
        injector::WriteMemory<uint8_t>(pattern.get_first<uint8_t>(), 0xEB, true); // UEPECInventory

        pattern = hook::pattern("75 25 FF 75 FC");
        injector::WriteMemory<uint8_t>(pattern.get_first<uint8_t>(), 0xEB, true); // MenuUniformSelect

        pattern = hook::pattern("8B 41 7C D1 E8 F7 D0");
        {
            injector::MakeNOP(pattern.get_first(), 10, true);
            static auto UEChallengeProfileExternal1 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs) { regs.eax = 0; });
        }

        pattern = hook::pattern("8B 41 7C C1 E8 03");
        {
            injector::MakeNOP(pattern.get_first(), 11, true);
            static auto UEChallengeProfileExternal2 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs) { regs.eax = 0; });
        }

        // MapConfigurationDLC.xml
        pattern = hook::pattern("0F 85 ? ? ? ? 39 7D EC");
        injector::MakeNOP(pattern.get_first(), 6, true);

        pattern = hook::pattern("0F 84 ? ? ? ? 8D 45 E8 50 8B CB");
        injector::MakeNOP(pattern.get_first(), 6, true);

        pattern = hook::pattern("7E 50 8B 45 E8");
        injector::MakeJMP(pattern.get_first(), hook::pattern("8B 0D ? ? ? ? 8B 01 57 FF 35 ? ? ? ? 57 68 ? ? ? ? FF 50 10 8B D8").get_first(), true);
    }

    InitLAN();
    InitWidescreenFix();
    WFP::onInitEvent().executeAll();
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

        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime([]()
        {
            SCWindow::EnableDpi();
            FixSettings();
            asi_settings::LiveLink::Start();
            DeveloperOverlay::Init();
            Init();
        }, hook::pattern("D9 1C 24 E8 ? ? ? ? D9 5E 0C"));
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
