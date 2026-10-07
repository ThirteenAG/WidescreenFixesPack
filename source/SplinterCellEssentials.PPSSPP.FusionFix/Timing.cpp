#include "Game.hpp"

namespace essentials {
namespace {
SafetyMipsMid loading;
injector::hook_back<int()> loaded;
int Loaded() { Unthrottle(false); return loaded.fun(); }

void InstallFrameRate() {
    if (!inireader.ReadInteger("MAIN", "Enable60FPS", 0)) return;
    const auto psnBudget = sites::Unique("08 3D 04 3C 89 88 84 34 00 60 84 44 1E 00 90 2A 01 65 14 46");
    if (psnBudget) {
        // PSN advances its engine clock by a fixed frame interval. Change that
        // interval with the loading/menu sleep budget, retaining real-time speed.
        const auto clock = sites::Unique("80 3F 04 3C 00 60 84 44 ?? ?? 04 3C ?? ?? 8D C4 03 63 0D 46 ?? ?? 04 3C ?? ?? 8E C4 00 73 0C 46 08 00 E0 03 ?? ?? 8C E4");
        if (!clock) return;
        injector::WriteMemory<uint16_t>(psnBudget, injector::HighWord(1.0f / 60));
        injector::WriteMemory<uint16_t>(psnBudget + 4, injector::LowWord(1.0f / 60));
        // Replace the two-instruction FPS load with 60.0f (lui a0; mtc1 a0,f13).
        // No native global is modified or overwritten during engine startup.
        injector::WriteMemory<uint32_t>(clock + 8, 0x3C044270u);
        injector::WriteMemory<uint32_t>(clock + 12, 0x44846800u);
    } else if (const auto loop = sites::MainLoop()) {
        // The disc build already advances simulation using elapsed real time.
        injector::WriteMemory<uint16_t>(loop + 0xB0, injector::HighWord(1.0f / 60));
        injector::WriteMemory<uint16_t>(loop + 0xB4, injector::LowWord(1.0f / 60));
    }
}
}

void InstallTiming() {
    InstallFrameRate();
    if (!inireader.ReadInteger("MAIN", "UnthrottleEmuDuringLoading", 1)) return;
    auto enable = sites::Unique("D2 43 05 3C 00 90 85 44 40 C2 05 3C");
    if (!enable) enable = sites::Unique("D2 43 06 3C 00 90 86 44 40 C2 06 3C");
    const auto disable = sites::Unique("34 00 B2 AF 38 00 BF AF ? ? ? ? 00 00 00 00", 8);
    // Optional loading acceleration cannot undo the game's control hooks when
    // a release has a different loading path.
    if (!enable || !disable) return;
    loading = safetymips::create_mid(enable, [](SafetyMipsContext&) { Unthrottle(true); });
    loaded.fun = injector::MakeCALL(disable, Loaded).get();
}
}
