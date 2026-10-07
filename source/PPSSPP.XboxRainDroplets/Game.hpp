#pragma once
#include "../Shared/Console/PSP.hpp"
#include "../Shared/Console/StoryParticles.hpp"
#include <array>
#include <cmath>
extern "C" char XboxRainDropletsData[255];
namespace rain {
using namespace console::droplets;
struct BeforeUIData { char signature[20];volatile uint32_t tick;uint32_t packet; };
extern BeforeUIData beforeUI;
inline bool Ready() { return XboxRainDropletsData[0]!='X'; }
inline Packet& Data() { return *reinterpret_cast<Packet*>(XboxRainDropletsData); }
inline uintptr_t GetAbsoluteAddress(uintptr_t at,int hi=0,int lo=4) { return SignedAddress(at,hi,lo); }
inline void Report(uint32_t list) { sceIoDevctl("emulator:",0x35,const_cast<uint32_t*>(&beforeUI.tick),sizeof(beforeUI.tick),reinterpret_cast<void*>(list),0); }
inline uintptr_t FrameCall(uintptr_t target) {
    uint32_t call=0x0C000000u|((target>>2)&0x03FFFFFFu);uintptr_t found=0;
    for (uintptr_t at=pattern.text_addr;at+4<=pattern.text_addr+pattern.text_size;at+=4) if (*reinterpret_cast<const uint32_t*>(at)==call) {
        if (found) return 0;
        found=at;
    }
    return found;
}
bool Stories();bool Essentials();
}
