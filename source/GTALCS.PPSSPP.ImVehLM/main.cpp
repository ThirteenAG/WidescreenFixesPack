#include "Models.hpp"
#include "LightRects.hpp"
#include "../Shared/Console/PortableCfg.hpp"
extern "C" { PSP_MODULE_INFO("GTALCS.ImVehLM", PSP_MODULE_USER, 2, 0); }
namespace vehicle {
namespace {
using namespace console::portable;
// Bounded pools, eight variants each: 8 car textures in use at once at full
// resolution (256x256 8-bit, 5.6 MB), 16 more at native size (2.9 MB).
VehicleLights<CacheCount, 16, 23008, 8, 88576> lights;
const decltype(lights)::Profile profile{models, sizeof(models) / sizeof(*models), FirstModel, lightRects, lightSets,
                                        {88, 597, 588, 592, 864}, true};
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
    if (packFile.open("ms0:/PSP/PLUGINS/GTALCS.PPSSPP.ImVehLM/lights.bin")) lights.textures.pack = Pack;
    if (hdFile.open("ms0:/PSP/PLUGINS/GTALCS.PPSSPP.ImVehLM/lights-hd.bin")) lights.hd.pack = PackHd;
    const auto opaque = pattern.get_first("50 00 84 8C ? ? ? ? 00 00 00 00 00 00 85 90 01 00 06 34 ? ? ? ? 00 00 00 00", -8);
    // CRenderer::RenderOneNonRoad (US 0x08AAA0B4) draws a vehicle's alpha atomics
    // (body, doors, glass) from CVisibilityPlugins' sorted list after the
    // vehicle's Render returns (RenderAlphaAtomics, US 0x08AAA268); the vehicle
    // hi-detail alpha callbacks queue them there instead of drawing them.
    const uintptr_t entry = pattern.get_first("D0 FF BD 27 44 00 85 8C 10 00 B0 AF 25 80 80 00 0E 00 A4 30 06 00 84 38", 0);
    const uintptr_t list = pattern.get_first("03 00 60 12 00 00 00 00 ? ? ? 0E 00 00 00 00 5C 00 04 8E 25 28 40 02 90 00 84 24", 8);
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
    if (!console::portable::StartReported("GTA3", "ms0:/PSP/PLUGINS/GTALCS.PPSSPP.ImVehLM/GTALCS.PPSSPP.ImVehLM.cfg",
                                "ms0:/PSP/PLUGINS/GTALCS.PPSSPP.ImVehLM/GTALCS.PPSSPP.ImVehLM.log")) return 0;
    console::portable::IniValue values[] = {{"MAIN", "VehicleLights", "1"}};
    if (!console::portable::CompleteCfg("ms0:/PSP/PLUGINS/GTALCS.PPSSPP.ImVehLM/GTALCS.PPSSPP.ImVehLM.cfg", values))
    {
        logger.Write("Vehicle lights disabled: CFG settings could not be saved.\n");
        return 0;
    }
    if (!inireader.ReadBoolean("MAIN", "VehicleLights", true)) return 0;
    return vehicle::Install();
}
