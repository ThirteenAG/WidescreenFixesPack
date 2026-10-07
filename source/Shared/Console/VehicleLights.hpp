#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace console {
// Both Stories games store each damaged lamp in a two-bit field. Keep the
// selection independent of platform layouts and texture storage.
struct VehicleLightState {
    bool engine, headlights, handbrake;
    float accelerator, brake;
    uint32_t damage;

    unsigned texture() const {
        if (!engine) return 0;
        const auto broken = [this](unsigned lamp) { return ((damage >> (lamp * 2)) & 3) == 1; };
        const bool front = !broken(0) || !broken(1);
        const bool rear = !broken(2) || !broken(3);
        unsigned tail = headlights && rear ? 1 : 0;
        if (rear) {
            if (brake > 0 || handbrake) tail = 2;
            if (accelerator < 0) tail = 3;
        }
        return (headlights && front ? 4 : 0) + tail;
    }
};

// Light areas of a car body texture, generated offline by
// tools/vehicle-lights/generate.py. TailReverse lamps turn white when
// reversing, the way the original hand-painted textures did.
enum LightRole : uint8_t { LightHead, LightTail, LightTailReverse, LightBrake, LightReverse, LightRoles };
struct LightRect { uint8_t x0, y0, x1, y1; LightRole role; };
struct LightSet { uint16_t first, count, width, height; };

// GS memory tables for the PS2 levels stored for a wider upload: PSMT4 block
// and nibble order, and the PSMCT16 position of each block and halfword.
namespace ps2gs {
inline constexpr uint8_t block4[32] = {
    0, 2, 8, 10, 1, 3, 9, 11,
    4, 6, 12, 14, 5, 7, 13, 15,
    16, 18, 24, 26, 17, 19, 25, 27,
    20, 22, 28, 30, 21, 23, 29, 31,
};
inline constexpr uint16_t column4[512] = {
    0, 8, 32, 40, 64, 72, 96, 104, 2, 10, 34, 42, 66, 74, 98, 106,
    4, 12, 36, 44, 68, 76, 100, 108, 6, 14, 38, 46, 70, 78, 102, 110,
    16, 24, 48, 56, 80, 88, 112, 120, 18, 26, 50, 58, 82, 90, 114, 122,
    20, 28, 52, 60, 84, 92, 116, 124, 22, 30, 54, 62, 86, 94, 118, 126,
    65, 73, 97, 105, 1, 9, 33, 41, 67, 75, 99, 107, 3, 11, 35, 43,
    69, 77, 101, 109, 5, 13, 37, 45, 71, 79, 103, 111, 7, 15, 39, 47,
    81, 89, 113, 121, 17, 25, 49, 57, 83, 91, 115, 123, 19, 27, 51, 59,
    85, 93, 117, 125, 21, 29, 53, 61, 87, 95, 119, 127, 23, 31, 55, 63,
    192, 200, 224, 232, 128, 136, 160, 168, 194, 202, 226, 234, 130, 138, 162, 170,
    196, 204, 228, 236, 132, 140, 164, 172, 198, 206, 230, 238, 134, 142, 166, 174,
    208, 216, 240, 248, 144, 152, 176, 184, 210, 218, 242, 250, 146, 154, 178, 186,
    212, 220, 244, 252, 148, 156, 180, 188, 214, 222, 246, 254, 150, 158, 182, 190,
    129, 137, 161, 169, 193, 201, 225, 233, 131, 139, 163, 171, 195, 203, 227, 235,
    133, 141, 165, 173, 197, 205, 229, 237, 135, 143, 167, 175, 199, 207, 231, 239,
    145, 153, 177, 185, 209, 217, 241, 249, 147, 155, 179, 187, 211, 219, 243, 251,
    149, 157, 181, 189, 213, 221, 245, 253, 151, 159, 183, 191, 215, 223, 247, 255,
    256, 264, 288, 296, 320, 328, 352, 360, 258, 266, 290, 298, 322, 330, 354, 362,
    260, 268, 292, 300, 324, 332, 356, 364, 262, 270, 294, 302, 326, 334, 358, 366,
    272, 280, 304, 312, 336, 344, 368, 376, 274, 282, 306, 314, 338, 346, 370, 378,
    276, 284, 308, 316, 340, 348, 372, 380, 278, 286, 310, 318, 342, 350, 374, 382,
    321, 329, 353, 361, 257, 265, 289, 297, 323, 331, 355, 363, 259, 267, 291, 299,
    325, 333, 357, 365, 261, 269, 293, 301, 327, 335, 359, 367, 263, 271, 295, 303,
    337, 345, 369, 377, 273, 281, 305, 313, 339, 347, 371, 379, 275, 283, 307, 315,
    341, 349, 373, 381, 277, 285, 309, 317, 343, 351, 375, 383, 279, 287, 311, 319,
    448, 456, 480, 488, 384, 392, 416, 424, 450, 458, 482, 490, 386, 394, 418, 426,
    452, 460, 484, 492, 388, 396, 420, 428, 454, 462, 486, 494, 390, 398, 422, 430,
    464, 472, 496, 504, 400, 408, 432, 440, 466, 474, 498, 506, 402, 410, 434, 442,
    468, 476, 500, 508, 404, 412, 436, 444, 470, 478, 502, 510, 406, 414, 438, 446,
    385, 393, 417, 425, 449, 457, 481, 489, 387, 395, 419, 427, 451, 459, 483, 491,
    389, 397, 421, 429, 453, 461, 485, 493, 391, 399, 423, 431, 455, 463, 487, 495,
    401, 409, 433, 441, 465, 473, 497, 505, 403, 411, 435, 443, 467, 475, 499, 507,
    405, 413, 437, 445, 469, 477, 501, 509, 407, 415, 439, 447, 471, 479, 503, 511,
};
inline constexpr uint8_t blockX16[32] = {
    0, 0, 1, 1, 0, 0, 1, 1, 2, 2, 3, 3, 2, 2, 3, 3,
    0, 0, 1, 1, 0, 0, 1, 1, 2, 2, 3, 3, 2, 2, 3, 3,
};
inline constexpr uint8_t blockY16[32] = {
    0, 1, 0, 1, 2, 3, 2, 3, 0, 1, 0, 1, 2, 3, 2, 3,
    4, 5, 4, 5, 6, 7, 6, 7, 4, 5, 4, 5, 6, 7, 6, 7,
};
inline constexpr uint8_t columnX16[128] = {
    0, 8, 1, 9, 0, 8, 1, 9, 2, 10, 3, 11, 2, 10, 3, 11,
    4, 12, 5, 13, 4, 12, 5, 13, 6, 14, 7, 15, 6, 14, 7, 15,
    0, 8, 1, 9, 0, 8, 1, 9, 2, 10, 3, 11, 2, 10, 3, 11,
    4, 12, 5, 13, 4, 12, 5, 13, 6, 14, 7, 15, 6, 14, 7, 15,
    0, 8, 1, 9, 0, 8, 1, 9, 2, 10, 3, 11, 2, 10, 3, 11,
    4, 12, 5, 13, 4, 12, 5, 13, 6, 14, 7, 15, 6, 14, 7, 15,
    0, 8, 1, 9, 0, 8, 1, 9, 2, 10, 3, 11, 2, 10, 3, 11,
    4, 12, 5, 13, 4, 12, 5, 13, 6, 14, 7, 15, 6, 14, 7, 15,
};
inline constexpr uint8_t columnY16[128] = {
    0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1,
    0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1,
    2, 2, 2, 2, 3, 3, 3, 3, 2, 2, 2, 2, 3, 3, 3, 3,
    2, 2, 2, 2, 3, 3, 3, 3, 2, 2, 2, 2, 3, 3, 3, 3,
    4, 4, 4, 4, 5, 5, 5, 5, 4, 4, 4, 4, 5, 5, 5, 5,
    4, 4, 4, 4, 5, 5, 5, 5, 4, 4, 4, 4, 5, 5, 5, 5,
    6, 6, 6, 6, 7, 7, 7, 7, 6, 6, 6, 6, 7, 7, 7, 7,
    6, 6, 6, 6, 7, 7, 7, 7, 6, 6, 6, 6, 7, 7, 7, 7,
};
}

