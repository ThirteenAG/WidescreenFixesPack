module;

#include <stdafx.h>
#include <unordered_set>

export module Core;

import ComVars;

namespace UObject
{
    std::unordered_set<std::wstring> cachedTypes = {
        L"EPlayerController",
        L"EchelonMainHUD",
        L"EGameInteraction",
        L"EDoorMarker",
        L"EKeyPad",
        L"EElevatorPanel",
        L"EDoorMarker",
    };

    wchar_t* (__fastcall* GetFullName)(void*, void*, wchar_t*) = nullptr;
    void* (__fastcall* FindState)(void*, void*, int) = nullptr;

    SafetyHookInline shGotoState = {};
    int __fastcall GotoState(void* uObject, void* edx, int StateID, int a3)
    {
        wchar_t buffer[256];
        auto objName = std::wstring_view(GetFullName(uObject, edx, buffer));

        size_t spacePos = objName.find(L' ');
        std::wstring type = (spacePos != std::wstring::npos) ? std::wstring(objName.substr(0, spacePos)) : std::wstring(objName);
        if (cachedTypes.count(type))
        {
            auto svStateName = std::wstring_view(GetFullName(FindState(uObject, edx, StateID), edx, buffer));
            size_t lastDot = svStateName.rfind(L'.');
            std::wstring stateName = (lastDot != std::wstring::npos) ? std::wstring(svStateName.substr(lastDot + 1)) : std::wstring(svStateName);
            objectStates[type] = stateName;
        }

        // When the game reloads, nothing resets it, so I'll do it here
        if (!type.contains(L"Hack") && UObject::GetState(L"EPlayerController") != L"s_InteractWithObject")
            bHackingGameplay = false;

        return shGotoState.unsafe_fastcall<int>(uObject, edx, StateID, a3);
    }
}

export void InitCore()
{
    auto pattern = hook::pattern("56 8B 74 24 08 85 F6 57 8B F9 75");
    UObject::GetFullName = (decltype(UObject::GetFullName))pattern.get_first();

    pattern = hook::pattern("8B 54 24 04 8B 49 24 8B C2");
    UObject::FindState = (decltype(UObject::FindState))pattern.get_first();

    pattern = hook::pattern("83 EC 0C 55 56 8B F1 8B 46 0C");
    UObject::shGotoState = safetyhook::create_inline(pattern.get_first(), UObject::GotoState);

    pattern = hook::pattern("8B 7C 24 0C 52 8D 44 24 14");
    static auto EHackingGameplayStartHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        bHackingGameplay = true;
    });

    pattern = hook::pattern("83 A6 FC 04 00 00 FE");
    static auto EHackingGameplayEndHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        bHackingGameplay = false;
    });
}