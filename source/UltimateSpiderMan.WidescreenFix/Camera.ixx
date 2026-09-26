module;

#include "stdafx.h"
#include <fstream>
#include <set>

export module Camera;

import Screen;
import ExtremeDrawDistance;

float* pAspect = nullptr, * pFOV = nullptr, * pZoom = nullptr;
uint8_t* pIgnoreZoom = nullptr;

int32_t* pDisableOcclusionCulling = nullptr;
SafetyHookInline shRenderWorld;
void __fastcall RenderWorld(uintptr_t manager, void*, uintptr_t camera, int player)
{
    // Use the engine's own bypass: reset active occluders and skip both
    // occlusion tests, while retaining frustum, distance and streaming limits.
    auto saved = *pDisableOcclusionCulling;
    if (IsBackBufferScene())
        *pDisableOcclusionCulling = 3;
    shRenderWorld.thiscall<void>(manager, camera, player);
    *pDisableOcclusionCulling = saved;
}

float AdjustHorizontalFOV(float fov)
{
    return 2.0f * std::atan(std::tan(fov * 0.5f) * Screen.fAspectRatioDiff * Screen.fFOVFactor);
}

bool bTraceComicCapture = false;
bool bComicCapturePass = false;
std::ofstream& ComicCaptureLog()
{
    static std::ofstream log(GetThisModulePath() / "USM.ComicCapture.log", std::ios::trunc);
    return log;
}

SafetyHookInline shRenderComicCamera;
void __fastcall RenderComicCamera(uintptr_t component, void*, float* info)
{
    static std::set<std::array<uintptr_t, 4>> seen;
    auto capture = reinterpret_cast<uint8_t*>(info)[323];
    if (bTraceComicCapture && seen.size() < 128)
    {
        auto texture = *reinterpret_cast<uintptr_t*>(component + 0x44);
        if (seen.insert({ component, texture, capture, IsBackBufferScene() }).second)
        {
            auto& log = ComicCaptureLog();
            log << "panel component=" << component << " texture=" << texture
                << " capture=" << int(capture) << " backbuffer=" << IsBackBufferScene()
                << " screen=" << Screen.Width << ',' << Screen.Height << " rect=";
            for (size_t i = 69; i < 73; ++i) log << info[i] << ',';
            log << " matrix=";
            for (size_t i = 0; i < 16; ++i) log << info[i] << ',';
            if (texture)
                log << " textureSize=" << *reinterpret_cast<uint32_t*>(texture + 24)
                << ',' << *reinterpret_cast<uint32_t*>(texture + 28);
            log << std::endl;
        }
    }
    auto savedCapturePass = bComicCapturePass;
    bComicCapturePass = capture != 0;
    shRenderComicCamera.thiscall<void>(component, info);
    bComicCapturePass = savedCapturePass;
}

SafetyHookInline shRebuildViewFrame;
int RebuildViewFrameForScene(bool comicCamera)
{
    if (!comicCamera && *pCurrentScene && !IsBackBufferScene())
        return shRebuildViewFrame.ccall<int>();

    auto& aspect = *pAspect; // geometry_manager::PROJ_ASPECT
    auto& fov = *pFOV; // PROJ_FIELD_OF_VIEW (horizontal radians)
    auto& zoom = *pZoom; // PROJ_ZOOM
    auto originalAspect = aspect;
    auto originalFOV = fov;
    auto originalZoom = zoom;

    // Rebuild the geometry manager and NGL projections, including the culling planes,
    // with the same Hor+ camera. Restore the game's camera state afterwards so zoom
    // changes and repeated rebuilds never compound the correction.
    aspect *= Screen.fAspectRatioDiff;
    // Capture renders into a pixel-sized rectangle in the back buffer. The
    // native rebuild measures that rectangle on its 640x480 logical canvas,
    // dividing its aspect by the display ratio before the texture is copied.
    // Cancel that conversion; the captured panel later expands to the same
    // widescreen layout as the live scene. Horizontal FOV is adjusted once.
    // World rendering rebuilds this projection again when it applies the
    // camera and far plane. Keep the correction for the entire capture pass,
    // including those nested rebuilds, rather than just the initial call.
    if (bComicCapturePass)
        aspect *= Screen.fAspectRatioDiff;
    // Native projection includes zoom, but its culling planes and region rays
    // read FOV alone. Bake zoom into FOV for this rebuild so they agree.
    fov = AdjustHorizontalFOV(originalFOV * (*pIgnoreZoom ? 1.0f : originalZoom));
    zoom = 1.0f;

    if (bTraceComicCapture && bComicCapturePass)
    {
        static std::set<std::array<uint32_t, 5>> seen;
        auto target = *pCurrentScene ? *reinterpret_cast<uintptr_t*>(*pCurrentScene + 0x334) : 0;
        if (seen.size() < 128 && seen.insert({ uint32_t(target), std::bit_cast<uint32_t>(originalAspect),
            std::bit_cast<uint32_t>(originalFOV), std::bit_cast<uint32_t>(originalZoom), uint32_t(comicCamera) }).second)
        {
            auto& log = ComicCaptureLog();
            log << "projection initial=" << comicCamera << " target=" << target << " backbuffer=" << IsBackBufferScene()
                << " original=" << originalAspect << ',' << originalFOV << ',' << originalZoom
                << " adjusted=" << aspect << ',' << fov;
            if (target) log << " targetSize=" << *reinterpret_cast<uint32_t*>(target + 24)
                << ',' << *reinterpret_cast<uint32_t*>(target + 28);
            log << std::endl;
        }
    }

    auto result = shRebuildViewFrame.ccall<int>();
    aspect = originalAspect;
    fov = originalFOV;
    zoom = originalZoom;
    return result;
}