// A paletted 4/8-bit texture: mip levels followed by an RGBA palette, as both
// RslRaster implementations store it.
struct LightImage {
    // ps2Swizzle: 8 = PSMT8 texels stored for a PSMCT32 upload of half the width
    // and height, 4 = PSMT4 texels stored for a PSMCT16 upload of half the width
    // and height (PS2 LCS rasters with the level's swizzle bit set).
    struct Level { uint32_t offset; uint16_t width, height, stride; bool usable; uint8_t ps2Swizzle; };
    Level levels[8];
    unsigned count, depth, colours;
    uint32_t palette, bytes;
    bool swizzled;   // PSP GE layout: 16-byte by 8-row blocks
    bool ps2Clut;    // PS2 CSM1 order for 256 entries

    // Byte holding texel (x, y); for 4-bit texels `high` selects the upper nibble.
    uint32_t locate(const Level& level, unsigned x, unsigned y, bool& high) const {
        high = x & 1;
        if (level.ps2Swizzle == 8) {
            // 16x16 blocks, column pairs swapped every four rows.
            const unsigned w = level.width;
            const unsigned block = (y & ~15u) * w + (x & ~15u) * 2;
            const unsigned swap = ((y + 2) >> 2 & 1) * 4;
            const unsigned row = (((y & ~3u) >> 1) + (y & 1)) & 7;
            return level.offset + block + row * w * 2 + ((x + swap) & 7) * 4 + ((y >> 1) & 1) + ((x >> 2) & 2);
        }
        if (level.ps2Swizzle == 4) {
            using namespace ps2gs;
            // GS nibble address in a PSMT4 buffer (128x128 pages, 32x16 blocks) ...
            const unsigned w = level.width, pages4 = w > 128 ? w / 128 : 1;
            const unsigned page = (y >> 7) * pages4 + (x >> 7);
            const unsigned nibble = ((page * 8192 + block4[((y & 127) >> 4) * 4 + ((x & 127) >> 5)] * 256) << 1) + column4[(y & 15) * 32 + (x & 31)];
            // ... read back as PSMCT16 (64x64 pages, 16x8 blocks) of half the width.
            const unsigned byte = nibble >> 1, w16 = w / 2, pages16 = w16 > 64 ? w16 / 64 : 1;
            const unsigned page16 = byte >> 13, block = (byte >> 8) & 31, half = (byte >> 1) & 127;
            const unsigned px = (page16 % pages16) * 64 + blockX16[block] * 16 + columnX16[half];
            const unsigned py = (page16 / pages16) * 64 + blockY16[block] * 8 + columnY16[half];
            high = nibble & 1;
            return level.offset + (py * w16 + px) * 2 + (byte & 1);
        }
        const unsigned column = depth == 4 ? x >> 1 : x;
        if (!swizzled) return level.offset + y * level.stride + column;
        return level.offset + ((((y >> 3) * (level.stride >> 4)) + (column >> 4)) << 7) + ((y & 7) << 4) + (column & 15);
    }
    unsigned get(const uint8_t* data, const Level& level, unsigned x, unsigned y) const {
        bool high;
        const uint8_t value = data[locate(level, x, y, high)];
        return depth == 8 ? value : high ? value >> 4 : value & 15;
    }
    void set(uint8_t* data, const Level& level, unsigned x, unsigned y, unsigned index) const {
        bool high;
        uint8_t& value = data[locate(level, x, y, high)];
        if (depth == 8) value = uint8_t(index);
        else value = high ? uint8_t((value & 0x0F) | (index << 4)) : uint8_t((value & 0xF0) | (index & 15));
    }
    const uint8_t* colour(const uint8_t* data, unsigned index) const {
        if (ps2Clut && colours == 256) index = (index & 0xE7) | ((index & 8) << 1) | ((index & 16) >> 1);
        return data + palette + index * 4;
    }
    uint8_t* colour(uint8_t* data, unsigned index) const {
        return const_cast<uint8_t*>(colour(static_cast<const uint8_t*>(data), index));
    }
};

