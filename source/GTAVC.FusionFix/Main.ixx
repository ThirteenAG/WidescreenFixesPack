module;

#include <stdafx.h>
#include "common.h"
#include <algorithm>
#include <array>
#include <cmath>
#include "FixSettings.hpp"
#include "RawMouse.hpp"

export module Main;

import Draw;
import Skeleton;
import Camera;
import Entity;
import CutsceneMgr;
import Sprite2d;

// ScanWorld and RequestObjectsInFrustum project only one pair of corners onto
// the sector grid. When the camera pitches, the other pair can lie outside
// that triangle. Keep the native scanner, but give it the complete footprint.
namespace FrustumScan
{
    inline RwV2d ToSector(const RwV3d& point)
    {
        return { point.x * 0.02f + 48.0f, point.y * 0.02f + 40.0f };
    }

    inline RwV3d Transform(const RwV3d& point, const RwMatrix& matrix)
    {
        return {
            matrix.right.x * point.x + matrix.up.x * point.y + matrix.at.x * point.z + matrix.pos.x,
            matrix.right.y * point.x + matrix.up.y * point.y + matrix.at.y * point.z + matrix.pos.y,
            matrix.right.z * point.x + matrix.up.z * point.y + matrix.at.z * point.z + matrix.pos.z
        };
    }

    inline double Cross(const RwV2d& a, const RwV2d& b, const RwV2d& c)
    {
        return (double(b.x) - a.x) * (double(c.y) - a.y)
             - (double(b.y) - a.y) * (double(c.x) - a.x);
    }

    struct Polygon
    {
        std::array<RwV2d, 5> points{};
        int count = 0;
    };

    inline Polygon ConvexHull(std::array<RwV2d, 5> points)
    {
        for (const auto& point : points)
            if (!std::isfinite(point.x) || !std::isfinite(point.y))
                return {};

        std::sort(points.begin(), points.end(), [](const auto& a, const auto& b)
        {
            return a.x < b.x || (a.x == b.x && a.y < b.y);
        });
        const auto end = std::unique(points.begin(), points.end(), [](const auto& a, const auto& b)
        {
            return a.x == b.x && a.y == b.y;
        });
        const int size = static_cast<int>(end - points.begin());
        if (size < 3)
            return {};

        std::array<RwV2d, 10> hull{};
        int count = 0;
        for (int i = 0; i < size; ++i)
        {
            while (count >= 2 && Cross(hull[count - 2], hull[count - 1], points[i]) <= 0.0)
                --count;
            hull[count++] = points[i];
        }
        const int lowerCount = count;
        for (int i = size - 2; i >= 0; --i)
        {
            while (count > lowerCount && Cross(hull[count - 2], hull[count - 1], points[i]) <= 0.0)
                --count;
            hull[count++] = points[i];
        }
        --count; // the first point is repeated at the end
        if (count < 3)
            return {};

        Polygon result;
        result.count = count;
        std::copy_n(hull.begin(), count, result.points.begin());
        return result;
    }

    struct Footprint
    {
        std::array<RwV2d, 9> anchors{};
        std::array<std::array<RwV2d, 5>, 3> ranges{}; // far, LOD, priority
        bool valid = false;

        void Capture(const std::array<RwV3d, 9>& local, const RwV3d* world, const RwMatrix& matrix)
        {
            valid = false;
            if (!std::isfinite(local[1].z) || local[1].z <= 0.0f || local[5].z <= 0.0f)
                return;

            const float lodScale = local[5].z / local[1].z;
            const float priorityX = local[5].x != 0.0f ? local[7].x / local[5].x : 0.2f;
            const float priorityY = local[5].y != 0.0f ? local[7].y / local[5].y : 0.2f;
            for (size_t i = 0; i < anchors.size(); ++i)
                anchors[i] = ToSector(world[i]);
            for (auto& range : ranges)
                range[0] = anchors[0];

            for (size_t i = 1; i <= 4; ++i)
            {
                ranges[0][i] = anchors[i];
                auto corner = local[i] * lodScale;
                ranges[1][i] = ToSector(Transform(corner, matrix));
                corner.x *= priorityX;
                corner.y *= priorityY;
                ranges[2][i] = ToSector(Transform(corner, matrix));
            }
            // Use the exact native values for the pair that was already
            // transformed; avoid rounding differences at sector boundaries.
            const int left = local[5].y >= 0.0f ? 1 : 4;
            const int right = local[5].y >= 0.0f ? 2 : 3;
            ranges[1][left] = anchors[5];
            ranges[1][right] = anchors[6];
            ranges[2][left] = anchors[7];
            ranges[2][right] = anchors[8];
            valid = true;
        }

