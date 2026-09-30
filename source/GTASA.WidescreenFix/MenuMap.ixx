module;

#include <stdafx.h>
#include "common.h"

export module MenuMap;

import Skeleton;
import Draw;
import Menu;

// The map uses a virtual canvas with the same horizontal scale as the HUD.
// Its origin and input limits live in this space; no cursor or vertex offsets.
float mapCenterX = 320.0f;
float mapRight = 580.0f;
float mapCursorRight = 576.0f;
float mapRightScale = 580.0f / 640.0f;

static void UpdateMapCanvas()
{
    const float canvasWidth = SCREEN_WIDTH / SCREEN_SCALE_X(1.0f);
    const float oldCenter = mapCenterX;
    mapCenterX = canvasWidth * 0.5f;
    mapRight = canvasWidth - 60.0f;
    mapCursorRight = canvasWidth - 64.0f;
    mapRightScale = mapRight / canvasWidth;
    if (FrontendMenuManager->m_bMapLoaded)
        FrontendMenuManager->m_vMapOrigin.x += mapCenterX - oldCenter;
}

class MenuMap
{
public:
    MenuMap()
    {
        WFP::onGameInitEvent() += []()
        {
            CIniReader iniReader("");
            if (!iniReader.ReadInteger("MAIN", "ScalingMode", 1)) return;

            // PrintMap initializes a map centered on the player.
            auto pattern = hook::pattern("C7 47 68 00 00 A0 43 C7 47 6C 00 00 4E 43");
            injector::MakeNOP(pattern.get_first(), 7, true);
            static auto InitMapOrigin = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                reinterpret_cast<CMenuManager*>(regs.edi)->m_vMapOrigin.x = mapCenterX;
            });

            // AdditionalOptionInput zooms and pans around this same center.
            for (auto address : { 0x577445, 0x577E32, 0x577EE4, 0x5782E5, 0x578394 })
                injector::WriteMemory(address + 2, &mapCenterX, true);

            // The mouse bounds, panning limits, and map frame use the full canvas.
            for (auto address : { 0x577714, 0x577780, 0x578074, 0x578289 })
                injector::WriteMemory(address + 2, &mapCursorRight, true);
            for (auto address : { 0x575353, 0x577E1D, 0x577EC1, 0x577F45, 0x5787C6, 0x5787D3 })
                injector::WriteMemory(address + 2, &mapRight, true);
            injector::WriteMemory(0x575361 + 2, &mapRightScale, true);

            // Calls to StretchX(580) used by map hit testing must use canvasWidth - 60.
            pattern = hook::pattern("68 00 00 11 44 E8 ? ? ? ?");
            pattern.for_each_result([](hook::pattern_match match)
            {
                auto address = reinterpret_cast<uintptr_t>(match.get<void>());
                if (address < 0x5773D0 || address >= 0x578F50) return;
                static std::vector<SafetyHookMid> MapRightArgs;
                MapRightArgs.emplace_back(safetyhook::create_mid(address + 5, [](SafetyHookContext& regs)
                {
                    *reinterpret_cast<float*>(regs.esp) = mapRight;
                }));
            });

            // Pan toward the actual screen center while dragging with the mouse.
            static auto MapCenterArg = safetyhook::create_mid(0x577D69, [](SafetyHookContext& regs)
            {
                *reinterpret_cast<float*>(regs.esp) = mapCenterX;
            });

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