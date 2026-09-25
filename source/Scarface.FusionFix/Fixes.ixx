module;
#include "stdafx.h"

export module Fixes;
import ComVars;

// WeaponStatePropTemplate: side minimum at +0x60, maximum at +0x64.
// GetBarrelCreepSideMax in the shipped source returns the minimum by mistake.
float __fastcall GetBarrelCreepSideMax(void* weapon, void*)
{
    return Game::Field<float>(Game::Field<void*>(weapon, 0x150), 0x64);
}

class Fixes
{
public:
    Fixes()
    {
        WFP::onInitEvent() += []()
        {
            CIniReader iniReader("");
            auto bSkipIntro = iniReader.ReadInteger("MAIN", "SkipIntro", 1) != 0;
            auto bFixRecoilRange = iniReader.ReadInteger("FIXES", "FixRecoilRange", 1) != 0;
            static auto fForceAlphaRef = iniReader.ReadFloat("TEST", "ForceAlphaRef", 0.0f);

            if (bSkipIntro)
            {
                auto pattern = hook::pattern("0F 85 ? ? ? ? 6A ? 6A ? 6A ? 6A ? 68 ? ? ? ? E8 ? ? ? ? 83 C4 ? 5E");
                injector::MakeNOP(pattern.get_first(), 6); // Legal
                pattern = hook::pattern("E8 ? ? ? ? 83 C4 20 5F 5E C7 45");
                injector::MakeNOP(pattern.get_first(), 5); // Movies.
                pattern = hook::pattern("75 22 E8 ? ? ? ? 8B C8");
                injector::MakeNOP(pattern.get_first(), 2); // Press to start.
            }

            if (bFixRecoilRange)
            {
                // First pushed argument is MAX; the second call still supplies MIN.
                auto pattern = hook::pattern("E8 ? ? ? ? 51 8B CE ? ? ? E8 ? ? ? ? 51 8B 0D ? ? ? ? ? ? ? E8 ? ? ? ? ? ? ? ? ? ? 8B 0D");
                injector::MakeCALL(pattern.get_first(), GetBarrelCreepSideMax, true);
            }

            if (fForceAlphaRef)
            {
                auto pattern = hook::pattern("D9 05 ? ? ? ? 8B 4E 14 D9 7C 24 12");
                injector::WriteMemory(pattern.get_first(2), &fForceAlphaRef, true);
            }
        };
    }
} Fixes;
