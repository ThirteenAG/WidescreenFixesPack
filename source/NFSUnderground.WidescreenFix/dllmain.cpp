#include "stdafx.h"
#include "d3d9.h"
#include "FixSettings.hpp"
#include "../Shared/NFS/Cursor.hpp"

import ComVars;

// The settings' runtime interface for menu hosts and editors (AsiSettings_GetApi)
ASI_SETTINGS_EXPORT(FixSettings())

std::once_flag of;

void MainLoop()
{
    std::call_once(of, []()
    {
        WFP::onGameInitEvent().executeAll();
    });

    CursorConfinement::Frame(hWnd.get());
    WFP::onGameProcessEvent().executeAll();

    static bool oldMenuState = GameFlowManager::IsPaused();
    bool curMenuState = GameFlowManager::IsPaused();
    if (curMenuState != oldMenuState)
    {
        if (curMenuState)
            WFP::onMenuEnterEvent().executeAll();
        else
            WFP::onMenuExitEvent().executeAll();
    }
    oldMenuState = curMenuState;
    if (curMenuState)
        WFP::onMenuDrawingEvent().executeAll();
}

// The fix supports Speed.exe 1.4 (the US exe and its no-CD versions, and the Russian 1.4). Other versions, such
// as the original 1.0, pass the startup check but lack much of the code the fix patches, and patching them would
// crash. Known 1.4 builds are recognised by their PE header; for anything else every code pattern the fix needs
// unconditionally must be present.
bool IsSupportedExe()
{
    auto module = GetModuleHandleW(nullptr);
    auto dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(module);
    auto nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(reinterpret_cast<const uint8_t*>(module) + dos->e_lfanew);
    struct Build { DWORD timestamp; DWORD imageSize; };
    static constexpr Build KnownBuilds[] =
    {
        { 0x400721AA, 0x394368 },   // 1.4 US, and no-CD versions of it
        { 0x72454562, 0x394368 },   // 1.4 US, no-CD
        { 0x438590E2, 0x39B5BC },   // 1.4 Russian
    };
    for (auto& build : KnownBuilds)
    {
        if (nt->FileHeader.TimeDateStamp == build.timestamp && nt->OptionalHeader.SizeOfImage == build.imageSize)
            return true;
    }

    static const char* RequiredPatterns[] =
    {
        "8B F8 A0 ? ? ? ? 84 C0 0F 85",
        "83 3D ? ? ? ? 01 8B 77 58",
        "BE ? ? ? ? E8 ? ? ? ? A1 ? ? ? ? 83 C4 ? 85 C0",
        "53 55 8B 6C 24 0C 8B 5D 18 85 DB 57 8B 7C 24 14 74 06 57 E8",
        "55 8B EC 83 E4 ? 81 EC ? ? ? ? A1 ? ? ? ? 53 8B 5D",
        "E8 ? ? ? ? 8B 43 ? 50 8D BC 24",
        "8B 4B 1C 66 89 81 C4 00 00 00 66 83 3F 03",
        "C6 43 ? ? 74 ? 8B 43 ? 85 C0 0F 84",
        "D8 0D ? ? ? ? 57 0F B6 B8",
        "C7 ? ? ? ? 00 00 00 00 A0 43 C7 ? ? ? ? 00 00 00 00 70 43",
        "C7 ? ? ? 00 00 A0 43 C7 ? ? ? 00 00 70 43",
        "D8 1D ? ? ? ? DF E0 F6 C4 41 75 0F D9 45 FC",
        "81 C2 C0 FE FF FF",
        "05 C0 FE FF FF",
        "8D 88 C0 FE FF FF",
        "8B 06 8B 4C 24 08",
        "8B 07 8B 4C 24 0C",
        "8B 47 1C 8B 4F 20 89 84 24 10 01 00 00 8B 47 24 89 84 24 18 01 00 00 39 5C 24 04",
        "D8 05 ? ? ? ? D9 5B 68 D8 05 ? ? ? ? D9 5B 6C D9 40 3C",
        "8B 4D 08 D9 41 68 53 57 8B 5D 0C 8B F8",
        "D9 40 68 8B 4D 0C D8 60 70 53 56 8D 91 83 00 00 00 D9 54 24 34",
        "D9 40 1C 8B 4D 08 D9 40 20",
        "D8 43 1C 8B 54 24 18 8B 44 24 10 D9 1A 8B 4E 68 50 51 57",
        "68 00 00 80 3F 68 00 00 00 3F 68 00 00 00 3F 68 00 00 00 BF 68 00 00 00 BF 50 E8",
        "83 EC 18 56 8B 74 24 24 8B 04 B5 ? ? ? ? 85 C0 57 74",
        "A3 ? ? ? ? B0 80 89 0D",
        "81 EC AC 03 00 00 55 8B AC 24 B4 03 00 00 8B 85 30 01 00 00",
        "E8 ? ? ? ? 8B 46 08 8B 3D",
        "8B C8 83 E1 1F 40 84 C9",
        "8B 44 24 ? 8B 4C 24 ? 8B 53",
        "85 C0 7D ? 3D ? ? ? ? 74 ? 3D ? ? ? ? 0F 85 ? ? ? ? A1 ? ? ? ? ? ? 50 FF 52 ? 5F 5B",
        "8A 44 24 1C A2 ? ? ? ? E8",
        "57 57 50 E8 ? ? ? ? 8B 4C 24 ? 68 00 20 00 00",
        "57 E8 ? ? ? ? 8A 46 0C 83 C4 10 A8 04",
        "85 C0 74 3D 8B 0D ? ? ? ? 85 C9 56 0F B6 70 F0 57 8D 78 E8",
        "74 ? 85 C0 0F 95 C1",
        "BF ? ? ? ? 6A ? E8",
        "81 FF ? ? ? ? 89 46 ? 89 48 ? ? ? 7C ? ? ? 85 F6",
        "8B 44 24 ? 3D ? ? ? ? 56 74",
        "89 36 89 76 04 A1 ? ? ? ? 83 F8 05",
        "8B 4E 04 89 01 83 C7 04 81 FF ? ? ? ? 89 46 04 89 48 04 89 30 7C C8 8B 36",
        "C7 05 ? ? ? ? 22 56 00 00 A1 ? ? ? ? 85 C0",
        "D9 54 24 14 D9 E1 D8 1D ? ? ? ? DF E0 F6 C4 05 7A 08",
        "8A 44 24 24 8B 6C 24 68 8B 7D 0C 83 CE FF 84 C0 8D 4C 24 24 74 ? 6B F6 21",
        "0F 84 B8 00 00 00 8D 4C 24 44",
        "99 D9 05 ? ? ? ? B9 64 00 00 00",
        "D8 0D ? ? ? ? D9 5C 24 0C D8 15 ? ? ? ? DF E0 F6 C4 41 74 08 DD D8 D9 05",
        "D9 1D ? ? ? ? DD D8 75 05 B8 01 00 00 00",
        "52 E8 ? ? ? ? 8B 2D ? ? ? ? BF",
        "68 89 88 88 3C 68 ? ? ? ? E8",
        "A1 ? ? ? ? 83 F8 0A 0F 87 ? ? ? ? FF 24 85 ? ? ? ? 56 E8",
        "83 EC 20 55 8B 6C 24 28 56 57 55 68 ? ? ? ? 6A 20 8D 74 24 18 E8 ? ? ? ? 8B 7B 0C 83 C4 0C",
        "6A 00 55 BB 01 00 00 00 E8 ? ? ? ? 8B FB 57 55 BB 02 00 00 00 E8",
        "56 57 8B F0 E8 ? ? ? ? 85 F6 8B F8 74 18 8B C6 E8 ? ? ? ? 85 C0 74 0D 8B CF 8B D0 E8 ? ? ? ? 85 C0 75 14 A1 ? ? ? ? 85 C0 74 27",
        "57 8B F8 39 3C FD ? ? ? ? 74 14 33 C0",
        "8B 0D ? ? ? ? 57 8B F8 3B F9 0F 84 ? ? ? ? 53 33 DB 83 F9 FF 0F 84 ? ? ? ? 56 33 F6 B8",
        "E8 ? ? ? ? ? ? ? ? ? ? ? 83 C4 ? 50 FF 91 ? ? ? ? EB ? 33 F6",
        "DB 40 18 C7 44 24",
        "D8 3D ? ? ? ? D9 1F E8 ? ? ? ? D9",
        "D8 0D ? ? ? ? ? ? E8 ? ? ? ? DD D8",
        "D8 3D ? ? ? ? D9 E0 D9 5E 54 D9 44 24 20",
        "8B 04 8D ? ? ? ? 85 C0 0F 84",
        "8B 0C 85 ? ? ? ? 85 C9 75 ? 85 C0",
        "8B 0C 85 ? ? ? ? 85 C9 74 ? A3 ? ? ? ? 40",
        "8B 34 85 ? ? ? ? 33 C9",
        "39 0C 85 ? ? ? ? 75",
        "39 0C 85 ? ? ? ? 74 ? E8 ? ? ? ? 5E 5B C3",
        "BA ? ? ? ? 79 ? 89 53",
        "BA ? ? ? ? 33 C9 3B C2",
        "83 FF ? 0F 8C ? ? ? ? 5F 5E 5D",
        "68 ? ? ? ? BE ? ? ? ? C7 05",
        "68 ? ? ? ? BE ? ? ? ? E8 ? ? ? ? 83 C4 ? 5F",
        "B8 ? ? ? ? BE ? ? ? ? 77",
        "B8 ? ? ? ? BE ? ? ? ? 50 E8 ? ? ? ? 83 C4 ? 5E E9 ? ? ? ? B8 ? ? ? ? BE ? ? ? ? 50 E8 ? ? ? ? 83 C4 ? 5E E9 ? ? ? ? B8 ? ? ? ? BE ? ? ? ? 50 E8 ? ? ? ? 83 C4 ? 5E E9 ? ? ? ? B8",
        "B8 ? ? ? ? BE ? ? ? ? 50 E8 ? ? ? ? 83 C4 ? 5E E9 ? ? ? ? B8 ? ? ? ? BE ? ? ? ? 50 E8 ? ? ? ? 83 C4 ? 5E E9 ? ? ? ? B8 ? ? ? ? BE ? ? ? ? 50 E8 ? ? ? ? 83 C4 ? 5E E9 ? ? ? ? BE",
        "B8 ? ? ? ? BE ? ? ? ? 50 E8 ? ? ? ? 83 C4 ? 5E E9 ? ? ? ? B8 ? ? ? ? BE ? ? ? ? 50 E8 ? ? ? ? 83 C4 ? 5E E9 ? ? ? ? BE",
        "B8 ? ? ? ? BE ? ? ? ? 50 E8 ? ? ? ? 83 C4 ? 5E E9 ? ? ? ? BE",
        "8B 0D ? ? ? ? 8B 15 ? ? ? ? B8",
        "89 1D ? ? ? ? 89 1D ? ? ? ? 89 35",
        "D8 25 ? ? ? ? E8 ? ? ? ? 8B 4E",
        "89 0D ? ? ? ? A3 ? ? ? ? 89 0D ? ? ? ? E9",
        "A3 ? ? ? ? A3 ? ? ? ? 75 ? A3 ? ? ? ? 89 0D ? ? ? ? A3",
        "8B 43 ? 83 C4 ? 83 CE",
        "8B C3 8D 4C 24 ? ? ? ? ? ? 5F 5E",
        "BB ? ? ? ? 8B 43 ? 8D 54 24",
        "55 E8 ? ? ? ? 83 C4 ? 5F 5E 5D 5B 8B E5",
        "0F 85 ? ? ? ? 5F 5E 5D 5B 81 C4",
        "C7 44 24 ? ? ? ? ? 89 5C 24 ? 89 5C 24 ? FF 15",
        "39 35 ? ? ? ? 74 ? 89 35 ? ? ? ? 33 C0",
        "3D ? ? ? ? 75 ? A1 ? ? ? ? ? ? 50 FF 51 ? 3D ? ? ? ? 75 ? E8 ? ? ? ? 39 35",
        "3D ? ? ? ? 75 ? E8 ? ? ? ? 39 35",
        "A1 ? ? ? ? 74 ? 85 C0",
        "E9 ? ? ? ? 85 C0 74 ? A1",
        "A0 ? ? ? ? 84 C0 75 ? A1 ? ? ? ? 85 C0 74 ? 6A ? 6A ? 6A",
        "8B 0D ? ? ? ? 68 ? ? ? ? 6A ? 51 FF 15",
        "A1 ? ? ? ? 68 ? ? ? ? 50 FF 15",
        "A1 ? ? ? ? 85 C0 74 ? A1 ? ? ? ? 68",
        "A1 ? ? ? ? 8B 0C 85 ? ? ? ? 85 C9",
        "A3 ? ? ? ? 39 1D ? ? ? ? 89 15",
        "A1 ? ? ? ? 85 C0 74 ? ? ? 83 F8 ? 74 ? 83 F8 ? 75 ? 8A 43",
        "83 3D ? ? ? ? 06 ? ? A1",
        "A1 ? ? ? ? 33 C9 3B D3",
        "E8 ? ? ? ? 5B 5F C3 C1 E6",
        "E8 ? ? ? ? 83 C4 ? C2 ? ? ? ? C2",
        "A1 ? ? ? ? 85 C0 74 ? BB",
        "E8 ? ? ? ? 3B C6 74 ? C7 40",
        "E8 ? ? ? ? 85 C0 75 ? A1 ? ? ? ? 85 C0 74 ? 8B 40 ? 56 8B CF E8 ? ? ? ? 5E",
        "E8 ? ? ? ? 8A 43 ? 84 C0 74 ? E8",
        "E8 ? ? ? ? 8B 75 ? 3B F3 74 ? 81 4E",
        "8B 44 24 ? 8B 4C 24 ? ? ? 50 8B 44 24 ? 51 50",
        "BE ? ? ? ? ? ? ? ? 50 FF 91",
        "A1 ? ? ? ? ? ? 83 C4 ? 50",
    };
    for (auto required : RequiredPatterns)
    {
        if (hook::pattern(required).empty())
            return false;
    }
    return true;
}

