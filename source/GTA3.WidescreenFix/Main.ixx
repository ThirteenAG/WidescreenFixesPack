module;

#include <stdafx.h>
#include "common.h"

export module Main;

import Draw;
import Skeleton;
import Camera;

SafetyHookInline shCameraSize = {};
void __cdecl CameraSize(RwCamera* camera, RwRect* rect, float viewWindow, float aspectRatio)
{
    return shCameraSize.unsafe_ccall(camera, rect, SCREEN_VIEWWINDOW, SCREEN_ASPECT_RATIO);
}

class Main
{
public:
    Main()
    {
        WFP::onInitEvent() += []()
        {
            // default res
            auto [x, y] = GetDesktopRes();
            auto pattern = hook::pattern("81 3C ? ? ? ? ? 75 ? 81 7C 24 ? ? ? ? ? 75 ? 83 7C 24 ? ? 75 ? 8B 44 24 ? 83 E0 ? 75");
            injector::WriteMemory<int32_t>(pattern.get_first(3), x, true);
            injector::WriteMemory<int32_t>(pattern.get_first(13), y, true);
            injector::MakeNOP(pattern.get_first(24), 2, true);
        };

        WFP::onGameInitEvent() += []()
        {
            auto pattern = hook::pattern("E8 ? ? ? ? C7 44 24 ? ? ? ? ? 8B 54 24");
            shCameraSize = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), CameraSize);

            pattern = hook::pattern("E8 ? ? ? ? 80 3D ? ? ? ? ? 74 ? 81 3D");
            static auto Idle1 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                CDraw::SetAspectRatio(CDraw::FindAspectRatio());
            });

            pattern = hook::pattern("83 3D ? ? ? ? ? ? ? 74 ? 83 C4");
            static auto FrontendIdle1 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                CDraw::SetAspectRatio(CDraw::FindAspectRatio());
            });

            //DoFade
            pattern = hook::pattern("0F 84 ? ? ? ? 8B 35 ? ? ? ? 89 F0 3D");
            injector::WriteMemory<uint16_t>(pattern.get_first(), 0xE990, true);

            //CWeapon::DoBulletImpact (smoke misplacement)
            pattern = hook::pattern("50 ? ? ? ? ? ? ? 6A ? C7 84 24 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? E8 ? ? ? ? 83 C4");
            if (!pattern.empty())
                injector::WriteMemory<uint8_t>(pattern.get_first(), 0x55, true); //push ebp

            pattern = hook::pattern("50 6A ? C7 84 24 ? ? ? ? ? ? ? ? E8 ? ? ? ? 8B 84 24");
            if (!pattern.empty())
                injector::WriteMemory<uint8_t>(pattern.get_first(), 0x55, true); //push ebp
        };
    }
} Main;