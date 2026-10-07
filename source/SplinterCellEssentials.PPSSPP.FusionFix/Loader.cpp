#include "Screens.hpp"
// PSPLoader stage: the boot "Loading, please wait." picture and every FMV
// (intros and mission briefings) are shown by the loader, not by the game.
// PPSSPP stretches the 480x272 frame to the window, so both are contained at
// the native aspect here, with black side bars.
namespace essentials {
namespace {
injector::hook_back<void*(void*, const void*, unsigned)> pictureCopy;
void* CopyPicture(void* frame, const void* picture, unsigned bytes) {
#ifndef NDEBUG
    logger.WriteF("Loader picture %08X <- %08X (%u)", unsigned(uintptr_t(frame)), unsigned(uintptr_t(picture)), bytes);
#endif
    // The loader reads the file into its own buffer first; resample it there,
    // so the single native copy uploads the finished picture.
    if (bytes == 512 * 272 * 2 && picture) ContainPicture(static_cast<uint16_t*>(const_cast<void*>(picture)));
    return pictureCopy.fun(frame, picture, bytes);
}

// ---- PSPMoviePlayer: patched after sceKernelLoadModule, before it starts.
injector::hook_back<int(int, unsigned, void*, int*, void*)> startModule;
injector::hook_back<int(int, int, int, int, int, int, int)> drawStrip;
injector::hook_back<void()> drawSubtitles;
uintptr_t spriteContext; // address of the GU context pointer (sprite mode at +0x38)
float* subtitle;         // position (x at [0]) and scale (x at [3])

void Emit(uint32_t*& list, uint32_t command) { *list++ = command; }
// Opaque black through-mode rectangle, written into the player's own list
// with the same inline-vertex layout its sprite helper uses.
void BlackBar(int x0, int x1) {
    if (x1 <= x0) return;
    auto context = *reinterpret_cast<uint8_t**>(spriteContext);
    auto& list = *reinterpret_cast<uint32_t**>(context + 8);
    auto p = list;
    Emit(p, 0x1E000000);                                 // texture off
    auto data = p + 2, after = data + 6;
    const auto end = uintptr_t(after);
    Emit(p, 0x10000000 | ((end >> 24) & 0xF) << 16);     // BASE
    Emit(p, 0x08000000 | (end & 0xFFFFFF));              // JUMP over vertices
    struct Vertex { uint32_t color; int16_t x, y, z, pad; };
    auto vertex = reinterpret_cast<Vertex*>(data);
    vertex[0] = {0xFF000000u, int16_t(x0), 0, 0, 0};
    vertex[1] = {0xFF000000u, int16_t(x1), 272, 0, 0};
    p = after;
    const auto at = uintptr_t(data);
    Emit(p, 0x12000000 | 1u << 23 | 7u << 2 | 2u << 7); // VTYPE through, 8888, s16
    Emit(p, 0x10000000 | ((at >> 24) & 0xF) << 16);
    Emit(p, 0x01000000 | (at & 0xFFFFFF));               // VADDR
    Emit(p, 0x04060002);                                 // PRIM sprites, 2 vertices
    Emit(p, 0x1E000001);                                 // texture on
    list = p;
}
float stripScale = 1;
int DrawStrip(int x, int y, int z, int u, int v, int flip, int rotate) {
    if (x == 0) stripScale = Contain(); // once per frame
    const float scale = stripScale;
    if (scale >= 1) return drawStrip.fun(x, y, z, u, v, flip, rotate);
    if (x >= 480) { // the 17th column lies beyond the frame; cover the sides instead
        const int left = ContainX(0, scale);
        BlackBar(0, left);
        BlackBar(480 - left, 480);
        return 0;
    }
    // Sixteen 32-pixel columns; place each at its rounded edges so they abut.
    const int x0 = ContainX(float(x), scale), x1 = ContainX(float(x + 32), scale);
    auto context = *reinterpret_cast<uint8_t**>(spriteContext);
    auto& width = *reinterpret_cast<int*>(context + 0x40);
    const int native = width;
    width = x1 - x0;
    const int result = drawStrip.fun(x0, y, z, u, v, flip, rotate);
    width = native;
    return result;
}
void DrawSubtitles() {
    const float scale = stripScale;
    if (scale >= 1 || !subtitle) return drawSubtitles.fun();
    // The subtitle quad is placed around the centre of the projection.
    const float x = subtitle[0], width = subtitle[3];
    subtitle[0] = x * scale; subtitle[3] = width * scale;
    drawSubtitles.fun();
    subtitle[0] = x; subtitle[3] = width;
}
uintptr_t HighLow(uint32_t high, uint32_t low) { return (uintptr_t(high & 0xFFFF) << 16) + int16_t(low & 0xFFFF); }
uintptr_t JalTarget(uintptr_t at) {
    return ((at + 4) & 0xF0000000) | ((injector::ReadMemory<uint32_t>(at) & 0x03FFFFFF) << 2);
}
void PatchMoviePlayer(int id) {
    SceKernelModuleInfo info{}; info.size = sizeof(info);
    if (sceKernelQueryModuleInfo(id, &info) < 0 || std::strcmp(info.name, "PSPMoviePlayer")) return;
    const uintptr_t text = info.text_addr; const size_t size = info.text_size;
    // displayThread frame: sixteen sceGuDrawSprite columns of the decoded picture.
    const auto strip = range_pattern.get_first(text, size,
        "25 20 40 02 25 28 00 00 25 30 00 00 25 38 40 02 25 40 00 00 25 48 00 00 ?? ?? ?? ?? 25 50 00 00 01 00 31 26 10 00 24 2A", 24);
    const auto subtitles = range_pattern.get_first(text, size,
        "03 63 18 46 ?? ?? ?? ?? 00 00 00 00 02 A3 16 46 03 63 18 46 ?? ?? ?? ?? 00 00 00 00 ?? ?? ?? ?? 00 00 00 00", 28);
    if (!strip) return;
    // sceGuDrawSprite -> its sprite-mode store "lui v1; lw a4,(v1); sw a3,0x44(a4)" in sceGuSpriteMode.
    const auto mode = range_pattern.get_first(text, size,
        "?? ?? 03 3C ?? ?? 68 8C 44 00 07 AD 38 00 04 AD 3C 00 05 AD 08 00 E0 03 40 00 06 AD", 0);
    if (!mode) return;
    spriteContext = HighLow(injector::ReadMemory<uint32_t>(mode), injector::ReadMemory<uint32_t>(mode + 4));
    drawStrip.fun = injector::MakeCALL(strip, DrawStrip).get();
    if (subtitles) {
        // Subtitle state block: "lui s3,hi; addiu s0,s3,lo" at +0x50 of the drawer.
        const auto drawer = JalTarget(subtitles);
        const auto high = injector::ReadMemory<uint32_t>(drawer + 0x50), low = injector::ReadMemory<uint32_t>(drawer + 0x54);
        if ((high & 0xFFFF0000) == 0x3C130000 && (low & 0xFFFF0000) == 0x26700000) {
            subtitle = reinterpret_cast<float*>(HighLow(high, low));
            drawSubtitles.fun = injector::MakeCALL(subtitles, DrawSubtitles).get();
        }
    }
    injector::FlushCaches();
#ifndef NDEBUG
    logger.WriteF("Movie player %08X strip %08X subtitles %08X context %08X", unsigned(text), unsigned(strip), unsigned(subtitles), unsigned(spriteContext));
#endif
}
int StartModule(int id, unsigned size, void* arguments, int* status, void* options) {
    PatchMoviePlayer(id);
    return startModule.fun(id, size, arguments, status, options);
}
}

namespace {
void PatchLoader(uintptr_t text, size_t size) {
    // LOADING_IMAGE: sceIoRead into a buffer, then memcpy to the frame buffer.
    const auto copy = range_pattern.get_first(text, size, "5C 00 A5 8F 25 20 20 02 ?? ?? ?? ?? 25 30 00 02 ?? ?? ?? ?? 00 00 00 00 ?? ?? ?? ?? 00 00 00 00 25 20 40 00 00 02 05 34", 8);
    if (copy) pictureCopy.fun = injector::MakeCALL(copy, CopyPicture).get();
    // The movie player is the only module the loader starts.
    const auto start = range_pattern.get_first(text, size, "18 02 B2 8F 01 00 45 24 14 02 A7 27 25 20 40 02 25 30 20 02 ?? ?? ?? ?? 25 40 00 00", 20);
    if (start) startModule.fun = injector::MakeCALL(start, StartModule).get();
#ifndef NDEBUG
    logger.WriteF("Loader %08X: picture copy %08X, start module %08X", unsigned(text), unsigned(copy), unsigned(start));
#endif
}
// The game returns to the menu or the next mission through RestartPSP: it
// loads PSPLOADERFIX.PRX as a module and starts it, without a new plugin
// instance, so this game-stage instance patches that loader before it runs.
injector::hook_back<int(int, unsigned, void*, int*, void*)> startLoader;
int StartLoader(int id, unsigned size, void* arguments, int* status, void* options) {
    SceKernelModuleInfo info{}; info.size = sizeof(info);
    if (sceKernelQueryModuleInfo(id, &info) >= 0 && !std::strcmp(info.name, "PSPLoader")) {
        PatchLoader(info.text_addr, info.text_size);
        injector::FlushCaches();
    }
    return startLoader.fun(id, size, arguments, status, options);
}
}

void InstallLoaderScreens() {
    containImages = inireader.ReadInteger("MAIN", "ContainFullscreenImages", 1) != 0;
    if (!containImages) return;
    PatchLoader(pattern.text_addr, pattern.text_size);
}

void InstallLoaderRestart(uintptr_t text, size_t size) {
    const auto start = range_pattern.get_first(text, size, "01 00 45 24 25 20 00 02 25 30 20 02 25 38 40 02 ?? ?? ?? ?? 25 40 00 00", 16);
    if (start) startLoader.fun = injector::MakeCALL(start, StartLoader).get();
}
}
