module;

#include <stdafx.h>
#include "common.h"

export module MenuMap;

import Skeleton;
import Draw;
import Menu;

// The map uses a virtual canvas with the same horizontal scale as the HUD.
// Its origin and input limits live in this space; no cursor or vertex offsets.
bool bFullscreenMap = true;
float mapCenterX = 320.0f;
float mapInset = 0.0f;
float mapRight = 640.0f;
float mapBottom = 448.0f;
float mapCursorLeft = 4.0f;
float mapCursorRight = 636.0f;
float mapCursorBottom = 444.0f;
float mapLeftScale = 0.0f;
float mapRightScale = 1.0f;
float mapTopScale = 0.0f;
float mapBottomScale = 1.0f;

static void UpdateMapCanvas()
{
    const float canvasWidth = SCREEN_WIDTH / SCREEN_SCALE_X(1.0f);
    const float oldCenter = mapCenterX;
    mapCenterX = canvasWidth * 0.5f;
    mapInset = bFullscreenMap ? 0.0f : 60.0f;
    mapRight = canvasWidth - mapInset;
    mapBottom = 448.0f - mapInset;
    mapCursorLeft = mapInset + 4.0f;
    mapCursorRight = mapRight - 4.0f;
    mapCursorBottom = mapBottom - 4.0f;
    mapLeftScale = mapInset / canvasWidth;
    mapRightScale = mapRight / canvasWidth;
    mapTopScale = mapInset / 448.0f;
    mapBottomScale = mapBottom / 448.0f;
    if (FrontendMenuManager->m_bMapLoaded)
        FrontendMenuManager->m_vMapOrigin.x += mapCenterX - oldCenter;
}

static void ClampMapOrigin(CMenuManager* menu)
{
    // The allowed pan range shrinks continuously with zoom. It reaches zero
    // when an axis fits in the viewport, without switching from free pan to center.
    const float zoom = menu->m_fMapZoom;
    auto clamp = [zoom](float center, float left, float right)
    {
        const float midpoint = (left + right) * 0.5f;
        const float panRange = std::max(zoom - (right - left) * 0.5f, 0.0f);
        return std::clamp(center, midpoint - panRange, midpoint + panRange);
    };
    menu->m_vMapOrigin.x = clamp(menu->m_vMapOrigin.x, mapInset, mapRight);
    menu->m_vMapOrigin.y = clamp(menu->m_vMapOrigin.y, mapInset, mapBottom);
}

using MapTransformPoint = void(__cdecl*)(CVector2D&, const CVector2D&);
MapTransformPoint mapTransformPoint = nullptr;

static void SetMapZoomReference(SafetyHookContext& regs)
{
    auto menu = reinterpret_cast<CMenuManager*>(regs.esi);
    CVector2D point;
    mapTransformPoint(point, menu->m_vMousePos);

    // The native zoom branches retain the point described by these four locals.
    // Use the crosshair's radar coordinates instead of the viewport midpoint.
    // Reuse the game's projection, including its radar range and Y direction.
    *reinterpret_cast<float*>(regs.esp + 0x2C) = point.x * menu->m_fMapZoom;
    *reinterpret_cast<float*>(regs.esp + 0x7C) = -point.y * menu->m_fMapZoom;
    *reinterpret_cast<float*>(regs.esp + 0x84) = point.x;
    *reinterpret_cast<float*>(regs.esp + 0x10) = -point.y;
}

static float MapAreaNameY(float y)
{
    // The fullscreen map has no lower frame to separate the name from controls.
    return bFullscreenMap ? y - SCREEN_HEIGHT * (40.0f / 448.0f) : y;
}

static float __stdcall MapLeftX(float) { return SCREEN_SCALE_X(mapInset); }
static float __stdcall MapRightX(float) { return SCREEN_SCALE_X(mapRight); }
static float __stdcall MapTopY(float) { return SCREEN_HEIGHT * mapInset / 448.0f; }
static float __stdcall MapBottomY(float) { return SCREEN_HEIGHT * mapBottom / 448.0f; }
static float __stdcall MapCenter(float) { return SCREEN_SCALE_X(mapCenterX); }

class MenuMap
{
public:
    MenuMap()
    {
        WFP::onGameInitEvent() += []()
        {
            CIniReader iniReader("");
            if (!iniReader.ReadInteger("MAIN", "ScalingMode", 1)) return;
            bFullscreenMap = iniReader.ReadInteger("MAIN", "FullscreenMap", 1) != 0;

            // PrintMap initializes a map centered on the player.
            auto pattern = hook::pattern("C7 47 68 00 00 A0 43 C7 47 6C 00 00 4E 43");
            injector::MakeNOP(pattern.get_first(), 7, true);
            static auto InitMapOrigin = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                reinterpret_cast<CMenuManager*>(regs.edi)->m_vMapOrigin.x = mapCenterX;
            });

            // Opening the map recenters on the player through CRadar as well.
            injector::WriteMemory(0x585BB7 + 2, &mapCenterX, true);

            // Zoom around the selected world point for mouse, keyboard and pad.
            // At this branch the native x87 calculations have stored all locals.
            mapTransformPoint = reinterpret_cast<MapTransformPoint>(injector::GetBranchDestination(0x5776F0).as_int());
            static auto ZoomReference = safetyhook::create_mid(0x57747D, SetMapZoomReference);

            // Panning uses the viewport midpoint.
            for (auto address : { 0x577E32, 0x577EE4, 0x5782E5, 0x578394 })
                injector::WriteMemory(address + 2, &mapCenterX, true);

