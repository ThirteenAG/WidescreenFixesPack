module;

#include "stdafx.h"
#include <d3d9.h>
#include <fstream>
#include <sstream>
#include <map>
#include <set>

export module Frontend;

import Screen;

bool bTraceScreenEffects = false;
// Reject repeated draws before constructing strings or touching the log file.
bool BeginScreenEffectTrace(const std::array<uintptr_t, 6>& key)
{
    static std::set<std::array<uintptr_t, 6>> seen;
    return seen.insert(key).second;
}

void TraceScreenEffect(const std::string& key, const std::string& entry)
{
    // Bound each rendering source independently, so animated menu coordinates
    // cannot stop diagnostics before a gameplay effect is encountered.
    static std::map<std::string, uint32_t> entries;
    auto& count = entries[key];
    if (count >= 8)
        return;
    ++count;
    static std::ofstream log(GetThisModulePath() / "USM.ScreenEffects.log", std::ios::trunc);
    log << GetTickCount64() << ' ' << entry << std::endl;
}

SafetyHookInline shDrawComicColor;
void __fastcall DrawComicColor(uintptr_t component, void*, float* info)
{
    auto color = reinterpret_cast<float*>(component + 8);
    if (bTraceScreenEffects && *reinterpret_cast<uint8_t*>(component + 0x1C) &&
        color[3] * info[78] > 0.0f &&
        BeginScreenEffectTrace({ 1, component, std::bit_cast<uint32_t>(color[0]),
            std::bit_cast<uint32_t>(color[1]), std::bit_cast<uint32_t>(color[2]), 0 }))
    {
        std::ostringstream entry;
        entry << "ComicColor backbuffer=" << IsBackBufferScene() << " rgb=" << color[0] << ',' << color[1] << ',' << color[2]
            << " rect=" << info[69] << ',' << info[70] << ',' << info[71] << ',' << info[72]
            << " matrix=";
        for (int i = 0; i < 16; ++i)
            entry << info[i] << ',';
        std::ostringstream key;
        key << "ComicColor " << component << ' ' << IsBackBufferScene();
        entry << " alpha=" << color[3] * info[78];
        TraceScreenEffect(key.str(), entry.str());
    }
    shDrawComicColor.thiscall<void>(component, info);
}

SafetyHookInline shListAddMesh;
void __cdecl ListAddMesh(uintptr_t mesh, const float* matrix, void* params, void* shaderParams)
{
    // The scripted camera flash is a world-space plane, not a HUD quad.
    // Expand its local camera-right/up axes by the same tangent ratios as
    // the Hor+ projection. Preserve its camera-relative position and depth.
    auto name = mesh ? *reinterpret_cast<const uint32_t**>(mesh) : nullptr;
    if (!name || *name != 0x382C7DCB || !matrix || !IsBackBufferScene()) // fx_cam_flash000
        return shListAddMesh.ccall<void>(mesh, matrix, params, shaderParams);

    std::array<float, 16> adjusted;
    std::copy_n(matrix, adjusted.size(), adjusted.begin());
    for (size_t i = 0; i < 3; ++i)
    {
        adjusted[i] *= Screen.fAspectRatioDiff * Screen.fFOVFactor;
        adjusted[4 + i] *= Screen.fFOVFactor;
    }
    shListAddMesh.ccall<void>(mesh, adjusted.data(), params, shaderParams);
}

struct Vector4
{
    float x, y, z, w;
};

float* (__cdecl* pBuildUIMatrix)(float* pBuildUIMatrix) = nullptr;
void(__cdecl* pSetWorldToView)(const float* pSetWorldToView) = nullptr;
int(__thiscall* pUpdateSlider)(int32_t* pUpdateSlider) = nullptr;
void(__cdecl* pSetViewport)(float, float, float, float) = nullptr;
void(__cdecl* pProjectPoint)(Vector4*, Vector4) = nullptr;
float* (__cdecl* pProjectHUDMatrixPoint)(float*, const float*, const float* pProjectHUDMatrixPoint) = nullptr;

void ScaleUIMatrix(float* matrix)
{
    for (size_t i = 0; i < 16; i += 4)
    {
        matrix[i] *= Screen.fHudScaleX;
        matrix[i + 1] *= Screen.fHudScaleY;
    }
}

void** pWhiteTexture = nullptr;
uintptr_t pComicTextureVtable = 0;
SafetyHookInline shDrawComicPanel;

bool HasComicArtwork(uintptr_t panel)
{
    // A panel's linked components share one transform: fit the artwork, border
    // and clipping together. Camera panels and solid-color overlays keep theirs.
    for (auto component = *reinterpret_cast<uintptr_t*>(panel + 0x60); component;
        component = *reinterpret_cast<uintptr_t*>(component + 4))
    {
        if (*reinterpret_cast<uintptr_t*>(component) == pComicTextureVtable)
        {
            auto texture = *reinterpret_cast<void**>(component + 0x28);
            if (texture && texture != *pWhiteTexture)
                return true;
        }
    }
    return false;
}