void Init()
{
    NFSCursor::Initialize();
    if (!IsSupportedExe())
    {
        MessageBoxW(nullptr, L"This version of Speed.exe isn't supported by the widescreen fix. Please use version 1.4.\n\nThe game will start without the fix.",
            L"NFSUnderground.WidescreenFix", MB_ICONWARNING | MB_OK);
        return;
    }

    auto pattern = hook::pattern("A1 ? ? ? ? 68 ? ? ? ? 50 FF 15");
    hWnd.SetAddress(*pattern.get_first<HWND*>(1));
    WindowRect.SetAddress(*pattern.get_first<tagRECT*>(6));

    pattern = hook::pattern("A1 ? ? ? ? 85 C0 74 ? A1 ? ? ? ? 68");
    dwWindowedMode.SetAddress(*pattern.get_first<uint32_t*>(1));

    pattern = hook::pattern("A1 ? ? ? ? 8B 0C 85 ? ? ? ? 85 C9");
    g_RacingResolution.SetAddress(*pattern.get_first<int*>(1));

    pattern = hook::pattern("A3 ? ? ? ? 39 1D ? ? ? ? 89 15");
    static auto MainLoopHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        MainLoop();
    });

    pattern = hook::pattern("A1 ? ? ? ? 85 C0 74 ? ? ? 83 F8 ? 74 ? 83 F8 ? 75 ? 8A 43");
    GameFlowManager::pRaceCoordinator = *pattern.get_first<int**>(1);

    pattern = hook::pattern("83 3D ? ? ? ? 06 ? ? A1");
    nGameState.SetAddress(*pattern.get_first<int32_t*>(2));

    pattern = hook::pattern("A1 ? ? ? ? 33 C9 3B D3");
    cFEng::pInstance = *pattern.get_first<void**>(1);

    pattern = hook::pattern("E8 ? ? ? ? 5B 5F C3 C1 E6");
    cFEng::MakeLoadedPackagesDirty = (decltype(cFEng::MakeLoadedPackagesDirty))injector::GetBranchDestination(pattern.get_first(0)).as_int();

    pattern = hook::pattern("E8 ? ? ? ? 83 C4 ? C2 ? ? ? ? C2");
    cFEng::PopPackage = (decltype(cFEng::PopPackage))injector::GetBranchDestination(pattern.get_first(0)).as_int();

    pattern = hook::pattern("A1 ? ? ? ? 85 C0 74 ? BB");
    pCurrentMoviePlayer.SetAddress(*pattern.get_first<void**>(1));

    pattern = hook::pattern("E8 ? ? ? ? 3B C6 74 ? C7 40");
    FEPkgMgr_FindPackage = (decltype(FEPkgMgr_FindPackage))injector::GetBranchDestination(pattern.get_first(0)).as_int();

    pattern = hook::pattern("E8 ? ? ? ? 85 C0 75 ? A1 ? ? ? ? 85 C0 74 ? 8B 40 ? 56 8B CF E8 ? ? ? ? 5E");
    FEngFindObject = (decltype(FEngFindObject))injector::GetBranchDestination(pattern.get_first(0)).as_int();

    pattern = hook::pattern("E8 ? ? ? ? 8A 43 ? 84 C0 74 ? E8");
    FEngSetInvisible = (decltype(FEngSetInvisible))injector::GetBranchDestination(pattern.get_first(0)).as_int();

    pattern = hook::pattern("E8 ? ? ? ? 8B 75 ? 3B F3 74 ? 81 4E");
    FEHashUpper = (decltype(FEHashUpper))injector::GetBranchDestination(pattern.get_first(0)).as_int();

    // EAXSound::Update(RealTimeElapsed) in MainLoop
    pattern = hook::pattern("52 E8 ? ? ? ? 8B 2D ? ? ? ? BF");
    RealTimeElapsed.SetAddress(*pattern.get_first<float*>(8));

    pattern = hook::pattern("8B 44 24 ? 8B 4C 24 ? ? ? 50 8B 44 24 ? 51 50");
    static auto FEPkgMgr_SendMessageToPackageHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        onMenuScreenBaseNotify().executeAll((MenuScreen*)regs.esi);
    });

    pattern = hook::pattern("BE ? ? ? ? ? ? ? ? 50 FF 91");
    static auto BeforeResetHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        WFP::onBeforeReset().executeAll();
    });

    pattern = hook::pattern("A1 ? ? ? ? ? ? 83 C4 ? 50");
    Direct3DDevice.SetAddress(*pattern.get_first<IDirect3DDevice9**>(1));
    static auto BeforeEndSceneHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        WFP::onEndScene().executeAll();
    });

    WFP::onInitEvent().executeAll();

    // Serve the initialized settings to editors while the game runs.
    asi_settings::LiveLink::Start();

    static auto futures = WFP::onInitEventAsync().executeAllAsync();

    WFP::onGameInitEvent() += []()
    {
        for (auto& f : futures.get())
            f.wait();
        futures.get().clear();
    };
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("68 20 03 00 00 BE 58 02 00 00"));
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
