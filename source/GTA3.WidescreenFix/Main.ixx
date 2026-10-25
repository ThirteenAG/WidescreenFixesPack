module;

#include <stdafx.h>
#include "../Shared/CheckedPattern.hpp"
#include "common.h"
#include "FrustumScan.hpp"
#include "FixSettings.hpp"

export module Main;

import Draw;
import Skeleton;
import Camera;
import Entity;
import CutsceneMgr;
import Sprite2d;

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

namespace
{
    using SectorCallback = void(__cdecl*)(void*);
    RwV3d* (__cdecl* TransformScanPointsNative)(RwV3d*, const RwV3d*, int32_t, const RwMatrix*) = nullptr;
    void (__cdecl* ScanSectorPolyNative)(RwV2d*, int32_t, SectorCallback) = nullptr;
    thread_local FrustumScan::Footprint scanFootprint;

    RwV3d* __cdecl TransformScanPoints(RwV3d* out, const RwV3d* in, int32_t count, const RwMatrix* matrix)
    {
        scanFootprint.valid = false;
        if (count != 9)
            return TransformScanPointsNative(out, in, count, matrix);

        // The native transform is in-place. Capture its camera-space inputs
        // before they are overwritten, and retain its authoritative anchors.
        std::array<RwV3d, 9> local;
        std::copy_n(in, local.size(), local.begin());
        auto result = TransformScanPointsNative(out, in, count, matrix);
        scanFootprint.Capture(local, out, *matrix);
        return result;
    }

    void __cdecl ScanSectorPoly(RwV2d* points, int32_t count, SectorCallback callback)
    {
        auto polygon = scanFootprint.Expand(points, count);
        if (polygon.count >= 3)
            ScanSectorPolyNative(polygon.points.data(), polygon.count, callback);
        else
            ScanSectorPolyNative(points, count, callback);
    }

    void InitWaterFrustum()
    {
        // A pitched or wide view can cross the camera's compass half-plane.
        // Scan both halves and retain the native per-tile frustum tests.
        auto direction{ hook::pattern("D9 46 08 D8 1D ? ? ? ? DF E0 F6 C4 45 0F 85 ? ? ? ? D9 46 04 DD DA") };
        if (direction.size() != 1)
            return;

        auto branch{ direction.get_first<uint8_t>(14) };
        auto target{ branch + 6 + *reinterpret_cast<int32_t*>(branch + 2) };
        injector::MakeJMP(branch, target, true);
        injector::MakeNOP(branch + 5, 1, true);
    }

    void InitFrustumScan()
    {
        auto render = hook::pattern("6A 09 51 50 D9 9C 24 ? ? ? ? DE D9 E8 ? ? ? ? 0F B6 05 ? ? ? ? 83 C4 10");
        auto request = hook::pattern("56 6A 09 51 50 E8 ? ? ? ? 0F B6 05 ? ? ? ? 83 C4 10");
        if (render.size() != 1 || request.size() != 1)
            return;

        auto renderCall = render.get_first<uint8_t>(13);
        auto requestCall = request.get_first<uint8_t>(5);
        const auto transform = injector::GetBranchDestination(renderCall).as_int();
        if (transform != injector::GetBranchDestination(requestCall).as_int())
            return;

        // These four calls belong to ScanWorld (subway, priority, regular)
        // and RequestObjectsInFrustum. Validate them all before patching any.
        auto scans = hook::pattern("D9 9C 24 A8 00 00 00 E8 ? ? ? ?");
        std::vector<uint8_t*> renderScans, requestScans;
        for (size_t i = 0; i < scans.size(); ++i)
        {
            auto call = scans.get(i).get<uint8_t>(7);
            const auto address = reinterpret_cast<uintptr_t>(call);
            if (address > reinterpret_cast<uintptr_t>(renderCall)
                && address < reinterpret_cast<uintptr_t>(renderCall) + 0x700)
                renderScans.push_back(call);
            if (address > reinterpret_cast<uintptr_t>(requestCall)
                && address < reinterpret_cast<uintptr_t>(requestCall) + 0x700)
                requestScans.push_back(call);
        }
        if (renderScans.size() != 3 || requestScans.size() != 1)
            return;

        const auto scanner = injector::GetBranchDestination(requestScans.front()).as_int();
        for (auto call : renderScans)
            if (injector::GetBranchDestination(call).as_int() != scanner)
                return;

        TransformScanPointsNative = reinterpret_cast<decltype(TransformScanPointsNative)>(transform);
        ScanSectorPolyNative = reinterpret_cast<decltype(ScanSectorPolyNative)>(scanner);
        injector::MakeCALL(renderCall, TransformScanPoints, true);
        injector::MakeCALL(requestCall, TransformScanPoints, true);
        for (auto call : renderScans)
            injector::MakeCALL(call, ScanSectorPoly, true);
        injector::MakeCALL(requestScans.front(), ScanSectorPoly, true);
    }
}