void __fastcall DrawComicPanel(uintptr_t panel, void*, float** renderInfo)
{
    if (!IsBackBufferScene() || !HasComicArtwork(panel))
        return shDrawComicPanel.thiscall<void>(panel, renderInfo);

    auto matrix = *renderInfo;
    std::array<float, 16> original;
    std::copy_n(matrix, original.size(), original.begin());
    ScaleUIMatrix(matrix);
    // Comic transforms use the 640x480 pixel canvas, not clip-space origin.
    matrix[12] = Screen.HudX(original[12]);
    matrix[13] = Screen.HudY(original[13]);
    shDrawComicPanel.thiscall<void>(panel, renderInfo);
    std::copy(original.begin(), original.end(), matrix);
}

float* __cdecl BuildUIMatrix(float* matrix)
{
    pBuildUIMatrix(matrix);
    if (IsBackBufferScene())
        ScaleUIMatrix(matrix);
    return matrix;
}

void __cdecl SetHUDWorldToView(const float* matrix)
{
    std::array<float, 16> adjusted;
    std::copy_n(matrix, adjusted.size(), adjusted.begin());
    if (IsBackBufferScene())
        ScaleUIMatrix(adjusted.data());
    pSetWorldToView(adjusted.data());
}

int __fastcall SetMenuSliderRect(int32_t* slider, void*, int32_t x, int32_t y, int32_t width, int32_t height)
{
    // PC sliders draw pretransformed D3D vertices, bypassing the NGL UI matrix.
    // Store corrected pixel bounds so native drawing and mouse hit tests agree.
    auto manager = reinterpret_cast<float*>(slider[7]);
    slider[1] = static_cast<int32_t>(Screen.HudX(static_cast<float>(x)) * manager[11]);
    slider[2] = static_cast<int32_t>(Screen.HudY(static_cast<float>(y)) * manager[12]);
    slider[3] = static_cast<int32_t>(Screen.HudX(static_cast<float>(x + width - 1)) * manager[11]);
    slider[4] = static_cast<int32_t>(Screen.HudY(static_cast<float>(y + height - 1)) * manager[12]);
    return pUpdateSlider(slider);
}

SafetyHookInline shSetMenuWidgetRect;
int __fastcall SetMenuWidgetRect(int32_t* widget, void*, int32_t x, int32_t y, int32_t width, int32_t height)
{
    // Borrowed textures include the back-buffer snapshot behind modal dialogs.
    // It already contains the fitted UI and must retain its original screen bounds.
    if (!reinterpret_cast<uint8_t*>(widget)[0x50])
        return shSetMenuWidgetRect.thiscall<int>(widget, x, y, width, height);

    // All PC controls must store fitted pixel bounds for both rendering and hit
    // testing, including text buttons, key-binding columns and scrollbars.
    auto manager = reinterpret_cast<float*>(widget[7]);
    widget[1] = static_cast<int32_t>(Screen.HudX(static_cast<float>(x)) * manager[11]);
    widget[2] = static_cast<int32_t>(Screen.HudY(static_cast<float>(y)) * manager[12]);
    widget[3] = static_cast<int32_t>(Screen.HudX(static_cast<float>(x + width - 1)) * manager[11]);
    widget[4] = static_cast<int32_t>(Screen.HudY(static_cast<float>(y + height - 1)) * manager[12]);
    auto vtable = *reinterpret_cast<uintptr_t**>(widget);
    return reinterpret_cast<int(__thiscall*)(int32_t*)>(vtable[1])(widget);
}

SafetyHookInline shDrawMenuText;
void __fastcall DrawMenuText(float* manager, void*, const char* text, int left, int top, int right, int bottom,
    uint32_t color, int alignment, float scaleX, float scaleY, int offset)
{
    // Native text drawing converts pixel bounds to NGL coordinates. Undo the fit
    // here, before alignment, because NGL applies it again when rendering text.
    auto unscaleX = [&](int x) { return static_cast<int>(std::lround(Screen.UnscaleX(x / manager[11]) * manager[11])); };
    auto unscaleY = [&](int y) { return static_cast<int>(std::lround(Screen.UnscaleY(y / manager[12]) * manager[12])); };
    shDrawMenuText.thiscall<void>(manager, text, unscaleX(left), unscaleY(top), unscaleX(right), unscaleY(bottom),
        color, alignment, scaleX, scaleY, offset);
}

int __fastcall SetMenuHighlightRect(int32_t* widget, void*, int x, int y, int width, int height, int style)
{
    // Selection artwork has a second rectangle, separate from the control's hit box.
    auto manager = reinterpret_cast<float*>(widget[7]);
    widget[34] = static_cast<int32_t>(Screen.HudX(static_cast<float>(x)) * manager[11]);
    widget[35] = static_cast<int32_t>(Screen.HudY(static_cast<float>(y)) * manager[12]);
    widget[36] = static_cast<int32_t>(Screen.HudX(static_cast<float>(x + width - 1)) * manager[11]);
    widget[37] = static_cast<int32_t>(Screen.HudY(static_cast<float>(y + height - 1)) * manager[12]);
    widget[38] = style;
    return style;
}

int(__thiscall* pDrawMenuTexture)(int32_t*) = nullptr;
int __fastcall DrawMenuTexture(int32_t* widget, void*)
{
    auto manager = reinterpret_cast<float*>(widget[7]);
    // Clear behind a full-screen background before drawing its fitted artwork.
    // This leaves black bars without covering the menu or the free-moving cursor.
    if (reinterpret_cast<uint8_t*>(widget)[0x50] &&
        widget[1] == static_cast<int32_t>(Screen.HudX(0.0f) * manager[11]) &&
        widget[2] == static_cast<int32_t>(Screen.HudY(0.0f) * manager[12]) &&
        widget[3] == static_cast<int32_t>(Screen.HudX(639.0f) * manager[11]) &&
        widget[4] == static_cast<int32_t>(Screen.HudY(479.0f) * manager[12]))
    {
        auto device = reinterpret_cast<IDirect3DDevice9**>(manager)[2];
        device->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
    }
    return pDrawMenuTexture(widget);
}

