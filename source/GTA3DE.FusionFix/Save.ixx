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
            const auto saveAddress = hook::pattern("40 53 48 81 EC 30 08 00 00 48 8B 05 ? ? ? ? 48 33 C4 48 89 84 24 20 08 00 00 83 3D ? ? ?").get_first();
            const auto helpAddress = hook::pattern("48 89 74 24 20 57 48 83 EC 40 80 3D ? ? ? ? 01 0F B6 F2 48 8B F9 0F").get_first();
            const auto callAddress = hook::pattern("E8 ? ? ? ? 48 8D 15 ? ? ? ? C7 05 ? ? ? ? 00 00 00 00 48 8D").get_first();
            if (!saveAddress || !helpAddress || !callAddress)
                return;
            const MissionState mission{reinterpret_cast<const uint8_t*>((injector::ReadRelativeOffset(hook::pattern("4C 8D 0D ? ? ? ? 42 8B 4C 0F F9 81 E1 FF FF FF 00 81 F9 50 00 01 00").get_first<uint8_t>(3)).as_int())),
                reinterpret_cast<const uint32_t*>((injector::ReadRelativeOffset(hook::pattern("8B 05 ? ? ? ? 85 C0 74 ? 42 83 BC 20 70 04 FD 04 01 0F 84 ? ? ?").get_first<uint8_t>(2)).as_int())), 163840};
            const auto saveForPause = reinterpret_cast<bool(*)(int)>(saveAddress);
            const auto showHelp = reinterpret_cast<void(*)(FString*, bool)>(helpAddress);
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
                    key.Data = const_cast<wchar_t*>(L"FESZ_L1");
                    key.Count = key.Max = 8;
                    showHelp(&key, false);
                }
            });
        };
    }
} SaveModuleInstance;
