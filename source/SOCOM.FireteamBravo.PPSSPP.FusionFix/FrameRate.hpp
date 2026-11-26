#pragma once

namespace FrameRate
{
inline int limit{ 60 };
inline injector::hook_back<int(uintptr_t, uint8_t)> frame;
inline int Pace(uintptr_t renderer, uint8_t nativeWait)
{
    static uint32_t lastPresent{UINT32_MAX};
    Geometry::worldSeen = false;
    Graphics::barsDrawn = false;
    if (dualAnalog)
        Stick::host.Poll(sceKernelGetSystemTimeLow());
    // The native present waits for the next vblank. Add one only when no
    // vblank has elapsed since the previous present.
    if (!fastForward && limit == 30 && nativeWait && sceDisplayGetVcount() == lastPresent)
        sceDisplayWaitVblankStart();
    const int result = frame.fun(renderer, nativeWait);
    lastPresent = sceDisplayGetVcount();
    return result;
}
inline void Install()
{
    const auto site = sites::Required("? ? 04 3C 01 00 05 34 ? ? ? ? ? ? 84 24 ? ? 04 3C ? ? 84 8C 00 00 12 34 7C 01 85 8C D8 00 84 8C 03 00 B1 38");
    frame.fun = injector::MakeCALL(site + 8, Pace).get();
}
}
