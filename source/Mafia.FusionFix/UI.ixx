module;

#include <stdafx.h>
#include <cmath>
#include <map>

export module UI;

import ComVars;

struct UIVertex
{
    float x, y, z, rhw;
    uint32_t color, specular;
    float u, v;
};
static_assert(sizeof(UIVertex) == 32);
SafetyHookInline uiDrawHook;

uint32_t __stdcall DrawUI(void* graph, int type, int count, const UIVertex* vertices, int flags)
{
    float scale = Screen.fHeight / 600.0f;
    if (flags != 1 || (type != 1 && type != 2) || count <= 0 || count > 65536 || !vertices || scale <= 1.0f)
        return uiDrawHook.stdcall<uint32_t>(graph, type, count, vertices, flags);

    // D3D8 lines have a fixed one-pixel width. Submit each UI line as two
    // triangles, retaining the original positions, colors, UVs and depth.
    std::vector<UIVertex> triangles;
    triangles.reserve(static_cast<size_t>(count) * 6);
    for (int i = 0; i < count; ++i)
    {
        auto a = vertices[type == 1 ? i * 2 : i];
        auto b = vertices[type == 1 ? i * 2 + 1 : i + 1];
        float dx = b.x - a.x, dy = b.y - a.y;
        float length = std::sqrt(dx * dx + dy * dy);
        if (!std::isfinite(length))
            return uiDrawHook.stdcall<uint32_t>(graph, type, count, vertices, flags);
        UIVertex tl = a, tr = b, bl = a, br = b;
        if (length <= 1.5f)
        {
            // The dot crosshair is a one-pixel diagonal. Give it a square
            // footprint without changing its aiming position.
            float x = (a.x + b.x) * 0.5f, y = (a.y + b.y) * 0.5f;
            tl.x = bl.x = x - scale * 0.5f;
            tr.x = br.x = x + scale * 0.5f;
            tl.y = tr.y = y - scale * 0.5f;
            bl.y = br.y = y + scale * 0.5f;
        }
        else
        {
            float x = -dy * scale * 0.5f / length, y = dx * scale * 0.5f / length;
            tl.x -= x; tl.y -= y;
            tr.x -= x; tr.y -= y;
            bl.x += x; bl.y += y;
            br.x += x; br.y += y;
        }
        triangles.insert(triangles.end(), {tl, tr, bl, tr, br, bl});
    }
    return uiDrawHook.stdcall<uint32_t>(graph, 3, count * 2, triangles.data(), flags);
}

export HCURSOR WINAPI LoadCursorHook(HINSTANCE instance, LPCSTR name)
{
    auto cursor = LoadCursorA(instance, name);
    if (!instance && name == MAKEINTRESOURCEA(32512) && pResolutionHeight && *pResolutionHeight > 0)
    {
        float scale = static_cast<float>(*pResolutionHeight) / 600.0f;
        int width = std::max(1, static_cast<int>(std::lround(GetSystemMetrics(SM_CXCURSOR) * scale)));
        int height = std::max(1, static_cast<int>(std::lround(GetSystemMetrics(SM_CYCURSOR) * scale)));
        // Shared stock cursors ignore requested sizes once cached. Copy the
        // actual themed cursor and retain each copy for the window-class lifetime.
        static std::map<std::pair<int, int>, HCURSOR> cursors;
        auto& scaledCursor = cursors[{width, height}];
        if (!scaledCursor)
            scaledCursor = static_cast<HCURSOR>(CopyImage(cursor, IMAGE_CURSOR, width, height, 0));
        if (scaledCursor) return scaledCursor;
    }
    return cursor;
}

export void InitUIScaling()
{
    // The graphics pointer precedes the packed resolution fields in all three
    // versions. Use the object's native Draw method, including renamed engines.
    auto graph = *reinterpret_cast<void**>(reinterpret_cast<uintptr_t>(pResolutionWidth) - 6);
    if (!graph) return;
    auto draw = (*reinterpret_cast<uintptr_t**>(graph))[88 / sizeof(uintptr_t)];
    uiDrawHook = safetyhook::create_inline(reinterpret_cast<void*>(draw), DrawUI);
}

