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

SafetyHookInline shAvoidTheGeometry = {};
void __fastcall AvoidTheGeometry(CCamera* camera, void* edx, const CVector* Source, const CVector* TargetPos, CVector* NewSource, float FOV)
{
    return shAvoidTheGeometry.unsafe_fastcall(camera, edx, Source, TargetPos, NewSource, CDraw::ConvertFOVInverse(FOV));
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

            pattern = find_pattern("E8 ? ? ? ? 0F B6 0D ? ? ? ? 8A 58");
            FindPlayerPed = (decltype(FindPlayerPed))injector::GetBranchDestination(pattern.get_first()).as_int();

            pattern = find_pattern("E8 ? ? ? ? 85 C0 74 ? ? ? ? ? ? ? ? ? ? ? DF E0");
            FindPlayerVehicle = (decltype(FindPlayerVehicle))injector::GetBranchDestination(pattern.get_first()).as_int();

            //CWeapon::DoBulletImpact (smoke misplacement)
            pattern = hook::pattern("50 ? ? ? ? ? ? ? 6A ? C7 84 24 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? E8 ? ? ? ? 83 C4");
            if (!pattern.empty())
                injector::WriteMemory<uint8_t>(pattern.get_first(), 0x55, true); //push ebp

            pattern = hook::pattern("50 6A ? C7 84 24 ? ? ? ? ? ? ? ? E8 ? ? ? ? 83 C4 ? 8B 84 24");
            if (!pattern.empty())
                injector::WriteMemory<uint8_t>(pattern.get_first(), 0x55, true); //push ebp

            //DoFade
            pattern = hook::pattern("74 ? 8B 1D ? ? ? ? 89 D8");
            injector::WriteMemory<uint8_t>(pattern.get_first(), 0xEB, true);

            //CCam::Process_WheelCam
            pattern = hook::pattern("C7 85 ? ? ? ? ? ? ? ? 8B 95 ? ? ? ? 8A 42");
            injector::MakeNOP(pattern.get_first(), 10, true);
            static auto Process_WheelCamFOV = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                *(float*)(regs.ebp + 0xB4) = CDraw::ConvertFOVInverse(70.0f);
            });

            //CCamera::AvoidTheGeometry
            pattern = hook::pattern("E8 ? ? ? ? D9 EE D9 EE D9 44 24 ? D8 A5 ? ? ? ? DD D9 D9 44 24 ? D8 A5 ? ? ? ? DD DA D9 44 24 ? D8 A5 ? ? ? ? D9 9D ? ? ? ? D9 C1 D9 9D ? ? ? ? D9 95 ? ? ? ? DE D9 8D 8D ? ? ? ? E8 ? ? ? ? C7 85 ? ? ? ? ? ? ? ? C7 85 ? ? ? ? ? ? ? ? C7 85 ? ? ? ? ? ? ? ? 8D 85 ? ? ? ? 8D B5");
            shAvoidTheGeometry = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), AvoidTheGeometry);
        };
    }
} Main;