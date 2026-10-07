#pragma once
#include "../../external/injector/include/ps2/runtime.hpp"
#include "../../external/injector/include/ps2/game_abi.hpp"
#include "../Shared/Console/StoryParticles.hpp"
#include <cmath>
extern "C" {
#include "../../external/injector/include/ps2/pcsx2f_api.h"
#include "../../external/injector/include/ps2/patterns.h"
extern char XboxRainDropletsData[255];
}
namespace rain {
using namespace console::droplets;
inline uintptr_t GetAbsoluteAddress(uintptr_t a,int hi=0,int lo=4) { return SignedAddress(a,hi,lo); }
inline bool Ready() { return XboxRainDropletsData[0]!='X'; }
inline Packet& Data() { return *reinterpret_cast<Packet*>(XboxRainDropletsData); }
inline uintptr_t FrameCall(uintptr_t target) {
    if (!target) return 0;
    uint32_t instruction=0x0C000000u|((target>>2)&0x03FFFFFFu);
    uintptr_t result=0;
    for (uintptr_t a=pattern.text_addr;a+4<=pattern.text_addr+pattern.text_size;a+=4)
        if (*reinterpret_cast<const uint32_t*>(a)==instruction) {
            if (result) return 0; // The render phase must have exactly one caller.
            result=a;
        }
    return result;
}
bool Stories();bool TrueCrime();
}