int __cdecl RebuildViewFrame()
{
    return RebuildViewFrameForScene(false);
}

int __cdecl RebuildComicViewFrame()
{
    // Initial panel camera setup; capture renders into the back buffer before
    // copying its rectangle to a texture. Nested rebuilds use the scoped flag.
    return RebuildViewFrameForScene(true);
}

struct VisibilityPoint
{
    float x, z;
};

struct VisibilityPolygon
{
    VisibilityPoint points[14];
    uint32_t count;
};
static_assert(offsetof(VisibilityPolygon, count) == 0x70);

float* pVisibilityOrigin = nullptr;
void CoverVisibilityArcs(VisibilityPolygon& polygon, float originX, float originZ, float radius)
{
    // sub_52A320 appends one translated copy of every vertex before
    // rebuilding the hull. Reserve half of the 14-point buffer for it.
    constexpr size_t maxInitialPoints = 7;
    if (!std::isfinite(radius) || radius <= 0.0f || polygon.count >= maxInitialPoints)
        return;

    // Native corner/centre rays are clipped to a circle, then joined with
    // chords. Add tangent intersections so the broad-phase polygon covers
    // the intervening arcs; later mesh tests still enforce the native radius.
    struct Ray { float angle; VisibilityPoint direction; };
    std::array<Ray, 14> rays;
    size_t count = 0;
    for (uint32_t i = 0; i < polygon.count; ++i)
    {
        auto x = polygon.points[i].x - originX;
        auto z = polygon.points[i].z - originZ;
        auto length = std::hypot(x, z);
        if (std::isfinite(length) && std::abs(length - radius) <= radius * 0.0001f)
            rays[count++] = { std::atan2(z, x), {x / length, z / length} };
    }
    if (count < 2)
        return;
    std::sort(rays.begin(), rays.begin() + count, [](const Ray& a, const Ray& b) { return a.angle < b.angle; });
    for (size_t i = 0; i < count && polygon.count < maxInitialPoints; ++i)
    {
        const auto& a = rays[i];
        const auto& b = rays[(i + 1) % count];
        auto angle = b.angle - a.angle;
        if (angle < 0.0f)
            angle += 6.28318530718f;
        // Do not bridge the back of the frustum or nearly opposite rays.
        if (angle <= 0.0001f || angle >= 2.09439510239f)
            continue;
        auto denominator = 1.0f + a.direction.x * b.direction.x + a.direction.z * b.direction.z;
        polygon.points[polygon.count++] = {
            originX + radius * (a.direction.x + b.direction.x) / denominator,
            originZ + radius * (a.direction.z + b.direction.z) / denominator };
    }
}

float fDrawDistance = 167.0f;
float fRenderRegionDistanceSquared = 167.0f * 167.0f;
std::array<float, 16> DrawDistances, DrawDistancesSquared;

