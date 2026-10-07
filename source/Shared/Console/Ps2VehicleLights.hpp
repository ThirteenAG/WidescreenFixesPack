#pragma once
#include "VehicleLights.hpp"
#include <cstddef>
#include <cstring>

// Stories games on the PS2: lit car textures generated from the game's own
// rasters (see VehicleLights.hpp). Shared by the VCS and LCS ImVehLM plugins.
namespace console::ps2lights {
struct Raster { uint32_t unknown[2]; uint8_t* data; uint32_t flags; };
struct Texture { Raster* raster; void* dictionary; void* links[2]; char name[32], mask[32]; };
struct Material { Texture* texture; uint32_t color, references; void* effects; };
struct Geometry { uint8_t object[8]; int16_t references, padding; Material** materials; int32_t count, capacity; void* skin; };
struct Link { Link *next, *previous; };
struct Group { uint8_t object[8]; Link elements; };
struct Element { uint8_t object[20]; Geometry* geometry; Group* group; Link link; };
static_assert(sizeof(Texture) == 80 && sizeof(Raster) == 16);
static_assert(sizeof(Material) == 16 && sizeof(Geometry) == 28);
static_assert(offsetof(Element, geometry) == 20 && offsetof(Element, link) == 28);
struct Model { const char* texture; int16_t cache; };
struct Layout { unsigned model, flags, accelerator, brake, damage; };

template<class T> T field(const void* object, size_t offset) {
    T value;
    std::memcpy(&value, static_cast<const uint8_t*>(object) + offset, sizeof(T));
    return value;
}

// RslRaster flags: width/height log2 in bits 0-11, depth in 12-17, mip count in
// 20-23 and one bit per level from 24 for levels stored pre-swizzled for a
// 32/16-bit upload. Levels are w*h*depth/8 bytes; the RGBA palette (alpha
// 0x80 = opaque, CSM1 order for 256 entries) follows the last level. Car
// textures are linear; swizzled levels are left as they are.
struct LightTraits {
    using Texture = ps2lights::Texture;
    using Raster = ps2lights::Raster;
    static constexpr uint32_t Recent = 30000000;   // EE cycles, about 0.1 s
    static uint8_t* Data(Raster& r) { return r.data; }
    static uint32_t Key(const Raster& r) { return r.flags; }
    static bool Describe(const Raster& r, LightImage& image) {
        const unsigned width = r.flags & 63, height = (r.flags >> 6) & 63, depth = (r.flags >> 12) & 63;
        const unsigned levels = (r.flags >> 20) & 15, swizzled = r.flags >> 24;
        if (width < 3 || width > 8 || height < 3 || height > 8 || (depth != 4 && depth != 8) || !levels || levels > 8) return false;
        image = {};
        image.depth = depth; image.ps2Clut = true; image.colours = depth == 8 ? 256 : 16; image.count = levels;
        unsigned offset = 0, w = 1u << width, h = 1u << height;
        for (unsigned l = 0; l < levels; ++l) {
            const unsigned stride = w * depth / 8;
            image.levels[l] = {offset, uint16_t(w), uint16_t(h), uint16_t(stride), stride && !((swizzled >> l) & 1)};
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

// Swaps the lit variant into the car's materials for the duration of its draw.
//
// A vehicle can be drawn in two steps: CVehicle::Render draws the opaque
// elements and queues those with alpha (body, doors, glass) in a sorted list
// that CRenderer::RenderOneNonRoad draws right after Render returns. The
// swap is applied to both steps: render() records the vehicle it lit, and
// sorted() lights it again around the list draw, but only when it is the
// entity RenderOneNonRoad is drawing (`drawn`, set at its entry).
// HD slots: 256x256 8-bit, one level (VCS) and the palette.
template<unsigned CacheCount, unsigned Slots, unsigned TextureBytes, unsigned HdSlots = 8, unsigned HdBytes = 66560>
struct VehicleLights {
    LightTextures<LightTraits, Slots, TextureBytes, CacheCount> textures;
    HdLightTextures<LightTraits, HdSlots, HdBytes, CacheCount> hd;
    const void* current = nullptr;   // last vehicle given lit textures by render()
    const void* drawn = nullptr;     // entity passed to CRenderer::RenderOneNonRoad
    struct Profile {
        const Model* models; unsigned count; int first;
        const LightRect* rects; const LightSet* sets; Layout layout;
    };

    // Swaps lit textures into the vehicle's materials; false when it has none.
    bool apply(void* vehicle, const Profile& profile, MaterialChanges& changed) {
        const unsigned index = unsigned(field<int16_t>(vehicle, profile.layout.model) - profile.first);
        if (index >= profile.count || profile.models[index].cache < 0) return false;
        const auto& model = profile.models[index];
        auto group = field<Group*>(vehicle, 0x50);
        if (!group || group->object[0] == 1) return false;   // a single element, not a group
        const auto flags = field<uint8_t>(vehicle, profile.layout.flags);
        const VehicleLightState state{
            (flags & 0x10) != 0, (flags & 0x40) != 0, (flags & 0x20) != 0,
            field<float>(vehicle, profile.layout.accelerator), field<float>(vehicle, profile.layout.brake),
            field<uint32_t>(vehicle, profile.layout.damage)
        };
        const unsigned variant = state.texture();
        Texture* source = nullptr;
        unsigned elements = 0;
        // The native group list links the element at +28. Geometry material arrays
        // are shared across vehicle instances; only borrow them for this draw.
        for (Link* link = group->elements.next; link && link != &group->elements; link = link->next) {
            if (++elements > 256) { changed.restore(); return false; }
            const auto element = reinterpret_cast<Element*>(reinterpret_cast<uint8_t*>(link) - offsetof(Element, link));
            const auto geometry = element->geometry;
            if (!geometry || !geometry->materials || geometry->count < 0 || geometry->count > 64) continue;
            for (int32_t i = 0; i < geometry->count; ++i) {
                auto material = geometry->materials[i];
                if (!material || !material->texture || std::strncmp(material->texture->name, model.texture, 32)) continue;
                // One model's matching materials share one original raster; leave any other alone.
                if (source && source != material->texture) continue;
                source = material->texture;
                auto lit = hd.get(unsigned(model.cache), material->texture, variant);
                if (!lit) lit = textures.get(unsigned(model.cache), material->texture, variant, profile.rects, profile.sets[model.cache]);
                if (lit)
                    if (!changed.add(material, lit)) { changed.restore(); return false; }
            }
        }
        return changed.count != 0;
    }
    template<class Render> uint64_t render(void* vehicle, const Profile& profile, Render original) {
        MaterialChanges changed;
        current = apply(vehicle, profile, changed) ? vehicle : nullptr;
        return original();
    }
    // The alpha list of the vehicle RenderOneNonRoad is drawing. Boats, helis
    // and other Render paths never set `current`; a vehicle left over from one
    // of those may have been deleted since, so it is only used when it is the
    // entity being drawn right now.
    template<class Render> uint64_t sorted(const Profile& profile, Render original) {
        void* vehicle = const_cast<void*>(current);
        current = nullptr;
        if (!vehicle || vehicle != drawn) return original();
        MaterialChanges changed;
        apply(vehicle, profile, changed);
        return original();
    }
};
}
