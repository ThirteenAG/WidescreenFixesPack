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
};

export bool HudNeedsToMoveLeft(uint32_t id)
{
    static const hud_PC elements[] = {
        hud_PC::A_Communications,
        hud_PC::A_timer,
        hud_PC::A_Chat, // Coop text chat
        hud_PC::TEMP_WorkInProgress, // Debug version string
    };

    return std::any_of(std::begin(elements), std::end(elements), [id](hud_PC hash)
    {
        return id == std::to_underlying(hash);
    });
}

export bool HudNeedsToMoveLeftGoggles(uint32_t id)
{
    static const hud_PC elements[] = {
        // Top left goggles borders
        hud_PC::Goggles_border2,
        hud_PC::Goggles_border04,
        hud_PC::Goggles_border_white1,
        // Bottom left goggles borders
        hud_PC::Goggles_border3,
        hud_PC::Goggles_border01,
        hud_PC::Goggles_border_white2,
        // Brackets
        hud_PC::check_left,
    };

    return std::any_of(std::begin(elements), std::end(elements), [id](hud_PC hash)
    {
        return id == std::to_underlying(hash);
    });
}

export bool HudNeedsToMoveRight(uint32_t id)
{
    static const hud_PC elements[] = {
        hud_PC::A_HUD,
        hud_PC::A_Interact,
        hud_PC::B_interact,
        hud_PC::List_interaction,
        hud_PC::Slider_soundmeter,
        hud_PC::A_missionMessage2lines,
        hud_PC::A_missionMessage4lines,
        hud_PC::A_missionMessage6lines,
        hud_PC::A_Goals,
        hud_PC::A_Notes,
        hud_PC::I_Alarms,
        hud_PC::L_Alarms,
        hud_PC::A_Info_teamate,     // Coop teammate's username and health
        hud_PC::I_Talking,          // Coop teammate's microphone icon
        hud_PC::A_icon_coopaction,  // Coop action icon
    };

    return std::any_of(std::begin(elements), std::end(elements), [id](hud_PC hash)
    {
        return id == std::to_underlying(hash);
    });
}

