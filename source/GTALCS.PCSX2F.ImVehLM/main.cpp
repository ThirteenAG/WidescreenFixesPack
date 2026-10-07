#include "Models.hpp"
#include "LightRects.hpp"
#include "LightPack.hpp"
#include "../../external/injector/include/ps2/runtime.hpp"
#include "../../external/injector/include/ps2/game_abi.hpp"
#include "../../external/injector/include/ps2/safetymips.hpp"
#include <cstdio>

extern "C" {
#include "../../external/injector/include/ps2/pcsx2f_api.h"
#include "../../external/injector/include/ps2/inireader.h"
int CompatibleCRCList[] = {static_cast<int>(0x7EA439F5)};
char PluginData[MaxIniSize] = {1};
char OSDText[OSDStringNum][OSDStringSize] = {{1}};
}

namespace imvehlm {
namespace {
// CVehicle on LCS PS2 matches the PSP layout: model index 0x58, flags byte 0x255
// (engine 0x10, handbrake 0x20, lights 0x40), gas/brake pedals 0x24C/0x250 and the
// damage manager at 0x350 with the lamp status word at +0x10.
struct Layout { unsigned model, flags, accelerator, brake, damage; };
constexpr Layout layout{0x58, 0x255, 0x24C, 0x250, 0x360};
// The BF Injection also draws a Freeway body material.
constexpr int BuggyModel = 155, FreewayModel = 207;

// Lit head/tail/brake/reverse variants come from the converted hand-painted
// art (PSP LCS art, the bodies match), or are generated from the game's own car
// texture where it does not fit, when first drawn, and swapped into the car's
// materials only while it is drawn. Bounded pool: 16 textures in use at once.
//
// CRenderer::RenderOneNonRoad draws a vehicle in two steps: CVehicle::Render
// draws the opaque elements and queues those with alpha (body, doors, glass)
// in a per-vehicle sorted list, which is drawn right after it returns. The
// swap is applied to both steps for the vehicle just rendered.
template<unsigned CacheCount, unsigned Slots, unsigned TextureBytes, unsigned HdSlots, unsigned HdBytes> struct VehicleLights {
    console::LightTextures<LightTraits, Slots, TextureBytes, CacheCount> textures;
    console::HdLightTextures<LightTraits, HdSlots, HdBytes, CacheCount> hd;
    void* current = nullptr;   // last car or bike given lit textures
    void* drawn = nullptr;     // entity passed to CRenderer::RenderOneNonRoad

    // Swaps lit textures into the vehicle's materials; false when it has none.
    bool apply(void* vehicle, MaterialChanges& changed) {
        constexpr unsigned count = sizeof(models) / sizeof(*models);
        const int index = field<int16_t>(vehicle, layout.model) - FirstModel;
        if (unsigned(index) >= count || models[index].cache < 0) return false;
        auto group = field<Group*>(vehicle, 0x50);
        if (!group || group->object[0] == 1) return false;   // a single element, not a group
        const auto flags = field<uint8_t>(vehicle, layout.flags);
        const console::VehicleLightState state{
            (flags & 0x10) != 0, (flags & 0x40) != 0, (flags & 0x20) != 0,
            field<float>(vehicle, layout.accelerator), field<float>(vehicle, layout.brake),
            field<uint32_t>(vehicle, layout.damage)
        };
        const unsigned variant = state.texture();
        Texture* sources[CacheCount]{};
        unsigned elements = 0;
        // The native group list links the element at +28. Geometry material arrays
        // are shared across vehicle instances; only borrow them for this draw.
        for (Link* link = group->elements.next; link && link != &group->elements; link = link->next) {
            if (++elements > 256) { changed.restore(); return false; }
            const auto geometry = reinterpret_cast<Element*>(reinterpret_cast<uint8_t*>(link) - offsetof(Element, link))->geometry;
            if (!geometry || !geometry->materials || geometry->count < 0 || geometry->count > 64) continue;
            for (int32_t i = 0; i < geometry->count; ++i) {
                auto material = geometry->materials[i];
                if (!material || !material->texture) continue;
                int textureIndex = index;
                if (FirstModel + index == BuggyModel && !std::strncmp(material->texture->name, models[FreewayModel - FirstModel].texture, 32))
                    textureIndex = FreewayModel - FirstModel;
                const auto& model = models[textureIndex];
                if (std::strncmp(material->texture->name, model.texture, 32)) continue;
                // One model's matching materials share one original raster; leave any other alone.
                auto& source = sources[model.cache];
                if (source && source != material->texture) continue;
                source = material->texture;
                auto lit = hd.get(unsigned(model.cache), material->texture, variant);
                if (!lit) lit = textures.get(unsigned(model.cache), material->texture, variant, vehicle::lightRects, vehicle::lightSets[model.cache]);
                if (lit)
                    if (!changed.add(material, lit)) { changed.restore(); return false; }
            }
        }
        return changed.count != 0;
    }
    template<class Render> uint64_t render(void* vehicle, Render original) {
        MaterialChanges changed;
        current = apply(vehicle, changed) ? vehicle : nullptr;
        return original();
    }
    template<class Render> uint64_t sorted(Render original) {
        void* vehicle = current;
        current = nullptr;
        // Only the list of the car or bike RenderOneNonRoad is drawing. Boats, helis
        // and other Render paths never set `current`; a car left over from one of
        // those may have been deleted since.
        if (!vehicle || vehicle != drawn) return original();
        MaterialChanges changed;
        apply(vehicle, changed);
        return original();
    }
};

// 128x128 8-bit body textures: five levels and a 256-colour palette, 22848 bytes.
// Full resolution (256x256 8-bit, five levels): 8 textures, 5.6 MB.
VehicleLights<CacheCount, 16, 22848, 8, 88320> lights;
// Converted hand-painted variants (tools/vehicle-lights/pack.py), embedded.
const uint8_t* Pack(unsigned cache, uint32_t* size) {
    return console::LightPack::Load(vehicle::lightPack(), vehicle::lightPackSize, cache, size);
}
const uint8_t* PackHd(unsigned cache, uint32_t* size) {
    return console::LightPack::Load(vehicle::lightPackHd(), vehicle::lightPackHdSize, cache, size);
}
injector::hook_back<uint64_t (*)(void*)> render;
injector::hook_back<uint64_t (*)()> renderSorted;
pcsx2::GameCallback<uint64_t(void*)> renderCallback;
pcsx2::GameCallback<uint64_t()> sortedCallback;
SafetyMipsMid renderOneNonRoad;

uint64_t Render(void* vehicle) {
    return lights.render(vehicle, [&] { return render.fun(vehicle); });
}
uint64_t RenderSorted() {
    return lights.sorted([] { return renderSorted.fun(); });
}

void Rejected(pcsx2_hook_status status) {
    std::snprintf(OSDText[0], OSDStringSize, "LCS ImVehLM disabled: patch validation failed (%u)", unsigned(status));
}
}
}

