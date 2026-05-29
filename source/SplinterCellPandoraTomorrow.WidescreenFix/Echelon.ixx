module;

#include <stdafx.h>

export module Echelon;

import ComVars;

// APlayerController packed save/load flags at +0x4CC
namespace APlayerControllerSaveFlags
{
    constexpr unsigned kOffset        = 0x4CC;

    // HUD quickload/quicksave indicators
    constexpr uint32_t kBQuickLoading = 0x04u;
    constexpr uint32_t kBQuickSaving  = 0x08u;
}

namespace AEPlayerControllerCheckpoint
{
    // Delay SAVEGAME a couple of ticks so the QUICKSAVING HUD frame is rendered
    // before the synchronous save stalls the game loop
    constexpr int kSaveDelayFrames = 2;
    int  saveDelayRemaining = 0;
    void* pendingSavePC     = nullptr;

    // Rewrites QUICKSAVING -> CHECKPOINT during checkpoint saves only
    bool bCheckpointMode = false;

    // Defer LOADGAME so the QUICKLOADING HUD frame renders before the synchronous load stalls the game loop
    int  loadDelayRemaining = 0;
    void* pendingLoadEngine = nullptr;
    void* pendingLoadPC     = nullptr;
    std::wstring pendingLoadCmd;

    // Deferred save/load functions
    void RunDeferredSave(void* playerController);
    void RunDeferredLoad();
}

namespace AEPlayerController
{
    SafetyHookInline shTick = {};
    void __fastcall Tick(void* playerController, void* edx, int a2, int a3)
    {
        if (playerController)
        {
            auto ptr = *((uintptr_t*)playerController + 3);
            if (ptr)
            {
                auto ptr2 = *(uintptr_t*)(ptr + 0x1C);
                if (ptr2)
                {
                    auto StateID = *(int*)(ptr2 + 0x20);

                    wchar_t buffer[256];
                    auto objName = std::wstring_view(UObject::GetFullName(playerController, edx, buffer));

                    size_t spacePos = objName.find(L' ');
                    std::wstring type = (spacePos != std::wstring::npos) ? std::wstring(objName.substr(0, spacePos)) : std::wstring(objName);

                    auto svStateName = std::wstring_view(UObject::GetFullName(UObject::FindState(playerController, edx, StateID), edx, buffer));
                    size_t lastDot = svStateName.rfind(L'.');
                    std::wstring stateName = (lastDot != std::wstring::npos) ? std::wstring(svStateName.substr(lastDot + 1)) : std::wstring(svStateName);
                    UObject::objectStates[type] = stateName;
                }
            }

            // Checkpoints
            // Run deferred SAVEGAME after a short delay so the QUICKSAVING HUD frame
            // renders before the synchronous save stalls the game loop
            if (AEPlayerControllerCheckpoint::saveDelayRemaining > 0
                && AEPlayerControllerCheckpoint::pendingSavePC == playerController)
            {
                if (--AEPlayerControllerCheckpoint::saveDelayRemaining == 0)
                {
                    void* pc = AEPlayerControllerCheckpoint::pendingSavePC;
                    AEPlayerControllerCheckpoint::pendingSavePC = nullptr;
                    AEPlayerControllerCheckpoint::RunDeferredSave(pc);
                }
            }

            // Run deferred LOADGAME after a short delay so the QUICKLOADING HUD frame
            // renders before the synchronous load stalls the game loop
            if (AEPlayerControllerCheckpoint::loadDelayRemaining > 0
                && AEPlayerControllerCheckpoint::pendingLoadPC == playerController)
            {
                if (--AEPlayerControllerCheckpoint::loadDelayRemaining == 0)
                    AEPlayerControllerCheckpoint::RunDeferredLoad();
            }
        }

        return shTick.unsafe_fastcall(playerController, edx, a2, a3);
    }

    // LedgeGrab FPS fix - player would sometimes fail to grab a ledge while falling at high fps
    constexpr float kLedgeStep = 1.0f / 30.0f;
    constexpr uintptr_t kLedgeLevelOff     = 0xb8;
    constexpr uintptr_t kLedgeDeltaTimeOff = 0x764;

