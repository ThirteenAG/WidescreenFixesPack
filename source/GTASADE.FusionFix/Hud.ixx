module;

#include "stdafx.h"
#include <string_view>

export module Hud;

import Settings;
import Build;

namespace
{
    thread_local bool isDefaultWidget = false;
    SafetyHookInline parseWidgetHook;
    SafetyHookMid beforeScanHook, afterScanHook;
    struct WidgetLayout
    {
        float x = 0, y = 0, width = 0, height = 0;
        bool valid = false;
    };
    WidgetLayout playerInfo, radar;
    struct ScanOutputs
    {
        float *x = nullptr, *y = nullptr, *width = nullptr, *height = nullptr;
        bool playerInfo = false, radar = false;
    };
    thread_local ScanOutputs outputs;

    uintptr_t ParseWidgetPositionLineFromFile(const char* widget, char isDefault)
    {
        const auto previous = std::exchange(isDefaultWidget, isDefault != 0);
        const auto previousOutputs = outputs;
        const auto result = parseWidgetHook.ccall<uintptr_t>(widget, isDefault);
        isDefaultWidget = previous;
        outputs = previousOutputs;
        return result;
    }

    void CaptureScan(SafetyHookContext& context)
    {
        outputs = {};
        const std::string_view line(reinterpret_cast<const char*>(context.rcx));
        outputs.playerInfo = line.contains("WIDGET_POSITION_PLAYER_INFO");
        outputs.radar = line.contains("WIDGET_POSITION_RADAR");
        if (!outputs.playerInfo && !outputs.radar)
            return;
        // Windows x64 varargs: buffer, format, name and x use registers;
        // the y/width/height pointers follow in the outgoing stack arguments.
        outputs.x = reinterpret_cast<float*>(context.r9);
        outputs.y = *reinterpret_cast<float**>(context.rsp + 0x20);
        outputs.width = *reinterpret_cast<float**>(context.rsp + 0x28);
        outputs.height = *reinterpret_cast<float**>(context.rsp + 0x30);
    }

    void ScaleWidget(SafetyHookContext& context)
    {
        if (static_cast<int>(context.rax) != 7 || (!outputs.playerInfo && !outputs.radar))
            return;
        auto& baseline = outputs.playerInfo ? playerInfo : radar;
        auto& x = *outputs.x;
        auto& y = *outputs.y;
        auto& width = *outputs.width;
        auto& height = *outputs.height;
        if (isDefaultWidget)
            baseline = {x, y, width, height, true};
        else if (baseline.valid)
        {
            x = baseline.x;
            y = baseline.y;
            width = baseline.width;
            height = baseline.height;
        }
        const auto scale = outputs.playerInfo ? Settings.hudScale : Settings.radarScale;
        const auto sign = outputs.playerInfo ? 1.0f : -1.0f;
        x += sign * width * (1.0f - scale);
        y -= sign * height * (1.0f - scale);
        width *= scale;
        height *= scale;
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
            const auto scanner = HudScanCallAddress();
            if (!parser || !scanner)
                return;
            const auto after = static_cast<uint8_t*>(scanner) + 5;
            // The parser's native scanner and return value stay intact.
            constexpr uint8_t jump[] = {0xE9, 0x62, 0x01, 0x00, 0x00};
            if (!std::equal(std::begin(jump), std::end(jump), after))
                return;
            parseWidgetHook = safetyhook::create_inline(parser, ParseWidgetPositionLineFromFile);
            beforeScanHook = safetyhook::create_mid(scanner, CaptureScan);
            afterScanHook = safetyhook::create_mid(after, ScaleWidget);
            if (!parseWidgetHook || !beforeScanHook || !afterScanHook)
            {
                afterScanHook = {};
                beforeScanHook = {};
                parseWidgetHook = {};
            }
        };
    }
} HudModuleInstance;
