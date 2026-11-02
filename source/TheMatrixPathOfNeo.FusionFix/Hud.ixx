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
    inline thread_local const char* Root = "";
    inline void (*BeforeRoot)(void*) = nullptr;
    inline void (*PrepareRoot)(void*, const char*) = nullptr;
    inline injector::hook_back<void(__cdecl*)(void*, void*, void*, void*, float, float, float, float, void*, float, float, float)> DrawHook;

    inline bool Contains(const char* text, const char* part)
    {
        return text && StrStrIA(text, part);
    }

    inline int __cdecl DrawRoot(void* root)
    {
        const auto previous = Root;
        Root = *reinterpret_cast<const char**>(static_cast<char*>(root) + 632);
        if (PrepareRoot)
            PrepareRoot(root, Root);
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
        if (Contains(Root, "healthbar") || Contains(Root, "focusbar") || Contains(Root, "weaponselector"))
            return -1;
        if (Contains(Root, "eeg.lou") || Contains(Root, "skillcount") || Contains(Root, "score_counter"))
            return 1;
        return 0;
    }

    inline bool Hud()
    {
        return Contains(Root, "healthbar") || Contains(Root, "focusbar") || Contains(Root, "weaponselector") ||
            Contains(Root, "eeg.lou") || Contains(Root, "prompts") || Contains(Root, "interact") ||
            Contains(Root, "objectives") || Contains(Root, "reticle") || Contains(Root, "countdown") ||
            Contains(Root, "score_counter") || Contains(Root, "skillcount") || Contains(Root, "Rescue");
    }

    inline void __cdecl Draw(void* a1, void* a2, void* a3, void* a4, float red, float green, float blue, float alpha,
        void* material, float depth, float rotation, float flags)
    {
        if (!a1 || !a2 || !a3 || !a4 || !material || Screen.fAspectRatio <= 0.0f)
            return DrawHook.fun(a1, a2, a3, a4, red, green, blue, alpha, material, depth, rotation, flags);
        const auto name = static_cast<const char*>(material) + 4;
        std::array<float, 2> first{}, last{}, uvFirst{}, uvLast{};
        std::memcpy(first.data(), a1, sizeof(first));
        std::memcpy(last.data(), a2, sizeof(last));
        std::memcpy(uvFirst.data(), a3, sizeof(uvFirst));
        std::memcpy(uvLast.data(), a4, sizeof(uvLast));
        const bool full = first[0] <= 0.001f && last[0] >= 0.999f && first[1] <= 0.001f && last[1] >= 0.999f;
        bool movie = false;
        for (const auto candidate : movies)
        {
            if (_strnicmp(name, candidate, std::strlen(candidate)) == 0)
            {
                movie = true;
                break;
            }
        }
        const bool effect = Contains(name, "film_scratch") || Contains(name, "oldfilm_noise") ||
            Contains(name, "damage_glow") || Contains(Root, "FadeToBlack") ||
            (full && Contains(name, "Black_888"));
        const bool movieFrame = movie && last[0] - first[0] >= 0.9f && last[1] - first[1] >= 0.4f &&
            std::abs(first[0] + last[0] - 1.0f) < 0.1f;
        if (movieFrame)
        {
            NeoCinema::MovieSeen = true;
            const float sourceAspect = NeoLayout::DesignAspect * (last[0] - first[0]) / (last[1] - first[1]);
            // The whole movie at its own aspect: Original inside the game's centred 4:3 area, Fill inside the whole screen
            const float screen = Screen.fAspectRatio;
            const float area = FixSettings().Int("MAIN.FMVWidescreenMode") == 0 ? NeoLayout::DesignAspect : screen;
            const float width = std::min(1.0f, area / screen) * std::min(1.0f, sourceAspect / area);
            const float height = std::min(1.0f, screen / area) * std::min(1.0f, area / sourceAspect);
            first = { (1.0f - width) * 0.5f, (1.0f - height) * 0.5f };
            last = { 1.0f - first[0], 1.0f - first[1] };
        }
        else if (!effect)
        {
            const auto transform = NeoLayout::Box(Screen.fAspectRatio, Hud() ? Constraint.load() : NeoLayout::DesignAspect, Anchor(name));
            first = { transform.X(first[0]), transform.Y(first[1]) };
            last = { transform.X(last[0]), transform.Y(last[1]) };
        }
        return DrawHook.fun(first.data(), last.data(), uvFirst.data(), uvLast.data(), red, green, blue, alpha,
            material, depth, rotation, flags);
    }

    inline void Install()
    {
        Constraint = NeoLayout::ParseAspect(FixSettings().String("MAIN.HudAspectRatioConstraint"));
        FixSettings().OnChange("MAIN.HudAspectRatioConstraint", []()
        {
            Constraint = NeoLayout::ParseAspect(FixSettings().String("MAIN.HudAspectRatioConstraint"));
        });
        auto root = hook::pattern("83 EC 08 A1 ? ? ? ? 8B 48 10 85 C9 C7 44 24 00 00 00 80 3F");
        auto draw = hook::pattern("E8 ? ? ? ? 83 C4 30 5F 5E 5B 8B E5 5D C3");
        RootHook = safetyhook::create_inline(root.get_first(), DrawRoot, SafetyHookInline::StartDisabled);
        (void)RootHook.enable();
        if (RootHook)
        {
            // The render thread may already draw: the original target is set before the call is redirected
            DrawHook.fun = injector::GetBranchDestination(draw.get_first()).get();
            injector::MakeCALL(draw.get_first(), Draw, true);
        }
    }
}
