module;

#include <stdafx.h>

export module NOSTrailFix;

import ComVars;

float fCustomNOSTrailLength = 1.0f;
SafetyHookInline shUpdateCarStreakingFlares = {};
void __cdecl UpdateCarStreakingFlares(void* eView)
{
    static float accumulator = 0.0f;
    constexpr float kBaseStreakUpdateInterval = 1.0f / 60.0f;
    float effectiveInterval = kBaseStreakUpdateInterval * fCustomNOSTrailLength;

    accumulator += actualDeltaTime;

    if (accumulator >= effectiveInterval)
    {
        accumulator -= effectiveInterval;
        return shUpdateCarStreakingFlares.unsafe_ccall(eView);
    }
}

class NOSTrailFix
{
public:
    NOSTrailFix()
    {
        WFP::onInitEventAsync() += []()
        {
            CIniReader iniReader("");
            bool bFixNOSTrailLength = iniReader.ReadInteger("NOSTrail", "FixNOSTrailLength", 1) == 1;
            fCustomNOSTrailLength = iniReader.ReadFloat("NOSTrail", "CustomNOSTrailLength", 1.0f);

            if (bFixNOSTrailLength)
            {
                auto pattern = hook::pattern("E8 ? ? ? ? 8B 15 ? ? ? ? 8B 47");
                shUpdateCarStreakingFlares = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), UpdateCarStreakingFlares); // 0x61AD80
            }
        };
    }
} NOSTrailFix;