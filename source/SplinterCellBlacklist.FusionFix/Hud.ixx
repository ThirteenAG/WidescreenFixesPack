module;

#include <stdafx.h>
#include <stdexcept>
#include <unordered_map>
#include <mutex>
#include "FixSettings.hpp"

export module Hud;

import ComVars;
import HudFrame;

namespace
{
    struct Point
    {
        float x, y, z;
    };
    struct State
    {
        uintptr_t root = 0;
        Point applied{};
        HudFrame::Offset offset{};
        bool seen = false;
    };
    struct Anchor
    {
        const char* name;
        int x, y;
    };
    constexpr Anchor Anchors[] =
    {
        { "HudGadgets", -1, 1 }, { "HudAdvGadgets", -1, 1 }, { "HudAdvGadgets_MERC", -1, 1 },
        { "HudAdvGadgetsClassic", -1, 1 }, { "HudAdvGadgetsClassic_MERC", -1, 1 },
        { "HudTips", -1, 1 }, { "HudTips_ADV", -1, 1 },
        { "HudWeapon", 1, 1 }, { "HudAdvWeapon", 1, 1 }, { "HudAdvWeapon_MERC", 1, 1 },
        { "HudMarkAndExecute", 1, 0 }, { "HudRadar", -1, 0 }, { "HudMPRadar", -1, 0 },
        { "HudMPBMSRadar", -1, 0 }, { "HudMPHealth", -1, 1 },
        { "HudMPScoreBarTDM", 0, -1 }, { "HudMPScoreBarSVM", 0, -1 },
        { "HudMPScoreBarFL", 0, -1 }, { "HudMPScoreBarCTI", 0, -1 },
        { "HudMPScoreBarTDM_MERC", 0, -1 }, { "HudMPScoreBarSVM_MERC", 0, -1 },
        { "HudMPScoreBarFL_MERC", 0, -1 }, { "HudMPScoreBarCTI_MERC", 0, -1 }
    };
    std::unordered_map<uintptr_t, State> States;
    void* SetRoot = nullptr;
    uintptr_t(__thiscall* GetViewport)(uintptr_t) = nullptr;
    std::atomic<float> Constraint{ 0.0f };
    bool Dx9 = false;
    struct CullRoot
    {
        uintptr_t movie, node;
        float x, y;
    };
    std::mutex CullMutex;
    std::vector<CullRoot> CullRoots, NextCullRoots;
    uintptr_t(__fastcall* ReadRoot)(uintptr_t) = nullptr;
    SafetyHookInline CullRectHook{};
    std::vector<SafetyHookMid> MovieResetHooks;

    void PublishRoots()
    {
        std::lock_guard lock(CullMutex);
        CullRoots.swap(NextCullRoots);
    }

    void ForgetMovie(uintptr_t movie)
    {
        std::lock_guard lock(CullMutex);
        const auto matches = [movie](const auto& root) { return root.movie == movie; };
        std::erase_if(CullRoots, matches);
        std::erase_if(NextCullRoots, matches);
    }

    char __fastcall CullRect(uintptr_t viewport, void* edx, float* out, char orient)
    {
        const auto result = CullRectHook.thiscall<char>(viewport, out, orient);
        if (!result)
            return result;
        std::lock_guard lock(CullMutex);
        for (const auto& root : CullRoots)
        {
            if (ReadRoot(root.node) + 160 != viewport)
                continue;
            // Culling happens before the HUD projection places the original stage on screen.
            out[0] -= root.x;
            out[1] -= root.y;
            out[2] += root.x;
            out[3] += root.y;
            break;
        }
        return result;
    }

    void Set(uintptr_t root, const Point& point)
    {
        if (Dx9)
        {
            const auto position = &point;
            const auto method = SetRoot;
            __asm
            {
                mov esi, root
                mov ecx, position
                call method
            }
        }
        else
            reinterpret_cast<void(__thiscall*)(uintptr_t, const Point*)>(SetRoot)(root, &point);
    }

    const Anchor* Identify(uintptr_t scene)
    {
        const auto id = *reinterpret_cast<const uintptr_t*>(scene + 0x1AC);
        const auto name = id ? *reinterpret_cast<const char* const*>(id) : nullptr;
        const auto count = id ? *reinterpret_cast<const int*>(id + 4) : 0;
        if (!name || count < 2 || count > 128 || name[count - 1])
            return nullptr;
        for (const auto& anchor : Anchors)
            if (_stricmp(name, anchor.name) == 0)
                return &anchor;
        return nullptr;
    }

