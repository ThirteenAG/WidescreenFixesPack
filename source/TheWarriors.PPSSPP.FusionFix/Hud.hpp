#pragma once

// Canvas positions are authored in screen fractions and converted to 2D units
// by one device function when a widget is placed or a string is laid out.
// Side elements move to the screen edges (or to the HUD box). Widgets are
// classified by their position (wide panels by their extent) and text by the
// first run of its widget, so boxes, their text and multi-part strings stay
// together. Positions are converted when an element is created, so after a
// window resize older elements keep their place until they are rebuilt.
namespace Hud
{
inline bool textActive;
inline float textX, textY;
// One text widget can lay out several runs (lines, colored words); they all
// follow the side of its first run.
struct Group { bool active, decided; float x, y; };
inline Group group;
inline uintptr_t widget;
inline SafetyMipsInline convertHook, textHook, textWidgetHook, positionHook, radarHooks[3], pauseHooks[2];
inline injector::hook_back<int(int, int, float*, int, int, int, unsigned char, unsigned, float, float, float, float**)> text;
inline injector::hook_back<int(uintptr_t, float*, unsigned char)> position;
inline injector::hook_back<float*(float*, uintptr_t, unsigned char, int, unsigned char)> textWidget;
inline injector::hook_back<int(uintptr_t, float*)> radar[3];
inline injector::hook_back<int(uintptr_t)> pauseStart, pauseFinish;

// Pause menus are laid out around the center; only the gameplay HUD and the
// front end follow the screen edges.
inline bool paused;

inline float Side(float center, float limit = 0.3f)
{
    if (paused)
        return 0.0f;
    return center < limit ? -1.0f : center > 1.0f - limit ? 1.0f : 0.0f;
}

// Vertically only the top and bottom rows move, so menu lists stay together.
inline float Row(float center)
{
    return Side(center, 0.15f);
}

// Wide boxes (tutorial and message panels) are centered near the middle but
// start at a screen edge.
inline float WideSide(float center, float half)
{
    if (paused || half <= 0.0f || half >= 0.5f)
        return Side(center);
    if (center < 0.5f && center - half < 0.02f)
        return -1.0f;
    if (center > 0.5f && center + half > 0.98f)
        return 1.0f;
    return Side(center);
}

inline void Convert(uintptr_t device, float* coordinates)
{
    const float width = *reinterpret_cast<const float*>(device + 0x1890);
    const float height = *reinterpret_cast<const float*>(device + 0x1894);
    const float base = width / height;
    const float x = coordinates[0], y = coordinates[2];
    float sideX, sideY;
    if (textActive)
    {
        sideX = Side(textX);
        sideY = Row(textY);
    }
    else if (widget)
    {
        // Widget size in 2D units: the native canvas is base units wide, 1 high.
        sideX = WideSide(x, *reinterpret_cast<const float*>(widget + 128) / base * 0.5f);
        sideY = Row(y);
    }
    else
    {
        sideX = Side(x);
        sideY = Row(y);
    }
    coordinates[1] = 1.1f;
    coordinates[2] = 0.5f - y - sideY * Display::shiftY;
    coordinates[0] = base * x - base * 0.5f + sideX * Display::shiftX;
}

inline int Text(int font, int string, float* size, int flags, int a4, int a5, unsigned char a6, unsigned length,
    float x, float y, float scale, float** vertices)
{
    const bool active = textActive;
    const float previousX = textX, previousY = textY;
    textActive = true;
    textX = x;
    textY = y;
    if (group.active)
    {
        if (!group.decided)
            group = { true, true, x, y };
        textX = group.x;
        textY = group.y;
    }
    const int result = text.fun(font, string, size, flags, a4, a5, a6, length, x, y, scale, vertices);
    textActive = active;
    textX = previousX;
    textY = previousY;
    return result;
}

inline int PauseStart(uintptr_t screen)
{
    paused = true;
    return pauseStart.fun(screen);
}

inline int PauseFinish(uintptr_t screen)
{
    const int result = pauseFinish.fun(screen);
    paused = false;
    return result;
}

inline float* TextWidget(float* out, uintptr_t object, unsigned char cached, int a3, unsigned char a4)
{
    const auto previous = group;
    group = { true, false, 0.0f, 0.0f };
    const auto result = textWidget.fun(out, object, cached, a3, a4);
    group = previous;
    return result;
}

inline int Position(uintptr_t object, float* coordinates, unsigned char convert)
{
    const auto previous = widget;
    widget = object;
    const int result = position.fun(object, coordinates, convert);
    widget = previous;
    return result;
}

// The radar stores its center in 2D units and places the map and its blips
// around it each frame; move the center to its edge while it is drawn.
template<int Index> int Radar(uintptr_t object, float* data)
{
    static int depth;
    if (depth++)
    {
        const int result = radar[Index].fun(object, data);
        --depth;
        return result;
    }
    // The radar is drawn at its own depth, where offsets come out 5% larger
    // than on the canvas.
    constexpr float plane = 1.0f / 1.05f;
    auto center = reinterpret_cast<float*>(object + 0x30C0);
    const float dx = Side(center[0] / Display::native + 0.5f) * Display::shiftX * plane;
    const float dy = -Side(0.5f - center[2]) * Display::shiftY * plane;
    center[0] += dx;
    center[2] += dy;
    const int result = radar[Index].fun(object, data);
    center[0] -= dx;
    center[2] -= dy;
    --depth;
    return result;
}

inline void Install()
{
    convertHook = safetymips::create_inline(Required("94 18 80 C4 90 18 83 C4 ? ? 02 3C ? ? 44 C4 C3 18 00 46 00 00 A1 C4 08 00 A2 C4"), Convert);
    textHook = safetymips::create_inline(Required("F0 FE BD 27 FF 00 4A 31 FF FF 02 24 E0 00 BE AF 21 F0 20 01"), Text);
    text.fun = textHook.original<decltype(text.fun)>();
    textWidgetHook = safetymips::create_inline(Required("B0 F8 BD 27 24 07 B5 AF FF 00 08 31 21 A8 A0 00 10 07 B0 AF FF 00 D0 30"), TextWidget);
    textWidget.fun = textWidgetHook.original<decltype(textWidget.fun)>();
    positionHook = safetymips::create_inline(Required("D0 FF BD 27 10 00 B0 AF 21 40 A0 00 FF 00 C6 30 14 00 BF AF 21 80 80 00 20 00 B4 E7"), Position);
    position.fun = positionHook.original<decltype(position.fun)>();
    pauseHooks[0] = safetymips::create_inline(Required("10 FF BD 27 B8 00 B2 AF 21 90 80 00 ? ? 04 3C ? ? 84 24 D4 00 BF AF C0 00 B4 AF"), PauseStart);
    pauseStart.fun = pauseHooks[0].original<decltype(pauseStart.fun)>();
    pauseHooks[1] = safetymips::create_inline(Required("E0 FF BD 27 18 00 B2 AF 21 90 80 00 ? ? 04 3C ? ? 84 24 1C 00 BF AF 14 00 B1 AF"), PauseFinish);
    pauseFinish.fun = pauseHooks[1].original<decltype(pauseFinish.fun)>();
    // Radar frame, blips and map.
    const uintptr_t radarSites[3]
    {
        Required("50 FD BD 27 64 02 B1 AF ? ? 02 3C 80 02 BF AF 7C 02 B7 AF 78 02 B6 AF"),
        Required("60 FE BD 27 60 01 B4 AF 21 A0 A0 00 54 01 B1 AF 74 01 BF AF 70 01 BE AF"),
        Required("A0 FE BD 27 2C 01 B7 AF ? ? 03 3C ? ? 62 24 14 01 B1 AF 21 B8 A0 00 01 00 05 24"),
    };
    radarHooks[0] = safetymips::create_inline(radarSites[0], Radar<0>);
    radarHooks[1] = safetymips::create_inline(radarSites[1], Radar<1>);
    radarHooks[2] = safetymips::create_inline(radarSites[2], Radar<2>);
    for (int i = 0; i < 3; ++i)
        radar[i].fun = radarHooks[i].original<decltype(radar[i].fun)>();
}
}
