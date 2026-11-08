module;

#include <stdafx.h>

export module FrameRate;

import ComVars;

export namespace BetterSync
{
    // STUB: the BetterSync frame limiter authored by REDACTED (an adaptation of ThirteenAG's d3d8-wrapper
    // FrameLimiter) has been removed. The settings interface below is kept because other code depends on it.
    float fFPSLimit = 60.0f;
    struct FrameLimiter
    {
        enum FPSLimitMode { FPS_NONE, FPS_REALTIME, FPS_ACCURATE };
    };
    FrameLimiter::FPSLimitMode mFPSLimitMode = FrameLimiter::FPS_ACCURATE;

    uintptr_t sub_6C4FC0 = 0;
    bool bEnabled = false;

    // Once per frame, where the game waits for its next frame
    void CustomSyncFunc()
    {
        WFP::onGameProcessEvent().executeAll();
        // STUB: removed code authored by REDACTED
    }
}