void SetDrawDistance(float scale)
{
    if (!std::isfinite(scale))
        scale = 1.0f;
    scale = std::clamp(scale, 1.0f, 2.0f);
    if (scale == 1.0f)
        return;

    // Render resident geometry farther out; never change pack requests,
    // streaming slots, collision residency or mission/allocator capacities.
    // The 334-unit ceiling leaves a margin below the stock 600-unit search.
    // Residency is still controlled by the game; retain its low-detail fallback.
    fDrawDistance = 167.0f * scale;
    fRenderRegionDistanceSquared = fDrawDistance * fDrawDistance;

    // build_render_data_regions uses sqrt(height^2 + 167^2), independently
    // of mesh fade distances. Redirect only its operand, not the shared constant.
    auto pattern = hook::pattern("D9 00 D9 C0 DE C9 D8 05 ? ? ? ? D9 FA D9 54 24 10"); //0x5470B0 + 8
    injector::WriteMemory(pattern.get_first(8), &fRenderRegionDistanceSquared, true);

    pattern = hook::pattern("D9 04 8D ? ? ? ? D9 44 24"); //0x53D1AF + 3
    auto originalDistances = *pattern.get_first<float*>(3);
    auto linearOperand = pattern.get_first(3);
    pattern = hook::pattern("D9 04 8D ? ? ? ? C3"); //0x51E8B6 + 3
    auto originalSquared = *pattern.get_first<float*>(3);
    auto squaredOperand = pattern.get_first(3);

    // Preserve the authored tables used when assigning distance categories.
    for (size_t i = 0; i < DrawDistances.size(); ++i)
    {
        DrawDistances[i] = originalDistances[i] * scale;
        DrawDistancesSquared[i] = originalSquared[i] * scale * scale;
    }
    injector::WriteMemory(linearOperand, DrawDistances.data(), true);
    injector::WriteMemory(squaredOperand, DrawDistancesSquared.data(), true);
    pattern = hook::pattern("D8 1D ? ? ? ? 89 44 24 ? C7 44 24"); //0x53CEFC + 2
    injector::WriteMemory(pattern.get_first(2), &fDrawDistance, true);
    pattern = hook::pattern("D8 1D ? ? ? ? C7 44 24 ? ? ? ? ? DF E0 F6 C4 05 8D 44 24 ? 7B ? 8D 44 24 ? D9 00"); //0x53D377 + 2
    injector::WriteMemory(pattern.get_first(2), &fDrawDistance, true);
    pattern = hook::pattern("C7 44 24 ? ? ? ? ? DF E0 F6 C4 05 8D 44 24 ? 7B ? 8D 44 24 ? 8B 08"); //0x53CF06 + 4
    injector::WriteMemory(pattern.get_first(4), fDrawDistance, true);
    pattern = hook::pattern("C7 44 24 ? ? ? ? ? DF E0 F6 C4 05 8D 44 24 ? 7B ? 8D 44 24 ? D9 00"); //0x53D37D + 4
    injector::WriteMemory(pattern.get_first(4), fDrawDistance, true);
    pattern = hook::pattern("D9 04 85 ? ? ? ? 0F B6 4E"); //0x51E8E3 + 3
    injector::WriteMemory(pattern.get_first(3), DrawDistancesSquared.data(), true);
    pattern = hook::pattern("D9 04 8D ? ? ? ? 8A 4D"); //0x53A2E4 + 3
    injector::WriteMemory(pattern.get_first(3), DrawDistancesSquared.data(), true);
}

class Camera
{
public:
    Camera()
    {
        WFP::onInitEvent() += []
        {
            auto pattern = hook::pattern("E8 ? ? ? ? D8 3D"); //0x515290 + 2
            pAspect = injector::ReadMemory<float*>(injector::GetBranchDestination(pattern.get_first()).as_int() + 2);

            pattern = hook::pattern("E8 ? ? ? ? D8 0D ? ? ? ? D8 0D ? ? ? ? D9 44 24"); //0x515270 + 2
            pFOV = injector::ReadMemory<float*>(injector::GetBranchDestination(pattern.get_first()).as_int() + 2);

            pattern = hook::pattern("E8 ? ? ? ? D9 5E ? 5F 32 C0"); //0x515280 + 2
            pZoom = injector::ReadMemory<float*>(injector::GetBranchDestination(pattern.get_first()).as_int() + 2);

            pattern = hook::pattern("8A 0D ? ? ? ? 84 C9 89 44 24 24 74 08 DD D8"); //0x53A99C + 2
            pIgnoreZoom = *pattern.get_first<uint8_t*>(2);

            pattern = hook::pattern("E8 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 8D 94 24 ? ? ? ? 52 8D 84 24 ? ? ? ? 50 8D 4C 24"); //0x53A930
            shRebuildViewFrame = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), RebuildViewFrame);

            pattern = hook::pattern("E8 ? ? ? ? 6A 00 E8 ? ? ? ? 6A 04 E8 ? ? ? ? 50 E8 ? ? ? ? 8A 46 48"); //0x741C68
            injector::MakeCALL(pattern.get_first(), RebuildComicViewFrame, true);

            // Bounded transition diagnostics; hidden from the distributed INI.
            bTraceComicCapture = CIniReader("").ReadInteger("DEBUG", "TraceComicCapture", 0) != 0;
            pattern = hook::pattern("64 A1 00 00 00 00 6A FF 68 ? ? ? ? 50 64 89 25 00 00 00 00 83 EC 40 56 8B F1 83 7E 2C 04"); //0x7419C0
            shRenderComicCamera = safetyhook::create_inline(pattern.get_first(), RenderComicCamera);

