#include "stdafx.h"
#include "FixSettings.hpp"
#include "Shared/WidescreenFixConflict.hpp"

// STUB: The Spyro Reignited Trilogy widescreen fix (black bar removal, aspect ratio / FOV hooks,
// camera continuation hook) was authored by REDACTED and has been removed.

void Init()
{
    [[maybe_unused]] auto& settings = FixSettings();
    asi_settings::LiveLink::Start();

    // STUB: removed code authored by REDACTED
}

ASI_SETTINGS_EXPORT(FixSettings())

CEXP void InitializeASI()
{
    // STUB: removed code authored by REDACTED
    if (WidescreenFixConflict::Found())
        return;
    CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("E8 ? ? ? ? 0F B7 D8 E8 ? ? ? ? 4C 8B"));
}

BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD /*reason*/, LPVOID /*lpReserved*/)
{
    // STUB: removed code authored by REDACTED
    return TRUE;
}
