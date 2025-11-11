module;

#include <stdafx.h>

export module Lead3DEngine;

import ComVars;

SafetyHookInline shBinkOpen{};
int __stdcall BinkOpen(const char* path, int flags)
{
    auto handle = shBinkOpen.stdcall<int>(path, flags);

    auto s = std::string_view(path);
    if (s.contains("Logo_") || s.contains("_Load") || s.contains("C00_") || s.contains("C05_"))
        OpenedVideosList.push_back(handle);

    return handle;
}

SafetyHookInline shBinkClose{};
int __stdcall BinkClose(int handle)
{
    OpenedVideosList.remove(handle);
    return shBinkClose.stdcall<int>(handle);
}

export void InitLead3DEngine()
{
    auto pattern = hook::module_pattern(GetModuleHandle(L"Lead3DEngine"), "FF 15 ? ? ? ? 8B CE 89 86");
    shBinkOpen = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), BinkOpen);

    pattern = hook::module_pattern(GetModuleHandle(L"Lead3DEngine"), "FF 15 ? ? ? ? 38 1D");
    shBinkClose = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), BinkClose);
}