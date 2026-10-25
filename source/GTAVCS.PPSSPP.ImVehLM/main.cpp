#include "Models.hpp"
#include "LightRects.hpp"
#include "../Shared/Console/PortableCfg.hpp"
extern "C" { PSP_MODULE_INFO("GTAVCS.ImVehLM", PSP_MODULE_USER, 2, 0); }
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
int Install() {
    if (!BeginReported()) return -1;
    if (packFile.open("ms0:/PSP/PLUGINS/GTAVCS.PPSSPP.ImVehLM/lights.bin")) lights.textures.pack = Pack;
    if (hdFile.open("ms0:/PSP/PLUGINS/GTAVCS.PPSSPP.ImVehLM/lights-hd.bin")) lights.hd.pack = PackHd;
    const auto opaque = pattern.get_first("00 00 BF AF 50 00 85 8C ? ? ? ? 00 00 00 00 4C 00 84 8C", -4);
    // CRenderer::RenderOneNonRoad (US 0x08A7A664) draws the vehicle's alpha
    // elements (body, doors, glass) from a sorted list after CVehicle::Render
    // returns (US 0x08A7A904). Both sites match once on the US and EU discs;
    // without them only the alpha step stays unlit.
    const uintptr_t entry = pattern.get_first("E0 FF BD 27 FF 00 A5 30 00 00 B0 AF 04 00 B1 AF 08 00 B2 AF 0C 00 B3 AF 10 00 B4 AF 14 00 B5 AF 18 00 BF AF ? ? A0 10 25 80 80 00 48 00 04 8E 0E 00 84 30 0A 00 84 38", 0);
    const uintptr_t list = pattern.get_first("09 F8 C0 00 21 20 05 02 ? ? 60 12 00 00 00 00 ? ? ? ? 00 00 00 00 5C 00 04 8E 25 28 40 02 98 00 84 24", 16);
    if (!opaque || !entry || !list)
    {
        logger.Write("Vehicle lights disabled: unsupported render code.\n");
        return -1;
    }
    render = safetymips::create_inline(opaque, Render);
    {
        renderSorted.fun = injector::MakeCALL(list, RenderSorted).get();
        renderOneNonRoad = safetymips::create_mid(entry, [](SafetyMipsContext& regs) {
            lights.drawn = reinterpret_cast<const void*>(uintptr_t(regs.a0));
        });
    }
    return Finish();
}
}
}
extern "C" int module_start(SceSize, void*) {
    if (!console::portable::StartReported("GTA3", "ms0:/PSP/PLUGINS/GTAVCS.PPSSPP.ImVehLM/GTAVCS.PPSSPP.ImVehLM.cfg",
                                "ms0:/PSP/PLUGINS/GTAVCS.PPSSPP.ImVehLM/GTAVCS.PPSSPP.ImVehLM.log")) return 0;
    console::portable::IniValue values[] = {{"MAIN", "VehicleLights", "1"}};
    if (!console::portable::CompleteCfg("ms0:/PSP/PLUGINS/GTAVCS.PPSSPP.ImVehLM/GTAVCS.PPSSPP.ImVehLM.cfg", values))
    {
        logger.Write("Vehicle lights disabled: CFG settings could not be saved.\n");
        return 0;
    }
    if (!inireader.ReadBoolean("MAIN", "VehicleLights", true)) return 0;
    return vehicle::Install();
}
