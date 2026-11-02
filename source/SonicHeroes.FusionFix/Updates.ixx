module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"

export module Updates;

import ComVars;

export
{
namespace Updates
{

    void WINAPI Exit(UINT code)
    {
        RWGTA::UpdateCheck::OnExit();
        ExitProcess(code);
    }
}

void InitUpdates()
{
    UpdateGameState.SetAddress(*hook::pattern("8B 46 04 53 55 33 ED 3B C5 57 89 2E BF 01 00 00 00 75 16").get_first<uintptr_t*>(0x14));
    RWGTA::UpdateCheck::Config config;
    config.pluginId = "SonicHeroes.FusionFix";
    config.fileVersion = rsc_FileVersion;
    config.settings = &FixSettings();
    config.playable = []()
    {
        const auto state = UpdateGameState.get();
        DWORD process = 0;
        GetWindowThreadProcessId(GetForegroundWindow(), &process);
        return state && *reinterpret_cast<int32_t*>(state + 0x38) == 1 && process == GetCurrentProcessId();
    };
    config.show = [](const std::wstring& line)
    {
        MessageBoxW(GetForegroundWindow(), line.c_str(), L"SONIC HEROES(TM)", MB_OK | MB_ICONINFORMATION);
    };
    RWGTA::UpdateCheck::Init(std::move(config));
    IATHook::Replace(GetModuleHandleW(nullptr), "KERNEL32.dll", std::forward_as_tuple("ExitProcess", Updates::Exit));
}
}
