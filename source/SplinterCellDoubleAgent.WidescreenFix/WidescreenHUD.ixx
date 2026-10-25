module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "TrackedHud.hpp"

export module WidescreenHUD;

import ComVars;
import HudIDs;

void** pHudAreaWidgetDtor = nullptr;
void** pHudPlaceholderViewportDtor = nullptr;
void** pHudImageRendererDtor = nullptr;
void** pHudTextLabelDtor = nullptr;
void** pHudUnkDtor = nullptr;

struct Position2D
{
    int16_t X;
    int16_t Y;
};

struct RectBounds
{
    int16_t Left;
    int16_t Right;
    int16_t Top;
    int16_t Bottom;
};

struct ColorRGBA
{
    uint8_t R;
    uint8_t G;
    uint8_t B;
    uint8_t A;
};

struct TilingOffset
{
    float U;
    float V;
    float OffsetU;
    float OffsetV;
};

enum class TextAlignment
{
    LEFT,
    CENTER,
    RIGHT
};

// Common for position-based elements like AREAINST, BUTTONINST, SLIDER,
// and chat-related areas (e.g., A_Goggles, B_interact, A_timer).
// Represents interactive or positioned UI areas.
struct HudAreaWidget
{
    ColorRGBA Color;
    Position2D Position;
};

// Used for most IMAGE elements (e.g., I_Alarms, green/back textures, borders, strokes).
// Handles rectangular bounds and tiling for visual assets.
struct HudImageRenderer
{
    ColorRGBA Color;
    RectBounds Bounds;
};

// Exclusive to TEXT elements (e.g., L_Alarms, L_Timer, L_Text). 
// Manages text rendering with bounds, fonts, and alignment.
struct HudTextLabel
{
    ColorRGBA Color;
    RectBounds Bounds;
};

// Handles partial/optional rectangular bounds for screen-filling or viewport-like elements
// (e.g., PLACEHOLDER for video/fullscreen overlays). Defaults missing <LEFT>/<TOP> to 0;
// supports offsets like -4 for near-edge alignment (e.g., <RIGHT>636</RIGHT> = 640-4).
// Matches patterns where <RIGHT>/<BOTTOM> define extent from origin, common for placeholders or
// dynamic viewports.
struct HudPlaceholderViewport
{
    ColorRGBA Color;
    RectBounds Bounds;
};

struct HudElement
{
    void** pDtor;

    union
    {
        HudAreaWidget AsAreaWidget;
        HudImageRenderer AsImageRenderer;
        HudTextLabel AsTextLabel;
        HudPlaceholderViewport AsPlaceholderViewport;
    };
};

export bool HudNeedsToMoveLeft(uint32_t id)
{
    static const hud_PC elements[] = {
        /*Area:*/hud_PC::id167625891,  // 0 1
        /*Area:*/hud_PC::id1145254289, // 0 -3
        /*Area:*/hud_PC::id1388722629, // 0 57
        /*Area:*/hud_PC::id2329249591, // 0 120
        /*Area:*/hud_PC::id2662917053, // 0 268
        /*Area:*/hud_PC::id1476254476, // 0 373
        /*Area:*/hud_PC::id1388722629, // 0 83
        /*Area:*/hud_PC::id2662917053, // 0 394
        /*Area:*/hud_PC::id1476254476, // 0 500
        /*Area:*/hud_PC::id3477671439, // 0 7
    };

    return std::any_of(std::begin(elements), std::end(elements), [id](hud_PC arr_id)
    {
        return id == std::to_underlying(arr_id);
    });
}

export bool HudNeedsToMoveRight(uint32_t id)
{
    return false;
    //static const hud_PC elements[] = {
    //    hud_PC(0),
    //};
    //
    //return std::any_of(std::begin(elements), std::end(elements), [id](hud_PC hash)
    //{
    //    return id == std::to_underlying(hash);
    //});
}

DAHud::Positions trackedPositions;
SafetyHookInline areaWidgetDtorHook;
SafetyHookInline shsub_106EE220 = {};

void* __fastcall DestroyAreaWidget(void* widget, void* edx, unsigned int flags)
{
    // Keyframes own these values; retire the coordinate before native destruction/free.
    trackedPositions.Remove(widget);
    return areaWidgetDtorHook.unsafe_fastcall<void*>(widget, edx, flags);
}

