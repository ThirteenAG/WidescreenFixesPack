#pragma once

namespace FrameRate
{
inline int limit{ 60 };
inline injector::hook_back<int(uintptr_t, uint8_t)> frame;

inline int Pace(uintptr_t renderer, uint8_t nativeWait)
{
    // The native path already waits for a vblank when nativeWait is set.
    // Do not constrain the dedicated fast-loading path.
    if (!fastForward)
    {
        const int waits = (limit == 30 ? 2 : 1) - (nativeWait ? 1 : 0);
        for (int i = 0; i < waits; ++i)
            sceDisplayWaitVblankStart();
    }
    return frame.fun(renderer, nativeWait);
}
inline void ReadOptions()
{
    limit = std::clamp(inireader.ReadInteger("MAIN", "FPSLimit", 60), 30, 60);
}
inline void Install()
{
    const auto call = sites::Required("FF 00 DE 33 ? ? ? ? 25 20 20 02 25 20 C0 02 ? ? ? ? 25 28 C0 03 03 00 C4 92 03 00 80 10", 16);
    frame.fun = injector::MakeCALL(call, Pace).get();
}
}
