module;

#include "stdafx.h"
#include "FixSettings.hpp"

export module Echelon;

import ComVars;
import Cinematics;

namespace AEPlayerController
{
    SafetyHookInline shTick = {};
    void UpdateState(void* controller, void* edx)
    {
        if (!controller)
            return;

        wchar_t buffer[256];
        auto& state = UObject::controllerState;
        if (state.controller != controller)
        {
            const std::wstring_view fullName = UObject::GetFullName(controller, edx, buffer);
            const auto type = fullName.substr(0, fullName.find(L' '));
            if (type != L"EPlayerController" && type != L"ECoopPlayerController")
                return;
        }

        const auto frame = reinterpret_cast<uintptr_t*>(controller)[3];
        const auto node = frame ? *reinterpret_cast<const uintptr_t*>(frame + 0x1C) : 0;
        if (!node)
        {
            UObject::controllerState = {};
            return;
        }

        const auto id = *reinterpret_cast<const int*>(node + 0x20);
        if (state.controller != controller || state.node != node || state.id != id)
        {
            const auto object = UObject::FindState(controller, edx, id);
            if (!object)
            {
                state = {};
                return;
            }
            const std::wstring_view name = UObject::GetFullName(object, edx, buffer);
            state.name = name.substr(name.rfind(L'.') + 1);
            state.controller = controller;
            state.node = node;
            state.id = id;
        }
    }

    int __fastcall Tick(void* playerController, void* edx, float delta, int tickType)
    {
        UpdateState(playerController, edx);
        return shTick.unsafe_fastcall<int>(playerController, edx, delta, tickType);
    }
}

namespace AEPattern
{
    SafetyHookInline shTick = {};
    int __fastcall Tick(void* AEPattern, void* edx, float a2, int a3)
    {
        std::wstring CameraShot = L"";
        void* shot = nullptr;

        if (AEPattern)
        {
            auto AEchelonLevelInfo = *((uintptr_t*)AEPattern + 0x81);
            if (AEchelonLevelInfo)
            {
                auto ptr = *((uintptr_t*)AEchelonLevelInfo + 0x4F9);
                if (ptr)
                {
                    shot = reinterpret_cast<void*>(ptr);
                    CameraShot = GetFName((void*)(ptr + 0x38), 0);
                }
            }
        }

        Cinematics::Shot(shot, CameraShot == L"Take_01_EnterSWAT" ? 4.0f / 3.0f : 16.0f / 9.0f);
        gCurrentCameraShotName = CameraShot;

        return shTick.unsafe_fastcall<int>(AEPattern, edx, a2, a3);
    }
}

export void InitEchelon()
{

    gBlacklistIndicators = FixSettings().Int("BONUS.BlacklistIndicators");
    auto bEnableRunDuringForcedWalk = FixSettings().Int("MISC.EnableRunDuringForcedWalk") != 0;
    bDisablePreCache = FixSettings().Int("MAIN.DisablePreCache") != 0;

    #if _DEBUG
    bDisablePreCache = true;
    #endif

    if (gBlacklistIndicators || bLightSyncRGB)
    {
        auto pattern = hook::module_pattern(GetModuleHandle(L"Echelon"), "8B 96 ? ? ? ? 8D 0C 80");
        struct BlacklistIndicatorsHook
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.edx = *(uint32_t*)(regs.esi + 0x15B8);
                regs.ecx = regs.eax + regs.eax * 4;

                if (regs.ecx == 0)
                {
                    auto fPlayerLuminosity = *(float*)(regs.edx + regs.ecx * 4 + 4);
                    if (fPlayerLuminosity > 120.0f) fPlayerLuminosity = 120.0f;

                    auto v = 120.0f - fPlayerLuminosity;
                    if (v <= 10.0f) v = 10.0f;
                    const float visibility = v / 120.0f;
                    gVisibility.store(gBlacklistIndicators == 2 ? 1.0f - visibility : visibility, std::memory_order_relaxed);
                }
            }
        }; injector::MakeInline<BlacklistIndicatorsHook>(pattern.get_first(0), pattern.get_first(6));

        pattern = hook::module_pattern(GetModuleHandle(L"Echelon"), "8B 81 ? ? ? ? 8B 54 24 04 89 02 8B 81 ? ? ? ? 8B 4C 24 08 89 01");
        struct GetPrimaryAmmoHook
        {
            void operator()(injector::reg_pack& regs)
            {
                const auto ammo = *reinterpret_cast<uint32_t*>(regs.ecx + 0x0B6C);
                const auto capacity = *reinterpret_cast<uint32_t*>(regs.ecx + 0x0B6C + 4);
                ammoSnapshot.store((uint64_t{ capacity } << 32) | ammo, std::memory_order_relaxed);
                regs.eax = ammo;
            }
        }; injector::MakeInline<GetPrimaryAmmoHook>(pattern.get_first(0), pattern.get_first(6));
    }

    // Gadgets unlocked by secondary objectives: 1.00 clears the player's unlocked gadget mask at every mission start
    // instead of taking it from the profile, so earned gadgets stay locked. Later builds already load it.
    {
        auto pattern = hook::module_pattern(GetModuleHandle(L"Echelon"), "89 9E 08 07 00 00 C7 45 EC FF FF FF FF");
        if (!pattern.empty()) // 1.00
        {
            injector::MakeNOP(pattern.get_first(), 6, true);

            pattern = hook::module_pattern(GetModuleHandle(L"Echelon"), "3B FB 8B 48 08 89 8E 04 07 00 00");
            static auto RestoreUnlockedGadgetsHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                // eax = player profile data, its first field is the unlocked gadget mask
                *(uint32_t*)(regs.esi + 0x708) = *(uint32_t*)regs.eax;
            });
        }
    }

    //EPlayerController additional state cache
    AEPlayerController::shTick = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"Echelon"), "?Tick@AEPlayerController@@UAEHMW4ELevelTick@@@Z"), AEPlayerController::Tick);
    AEPattern::shTick = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"Echelon"), "?Tick@AEPattern@@UAEHMW4ELevelTick@@@Z"), AEPattern::Tick);

    if (bEnableRunDuringForcedWalk)
    {
        auto pattern = hook::module_pattern(GetModuleHandle(L"Echelon"), "75 ? ? ? ? ? ? ? ? ? ? DF E0 F6 C4 ? 7A ? C7 45 ? ? ? ? ? EB");
        injector::WriteMemory<uint8_t>(pattern.get_first<uint8_t>(), 0xEB, true); //jz -> jmp
    }

    if (bDisablePreCache)
    {
        auto pattern = hook::module_pattern(GetModuleHandle(L"Echelon"), "F7 80 ? ? ? ? ? ? ? ? 74 ? 6A ? E9");
        static auto loc_10156B76 = (uintptr_t)pattern.get_first(12);
        static auto AEAIManagerUpdateMusicHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            if (bLoadMapWasCalled)
            {
                bLoadMapWasCalled = false;
                regs.eip = loc_10156B76;
            }
        });
    }
}
