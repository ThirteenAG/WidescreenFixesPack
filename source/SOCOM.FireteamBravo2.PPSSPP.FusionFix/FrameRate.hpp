#pragma once

namespace FrameRate
{
inline int limit{ 60 };
inline injector::hook_back<int(uintptr_t, uint8_t)> frame;

inline int Pace(uintptr_t renderer, uint8_t nativeWait)
{
    static uint32_t lastPresent{ UINT32_MAX };
    if (options.dualAnalog)
        Stick::host.Poll(sceKernelGetSystemTimeLow());
    if (!fastForward)
    {
        // Count vblanks already elapsed and the native wait toward the cap.
        const unsigned waits = (limit == 30 ? 2u : 1u) - (nativeWait ? 1u : 0u);
        const uint32_t elapsed = lastPresent == UINT32_MAX ? 0u : sceDisplayGetVcount() - lastPresent;
        for (uint32_t i = elapsed; i < waits; ++i)
            sceDisplayWaitVblankStart();
    }
    const int result = frame.fun(renderer, nativeWait);
    lastPresent = sceDisplayGetVcount();
    return result;
}
inline void Install()
{
    const auto call = sites::Required("FF 00 DE 33 ? ? ? ? 25 20 20 02 25 20 C0 02 ? ? ? ? 25 28 C0 03 03 00 C4 92 03 00 80 10", 16);
    frame.fun = injector::MakeCALL(call, Pace).get();
}
}