namespace lights {
enum Tone : uint8_t { ToneNone, ToneWarm, ToneWhite, ToneTail, ToneBrake };
// Variants follow VehicleLightState::texture(): +4 headlights, +1 tail, +2 brake, +3 reverse.
inline Tone ToneOf(unsigned role, unsigned variant) {
    const bool head = variant >= 4;
    const unsigned rear = variant & 3;
    switch (role) {
    case LightHead: return head ? ToneWarm : ToneNone;
    case LightTail: return rear == 1 || (rear == 3 && head) ? ToneTail : rear == 2 ? ToneBrake : ToneNone;
    case LightTailReverse: return rear == 1 ? ToneTail : rear == 2 ? ToneBrake : rear == 3 ? (head ? ToneWarm : ToneWhite) : ToneNone;
    case LightBrake: return rear == 2 ? ToneBrake : ToneNone;
    case LightReverse: return rear == 3 ? (head ? ToneWarm : ToneWhite) : ToneNone;
    default: return ToneNone;
    }
}
// White tones share one palette ramp, red tones the other.
inline unsigned Family(Tone tone) { return tone == ToneWarm || tone == ToneWhite ? 0 : 1; }
// Dim edge to bright core for each tone.
inline constexpr uint8_t ramps[5][2][3] = {
    {{0, 0, 0}, {0, 0, 0}},
    {{255, 198, 124}, {255, 249, 228}},   // headlights: warm white
    {{208, 212, 222}, {255, 255, 255}},   // reverse: white
    {{150, 8, 4}, {255, 40, 24}},         // tail lights
    {{236, 26, 10}, {255, 108, 70}},      // brake lights: brightest red
};
inline constexpr unsigned MaxRamp = 6, None = 0xFF;
inline unsigned Value(const uint8_t* c) { unsigned v = c[0] > c[1] ? c[0] : c[1]; return v > c[2] ? v : c[2]; }

// Shared scratch for one generation at a time (rendering is single threaded).
struct Scratch {
    uint8_t roles[32768];
    uint16_t counts[LightRoles + 1][256];
};
inline Scratch scratch;

// Rasterise the rectangles of every usable level into scratch.roles.
inline bool BuildRoles(const LightImage& image, const LightRect* rects, const LightSet& set, uint32_t* at) {
    uint32_t total = 0;
    for (unsigned l = 0; l < image.count; ++l) { at[l] = total; total += uint32_t(image.levels[l].width) * image.levels[l].height; }
    if (total > sizeof(scratch.roles) || !set.width || !set.height) return false;
    std::memset(scratch.roles, None, total);
    for (unsigned l = 0; l < image.count; ++l) {
        const auto& level = image.levels[l];
        if (!level.usable) continue;
        uint8_t* map = scratch.roles + at[l];
        for (unsigned i = 0; i < set.count; ++i) {
            const auto& r = rects[set.first + i];
            // Partly covered texels of smaller levels belong to the lamp: lights stay visible at a distance.
            unsigned x0 = r.x0 * level.width / set.width, y0 = r.y0 * level.height / set.height;
            unsigned x1 = ((r.x1 + 1u) * level.width + set.width - 1) / set.width;
            unsigned y1 = ((r.y1 + 1u) * level.height + set.height - 1) / set.height;
            if (x1 > level.width) x1 = level.width;
            if (y1 > level.height) y1 = level.height;
            for (unsigned y = y0; y < y1; ++y) std::memset(map + y * level.width + x0, r.role, x1 > x0 ? x1 - x0 : 0);
        }
    }
    return true;
}
}

// Per-texture palette plan shared by all eight variants. Palette entries used
// only by lit lamps are recoloured in place, which keeps the painted detail;
// lamp texels sharing an entry with the body move to the closest recoloured
// entry, or to free entries holding a ramp of the lamp colour.
struct LightPlan {
    enum : uint8_t { Keep, Own0, Own1, Slot0, Slot1 };
    uint8_t remap[256];
    uint8_t kind[8][256];                     // per variant and palette entry
    uint8_t shade[8][256];                    // brightness of Slot entries
    uint8_t value[LightRoles][2];             // brightness range of each lamp role (min, max)
    uint8_t role[256];                        // role using each entry most
    uint8_t alpha[2];
    uint8_t lit;                              // variants with lit lamps
    bool merged;

    static unsigned Shade(unsigned v, unsigned lo, unsigned hi) {
        unsigned t = hi > lo ? (v > lo ? v - lo : 0) * 256 / (hi - lo + 1) : 255;
        if (t > 255) t = 255;
        return 96 + t * 159 / 255;   // keep lamps bright: 96..255
    }

    // Uses of entry i in variant v: returns texels left unlit, sets the lit families (bit 0 white, bit 1 red).
    static unsigned Uses(const uint16_t (*counts)[256], unsigned v, unsigned i, unsigned* families) {
        using namespace lights;
        unsigned unlit = counts[LightRoles][i];
        *families = 0;
        for (unsigned r = 0; r < LightRoles; ++r) {
            if (!counts[r][i]) continue;
            const Tone tone = ToneOf(r, v);
            if (tone == ToneNone) unlit += counts[r][i];
            else *families |= 1u << Family(tone);
        }
        return unlit;
    }

