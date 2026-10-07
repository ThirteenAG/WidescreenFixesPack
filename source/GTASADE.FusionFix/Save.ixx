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
    thread_local bool quicksaveSucceeded = false;
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
            const auto saveAddress = hook::pattern("40 53 48 83 EC 20 45 33 C0 8B DA E8 ? ? ? ? 0F B6 D3 E8 ? ? ? ?").get_first();
            const auto helpAddress = hook::pattern("48 89 5C 24 10 48 89 74 24 18 48 89 7C 24 20 55 41 56 41 57 48 8D AC 24 50 FF FF FF 48 81 EC B0 01 00 00 48 8B 05 ? ? ? ? 48 33 C4 48 89 85 A0 00 00 00 48 63 59 08").get_first();
            const auto clockCall = hook::pattern("E8 ? ? ? ? 0F B6 15 ? ? ? ? 4C 8D 05 ? ? ? ? F3 0F 10 05 ?").get_first();
            const auto saveResult = hook::pattern("40 53 48 83 EC 20 45 33 C0 8B DA E8 ? ? ? ? 0F B6 D3 E8 ? ? ? ?").get_first(24);
            if (!saveAddress || !helpAddress || !clockCall || !saveResult)
                return;
            const MissionState mission{reinterpret_cast<const uint8_t*>((injector::ReadRelativeOffset(hook::pattern("4C 8D 0D ? ? ? ? 42 8B 4C 0F F9 81 E1 FF FF FF 00 81 F9 50 00 01 00").get_first<uint8_t>(3)).as_int())),
                reinterpret_cast<const uint32_t*>((injector::ReadRelativeOffset(hook::pattern("8B 05 ? ? ? ? 40 32 F6 F3 0F 11 55 88 45 0F B6 F1 44 0F 28 F1 48 8B").get_first<uint8_t>(2)).as_int())), 339000};
            const auto widescreen = reinterpret_cast<const uint8_t*>((injector::ReadRelativeOffset(hook::pattern("80 3D ? ? ? ? 01 48 8D 1D ? ? ? ? 0F 94 05 ? ? ? ? 66 90 48").get_first<uint8_t>(2)).as_int() + 1));
            const auto menuCount = reinterpret_cast<const uint32_t*>((injector::ReadRelativeOffset(hook::pattern("39 3D ? ? ? ? C7 05 ? ? ? ? 05 00 00 00 0F 8F ? ? ? ? 48 39").get_first<uint8_t>(2)).as_int()));
            const auto pendingMenu = reinterpret_cast<const uint64_t*>((injector::ReadRelativeOffset(hook::pattern("48 39 3D ? ? ? ? 0F 85 ? ? ? ? 48 8D 0D ? ? ? ? 40 88 3D ?").get_first<uint8_t>(3)).as_int()));
            const auto saveToSlot = reinterpret_cast<uintptr_t(*)(uintptr_t, int)>(saveAddress);
            const auto showHelp = reinterpret_cast<void(*)(FString*, uintptr_t, bool, bool)>(helpAddress);
            static auto clockHook = safetyhook::create_mid(clockCall, [](SafetyHookContext& context)
            {
                // Normal saves retain the native six-hour advance. Only this
                // quicksave invocation uses the existing one-minute behavior.
                if (quicksaveInProgress)
                    context.rcx = 1;
            });
            static auto resultHook = safetyhook::create_mid(saveResult, [](SafetyHookContext& context)
            {
                // Native SaveSlot returns zero on success and 2 on failure;
                // the helper's final CPad::Update return is unrelated.
                if (quicksaveInProgress)
                    quicksaveSucceeded = (context.rax & 0xff) == 0;
            });
            if (!clockHook || !resultHook)
            {
                clockHook = {};
                resultHook = {};
                return;
            }
            RegisterQuicksave([mission, widescreen, menuCount, pendingMenu]()
            {
                return !mission.IsOnMission() && !*widescreen && !*menuCount && !*pendingMenu;
            }, [saveToSlot, showHelp]()
            {
                SaveScope scope;
                quicksaveSucceeded = false;
                saveToSlot(0, Settings.saveSlot);
                if (!quicksaveSucceeded)
                    return;
                FString key;
                key.Data = const_cast<wchar_t*>(L"FESZ_WR");
                key.Count = key.Max = 8;
                showHelp(&key, key.Count, false, true);
            });
        };
    }
} SaveModuleInstance;
