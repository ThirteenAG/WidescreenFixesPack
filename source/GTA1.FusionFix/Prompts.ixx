module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/GTAClassic/InputDevices.hpp"

export module Prompts;

import ComVars;
import Log;

// Prompts follow the last device: while the pad is used, the frontend's key labels (Enter, Esc) and the quit question
// ("Press ENTER to confirm / or ESC to cancel") name the pad's buttons. The frontend looks its labels up once and
// keeps the texts in globals, which are switched every frontend frame; the quit question is looked up when it opens.
export namespace GTA1Prompts
{
    struct Label
    {
        const char* key;
        WORD button;
        const char** global = nullptr;
        const char* native = nullptr;
    };
    Label Labels[] = { { "rtn-key", XINPUT_GAMEPAD_A }, { "esc-key", XINPUT_GAMEPAD_B } };
    SafetyHookInline TextHook;
    const char* (__cdecl* Lookup)(const char*) = nullptr;

    std::string Narrow(const wchar_t* text)
    {
        std::string out;
        for (; text && *text; ++text) out += char(*text);
        return out;
    }

    // The native text with its key word (any case) replaced by the button's name
    std::string Replace(std::string text, const char* key, WORD button)
    {
        std::string word = key ? key : "", lower = text;
        for (auto& c : word) c = char(tolower(uint8_t(c)));
        for (auto& c : lower) c = char(tolower(uint8_t(c)));
        if (auto at = word.empty() ? std::string::npos : lower.find(word); at != std::string::npos)
            text.replace(at, word.size(), Narrow(InputDevices::ButtonName(button)));
        return text;
    }

    const char* __cdecl Text(const char* key)
    {
        auto native = TextHook.ccall<const char*>(key);
        if (!InputDevices::PadLast || !key || !native)
            return native;
        bool quit = strcmp(key, "quit2") == 0, cancel = strcmp(key, "quit3") == 0;
        if (!quit && !cancel)
            return native;
        static std::string prompt[2];
        prompt[cancel] = Replace(native, TextHook.ccall<const char*>(quit ? "rtn-key" : "esc-key"), quit ? XINPUT_GAMEPAD_A : XINPUT_GAMEPAD_B);
        return prompt[cancel].c_str();
    }

    // Each frontend frame
    void Frame()
    {
        static std::string names[std::size(Labels)];
        for (size_t i = 0; i < std::size(Labels); ++i)
        {
            auto& label = Labels[i];
            if (!label.global || !*label.global)
                continue;
            if (*label.global != names[i].c_str())
                label.native = *label.global;
            names[i] = Narrow(InputDevices::ButtonName(label.button));
            *label.global = InputDevices::PadLast ? names[i].c_str() : label.native;
        }
    }

    // The frontend's setup: "push offset <key>; mov <previous global>, eax; call <lookup>; push offset <next key>;
    // mov <global>, eax"
    void Install()
    {
        auto module = GetModuleHandleW(nullptr);
        for (auto& label : Labels)
        {
            std::string bytes;
            for (auto c : std::string_view(label.key))
            {
                char hex[4];
                sprintf_s(hex, "%02X ", uint8_t(c));
                bytes += hex;
            }
            // The key's text is data: search the whole image, not only its code
            auto headers = reinterpret_cast<PIMAGE_NT_HEADERS>(reinterpret_cast<uint8_t*>(module) + reinterpret_cast<PIMAGE_DOS_HEADER>(module)->e_lfanew);
            auto begin = reinterpret_cast<uintptr_t>(module);
            auto text = hook::range_pattern(begin, begin + headers->OptionalHeader.SizeOfImage, bytes + "00");
            text.get_first();
            for (size_t t = 0; t < text.size() && !label.global; ++t)
            {
                auto address = reinterpret_cast<uintptr_t>(text.get(t).get<void>());
                char push[64];
                sprintf_s(push, "68 %02X %02X %02X %02X A3 ? ? ? ? E8 ? ? ? ? 68 ? ? ? ? A3", unsigned(address & 0xFF), unsigned((address >> 8) & 0xFF),
                    unsigned((address >> 16) & 0xFF), unsigned(address >> 24));
                auto use = hook::module_pattern(module, push);
                if (use.empty() && t + 1 < text.size())
                    continue;
                label.global = *use.get_first<const char**>(21);
                if (!Lookup)
                    Lookup = reinterpret_cast<decltype(Lookup)>(GTA1Build::Target(use.get_first(10)));
            }
        }
        TextHook = safetyhook::create_inline(reinterpret_cast<void*>(Lookup), Text);
        Log::Write("GTA1 prompts follow the last device.");
    }
}
