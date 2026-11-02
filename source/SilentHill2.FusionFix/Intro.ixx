module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/SilentHill/Display.hpp"


export module Intro;

import ComVars;

export namespace SH2Intro
{
    inline SafetyHookInline Hooks[3];

    inline hook::pattern DataPattern(const std::string& signature)
    {
        auto module = reinterpret_cast<uint8_t*>(GetModuleHandleW(nullptr));
        auto dos = reinterpret_cast<IMAGE_DOS_HEADER*>(module);
        auto nt = reinterpret_cast<IMAGE_NT_HEADERS*>(module + dos->e_lfanew);
        auto section = IMAGE_FIRST_SECTION(nt);
        hook::pattern result;
        for (int i = 0; i < nt->FileHeader.NumberOfSections; ++i, ++section)
        {
            if (!(section->Characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA) ||
                (section->Characteristics & IMAGE_SCN_MEM_EXECUTE))
                continue;
            auto begin = reinterpret_cast<uintptr_t>(module + section->VirtualAddress);
            auto candidate = hook::range_pattern(begin, begin + section->Misc.VirtualSize, signature);
            if (!candidate.empty())
            {
                result = std::move(candidate);
                break;
            }
        }
        return result;
    }

    template<size_t Index>
    int __cdecl Splash()
    {
        if (FixSettings().Bool("MAIN.SkipIntro") && GameMode.is_initialized() && GamePhase.is_initialized() &&
            GameMode.get() >= 3 && GameMode.get() <= 5 && GamePhase.get() == 0)
        {
            injector::WriteMemory<int>(GameMode.get_ptr(), GameMode.get() + 1, true);
            return 0;
        }
        return Hooks[Index].ccall<int>();
    }

    inline void Init()
    {
        const char* signatures[] =
        {
            "64 61 74 61 2F 70 69 63 2F 65 74 63 2F 63 65 73 61 2E 74 65 78 00",
            "64 61 74 61 2F 70 69 63 2F 65 74 63 2F 6B 6F 6E 61 6D 69 32 30 34 38 78 32 30 34 38 2E 74 65 78 00",
            "64 61 74 61 2F 70 69 63 2F 65 74 63 2F 73 70 6C 61 73 68 5F 6C 6F 67 6F 2E 74 65 78 00",
        };
        void* wrappers[3]{};
        for (size_t i = 0; i < std::size(signatures); ++i)
        {
            auto name = DataPattern(signatures[i]);
            auto asset = DataPattern(pattern_str(to_bytes(name.get_first())) + " FF FF FF FF 00 00 00 00 01 00 00 00");
            auto wrapper = hook::pattern("56 BE " + pattern_str(to_bytes(asset.get_first())) + " E8 ? ? ? ? 5E C3");
            wrappers[i] = wrapper.get_first();
        }
        Hooks[0] = safetyhook::create_inline(wrappers[0], &Splash<0>);
        Hooks[1] = safetyhook::create_inline(wrappers[1], &Splash<1>);
        Hooks[2] = safetyhook::create_inline(wrappers[2], &Splash<2>);
    }
}
