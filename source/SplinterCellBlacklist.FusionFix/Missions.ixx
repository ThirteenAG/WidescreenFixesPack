module;

#include <stdafx.h>

export module Missions;

import ComVars;

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

namespace UI
{
    SafetyHookInline shGetIsDifficultyLevelPerfectionist{};
    bool GetIsDifficultyLevelPerfectionist()
    {
        return shGetIsDifficultyLevelPerfectionist.ccall<bool>();
    }

    bool GetIsDifficultyLevelPerfectionistHook()
    {
        return false;
    }
}

namespace FThermalSonarVisionComponent
{
    SafetyHookInline shSetCurrentActiveSonarWaveRange{};
    void __fastcall SetCurrentActiveSonarWaveRange(void* _this, void* edx, float range)
    {
        if (UI::GetIsDifficultyLevelPerfectionist())
            range /= 1.5f;
        return shSetCurrentActiveSonarWaveRange.fastcall(_this, edx, range);
    }
}

export void InitMissions()
{
    CIniReader iniReader("");
    auto bDisableNightVisionFlash = iniReader.ReadInteger("MAIN", "DisableNightVisionFlash", 1) != 0;
    auto bDisablePerfectionistChecks = iniReader.ReadInteger("MAIN", "DisablePerfectionistChecks", 1) != 0;
    auto nDefaultMissionFilter = std::clamp(iniReader.ReadInteger("MAIN", "DefaultMissionFilter", 1), 0, 3);
    auto bSMIMapDisableStartupAnimation = iniReader.ReadInteger("MAIN", "SMIMapDisableStartupAnimation", 1) != 0;

    sExtractionWaveConfigs = iniReader.ReadString("EXTRACTION", "ExtractionWaveConfigs", "Default");
    nExtractionWaveEnemyMultiplier = std::clamp(iniReader.ReadInteger("EXTRACTION", "ExtractionWaveEnemyMultiplier", 1), 1, 9999);
    nExtractionWaveEnemyRandomRangeMin = std::clamp(iniReader.ReadInteger("EXTRACTION", "ExtractionWaveEnemyRandomRangeMin", 0), 0, 9999);
    nExtractionWaveEnemyRandomRangeMax = std::clamp(iniReader.ReadInteger("EXTRACTION", "ExtractionWaveEnemyRandomRangeMax", 4), 1, 9999);

    static auto sHUNTERReinforcementsNumber = iniReader.ReadString("HUNTER", "ReinforcementsNumber", "Default");
    static auto nHUNTERReinforcementsEnemyMultiplier = std::clamp(iniReader.ReadInteger("HUNTER", "ReinforcementsEnemyMultiplier", 1), 1, 9999);
    static auto nHUNTERReinforcementsEnemyRandomRangeMin = std::clamp(iniReader.ReadInteger("HUNTER", "ReinforcementsEnemyRandomRangeMin", 1), 1, 9999);
    static auto nHUNTERReinforcementsEnemyRandomRangeMax = std::clamp(iniReader.ReadInteger("HUNTER", "ReinforcementsEnemyRandomRangeMax", 1), 1, 9999);

    static auto bGHOSTDisableMissionFailOnDetection = iniReader.ReadInteger("GHOST", "DisableMissionFailOnDetection", 1) != 0;

    static auto bCOOPDisableMissionFailOnDetection = iniReader.ReadInteger("COOP", "DisableMissionFailOnDetection", 0) != 0;
    static auto sCOOPReinforcementsNumber = iniReader.ReadString("COOP", "ReinforcementsNumber", "Default");
    static auto nCOOPReinforcementsEnemyMultiplier = std::clamp(iniReader.ReadInteger("COOP", "ReinforcementsEnemyMultiplier", 1), 1, 9999);
    static auto nCOOPReinforcementsEnemyRandomRangeMin = std::clamp(iniReader.ReadInteger("COOP", "ReinforcementsEnemyRandomRangeMin", 1), 1, 9999);
    static auto nCOOPReinforcementsEnemyRandomRangeMax = std::clamp(iniReader.ReadInteger("COOP", "ReinforcementsEnemyRandomRangeMax", 1), 1, 9999);

    static auto bCAMPAIGNDisableMissionFailOnDetection = iniReader.ReadInteger("CAMPAIGN", "DisableMissionFailOnDetection", 0) != 0;
    auto bEnableRunDuringForcedWalk = iniReader.ReadInteger("CAMPAIGN", "EnableRunDuringForcedWalk", 1) != 0;

    // GameMode
    auto pattern = hook::pattern("55 8B EC 83 3D ? ? ? ? ? 75 55");
    shLead_SetCurrentGameMode = safetyhook::create_inline(pattern.get_first(), Lead_SetCurrentGameMode);

    // Disable perfectionist checks
    pattern = hook::pattern("53 32 DB E8 ? ? ? ? 85 C0 74 29");
    UI::shGetIsDifficultyLevelPerfectionist = safetyhook::create_inline(pattern.get_first(), bDisablePerfectionistChecks ? UI::GetIsDifficultyLevelPerfectionistHook : UI::GetIsDifficultyLevelPerfectionist);

    pattern = hook::pattern("E8 ? ? ? ? F3 0F 10 46 ? 51 8B CE");
    FThermalSonarVisionComponent::shSetCurrentActiveSonarWaveRange = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), FThermalSonarVisionComponent::SetCurrentActiveSonarWaveRange);

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
        // AECoopHunterSpawner
        pattern = hook::pattern("85 5E 08 75 29 8B 06 8B 50 0C 6A 04 8D 8F ? ? ? ? 51 8B CE FF D2 85 5E 08 75 12 8B 06 8B 50 0C 6A 04 8D 8F ? ? ? ? 51 8B CE FF D2 F6 46 08 02");
        static auto FCheckpointPackReaderHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            if (CurrentGameMode == HUNTER)
            {
                if (iequals(sHUNTERReinforcementsNumber, "Random"))
                    *(uint32_t*)(regs.edi + 0x370) = GetRandomInt(nHUNTERReinforcementsEnemyRandomRangeMin, nHUNTERReinforcementsEnemyRandomRangeMax);
            }
            else if (CurrentGameMode == COOP)
            {
                if (iequals(sCOOPReinforcementsNumber, "Random"))
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

    if (bEnableRunDuringForcedWalk)
    {
        pattern = hook::pattern("74 18 8B 80 ? ? ? ? 85 C0 74 0E 8B 4E 3C 83 E1 01 51 8B C8 E8");
        injector::WriteMemory<uint8_t>(pattern.get_first(), 0xEB, true); // jz -> jmp
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

    {
        pattern = hook::pattern("B9 ? ? ? ? 89 8E ? ? ? ? F3 0F 7E 05");
        injector::WriteMemory(pattern.get_first(1), nDefaultMissionFilter, true);
    }

    if (bSMIMapDisableStartupAnimation)
    {
        pattern = hook::pattern("0F 8E ? ? ? ? 8D 8E ? ? ? ? E8 ? ? ? ? 80 BE");
        injector::WriteMemory<uint16_t>(pattern.count(2).get(1).get<void>(), 0xE990, true); // jle -> jmp
    }
}
