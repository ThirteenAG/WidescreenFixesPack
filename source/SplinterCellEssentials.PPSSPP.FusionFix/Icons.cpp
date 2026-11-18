#include "Game.hpp"
#include "Shared/Console/PatchNotice.hpp"
#include "../Shared/Console/PadButtonArtwork.hpp"
#include <cmath>

// MAIN.GamepadIcons: the face buttons in the game's button art (menu hints,
// the interaction list, HUD prompts) show Xbox or Nintendo letters instead of
// the PSP symbols. The buttons are parts of three 256-wide swizzled 8-bit
// atlases whose palettes are shared with other artwork, so each symbol is
// erased to its button's disc colour and the letter is drawn in the symbol's
// colour, blended through the palette's own shades.
namespace essentials {
namespace {
namespace pad = console::padbuttons;
using Button = pad::Button;
constexpr unsigned AtlasWidth = 256;
// A button disc: centre, the radius of its symbol and the size of the artwork drawn there.
struct Disc { float x, y, radius, size; Button button; };
constexpr Disc menuDiscs[] = {
    {13, 65, 8.6f, 21, Button::Circle}, {39, 65, 8.6f, 21, Button::Triangle}, {65, 65, 8.6f, 21, Button::Cross},
    {91, 65, 8.6f, 21, Button::Square}, {143, 65, 8.6f, 21, Button::Triangle}, {169, 65, 8.6f, 21, Button::Cross},
    {195, 65, 8.6f, 21, Button::Square},
};
constexpr Disc interactionDiscs[] = {
    {243, 55, 9.6f, 27, Button::Circle}, {243, 81, 9.6f, 27, Button::Triangle},
    {243, 107, 9.6f, 27, Button::Square}, {243, 133, 9.6f, 27, Button::Cross},
};
constexpr Disc smallDiscs[] = {
    {8.5f, 148.5f, 5.6f, 13, Button::Cross}, {26.5f, 148.5f, 5.6f, 13, Button::Triangle},
    {44.5f, 148.5f, 5.6f, 13, Button::Circle}, {62.5f, 148.5f, 5.6f, 13, Button::Square},
};
struct Atlas {
    uint32_t name;
    unsigned height, size; // TSIZE0
    const Disc* discs;
    unsigned discCount;
    uint8_t* original;
    uintptr_t texture, pixels;
    const uint32_t* palette;
    unsigned markAt;
    uint8_t mark[16];
};
alignas(64) uint8_t menuOriginal[AtlasWidth * 128];
alignas(64) uint8_t interactionOriginal[AtlasWidth * 256];
alignas(64) uint8_t smallOriginal[AtlasWidth * 256];
Atlas atlases[] = {
    {0xB673BEB0, 128, 0x708, menuDiscs, sizeof(menuDiscs) / sizeof(*menuDiscs), menuOriginal},
    {0x1552B525, 256, 0x808, interactionDiscs, sizeof(interactionDiscs) / sizeof(*interactionDiscs), interactionOriginal},
    {0x3B04CA60, 256, 0x808, smallDiscs, sizeof(smallDiscs) / sizeof(*smallDiscs), smallOriginal},
};
int family = -1;
uintptr_t dlManager;
SafetyMipsMid drawTile;

uint8_t& Texel(uint8_t* data, unsigned x, unsigned y) {
    return data[((y / 8) * (AtlasWidth / 16) + x / 16) * 128 + (y % 8) * 16 + x % 16];
}
float Luminance(uint32_t c) { return 0.299f * (c & 255) + 0.587f * (c >> 8 & 255) + 0.114f * (c >> 16 & 255); }
// The shared artwork: PSMT8 indices and a CSM1-ordered CLUT with PS2 alpha.
uint32_t ArtColor(const uint8_t* art, unsigned index) {
    const unsigned swizzled = (index & ~0x18u) | ((index & 8) << 1) | ((index & 16) >> 1);
    uint32_t c; std::memcpy(&c, art + 4096 + swizzled * 4, 4);
    return (c & 0xFFFFFF) | (std::min(unsigned(c >> 24) * 255u / 128u, 255u) << 24);
}
// Letter coverage of an artwork: distance from its dominant opaque colour.
void Coverage(const uint8_t* art, float (&out)[64 * 64]) {
    unsigned counts[256]{};
    for (unsigned i = 0; i < 4096; ++i) if ((ArtColor(art, art[i]) >> 24) > 200) ++counts[art[i]];
    unsigned dominant = 0;
    for (unsigned i = 1; i < 256; ++i) if (counts[i] > counts[dominant]) dominant = i;
    const float base = Luminance(ArtColor(art, dominant));
    float peak = 1;
    for (unsigned i = 0; i < 4096; ++i) {
        const uint32_t c = ArtColor(art, art[i]);
        out[i] = std::fabs(Luminance(c) - base) * float(c >> 24) / 255.0f;
        peak = std::max(peak, out[i]);
    }
    for (auto& v : out) v = std::min(1.0f, v / (0.8f * peak));
}
unsigned Nearest(const uint32_t* palette, const float (&target)[4]) {
    unsigned best = 0; float bestDistance = 1e30f;
    for (unsigned i = 0; i < 256; ++i) {
        float distance = 0;
        for (unsigned k = 0; k < 4; ++k) { const float d = float(palette[i] >> (8 * k) & 255) - target[k]; distance += d * d; }
        if (distance < bestDistance) { bestDistance = distance; best = i; }
    }
    return best;
}
void DrawDisc(const Atlas& atlas, uint8_t* data, const Disc& disc) {
    const auto palette = atlas.palette;
    const int left = int(disc.x - disc.radius - 1), right = int(disc.x + disc.radius + 1);
    const int top = int(disc.y - disc.radius - 1), bottom = int(disc.y + disc.radius + 1);
    // Disc colour: the most common colour near the centre; symbol: the brightest.
    const float sample = disc.radius * 0.7f;
    unsigned counts[256]{}, symbol = 0;
    for (int y = top; y <= bottom; ++y)
        for (int x = left; x <= right; ++x) {
            const float dx = x + 0.5f - disc.x, dy = y + 0.5f - disc.y;
            if (dx * dx + dy * dy >= disc.radius * disc.radius) continue;
            const unsigned index = Texel(atlas.original, x, y);
            if (dx * dx + dy * dy < sample * sample) ++counts[index];
            if (Luminance(palette[index]) > Luminance(palette[symbol])) symbol = index;
        }
    unsigned background = 0;
    for (unsigned i = 1; i < 256; ++i) if (counts[i] > counts[background]) background = i;
    static float coverage[64 * 64];
    Coverage(pad::artwork[family][unsigned(disc.button)], coverage);
    uint8_t shades[16];
    for (unsigned level = 0; level < 16; ++level) {
        const float t = level / 15.0f;
        float target[4];
        for (unsigned k = 0; k < 4; ++k)
            target[k] = float(palette[background] >> (8 * k) & 255) * (1 - t) + float(palette[symbol] >> (8 * k) & 255) * t;
        shades[level] = uint8_t(Nearest(palette, target));
    }
    for (int y = top; y <= bottom; ++y)
        for (int x = left; x <= right; ++x) {
            const float dx = x + 0.5f - disc.x, dy = y + 0.5f - disc.y;
            if (dx * dx + dy * dy >= disc.radius * disc.radius) continue;
            const int u = int((dx / disc.size + 0.5f) * 64), v = int((dy / disc.size + 0.5f) * 64);
            const float c = u >= 0 && u < 64 && v >= 0 && v < 64 ? coverage[v * 64 + u] : 0.0f;
            Texel(data, x, y) = shades[int(c * 15 + 0.5f)];
        }
}
// Texture setup of a UTexture: its display list in the offline display list
// memory (TBP0, TBW0, TSIZE0, TMODE, TPSM, CBP, CBW).
bool Locate(Atlas& atlas, uintptr_t texture) {
    const auto manager = *reinterpret_cast<const uintptr_t*>(dlManager);
    const auto offset = *reinterpret_cast<const uintptr_t*>(texture + 176);
    if (!manager || !offset) return false;
    const auto list = reinterpret_cast<const uint32_t*>(*reinterpret_cast<const uintptr_t*>(manager + 4) + *reinterpret_cast<const uint32_t*>(offset));
    uint32_t tbp = 0, tbw = 0, size = 0, mode = 0, format = ~0u, cbp = 0, cbw = 0;
    for (unsigned i = 0; i < 16 && list[i] >> 24 != 0x0B; ++i) {
        const uint32_t value = list[i] & 0xFFFFFF;
        switch (list[i] >> 24) {
            case 0xA0: tbp = value; break;
            case 0xA8: tbw = value; break;
            case 0xB8: size = value; break;
            case 0xC2: mode = value; break;
            case 0xC3: format = value; break;
            case 0xB0: cbp = value; break;
            case 0xB1: cbw = value; break;
        }
    }
    if (format != 5 || !(mode & 1) || (tbw & 0xFFFF) != AtlasWidth || size != atlas.size || !tbp || !cbp) return false;
    atlas.texture = texture;
    atlas.pixels = tbp | (tbw >> 16 & 0xF) << 24;
    atlas.palette = reinterpret_cast<const uint32_t*>(cbp | (cbw >> 16 & 0xF) << 24);
    return true;
}
void Apply(Atlas& atlas) {
    const auto data = reinterpret_cast<uint8_t*>(atlas.pixels);
    const unsigned bytes = AtlasWidth * atlas.height;
    std::memcpy(atlas.original, data, bytes);
    for (unsigned i = 0; i < atlas.discCount; ++i) DrawDisc(atlas, data, atlas.discs[i]);
    sceKernelDcacheWritebackRange(data, bytes);
    // Remember changed bytes so a reload of the original atlas is noticed.
    atlas.markAt = 0;
    for (unsigned at = 0; at + sizeof(atlas.mark) <= bytes; at += sizeof(atlas.mark))
        if (std::memcmp(data + at, atlas.original + at, sizeof(atlas.mark))) { atlas.markAt = at; break; }
    std::memcpy(atlas.mark, data + atlas.markAt, sizeof(atlas.mark));
}
bool Object(uintptr_t at) { return at >= 0x08800000 && at < 0x0A000000 && !(at & 3); }
uint32_t ClassName(uintptr_t object) {
    const auto type = *reinterpret_cast<const uintptr_t*>(object + 0x24);
    return Object(type) ? *reinterpret_cast<const uint32_t*>(type + 0x20) : 0;
}
// Menus draw the atlases through a FinalBlend; modifiers keep their material at +0x4C.
uintptr_t Texture(uintptr_t material) {
    for (unsigned depth = 0; depth < 3 && Object(material); ++depth) {
        const uint32_t type = ClassName(material);
        if (type == NameCrc("Texture")) return material;
        if (type != NameCrc("FinalBlend") && type != NameCrc("ColorModifier") && type != NameCrc("TexScaler") &&
            type != NameCrc("TexPanner") && type != NameCrc("TexOscillator")) return 0;
        material = *reinterpret_cast<const uintptr_t*>(material + 0x4C);
    }
    return 0;
}
void Check(uintptr_t material) {
    const auto texture = Texture(material);
    if (!texture) return;
    const uint32_t name = *reinterpret_cast<const uint32_t*>(texture + 0x20);
    for (auto& atlas : atlases) {
        if (atlas.name != name) continue;
        if (texture != atlas.texture) {
            if (Locate(atlas, texture)) Apply(atlas);
        } else if (std::memcmp(reinterpret_cast<const uint8_t*>(atlas.pixels) + atlas.markAt, atlas.mark, sizeof(atlas.mark))) {
            Apply(atlas);
        }
        return;
    }
}
}
void InstallIcons() {
    // 0 Auto (Xbox), 1 Xbox, 2 PlayStation (the game's own art), 3 Nintendo.
    const int style = std::clamp(inireader.ReadInteger("MAIN", "GamepadIcons", 0), 0, 3);
    if (style == 2) return;
    family = int(style == 3 ? pad::Family::Nintendo : pad::Family::Xbox);
    const auto singleton = sites::Unique("?? ?? 04 3C 08 00 E0 03 ?? ?? 82 8C E0 FF BD 27 14 00 B1 AF 00 00 11 34 10 00 B0 AF 18 00 BF AF ?? ?? ?? 0E 6C 00 04 34");
    const auto tile = sites::DrawTile();
    if (!singleton || !tile)
    {
        PatchRejected(PSP_HOOK_UNSUPPORTED_INSTRUCTION);
        return;
    }
    dlManager = Absolute(singleton, 0, 8);
    // FCanvasUtil::DrawTile(..., UMaterial* material in a1, ...).
    drawTile = safetymips::create_mid(tile, [](SafetyMipsContext& regs) { Check(uintptr_t(regs.a1)); });
}
}
