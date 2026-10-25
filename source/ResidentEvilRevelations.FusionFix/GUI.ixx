module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "../Shared/PatternCheck.hpp"
#include "HudLayout.hpp"
#include <unordered_map>
#include <array>

export module GUI;

import ComVars;

// The HUD, menus and 2D screens. Every GUI object (uGUI...) is drawn through one function, which sets the transform
// from the object's 1280x720 layout to the screen in a vertex shader constant (scale x, scale y, offset x, offset y).
// The game stretches the layout over the whole screen; the fix keeps its 16:9 shape, per class:
// - Most objects: the 16:9 layout centered.
// - Equipment, objectives and the minimap: at the left or right edge of the chosen HUD box.
//   Unclassified widgets stay centered; Auto uses the whole screen width.
// - Backgrounds of the title menu (uGUIScheduler): full width, moved to the center.
// - Subtitles: centered, the text as wide as on a 16:9 screen.
// - Full-screen layers (damage, pause blur): stretched over the whole screen as the game does.

namespace
{
    using RevHudLayout::Mode;

    struct ClassMode
    {
        std::string_view name;
        Mode mode;
    };

    constexpr std::array<ClassMode, 6> HudAnchors
    {{
        { "uGUI_MainEquipWin", Mode::Right },
        { "uGUI_MainEquipWinBlur", Mode::Right },
        { "uGUI_SubEquipWin", Mode::Left },
        { "uGUI_SubEquipWinBlur", Mode::Left },
        { "uGUI_GoalHint", Mode::Left },
        { "uGUI_GoalHintBlur", Mode::Left },
    }};

    uintptr_t* pGameSystem = nullptr;

    bool IsMinimap()
    {
        if (!pGameSystem || !*pGameSystem)
            return false;
        const auto manager = *reinterpret_cast<const uintptr_t*>(*pGameSystem + 44);
        if (!manager)
            return false;
        const auto map = *reinterpret_cast<const uintptr_t*>(manager + 72);
        if (!map)
            return false;
        const auto name = GetClassName(reinterpret_cast<void*>(map));
        if (!name || std::string_view(name) != "uGUI_Map")
            return false;
        // The map stores its original layout position before converting it to screen pixels each frame.
        const auto x = *reinterpret_cast<const float*>(map + 3800);
        const auto y = *reinterpret_cast<const float*>(map + 3804);
        return std::isfinite(x) && std::isfinite(y) && std::abs(x - 856.0f) < 0.01f && std::abs(y + 353.0f) < 0.01f;
    }

    void InitializeMapContext()
    {
        auto pattern = hook::pattern("8B 0D ? ? ? ? E8 ? ? ? ? 8B 48 48 85 C9 0F 84 ? ? ? ? F6 81 48 01 00 00 01 0F 84 ? ? ? ? E8 ? ? ? ? 3C 01");
        auto layout = hook::pattern("D9 46 40 D9 9E D8 0E 00 00 D9 46 44 D9 9E DC 0E 00 00 5E C3");
        if (pattern.size() == 1 && layout.size() == 1)
        {
            const auto site = pattern.get_first<uint8_t>();
            const auto getter = injector::GetBranchDestination(site + 6).get<uint8_t>();
            if (getter && std::memcmp(getter, "\x8B\x41\x2C\xC3", 4) == 0)
                pGameSystem = *reinterpret_cast<uintptr_t**>(site + 2);
        }
        if (!pGameSystem)
            OutputDebugStringA("Revelations: minimap HUD placement unavailable.\n");
    }

    const int* pDisableDamageOverlay = nullptr;

    Mode nCurrentMode = Mode::Center;
    bool bRadarBlip = false;

