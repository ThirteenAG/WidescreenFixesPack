#include "Game.hpp"
namespace rain {
namespace {
StoryProfile profile;
bool viceCity;
uintptr_t TheCamera,dword_08BC9100,dword_08BB3C38,dword_08BB456C,dword_08BB4570,dword_08BB345C,dword_08BB194C;
uintptr_t dword_8B8EE20,dword_8B5E180,dword_8B5E9D0,dword_8B5E9D4,dword_8B303A7,dword_8B58DF0;
uintptr_t listPosition;
SafetyMipsMid particleHook;
injector::hook_back<void()> nativeFrame;
void Tick() {
    StoryProfile current=profile;
    if (viceCity) {
        // This callback is entered by a native call and is compiled with -G0.
        // Its gp is the native caller's base; the SDK runtime never replaces it.
        uintptr_t gp=reinterpret_cast<uintptr_t>(injector::GetGP());
        current.rain+=gp;current.cameraNoRain+=gp;current.playerNoRain+=gp;
        current.cutscene+=gp;current.area+=gp;
    }
    Update(Data(),current,MemoryBegin,MemoryEnd);
}
void BeforeUI() { Report(listPosition ? *reinterpret_cast<const volatile uint32_t*>(listPosition) : 0);nativeFrame.fun(); }
}
bool Stories() {
    // Game threads can replace instructions with PPSSPP JIT markers while a
    // delayed plugin scans. Keep this scan and installation on canonical code.
    struct ScanScope {
        int dispatch = sceKernelSuspendDispatchThread();
        ScanScope() { sceKernelIcacheInvalidateRange(reinterpret_cast<void*>(pattern.text_addr),pattern.text_size); }
        ~ScanScope() { sceKernelResumeDispatchThread(dispatch); }
    } scan;
    uintptr_t tick=pattern.get(0,"1C 00 BF AF ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? 01 00 15 34",4);
    viceCity=tick!=0;
    uintptr_t particle=0,render=0;
    if (viceCity) {
        uintptr_t ptr_6710 = pattern.get(0, "00 00 B0 AF 08 00 BF AF ? ? ? ? 25 80 80 00 01 00 04 34 ? ? ? ? ? ? ? ? 25 20 20 02 ? ? ? ? ? ? ? ? ? ? ? ? 25 20 20 02 ? ? ? ? 00 00 00 00", -8);
        dword_08BC9100 = GetAbsoluteAddress(ptr_6710, 0, 4);
        { auto at=pattern.get(0, "A3 3C 04 3C 0A D7 84 34 00 68 84 44 3E 60 0D 46 00 00 00 00 ? ? ? ? 00 00 00 00 ? ? ? ? 25 20 20 02", -4); if (!at) return false; dword_08BB3C38=uintptr_t(intptr_t(injector::ReadMemory<int16_t>(at))); }
        { auto at=pattern.get(0, "80 00 44 30 ? ? ? ? 00 00 00 00 ? ? ? ? 01 00 10 34", -4); if (!at) return false; dword_08BB456C=uintptr_t(intptr_t(injector::ReadMemory<int16_t>(at))); }
        { auto at=pattern.get(0, "00 00 00 00 20 00 B0 27 ? ? ? ? 25 20 00 02 00 00 00 DA", -20); if (!at) return false; dword_08BB4570=uintptr_t(intptr_t(injector::ReadMemory<int16_t>(at))); }
        { auto at=pattern.get(0, "00 00 00 00 ? ? ? ? 1C 00 85 8C 02 00 A5 38", -20); if (!at) return false; dword_08BB345C=uintptr_t(intptr_t(injector::ReadMemory<int16_t>(at))); }
        { auto at=pattern.get(0, "02 00 04 34 ? ? ? ? 01 00 04 34 30 00 A7 93", -4); if (!at) return false; dword_08BB194C=uintptr_t(intptr_t(injector::ReadMemory<int16_t>(at))); }
        uintptr_t ptr_1F0C = pattern.get(0, "00 00 B0 AF 04 00 B1 AF 0C 00 B3 AF 10 00 B4 AF 14 00 B5 AF 18 00 B6 AF 1C 00 BF AF", -8);
        TheCamera = GetAbsoluteAddress(ptr_1F0C, 0, 4);
        profile={TheCamera,0x7BC,dword_08BC9100,0x20,0,dword_08BB3C38,dword_08BB456C,dword_08BB4570,dword_08BB345C,dword_08BB194C,0x13,true};
        particle=pattern.get(0,"D0 00 B0 AF 25 80 80 00 00 01 A4 8F",-4);
        // The widescreen fix owns Render2DStuff's entry. Locate its untouched
        // register saves instead, then wrap the native call to that entry.
        render=pattern.get(0,"40 00 B0 AF 44 00 B1 AF 48 00 B2 AF 4C 00 BF AF ? ? ? ? 25 28 00 00",-20);
        listPosition=GetAbsoluteAddress(pattern.get(0,"30 00 A7 A3 ? ? ? ? ? ? ? ? 00 B0 80 44 00 00 A5 8C",-4),0,8);
    } else {
        tick=pattern.get(0,"0F 00 05 34 ? ? ? ? 00 00 00 00 ? ? ? ? 25 20 60 02",-4);
        if (!tick) return false;
        uintptr_t ptr_882B9BC = pattern.get(0, "01 00 04 34 00 01 04 A2", -20);
        dword_8B8EE20 = GetAbsoluteAddress(ptr_882B9BC, 0, 4);
        uintptr_t ptr_8817F1C = pattern.get(0, "A3 3C 04 3C 0A D7 84 34 00 68 84 44 3E 60 0D 46 00 00 00 00 ? ? ? ? 00 00 00 00 ? ? ? ? 25 20 20 02", -8);
        dword_8B5E180 = GetAbsoluteAddress(ptr_8817F1C, 0, 4);
        uintptr_t ptr_8ACA280 = pattern.get(0, "80 00 44 30", -8);
        dword_8B5E9D0 = GetAbsoluteAddress(ptr_8ACA280, 0, 4);
        uintptr_t ptr_8ACA338 = pattern.get(0, "00 00 00 00 30 00 B0 27 ? ? ? ? 25 20 00 02 00 00 00 DA", -24);
        dword_8B5E9D4 = GetAbsoluteAddress(ptr_8ACA338, 0, 4);
        uintptr_t ptr_892AED4 = pattern.get(0, "00 00 00 00 ? ? ? ? ? ? ? ? ? ? ? ? 42 6B 0D 46 80 40 04 3C", -12);
        dword_8B303A7 = GetAbsoluteAddress(ptr_892AED4, 0, 4);
        uintptr_t ptr_8AD1508 = pattern.get(0, "25 20 40 02 ? ? ? ? 0C 00 05 34 ? ? ? ? 25 20 60 02", -8);
        dword_8B58DF0 = GetAbsoluteAddress(ptr_8AD1508, 0, 4);
        uintptr_t ptr_8819700 = pattern.get(0, "00 29 05 00 21 30 05 00 C0 28 05 00 21 30 C5 00 80 28 05 00 21 28 C5 00 21 20 A4 00 AC 01 84 84 10 00 05 34 ? ? ? ? 00 3F 04 3C", -12);
        TheCamera = GetAbsoluteAddress(ptr_8819700, 0, 4);
        profile={TheCamera,0xAB0,dword_8B8EE20,0x131,0,dword_8B5E180,dword_8B5E9D0,dword_8B5E9D4,dword_8B303A7,dword_8B58DF0,0,false};
        particle=pattern.get(0,"F4 00 B0 AF 25 80 80 00 20 01 A4 8F",-4);
        render=pattern.get(0,"06 00 04 34 54 00 B4 E7 58 00 B6 E7",-4);
        listPosition=GetAbsoluteAddress(pattern.get(0,"F0 FF 12 24 ? ? ? ? 24 90 92 00 20 00 06 34",-12));
    }
    uintptr_t phase=FrameCall(render);
    if (!profile.camera || !profile.menu || !profile.rain || !profile.cameraNoRain || !profile.playerNoRain || !profile.cutscene || !profile.area || !particle || !phase || !listPosition) return false;
    if (!console::portable::Begin()) return false;
    injector::MakeCALL(tick,Tick);
    nativeFrame.fun=injector::MakeCALL(phase,BeforeUI).get();
    safetymips::Options options;options.preserve=PSP_HOOK_SAVE_FPU;
    particleHook=safetymips::create_mid(particle,[](SafetyMipsContext& regs) {
        Particle(Data(),viceCity ? vcsParticles : lcsParticles,unsigned(regs.a0),reinterpret_cast<const Vec3*>(uintptr_t(regs.a1)));
    },options);
    return console::portable::Finish()==0;
}
}
