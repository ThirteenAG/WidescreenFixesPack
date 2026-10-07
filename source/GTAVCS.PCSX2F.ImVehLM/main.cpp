#include "Models.hpp"
#include "LightRects.hpp"
#include "LightPack.hpp"
#include "../../external/injector/include/ps2/runtime.hpp"
#include "../../external/injector/include/ps2/game_abi.hpp"
#include "../../external/injector/include/ps2/safetymips.hpp"
#include <cstdio>

extern "C" {
#include "../../external/injector/include/ps2/pcsx2f_api.h"
int CompatibleCRCList[] = {0x4F32A11F};
char OSDText[OSDStringNum][OSDStringSize] = {{1}};
}

namespace imvehlm {
namespace {
// Lit head/tail/brake/reverse variants come from the converted hand-painted art,
// or are generated from the game's own car texture where there is none, when
// first drawn; a bounded pool holds 20 textures in use at once.
// Full resolution (256x256 8-bit, one level): 8 textures, 4.3 MB.
console::ps2lights::VehicleLights<CacheCount, 20, 8256, 8, 66560> lights;
const decltype(lights)::Profile profile{models, sizeof(models) / sizeof(*models), FirstModel,
    vehicle::lightRects, vehicle::lightSets, {0x56, 0x265, 0x25C, 0x260, 0x3C0}};
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
    return lights.render(vehicle, profile, [&] { return render.fun(vehicle); });
}
uint64_t RenderSorted() {
    return lights.sorted(profile, [] { return renderSorted.fun(); });
}

void Rejected(pcsx2_hook_status status) {
    std::snprintf(OSDText[0], OSDStringSize, "VCS ImVehLM disabled: patch validation failed (%u)", unsigned(status));
}
}
}

extern "C" void init() {
    using namespace imvehlm;
    if (injector::InitializeCheckedRuntime(Rejected) != PCSX2_HOOK_OK) return;
    lights.textures.pack = Pack;
    lights.hd.pack = PackHd;
    renderCallback.bind(Render);
    render.fun = injector::MakeCALL(0x14C684, renderCallback.address()).get();
    render.fun = injector::MakeCALL(0x343F8C, renderCallback.address()).get();
    // CRenderer::RenderOneNonRoad (0x35D610) draws a vehicle's alpha elements
    // (body, doors, glass) after CVehicle::Render returns, from the list that
    // the element callbacks fill (0x6F4030, drawn by sub_36EED0).
    sortedCallback.bind(RenderSorted);
    renderSorted.fun = injector::MakeCALL(0x35D8E8, sortedCallback.address()).get();
    // RenderOneNonRoad entry (addiu sp / sd, no branch targets): the entity it draws.
    renderOneNonRoad = safetymips::create_mid(0x35D610, [](SafetyMipsContext& regs) {
        lights.drawn = reinterpret_cast<const void*>(uintptr_t(regs.a0));
    });
    injector::FlushCaches();
}
extern "C" int main() { return 0; }
