module;

#include <stdafx.h>
#include "common.h"
#include "callargs.h"

export module Main;

import Draw;
import Skeleton;
import Camera;

using tCameraSize = void(__cdecl*)(RwCamera*, RwRect*, float, float);

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
            auto pattern = hook::pattern("E8 ? ? ? ? 83 C4 ? A1 ? ? ? ? 50 E8 ? ? ? ? A1 ? ? ? ? 8D 54 24");
            static auto DoRWStuffStartOfFrame = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                auto [camera, rect, viewWindow, aspectRatio] = deduce_args<tCameraSize>(regs);
                viewWindow = SCREEN_VIEWWINDOW;
                aspectRatio = SCREEN_ASPECT_RATIO;
            });

            pattern = hook::pattern("E8 ? ? ? ? 83 C4 ? A1 ? ? ? ? 50 E8 ? ? ? ? A1 ? ? ? ? 59");
            static auto DoRWStuffStartOfFrame_Horizon = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                auto [camera, rect, viewWindow, aspectRatio] = deduce_args<tCameraSize>(regs);
                viewWindow = SCREEN_VIEWWINDOW;
                aspectRatio = SCREEN_ASPECT_RATIO;
            });

            pattern = hook::pattern("E8 ? ? ? ? 80 3D ? ? ? ? ? 74 ? 81 3D");
            static auto Idle1 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                CDraw::SetAspectRatio(CDraw::FindAspectRatio());
            });

            pattern = hook::pattern("E8 ? ? ? ? A1 ? ? ? ? 83 C4 ? 50 E8 ? ? ? ? A1 ? ? ? ? 59 6A ? 68 ? ? ? ? 50 E8 ? ? ? ? A1 ? ? ? ? 83 C4 ? 50 E8 ? ? ? ? 85 C0 59 75 ? 83 C4 ? C3 89 C0");
            static auto Idle2 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                auto [camera, rect, viewWindow, aspectRatio] = deduce_args<tCameraSize>(regs);

                viewWindow = SCREEN_VIEWWINDOW;
                aspectRatio = SCREEN_ASPECT_RATIO;
            });

            pattern = hook::pattern("83 3D ? ? ? ? ? ? ? 74 ? 83 C4");
            static auto FrontendIdle1 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                CDraw::SetAspectRatio(CDraw::FindAspectRatio());
            });

            pattern = hook::pattern("E8 ? ? ? ? A1 ? ? ? ? 83 C4 ? 50 E8 ? ? ? ? A1 ? ? ? ? 59 6A ? 68 ? ? ? ? 50 E8 ? ? ? ? A1 ? ? ? ? 83 C4 ? 50 E8 ? ? ? ? 85 C0 59 75 ? 83 C4 ? C3 E8");
            static auto FrontendIdle2 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                auto [camera, rect, viewWindow, aspectRatio] = deduce_args<tCameraSize>(regs);

                viewWindow = SCREEN_VIEWWINDOW;
                aspectRatio = SCREEN_ASPECT_RATIO;
            });

            pattern = hook::pattern("E8 ? ? ? ? 83 C4 ? B8 ? ? ? ? 83 C4 ? 5B");
            static auto AppEventHandler = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                auto [camera, rect, viewWindow, aspectRatio] = deduce_args<tCameraSize>(regs);

                viewWindow = SCREEN_VIEWWINDOW;
                aspectRatio = SCREEN_ASPECT_RATIO;
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