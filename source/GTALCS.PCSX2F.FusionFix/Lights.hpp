#pragma once
#include "../Shared/Console/VehicleLights.hpp"
#include <cstddef>
#include <cstring>

// LCS PS2 render structures. They follow Shared/Console/Ps2VehicleLights.hpp
// (VCS PS2) except for the raster, which is only a data pointer and the flags.
namespace imvehlm {
struct Raster { uint8_t* data; uint32_t flags; };
struct Texture { Raster* raster; void* dictionary; void* links[2]; char name[32], mask[32]; };
struct Material { Texture* texture; uint32_t color, references; void* effects; };
struct Geometry { uint8_t object[8]; int16_t references, padding; Material** materials; int32_t count, capacity; void* skin; };
struct Link { Link *next, *previous; };
struct Group { uint8_t object[8]; Link elements; };
struct Element { uint8_t object[20]; Geometry* geometry; Group* group; Link link; };
static_assert(sizeof(Texture) == 80 && sizeof(Raster) == 8);
static_assert(sizeof(Material) == 16 && sizeof(Geometry) == 28);
static_assert(offsetof(Element, geometry) == 20 && offsetof(Element, link) == 28);
struct Model { const char* texture; int16_t cache; };

template<class T> T field(const void* object, size_t offset) {
    T value;
    std::memcpy(&value, static_cast<const uint8_t*>(object) + offset, sizeof(T));
    return value;
}

// RslRaster flags: width/height log2 in bits 0-11, depth in 12-17, mip count in
// 20-23 and one bit per level from 24 for levels stored for a wider upload.
// Levels are w*h*depth/8 bytes; the RGBA palette (alpha 0x80 = opaque, CSM1
// order for 256 entries) follows the last level. Flagged levels hold 8-bit
// texels in PSMCT32 order or 4-bit texels in PSMCT16 order (checked against
// runtime dumps); LightImage::locate addresses them.
struct LightTraits {
    using Texture = imvehlm::Texture;
    using Raster = imvehlm::Raster;
    static constexpr uint32_t Recent = 30000000;   // EE cycles, about 0.1 s
    static uint8_t* Data(Raster& r) { return r.data; }
    static uint32_t Key(const Raster& r) { return r.flags; }
    static bool Describe(const Raster& r, console::LightImage& image) {
        const unsigned width = r.flags & 63, height = (r.flags >> 6) & 63, depth = (r.flags >> 12) & 63;
        const unsigned levels = (r.flags >> 20) & 15, swizzled = r.flags >> 24;
        if (width < 3 || width > 8 || height < 3 || height > 8 || (depth != 4 && depth != 8) || !levels || levels > 8) return false;
        image = {};
        image.depth = depth; image.ps2Clut = true; image.colours = depth == 8 ? 256 : 16; image.count = levels;
        unsigned offset = 0, w = 1u << width, h = 1u << height;
        for (unsigned l = 0; l < levels; ++l) {
            const unsigned stride = w * depth / 8;
            // Flagged levels smaller than one GS block column are left as they are.
            const bool packed = (swizzled >> l) & 1;
            const bool fits = depth == 8 ? w >= 16 && h >= 16 : w >= 16 && h >= 8;
            image.levels[l] = {offset, uint16_t(w), uint16_t(h), uint16_t(stride), stride && (!packed || fits),
                               uint8_t(packed && fits ? depth : 0)};
            offset += stride * h;
            w = w > 1 ? w / 2 : 1; h = h > 1 ? h / 2 : 1;
        }
        image.palette = offset;
        image.bytes = offset + image.colours * 4;
        return true;
    }
    static unsigned Log2(unsigned v) { unsigned l = 0; while ((2u << l) <= v) ++l; return l; }
    // HD raster: the art's size, 8-bit (PSMT8, 256-entry CSM1 palette), linear levels.
    static bool Shape(Raster& out, const Raster& in, unsigned w, unsigned h, unsigned levels) {
        if (w < 8 || h < 8 || w > 256 || h > 256 || !levels || levels > 7) return false;
        out = in;
        out.data = nullptr;
        out.flags = Log2(w) | Log2(h) << 6 | 8u << 12 | (in.flags & 0x000C0000u) | uint32_t(levels) << 20;
        return true;
    }
    static void Copy(Raster& target, const Raster& source, uint8_t* data) { target = source; target.data = data; }
    static bool Bound(const Raster*, unsigned) { return false; }
    static uint32_t Now() { uint32_t count; asm volatile("mfc0 %0, $9" : "=r"(count)); return count; }
    // PCSX2 does not model the data cache for texture DMA.
    static void Flush(const void*, unsigned) {}
};

struct MaterialChanges {
    struct Change { Material* material; Texture* original; } changes[64];
    unsigned count = 0;
    bool add(Material* material, Texture* replacement) {
        for (unsigned i = 0; i < count; ++i) if (changes[i].material == material) return true;
        if (count == sizeof(changes) / sizeof(*changes)) return false;
        changes[count++] = {material, material->texture};
        material->texture = replacement;
        return true;
    }
    void restore() {
        while (count) {
            const auto& change = changes[--count];
            change.material->texture = change.original;
        }
    }
    ~MaterialChanges() { restore(); }
};
}
