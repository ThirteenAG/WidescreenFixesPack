module;

#include <stdafx.h>

export module Unlocks;

import ComVars;

namespace fb
{
    namespace NFSUIVehicleComp
    {
        struct Unlockable
        {
            uint8_t pad[0x18];
            bool m_isUnlocked;
            bool m_hideHowTo;
            bool m_isHiddenUnlock;
            bool m_isPromoContent;
        };

        SafetyHookInline shgetMatchingGarageCar = {};
        Unlockable* __cdecl getMatchingGarageCar(uint32_t attribSysClassKey, uint32_t attribSysCollectionKey)
        {
            auto GarageCar = shgetMatchingGarageCar.unsafe_ccall<Unlockable*>(attribSysClassKey, attribSysCollectionKey);
            GarageCar->m_isUnlocked = true;
            return GarageCar;
        }
    }

    namespace UIDataValue
    {
        SafetyHookInline shsetValue = {};
        void __fastcall setValue(void* UIDataValue, void* edx, int index, const char** key, uint8_t* dataValue)
        {
            auto strKey = std::string_view(*key);

            if (strKey == "isLocked")
            {
                *dataValue = 0;
            }
            else if (strKey == "IsUnlocked")
            {
                *dataValue = 1;
            }

            return shsetValue.unsafe_fastcall(UIDataValue, edx, index, key, dataValue);
        }

        SafetyHookInline shsetValue2 = {};
        void __fastcall setValue2(void* UIDataValue, void* edx, const char** key, uint8_t* dataValue)
        {
            auto strKey = std::string_view(*key);

            if (strKey == "isLocked")
            {
                *dataValue = 0;
            }
            else if (strKey == "IsUnlocked")
            {
                *dataValue = 1;
            }

            return shsetValue2.unsafe_fastcall(UIDataValue, edx, key, dataValue);
        }
    }
}

class UI
{
public:
    UI()
    {
        WFP::onInitEvent() += []()
        {
            CIniReader iniReader("");
            auto bUnlockAllCars = iniReader.ReadInteger("UNLOCKS", "UnlockAllCars", 1) != 0;
            auto bUnlockChallenges = iniReader.ReadInteger("UNLOCKS", "UnlockChallenges", 1) != 0;
            auto bUnlockEverything = iniReader.ReadInteger("UNLOCKS", "UnlockEverything", 1) != 0;

            if (bUnlockChallenges)
            {
                auto pattern = hook::pattern("E8 ? ? ? ? 8B 44 24 ? 8B 4C 24 ? 2B C1 83 F8 ? 7E ? 3B CB 74 ? 50 51 8D 4C 24 ? E8 ? ? ? ? 8D 4C 24 ? E8 ? ? ? ? 8A 8C 24 ? ? ? ? 8B 54 24 ? 80 E1 ? B8");
                fb::UIDataValue::shsetValue = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), fb::UIDataValue::setValue);

                pattern = hook::pattern("E8 ? ? ? ? 8D 8C 24 ? ? ? ? E8 ? ? ? ? 8B 44 24 ? 8B 4C 24 ? 2B C1 83 F8 ? 7E ? 3B CB 74 ? 50 51 8D 4C 24 ? E8 ? ? ? ? 8A 84 24");
                fb::UIDataValue::shsetValue2 = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), fb::UIDataValue::setValue2);
            }

            if (bUnlockAllCars)
            {
                auto pattern = hook::pattern("E8 ? ? ? ? 83 C4 ? 80 7C 24 ? ? 74 ? 80 78");
                fb::NFSUIVehicleComp::shgetMatchingGarageCar = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), fb::NFSUIVehicleComp::getMatchingGarageCar);
            }

            if (bUnlockEverything)
            {
                auto pattern = hook::pattern("8B 56 ? 0F B7 48");
                static auto UnlockablePropertiesHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    auto property = std::string_view(*(const char**)regs.eax);

                    if (property == "Unlockers")
                    {
                        static auto undef = "Undefined";
                        static auto addr_undef = &undef;
                        regs.eax = (uintptr_t)&addr_undef;
                    }
                });
            }
        };
    }
} UI;