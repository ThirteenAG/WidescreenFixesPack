module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <atomic>
#include <unordered_map>

export module HUD;

import ComVars;
import SplitScreen;

// The HUD and the 2D screens. The game lays them out in a 1280x720 space and stretches that to the screen. The fix
// scales them by the screen height instead and places them:
// - HUD sprites in the box of MAIN.HudAspectRatioConstraint (Auto = the whole screen width): sprites on the left
//   third of the layout stay at the box's left edge, sprites on the right third at its right edge, the rest centered.
//   Menus (the GUI units of the front end, options, records, results, loading screens) stay in the centered 16:9 box.
// - Full-screen backgrounds (pause, skills, weapon name strips) are widened to the whole screen.
// - Split screen: each player's HUD in that player's viewport; the game's split HUD is laid out for an 800x360 box.
// The native aspect compensation keeps sprite and font sizes uniform on narrow screens.

namespace
{
    float fAspectRatioInv = 1.0f / defaultAspectRatio;

    // The game moves the HUD gauges vertically for screens narrower than 16:9 by (16:9 / aspect - 1); on wider screens
    // that moved them towards the center. The fix gives those formulas max(16:9, aspect) instead of 16:9.
    float fCockpitAspectRef = defaultAspectRatio;
    std::atomic<float> fHudAspect{ 0.0f };   // MAIN.HudAspectRatioConstraint; 0 = Auto (the whole width)
    thread_local bool bMenuText = false;
    bool bSubtitlesNow = false; // the next HUD sprite is a cutscene subtitle: the game centers it on the screen

    constexpr float fSplitHudWidth = 800.0f;
    constexpr float fSplitHudHeight = 360.0f;

    // Narrow screens retain the native vertical aspect compensation.
    float GetHudDiff()
    {
        return (std::max)(GetDiff(), 1.0f);
    }

    // Parsed once per change: the setting is written by the live link while the render thread draws
    void UpdateHudAspect()
    {
        const std::string s = FixSettings().String("MAIN.HudAspectRatioConstraint");
        float value = 0.0f;
        float w = 0.0f, h = 0.0f;
        if (s.empty() || _stricmp(s.c_str(), "Auto") == 0)
            value = 0.0f;
        else if (sscanf_s(s.c_str(), "%f:%f", &w, &h) == 2 && w > 0.0f && h > 0.0f)
            value = w / h;
        else if (sscanf_s(s.c_str(), "%f", &w) == 1 && w > 0.0f)
            value = w;
        if (value > 0.0f)
            value = std::clamp(value, 4.0f / 3.0f, 32.0f / 9.0f);
        fHudAspect.store(value);
    }

    float GetHudAspectSetting()
    {
        return fHudAspect.load();
    }

    // Split screen with the side-by-side layout: the 800x360 split HUD fitted into the player's viewport
    float GetSideBySideHudScale()
    {
        return (std::min)((float)SplitScreen::GetCurrentResX() / fSplitHudWidth, (float)SplitScreen::GetCurrentResY() / fSplitHudHeight);
    }

    // Offset that centers the 16:9 layout on the screen (or the split HUD in a player's viewport)
    int32_t GetHudOffset()
    {
        if (IsSplitScreenActive())
        {
            if (IsSplitScreenVertical())
                return (int32_t)(((float)SplitScreen::GetCurrentResX() - fSplitHudWidth * GetSideBySideHudScale()) / 2.0f);
            return (int32_t)(((float)SplitScreen::GetCurrentResX() - (float)SplitScreen::GetNativeResX()) / 2.0f);
        }
        return (std::max)((int32_t)((GetResX() - (GetResY() * defaultAspectRatio)) / 2.0f), 0);
    }

    int32_t GetHudOffsetY()
    {
        if (!IsSplitScreenActive() || !IsSplitScreenVertical())
            return 0;
        return (int32_t)(((float)SplitScreen::GetCurrentResY() - fSplitHudHeight * GetSideBySideHudScale()) / 2.0f);
    }

