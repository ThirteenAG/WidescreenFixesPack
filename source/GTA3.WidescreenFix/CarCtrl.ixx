module;

#include <stdafx.h>
#include "common.h"

export module CarCtrl;

import Skeleton;
import Camera;

class CarCtrl
{
public:
    CarCtrl()
    {
        WFP::onGameInitEvent() += []()
        {
            static float requestMultiplier = 1.0f;

            //CCarCtrl::GenerateOneRandomCar
            auto pattern = hook::pattern("D9 00 D9 9C 24 ? ? ? ? D9 40 ? D9 9C 24 ? ? ? ? D9 40 ? 59");
            static auto GenerateOneRandomCar = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                float fovScale = std::clamp((SCREEN_WIDTH / SCREEN_HEIGHT) / (4.0f / 3.0f), 1.0f, (16.0f / 9.0f) / (4.0f / 3.0f));
                requestMultiplier = TheCamera->GenerationDistMultiplier * fovScale;
            });

            pattern = hook::pattern("D8 0D ? ? ? ? C6 44 24 ? ? D9 5C 24 ? E9 55 14 00 00");
            injector::WriteMemory(pattern.get_first(2), &requestMultiplier, true);

            pattern = hook::pattern("D8 0D ? ? ? ? C6 44 24 ? ? D9 5C 24 ? E9 3F 14 00 00");
            injector::WriteMemory(pattern.get_first(2), &requestMultiplier, true);

            pattern = hook::pattern("D8 0D ? ? ? ? C6 44 24 ? ? D9 5C 24 ? E9 F0 13 00 00");
            injector::WriteMemory(pattern.get_first(2), &requestMultiplier, true);

            pattern = hook::pattern("D8 0D ? ? ? ? C6 44 24 ? ? D9 5C 24 ? E9 DA 13 00 00");
            injector::WriteMemory(pattern.get_first(2), &requestMultiplier, true);

            pattern = hook::pattern("D8 0D ? ? ? ? C6 44 24 ? ? D9 5C 24 ? E9 99 13 00 00");
            injector::WriteMemory(pattern.get_first(2), &requestMultiplier, true);

            pattern = hook::pattern("D8 0D ? ? ? ? C6 44 24 ? ? D9 5C 24 ? E9 56 13 00 00");
            injector::WriteMemory(pattern.get_first(2), &requestMultiplier, true);

            pattern = hook::pattern("D9 05 ? ? ? ? DD D9 D9 05 ? ? ? ? D8 C9 D8 DA");
            injector::WriteMemory(pattern.get_first(2), &requestMultiplier, true);

            pattern = hook::pattern("D8 0D ? ? ? ? D8 DA DF E0 F6 C4 45 0F 85 ? ? ? ? 85 DB");
            injector::WriteMemory(pattern.get_first(2), &requestMultiplier, true);

            //CCarCtrl::PossiblyRemoveVehicle
            pattern = hook::pattern("89 D9 89 44 24 ? E8 ? ? ? ? 84 C0 75");
            static auto PossiblyRemoveVehicle = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                float fovScale = (SCREEN_WIDTH / SCREEN_HEIGHT) / (4.0f / 3.0f);
                requestMultiplier = TheCamera->GenerationDistMultiplier * fovScale;
            });

            pattern = hook::pattern("D8 0D ? ? ? ? EB 07 90 D9 05 ? ? ? ? D9 5C 24");
            injector::WriteMemory(pattern.get_first(2), &requestMultiplier, true);
        };
    }
} CarCtrl;