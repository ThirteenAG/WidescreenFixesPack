#include "Game.hpp"
#include "Sites.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>

extern "C" {
int CompatibleCRCList[] = {0x42A9C4EC, 0x7A9D67B8, 0x77B4F13C, static_cast<int>(0xB1AC3BEB)};
int CompatibleElfCRCList[] = {0x42A9C4EC, 0x1118ACD0, 0x7A9D67B8, static_cast<int>(0xB73CDCFA),
                            0x77B4F13C, static_cast<int>(0xA4334B91), static_cast<int>(0xB1AC3BEB), static_cast<int>(0xB2D44C6C)};
int PCSX2Data[PCSX2Data_Size] = {1};
char OSDText[OSDStringNum][OSDStringSize] = {{1}};
char PluginData[MaxIniSize] = {1};
char FrameLimitUnthrottle;
}

namespace {
void Rejected(pcsx2_hook_status status) {
    std::snprintf(OSDText[0], OSDStringSize, "True Crime fix disabled: patch validation failed (%u)", unsigned(status));
}
float Constraint(const char* value) {
    if (!value) return -1;
    char* end = nullptr;
    const float numerator = std::strtof(value, &end);
    if (end == value || *end != ':') return -1;
    const char* denominatorStart = end + 1;
    const float denominator = std::strtof(denominatorStart, &end);
    if (end == denominatorStart || *end || !(numerator > 0 && denominator > 0)) return -1;
    return console::bounded(numerator / denominator, 4.0f / 3.0f, 32.0f / 9.0f, -1);
}
uintptr_t Absolute(uintptr_t at) {
    return (uintptr_t(injector::ReadMemory<uint16_t>(at)) << 16) + int16_t(injector::ReadMemory<uint16_t>(at + 4));
}
bool Required(uintptr_t value, const char* name) {
    if (value) return true;
    std::snprintf(OSDText[0], OSDStringSize, "True Crime fix disabled: missing %s pattern", name);
    return false;
}
}

extern "C" void init() {
    using namespace tcny;
    if (injector::InitializeCheckedRuntime(Rejected) != PCSX2_HOOK_OK) return;
    inireader.SetIniPath(PluginData + sizeof(uint32_t), *reinterpret_cast<const uint32_t*>(PluginData));
    const bool skipIntro = inireader.ReadInteger("MAIN", "SkipIntro", 1) != 0;
    enable60 = inireader.ReadInteger("MAIN", "Enable60FPS", 0) != 0;
    unthrottle = inireader.ReadInteger("MAIN", "UnthrottleEmuDuringLoading", 1) != 0;
    char text[64];
    display.constraint = Constraint(inireader.ReadString("MAIN", "HudAspectRatioConstraint", "auto", text, sizeof(text)));
    // As before the rewrite: fast-forward from boot until the first engine frame reports loading state.
    FrameLimitUnthrottle = unthrottle;
    // The launcher and the streamed TC2 engine are separate ELF generations.
    if (const auto intro = sites::at_100350()) {
        if (skipIntro) injector::MakeNOP(intro);
        injector::FlushCaches();
        return;
    }
    const auto load = sites::at_1FD788(), processCall = sites::at_1FE0C4();
    const auto frameCall = sites::at_4A1C60(), intervalCall = sites::at_4A1CC0();
    const auto wide = sites::at_204EBC(), scale = sites::at_204DD8();
    const auto width = sites::at_2346E8(), layout = sites::at_23C638();
    const auto subtitle = sites::at_20BA48(), models = sites::at_255E88(), map = sites::at_265744();
    const auto menuBlips = sites::at_265224(), radarBlips = sites::at_267F74(), blips = sites::at_268094();
    const auto radar = sites::at_2517A8(), radarDisc = sites::at_2478FC(), radarClip = sites::at_2519D0();
    const uintptr_t required[] = {load, processCall, frameCall, intervalCall, wide, scale, width, layout,
                                 subtitle, models, map, menuBlips, radarBlips, blips, radar, radarDisc, radarClip};
    const char* names[] = {"loading", "game state", "frame", "interval", "native widescreen", "projection",
                          "HUD width", "HUD layout", "subtitles", "models", "map", "menu blips", "radar blips",
                          "blip origin", "radar", "radar disc", "radar clip"};
    for (unsigned i = 0; i < sizeof(required) / sizeof(*required); ++i) {
        // Without the per-frame loading check, boot fast-forward would never end.
        if (!Required(required[i], names[i])) { FrameLimitUnthrottle = 0; injector::FlushCaches(); return; }
    }
    display.width = sites::at_204DB0() ? 512.0f : 640.0f;
    display.refresh();
    loading = reinterpret_cast<int*>(Absolute(load));
    mapCallers[0] = menuBlips + 8; mapCallers[1] = radarBlips + 8;
    const auto process = injector::GetBranchDestination(processCall).as_int();
    InstallTiming(process, frameCall + 16, intervalCall);
    InstallHud(width, layout, subtitle, models, map, blips, radar, radarDisc, scale);
    injector::WriteMemory<uint32_t>(wide, 0x0000282B); // SLTU a1, zero, zero
    injector::WriteMemory<uint32_t>(radarClip, 0x00073A00); // SLL a3, a3, 8
    // A rejected session installs nothing, including the frame hook that ends boot fast-forward.
    if (injector::FlushCaches() != PCSX2_HOOK_OK) FrameLimitUnthrottle = 0;
}
extern "C" int main() { return 0; }
