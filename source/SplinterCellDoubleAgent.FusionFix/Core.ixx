module;

#include <stdafx.h>

export module Core;

import ComVars;

namespace
{
    SafetyHookInline conditionalDestroy;

    int __fastcall ConditionalDestroy(void* object, void* edx)
    {
        if (UObject::controllerState.controller == object)
            UObject::controllerState = {};
        return conditionalDestroy.unsafe_fastcall<int>(object, edx);
    }
}

export void InitCore()
{
    const auto core = GetModuleHandleW(L"Core.dll");
    conditionalDestroy = safetyhook::create_inline(GetProcAddress(core, "?ConditionalDestroy@UObject@@QAEHXZ"), ConditionalDestroy);
    UObject::GetFullName = (decltype(UObject::GetFullName))FindProcAddress(GetModuleHandle(L"Core"), "?GetFullName@UObject@@QBEPBGPAG@Z");
    UObject::FindState = (decltype(UObject::FindState))GetProcAddress(GetModuleHandle(L"Core"), "?FindState@UObject@@QAEPAVUState@@VFName@@@Z");

    GetFName = (decltype(GetFName))FindProcAddress(GetModuleHandle(L"Core"), "??DFName@@QBEPBGXZ");
}
