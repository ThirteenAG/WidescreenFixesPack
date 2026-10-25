module;

#include <stdafx.h>
#include "../Shared/PatternPreflight.hpp"

export module NOSTrailFix;

import ComVars;

// The NOS trail is drawn from the car's movement during one simulation step, so it got shorter as the
// simulation rate went up. It's scaled to the length of a 1/60 s step.

class NOSTrailFix
{
public:
    NOSTrailFix()
    {
        WFP::onInitEventAsync() += []()
        {
            [&]()
            {
                if (!PatternPreflight::Check("NFSCarbon NOSTrailFix", {
                    { "EB ? 8D 9B ? ? ? ? 40", 1, false },
                }))
                    return;

                auto pattern = hook::pattern("EB ? 8D 9B ? ? ? ? 40");
                injector::MakeNOP(pattern.get_first(0), 8, true);
                static auto NOSTrailHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    auto worldPos1 = reinterpret_cast<bVector3*>(regs.edx);
                    auto worldPos2 = reinterpret_cast<bVector3*>(regs.esi);
                    regs.edx = regs.esp + 0x40;
                    auto flarePos = reinterpret_cast<bVector3*>(regs.edx);

                    float scale = static_cast<float>(SimRate) / 60.0f;
                    flarePos->x = (worldPos1->x - worldPos2->x) * scale;
                    flarePos->y = (worldPos1->y - worldPos2->y) * scale;
                    flarePos->z = (worldPos1->z - worldPos2->z) * scale;

                    regs.eax = 0;
                });
            }();

        };
    }
} NOSTrailFix;
