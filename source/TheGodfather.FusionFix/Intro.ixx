module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <atomic>
#include <stdexcept>

export module Intro;

import NativeMenuABI;

export namespace GodfatherIntro
{
    SafetyHookInline startHook;
    SafetyHookInline textDecodeHook;
    std::atomic<bool> skip{ true };

    const unsigned char* __stdcall DecodeText(int* value, const unsigned char* text)
    {
        if (*text >= 0x80 && *text < 0xC0)
        {
            wchar_t character{};
            if (!MultiByteToWideChar(1252, 0, reinterpret_cast<const char*>(text), 1, &character, 1))
                character = L'?';
            *value = character;
            return text + 1;
        }
        return textDecodeHook.unsafe_stdcall<const unsigned char*>(value, text);
    }

    GodfatherMenu::Value* __cdecl Start(void* context)
    {
        if (!skip.load(std::memory_order_relaxed))
            return startHook.unsafe_ccall<GodfatherMenu::Value*>(context);
        if (context)
        {
            const auto bytes = static_cast<uint8_t*>(context);
            bytes[145 + GodfatherMenu::layoutShift] = 0;
            bytes[146 + GodfatherMenu::layoutShift] = 0;
        }
        return GodfatherMenu::api.undefined();
    }

    void Init()
    {
        if (GodfatherMenu::layoutShift) // Chinese executable
        {
            auto decode = hook::pattern("8B 44 24 08 8A 08 80 F9 7F 77 0D 8B 54 24 04 0F BE C9 89 0A 40 C2 08 00");
            textDecodeHook = safetyhook::create_inline(decode.get_first(), DecodeText);
        }
        auto start = hook::pattern("8B 44 24 04 B1 01 83 EC 08 56 88 88 91 00 00 00 88 88 92 00 00 00 E8");
        if (GodfatherMenu::layoutShift) // Chinese executable
            start = hook::pattern("8B 44 24 04 B1 01 83 EC 08 56 88 88 95 00 00 00 88 88 96 00 00 00 E8");
        skip.store(FixSettings().Bool("MAIN.SkipIntro"), std::memory_order_relaxed);
        startHook = safetyhook::create_inline(start.get_first(), Start);
        FixSettings().OnChange("MAIN.SkipIntro", []()
        {
            skip.store(FixSettings().Bool("MAIN.SkipIntro"), std::memory_order_relaxed);
        });
    }
}

