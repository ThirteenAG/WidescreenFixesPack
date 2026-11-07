#pragma once

namespace Hud
{
inline float scale{ 1.0f }, constraint{};
// The projection fits the native canvas into the screen: wider screens show
// it in a centred 16:9 area, narrower ones in a full-width band. wide and tall
// give the screen size in native canvas units relative to the canvas.
inline float wide{ 1.0f }, tall{ 1.0f }, edgeRatio{ 1.0f }, pixels{ 1.0f };
inline bool scopeActive{};
inline uintptr_t manager;
inline injector::hook_back<int(uintptr_t, float)> update;
using ProjectFunction = int(uintptr_t, const float*, float*);
inline injector::game_fn<ProjectFunction> project[3];
inline constexpr const char* markerSignature = "E0 FF BD 27 10 00 B0 AF 14 00 BF AF ? 00 80 10 25 80 A0 00 ? 00 00 12 ? ? 05 3C ? ? A5 24 ? ? A6 8C 25 28 80 00 25 20 C0 00";

struct Profile { unsigned offset; float x, y; };
// Anchors in canvas pixels from the canvas centre. Each module owns a
// scene-graph root for its artwork.
inline constexpr Profile profiles[] {
    { 932, -240.0f, -136.0f }, // weapon
    { 576,  240.0f, -136.0f }, // team
    { 1004, 240.0f,  136.0f }, // compass
    { 1552,   0.0f,    0.0f }, // reticule
    { 4704,   0.0f,    0.0f }, // directional threats
    { 400, -240.0f, -136.0f }, // weapon selection
    { 1524, 240.0f,   0.0f }, // update notification
    { 4912, 240.0f,   0.0f }, // PSP indicators and free look
    { 1776,   0.0f,  136.0f }, // contextual actions
    { 1484, -240.0f,  136.0f }, // mission timer
    { 1500, -240.0f,  136.0f }, // action timer
};
// The HUD root is a plane in front of the camera; its units are not canvas
// pixels. Convert with the plane's depth and the camera's half angle.
inline float PixelsPerUnit()
{
    const auto root = *reinterpret_cast<const uintptr_t*>(manager);
    const auto camera = *Geometry::sceneCamera;
    if (!root || !camera) return 1.0f;
    const auto plane = reinterpret_cast<const float*>(root + 16);
    const auto view = reinterpret_cast<const float*>(camera + 16);
    const float depth = std::fabs((plane[12] - view[12]) * view[8] + (plane[13] - view[13]) * view[9] +
                                  (plane[14] - view[14]) * view[10]);
    const float tangent = std::tan(*reinterpret_cast<const float*>(camera + 416));
    return depth > 1.0f && tangent > 0.0f ? 240.0f / (depth * tangent) : 1.0f;
}
inline void Layout()
{
    constexpr float native = 480.0f / 272.0f;
    const float aspect = Geometry::AspectRatio();
    wide = std::max(aspect / native, 1.0f);
    tall = std::max(native / aspect, 1.0f);
    edgeRatio = constraint > 0.0f && aspect > constraint ? constraint / aspect : 1.0f;
    pixels = PixelsPerUnit();
    const float units = 1.0f / pixels;
    for (const auto& profile : profiles)
    {
        const auto node = *reinterpret_cast<const uintptr_t*>(manager + profile.offset);
        if (!node) continue; // Modules are absent until the native HUD is loaded.
        auto matrix = reinterpret_cast<float*>(node + 16);
        matrix[0] = scale;
        matrix[5] = scale;
        matrix[12] = profile.x * units * (wide * edgeRatio - scale);
        matrix[13] = profile.y * units * (tall - scale);
    }
    // The team-action marker moves with its target; scale its contents in
    // place without changing the position supplied by the game.
    const auto actionMarker = *reinterpret_cast<const uintptr_t*>(manager + 4576);
    if (actionMarker)
    {
        auto matrix = reinterpret_cast<float*>(actionMarker + 16);
        matrix[0] = scale;
        matrix[5] = scale;
    }
    // The wheel's top and bottom labels need room even at maximum HUD scale.
    const auto wheel = *reinterpret_cast<const uintptr_t*>(manager + 3032);
    if (wheel)
    {
        const float wheelScale = std::min(scale, 1.2f);
        auto matrix = reinterpret_cast<float*>(wheel + 16);
        matrix[0] = wheelScale;
        matrix[5] = wheelScale;
        matrix[12] = matrix[13] = 0.0f;
    }
    scopeActive = Geometry::ScopeOverlay();
}
inline int Update(uintptr_t object, float delta)
{
    const int result = update.fun(object, delta);
    Layout();
    return result;
}
// World positions for markers and name tags. The game projects them with the
// widened camera, but draws them on the fitted canvas; give them native
// canvas coordinates so the fit puts them back on their targets.
template<unsigned Index> int Project(uintptr_t camera, const float* point, float* result)
{
    const int visible = project[Index](camera, point, result);
    if (visible)
    {
        result[0] *= wide;
        result[1] *= tall;
    }
    return visible;
}
inline bool Install()
{
    uintptr_t calls[3];
    for (size_t i = 0; i < 3; ++i)
    {
        calls[i] = pattern.get(i, markerSignature, 44);
        if (!calls[i]) return false;
    }
    if (pattern.get(3, markerSignature, 44)) return false;
    project[0] = reinterpret_cast<ProjectFunction*>(injector::MakeCALL(calls[0], Project<0>).as_int());
    project[1] = reinterpret_cast<ProjectFunction*>(injector::MakeCALL(calls[1], Project<1>).as_int());
    project[2] = reinterpret_cast<ProjectFunction*>(injector::MakeCALL(calls[2], Project<2>).as_int());
    const auto site = sites::Required("? ? 04 3C 06 A3 00 46 ? ? ? ? ? ? 84 24 ? ? 11 3C ? ? 24 8E 0A 00 80 10");
    manager = Absolute(site, 0, 12);
    update.fun = injector::MakeCALL(site + 8, Update).get();
    return true;
}
}
