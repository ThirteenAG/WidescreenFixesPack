module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module Hud;

import Cinematics;
import Splitscreen;
import HudLayout;

namespace
{
    std::atomic<float> Constraint{ 0.0f };
    struct Number
    {
        int type = 0, pad = 0;
        double value = 0.0;
    };

    std::optional<double> Get(uintptr_t movie, const char* path)
    {
        Number value;
        const auto method = reinterpret_cast<bool(__thiscall*)(uintptr_t, Number*, const char*)>(
            (*reinterpret_cast<uintptr_t**>(movie))[48 / 4]);
        if (!method(movie, &value, path) || value.type != 3 || !std::isfinite(value.value))
            return std::nullopt;
        return value.value;
    }

    bool Set(uintptr_t movie, const char* path, double value)
    {
        Number number{ 3, 0, value };
        const auto method = reinterpret_cast<bool(__thiscall*)(uintptr_t, const char*, const Number*, int)>(
            (*reinterpret_cast<uintptr_t**>(movie))[44 / 4]);
        return method(movie, path, &number, 0);
    }

    void Adjust(uintptr_t movie, const char* clip, const char* member, double adjustment, bool multiply)
    {
        const std::string base = *clip ? std::string("_root.") + clip : "_root";
        const std::string property = base + "." + member;
        const auto current = Get(movie, property.c_str());
        if (!current)
            return;
        const std::string previousPath = base + ".FixHudLast" + member;
        const auto previous = Get(movie, previousPath.c_str());
        if (adjustment == (multiply ? 1.0 : 0.0) && !previous)
            return;
        const std::string offsetPath = base + ".FixHudAdjustment" + member;
        const auto preceding = Get(movie, offsetPath.c_str());
        if (previous && preceding && *current == *previous && adjustment == *preceding)
            return;
        double native = *current;
        if (previous && preceding && *current == *previous)
            native = multiply ? native / *preceding : native - *preceding;
        const double value = multiply ? native * adjustment : native + adjustment;
        if (!std::isfinite(value) || std::abs(value) > 65536.0)
            return;
        if (value != *current && !Set(movie, property.c_str(), value))
            return;
        // Display properties can round; retain the coordinate Flash actually applied.
        const double applied = Get(movie, property.c_str()).value_or(value);
        if (!previous || *previous != applied)
            Set(movie, previousPath.c_str(), applied);
        if (!preceding || *preceding != adjustment)
            Set(movie, offsetPath.c_str(), adjustment);
    }

    void Move(uintptr_t movie, const char* clip, char axis, double offset)
    {
        const char member[]{ '_', axis, 0 };
        Adjust(movie, clip, member, offset, false);
    }

    bool IsHud(uintptr_t movie)
    {
        const auto path = *reinterpret_cast<const char**>(movie + 4);
        const int count = *reinterpret_cast<int*>(movie + 8);
        if (!path || count < 2 || count > 4096 || path[count - 1] != 0)
            return false;
        const auto name = std::string_view(path, count - 1);
        const auto file = name.substr(name.find_last_of("\\/") + 1);
        constexpr const char* names[] = { "HUD_Single.swf", "HUD_Single.gfx", "HUD_Multi.swf", "HUD_Multi.gfx",
            "HUD_SplitScreen.swf", "HUD_SplitScreen.gfx", "HUD_SplitScreen2.swf", "HUD_SplitScreen2.gfx" };
        return std::any_of(std::begin(names), std::end(names), [&](const char* known)
        {
            return _stricmp(std::string(file).c_str(), known) == 0;
        });
    }

    void UpdateConstraint()
    {
        const auto chosen = ParseWidescreenHudOffset(FixSettings().String("MAIN.HudAspectRatioConstraint"));
        Constraint.store(chosen && std::isfinite(*chosen) && *chosen > 0 ?
            std::clamp(*chosen, 4.0f / 3.0f, 32.0f / 9.0f) : 0.0f, std::memory_order_relaxed);
    }
}

export namespace Hud
{
    void Init()
    {
        UpdateConstraint();
        FixSettings().OnChange("MAIN.HudAspectRatioConstraint", UpdateConstraint);
    }

    SCCHud::Offsets Frame(int width, int height, int left, int top, int viewWidth, int viewHeight,
        int stageWidth = 1280, int stageHeight = 720)
    {
        return SCCHud::Calculate(width, height, left, top, viewWidth, viewHeight, stageWidth, stageHeight,
            Constraint.load(std::memory_order_relaxed));
    }