    // Offset of a HUD sprite at x (in the 1280 wide layout) with edge anchoring in the MAIN.HudAspectRatioConstraint box
    int32_t GetAnchoredHudOffset(float layoutX)
    {
        const int32_t center = GetHudOffset();
        if (IsSplitScreenActive() || GetDiff() < 0.999f)
            return center;

        // A box narrower than the 16:9 layout moves the side elements inwards
        const float screenW = (float)GetResX();
        const float layoutW = (float)GetResY() * defaultAspectRatio;
        auto aspect = GetHudAspectSetting();
        const float boxW = aspect > 0.0f ? (std::min)(screenW, (float)GetResY() * aspect) : screenW;

        const float boxLeft = (screenW - boxW) / 2.0f;
        if (layoutX < 1280.0f / 3.0f)
            return (int32_t)boxLeft;
        if (layoutX > 1280.0f * 2.0f / 3.0f)
            return (int32_t)(boxLeft + boxW - layoutW);
        return center;
    }

    // ScaleX/ScaleY: 1280x720 layout units to screen pixels
    float __cdecl ScaleX(int x)
    {
        if (IsSplitScreenActive() && IsSplitScreenVertical())
            return (float)x * GetSideBySideHudScale();
        if (IsSplitScreenActive())
        {
            auto viewport = GetPlayerViewport(0);
            return (float)(viewport[2] - viewport[0]) * (float)x * (1.0f / (SplitScreen::GetRelativeResX() * GetDiff()));
        }
        return (float)(x * GetResX()) * ((1.0f / (GetRelativeResX() * GetHudDiff())));
    }

    float __cdecl ScaleY(int y)
    {
        if (IsSplitScreenActive() && IsSplitScreenVertical())
            return (float)y * GetSideBySideHudScale();
        if (IsSplitScreenActive())
        {
            auto viewport = GetPlayerViewport(0);
            return (float)(viewport[2] - viewport[0])
                * SplitScreen::GetRelativeResY()
                * (1.0f / (SplitScreen::GetRelativeResX() * GetDiff()))
                * (float)y
                * (1.0f / SplitScreen::GetRelativeResY());
        }
        return (float)(y * GetResY()) * (1.0f / GetRelativeResY());
    }

    float __stdcall ScaleX_stdcall(int x)
    {
        return ScaleX(x);
    }

    float __stdcall ScaleY_stdcall(int y)
    {
        return ScaleY(y);
    }

    // Centered in the 16:9 layout box
    float __cdecl ScaleX_center(int x)
    {
        return ScaleX(x) + GetHudOffset();
    }

    float __stdcall ScaleX_center_stdcall(int x)
    {
        return ScaleX(x) + GetHudOffset();
    }

    // The right edge of a full-screen background
    float __stdcall ScaleX_stretch_stdcall(int x)
    {
        return ScaleX(x) + (GetHudOffset() * 2.0f);
    }

    // The class name of the GUI unit drawing a sprite: vtable slot 4 returns its type info, which holds the name at +4
    const char* GetUnitName(void* unit)
    {
        auto vtable = *(uintptr_t**)unit;
        auto dti = ((uintptr_t(__fastcall*)(void*))vtable[4])(unit);
        return dti ? *(const char**)(dti + 4) : nullptr;
    }

    // Menus, loading screens and result screens keep their 16:9 layout; everything else is HUD
    bool IsMenuUnit(void* unit)
    {
        static const std::string_view Menus[] =
        {
            "uIdStartUp", "uIdMenu", "uIdOption", "uIdTitle", "uIdAlert", "uIdCredit", "uIdStaff", "uIdRecord",
            "uIdLeaderboard", "uIdDogTag", "uIdCustomize", "uIdSkillSet", "uIdResult", "uIdSave00", "uIdSignIn",
            "uIdSpecial", "uIdExtra", "uIdTrial", "uIdNowLoading", "uIdCapLogo", "uIdEndlogo", "uIdGameover",
        };
        static std::unordered_map<uintptr_t, bool> known;
        if (!unit)
            return false;
        const auto vtable = *(uintptr_t*)unit;
        if (auto it = known.find(vtable); it != known.end())
            return it->second;
        const auto name = GetUnitName(unit);
        if (!name)
            return false;
        const std::string_view view(name);
        const bool menu = std::any_of(std::begin(Menus), std::end(Menus), [&](std::string_view prefix) { return view.starts_with(prefix); });
        return known.emplace(vtable, menu).first->second;
    }

