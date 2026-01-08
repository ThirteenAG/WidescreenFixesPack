module;

#include <stdafx.h>
#include <unordered_set>

export module Core;

import ComVars;

SafetyHookInline shappFromAnsi = {};
wchar_t* __cdecl appFromAnsi(const char* a1)
{
    bool bReqPtr = false;
    std::vector<int> offsets = { 0, 24, 48, 74, 101 };
    for (size_t i = 0; i < ResList.size(); i++)
    {
        const auto& r = ResList[i].first;
        std::string str(r.begin(), r.end());
        if (std::string_view(a1 + offsets[i]) == str)
            bReqPtr = true;
        else
        {
            bReqPtr = false;
            break;
        }
    }

    if (bReqPtr)
    {
        for (size_t i = 0; i < ResList.size(); i++)
        {
            const auto& r = ResList[i].second;
            std::string str(r.begin(), r.end());
            injector::scoped_unprotect p{ const_cast<char*>(a1 + offsets[i]), str.size() + 1 };
            strcpy_s(const_cast<char*>(a1 + offsets[i]), str.size() + 1, str.c_str());
        }
    }

    return shappFromAnsi.unsafe_ccall<wchar_t*>(a1);
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
    auto pattern = find_module_pattern(GetModuleHandle(L"Core"), "C7 85 D4 F1 FF FF 00 00 00 00", "C7 85 ? ? ? ? ? ? ? ? EB ? 8B 8D ? ? ? ? 83 C1 ? 89 8D ? ? ? ? 81 BD ? ? ? ? ? ? ? ? 0F 83"); //0x1000CE5E
    uint32_t pfappInit = (uint32_t)pattern.get_first();

    auto rpattern = hook::range_pattern(pfappInit, pfappInit + 0x900, "80 02 00 00");
    injector::WriteMemory<uint16_t>(rpattern.count(2).get(0).get<uint32_t>(-6), 0x7EEB, true); //pfappInit + 0x5FC
    injector::WriteMemory(rpattern.count(2).get(1).get<uint32_t>(0), Screen.Width, true);  //pfappInit + 0x67E + 0x1
    rpattern = hook::range_pattern(pfappInit, pfappInit + 0x900, "E0 01 00 00");
    injector::WriteMemory(rpattern.count(2).get(1).get<uint32_t>(0), Screen.Height, true); //pfappInit + 0x69F + 0x1

    shappFromAnsi = safetyhook::create_inline(FindProcAddress(GetModuleHandle(L"Core"), "?appFromAnsi@@YAPBGPBD@Z", "?appFromAnsi@@YAPB_WPBD@Z"), appFromAnsi);

    UObject::GetFullName = (decltype(UObject::GetFullName))FindProcAddress(GetModuleHandle(L"Core"), "?GetFullName@UObject@@QBEPBGPAG@Z", "?GetFullName@UObject@@QBEPB_WPA_W@Z");
    UObject::FindState = (decltype(UObject::FindState))GetProcAddress(GetModuleHandle(L"Core"), "?FindState@UObject@@QAEPAVUState@@VFName@@@Z");
    UObject::shGotoState = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"Core"), "?GotoState@UObject@@UAE?AW4EGotoState@@VFName@@@Z"), UObject::GotoState);

    pattern = hook::module_pattern(GetModuleHandle(L"Core"), "33 C0 0F B7 0B");
    if (!pattern.empty())
    {
        static auto OverwriteResolutions = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            auto str = std::wstring_view((const wchar_t*)regs.ebx);

            for (const auto& it : ResList)
            {
                if (str == it.first)
                {
                    wcscpy_s((wchar_t*)regs.ebx, it.second.size() + 1, it.second.c_str());
                    break;
                }
            }
        });
    }
    else
    {
        pattern = hook::module_pattern(GetModuleHandle(L"Core"), "33 C0 66 85 C9");
        static auto OverwriteResolutions = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            auto str = std::wstring_view((const wchar_t*)regs.esi);

            for (const auto& it : ResList)
            {
                if (str == it.first)
                {
                    wcscpy_s((wchar_t*)regs.esi, it.second.size() + 1, it.second.c_str());
                    break;
                }
            }
        });
    }

    UProperty::shCopyCompleteValue = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"Core"), "?CopyCompleteValue@UProperty@@UBEXPAX0PAVUObject@@@Z"), UProperty::CopyCompleteValue);
    UObjectProperty::shCopyCompleteValue = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"Core"), "?CopyCompleteValue@UObjectProperty@@UBEXPAX0PAVUObject@@@Z"), UObjectProperty::CopyCompleteValue);
    UNameProperty::shCopyCompleteValue = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"Core"), "?CopyCompleteValue@UNameProperty@@UBEXPAX0PAVUObject@@@Z"), UNameProperty::CopyCompleteValue);
    UIntProperty::shCopyCompleteValue = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"Core"), "?CopyCompleteValue@UIntProperty@@UBEXPAX0PAVUObject@@@Z"), UIntProperty::CopyCompleteValue);
    UFloatProperty::shCopyCompleteValue = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"Core"), "?CopyCompleteValue@UFloatProperty@@UBEXPAX0PAVUObject@@@Z"), UFloatProperty::CopyCompleteValue);
    UByteProperty::shCopyCompleteValue = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"Core"), "?CopyCompleteValue@UByteProperty@@UBEXPAX0PAVUObject@@@Z"), UByteProperty::CopyCompleteValue);
    UArrayProperty::shCopyCompleteValue = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"Core"), "?CopyCompleteValue@UArrayProperty@@UBEXPAX0PAVUObject@@@Z"), UArrayProperty::CopyCompleteValue);
}