    void Render(uintptr_t object, uintptr_t view, int32_t* viewport)
    {
        if (!object || !view || !IsHud(object))
            return;
        const auto description = *reinterpret_cast<uintptr_t*>(object + 32);
        if (!description)
            return;
        const int width = *reinterpret_cast<int*>(description + 8), height = *reinterpret_cast<int*>(description + 12);
        if (width <= 0 || height <= 0 || width > 16384 || height > 16384)
            return;
        int player = 0;
        RECT area{ 0, 0, viewport[0], viewport[1] };
        const auto path = *reinterpret_cast<const char**>(object + 4);
        const auto file = std::string_view(path).substr(std::string_view(path).find_last_of("\\/") + 1);
        if (_stricmp(std::string(file).c_str(), "HUD_SplitScreen2.swf") == 0 ||
            _stricmp(std::string(file).c_str(), "HUD_SplitScreen2.gfx") == 0)
            player = 1;
        const bool splitMovie = _strnicmp(std::string(file).c_str(), "HUD_SplitScreen", 15) == 0;
        if (auto local = splitMovie ? GetLocalPlayerViewport(player) : std::nullopt)
        {
            area = *local;
            if (viewport[0] == area.right - area.left && viewport[1] == area.bottom - area.top)
                area = { 0, 0, viewport[0], viewport[1] };
        }
        const int areaWidth = area.right - area.left, areaHeight = area.bottom - area.top;
        const auto offset = Frame(areaWidth, areaHeight, viewport[2] - area.left, viewport[3] - area.top,
            viewport[4], viewport[5], width, height);
        if (viewport[4] > 0 && viewport[5] > 0)
        {
            for (const auto clip : { "BlackFlash", "SuppressionHalo" })
            {
                Adjust(view, clip, "_xscale", std::max(1.0, double(areaWidth) / viewport[4]), true);
                Adjust(view, clip, "_yscale", std::max(1.0, double(areaHeight) / viewport[5]), true);
            }
        }
        struct Anchor { const char* clip; int x; int y; };
        constexpr Anchor anchors[] =
        {
            { "HUDBar", 1, 1 }, { "CosteReload", 1, 1 }, { "HUD_GameMessage", 1, 0 },
            { "Challenge", 1, 0 }, { "InstructionMessage", -1, 0 }, { "CameraInformation", -1, -1 },
            { "HudTimer", 0, -1 }, { "GodModeFeedback", 0, -1 }, { "PiP_Bar", 0, -1 },
            { "HunterMod.HunterStatus", 1, -1 }, { "LastStandMod.HunterStatus", 1, -1 },
            { "CoopObjectives", 1, 0 }, { "EMPHealthObjectives", 1, 0 }
        };
        for (const auto& anchor : anchors)
        {
            if (anchor.x) Move(view, anchor.clip, 'x', anchor.x < 0 ? offset.left : offset.right);
            if (anchor.y) Move(view, anchor.clip, 'y', anchor.y < 0 ? offset.top : offset.bottom);
        }
        const double subtitleInset = viewport[5] > 0 ?
            Cinematics::SubtitleInset(!splitMovie && GetLocalPlayerViewport(0) ? -1 : player, areaWidth, areaHeight) * height / viewport[5] : 0.0;
        Move(view, "HudSubtitle", 'y', offset.bottom - subtitleInset);
        constexpr const char* prompts[] = { "HudAButton", "HudBButton", "HudXButton", "HudYButton", "HudXHoldButton",
            "HudPressRTHButton", "HudPressLTHButton", "HudRBButton", "HudLBButton", "HudRTButton", "HudLTButton",
            "HudDPadUpButton", "HudDPadRightButton", "HudDPadDownButton", "HudDPadLeftButton", "HudStartButton", "HudSelectButton",
            "HudDPadLeftRightButton", "ButtonTips" };
        for (const auto clip : prompts)
        {
            const auto base = std::string("_root.") + clip;
            auto x = Get(view, (base + "._x").c_str());
            const auto previous = Get(view, (base + ".FixHudLast_x").c_str());
            const auto adjustment = Get(view, (base + ".FixHudAdjustment_x").c_str());
            if (x && previous && adjustment && *x == *previous)
                *x -= *adjustment;
            if (x) Move(view, clip, 'x', *x < width * 0.5 ? offset.left : offset.right);
        }

        if (areaWidth > 0 && areaHeight > 0 && viewport[4] > 0 && viewport[5] > 0)
        {
            // Extend the Flash clip without changing the native stage's pixel scale or origin.
            Adjust(view, "", "_xscale", double(viewport[4]) / areaWidth, true);
            Adjust(view, "", "_yscale", double(viewport[5]) / areaHeight, true);
            Move(view, "", 'x', double(viewport[2] - area.left) * width / areaWidth);
            Move(view, "", 'y', double(viewport[3] - area.top) * height / areaHeight);
            viewport[2] = area.left;
            viewport[3] = area.top;
            viewport[4] = areaWidth;
            viewport[5] = areaHeight;
        }
    }
}
