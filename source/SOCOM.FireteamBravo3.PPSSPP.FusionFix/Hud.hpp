#pragma once

namespace Hud
{
enum class Role
{
    Frontend, World, LeftTop, LeftBottom, RightTop, RightBottom,
    Center, CenterBottom, Contained, Fullscreen, Scope, Message, Map, Mixed, Stats, Menu, Tactical
};
struct Profile
{
    const char* pattern;
    Role role;
    uintptr_t address;
};
#include "HudSites.hpp"

struct Affine
{
    float x{ 1.0f }, y{ 1.0f }, tx{}, ty{};
};
inline float scale{ 1.0f }, constraint{};
inline Role role{ Role::Frontend };
inline Affine current;
inline bool rawReady;
alignas(16) inline float raw[16];
inline const float* cached;
inline SafetyMipsInline projectionHook, projectHook, fillHook, textHook, promptHook, scissorHook;
inline SafetyMipsMid dispatchHook, statsHook;
inline SafetyMipsInline menuHook, screenHook, loadingHook, loadingMPHook;
inline injector::hook_back<int(const float*)> submit;
inline injector::hook_back<void(uintptr_t, float*, uintptr_t, uintptr_t)> project;
inline injector::hook_back<int(int, int, int, int, uint32_t)> fill;
inline injector::hook_back<int(uintptr_t, int, int, const char*, uint32_t, int, int)> text;
inline injector::hook_back<int(uintptr_t, int, int, int, int, const char*, uint32_t, uint32_t, uintptr_t, int)> prompt;
inline injector::hook_back<void(int, int, int, int)> scissor;
inline injector::hook_back<uintptr_t(uintptr_t, float)> menu;
inline injector::hook_back<int(uintptr_t, uint8_t)> screen;
inline injector::hook_back<int()> loading, loadingMP;
inline bool textScoped;
inline bool cinematicFrame;
inline float renderDelta{ 1.0f / 30.0f };
inline void (*menuExtra)(uintptr_t);

inline float AspectRatio()
{
    return forcedAspect > 0.0f ? forcedAspect : Aspect();
}
inline Affine Fit()
{
    constexpr float native = 480.0f / 272.0f;
    const float aspect = AspectRatio();
    return { std::min(native / aspect, 1.0f), std::min(aspect / native, 1.0f), 0, 0 };
}
inline Affine Anchored(float x, float y, bool world = false)
{
    const float aspect = AspectRatio();
    const float sx = 480.0f / 272.0f / aspect * scale;
    const float sy = scale;
    const float edge = world ? x : console::hud_edge(x, 480, aspect, constraint);
    const float bx = edge - x * sx;
    const float by = y * (1.0f - sy);
    return { sx, sy, sx + 2.0f * bx / 480.0f - 1.0f, 1.0f - sy - 2.0f * by / 272.0f };
}
inline bool Canvas(const float* matrix)
{
    return std::fabs(matrix[0] - 2.0f / 480.0f) < 0.000001f &&
        std::fabs(matrix[5] + 2.0f / 272.0f) < 0.000001f &&
        matrix[3] == 0 && matrix[7] == 0 && matrix[11] == 0 &&
        matrix[12] == -1 && matrix[13] == 1 && matrix[15] == 1;
}
inline int Apply(const float* matrix)
{
    if (!Canvas(matrix) && role != Role::Map && role != Role::Tactical)
        return submit.fun(matrix);
    alignas(16) float adjusted[16];
    for (unsigned i = 0; i < 16; i += 4)
    {
        adjusted[i] = matrix[i] * current.x + matrix[i + 3] * current.tx;
        adjusted[i + 1] = matrix[i + 1] * current.y + matrix[i + 3] * current.ty;
        adjusted[i + 2] = matrix[i + 2];
        adjusted[i + 3] = matrix[i + 3];
    }
    return submit.fun(adjusted);
}
inline void Reapply()
{
    if (!rawReady)
    {
        std::memcpy(raw, cached, sizeof(raw));
        rawReady = true;
    }
    Apply(raw);
}
inline void Scissor(int x, int y, int width, int height);
inline void ScopeSides();
struct MenuScope
{
    Role previousRole{ role };
    Affine previous{ current };
    MenuScope()
    {
        role = Role::Menu;
        current = Fit();
        Reapply();
        Scissor(0, 0, 480, 272);
    }
    ~MenuScope()
    {
        role = previousRole;
        current = previous;
        Reapply();
        Scissor(0, 0, 480, 272);
    }
};
inline uintptr_t Menu(uintptr_t object, float delta)
{
    MenuScope scope;
    const float previousDelta = renderDelta;
    renderDelta = delta;
    const auto result = menu.fun(object, delta);
    ScopeSides();
    if (menuExtra)
        menuExtra(object);
    renderDelta = previousDelta;
    static bool reported = false;
    if (!reported)
    {
        reported = true;
        logger.Write("Native menu canvas fitted.\n");
    }
    return result;
}
inline int Screen(uintptr_t object, uint8_t background)
{
    MenuScope scope;
    const int result = screen.fun(object, background);
    ScopeSides();
    return result;
}
inline int Loading()
{
    MenuScope scope;
    return loading.fun();
}
inline int LoadingMP()
{
    MenuScope scope;
    return loadingMP.fun();
}
inline int Projection(const float* matrix)
{
    std::memcpy(raw, matrix, sizeof(raw));
    rawReady = true;
    if (role == Role::Frontend)
        current = {};
    return Apply(raw);
}
inline int Fill(int x, int y, int w, int h, uint32_t color)
{
    const bool wide = x <= 0 && x + w >= 480;
    const bool whole = wide && y <= 0 && y + h >= 272;
    if (wide && rawReady && Canvas(raw))
    {
        const auto previous = current;
        current.x = 1.0f;
        current.tx = 0.0f;
        if (whole)
        {
            current.y = 1.0f;
            current.ty = 0.0f;
        }
        Reapply();
        const int result = fill.fun(x, y, w, h, color);
        current = previous;
        Reapply();
        return result;
    }
    return fill.fun(x, y, w, h, color);
}
inline void ScopeSides()
{
    if (!rawReady || !Canvas(raw))
        return;
    const auto fit = Fit();
    const int x = int(std::ceil(240 * (1.0f - fit.x)));
    const int y = int(std::ceil(136 * (1.0f - fit.y)));
    submit.fun(raw);
    scissor.fun(0, 0, 480, 272);
    if (x > 0)
    {
        fill.fun(0, 0, x, 272, 0xFF000000);
        fill.fun(480 - x, 0, x, 272, 0xFF000000);
    }
    if (y > 0)
    {
        fill.fun(0, 0, 480, y, 0xFF000000);
        fill.fun(0, 272 - y, 480, y, 0xFF000000);
    }
    Reapply();
    Scissor(0, 0, 480, 272);
}
inline void Project(uintptr_t context, float* point, uintptr_t view, uintptr_t projection)
{
    project.fun(context, point, view, projection);
    if (role != Role::World || !std::isfinite(point[0]) || !std::isfinite(point[1]))
        return;
    // Leave native visibility tests in the original screen coordinates.
    // Only the subsequent linked artwork pivots around the projected point.
    const bool visible = point[0] > 0 && point[0] < 480 && point[1] > 0 && point[1] < 272;
    current = Anchored(std::clamp(point[0], 0.0f, 480.0f), std::clamp(point[1], 0.0f, 272.0f), visible);
    Reapply();
}
inline int Text(uintptr_t context, int x, int y, const char* value, uint32_t color, int wrap, int alignment)
{
    const auto previous = current;
    const bool separate = !textScoped && (role == Role::Mixed ||
        (role == Role::Contained && alignment == 2 && x >= 440) ||
        (role == Role::Center && alignment == 2 && x >= 400) ||
        (role == Role::RightBottom && alignment == 2 && y <= 32));
    if (separate)
    {
        const float anchor = (alignment & 15) == 1 ? 240.0f : (alignment & 15) == 2 ? 480.0f : 0.0f;
        current = Anchored(anchor, y < 136 ? 0.0f : 272.0f);
        Reapply();
    }
    const int result = text.fun(context, x, y, value, color, wrap, alignment);
    if (separate)
    {
        current = previous;
        Reapply();
    }
    return result;
}
inline void Scissor(int x, int y, int width, int height)
{
    if (role == Role::Frontend || width <= 0 || height <= 0 ||
        (role != Role::Menu && role != Role::Tactical && x <= 0 && y <= 0 && x + width >= 480 && y + height >= 272))
    {
        scissor.fun(x, y, width, height);
        return;
    }
    const float bx = (1.0f - current.x + current.tx) * 240.0f;
    const float by = (1.0f - current.y - current.ty) * 136.0f;
    const int left = std::clamp(int(std::floor(current.x * x + bx)), 0, 480);
    const int top = std::clamp(int(std::floor(current.y * y + by)), 0, 272);
    const int right = std::clamp(int(std::ceil(current.x * (x + width) + bx)), 0, 480);
    const int bottom = std::clamp(int(std::ceil(current.y * (y + height) + by)), 0, 272);
    if (right <= left || bottom <= top)
        scissor.fun(1, 1, 0, 0);
    else
        scissor.fun(left, top, right - left, bottom - top);
}
inline int Prompt(uintptr_t context, int x, int y, int paddingX, int paddingY,
    const char* value, uint32_t color, uint32_t background, uintptr_t texture, int alignment)
{
    const auto previous = current;
    const bool scoped = textScoped;
    const bool separate = role == Role::Mixed;
    if (separate)
    {
        const float anchor = (alignment & 15) == 1 ? 240.0f :
            (alignment & 15) == 2 || x >= 440 ? 480.0f : x <= 10 ? 0.0f : 240.0f;
        current = Anchored(anchor, y < 136 ? 0.0f : 272.0f);
        textScoped = true;
        Reapply();
    }
    const int result = prompt.fun(context, x, y, paddingX, paddingY, value, color, background, texture, alignment);
    textScoped = scoped;
    if (separate)
    {
        current = previous;
        Reapply();
    }
    return result;
}
inline Affine Select(Role selected, uintptr_t object)
{
    if (cinematicFrame && selected == Role::CenterBottom)
        return Fit();
    switch (selected)
    {
        case Role::LeftTop: return Anchored(0, 0);
        case Role::LeftBottom: return Anchored(0, 272);
        case Role::Stats:
        case Role::RightTop: return Anchored(480, 0);
        case Role::RightBottom: return Anchored(480, 272);
        case Role::CenterBottom: return Anchored(240, 272);
        case Role::Fullscreen: return {};
        case Role::Frontend: return {};
        case Role::Contained:
        case Role::Scope:
        case Role::Mixed: return Fit();
        case Role::Menu: return Fit();
        case Role::Tactical: return Fit();
        case Role::Message:
        {
            const float x = *reinterpret_cast<const float*>(object + 40);
            const float y = *reinterpret_cast<const float*>(object + 44);
            const float w = *reinterpret_cast<const float*>(object + 52);
            if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(w))
                return Anchored(240, 136);
            return Anchored(x >= 240 ? 480 : x + w <= 240 ? 0 : 240, y < 136 ? 0 : 272);
        }
        case Role::Map:
        {
            const auto map = *reinterpret_cast<const uintptr_t*>(object + 16);
            if (!map)
                return Fit();
            const float x = *reinterpret_cast<const float*>(map + 384);
            const float w = *reinterpret_cast<const float*>(map + 392);
            return w > 0 && w <= 160 && x >= 240 ? Anchored(480, 0) : Fit();
        }
        default: return Anchored(240, 136);
    }
}
inline void StatsBottom()
{
    if (role == Role::Stats)
    {
        current = Anchored(480, 272);
        Reapply();
    }
}
inline int Draw(uintptr_t object, uintptr_t context, uintptr_t function)
{
    const auto previousRole = role;
    const auto previous = current;
    role = Role::Center;
    for (const auto& profile : profiles)
    {
        if (profile.address == function)
        {
            role = profile.role;
            break;
        }
    }
    const auto model = *reinterpret_cast<const uintptr_t*>(object + 8);
    if (model && role != Role::Fullscreen)
    {
        const auto map = *reinterpret_cast<const uintptr_t*>(model + 56);
        const auto state = map ? *reinterpret_cast<const uintptr_t*>(map + 16) : 0;
        const bool open = state && *reinterpret_cast<const uint8_t*>(state + 144);
        if (open || *reinterpret_cast<const uint8_t*>(model + 440))
            role = Role::Tactical;
    }
    current = Select(role, object);
    Reapply();
    if (role == Role::Scope)
    {
        // The same observer also draws unmasked aiming reticles. Only the
        // binocular views paint a full-frame mask around their image.
        const auto view = *reinterpret_cast<const uintptr_t*>(object + 4);
        const int mode = *reinterpret_cast<const int*>(view + 40);
        if (mode == 2 || mode == 5)
            ScopeSides();
    }
    const int result = injector::cstd<int(uintptr_t, uintptr_t)>::call(function, object, context);
    role = previousRole;
    current = previousRole == Role::Frontend ? Affine{} : previous;
    Reapply();
    return result;
}
}
