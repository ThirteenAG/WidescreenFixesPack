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
        return { point.x * 0.025f + 50.0f, point.y * 0.025f + 50.0f };
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
        auto branch{ hook::pattern("D9 46 08 D8 1D ? ? ? ? DF E0 F6 C4 45 0F 85 ? ? ? ? D9 46 04 DD DA").get_first<uint8_t>(14) };
        auto target{ branch + 6 + *reinterpret_cast<int32_t*>(branch + 2) };
        injector::MakeJMP(branch, target, true);
        injector::MakeNOP(branch + 5, 1, true);
    }

    void InitFrustumScan()
    {
        auto render = hook::pattern("6A 09 51 50 D9 9C 24 ? ? ? ? DE D9 E8 ? ? ? ? 0F B6 05 ? ? ? ? 83 C4 10");
        auto request = hook::pattern("56 6A 09 51 50 E8 ? ? ? ? 0F B6 05 ? ? ? ? 83 C4 10");
        auto renderCall = render.get_first<uint8_t>(13);
        auto requestCall = request.get_first<uint8_t>(5);
        const auto transform = injector::GetBranchDestination(renderCall).as_int();
        // The three render scans and the streaming scan share the native scanner.
        auto renderScans = hook::range_pattern(reinterpret_cast<uintptr_t>(renderCall), reinterpret_cast<uintptr_t>(renderCall) + 0x700,
            "D9 9C 24 A8 00 00 00 E8 ? ? ? ?").count(3);
        auto requestScans = hook::range_pattern(reinterpret_cast<uintptr_t>(requestCall), reinterpret_cast<uintptr_t>(requestCall) + 0x700,
            "D9 9C 24 A8 00 00 00 E8 ? ? ? ?").count(1);
        const auto scanner = injector::GetBranchDestination(requestScans.get_first(7)).as_int();

        TransformScanPointsNative = reinterpret_cast<decltype(TransformScanPointsNative)>(transform);
        ScanSectorPolyNative = reinterpret_cast<decltype(ScanSectorPolyNative)>(scanner);
        injector::MakeCALL(renderCall, TransformScanPoints, true);
        injector::MakeCALL(requestCall, TransformScanPoints, true);
        for (size_t i = 0; i < 3; ++i)
            injector::MakeCALL(renderScans.get(i).get<void>(7), ScanSectorPoly, true);
        injector::MakeCALL(requestScans.get_first(7), ScanSectorPoly, true);
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
        WFP::onGameInitEvent() += []()
        {
            if (!GetModuleHandleW(L"GInputIII.asi"))
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
            // resulting path against a fixed 160 units. Wider spawn radii can
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

            pattern = find_pattern("E8 ? ? ? ? 8B 80 ? ? ? ? 8B 50 ? 83 FA ? 7E");
            FindPlayerPed = (decltype(FindPlayerPed))injector::GetBranchDestination(pattern.get_first()).as_int();

            pattern = find_pattern("E8 ? ? ? ? 85 C0 74 ? ? ? ? ? ? ? ? ? ? DF E0");
            FindPlayerVehicle = (decltype(FindPlayerVehicle))injector::GetBranchDestination(pattern.get_first()).as_int();

            pattern = find_pattern("E8 ? ? ? ? 59 59 83 C4 ? 5D 5F 5E 5B C3 ? ? EB");
            RwCameraSetNearClipPlane = (decltype(RwCameraSetNearClipPlane))injector::GetBranchDestination(pattern.get_first()).as_int();

            pattern = hook::pattern("E8 ? ? ? ? 80 3D ? ? ? ? ? 74 ? 81 3D");
            static auto Idle1 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                RWGTA::GameThread::Run();
                CDraw::SetAspectRatio(CDraw::FindAspectRatio());
            });

            pattern = hook::pattern("83 3D ? ? ? ? ? ? ? 74 ? 83 C4");
            static auto FrontendIdle1 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                RWGTA::GameThread::Run();
                CDraw::SetAspectRatio(CDraw::FindAspectRatio());
            });

            //DoFade
            pattern = hook::pattern("E8 ? ? ? ? 80 3D ? ? ? ? ? 59 59 74 ? 80 3D");
            hbDoFadeDrawRect.fun = injector::MakeCALL(pattern.get_first(), DoFadeDrawRect, true).get();

            //CWeapon::DoBulletImpact (smoke misplacement)
            pattern = hook::pattern("50 ? ? ? ? ? ? ? 6A ? C7 84 24 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? E8 ? ? ? ? 83 C4");

                injector::WriteMemory<uint8_t>(pattern.get_first(), 0x55, true); //push ebp

            pattern = hook::pattern("50 6A ? C7 84 24 ? ? ? ? ? ? ? ? E8 ? ? ? ? 8B 84 24");

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
            pattern = hook::pattern("D9 83 ? ? ? ? D9 9B ? ? ? ? D9 83 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? 80 7B ? ? 74 ? 80 7B ? ? 75");
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
            injector::MakeNOP(pattern.get_first(), 10, true);
            static auto Process_WheelCamFOV = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                *(float*)(regs.ebx + 0xB0) = CDraw::ConvertFOVInverse(70.0f);
            });
        };
    }
} Main;
