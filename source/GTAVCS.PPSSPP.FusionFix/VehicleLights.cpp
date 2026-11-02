#include "Models.hpp"
#include "LightRects.hpp"
#include "Game.hpp"
#include "Shared/Console/PatchNotice.hpp"
#include "../Shared/Console/PortableCfg.hpp"
namespace vehicle {
namespace {
using namespace console::portable;
// Bounded pools, eight variants each: 8 car textures in use at once at full
// resolution (256x256 8-bit, 5.6 MB), 20 more at native size (1.8 MB).
VehicleLights<CacheCount, 20, 11296, 8, 88576> lights;
const decltype(lights)::Profile profile{models, sizeof(models) / sizeof(*models), FirstModel, lightRects, lightSets,
                                        {86, 613, 604, 608, 960}, false};
// Converted hand-painted variants (tools/vehicle-lights/pack.py), read per texture.
// Native size (when the HD pool is full) and full resolution 8-bit.
LightPackFile packFile, hdFile;
const uint8_t* Pack(unsigned cache, uint32_t* size) { return packFile.load(cache, size); }
const uint8_t* PackHd(unsigned cache, uint32_t* size) { return hdFile.load(cache, size); }
SafetyMipsInline render;
SafetyMipsMid renderOneNonRoad;
injector::hook_back<void (*)()> renderSorted;
void Render(void* car) {
    lights.render(car, profile, [&] { render.call<void>(car); });
}
void RenderSorted() {
    lights.sorted(profile, [] { renderSorted.fun(); });
}
uintptr_t UniqueSite(const char* signature, int offset)
{
    const auto address = pattern.get_first(signature, offset);
    if (!address || pattern.get(1, signature, offset) || address < pattern.text_addr ||
        pattern.text_size < 16 || address - pattern.text_addr > pattern.text_size - 16)
    {
        return 0;
    }
    return address;
}
}
void Install() {
    if (packFile.open("ms0:/PSP/PLUGINS/GTAVCS.PPSSPP.FusionFix/lights.bin")) lights.textures.pack = Pack;
    if (hdFile.open("ms0:/PSP/PLUGINS/GTAVCS.PPSSPP.FusionFix/lights-hd.bin")) lights.hd.pack = PackHd;
    const uintptr_t site = UniqueSite("00 00 BF AF 50 00 85 8C ? ? ? ? 00 00 00 00 4C 00 84 8C", -4);
    // CRenderer::RenderOneNonRoad (US 0x08A7A664) draws the vehicle's alpha
    // elements (body, doors, glass) from a sorted list after CVehicle::Render
    // returns (US 0x08A7A904). Both sites match once on the US and EU discs;
    // without them only the alpha step stays unlit.
    const uintptr_t entry = UniqueSite("E0 FF BD 27 FF 00 A5 30 00 00 B0 AF 04 00 B1 AF 08 00 B2 AF 0C 00 B3 AF 10 00 B4 AF 14 00 B5 AF 18 00 BF AF ? ? A0 10 25 80 80 00 48 00 04 8E 0E 00 84 30 0A 00 84 38", 0);
    const uintptr_t list = UniqueSite("09 F8 C0 00 21 20 05 02 ? ? 60 12 00 00 00 00 ? ? ? ? 00 00 00 00 5C 00 04 8E 25 28 40 02 98 00 84 24", 16);
    if (!site || !entry || !list)
    {
        PatchRejected(PSP_HOOK_UNSUPPORTED_INSTRUCTION);
        return;
    }
    render = safetymips::create_inline(site, Render);
    renderSorted.fun = injector::MakeCALL(list, RenderSorted).get();
    renderOneNonRoad = safetymips::create_mid(entry, [](SafetyMipsContext& regs) {
        lights.drawn = reinterpret_cast<const void*>(uintptr_t(regs.a0));
    });
}
}
// MAIN.VehicleLights (read at startup): lit head, tail, brake and reverse lights on vehicle bodies.
void vcsws::InstallVehicleLights() { vehicle::Install(); }