    bool build(const uint8_t* source, const LightImage& image, const LightRect* rects, const LightSet& set) {
        using namespace lights;
        uint32_t at[8];
        if (!BuildRoles(image, rects, set, at)) return false;
        auto& counts = scratch.counts;
        std::memset(counts, 0, sizeof(counts));
        unsigned low[LightRoles], high[LightRoles];
        for (unsigned r = 0; r < LightRoles; ++r) { low[r] = 255; high[r] = 0; }
        uint16_t alphaCounts[2][256]{};
        for (unsigned l = 0; l < image.count; ++l) {
            const auto& level = image.levels[l];
            if (!level.usable) continue;
            const uint8_t* map = scratch.roles + at[l];
            for (unsigned y = 0; y < level.height; ++y)
                for (unsigned x = 0; x < level.width; ++x) {
                    const unsigned index = image.get(source, level, x, y), role = map[y * level.width + x];
                    ++counts[role == None ? LightRoles : role][index];
                    if (role != None && l == 0) {
                        const unsigned v = Value(image.colour(source, index));
                        if (v < low[role]) low[role] = v;
                        if (v > high[role]) high[role] = v;
                        ++alphaCounts[role == LightHead ? 0 : 1][index];
                    }
                }
        }
        for (unsigned r = 0; r < LightRoles; ++r) {
            value[r][0] = uint8_t(low[r] > high[r] ? 0 : low[r]);
            value[r][1] = uint8_t(high[r]);
        }
        for (unsigned i = 0; i < 256; ++i) {
            unsigned best = 0;
            for (unsigned r = 1; r < LightRoles; ++r) if (counts[r][i] > counts[best][i]) best = r;
            role[i] = uint8_t(best);
        }
        for (unsigned side = 0; side < 2; ++side) {
            unsigned best = 0;
            for (unsigned i = 1; i < image.colours; ++i) if (alphaCounts[side][i] > alphaCounts[side][best]) best = i;
            alpha[side] = image.colour(source, best)[3];
        }
        bool unusable = false;
        for (unsigned l = 0; l < image.count; ++l) unusable |= !image.levels[l].usable;
        for (unsigned i = 0; i < 256; ++i) remap[i] = uint8_t(i);
        merged = false;
        // Per variant: own entries per family, free entries, families whose texels must move.
        auto classify = [&](unsigned v, unsigned* own, unsigned* freeCount, bool* needs) {
            own[0] = own[1] = 0; needs[0] = needs[1] = false; *freeCount = 0;
            for (unsigned i = 0; i < image.colours; ++i) {
                unsigned families;
                const unsigned unlit = Uses(counts, v, i, &families);
                if (!unlit && (families == 1 || families == 2)) ++own[families - 1];
                else if (!unlit) ++*freeCount;
                if (families && (unlit || families == 3)) {
                    if (families & 1) needs[0] = true;
                    if (families & 2) needs[1] = true;
                }
            }
        };
        auto shortage = [&](unsigned v) {
            unsigned own[2], freeCount; bool needs[2];
            classify(v, own, &freeCount, needs);
            const unsigned required = unsigned(needs[0] && !own[0]) + unsigned(needs[1] && !own[1]);
            return int(required) - int(freeCount);
        };
        // 16-colour textures can run out of entries: merge the closest pair of
        // body colours, the same way in every variant so the body never flickers.
        // A lit family looks best with at least three tone steps; merge nearly equal
        // body colours for them too, but only when the change is hard to see.
        auto thin = [&](unsigned v) {
            unsigned own[2], freeCount; bool needs[2];
            classify(v, own, &freeCount, needs);
            unsigned lacking = 0;
            for (unsigned f = 0; f < 2; ++f) if (needs[f] && own[f] < 3) lacking += 3 - own[f];
            return lacking > freeCount;
        };
        for (unsigned merges = 0; merges < 6; ++merges) {
            int worst = 0;
            bool more = false;
            for (unsigned v = 1; v < 8; ++v) { const int s = shortage(v); if (s > worst) worst = s; more |= thin(v); }
            if (worst <= 0 && !more) break;
            if (unusable) { if (worst > 0) return false; break; }
            unsigned bestA = 0, bestB = 0, bestDistance = ~0u, totals[256];
            for (unsigned i = 0; i < image.colours; ++i) { totals[i] = 0; for (unsigned c = 0; c <= LightRoles; ++c) totals[i] += counts[c][i]; }
            for (unsigned a = 0; a < image.colours; ++a) {
                if (!totals[a]) continue;
                const uint8_t* ca = image.colour(source, a);
                for (unsigned b = a + 1; b < image.colours; ++b) {
                    if (!totals[b]) continue;
                    const uint8_t* cb = image.colour(source, b);
                    if (ca[3] != cb[3]) continue;
                    const int dr = ca[0] - cb[0], dg = ca[1] - cb[1], db = ca[2] - cb[2];
                    const int dc = (ca[0] - ca[1]) - (cb[0] - cb[1]);   // keep hues apart
                    const unsigned distance = unsigned(2 * dr * dr + 4 * dg * dg + db * db + 3 * dc * dc);
                    if (distance < bestDistance) { bestDistance = distance; bestA = a; bestB = b; }
                }
            }
            if (bestDistance == ~0u) { if (worst > 0) return false; break; }
            if (worst <= 0 && bestDistance > 3000) break;
            unsigned from = bestA, to = bestB;
            if (totals[from] > totals[to]) { from = bestB; to = bestA; }
            for (unsigned c = 0; c <= LightRoles; ++c) { counts[c][to] += counts[c][from]; counts[c][from] = 0; }
            for (unsigned i = 0; i < 256; ++i) if (remap[i] == from) remap[i] = uint8_t(to);
            merged = true;
        }
        const unsigned limit = image.colours == 16 ? 3 : MaxRamp;
        lit = 0;
        for (unsigned v = 0; v < 8; ++v) {
            std::memset(kind[v], Keep, sizeof(kind[v]));
            unsigned own[2], freeCount; bool needs[2];
            classify(v, own, &freeCount, needs);
            if (shortage(v) > 0) continue;
            // Families without own entries need a ramp; with few own entries a short ramp adds tone steps.
            unsigned want[2] = {0, 0}, spare = freeCount;
            for (unsigned f = 0; f < 2; ++f) if (needs[f] && !own[f]) { want[f] = 1; --spare; }
            for (bool added = true; spare && added;) {
                added = false;
                for (unsigned k = 0; k < 2 && spare; ++k) {   // red lamps first
                    const unsigned f = 1 - k;
                    if (needs[f] && own[f] + want[f] < limit) { ++want[f]; --spare; added = true; }
                }
            }
            bool any = false;
            for (unsigned i = 0; i < image.colours; ++i) {
                unsigned families;
                const unsigned unlit = Uses(counts, v, i, &families);
                if (families) any = true;
                if (!unlit && (families == 1 || families == 2)) kind[v][i] = uint8_t(families == 1 ? Own0 : Own1);
                else if (!unlit)
                    for (unsigned f = 0; f < 2; ++f)
                        if (want[f]) { kind[v][i] = uint8_t(f ? Slot1 : Slot0); --want[f]; break; }
            }
            // Spread the ramp entries over the bright range.
            for (unsigned f = 0; f < 2; ++f) {
                unsigned n = 0, k = 0;
                for (unsigned i = 0; i < image.colours; ++i) n += kind[v][i] == (f ? Slot1 : Slot0);
                for (unsigned i = 0; i < image.colours; ++i)
                    if (kind[v][i] == (f ? Slot1 : Slot0)) { shade[v][i] = uint8_t(n == 1 ? 224 : 120 + k * 135 / (n - 1)); ++k; }
            }
            if (any) lit |= uint8_t(1u << v);
        }
        return true;
    }

