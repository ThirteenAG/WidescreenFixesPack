module;

#include <stdafx.h>
#include "common.h"

export module Main;

import Draw;
import Skeleton;
import Camera;
import Entity;
import CutsceneMgr;

export GameRef<CScene> Scene;

export CEntity* (__cdecl* FindPlayerPed)() = nullptr;
export CEntity* (__cdecl* FindPlayerVehicle)() = nullptr;
export void (__cdecl* RwCameraSetNearClipPlane)(RwCamera* camera, float nearClip) = nullptr;

SafetyHookInline shCameraSize = {};
void __cdecl CameraSize(RwCamera* camera, RwRect* rect, float viewWindow, float aspectRatio)
{
    return shCameraSize.unsafe_ccall(camera, rect, SCREEN_VIEWWINDOW, SCREEN_ASPECT_RATIO);
}

RwV3d* RwV3dTransformPointsHook(RwV3d* pointsOut, const RwV3d* pointsIn, int32_t numPoints, const RwMatrix* matrix)
{
    for (int32_t i = 0; i < numPoints; i++)
    {
        const float x = pointsIn[i].x;
        const float y = pointsIn[i].y;
        const float z = pointsIn[i].z;

        pointsOut[i].x = matrix->right.x * x + matrix->up.x * y + matrix->at.x * z + matrix->pos.x;
        pointsOut[i].y = matrix->right.y * x + matrix->up.y * y + matrix->at.y * z + matrix->pos.y;
        pointsOut[i].z = matrix->right.z * x + matrix->up.z * y + matrix->at.z * z + matrix->pos.z;
    }

    return pointsOut;
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

            pattern = hook::pattern("A3 ? ? ? ? 75 ? A1 ? ? ? ? 50");
            Scene.SetAddress(*pattern.get_first<CScene*>(1));

            pattern = find_pattern("E8 ? ? ? ? 8B 80 ? ? ? ? 8B 50 ? 83 FA ? 7E");
            FindPlayerPed = (decltype(FindPlayerPed))injector::GetBranchDestination(pattern.get_first()).as_int();

            pattern = find_pattern("E8 ? ? ? ? 85 C0 74 ? ? ? ? ? ? ? ? ? ? DF E0");
            FindPlayerVehicle = (decltype(FindPlayerVehicle))injector::GetBranchDestination(pattern.get_first()).as_int();

            pattern = find_pattern("E8 ? ? ? ? 59 59 83 C4 ? 5D 5F 5E 5B C3 ? ? EB");
            RwCameraSetNearClipPlane = (decltype(RwCameraSetNearClipPlane))injector::GetBranchDestination(pattern.get_first()).as_int();

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

            //FireInstantHit doesn't set the target coords
            pattern = hook::pattern("C6 05 ? ? ? ? ? ? ? 8D B4 24");
            static auto FireInstantHitBugfix = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                auto& target = *(CVector*)(regs.esp + 0x84);
                auto& pPoint = *(CVector*)(regs.esp + 0xF4);
                target = pPoint;
            });

            //CCamera::IsSphereVisible
            pattern = hook::pattern("E8 ? ? ? ? ? ? ? ? ? ? ? ? ? ? 83 C4 ? ? ? ? ? ? ? ? ? ? ? ? ? DF E0 80 E4 ? 80 FC ? 75 ? 30 DB");
            injector::MakeCALL(pattern.get_first(), RwV3dTransformPointsHook, true);

            //CEntity::GetIsOnScreenComplex
            pattern = hook::pattern("E8 ? ? ? ? ? ? ? ? ? ? ? ? ? ? 83 C4 ? ? ? ? ? ? ? ? ? DF E0");
            injector::MakeCALL(pattern.get_first(), RwV3dTransformPointsHook, true);

            pattern = hook::pattern("E8 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? 31 D2 83 C4");
            injector::MakeCALL(pattern.get_first(), RwV3dTransformPointsHook, true);

            //CCamera::Process
            pattern = hook::pattern("D9 83 ? ? ? ? D9 9B ? ? ? ? D9 83 ? ? ? ? D8 0D");
            static auto LODDistFix = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                auto& LODDistMultiplier = *(float*)(regs.ebx + 0xEC);

                if (!CCutsceneMgr::IsRunning() || CCutsceneMgr::ms_useLodMultiplier)
                {
                    LODDistMultiplier = (70.0f * CDraw::GetAspectRatio() / (4.0f / 3.0f)) / CDraw::GetFOV();

                    if ((TheCamera->GetPosition().z > 55.0f && FindPlayerVehicle()))
                    {
                        constexpr float DEFAULT_NEAR = 0.9f;
                        LODDistMultiplier *= 1.0f + std::max((TheCamera->GetPosition().z - 55.0f) / 60.0f, 0.0f);
                        float NewNear = DEFAULT_NEAR * (1.0f + std::max((TheCamera->GetPosition().z - 55.0f) / 60.0f, 0.0f));
                        if (Scene->m_pRwCamera->nearPlane >= DEFAULT_NEAR)
                            RwCameraSetNearClipPlane(Scene->m_pRwCamera, NewNear);
                    }
                    if (LODDistMultiplier > 2.2f)
                        LODDistMultiplier = 2.2f;
                }
                else
                    LODDistMultiplier = 1.0f;
            });

            //CCam::Process_WheelCam
            pattern = hook::pattern("C7 83 ? ? ? ? ? ? ? ? 8B 93 ? ? ? ? 8A 42 ? 24 ? 3C ? 0F 85");
            injector::MakeNOP(pattern.get_first(), 10, true);
            static auto Process_WheelCamFOV = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                *(float*)(regs.ebx + 0xB0) = CDraw::ConvertFOVInverse(70.0f);
            });
        };
    }
} Main;