#pragma once
#include <cstdint>
#include <cstring>
#include <cstddef>

namespace console::droplets {
struct Vec3 { float x,y,z; };
struct Matrix { Vec3 right;uint32_t flags;Vec3 up;uint32_t pad1;Vec3 at;uint32_t pad2;Vec3 position;uint32_t pad3; };
// Host/guest protocol. Its order and exported 255-byte storage remain stable.
struct Packet {
    uint32_t enabledAddress,enabled;
    float rain;uint32_t rainAddress;
    Vec3 right,up,at,position;
    uint32_t matrixAddresses[12];
    Vec3 splashPosition;float splashDistance;int32_t splashDuration;float splashRemovalDistance;
    int32_t fillAmount;
    Vec3 movingPosition;float movingAmount;int32_t blood;
};
static_assert(sizeof(Packet)==160 && offsetof(Packet,movingAmount)==152);
struct ParticleProfile {
    uint16_t moving[9],blood[3],splashes[6];unsigned splashCount;
};
inline bool Contains(const uint16_t* list,unsigned size,unsigned value) {
    for (unsigned i=0;i<size;++i) if (list[i]==value) return true;
    return false;
}
inline void Particle(Packet& packet,const ParticleProfile& profile,unsigned type,const Vec3* position) {
    if (!position) return;
    if (Contains(profile.moving,9,type)) {
        packet.movingPosition=*position;packet.movingAmount=1;
        packet.blood=Contains(profile.blood,3,type);
    } else if (Contains(profile.splashes,profile.splashCount,type)) {
        packet.splashPosition=*position;packet.splashDistance=10;
        packet.splashDuration=300;packet.splashRemovalDistance=50;
    }
}
inline void Camera(Packet& packet,const Matrix& matrix) {
    packet.right=matrix.right;packet.up=matrix.up;packet.at=matrix.at;packet.position=matrix.position;
    // Publish values. No pointer from a destroyed camera can outlive this tick.
    for (auto& address:packet.matrixAddresses) address=0;
}
struct StoryProfile {
    uintptr_t camera;unsigned cameraOffset;
    uintptr_t menu;unsigned menuOffset;
    uintptr_t splash,rain,cameraNoRain,playerNoRain,cutscene,area;
    unsigned cutsceneOffset;
    bool indirectCutscene;
};
inline bool Readable(uintptr_t address,size_t bytes,uintptr_t begin,uintptr_t end) {
    return address>=begin && address<end && bytes<=end-address;
}
inline void Update(Packet& packet,const StoryProfile& game,uintptr_t begin,uintptr_t end) {
    packet.enabled=!*reinterpret_cast<const uint8_t*>(game.menu+game.menuOffset) &&
        (!game.splash || !*reinterpret_cast<const uint8_t*>(game.splash));
    bool blocked=*reinterpret_cast<const uint32_t*>(game.area)!=0 ||
        (*reinterpret_cast<const uint32_t*>(game.cameraNoRain)&8) ||
        (*reinterpret_cast<const uint32_t*>(game.playerNoRain)&8);
    uintptr_t cutscene=game.indirectCutscene ? *reinterpret_cast<const uint32_t*>(game.cutscene) : game.cutscene;
    if (cutscene && !Readable(cutscene+game.cutsceneOffset,1,begin,end)) {
        packet.enabled=0;packet.rain=0;return;
    }
    blocked=blocked || (cutscene && *reinterpret_cast<const uint8_t*>(cutscene+game.cutsceneOffset));
    float intensity=blocked ? 0 : *reinterpret_cast<const float*>(game.rain);
    packet.rain=intensity>=0 && intensity<=1 ? intensity : intensity>1 ? 1 : 0;
    uintptr_t camera=*reinterpret_cast<const uint32_t*>(game.camera+game.cameraOffset);
    uintptr_t node=Readable(camera,8,begin,end) ? *reinterpret_cast<const uint32_t*>(camera+4) : 0;
    if (Readable(node,sizeof(Matrix)+16,begin,end)) Camera(packet,*reinterpret_cast<const Matrix*>(node+16));
    else { packet.enabled=0;packet.rain=0; }
}
inline uintptr_t SignedAddress(uintptr_t instruction,int high=0,int low=4) {
    if (!instruction) return 0;
    return (uintptr_t(*reinterpret_cast<const uint16_t*>(instruction+high))<<16)+
        int16_t(*reinterpret_cast<const uint16_t*>(instruction+low));
}
}
