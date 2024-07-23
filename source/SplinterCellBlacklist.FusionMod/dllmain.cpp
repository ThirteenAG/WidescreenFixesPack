#include "stdafx.h"
#include <LEDEffects.h>
#include <random>

HWND WindowHandle;

enum eGameMode
{
    CAMPAIGN,
    PALADIN,
    HUNTER,
    GHOST,
    EXTRACTION,
    COOP,
};

int CurrentGameMode = -1;
SafetyHookInline shLead_SetCurrentGameMode{};
void __cdecl Lead_SetCurrentGameMode(int gameMode, int a2)
{
    CurrentGameMode = gameMode;
    return shLead_SetCurrentGameMode.ccall(gameMode, a2);
}

std::string sExtractionWaveConfigs = "Default";
int nExtractionWaveEnemyMultiplier = 1;
int nExtractionWaveEnemyRandomRangeMin = 0;
int nExtractionWaveEnemyRandomRangeMax = 4;
int GetRandomInt(int range_begin, int range_end)
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(range_begin, range_end);
    return dis(gen);
}

namespace ExtractionSubWaveEnemy
{
    int curWaveEnemyCount = 0;
    int curStartConditionType = 0;
    injector::hook_back<int(__cdecl*)(const char*)> hbappAtoi;
    int __cdecl appAtoi(const char* String)
    {
        auto i = hbappAtoi.fun(String);
        if (iequals(sExtractionWaveConfigs, "Random"))
            i *= GetRandomInt(nExtractionWaveEnemyRandomRangeMin, nExtractionWaveEnemyRandomRangeMax);
        else
            i *= nExtractionWaveEnemyMultiplier;
        curWaveEnemyCount += i;
        return i;
    }
}

bool (WINAPI* GetOverloadedFilePathA)(const char* lpFilename, char* out, size_t out_size) = nullptr;
namespace FFileManagerArc
{
    SafetyHookInline shLookup{};
    void* __fastcall Lookup(void* pFFileManagerArc, void* edx, const char* path)
    {
        //std::string s(MAX_PATH, '\0');
        if (GetOverloadedFilePathA(path, nullptr/*s.data()*/, 0/*s.size()*/))
            return nullptr;
        return shLookup.fastcall<void*>(pFFileManagerArc, edx, path);
    }
}

namespace l3d
{
    using LeadOptions = float*;
    LeadOptions* (*GetResource)() = nullptr;
    LeadOptions Lead_GetOptions(LeadOptions* lead)
    {
        return lead[2];
    }
}

float fScreenCullBias = 0.0f;
float fFOVDiff = 1.0f;
SafetyHookInline shTriggerScreenCullBias{};
void __cdecl TriggerScreenCullBias(char bStickyCamState)
{
    auto options = l3d::Lead_GetOptions(l3d::GetResource());

    // original code
    //if (bStickyCamState)
    //    options[130] = 0.3f;
    //else
    //    options[130] = 1.0f;

    if (!fScreenCullBias) // disable culling
        options[130] = 0.0001f;
    else
        options[130] = fScreenCullBias / fFOVDiff;
}

namespace UI
{
    namespace ScenePause
    {
        void* pUI = nullptr;
        void* pUnk = nullptr;
        SafetyHookInline shOnRadarChanged{};
        uint8_t __fastcall OnRadarChanged(void* ui, void* edx, void* a2, int value) // uint8_t UI::ScenePause::OnRadarChanged(Slot<class Z1 = unsigned int>*, unsigned int)
        {
            pUI = ui;
            pUnk = a2;
            return shOnRadarChanged.fastcall<uint8_t>(ui, edx, a2, value);
        }
    }
}

namespace HudRadar
{
    char (*HudRadarEnabled)() = nullptr;
}

namespace UInputManager
{
    bool bIsCoverKeyPressed = false;

    enum eInputAction
    {
        eCover = 0x6,
    };
}

