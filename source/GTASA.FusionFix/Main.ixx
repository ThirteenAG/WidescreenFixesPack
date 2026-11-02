module;

#include <stdafx.h>
#include "common.h"
#include "FixSettings.hpp"
#include "RawMouse.hpp"
#include <algorithm>
#include <array>
#include <cmath>

export module Main;

import Draw;
import Skeleton;
import Entity;
import CutsceneMgr;

namespace WaterFrustum
{
    inline double Cross(const RwV2d& a, const RwV2d& b, const RwV2d& c)
    {
        return (double(b.x) - a.x) * (double(c.y) - a.y)
             - (double(b.y) - a.y) * (double(c.x) - a.x);
    }

    struct Polygon
    {
        std::array<RwV2d, 5> points{};
        int count{};
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
        const auto end{ std::unique(points.begin(), points.end(), [](const auto& a, const auto& b)
        {
            return a.x == b.x && a.y == b.y;
        }) };
        const int size{ static_cast<int>(end - points.begin()) };
        if (size < 3)
            return {};

        std::array<RwV2d, 10> hull{};
        int count{};
        for (int i{}; i < size; ++i)
        {
            while (count >= 2 && Cross(hull[count - 2], hull[count - 1], points[i]) <= 0.0)
                --count;
            hull[count++] = points[i];
        }
        const int lowerCount{ count };
        for (int i{ size - 2 }; i >= 0; --i)
        {
            while (count > lowerCount && Cross(hull[count - 2], hull[count - 1], points[i]) <= 0.0)
                --count;
            hull[count++] = points[i];
        }
        --count; // the first point is repeated at the end
        if (count < 3)
            return {};

        Polygon result{};
        result.count = count;
        std::copy_n(hull.begin(), count, result.points.begin());
        return result;
    }

    inline bool Intersects(const Polygon& polygon, int x, int y)
    {
        for (int i{}; i < polygon.count; ++i)
        {
            const auto& a{ polygon.points[i] };
            const auto& b{ polygon.points[(i + 1) % polygon.count] };
            const RwV2d corner{ float(x + (b.y < a.y)), float(y + (b.x > a.x)) };
            if (Cross(a, b, corner) < -0.000001)
                return false;
        }
        return true;
    }
}

export GameRef<CScene> Scene;

SafetyHookInline shCameraSize = {};
void __cdecl CameraSize(RwCamera* camera, RwRect* rect, float viewWindow, float aspectRatio)
{
    return shCameraSize.unsafe_ccall(camera, rect, SCREEN_VIEWWINDOW, SCREEN_ASPECT_RATIO);
}

namespace
{
    using WaterCallback = void(__cdecl*)(int, int);
    char (__cdecl* WaterScanNative)(RwV2d*, int, WaterCallback){ nullptr };

    char __cdecl ScanWater(RwV2d* points, int count, WaterCallback callback)
    {
        if (count != 5)
            return WaterScanNative(points, count, callback);
        std::array<RwV2d, 5> input{};
        std::copy_n(points, input.size(), input.begin());
        const auto polygon{ WaterFrustum::ConvexHull(input) };
        if (polygon.count < 3)
            return WaterScanNative(points, count, callback);

        float minX{ input[0].x }, maxX{ minX }, minY{ input[0].y }, maxY{ minY };
        for (const auto& point : input)
        {
            minX = std::min(minX, point.x);
            maxX = std::max(maxX, point.x);
            minY = std::min(minY, point.y);
            maxY = std::max(maxY, point.y);
        }
        // Outside-water coordinates are stored in signed words by the game.
        if (minX < -32768.0f || minY < -32768.0f || maxX >= 32767.0f || maxY >= 32767.0f)
            return WaterScanNative(points, count, callback);

        // Include every intersected cell, even when only a corner crosses the footprint.
        for (int x{ int(std::floor(minX)) }; x <= int(std::floor(maxX)); ++x)
            for (int y{ int(std::floor(minY)) }; y <= int(std::floor(maxY)); ++y)
                if (WaterFrustum::Intersects(polygon, x, y))
                    callback(x, y);
        return 0;
    }

    void InitWaterFrustum()
    {
        auto scan{ hook::pattern("D9 44 24 78 D8 0D ? ? ? ? D8 05 ? ? ? ? D9 5C 24 40 E8 ? ? ? ? 81 C4 80 00 00 00 C3") };
        auto call{ scan.get_first<uint8_t>(20) };
        WaterScanNative = reinterpret_cast<decltype(WaterScanNative)>(injector::GetBranchDestination(call).as_int());
        injector::MakeCALL(call, ScanWater, true);
    }
}

