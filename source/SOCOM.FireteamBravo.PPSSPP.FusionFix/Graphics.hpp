#pragma once

namespace NativeMenu { int Language(); }
namespace Graphics
{
struct VideoVertex { float u, v, x, y, z; };
struct ClearVertex { uint32_t color; int16_t x, y, z, padding; };
static_assert(sizeof(ClearVertex) == 12);
alignas(16) inline ClearVertex vertices[8];
alignas(16) inline uint32_t commands[12];
inline injector::game_fn<void(uintptr_t)> callList;
inline injector::hook_back<int(void*, const void*, unsigned)> copy;
alignas(64) inline uint32_t videoList[512];
alignas(64) inline PspGeContext videoContext;
alignas(16) inline VideoVertex videoVertices[2];
inline bool guInitialized, barsDrawn, videoFailed;
inline injector::hook_back<int(uintptr_t, uintptr_t, uintptr_t)> hudPass;
inline injector::hook_back<int(uintptr_t)> endFrame;
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
    if (videoFailed) return copy.fun(destination, source, length);
    constexpr float native = 480.0f / 272.0f;
    const float aspect = Geometry::AspectRatio();
    const float sx = std::min(native / aspect, 1.0f);
    const float sy = std::min(aspect / native, 1.0f);
    if (sx == 1.0f && sy == 1.0f) return copy.fun(destination, source, length);
    if (length != 512 * 272 * 2) return MovieFallback(destination, source, length);
    if (sceGeDrawSync(0) < 0 || sceGeSaveContext(&videoContext) < 0) return MovieFallback(destination, source, length);
    if (!guInitialized) { sceGuInit(); guInitialized = true; }
    const float x = 240.0f * (1.0f - sx), y = 136.0f * (1.0f - sy);
    videoVertices[0] = { 0, 0, x, y, 0 };
    videoVertices[1] = { 480, 272, x + 480.0f * sx, y + 272.0f * sy, 0 };
    sceKernelDcacheWritebackRange(source, length);
    sceKernelDcacheWritebackRange(videoVertices, sizeof(videoVertices));
    sceGuStart(GU_DIRECT, videoList);
    sceGuDrawBuffer(GU_PSM_5650, reinterpret_cast<void*>(uintptr_t(destination) & 0x001FFFFF), 512);
    sceGuOffset(2048 - 240, 2048 - 136);
    sceGuViewport(2048, 2048, 480, 272);
    sceGuScissor(0, 0, 480, 272);
    sceGuEnable(GU_SCISSOR_TEST);
    sceGuDisable(GU_DEPTH_TEST); sceGuDisable(GU_ALPHA_TEST); sceGuDisable(GU_BLEND);
    sceGuDisable(GU_STENCIL_TEST); sceGuDisable(GU_CULL_FACE); sceGuDisable(GU_LIGHTING);
    sceGuDisable(GU_FOG); sceGuDisable(GU_COLOR_TEST); sceGuDisable(GU_COLOR_LOGIC_OP);
    sceGuPixelMask(0); sceGuClearColor(0xFF000000); sceGuClear(GU_COLOR_BUFFER_BIT);
    sceGuEnable(GU_TEXTURE_2D); sceGuTexMode(GU_PSM_5650, 0, 0, 0);
    sceGuTexImage(0, 512, 512, 512, source); sceGuTexFlush();
    sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGBA); sceGuTexFilter(GU_LINEAR, GU_LINEAR);
    sceGuTexWrap(GU_CLAMP, GU_CLAMP); sceGuTexScale(1, 1); sceGuTexOffset(0, 0);
    sceGuDrawArray(GU_SPRITES, GU_TEXTURE_32BITF | GU_VERTEX_32BITF | GU_TRANSFORM_2D, 2, nullptr, videoVertices);
    sceGuFinish(); sceGuSync(0, 0);
    if (sceGeRestoreContext(&videoContext) < 0) return MovieFallback(destination, source, length);
    return 0;
}
inline unsigned Rect(unsigned n, int l, int t, int r, int b)
{
    if (l >= r || t >= b) return n;
    vertices[n++] = { 0xFF000000, int16_t(l), int16_t(t), 0, 0 };
    vertices[n++] = { 0xFF000000, int16_t(r), int16_t(b), 0, 0 };
    return n;
}
inline void Bars()
{
    constexpr float native = 480.0f / 272.0f;
    const float aspect = Geometry::AspectRatio();
    const float sx = std::min(native / aspect, 1.0f);
    const float sy = std::min(aspect / native, 1.0f);
    const int x = int(std::ceil(240.0f * (1.0f - sx)));
    const int y = int(std::ceil(136.0f * (1.0f - sy)));
    unsigned n{};
    n = Rect(n, 0, 0, x, 272); n = Rect(n, 480 - x, 0, 480, 272);
    n = Rect(n, 0, 0, 480, y); n = Rect(n, 0, 272 - y, 480, 272);
    if (!n) return;
    const auto address = uintptr_t(vertices);
    constexpr uint32_t bottomRight = 479 | (271 << 10);
    const uint32_t list[] {
        0xD3000101, 0xD4000000, 0xD5000000 | bottomRight,
        0x15000000, 0x16000000 | bottomRight,
        0x1280011C, 0x10000000 | ((address >> 8) & 0x000F0000),
        0x01000000 | (address & 0x00FFFFFF), 0x04060000 | n,
        0xD3000000, 0x0B000000,
    };
    std::memcpy(commands, list, sizeof(list));
    sceKernelDcacheWritebackRange(vertices, sizeof(vertices));
    sceKernelDcacheWritebackRange(commands, sizeof(commands));
    callList(uintptr_t(commands));
}
inline void FrameBars()
{
    if (barsDrawn || !*reinterpret_cast<const uintptr_t*>(Hud::manager)) return;
    if (!Hud::scopeActive && !(Geometry::worldSeen && Geometry::Framed())) return;
    Bars();
    barsDrawn = true;
}
// Bars go after the scene and before the HUD, which draws scope readouts and
// prompts over them; frames without a HUD pass get them at the end.
inline int HudPass(uintptr_t renderer, uintptr_t root, uintptr_t camera)
{
    FrameBars();
    return hudPass.fun(renderer, root, camera);
}
inline int EndFrame(uintptr_t renderer)
{
    FrameBars();
    return endFrame.fun(renderer);
}
inline void Install()
{
    callList = decltype(callList)(sites::Required("? ? 06 3C ? ? C6 24 10 00 C5 8C 00 00 C7 8C 23 28 85 00 ? ? 04 3C ? ? 88 24 25 20 E0 00 64 01 07 8D 0B 00 E0 14"));
    const auto movie = sites::Required("25 28 80 00 ? ? 10 3C 04 00 06 3C ? ? 04 8E 14 00 BF AF ? ? ? ? 00 40 C6 24 ? ? 04 8E 00 02 05 34", 20);
    copy.fun = injector::MakeCALL(movie, MovieCopy).get();
    const auto hud = sites::Required("? ? 04 8E 01 00 84 30 07 00 80 10 ? ? 04 26 ? ? 04 3C 00 00 05 8E 25 30 40 02", 28);
    hudPass.fun = injector::MakeCALL(hud, HudPass).get();
    const auto end = sites::Required("? ? 05 3C ? ? A5 24 05 00 85 14 ? ? 04 3C ? ? 04 3C ? ? ? ? ? ? 84 24 ? ? 04 3C ? ? ? ? ? ? 84 24 25 10 00 00", 32);
    endFrame.fun = injector::MakeCALL(end, EndFrame).get();
}
}