    // Writes variant `variant` of `source` into `target` (image.bytes long).
    void generate(const uint8_t* source, uint8_t* target, const LightImage& image, const LightRect* rects,
                  const LightSet& set, unsigned variant) const {
        using namespace lights;
        std::memcpy(target, source, image.bytes);
        uint32_t at[8];
        if (!BuildRoles(image, rects, set, at)) return;
        if (merged)
            for (unsigned l = 0; l < image.count; ++l) {
                const auto& level = image.levels[l];
                for (unsigned y = 0; y < level.height; ++y)
                    for (unsigned x = 0; x < level.width; ++x) {
                        const unsigned index = image.get(source, level, x, y);
                        if (remap[index] != index) image.set(target, level, x, y, remap[index]);
                    }
            }
        if (!(lit & (1u << variant))) return;
        const uint8_t* kinds = kind[variant];
        // Tone of each family in this variant; headlights use the front brightness range.
        Tone tones[2]{};
        unsigned sides[2] = {1, 1};
        for (unsigned r = 0; r < LightRoles; ++r) {
            const Tone t = ToneOf(r, variant);
            if (t == ToneNone) continue;
            tones[Family(t)] = t;
            if (r == LightHead) sides[Family(t)] = 0;
        }
        // Recolour, and build brightness -> entry lookups for texels that have to move.
        uint8_t nearest[2][256];
        unsigned spacing[2] = {0, 0};
        for (unsigned f = 0; f < 2; ++f) {
            if (tones[f] == ToneNone) continue;
            const auto& ramp = ramps[tones[f]];
            uint16_t targets[256];
            unsigned count = 0;
            for (unsigned i = 0; i < image.colours; ++i) {
                const uint8_t k = kinds[i];
                if (k != (f ? Own1 : Own0) && k != (f ? Slot1 : Slot0)) continue;
                const unsigned s = k == (f ? Own1 : Own0)
                    ? Shade(Value(image.colour(source, i)), value[role[i]][0], value[role[i]][1]) : shade[variant][i];
                uint8_t* c = image.colour(target, i);
                for (unsigned n = 0; n < 3; ++n) c[n] = uint8_t((ramp[0][n] * (255 - s) + ramp[1][n] * s) / 255);
                c[3] = alpha[sides[f]];
                targets[count++] = uint16_t(i | (s << 8));
            }
            if (!count) { tones[f] = ToneNone; continue; }
            spacing[f] = 160 / count;
            for (unsigned s = 0; s < 256; ++s) {
                unsigned best = 0, distance = ~0u;
                for (unsigned t = 0; t < count; ++t) {
                    const int d = int(targets[t] >> 8) - int(s);
                    const unsigned a = unsigned(d < 0 ? -d : d);
                    if (a < distance) { distance = a; best = targets[t] & 0xFF; }
                }
                nearest[f][s] = uint8_t(best);
            }
        }
        // Lamp texels on shared entries move to the closest lit entry; texels at
        // the edge of a lamp are placed a little darker for a soft falloff.
        for (unsigned l = 0; l < image.count; ++l) {
            const auto& level = image.levels[l];
            if (!level.usable) continue;
            const uint8_t* map = scratch.roles + at[l];
            for (unsigned y = 0; y < level.height; ++y)
                for (unsigned x = 0; x < level.width; ++x) {
                    const unsigned lamp = map[y * level.width + x];
                    if (lamp == None) continue;
                    const Tone tone = ToneOf(lamp, variant);
                    if (tone == ToneNone) continue;
                    const unsigned f = Family(tone);
                    if (tones[f] == ToneNone) continue;
                    const unsigned original = image.get(source, level, x, y), index = remap[original];
                    if (kinds[index] == (f ? Own1 : Own0)) continue;
                    unsigned s = Shade(Value(image.colour(source, original)), value[lamp][0], value[lamp][1]);
                    const auto same = [&](unsigned nx, unsigned ny) {
                        const unsigned other = map[ny * level.width + nx];
                        if (other == None) return false;
                        const Tone o = ToneOf(other, variant);
                        return o != ToneNone && Family(o) == f;
                    };
                    if (level.width >= 8 && level.height >= 8 &&
                        ((x && !same(x - 1, y)) || (x + 1 < level.width && !same(x + 1, y)) ||
                         (y && !same(x, y - 1)) || (y + 1 < level.height && !same(x, y + 1)))) s = s * 3 / 4;
                    // Ordered dither between the few lit entries keeps the lamp's grain.
                    static constexpr uint8_t bayer[4][4] = {{0, 8, 2, 10}, {12, 4, 14, 6}, {3, 11, 1, 9}, {15, 7, 13, 5}};
                    const int jittered = int(s) + (int(bayer[y & 3][x & 3]) - 8) * int(spacing[f]) / 24;
                    s = unsigned(jittered < 0 ? 0 : jittered > 255 ? 255 : jittered);
                    image.set(target, level, x, y, nearest[f][s]);
                }
        }
    }
};