        Polygon Expand(const RwV2d* triangle, int count) const
        {
            if (!valid || count != 3)
                return {};
            const auto matches = [](const RwV2d& a, const RwV2d& b)
            {
                return std::abs(a.x - b.x) <= 0.001f && std::abs(a.y - b.y) <= 0.001f;
            };
            if (!matches(triangle[0], anchors[0]))
                return {};

            constexpr int pairs[][2] = { {1, 2}, {5, 6}, {7, 8} };
            for (size_t range = 0; range < ranges.size(); ++range)
            {
                if (!matches(triangle[1], anchors[pairs[range][0]])
                    || !matches(triangle[2], anchors[pairs[range][1]]))
                    continue;

                // ScanSectorPoly expects counterclockwise vertices. The old
                // triangle can even reverse its winding at steep pitches;
                // the hull supplies a consistent order for both scan edges.
                return ConvexHull(ranges[range]);
            }
            return {}; // leave modified or unrelated polygons alone
        }
    };
}

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
        auto directions{ hook::pattern("D9 41 08 D8 1D ? ? ? ? DF E0 F6 C4 45 0F 85 ? ? ? ? D9 41 04 DD DA").count(2) };
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
        auto renderCall = hook::pattern("6A 09 51 50 D9 9C 24 ? ? ? ? DE D9 DD D8 E8 ? ? ? ? 0F B6 05 ? ? ? ? 83 C4 10").get_first<uint8_t>(15);
        auto requestCall = hook::pattern("56 6A 09 51 50 E8 ? ? ? ? 0F B6 05 ? ? ? ? 83 C4 10").get_first<uint8_t>(5);
        const auto transform = injector::GetBranchDestination(renderCall).as_int();
        auto renderScans = hook::range_pattern(reinterpret_cast<uintptr_t>(renderCall), reinterpret_cast<uintptr_t>(renderCall) + 0x700,
            "D9 9C 24 ? ? ? ? E8 ? ? ? ?").count(2);
        auto requestScans = hook::range_pattern(reinterpret_cast<uintptr_t>(requestCall), reinterpret_cast<uintptr_t>(requestCall) + 0x700,
            "D9 9C 24 ? ? ? ? E8 ? ? ? ?").count(1);
        const auto scanner = injector::GetBranchDestination(requestScans.get_first(7)).as_int();

        TransformScanPointsNative = reinterpret_cast<decltype(TransformScanPointsNative)>(transform);
        ScanSectorPolyNative = reinterpret_cast<decltype(ScanSectorPolyNative)>(scanner);
        injector::MakeCALL(renderCall, TransformScanPoints, true);
        injector::MakeCALL(requestCall, TransformScanPoints, true);
        for (size_t i = 0; i < 2; ++i)
            injector::MakeCALL(renderScans.get(i).get<void>(7), ScanSectorPoly, true);
        injector::MakeCALL(requestScans.get_first(7), ScanSectorPoly, true);
    }
}


class Main
{
public:
    Main()
    {
        WFP::onGameInitEvent() += []()
        {
            if (!GetModuleHandleW(L"GInputVC.asi"))
                GtaRawMouse::Install(*reinterpret_cast<HWND*>(RsGlobal->ps));
        };

        WFP::onInitEvent() += []()
        {
            // default res
            auto [x, y] = GetDesktopRes();
            auto pattern = hook::pattern("81 3C ? ? ? ? ? 75 ? 81 7C 24 ? ? ? ? ? 75 ? 83 7C 24 ? ? 75 ? 8B 44 24 ? 83 E0 ? 75");
            injector::WriteMemory<int32_t>(pattern.get_first(3), x, true);
            injector::WriteMemory<int32_t>(pattern.get_first(13), y, true);
            injector::MakeNOP(pattern.get_first(24), 2, true);

            // Every video mode of 640x480 or more in the resolution list, whatever the video memory the game finds
            pattern = hook::pattern("29 C1 81 F9 00 00 C0 00 76");
            injector::MakeNOP(pattern.get_first(8), 2, true);

            // Skip Intro: start with the loading screen and the main menu, without the logo videos. Each video
            // initializes COM and the state after it releases it, so the release that follows the last one is paired here
            if (FixSettings().Bool("MAIN.SkipIntro"))
            {
                pattern = hook::pattern("C7 05 ? ? ? ? ? 00 00 00 E9 ? ? ? ? 83 3D ? ? ? ? 00 75 0D");
                GameRef<int32_t> gGameState(*pattern.get_first<int32_t*>(2));
                if (*gGameState == 0)
                {
                    CoInitialize(nullptr);
                    *gGameState = 5;
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
            shCameraSize = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), CameraSize);

            pattern = hook::pattern("A3 ? ? ? ? 75 ? A1 ? ? ? ? 50");
            Scene.SetAddress(*pattern.get_first<CScene*>(1));

            pattern = find_pattern("E8 ? ? ? ? 0F B6 0D ? ? ? ? 8A 58");
            FindPlayerPed = (decltype(FindPlayerPed))injector::GetBranchDestination(pattern.get_first()).as_int();

            pattern = find_pattern("E8 ? ? ? ? 85 C0 74 ? ? ? ? ? ? ? ? ? ? ? DF E0");
            FindPlayerVehicle = (decltype(FindPlayerVehicle))injector::GetBranchDestination(pattern.get_first()).as_int();

            // RwCameraSetNearClipPlane
            pattern = hook::pattern("D9 44 24 08 56 8B 74 24 08 56 D9 9E 80");
            RwCameraSetNearClipPlane = (decltype(RwCameraSetNearClipPlane))pattern.get_first();

            //CWeapon::DoBulletImpact (smoke misplacement)
            pattern = hook::pattern("50 ? ? ? ? ? ? ? 6A ? C7 84 24 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? E8 ? ? ? ? 83 C4");

                injector::WriteMemory<uint8_t>(pattern.get_first(), 0x55, true); //push ebp

            pattern = hook::pattern("50 6A ? C7 84 24 ? ? ? ? ? ? ? ? E8 ? ? ? ? 83 C4 ? 8B 84 24");

                injector::WriteMemory<uint8_t>(pattern.get_first(), 0x55, true); //push ebp

            //DoFade
            pattern = hook::pattern("E8 ? ? ? ? 8D 44 24 ? 8D 54 24 ? 52");
            hbGetScreenRect.fun = injector::MakeCALL(pattern.get_first(), GetScreenRect, true).get();

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
