#include "Game.hpp"
namespace rain {
namespace {
StoryProfile profile;
bool viceCity;
uintptr_t TheCamera,dword_729480,dword_486DE0,dword_4CD144,dword_4CD680,dword_4CD684,dword_48F820,dword_489F7C;
SafetyMipsMid particleHook;
pcsx2::GameCallback<void()> tickEntry,phaseEntry;
injector::hook_back<void()> nativeFrame;
void Tick() { if (Ready()) Update(Data(),profile,0x00100000,0x02000000); }
void BeforeUI() { PCSX2F_GuestBeforeUIDraw();nativeFrame.fun(); }
}
bool Stories() {
    uintptr_t tick=pattern.get(0,"52 00 02 3C ? ? ? ? ? ? ? ? ? ? ? ? 00 00 00 00 ? ? ? ? 00 00 00 00 ? ? ? ? 48 00 10 3C",-8);
    viceCity=tick!=0;
    uintptr_t particle=0,render=0;
    if (viceCity) {
        uintptr_t ptr_471414 = pattern.get(0, "73 00 10 3C ? ? ? ? 10 00 BF FF", 0);
        dword_729480 = GetAbsoluteAddress(ptr_471414, 0, 16);
        uintptr_t ptr_21D0BC = pattern.get(0, "48 00 02 3C ? ? ? ? ? ? ? ? 48 00 13 3C ? ? ? ? 2D B0 40 00", -0);
        dword_486DE0 = GetAbsoluteAddress(ptr_21D0BC, 0, 8);
        uintptr_t ptr_2B36C0 = pattern.get(0, "4D 00 02 3C CC 3D 01 3C CD CC 21 34 00 08 81 44 ? ? ? ? ? ? ? ? 00 00 B0 FF 2D 30 80 00", -0);
        dword_4CD144 = GetAbsoluteAddress(ptr_2B36C0, 0, 16);
        uintptr_t ptr_40CEE0 = pattern.get(0, "4D 00 04 3C ? ? ? ? 2D 88 A0 00", -0);
        dword_4CD680 = GetAbsoluteAddress(ptr_40CEE0, 0, 16);
        uintptr_t ptr_40CF80 = pattern.get(0, "4D 00 03 3C ? ? ? ? D0 44 42 26", -0);
        dword_4CD684 = GetAbsoluteAddress(ptr_40CF80, 0, 4);
        uintptr_t ptr_1DF68C = pattern.get(0, "49 00 10 3C ? ? ? ? ? ? ? ? 1C 00 43 8C ? ? ? ? 00 00 00 00 ? ? ? ? 1C 00 43 8C ? ? ? ? D0 01 B0 DF", -0);
        dword_48F820 = GetAbsoluteAddress(ptr_1DF68C, 0, 4);
        uintptr_t ptr_113FB8 = pattern.get(0, "49 00 02 3C AA 02 64 94", -0);
        dword_489F7C = GetAbsoluteAddress(ptr_113FB8, 0, 8);
        uintptr_t ptr_1091D4 = pattern.get(0, "6F 00 02 3C 00 00 B0 FF 08 00 B1 FF ? ? ? ? 2D 88 80 00", -0);
        TheCamera = GetAbsoluteAddress(ptr_1091D4, 0, 12);
        profile={TheCamera,0x7F4,dword_729480,0x40,dword_486DE0,dword_4CD144,dword_4CD680,dword_4CD684,dword_48F820,dword_489F7C,0x20,true};
        particle=pattern.get(0,"D0 00 B4 7F E0 00 B5 7F 2D A0 A0 00",-8);
        // Resolve Render2DStuff and hook its single caller. The widescreen fix
        // (SLUS-21590 only) hooks the entry before this plugin loads, so fall
        // back to that release's address when the entry bytes were replaced.
        render=pattern.get(0,"04 00 04 24 48 00 B5 FF 2D 28 00 00",-4);
        if (!render) render=0x21F348;
    } else {
        tick=pattern.get(0,"2D 20 40 00 63 00 04 3C",-4);
        if (!tick) return false;
        uintptr_t ptr_101E04 = pattern.get(0, "44 00 02 3C 10 00 B0 FF ? ? ? ? 18 00 B1 FF", -0);
        TheCamera = GetAbsoluteAddress(ptr_101E04, 0, 8);
        profile={TheCamera,0xBA0,0x63474D,0,0,0x3D9B94,0x3DA200,0x3DA1FC,0x3D5AC0,0x3D8430,0,false};
        particle=pattern.get(0,"E0 00 B5 7F D0 00 B4 7F 2D A8 80 00",-12);
        render=pattern.get(0,"04 00 04 24 20 00 B0 FF 2D 28 00 00",-4);
        if (!render) render=0x1F6338;
    }
    uintptr_t phase=FrameCall(render);
    if (!profile.camera || !profile.menu || !profile.rain || !profile.cameraNoRain || !profile.playerNoRain || !profile.cutscene || !profile.area || !particle || !phase) return false;
    if (injector::InitializeCheckedRuntime()!=PCSX2_HOOK_OK) return false;
    tickEntry.bind(Tick);injector::MakeCALL(tick,tickEntry.address());
    phaseEntry.bind(BeforeUI);nativeFrame.fun=injector::MakeCALL(phase,phaseEntry.address()).get();
    safetymips::Options options;options.preserve=PCSX2_HOOK_SAVE_FPU;
    particleHook=safetymips::create_mid(particle,[](SafetyMipsContext& regs) {
        if (Ready()) Particle(Data(),viceCity ? vcsParticles : lcsParticles,unsigned(regs.a0),reinterpret_cast<const Vec3*>(uintptr_t(regs.a1)));
    },options);
    return injector::FlushCaches()==PCSX2_HOOK_OK;
}
}
