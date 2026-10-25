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

SafetyHookInline shAvoidTheGeometry = {};
void __fastcall AvoidTheGeometry(CCamera* camera, void* edx, const CVector* Source, const CVector* TargetPos, CVector* NewSource, float FOV)
{
    return shAvoidTheGeometry.unsafe_fastcall(camera, edx, Source, TargetPos, NewSource, CDraw::ConvertFOVInverse(FOV));
}

injector::hook_back<void(__fastcall*)(CCamera*, void*, CRect*)> hbGetScreenRect;
void __fastcall GetScreenRect(CCamera* camera, void*, CRect* rect)
{
    *rect = CRect(0.0f, SCREEN_HEIGHT, SCREEN_WIDTH, 0.0f);

    if (TheCamera->m_WideScreenOn && AreCutsceneBordersVisible())
        *rect = GetCurrentCutsceneContentRect();
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
        // Both opaque and transparent water use the same compass half-plane shortcut.
        // Scan both halves and retain the native per-tile frustum tests.
        auto directions{ hook::pattern("D9 41 08 D8 1D ? ? ? ? DF E0 F6 C4 45 0F 85 ? ? ? ? D9 41 04 DD DA") };
        if (directions.size() != 2)
            return;

        for (size_t i{}; i < directions.size(); ++i)
        {
            auto branch{ directions.get(i).get<uint8_t>(14) };
            auto target{ branch + 6 + *reinterpret_cast<int32_t*>(branch + 2) };
            injector::MakeJMP(branch, target, true);
            injector::MakeNOP(branch + 5, 1, true);
        }
    }

    void InitFrustumScan()
    {
        auto render = hook::pattern("6A 09 51 50 D9 9C 24 ? ? ? ? DE D9 DD D8 E8 ? ? ? ? 0F B6 05 ? ? ? ? 83 C4 10");
        auto request = hook::pattern("56 6A 09 51 50 E8 ? ? ? ? 0F B6 05 ? ? ? ? 83 C4 10");
        if (render.size() != 1 || request.size() != 1)
            return;

        auto renderCall = render.get_first<uint8_t>(15);
        auto requestCall = request.get_first<uint8_t>(5);
        const auto transform = injector::GetBranchDestination(renderCall).as_int();
        if (transform != injector::GetBranchDestination(requestCall).as_int())
            return;

        // Validate the render and streaming scans before changing their shared footprint.
        auto scans = hook::pattern("D9 9C 24 ? ? ? ? E8 ? ? ? ?");
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
        if (renderScans.size() != 2 || requestScans.size() != 1)
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
                wfp::patterns::Report("GTAVC.WidescreenFix: feature signature unavailable.\n");
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
            // resulting path against a fixed 150 units. Wider spawn radii can
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
                wfp::patterns::Report("GTAVC.WidescreenFix: feature signature unavailable.\n");
                return;
            }
            shCameraSize = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), CameraSize);

            pattern = hook::pattern("A3 ? ? ? ? 75 ? A1 ? ? ? ? 50");
            if (pattern.size() < 1)
            {
                wfp::patterns::Report("GTAVC.WidescreenFix: feature signature unavailable.\n");
                return;
            }
            Scene.SetAddress(*pattern.get_first<CScene*>(1));

            pattern = find_pattern("E8 ? ? ? ? 0F B6 0D ? ? ? ? 8A 58");
            if (pattern.size() < 1)
            {
                wfp::patterns::Report("GTAVC.WidescreenFix: feature signature unavailable.\n");
                return;
            }
            FindPlayerPed = (decltype(FindPlayerPed))injector::GetBranchDestination(pattern.get_first()).as_int();

            pattern = find_pattern("E8 ? ? ? ? 85 C0 74 ? ? ? ? ? ? ? ? ? ? ? DF E0");
            if (pattern.size() < 1)
            {
                wfp::patterns::Report("GTAVC.WidescreenFix: feature signature unavailable.\n");
                return;
            }
            FindPlayerVehicle = (decltype(FindPlayerVehicle))injector::GetBranchDestination(pattern.get_first()).as_int();

            //CWeapon::DoBulletImpact (smoke misplacement)
            pattern = hook::pattern("50 ? ? ? ? ? ? ? 6A ? C7 84 24 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? E8 ? ? ? ? 83 C4");
            if (!pattern.empty())
                injector::WriteMemory<uint8_t>(pattern.get_first(), 0x55, true); //push ebp

            pattern = hook::pattern("50 6A ? C7 84 24 ? ? ? ? ? ? ? ? E8 ? ? ? ? 83 C4 ? 8B 84 24");
            if (!pattern.empty())
                injector::WriteMemory<uint8_t>(pattern.get_first(), 0x55, true); //push ebp

            //DoFade
            pattern = hook::pattern("E8 ? ? ? ? 8D 44 24 ? 8D 54 24 ? 52");
            if (pattern.size() < 1)
            {
                wfp::patterns::Report("GTAVC.WidescreenFix: feature signature unavailable.\n");
                return;
            }
            hbGetScreenRect.fun = injector::MakeCALL(pattern.get_first(), GetScreenRect, true).get();

            //CCam::Process_WheelCam
            pattern = hook::pattern("C7 85 ? ? ? ? ? ? ? ? 8B 95 ? ? ? ? 8A 42");
            if (pattern.size() < 1)
            {
                wfp::patterns::Report("GTAVC.WidescreenFix: feature signature unavailable.\n");
                return;
            }
            injector::MakeNOP(pattern.get_first(), 10, true);
            static auto Process_WheelCamFOV = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                *(float*)(regs.ebp + 0xB4) = CDraw::ConvertFOVInverse(70.0f);
            });

            //CCamera::AvoidTheGeometry
            pattern = hook::pattern("E8 ? ? ? ? D9 EE D9 EE D9 44 24 ? D8 A5 ? ? ? ? DD D9 D9 44 24 ? D8 A5 ? ? ? ? DD DA D9 44 24 ? D8 A5 ? ? ? ? D9 9D ? ? ? ? D9 C1 D9 9D ? ? ? ? D9 95 ? ? ? ? DE D9 8D 8D ? ? ? ? E8 ? ? ? ? C7 85 ? ? ? ? ? ? ? ? C7 85 ? ? ? ? ? ? ? ? C7 85 ? ? ? ? ? ? ? ? 8D 85 ? ? ? ? 8D B5");
            if (pattern.size() < 1)
            {
                wfp::patterns::Report("GTAVC.WidescreenFix: feature signature unavailable.\n");
                return;
            }
            shAvoidTheGeometry = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), AvoidTheGeometry);
        };
    }
} Main;
