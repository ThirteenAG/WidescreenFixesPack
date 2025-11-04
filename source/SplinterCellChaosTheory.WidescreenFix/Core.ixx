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
        L"EPlayerCam",
    };

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

// Base UProperty: pass-through
namespace UProperty
{
    SafetyHookInline shCopyCompleteValue = {};
    void __fastcall CopyCompleteValue(void* uProperty, void* edx, char* dst, char* src, void* uObject)
    {
        shCopyCompleteValue.unsafe_fastcall(uProperty, edx, dst, src, uObject);
    }
}

namespace UIntProperty
{
    SafetyHookInline shCopyCompleteValue = {};
    void __fastcall CopyCompleteValue(void* uIntProperty, void* edx, int* dst, int* src, void* uObject)
    {
        shCopyCompleteValue.unsafe_fastcall(uIntProperty, edx, dst, src, uObject);
        UIntOverrides::ApplyScalarOverride<UIntOverrides, int>(uIntProperty, edx, dst);
    }
}

namespace UFloatProperty
{
    SafetyHookInline shCopyCompleteValue = {};
    void __fastcall CopyCompleteValue(void* uFloatProperty, void* edx, float* dst, float* src, void* uObject)
    {
        shCopyCompleteValue.unsafe_fastcall(uFloatProperty, edx, dst, src, uObject);
        UFloatOverrides::ApplyScalarOverride<UFloatOverrides, float>(uFloatProperty, edx, dst);
    }
}

namespace UByteProperty
{
    SafetyHookInline shCopyCompleteValue = {};
    void __fastcall CopyCompleteValue(void* uByteProperty, void* edx, uint8_t* dst, uint8_t* src, void* uObject)
    {
        shCopyCompleteValue.unsafe_fastcall(uByteProperty, edx, dst, src, uObject);
        UByteOverrides::ApplyScalarOverride<UByteOverrides, uint8_t>(uByteProperty, edx, dst);
    }
}

namespace UNameProperty
{
    SafetyHookInline shCopyCompleteValue = {};
    void __fastcall CopyCompleteValue(void* uNameProperty, void* edx, FName* dst, FName* src, void* uObject)
    {
        shCopyCompleteValue.unsafe_fastcall(uNameProperty, edx, dst, src, uObject);
        UNameOverrides::ApplyScalarOverride<UNameOverrides, FName>(uNameProperty, edx, dst);
    }
}

namespace UObjectProperty
{
    SafetyHookInline shCopyCompleteValue = {};
    void __fastcall CopyCompleteValue(void* uObjectProperty, void* edx, void** dst, void** src, void* uObject)
    {
        shCopyCompleteValue.unsafe_fastcall(uObjectProperty, edx, dst, src, uObject);
        UObjectOverrides::ApplyScalarOverride<UObjectOverrides, void*>(uObjectProperty, edx, dst);
    }
}

namespace UArrayProperty
{
    SafetyHookInline shCopyCompleteValue = {};
    void __fastcall CopyCompleteValue(void* uArrayProperty, void* edx, void* dst, const void** src, void* uObject)
    {
        shCopyCompleteValue.unsafe_fastcall(uArrayProperty, edx, dst, src, uObject);
        UArrayOverrides::ApplyArrayOverride<UArrayOverrides>(uArrayProperty, edx, reinterpret_cast<FArray*>(dst));
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

    //no need to use for now
    //pattern = hook::pattern("66 8B 41 34 66 3D 01 00 75 ? 8B 44 24 08 8B 08 8B 54 24 04 89 0A C2 0C 00 33 D2 66 85 C0 76 ? 8B 44 24 04 56");
    //UIntProperty::shCopyCompleteValue = safetyhook::create_inline(pattern.get_first(), UIntProperty::CopyCompleteValue);
}