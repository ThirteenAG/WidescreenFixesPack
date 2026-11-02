module;

#include "stdafx.h"
#include "FixSettings.hpp"
#include <string_view>

export module Hud;

import Settings;
import Build;

namespace
{
    constexpr float DesignAspect = 16.0f / 9.0f;
    SafetyHookInline parseWidgetHook, widthHook, heightHook;
    SafetyHookMid storeWidgetHook, boundsHook;
    std::atomic<bool> reloadLayout = false;
    std::atomic<float> hudAspect = 0.0f;
    std::atomic<int> viewportWidth = 1920, viewportHeight = 1080;
    thread_local unsigned parsingLayout = 0;
    thread_local bool defaultLayout = false;
    struct WidgetLayout
    {
        float x, y, width, height;
    };
    std::map<std::string, WidgetLayout, std::less<>> defaults;

    int ViewportWidth()
    {
        return parsingLayout ? 1920 : widthHook.ccall<int>();
    }

    int ViewportHeight()
    {
        return parsingLayout ? 1080 : heightHook.ccall<int>();
    }

    uintptr_t ParseWidgetPositionLineFromFile(const char* widget, char isDefault)
    {
        const auto previousDefault = std::exchange(defaultLayout, isDefault != 0);
        // Convert the native file in its design canvas before placing the
        // result. Native conversion otherwise adds a second, fixed HUD box.
        ++parsingLayout;
        const auto result = parseWidgetHook.ccall<uintptr_t>(widget, isDefault);
        --parsingLayout;
        defaultLayout = previousDefault;
        return result;
    }

    void PlaceWidget(SafetyHookContext& context)
    {
        auto stack = reinterpret_cast<uint8_t*>(context.rsp);
        auto frame = reinterpret_cast<uint8_t*>(context.rbp);
        const std::string_view name(reinterpret_cast<const char*>(frame + 0x90));
        auto& x = *reinterpret_cast<float*>(stack + 0x6C);
        auto& y = *reinterpret_cast<float*>(stack + 0x70);
        auto& width = *reinterpret_cast<float*>(stack + 0x74);
        auto& height = *reinterpret_cast<float*>(stack + 0x68);
        const bool playerInfo = name == "WIDGET_POSITION_PLAYER_INFO";
        const bool radar = name == "WIDGET_POSITION_RADAR";
        if (playerInfo || radar)
        {
            // The saved adjustable file includes the previous HUD scale.
            // Always scale these two widgets from their unscaled file values.
            if (defaultLayout)
                defaults[std::string(name)] = { x, y, width, height };
            else if (const auto it = defaults.find(name); it != defaults.end())
            {
                x = it->second.x;
                y = it->second.y;
                width = it->second.width;
                height = it->second.height;
            }
            const auto scale = playerInfo ? Settings.hudScale : Settings.radarScale;
            const auto side = playerInfo ? 1.0f : -1.0f;
            x += side * width * (1.0f - scale);
            // Positions use 448 vertical units; these widgets' width-based
            // sizes use 360 at the 16:9 design aspect. Keep the outer Y edge.
            y -= side * height * (1.0f - scale) * (448.0f / 360.0f);
            width *= scale;
            height *= scale;
        }
    }

    void PlaceBounds(SafetyHookContext& context)
    {
        if (parsingLayout)
            return;
        auto widget = reinterpret_cast<uint8_t*>(context.rdi);
        const auto flags = *reinterpret_cast<const uint32_t*>(widget + 0x6C);
        // Fixed-canvas screens already use the native centered 4:3 path.
        if (flags & 0x1000)
            return;
        const float screenWidth = float(viewportWidth.load());
        const float screenHeight = float(viewportHeight.load());
        const auto aspect = screenWidth / screenHeight;
        const auto scale = DesignAspect / aspect;
        const auto requested = hudAspect.load();
        const auto box = requested > 0.0f ? std::min(requested, aspect) : aspect;
        const auto inset = (screenWidth - screenHeight * box) * 0.5f;
        auto rect = reinterpret_cast<float*>(widget + 0xD0);
        auto x = (rect[0] + rect[2]) * 0.5f;
        const auto y = (rect[1] + rect[3]) * 0.5f;
        const auto halfWidth = (rect[2] - rect[0]) * 0.5f * scale;
        // Bit 1 makes native widget height use screen width as well. Other
        // widgets use the vertical canvas and need only horizontal correction.
        const auto halfHeight = (rect[3] - rect[1]) * 0.5f * ((flags & 2) ? scale : 1.0f);
        const auto center = screenWidth * 0.5f;
        if (x < center - 0.5f)
            x = inset + x * scale;
        else if (x > center + 0.5f)
            x = screenWidth - inset - (screenWidth - x) * scale;
        else
            x = center + (x - center) * scale;
        rect[0] = x - halfWidth;
        rect[1] = y - halfHeight;
        rect[2] = x + halfWidth;
        rect[3] = y + halfHeight;
    }

    void ReadHudAspect()
    {
        const auto value = ParseWidescreenHudOffset(FixSettings().String("MAIN.HudAspectRatioConstraint"));
        hudAspect = value ? std::clamp(*value, 4.0f / 3.0f, 32.0f / 9.0f) : 0.0f;
    }
}

class HudModule
{
public:
    HudModule()
    {
        WFP::onInitEvent() += []()
        {
            const auto parser = HudParserAddress();
            const auto store = HudStoreAddress();
            const auto bounds = HudBoundsAddress();
            const auto viewport = HudViewportCallsAddress();
            const auto width = reinterpret_cast<void*>(injector::GetBranchDestination(viewport).as_int());
            const auto height = reinterpret_cast<void*>(injector::GetBranchDestination(static_cast<uint8_t*>(viewport) + 12).as_int());
            static const auto reloadLayoutFiles = reinterpret_cast<void(*)()>(HudReloadAddress());
            widthHook = safetyhook::create_inline(width, ViewportWidth);
            heightHook = safetyhook::create_inline(height, ViewportHeight);
            storeWidgetHook = safetyhook::create_mid(store, PlaceWidget);
            boundsHook = safetyhook::create_mid(bounds, PlaceBounds);
            parseWidgetHook = safetyhook::create_inline(parser, ParseWidgetPositionLineFromFile);
            if (!widthHook || !heightHook || !storeWidgetHook || !boundsHook || !parseWidgetHook)
                throw std::runtime_error("Could not install HUD layout hooks");
            ReadHudAspect();
            FixSettings().OnChange("MAIN.HudAspectRatioConstraint", ReadHudAspect);
            for (const auto id : { "MAIN.HudScale", "MAIN.RadarScale" })
                FixSettings().OnChange(id, []() { reloadLayout = true; });
            WFP::onGameProcessEvent() += []()
            {
                const auto width = widthHook.ccall<int>();
                const auto height = heightHook.ccall<int>();
                if (width <= 0 || height <= 0)
                    return;
                viewportWidth = width;
                viewportHeight = height;
                if (reloadLayout.exchange(false))
                    reloadLayoutFiles();
            };
        };
    }
} HudModuleInstance;