void Init()
{
    CIniReader iniReader("");
    static auto bSkipIntro = iniReader.ReadInteger("MAIN", "SkipIntro", 1) != 0;
    static auto bSkipPressAnyKeyScreen = iniReader.ReadInteger("MAIN", "SkipPressAnyKeyScreen", 1) != 0;
    static auto bFullscreenAtStartup = iniReader.ReadInteger("MAIN", "FullscreenAtStartup", 1) != 0;
    auto bDisableNegativeMouseAcceleration = iniReader.ReadInteger("MAIN", "DisableNegativeMouseAcceleration", 1) != 0;
    auto bUltraWideSupport = iniReader.ReadInteger("MAIN", "UltraWideSupport", 1) != 0;
    static auto fFOVFactor = std::clamp(iniReader.ReadFloat("MAIN", "FOVFactor", 1.0f), 0.5f, 2.5f);
    fScreenCullBias = std::clamp(iniReader.ReadFloat("MAIN", "ScreenCullBias", 0.0f), 0.0f, 1.0f);
    static float fShadowCullDist = std::clamp(iniReader.ReadFloat("MAIN", "ShadowCullDist", 100.0f), 90.0f, 120.0f);
    auto bDisableNightVisionFlash = iniReader.ReadInteger("MAIN", "DisableNightVisionFlash", 1) != 0;

    sExtractionWaveConfigs = iniReader.ReadString("EXTRACTION", "ExtractionWaveConfigs", "Default");
    nExtractionWaveEnemyMultiplier = std::clamp(iniReader.ReadInteger("EXTRACTION", "ExtractionWaveEnemyMultiplier", 1), 1, 9999);
    nExtractionWaveEnemyRandomRangeMin = std::clamp(iniReader.ReadInteger("EXTRACTION", "ExtractionWaveEnemyRandomRangeMin", 0), 0, 9999);
    nExtractionWaveEnemyRandomRangeMax = std::clamp(iniReader.ReadInteger("EXTRACTION", "ExtractionWaveEnemyRandomRangeMax", 4), 1, 9999);

    static auto sHUNTERReinforcementsNumber = iniReader.ReadString("HUNTER", "ReinforcementsNumber", "Default");
    static auto nHUNTERReinforcementsEnemyMultiplier = std::clamp(iniReader.ReadInteger("HUNTER", "ReinforcementsEnemyMultiplier", 1), 1, 9999);
    static auto nHUNTERReinforcementsEnemyRandomRangeMin = std::clamp(iniReader.ReadInteger("HUNTER", "ReinforcementsEnemyRandomRangeMin", 1), 1, 9999);
    static auto nHUNTERReinforcementsEnemyRandomRangeMax = std::clamp(iniReader.ReadInteger("HUNTER", "ReinforcementsEnemyRandomRangeMax", 1), 1, 9999);

    auto bUnlockDLC = iniReader.ReadInteger("UNLOCKS", "UnlockDLC", 1) != 0;
    static auto bUnlockAllNonCampaignMissions = iniReader.ReadInteger("UNLOCKS", "UnlockAllNonCampaignMissions", 1) != 0;
    static auto bUnlockAllCampaignMissions = iniReader.ReadInteger("UNLOCKS", "UnlockAllCampaignMissions", 0) != 0;

    auto bToggleRadarHotkey = iniReader.ReadInteger("RADAR", "ToggleRadarHotkey", 0) != 0;

    auto sDedicatedServerExePath = iniReader.ReadString("STARTUP", "DedicatedServerExePath", "");

    static auto bGHOSTDisableMissionFailOnDetection = iniReader.ReadInteger("GHOST", "DisableMissionFailOnDetection", 1) != 0;

    static auto bCOOPDisableMissionFailOnDetection = iniReader.ReadInteger("COOP", "DisableMissionFailOnDetection", 0) != 0;
    static auto sCOOPReinforcementsNumber = iniReader.ReadString("COOP", "ReinforcementsNumber", "Default");
    static auto nCOOPReinforcementsEnemyMultiplier = std::clamp(iniReader.ReadInteger("COOP", "ReinforcementsEnemyMultiplier", 1), 1, 9999);
    static auto nCOOPReinforcementsEnemyRandomRangeMin = std::clamp(iniReader.ReadInteger("COOP", "ReinforcementsEnemyRandomRangeMin", 1), 1, 9999);
    static auto nCOOPReinforcementsEnemyRandomRangeMax = std::clamp(iniReader.ReadInteger("COOP", "ReinforcementsEnemyRandomRangeMax", 1), 1, 9999);

    static auto bCAMPAIGNDisableMissionFailOnDetection = iniReader.ReadInteger("CAMPAIGN", "DisableMissionFailOnDetection", 0) != 0;
    auto bEnableRunDuringForcedWalk = iniReader.ReadInteger("CAMPAIGN", "EnableRunDuringForcedWalk", 1) != 0;

    if (!sDedicatedServerExePath.empty())
    {
        std::error_code ec;
        auto dsPath = std::filesystem::path(sDedicatedServerExePath);
        auto processPath = dsPath.is_absolute() ? dsPath : (GetExeModulePath() / dsPath);
        auto workingDir = std::filesystem::path(processPath).remove_filename();
        if (std::filesystem::exists(processPath, ec))
        {
            HANDLE hJob = CreateJobObject(nullptr, nullptr);
            JOBOBJECT_EXTENDED_LIMIT_INFORMATION info = { };
            info.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
            SetInformationJobObject(hJob, JobObjectExtendedLimitInformation, &info, sizeof(info));
            STARTUPINFO si = { sizeof(si) };
            PROCESS_INFORMATION pi;
            if (CreateProcessInJob(hJob,
                processPath.c_str(),
                NULL,
                nullptr, nullptr, FALSE, CREATE_NO_WINDOW,
                nullptr, workingDir.c_str(), &si, &pi)) {
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
            }
        }
    }

    //allow loading unpacked files
    {
        ModuleList dlls;
        dlls.Enumerate(ModuleList::SearchLocation::LocalOnly);
        for (auto& e : dlls.m_moduleList)
        {
            auto m = std::get<HMODULE>(e);
            if (IsModuleUAL(m)) {
                GetOverloadedFilePathA = (decltype(GetOverloadedFilePathA))GetProcAddress(m, "GetOverloadedFilePathA");
                break;
            }
        }

        if (GetOverloadedFilePathA)
        {
            auto pattern = hook::pattern("55 8B EC 6A FF 68 ? ? ? ? 64 A1 ? ? ? ? 50 83 EC 1C 53 56 57 A1 ? ? ? ? 33 C5 50 8D 45 F4 64 A3 ? ? ? ? 8B F1 8B 45 08");
            FFileManagerArc::shLookup = safetyhook::create_inline(pattern.get_first(), FFileManagerArc::Lookup);
        }
    }

    //skip systemdetection
    auto pattern = hook::pattern("0F 84 ? ? ? ? 68 ? ? ? ? 89 B5");
    injector::WriteMemory<uint16_t>(pattern.get_first(), 0xE990, true); //jz -> jmp

    //HWND
    pattern = hook::pattern("8B 8E ? ? ? ? 8B 41 04 85 C0");
    static auto GetHWND = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        WindowHandle = (HWND)regs.eax;
    });

    //WindowStyle
    pattern = hook::pattern("83 3D ? ? ? ? ? 0F 85 ? ? ? ? A1 ? ? ? ? 3B 46 08");
    static int* pWindowStyle = *pattern.get_first<int*>(2);

    //Resolution
    static int* pViewportResolutionWidth = nullptr;
    static int* pViewportResolutionHeight = nullptr;

    pattern = hook::pattern("A3 ? ? ? ? 8B 8E ? ? ? ? 89 0D ? ? ? ? 85 DB 74 13 83 BE");
    if (!pattern.empty())
    {
        pViewportResolutionWidth = *pattern.get_first<int*>(1);
        pViewportResolutionHeight = *pattern.get_first<int*>(13);
    }
    else
    {
        pattern = hook::pattern("89 0D ? ? ? ? 8B 96 ? ? ? ? 89 15 ? ? ? ? 85 DB 74 13");
        pViewportResolutionWidth = *pattern.get_first<int*>(2);
        pViewportResolutionHeight = *pattern.get_first<int*>(14);
    }

    //UnrealMain
    pattern = hook::pattern("0F 84 ? ? ? ? 56 56 56 68 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 83 C4 18 50 8D 8D");
    injector::WriteMemory<uint16_t>(pattern.get_first(), 0xE990, true); //jz -> jmp

    //GameMode
    pattern = hook::pattern("55 8B EC 83 3D ? ? ? ? ? 75 55");
    shLead_SetCurrentGameMode = safetyhook::create_inline(pattern.get_first(), Lead_SetCurrentGameMode);

    if (bSkipIntro || bFullscreenAtStartup || bSkipPressAnyKeyScreen)
    {
        enum eWindowStyle
        {
            ExclusiveFullscreen,
            Windowed,
            Borderless
        };

        //InitBootVideos
        if (bSkipIntro)
        {
            auto pattern = hook::pattern("E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? 8B E5");
            injector::MakeNOP(pattern.get_first(5), 5, true);
        }

        static int nOnce = 0;
        pattern = hook::pattern("8B 86 ? ? ? ? 8B 8E ? ? ? ? 89 86 ? ? ? ? 89 86");
        static auto kbd = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            nOnce = 1;
        });

        pattern = hook::pattern("89 9E ? ? ? ? C7 86 ? ? ? ? ? ? ? ? 88 9E ? ? ? ? 88 9E");
        static auto kbd2 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            nOnce = 2;
        });

        std::thread([]()
        {
            bool bOnce = false;
            while (true)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                if (nOnce == 1)
                {
                    if (WindowHandle == GetForegroundWindow())
                    {
                        if (bFullscreenAtStartup)
                        {
                            if ((GetWindowLongA(WindowHandle, GWL_STYLE) & WS_BORDER) != 0 && pWindowStyle && *pWindowStyle == ExclusiveFullscreen)
                            {
                                keybd_event(VK_RMENU, 0x45, KEYEVENTF_EXTENDEDKEY | 0, 0);
                                keybd_event(VK_RETURN, 0x45, KEYEVENTF_EXTENDEDKEY | 0, 0);
                                keybd_event(VK_RETURN, 0x45, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
                                keybd_event(VK_RMENU, 0x45, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
                            }
                        }

                        if (bSkipPressAnyKeyScreen)
                        {
                            if ((GetAsyncKeyState(VK_MENU) & 0xF000) == 0)
                            {
                                keybd_event(VK_RETURN, 0x45, KEYEVENTF_EXTENDEDKEY | 0, 0);
                                keybd_event(VK_RETURN, 0x45, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
                            }
                        }
                    }
                    else
                    {
                        if (!bOnce)
                        {
                            bOnce = true;
                            SetForegroundWindow(WindowHandle);
                        }
                    }
                }
                else if (nOnce == 2)
                    break;
            }
        }).detach();
    }

    if (bDisableNegativeMouseAcceleration)
    {
        auto pattern = hook::pattern("0F 28 CA EB 13 0F 57 ED");
        injector::MakeNOP(pattern.get_first(0), 3, true);

        pattern = hook::pattern("0F 28 C8 F3 0F 10 07");
        injector::MakeNOP(pattern.get_first(0), 3, true);

        pattern = hook::pattern("0F 28 C2 EB 15 0F 57 D2");
        injector::MakeNOP(pattern.get_first(0), 3, true);

        pattern = hook::pattern("0F 28 C3 D9 86");
        injector::MakeNOP(pattern.get_first(0), 3, true);
    }

    {
        pattern = hook::pattern("E8 ? ? ? ? 83 C4 04 89 45 E4 85 C0 0F 84"); // quantity in ExtractionWaveConfigXMLParser::LoadSubWaves
        ExtractionSubWaveEnemy::hbappAtoi.fun = injector::MakeCALL(pattern.get_first(0), ExtractionSubWaveEnemy::appAtoi).get();

        pattern = hook::pattern("E8 ? ? ? ? 83 C4 0C 85 C0 75 0C 8B 45 E8"); // ExtractionWaveConfigXMLParser::LoadSubWaves
        static auto KillCountSoftlockFix = safetyhook::create_mid(pattern.get_first(0), [](SafetyHookContext& regs)
        {
            auto type = std::string_view((const char*)regs.esi);
            int value = regs.edi;
            if (type == "Kills")
            {
                if (ExtractionSubWaveEnemy::curWaveEnemyCount < value)
                {
                    value = ExtractionSubWaveEnemy::curWaveEnemyCount;
                    regs.edi = value;
                    regs.eax = value;
                }

                ExtractionSubWaveEnemy::curStartConditionType = value;
                ExtractionSubWaveEnemy::curWaveEnemyCount = 0;
            }
            else
            {
                ExtractionSubWaveEnemy::curStartConditionType = 0;
                ExtractionSubWaveEnemy::curWaveEnemyCount = 0;
            }
        });
    }

    {
        //AECoopHunterSpawner
        auto pattern = hook::pattern("85 5E 08 75 29 8B 06 8B 50 0C 6A 04 8D 8F ? ? ? ? 51 8B CE FF D2 85 5E 08 75 12 8B 06 8B 50 0C 6A 04 8D 8F ? ? ? ? 51 8B CE FF D2 F6 46 08 02");
        static auto FCheckpointPackReaderHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            if (CurrentGameMode == HUNTER)
            {
                if (iequals(sHUNTERReinforcementsNumber, "Random"))
                    *(uint32_t*)(regs.edi + 0x370) = GetRandomInt(nHUNTERReinforcementsEnemyRandomRangeMin, nHUNTERReinforcementsEnemyRandomRangeMax);
            }
            else if (CurrentGameMode == COOP)
            {
                if (iequals(sHUNTERReinforcementsNumber, "Random"))
                    *(uint32_t*)(regs.edi + 0x370) = GetRandomInt(nCOOPReinforcementsEnemyRandomRangeMin, nCOOPReinforcementsEnemyRandomRangeMax);
            }
        });

        pattern = hook::pattern("8B 16 8B 82 ? ? ? ? 8B CE FF D0 8B 16 8B 82 ? ? ? ? 8B CE FF D0 C6 86");
        static auto AECoopHunterSpawnerHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            if (CurrentGameMode == HUNTER)
            {
                if (iequals(sHUNTERReinforcementsNumber, "Random"))
                    *(uint32_t*)(regs.esi + 0x370) = GetRandomInt(nHUNTERReinforcementsEnemyRandomRangeMin, nHUNTERReinforcementsEnemyRandomRangeMax);
                else
                    *(uint32_t*)(regs.esi + 0x370) *= nHUNTERReinforcementsEnemyMultiplier;
            }
            else if (CurrentGameMode == COOP)
            {
                if (iequals(sCOOPReinforcementsNumber, "Random"))
                    *(uint32_t*)(regs.esi + 0x370) = GetRandomInt(nCOOPReinforcementsEnemyRandomRangeMin, nCOOPReinforcementsEnemyRandomRangeMax);
                else
                    *(uint32_t*)(regs.esi + 0x370) *= nCOOPReinforcementsEnemyMultiplier;
            }
        });
    }

    {
        pattern = hook::pattern("F6 86 ? ? ? ? ? 0F 84 ? ? ? ? 8B 96 ? ? ? ? 52");
        static auto loc_F1D09C = (uintptr_t)pattern.get_first(0);

        pattern = hook::pattern("0F 86 ? ? ? ? 0F B6 8E");
        struct AECooperativeMatchManager__TickSpecial
        {
            void operator()(injector::reg_pack& regs)
            {
                if ((bGHOSTDisableMissionFailOnDetection && CurrentGameMode == GHOST) ||
                (bCOOPDisableMissionFailOnDetection && CurrentGameMode == COOP) || 
                (bCAMPAIGNDisableMissionFailOnDetection && CurrentGameMode == CAMPAIGN))
                    *(uintptr_t*)(regs.esp - 4) = loc_F1D09C;
            }
        }; injector::MakeInline<AECooperativeMatchManager__TickSpecial>(pattern.get_first(0), pattern.get_first(6));
    }

    if (bUnlockDLC)
    {
        //NetOnlineContentRightsManager::GetUnlocksFromSaveGame
        auto pattern = find_pattern("74 04 C6 40 04 01 56");
        injector::MakeNOP(pattern.get_first(), 2, true);
        
        pattern = find_pattern("39 5E 38 74 2C");
        injector::MakeNOP(pattern.get_first(), 5, true);
        static auto SPHelmetSuitC = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            *(uint32_t*)(regs.esi + 0x38) = regs.ebx;
        });
        
        pattern = find_pattern("39 5E 3C 74 27");
        injector::MakeNOP(pattern.get_first(), 5, true);
        static auto SPBootsSuitL = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            *(uint32_t*)(regs.esi + 0x3C) = regs.ebx;
        });
        
        pattern = find_pattern("39 5E 40 74 27");
        injector::MakeNOP(pattern.get_first(), 5, true);
        static auto SPGlovesSuitL = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            *(uint32_t*)(regs.esi + 0x40) = regs.ebx;
        });
    }

    if (bUnlockAllNonCampaignMissions || bUnlockAllCampaignMissions)
    {
        auto pattern = find_pattern("89 86 ? ? ? ? 8B 8E ? ? ? ? 6A 00");
        injector::MakeNOP(pattern.get_first(), 6, true);
        static auto SC6Mission__SerializeProfile = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            auto& MissionEnum = *(uint8_t*)(regs.esi + 0xA4);
            auto& MissionStatusEnum = *(uint32_t*)(regs.esi + 0xB4);
            MissionStatusEnum = regs.eax;
            
            if (!MissionStatusEnum)
            {
                if (bUnlockAllNonCampaignMissions && MissionEnum >= 13 && MissionEnum <= 16) // briggs
                    MissionStatusEnum = 1;
                if (bUnlockAllNonCampaignMissions && MissionEnum >= 13 && MissionEnum <= 28) // grim
                    MissionStatusEnum = 1;

                if (bUnlockAllCampaignMissions)
                {
                    if (MissionEnum && MissionEnum <= 12)
                    {
                        MissionStatusEnum = 1;
                    }

                    if (MissionEnum == 21 || MissionEnum == 37 || MissionEnum == 38)
                    {
                        MissionStatusEnum = 1;
                    }
                }
            }
        });
    }

    if (!sExtractionWaveConfigs.empty() && !iequals(sExtractionWaveConfigs, "Default"))
    {
        std::error_code ec;
        std::string prefix = GetOverloadedFilePathA ? "..\\..\\" : ".\\update\\";
        std::filesystem::path xmlPathDefault = prefix + std::string("Data\\ExtractionWaveConfigs\\") + sExtractionWaveConfigs + "\\DefaultWaveConfig.xml";
        std::filesystem::path xmlPath = prefix + std::string("Data\\ExtractionWaveConfigs\\") + sExtractionWaveConfigs + "\\%s.xml";
        auto p = GetExeModulePath() / std::filesystem::path(xmlPath).remove_filename();

        if (std::filesystem::exists(p / "D_Amman.xml", ec) && std::filesystem::exists(p / "D_Bratislava.xml", ec) && std::filesystem::exists(p / "D_Kigali.xml", ec) && std::filesystem::exists(p / "D_Sanaa.xml", ec))
        {
            static std::string s = xmlPath.string();
            auto pattern = find_pattern("68 ? ? ? ? E8 ? ? ? ? 83 C4 08 50 8D 4D D8 E8 ? ? ? ? BB");
            injector::WriteMemory(pattern.get_first(1), s.data(), true);
        }

        if (std::filesystem::exists(GetExeModulePath() / xmlPathDefault, ec))
        {
            static std::string s = xmlPathDefault.string();
            auto pattern = find_pattern("68 ? ? ? ? E8 ? ? ? ? 8B F0 83 C4 08 39 75 D8 74 3D 80 3E 00 74 0C 56 E8 ? ? ? ? 83 C4 04 40 EB 02 33 C0 89 45 DC 39 45 E0 7D 10 6A 01 8D 4D D8 89 45 E0 E8 ? ? ? ? 8B 45 DC 85 C0");
            injector::WriteMemory(pattern.get_first(1), s.data(), true);
        }
    }

    static constexpr float fDefaultAspectRatio = 16.0f / 9.0f;

    static auto GetAspectRatio = []() -> float
    {
        if (!pViewportResolutionWidth || !pViewportResolutionHeight || *pViewportResolutionWidth == -1 || *pViewportResolutionHeight == -1)
            return fDefaultAspectRatio;
        return static_cast<float>(*pViewportResolutionWidth) / static_cast<float>(*pViewportResolutionHeight);
    };

    if (bUltraWideSupport)
    {
        auto pattern = find_pattern("F3 0F 10 45 ? 0F 5A C9 0F 5A C0 F2 0F 5E C1 F2 0F 59 05", "F3 0F 10 45 ? 8B 46 34 0F 5A C9");
        injector::MakeNOP(pattern.get_first(), 5, true);
        static auto BootSeqCommon__Render_Bink_textures = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            if (GetAspectRatio() > fDefaultAspectRatio)
                regs.xmm0.f32[0] = (((*(float*)(regs.ebp + 0x14)) * fDefaultAspectRatio) - regs.xmm1.f32[0]) * (-0.5f);
            else
                regs.xmm0.f32[0] = *(float*)(regs.ebp + 0x20);
        });

        pattern = hook::pattern("F2 0F 10 0D ? ? ? ? F2 0F 10 15 ? ? ? ? 89 47 10");
        injector::MakeNOP(pattern.get_first(), 8, true);
        static auto FCanvasUtil__SetViewport = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            if (GetAspectRatio() > fDefaultAspectRatio)
                regs.xmm1.f64[0] = 2.0 / (GetAspectRatio() / fDefaultAspectRatio);
            else
                regs.xmm1.f64[0] = 2.0;
        });

        //NetOnlineManager::NetOnlineManager
        pattern = hook::pattern("C7 45 ? ? ? ? ? 8D 64 24 00 53");
        injector::WriteMemory<uint32_t>(pattern.get_first(3), 4372, true);

        //UD3DRenderDevice
        pattern = hook::pattern("76 23 8B 06");
        injector::WriteMemory<uint8_t>(pattern.get_first(0), 0xEB, true); //jbe -> jmp
        
        //UD3DRenderDevice Letterbox
        pattern = find_pattern("3B 05 ? ? ? ? 74 05 BE ? ? ? ? 8B C2", "3B 0D ? ? ? ? 74 05 BE ? ? ? ? 8B C2");
        static auto Letterbox = *pattern.get_first<int*>(2);

        pattern = hook::pattern("76 10 8B 03 8B 4D 10 89 06 8B 11");
        static auto loc_A80681 = (uintptr_t)pattern.get_first(13 + 5);
        struct UD3DRenderDevice__Letterbox
        {
            void operator()(injector::reg_pack& regs)
            {
                if (float(*(uintptr_t*)regs.ebx) / float(*(uintptr_t*)(*(uintptr_t*)(regs.ebp + 0x10))) > fDefaultAspectRatio)
                {
                
                }
                else
                {
                    if (regs.xmm0.f32[0] > regs.xmm5.f32[0])
                    {
                        *(uintptr_t*)(regs.esp - 4) = loc_A80681;
                    }
                    else
                    {
                        regs.eax = *(uintptr_t*)regs.ebx;
                        regs.ecx = *(uintptr_t*)(regs.ebp + 0x10);
                        *(uintptr_t*)regs.esi = regs.eax; //1920
                        regs.edx = *(uintptr_t*)regs.ecx;
                        *(uintptr_t*)regs.edi = regs.edx; //823

                        *(uintptr_t*)regs.esi = regs.eax;
                        *(uintptr_t*)regs.edi = int((float)regs.eax / fDefaultAspectRatio);
                    }
                }
            }
        }; injector::MakeInline<UD3DRenderDevice__Letterbox>(pattern.get_first(-3), pattern.get_first(13));

        static auto ScaleHUD = [](auto& x, auto& y, auto& w, auto& h)
        {
            if (GetAspectRatio() > fDefaultAspectRatio)
            {
                auto width = (float)h * fDefaultAspectRatio;
                x = (int)(((float)w - width) * 0.5f);
                w = (int)width;
            }
            else
            {
                auto height = (float)w / fDefaultAspectRatio;
                y = (int)(((float)h - height) * 0.5f);
                h = (int)height;
            }
        };

        pattern = hook::pattern("F3 0F 11 04 24 50 8B 85 ? ? ? ? 51 52 50 57");
        if (!pattern.count_hint(1).empty())
        {
            static auto D3DRenderer__RenderHUD = safetyhook::create_mid(pattern.get_first(0), [](SafetyHookContext& regs)
            {
                auto& a2 = regs.edi;
                auto& x = *(uint32_t*)(regs.ebp - 0xAC);
                auto& y = regs.edx;
                auto& w = regs.ecx;
                auto& h = regs.eax;

                ScaleHUD(x, y, w, h);
            });
        }
        else
        {
            pattern = hook::pattern("F3 0F 11 04 24 52 8B 95");
            static auto D3DRenderer__RenderHUD = safetyhook::create_mid(pattern.get_first(0), [](SafetyHookContext& regs)
            {
                if (GetAspectRatio() > fDefaultAspectRatio)
                {
                    auto& a2 = regs.esi;
                    auto& x = *(uint32_t*)(regs.ebp - 0xB4);
                    auto& y = regs.ecx;
                    auto& w = regs.eax;
                    auto& h = regs.edx;
                
                    ScaleHUD(x, y, w, h);
                }
            });
        }

        pattern = hook::pattern("F3 0F 10 0D ? ? ? ? F3 0F 11 4C 24 ? 05 ? ? ? ? 66 0F 5A C0 F3 0F 11 04 24 50 8B 45 08 50 E8 ? ? ? ? 83 C4 10 5D C3");
        injector::MakeNOP(pattern.count(2).get(0).get<void>(0), 8, true);
        static auto ScreenFade1 = safetyhook::create_mid(pattern.count(2).get(0).get<void>(0), [](SafetyHookContext& regs)
        {
            if (GetAspectRatio() > fDefaultAspectRatio)
                regs.xmm1.f32[0] = 1.0f / (GetAspectRatio() / fDefaultAspectRatio);
            else
                regs.xmm1.f32[0] = 1.0f;
        });

        static auto ScreenFade2 = safetyhook::create_mid(pattern.count(2).get(1).get<void>(0), [](SafetyHookContext& regs)
        {
            if (GetAspectRatio() > fDefaultAspectRatio)
                regs.xmm1.f32[0] = 1.0f / (GetAspectRatio() / fDefaultAspectRatio);
            else
                regs.xmm1.f32[0] = 1.0f;
        });

        pattern = hook::pattern("F3 0F 11 40 ? 5B 5E 8B E5 5D C3");
        injector::MakeNOP(pattern.get_first(0), 5, true);
        static auto UI_Alignment = safetyhook::create_mid(pattern.get_first(0), [](SafetyHookContext& regs)
        {
            *(float*)(regs.eax + 0) = *(float*)(regs.esi + 0x3C) * 1280.0f / (float)*(int*)(regs.ebp - 8);
            *(float*)(regs.eax + 4) = *(float*)(regs.esi + 0x40) * 720.0f / (float)*(int*)(regs.ebp - 4);

            if (GetAspectRatio() > fDefaultAspectRatio)
            {
                *(float*)(regs.eax + 0) = *(float*)(regs.esi + 0x3C) * (GetAspectRatio() * 720.0f) / (float)*(int*)(regs.ebp - 8);
                *(float*)(regs.eax + 0) += ((GetAspectRatio() * 720.0f) - 1280.0f) * -0.5f;
            }
            else
            {
                if (Letterbox && *Letterbox)
                {
                    *(float*)(regs.eax + 4) = *(float*)(regs.esi + 0x40) * (720.0f / (GetAspectRatio() / fDefaultAspectRatio)) / (float)*(int*)(regs.ebp - 4);
                }
            }
        });

        pattern = find_pattern("F3 0F 10 46 ? F3 0F 11 85 ? ? ? ? 8D 85", "F3 0F 10 05 ? ? ? ? F3 0F 11 44 24 ? 8D 14 24");
        static auto UI__ScenePause__PerformPauseOption = safetyhook::create_mid(pattern.get_first(0), [](SafetyHookContext& regs)
        {
            if (GetAspectRatio() > fDefaultAspectRatio)
                regs.xmm0.f32[0] = GetAspectRatio();
            else
                regs.xmm0.f32[0] = *(float*)(regs.esi + 0x50);
        });

        // Disable Blood Overlay
        pattern = hook::pattern("F3 0F 11 2C 24 E8 ? ? ? ? 8B 86");
        injector::MakeNOP(pattern.get_first(0), 5, true);
        static auto UI__HudHealthFeedback__SceneTick = safetyhook::create_mid(pattern.get_first(0), [](SafetyHookContext& regs)
        {
            if (GetAspectRatio() > fDefaultAspectRatio)
                *(float*)(regs.esp) = 0.0f;
            else
                *(float*)(regs.esp) = regs.xmm5.f32[0];
        });

        // Disable Fading
        pattern = hook::pattern("89 96 ? ? ? ? F3 0F 11 86 ? ? ? ? F3 0F 11 86 ? ? ? ? 5E");
        injector::MakeNOP(pattern.get_first(0), 6, true);
        static auto Lead_DoFade = safetyhook::create_mid(pattern.get_first(0), [](SafetyHookContext& regs)
        {
            if (GetAspectRatio() > fDefaultAspectRatio)
                *(uint32_t*)(regs.esi + 0x7FC) = 0;
            else
                *(uint32_t*)(regs.esi + 0x7FC) = regs.edx;
        });
    }

    // FOV
    {
        pattern = hook::pattern("F3 0F 10 46 ? 0F 57 E4 0F 2E C4");
        struct CameraImpl__UpdateFOV
        {
            void operator()(injector::reg_pack& regs)
            {
                float val = 0.0f;

                _asm { fstp dword ptr[val] }

                fFOVDiff = val;
                if (GetAspectRatio() > fDefaultAspectRatio)
                    val = AdjustFOV(val, GetAspectRatio(), fDefaultAspectRatio);

                val *= fFOVFactor;

                if (val > 162.5f)
                    val = 162.5f;

                fFOVDiff = val / fFOVDiff;

                *(float*)(regs.esi + 0x34) = val;
                regs.xmm0.f32[0] = val;

                _asm { fld dword ptr[val] }

                TriggerScreenCullBias(false);
            }
        }; injector::MakeInline<CameraImpl__UpdateFOV>(pattern.get_first(0));

        // Cull
        pattern = hook::pattern("55 8B EC E8 ? ? ? ? 8B C8 E8 ? ? ? ? 80 7D 08 00");
        l3d::GetResource = (l3d::LeadOptions*(*)())injector::GetBranchDestination(pattern.get_first(3), true).as_int();
        shTriggerScreenCullBias = safetyhook::create_inline(pattern.get_first(), TriggerScreenCullBias);

        // Shadow Cull
        pattern = hook::pattern("F3 0F 10 0D ? ? ? ? F3 0F 11 85 ? ? ? ? 66 0F 6E 81");
        injector::WriteMemory(pattern.get_first(4), &fShadowCullDist, true);
    }

    if (bToggleRadarHotkey && false)
    {
        auto pattern = hook::pattern("55 8B EC 6A FF 68 ? ? ? ? 64 A1 ? ? ? ? 50 83 EC 4C 53 56 57 A1 ? ? ? ? 33 C5 50 8D 45 F4 64 A3 ? ? ? ? 8B D9 83 7D 0C 00");
        UI::ScenePause::shOnRadarChanged = safetyhook::create_inline(pattern.get_first(), UI::ScenePause::OnRadarChanged);

        pattern = hook::pattern("E8 ? ? ? ? 8B 8E ? ? ? ? 51 8B 8E ? ? ? ? E8 ? ? ? ? 8B 8E ? ? ? ? E8");
        static auto UI__ScenePause__InitSlots = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            UI::ScenePause::pUI = (void*)regs.ecx;
            UI::ScenePause::pUnk = (void*)(regs.ecx + 0x68C);
        });

        pattern = hook::pattern("55 8B EC 51 89 4D FC 8B 45 FC 0F B6 88 ? ? ? ? 85 C9 75 68");
        static auto UI__HudSelectionWheel__SceneTick = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            if (UInputManager::bIsCoverKeyPressed)
            {
                UInputManager::bIsCoverKeyPressed = false;
                if (UI::ScenePause::pUI && HudRadar::HudRadarEnabled && UI::ScenePause::pUnk)
                    UI::ScenePause::shOnRadarChanged.fastcall<uint8_t>(UI::ScenePause::pUI, (void*)UI::ScenePause::shOnRadarChanged.target(), UI::ScenePause::pUnk, !HudRadar::HudRadarEnabled());
            }
        });

        pattern = hook::pattern("8B 47 20 85 C0 74 08 50");
        static auto ButtonHandler__KeyEvent = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            auto key = *(uint32_t*)(regs.edi + 0x20);
            if (key == UInputManager::eCover)
                UInputManager::bIsCoverKeyPressed = true;
        });

        pattern = hook::pattern("55 8B EC 83 EC 14 C6 45 FF 00");
        HudRadar::HudRadarEnabled = (char (*)())pattern.get_first();
    }

    if (bEnableRunDuringForcedWalk)
    {
        pattern = hook::pattern("74 18 8B 80 ? ? ? ? 85 C0 74 0E 8B 4E 3C 83 E1 01 51 8B C8 E8");
        injector::WriteMemory<uint8_t>(pattern.get_first(), 0xEB, true); //jz -> jmp
    }

    if (bDisableNightVisionFlash)
    {
        // FThermalSonarVisionComponent::SetActiveVisionMode
        pattern = hook::pattern("D9 46 70 D9 9E");
        injector::MakeNOP(pattern.get_first(), 9, true);
        static auto SetActiveVisionModeHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            *(float*)(regs.esi + 0x8C) = -0.1f;
        });
    }
}