// Hand-painted variants converted offline by tools/vehicle-lights/pack.py into
// the texture's own size, depth and palette size. Block layout (little endian):
// u8 widthLog2, heightLog2, depth, levels; u16 colours, variants; u32 texels;
// base indices (levels concatenated, row major, 4-bit low nibble first);
// palette (colours x RGBA, native alpha); then for variants 1..7: u16 changed
// palette entries {u8 index, RGBA}, u32 run bytes, runs {u16 skip, u16 count,
// u8 index[count]}. A pack is "IVLP", u32 count, u32 offset[count], blocks.
struct LightPack {
    // Texel indices of one state while it is written (up to 256x256 with all mips).
    struct Indices { uint8_t data[90112]; };
    static Indices& Index() { static Indices indices; return indices; }
    template<class T> static T Read(const uint8_t* p) { T v; std::memcpy(&v, p, sizeof(v)); return v; }
    // The art covers this raster: same size, depth, palette size and enough levels.
    static bool Fits(const uint8_t* b, uint32_t size, const LightImage& image) {
        if (!b || size < 12) return false;
        const unsigned w = 1u << b[0], h = 1u << b[1], depth = b[2], levels = b[3];
        const unsigned colours = Read<uint16_t>(b + 4), variants = Read<uint16_t>(b + 6);
        const uint32_t texels = Read<uint32_t>(b + 8);
        return image.count && w == image.levels[0].width && h == image.levels[0].height && depth == image.depth &&
               colours == image.colours && variants == 8 && levels >= image.count && texels <= sizeof(Indices::data) &&
               12 + (depth == 4 ? (texels + 1) / 2 : texels) + colours * 4 <= size;
    }
    // Writes `variant` into target: the original raster bytes with every level the art covers replaced.
    static bool Decode(const uint8_t* b, uint32_t size, unsigned variant, const uint8_t* source, uint8_t* target,
                       const LightImage& image) {
        const unsigned w = 1u << b[0], h = 1u << b[1], depth = b[2], levels = b[3];
        const unsigned colours = Read<uint16_t>(b + 4);
        const uint32_t texels = Read<uint32_t>(b + 8);
        uint8_t* index = Index().data;
        const uint8_t* p = b + 12;
        const uint8_t* end = b + size;
        for (uint32_t t = 0; t < texels; ++t) index[t] = depth == 4 ? (t & 1 ? p[t >> 1] >> 4 : p[t >> 1] & 15) : p[t];
        p += depth == 4 ? (texels + 1) / 2 : texels;
        if (source) std::memcpy(target, source, image.bytes);
        else std::memset(target, 0, image.bytes);
        const uint8_t* palette = p;
        p += colours * 4;
        for (unsigned i = 0; i < colours; ++i) std::memcpy(image.colour(target, i), palette + i * 4, 4);
        for (unsigned v = 1; v < 8; ++v) {
            if (p + 2 > end) return false;
            const unsigned changes = Read<uint16_t>(p);
            if (p + 2 + changes * 5 + 4 > end) return false;
            if (v == variant)
                for (unsigned c = 0; c < changes; ++c) {
                    const uint8_t* change = p + 2 + c * 5;
                    if (change[0] < colours) std::memcpy(image.colour(target, change[0]), change + 1, 4);
                }
            p += 2 + changes * 5;
            const uint32_t bytes = Read<uint32_t>(p);
            p += 4;
            if (p + bytes > end) return false;
            if (v == variant) {
                uint32_t at = 0;
                for (const uint8_t* run = p; run + 4 <= p + bytes;) {
                    at += Read<uint16_t>(run);
                    const unsigned count = Read<uint16_t>(run + 2);
                    if (run + 4 + count > p + bytes || at + count > texels) return false;
                    std::memcpy(index + at, run + 4, count);
                    at += count;
                    run += 4 + count;
                }
            }
            p += bytes;
        }
        uint32_t at = 0;
        for (unsigned l = 0; l < levels && l < image.count; ++l) {
            const unsigned lw = w >> l ? w >> l : 1, lh = h >> l ? h >> l : 1;
            const auto& level = image.levels[l];
            if (level.usable && level.width == lw && level.height == lh)
                for (unsigned y = 0; y < lh; ++y)
                    for (unsigned x = 0; x < lw; ++x) image.set(target, level, x, y, index[at + y * lw + x]);
            at += lw * lh;
        }
        return true;
    }
    // LZ4 block decoder (tools/vehicle-lights/lz.py); false on malformed input.
    static bool Unpack(const uint8_t* src, uint32_t size, uint8_t* dst, uint32_t raw) {
        const uint8_t* end = src + size;
        uint32_t out = 0;
        while (src < end) {
            const unsigned token = *src++;
            uint32_t literals = token >> 4;
            if (literals == 15)
                for (unsigned b = 255; b == 255 && src < end;) { b = *src++; literals += b; }
            if (literals > uint32_t(end - src) || literals > raw - out) return false;
            std::memcpy(dst + out, src, literals);
            src += literals; out += literals;
            if (src >= end) break;
            if (end - src < 2) return false;
            const uint32_t offset = uint32_t(src[0]) | uint32_t(src[1]) << 8;
            src += 2;
            uint32_t match = (token & 15) + 4;
            if ((token & 15) == 15)
                for (unsigned b = 255; b == 255 && src < end;) { b = *src++; match += b; }
            if (!offset || offset > out || match > raw - out) return false;
            for (uint32_t i = 0; i < match; ++i, ++out) dst[out] = dst[out - offset];
        }
        return out == raw;
    }
    // Stored (compressed) block of `cache` in a pack held in memory: "IVL2", u32
    // count, u32 offset[count]; each block is u32 raw size, u32 stored size, LZ4 data.
    static const uint8_t* Find(const uint8_t* pack, uint32_t packSize, unsigned cache, uint32_t* raw, uint32_t* stored) {
        if (!pack || packSize < 8 || std::memcmp(pack, "IVL2", 4)) return nullptr;
        const uint32_t count = Read<uint32_t>(pack + 4);
        if (cache >= count || 8 + count * 4 > packSize) return nullptr;
        const uint32_t offset = Read<uint32_t>(pack + 8 + cache * 4);
        if (!offset || offset + 8 > packSize) return nullptr;
        *raw = Read<uint32_t>(pack + offset);
        *stored = Read<uint32_t>(pack + offset + 4);
        if (*stored > packSize - offset - 8) return nullptr;
        return pack + offset + 8;
    }
    // Shared buffer for one unpacked block at a time (rendering is single threaded).
    struct Buffer { alignas(16) uint8_t data[160 * 1024]; };
    static Buffer& Unpacked() { static Buffer buffer; return buffer; }
    // Unpacks `cache` from an embedded pack.
    static const uint8_t* Load(const uint8_t* pack, uint32_t packSize, unsigned cache, uint32_t* size) {
        uint32_t raw = 0, stored = 0;
        const uint8_t* block = Find(pack, packSize, cache, &raw, &stored);
        if (!block || raw > sizeof(Buffer::data) || !Unpack(block, stored, Unpacked().data, raw)) return nullptr;
        *size = raw;
        return Unpacked().data;
    }
};

