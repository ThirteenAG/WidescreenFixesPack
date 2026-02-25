module;

#include <stdafx.h>

export module WidescreenHUD2;

import ComVars;
import HudIDs;

void* pHudAreaWidgetVTable = nullptr;
void* pHudImageRendererVTable = nullptr;
void* pHudTextLabelVTable = nullptr;
void* pHudPlaceholderViewportVTable = nullptr;

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
    void* pVTable;

    union
    {
        HudAreaWidget AsAreaWidget;
        HudImageRenderer AsImageRenderer;
        HudTextLabel AsTextLabel;
        HudPlaceholderViewport AsPlaceholderViewport;
    };

    void Move(int16_t offset = 0)
    {
        if (pVTable == pHudAreaWidgetVTable)
        {
            AsAreaWidget.Position.X += offset;
        }
        else if (pVTable == pHudImageRendererVTable)
        {
            AsImageRenderer.Bounds.Left += offset;
            AsImageRenderer.Bounds.Right += offset;
        }
        else if (pVTable == pHudTextLabelVTable)
        {
            AsTextLabel.Bounds.Left += offset;
            AsTextLabel.Bounds.Right += offset;
        }
        else if (pVTable == pHudPlaceholderViewportVTable)
        {
            AsPlaceholderViewport.Bounds.Left += offset;
            AsPlaceholderViewport.Bounds.Right += offset;
        }
    }

    void MoveLeft(float offset)
    {
        Move(-static_cast<int16_t>(offset));
    }

    void MoveRight(float offset)
    {
        Move(static_cast<int16_t>(offset));
    }
};

export bool HudNeedsToMoveLeft(uint32_t id)
{
    static const HudID elements[] = {
        AREAINST::A_Communications,
        AREAINST::A_timer,
        AREAINST::A_Chat, // Coop text chat
        TEXT::TEMP_WorkInProgress, // Debug version string
    };

    return std::any_of(std::begin(elements), std::end(elements), [id](HudID hash)
    {
        return id == hash;
    });
}

export bool HudNeedsToMoveLeftGoggles(uint32_t id)
{
    static const HudID elements[] = {
        // Top left goggles borders
        IMAGE::Goggles_border2,
        IMAGE::Goggles_border04,
        IMAGE::Goggles_border_white1,
        // Bottom left goggles borders
        IMAGE::Goggles_border3,
        IMAGE::Goggles_border01,
        IMAGE::Goggles_border_white2,
        // Brackets
        AREAINST::check_left,
    };

    return std::any_of(std::begin(elements), std::end(elements), [id](HudID hash)
    {
        return id == hash;
    });
}

export bool HudNeedsToMoveRight(uint32_t id)
{
    static const HudID elements[] = {
        AREAINST::A_HUD,
        AREAINST::A_Interact,
        BUTTONINST::B_interact,
        LIST::List_interaction,
        SLIDER::Slider_soundmeter,
        AREAINST::A_missionMessage2lines,
        AREAINST::A_missionMessage4lines,
        AREAINST::A_missionMessage6lines,
        AREAINST::A_Goals,
        AREAINST::A_Notes,
        IMAGE::I_Alarms,
        TEXT::L_Alarms,
        AREAINST::A_Info_teamate,     // Coop teammate's username and health
        IMAGE::I_Talking,             // Coop teammate's microphone icon
        AREAINST::A_icon_coopaction,  // Coop action icon
    };

    return std::any_of(std::begin(elements), std::end(elements), [id](HudID hash)
    {
        return id == hash;
    });
}

export bool HudNeedsToMoveRightGoggles(uint32_t id)
{
    static const HudID elements[] = {
        // Top right goggles borders
        IMAGE::Goggles_border1,
        IMAGE::Goggles_border03,
        IMAGE::Goggles_border_white4,
        // Bottom right goggles borders
        IMAGE::Goggles_border4,
        IMAGE::Goggles_border02,
        IMAGE::Goggles_border_white3,
        // Brackets
        AREAINST::check_right
    };

    return std::any_of(std::begin(elements), std::end(elements), [id](HudID hash)
    {
        return id == hash;
    });
}

export void InitWidescreenHUD2()
{
    if (Screen.nHudWidescreenMode <= 1)
        return;

    auto pattern = hook::pattern("C7 00 ? ? ? ? 8B 15 ? ? ? ? 89 50 08 C2 04 00");
    pHudAreaWidgetVTable = *pattern.get_first<void*>(2);

    pattern = hook::pattern("C7 00 ? ? ? ? 89 48 10 89 50 14");
    pHudImageRendererVTable = *pattern.get_first<void*>(2);

    pattern = hook::pattern("C7 00 ? ? ? ? 66 89 50 10");
    pHudTextLabelVTable = *pattern.get_first<void*>(2);

    pattern = hook::pattern("C7 00 ? ? ? ? 8B 15 ? ? ? ? 89 50 08 8B 0D ? ? ? ? 89 48 0C C3");
    pHudPlaceholderViewportVTable = *pattern.get_first<void*>(2);

    pattern = hook::pattern("FF 52 ? 8B CF E8 ? ? ? ? 8D 4C 24 1C");
    static auto MAGMAParserHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        auto id = *(uint32_t*)(regs.esp + 0x18);
        auto& element = *reinterpret_cast<HudElement*>(regs.eax);

        if (HudNeedsToMoveLeft(id))
        {
            element.MoveLeft(Screen.fWidescreenHudOffset);
        }
        else if (HudNeedsToMoveLeftGoggles(id))
        {
            // Left goggles borders get extra 15 pixel offset
            element.MoveLeft(Screen.fWidescreenHudOffset + 15.0f);
        }
        else if (HudNeedsToMoveRight(id))
        {
            element.MoveRight(Screen.fWidescreenHudOffset);
        }
        else if (HudNeedsToMoveRightGoggles(id))
        {
            // Right goggles borders get extra 20 pixel offset
            element.MoveRight(Screen.fWidescreenHudOffset + 20.0f);
        }
    });
}