#include "Sites.hpp"
#include "Addresses.hpp"
#include "Lights.hpp"
#include "Traffic.hpp"
extern "C" {
#include <pspkernel.h>
}
PSP_MODULE_INFO("GTAVCS.PPSSPP.Project2DFX", PSP_MODULE_USER, 2, 0);
namespace vcsfx {
namespace {
using namespace console::portable;
StoryLights<sizeof(lights) / sizeof(lights[0])> lod(lights);
StoryStars stars;
DistantTraffic traffic;
bool lightsEnabled;
alignas(16) uint8_t coronaPool[1024 * 112]{};
LightRenderer renderer{};
SafetyMipsInline trafficDisplay, trafficType;
unsigned activeGroup;
bool learning;
using Corona = void(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, const ProjectedPoint*,
    uint32_t, uint32_t, float, float, float, float, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
Corona* nativeCorona;
void RegisterTraffic(uint32_t id, uint32_t r, uint32_t g, uint32_t b, uint32_t alpha,
    const ProjectedPoint* position, uint32_t type, uint32_t flare, float radius, float range,
    float angle, float pull, uint32_t reflection, uint32_t los, uint32_t streak, uint32_t flag, uint32_t flag2) {
    lod.learnTraffic(position, activeGroup);
    nativeCorona(id, r, g, b, alpha, position, type, flare, radius, range, angle, pull,
                 reflection, los, streak, flag, flag2);
}
void DisplayTraffic(void* entity) {
    unsigned previousGroup = activeGroup;
    bool previousLearning = learning;
    activeGroup = 0; learning = true;
    trafficDisplay.call<void>(entity);
    activeGroup = previousGroup; learning = previousLearning;
}
int FindTrafficType(void* entity) {
    int group = trafficType.call<int>(entity);
    if (learning) activeGroup = unsigned(group);
    return group;
}
int RenderLights(int state, uintptr_t data) {
    traffic.render();
    if(lightsEnabled)lod.render(SpriteBudget(Address<0x8BC7370>(),Address<0x8BC7370>()+12));
    return renderer.state(state, data);
}
void RenderStars() { stars.render(SpriteBudget(Address<0x8BC7370>(),Address<0x8BC7370>()+12)); }
void ExpandCoronas(unsigned limit) {
    uintptr_t pool = reinterpret_cast<uintptr_t>(coronaPool);
    for (const auto& pointer : poolPointers) {
        uint32_t high = injector::ReadMemory<uint32_t>(Address(pointer[0]));
        uint32_t low = injector::ReadMemory<uint32_t>(Address(pointer[1]));
        bool signedLow = (low >> 26) == 9;
        injector::WriteMemory<uint32_t>(Address(pointer[0]), (high & 0xFFFF0000u) | ((pool + (signedLow ? 0x8000u : 0u)) >> 16));
        injector::WriteMemory<uint32_t>(Address(pointer[1]), (low & 0xFFFF0000u) | (pool & 0xFFFFu));
    }
    for (uintptr_t address : poolLimits)
        injector::WriteMemory<uint32_t>(Address(address), (injector::ReadMemory<uint32_t>(Address(address)) & 0xFFFF0000u) | limit);
}
}
}
extern "C" int module_start(SceSize, void*) {
    using namespace vcsfx;
    using namespace console::portable;
    if (!Start("GTA3", "ms0:/PSP/PLUGINS/GTAVCS.PPSSPP.Project2DFX/GTAVCS.PPSSPP.Project2DFX.ini",
        "ms0:/PSP/PLUGINS/GTAVCS.PPSSPP.Project2DFX/GTAVCS.PPSSPP.Project2DFX.log")) return -1;
    // PPSSPP loads its plugin before the game's render objects are initialized.
    sceKernelDelayThread(250000);
    if (!Begin()) return -1;
    if (!InitializeAddresses()) return -1;
    renderer = {
        Address<0x8BC7E30>() + 0x9B0, Address<0x8BB3B40>(), Address<0x8BB3B41>(), Address<0x8BAFD90>(), Address<0x8BB3C3C>(), Address<0x8BB3E2C>(), Address<0x8BA10A8>(), Address<0x8BAFB38>(), Address<0x8BAFB3C>(),
        reinterpret_cast<decltype(renderer.project)>(Address<0x8aa82d4>()),
        reinterpret_cast<decltype(renderer.sprite)>(Address<0x8aa9b20>()),
        reinterpret_cast<decltype(renderer.state)>(Address<0x8861668>()),
        reinterpret_cast<decltype(renderer.flush)>(Address<0x8aa8bdc>()),
        {reinterpret_cast<int (*)()>(Address<0x8a0f284>()), reinterpret_cast<int (*)()>(Address<0x8a0f2e0>())},
        4, 6, 8, 9, 1.0f, 1000.0f, 1.0f / 3.0f, 900
    };
    renderer.radius = console::bounded(inireader.ReadFloat("PROJECT2DFX", "CoronaRadiusMultiplier", 1.0f), 0.01f, 10.0f, 1.0f);
    renderer.range = console::bounded(inireader.ReadFloat("PROJECT2DFX", "CoronaFarClip", 1000.0f), 100.0f, 4000.0f, 1000.0f);
    int visible = inireader.ReadInteger("PROJECT2DFX", "MaxVisibleLights", 900);
    renderer.limit = unsigned(visible < 1 ? 1 : (visible > 2048 ? 2048 : visible));
    int coronas = inireader.ReadInteger("PROJECT2DFX", "CoronaLimit", 0);
    if (coronas > 0) ExpandCoronas(unsigned(coronas < 56 ? 56 : (coronas > 1024 ? 1024 : coronas)));
    traffic.initialize(renderer);
    injector::MakeCALL(Address<0x890243c>(), RenderLights);
    lightsEnabled=inireader.ReadInteger("PROJECT2DFX", "RenderLodLights", 1)!=0;
    if (lightsEnabled) {
        lod.initialize(renderer, false);
        nativeCorona = reinterpret_cast<Corona*>(Address<0x8981324>());
        trafficDisplay = safetymips::create_inline(Address<0x8a0f9a4>(), DisplayTraffic);
        trafficType = safetymips::create_inline(Address<0x8a0f384>(), FindTrafficType);
        for (uintptr_t site : trafficCalls) injector::MakeCALL(Address(site), RegisterTraffic);
        injector::WriteMemory<uint32_t>(Address<0x8B01D50>(), 0x3C044448);
        injector::WriteMemory<uint32_t>(Address<0x8B01DA0>(), 0x3C044448);
    }
    if (inireader.ReadInteger("PROJECT2DFX", "SkyGfx", 1)) {
        stars.initialize(renderer);
        injector::MakeCALL(Address<0x888d604>(), RenderStars);
    }
    return Finish();
}
