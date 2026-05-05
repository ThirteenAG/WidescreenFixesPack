module;

#include <stdafx.h>
#include "common.h"

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
    CCamera::UpdatePlayerVehicleSpeedBlur(camera);
    return hbCalculateDerivedValues.fun(camera, edx);
}

class Misc
{
public:
    Misc()
    {
        WFP::onGameInitEvent() += []()
        {
            CIniReader iniReader("");
            ReplaceTextShadowWithOutline = iniReader.ReadInteger("MISC", "ReplaceTextShadowWithOutline", 0);
            auto bCarSpeedDependantFOV = iniReader.ReadInteger("MISC", "CarSpeedDependantFOV", 1) != 0;
            auto bVCSCamShake = iniReader.ReadInteger("MISC", "VCSCamShake", 0) != 0;

            if (ReplaceTextShadowWithOutline)
            {
                auto pattern = hook::pattern("6A 32 6A 64 6A 64 6A 64 E8");
                injector::WriteMemory<uint8_t>(pattern.count(1).get(0).get<uint32_t>(1), 0x00, true); // cursor shadow alpha 0x4A35A2

                pattern = hook::pattern("E8 ? ? ? ? 8B 85 08 01 00 00 83 C4 0C 3D FF 00 00 00");
                injector::MakeNOP(pattern.count(1).get(0).get<uint32_t>(0), 5, true); //menu title shadows 0x49E30E

                pattern = hook::pattern("83 D8 ? ? ? 89 44 24 ? 50 ? ? ? ? ? ? ? E9");
                static auto SubtitlesHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    CFont::Details->dropShadowPosition = 2;
                });
            }

            if (bCarSpeedDependantFOV)
            {
                auto pattern = hook::pattern("C7 83 ? ? ? ? ? ? ? ? 80 7B");
                injector::MakeNOP(pattern.get_first(), 10, true);
                static auto Process_Cam_On_A_String_Hook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    constexpr auto DefaultFOV = 70.0f;
                    auto cam = (CCam*)regs.ebx;

                    if (cam->ResetStatics)
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

            if (bVCSCamShake)
            {
                auto pattern = hook::pattern("E8 ? ? ? ? FF B4 24 ? ? ? ? E8 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? D9 58 30");
                hbCalculateDerivedValues.fun = injector::MakeCALL(pattern.get_first(), CalculateDerivedValues, true).get();
            }
        };
    }
} Misc;