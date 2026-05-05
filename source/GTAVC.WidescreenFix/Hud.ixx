module;

#include <stdafx.h>
#include "common.h"
#include "callargs.h"

export module Hud;

import Skeleton;
import Draw;
import Sprite2d;
import Camera;
import Frontend;

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

class Hud
{
public:
    Hud()
    {
        WFP::onGameInitEvent() += []()
        {
            auto pattern = hook::pattern("8B 04 CD ? ? ? ? 39 C6");
            static auto HudDraw1 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                g_wantsToMoveHudLeft = false;
                g_wantsToMoveHudRight = true;
            });

            // Radar
            pattern = hook::pattern("83 3D ? ? ? ? ? 0F 84 ? ? ? ? 80 3D ? ? ? ? ? 0F 85");
            static auto HudDraw2 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                CFont::DrawFonts();
                g_wantsToMoveHudRight = false;
                g_wantsToMoveHudLeft = true;
            });

            pattern = hook::pattern("80 3D ? ? ? ? ? 0F 85 ? ? ? ? 31 ED");
            static auto HudDraw3 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                g_wantsToMoveHudRight = false;
                g_wantsToMoveHudLeft = false;
            });

            // Help message
            pattern = hook::pattern("66 83 3D ? ? ? ? ? 0F 84 ? ? ? ? 68");
            static auto HudDraw4 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                CFont::DrawFonts();
                g_wantsToMoveHudRight = false;
                g_wantsToMoveHudLeft = true;
            });

            pattern = hook::pattern("66 83 3D ? ? ? ? ? 0F 84 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? DF E0 80 E4 ? 80 FC ? 75 ? C7 05 ? ? ? ? ? ? ? ? C7 05 ? ? ? ? ? ? ? ? C7 05 ? ? ? ? ? ? ? ? E9");
            static auto HudDraw5 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                CFont::DrawFonts();
                g_wantsToMoveHudRight = false;
                g_wantsToMoveHudLeft = false;
            });

            pattern = hook::pattern("81 C4 ? ? ? ? 5D 5F 5E 5B C3 8D 80 ? ? ? ? 80 3D");
            static auto HudDrawRet1 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                CFont::DrawFonts();
                g_wantsToMoveHudRight = false;
                g_wantsToMoveHudLeft = false;
            });

            pattern = hook::pattern("81 C4 ? ? ? ? 5D 5F 5E 5B C3 89 C0 8D 40 ? 66 C7 05");
            static auto HudDrawRet2 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                CFont::DrawFonts();
                g_wantsToMoveHudRight = false;
                g_wantsToMoveHudLeft = false;
            });

            pattern = hook::pattern("81 C4 ? ? ? ? 5D 5F 5E 5B C3 ? ? E9 ? ? ? ? 8D 44 20 ? ? ? E9");
            static auto HudDrawRet3 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                CFont::DrawFonts();
                g_wantsToMoveHudRight = false;
                g_wantsToMoveHudLeft = false;
            });

            //Mission Title
            pattern = hook::pattern("66 83 3D ? ? ? ? ? 0F 84 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? DF E0 80 E4 ? 80 FC ? 75 ? C7 05 ? ? ? ? ? ? ? ? C7 05 ? ? ? ? ? ? ? ? C7 05 ? ? ? ? ? ? ? ? C7 05");
            static auto HudDrawAfterFade1 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                CFont::DrawFonts();
                g_wantsToMoveHudRight = true;
                g_wantsToMoveHudLeft = false;
            });

            pattern = hook::pattern("83 C4 60 5D 5E 5B C3 DD D8");
            static auto HudDrawAfterFade2 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                CFont::DrawFonts();
                g_wantsToMoveHudRight = false;
                g_wantsToMoveHudLeft = false;
            });

            pattern = hook::pattern("0F B7 0D ? ? ? ? 0F BF C7");
            static auto RenderPickUpText = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                CFont::DrawFonts();
            });
        };
    }
} Hud;