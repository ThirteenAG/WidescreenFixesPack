#include "Game.hpp"
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include "../Shared/Console/PS2Settings.hpp"
PCSX2F_CFG_SETTINGS()
extern "C" {
int CompatibleCRCList[] = {static_cast<int>(0xC0498D24), static_cast<int>(0xABE2FDE9)};
int CompatibleElfCRCList[] = {static_cast<int>(0xC0498D24), static_cast<int>(0xABE2FDE9), 0x0198F1AD, 0x6BD0E9C2};
int PCSX2Data[PCSX2Data_Size] = {1};
char OSDText[OSDStringNum][OSDStringSize] = {{1}};
}
namespace {
SafetyMipsMid projection;
void Rejected(pcsx2_hook_status status) {
    std::snprintf(OSDText[0], OSDStringSize, "Double Agent fix disabled: patch validation failed (%u)", unsigned(status));
}
bool Launcher(bool skip) {
    const auto branch = pattern.get_first("00 00 00 00 ? ? ? ? 00 00 00 00 ? ? ? ? 2D 28 00 00 ? ? ? ? 2D 30 00 00 2D 38 00 00", -12);
    if (!branch) return false;
    if (skip) {
        // Leave memory-card setup and the frontend state loop intact. Only the
        // startup logos movie is optional; jumping past the frontend also skips
        // initialization required by the next executable.
        const auto args = branch + 24;
        const auto hi = scda::Read<uint32_t>(args);
        const auto lo = scda::Read<uint32_t>(args + 8);
        const auto call = args + 24;
        const auto name = static_cast<uint32_t>((hi & 0xffff) << 16) +
                          static_cast<int16_t>(lo & 0xffff);
        if ((hi & 0xffff0000) == 0x3c040000 && (lo & 0xffff0000) == 0x24840000 &&
            name >= 0x100000 && name <= 0x01fffffa &&
            scda::Read<uint32_t>(call) >> 26 == 3 &&
            scda::Read<uint32_t>(call + 4) == 0x0000482d &&
            scda::Read<uint32_t>(name) == 0x6f676f6c &&
            scda::Read<uint16_t>(name + 4) == 0x0073) {
            injector::MakeNOP(call);
        } else {
            Rejected(PCSX2_HOOK_CONFLICT);
        }
    }
    return true;
}
}
extern "C" void init() {
    using namespace scda;
    if (injector::InitializeCheckedRuntime(Rejected) != PCSX2_HOOK_OK) return;
    console::ps2settings::Value options[] = {{"MAIN", "SkipIntro", "1"}, {"MAIN", "HudScale", "1.0"},
                                            {"HUD", "WidescreenHud", "1"}
#ifdef WFP_PS2_DEBUG
                                            , {"DEBUG", "TraceDraws", "0"}
#endif
                                            };
    if (!console::ps2settings::load(options))
        std::snprintf(OSDText[0], OSDStringSize, "CFG settings unavailable: update the plugin injector for CFG support.");
    if (Launcher(console::ps2settings::boolean(options[0], true))) { injector::FlushCaches(); return; }
    game = Read<uint32_t>(0x25F55C) == 0x3C023F80 ? US : EU;
    if (injector::GetBranchDestination(game.projectionCaller - 8).as_int() != game.projection) {
        Rejected(PCSX2_HOOK_CONFLICT); injector::FlushCaches(); return;
    }
    char* end = nullptr;
    const float scale = std::strtof(options[1].value, &end);
    while (end && (*end == ' ' || *end == '\t')) ++end;
    hudSize = end && end != options[1].value && !*end && std::isfinite(scale) ? console::bounded(scale, 0.5f, 1.5f, 1) : 1;
    widescreenHud = console::ps2settings::boolean(options[2], true);
#ifdef WFP_PS2_DEBUG // build-module.ps1 -Defines WFP_PS2_DEBUG
    traceDraws = console::ps2settings::boolean(options[3], false);
    logger.SetBuffer(OSDText, OSDStringNum, OSDStringSize);
#endif
    projection = safetymips::create_mid(game.projection, [](SafetyMipsContext& regs) {
        if (uintptr_t(regs.ra) == game.projectionCaller)
            regs.f12 *= (4.0f / 3.0f) / console::ps2Aspect(PCSX2Data);
    });
    InstallHud();
    injector::FlushCaches();
}
extern "C" int main() { return 0; }
