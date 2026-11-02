#pragma once

// Movies are 480x272 PSMF streams drawn as one textured sprite over the whole
// 512x320 render target, which is then presented on the display. Keep the
// complete picture at its own aspect and fill the rest black.
namespace Movie
{
struct BarVertex { uint32_t color; int16_t x, y, z, padding; };
static_assert(sizeof(BarVertex) == 12);
struct List { uint32_t* commands; uint32_t unused[2]; uintptr_t vertices; };
inline injector::hook_back<int(List*, int, int, int, int, int, int, int, int, int)> draw;

inline int Draw(List* list, int x0, int y0, int u0, int v0, int x1, int y1, int u1, int v1, int z)
{
    const float sx = std::min(Display::frame / Display::aspect, 1.0f);
    const float sy = std::min(Display::aspect / Display::frame, 1.0f);
    const int width = int(std::lround((x1 - x0) * sx));
    const int height = int(std::lround((y1 - y0) * sy));
    if (width == x1 - x0 && height == y1 - y0)
        return draw.fun(list, x0, y0, u0, v0, x1, y1, u1, v1, z);
    const int left = x0 + (x1 - x0 - width) / 2;
    const int top = y0 + (y1 - y0 - height) / 2;

    // Two black clear-mode sprites for the uncovered sides.
    list->vertices = (list->vertices - 4 * sizeof(BarVertex)) & ~uintptr_t(3);
    auto bars = reinterpret_cast<BarVertex*>(list->vertices);
    const auto edge = [](int value) { return int16_t(value); };
    if (width < x1 - x0)
    {
        bars[0] = { 0xFF000000, edge(x0), edge(y0), 0, 0 };
        bars[1] = { 0xFF000000, edge(left), edge(y1), 0, 0 };
        bars[2] = { 0xFF000000, edge(left + width), edge(y0), 0, 0 };
        bars[3] = { 0xFF000000, edge(x1), edge(y1), 0, 0 };
    }
    else
    {
        bars[0] = { 0xFF000000, edge(x0), edge(y0), 0, 0 };
        bars[1] = { 0xFF000000, edge(x1), edge(top), 0, 0 };
        bars[2] = { 0xFF000000, edge(x0), edge(top + height), 0, 0 };
        bars[3] = { 0xFF000000, edge(x1), edge(y1), 0, 0 };
    }
    const auto address = list->vertices;
    auto command = list->commands;
    *command++ = 0xD3000101;
    *command++ = 0x1280011C;
    *command++ = 0x10000000 | ((address >> 8) & 0x000F0000);
    *command++ = 0x01000000 | (address & 0x00FFFFFF);
    *command++ = 0x04060004;
    *command++ = 0xD3000000;
    list->commands = command;
    return draw.fun(list, left, top, u0, v0, left + width, top + height, u1, v1, z);
}

inline void Install()
{
    draw.fun = injector::MakeCALL(Required("10 01 02 24 00 00 A2 AF ? ? D4 AE ? ? ? ? 04 00 A0 AF", 12), Draw).get();
}
}
