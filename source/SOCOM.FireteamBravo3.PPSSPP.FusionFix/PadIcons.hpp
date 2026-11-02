#pragma once

namespace NativeMenu { int Language(); }
namespace PadIcons
{
using Button = console::padbuttons::Button;
inline int style;
struct Box
{
    int left, top, right, bottom;
};
struct Entry
{
    uint32_t hash;
    Button button;
};
inline constexpr Entry entries[]
{
    // Native resource identifiers use CRC32 hashes.
    { 0x17277050, Button::LeftStick }, { 0x63EA3F83, Button::Circle },
    { 0xF70ACC3F, Button::Square }, { 0xDE0519B9, Button::Triangle },
    { 0xE039BA44, Button::Cross }, { 0x98A9AFFF, Button::Down },
    { 0x28472582, Button::Right }, { 0x855ACE37, Button::Up },
    { 0xFE31D8AC, Button::Left }, { 0x90102B52, Button::L1 },
    { 0x6A1F1631, Button::R1 }, { 0xC38C0DEE, Button::Start },
    { 0x711B4E56, Button::Select }, { 0x3E842A94, Button::R1 },
    { 0x942F4B6D, Button::L1 }, { 0xB431371C, Button::Circle },
    { 0x62B7E2D1, Button::Square }, { 0xA8B1BB1B, Button::Triangle },
    { 0x01AA2FDE, Button::Cross }, { 0x3C3B7756, Button::Cross },
    { 0xDA43CE88, Button::Right }, { 0x32DE1263, Button::Circle },
    { 0x8FE8CAF7, Button::Square }, { 0x29997306, Button::Triangle },
    { 0x41F45112, Button::Cross }, { 0x4881B1FA, Button::Down },
    { 0xFFAC3561, Button::Left }, { 0x3A17B53A, Button::Left },
    { 0x532FD258, Button::Left }, { 0xC8DC428C, Button::Right },
    { 0xD84DE615, Button::Right }, { 0xC26E0F91, Button::Right },
    { 0x3F9E6090, Button::Up }, { 0x61108965, Button::Select },
    { 0x12316474, Button::Start }, { 0x4980A0FB, Button::L1 },
    { 0x2AE02819, Button::L1 }, { 0x7BCCD19E, Button::Circle },
    { 0x2DB264EC, Button::Circle }, { 0x2D823918, Button::R1 },
    { 0xCFC916FB, Button::R1 }, { 0x6924B164, Button::Select },
    { 0xE7010B0C, Button::Square }, { 0x4D279B76, Button::Square },
    { 0xD379E5AF, Button::Start }, { 0x66DBA2FC, Button::Triangle },
    { 0xE120D44D, Button::Triangle }, { 0xF81F5459, Button::Cross },
    { 0xE07C0BCA, Button::Cross }, { 0x8D6EF664, Button::LeftStick },
    { 0x2E19C6A9, Button::Left }, { 0x58FDF913, Button::Right }
};
inline uint32_t Pixel(const uint8_t* artwork, int x, int y)
{
    if (x < 0 || y < 0 || x >= 64 || y >= 64)
        return 0;
    if (artwork == console::padbuttons::artwork[0][unsigned(Button::Start)])
    {
        const int dx = x * 2 - 63, dy = y * 2 - 63;
        const int radius = dx * dx + dy * dy;
        const bool ring = radius >= 44 * 44 && radius <= 49 * 49;
        const bool lines = x >= 20 && x < 44 &&
            ((y >= 22 && y < 26) || (y >= 30 && y < 34) || (y >= 38 && y < 42));
        return ring || lines ? 0xFFFFFFFF : 0;
    }
    const unsigned index = artwork[y * 64 + x];
    const unsigned swizzled = (index & ~0x18u) | ((index & 8) << 1) | ((index & 16) >> 1);
    uint32_t pixel;
    std::memcpy(&pixel, artwork + 4096 + swizzled * 4, 4);
    return (pixel & 0xFFFFFF) | (std::min(unsigned(pixel >> 24) * 255u / 128u, 255u) << 24);
}
inline void Raster(uint32_t* pixels, unsigned width, unsigned height, Box target, const uint8_t* artwork)
{
    Box source{ 64, 64, 0, 0 };
    for (int y = 0; y < 64; ++y)
        for (int x = 0; x < 64; ++x)
            if (Pixel(artwork, x, y) >> 24)
            {
                source.left = std::min(source.left, x);
                source.top = std::min(source.top, y);
                source.right = std::max(source.right, x + 1);
                source.bottom = std::max(source.bottom, y + 1);
            }
    const float scale = std::min(float(target.right - target.left) / float(source.right - source.left),
        float(target.bottom - target.top) / float(source.bottom - source.top));
    const float left = (target.left + target.right - (source.right - source.left) * scale) * 0.5f;
    const float top = (target.top + target.bottom - (source.bottom - source.top) * scale) * 0.5f;
    std::memset(pixels, 0, width * height * 4);
    for (int y = target.top; y < target.bottom; ++y)
        for (int x = target.left; x < target.right; ++x)
        {
            const float sx = (x + 0.5f - left) / scale + source.left - 0.5f;
            const float sy = (y + 0.5f - top) / scale + source.top - 0.5f;
            const int ix = int(std::floor(sx)), iy = int(std::floor(sy));
            const float fx = sx - ix, fy = sy - iy;
            float alpha{}, rgb[3]{};
            for (unsigned dy = 0; dy < 2; ++dy)
                for (unsigned dx = 0; dx < 2; ++dx)
                {
                    const uint32_t pixel = Pixel(artwork, ix + dx, iy + dy);
                    const float weight = (dx ? fx : 1 - fx) * (dy ? fy : 1 - fy);
                    const float a = float(pixel >> 24) * weight;
                    alpha += a;
                    for (unsigned c = 0; c < 3; ++c)
                        rgb[c] += float((pixel >> (c * 8)) & 255) * a;
                }
            if (alpha > 0)
            {
                uint32_t result = uint32_t(std::round(alpha)) << 24;
                for (unsigned c = 0; c < 3; ++c)
                    result |= uint32_t(std::round(rgb[c] / alpha)) << (c * 8);
                pixels[y * width + x] = result;
            }
        }
}
inline Box NativeBox(const uint8_t* header, const uint8_t* pixels, const uint32_t* palette)
{
    uint16_t width, height;
    uint32_t flags;
    std::memcpy(&width, header + 4, 2);
    std::memcpy(&height, header + 6, 2);
    std::memcpy(&flags, header + 8, 4);
    Box box{ width, height, 0, 0 };
    const unsigned bits = header[13] == 0 ? 4 : 8;
    const unsigned stride = width * bits / 8;
    for (unsigned y = 0; y < height; ++y)
        for (unsigned x = 0; x < width; ++x)
        {
            const unsigned byteX = x * bits / 8;
            const unsigned offset = flags & 1 ? ((y / 8) * (stride / 16) + byteX / 16) * 128 +
                (y % 8) * 16 + byteX % 16 : y * stride + byteX;
            const unsigned index = bits == 4 ? ((pixels[offset] >> ((x & 1) * 4)) & 15) : pixels[offset];
            if (palette[index] >> 24)
            {
                box.left = std::min(box.left, int(x));
                box.top = std::min(box.top, int(y));
                box.right = std::max(box.right, int(x + 1));
                box.bottom = std::max(box.bottom, int(y + 1));
            }
        }
    return box;
}
inline unsigned Log2(unsigned value)
{
    unsigned result{};
    while ((1u << result) < value)
        ++result;
    return result;
}
inline void Commands(uint32_t* commands, uintptr_t address, unsigned width, unsigned height)
{
    const uint32_t values[]
    {
        0xC2000000, 0xC3000003, 0xC8000001, 0xC6000101,
        0xA0000000 | uint32_t(address & 0xFFFFFF),
        0xA8000000 | width | uint32_t((address >> 8) & 0xFF0000),
        0xB8000000 | (Log2(height) << 8) | Log2(width),
        0xC9010100, 0x1E000001, 0xCB000000, 0x0B000000
    };
    std::memcpy(commands, values, sizeof(values));
}
struct Cached
{
    bool ready;
    uintptr_t wrapper[2];
    alignas(16) uint32_t commands[12];
    alignas(16) uint32_t pixels[32 * 32];
};
inline Cached cache[2][sizeof(entries) / sizeof(entries[0])];
inline SafetyMipsInline bindHook;
inline injector::hook_back<int(uintptr_t)> bind;
inline int NativeButton(uintptr_t object)
{
    static bool reported;
    if (!reported)
    {
        reported = true;
        static constexpr const char* messages[] = {
            "Fusion Fix: unsupported button artwork; using native icons.\n",
            "Fusion Fix : image de bouton inconnue ; icones d'origine utilisees.\n",
            "Fusion Fix: Tastensymbol unbekannt; Originalsymbole werden verwendet.\n",
            "Fusion Fix: immagine pulsante sconosciuta; uso icone originali.\n",
            "Fusion Fix: imagen de boton desconocida; se usan iconos originales.\n"
        };
        PatchNotice(messages[NativeMenu::Language()]);
    }
    return bind.fun(object);
}
inline int Bind(uintptr_t object)
{
    if (style == 2)
        return bind.fun(object);
    const auto native = reinterpret_cast<const uintptr_t*>(object);
    const auto header = reinterpret_cast<const uint8_t*>(native[0]);
    uint32_t hash;
    std::memcpy(&hash, header, 4);
    const Entry* entry{};
    for (const auto& candidate : entries)
        if (candidate.hash == hash)
        {
            entry = &candidate;
            break;
        }
    if (!entry)
        return bind.fun(object);
    uint16_t width, height;
    std::memcpy(&width, header + 4, 2);
    std::memcpy(&height, header + 6, 2);
    if ((header[13] != 0 && header[13] != 1) || header[16] != 6 || !width || !height || width > 32 || height > 32)
        return NativeButton(object);
    const unsigned family = style == 3 ? 1 : 0;
    // The cache owns its pixels and commands; no native texture pointers survive this call.
    auto item = &cache[family][entry - entries];
    if (item->ready)
        return bind.fun(reinterpret_cast<uintptr_t>(item->wrapper));
    logger.WriteF("Preparing controller artwork: %08X, family %u.\n", hash, family);
    const auto box = NativeBox(header, reinterpret_cast<const uint8_t*>(native[5]),
        reinterpret_cast<const uint32_t*>(native[6]));
    if (box.right <= box.left || box.bottom <= box.top)
        return NativeButton(object);
    Raster(item->pixels, width, height, box, console::padbuttons::artwork[family][unsigned(entry->button)]);
    Commands(item->commands, reinterpret_cast<uintptr_t>(item->pixels), width, height);
    item->wrapper[1] = reinterpret_cast<uintptr_t>(item->commands);
    sceKernelDcacheWritebackInvalidateRange(item, sizeof(Cached));
    item->ready = true;
    logger.WriteF("Controller artwork prepared: %08X, family %u.\n", hash, family);
    return bind.fun(reinterpret_cast<uintptr_t>(item->wrapper));
}
inline void Install()
{
    const auto site = Hud::Required("03 00 80 14 06 8D 00 46 31 00 00 10 00 00 00 00 ? ? ? ? 00 00 00 00 ? ? 04 3C ? ? 85 8C 00 12 06 3C 04 00 A7 24 9F 01 C6 24 ? ? 87 AC", 16);
    const uintptr_t address = injector::GetBranchDestination(site).as_int();
    bindHook = safetymips::create_inline(address, Bind);
    bind.fun = bindHook.original<decltype(bind.fun)>();
}
}
