#pragma once

namespace FrameRate
{
inline int limit{ 60 };
inline injector::hook_back<int(uintptr_t, uint8_t)> frame;
inline int Pace(uintptr_t renderer, uint8_t nativeWait)
{
    Geometry::worldSeen = false;
    Graphics::barsDrawn = false;
    if (!fastForward && limit == 30)
        sceDisplayWaitVblankStart();
    return frame.fun(renderer, nativeWait);
}
inline void Install()
{
    const auto site = sites::Required("? ? 04 3C 01 00 05 34 ? ? ? ? ? ? 84 24 ? ? 04 3C ? ? 84 8C 00 00 12 34 7C 01 85 8C D8 00 84 8C 03 00 B1 38");
    frame.fun = injector::MakeCALL(site + 8, Pace).get();
}
}