    void Scene(uintptr_t scene, bool covered)
    {
        const auto anchor = Identify(scene);
        if (!anchor)
            return;
        const auto root = *reinterpret_cast<const uintptr_t*>(scene + 0x1A8);
        if (!root)
            return;
        const auto viewport = GetViewport(scene);
        if (bSplitscreen && !viewport)
            return;
        if (!covered)
        {
            const auto holder = *reinterpret_cast<const uintptr_t*>(scene + 400);
            const auto movie = holder ? *reinterpret_cast<const uintptr_t*>(holder) : 0;
            const auto node = movie ? *reinterpret_cast<const uintptr_t*>(movie + 68) : 0;
            const auto aspect = GetAspectRatio();
            std::lock_guard lock(CullMutex);
            if (node && std::isfinite(aspect) && aspect > 0 &&
                !std::ranges::any_of(NextCullRoots, [node](const auto& entry) { return entry.node == node; }))
                NextCullRoots.push_back({ movie, node, std::max(0.0f, 360.0f * aspect - 640.0f),
                    std::max(0.0f, 640.0f / aspect - 360.0f) });
        }
        auto& state = States[scene];
        if (state.root != root)
            state = { root };
        state.seen = true;
        Point current = *reinterpret_cast<const Point*>(root + 16);
        if (!std::isfinite(current.x) || !std::isfinite(current.y) || !std::isfinite(current.z))
            return;
        auto native = current;
        if (current.x == state.applied.x && current.y == state.applied.y && current.z == state.applied.z)
        {
            native.x -= state.offset.x;
            native.y -= state.offset.y;
        }
        const auto offset = covered ? HudFrame::Offset{} :
            HudFrame::Calculate(GetAspectRatio(), Constraint.load(std::memory_order_relaxed), anchor->x, anchor->y, bSplitscreen);
        const Point target{ native.x + offset.x, native.y + offset.y, native.z };
        if (target.x != current.x || target.y != current.y)
            Set(root, target);
        state.applied = *reinterpret_cast<const Point*>(root + 16);
        state.offset = offset;
    }

    void UpdateConstraint()
    {
        const auto chosen = ParseWidescreenHudOffset(FixSettings().String("MAIN.HudAspectRatioConstraint"));
        Constraint.store(chosen && std::isfinite(*chosen) && *chosen > 0 ?
            std::clamp(*chosen, 4.0f / 3.0f, 32.0f / 9.0f) : 0.0f, std::memory_order_relaxed);
    }
}

export namespace Hud
{
    void Update(uintptr_t manager, bool covered)
    {
        if (!SetRoot || !GetViewport || !manager)
            return;
        {
            std::lock_guard lock(CullMutex);
            NextCullRoots.clear();
        }
        for (auto& [scene, state] : States)
            state.seen = false;
        const auto slots = *reinterpret_cast<const uintptr_t*>(manager + 0x120);
        const auto count = *reinterpret_cast<const int*>(manager + 0x124);
        if (!slots || count < 0 || count > 1024)
            return;
        for (int i = 0; i < count; ++i)
        {
            const auto slot = slots + i * 12;
            if (*reinterpret_cast<const int*>(slot) < -2)
                continue;
            const auto scenes = *reinterpret_cast<const uintptr_t*>(slot + 8);
            const auto data = scenes ? *reinterpret_cast<const uintptr_t* const*>(scenes) : nullptr;
            const auto size = scenes ? *reinterpret_cast<const int*>(scenes + 4) : 0;
            if (!data || size < 0 || size > 512)
                continue;
            for (int j = 0; j < size; ++j)
                if (data[j])
                    Scene(data[j], covered);
        }
        std::erase_if(States, [](const auto& entry) { return !entry.second.seen; });
        PublishRoots();
    }

    void Init()
    {
        auto cull = hook::pattern("55 8B EC 83 EC 1C 33 C0 56 FF 75 0C 89 45 E4 89 45 E8 89 45 EC 89 45 F0 8D 45 E4 50 8B F1");
        auto read = hook::pattern("56 8B C1 8B F0 81 E6 00 F0 FF FF 2B C6 83 E8 1C 99 6A 1C 59 F7 F9 8B 4E 14 5E 8B 44 81 14 83 E0 FE C3");
        auto reset = hook::pattern("8B 49 0C 8B 01 FF 60 24");
        auto viewport = hook::pattern("55 8B EC 83 EC 10 89 4D F0 C7 45 FC 00 00 00 00 8B 45 F0 83 B8 5C 02 00 00 00 74 3D 8B 4D F0 8B 91 5C 02 00 00 8B 82 10 03 00 00 50 E8 ? ? ? ? 83 C4 04 85 C0");
        auto dx11 = hook::pattern("55 8B EC 51 56 8B F1 83 7E 5C 00 0F 84 ? ? ? ? 8B 4D 08 F3 0F 10 01 F3 0F 10 56 10");
        auto dx9 = hook::pattern("55 8B EC 83 EC 08 83 7E 5C 00 0F 84 ? ? ? ? F3 0F 10 01 F3 0F 10 56 10");
        Dx9 = !dx9.empty();
        SetRoot = Dx9 ? dx9.get_first<uint8_t>() : dx11.get_first<uint8_t>();
        GetViewport = reinterpret_cast<decltype(GetViewport)>(viewport.get_first());
        ReadRoot = reinterpret_cast<decltype(ReadRoot)>(read.get_first());
        if (reset.size() != 2)
        {
            throw std::runtime_error("HUD reset signatures unavailable");
        }
        for (size_t i = 0; i < reset.size(); ++i)
        {
            MovieResetHooks.push_back(safetyhook::create_mid(reset.get(i).get<void>(), [](SafetyHookContext& regs)
            {
                ForgetMovie(regs.ecx);
            }));
        }
        CullRectHook = safetyhook::create_inline(cull.get_first(), CullRect);
        UpdateConstraint();
        FixSettings().OnChange("MAIN.HudAspectRatioConstraint", UpdateConstraint);
    }
}