    Mode GetClassMode(void* object)
    {
        static std::unordered_map<uintptr_t, Mode> known;
        auto vtable = *(uintptr_t*)object;
        auto it = known.find(vtable);
        if (it != known.end())
            return it->second;

        Mode mode = Mode::Center;
        if (auto name = GetClassName(object))
        {
            std::string_view n(name);
            if (n == "uGUI_Map" || n == "uGUI_MapBaseAndHerb" || n == "uGUI_MapBaseAndHerbBlur")
                return IsMinimap() ? Mode::Right : Mode::Center;
            if (n == "uGUISubtitles" || n == "uGUISubtitlesFix")
                mode = Mode::Subtitles;
            else if (n == "uGUIScheduler")
                mode = Mode::Offset;
            else if (n == "uGUI_PauseBlurHD" || n == "uGUI_Damage" || n == "uGUI_DamageChoke")
                mode = Mode::Vanilla;
            else
            {
                for (auto& anchor : HudAnchors)
                {
                    if (anchor.name == n)
                        mode = anchor.mode;
                }
            }
        }
        known.emplace(vtable, mode);
        return mode;
    }

    bool IsDamageOverlay(void* object)
    {
        auto name = GetClassName(object);
        return name && (std::string_view(name) == "uGUI_Damage" || std::string_view(name) == "uGUI_DamageChoke");
    }

    SafetyHookInline shDrawGUI = {};
    void __fastcall DrawGUI(void* object, void* edx, void* context)
    {
        auto mode = GetClassMode(object);
        if (mode == Mode::Vanilla && *pDisableDamageOverlay && IsDamageOverlay(object))
            return;
        const auto previous = nCurrentMode;
        nCurrentMode = mode;
        shDrawGUI.unsafe_fastcall(object, edx, context);
        nCurrentMode = previous;
    }

    // The transform the game just computed: constants c[0..3] = scale x, scale y, offset x, offset y (clip space).
    // The game's layout covers the whole screen width.
    void AdjustTransform(float* c, Mode mode)
    {
        RevHudLayout::Transform(c, mode, GetAspectRatio(), RevHudLayout::Aspect.load());
    }

    // World-map icons use a separate transform, with the same placement as the minimap.
    void(__cdecl* DrawRadarBlip)(void* context) = nullptr;
    void __cdecl DrawRadarBlipHook(void* context)
    {
        const auto previous = bRadarBlip;
        bRadarBlip = true;
        DrawRadarBlip(context);
        bRadarBlip = previous;
    }
}

class GUI
{
public:
    GUI()
    {
        WFP::onInitEvent() += []()
        {
            auto& settings = FixSettings();
            pDisableDamageOverlay = &settings.Int("MAIN.DisableDamageOverlay");
            RevHudLayout::SetAspect(settings.String("MAIN.HudAspectRatioConstraint"));
            settings.OnChange("MAIN.HudAspectRatioConstraint", []()
            {
                RevHudLayout::SetAspect(FixSettings().String("MAIN.HudAspectRatioConstraint"));
            });
            InitializeMapContext();

            auto pattern = hook::pattern("83 EC 34 8B 44 24 38 53 55 56 8B 70 04");
            if (PatternAvailable(pattern, "ResidentEvilRevelations.FusionFix: GUI"))
                shDrawGUI = safetyhook::create_inline(pattern.get_first(), DrawGUI);

            // Right after the transform is written: eax = the constants
            pattern = hook::pattern("83 66 4C FD 4D 74 17 4D 74 0B");
            if (PatternAvailable(pattern, "ResidentEvilRevelations.FusionFix: GUI"))
            {
                static auto TransformHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    const auto mode = nCurrentMode;
                    nCurrentMode = Mode::Center;
                    AdjustTransform((float*)regs.eax, mode);
                });
            }

            pattern = hook::pattern("E8 ? ? ? ? 8B 86 ? ? ? ? 8B 0D ? ? ? ? 83 C4 04");
            if (PatternAvailable(pattern, "ResidentEvilRevelations.FusionFix: GUI"))
            {
                DrawRadarBlip = (decltype(DrawRadarBlip))injector::GetBranchDestination(pattern.get_first()).as_int();
                injector::MakeCALL(pattern.get_first(), DrawRadarBlipHook, true);
            }

            pattern = hook::pattern("83 66 4C FD 8B CE E8 ? ? ? ? 8B 44 24 20");
            if (PatternAvailable(pattern, "ResidentEvilRevelations.FusionFix: GUI"))
            {
                static auto RadarBlipTransformHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    if (bRadarBlip)
                        AdjustTransform((float*)regs.eax, IsMinimap() ? Mode::Right : Mode::Center);
                });
            }
        };
    }
} GUI;
