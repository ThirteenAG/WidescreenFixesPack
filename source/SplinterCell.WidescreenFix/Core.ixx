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

    void* (__fastcall* FindState)(void*, void*, int) = nullptr;

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

export void InitCore()
{
    // set player speed to max on game start
    auto pattern = hook::module_pattern(GetModuleHandle(L"Core"), "8B D9 C1 E9 02 83 E3 03 F3 A5 8B CB F3 A4 5B 5F 5E 59 8B 44 24 ? 8B 4C 24 ? 3B C1");
    static auto UObjectInitPropertiesHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        EchelonGameInfoPtrs.push_back(regs.ebp);
    });

    shUStrPropertyCopySingleValue = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"Core"), "?CopySingleValue@UStrProperty@@UBEXPAX0PAVUObject@@@Z"), UStrPropertyCopySingleValue);

    UObject::GetFullName = (decltype(UObject::GetFullName))GetProcAddress(GetModuleHandle(L"Core"), "?GetFullName@UObject@@QBEPBGPAG@Z");
    UObject::FindState = (decltype(UObject::FindState))GetProcAddress(GetModuleHandle(L"Core"), "?FindState@UObject@@QAEPAVUState@@VFName@@@Z");
    UObject::shGotoState = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"Core"), "?GotoState@UObject@@UAE?AW4EGotoState@@VFName@@@Z"), UObject::GotoState);
}