// Shift an entire widget, not individual vertices around the screen midpoint.
// The radar frame, terrain, compass and blips share this offset.
float fWidgetOffset = 0.0f;

float GetWidgetOffset(uintptr_t panel)
{
    if (!panel)
        return 0.0f;
    auto center = *reinterpret_cast<float*>(panel + 0x14); // PanelQuad::GetCenterX
    return center < 320.0f ? -Screen.fHudOffset : Screen.fHudOffset;
}

SafetyHookInline shDrawHealth;
float GetEdgeOffset(float left, float right)
{
    // Only move layouts confined to one side of the original HUD. Layouts that
    // reach the center (including centered challenge prompts) remain centered.
    if (!std::isfinite(left) || !std::isfinite(right) || left > right)
        return 0.0f;
    if (right < 304.0f)
        return -Screen.fHudOffset;
    if (left > 336.0f)
        return Screen.fHudOffset;
    return 0.0f;
}

float GetPanelEdgeOffset(uintptr_t panel)
{
    if (!panel)
        return 0.0f;
    float left = INFINITY, right = -INFINITY;
    // PanelFile contains mVector<PanelQuad> and mVector<FEText>. Use their
    // logical anchors, not animated draw matrices or world-projected positions.
    for (auto [vectorOffset, positionOffset] : { std::pair{ 0x00, 0x14 }, std::pair{ 0x14, 0x34 } })
    {
        auto count = *reinterpret_cast<uint32_t*>(panel + vectorOffset + 4);
        auto objects = *reinterpret_cast<uintptr_t**>(panel + vectorOffset + 8);
        for (uint32_t i = 0; objects && i < count; ++i)
        {
            if (!objects[i])
                continue;
            auto x = *reinterpret_cast<float*>(objects[i] + positionOffset);
            if (!std::isfinite(x))
                return 0.0f;
            left = std::min(left, x);
            right = std::max(right, x);
        }
    }
    return GetEdgeOffset(left, right);
}

injector::hook_back<void(__thiscall*)(uintptr_t)> hbDrawEdgePanel;
void __fastcall DrawEdgePanel(uintptr_t panel, void*)
{
    auto savedOffset = fWidgetOffset;
    fWidgetOffset = GetPanelEdgeOffset(panel);
    hbDrawEdgePanel.fun(panel);
    fWidgetOffset = savedOffset;
}

SafetyHookInline shDrawTimer;
int __fastcall DrawTimer(uintptr_t widget, void*)
{
    if (!*reinterpret_cast<uintptr_t*>(widget + 0x04))
        return shDrawTimer.thiscall<int>(widget);
    auto animation = *reinterpret_cast<uintptr_t*>(widget + 0x20);
    if (!*reinterpret_cast<uint8_t*>(widget + 0x2A) &&
        !(animation && *reinterpret_cast<uint8_t*>(animation + 0x2D)))
        return shDrawTimer.thiscall<int>(widget);

    auto savedOffset = fWidgetOffset;
    auto x = *reinterpret_cast<float*>(widget + 0x2C); // Timer's configured horizontal anchor.
    fWidgetOffset = GetEdgeOffset(x, x);
    auto result = shDrawTimer.thiscall<int>(widget);
    fWidgetOffset = savedOffset;
    return result;
}

SafetyHookInline shDrawPursuit;
uint32_t __fastcall DrawPursuit(uintptr_t widget, void*)
{
    // Inactive/unloaded indicators can retain uninitialized or stale quad
    // pointers. Match the native renderer's guards before inspecting the panel.
    if (!*reinterpret_cast<uint8_t*>(widget))
        return shDrawPursuit.thiscall<uint32_t>(widget);
    auto panel = *reinterpret_cast<uintptr_t*>(widget + 0x08);
    if (!panel)
        return shDrawPursuit.thiscall<uint32_t>(widget);

    auto savedOffset = fWidgetOffset;
    fWidgetOffset = GetPanelEdgeOffset(panel);
    auto result = shDrawPursuit.thiscall<uint32_t>(widget);
    fWidgetOffset = savedOffset;
    return result;
}

SafetyHookInline shDrawChallenge;
void __fastcall DrawChallenge(uintptr_t widget, void*)
{
    if (!*reinterpret_cast<uintptr_t*>(widget + 0x04) ||
        !*reinterpret_cast<uint8_t*>(widget + 0x4C) ||
        !(*reinterpret_cast<float*>(widget + 0x48) <= 0.0f))
        return shDrawChallenge.thiscall<void>(widget);

    auto savedOffset = fWidgetOffset;
    float left = INFINITY, right = -INFINITY;
    // The track-and-field gauge constructs its quads individually.
    for (size_t i = 2; i < 18; ++i)
    {
        auto quad = reinterpret_cast<uintptr_t*>(widget)[i];
        if (!quad)
            continue;
        auto x = *reinterpret_cast<float*>(quad + 0x14);
        left = std::min(left, x);
        right = std::max(right, x);
    }
    fWidgetOffset = GetEdgeOffset(left, right);
    shDrawChallenge.thiscall<void>(widget);
    fWidgetOffset = savedOffset;
}

