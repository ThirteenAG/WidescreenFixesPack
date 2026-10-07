#include "Controls.hpp"
#include "ButtonIcons.hpp"
#include "../Shared/Console/StoriesButtonText.hpp"
#include <cstring>

// PC key artwork for GTA LCS PS2 (help text, menu footers, bindings page).
//
// LCS PS2 RslRaster (verified in the IDB, sub_2B5A18 -> sub_29E278): +0 pixel
// data, +4 flags = log2 width | log2 height << 6 | depth << 12 | mip levels << 20,
// byte +7 bit 0 = swizzled. An 8-bit raster uploads its 256-entry CT32 CLUT
// (16x16, CSM1) right after the pixels. VCS keeps two extra words in front,
// which is why its layout rendered garbled here. The generated artwork is
// already in this layout and stays plugin-owned; the renderer only borrows it.
//
// Help text names controls with "~k~ ~ACTION~". CText substitutes them
// (0x2F8F58) with the GXT entry "C<mode><ACTION>". With the PC scheme the
// substitution produces the bound keys instead: private-use characters, drawn
// by the font's PrintChar hook as key icons, or key names without artwork.
namespace lcs {
namespace {
namespace art = console::pcbuttons;
using namespace console::stories;
struct Raster { const uint8_t* data; uint32_t flags; };
struct Texture { Raster* raster; uint8_t remaining[84]; };
constexpr unsigned artworkCount = unsigned(art::Key::Count);
Raster rasters[artworkCount]{};
Texture textures[artworkCount]{};
Sprite sprites[artworkCount]{};
// Text characters 0xE100 + artwork index; the font works with character - 32.
constexpr uint16_t iconCharacter = 0xE100, iconGlyph = iconCharacter - 32;
// Native glyph used to size icons: the cross button (0xE2 - 32).
constexpr int16_t buttonGlyph = 0xC2;

safetymips::GameInline<int(const uint16_t*, uint16_t*)> substitute;
safetymips::GameInline<uint64_t(int, float, float)> printChar;
safetymips::GameInline<float(int, int)> rawWidth;
safetymips::GameInline<float(int)> scaledWidth;
safetymips::GameInline<uint64_t(Sprite*, const Color*, float, float, float, float)> drawButton;
pcsx2::GameFunction<uint64_t(const Rect*, const Color*, float, float, float, float, float, float, float, float)> batchQuad;
pcsx2::GameFunction<uint64_t()> flushBatch;
pcsx2::GameFunction<uint64_t(Sprite*)> setRaster;
pcsx2::GameFunction<uint64_t(Sprite*, const Rect*, const Color*, float, float, float, float, float, float, float, float)> drawSprite;

int Icon(uint16_t glyph) {
    const unsigned index = uint16_t(glyph - iconGlyph);
    return index < artworkCount ? int(index) : -1;
}
float Aspect(unsigned index) { return float(art::artwork[index].contentWidth) / float(art::height); }

// Help text actions and the bindings that name them.
struct HelpAction { const char* gxt; Act keys[4]; uint8_t count; const char* mouse; };
constexpr HelpAction help[] = {
    {"AMBUY", {}, 0, "Enter"}, {"TRSK", {}, 0, "Enter"},
    {"AMEXI", {Act::EnterVehicle}, 1, nullptr}, {"VEEE", {Act::EnterVehicle}, 1, nullptr},
    {"AMMOV", {Act::Forward, Act::Left, Act::Backward, Act::Right}, 4, nullptr},
    {"ANS", {Act::Phone}, 1, nullptr}, {"PUCF", {Act::Phone}, 1, nullptr},
    {"CVEIW", {Act::Camera}, 1, nullptr}, {"FREE1", {Act::Aim}, 1, nullptr}, {"PDLT", {Act::Aim}, 1, nullptr},
    {"FREE2", {}, 0, "mouse"}, {"PDLO1", {}, 0, "mouse"}, {"PDLO2", {}, 0, "mouse"}, {"PDLOO", {}, 0, "mouse"},
    {"PDBAK", {Act::LookBehind}, 1, nullptr}, {"VELB", {Act::LookBehind}, 1, nullptr},
    {"PDCTL", {Act::PrevTarget, Act::NextTarget}, 2, nullptr},
    {"PDCWE", {Act::PrevWeapon, Act::NextWeapon}, 2, nullptr},
    {"PDFW", {Act::Attack}, 1, nullptr}, {"PED_FIREWEAPON", {Act::Attack}, 1, nullptr},
    {"PDSPR", {Act::Sprint}, 1, nullptr}, {"SNZI", {Act::ZoomIn}, 1, nullptr}, {"SNZO", {Act::ZoomOut}, 1, nullptr},
    {"TGSUB", {Act::Mission}, 1, nullptr}, {"VEACC", {Act::Accelerate}, 1, nullptr}, {"VEBRK", {Act::Brake}, 1, nullptr},
    {"VECRS", {Act::PrevRadio, Act::NextRadio}, 2, nullptr}, {"VEHB", {Act::Handbrake}, 1, nullptr},
    {"VEHN", {Act::Horn}, 1, nullptr}, {"VELL", {Act::LookLeft}, 1, nullptr}, {"VELR", {Act::LookRight}, 1, nullptr},
    {"VESTR", {Act::SteerLeft, Act::SteerRight}, 2, nullptr},
    {"VEWEA", {Act::TurretLeft, Act::TurretRight}, 2, nullptr}, {"VEWEI", {Act::TurretUp, Act::TurretDown}, 2, nullptr},
    {"VEWEP", {Act::VehicleFire}, 1, nullptr},
};
struct Writer {
    uint16_t* out; unsigned used = 0, limit;
    void put(uint16_t c) { if (used + 1 < limit) out[used++] = c; }
    void text(const char* s) { while (*s) put(uint16_t(*s++)); }
    void key(uint8_t code) {
        const auto* image = art::Find(code);
        if (image) { put(uint16_t(iconCharacter + unsigned(image - art::artwork))); return; }
        char name[24]; KeyText(code, name); text(name);
    }
    void finish() { out[used] = 0; }
};
// Gameplay entries that name buttons in plain words (R1 = Aim on this
// scheme). The rest (BV_*, CVHELP, DBGHELP, LOADCAR, LS2_H1, WTC_INS, MC_04)
// belong to debug menus that retail builds never show.
constexpr TextRewrite entries[] = {
    {"TUP_AD", "~w~You have 3 adrenaline pills, Press {Aim} to use one."},
    {"GA_7", "~w~Arm with ~h~ ~k~ ~PED_FIREWEAPON~~w~. Bomb will go off when engine is started."},
};
// Callers may keep the pointer (brief messages), so every entry owns its text.
uint16_t entryText[sizeof(entries) / sizeof(*entries)][320];
int Substitute(const uint16_t* source, uint16_t* output) {
    if (!settings.pcControls || !source || !output) return substitute.call(source, output);
    // Same parse as the native routine: skip to the action's opening '~',
    // read up to the closing '~'. The result counts the name plus one.
    const uint16_t* p = source;
    if (*p != u'~') { ++p; for (unsigned guard = 0; *p != u'~' && guard < 64; ++guard) ++p; }
    ++p;
    char name[24]{};
    unsigned length = 0;
    while (p[length] != u'~' && p[length] && length < sizeof(name) - 1) { name[length] = char(p[length]); ++length; }
    if (p[length] != u'~') return substitute.call(source, output);
    for (const auto& entry : help) {
        if (std::strcmp(name, entry.gxt)) continue;
        // The caller's buffer holds 256 characters.
        Writer writer{output, 0, 64};
        if (entry.mouse) {
            if (!std::strcmp(entry.mouse, "mouse")) writer.key(code::mouseMove);
            else writer.key(code::enter);
        }
        for (unsigned i = 0; i < entry.count; ++i) {
            if (i) writer.put(u' ');
            const uint8_t key = bindings.primary(entry.keys[i]);
            if (key) writer.key(key); else writer.text("-");
        }
        writer.finish();
        return int(length + 1);
    }
    return substitute.call(source, output);
}

// CFont::PrintChar(glyph) with x in f12, y in f13 (0x164C50). Button glyphs
// (>= 0xC0) are 16 font units high, 4 below the line top, and about as wide.
uint64_t PrintChar(int glyph, float x, float y) {
    const int index = Icon(uint16_t(glyph));
    if (index < 0) return printChar.call(glyph, x, y);
    if (!(x > 0.0f && x < 640.0f && y > -12.0f && y < 640.0f)) return 0;
    const float scaleY = *reinterpret_cast<const float*>(0x408428);
    const float wide = *reinterpret_cast<const uint8_t*>(0x3D8B8A) ? 0.486f : 0.54f;
    // Slightly smaller than the button cell, so the icon keeps the spacing
    // and baseline of the surrounding text.
    const float height = 14.0f * scaleY;
    const Rect rect{x, y + 3.0f * scaleY + height, x + height * (30.72f * wide / 16.0f) * Aspect(unsigned(index)), y + 3.0f * scaleY};
    // The glyph quads are batched with the font raster; draw the key with its
    // own raster in between, then restore the font's.
    flushBatch();
    setRaster(&sprites[index]);
    const auto* fontColor = reinterpret_cast<const Color*>(0x40842C);
    // Shadow passes keep their dark tint, like the native button glyphs.
    const Color color = fontColor->red < 64 && fontColor->green < 64 && fontColor->blue < 64
        ? *fontColor : Color{255, 255, 255, fontColor->alpha};
    const float u = float(art::artwork[index].contentWidth) / float(art::artwork[index].width);
    batchQuad(&rect, &color, 0, 0, u, 0, 0, 1, u, 1);
    flushBatch();
    const auto font = *reinterpret_cast<const int16_t*>(0x408444);
    setRaster(reinterpret_cast<Sprite*>(0x408458 + 4 * font));
    return 0;
}
float RawWidth(int glyph, int flag) {
    const int index = Icon(uint16_t(glyph));
    return index < 0 ? rawWidth.call(glyph, flag) : rawWidth.call(buttonGlyph, flag) * Aspect(unsigned(index));
}
float ScaledWidth(int glyph) {
    const int index = Icon(uint16_t(glyph));
    return index < 0 ? scaledWidth.call(glyph) : scaledWidth.call(buttonGlyph) * Aspect(unsigned(index));
}

// Menu footer prompts draw the frontend button sprites (CMenuManager +1196,
// four bytes each, from "btn_*_PSP") with sub_320D20. Key for each button
// while the PC scheme drives the menus (Controls.cpp).
uint8_t FooterKey(uintptr_t sprite) {
    const auto offset = sprite - address::menuManager;
    if (offset < 1300 || offset > 1348 || (offset & 3)) return 0;
    switch ((offset - 1196) / 4) {
    case 26: return bindings.primary(Act::Attack);  // circle
    case 27: return code::enter;                    // cross
    case 28: return code::down;
    case 30: return bindings.primary(Act::Phone);   // L1
    case 31: return code::left;
    case 32: return bindings.primary(Act::Aim);     // R1
    case 33: return code::right;
    case 34: return bindings.primary(Act::Camera);  // select
    case 35: return bindings.primary(Act::Jump);    // square
    case 36: return code::escape;                   // start
    case 37: return code::back;                     // triangle
    case 38: return code::up;
    default: return 0;
    }
}
// sub_320D20(sprite, color, x, y, width, height) draws with the render
// states for frontend icons (f12..f15, e.g. 0x344DF4).
uint64_t DrawButton(Sprite* sprite, const Color* color, float left, float top, float native, float height) {
    const uint8_t key = settings.pcControls ? FooterKey(reinterpret_cast<uintptr_t>(sprite)) : 0;
    const auto* image = key ? art::Find(key) : nullptr;
    if (!image) return drawButton.call(sprite, color, left, top, native, height);
    // Keep the prompt's height and left edge; wide keys take a little of the
    // gap before the caption.
    float width = height * float(image->contentWidth) / float(art::height);
    if (width > native * 1.3f) width = native * 1.3f;
    DrawKeyIcon(key, left, top, height, *color, width);
    return 0;
}
}
const uint16_t* ButtonText(const char* key) {
    if (!settings.pcControls || !key) return nullptr;
    const auto* rewrite = FindRewrite(entries, key);
    if (!rewrite) return nullptr;
    auto& buffer = entryText[rewrite - entries];
    Writer writer{buffer, 0, sizeof(buffer) / sizeof(*buffer)};
    FormatText(rewrite->text, bindings, writer);
    writer.finish();
    return buffer;
}
float KeyIconWidth(uint8_t key, float height) {
    const auto* image = art::Find(key);
    return image ? height * float(image->contentWidth) / float(art::height) : 0.0f;
}
float DrawKeyIcon(uint8_t key, float x, float y, float height, const Color& color, float width) {
    const auto* image = art::Find(key);
    if (!image) return 0.0f;
    if (width <= 0.0f) width = KeyIconWidth(key, height);
    const auto index = unsigned(image - art::artwork);
    const Rect rect{x, y + height, x + width, y};
    const float u = float(image->contentWidth) / float(image->width);
    drawSprite(&sprites[index], &rect, &color, 0, 0, u, 0, 0, 1, u, 1);
    return width;
}
void InstallButtonIcons() {
    for (unsigned i = 0; i < artworkCount; ++i) {
        const auto& image = art::artwork[i];
        rasters[i] = {image.data, art::Log2(image.width) | (art::log2Height << 6) | (8u << 12) | (1u << 20)};
        textures[i].raster = &rasters[i];
        sprites[i].texture = &textures[i];
    }
    batchQuad.bind(0x320BB8); flushBatch.bind(0x320CB0); setRaster.bind(0x321980); drawSprite.bind(0x320E60);
    substitute = safetymips::create_inline_game(0x2F8F58, Substitute);
    rawWidth = safetymips::create_inline_game(0x1670B0, RawWidth);
    scaledWidth = safetymips::create_inline_game(0x167258, ScaledWidth);
    printChar = safetymips::create_inline_game(0x164C50, PrintChar);
    drawButton = safetymips::create_inline_game(0x320D20, DrawButton);
}
}
