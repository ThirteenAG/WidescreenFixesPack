module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <atomic>
#include <shlwapi.h>

#pragma comment(lib, "shlwapi.lib")


export module Hud;

import ComVars;
import Layout;
import Cinematics;

export namespace NeoHud
{
    inline SafetyHookInline RootHook;
    inline std::atomic<float> Constraint{ 0.0f };
    inline std::atomic<bool> FillMovies{ true };
    inline std::atomic<bool> OptionsSubpageVisible{ false };
    struct HudRoot
    {
        int anchor{};
        bool gameplay{};
        bool fade{};
        bool options{};
        bool hideOptionFocus{};
    };
    inline thread_local HudRoot Root;
    inline void (*BeforeRoot)(void*) = nullptr;
    inline void (*PrepareRoot)(void*, const char*) = nullptr;
    inline injector::hook_back<void(__cdecl*)(void*, void*, void*, void*, float, float, float, float, void*, float, float, float)> DrawHook;

    inline bool Contains(const char* text, const char* part)
    {
        return text && StrStrIA(text, part);
    }

    inline HudRoot ClassifyRoot(const char* name)
    {
        const bool left = Contains(name, "healthbar") || Contains(name, "focusbar") || Contains(name, "weaponselector");
        const bool right = !left && (Contains(name, "eeg.lou") || Contains(name, "skillcount") || Contains(name, "score_counter"));
        return { left ? -1 : right ? 1 : 0,
            left || right || Contains(name, "prompts") || Contains(name, "interact") || Contains(name, "objectives") ||
                Contains(name, "reticle") || Contains(name, "countdown") || Contains(name, "Rescue"),
            Contains(name, "FadeToBlack"),
            Contains(name, "gm_options_PC.lou") };
    }

    inline int __cdecl DrawRoot(void* root)
    {
        const auto previous = Root;
        const auto name = *reinterpret_cast<const char**>(static_cast<char*>(root) + 632);
        Root = ClassifyRoot(name);
        if (PrepareRoot)
            PrepareRoot(root, name);
        if (BeforeRoot)
            BeforeRoot(root);
        const auto result = RootHook.ccall<int>(root);
        Root = previous;
        return result;
    }

    // Top left: health, Focus, the weapon and the allies' portraits. Top right: the operator's radio and the counters.
    // The hit direction arrow of the health display sits in the middle.
    inline int Anchor(const char* material)
    {
        if (Contains(material, "Reticle_arrow"))
            return 0;
        return Root.anchor;
    }

    inline bool MovieFrame(const char* name, const std::array<float, 2>& first, const std::array<float, 2>& last)
    {
        if (!(last[0] - first[0] >= 0.9f && last[1] - first[1] >= 0.4f &&
            std::abs(first[0] + last[0] - 1.0f) < 0.1f))
            return false;
        for (const auto candidate : movies)
        {
            if (_strnicmp(name, candidate, std::strlen(candidate)) == 0)
                return true;
        }
        return false;
    }

    inline void __cdecl Draw(void* a1, void* a2, void* a3, void* a4, float red, float green, float blue, float alpha,
        void* material, float depth, float rotation, float flags)
    {
        if (!a1 || !a2 || !a3 || !a4 || !material || Screen.fAspectRatio <= 0.0f)
            return DrawHook.fun(a1, a2, a3, a4, red, green, blue, alpha, material, depth, rotation, flags);
        const auto name = static_cast<const char*>(material) + 4;
        if (Root.hideOptionFocus && (Contains(name, "hud_focus_icon4") || Contains(name, "hud_focus_glow")))
            return;
        std::array<float, 2> first{}, last{}, uvFirst{}, uvLast{};
        std::memcpy(first.data(), a1, sizeof(first));
        std::memcpy(last.data(), a2, sizeof(last));
        std::memcpy(uvFirst.data(), a3, sizeof(uvFirst));
        std::memcpy(uvLast.data(), a4, sizeof(uvLast));
        if (OptionsSubpageVisible.load(std::memory_order_relaxed) && first[0] > 0.33f && last[0] < 0.43f &&
            first[1] > 0.35f && last[1] < 0.79f && (Contains(name, "hud_line_horizontal") ||
                Contains(name, "hud_line_vertical") || Contains(name, "hud_fill_box")))
            return;
        const bool full = first[0] <= 0.001f && last[0] >= 0.999f && first[1] <= 0.001f && last[1] >= 0.999f;
        if (MovieFrame(name, first, last))
        {
            NeoCinema::MovieSeen = true;
            const float sourceAspect = NeoLayout::DesignAspect * (last[0] - first[0]) / (last[1] - first[1]);
            // The whole movie at its own aspect: Original inside the game's centred 4:3 area, Fill inside the whole screen
            const float screen = Screen.fAspectRatio;
            const float area = FillMovies.load() ? screen : NeoLayout::DesignAspect;
            const float width = std::min(1.0f, area / screen) * std::min(1.0f, sourceAspect / area);
            const float height = std::min(1.0f, screen / area) * std::min(1.0f, area / sourceAspect);
            first = { (1.0f - width) * 0.5f, (1.0f - height) * 0.5f };
            last = { 1.0f - first[0], 1.0f - first[1] };
        }
        else if (!Root.fade && !Contains(name, "film_scratch") && !Contains(name, "oldfilm_noise") &&
            !Contains(name, "damage_glow") && !(full && Contains(name, "Black_888")))
        {
            const auto transform = NeoLayout::Box(Screen.fAspectRatio, Root.gameplay ? Constraint.load() : NeoLayout::DesignAspect, Anchor(name));
            first = { transform.X(first[0]), transform.Y(first[1]) };
            last = { transform.X(last[0]), transform.Y(last[1]) };
        }
        return DrawHook.fun(first.data(), last.data(), uvFirst.data(), uvLast.data(), red, green, blue, alpha,
            material, depth, rotation, flags);
    }

    inline void Install()
    {
        WatchNeoSetting("MAIN.HudAspectRatioConstraint", [](const std::string& value)
        {
            Constraint = NeoLayout::ParseAspect(value);
        });
        WatchNeoSetting("MAIN.FMVWidescreenMode", [](const std::string& value)
        {
            FillMovies = value != "0";
        });
        auto root = hook::pattern("83 EC 08 A1 ? ? ? ? 8B 48 10 85 C9 C7 44 24 00 00 00 80 3F");
        auto draw = hook::pattern("E8 ? ? ? ? 83 C4 30 5F 5E 5B 8B E5 5D C3");
        RootHook = safetyhook::create_inline(root.get_first(), DrawRoot, SafetyHookInline::StartDisabled);
        (void)RootHook.enable();
        // The render thread may already draw: the original target is set before the call is redirected
        DrawHook.fun = injector::GetBranchDestination(draw.get_first()).get();
        injector::MakeCALL(draw.get_first(), Draw, true);
    }
}