    SafetyHookInline shCheckLedgeGrabArms = {};
    int __fastcall CheckLedgeGrabArms(void* pc, void* edx, void* p1, void* p2, void* p3, void* p4, float p5)
    {
        if (pc)
        {
            const auto level = *reinterpret_cast<uintptr_t*>(reinterpret_cast<uintptr_t>(pc) + kLedgeLevelOff);
            if (level)
            {
                float& levelDt = *reinterpret_cast<float*>(level + kLedgeDeltaTimeOff);
                const float realDt = levelDt;
                if (realDt > 0.0f && realDt < kLedgeStep)
                {
                    levelDt = kLedgeStep;
                    const int r = shCheckLedgeGrabArms.unsafe_fastcall<int>(pc, edx, p1, p2, p3, p4, p5);
                    levelDt = realDt;
                    return r;
                }
            }
        }
        return shCheckLedgeGrabArms.unsafe_fastcall<int>(pc, edx, p1, p2, p3, p4, p5);
    }
}

// Restore PC checkpoint saves by hooking EPlayerController::InitialSave()
// and issuing SAVEGAME through ConsoleCommand.
namespace AEPlayerControllerCheckpoint
{
    // Core.dll imports
    void(__fastcall* FName_ctor)(int* /*FName out*/, void* /*edx*/, const wchar_t* Name, int FindType) = nullptr;
    void* (__fastcall* FindFunction)(void* /*UObject*/, void* /*edx*/, int /*FName*/, int) = nullptr;
    void(__fastcall* ProcessEvent)(void* /*UObject*/, void* /*edx*/, void* /*UFunction*/, void* /*Parms*/, void* /*Result*/) = nullptr;

    // Core::GNull FOutputDevice
    void** gNullSlot = nullptr;

    // Cached FName indices
    int initialSaveNameIdx    = 0;
    int consoleCommandNameIdx = 0;

    // Cached ConsoleCommand UFunction
    void* consoleCommandFn = nullptr;

    SafetyHookInline shProcessInternal = {};
    SafetyHookInline shLocalize        = {};
    SafetyHookInline shUGameEngineExec = {};

    // FFrame::Node and UObject::Name offsets
    static constexpr unsigned kFFrameNodeOffset = 0x04;
    static constexpr unsigned kUObjectNameOffset = 0x20;

    // ConsoleCommand(string Command) parameter struct
    struct ConsoleCmdParms
    {
        // FString Command
        wchar_t* Data;
        int ArrayNum;
        int ArrayMax;

        // Extra space for return/padding differences across builds
        uint8_t pad[32];
    };

    constexpr int kCheckpointSlots = 3;

    // Find the most recently used Save\<Profile>\ directory.
    // Returns empty if no save folder exists.
    std::filesystem::path FindActiveProfileFolder()
    {
        try
        {
            const auto saveRoot = GetExeModulePath() / L".." / L"Save";
            if (!std::filesystem::is_directory(saveRoot))
                return {};

            std::filesystem::path best;
            std::filesystem::file_time_type bestTime{};
            bool haveBest = false;

            for (const auto& entry : std::filesystem::directory_iterator(saveRoot))
            {
                if (!entry.is_directory())
                    continue;

                const auto t = entry.last_write_time();
                if (!haveBest || t > bestTime)
                {
                    best     = entry.path();
                    bestTime = t;
                    haveBest = true;
                }
            }
            return best;
        }
        catch (...)
        {
            return {};
        }
    }

