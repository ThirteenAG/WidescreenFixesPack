#include "Screens.hpp"
// Game stage loading screens (main menu picture, mission briefing, "Press X"
// prompt). They draw with through-mode GE sprites, CPU copies and direct
// frame-buffer writes, not the UI canvas, so they are contained here at the
// native aspect with black side bars.
namespace essentials {
namespace {
SafetyMipsInline rect;
injector::hook_back<void*(void*, const void*, unsigned)> pictureCopy;
injector::hook_back<int(int, int, int, int, int, int, int)> sprite;
uintptr_t spriteContext;

void DrawRect(int x, int y, int width, int height, int color) {
    // Everything but full-screen fades follows the contained picture.
    const float scale = Contain();
    const auto left = int16_t(x), top = int16_t(y), size = int16_t(width), tall = int16_t(height);
    if (scale < 1 && !(top <= 0 && tall >= 272)) {
        const int right = left + size > 480 ? 480 : left + size; // 512-wide bands end at the screen edge
        const int x0 = ContainX(left, scale), x1 = ContainX(right, scale);
        x = x0; width = x1 - x0;
    }
    rect.call<void>(x, y, width, height, color);
}
void* CopyPicture(void* frame, const void* picture, unsigned bytes) {
#ifndef NDEBUG
    logger.WriteF("Loading picture %08X <- %08X (%u)", unsigned(uintptr_t(frame)), unsigned(uintptr_t(picture)), bytes);
#endif
    // LoadingMenu: read into the back buffer, shown, then copied to the other one.
    if (bytes == 512 * 272 * 2 && picture) ContainPicture(static_cast<uint16_t*>(const_cast<void*>(picture)));
    return pictureCopy.fun(frame, picture, bytes);
}
void SaveProgressArea();
// Fast-load stream (PSN): the first chunk is the LoadingMenu picture,
// decompressed straight into the displayed frame buffer.
injector::hook_back<int(void*, unsigned, unsigned, void*, int)> streamChunk;
int StreamChunk(void* stream, unsigned destination, unsigned bytes, void* buffer, int file) {
    const int result = streamChunk.fun(stream, destination, bytes, buffer, file);
#ifndef NDEBUG
    static int logged;
    if (logged < 6) { ++logged; logger.WriteF("chunk %08X %X", destination, bytes); }
#endif
    if ((destination & 0x0FFFFFFF) == 0x04000000 && bytes == 512 * 272 * 2) {
        ContainPicture(reinterpret_cast<uint16_t*>(destination));
        SaveProgressArea();
    }
    return result;
}
// Fast-load progress bars are written by the CPU straight into the frame
// buffer: a vertical bar at x 448..469 (single player) or a horizontal one at
// rows 260..265 from x 31 (multiplayer). Move them with the picture.
injector::hook_back<void(float)> progress;
constexpr int stripX = 440, rowsY = 256;
uint16_t rightStrip[272][480 - stripX], bottomRows[272 - rowsY][480];
uint16_t* Frame() { return reinterpret_cast<uint16_t*>(0x04000000); }
void SaveProgressArea() {
    auto frame = Frame();
    for (int y = 0; y < 272; ++y) std::memcpy(rightStrip[y], frame + y * 512 + stripX, sizeof(rightStrip[y]));
    for (int y = rowsY; y < 272; ++y) std::memcpy(bottomRows[y - rowsY], frame + y * 512, sizeof(bottomRows[0]));
}
void Progress(float fraction) {
    progress.fun(fraction);
    const float scale = Contain();
    if (scale >= 1) return;
    auto frame = Frame();
    static uint16_t bar[272][22], line[6][480];
    // Vertical bar: rows 9..261, columns 448..469.
    for (int y = 0; y < 272; ++y) std::memcpy(bar[y], frame + y * 512 + 448, sizeof(bar[y]));
    for (int y = rowsY; y < 272; ++y) if (y >= 260 && y < 266) std::memcpy(line[y - 260], frame + y * 512, sizeof(line[0]));
    for (int y = 0; y < 272; ++y) std::memcpy(frame + y * 512 + stripX, rightStrip[y], sizeof(rightStrip[y]));
    for (int y = rowsY; y < 272; ++y) std::memcpy(frame + y * 512, bottomRows[y - rowsY], sizeof(bottomRows[0]));
    const int x0 = ContainX(448, scale), x1 = ContainX(470, scale);
    for (int y = 0; y < 272; ++y)
        for (int x = x0; x < x1; ++x) {
            const auto pixel = bar[y][(x - x0) * 22 / (x1 - x0)];
            if (pixel != rightStrip[y][448 - stripX + (x - x0) * 22 / (x1 - x0)]) frame[y * 512 + x] = pixel;
        }
    for (int y = 260; y < 266; ++y)
        for (int x = ContainX(0, scale); x < ContainX(480, scale); ++x) {
            const int source = int((x + 0.5f - ContainX(0, scale)) / scale);
            const auto pixel = line[y - 260][source < 480 ? source : 479];
            if (pixel != bottomRows[y - rowsY][source < 480 ? source : 479]) frame[y * 512 + x] = pixel;
        }
}
// Fast-load mission start (disc): the pre-rendered briefing (.BRF) is
// uncompressed straight into the front buffer, then copied to the back one.
SafetyMipsInline uncompressFront;
int UncompressFront(const char* path, unsigned language, void* buffer) {
    const int result = uncompressFront.call<int>(path, language, buffer);
    if (result == 0) {
        ContainPicture(Frame());
        SaveProgressArea();
    }
    return result;
}
int DrawSprite(int x, int y, int z, int u, int v, int flip, int rotate) {
    const float scale = Contain();
    if (scale >= 1) return sprite.fun(x, y, z, u, v, flip, rotate);
    auto context = *reinterpret_cast<uint8_t**>(spriteContext);
    auto& width = *reinterpret_cast<int*>(context + 0x40);
    const int native = width;
    const int x0 = ContainX(float(x), scale), x1 = ContainX(float(x + native), scale);
    width = x1 - x0;
    const int result = sprite.fun(x0, y, z, u, v, flip, rotate);
    width = native;
    return result;
}
unsigned HookSprites(uintptr_t function, size_t size, uintptr_t target) {
    unsigned count = 0;
    const uint32_t call = 0x0C000000u | ((target >> 2) & 0x03FFFFFF);
    for (uintptr_t at = function; function && at < function + size; at += 4)
        if (injector::ReadMemory<uint32_t>(at) == call) { sprite.fun = injector::MakeCALL(at, DrawSprite).get(); ++count; }
    return count;
}

// The loading code sits at the start of both executables (menu and mission);
// search only there, so these hooks are published before the game's first
// loading screen while the remaining startup patches are still being found.
uintptr_t Early(const char* signature, int offset = 0) {
    const uintptr_t start = pattern.text_addr; const size_t size = pattern.text_size < 0x40000 ? pattern.text_size : 0x40000;
    const auto first = range_pattern.get_first(start, size, signature, offset);
    return first && !range_pattern.get(1, start, size, signature, offset) ? first : 0;
}
}

void InstallLoadingScreens() {
    // draw_rect: briefing frame, progress bars, fades.
    if (const auto address = Early("B0 FF BD 27 38 00 B6 AF ?? ?? 16 3C ?? ?? C9 8E 30 00 B4 AF 25 A0 80 00 34 00 B5 AF 28 00 24 25 ?? ?? 15 3C ?? ?? A9 8E"))
        rect = safetymips::create_inline(address, DrawRect);
    // ShowProgressInit: LoadingMenu picture copy between the two frame buffers.
    if (const auto copy = Early("04 00 06 3C 20 00 A4 8F 25 28 40 02 ?? ?? ?? ?? 00 40 C6 24 ?? ?? ?? ?? 00 00 00 00", 12))
        pictureCopy.fun = injector::MakeCALL(copy, CopyPicture).get();
    InstallLoaderRestart(pattern.text_addr, pattern.text_size < 0x40000 ? pattern.text_size : 0x40000);
    if (const auto chunk = Early("88 00 85 8E 84 00 86 8E 25 20 60 02 25 38 40 02 ?? ?? ?? ?? 25 40 00 02", 16))
        streamChunk.fun = injector::MakeCALL(chunk, StreamChunk).get();
    // FastLoadProgressShow (PSN menu / disc mission builds differ in prologue).
    auto bars = Early("D0 FF BD 27 10 00 B4 E7 14 00 B6 E7 18 00 B0 AF 1C 00 B1 AF 20 00 B2 AF 24 00 B3 AF 28 00 BF AF ?? ?? ?? ?? 06 65 00 46");
    if (!bars) bars = Early("D0 FF BD 27 10 00 B4 E7 14 00 B0 AF 18 00 B1 AF 1C 00 B2 AF 20 00 B3 AF 24 00 BF AF ?? ?? ?? ?? 06 65 00 46");
    if (bars) {
        const uint32_t call = 0x0C000000u | ((bars >> 2) & 0x03FFFFFF);
        const uintptr_t start = pattern.text_addr, end = start + (pattern.text_size < 0x40000 ? pattern.text_size : 0x40000);
        for (uintptr_t at = start; at < end; at += 4)
            if (injector::ReadMemory<uint32_t>(at) == call) progress.fun = injector::MakeCALL(at, Progress).get();
    }
    if (const auto front = Early("C0 FF BD 27 2C 00 B0 AF 30 00 B1 AF 25 80 C0 00 25 88 A0 00 34 00 B2 AF 38 00 B3 AF 3C 00 BF AF"))
        uncompressFront = safetymips::create_inline(front, UncompressFront);
    // Briefing text/picture and "Press X" sprites (sceGuSpriteMode + sceGuDrawSprite).
    // Every sceGuDrawSprite call at the start of the executable belongs to
    // these loading screens (the renderer's own uses are much further on).
    const auto mode = Early("?? ?? 03 3C ?? ?? 68 8C 44 00 07 AD 38 00 04 AD 3C 00 05 AD 08 00 E0 03 40 00 06 AD");
    const auto draw = Early("?? ?? 0B 3C 21 18 80 00 ?? ?? 64 8D 21 60 C0 00 21 68 E0 00 21 70 00 01 21 C0 20 01 21 10 A0 00 F0 FF BD 27");
    if (mode && draw) {
        spriteContext = (uintptr_t(injector::ReadMemory<uint16_t>(mode)) << 16) + int16_t(injector::ReadMemory<uint16_t>(mode + 4));
        HookSprites(pattern.text_addr, pattern.text_size < 0x40000 ? pattern.text_size : 0x40000, draw);
    }
}
}
