module;

#include <stdafx.h>
#include "common.h"
#include "callargs.h"

export module Hud;

import Skeleton;
import Draw;
import Sprite2d;

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
        };
    }
} Hud;