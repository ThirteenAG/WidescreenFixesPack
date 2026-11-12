module;
#include "stdafx.h"
#include "FixSettings.hpp"

export module Fixes;
import ComVars;

// WeaponStatePropTemplate: side minimum at +0x60, maximum at +0x64.
// GetBarrelCreepSideMax in the shipped source returns the minimum by mistake.
float __fastcall GetBarrelCreepSideMax(void* weapon, void*)
{
    return Game::Field<float>(Game::Field<void*>(weapon, 0x150), 0x64);
}

class Fixes
{
public:
    Fixes()
    {
        WFP::onInitEvent() += []()
        {
            auto& settings = FixSettings();
            auto bSkipIntro = settings.Bool("MAIN.SkipIntro");

            // ControllerDirectInput's constructor spins on DIERR_OTHERAPPHASPRIO while
            // unfocused, preventing the main thread from pumping window messages.
            // Keep the initial Acquire; Update already retries it once per frame and
            // stops on failure, so focus can be acquired normally after background startup.
            auto pattern = hook::pattern("FF 51 1C 3D 05 00 07 80 74 ED 8B 95 ? ? ? ? 03 D2 03 D2"); // 0x6DDB9B + 8
            injector::MakeNOP(pattern.get_first(8), 2);

            if (bSkipIntro)
            {
                auto pattern = hook::pattern("0F 85 ? ? ? ? 6A ? 6A ? 6A ? 6A ? 68 ? ? ? ? E8 ? ? ? ? 83 C4 ? 5E");
                injector::MakeNOP(pattern.get_first(), 6); // Legal
                pattern = hook::pattern("E8 ? ? ? ? 83 C4 20 5F 5E C7 45");
                injector::MakeNOP(pattern.get_first(), 5); // Movies.
            }
            if (settings.Bool("MAIN.SkipPressStart"))
            {
                // The game goes past the start screen by itself once Start has been pressed in the session;
                // treat it as pressed. Running the profile check inside the start screen state instead
                // repeats it afterwards, and the profile then fails to load.
                pattern = hook::pattern("74 07 C7 46 68 00 00 00 00");
                injector::MakeNOP(pattern.get_first(), 2);
            }

            // First pushed argument is MAX; the second call still supplies MIN.
            auto recoilPattern = hook::pattern("E8 ? ? ? ? 51 8B CE ? ? ? E8 ? ? ? ? 51 8B 0D ? ? ? ? ? ? ? E8 ? ? ? ? ? ? ? ? ? ? 8B 0D");
            injector::MakeCALL(recoilPattern.get_first(), GetBarrelCreepSideMax, true);
        };
    }
} Fixes;