std::string currentGogglesLight = "LightGreen";
int __cdecl appStricmpHook(const char* String1, const char* String2)
{
    auto ret = _stricmp(String1, String2);
    if (!ret)
        currentGogglesLight = String1;
    else
        currentGogglesLight = "LightGreen";
    return ret;
}

void InitLED()
{
    CIniReader iniReader("");
    auto bLightSyncRGB = iniReader.ReadInteger("LOGITECH", "LightSyncRGB", 1);

    if (bLightSyncRGB)
    {
        //LoadoutCommon::FsBaseLoadout::GetSuitLightColor
        auto pattern = hook::pattern("E8 ? ? ? ? 83 C4 08 85 C0 0F 84 ? ? ? ? 46 83 FE 0A");
        injector::MakeCALL(pattern.get_first(0), appStricmpHook, true);

        LEDEffects::Inject([]()
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            auto [R, G, B] = LEDEffects::RGBtoPercent(17, 138, 41, 1.0f);

            if (std::string_view(currentGogglesLight) == "LightGreen")
            {
                std::tie(R, G, B) = LEDEffects::RGBtoPercent(17, 138, 41, 1.0f);
            }
            else if (std::string_view(currentGogglesLight) == "LightPaleOrange")
            {
                std::tie(R, G, B) = LEDEffects::RGBtoPercent(158, 118, 29, 1.0f);
            }
            else if (std::string_view(currentGogglesLight) == "LightRed")
            {
                std::tie(R, G, B) = LEDEffects::RGBtoPercent(163, 18, 15, 1.0f);
            }
            else if (std::string_view(currentGogglesLight) == "LightBlue")
            {
                std::tie(R, G, B) = LEDEffects::RGBtoPercent(24, 70, 213, 1.0f);
            }
            else if (std::string_view(currentGogglesLight) == "LightPink")
            {
                std::tie(R, G, B) = LEDEffects::RGBtoPercent(163, 33, 116, 1.0f);
            }
            else if (std::string_view(currentGogglesLight) == "LightPurple")
            {
                std::tie(R, G, B) = LEDEffects::RGBtoPercent(111, 34, 210, 1.0f);
            }
            else if (std::string_view(currentGogglesLight) == "LightWhite")
            {
                std::tie(R, G, B) = LEDEffects::RGBtoPercent(162, 182, 208, 1.0f);
            }
            else if (std::string_view(currentGogglesLight) == "LightGold")
            {
                std::tie(R, G, B) = LEDEffects::RGBtoPercent(145, 172, 30, 1.0f);
            }
            else if (std::string_view(currentGogglesLight) == "LightOrange")
            {
                std::tie(R, G, B) = LEDEffects::RGBtoPercent(161, 89, 12, 1.0f);
            }
            else if (std::string_view(currentGogglesLight) == "LightPaleBlue")
            {
                std::tie(R, G, B) = LEDEffects::RGBtoPercent(23, 159, 205, 1.0f);
            }

            LEDEffects::SetLighting(R, G, B, false, false, false);
        });
    }
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("0F 84 ? ? ? ? 56 56 56 68 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 83 C4 18 50 8D 8D"));
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(InitLED, hook::pattern("E8 ? ? ? ? 83 C4 08 85 C0 0F 84 ? ? ? ? 46 83 FE 0A"));
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

    }
    return TRUE;
}