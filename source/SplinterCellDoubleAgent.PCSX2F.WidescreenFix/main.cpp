#include "Game.hpp"
#include <cstdio>
extern "C" {
int CompatibleCRCList[] = {static_cast<int>(0xC0498D24), static_cast<int>(0xABE2FDE9)};
int CompatibleElfCRCList[] = {static_cast<int>(0xC0498D24), static_cast<int>(0xABE2FDE9), 0x0198F1AD, 0x6BD0E9C2};
int PCSX2Data[PCSX2Data_Size] = {1};
char OSDText[OSDStringNum][OSDStringSize] = {{1}}, PluginData[MaxIniSize] = {1};
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
        const auto check = pattern.get(1, "00 00 45 8C ? ? ? ? B8 00 0B 24 ? ? ? ? 00 00 00 00", 20);
        const auto from = pattern.get_first("00 00 00 00 02 00 02 24 ? ? ? ? 00 00 00 00 01 00 02 24 ? ? ? ? 00 00 00 00 ? ? ? ? 00 00 00 00 ? ? ? ? 00 00 00 00 00 00 00 00 ? ? ? ? 00 00 00 00", -4);
        const auto to = pattern.get_first("03 00 02 24 00 00 00 00 ? ? ? ? 00 00 00 00", 0);
        if (check && from && to) {
            injector::WriteMemory<uint16_t>(branch + 2, 0x1000);
            injector::MakeNOP(check);
            injector::MakeJMP(from, to);
        }
    }
    return true;
}
}
extern "C" void init() {
    using namespace scda;
    if (injector::InitializeCheckedRuntime(Rejected) != PCSX2_HOOK_OK) return;
    inireader.SetIniPath(PluginData + sizeof(uint32_t), Read<uint32_t>(uintptr_t(PluginData)));
    if (Launcher(inireader.ReadInteger("MAIN", "SkipIntro", 1) != 0)) { injector::FlushCaches(); return; }
    game = Read<uint32_t>(0x25F55C) == 0x3C023F80 ? US : EU;
    if (injector::GetBranchDestination(game.projectionCaller - 8).as_int() != game.projection) {
        Rejected(PCSX2_HOOK_CONFLICT); injector::FlushCaches(); return;
    }
    hudSize = console::bounded(inireader.ReadFloat("HUD", "HudScale", 1), 0.5f, 1.5f, 1);
    widescreenHud = inireader.ReadInteger("HUD", "WidescreenHud", 1) != 0;
#ifdef WFP_PS2_DEBUG // build-module.ps1 -Defines WFP_PS2_DEBUG
    traceDraws = inireader.ReadInteger("DEBUG", "TraceDraws", 0) != 0;
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
