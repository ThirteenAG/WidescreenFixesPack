#include "Game.hpp"
#include <cstdio>
extern "C" {
#include "../../external/injector/include/ps2/pcsx2f_api.h"
#include "../../external/injector/include/ps2/inireader.h"
int CompatibleCRCList[] = {static_cast<int>(0x7EA439F5)};
char PluginData[MaxIniSize] = {1};
char OSDText[OSDStringNum][OSDStringSize] = {{1}};
}
namespace {
void Rejected(pcsx2_hook_status status) {
    std::snprintf(OSDText[0], OSDStringSize, "LCS Project2DFX disabled: patch validation failed (%u)", unsigned(status));
}
}
extern "C" void init() {
    using namespace lcsfx;
    if (injector::InitializeCheckedRuntime(Rejected) != PCSX2_HOOK_OK) return;
    inireader.SetIniPath(PluginData + sizeof(uint32_t), *reinterpret_cast<const uint32_t*>(PluginData));
    settings.lights = inireader.ReadInteger("PROJECT2DFX", "RenderLodLights", 1) != 0;
    settings.traffic = inireader.ReadInteger("DISTANTTRAFFIC", "DistantCars", 1) != 0 || inireader.ReadInteger("DISTANTTRAFFIC", "DistantBoats", 1) != 0;
    int limit = inireader.ReadInteger("PROJECT2DFX", "CoronaLimit", 900);
    settings.limit = unsigned(limit < 1 ? 1 : (limit > 2048 ? 2048 : limit));
    settings.radius = console::bounded(inireader.ReadFloat("PROJECT2DFX", "CoronaRadiusMultiplier", 1.0f), 0.01f, 10.0f, 1.0f);
    settings.range = console::bounded(inireader.ReadFloat("PROJECT2DFX", "CoronaFarClip", 500.0f), 100.0f, 4000.0f, 500.0f);
    if (settings.traffic) InstallTraffic();
    if (settings.lights || settings.traffic) InstallLights();
    injector::FlushCaches();
}
extern "C" int main() { return 0; }
