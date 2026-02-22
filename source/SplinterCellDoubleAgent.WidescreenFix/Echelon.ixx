module;

#include "stdafx.h"

export module Echelon;

import ComVars;

namespace AEPlayerController
{
    SafetyHookInline shTick = {};
    void __fastcall Tick(void* playerController, void* edx, float a2, int a3)
    {
        if (playerController)
        {
            auto ptr = *((uintptr_t*)playerController + 3);
            if (ptr)
            {
                auto ptr2 = *(uintptr_t*)(ptr + 0x1C);
                if (ptr2)
                {
                    auto StateID = *(int*)(ptr2 + 0x20);

                    wchar_t buffer[256];
                    auto objName = std::wstring_view(UObject::GetFullName(playerController, edx, buffer));

                    size_t spacePos = objName.find(L' ');
                    std::wstring type = (spacePos != std::wstring::npos) ? std::wstring(objName.substr(0, spacePos)) : std::wstring(objName);

                    auto svStateName = std::wstring_view(UObject::GetFullName(UObject::FindState(playerController, edx, StateID), edx, buffer));
                    size_t lastDot = svStateName.rfind(L'.');
                    std::wstring stateName = (lastDot != std::wstring::npos) ? std::wstring(svStateName.substr(lastDot + 1)) : std::wstring(svStateName);
                    UObject::objectStates[type] = stateName;
                }
            }
        }

        return shTick.unsafe_fastcall(playerController, edx, a2, a3);
    }
}

namespace AEPattern
{
    SafetyHookInline shTick = {};
    void __fastcall Tick(void* AEPattern, void* edx, float a2, int a3)
    {
        std::wstring CameraShot = L"";

        if (AEPattern)
        {
            auto AEchelonLevelInfo = *((uintptr_t*)AEPattern + 0x81);
            if (AEchelonLevelInfo)
            {
                auto ptr = *((uintptr_t*)AEchelonLevelInfo + 0x4F9);
                if (ptr)
                {
                    CameraShot = GetFName((void*)(ptr + 0x38), 0);
                }
            }
        }

        gCurrentCameraShotName = CameraShot;

        return shTick.unsafe_fastcall(AEPattern, edx, a2, a3);
    }
}

export void InitEchelon()
{
    CIniReader iniReader("");
    gBlacklistIndicators = iniReader.ReadInteger("BONUS", "BlacklistIndicators", 0);

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
                    gVisibility = ((float)v / 120.0f);
                    if (gBlacklistIndicators == 2)
                        gVisibility = 1.0f - ((float)v / 120.0f);
                }
            }
        }; injector::MakeInline<BlacklistIndicatorsHook>(pattern.get_first(0), pattern.get_first(6));

        pattern = hook::module_pattern(GetModuleHandle(L"Echelon"), "8B 81 ? ? ? ? 8B 54 24 04 89 02 8B 81 ? ? ? ? 8B 4C 24 08 89 01");
        struct GetPrimaryAmmoHook
        {
            void operator()(injector::reg_pack& regs)
            {
                curAmmoInClip = *(uint32_t*)(regs.ecx + 0x0B6C);
                curClipCapacity = *(uint32_t*)(regs.ecx + 0x0B6C + 4);
                regs.eax = curAmmoInClip;
            }
        }; injector::MakeInline<GetPrimaryAmmoHook>(pattern.get_first(0), pattern.get_first(6));
    }

    //EPlayerController additional state cache
    AEPlayerController::shTick = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"Echelon"), "?Tick@AEPlayerController@@UAEHMW4ELevelTick@@@Z"), AEPlayerController::Tick);
    AEPattern::shTick = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"Echelon"), "?Tick@AEPattern@@UAEHMW4ELevelTick@@@Z"), AEPattern::Tick);
}