void __fastcall DrawHealth(uintptr_t widget, void*)
{
    auto index = *reinterpret_cast<int32_t*>(widget + 0x38);
    if (index < 0 || index >= *reinterpret_cast<int32_t*>(widget + 0x34))
        return shDrawHealth.thiscall<void>(widget);
    auto panel = reinterpret_cast<uintptr_t*>(widget)[index];
    if (!panel)
        return shDrawHealth.thiscall<void>(widget);

    auto savedOffset = fWidgetOffset;
    // Use the active panel rather than the cached quad retained after DeInit.
    fWidgetOffset = GetPanelEdgeOffset(panel);
    shDrawHealth.thiscall<void>(widget);
    fWidgetOffset = savedOffset;
}

SafetyHookInline shDrawRadar;
SafetyHookInline shDrawRace;
void __fastcall DrawRace(uintptr_t widget, void*)
{
    // fe_distance_race::Draw submits the frame, progress bars and racer icons.
    // Anchor the entire animated widget to the right-hand HUD boundary.
    auto savedOffset = fWidgetOffset;
    fWidgetOffset = Screen.fHudOffset;
    shDrawRace.thiscall<void>(widget);
    fWidgetOffset = savedOffset;
}

char __fastcall DrawRadar(uintptr_t widget, void*)
{
    auto animation = *reinterpret_cast<uintptr_t*>(widget + 0x3A0);
    if (!*reinterpret_cast<uint8_t*>(widget + 0x3A8) &&
        !(animation && *reinterpret_cast<uint8_t*>(animation + 0x2D)))
        return shDrawRadar.thiscall<char>(widget);

    auto savedOffset = fWidgetOffset;
    fWidgetOffset = GetWidgetOffset(*reinterpret_cast<uintptr_t*>(widget + 0x39C));
    auto result = shDrawRadar.thiscall<char>(widget);
    fWidgetOffset = savedOffset;
    return result;
}

void __cdecl SetRadarViewport(float left, float top, float right, float bottom)
{
    if (IsBackBufferScene())
    {
        left = Screen.HudX(left + fWidgetOffset);
        top = Screen.HudY(top);
        // nglSetViewport uses inclusive right/bottom coordinates.
        right = Screen.HudX(right + 1.0f + fWidgetOffset) - 1.0f;
        bottom = Screen.HudY(bottom + 1.0f) - 1.0f;
    }
    pSetViewport(left, top, right, bottom);
}

void __cdecl ProjectHUDPoint(Vector4* result, Vector4 point)
{
    pProjectPoint(result, point);
    if (IsBackBufferScene())
    {
        // NGL returns logical screen coordinates AFTER applying the viewport.
        // Convert back before the game clamps the blip and places its HUD quad.
        result->x = Screen.UnscaleX(result->x) - fWidgetOffset;
        result->y = Screen.UnscaleY(result->y);
    }
}

float* __cdecl ProjectHUDMatrixPoint(float* result, const float* matrix, const float* point)
{
    // Map and enemy indicators use geometry_manager projection instead of
    // nglProjectPoint. Undo the HUD fit at the anchor, before placing the quad.
    pProjectHUDMatrixPoint(result, matrix, point);
    if (IsBackBufferScene())
    {
        result[0] = Screen.UnscaleX(result[0]);
        result[1] = Screen.UnscaleY(result[1]);
    }
    return result;
}

struct NGLQuad
{
    struct Vertex
    {
        float x, y, u, v;
        uint32_t color;
    } vertices[4];
    float z;
    uint32_t mapFlags;
    uint32_t blendMode;
    uint32_t blendModeConstant;
    void* texture;
};
static_assert(sizeof(NGLQuad) == 0x64);

SafetyHookInline shListAddQuad;

void __cdecl ListAddScreenQuad(const NGLQuad* quad)
{
    if (!quad || !IsBackBufferScene())
        return shListAddQuad.ccall<void>(quad);

    auto adjusted = *quad;
    for (auto& vertex : adjusted.vertices)
    {
        vertex.x = Screen.UnscaleX(vertex.x);
        vertex.y = Screen.UnscaleY(vertex.y);
    }
    shListAddQuad.ccall<void>(&adjusted);
}

void AddPillarbox(float left, float top, float right, float bottom, float z)
{
    NGLQuad quad = {};
    quad.z = z;
    quad.mapFlags = 194;
    quad.blendMode = 2;
    quad.vertices[0] = { left, top, 0.0f, 0.0f, 0xFF000000 };
    quad.vertices[1] = { right, top, 1.0f, 0.0f, 0xFF000000 };
    quad.vertices[2] = { left, bottom, 0.0f, 1.0f, 0xFF000000 };
    quad.vertices[3] = { right, bottom, 1.0f, 1.0f, 0xFF000000 };
    shListAddQuad.ccall<void>(&quad);
}

