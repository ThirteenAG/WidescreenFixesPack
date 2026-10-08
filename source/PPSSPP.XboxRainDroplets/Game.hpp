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
// User memory. plugin.ini asks for memory = 93, which PPSSPP always grants to a plugin
// that is loaded (the most it allows), so game objects can live up to 0x08000000 + 93 MB.
constexpr uintptr_t MemoryBegin=0x08800000,MemoryEnd=0x08000000+(93u<<20);
// The host (XboxRainDroplets) reads a packed block: the 21-byte "XBOXRAINDROPLETSDATA"
// signature stays in place and the packet follows it unaligned. The game writes an
// aligned copy, which is published once a frame, right before the host draws.
constexpr size_t PacketOffset=21;
inline Packet packet{};
inline Packet& Data() { return packet; }
inline void PublishEvent(void* field,size_t size) {
    uint8_t* shared=reinterpret_cast<uint8_t*>(XboxRainDropletsData)+PacketOffset;
    size_t offset=static_cast<uint8_t*>(field)-reinterpret_cast<uint8_t*>(&packet);
    memcpy(shared+offset,field,size);
    memset(field,0,size);
}
// Called from hooks that don't save the FPU, so no float is touched here.
inline void Publish() {
    memcpy(XboxRainDropletsData+PacketOffset,&packet,offsetof(Packet,splashPosition));
    // One-shot events: the host clears them once used, so each is published only once.
    uint32_t moving;memcpy(&moving,&packet.movingAmount,sizeof(moving));
    if (packet.splashDuration) PublishEvent(&packet.splashPosition,offsetof(Packet,fillAmount)-offsetof(Packet,splashPosition));
    if (packet.fillAmount) PublishEvent(&packet.fillAmount,sizeof(packet.fillAmount));
    if (moving<<1) PublishEvent(&packet.movingPosition,sizeof(Packet)-offsetof(Packet,movingPosition));
}
static_assert(PacketOffset+sizeof(Packet)<=255);
inline uintptr_t GetAbsoluteAddress(uintptr_t at,int hi=0,int lo=4) { return SignedAddress(at,hi,lo); }
inline void Report(uint32_t list) {
    Publish();
    sceIoDevctl("emulator:",0x35,const_cast<uint32_t*>(&beforeUI.tick),sizeof(beforeUI.tick),reinterpret_cast<void*>(list),0);
}
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