    // Pick the next checkpoint slot, preferring empty slots before overwriting the oldest save
    std::wstring PickCheckpointSlot(const wchar_t* baseName)
    {
        const std::wstring base = baseName;
        const auto folder = FindActiveProfileFolder();
        if (folder.empty())
            return base + L"1";

        bool     exists [kCheckpointSlots] = {};
        FILETIME mtimes [kCheckpointSlots] = {};

        for (int i = 0; i < kCheckpointSlots; ++i)
        {
            const std::wstring file = base + std::to_wstring(i + 1) + L".SAV";
            const auto full = folder / file;

            WIN32_FILE_ATTRIBUTE_DATA fad{};
            if (GetFileAttributesExW(full.c_str(), GetFileExInfoStandard, &fad)
                && !(fad.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                exists[i] = true;
                mtimes[i] = fad.ftLastWriteTime;
            }
        }

        // First empty slot wins
        for (int i = 0; i < kCheckpointSlots; ++i)
            if (!exists[i])
                return base + std::to_wstring(i + 1);

        // All filled - overwrite the oldest slot
        int oldest = 0;
        for (int i = 1; i < kCheckpointSlots; ++i)
            if (CompareFileTime(&mtimes[i], &mtimes[oldest]) < 0)
                oldest = i;
        return base + std::to_wstring(oldest + 1);
    }

    // Resolve the localized checkpoint save name
    std::wstring GetLocalizedCheckpointName()
    {
        if (shLocalize)
        {
            const wchar_t* localized = shLocalize.unsafe_ccall<const wchar_t*>(
                L"HUD", L"CheckpointName", L"Localization\\HUD", nullptr, 0);
            if (localized && localized[0] != L'\0'
                && wcscmp(localized, L"CheckpointName") != 0)
            {
                return localized;
            }
        }
        return L"CHECKPOINT";
    }

    // True for bare "LOADGAME" commands (the F8 quickload path)
    // Commands with explicit FILENAME= are left untouched
    bool IsBareLoadGameCommand(const wchar_t* cmd)
    {
        if (!cmd)
            return false;

        while (*cmd == L' ' || *cmd == L'\t')
            ++cmd;

        static constexpr wchar_t kVerb[] = L"LOADGAME";
        for (int i = 0; i < 8; ++i)
        {
            if (cmd[i] == 0 || towupper(cmd[i]) != kVerb[i])
                return false;
        }

        const wchar_t after = cmd[8];
        if (after != 0 && after != L' ' && after != L'\t')
            return false;

        // Ignore explicit LOADGAME FILENAME=... requests
        for (const wchar_t* p = cmd + 8; *p; ++p)
        {
            if (towupper(p[0]) == L'F' && towupper(p[1]) == L'I'
                && towupper(p[2]) == L'L' && towupper(p[3]) == L'E'
                && towupper(p[4]) == L'N' && towupper(p[5]) == L'A'
                && towupper(p[6]) == L'M' && towupper(p[7]) == L'E'
                && p[8] == L'=')
            {
                return false;
            }
        }

        return true;
    }

    // Return the most recently modified QUICKSAVE/CHECKPOINT slot
    std::wstring FindMostRecentQuickloadTarget()
    {
        const auto folder = FindActiveProfileFolder();
        if (folder.empty())
            return {};

        const std::wstring base = GetLocalizedCheckpointName();

        std::wstring bestName;
        FILETIME     bestTime{};
        bool         haveBest = false;

        const auto tryName = [&](const std::wstring& name)
        {
            const auto full = folder / (name + L".SAV");
            WIN32_FILE_ATTRIBUTE_DATA fad{};
            if (GetFileAttributesExW(full.c_str(), GetFileExInfoStandard, &fad)
                && !(fad.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                if (!haveBest || CompareFileTime(&fad.ftLastWriteTime, &bestTime) > 0)
                {
                    bestName = name;
                    bestTime = fad.ftLastWriteTime;
                    haveBest = true;
                }
            }
        };

        // Engine quicksave slots (not localized in SCPT)
        tryName(L"QUICKSAVE");
        //tryName(L"TEMPSAVE");

        for (int i = 0; i < kCheckpointSlots; ++i)
            tryName(base + std::to_wstring(i + 1));

        return bestName;
    }

    // Engine.Client.Viewports[0].Actor -> active APlayerController
    void* GetActivePlayerController(void* engine)
    {
        if (!engine)
            return nullptr;
        void* client = *(void**)((char*)engine + 0x1C);
        if (!client)
            return nullptr;
        void** viewportsData = *(void***)((char*)client + 0x30);
        if (!viewportsData)
            return nullptr;
        void* viewport = viewportsData[0];
        if (!viewport)
            return nullptr;
        return *(void**)((char*)viewport + 0x34);
    }

    // Execute deferred LOADGAME command (queued by UGameEngineExecHook)
    void RunDeferredLoad()
    {
        if (!pendingLoadEngine || pendingLoadCmd.empty())
            return;

        void* out = (gNullSlot ? *gNullSlot : nullptr);

        shUGameEngineExec.unsafe_fastcall<int>(
            pendingLoadEngine, nullptr, pendingLoadCmd.c_str(), out);

        pendingLoadEngine = nullptr;
        pendingLoadPC     = nullptr;
        pendingLoadCmd.clear();
    }

    // Hook UGameEngine::Exec
    // Rewrites bare LOADGAME (F8) to load the most recent checkpoint/quicksave
    // instead of defaulting to QUICKSAVE
    //
    // Also forces bQuickLoading for HUD feedback and defers execution so the
    // loading overlay renders before the synchronous load stalls the game loop
    int __fastcall UGameEngineExecHook(void* self, void* edx, const wchar_t* cmd, void* out)
    {
        if (bEnableCheckpoints && IsBareLoadGameCommand(cmd))
        {
            const std::wstring target = FindMostRecentQuickloadTarget();

            if (!target.empty() && target != L"QUICKSAVE")
            {
                void* pc = GetActivePlayerController(self);
                if (pc)
                {
                    // Flip bQuickLoading so EchelonMainHUD picks up the
                    // box on the very next render
                    uint32_t* flags = reinterpret_cast<uint32_t*>(
                        reinterpret_cast<char*>(pc) + APlayerControllerSaveFlags::kOffset);
                    *flags |= APlayerControllerSaveFlags::kBQuickLoading;

                    pendingLoadEngine = self;
                    pendingLoadPC     = pc;
                    pendingLoadCmd    = L"Loadgame FILENAME=";
                    pendingLoadCmd   += target;
                    loadDelayRemaining = kSaveDelayFrames;

                    // Report success, actual load is executed later
                    // from AEPlayerController::Tick when the delay expires
                    return 1;
                }

                // Fallback: execute rewritten LOADGAME immediately
                wchar_t modified[200];
                swprintf_s(modified, L"Loadgame FILENAME=%s", target.c_str());
                return shUGameEngineExec.unsafe_fastcall<int>(self, edx, modified, out);
            }
        }

        return shUGameEngineExec.unsafe_fastcall<int>(self, edx, cmd, out);
    }

    // Core::Localize hook
    // Rewrites QUICKSAVING -> CHECKPOINT during checkpoint saves so HUD uses
    // checkpoint-specific string; F5 quicksaves are unaffected
    const wchar_t* __cdecl LocalizeHook(const wchar_t* Section, const wchar_t* Key,
                                        const wchar_t* Package, const wchar_t* LangExt, int Optional)
    {
        if (bCheckpointMode && Key && wcscmp(Key, L"QUICKSAVING") == 0)
            Key = L"CHECKPOINT";
        return shLocalize.unsafe_ccall<const wchar_t*>(Section, Key, Package, LangExt, Optional);
    }

    void RunDeferredSave(void* playerController)
    {
        if (!FindFunction || !ProcessEvent || !playerController)
            return;

        if (!consoleCommandFn && consoleCommandNameIdx != 0)
            consoleCommandFn = FindFunction(playerController, nullptr, consoleCommandNameIdx, 0);

        if (!consoleCommandFn)
            return;

        // Get localized checkpoint base name (HUD.CheckpointName) and select a
        // 1..kCheckpointSlots save slot (first free, otherwise oldest)
        const std::wstring checkpointBase = GetLocalizedCheckpointName();
        const std::wstring slotName       = PickCheckpointSlot(checkpointBase.c_str());

        // Build SAVEGAME command for selected slot
        wchar_t cmdBuf[200];
        swprintf_s(cmdBuf, L"SAVEGAME FILENAME=%s OVERWRITE=TRUE", slotName.c_str());
        const int cmdLen = static_cast<int>(wcslen(cmdBuf)) + 1;

        ConsoleCmdParms parms{};
        parms.Data     = cmdBuf;
        parms.ArrayNum = cmdLen;
        parms.ArrayMax = cmdLen;

        // Execute SAVEGAME command (blocks until save completes)
        ProcessEvent(playerController, nullptr, consoleCommandFn, &parms, nullptr);

        // Clear quicksave HUD flag after save completes
        uint32_t* flags = reinterpret_cast<uint32_t*>(
            reinterpret_cast<char*>(playerController) + APlayerControllerSaveFlags::kOffset);
        *flags &= ~APlayerControllerSaveFlags::kBQuickSaving;

        bCheckpointMode = false;
    }

    void TriggerCheckpointSave(void* playerController)
    {
        if (!FName_ctor || !FindFunction || !ProcessEvent || !playerController)
            return;

        if (consoleCommandNameIdx == 0)
            FName_ctor(&consoleCommandNameIdx, nullptr, L"ConsoleCommand", 1 /* FNAME_Add */);

        if (!consoleCommandFn && consoleCommandNameIdx != 0)
            consoleCommandFn = FindFunction(playerController, nullptr, consoleCommandNameIdx, 0);

        if (!consoleCommandFn)
            return;

        // Enable QUICKSAVING HUD for checkpoint save
        uint32_t* flags = reinterpret_cast<uint32_t*>(
            reinterpret_cast<char*>(playerController) + APlayerControllerSaveFlags::kOffset);
        *flags |= APlayerControllerSaveFlags::kBQuickSaving;

        bCheckpointMode = true;

        // Defer SAVEGAME so HUD renders before synchronous freeze
        saveDelayRemaining = kSaveDelayFrames;
        pendingSavePC      = playerController;
    }

    void __fastcall ProcessInternalHook(void* self, void* edx, void* stack, void* result)
    {
        // Lazy-init InitialSave FName (wait for script package to register it)
        if (initialSaveNameIdx == 0 && FName_ctor)
            FName_ctor(&initialSaveNameIdx, nullptr, L"InitialSave", 1 /* FNAME_Add */);

        if (initialSaveNameIdx != 0 && stack)
        {
            void* node = *reinterpret_cast<void**>(
                reinterpret_cast<char*>(stack) + kFFrameNodeOffset);
            if (node)
            {
                const int nameIdx = *reinterpret_cast<int*>(
                    reinterpret_cast<char*>(node) + kUObjectNameOffset);
                if (nameIdx == initialSaveNameIdx)
                {
                    TriggerCheckpointSave(self);
                    // Continue original flow for internal state updates
                }
            }
        }

        shProcessInternal.unsafe_fastcall(self, edx, stack, result);
    }

    void Init()
    {
        FName_ctor    = decltype(FName_ctor)(GetProcAddress(GetModuleHandle(L"Core"), "??0FName@@QAE@PB_WW4EFindName@@@Z"));
        FindFunction  = decltype(FindFunction)(GetProcAddress(GetModuleHandle(L"Core"), "?FindFunction@UObject@@QAEPAVUFunction@@VFName@@H@Z"));
        ProcessEvent  = decltype(ProcessEvent)(GetProcAddress(GetModuleHandle(L"Core"), "?ProcessEvent@UObject@@UAEXPAVUFunction@@PAX1@Z"));
        gNullSlot     = (void**)GetProcAddress(GetModuleHandle(L"Core"), "?GNull@@3PAVFOutputDevice@@A");

        if (FName_ctor)
        {
            FName_ctor(&initialSaveNameIdx,    nullptr, L"InitialSave",    1 /* FNAME_Add */);
            FName_ctor(&consoleCommandNameIdx, nullptr, L"ConsoleCommand", 1 /* FNAME_Add */);
        }

        if (auto pProcessInternal = GetProcAddress(GetModuleHandle(L"Core"), "?ProcessInternal@UObject@@QAEXAAUFFrame@@QAX@Z"))
            shProcessInternal = safetyhook::create_inline(pProcessInternal, ProcessInternalHook);

        if (auto pLocalize = GetProcAddress(GetModuleHandle(L"Core"), "?Localize@@YAPB_WPB_W000H@Z"))
            shLocalize = safetyhook::create_inline(pLocalize, LocalizeHook);

        if (auto pExec = GetProcAddress(GetModuleHandle(L"Engine"), "?Exec@UGameEngine@@UAEHPB_WAAVFOutputDevice@@@Z"))
            shUGameEngineExec = safetyhook::create_inline(pExec, UGameEngineExecHook);
    }
}

namespace AETextureManager
{
    injector::hook_back<void(__fastcall*)(void*, void*, void*, int, float, float, float, float, float, float)> hbDrawTileFromManager = {};
    void __fastcall DrawTileFromManager(void* AETextureManager, void* edx, void* UECanvas, int a3, float width, float height, float x, float y, float a8, float a9)
    {
        if (Screen.nCutsceneBorders != 0 && (int)x == 0 && (int)width == 640 && (int)height == 60 && ((int)y == 0 || int(y + height) == 480))
        {
            if (UObject::GetState(L"EchelonMainHUD") == L"s_Cinematic")
            {
                if (Screen.nCutsceneBorders == 1)
                {
                    x = 0.0f;
                    y = 0.0f;
                    width = 0.0f;
                    height = 0.0f;
                }
                else if (Screen.nCutsceneBorders == 2)
                {
                    const float newBorderHeight = (480.0f - 270.0f * Screen.fAspectRatio) / 2.0f;
                    if (newBorderHeight <= 0)
                    {
                        x = 0.0f;
                        y = 0.0f;
                        width = 0.0f;
                        height = 0.0f;
                    }
                    else
                    {
                        if (y != 0)
                            y = 480.0f - newBorderHeight;
                        height = newBorderHeight;
                    }
                }
            }
        }

        return hbDrawTileFromManager.fun(AETextureManager, edx, UECanvas, a3, width, height, x, y, a8, a9);
    }
}

export void InitEchelon()
{
    // EPlayerController additional state cache
    AEPlayerController::shTick = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"Echelon"), "?Tick@AEPlayerController@@UAEHMW4ELevelTick@@@Z"), AEPlayerController::Tick);

    // LedgeGrab FPS fix
    AEPlayerController::shCheckLedgeGrabArms = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"Echelon"), "?CheckLedgeGrabArms@AEPlayerController@@QAEHPBVGEObject@@0AAVFVector@@1M@Z"), AEPlayerController::CheckLedgeGrabArms);

    // Restores the checkpoints from the Xbox version of the game
    if (bEnableCheckpoints)
        AEPlayerControllerCheckpoint::Init();

    // Letterboxing
    auto pattern = find_module_pattern(GetModuleHandle(L"Echelon"), "D9 1C 24 51 50 8B CE E8 ? ? ? ? 5E", "FF B6 ? ? ? ? 57 E8 ? ? ? ? 5F");
    AETextureManager::hbDrawTileFromManager.fun = injector::MakeCALL(pattern.get_first(7), AETextureManager::DrawTileFromManager, true).get();

    // Hud Helpers
    pattern = find_module_pattern(GetModuleHandle(L"Echelon"), "85 C0 7C ? 8B 89 6C 03 00 00", "85 C0 78 ? F3 0F 10 45 24");
    static auto AETextureManagerDrawTileFromManagerHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        curDrawTileManagerTextureIndex = regs.eax;
    });

    pattern = find_module_pattern(GetModuleHandle(L"Echelon"), "85 C0 7C ? 8B 4C 24 08", "85 C0 78 ? F3 0F 10 45 FC");
    static auto AETextureManagerexecDrawTileFromManagerHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        curDrawTileManagerTextureIndex = regs.eax;
    });

    // set player speed to max on game start
    UIntOverrides::Register(L"IntProperty Echelon.EchelonGameInfo.m_defautSpeed", +[]() -> int
    {
        return 5;
    });

    // Camera acceleration
    pattern = find_module_pattern(GetModuleHandle(L"Echelon"), "0F 8A ? ? ? ? F3 0F 10 81 ? ? ? ? 0F 2E C1 9F F6 C4 ? 0F 8A ? ? ? ? F3 0F 10 89");
    if (!pattern.empty())
        injector::WriteMemory<uint16_t>(pattern.get_first(0), 0xE990, true); // jp -> jmp
    else
    {
        pattern = find_module_pattern(GetModuleHandle(L"Echelon"), "7A ? D9 05 ? ? ? ? D9 81 ? ? ? ? DA E9 DF E0 F6 C4 ? 7A");
        injector::WriteMemory<uint8_t>(pattern.get_first(0), 0xEB, true); // jp -> jmp
    }

    // Aiming camera smoothing
    pattern = find_module_pattern(GetModuleHandle(L"Echelon"), "0F 84 ? ? ? ? 8B 86 ? ? ? ? DD D8", "0F 84 ? ? ? ? 8B 87 ? ? ? ? 83 EC ? F3 0F 10 80");
    injector::WriteMemory<uint16_t>(pattern.get_first(0), 0xE990, true); // jz -> jmp
}