    // HudScale(x, y, sprite, keepAspect): places a HUD sprite
    void(__stdcall* HudScale)(int32_t* x, int32_t* y, int sprite, uint16_t keepAspect) = nullptr;
    void __stdcall HudScaleHook(int32_t* x, int32_t* y, int sprite, uint16_t keepAspect)
    {
        fAspectRatioInv = 1.0f / (std::max)(GetAspectRatio(), defaultAspectRatio);
        fCockpitAspectRef = (std::max)(GetAspectRatio(), defaultAspectRatio);

        // Sprites are placed by their left edge; text boxes have a width (+16)
        const float width = *(float*)(sprite + 16);
        const float layoutX = (float)*x + (width > 0.0f ? width / 2.0f : 0.0f);
        // x points to the drawing function's own argument; the GUI unit is its first argument, two slots before
        const bool menu = bMenuText || IsMenuUnit(*(void**)((uintptr_t)x - 8));
        int32_t offset = menu ? GetHudOffset() : bSubtitlesNow ? 0 : GetAnchoredHudOffset(layoutX);
        if (bMenuText) keepAspect = 1;
        bSubtitlesNow = false;

        HudScale(x, y, sprite, keepAspect);
        *x += offset;
        *y += GetHudOffsetY();
    }
}

namespace
{
    // The health and ammo gauges are 3D models placed in front of the HUD camera at a fixed distance from the screen's
    // center. They move sideways by as much as the box is wider than 16:9 at their depth: view units per half screen
    // height at depth 1 = fCockpitTanHalfFov. The parts placed inside a gauge (digits, icons) use their own scale.
    constexpr float fCockpitTanHalfFov = 0.2f;
    constexpr float fCockpitPartTanHalfFov = 0.275f;

    // How far a gauge at view position x and depth moves: right-side gauges (the camera's x axis points to the
    // screen's right) to the box's right edge, left-side ones to its left edge
    float GetCockpitOffset(float x, float depth, float tanHalfFov = fCockpitTanHalfFov)
    {
        if (IsSplitScreenActive() || GetDiff() < 0.999f || depth <= 0.0f)
            return 0.0f;
        const float screen = GetAspectRatio();
        const float aspect = GetHudAspectSetting();
        const float box = aspect > 0.0f ? (std::min)(screen, aspect) : screen;
        const float offset = depth * tanHalfFov * (box - defaultAspectRatio);
        return x < -1.0f ? -offset : offset;
    }

    // The health gauges' position (x, y, z), after the game's sway with the camera
    SafetyHookInline shCockpitPosition = {};
    float* __fastcall CockpitPosition(void* unit, void* edx, float* position)
    {
        auto result = shCockpitPosition.unsafe_thiscall<float*>(unit, position);
        fCockpitAspectRef = (std::max)(GetAspectRatio(), defaultAspectRatio);
        position[0] += GetCockpitOffset(position[0], -position[2]);
        return result;
    }

    // The ammo counter's digits: position (x, y, z) of digit n
    SafetyHookInline shAmmoDigitPosition = {};
    void* __fastcall AmmoDigitPosition(void* unit, void* edx, float* position, int digit)
    {
        auto result = shAmmoDigitPosition.unsafe_thiscall<void*>(unit, position, digit);
        position[0] += GetCockpitOffset(position[0], -position[2], fCockpitPartTanHalfFov);
        return result;
    }
}

export class NativeMenuLayoutScope
{
    bool previous;
public:
    NativeMenuLayoutScope() : previous(bMenuText) { bMenuText = true; }
    ~NativeMenuLayoutScope() { bMenuText = previous; }
};

// Subtitles of cutscenes are placed by the game for the whole screen
export void SetSubtitlesNow()
{
    bSubtitlesNow = true;
}

export float GetHudAspectRatioInv()
{
    return fAspectRatioInv;
}

