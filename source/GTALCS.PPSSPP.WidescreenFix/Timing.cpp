#include "Game.hpp"
namespace lcsws {
namespace {
injector::hook_back<void(int,float,float,float)> audioService;
bool audioFrame;
uintptr_t limiter;
uint32_t originalLimiter;
bool limited = true;
void AudioService(int entity, float a, float b, float c) {
    audioFrame=!audioFrame;
    if (audioFrame || !settings.fps) audioService.fun(entity,a,b,c);
}
void SetLimiter(bool enable) {
    if (limited==enable) return;
    // PPSSPP keeps JIT block markers in compiled code; invalidation restores
    // the original word before it is compared.
    sceKernelIcacheInvalidateRange(reinterpret_cast<const void*>(limiter),4);
    if (injector::ReadMemory<uint32_t>(limiter)!=(limited ? originalLimiter : 0)) return;
    if (injector::WriteMemory<uint32_t>(limiter,enable ? originalLimiter : 0)==PSP_HOOK_OK) {
        limited=enable;
        (void)injector::FlushCaches();
    }
}
void Tick() {
    TickMenu();
    if (!settings.unthrottle) console::portable::Unthrottle(false);
    else {
        bool menu=*reinterpret_cast<uint8_t*>(Address<0x8B8EE20>()+0x131)!=0;
        float black=*reinterpret_cast<float*>(Address<0x8B490E4>());
        console::portable::Unthrottle(!menu && black!=0);
    }
}
}
void ApplyFrameRate() { SetLimiter(!settings.fps); }
void InstallTiming() {
    injector::MakeCALL(Address<0x89C4E70>(),Tick);
    if (settings.unthrottle) console::portable::Unthrottle(true);
    // The frame rate can change in the Display menu. Simulation uses its
    // native time step; streamed audio keeps its original 30 Hz cadence at 60 FPS.
    limiter=Address<0x8ab338c>();
    originalLimiter=injector::ReadMemory<uint32_t>(limiter);
    if (settings.fps) { injector::MakeNOP(limiter); limited=false; }
    audioService.fun=injector::MakeCALL(Address<0x88646f4>(),AudioService).get();
}
}
