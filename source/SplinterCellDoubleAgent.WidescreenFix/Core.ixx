module;

#include <stdafx.h>
#include <unordered_set>

export module Core;

import ComVars;

namespace UObject
{
    std::unordered_set<std::wstring> cachedTypes = {
        //L"EPlayerController",
        //L"EchelonMainHUD",
        //L"EGameInteraction",
        //L"EDoorMarker",
        //L"EKeyPad",
        //L"EElevatorPanel",
        //L"EDoorMarker",
        //L"EOpticCable",
        //L"EPlayerCam",
        //L"EGoggle",
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

        return shGotoState.unsafe_fastcall<int>(uObject, edx, StateID, a3);
    }
}

export void InitCore()
{
    UObject::GetFullName = (decltype(UObject::GetFullName))FindProcAddress(GetModuleHandle(L"Core"), "?GetFullName@UObject@@QBEPBGPAG@Z");
    UObject::FindState = (decltype(UObject::FindState))GetProcAddress(GetModuleHandle(L"Core"), "?FindState@UObject@@QAEPAVUState@@VFName@@@Z");
    UObject::shGotoState = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"Core"), "?GotoState@UObject@@UAE?AW4EGotoState@@VFName@@H@Z"), UObject::GotoState);
}