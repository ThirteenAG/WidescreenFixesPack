#include "Game.hpp"
#include "../../external/injector/include/ps2/patches.hpp"
#include <cstring>
#include <cstdio>
extern "C" {
#include "../../external/injector/include/ps2/pcsx2f_api.h"
#include "../../external/injector/include/ps2/patterns.h"
}

// 60 FPS and the missions that count frames.
//
// The VBlank interrupt handler (sub_370300) only runs its phase-2 frame work
// when a0 == 2 ("bne a0, v0" at 0x370314). NOP-ing that branch runs it on
// every notification. The word is switched from the game thread (init, menu,
// game tick), so no plugin code runs inside the interrupt handler.
//
// A few mission scripts add fixed amounts per frame. When a mission is loaded
// (COMMAND_LOAD_AND_LAUNCH_MISSION_INTERNAL), those values are halved before
// its first instruction runs; each record keeps both values so the frame rate
// can change in the menu during the mission. The concert mission's rhythm part
// stays at 30 FPS.
namespace vcs {
namespace {
constexpr uintptr_t sixtyFPSSite = 0x370314;
constexpr uintptr_t scriptSpace = 0x48741C, mainScriptSize = 0x4CD3D8, largestMission = 0x4CD3E8;
constexpr uintptr_t onAMissionFlag = 0x4CD3D4, theText = 0x48A2A0;
pcsx2::Patch sixtyFPSPatch;
pcsx2::GameCallback<uint64_t(uintptr_t)> beginMission;
pcsx2::GameFunction<uint64_t(uintptr_t)> processScript;
pcsx2::GameFunction<int(uintptr_t, const char*)> textExists;
bool concert = false, concertActive = false;

struct MissionPatch { uintptr_t at; uint8_t size, original[4], patched[4]; };
MissionPatch missionPatches[12];
unsigned missionPatchCount = 0;
void RecordPatch(uintptr_t at, const void* patched, uint8_t size) {
    if (missionPatchCount >= sizeof(missionPatches) / sizeof(missionPatches[0]) || size > 4) return;
    auto& patch = missionPatches[missionPatchCount++];
    patch.at = at; patch.size = size;
    std::memcpy(patch.original, reinterpret_cast<const void*>(at), size);
    std::memcpy(patch.patched, patched, size);
}
void ApplyMissionPatches(bool sixty) {
    for (unsigned i = 0; i < missionPatchCount; ++i) {
        const auto& patch = missionPatches[i];
        const auto* from = sixty ? patch.original : patch.patched;
        const auto* to = sixty ? patch.patched : patch.original;
        // Skip script memory a later mission reused.
        if (std::memcmp(reinterpret_cast<const void*>(patch.at), from, patch.size)) continue;
        std::memcpy(reinterpret_cast<void*>(patch.at), to, patch.size);
    }
}
bool OnMission() {
    const auto space = *reinterpret_cast<const uintptr_t*>(scriptSpace);
    const auto offset = *reinterpret_cast<const uint32_t*>(onAMissionFlag);
    return space && offset && offset < 0x200000 && *reinterpret_cast<const int32_t*>(space + offset) == 1;
}
void SetFrameRate() {
    if (!sixtyFPSPatch) return;
    const bool sixty = settings.sixtyFPS && !concertActive;
    if (sixtyFPSPatch.enabled() == sixty) return;
    const auto status = sixty ? sixtyFPSPatch.enable() : sixtyFPSPatch.disable();
    if (status != PCSX2_HOOK_OK)
        std::snprintf(OSDText[0], OSDStringSize, "VCS 60 FPS patch failed (%u)", unsigned(status));
}
uint64_t BeginMission(uintptr_t script) {
    const auto space = *reinterpret_cast<const uintptr_t*>(scriptSpace);
    const auto mainSize = *reinterpret_cast<const uint32_t*>(mainScriptSize);
    const auto capacity = *reinterpret_cast<const uint32_t*>(largestMission);
    struct Fix { const char* pattern; unsigned offset; };
    // Frame-based additions: opcode, variable, then the float that is halved.
    static constexpr Fix fixes[] = {
        {"08 00 ? ? ? 8F C2 75 3C", 5}, // Boomshine Blowout
        {"0A 00 ? ? ? 0A D7 23 3D", 5}, // Hose the Hoes, The Exchange
        {"0A 00 ? ? ? 8F C2 F5 3C", 5},
        {"0A 00 ? ? ? 0A D7 23 3C", 5},
        {"08 00 ? ? ? 0A D7 A3 3B", 5}, // Balls, Farewell To Arms
    };
    concert = concertActive = false;
    missionPatchCount = 0;
    if (space && mainSize < 0x200000 && capacity > 0 && capacity < 0x200000 && space + mainSize + capacity < 0x2000000) {
        const uintptr_t mission = space + mainSize;
        concert = !std::memcmp(reinterpret_cast<const void*>(mission), "\x38\x02\x0ARENC7", 9);
        for (const auto& fix : fixes) {
            const uintptr_t at = range_pattern.get_first(mission, capacity, fix.pattern, int(fix.offset));
            if (!at) continue;
            float value;
            std::memcpy(&value, reinterpret_cast<const void*>(at), sizeof(value));
            value *= 0.5f;
            RecordPatch(at, &value, sizeof(value));
        }
        // A truck timer counted in frames (7000 -> 14000) and a frame-based amount.
        const uintptr_t truck = range_pattern.get_first(mission, capacity, "2F 04 11 08 58 1B 4C 01 11 08 58 1B", 0);
        static constexpr uint16_t truckTime = 14000;
        static constexpr uint8_t lanceValue = 44;
        if (truck) { RecordPatch(truck + 4, &truckTime, 2); RecordPatch(truck + 10, &truckTime, 2); }
        const uintptr_t lance = range_pattern.get_first(mission, capacity, "0B 00 ? ? ? 0B", 0);
        if (lance) RecordPatch(lance + 5, &lanceValue, 1);
        ApplyMissionPatches(settings.sixtyFPS);
    }
    SetFrameRate();
    return processScript(script);
}
}
void ApplyFrameRate() {
    ApplyMissionPatches(settings.sixtyFPS);
    SetFrameRate();
}
void TickTiming() {
    if (!concert) return;
    const auto text = *reinterpret_cast<const uintptr_t*>(theText);
    concertActive = OnMission() && text && textExists(text, "REN7_O9") != 0;
    if (!OnMission()) concert = false;
    SetFrameRate();
}
void InstallTiming() {
    static constexpr uint32_t nop = 0;
    const auto status = sixtyFPSPatch.create(injector::detail::backend, sixtyFPSSite, &nop, 1);
    if (status != PCSX2_HOOK_OK)
        std::snprintf(OSDText[0], OSDStringSize, "VCS 60 FPS patch failed (%u)", unsigned(status));
    processScript.bind(0x255FA8); // CRunningScript::Process
    textExists.bind(0x3FB218);    // CText::Exists
    beginMission.bind(BeginMission);
    injector::MakeCALL(0x3B232C, beginMission.address());
    SetFrameRate();
}
}
