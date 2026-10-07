#include "Sites.hpp"
#include "Addresses.hpp"
#include "Lights.hpp"
#include "Traffic.hpp"
extern "C" {
#include <pspkernel.h>
}
PSP_MODULE_INFO("GTALCS.PPSSPP.Project2DFX", PSP_MODULE_USER, 2, 0);
namespace lcsfx {
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
    uint32_t, uint32_t, float, float, float, float, uint32_t, uint32_t, uint32_t, uint32_t);
Corona* nativeCorona;
void RegisterTraffic(uint32_t id, uint32_t r, uint32_t g, uint32_t b, uint32_t alpha,
    const ProjectedPoint* position, uint32_t type, uint32_t flare, float radius, float range,
    float angle, float pull, uint32_t reflection, uint32_t los, uint32_t streak, uint32_t flag) {
    lod.learnTraffic(position, activeGroup);
    nativeCorona(id, r, g, b, alpha, position, type, flare, radius, range, angle, pull,
                 reflection, los, streak, flag);
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
    if (lightsEnabled) lod.render(SpriteBudget(Address<0x8B8ECD0>(),Address<0x8B8ECD0>()+12));
    return renderer.state(state, data);
}
void RenderStars() { stars.render(SpriteBudget(Address<0x8B8ECD0>(),Address<0x8B8ECD0>()+12)); }
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
    using namespace lcsfx;
    using namespace console::portable;
    if (!Start("GTA3", "ms0:/PSP/PLUGINS/GTALCS.PPSSPP.Project2DFX/GTALCS.PPSSPP.Project2DFX.ini",
        "ms0:/PSP/PLUGINS/GTALCS.PPSSPP.Project2DFX/GTALCS.PPSSPP.Project2DFX.log")) return -1;
    // PPSSPP loads its plugin before the game's render objects are initialized.
    sceKernelDelayThread(250000);
    if (!Begin()) return -1;
    if (!InitializeAddresses()) return -1;
    renderer = {
        Address<0x8B833A0>() + 0xA70, Address<0x8B5E1A8>(), Address<0x8B5E1A9>(), Address<0x8B5E144>(), Address<0x8B5E184>(), Address<0x8B5E208>(), Address<0x8B4BF04>(), Address<0x8B56ACC>(), Address<0x8B56AD0>(),
        reinterpret_cast<decltype(renderer.project)>(Address<0x8a25970>()),
        reinterpret_cast<decltype(renderer.sprite)>(Address<0x8a27098>()),
        reinterpret_cast<decltype(renderer.state)>(Address<0x88b7a70>()),
        reinterpret_cast<decltype(renderer.flush)>(Address<0x8a25f64>()),
        {reinterpret_cast<int (*)()>(Address<0x8865a58>()), reinterpret_cast<int (*)()>(Address<0x8865abc>())},
        6, 8, 10, 11, 1.0f, 1000.0f, 0.5f, 900
    };
    renderer.radius = console::bounded(inireader.ReadFloat("PROJECT2DFX", "CoronaRadiusMultiplier", 1.0f), 0.01f, 10.0f, 1.0f);
    renderer.range = console::bounded(inireader.ReadFloat("PROJECT2DFX", "CoronaFarClip", 1000.0f), 100.0f, 4000.0f, 1000.0f);
    int visible = inireader.ReadInteger("PROJECT2DFX", "MaxVisibleLights", 900);
    renderer.limit = unsigned(visible < 1 ? 1 : (visible > 2048 ? 2048 : visible));
    int coronas = inireader.ReadInteger("PROJECT2DFX", "CoronaLimit", 1000);
    if (coronas > 0) ExpandCoronas(unsigned(coronas < 56 ? 56 : (coronas > 1024 ? 1024 : coronas)));
    traffic.initialize(renderer);
    injector::MakeCALL(Address<0x89fb164>(), RenderLights);
    lightsEnabled = inireader.ReadInteger("PROJECT2DFX", "RenderLodLights", 1) != 0;
    if (lightsEnabled) {
        lod.initialize(renderer, true);
        nativeCorona = reinterpret_cast<Corona*>(Address<0x89fa098>());
        trafficDisplay = safetymips::create_inline(Address<0x886621c>(), DisplayTraffic);
        trafficType = safetymips::create_inline(Address<0x8865bcc>(), FindTrafficType);
        for (uintptr_t site : trafficCalls) injector::MakeCALL(Address(site), RegisterTraffic);
    }
    if (inireader.ReadInteger("PROJECT2DFX", "SkyGfx", 1)) {
        stars.initialize(renderer);
        injector::MakeCALL(Address<0x8836ec8>(), RenderStars);
    }
    return Finish();
}
