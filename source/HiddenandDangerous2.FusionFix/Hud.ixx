module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module Hud;

import ComVars;

// The gameplay HUD is one scene drawn in a 4:3 frame. Its groups are moved to the screen edges (or the edges of the
// chosen HUD box) right after the engine builds their world matrices, so animations, children and the texts placed
// on them follow. The bottom panel is split: soldiers on the left, the weapon panel on the right. The scope
// masks keep the 4:3 frame with black sides.

// Nesting depth of the game functions that build 2D scenes (menus, loading screens, HUD)

export namespace HD2Hud
{
    enum Anchor : int8_t { Center = 0, Left = -1, Right = 1 };

    inline uint8_t* owner{};
    inline uint8_t* panel{};
    inline float shift{};
    inline std::atomic<float> boxAspect{};
    inline std::atomic<bool> dirty{};
    inline std::vector<std::pair<uint8_t*, int8_t>> anchors;
    inline SafetyHookInline worldMatrixHook;
    inline SafetyHookInline initHook;
    inline SafetyHookInline releaseHook;
    inline uint8_t* relayout{};
    inline uint8_t* masks[2]{};
    inline int sideWidth{};
    inline int screenWidth{};
    inline int screenHeight{};
    inline SafetyHookInline renderHook;

    inline uint8_t* Model()
    {
        return owner ? *reinterpret_cast<uint8_t**>(owner + 8) : nullptr;
    }

    inline std::string_view Name(uint8_t* frame)
    {
        auto name = *reinterpret_cast<const char**>(frame + 232);
        if (!name)
            return {};
        if (auto dot = std::strrchr(name, '.'))
            name = dot + 1;
        return name;
    }

    inline int8_t Classify(std::string_view id, bool inPanel)
    {
        if (inPanel)
            return id == "tabl02" ? Right : Left;
        if (id == "kompas" || id == "inventory")
            return Left;
        if (id == "select_weapon" || id == "infosel" || id == "mp_scoretable" || id == "explosivetimer" ||
            id == "carnage_stats" || id.starts_with("info"))
            return Right;
        return Center;
    }

    inline int8_t AnchorOf(uint8_t* frame, bool inPanel)
    {
        for (auto& [known, anchor] : anchors)
        {
            if (known == frame)
                return anchor;
        }
        const auto name = Name(frame);
        if (!inPanel && name == "down_panel")
            panel = frame;
        const auto anchor = Classify(name, inPanel);
        anchors.emplace_back(frame, anchor);
        return anchor;
    }

    inline void __fastcall WorldMatrix(uint8_t* frame, void*)
    {
        worldMatrixHook.thiscall<void>(frame);
        auto model = Model();
        if (!model || shift == 0.0f)
            return;
        auto parent = *reinterpret_cast<uint8_t**>(frame + 264);
        if (parent != model && (!panel || parent != panel))
            return;
        if (const auto anchor = AnchorOf(frame, parent == panel))
            reinterpret_cast<float*>(frame + 64)[12] += anchor * shift;
    }

    // Scene units the side groups move outwards: the 4:3 frame is 1.0 wide
    inline void Update(float screenAspect)
    {
        const auto box = boxAspect.load(std::memory_order_relaxed);
        const auto width = box > 0.0f ? std::min(screenAspect, box) : screenAspect;
        const auto value = std::max(0.0f, (width / (4.0f / 3.0f) - 1.0f) * 0.5f);
        if (value != shift)
        {
            shift = value;
            dirty.store(true, std::memory_order_release);
        }
    }

    inline void Invalidate(uint8_t* frame)
    {
        auto& flags = *reinterpret_cast<uint32_t*>(frame + 224);
        flags = (flags & 0xBFFFFEDFu) | 0x40000000u;
        auto table = *reinterpret_cast<uintptr_t**>(frame);
        reinterpret_cast<void(__stdcall*)(uint8_t*)>(table[9])(frame);
    }

