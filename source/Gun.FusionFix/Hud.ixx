module;

#include <stdafx.h>
#include <d3d9.h>
#include "FixSettings.hpp"
#include <atomic>
#include <unordered_map>

export module Hud;

import Frontend;

// The HUD is a tree of screen elements laid out on the 640x480 screen, which the fix keeps in a centred 4:3 area.
// Its corner groups move as a whole to the sides of the HUD box: the whole screen (Auto) or a centred box of the
// chosen shape. Menus, messages, objectives and full-screen layers keep their place.
export namespace GunHud
{
    inline SafetyHookMid UpdateHook;
    inline std::vector<SafetyHookMid> RadarBlipHooks;
    inline std::atomic<float> Width{ 640.0f }, Height{ 480.0f };
    inline std::atomic<float> Aspect{ 0.0f };

    inline uint32_t Id(const uint8_t* element)
    {
        return element ? *reinterpret_cast<const uint32_t*>(element + 24) : 0;
    }

    // -1 left corner, +1 right corner, 0 stays centred.
    inline int Side(uint32_t id)
    {
        static const auto sides = []()
        {
            std::unordered_map<uint32_t, int> result
            {
                { GunMenu::Checksum("hud_anchor"), -1 },
                { GunMenu::Checksum("mission_meter_anchor_1"), -1 },
                { GunMenu::Checksum("mission_meter_anchor_2"), -1 },
                { GunMenu::Checksum("hud_instruct_anchor"), 1 },
                { GunMenu::Checksum("radar_ring_anchor"), 1 },
                { GunMenu::Checksum("map_anchor"), 1 },
            };
            for (int i = 0; i < 64; ++i)
                result[GunMenu::Checksum(("map" + std::to_string(i)).c_str())] = 1;
            return result;
        }();
        auto side = sides.find(id);
        return side == sides.end() ? 0 : side->second;
    }

    // Offset of the corner groups in 640x480 units.
    inline float Offset()
    {
        const float width = Width.load(std::memory_order_relaxed), height = Height.load(std::memory_order_relaxed);
        if (width <= 0.0f || height <= 0.0f)
            return 0.0f;
        const float aspect = Aspect.load(std::memory_order_relaxed);
        const float box = aspect > 0.0f ? std::min(width, height * aspect) : width;
        return std::max(0.0f, (box - height * 4.0f / 3.0f) * 0.5f) / (height / 480.0f);
    }

    inline void Update()
    {
        const auto parsed = ParseWidescreenHudOffset(FixSettings().String("MAIN.HudAspectRatioConstraint"));
        Aspect.store(parsed && std::isfinite(*parsed) && *parsed > 0.0f ? std::clamp(*parsed, 4.0f / 3.0f, 32.0f / 9.0f) : 0.0f,
            std::memory_order_relaxed);
    }

    inline void Resize(int width, int height)
    {
        Width.store(static_cast<float>(width), std::memory_order_relaxed);
        Height.store(static_cast<float>(height), std::memory_order_relaxed);
    }

    inline void Init(int width, int height)
    {
        Resize(width, height);
        Update();
        FixSettings().OnChange("MAIN.HudAspectRatioConstraint", Update);
        // Radar dots and waypoint blips are sprites placed in screen units from the map mask's local position,
        // so they follow the radar group's shift here instead of through the element tree.
        auto blips = hook::pattern("8B 4F 18 83 EC 08 D9 5C 24 04 D9 44 24 ? D8 44 24 ? D9 1C 24 E8 ? ? ? ? 38 5F 1F").count(2);
        for (size_t i = 0; i < blips.size(); ++i)
        {
            RadarBlipHooks.push_back(safetyhook::create_mid(blips.get(i).get<void>(21), [](SafetyHookContext& ctx)
            {
                *reinterpret_cast<float*>(ctx.esp) += Offset();
            }));
        }
        auto update = hook::pattern("09 7E 08 8B 16 8B CE FF 52 28 8B 56 08 81 E2 FF EF FF FF");
        // After an element has placed itself and before its children follow it.
        UpdateHook = safetyhook::create_mid(update.get_first(3), [](SafetyHookContext& ctx)
        {
            static const auto root = GunMenu::Checksum("player1_panel_container");
            static std::unordered_map<uint32_t, float> applied;
            auto element = reinterpret_cast<uint8_t*>(ctx.esi);
            auto parent = *reinterpret_cast<uint8_t**>(element + 96);
            if (!parent || Id(parent) != root)
                return;
            const int side = Side(Id(element));
            if (!side)
                return;
            auto& flags = *reinterpret_cast<uint32_t*>(element + 8);
            auto& shift = applied[Id(element)];
            const float wanted = side * Offset();
            if (!(flags & 0x800))
            {
                // Placed on an earlier frame; place it again when the HUD box has changed.
                if (shift != wanted)
                    flags |= 0x400;
                return;
            }
            *reinterpret_cast<float*>(element + 308) += wanted;
            *reinterpret_cast<int16_t*>(element + 320) += static_cast<int16_t>(std::lround(wanted * 10.0f));
            shift = wanted;
        });
    }
}