namespace CEntryExitManager
{
    GameRef<int> ms_exitEnterState([]() -> int*
    {
        auto pattern = find_pattern("A1 ? ? ? ? 85 C0 75 ? 33 C0 8A 46");
        return *pattern.get_first<int*>(1);
    });
}

class Main
{
public:
    Main()
    {
        WFP::onGameInitEvent() += []()
        {
            if (!GetModuleHandleW(L"GInputSA.asi"))
                GtaRawMouse::Install(*reinterpret_cast<HWND*>(RsGlobal->ps));
        };

        WFP::onInitEvent() += []()
        {
            // Unlock resolutions. SA-MP rewrites these blocks, so the aspect ratio jump changes only while it is the
            // game's jnp (0x7B) after its fnstsw ax; test ah, 44h
            auto skipAspectCheck = [](uint8_t* jump)
            {
                if (!memcmp(jump - 5, "\xDF\xE0\xF6\xC4\x44", 5) && *jump == 0x7B)
                    injector::WriteMemory<uint8_t>(jump, 0xEB, true);
            };

            // Advanced Display Options (GetVideoModeList): width, height, aspect ratio and video memory checks
            // SilentPatch already NOPs the branch slots. The comparisons and loads identify both layouts.
            auto pattern = hook::pattern("81 FA 80 02 00 00 DA 74 24 ? D9 5C 24 ? ? ? ? ? ? ? 8B 4C 24 ? 81 F9 E0 01 00 00");
            injector::MakeNOP(pattern.get_first(14), 6, true);
            injector::MakeNOP(pattern.get_first(30), 6, true);
            skipAspectCheck(pattern.get_first<uint8_t>(51));
            pattern = hook::pattern("2B C8 81 F9 00 00 D0 02");
            injector::MakeNOP(pattern.get_first(8), 2, true);

            // Resolution selection dialog: width, height and aspect ratio checks
            pattern = hook::pattern("81 7C 24 ? 80 02 00 00 ? ? ? ? ? ? 81 7C 24 ? E0 01 00 00");
            injector::MakeNOP(pattern.get_first(8), 6, true);
            injector::MakeNOP(pattern.get_first(22), 6, true);
            skipAspectCheck(pattern.get_first<uint8_t>(108));

            // default res
            auto [x, y] = GetDesktopRes();
            pattern = hook::pattern("BD ? ? ? ? BB ? ? ? ? BE");
            injector::WriteMemory<int32_t>(pattern.get_first(1), x, true);
            injector::WriteMemory<int32_t>(pattern.get_first(6), y, true);

            // Skip Intro: start with the loading screen and the main menu, without the title screens and logo videos
            if (FixSettings().Bool("MAIN.SkipIntro"))
            {
                // SilentPatch can replace the splash case after this jump table.
                pattern = hook::pattern("A1 ? ? ? ? 83 F8 09 0F 87 ? ? ? ? FF 24 85 ? ? ? ? ? ? E8");
                GameRef<int32_t> gGameState(*pattern.get_first<int32_t*>(1));
                if (*gGameState == 0)
                    *gGameState = 5;
            }
        };

        WFP::onGameInitEvent() += []()
        {
            InitWaterFrustum();

            auto pattern = hook::pattern("E8 ? ? ? ? 8D 4C 24 ? 51 C7 44 24");
            shCameraSize = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), CameraSize);

            pattern = hook::pattern("8B 15 ? ? ? ? 52 E8 ? ? ? ? E8 ? ? ? ? A1");
            Scene.SetAddress(*pattern.get_first<CScene*>(2));

            // Fix sky multitude
            static float horizSpread = 10.0f;
            onResChange() += [](int Width, int Height)
            {
                horizSpread = 1.4f * (CDraw::GetAspectRatio() / (4.0f / 3.0f));
            };

            pattern = hook::pattern("D8 0D ? ? ? ? 83 C4 40");
            injector::WriteMemory<const void*>(pattern.get_first(2), &horizSpread, true);

            pattern = hook::pattern("D8 0D ? ? ? ? D9 C0 D9 44 24 ? D8 44 24");
            injector::WriteMemory<const void*>(pattern.get_first(2), &horizSpread, true);

            //CCam::Process_WheelCam
            pattern = hook::pattern("C7 86 ? ? ? ? ? ? ? ? 8A 47 ? 24 07");
            injector::MakeNOP(pattern.get_first(), 10, true);
            static auto Process_WheelCamFOV = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                *(float*)(regs.esi + 0xB4) = CDraw::ConvertFOVInverse(70.0f);
            });
        };
    }
} Main;
