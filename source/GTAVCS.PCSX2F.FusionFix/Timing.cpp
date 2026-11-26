#include "Game.hpp"
#include "Settings.hpp"
#include "ps2/patches.hpp"
#include <cstring>
#include <cstdio>
extern "C" {
#include "ps2/pcsx2f_api.h"
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
bool concert = false, concertPhaseRequested = false;
unsigned concertRetryDelay = 0;

struct MissionPatch { uintptr_t at; uint8_t size, original[4], patched[4]; };
MissionPatch missionPatches[12];
unsigned missionPatchCount = 0;
void RecordPatch(uintptr_t at, uint32_t original, uint32_t patched, uint8_t size)
{
    if (std::memcmp(reinterpret_cast<const void*>(at), &original, size) != 0)
        return;
    auto& patch = missionPatches[missionPatchCount++];
    patch.at = at;
    patch.size = size;
    std::memcpy(patch.original, &original, size);
    std::memcpy(patch.patched, &patched, size);
}

struct ScriptFix
{
    const char* name;
    uint16_t offset;
    uint8_t size;
    uint32_t original, patched;
};

constexpr ScriptFix scriptFixes[] = {
    {"PHI_A2", 0x202B, 4, 0x3C75C28F, 0x3BF5C28F}, // Boomshine Blowout
    {"LOU_B1", 0x06E7, 4, 0x3D23D70A, 0x3CA3D70A}, // Hose the Hoes
    {"LOU_B1", 0x0704, 4, 0x3CF5C28F, 0x3C75C28F},
    {"LOU_B1", 0x0714, 4, 0x3C23D70A, 0x3BA3D70A},
    {"DIA_C2", 0x1E44, 4, 0x3D23D70A, 0x3CA3D70A}, // The Exchange
    {"UMB_B2", 0x24D4, 4, 0x3BA3D70A, 0x3B23D70A}, // Balls
    {"GON_C4", 0x2968, 4, 0x3BA3D70A, 0x3B23D70A}, // Farewell to Arms
    {"GON_C4", 0x1763, 2, 7000, 14000},
    {"GON_C4", 0x1769, 2, 7000, 14000},
    {"LAN_C10", 0x229C, 1, 11, 44}
};
static_assert(sizeof(scriptFixes) / sizeof(*scriptFixes) <= sizeof(missionPatches) / sizeof(*missionPatches));

void RecordMissionPatches(uintptr_t mission, uint32_t capacity)
{
    // The first script command names the mission. One script starts with a
    // single command before the name.
    const auto* header = reinterpret_cast<const char*>(mission);
    if (capacity < 14)
        return;
    if (!std::memcmp(header, "\xEA\x01", 2))
        header += 2;
    if (std::memcmp(header, "\x38\x02\x0A", 3))
        return;
    const char* name = header + 3;
    concert = std::memcmp(name, "RENC7", sizeof("RENC7")) == 0;
    for (const auto& fix : scriptFixes)
    {
        const size_t nameSize = std::strlen(fix.name) + 1;
        if (fix.offset + fix.size <= capacity && !std::memcmp(name, fix.name, nameSize))
            RecordPatch(mission + fix.offset, fix.original, fix.patched, fix.size);
    }
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
void FrameRateFailure(pcsx2_hook_status status)
{
    static constexpr const char* languages[] = { "en", "fr", "de", "it", "es" };
    static constexpr const char* messages[] = {
        "VCS 60 FPS patch failed (%u)", "VCS echec du patch 60 FPS (%u)",
        "VCS 60-FPS-Patch fehlgeschlagen (%u)", "VCS patch 60 FPS non riuscita (%u)",
        "VCS fallo del parche de 60 FPS (%u)"
    };
    unsigned language = 0;
    for (unsigned i = 0; i < 5; ++i)
        if (std::strcmp(config::values[9].value, languages[i]) == 0)
            language = i;
    std::snprintf(OSDText[0], OSDStringSize, messages[language], unsigned(status));
}

bool SetFrameRate(bool sixty)
{
    if (sixtyFPSPatch.enabled() == sixty)
        return true;
    if (!sixtyFPSPatch)
        return false;
    const auto status = sixty ? sixtyFPSPatch.enable() : sixtyFPSPatch.disable();
    if (status != PCSX2_HOOK_OK)
        FrameRateFailure(status);
    return status == PCSX2_HOOK_OK;
}
bool DesiredFrameRate()
{
    return settings.sixtyFPS && !concertPhaseRequested;
}

void ApplyConcertRate()
{
    // Retry a failed phase transition on the 60th subsequent game tick.
    concertRetryDelay = SetFrameRate(DesiredFrameRate()) ? 0 : 59;
    ApplyMissionPatches(sixtyFPSPatch.enabled());
}

uint64_t BeginMission(uintptr_t script) {
    const auto space = *reinterpret_cast<const uintptr_t*>(scriptSpace);
    const auto mainSize = *reinterpret_cast<const uint32_t*>(mainScriptSize);
    const auto capacity = *reinterpret_cast<const uint32_t*>(largestMission);
    concert = concertPhaseRequested = false;
    missionPatchCount = 0;
    if (space && mainSize < 0x200000 && capacity > 0 && capacity < 0x200000 && space + mainSize + capacity < 0x2000000)
        RecordMissionPatches(space + mainSize, capacity);
    ApplyConcertRate();
    return processScript(script);
}
}
void ApplyFrameRate()
{
    if (!sixtyFPSPatch || !SetFrameRate(DesiredFrameRate()))
        settings.sixtyFPS = sixtyFPSPatch.enabled();
    ApplyMissionPatches(sixtyFPSPatch.enabled());
}

void TickTiming()
{
    bool requested = false;
    if (concert)
    {
        concert = OnMission();
        const auto text = *reinterpret_cast<const uintptr_t*>(theText);
        requested = concert && text && textExists(text, "REN7_O9") != 0;
    }
    if (requested != concertPhaseRequested)
    {
        concertPhaseRequested = requested;
        ApplyConcertRate();
        return;
    }
    if (sixtyFPSPatch.enabled() == DesiredFrameRate())
    {
        concertRetryDelay = 0;
        return;
    }
    if (concertRetryDelay)
    {
        --concertRetryDelay;
        return;
    }
    ApplyConcertRate();
}
void InstallTiming() {
    static constexpr uint32_t nop = 0;
    const auto status = sixtyFPSPatch.create(injector::detail::backend, sixtyFPSSite, &nop, 1);
    if (status != PCSX2_HOOK_OK)
        FrameRateFailure(status);
    processScript.bind(0x255FA8); // CRunningScript::Process
    textExists.bind(0x3FB218);    // CText::Exists
    beginMission.bind(BeginMission);
    injector::MakeCALL(0x3B232C, beginMission.address());
    ApplyFrameRate();
}
}