    // Game thread: rebuild the HUD groups and place the panel texts again after a change
    inline void Frame(int width, int height)
    {
        screenWidth = width;
        screenHeight = height;
        sideWidth = std::max(0, (width - height * 4 / 3) / 2);
        Update(float(width) / float(height));
        if (!owner || !(*reinterpret_cast<uint32_t*>(owner + 12) & 1) || !dirty.exchange(false, std::memory_order_acq_rel))
            return;
        auto model = Model();
        if (!model)
            return;
        for (auto frame = *reinterpret_cast<uint8_t**>(model + 268); frame; frame = *reinterpret_cast<uint8_t**>(frame + 272))
        {
            Invalidate(frame);
            if (frame == panel)
            {
                for (auto child = *reinterpret_cast<uint8_t**>(frame + 268); child; child = *reinterpret_cast<uint8_t**>(child + 272))
                    Invalidate(child);
            }
        }
        // Soldier panels, inventory, weapon panel and info list texts, as the game does when the bottom panel moves
        auto call = [](uint8_t* site, uint8_t* object)
        {
            reinterpret_cast<void(__thiscall*)(uint8_t*)>(injector::GetBranchDestination(site).as_int())(object);
        };
        for (int i = 5; i >= 0; --i)
            call(relayout + 19, owner + 0x107A0 - 0xC0 * (6 - i));
        call(relayout + 33, owner + 0x107E0);
        call(relayout + 44, owner + 0x2E80);
        call(relayout + 55, owner + 0x2F00);
        call(relayout + 66, owner + 0x1081C);
    }

    // The HUD scene is drawn after the world: a visible mask gets black sides first
    inline int __stdcall Render(uint8_t* scene, int flags)
    {
        if (owner && scene == *reinterpret_cast<uint8_t**>(owner) && sideWidth > 0 && HudDevice.get())
        {
            bool masked = false;
            for (auto mask : masks)
                masked |= mask && (*reinterpret_cast<uint32_t*>(mask + 224) & 1);
            if (masked)
            {
                const LONG rects[8]{ 0, 0, sideWidth, screenHeight, screenWidth - sideWidth, 0, screenWidth, screenHeight };
                using Clear = long(__stdcall*)(void*, DWORD, const LONG*, DWORD, DWORD, float, DWORD);
                auto object = HudDevice.get();
                reinterpret_cast<Clear>(reinterpret_cast<uintptr_t*>(*object)[36])(object, 2, rects, 1, 0xFF000000, 1.0f, 0);
            }
        }
        return renderHook.stdcall<int>(scene, flags);
    }

    inline char __fastcall Init(uint8_t* hud, void*)
    {
        owner = hud;
        panel = nullptr;
        anchors.clear();

        ++UiDepth;
        const auto result = initHook.thiscall<char>(hud);
        --UiDepth;
        if (!result)
        {
            owner = nullptr;
            return result;
        }
        auto scene = *reinterpret_cast<uint8_t**>(hud);
        auto table = *reinterpret_cast<uintptr_t**>(scene);
        auto find = reinterpret_cast<uint8_t*(__stdcall*)(uint8_t*, const char*, int)>(table[31]);
        masks[0] = find(scene, "sniper", 0xFFFF);
        masks[1] = find(scene, "scope", 0xFFFF);
        if (!renderHook)
            renderHook = safetyhook::create_inline(reinterpret_cast<void*>(table[30]), Render);
        return result;
    }

    inline char __fastcall Release(uint8_t* hud, void*)
    {
        if (hud == owner)
        {
            owner = nullptr;
            std::fill(std::begin(masks), std::end(masks), nullptr);
            panel = nullptr;
            anchors.clear();
        }
        return releaseHook.thiscall<char>(hud);
    }

    inline void Install(void* init)
    {
        initHook = safetyhook::create_inline(init, Init);
        auto release = hook::pattern("56 8B F1 57 33 FF 8B 56 0C 33 C0 3B D7 0F 94 C0 A8 01 74 05 5F 32 C0 5E C3");
        releaseHook = safetyhook::create_inline(release.get_first(), Release);
        relayout = hook::pattern("8D BE A0 07 01 00 BB 06 00 00 00 81 EF C0 00 00 00 8B CF E8 ? ? ? ? 4B 75 F0 8D 8E E0 07 01 00 E8 ? ? ? ? 8D 8E 80 2E 00 00 E8 ? ? ? ? 8D 8E 00 2F 00 00 E8 ? ? ? ? 8D 8E 1C 08 01 00 E8").get_first<uint8_t>();
        HudDevice.SetAddress(*hook::module_pattern(GetModuleHandleW(L"LS3DF.dll"),
            "8B 0D ? ? ? ? 8B 11 56 8B 74 24 10 6A 00 56 8B 74 24 14 56 50 6A 00 6A 00 51 FF 92 90 00 00 00").get_first<uintptr_t**>(2));
        auto worldMatrix = hook::module_pattern(GetModuleHandleW(L"LS3DF.dll"),
            "55 8B EC 83 E4 F0 83 EC 44 53 8B D9 8B 83 E0 00 00 00 A8 10 56 57");
        worldMatrixHook = safetyhook::create_inline(worldMatrix.get_first(), WorldMatrix);
    }
}
