#include "Game.hpp"
#include <array>

namespace lcs {
namespace {
template<uintptr_t Address, DrawMode Mode, class Signature> struct HudPhase;
template<uintptr_t Address, DrawMode Mode, class Return, class... Args>
struct HudPhase<Address, Mode, Return(Args...)> {
    inline static safetymips::GameInline<Return(Args...)> hook;
    static Return Draw(Args... args) {
        DrawScope scope(Mode);
        return hook.call(args...);
    }
    static void Install() { hook = safetymips::create_inline_game(Address, Draw); }
};
template<uintptr_t Address, DrawMode Mode, class Signature = uint64_t()>
void Phase() { HudPhase<Address, Mode, Signature>::Install(); }
std::array<SafetyMipsMid, 5> boundaries;
SafetyMipsMid advance;
pcsx2::GameFunction<float(int, int)> rawWidth;

// Every font record appended since the last flush, with its native glyph-width
// scale and the horizontal compression applied below (1 when none). The queue
// (0x408468, 960 bytes) holds a few dozen records between flushes; appending
// at a lower address means it was flushed and restarted.
struct QueuedRecord { uintptr_t address; float nativeScale, factor; bool caption; };
struct QueuedRecords {
    QueuedRecord entries[64];
    unsigned count = 0;
    bool overflow = false;
    void add(uintptr_t address, float nativeScale, float factor, bool caption) {
        if (count && address <= entries[count - 1].address) { count = 0; overflow = false; }
        if (count < sizeof(entries) / sizeof(*entries)) entries[count++] = {address, nativeScale, factor, caption};
        else overflow = true;
    }
    // The record whose text contains `text` (text follows its record), when it
    // is compressed or one of the plugin's own menu captions.
    const QueuedRecord* adjusted(uintptr_t text) const {
        if (overflow) return nullptr;
        for (unsigned i = count; i--;)
            if (entries[i].address < text) {
                const auto& entry = entries[i];
                return (entry.factor != 1.0f || entry.caption) && text - entry.address < 0x3C0 ? &entry : nullptr;
            }
        return nullptr;
    }
} queued;

// The font queue contains a 60-byte layout record followed by UTF-16 text.
// Correct each record once, immediately after it is appended. The native font
// renderer can then flush at any point without losing the originating HUD phase.
void QueuedText(SafetyMipsContext& regs) {
    auto record = reinterpret_cast<float*>(uintptr_t(regs.a1));
    // s1: the source text (sub_165418 keeps its first argument there).
    const bool caption = MenuCaption(uintptr_t(regs.s1));
    if (drawMode == DrawMode::None) { queued.add(uintptr_t(regs.a1), record[3], 1.0f, caption); return; }
    auto transform = DrawingTransform(false, record[1], record[2]);
    record[1] = transform.x(record[1]);
    record[2] = transform.y(record[2]);
    queued.add(uintptr_t(regs.a1), record[3], transform.scaleX > 0.0f ? transform.scaleX : 1.0f, caption);
    record[3] *= transform.scaleX; // Glyph width
    record[4] *= transform.scaleY; // Glyph height
    record[6] *= transform.scaleX; // Space advance
    record[7] *= transform.scaleY / transform.scaleX; // Slant dy/dx
    record[8] = transform.x(record[8]); // Slant reference x
    record[9] = transform.y(record[9]); // Slant reference y
}
// Glyph advance in the queue flush (sub_164FB8, 0x165378): f0 holds
// floor(width * scale * 4/3), s0 the glyph and s2 the text position. The
// native layout steps in whole quarter-pixel units; with a compressed glyph
// scale each floor dropped up to one unit per character, so narrow glyphs and
// spaces shrank more than the rest of the text and spaces vanished on wide
// screens ("MOREDISPLAY"). Use the native step, compressed linearly, which
// keeps the native proportions and matches the affine record position.
//
// The frontend font kerns some capitals tighter (second width column, e.g. -2
// for E and Y) while their artwork still reaches the cell edge, so a following
// space nearly disappears ("MOREDISPLAY OPTIONS"). Native captions avoid such
// pairs; the plugin's own captions use the uncondensed width there instead.
void GlyphAdvance(SafetyMipsContext& regs) {
    const auto* record = queued.adjusted(uintptr_t(regs.s2));
    if (!record) return;
    const int glyph = int(int16_t(regs.s0));
    float width = rawWidth(glyph, 0);
    if (record->caption && glyph > 0 && glyph < 0xC0 && *reinterpret_cast<const uint16_t*>(uintptr_t(regs.s2) + 2) == ' ') {
        const int font = *reinterpret_cast<const int16_t*>(0x408444);
        if (font >= 0 && font < 3) {
            const float cell = float(*reinterpret_cast<const int16_t*>(0x38D148 + 838 * font + 2 * glyph));
            if (width < cell) width = cell;
        }
    }
    regs.f0 = __builtin_floorf(width * record->nativeScale * 1.3333334f) * record->factor;
}
}
void InstallHud() {
    Phase<0x1F6338, DrawMode::Center>(); // Render2DStuff: fades, captions and script UI
    Phase<0x23F438, DrawMode::Center>(); // Native CHud, including scope/crosshair rendering
    Phase<0x2E4330, DrawMode::RightTop, uint64_t(void*)>(); // Mission timers and counters
    Phase<0x244B40, DrawMode::RightTop, uint64_t(int16_t)>();
    Phase<0x245138, DrawMode::RightTop, uint64_t(int16_t)>();
    Phase<0x164FB8, DrawMode::None>(); // Font queue records are already corrected
    Phase<0x1CA858, DrawMode::World>(); // CPickups::RenderPickUpText: labels keep their projected position
    Phase<0x1F66D0, DrawMode::Center>(); // Render2dStuffAfterFade: CHud::DrawAfterFade, help and big messages
    Phase<0x1F4540, DrawMode::RightTop>(); // DrawLoadingText: right-justified "Loading..." in the top-right corner
    Phase<0x2EB538, DrawMode::Center>(); // Credits

    boundaries[0] = safetymips::create_mid(0x240050, [](SafetyMipsContext&) { drawMode = DrawMode::RightTop; });
    boundaries[1] = safetymips::create_mid(0x240BA8, [](SafetyMipsContext&) { drawMode = DrawMode::RightBottom; });
    boundaries[2] = safetymips::create_mid(0x241E54, [](SafetyMipsContext&) { drawMode = DrawMode::LeftBottom; });
    boundaries[3] = safetymips::create_mid(0x241F80, [](SafetyMipsContext&) { drawMode = DrawMode::Center; });
    boundaries[4] = safetymips::create_mid<&QueuedText>(0x165720);
    rawWidth.bind(0x1670B0);
    advance = safetymips::create_mid<&GlyphAdvance>(0x165378);
}
}