void DrawMenuPillarboxes()
{
    if (!IsBackBufferScene())
        return;

    // Animated menu panels extend outside the original canvas. Mask that overflow
    // above the menu, but below the cursor (depth 0).
    if (Screen.fHudScaleX < 1.0f)
    {
        AddPillarbox(Screen.UnscaleX(0.0f), 0.0f, 0.0f, 480.0f, 0.5f);
        AddPillarbox(640.0f, 0.0f, Screen.UnscaleX(640.0f), 480.0f, 0.5f);
    }
    if (Screen.fHudScaleY < 1.0f)
    {
        AddPillarbox(0.0f, Screen.UnscaleY(0.0f), 640.0f, 0.0f, 0.5f);
        AddPillarbox(0.0f, 480.0f, 640.0f, Screen.UnscaleY(480.0f), 0.5f);
    }
}

void TraceQuad(const NGLQuad* quad, uintptr_t caller)
{
    if (bTraceScreenEffects && quad)
    {
        float left = quad->vertices[0].x, right = left;
        float top = quad->vertices[0].y, bottom = top;
        bool visible = false;
        for (const auto& vertex : quad->vertices)
        {
            left = std::min(left, vertex.x);
            right = std::max(right, vertex.x);
            top = std::min(top, vertex.y);
            bottom = std::max(bottom, vertex.y);
            visible |= (vertex.color >> 24) != 0;
        }
        uintptr_t shape = static_cast<uintptr_t>(visible) | ((right - left >= 300.0f) << 1) | ((bottom - top >= 200.0f) << 2) |
            ((left <= 1.0f && right >= 639.0f && top <= 1.0f && bottom >= 479.0f) << 3);
        if (!BeginScreenEffectTrace({ 4, caller, reinterpret_cast<uintptr_t>(quad->texture),
            quad->blendMode, quad->vertices[0].color & 0xFFFFFF, shape | (static_cast<uintptr_t>(IsBackBufferScene()) << 4) }))
            return;
        std::ostringstream key, entry;
        entry << "Quad caller=" << reinterpret_cast<void*>(caller) << " backbuffer=" << IsBackBufferScene()
            << " texture=" << quad->texture << " blend=" << quad->blendMode << " vertices=";
        for (auto& vertex : quad->vertices)
            entry << vertex.x << ',' << vertex.y << ',' << vertex.color << ';';
        key << "Quad " << reinterpret_cast<void*>(caller) << ' ' << IsBackBufferScene() << ' '
            << quad->texture << ' ' << quad->blendMode << ' ' << (quad->vertices[0].color & 0xFFFFFF)
            << ' ' << visible << ' ' << (right - left >= 300.0f) << ' ' << (bottom - top >= 200.0f)
            << ' ' << (left <= 1.0f && right >= 639.0f && top <= 1.0f && bottom >= 479.0f);
        TraceScreenEffect(key.str(), entry.str());
    }
}

void __cdecl ListAddQuad(const NGLQuad* quad)
{
    if (bTraceScreenEffects)
        TraceQuad(quad, reinterpret_cast<uintptr_t>(_ReturnAddress()));
    if (!quad || !IsBackBufferScene())
        return shListAddQuad.ccall<void>(quad);

    // NGL copies the quad into its render list; never modify the game's source asset.
    auto adjusted = *quad;
    float left = quad->vertices[0].x, right = left;
    float top = quad->vertices[0].y, bottom = top;
    for (auto& vertex : quad->vertices)
    {
        left = std::min(left, vertex.x);
        right = std::max(right, vertex.x);
        top = std::min(top, vertex.y);
        bottom = std::max(bottom, vertex.y);
    }
    bool rectangle = std::all_of(std::begin(quad->vertices), std::end(quad->vertices), [&](const auto& vertex)
    {
        return (vertex.x == left || vertex.x == right) && (vertex.y == top || vertex.y == bottom);
    });
    // PanelQuad can inset the top-left corner by one logical pixel. Treat
    // those screen-sized overlays like the equivalent 0,0..640,480 rectangle.
    bool fullscreen = rectangle && left <= 1.0f && right >= 639.0f && top <= 1.0f && bottom >= 479.0f;
    bool solid = !quad->texture || quad->texture == *pWhiteTexture;
    // Additive/subtractive quads are screen effects, not opaque artwork. In
    // particular, a full-screen flash must not acquire black pillarboxes.
    bool screenEffect = quad->blendMode == 3 || quad->blendMode == 4 ||
        quad->blendMode == 6 || quad->blendMode == 7 || quad->blendMode == 8;

    if (fullscreen && (solid || screenEffect))
    {
        // Fades, flashes and solid loading screens cover the whole back buffer.
        for (auto& vertex : adjusted.vertices)
        {
            vertex.x = Screen.UnscaleX(vertex.x == left ? std::min(left, 0.0f) : std::max(right, 640.0f));
            vertex.y = Screen.UnscaleY(vertex.y == top ? std::min(top, 0.0f) : std::max(bottom, 480.0f));
        }
    }
    else if (fullscreen && quad->texture)
    {
        // Keep artwork at its original aspect and cover the unused screen area.
        if (Screen.fHudScaleX < 1.0f)
        {
            AddPillarbox(Screen.UnscaleX(0.0f), 0.0f, 0.0f, 480.0f, quad->z);
            AddPillarbox(640.0f, 0.0f, Screen.UnscaleX(640.0f), 480.0f, quad->z);
        }
        if (Screen.fHudScaleY < 1.0f)
        {
            AddPillarbox(0.0f, Screen.UnscaleY(0.0f), 640.0f, 0.0f, quad->z);
            AddPillarbox(0.0f, 480.0f, 640.0f, Screen.UnscaleY(480.0f), quad->z);
        }
    }
    else
    {
        for (auto& vertex : adjusted.vertices)
            vertex.x += fWidgetOffset;
    }
    shListAddQuad.ccall<void>(&adjusted);
}

