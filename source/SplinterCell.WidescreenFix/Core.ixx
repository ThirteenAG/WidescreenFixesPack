module;

#include <stdafx.h>
#include <unordered_set>

export module Core;

import ComVars;

SafetyHookInline shUStrPropertyCopySingleValue = {};
void __fastcall UStrPropertyCopySingleValue(void* UStrProperty, void* edx, void* FArray1, void* FArray2, void* UObject)
{
    shUStrPropertyCopySingleValue.unsafe_fastcall(UStrProperty, edx, FArray1, FArray2, UObject);

    if (*(uint32_t*)FArray2 != 0)
    {
        auto ptr = *(uintptr_t*)FArray2;
        auto str = std::wstring_view((const wchar_t*)ptr);

        if (str == L"1024x768")
        {
            auto try_offsets = [&](const std::vector<int>& offsets) -> bool
            {
                bool bReqPtr = true;
                for (size_t i = 2; i < ResList.size(); ++i)
                {
                    if (std::wstring_view((const wchar_t*)(ptr + offsets[i])) != ResList[i].first)
                    {
                        bReqPtr = false;
                        break;
                    }
                }
                if (bReqPtr)
                {
                    for (size_t i = 2; i < ResList.size(); ++i)
                    {
                        auto dst = reinterpret_cast<wchar_t*>(ptr + offsets[i]);
                        injector::scoped_unprotect p{ dst, ResList[i].second.size() + 1 };
                        wcscpy_s(dst, ResList[i].second.size() + 1, ResList[i].second.c_str());
                    }
                }
                return bReqPtr;
            };

            std::vector<int> offsets = { 0, 0, 0, -20, -40 };
            if (!try_offsets(offsets))
            {
                offsets = { 0, 0, 0, 20, 40 };
                try_offsets(offsets);
            }
        }
    }
}

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
        L"EPCConsole",
    };

    SafetyHookInline shGotoState = {};
    int __fastcall GotoState(void* uObject, void* edx, int StateID)
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

        return shGotoState.unsafe_fastcall<int>(uObject, edx, StateID);
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
    shUStrPropertyCopySingleValue = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"Core"), "?CopySingleValue@UStrProperty@@UBEXPAX0PAVUObject@@@Z"), UStrPropertyCopySingleValue);

    UObject::GetFullName = (decltype(UObject::GetFullName))GetProcAddress(GetModuleHandle(L"Core"), "?GetFullName@UObject@@QBEPBGPAG@Z");
    UObject::FindState = (decltype(UObject::FindState))GetProcAddress(GetModuleHandle(L"Core"), "?FindState@UObject@@QAEPAVUState@@VFName@@@Z");
    UObject::shGotoState = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"Core"), "?GotoState@UObject@@UAE?AW4EGotoState@@VFName@@@Z"), UObject::GotoState);

    UProperty::shCopyCompleteValue = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"Core"), "?CopyCompleteValue@UProperty@@UBEXPAX0PAVUObject@@@Z"), UProperty::CopyCompleteValue);
    UObjectProperty::shCopyCompleteValue = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"Core"), "?CopyCompleteValue@UObjectProperty@@UBEXPAX0PAVUObject@@@Z"), UObjectProperty::CopyCompleteValue);
    UNameProperty::shCopyCompleteValue = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"Core"), "?CopyCompleteValue@UNameProperty@@UBEXPAX0PAVUObject@@@Z"), UNameProperty::CopyCompleteValue);
    UIntProperty::shCopyCompleteValue = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"Core"), "?CopyCompleteValue@UIntProperty@@UBEXPAX0PAVUObject@@@Z"), UIntProperty::CopyCompleteValue);
    UFloatProperty::shCopyCompleteValue = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"Core"), "?CopyCompleteValue@UFloatProperty@@UBEXPAX0PAVUObject@@@Z"), UFloatProperty::CopyCompleteValue);
    UByteProperty::shCopyCompleteValue = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"Core"), "?CopyCompleteValue@UByteProperty@@UBEXPAX0PAVUObject@@@Z"), UByteProperty::CopyCompleteValue);
    UArrayProperty::shCopyCompleteValue = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"Core"), "?CopyCompleteValue@UArrayProperty@@UBEXPAX0PAVUObject@@@Z"), UArrayProperty::CopyCompleteValue);
}
