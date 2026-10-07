#include "Game.hpp"
#include <cstdio>
extern "C" {
#include "../../external/injector/include/ps2/pcsx2f_api.h"
#include "../../external/injector/include/ps2/inireader.h"
int CompatibleCRCList[] = {0x4F32A11F};
char PluginData[MaxIniSize] = {1};
char OSDText[OSDStringNum][OSDStringSize] = {{1}};
}
namespace {
void Rejected(pcsx2_hook_status status) {
    std::snprintf(OSDText[0], OSDStringSize, "VCS Project2DFX disabled: patch validation failed (%u)", unsigned(status));
}
}
extern "C" void init() {
    using namespace vcsfx;
    if (injector::InitializeCheckedRuntime(Rejected) != PCSX2_HOOK_OK) return;
    inireader.SetIniPath(PluginData + sizeof(uint32_t), *reinterpret_cast<const uint32_t*>(PluginData));
    settings.lights = inireader.ReadInteger("PROJECT2DFX", "RenderLodLights", 0) != 0;
    settings.stars = inireader.ReadInteger("PROJECT2DFX", "SkyGfx", 1) != 0;
    settings.traffic = inireader.ReadInteger("DISTANTTRAFFIC", "DistantCars", 1) != 0 || inireader.ReadInteger("DISTANTTRAFFIC", "DistantBoats", 1) != 0;
    int limit = inireader.ReadInteger("PROJECT2DFX", "CoronaLimit", 900);
    settings.limit = unsigned(limit < 1 ? 1 : (limit > 2048 ? 2048 : limit));
    settings.radius = console::bounded(inireader.ReadFloat("PROJECT2DFX", "CoronaRadiusMultiplier", 1.0f), 0.01f, 10.0f, 1.0f);
    settings.range = console::bounded(inireader.ReadFloat("PROJECT2DFX", "CoronaFarClip", 500.0f), 100.0f, 4000.0f, 500.0f);
    settings.smallStars = console::bounded(inireader.ReadFloat("STARS", "SmallestStarsSize", 0.15f), 0.03f, 2.5f, 0.15f);
    settings.mediumStars = console::bounded(inireader.ReadFloat("STARS", "MiddleStarsSize", 0.6f), settings.smallStars, 2.5f, 0.6f);
    settings.largeStars = console::bounded(inireader.ReadFloat("STARS", "BiggestStarsSize", 1.2f), settings.smallStars, 2.5f, 1.2f);
    settings.largeChance = console::bounded(inireader.ReadFloat("STARS", "BiggestStarsChance", 20.0f), 0.0f, 100.0f, 20.0f) * 0.01f;
    if (settings.traffic) InstallTraffic();
    if (settings.lights||settings.traffic) InstallLights();
    if (settings.stars) InstallStars();
    injector::FlushCaches();
}
extern "C" int main() { return 0; }
