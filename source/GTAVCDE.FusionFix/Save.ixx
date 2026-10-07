module;

#include "stdafx.h"

export module Save;

import Build;
import Settings;
import Unreal;
import SaveCommon;

namespace
{
    thread_local bool quicksaveInProgress = false;
    struct SaveScope
    {
        bool previous = std::exchange(quicksaveInProgress, true);
        ~SaveScope() { quicksaveInProgress = previous; }
    };
}

class SaveModule
{
public:
    SaveModule()
    {
        WFP::onInitEvent() += []()
        {
            const auto saveAddress = hook::pattern("40 53 48 81 EC 30 08 00 00 48 8B 05 ? ? ? ? 48 33 C4 48 89 84 24 20 08 00 00 44 8B 05 ? ?").get_first();
            const auto helpAddress = hook::pattern("4C 8B DC 55 41 56 41 57 49 8D AB 58 FF FF FF 48 81 EC 90 01 00 00 48 8B").get_first();
            const auto callAddress = hook::pattern("E8 ? ? ? ? B0 01 C7 05 ? ? ? ? 00 00 00 00 E9 ? ? ? ? 48 83").get_first();
            if (!saveAddress || !helpAddress || !callAddress)
                return;
            const MissionState mission{reinterpret_cast<const uint8_t*>((injector::ReadRelativeOffset(hook::pattern("4C 8D 0D ? ? ? ? 42 8B 44 0F F9 25 FF FF FF 00 3D 50 00 01 00 EB ?").get_first<uint8_t>(3)).as_int())),
                reinterpret_cast<const uint32_t*>((injector::ReadRelativeOffset(hook::pattern("8B 0D ? ? ? ? 8B 84 39 E0 7A 04 05 33 FF 85 C9 74 ? FF C8 83 F8 01").get_first<uint8_t>(2)).as_int())), 260512};
            const auto saveForPause = reinterpret_cast<bool(*)(int)>(saveAddress);
            const auto showHelp = reinterpret_cast<void(*)(FString*, bool, int, int, int)>(helpAddress);
            static auto slotHook = safetyhook::create_mid(callAddress, [](SafetyHookContext& context)
            {
                if (quicksaveInProgress)
                    context.rdx = Settings.saveSlot;
            });
            if (!slotHook)
                return;
            RegisterQuicksave([mission]() { return !mission.IsOnMission(); }, [saveForPause, showHelp]()
            {
                SaveScope scope;
                // 5 is the native quicksave reason. The slot is a separate
                // argument to the storage call intercepted above.
                if (saveForPause(5))
                {
                    FString key;
                    key.Data = const_cast<wchar_t*>(L"FESZ_WR");
                    key.Count = key.Max = 8;
                    showHelp(&key, false, 0, 0, 0);
                }
            });
        };
    }
} SaveModuleInstance;