// Pool of generated variants. Each slot holds the eight variants of one car
// texture; slots are reused least-recently-used once they have not been drawn
// for Traits::Recent ticks (the GPU may still read a texture used this frame).
// Traits supplies Texture/Raster types and the raster description. All state
// starts zeroed (plugin bss, no constructors).
template<class Traits, unsigned SlotCount, unsigned Bytes, unsigned CacheCount> struct LightTextures {
    using Texture = typename Traits::Texture;
    using Raster = typename Traits::Raster;
    struct Slot {
        Texture* source;
        uint8_t* sourceData;
        uint32_t key, used;
        uint16_t owner;      // cache id + 1, 0 = free
        uint8_t ready;
        bool valid, packed;  // packed: all eight variants copied from the converted art
        LightImage image;
        LightPlan plan;
        Texture textures[8];
        Raster rasters[8];
        alignas(64) uint8_t pixels[8][Bytes];
    };
    static_assert(SlotCount < 255 && CacheCount < 65535);
    Slot slots[SlotCount];
    uint8_t map[CacheCount];  // slot index + 1, 0 = none
    // Converted art for a cache id (LightPack block), or nullptr for generated variants.
    const uint8_t* (*pack)(unsigned cache, uint32_t* size);

    // Returns the texture to draw for `variant`, or nullptr to keep the original.
    Texture* get(unsigned cache, Texture* original, unsigned variant, const LightRect* rects, const LightSet& set) {
        if (cache >= CacheCount || variant > 7 || !original || !original->raster) return nullptr;
        Raster* raster = original->raster;
        uint8_t* data = Traits::Data(*raster);
        if (!data) return nullptr;
        const uint32_t key = Traits::Key(*raster), now = Traits::Now();
        Slot* slot = map[cache] ? &slots[map[cache] - 1] : nullptr;
        if (slot && (slot->owner != cache + 1 || slot->source != original || slot->sourceData != data || slot->key != key)) {
            // Re-streamed: regenerate in place unless the old variants may still be in flight
            // or a cached display list pins another layout.
            if (slot->owner != cache + 1 || uint32_t(now - slot->used) < Traits::Recent ||
                (slot->key != key && Traits::Bound(slot->rasters, 8))) slot = nullptr;
            else prepare(*slot, cache, original, raster, data, key, rects, set);
        }
        if (!slot)
            // Several models can share a texture name with their own copies: reuse a slot already prepared for this one.
            for (auto& s : slots)
                if (s.owner == cache + 1 && s.source == original && s.sourceData == data && s.key == key) { slot = &s; map[cache] = uint8_t(&s - slots + 1); break; }
        if (!slot) {
            slot = acquire(key, now);
            if (!slot) return nullptr;
            prepare(*slot, cache, original, raster, data, key, rects, set);
        }
        slot->used = now;
        if (!slot->valid) return nullptr;
        if (slot->packed) return &slot->textures[variant];
        if (!(slot->plan.lit & (1u << variant))) {
            if (!slot->plan.merged) return nullptr;
            variant = 0;
        }
        if (!(slot->ready & (1u << variant))) {
            slot->plan.generate(data, slot->pixels[variant], slot->image, rects, set, variant);
            Traits::Flush(slot->pixels[variant], slot->image.bytes);
            slot->ready |= uint8_t(1u << variant);
        }
        return &slot->textures[variant];
    }

private:
    Slot* acquire(uint32_t key, uint32_t now) {
        Slot* best = nullptr;
        for (auto& s : slots) {
            if (!s.owner) return &s;
            if (uint32_t(now - s.used) < Traits::Recent) continue;
            if (s.key != key && Traits::Bound(s.rasters, 8)) continue;
            if (!best || uint32_t(now - s.used) > uint32_t(now - best->used)) best = &s;
        }
        return best;
    }
    void prepare(Slot& s, unsigned cache, Texture* original, Raster* raster, uint8_t* data, uint32_t key,
                 const LightRect* rects, const LightSet& set) {
        const uint8_t index = uint8_t(&s - slots + 1);
        if (s.owner && s.owner <= CacheCount && map[s.owner - 1] == index) map[s.owner - 1] = 0;
        s.owner = uint16_t(cache + 1); s.source = original; s.sourceData = data; s.key = key; s.ready = 0; s.packed = false;
        map[cache] = index;
        s.valid = Traits::Describe(*raster, s.image) && s.image.bytes <= Bytes;
        if (!s.valid) return;
        // Hand-painted art first; generated lighting where there is none or it does not fit.
        uint32_t size = 0;
        const uint8_t* art = pack ? pack(cache, &size) : nullptr;
        if (art && LightPack::Fits(art, size, s.image)) {
            s.packed = true;
            for (unsigned v = 0; v < 8 && s.packed; ++v) {
                s.packed = LightPack::Decode(art, size, v, data, s.pixels[v], s.image);
                Traits::Flush(s.pixels[v], s.image.bytes);
            }
            if (s.packed) s.ready = 0xFF;
        }
        if (!s.packed) s.valid = set.count && s.plan.build(data, s.image, rects, set);
        if (!s.valid) return;
        for (unsigned i = 0; i < 8; ++i) {
            Traits::Copy(s.rasters[i], *raster, s.pixels[i]);
            s.textures[i] = *original;
            s.textures[i].raster = &s.rasters[i];
            s.textures[i].dictionary = nullptr;
            s.textures[i].links[0] = s.textures[i].links[1] = nullptr;
        }
    }
};