injector::hook_back<void(__cdecl*)(const CRect*, const CRGBA*)> hbDoFadeDrawRect;
void __cdecl DoFadeDrawRect(const CRect* r, const CRGBA* col)
{
    CRect rect = CRect(0.0f, SCREEN_HEIGHT, SCREEN_WIDTH, 0.0f);

    if (TheCamera->m_WideScreenOn && AreCutsceneBordersVisible())
        rect = GetCurrentCutsceneContentRect();

    return hbDoFadeDrawRect.fun(&rect, col);
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
            if (pattern.size() < 1)
            {
                wfp::patterns::Report("GTA3.WidescreenFix: feature signature unavailable.\n");
                return;
            }
            injector::WriteMemory<int32_t>(pattern.get_first(3), x, true);
            injector::WriteMemory<int32_t>(pattern.get_first(13), y, true);
            injector::MakeNOP(pattern.get_first(24), 2, true);

            // Every video mode of 640x480 or more in the resolution list, whatever the video memory the game finds
            pattern = hook::pattern("29 C1 81 F9 00 00 C0 00 76");
            if (!pattern.empty())
                injector::MakeNOP(pattern.get_first(8), 2, true);

            // Skip Intro: start with the loading screen and the main menu, without the logo videos. Each video
            // initializes COM and the state after it releases it, so the release that follows the last one is paired here
            if (FixSettings().Bool("MAIN.SkipIntro"))
            {
                pattern = hook::pattern("C7 05 ? ? ? ? 01 00 00 00 E9 ? ? ? ? 83 3D ? ? ? ? 00 75 0D");
                if (!pattern.empty())
                {
                    auto gGameState = *pattern.get_first<int32_t*>(2);
                    if (*gGameState == 0)
                    {
                        CoInitialize(nullptr);
                        *gGameState = 5;
                    }
                }
            }
        };

        WFP::onGameInitEvent() += []()
        {
            InitFrustumScan();
            InitWaterFrustum();

            // GenerateOneRandomCar scales its spawn radius by the camera's
            // GenerationDistMultiplier, but TestCoorsCloseness compares the
            // resulting path against a fixed 160 units. Wider spawn radii can
            // exceed that limit and reject otherwise valid traffic.
            // Normalize only the car path distance before the native comparison;
            // keep pedestrian paths, traffic budgets and visibility tests intact.
            {
                auto pathCheck = hook::pattern("84 DB 75 1D D9 44 24 04 D8 1D ? ? ? ? DF E0 80 E4 05 80 FC 01 75 ? 83 C4 08 B0 01 5B C2 1C 00");
                if (pathCheck.size() == 1)
                {
                    static auto TrafficPathDistanceFix = safetyhook::create_mid(pathCheck.get_first(4), [](SafetyHookContext& regs)
                    {
                        const float multiplier = TheCamera->GenerationDistMultiplier;
                        if (std::isfinite(multiplier) && multiplier > 0.0f)
                            *reinterpret_cast<float*>(regs.esp + 4) /= multiplier;
                    });
                }
            }

            auto pattern = hook::pattern("E8 ? ? ? ? C7 44 24 ? ? ? ? ? 8B 54 24");
            if (pattern.size() < 1)
            {
                wfp::patterns::Report("GTA3.WidescreenFix: feature signature unavailable.\n");
                return;
            }
            shCameraSize = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), CameraSize);

            pattern = hook::pattern("A3 ? ? ? ? 75 ? A1 ? ? ? ? 50");
            if (pattern.size() < 1)
            {
                wfp::patterns::Report("GTA3.WidescreenFix: feature signature unavailable.\n");
                return;
            }
            Scene.SetAddress(*pattern.get_first<CScene*>(1));

            pattern = find_pattern("E8 ? ? ? ? 8B 80 ? ? ? ? 8B 50 ? 83 FA ? 7E");
            if (pattern.size() < 1)
            {
                wfp::patterns::Report("GTA3.WidescreenFix: feature signature unavailable.\n");
                return;
            }
            FindPlayerPed = (decltype(FindPlayerPed))injector::GetBranchDestination(pattern.get_first()).as_int();

            pattern = find_pattern("E8 ? ? ? ? 85 C0 74 ? ? ? ? ? ? ? ? ? ? DF E0");
            if (pattern.size() < 1)
            {
                wfp::patterns::Report("GTA3.WidescreenFix: feature signature unavailable.\n");
                return;
            }
            FindPlayerVehicle = (decltype(FindPlayerVehicle))injector::GetBranchDestination(pattern.get_first()).as_int();

            pattern = find_pattern("E8 ? ? ? ? 59 59 83 C4 ? 5D 5F 5E 5B C3 ? ? EB");
            if (pattern.size() < 1)
            {
                wfp::patterns::Report("GTA3.WidescreenFix: feature signature unavailable.\n");
                return;
            }
            RwCameraSetNearClipPlane = (decltype(RwCameraSetNearClipPlane))injector::GetBranchDestination(pattern.get_first()).as_int();

            pattern = hook::pattern("E8 ? ? ? ? 80 3D ? ? ? ? ? 74 ? 81 3D");
            if (pattern.size() < 1)
            {
                wfp::patterns::Report("GTA3.WidescreenFix: feature signature unavailable.\n");
                return;
            }
            static auto Idle1 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                RWGTA::GameThread::Run();
                CDraw::SetAspectRatio(CDraw::FindAspectRatio());
            });

            pattern = hook::pattern("83 3D ? ? ? ? ? ? ? 74 ? 83 C4");
            if (pattern.size() < 1)
            {
                wfp::patterns::Report("GTA3.WidescreenFix: feature signature unavailable.\n");
                return;
            }
            static auto FrontendIdle1 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                RWGTA::GameThread::Run();
                CDraw::SetAspectRatio(CDraw::FindAspectRatio());
            });

            //DoFade
            pattern = hook::pattern("E8 ? ? ? ? 80 3D ? ? ? ? ? 59 59 74 ? 80 3D");
            if (pattern.size() < 1)
            {
                wfp::patterns::Report("GTA3.WidescreenFix: feature signature unavailable.\n");
                return;
            }
            hbDoFadeDrawRect.fun = injector::MakeCALL(pattern.get_first(), DoFadeDrawRect, true).get();

            //CWeapon::DoBulletImpact (smoke misplacement)
            pattern = hook::pattern("50 ? ? ? ? ? ? ? 6A ? C7 84 24 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? E8 ? ? ? ? 83 C4");
            if (!pattern.empty())
                injector::WriteMemory<uint8_t>(pattern.get_first(), 0x55, true); //push ebp

            pattern = hook::pattern("50 6A ? C7 84 24 ? ? ? ? ? ? ? ? E8 ? ? ? ? 8B 84 24");
            if (!pattern.empty())
                injector::WriteMemory<uint8_t>(pattern.get_first(), 0x55, true); //push ebp

            //FireInstantHit doesn't set the target coords
            pattern = hook::pattern("C6 05 ? ? ? ? ? ? ? 8D B4 24");
            if (pattern.size() < 1)
            {
                wfp::patterns::Report("GTA3.WidescreenFix: feature signature unavailable.\n");
                return;
            }
            static auto FireInstantHitBugfix = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                auto& target = *(CVector*)(regs.esp + 0x84);
                auto& pPoint = *(CVector*)(regs.esp + 0xF4);
                target = pPoint;
            });

            //CCamera::IsSphereVisible
            pattern = hook::pattern("E8 ? ? ? ? ? ? ? ? ? ? ? ? ? ? 83 C4 ? ? ? ? ? ? ? ? ? ? ? ? ? DF E0 80 E4 ? 80 FC ? 75 ? 30 DB");
            if (pattern.size() < 1)
            {
                wfp::patterns::Report("GTA3.WidescreenFix: feature signature unavailable.\n");
                return;
            }
            injector::MakeCALL(pattern.get_first(), RwV3dTransformPointsHook, true);

            //CEntity::GetIsOnScreenComplex
            pattern = hook::pattern("E8 ? ? ? ? ? ? ? ? ? ? ? ? ? ? 83 C4 ? ? ? ? ? ? ? ? ? DF E0");
            if (pattern.size() < 1)
            {
                wfp::patterns::Report("GTA3.WidescreenFix: feature signature unavailable.\n");
                return;
            }
            injector::MakeCALL(pattern.get_first(), RwV3dTransformPointsHook, true);

            pattern = hook::pattern("E8 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? 31 D2 83 C4");
            if (pattern.size() < 1)
            {
                wfp::patterns::Report("GTA3.WidescreenFix: feature signature unavailable.\n");
                return;
            }
            injector::MakeCALL(pattern.get_first(), RwV3dTransformPointsHook, true);

            //CCamera::Process
            pattern = hook::pattern("D9 83 ? ? ? ? D9 9B ? ? ? ? D9 83 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? 80 7B ? ? 74 ? 80 7B ? ? 75");
            if (pattern.size() < 1)
            {
                wfp::patterns::Report("GTA3.WidescreenFix: feature signature unavailable.\n");
                return;
            }
            static auto LODDistFix = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                auto& LODDistMultiplier = *(float*)(regs.ebx + 0xEC);

                if (!CCutsceneMgr::IsRunning() || CCutsceneMgr::ms_useLodMultiplier)
                {
                    LODDistMultiplier = (70.0f * CDraw::GetAspectRatio() / (4.0f / 3.0f)) / CDraw::GetFOV();

                    //if ((TheCamera->GetPosition().z > 55.0f && FindPlayerVehicle()))
                    //{
                    //    constexpr float DEFAULT_NEAR = 0.9f;
                    //    LODDistMultiplier *= 1.0f + std::max((TheCamera->GetPosition().z - 55.0f) / 60.0f, 0.0f);
                    //    float NewNear = DEFAULT_NEAR * (1.0f + std::max((TheCamera->GetPosition().z - 55.0f) / 60.0f, 0.0f));
                    //    if (Scene->m_pRwCamera->nearPlane >= DEFAULT_NEAR)
                    //        RwCameraSetNearClipPlane(Scene->m_pRwCamera, NewNear);
                    //}
                    if (LODDistMultiplier > 2.2f)
                        LODDistMultiplier = 2.2f;
                }
                else
                    LODDistMultiplier = 1.0f;
            });

            // The native code copies the LOD multiplier to the population
            // multiplier. Its aspect-ratio factor sends traffic to a larger
            // spawn/despawn ring without increasing the local traffic budget.
            // Keep the wider rendering range, but use the original zoom-based
            // population range after that copy and before renderer LOD scaling.
            static auto GenerationDistFix = safetyhook::create_mid(pattern.get_first(12), [](SafetyHookContext& regs)
            {
                auto& GenerationDistMultiplier = *(float*)(regs.ebx + 0xF0);
                if (!CCutsceneMgr::IsRunning() || CCutsceneMgr::ms_useLodMultiplier)
                    GenerationDistMultiplier = std::min(70.0f / CDraw::GetFOV(), 2.2f);
                else
                    GenerationDistMultiplier = 1.0f;
            });

            //CCam::Process_WheelCam
            pattern = hook::pattern("C7 83 ? ? ? ? ? ? ? ? 8B 93 ? ? ? ? 8A 42 ? 24 ? 3C ? 0F 85");
            if (pattern.size() < 1)
            {
                wfp::patterns::Report("GTA3.WidescreenFix: feature signature unavailable.\n");
                return;
            }
            injector::MakeNOP(pattern.get_first(), 10, true);
            static auto Process_WheelCamFOV = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                *(float*)(regs.ebx + 0xB0) = CDraw::ConvertFOVInverse(70.0f);
            });
        };
    }
} Main;
