#include "Models.hpp"
#include "LightRects.hpp"
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
    // Keep the emulator's established delay before reading relocated code.
    // All storage remains in this plugin's own module.
    sceKernelDelayThread(100000);
    if (!Begin()) return -1;
    if (packFile.open("ms0:/PSP/PLUGINS/GTALCS.PPSSPP.ImVehLM/lights.bin")) lights.textures.pack = Pack;
    if (hdFile.open("ms0:/PSP/PLUGINS/GTALCS.PPSSPP.ImVehLM/lights-hd.bin")) lights.hd.pack = PackHd;
    render = safetymips::create_inline(pattern.get_first("50 00 84 8C ? ? ? ? 00 00 00 00 00 00 85 90 01 00 06 34 ? ? ? ? 00 00 00 00", -8), Render);
    // CRenderer::RenderOneNonRoad (US 0x08AAA0B4) draws a vehicle's alpha atomics
    // (body, doors, glass) from CVisibilityPlugins' sorted list after the
    // vehicle's Render returns (RenderAlphaAtomics, US 0x08AAA268); the vehicle
    // hi-detail alpha callbacks queue them there instead of drawing them.
    const uintptr_t entry = pattern.get_first("D0 FF BD 27 44 00 85 8C 10 00 B0 AF 25 80 80 00 0E 00 A4 30 06 00 84 38", 0);
    const uintptr_t list = pattern.get_first("03 00 60 12 00 00 00 00 ? ? ? 0E 00 00 00 00 5C 00 04 8E 25 28 40 02 90 00 84 24", 8);
    if (entry && list) {
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
    if (!console::portable::Start("GTA3", "ms0:/PSP/PLUGINS/GTALCS.PPSSPP.ImVehLM/GTALCS.PPSSPP.ImVehLM.ini",
                                "ms0:/PSP/PLUGINS/GTALCS.PPSSPP.ImVehLM/GTALCS.PPSSPP.ImVehLM.log")) return 0;
    return vehicle::Install();
}
