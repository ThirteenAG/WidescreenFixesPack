#pragma once
#include "RTTI.h"
#include <stdexcept>

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
        const auto table = tables.at(".?AVd3dDisplay@pure3d@@");
        const auto method = *reinterpret_cast<void**>(table + 20 * sizeof(void*));
        PresentHook = safetyhook::create_inline(method, Present);
        if (!PresentHook)
        {
            throw std::runtime_error("Native frame hook unavailable");
        }
        return bool(PresentHook);
    }
}