class HUD
{
public:
    HUD()
    {
        WFP::onInitEvent() += []()
        {
            UpdateHudAspect();
            FixSettings().OnChange("MAIN.HudAspectRatioConstraint", UpdateHudAspect);

            // The layout's 9:16 factor, used to keep sprites with a fixed aspect square
            static const char* AspectRefs[] =
            {
                "D8 0D ? ? ? ? 6A 00 83 EC 08",
                "F3 0F 10 15 ? ? ? ? 2B C2",
                "F3 0F 10 05 ? ? ? ? F3 0F 5E C1 51 F3 0F 11 44 24",
                "F3 0F 10 05 ? ? ? ? F3 0F 5E C1 52",
                "F3 0F 59 05 ? ? ? ? F3 0F 11 44 24 ? F3 0F 10 05 ? ? ? ? F3 0F 11 44 24",
                "F3 0F 10 0D ? ? ? ? F3 0F 5E C8 0F 2F CB",
                "F3 0F 10 05 ? ? ? ? F3 0F 5E C1 F3 0F 11 44 24 ? F3 0F 11 44 24",
            };
            for (auto ref : AspectRefs)
            {
                auto pattern = hook::pattern(ref);
                injector::WriteMemory(pattern.get_first(ref[0] == 'D' ? 2 : 4), &fAspectRatioInv, true);
            }

            // HUD sprites
            auto pattern = hook::pattern("E8 ? ? ? ? 8B CE E8 ? ? ? ? 8B 54 24 24");
            auto pattern2 = hook::pattern("E8 ? ? ? ? 8B CE E8 ? ? ? ? 66 0F 6E 44 24 ? 8B 54 24 10");
            HudScale = (decltype(HudScale))injector::GetBranchDestination(pattern.get_first()).as_int();
            injector::MakeCALL(pattern.get_first(), HudScaleHook, true);
            injector::MakeCALL(pattern2.get_first(), HudScaleHook, true);

            // The layout scale functions
            pattern = hook::pattern("51 8B 0D ? ? ? ? E8 ? ? ? ? 83 F8 01 75 1F");
            injector::MakeJMP(pattern.get_first(), ScaleX, true);
            pattern = hook::pattern("8B 0D ? ? ? ? 83 EC 08 E8 ? ? ? ? 83 F8 01 75 2D");
            injector::MakeJMP(pattern.get_first(), ScaleY, true);
            pattern = hook::pattern("51 8B 0D ? ? ? ? E8 ? ? ? ? 83 F8 01 75 21");
            injector::MakeJMP(pattern.get_first(), ScaleX_stdcall, true);
            pattern = hook::pattern("8B 0D ? ? ? ? 83 EC 08 E8 ? ? ? ? 83 F8 01 75 2F");
            injector::MakeJMP(pattern.get_first(), ScaleY_stdcall, true);

            // Elements centered in the 16:9 box: the skills screen background and other centered panels
            static const char* CenteredX[] =
            {
                "E8 ? ? ? ? E8 ? ? ? ? 89 06 8B 74 24 18",
                "E8 ? ? ? ? D9 5E 10 F3 0F 2C 4E",
                "E8 ? ? ? ? D9 5C 24 24 8B 55 0C",  // skills background
                "E8 ? ? ? ? D9 5C 24 3C 8B 45 0C",
            };
            for (auto ref : CenteredX)
            {
                pattern = hook::pattern(ref);
                injector::MakeCALL(pattern.get_first(), ScaleX_center, true);
            }

            // Rectangles: left edges stay, right edges of full-screen backgrounds go to the screen's right edge
            struct RectCall { const char* pattern; void* func; };
            static const RectCall RectCalls[] =
            {
                { "E8 ? ? ? ? D9 5E 04 8B 4C 24 14", ScaleX_stdcall },
                { "E8 ? ? ? ? D9 5E 0C 8B 44 24 1C", ScaleX_stretch_stdcall },
                { "E8 ? ? ? ? D9 5E 04 A1", ScaleX_stdcall },                     // pause background
                { "E8 ? ? ? ? D9 5E 0C A1", ScaleX_stretch_stdcall },
                { "E8 ? ? ? ? D9 5E 04 8B 4C 24 18 51 8B CF E8 ? ? ? ? D9 5E 08 8B 54 24 1C 52 8B CF E8 ? ? ? ? D9 5E 0C", ScaleX_center_stdcall }, // weapon name background
                { "E8 ? ? ? ? D9 5E 0C 8B 44 24 20 50", ScaleX_stdcall },
                { "E8 ? ? ? ? D9 5E 04 8B 54 24 18 52 8B CF E8 ? ? ? ? D9 54 24 30", ScaleX_stdcall },
                { "E8 ? ? ? ? D9 56 0C DA 4C 24 20 8B 4C 24 24", ScaleX_stretch_stdcall },
                { "E8 ? ? ? ? D9 5E 04 8B 4C 24 18 51 8B CF E8 ? ? ? ? D9 5E 08 8B 54 24 1C 52 8B CF E8 ? ? ? ? D9 56 0C", ScaleX_stdcall },
                { "E8 ? ? ? ? D9 56 0C DA 4C 24 20 8B 4C 24 28", ScaleX_stretch_stdcall },
                { "E8 ? ? ? ? D9 5E 04 8B 54 24 18 52 8B CF E8 ? ? ? ? 83 EC 08", ScaleX_stdcall },
                { "E8 ? ? ? ? D9 5E 0C 8B 44 24 20 F3 0F 10 05", ScaleX_stretch_stdcall },
            };
            for (auto& call : RectCalls)
            {
                pattern = hook::pattern(call.pattern);
                injector::MakeCALL(pattern.get_first(), call.func, true);
            }

            // The gauge's position, after the game's sway with the camera
            pattern = hook::pattern("8B 81 0C 02 00 00 8B 80 24 33 00 00 85 C0 0F 84");
            shCockpitPosition = safetyhook::create_inline(pattern.get_first(), CockpitPosition);

            // The gauges' vertical aspect compensation (16:9 / aspect): health and ammo of each character, their
            // digits and icons, and the radio messages
            static const struct { const char* pattern; size_t count; } AspectRefs16x9[] =
            {
                { "F3 0F 10 0D ? ? ? ? F3 0F 5E C8 F3 0F 5C 0D ? ? ? ? F3 0F 10 44 24 14 F3 0F 59 0D", 5 },
                { "F3 0F 10 15 ? ? ? ? F3 0F 5E D0 F3 0F 10 44 24 14 F3 0F 5C D1 F3 0F 59 15", 4 },
                { "F3 0F 10 25 ? ? ? ? F3 0F 5E E1 F3 0F 5C 25", 1 },
                { "F3 0F 10 15 ? ? ? ? 89 44 24 10 F3 0F 5E D1", 1 },
                { "F3 0F 10 15 ? ? ? ? F3 0F 5E D1 F3 0F 5C 15", 1 },
                { "F3 0F 10 15 ? ? ? ? F3 0F 5E D0 F3 0F 5C 15", 1 },
            };
            for (auto& ref : AspectRefs16x9)
            {
                pattern = hook::pattern(ref.pattern);
                pattern.count(ref.count).for_each_result([](hook::pattern_match match)
                {
                    const auto constant = *match.get<float*>(4);
                    if (*constant == defaultAspectRatio)
                        injector::WriteMemory(match.get<void>(4), &fCockpitAspectRef, true);
                });
            }

            pattern = hook::pattern("66 0F 6E 44 24 08 8B 44 24 04 83 EC 08 80 B9 F1 01 00 00 00");
            shAmmoDigitPosition = safetyhook::create_inline(pattern.get_first(), AmmoDigitPosition);

            // The weapon icon and grenade count: each part's position (x, y, z) is passed on the stack
            pattern = hook::pattern("F3 0F 11 44 24 30 E8 ? ? ? ? 53 8D 4C 24 34 51 8B CE E8");
            static auto WeaponPartsHook = safetyhook::create_mid(pattern.get_first(6), [](SafetyHookContext& regs)
            {
                auto position = *(float**)regs.esp;
                position[0] += GetCockpitOffset(position[0], -position[2], fCockpitPartTanHalfFov);
            });

            // The other gauges of this kind: their position (x, y, z) is on the stack at esp+0x10
            pattern = hook::pattern("F3 0F 10 15 ? ? ? ? F3 0F 5E D0 F3 0F 10 44 24 14 F3 0F 5C D1 F3 0F 59 15");
            static std::vector<SafetyHookMid> AmmoPositionHooks;
            pattern.count(4).for_each_result([](hook::pattern_match match)
            {
                AmmoPositionHooks.push_back(safetyhook::create_mid(match.get<void>(0), [](SafetyHookContext& regs)
                {
                    auto position = (float*)(regs.esp + 0x10);
                    position[0] += GetCockpitOffset(position[0], -position[2]);
                }));
            });
        };
    }
} HUD;
