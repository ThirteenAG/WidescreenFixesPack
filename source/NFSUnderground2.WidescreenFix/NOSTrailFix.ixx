module;

#include <stdafx.h>

export module NOSTrailFix;

import ComVars;

// The NOS trail is drawn between the car's positions of the last frames, which UpdateCarStreakingFlares
// records once per frame (it's called for every view RenderCars draws, and returns when the frame counter
// it saw last hasn't changed). At higher frame rates the cars moved less between frames and the trail got
// shorter. Record positions 60 times per second instead, counting each frame's time once even when the
// rear view mirror is drawn too.
namespace
{
    uint32_t* eFrameCounter = nullptr;

    SafetyHookInline shUpdateCarStreakingFlares = {};
    void __cdecl UpdateCarStreakingFlares(void* eView)
    {
        static float accumulator = 0.0f;
        static uint32_t lastFrame = UINT32_MAX;
        constexpr float kBaseStreakUpdateInterval = 1.0f / 60.0f;

        if (*eFrameCounter == lastFrame)
            return;
        lastFrame = *eFrameCounter;

        accumulator += actualDeltaTime;
        if (accumulator >= kBaseStreakUpdateInterval)
        {
            accumulator = std::fmod(accumulator, kBaseStreakUpdateInterval);
            shUpdateCarStreakingFlares.unsafe_ccall(eView);
        }
    }
}

class NOSTrailFix
{
public:
    NOSTrailFix()
    {
        WFP::onInitEventAsync() += []()
        {
            // RenderCars -> UpdateCarStreakingFlares, which starts with mov eax, [eFrameCounter]
            auto pattern = hook::pattern("E8 ? ? ? ? 8B 15 ? ? ? ? 8B 47");
            auto function = injector::GetBranchDestination(pattern.get_first()).as_int();
            if (*reinterpret_cast<uint8_t*>(function) != 0xA1)
                return;

            eFrameCounter = *reinterpret_cast<uint32_t**>(function + 1);
            shUpdateCarStreakingFlares = safetyhook::create_inline(function, UpdateCarStreakingFlares);
        };
    }
} NOSTrailFix;
