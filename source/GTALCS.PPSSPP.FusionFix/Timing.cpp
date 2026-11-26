#include "Game.hpp"
namespace lcsws {
namespace {
injector::hook_back<void(int,float,float,float)> audioService;
bool audioFrame;
uintptr_t limiter;
uint32_t originalLimiter;
bool limited = true;
bool desiredLimited = true;
unsigned rateRetry;
void AudioService(int entity, float a, float b, float c)
{
    if (!limited)
    {
        audioFrame = !audioFrame;
        if (!audioFrame)
            return;
    }
    audioService.fun(entity, a, b, c);
}

bool SetLimiter(bool enable)
{
    // JIT invalidation restores the native instruction in compiled blocks.
    sceKernelIcacheInvalidateRange(reinterpret_cast<const void*>(limiter), 4);
    if (injector::ReadMemory<uint32_t>(limiter) != (limited ? originalLimiter : 0))
        return false;
    if (injector::WriteMemory<uint32_t>(limiter, enable ? originalLimiter : 0) != PSP_HOOK_OK)
        return false;
    limited = enable;
    audioFrame = false;
    return true;
}
void Tick() {
    if (rateRetry) --rateRetry;
    TickMenu();
    PollSettings();
    ApplyFrameRate();
    TickCheats();
    if (!settings.unthrottle) console::portable::Unthrottle(false);
    else {
        bool menu=*reinterpret_cast<uint8_t*>(Address<0x8B8EE20>()+0x131)!=0;
        float black=*reinterpret_cast<float*>(Address<0x8B490E4>());
        console::portable::Unthrottle(!menu && black!=0);
    }
}
}
void ApplyFrameRate()
{
    const bool next = !settings.fps;
    if (next != desiredLimited)
    {
        desiredLimited = next;
        rateRetry = 0;
    }
    if (limited != desiredLimited && !rateRetry && !SetLimiter(desiredLimited))
    {
        rateRetry = 60;
        logger.Write("Frame limiter patch rejected. Retrying during gameplay.\n");
    }
}
void InstallTiming() {
    injector::MakeCALL(Address<0x89C4E70>(),Tick);
    if (settings.unthrottle) console::portable::Unthrottle(true);
    // The frame rate can change in the Display menu. Simulation uses its
    // native time step; streamed audio keeps its original 30 Hz cadence at 60 FPS.
    limiter=Address<0x8ab338c>();
    originalLimiter=injector::ReadMemory<uint32_t>(limiter);
    ApplyFrameRate();
    audioService.fun=injector::MakeCALL(Address<0x88646f4>(),AudioService).get();
}
}