extern "C" void init() {
    using namespace imvehlm;
    if (injector::InitializeCheckedRuntime(Rejected) != PCSX2_HOOK_OK) return;
    inireader.SetIniPath(PluginData + sizeof(uint32_t), *reinterpret_cast<const uint32_t*>(PluginData));
    if (inireader.ReadInteger("MAIN", "VehicleLights", 1) == 0) return;
    lights.textures.pack = Pack;
    lights.hd.pack = PackHd;
    renderCallback.bind(Render);
    sortedCallback.bind(RenderSorted);
    // CEntity::Render calls from CAutomobile::Render and CBike::Render.
    render.fun = injector::MakeCALL(0x1371A0, renderCallback.address()).get();
    render.fun = injector::MakeCALL(0x1608C0, renderCallback.address()).get();
    // The vehicle's sorted alpha list, drawn by CRenderer::RenderOneNonRoad after CVehicle::Render.
    renderSorted.fun = injector::MakeCALL(0x2C9E1C, sortedCallback.address()).get();
    // CRenderer::RenderOneNonRoad entry (addiu sp / li, no branch targets): the entity it draws.
    renderOneNonRoad = safetymips::create_mid(0x2C9C98, [](SafetyMipsContext& regs) {
        lights.drawn = reinterpret_cast<void*>(uintptr_t(regs.a0));
    });
    injector::FlushCaches();
}
extern "C" int main() { return 0; }
