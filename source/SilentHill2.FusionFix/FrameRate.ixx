module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/SilentHill/Display.hpp"


export module FrameRate;


export namespace SH2FrameRate
{
    inline int64_t __stdcall NativeBudget(uint32_t, uint32_t, uint32_t, uint32_t)
    {
        return 0;
    }

    inline void Apply()
    {
        int limit = FixSettings().Int("MAIN.FPSLimit");
        SHDisplay::SetFrameLimit(limit < 0 ? GetDesktopRefreshRate() : limit);
    }

    inline void Init()
    {
        auto timer = hook::pattern("6A 00 6A 1E 50 51 E8 ? ? ? ? A3 ? ? ? ? 89 15 ? ? ? ? C7 05");
        // Keep the native elapsed-time calculation; presentation supplies the precise wait.
        injector::MakeCALL(timer.get_first(6), &NativeBudget, true);
        Apply();
        FixSettings().OnChange("MAIN.FPSLimit", &Apply);
    }
}
