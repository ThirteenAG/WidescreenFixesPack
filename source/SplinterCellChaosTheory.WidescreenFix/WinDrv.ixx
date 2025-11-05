module;

#include <stdafx.h>

export module WinDrv;

import ComVars;
import Xidi;

namespace UWindowsViewport
{
    SafetyHookInline shUpdateRumble = {};
    void __fastcall UpdateRumble(void* UWindowsViewport, void* edx, float vibrateStrength, float shakeStrength)
    {
        if (!XidiSendVibration || bVibration)
            return;

        constexpr float RUMBLE_MIN_STRENGTH = 0.0f;
        constexpr float RUMBLE_MAX_STRENGTH = 1.0f;
        constexpr WORD RUMBLE_MAX_VALUE = 0xFFFF;

        const WORD left = static_cast<WORD>(std::clamp(vibrateStrength, RUMBLE_MIN_STRENGTH, RUMBLE_MAX_STRENGTH) * RUMBLE_MAX_VALUE);
        const WORD right = static_cast<WORD>(std::clamp(shakeStrength, RUMBLE_MIN_STRENGTH, RUMBLE_MAX_STRENGTH) * RUMBLE_MAX_VALUE);

        XidiSendVibration(-1, left, right);
    }
}

namespace AEPlayerController
{
    SafetyHookInline shbIsInRumbleMode = {};
    bool __fastcall bIsInRumbleMode(void* AEPlayerController, void* edx)
    {
        return true;
    }
}

export void InitWinDrv()
{
    auto pattern = hook::pattern("C7 06 ? ? ? ? C7 46 ? ? ? ? ? C7 46 ? ? ? ? ? 89 9E F4 01 00 00");
    auto UWindowsViewportVTtable = *pattern.get_first<uint32_t*>(2);
    UWindowsViewport::shUpdateRumble = safetyhook::create_inline(UWindowsViewportVTtable[25], UWindowsViewport::UpdateRumble); //-0x64

    pattern = hook::pattern("C7 06 ? ? ? ? E8 ? ? ? ? 8B 86 1C 08 00 00");
    auto APlayerControllerVTable = *pattern.get_first<uint32_t*>(2);
    AEPlayerController::shbIsInRumbleMode = safetyhook::create_inline(APlayerControllerVTable[178], AEPlayerController::bIsInRumbleMode); //0x2c8
}
