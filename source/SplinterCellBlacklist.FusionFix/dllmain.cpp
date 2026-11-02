#include "stdafx.h"
#include "FixSettings.hpp"
#include "Shared/SplinterCell/Window.hpp"

ASI_SETTINGS_EXPORT(FixSettings())

import ComVars;
import FileManager;
import Window;
import Startup;
import Mouse;
import FrameLimit;
import Focus;
import WidescreenFix;
import Hud;
import Missions;
import Unlocks;
import LED;
import Splitscreen;
import DeveloperOverlay;
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

// https://github.com/unixoide/5th-echelon
void LaunchDedicatedServer(const std::string& exePath)
{
    std::error_code ec;
    auto dsPath = std::filesystem::path(exePath);
    auto processPath = dsPath.is_absolute() ? dsPath : (GetExeModulePath() / dsPath);
    auto workingDir = std::filesystem::path(processPath).remove_filename();
    if (!std::filesystem::exists(processPath, ec))
        return;

    // closed together with the game
    HANDLE hJob = CreateJobObject(nullptr, nullptr);
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION info = {};
    info.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    SetInformationJobObject(hJob, JobObjectExtendedLimitInformation, &info, sizeof(info));
    STARTUPINFO si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    if (CreateProcessInJob(hJob, processPath.c_str(), NULL, nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, workingDir.c_str(), &si, &pi))
    {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
}

void Init()
{
    auto& settings = FixSettings();
    // Install before this fix's other executable hooks alter prefixes.
    BlacklistMenu::Init();
    asi_settings::LiveLink::Start();
    auto sDedicatedServerExePath = settings.Get("STARTUP.DedicatedServerExePath");

    if (!sDedicatedServerExePath.empty())
        LaunchDedicatedServer(sDedicatedServerExePath);

    InitFileManager();
    InitWindow();
    InitFocus();
    InitStartup();
    InitMouse();
    InitFrameLimit();
    InitMissions();
    InitUnlocks();
    Hud::Init();
    InitWidescreenFix();
    InitSplitscreen();
    DeveloperOverlay::Init();
    WFP::onInitEvent().executeAll();

    shSetProcessAffinityMask = safetyhook::create_inline(SetProcessAffinityMask, SetProcessAffinityMaskHook);
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("0F 84 ? ? ? ? 56 56 56 68 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 83 C4 18 50 8D 8D"));
        SCWindow::EnableDpi();
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