void __cdecl ListAddCursorQuad(const NGLQuad* quad)
{
    // NGL sorts translucent quads by depth, irrespective of submission order.
    // Keep the cursor in front of the menu masks without changing its source quad.
    auto adjusted = *quad;
    adjusted.z = 0.0f;
    ListAddQuad(&adjusted);
}

SafetyHookInline shListAddString;
void __cdecl ListAddString(void* font, const char* text, float x, float y, float z, uint32_t color, float scaleX, float scaleY)
{
    if (IsBackBufferScene())
        x += fWidgetOffset;
    shListAddString.ccall<void>(font, text, x, y, z, color, scaleX, scaleY);
}

class Frontend
{
public:
    Frontend()
    {
        WFP::onInitEvent() += []
        {
            CIniReader iniReader("");
            // Developer-only diagnostic; omitted from the shipped INI and off by default.
            bTraceScreenEffects = iniReader.ReadInteger("DEBUG", "TraceScreenEffects", 0) != 0;
            if (bTraceScreenEffects)
            {
                auto colorPattern = hook::pattern("83 EC 10 56 8B F1 8A 46 1D 84 C0 57 8B 7C 24 1C"); //0x73D9F0
                shDrawComicColor = safetyhook::create_inline(colorPattern.get_first(), DrawComicColor);
                auto rectPattern = hook::pattern("64 A1 00 00 00 00 6A FF 68 ? ? ? ? 50 64 89 25 00 00 00 00 83 EC 38 55 8B 6C 24 50 D9 45 0C"); //0x73ADB0
                static auto traceColorRect = safetyhook::create_mid(rectPattern.get_first(), [](SafetyHookContext& regs)
                {
                    auto args = reinterpret_cast<uintptr_t*>(regs.esp);
                    auto rect = reinterpret_cast<float*>(args[1]);
                    auto color = reinterpret_cast<float*>(args[2]);
                    auto matrix = reinterpret_cast<float*>(args[3]);
                    if (color[3] <= 0.0f || !BeginScreenEffectTrace({ 3, args[0],
                        std::bit_cast<uint32_t>(color[0]), std::bit_cast<uint32_t>(color[1]),
                        std::bit_cast<uint32_t>(color[2]), IsBackBufferScene() }))
                        return;
                    std::ostringstream key, entry;
                    key << "ColorRect " << args[0] << ' ' << IsBackBufferScene() << ' '
                        << color[0] << ',' << color[1] << ',' << color[2];
                    entry << key.str() << " alpha=" << color[3] << " rect=";
                    for (int i = 0; i < 4; ++i)
                        entry << rect[i] << ',';
                    entry << " matrix=";
                    for (int i = 0; i < 16; ++i)
                        entry << matrix[i] << ',';
                    TraceScreenEffect(key.str(), entry.str());
                });
            }
            auto meshPattern = hook::pattern("83 EC 64 53 8B 5C 24 6C 85 DB 55 56 57"); //0x770360
            shListAddMesh = safetyhook::create_inline(meshPattern.get_first(), ListAddMesh);
            auto pattern = hook::pattern("83 EC ? 56 8B F1 8B 46 ? 85 C0 0F 84 ? ? ? ? 8B 46 ? 85 C0 0F 84 ? ? ? ? 8B 46 ? ? ? 6A"); //0x583D20
            pUpdateSlider = reinterpret_cast<decltype(pUpdateSlider)>(pattern.get_first());

            pattern = hook::pattern("C7 06 ? ? ? ? 8B 46 ? 85 C0 C7 44 24 ? ? ? ? ? 74 ? 8B 08 50 FF 51 ? 8B 46 ? 85 C0 C7 44 24"); //0x583FFD + 2
            auto sliderVtable = *pattern.get_first<uintptr_t*>(2); // 0x88E6F8
            // SetRect is slot 9 (0x88E6F8 + 0x24 = 0x88E71C).
            injector::WriteMemory(&sliderVtable[9], SetMenuSliderRect, true);

            pattern = hook::pattern("DB 44 24 08 56 8B F1 57 8B 7E 1C D8 4F 30 E8 ? ? ? ? DB 44 24 0C 89 46 08"); //0x582E40
            shSetMenuWidgetRect = safetyhook::create_inline(pattern.get_first(), SetMenuWidgetRect);

            pattern = hook::pattern("81 EC 14 01 00 00 56 57 8B BC 24 20 01 00 00 85 FF 8B F1"); //0x5B24F0
            shDrawMenuText = safetyhook::create_inline(pattern.get_first(), DrawMenuText);

            pattern = hook::pattern("DB 44 24 08 56 8B F1 57 8B 7E 1C D8 4F 30 E8 ? ? ? ? DB 44 24 0C 89 86 8C 00 00 00"); //0x584FA0
            static auto MenuHighlightRectHook = safetyhook::create_inline(pattern.get_first(), SetMenuHighlightRect);

            // The static artwork and scrolling-band constructors share this sequence.
            pattern = hook::pattern("C7 03 ? ? ? ? 8B 51 08 50 52 89 74 24 50 E8"); //0x595C4D + 2, 0x595ECD + 2
            pattern.count(2);
            for (size_t i = 0; i < 2; ++i)
            {
                auto textureVtable = *pattern.get(i).get<uintptr_t*>(2);
                if (i == 0)
                {
                    pDrawMenuTexture = reinterpret_cast<decltype(pDrawMenuTexture)>(textureVtable[7]);
                    injector::WriteMemory(&textureVtable[7], DrawMenuTexture, true);
                }
            }

            pattern = hook::pattern("56 E8 ? ? ? ? 56 E8 ? ? ? ? 83 C4 18 5E 83 C4 0C C3"); //0x594E4C + 7
            injector::MakeCALL(pattern.get_first(7), ListAddCursorQuad, true);

            // RenderLoadMeter draws the active screen directly, bypassing the
            // menu-system vtable. Submit the mask in its scene before the cursor.
            pattern = hook::pattern("8B 0D ? ? ? ? E8 ? ? ? ? 84 DB 5B 75 ? 6A 01"); //0x6191E8
            static auto MainMenuPillarboxHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext&)
            {
                DrawMenuPillarboxes();
            });

