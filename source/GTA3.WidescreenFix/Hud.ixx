module;

#include <stdafx.h>
#include "common.h"
#include <callargs.h>

export module Hud;

import Skeleton;
import Draw;
import Sprite2d;
import Camera;

export namespace CHud
{
    GameRef<uint8_t> m_Wants_To_Draw_Hud([]() -> uint8_t*
    {
        auto pattern = find_pattern("80 3D ? ? ? ? ? 0F 84 ? ? ? ? 50 50 D9 83");
        if (!pattern.empty())
            return *pattern.get_first<uint8_t*>(2);
        return nullptr;
    });
}

namespace CRadar
{
    SafetyHookInline shShowRadarTrace = {};
    void ShowRadarTrace(float x, float y, uint32_t size, uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha)
    {
        if ((TheCamera->m_WideScreenOn || !CHud::m_Wants_To_Draw_Hud))
            return;

        auto col = CRGBA(red, green, blue, alpha);
        auto black = CRGBA(0, 0, 0, alpha);
        auto rect1 = CRect(x - SCREEN_SCALE_X(size + 1.0f), y - SCREEN_SCALE_Y(size + 1.0f), SCREEN_SCALE_X(size + 1.0f) + x, SCREEN_SCALE_Y(size + 1.0f) + y);
        auto rect2 = CRect(x - SCREEN_SCALE_X(size), y - SCREEN_SCALE_Y(size), SCREEN_SCALE_X(size) + x, SCREEN_SCALE_Y(size) + y);

        shDrawRect1.unsafe_ccall(&rect1, &black);
        shDrawRect1.unsafe_ccall(&rect2, &col);
    }
}

class Hud
{
public:
    Hud()
    {
        WFP::onGameInitEvent() += []()
        {
            auto pattern = hook::pattern("E8 ? ? ? ? 8D 44 24 ? 83 C4 ? 8D 9C 24");
            static auto HudDraw1 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                g_wantsToMoveHudLeft = false;
                g_wantsToMoveHudRight = true;
            });

            pattern = hook::pattern("66 83 3D ? ? ? ? ? ? ? 75 ? 66 83 3D");
            static auto HudDraw2 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                g_wantsToMoveHudRight = false;
                g_wantsToMoveHudLeft = true;
            });

            pattern = hook::pattern("80 3D ? ? ? ? ? 0F 85 ? ? ? ? 31 DB");
            static auto HudDraw3 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                g_wantsToMoveHudRight = false;
                g_wantsToMoveHudLeft = false;
            });

            //MODE_ROCKETLAUNCHER
            pattern = hook::pattern("E8 ? ? ? ? 83 C4 ? E9 ? ? ? ? 8D 84 20 ? ? ? ? 8D 8C 24");
            static auto HudDraw4 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                using tRenderOneXLUSprite = void(__cdecl*)(float x, float y, float z, float halfWidth, float halfHeight, uint8_t red, uint8_t green, uint8_t blue, int16_t alpha, float rhw, uint8_t intensity);
                auto [x, y, z, halfWidth, halfHeight, red, green, blue, alpha, rhw, intensity] = deduce_args<tRenderOneXLUSprite>(regs);
                x = SCREEN_WIDTH / 2;
                y = SCREEN_HEIGHT / 2;
                z = 1.0f;
                halfWidth = SCREEN_SCALE_AR(SCREEN_STRETCH_X(40.0f));
                halfHeight = SCREEN_STRETCH_Y(40.0f);
            });

            pattern = hook::pattern("66 83 3D ? ? ? ? ? 0F 84 ? ? ? ? 68");
            static auto HudDrawAfterFade1 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                g_wantsToMoveHudRight = false;
                g_wantsToMoveHudLeft = true;
            });

            pattern = hook::pattern("31 DB 31 ED BE ? ? ? ? 66 83 BD");
            static auto HudDrawAfterFade2 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                g_wantsToMoveHudRight = false;
                g_wantsToMoveHudLeft = false;
            });

            pattern = hook::pattern("E8 ? ? ? ? 59 E8 ? ? ? ? 6A ? E8 ? ? ? ? A1");
            static auto HudDrawAfterFade3 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                g_wantsToMoveHudRight = true;
                g_wantsToMoveHudLeft = false;
            });

            pattern = hook::pattern("83 C4 ? EB ? C7 05 ? ? ? ? ? ? ? ? 83 C4");
            static auto HudDrawAfterFade4 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                g_wantsToMoveHudRight = false;
                g_wantsToMoveHudLeft = false;
            });

            pattern = hook::pattern("E8 ? ? ? ? 83 C4 ? E9 ? ? ? ? 8D 84 20 ? ? ? ? FF 74 24");
            CRadar::shShowRadarTrace = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), CRadar::ShowRadarTrace);

            //static int SubtitleConstraint = 640;
            //pattern = hook::pattern("8B 0D ? ? ? ? 89 4C 24 ? 50");
            //injector::WriteMemory(pattern.get_first(2), &SubtitleConstraint, true);
            //
            //onResChange() += [](int Width, int Height)
            //{
            //    SubtitleConstraint = Width - (-fWidescreenHudOffset * 2.0f);
            //};
        };
    }
} Hud;