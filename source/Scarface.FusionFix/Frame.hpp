#pragma once
#include "RTTI.h"

namespace ScarfaceFrame
{
    inline SafetyHookInline PresentHook;
    inline std::vector<std::function<void(void*)>> Handlers;

    inline LRESULT __fastcall Present(void* display, void*)
    {
        const auto result = PresentHook.unsafe_fastcall<LRESULT>(display, nullptr);
        for (const auto& handler : Handlers)
            handler(display);
        return result;
    }

    inline bool Install(HMODULE module = GetModuleHandle(nullptr))
    {
        if (PresentHook)
            return true;
        const auto tables = ScarfaceRTTI::ScanVtables(module);
        const auto table = tables.find(".?AVd3dDisplay@pure3d@@");
        if (table == tables.end())
        {
            OutputDebugStringA("Scarface: native frame boundary unavailable.\n");
            return false;
        }
        const auto method = *reinterpret_cast<void**>(table->second + 20 * sizeof(void*));
        PresentHook = safetyhook::create_inline(method, Present);
        if (!PresentHook)
            OutputDebugStringA("Scarface: native frame hook unavailable.\n");
        return bool(PresentHook);
    }
}