            // The pause menu has its own scene; mask it before that scene ends.
            pattern = hook::pattern("8B 0D ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? 5E 83 C4 40 C3"); //0x60C0FC
            static auto PauseMenuPillarboxHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext&)
            {
                DrawMenuPillarboxes();
            });

            pattern = hook::pattern("E8 ? ? ? ? 89 44 24 ? A1 ? ? ? ? 83 C4"); //0x76B438
            pBuildUIMatrix = reinterpret_cast<decltype(pBuildUIMatrix)>(injector::MakeCALL(pattern.get_first(), BuildUIMatrix, true).get<void>());

            pattern = hook::pattern("E8 ? ? ? ? 8B 46 ? 8B 48 ? 83 C4 ? 85 C9"); //0x640EDB
            pSetWorldToView = reinterpret_cast<decltype(pSetWorldToView)>(injector::MakeCALL(pattern.get_first(), SetHUDWorldToView, true).get<void>());

            pattern = hook::pattern("E8 ? ? ? ? 83 C4 ? 8D 54 24 ? 52 8D 4C 24"); //0x641AA1
            pSetViewport = reinterpret_cast<decltype(pSetViewport)>(injector::MakeCALL(pattern.get_first(), SetRadarViewport, true).get<void>());

            pattern = hook::pattern("E8 ? ? ? ? ? ? ? ? ? ? ? ? 8B 8C 24 ? ? ? ? 8B 94 24 ? ? ? ? 83 C4"); //0x57C404
            pProjectPoint = reinterpret_cast<decltype(pProjectPoint)>(injector::MakeCALL(pattern.get_first(), ProjectHUDPoint, true).get<void>());

            pattern = hook::pattern("E8 ? ? ? ? 8B 4C 24 ? 8B 54 24 ? ? ? 89 4C 24 ? 8B CA 83 C4"); //0x628920
            injector::MakeCALL(pattern.get_first(), ProjectHUDPoint, true);

            pattern = hook::pattern("E8 ? ? ? ? 8B 44 24 ? 8B 4C 24 ? ? ? 83 C4"); //0x6295B6
            injector::MakeCALL(pattern.get_first(), ProjectHUDPoint, true);

            pattern = hook::pattern("E8 ? ? ? ? 8B 44 24 ? 8B 4C 24 ? 89 44 24 ? 89 4C 24"); //0x62A1DF
            injector::MakeCALL(pattern.get_first(), ProjectHUDPoint, true);

            pattern = hook::pattern("E8 ? ? ? ? ? ? ? ? ? ? ? ? 8B 54 24 ? 8B 44 24 ? 8B 4C 24"); //0x62A537
            injector::MakeCALL(pattern.get_first(), ProjectHUDPoint, true);

            pattern = hook::pattern("E8 ? ? ? ? 8A 44 24 ? 8B 6C 24"); //0x632536
            pProjectHUDMatrixPoint = reinterpret_cast<decltype(pProjectHUDMatrixPoint)>(injector::MakeCALL(pattern.get_first(), ProjectHUDMatrixPoint, true).get<void>());

            pattern = hook::pattern("E8 ? ? ? ? 8B 8D ? ? ? ? 8B 44 24"); //0x6326B2
            injector::MakeCALL(pattern.get_first(), ProjectHUDMatrixPoint, true);

            // World-space enemy indicators and targeting reticles bypass NGL's
            // projection helper. Keep their anchors on the projected enemies;
            // the regular HUD matrix still preserves each icon's proportions.
            pattern = hook::pattern("E8 ? ? ? ? 8B 44 24 14 8B 54 24 10 8B 4C 24 24 89 54 24 34"); //0x60E545
            injector::MakeCALL(pattern.get_first(), ProjectHUDMatrixPoint, true);

            pattern = hook::pattern("E8 ? ? ? ? D9 44 24 18 D8 1D ? ? ? ? 83 C4 0C DF E0 F6 C4 05"); //0x6124C8
            injector::MakeCALL(pattern.get_first(), ProjectHUDMatrixPoint, true);

            pattern = hook::pattern("E8 ? ? ? ? 8B 54 24 14 8B 4C 24 10 83 C4 0C 89 4C 24 20"); //0x612B06
            injector::MakeCALL(pattern.get_first(), ProjectHUDMatrixPoint, true);

            pattern = hook::pattern("E8 ? ? ? ? D9 85 34 01 00 00 D8 0D ? ? ? ? 8B 4C 24 20"); //0x619E31
            injector::MakeCALL(pattern.get_first(), ProjectHUDMatrixPoint, true);

            pattern = hook::pattern("E8 ? ? ? ? 8B 48 04 8B 10 8B 6C 24 54 89 4C 24 30"); //0x61A051
            injector::MakeCALL(pattern.get_first(), ProjectHUDMatrixPoint, true);

            pattern = hook::pattern("E8 ? ? ? ? 8B 4E ? 85 C9 74 ? E8 ? ? ? ? 8B 4E ? 85 C9 74 ? E8 ? ? ? ? 8B 46 ? 85 C0 74 ? 8B C8"); //0x61A3A0
            shDrawHealth = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), DrawHealth);

            pattern = hook::pattern("81 EC ? ? ? ? 53 56 8B F1 8B 86"); //0x641990
            shDrawRadar = safetyhook::create_inline(pattern.get_first(), DrawRadar);

            pattern = hook::pattern("56 8B F1 83 3E 00 74 ? 8A 86 C0 00 00 00 84 C0 75 ? 8B 46 60"); //0x60CF40
            shDrawRace = safetyhook::create_inline(pattern.get_first(), DrawRace);

            pattern = hook::pattern("56 8B F1 8B 46 04 85 C0 74 ? 8A 46 2A 84 C0 75 ? 8B 46 20"); //0x615B90
            shDrawTimer = safetyhook::create_inline(pattern.get_first(), DrawTimer);

            pattern = hook::pattern("8B 4E 4C 85 C9 5F 74 ? E8 ? ? ? ? 8B 46 50"); //0x635998 + 8
            shDrawPursuit = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first(8)).as_int(), DrawPursuit);

            // Restrict panel anchoring to screen-space HUD call sites. Enemy
            // health, targeting reticles, world markers and announcements do not use it.
            pattern = hook::pattern("8B 40 14 85 C0 74 07 8B C8 E8 ? ? ? ? 8B 4E 40 85 C9"); //0x6359B3 + 9 (score)
            hbDrawEdgePanel.fun = injector::MakeCALL(pattern.get_first(9), DrawEdgePanel, true).get();
            pattern = hook::pattern("8A 42 2D 84 C0 74 05 E8 ? ? ? ? 8B 4E 1C 85 C9"); //0x635A30 + 7 (chase distance)
            injector::MakeCALL(pattern.get_first(7), DrawEdgePanel, true);
            pattern = hook::pattern("8B 46 30 85 C0 74 0D 8B 00 85 C0 74 07 8B C8 E8 ? ? ? ? 8B 4E 34"); //0x635937 + 15 (tutorial controller gauge)
            injector::MakeCALL(pattern.get_first(15), DrawEdgePanel, true);
            pattern = hook::pattern("8B 46 48 85 C0 74 0D 8B 00 85 C0 74 07 8B C8 E8 ? ? ? ? E8"); //0x635A48 + 15 (combo words)
            injector::MakeCALL(pattern.get_first(15), DrawEdgePanel, true);

            pattern = hook::pattern("57 8B F9 8B 47 04 85 C0 74 ? 8A 47 4C 84 C0 74 ? D9 47 48"); //0x60E260
            shDrawChallenge = safetyhook::create_inline(pattern.get_first(), DrawChallenge);

            pattern = hook::pattern("8B 0D ? ? ? ? 8B 56 5C 89 0C 82 8B 4E 58 40"); //0x5102E3 + 2
            pWhiteTexture = *pattern.get_first<void**>(2);

            pattern = hook::pattern("C7 07 ? ? ? ? C7 44 24 18 00 00 00 00 8D 77 18"); //0x73166F + 2
            pComicTextureVtable = *pattern.get_first<uintptr_t>(2);

            pattern = hook::pattern("56 8B 71 60 85 F6 74 18 57 8B 7C 24 0C 8D 49 00"); //0x743440
            shDrawComicPanel = safetyhook::create_inline(pattern.get_first(), DrawComicPanel);

            pattern = hook::pattern("E8 ? ? ? ? 83 C4 ? E8 ? ? ? ? 5F 5D"); //0x77AFE0
            shListAddQuad = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), ListAddQuad);

            pattern = hook::pattern("E8 ? ? ? ? 83 C4 ? 8D 4C 24 ? C7 44 24 ? ? ? ? ? E8 ? ? ? ? 8B 4E"); //0x779C40
            shListAddString = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), ListAddString);

            // Motion blur samples the whole rendered world.
            pattern = hook::pattern("E8 ? ? ? ? 83 C4 ? E8 ? ? ? ? 5F 5D"); //0x52180B
            injector::MakeCALL(pattern.get_first(), ListAddScreenQuad, true);

            pattern = hook::pattern("D9 9C 24 ? ? ? ? E8 ? ? ? ? 83 C4 10"); //0x615F99
            static auto PanelMeshHook = safetyhook::create_mid(pattern.get_first(7), [](SafetyHookContext& regs)
            {
                if (IsBackBufferScene())
                {
                    auto matrix = *reinterpret_cast<float**>(regs.esp + 4);
                    matrix[12] += fWidgetOffset;
                }
            });
        };
    }
} Frontend;
