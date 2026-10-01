module;

#include <stdafx.h>

export module Unlocks;

import ComVars;

export void InitUnlocks()
{
    CIniReader iniReader("");
    auto bUnlockDLC = iniReader.ReadInteger("UNLOCKS", "UnlockDLC", 1) != 0;
    static auto bUnlockAllNonCampaignMissions = iniReader.ReadInteger("UNLOCKS", "UnlockAllNonCampaignMissions", 1) != 0;
    static auto bUnlockAllCampaignMissions = iniReader.ReadInteger("UNLOCKS", "UnlockAllCampaignMissions", 0) != 0;

    if (bUnlockDLC)
    {
        // NetOnlineContentRightsManager::GetUnlocksFromSaveGame
        auto pattern = find_pattern("74 04 C6 40 04 01 56");
        injector::MakeNOP(pattern.get_first(), 2, true);

        // NetOnlineContentRightsManager::ResetUPlayRewardPrivs
        pattern = find_pattern("32 DB 3B C6 74 1B");
        injector::MakeNOP(pattern.get_first(), 2, true);
        static auto ResetUPlayRewardPrivsHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            regs.ebx = 1;
        });

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
                        MissionStatusEnum = 1;

                    if (MissionEnum == 21 || MissionEnum == 37 || MissionEnum == 38)
                        MissionStatusEnum = 1;
                }
            }
        });
    }
}
