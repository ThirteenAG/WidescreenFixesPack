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
    auto pattern = hook::pattern("8B 88 ? ? ? ? 8B 6E");
    AEPlayerController::shTick = safetyhook::create_inline(pattern.get_first(-10), AEPlayerController::Tick);
}