export bool HudNeedsToMoveRightGoggles(uint32_t id)
{
    static const hud_PC elements[] = {
        // Top right goggles borders
        hud_PC::Goggles_border1,
        hud_PC::Goggles_border03,
        hud_PC::Goggles_border_white4,
        // Bottom right goggles borders
        hud_PC::Goggles_border4,
        hud_PC::Goggles_border02,
        hud_PC::Goggles_border_white3,
        // Brackets
        hud_PC::check_right
    };

    return std::any_of(std::begin(elements), std::end(elements), [id](hud_PC hash)
    {
        return id == std::to_underlying(hash);
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
            if (element.pVTable == pHudAreaWidgetVTable)
            {
                element.AsAreaWidget.Position.X -= static_cast<int16_t>(Screen.fWidescreenHudOffset);
            }
            else if (element.pVTable == pHudImageRendererVTable)
            {
                element.AsImageRenderer.Bounds.Left -= static_cast<int16_t>(Screen.fWidescreenHudOffset);
                element.AsImageRenderer.Bounds.Right -= static_cast<int16_t>(Screen.fWidescreenHudOffset);
            }
            else if (element.pVTable == pHudTextLabelVTable)
            {
                element.AsTextLabel.Bounds.Left -= static_cast<int16_t>(Screen.fWidescreenHudOffset);
                element.AsTextLabel.Bounds.Right -= static_cast<int16_t>(Screen.fWidescreenHudOffset);
            }
            else if (element.pVTable == pHudPlaceholderViewportVTable)
            {
                element.AsPlaceholderViewport.Bounds.Left -= static_cast<int16_t>(Screen.fWidescreenHudOffset);
                element.AsPlaceholderViewport.Bounds.Right -= static_cast<int16_t>(Screen.fWidescreenHudOffset);
            }
        }
        else if (HudNeedsToMoveLeftGoggles(id))
        {
            // Left goggles borders get extra 15 pixel offset
            if (element.pVTable == pHudAreaWidgetVTable)
            {
                element.AsAreaWidget.Position.X -= static_cast<int16_t>(Screen.fWidescreenHudOffset) + 15;
            }
            else if (element.pVTable == pHudImageRendererVTable)
            {
                element.AsImageRenderer.Bounds.Left -= static_cast<int16_t>(Screen.fWidescreenHudOffset) + 15;
                element.AsImageRenderer.Bounds.Right -= static_cast<int16_t>(Screen.fWidescreenHudOffset) + 15;
            }
            else if (element.pVTable == pHudTextLabelVTable)
            {
                element.AsTextLabel.Bounds.Left -= static_cast<int16_t>(Screen.fWidescreenHudOffset) + 15;
                element.AsTextLabel.Bounds.Right -= static_cast<int16_t>(Screen.fWidescreenHudOffset) + 15;
            }
            else if (element.pVTable == pHudPlaceholderViewportVTable)
            {
                element.AsPlaceholderViewport.Bounds.Left -= static_cast<int16_t>(Screen.fWidescreenHudOffset) + 15;
                element.AsPlaceholderViewport.Bounds.Right -= static_cast<int16_t>(Screen.fWidescreenHudOffset) + 15;
            }
        }
        else if (HudNeedsToMoveRight(id))
        {
            if (element.pVTable == pHudAreaWidgetVTable)
            {
                element.AsAreaWidget.Position.X += static_cast<int16_t>(Screen.fWidescreenHudOffset);
            }
            else if (element.pVTable == pHudImageRendererVTable)
            {
                element.AsImageRenderer.Bounds.Left += static_cast<int16_t>(Screen.fWidescreenHudOffset);
                element.AsImageRenderer.Bounds.Right += static_cast<int16_t>(Screen.fWidescreenHudOffset);
            }
            else if (element.pVTable == pHudTextLabelVTable)
            {
                element.AsTextLabel.Bounds.Left += static_cast<int16_t>(Screen.fWidescreenHudOffset);
                element.AsTextLabel.Bounds.Right += static_cast<int16_t>(Screen.fWidescreenHudOffset);
            }
            else if (element.pVTable == pHudPlaceholderViewportVTable)
            {
                element.AsPlaceholderViewport.Bounds.Left += static_cast<int16_t>(Screen.fWidescreenHudOffset);
                element.AsPlaceholderViewport.Bounds.Right += static_cast<int16_t>(Screen.fWidescreenHudOffset);
            }
        }
        else if (HudNeedsToMoveRightGoggles(id))
        {
            // Right goggles borders get extra 20 pixel offset
            if (element.pVTable == pHudAreaWidgetVTable)
            {
                element.AsAreaWidget.Position.X += static_cast<int16_t>(Screen.fWidescreenHudOffset) + 20;
            }
            else if (element.pVTable == pHudImageRendererVTable)
            {
                element.AsImageRenderer.Bounds.Left += static_cast<int16_t>(Screen.fWidescreenHudOffset) + 20;
                element.AsImageRenderer.Bounds.Right += static_cast<int16_t>(Screen.fWidescreenHudOffset) + 20;
            }
            else if (element.pVTable == pHudTextLabelVTable)
            {
                element.AsTextLabel.Bounds.Left += static_cast<int16_t>(Screen.fWidescreenHudOffset) + 20;
                element.AsTextLabel.Bounds.Right += static_cast<int16_t>(Screen.fWidescreenHudOffset) + 20;
            }
            else if (element.pVTable == pHudPlaceholderViewportVTable)
            {
                element.AsPlaceholderViewport.Bounds.Left += static_cast<int16_t>(Screen.fWidescreenHudOffset) + 20;
                element.AsPlaceholderViewport.Bounds.Right += static_cast<int16_t>(Screen.fWidescreenHudOffset) + 20;
            }
        }
    });
}