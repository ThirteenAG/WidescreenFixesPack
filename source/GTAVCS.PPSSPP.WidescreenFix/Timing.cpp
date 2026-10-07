#include "Game.hpp"
#include <cstring>
namespace vcsws {
namespace {
uintptr_t limiter;
uint32_t originalLimiter;
bool limited = true, concert, concertActive;
// Mission timing corrections for 60 FPS. Each record keeps both values so the
// frame rate can change in the menu during a mission; bounded to one mission.
struct MissionPatch { uintptr_t at; uint8_t size, original[4], patched[4]; };
MissionPatch missionPatches[12];
unsigned missionPatchCount;
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
        // Skip memory reused after the mission ended.
        if (std::memcmp(reinterpret_cast<const void*>(patch.at), from, patch.size)) continue;
        injector::WriteMemoryRaw(patch.at, to, patch.size);
    }
}
injector::hook_back<int(void*)> processMission;
bool OnMission() {
    uintptr_t script=*reinterpret_cast<uintptr_t*>(Address<0x8BAAB84>());
    uint32_t offset=*reinterpret_cast<uint32_t*>(Address<0x8BB3C80>());
    return script && offset && offset<0x200000 && *reinterpret_cast<uint32_t*>(script+offset)==1;
}
void SetLimiter(bool enable);
}
void ApplyFrameRate() {
    SetLimiter(!settings.fps || concertActive);
    ApplyMissionPatches(settings.fps != 0);
}
namespace {
void SetLimiter(bool enable) {
    if (limited==enable) return;
    uint32_t expected=limited ? originalLimiter : 0;
    // PPSSPP keeps JIT block markers in compiled code; invalidation restores the
    // original word before it is compared.
    sceKernelIcacheInvalidateRange(reinterpret_cast<const void*>(limiter),4);
    if (injector::ReadMemory<uint32_t>(limiter)!=expected) return;
    if (injector::WriteMemory<uint32_t>(limiter,enable ? originalLimiter : 0)==PSP_HOOK_OK) {
        limited=enable;
        (void)injector::FlushCaches();
    }
}
void Tick(void*) {
    if (!settings.unthrottle) console::portable::Unthrottle(false);
    else {
        bool menu=*reinterpret_cast<uint8_t*>(Address<0x8BC9100>()+0x20)!=0;
        // The original loading check uses the black-screen start timestamp.
        // cGU's similarly named frame-delay budget stays nonzero in gameplay.
        float black=*reinterpret_cast<float*>(Address<0x8BAD098>());
        float fade=*reinterpret_cast<float*>(Address<0x8BC7E30>()+0xA54);
        console::portable::Unthrottle(!menu && (black!=0 || fade==255.0f));
    }
    if (concert) {
        uintptr_t text=*reinterpret_cast<uintptr_t*>(Address<0x8BAF748>());
        auto get=reinterpret_cast<const uint16_t* (*)(uintptr_t,const char*)>(Address<0x89F6390>());
        const uint16_t* title=text ? get(text,"REN7_O9") : nullptr;
        concertActive=OnMission() && title && *title;
        if (!concertActive) concert=false;
        SetLimiter(!settings.fps || concertActive);
    }
    if (settings.pcCheats) TickCheats();
}
int BeginMission(void* script) {
    uintptr_t space=*reinterpret_cast<uintptr_t*>(Address<0x8BAAB84>());
    uint32_t mainSize=*reinterpret_cast<uint32_t*>(Address<0x8BB3CA4>());
    uint32_t capacity=*reinterpret_cast<uint32_t*>(Address<0x8BB3CB0>());
    struct Fix { const char* pattern; unsigned offset; };
    constexpr Fix fixes[]={
        {"08 00 ? ? ? 8F C2 75 3C",5}, // Boomshine Blowout
        {"0A 00 ? ? ? 8F C2 75 3D",5}, // The Exchange
        {"0A 00 ? ? ? 0A D7 23 3D",5}, // Hose the Hoes
        {"0A 00 ? ? ? 8F C2 F5 3C",5},
        {"0A 00 ? ? ? 0A D7 23 3C",5},
        {"08 00 ? ? ? 0A D7 A3 3B",5}, // Balls / Farewell To Arms
        {"08 00 ? ? ? 09 04 40 3F",5}, // In The Air Tonight
    };
    concert=concertActive=false; missionPatchCount=0;
    SetLimiter(!settings.fps);
    if (space>=0x08000000 && mainSize<0x200000 && capacity>0 && capacity<0x200000 &&
        space+mainSize+capacity<0x0DD00000) {
        uintptr_t mission=space+mainSize;
        for (unsigned i=0;i<sizeof(fixes)/sizeof(fixes[0]);++i) {
            uintptr_t at=range_pattern.get_first(mission,capacity,fixes[i].pattern,int(fixes[i].offset));
            if (!at) continue;
            float value;
            std::memcpy(&value,reinterpret_cast<void*>(at),sizeof(value));
            value*=0.5f;
            RecordPatch(at,&value,sizeof(value));
            if (i==6) concert=true;
        }
        uintptr_t truck=range_pattern.get_first(mission,capacity,"2F 04 11 08 58 1B 4C 01 11 08 58 1B",0);
        const uint16_t truckTime=14000; const uint8_t lanceValue=44;
        if (truck) { RecordPatch(truck+4,&truckTime,2); RecordPatch(truck+10,&truckTime,2); }
        uintptr_t lance=range_pattern.get_first(mission,capacity,"0B 00 ? ? ? 0B",0);
        if (lance) RecordPatch(lance+5,&lanceValue,1);
        ApplyMissionPatches(settings.fps != 0);
    }
    // The loaded mission is corrected before its first native execution.
    return processMission.fun(script);
}
}
void InstallTiming() {
    limiter=Address<0x8A070C8>();
    injector::MakeCALL(Address<0x8937A5C>(),Tick);
    if (settings.unthrottle) console::portable::Unthrottle(true);
    // The frame rate can change in the Display menu; the limiter and mission
    // corrections are applied or restored at runtime.
    originalLimiter=injector::ReadMemory<uint32_t>(limiter);
    if (settings.fps) { injector::MakeNOP(limiter); limited=false; }
    processMission.fun=injector::MakeCALL(Address<0x8ABC174>(),BeginMission).get();
}
}
