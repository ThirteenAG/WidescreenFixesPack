#include "Game.hpp"
#include <array>
namespace rain {
namespace {
float* CameraQuat;float* CameraPos;float* RainGrid;uint8_t* RainBitmap;
float (*GetRainIntensity)();int* RainCheck1;int* RainCheck2;int* nLoading;
bool paused=true,cutscene;
std::array<SafetyMipsMid,4> stateHooks;
pcsx2::GameCallback<void(void*)> rainEntry;
injector::hook_back<void(void*)> drawRain;
bool InsideRain(const Vec3& position) {
    for (unsigned i=0;i<256;++i) {
        if (!(RainBitmap[i>>3]&(1u<<(i&7)))) continue;
        auto* center=RainGrid+i*4;
        float x=position.x-center[0],y=position.y-center[1],z=position.z-center[2];
        if (x*x+y*y+z*z<=4) return true;
    }
    return false;
}
void Camera(Packet& packet) {
    float x=-CameraQuat[0],y=-CameraQuat[1],z=-CameraQuat[2],w=CameraQuat[3];
    Matrix matrix{};
    matrix.at={-2*(x*z+w*y),-2*(y*z-w*x),-(1-2*(x*x+y*y))};
    float length=std::sqrt(matrix.at.x*matrix.at.x+matrix.at.y*matrix.at.y+matrix.at.z*matrix.at.z);
    if (length>0.001f) { matrix.at.x/=length;matrix.at.y/=length;matrix.at.z/=length; }
    if (matrix.at.z<0.1f) matrix.at.z=0.1f;
    matrix.right={-(1-2*(y*y+z*z)),-2*(x*y+w*z),-2*(x*z-w*y)};
    matrix.up={-2*(x*y-w*z),-(1-2*(x*x+z*z)),-2*(y*z+w*x)};
    matrix.position={CameraPos[0],CameraPos[1],CameraPos[2]};
    console::droplets::Camera(packet,matrix);
}
void DrawRain(void* object) {
    drawRain.fun(object);
    if (!Ready()) return;
    auto& packet=Data();
    packet.enabled=*RainCheck1 && *RainCheck2 && !*nLoading && !paused && !cutscene;
    if (!packet.enabled) { packet.rain=0;return; }
    packet.rain=GetRainIntensity();
    if (packet.rain>=0.0133333206f && packet.rain<0.279999971f) packet.rain=0.279999971f;
    Camera(packet);
    if (!InsideRain(packet.position)) packet.rain=0;
    PCSX2F_GuestBeforeUIDraw();
}
}
bool TrueCrime() {
    uintptr_t rain=pattern.get(0,"0C 00 C0 AC ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? 30 00 B0 7B ? ? ? ? 20 00 B1 7B",16);
    if (!rain) return false;
        uintptr_t ptr_16F9A4 = pattern.get(0, "24 00 26 8E 00 00 03 7A 30 00 C3 7C", -16);
        CameraQuat = (float*)GetAbsoluteAddress(ptr_16F9A4, 0, 4);
        uintptr_t ptr_16F9E0 = pattern.get(0, "8F C2 21 34 00 60 81 44 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? 20 00 02 DA", -20);
        CameraPos = (float*)GetAbsoluteAddress(ptr_16F9E0, 0, 4);
        uintptr_t ptr_345940 = pattern.get(0, "A0 86 C6 34 ? ? ? ? ? ? ? ? ? ? ? ? 00 40 21 34 00 A8 81 44", -12);
        RainGrid = (float*)GetAbsoluteAddress(ptr_345940, 0, 8);
        uintptr_t ptr_345A84 = pattern.get(0, "40 20 13 00 ? ? ? ? ? ? ? ? ? ? ? ? C2 28 10 00 07 00 02 32 ? ? ? ? 04 10 43 00 00 00 A3 90 27 10 02 00 10 00 A4 AF", -4);
        RainBitmap = (uint8_t*)GetAbsoluteAddress(ptr_345A84, 0, 8);
        uintptr_t ptr_11AB2C = pattern.get(0, "86 05 00 46 ? ? ? ? 00 00 00 00 ? ? ? ? 46 06 00 46", -4);
        GetRainIntensity = (float(*)())injector::GetBranchDestination(ptr_11AB2C).as_int();
        uintptr_t ptr_101B48 = pattern.get(0, "5C 26 43 8C ? ? ? ? ? ? ? ? ? ? ? ? 00 00 00 00 ? ? ? ? 04 00 43 8C", -8);
        RainCheck1 = (int*)GetAbsoluteAddress(ptr_101B48, 0, 4);
        uintptr_t ptr_10E7D4 = pattern.get(0, "04 00 42 8E ? ? ? ? 02 15 02 00 60 00 83 8C", -4);
        RainCheck2 = (int*)GetAbsoluteAddress(ptr_10E7D4, 0, 8);
        uintptr_t ptr_1FD788 = pattern.get(0, "C0 26 05 8E ? ? ? ? BC 26 02 8E 09 F8 40 00", -8);
        nLoading = (int*)GetAbsoluteAddress(ptr_1FD788, 0, 4);
    uintptr_t pause=pattern.get(0,"08 00 03 8E 13 00 22 B2 0C 00 22 B6 14 00 23 AE ? ? ? ? ? ? ? ? ? ? ? ? 20 00 23 8E",0);
    uintptr_t resume=pattern.get(0,"24 00 22 8E ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? 00 60 81 44 ? ? ? ? ? ? ? ? ? ? ? ? 00 00 00 00",-16);
    uintptr_t start=pattern.get(0,"F0 00 BF FF 5C 26 02 AE ? ? ? ? 3C 26 04 8E",-4);
    uintptr_t stop=pattern.get(0,"00 00 81 44 ? ? ? ? ? ? ? ? 00 08 81 44 ? ? ? ? 94 00 00 AE",-4);
    if (!CameraQuat || !CameraPos || !RainGrid || !RainBitmap || !GetRainIntensity || !RainCheck1 || !RainCheck2 || !nLoading || !pause || !resume || !start || !stop) return false;
    if (injector::InitializeCheckedRuntime()!=PCSX2_HOOK_OK) return false;
    rainEntry.bind(DrawRain);drawRain.fun=injector::MakeCALL(rain,rainEntry.address()).get();
    safetymips::Options options;options.preserve=0;
    stateHooks[0]=safetymips::create_mid(pause,[](SafetyMipsContext&){paused=true;},options);
    stateHooks[1]=safetymips::create_mid(resume,[](SafetyMipsContext&){paused=false;},options);
    stateHooks[2]=safetymips::create_mid(start,[](SafetyMipsContext&){cutscene=true;},options);
    stateHooks[3]=safetymips::create_mid(stop,[](SafetyMipsContext&){cutscene=false;},options);
    return injector::FlushCaches()==PCSX2_HOOK_OK;
}
}
