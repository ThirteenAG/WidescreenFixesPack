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

        wchar_t buffer[256];
        auto sv = std::wstring_view(UObject::GetFullName(uIntProperty, edx, buffer));
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
    UObject::GetFullName = (decltype(UObject::GetFullName))FindProcAddress(GetModuleHandle(L"Core"), "?GetFullName@UObject@@QBEPBGPAG@Z");
    UObject::FindState = (decltype(UObject::FindState))GetProcAddress(GetModuleHandle(L"Core"), "?FindState@UObject@@QAEPAVUState@@VFName@@@Z");
    UObject::shGotoState = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"Core"), "?GotoState@UObject@@UAE?AW4EGotoState@@VFName@@H@Z"), UObject::GotoState);

    //UProperty::shCopyCompleteValue = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"Core"), "?CopyCompleteValue@UProperty@@UBEXPAX0PAVUObject@@@Z"), UProperty::CopyCompleteValue);
    //UObjectProperty::shCopyCompleteValue = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"Core"), "?CopyCompleteValue@UObjectProperty@@UBEXPAX0PAVUObject@@@Z"), UObjectProperty::CopyCompleteValue);
    //UNameProperty::shCopyCompleteValue = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"Core"), "?CopyCompleteValue@UNameProperty@@UBEXPAX0PAVUObject@@@Z"), UNameProperty::CopyCompleteValue);
    //UIntProperty::shCopyCompleteValue = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"Core"), "?CopyCompleteValue@UIntProperty@@UBEXPAX0PAVUObject@@@Z"), UIntProperty::CopyCompleteValue);
    //UFloatProperty::shCopyCompleteValue = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"Core"), "?CopyCompleteValue@UFloatProperty@@UBEXPAX0PAVUObject@@@Z"), UFloatProperty::CopyCompleteValue);
    //UByteProperty::shCopyCompleteValue = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"Core"), "?CopyCompleteValue@UByteProperty@@UBEXPAX0PAVUObject@@@Z"), UByteProperty::CopyCompleteValue);
    //UArrayProperty::shCopyCompleteValue = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"Core"), "?CopyCompleteValue@UArrayProperty@@UBEXPAX0PAVUObject@@@Z"), UArrayProperty::CopyCompleteValue);
}