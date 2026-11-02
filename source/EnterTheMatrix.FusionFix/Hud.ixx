module;

#include <stdafx.h>

export module Hud;

import ComVars;
import Layout;

namespace HudDraw
{
    SafetyHookInline MainHook, WeaponsHook, TimerHook, WorldHook, SniperHook, BinocularHook;

    // The gameplay HUD: its elements near a side of the 4:3 frame move to that side of the HUD box. Everything else
    // the game draws in 2D (menus, subtitles, messages) stays in the 4:3 frame.
    template <SafetyHookInline& Hook, typename... Args>
    int __cdecl Anchored(Args... args)
    {
        const bool previous = std::exchange(Layout::InHud, true);
        const int result = Hook.ccall<int>(args...);
        Layout::InHud = previous;
        return result;
    }

    int __cdecl World(void* a1, int a2)
    {
        const auto previous = std::exchange(Layout::Current, Layout::Context::World);
        const int result = WorldHook.ccall<int>(a1, a2);
        Layout::Current = previous;
        return result;
    }

    int __cdecl Sniper(int a1)
    {
        const auto previous = std::exchange(Layout::Current, Layout::Context::Mask);
        const int result = SniperHook.ccall<int>(a1);
        Layout::Current = previous;
        return result;
    }

    int __cdecl Binocular()
    {
        const auto previous = std::exchange(Layout::Current, Layout::Context::Mask);
        const int result = BinocularHook.ccall<int>();
        Layout::Current = previous;
        return result;
    }
}

class Hud
{
public:
    Hud()
    {
        WFP::onInitEvent() += []()
        {
            // HUD: health, Focus, weapon, boss and vehicle displays; the weapon selection; mission timers
            auto pattern = hook::pattern("55 8B EC 83 EC 10 E8 ? ? ? ? A1 ? ? ? ? 83 B8 F8 00 00 00 01 74 0F");
            HudDraw::MainHook = safetyhook::create_inline(pattern.get_first(), HudDraw::Anchored<HudDraw::MainHook, int, int>);
            pattern = hook::pattern("55 8B EC 81 EC A8 00 00 00 C7 85 74 FF FF FF 58 00 00 00");
            HudDraw::WeaponsHook = safetyhook::create_inline(pattern.get_first(), HudDraw::Anchored<HudDraw::WeaponsHook, int>);
            pattern = hook::pattern("55 8B EC 83 EC 5C 83 3D ? ? ? ? 00 0F 84 ? ? ? ? 83 3D ? ? ? ? 00 0F 8E");
            HudDraw::TimerHook = safetyhook::create_inline(pattern.get_first(), HudDraw::Anchored<HudDraw::TimerHook>);
            pattern = hook::pattern("55 8B EC 83 EC 58 A1 ? ? ? ? 8B 48 3C 8B 91 88 01 00 00");
            HudDraw::WorldHook = safetyhook::create_inline(pattern.get_first(), HudDraw::World);
            pattern = hook::pattern("55 8B EC 83 EC 08 83 7D 08 00 75 0B A1 ? ? ? ? 8B 48 3C 89 4D 08 C7 45 F8 2D 00 00 00 68");
            HudDraw::SniperHook = safetyhook::create_inline(pattern.get_first(), HudDraw::Sniper);
            pattern = hook::pattern("55 8B EC 83 EC 08 C7 45 F8 2D 00 00 00 68 ? ? ? ? A1 ? ? ? ? 50 FF 15");
            HudDraw::BinocularHook = safetyhook::create_inline(pattern.get_first(), HudDraw::Binocular);
        };
    }
} Hud;
