#pragma once
#include "PSP.hpp"
#include "VehicleLights.hpp"
#include <cstddef>
namespace console::portable {
struct VehicleModel { const char* texture; int16_t cache; };
struct VehicleLayout { unsigned model, flags, accelerator, brake, damage; };
namespace vehicle {
struct Raster {
    void* displayList; uint8_t* data;
    int16_t stride; uint8_t width, height, depth, levels; uint16_t flags;
};
struct Texture { Raster* raster; void* dictionary; void* links[2]; char name[32], mask[32]; };
struct Material { Texture* texture; uint32_t color, references; void* effects; };
struct Geometry { uint8_t object[8]; int16_t references, padding; Material** materials; int32_t count, capacity; void* skin; };
struct Link { Link *next, *previous; };
struct Group { uint8_t object[8]; Link elements; };
struct Element { uint8_t object[20]; Geometry* geometry; Group* group; Link link; };
static_assert(sizeof(Raster) == 16 && sizeof(Texture) == 80 && sizeof(Geometry) == 28);
static_assert(offsetof(Element, geometry) == 20 && offsetof(Element, link) == 28);
template<class T> T field(const void* object, unsigned offset) {
    T result; std::memcpy(&result, static_cast<const uint8_t*>(object) + offset, sizeof(result)); return result;
}
// RslRaster on the PSP: 4/8-bit swizzled levels, each stride * height bytes
// (the stride halves down to 16 bytes), followed by the palette. The format
// byte holds the palette size (0x40: 16, 0x20: 256) and the CLUT format (5: RGBA8888).
struct LightTraits {
    using Texture = vehicle::Texture;
    using Raster = vehicle::Raster;
    static constexpr uint32_t Recent = 100000;   // microseconds
    static uint8_t* Data(Raster& r) { return r.data; }
    static uint32_t Key(const Raster& r) {
        return (uint32_t(uint16_t(r.stride)) & 0xFFF) | uint32_t(r.width & 15) << 12 | uint32_t(r.height & 15) << 16 |
               uint32_t(r.levels & 15) << 20 | uint32_t(r.depth == 8) << 24 | uint32_t((r.flags >> 8) & 0x7F) << 25;
    }
    static bool Describe(const Raster& r, LightImage& image) {
        const unsigned format = r.flags >> 8, depth = r.depth, levels = r.levels & 63;
        const unsigned width = r.width & 63, height = r.height & 63;
        if ((format & 0xF) != 5 || !(format & 0x60) || (depth != 4 && depth != 8) || !levels || levels > 8 ||
            width < 2 || width > 8 || height < 2 || height > 8 || r.stride < 16) return false;
        image = {};
        image.depth = depth; image.swizzled = true; image.colours = format & 0x40 ? 16 : 256; image.count = levels;
        unsigned offset = 0, stride = unsigned(r.stride), w = 1u << width, h = 1u << height;
        if (stride * 8 / depth < w || (stride & 15)) return false;
        for (unsigned l = 0; l < levels; ++l) {
            // Swizzled levels shorter than one 8-row block are only addressable with a 16-byte stride.
            image.levels[l] = {offset, uint16_t(w), uint16_t(h), uint16_t(stride), h >= 8 || stride == 16};
            offset += stride * h;
            if (stride >= 17) stride /= 2;
            w = w > 1 ? w / 2 : 1; h = h > 1 ? h / 2 : 1;
        }
        image.palette = offset;
        image.bytes = offset + image.colours * 4;
        return true;
    }
    static unsigned Log2(unsigned v) { unsigned l = 0; while ((2u << l) <= v) ++l; return l; }
    // HD raster: the art's size, 8-bit with a 256-entry RGBA8888 palette, `levels`
    // levels (stride halving down to 16 bytes, as Describe reads it).
    static bool Shape(Raster& out, const Raster& in, unsigned w, unsigned h, unsigned levels) {
        if (w < 8 || h < 8 || w > 256 || h > 256 || !levels || levels > 8) return false;
        out = in;
        out.displayList = nullptr; out.data = nullptr;
        out.width = uint8_t((in.width & ~63) | Log2(w));
        out.height = uint8_t((in.height & ~63) | Log2(h));
        out.depth = 8;
        out.stride = int16_t(w < 16 ? 16 : w);
        out.levels = uint8_t((in.levels & 0xC0) | levels);
        const unsigned format = ((in.flags >> 8) & 0x90) | 0x20 | 0x05;
        out.flags = uint16_t((in.flags & 0xFF) | format << 8);
        return true;
    }
    static void Copy(Raster& target, const Raster& source, uint8_t* data) {
        // A display list the game cached for this copy stays valid: same address and layout.
        void* list = target.displayList;
        target = source;
        target.displayList = list;
        target.data = data;
    }
    static bool Bound(const Raster* rasters, unsigned count) {
        for (unsigned i = 0; i < count; ++i) if (rasters[i].displayList) return true;
        return false;
    }
    static uint32_t Now() { return sceKernelGetSystemTimeLow(); }
    static void Flush(const void* data, unsigned bytes) { sceKernelDcacheWritebackRange(data, bytes); }
};
struct MaterialChanges {
    struct Change { Material* material; Texture* original; } changes[64];
    unsigned count = 0;
    bool add(Material* material, Texture* replacement) {
        for (unsigned i = 0; i < count; ++i) if (changes[i].material == material) return true;
        if (count == 64) return false;
        changes[count++] = {material, material->texture}; material->texture = replacement; return true;
    }
    void restore() { while (count) { const auto& c = changes[--count]; c.material->texture = c.original; } }
    ~MaterialChanges() { restore(); }
};
}
// lights.bin from tools/vehicle-lights/pack.py next to the plugin: the offset
// table is read at startup, a texture's block when that texture is first drawn,
// into one bounded buffer (the slot keeps the decoded variants).
struct LightPackFile {
    char path[128];
    uint32_t size, count, offsets[256];
    // One stored block at a time, shared by both pack files.
    struct Stored { alignas(16) uint8_t data[128 * 1024]; };
    static Stored& Buffer() { static Stored stored; return stored; }
    bool open(const char* file) {
        count = 0;
        std::strncpy(path, file, sizeof(path) - 1);
        const SceUID handle = sceIoOpen(path, PSP_O_RDONLY, 0777);
        if (handle < 0) return false;
        const SceOff end = sceIoLseek(handle, 0, PSP_SEEK_END);
        sceIoLseek(handle, 0, PSP_SEEK_SET);
        uint8_t header[8];
        bool ok = end > 8 && end < 0x10000000 && sceIoRead(handle, header, 8) == 8 && !std::memcmp(header, "IVL2", 4);
        uint32_t n = 0;
        if (ok) { std::memcpy(&n, header + 4, 4); ok = n <= 256 && sceIoRead(handle, offsets, n * 4) == int(n * 4); }
        sceIoClose(handle);
        if (!ok) return false;
        size = uint32_t(end); count = n;
        return true;
    }
    // Reads and unpacks one texture's block (u32 raw size, u32 stored size, LZ4 data).
    const uint8_t* load(unsigned cache, uint32_t* bytes) {
        if (cache >= count || !offsets[cache] || offsets[cache] + 8 > size) return nullptr;
        const SceUID handle = sceIoOpen(path, PSP_O_RDONLY, 0777);
        if (handle < 0) return nullptr;
        uint32_t lengths[2] = {0, 0};
        auto& buffer = Buffer().data;
        bool ok = sceIoLseek(handle, offsets[cache], PSP_SEEK_SET) == SceOff(offsets[cache]) &&
                  sceIoRead(handle, lengths, 8) == 8 && lengths[1] <= sizeof(buffer) && lengths[1] <= size - offsets[cache] - 8 &&
                  sceIoRead(handle, buffer, lengths[1]) == int(lengths[1]);
        sceIoClose(handle);
        auto& unpacked = LightPack::Unpacked().data;
        if (!ok || lengths[0] > sizeof(unpacked) || !LightPack::Unpack(buffer, lengths[1], unpacked, lengths[0])) return nullptr;
        *bytes = lengths[0];
        return unpacked;
    }
};
// Lit variants come from the converted hand-painted art where it exists and
// are generated from the game's own raster otherwise, the first time a car
// texture is drawn (or after it was streamed in again). They are only swapped
// into the car's materials for the duration of its draw.
//
// CRenderer::RenderOneNonRoad draws a vehicle in two steps: CVehicle::Render
// draws the opaque elements and queues those with alpha (body, doors, glass)
// in a sorted list that is drawn right after Render returns. render() records
// the car it lit; sorted() lights it again around that list draw, but only
// when it is the entity RenderOneNonRoad is drawing (`drawn`, set at entry).
// HD slots: 256x256 8-bit, eight levels and the palette.
template<unsigned CacheCount, unsigned Slots, unsigned TextureBytes, unsigned HdSlots = 8, unsigned HdBytes = 88576>
struct VehicleLights {
    LightTextures<vehicle::LightTraits, Slots, TextureBytes, CacheCount> textures;
    HdLightTextures<vehicle::LightTraits, HdSlots, HdBytes, CacheCount> hd;
    const void* current = nullptr;   // last car given lit textures by render()
    const void* drawn = nullptr;     // entity passed to CRenderer::RenderOneNonRoad
    struct Profile {
        const VehicleModel* models; unsigned count; int first;
        const LightRect* rects; const LightSet* sets;
        VehicleLayout layout; bool buggyFreeway;
    };
    bool apply(void* car, const Profile& p, vehicle::MaterialChanges& changed) {
        using namespace vehicle;
        int index = field<int16_t>(car, p.layout.model) - p.first;
        if (unsigned(index) >= p.count || p.models[index].cache < 0) return false;
        auto group = field<Group*>(car, 0x50);
        if (!group || group->object[0] == 1) return false;
        const auto flags = field<uint8_t>(car, p.layout.flags);
        const VehicleLightState state{(flags & 0x10) != 0, (flags & 0x40) != 0, (flags & 0x20) != 0,
            field<float>(car, p.layout.accelerator), field<float>(car, p.layout.brake), field<uint32_t>(car, p.layout.damage)};
        const unsigned variant = state.texture();
        Texture* sources[CacheCount]{};
        unsigned elements = 0;
        for (Link* link = group->elements.next; link && link != &group->elements; link = link->next) {
            if (++elements > 256) { changed.restore(); return false; }
            auto geometry = reinterpret_cast<Element*>(reinterpret_cast<uint8_t*>(link) - offsetof(Element, link))->geometry;
            if (!geometry || !geometry->materials || geometry->count < 0 || geometry->count > 64) continue;
            for (int i = 0; i < geometry->count; ++i) {
                auto material = geometry->materials[i];
                if (!material || !material->texture) continue;
                int textureIndex = index;
                // The buggy has a freeway material as well as its own body.
                if (p.buggyFreeway && p.first + index == 155 && !std::strncmp(material->texture->name, "freeway8bit128", 32)) textureIndex = 207 - p.first;
                const auto& model = p.models[textureIndex];
                if (std::strncmp(material->texture->name, model.texture, 32)) continue;
                // One model's matching materials share one original raster; leave any other alone.
                auto& source = sources[model.cache];
                if (source && source != material->texture) continue;
                source = material->texture;
                auto lit = hd.get(unsigned(model.cache), material->texture, variant);
                if (!lit) lit = textures.get(unsigned(model.cache), material->texture, variant, p.rects, p.sets[model.cache]);
                if (lit)
                    if (!changed.add(material, lit)) { changed.restore(); return false; }
            }
        }
        return changed.count != 0;
    }
    template<class Render> void render(void* car, const Profile& p, Render original) {
        vehicle::MaterialChanges changed;
        current = apply(car, p, changed) ? car : nullptr;
        original();
    }
    // Boats, helis and other Render paths never reach the list draw with
    // `drawn` set to them; a car left over from one of those may have been
    // deleted since, so `current` is only used when it is the entity drawn now.
    template<class Render> void sorted(const Profile& p, Render original) {
        void* car = const_cast<void*>(current);
        current = nullptr;
        if (!car || car != drawn) { original(); return; }
        vehicle::MaterialChanges changed;
        apply(car, p, changed);
        original();
    }
    // Single-step form for callers without the list hook.
    template<class Render> void render(void* car, const VehicleModel* models, unsigned count, int first,
                                      const LightRect* rects, const LightSet* sets,
                                      VehicleLayout layout, bool buggyFreeway, Render original) {
        render(car, Profile{models, count, first, rects, sets, layout, buggyFreeway}, original);
    }
};
}