            // Rendering, tile streaming and input share these virtual bounds.
            for (auto address : { 0x577714, 0x577780, 0x578074, 0x578289 })
                injector::WriteMemory(address + 2, &mapCursorRight, true);
            for (auto address : { 0x577794, 0x577800, 0x577FC4, 0x57841C,
                0x577814, 0x577888, 0x578124, 0x578751 })
                injector::WriteMemory(address + 2, &mapCursorLeft, true);
            for (auto address : { 0x57789C, 0x577908, 0x5781D4, 0x5785B2 })
                injector::WriteMemory(address + 2, &mapCursorBottom, true);
            for (auto address : { 0x573D64, 0x57526E, 0x57532E, 0x58806B, 0x588095,
                0x5782D0, 0x578374, 0x5783F8, 0x578600, 0x5786A4, 0x578730 })
                injector::WriteMemory(address + 2, &mapInset, true);
            for (auto address : { 0x573DCD, 0x575353, 0x577E1D, 0x577EC1, 0x577F45 })
                injector::WriteMemory(address + 2, &mapRight, true);
            for (auto address : { 0x573DF3, 0x575303, 0x578467, 0x578506, 0x57858D })
                injector::WriteMemory(address + 2, &mapBottom, true);
            injector::WriteMemory(0x57533C + 2, &mapLeftScale, true);
            injector::WriteMemory(0x575361 + 2, &mapRightScale, true);
            injector::WriteMemory(0x5752EC + 2, &mapTopScale, true);
            injector::WriteMemory(0x575311 + 2, &mapBottomScale, true);

            // Mouse hit tests and dragging use the same bounds as drawing.
            injector::MakeCALL(0x577C7B, MapTopY, true);
            injector::MakeCALL(0x577C94, MapBottomY, true);
            injector::MakeCALL(0x577CB7, MapLeftX, true);
            injector::MakeCALL(0x577CD0, MapRightX, true);
            for (auto address : { 0x577D6F, 0x577D99, 0x577DAB })
                injector::MakeCALL(address, MapCenter, true);

            // Replace only the map-origin clamp, before it loads the x87 stack.
            injector::MakeJMP(0x578785, 0x578821, true);
            static auto ClampOrigin = safetyhook::create_mid(0x578785, [](SafetyHookContext& regs)
            {
                ClampMapOrigin(reinterpret_cast<CMenuManager*>(regs.esi));
            });

            // Zoom input bypasses the native pan clamp. Center before both
            // tile selection and rendering so they use the same map origin.
            static auto ClampStreamingOrigin = safetyhook::create_mid(0x573D24, [](SafetyHookContext& regs)
            {
                ClampMapOrigin(reinterpret_cast<CMenuManager*>(regs.ebx));
            });
            static auto ClampDrawOrigin = safetyhook::create_mid(0x575246, [](SafetyHookContext& regs)
            {
                ClampMapOrigin(reinterpret_cast<CMenuManager*>(regs.edi));
            });

            // Only this PrintString call draws the map's area name.
            static auto AreaName = safetyhook::create_mid(0x575F89, [](SafetyHookContext& regs)
            {
                auto& y = *reinterpret_cast<float*>(regs.esp + 4);
                y = MapAreaNameY(y);
            });

            if (bFullscreenMap)
            {
                // Remove the frame and its masks, leaving the map and menu text.
                for (auto address : { 0x575BF6, 0x575C40, 0x575C84, 0x575CCE,
                    0x575D1F, 0x575D6F, 0x575DC2, 0x575E12 })
                    injector::MakeNOP(address, 5, true);
            }

            // Retain the previous fullscreen map's zoom and pan speed.
            static float speed = 14.0f;
            for (auto address : { 0x577679, 0x5779A8, 0x577E70, 0x578320, 0x5784B5, 0x578650 })
                injector::WriteMemory(address + 2, &speed, true);

            // Width alone is not enough to identify the original resolution.
            // Always run the scale branch, including 640-wide non-4:3 modes.
            for (auto address : { 0x575322, 0x575347, 0x57536F, 0x5754D0, 0x57556A, 0x575634,
                0x575670, 0x575732, 0x57576C, 0x5757E5, 0x57581B, 0x575894, 0x5758CA,
                0x575946, 0x575976 })
                injector::WriteMemory<int32_t>(address + 2, -1, true);
            for (auto address : { 0x575A48, 0x575A83 })
                injector::WriteMemory<int32_t>(address + 1, -1, true);
            for (auto address : { 0x575B6A, 0x5761FB })
                injector::WriteMemory<int32_t>(address + 6, -1, true);
            for (auto address : { 0x575EC6, 0x576017 })
                injector::WriteMemory<int32_t>(address + 1, -1, true);

            // The legend is a centered dialog over the full-width map. Move its
            // layout coordinates before drawing, including the two icon columns.
            static auto LegendWindow = safetyhook::create_mid(0x5760A1, [](SafetyHookContext& regs)
            {
                auto rect = *reinterpret_cast<CRect**>(regs.esp);
                rect->left -= fWidescreenHudOffset43;
                rect->right -= fWidescreenHudOffset43;
            });
            static auto LegendEntry = safetyhook::create_mid(0x5761EB, [](SafetyHookContext& regs)
            {
                *reinterpret_cast<int32_t*>(regs.esp) -= static_cast<int32_t>(std::lround(fWidescreenHudOffset43));
            });

            UpdateMapCanvas();
            onResChange() += [](int Width, int Height) { UpdateMapCanvas(); };
        };
    }
} MenuMap;