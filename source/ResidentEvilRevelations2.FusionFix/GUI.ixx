module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "../Shared/PatternCheck.hpp"
#include <unordered_map>

export module GUI;

import ComVars;
import SplitScreen;

// The HUD, menus and 2D screens. Every GUI object (uGUI...) is drawn through one function, which sets the transform
// from the object's 1280x720 layout to the screen in a vertex shader constant (scale x, scale y, offset x, offset y).
// The game scales the layout by the screen height, starting at the left edge; the fix moves it per class:
// - Most objects: the layout centered (in split screen, in the player's viewport).
// - Full-screen layers (fades, damage): stretched over the whole screen.
// - Subtitles: centered, with the text narrowed back to the layout's width.
// - The distant/near command markers: as the game places them (they follow 3D positions).
// The 3D items of the inventory are rendered into the layout and follow it.

namespace
{
    enum class Mode
    {
        Center,     // the 16:9 layout centered (the game draws it at the left edge)
        Stretch,    // over the whole screen
        Subtitles,  // centered, narrowed back to 16:9
        Offset,     // centered, only when drawn for the whole screen
        Vanilla,    // as the game draws it
        Hidden,     // not drawn
    };

    const int* pDisableDamageOverlay = nullptr;
    const int* pDisableFade = nullptr;
    const int* pDisableGUICommandFar = nullptr;

    Mode nCurrentMode = Mode::Center;

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
            if (n == "uBioGUISubtitles")
                mode = Mode::Subtitles;
            else if (n == "uGUIFade" || n == "uGUIDamage" || n == "uGUIDamage2")
                mode = Mode::Stretch;
            else if (n == "uGUICommandBase" || n == "uGUICommandNear")
                mode = Mode::Vanilla;
            else if (n == "uGUICommandFar")
                mode = Mode::Offset;
        }
        known.emplace(vtable, mode);
        return mode;
    }

    Mode GetMode(void* object)
    {
        auto mode = GetClassMode(object);
        if (mode == Mode::Stretch)
        {
            if (auto name = GetClassName(object); name && std::string_view(name) == "uGUIFade")
                return *pDisableFade ? Mode::Hidden : mode;
            return *pDisableDamageOverlay ? Mode::Hidden : mode;
        }
        if (mode == Mode::Offset && *pDisableGUICommandFar)
            return Mode::Hidden;
        return mode;
    }

    SafetyHookInline shDrawGUI = {};
    void __fastcall DrawGUI(void* object, void* edx, void* context)
    {
        auto mode = GetMode(object);
        if (mode == Mode::Hidden)
            return;
        nCurrentMode = mode;
        shDrawGUI.unsafe_fastcall(object, edx, context);
    }

    // The transform the game just computed: constants c[0..3] = scale x, scale y, offset x, offset y (clip space).
    // The game's offset x places the layout's left edge at the viewport's left edge.
    void AdjustTransform(float* c, int32_t width, int32_t height)
    {
        const auto mode = nCurrentMode;
        nCurrentMode = Mode::Center;
        if (mode == Mode::Vanilla)
            return;

        if (mode == Mode::Stretch)
        {
            c[0] *= GetDiff();
            c[2] = -1.0f;
            return;
        }

        float diff = GetDiff();
        float shift = 1.0f - 1.0f / diff;   // centered
        if (IsSplitScreenActive())
        {
            const bool vertical = IsSplitScreenVertical();
            const bool inViewport = vertical ? (width == SplitScreen::GetCurrentResX() || width == SplitScreen::GetCurrentResX() + 1)
                                             : (height == SplitScreen::GetCurrentResY() || height == SplitScreen::GetCurrentResY() + 1);
            if (inViewport)
            {
                // The game's split HUD is laid out for its 800x360 viewport
                const float viewportAspect = vertical ? GetAspectRatio() / 2.0f : GetAspectRatio() * 2.0f;
                diff = viewportAspect / ((float)SplitScreen::NativeResX / (float)SplitScreen::NativeResY);
                if (diff < 1.0f)
                {
                    // Narrower than the layout: fitted to the width, centered vertically
                    c[0] *= diff;
                    c[1] *= diff;
                    c[2] = (c[2] + 1.0f) * diff - 1.0f;
                    c[3] = (c[3] - 1.0f) * diff + diff;
                    return;
                }
                shift = 1.0f - 1.0f / diff;
            }
            else if (height == GetResY())
                shift = 1.0f - 1.0f / (GetAspectRatio() / defaultAspectRatio);
            else
                return;
        }
        else if (mode == Mode::Offset && height != GetResY())
            return;

        if (mode == Mode::Subtitles)
            c[0] /= diff;
        c[2] += shift;
    }

    // Inventory: the 3D item views, placed in the layout
    SafetyHookInline shSetItemViewPos = {};
    void __fastcall SetItemViewPos(void* view, void* edx, int32_t x, int32_t y)
    {
        if (IsSplitScreenActive())
            x += (int32_t)((720.0f * GetAspectRatio()) - (float)SplitScreen::NativeResX);
        else
            x += (int32_t)(((720.0f * GetAspectRatio()) - 1280.0f) / 2.0f);
        shSetItemViewPos.unsafe_fastcall(view, edx, x, y);
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
            pDisableFade = &settings.Int("MAIN.DisableFade");
            pDisableGUICommandFar = &settings.Int("MAIN.DisableGUICommandFar");

            auto pattern = hook::pattern("55 8B EC 83 E4 F8 83 EC 40 8B 45 08");
            if (PatternAvailable(pattern, "ResidentEvilRevelations2.FusionFix: GUI"))
                shDrawGUI = safetyhook::create_inline(pattern.get_first(), DrawGUI);

            // Right after the transform is written: edx = the constants; the viewport's size in the layout space:
            // right at esp+0x30, left at esp+0x0C, bottom at esp+0x10
            pattern = hook::pattern("83 66 4C FD 8B 45 08 F3 0F 10 44 24 38");
            if (PatternAvailable(pattern, "ResidentEvilRevelations2.FusionFix: GUI"))
            {
                static auto TransformHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    AdjustTransform((float*)regs.edx, *(int32_t*)(regs.esp + 0x30) - *(int32_t*)(regs.esp + 0x0C), *(int32_t*)(regs.esp + 0x10));
                });
            }

            pattern = hook::pattern("A1 ? ? ? ? 53 8B D1 56 80 BA 90 00 00 00 00");
            if (PatternAvailable(pattern, "ResidentEvilRevelations2.FusionFix: GUI"))
                shSetItemViewPos = safetyhook::create_inline(pattern.get_first(), SetItemViewPos);
        };
    }
} GUI;
