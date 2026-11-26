module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module Unlocks;

import ComVars;

export void InitUnlocks()
{
    auto& settings = FixSettings();
    auto bUnlockDLC = settings.Int("UNLOCKS.UnlockDLC") != 0;
    static auto bUnlockAllNonCampaignMissions = settings.Int("UNLOCKS.UnlockAllNonCampaignMissions") != 0;
    static auto bUnlockAllCampaignMissions = settings.Int("UNLOCKS.UnlockAllCampaignMissions") != 0;

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
        static auto unlock = [](uintptr_t mission, uint32_t status)
        {
            if (status != 0)
                return status;

            const auto id = *reinterpret_cast<const uint8_t*>(mission + 0xA4);
            const bool fourthEchelon = id >= 13 && id <= 28;
            const bool campaign = (id >= 1 && id <= 12) || id == 37 || id == 38;
            return static_cast<uint32_t>((fourthEchelon && bUnlockAllNonCampaignMissions) ||
                                         (campaign && bUnlockAllCampaignMissions));
        };

        // Profile deserialization and the three compiled forms of mission reset.
        static auto deserialize = safetyhook::create_mid(find_pattern("89 86 B4 00 00 00 8B 8E BC 00 00 00 6A 00").get_first(), [](SafetyHookContext& regs)
        {
            regs.eax = unlock(regs.esi, regs.eax);
        });
        static auto reset = safetyhook::create_mid(find_pattern("89 81 B4 00 00 00 33 C0 B2 FF").get_first(), [](SafetyHookContext& regs)
        {
            regs.eax = unlock(regs.ecx, regs.eax);
        });
        static auto resetProfile = safetyhook::create_mid(find_pattern("89 90 B4 00 00 00 B2 FF 88 98").get_first(), [](SafetyHookContext& regs)
        {
            regs.edx = unlock(regs.eax, regs.edx);
        });
        static auto resetAll = safetyhook::create_mid(find_pattern("89 B0 B4 00 00 00 88 98 B8 00 00 00").get_first(), [](SafetyHookContext& regs)
        {
            regs.esi = unlock(regs.eax, regs.esi);
        });
    }
}
