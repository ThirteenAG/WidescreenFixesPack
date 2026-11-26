#include "Game.hpp"
#include <cstring>
namespace vcsws {
namespace {
uintptr_t limiter;
uint32_t originalLimiter;
bool limited = true, desiredLimited = true;
bool concert = false, concertWasRunning = false, concertPhase = false;
unsigned rateRetry;

struct MissionPatch
{
    uintptr_t address;
    uint8_t size;
    uint32_t original, patched;
};
uintptr_t loadedMission;
unsigned loadedBytes;
injector::hook_back<unsigned(void*, char*, unsigned)> readMission;
injector::hook_back<int(void*)> processMission;
injector::game_fn<int(uintptr_t, const char*)> textExists;

struct MissionFix
{
    const char* mission;
    const char* pattern;
    unsigned offset;
    uint8_t size;
    uint32_t original, patched;
};

constexpr MissionFix missionFixes[] = {
    {"PHI_A2", "08 00 ? ? ? 8F C2 75 3C", 5, 4, 0x3C75C28Fu, 0x3BF5C28Fu},
    {"DIA_C2", "0A 00 ? ? ? 8F C2 75 3D", 5, 4, 0x3D75C28Fu, 0x3CF5C28Fu},
    {"LOU_B1", "0A 00 ? ? ? 0A D7 23 3D", 5, 4, 0x3D23D70Au, 0x3CA3D70Au},
    {"LOU_B1", "0A 00 ? ? ? 8F C2 F5 3C", 5, 4, 0x3CF5C28Fu, 0x3C75C28Fu},
    {"LOU_B1", "0A 00 ? ? ? 0A D7 23 3C", 5, 4, 0x3C23D70Au, 0x3BA3D70Au},
    {"UMB_B2", "08 00 ? ? ? 0A D7 A3 3B", 5, 4, 0x3BA3D70Au, 0x3B23D70Au},
    {"GON_C4", "08 00 ? ? ? 0A D7 A3 3B", 5, 4, 0x3BA3D70Au, 0x3B23D70Au},
    {"GON_C4", "2F 04 11 08 58 1B 4C 01 11 08 58 1B", 4, 2, 7000, 14000},
    {"GON_C4", "2F 04 11 08 58 1B 4C 01 11 08 58 1B", 10, 2, 7000, 14000},
    {"LAN_C10", "0B 00 ? ? ? 0B", 5, 1, 11, 44},
    {"RENC7", "08 00 ? ? ? 09 04 40 3F", 5, 4, 0x3F400409u, 0x3EC00409u},
};
MissionPatch missionPatches[sizeof(missionFixes) / sizeof(*missionFixes)];
unsigned missionPatchCount;

void RecordMissionPatches(uintptr_t mission, unsigned size)
{
    if (size < 12) return;
    auto* header = reinterpret_cast<const char*>(mission);
    if (!std::memcmp(header, "\xEA\x01", 2))
    {
        header += 2;
        if (size < 14) return;
    }
    if (std::memcmp(header, "\x38\x02\x0A", 3)) return;
    const char* name = header + 3;
    concert = !std::memcmp(name, "RENC7", sizeof("RENC7"));
    for (const auto& fix : missionFixes)
    {
        if (std::memcmp(name, fix.mission, std::strlen(fix.mission) + 1)) continue;
        const auto address = range_pattern.get_first(mission, size, fix.pattern, fix.offset);
        if (!address || range_pattern.get(1, mission, size, fix.pattern, fix.offset) ||
            std::memcmp(reinterpret_cast<const void*>(address), &fix.original, fix.size))
        {
            logger.WriteF("Mission timing patch rejected for %s at operand %u.\n", fix.mission, fix.offset);
            continue;
        }
        missionPatches[missionPatchCount++] = {address, fix.size, fix.original, fix.patched};
    }
}

void ApplyMissionPatches(bool sixty)
{
    for (unsigned i = 0; i < missionPatchCount; ++i)
    {
        const auto& patch = missionPatches[i];
        const uint32_t from = sixty ? patch.original : patch.patched;
        const uint32_t to = sixty ? patch.patched : patch.original;
        auto* address = reinterpret_cast<void*>(patch.address);
        if (!std::memcmp(address, &from, patch.size))
            std::memcpy(address, &to, patch.size);
    }
}

bool OnMission()
{
    const auto space = *reinterpret_cast<const uintptr_t*>(Address<0x8BAAB84>());
    const auto offset = *reinterpret_cast<const uint32_t*>(Address<0x8BB3C80>());
    return space && offset && offset < 0x200000 && *reinterpret_cast<const uint32_t*>(space + offset) == 1;
}

bool SetLimiter(bool enable)
{
    const uint32_t expected = limited ? originalLimiter : 0;
    // Invalidate compiled JIT blocks before inspecting the original instruction.
    sceKernelIcacheInvalidateRange(reinterpret_cast<const void*>(limiter), 4);
    if (injector::ReadMemory<uint32_t>(limiter) != expected ||
        injector::WriteMemory<uint32_t>(limiter, enable ? originalLimiter : 0) != PSP_HOOK_OK)
        return false;
    limited = enable;
    return true;
}

void UpdateConcertPhase()
{
    concertPhase = false;
    if (!concert) return;
    if (!OnMission())
    {
        if (concertWasRunning) concert = false;
        return;
    }
    concertWasRunning = true;
    const auto text = *reinterpret_cast<const uintptr_t*>(Address<0x8BAF748>());
    concertPhase = text && textExists(text, "REN7_O9");
}
}

void ApplyFrameRate()
{
    const bool next = !settings.fps || concertPhase;
    if (next != desiredLimited)
    {
        desiredLimited = next;
        rateRetry = 0;
    }
    if (limited == desiredLimited || rateRetry) return;
    if (!SetLimiter(desiredLimited))
    {
        rateRetry = 60;
        logger.Write("Frame limiter patch rejected. Retrying during gameplay.\n");
        return;
    }
    ApplyMissionPatches(!limited);
}

namespace {
void Tick(void*) {
    if (!settings.unthrottle) console::portable::Unthrottle(false);
    else {
        bool menu=*reinterpret_cast<uint8_t*>(Address<0x8BC9100>()+0x20)!=0;
        // The original loading check uses the black-screen start timestamp.
        // cGU's similarly named frame-delay budget stays nonzero in gameplay.
        float black=*reinterpret_cast<float*>(Address<0x8BAD098>());
        float fade=*reinterpret_cast<float*>(Address<0x8BC7E30>()+0xA54);
        console::portable::Unthrottle(!menu && (black!=0 || fade==255.0f ||
            (settings.unthrottleMode==2 && fade>0.0f)));
    }
    UpdateConcertPhase();
    if (rateRetry) --rateRetry;
    ApplyFrameRate();
    TickCheats();
    PollSettings();
}
unsigned ReadMission(void* file, char* destination, unsigned bytes)
{
    const unsigned read = readMission.fun(file, destination, bytes);
    loadedMission = reinterpret_cast<uintptr_t>(destination);
    loadedBytes = read <= bytes ? read : 0;
    return read;
}

int BeginMission(void* script)
{
    const auto space = *reinterpret_cast<const uintptr_t*>(Address<0x8BAAB84>());
    const auto mainSize = *reinterpret_cast<const uint32_t*>(Address<0x8BB3CA4>());
    const auto capacity = *reinterpret_cast<const uint32_t*>(Address<0x8BB3CB0>());
    concert = concertWasRunning = concertPhase = false;
    missionPatchCount = 0;
    if (space >= 0x08000000 && mainSize < 0x200000 && loadedBytes <= capacity &&
        capacity < 0x200000 && space + mainSize + capacity < 0x0DD00000 &&
        loadedMission == space + mainSize)
        RecordMissionPatches(loadedMission, loadedBytes);
    loadedMission = 0;
    loadedBytes = 0;
    ApplyFrameRate();
    ApplyMissionPatches(!limited);
    return processMission.fun(script);
}
}

void InstallTiming()
{
    limiter = Address<0x8A070C8>();
    originalLimiter = injector::ReadMemory<uint32_t>(limiter);
    textExists = decltype(textExists)(Address<0x89F64FC>());
    injector::MakeCALL(Address<0x8937A5C>(), Tick);
    if (settings.unthrottle) console::portable::Unthrottle(true);
    readMission.fun = injector::MakeCALL(Address<0x8ABC128>(), ReadMission).get();
    processMission.fun = injector::MakeCALL(Address<0x8ABC174>(), BeginMission).get();
}
}
