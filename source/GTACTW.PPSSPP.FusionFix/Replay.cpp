#include "Game.hpp"
namespace ctw {
namespace {
SafetyMipsInline script, manager;
alignas(16) uint8_t replayStatus[2848];
uintptr_t textBegin, textEnd;
bool AvailabilityQuery(uintptr_t caller) {
    // Replay code is an overlay. Recognize a read-only two-bit status query
    // instead of assuming an overlay address or altering the persistent save.
    if ((caller >= textBegin && caller < textEnd) || !Guest(caller, 128)) return false;
    const auto pda = at<uintptr_t>(Address<0x08C11258>());
    if (!Guest(pda, 200) || at<unsigned>(pda + 196) != 0x23) return false;
    bool shift = false;
    for (unsigned offset = 0; offset < 128; offset += 4) {
        const uint32_t word = at<uint32_t>(caller + offset);
        const unsigned opcode = word >> 26;
        if (opcode == 3 || opcode == 2 || (opcode >= 40 && opcode <= 46)) return false;
        if (opcode == 0 && (word & 63) == 6) shift = true; // SRLV
        if (shift && opcode == 12 && (word & 0xFFFF) == 3) return true;
        if (opcode >= 4 && opcode <= 7) return false;
    }
    return false;
}
uintptr_t StatusView(uintptr_t original, uintptr_t caller) {
    if (!Guest(original, sizeof(replayStatus)) || !AvailabilityQuery(caller)) return original;
    std::memcpy(replayStatus, reinterpret_cast<const void*>(original), sizeof(replayStatus));
    // Xin's two bonus missions are excluded from the completion percentage.
    // Only the replay predicate sees them as available; mission progress, save
    // writes and the replay engine itself continue to use the original section.
    auto* flags = reinterpret_cast<uint32_t*>(replayStatus);
    for (unsigned mission : {103u, 104u}) flags[mission >> 4] |= 3u << (2 * (mission & 15));
    return reinterpret_cast<uintptr_t>(replayStatus);
}
uintptr_t Script() {
    const auto caller = uintptr_t(__builtin_return_address(0));
    return StatusView(script.call<uintptr_t>(), caller);
}
uintptr_t Manager(uintptr_t object) {
    const auto caller = uintptr_t(__builtin_return_address(0));
    return StatusView(manager.call<uintptr_t>(object), caller);
}
}
void InstallReplay() {
    if (!inireader.ReadInteger("MAIN", "UnlockXinMissionsInReplayBoard", 0)) return;
    SceKernelModuleInfo info{};
    if (!FindModule("CTW", info)) return;
    textBegin = info.text_addr; textEnd = textBegin + info.text_size;
    safetymips::Options caller;caller.preserve_game_callees=false;
    script = safetymips::create_inline(Address<0x08A05F28>(), Script, caller);
    manager = safetymips::create_inline(Address<0x08A36E2C>(), Manager, caller);
}
}