            pattern = hook::pattern("D9 44 24 14 5E D8 64 24 08 D9 44 24 14 D8 64 24 0C DE F9"); //0x73A28E
            static auto ComicCapturePanelAspect = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                if (!bComicCapturePass)
                    return;
                // setup_geomgr has already submitted the viewport. These local
                // bounds are now used only to narrow horizontal FOV for tall
                // panels. Undo the capture canvas's X compression here, before
                // the native 4:3 comparison, so wide panels are not zoomed in.
                auto bounds = reinterpret_cast<float*>(regs.esp + 0x0C);
                auto width = bounds[2] - bounds[0];
                bounds[2] = bounds[0] + width * Screen.fAspectRatioDiff;
                if (bTraceComicCapture)
                {
                    static uint32_t count = 0;
                    if (count < 32)
                    {
                        ++count;
                        ComicCaptureLog() << "capture FOV bounds width=" << width
                            << " corrected=" << bounds[2] - bounds[0]
                            << " height=" << bounds[3] - bounds[1] << std::endl;
                    }
                }
            });
            if (bTraceComicCapture)
            {
                pattern = hook::pattern("E8 ? ? ? ? 83 C4 0C 83 C4 10 C3"); //0x733365
                static auto TraceComicCopy = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    static uint32_t count = 0;
                    if (count >= 64)
                        return;
                    ++count;
                    auto args = reinterpret_cast<uintptr_t*>(regs.esp);
                    auto& log = ComicCaptureLog();
                    log << "copy source=" << args[0] << " destination=" << args[1];
                    for (size_t i = 0; i < 2; ++i)
                        if (args[i]) log << " size" << i << '='
                            << *reinterpret_cast<uint32_t*>(args[i] + 24) << ','
                            << *reinterpret_cast<uint32_t*>(args[i] + 28);
                    auto rect = reinterpret_cast<int32_t*>(args[2]);
                    log << " rect=";
                    for (size_t i = 0; i < 4; ++i) log << rect[i] << ',';
                    log << std::endl;
                });
            }

            pattern = hook::pattern("D8 05 ? ? ? ? D9 5C 24 04 8B 54 24 04 D8 05"); //0x529E80 + 2
            pVisibilityOrigin = *pattern.get_first<float*>(2);
            pattern = hook::pattern("56 E8 ? ? ? ? 83 C4 04 5F 83 C4 40 C3"); //0x52A114
            static auto visibilityArcHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                if (Screen.fAspectRatioDiff <= 1.0f || !IsBackBufferScene())
                    return;
                // At the final hull-build call: 0x40 locals + saved EDI,
                // followed by return address, ground height and radius.
                auto radius = *reinterpret_cast<float*>(regs.esp + 0x4C);
                CoverVisibilityArcs(*reinterpret_cast<VisibilityPolygon*>(regs.esi),
                    pVisibilityOrigin[0], pVisibilityOrigin[2], radius);
            });

            CIniReader iniReader("");
            // Developer-only diagnostics: omitted from the shipped INI and off by default.
            if (iniReader.ReadInteger("DEBUG", "TraceBuildingVisibility", 0))
            {
                static std::ofstream visibilityLog(GetThisModulePath() / "USM.BuildingVisibility.log", std::ios::trunc);
                visibilityLog << "Visibility-list trace active; stock capacity = 20.\n" << std::flush;
                pattern = hook::pattern("83 FA 14 7D 07 89 44 91 08 FF 41 04"); //0x562AA2
                static auto traceRegionCapacity = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    static uint32_t recorded = 0;
                    if (regs.edx < 20 || recorded >= 16)
                        return;
                    ++recorded;
                    visibilityLog << "Dropped region=" << std::hex << regs.eax << std::dec
                        << " count=" << regs.edx << " time=" << GetTickCount64() << '\n' << std::flush;
                });
            }

            if (iniReader.ReadInteger("DEBUG", "DisableOcclusionCulling", 0))
            {
                pattern = hook::pattern("A1 ? ? ? ? 83 C4 04 83 F8 03 74 38 56 8B CF"); //0x54B26A + 1
                pDisableOcclusionCulling = *pattern.get_first<int32_t*>(1);
                pattern = hook::pattern("51 53 55 56 57 8B F9 89 7C 24 10 E8 ? ? ? ? 8B 74 24 18 56"); //0x54B250
                shRenderWorld = safetyhook::create_inline(pattern.get_first(), RenderWorld);
            }
            auto distanceScale = iniReader.ReadFloat("MAIN", "DrawDistanceScale", 1.0f);
            if (!std::isfinite(distanceScale))
                distanceScale = 1.0f;
            distanceScale = std::clamp(distanceScale, 1.0f, 10.0f);
            if (distanceScale > 2.0f)
                InitExtremeDrawDistance(distanceScale);
            // Also provides the normal 2x fallback if expanded allocation fails.
            SetDrawDistance(distanceScale);
        };
    }
} Camera;
