#pragma once

namespace NativeMenu { int Language(); }
namespace Graphics
{
struct VideoVertex { float u, v, x, y, z; };
struct ClearVertex { uint32_t color; int16_t x, y, z, padding; };
static_assert(sizeof(VideoVertex) == 20 && sizeof(ClearVertex) == 12);
alignas(64) inline uint32_t videoList[512];
alignas(64) inline PspGeContext videoContext;
alignas(16) inline VideoVertex videoVertices[2];
alignas(16) inline ClearVertex clearVertices[8];
alignas(16) inline uint32_t clearList[12];
inline bool guInitialized, scopeSeen, videoReported, videoFailed;
inline SafetyMipsInline endHook, leafHook, meshHook;
inline injector::hook_back<int(void*, const void*, unsigned)> copy;
inline injector::hook_back<int(uintptr_t)> end;
inline injector::hook_back<int(uintptr_t, uintptr_t, const float*, const uint32_t*)> leaf;
inline injector::hook_back<int(uintptr_t, const uint16_t*, uintptr_t, const float*, const uint32_t*)> mesh;
inline int (*callList)(uintptr_t);

inline int MovieFallback(void* destination, const void* source, unsigned length)
{
    if (!videoFailed)
    {
        videoFailed = true;
        static constexpr const char* messages[] = {
            "Fusion Fix: movie fitting unavailable; using native video.\n",
            "Fusion Fix : cadrage video indisponible ; video native utilisee.\n",
            "Fusion Fix: Videoanpassung nicht verfuegbar; natives Video.\n",
            "Fusion Fix: adattamento video non disponibile; video originale.\n",
            "Fusion Fix: ajuste de video no disponible; video original.\n"
        };
        PatchNotice(messages[NativeMenu::Language()]);
    }
    return copy.fun(destination, source, length);
}
inline int MovieCopy(void* destination, const void* source, unsigned length)
{
    if (videoFailed)
        return copy.fun(destination, source, length);
    const auto fit = Hud::TransformFor(Hud::Role::Frontend);
    if (fit.scaleX == 1.0f && fit.scaleY == 1.0f)
        return copy.fun(destination, source, length);
    if (length != 512 * 272 * 4)
        return MovieFallback(destination, source, length);
    if (!videoReported)
    {
        videoReported = true;
        logger.WriteF("Native movie fit %.6f x %.6f; complete 480 x 272 source.\n", double(fit.scaleX), double(fit.scaleY));
    }

    // The native player has finished the previous subtitle list before copying
    // this decoded frame. Preserve its complete GE state around the fitted blit.
    if (sceGeDrawSync(0) < 0 || sceGeSaveContext(&videoContext) < 0)
        return MovieFallback(destination, source, length);
    if (!guInitialized)
    {
        sceGuInit();
        guInitialized = true;
    }
    videoVertices[0] = { 0, 0, fit.offsetX / 20, fit.offsetY / 20, 0 };
    videoVertices[1] = { 480, 272, (fit.offsetX + 9600 * fit.scaleX) / 20, (fit.offsetY + 5440 * fit.scaleY) / 20, 0 };
    sceKernelDcacheWritebackRange(source, length);
    sceKernelDcacheWritebackRange(videoVertices, sizeof(videoVertices));
    sceGuStart(GU_DIRECT, videoList);
    sceGuDrawBuffer(GU_PSM_8888, reinterpret_cast<void*>(uintptr_t(destination) & 0x001FFFFF), 512);
    sceGuOffset(2048 - 240, 2048 - 136);
    sceGuViewport(2048, 2048, 480, 272);
    sceGuScissor(0, 0, 480, 272);
    sceGuEnable(GU_SCISSOR_TEST);
    sceGuDisable(GU_DEPTH_TEST);
    sceGuDisable(GU_ALPHA_TEST);
    sceGuDisable(GU_BLEND);
    sceGuDisable(GU_STENCIL_TEST);
    sceGuDisable(GU_CULL_FACE);
    sceGuDisable(GU_LIGHTING);
    sceGuDisable(GU_FOG);
    sceGuDisable(GU_COLOR_TEST);
    sceGuDisable(GU_COLOR_LOGIC_OP);
    sceGuPixelMask(0);
    sceGuClearColor(0xFF000000);
    sceGuClear(GU_COLOR_BUFFER_BIT);
    sceGuEnable(GU_TEXTURE_2D);
    sceGuTexMode(GU_PSM_8888, 0, 0, 0);
    sceGuTexImage(0, 512, 512, 512, source);
    sceGuTexFlush();
    sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGBA);
    sceGuTexFilter(GU_LINEAR, GU_LINEAR);
    sceGuTexWrap(GU_CLAMP, GU_CLAMP);
    sceGuTexScale(1, 1);
    sceGuTexOffset(0, 0);
    sceGuDrawArray(GU_SPRITES, GU_TEXTURE_32BITF | GU_VERTEX_32BITF | GU_TRANSFORM_2D, 2, nullptr, videoVertices);
    sceGuFinish();
    sceGuSync(0, 0);
    if (sceGeRestoreContext(&videoContext) < 0)
        return MovieFallback(destination, source, length);
    return 0;
}
// Canvas-space bounds (twips) of shape points under a fitted movie transform.
struct Bounds
{
    float left{ 1e9f }, top{ 1e9f }, right{ -1e9f }, bottom{ -1e9f };
    void Add(const float* matrix, const console::Transform& fit, float x, float y)
    {
        const float canvasX = (matrix[0] * x + matrix[4] * y + matrix[12] - fit.offsetX) / fit.scaleX;
        const float canvasY = (matrix[1] * x + matrix[5] * y + matrix[13] - fit.offsetY) / fit.scaleY;
        left = std::min(left, canvasX);
        right = std::max(right, canvasX);
        top = std::min(top, canvasY);
        bottom = std::max(bottom, canvasY);
    }
    bool Covers() const { return left <= 20 && top <= 20 && right >= 9580 && bottom >= 5420; }
    bool Outside() const { return right <= 0 || left >= 9600 || bottom <= 0 || top >= 5440; }
};
// Plain colour or a small soft texture sampled once: stretching it is invisible.
inline bool Plain(uintptr_t texture, unsigned uSpan, unsigned vSpan)
{
    if (!texture)
        return true;
    const auto size = reinterpret_cast<const uint16_t*>(texture + 20);
    return size[0] * size[1] <= 64 * 64 && uSpan <= 32768 && vSpan <= 32768;
}
inline void Unfit(float (&screen)[16], const float* matrix, const console::Transform& fit)
{
    for (unsigned i = 0; i < 16; i += 4)
    {
        screen[i] = (matrix[i] - matrix[i + 3] * fit.offsetX) / fit.scaleX;
        screen[i + 1] = (matrix[i + 1] - matrix[i + 3] * fit.offsetY) / fit.scaleY;
        screen[i + 2] = matrix[i + 2];
        screen[i + 3] = matrix[i + 3];
    }
}
inline bool Fitted(console::Transform& fit)
{
    if (Hud::role == Hud::Role::World)
        return false;
    fit = Hud::TransformFor(Hud::role);
    return fit.scaleX != 1.0f || fit.scaleY != 1.0f || fit.offsetX != 0.0f || fit.offsetY != 0.0f;
}
// Sprite shapes: { texture, then three layers of { count, vertices } } per part.
inline int Leaf(uintptr_t renderer, uintptr_t shape, const float* matrix, const uint32_t* color)
{
    struct Vertex { uint16_t u, v; float x, y, z; };
    if (Hud::role == Hud::Role::Scope)
        scopeSeen = true;
    console::Transform fit;
    if (!shape || !Fitted(fit))
        return leaf.fun(renderer, shape, matrix, color);
    const auto words = reinterpret_cast<const uint32_t*>(shape);
    Bounds bounds;
    bool single = words[28] == 1, plain = true;
    for (unsigned part = 0; part < words[28]; ++part)
        for (unsigned layer = 0; layer < 3; ++layer)
        {
            const auto count = words[part * 7 + 1 + layer * 2];
            const auto vertices = reinterpret_cast<const Vertex*>(uintptr_t(words[part * 7 + 2 + layer * 2]));
            single &= count == 0 || count == 2;
            for (unsigned i = 0; i < count; ++i)
                bounds.Add(matrix, fit, vertices[i].x, vertices[i].y);
            if (count == 2)
                plain &= Plain(words[part * 7], std::abs(vertices[1].u - vertices[0].u), std::abs(vertices[1].v - vertices[0].v));
        }
    // Off the canvas is off the screen in the original framing.
    if (bounds.Outside())
        return 0;
    // A dim or fade over the whole canvas covers the whole screen.
    if (Hud::role != Hud::Role::Scope && single && plain && bounds.Covers())
    {
        alignas(16) float screen[16];
        Unfit(screen, matrix, fit);
        return leaf.fun(renderer, shape, screen, color);
    }
    return leaf.fun(renderer, shape, matrix, color);
}
// Mesh shapes: a definition with parts of { primitive, vertex type, vertices, count }.
inline int Mesh(uintptr_t renderer, const uint16_t* reference, uintptr_t unused, const float* matrix, const uint32_t* color)
{
    console::Transform fit;
    if (!Fitted(fit) || (*reinterpret_cast<const uint8_t*>(renderer + 6) & 8))
        return mesh.fun(renderer, reference, unused, matrix, color);
    const auto definition = *reinterpret_cast<const uintptr_t*>(*reinterpret_cast<const uintptr_t*>(
        *reinterpret_cast<const uintptr_t*>(renderer) + 8) + 4 * *reference);
    const auto parts = *reinterpret_cast<const uint32_t*>(definition + 4);
    const auto list = *reinterpret_cast<const uint32_t* const*>(definition + 8);
    Bounds bounds;
    bool plain = parts == 1;
    for (unsigned i = 0; i < parts; ++i)
    {
        const auto part = reinterpret_cast<const uint32_t*>(uintptr_t(list[i]));
        const uint32_t type = part[1];
        // Only float positions with optional texture coordinates are measured.
        if ((type & 0x180) != 0x180 || (type & ~0x183u))
            return mesh.fun(renderer, reference, unused, matrix, color);
        const unsigned uv = (type & 3) == 1 ? 2 : (type & 3) == 2 ? 4 : (type & 3) == 3 ? 8 : 0;
        const unsigned offset = (uv + 3) & ~3u, stride = offset + 12;
        const auto bytes = reinterpret_cast<const uint8_t*>(uintptr_t(part[2]));
        unsigned uMin{ 65535 }, uMax{}, vMin{ 65535 }, vMax{};
        for (unsigned j = 0; j < part[3]; ++j)
        {
            const auto position = reinterpret_cast<const float*>(bytes + j * stride + offset);
            bounds.Add(matrix, fit, position[0], position[1]);
            if (uv == 4)
            {
                const auto texel = reinterpret_cast<const uint16_t*>(bytes + j * stride);
                uMin = std::min<unsigned>(uMin, texel[0]);
                uMax = std::max<unsigned>(uMax, texel[0]);
                vMin = std::min<unsigned>(vMin, texel[1]);
                vMax = std::max<unsigned>(vMax, texel[1]);
            }
        }
        plain &= part[3] <= 6 && (part[4] ? uv == 4 && Plain(part[4], uMax - uMin, vMax - vMin) : !uv);
    }
    if (bounds.Outside())
        return 0;
    if (Hud::role != Hud::Role::Scope && plain && bounds.Covers())
    {
        alignas(16) float screen[16];
        Unfit(screen, matrix, fit);
        return mesh.fun(renderer, reference, unused, screen, color);
    }
    return mesh.fun(renderer, reference, unused, matrix, color);
}
inline unsigned Rect(unsigned count, int left, int top, int right, int bottom)
{
    if (left >= right || top >= bottom)
        return count;
    clearVertices[count++] = { 0xFF000000, int16_t(left), int16_t(top), 0, 0 };
    clearVertices[count++] = { 0xFF000000, int16_t(right), int16_t(bottom), 0, 0 };
    return count;
}
inline void Bars()
{
    const auto fit = Hud::TransformFor(Hud::Role::Frontend);
    const int x = int(std::ceil(fit.offsetX / 20));
    const int y = int(std::ceil(fit.offsetY / 20));
    unsigned count{};
    count = Rect(count, 0, 0, x, 272);
    count = Rect(count, 480 - x, 0, 480, 272);
    count = Rect(count, 0, 0, 480, y);
    count = Rect(count, 0, 272 - y, 480, 272);
    if (!count)
        return;
    const auto address = uintptr_t(clearVertices);
    constexpr uint32_t bottomRight = 479 | (271 << 10);
    const uint32_t commands[]
    {
        0xD3000101, 0xD4000000, 0xD5000000 | bottomRight,
        0x15000000, 0x16000000 | bottomRight,
        0x1280011C, 0x10000000 | ((address >> 8) & 0x000F0000),
        0x01000000 | (address & 0x00FFFFFF), 0x04060000 | count,
        0xD3000000, 0x0B000000,
    };
    std::memcpy(clearList, commands, sizeof(commands));
    sceKernelDcacheWritebackRange(clearVertices, sizeof(clearVertices));
    sceKernelDcacheWritebackRange(clearList, sizeof(clearList));
    callList(uintptr_t(clearList));
}
inline int End(uintptr_t renderer)
{
    if (renderer == Geometry::worldOffsets - 1332 && !*reinterpret_cast<const uint8_t*>(renderer + 2))
    {
        if (scopeSeen || Hud::scopeDrawn || (Geometry::worldSeen && Geometry::Framed()))
            Bars();
        scopeSeen = false;
        Hud::scopeDrawn = false;
        Geometry::worldSeen = false;
    }
    return end.fun(renderer);
}
inline void Install()
{
    const auto copySite = sites::Required("? ? 11 3C 09 00 06 3C ? ? 24 8E 25 28 00 02 ? ? ? ? 00 80 C6 24 ? ? 24 8E 00 02 05 34", 16);
    const auto endSite = sites::Required("D0 FF BD 27 20 00 B0 AF 25 80 80 00 24 00 B1 AF 28 00 BF AF ? ? ? ? 02 00 11 26 20 2F 04 8E");
    const auto listSite = sites::Required("F0 FF BD 27 00 00 BF AF ? ? 06 3C ? ? C7 24 10 00 EB 8C 21 30 80 00 00 1E 88 7C 00 5C 8C 7C");
    const auto leafSite = sites::Required("10 FF BD 27 06 00 84 80 C8 00 B0 AF 25 80 A0 00 08 00 85 30 25 20 C0 00 CC 00 B1 AF D0 00 B2 AF");
    copy.fun = injector::MakeCALL(copySite, MovieCopy).get();
    callList = reinterpret_cast<decltype(callList)>(listSite);
    endHook = safetymips::create_inline(endSite, End);
    end.fun = endHook.original<decltype(end.fun)>();
    leafHook = safetymips::create_inline(leafSite, Leaf);
    leaf.fun = leafHook.original<decltype(leaf.fun)>();
    const auto meshSite = sites::Required("60 FF BD 27 06 00 86 80 74 00 B1 AF 25 88 80 00 7C 00 B3 AF 08 00 C6 30 25 20 A0 00 25 98 E0 00 70 00 B0 AF 78 00 B2 AF");
    meshHook = safetymips::create_inline(meshSite, Mesh);
    mesh.fun = meshHook.original<decltype(mesh.fun)>();
}
}