void __fastcall sub_106EE220(uint32_t* magma, void* edx, void** frame)
{
    shsub_106EE220.unsafe_fastcall(magma, edx, frame);
    if (!magma || !frame || !frame[1])
        return;
    const auto id = magma[1];
    const int direction = HudNeedsToMoveLeft(id) ? -1 : HudNeedsToMoveRight(id) ? 1 : 0;
    auto& element = *reinterpret_cast<HudElement*>(frame[1]);
    if (!direction || !element.pDtor || *element.pDtor != pHudAreaWidgetDtor)
        return;
    if (areaWidgetDtorHook)
        trackedPositions.Add(&element, element.AsAreaWidget.Position.X, direction);
    else
        element.AsAreaWidget.Position.X = int16_t(std::clamp(int(element.AsAreaWidget.Position.X) +
            direction * DAHud::Positions::Offset(Screen.fWidescreenHudOffset), int(INT16_MIN), int(INT16_MAX)));
}

export void RefreshHudConstraint()
{
    if (Screen.nHudWidescreenMode < 1 || Screen.Width <= 0 || Screen.Height <= 0 || Screen.fWidth <= 0.0f || Screen.fHeight <= 0.0f ||
        !std::isfinite(Screen.fWidth) || !std::isfinite(Screen.fHeight) || !std::isfinite(Screen.fAspectRatio))
        return;
    static std::string previous;
    static int previousWidth = 0, previousHeight = 0;
    const auto wanted = FixSettings().Wanted("MAIN.HudAspectRatioConstraint");
    if (wanted == previous && Screen.Width == previousWidth && Screen.Height == previousHeight)
        return;
    if (!areaWidgetDtorHook)
        return;

    Screen.fHudAspectRatioConstraint = ParseWidescreenHudOffset(wanted);
    const auto constraint = Screen.fHudAspectRatioConstraint;
    float width = Screen.fWidth;
    if (constraint && (*constraint < 0.0f || *constraint > 32.0f / 9.0f))
        Screen.fWidescreenHudOffset = *constraint;
    else
    {
        if (constraint)
            width = Screen.fHeight * ClampHudAspectRatio(*constraint, Screen.fAspectRatio);
        Screen.fWidescreenHudOffset = -CalculateWidescreenOffset(width, Screen.fHeight, 800.0f, 600.0f);
    }
    Screen.FilmstripOffset = int32_t(Screen.fWidth - width + float(Screen.FilmstripScaleX) / 5.25f);
    trackedPositions.SetOffset(Screen.fWidescreenHudOffset);
    previous = wanted;
    previousWidth = Screen.Width;
    previousHeight = Screen.Height;
}

export void InitWidescreenHUD()
{
    if (Screen.nHudWidescreenMode < 1)
    {
        FixSettings().Availability("MAIN.HudAspectRatioConstraint", [](std::string_view) { return false; });
        return;
    }

    auto pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "56 8B F1 E8 ? ? ? ? F6 44 24 08 01 74 ? 56 E8 ? ? ? ? 83 C4 04 8B C6 5E C2 04 00");
    pHudAreaWidgetDtor = pattern.get(41).get<void*>(0);
    pHudPlaceholderViewportDtor = pattern.get(47).get<void*>(0);
    pHudImageRendererDtor = pattern.get(52).get<void*>(0);
    pHudUnkDtor = pattern.get(91).get<void*>(0);
    pHudTextLabelDtor = pattern.get(92).get<void*>(0);

    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 51 56 8B F1 8B 4E 0C");
    trackedPositions.SetOffset(Screen.fWidescreenHudOffset);
    areaWidgetDtorHook = safetyhook::create_inline(reinterpret_cast<void*>(pHudAreaWidgetDtor), DestroyAreaWidget);
    if (!areaWidgetDtorHook)
        OutputDebugStringA("Splinter Cell Double Agent: live HUD lifetime tracking unavailable; startup layout retained.\n");
    shsub_106EE220 = safetyhook::create_inline(pattern.get_first(0), sub_106EE220);
    const bool live = bool(areaWidgetDtorHook) && bool(shsub_106EE220);
    FixSettings().Availability("MAIN.HudAspectRatioConstraint", [live](std::string_view) { return live; });
}
// Only expose our exact owned detour for native-menu body validation.
export const SafetyHookInline* NativeMenuAddFrameHook()
{
    return shsub_106EE220 && shsub_106EE220.enabled() &&
        shsub_106EE220.destination_address() == reinterpret_cast<uintptr_t>(sub_106EE220) ? &shsub_106EE220 : nullptr;
}