// Full-resolution 8-bit variants from the converted art. The plugin's rasters
// take the art's size and depth (Traits::Shape), so each slot holds eight
// rasters of up to Bytes. Used first; cars beyond the pool, or without HD art,
// keep the native-size LightTextures. One new texture is prepared per frame at
// most (it decodes eight rasters), the others wait a frame on native art.
template<class Traits, unsigned SlotCount, unsigned Bytes, unsigned CacheCount> struct HdLightTextures {
    using Texture = typename Traits::Texture;
    using Raster = typename Traits::Raster;
    struct Slot {
        Texture* source;
        uint8_t* sourceData;
        uint32_t key, shape, used;
        uint16_t owner;      // cache id + 1, 0 = free
        Texture textures[8];
        Raster rasters[8];
        alignas(64) uint8_t pixels[8][Bytes];
    };
    static_assert(SlotCount < 255 && CacheCount < 65535);
    Slot slots[SlotCount];
    uint8_t map[CacheCount];      // slot index + 1
    uint8_t missing[CacheCount];  // 1 = no HD art for this cache id (or it does not fit)
    uint32_t prepared;            // Now() of the last preparation
    bool primed;
    const uint8_t* (*pack)(unsigned cache, uint32_t* size);

    Texture* get(unsigned cache, Texture* original, unsigned variant) {
        if (!pack || cache >= CacheCount || variant > 7 || missing[cache] || !original || !original->raster) return nullptr;
        Raster* raster = original->raster;
        uint8_t* data = Traits::Data(*raster);
        if (!data) return nullptr;
        const uint32_t key = Traits::Key(*raster), now = Traits::Now();
        Slot* slot = map[cache] ? &slots[map[cache] - 1] : nullptr;
        if (slot && slot->owner == cache + 1 && slot->source == original && slot->sourceData == data && slot->key == key) {
            slot->used = now;
            return &slot->textures[variant];
        }
        for (auto& s : slots)
            if (s.owner == cache + 1 && s.source == original && s.sourceData == data && s.key == key) {
                map[cache] = uint8_t(&s - slots + 1); s.used = now;
                return &s.textures[variant];
            }
        // A new texture: only when a slot can take it and nothing was prepared this frame.
        if (primed && uint32_t(now - prepared) < Traits::Recent / 6) return nullptr;
        if (!available(now)) return nullptr;
        uint32_t size = 0;
        const uint8_t* art = pack(cache, &size);
        Raster shaped;
        LightImage image;
        if (!art || size < 12 || !Traits::Shape(shaped, *raster, 1u << art[0], 1u << art[1], art[3]) ||
            !Traits::Describe(shaped, image) || image.bytes > Bytes || !LightPack::Fits(art, size, image)) {
            missing[cache] = 1;
            return nullptr;
        }
        const uint32_t shape = Traits::Key(shaped);
        slot = acquire(shape, now);
        if (!slot) return nullptr;
        prepared = now; primed = true;
        const uint8_t index = uint8_t(slot - slots + 1);
        if (slot->owner && slot->owner <= CacheCount && map[slot->owner - 1] == index) map[slot->owner - 1] = 0;
        slot->owner = 0;
        for (unsigned v = 0; v < 8; ++v) {
            if (!LightPack::Decode(art, size, v, nullptr, slot->pixels[v], image)) { missing[cache] = 1; return nullptr; }
            Traits::Flush(slot->pixels[v], image.bytes);
            Traits::Copy(slot->rasters[v], shaped, slot->pixels[v]);
            slot->textures[v] = *original;
            slot->textures[v].raster = &slot->rasters[v];
            slot->textures[v].dictionary = nullptr;
            slot->textures[v].links[0] = slot->textures[v].links[1] = nullptr;
        }
        slot->owner = uint16_t(cache + 1); slot->source = original; slot->sourceData = data; slot->key = key;
        slot->shape = shape; slot->used = now;
        map[cache] = index;
        return &slot->textures[variant];
    }

private:
    bool available(uint32_t now) const {
        for (const auto& s : slots) if (!s.owner || uint32_t(now - s.used) >= Traits::Recent) return true;
        return false;
    }
    Slot* acquire(uint32_t shape, uint32_t now) {
        Slot* best = nullptr;
        for (auto& s : slots) {
            if (!s.owner && !Traits::Bound(s.rasters, 8)) return &s;
            if (s.owner && uint32_t(now - s.used) < Traits::Recent) continue;
            // A cached GE display list pins the slot's layout.
            if (s.shape != shape && Traits::Bound(s.rasters, 8)) continue;
            if (!best || uint32_t(now - s.used) > uint32_t(now - best->used)) best = &s;
        }
        return best;
    }
};
}
