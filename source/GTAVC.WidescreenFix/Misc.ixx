module;

#include <stdafx.h>
#include "../Shared/CheckedPattern.hpp"
#include "common.h"
#include "FixSettings.hpp"

export module Misc;

import Skeleton;
import Frontend;
import Camera;
import Entity;
import Vehicle;
import Timer;
import Draw;

injector::hook_back<void(__fastcall*)(CCamera*, void*)> hbCalculateDerivedValues;
void __fastcall CalculateDerivedValues(CCamera* camera, void* edx)
{
    static auto& bVCSCamShake = FixSettings().Int("MISC.VCSCamShake");
    if (bVCSCamShake)
        CCamera::UpdatePlayerVehicleSpeedBlur(camera);
    return hbCalculateDerivedValues.fun(camera, edx);
}

static void InstallVCSCamShake()
{
    static bool installed = false;
    if (installed || !FixSettings().Int("MISC.VCSCamShake"))
        return;
    installed = true;
    auto pattern = hook::pattern("E8 ? ? ? ? FF B4 24 ? ? ? ? E8 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? D9 58 30");
    if (!pattern.empty())
        hbCalculateDerivedValues.fun = injector::MakeCALL(pattern.get_first(), CalculateDerivedValues, true).get();
}

static void InstallCarSpeedDependantFOV()
{
    static auto& bCarSpeedDependantFOV = FixSettings().Int("MISC.CarSpeedDependantFOV");
    static bool installed = false;
    if (installed || !bCarSpeedDependantFOV)
        return;
    installed = true;
    auto pattern = hook::pattern("C7 83 ? ? ? ? ? ? ? ? 80 7B");
    if (pattern.empty())
        return;
    injector::MakeNOP(pattern.get_first(), 10, true);
    static auto Process_Cam_On_A_String_Hook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        constexpr auto DefaultFOV = 70.0f;
        auto cam = (CCam*)regs.ebx;

        // Off: the game's fixed field of view (the instruction this replaces)
        if (!bCarSpeedDependantFOV || cam->ResetStatics)
        {
            cam->FOV = DefaultFOV;
        }
        else if (cam->CamTargetEntity)
        {
            CVehicle* car = (CVehicle*)cam->CamTargetEntity;

            float forwardSpeed = DotProduct(car->GetForward(), car->m_vecMoveSpeed);

            if (forwardSpeed > 0.4f)
                cam->FOV += (forwardSpeed - 0.4f) * CTimer::ms_fTimeStep;

            if (cam->FOV > DefaultFOV)
                cam->FOV = pow(0.98f, CTimer::ms_fTimeStep) * (cam->FOV - DefaultFOV) + DefaultFOV;

            if (cam->FOV > DefaultFOV + 30.0f)
                cam->FOV = DefaultFOV + 30.0f;
        }
    });
}

class Misc
{
public:
    Misc()
    {
        WFP::onGameInitEvent() += []()
        {
            // Text Outline applies after a restart; the camera settings while the game runs
            ReplaceTextShadowWithOutline = FixSettings().Int("MISC.ReplaceTextShadowWithOutline");

            auto pattern = hook::pattern("83 D8 ? ? ? 89 44 24 ? 50 ? ? ? ? ? ? ? E9");
            if (pattern.size() < 1)
            {
                wfp::patterns::Report("GTAVC.WidescreenFix: feature signature unavailable.\n");
                return;
            }
            static auto SubtitlesHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                CFont::Details->dropColor = { 0, 0, 0, 0xFF };
                CFont::Details->dropShadowPosition = 2;
            });

            if (ReplaceTextShadowWithOutline)
            {
                auto pattern = hook::pattern("6A 32 6A 64 6A 64 6A 64 E8");
                if (pattern.size() < 1)
                {
                    wfp::patterns::Report("GTAVC.WidescreenFix: feature signature unavailable.\n");
                    return;
                }
                injector::WriteMemory<uint8_t>(pattern.count(1).get(0).get<uint32_t>(1), 0x00, true); // cursor shadow alpha 0x4A35A2

                pattern = hook::pattern("E8 ? ? ? ? 8B 85 08 01 00 00 83 C4 0C 3D FF 00 00 00");
                if (pattern.size() < 1)
                {
                    wfp::patterns::Report("GTAVC.WidescreenFix: feature signature unavailable.\n");
                    return;
                }
                injector::MakeNOP(pattern.count(1).get(0).get<uint32_t>(0), 5, true); //menu title shadows 0x49E30E

                // radio shadow 0x5FA1A5
                pattern = hook::pattern("68 ? ? ? ? 6A ? 6A ? 6A ? E8 ? ? ? ? 8D 44 24 ? 50 E8 ? ? ? ? ? ? ? ? ? ? 59 53");
                if (pattern.size() < 1)
                {
                    wfp::patterns::Report("GTAVC.WidescreenFix: feature signature unavailable.\n");
                    return;
                }
                injector::WriteMemory(pattern.get_first(1), 0x00000000, true);
                static auto RadioHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    CFont::Details->dropColor = { 0, 0, 0, 0xFF };
                    CFont::Details->dropShadowPosition = 2;
                });
            }

            // The camera settings' hooks are installed the first time they're turned on
            InstallCarSpeedDependantFOV();
            OnSettingChange("MISC.CarSpeedDependantFOV", []() { InstallCarSpeedDependantFOV(); });
            InstallVCSCamShake();
            OnSettingChange("MISC.VCSCamShake", []() { InstallVCSCamShake(); });
        };
    }
} Misc;