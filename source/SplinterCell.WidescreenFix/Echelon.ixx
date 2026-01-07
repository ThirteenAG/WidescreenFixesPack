module;

#include <stdafx.h>

export module Echelon;

import ComVars;

namespace AEPlayerController
{
    SafetyHookInline shTick = {};
    void __fastcall Tick(void* playerController, void* edx, int a2, int a3)
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

export void InitEchelon()
{
    //EPlayerController additional state cache
    AEPlayerController::shTick = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"Echelon"), "?Tick@AEPlayerController@@UAEHMW4ELevelTick@@@Z"), AEPlayerController::Tick);

    // Hud Helpers
    auto pattern = find_module_pattern(GetModuleHandle(L"Echelon"), "85 C0 7C ? 8B 89 A8 02 00 00");
    static auto AETextureManagerDrawTileFromManagerHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        curDrawTileManagerTextureIndex = regs.eax;
    });

    pattern = find_module_pattern(GetModuleHandle(L"Echelon"), "85 C0 7C ? D9 44 24 08");
    static auto AETextureManagerexecDrawTileFromManagerHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        curDrawTileManagerTextureIndex = regs.eax;
    });

    // set player speed to max on game start
    if (!IsEnhanced())
    {
        UIntOverrides::Register(L"IntProperty Echelon.EchelonGameInfo.m_defautSpeed", +[]() -> int
        {
            return 5;
        });
    }

    // Camera acceleration
    pattern = find_module_pattern(GetModuleHandle(L"Echelon"), "7A ? D9 81 DC 03 00 00");
    injector::WriteMemory<uint8_t>(pattern.get_first(0), 0xEB, true); // jp -> jmp

    // Camera minYaw/maxYaw
    pattern = find_module_pattern(GetModuleHandle(L"Echelon"), "E8 ? ? ? ? 89 44 24 ? 8A 86");
    static auto AEPlayerCamUpdateRotationForSamHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        auto& minYaw = *(int32_t*)(regs.esp + 0x24);
        auto& maxYaw = *(int32_t*)(regs.esp + 0x28);

        enum ECamMode
        {
            ECM_Walking,
            ECM_WalkingCr,
            ECM_FirstPerson,
            ECM_FirstPersonCr,
            ECM_Grab,
            ECM_GrabFP,
            ECM_Carry,
            ECM_CarryCr,
            ECM_Throw,
            ECM_ThrowCr,
            ECM_Sniping,
            ECM_HSphere,
            ECM_FSphere,
            ECM_HOH,
            ECM_HOHFU,
            ECM_HOHFP,
            ECM_BTW,
            ECM_BTWPeakLeft,
            ECM_BTWPeakRight,
            ECM_BTWLeftFP,
            ECM_BTWRightFP,
            ECM_Rapel,
            ECM_RapelFP,
            ECM_DoorPeekRight,
            ECM_DoorPeekLeft,
            ECM_SplitJump,
            ECM_SplitJumpFP,
        };

        auto camMode = ECamMode(*(int8_t*)(regs.esi + 0x12B0));

        if (camMode == ECM_BTWPeakLeft || camMode == ECM_BTWPeakRight)
        {
            if (UObject::GetState(L"EPlayerController") == L"s_PlayerBTWThrow")
            {
                minYaw = -9000;
                maxYaw = 3000;
            }
        }
    });
}
