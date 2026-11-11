#pragma once
#include <string_view>

namespace ScarfaceText
{
    using Function = const wchar_t*(__cdecl*)(const char*);
    inline Function target = nullptr;
    inline bool attempted = false;
    inline SafetyHookInline lookupHook;
    inline const wchar_t* (*overrideText)(std::string_view) = nullptr;

    inline Function Resolve(HMODULE module = GetModuleHandleW(nullptr))
    {
        if (attempted)
            return target;
        attempted = true;
        auto pattern = hook::module_pattern(module, "55 8B 6C 24 08 56 57 BF ? ? ? ? 8B F5 B9 01 00 00 00 33 C0 F3 A6");
        if (pattern.empty())
            pattern = hook::module_pattern(module, "55 8B 6C 24 08 56 57 E9 ? ? ? ? 8B F5 B9 01 00 00 00 33 C0 F3 A6");
        target = reinterpret_cast<Function>(pattern.get_first());
        return target;
    }

    inline const wchar_t* __cdecl Lookup(const char* key)
    {
        if (key && overrideText)
        {
            const auto length = strnlen(key, 512);
            if (length < 512)
                if (auto text = overrideText(std::string_view(key, length)))
                    return text;
        }
        return lookupHook.unsafe_ccall<const wchar_t*>(key);
    }

    inline void Install()
    {
        if (!lookupHook)
            lookupHook = safetyhook::create_inline(Resolve(), Lookup);
    }

    inline void Shutdown()
    {
        overrideText = nullptr;
        lookupHook.reset();
    }
}
