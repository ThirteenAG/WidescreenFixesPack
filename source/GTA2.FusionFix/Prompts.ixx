module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/GTAClassic/InputDevices.hpp"

export module Prompts;

import ComVars;

// Prompts follow the last device: while the pad is used, the game's key hints (the frontend's footer and the quit
// question) name the pad's buttons instead of ENTER, ESC, DEL and BACKSPACE. The texts come from the game's text
// lookup, which the frontend and HUD call every frame, so a switch shows at once.
export namespace GTA2Prompts
{
    SafetyHookInline TextHook;

    std::wstring Replace(std::wstring text, const std::wstring& key, const std::wstring& button)
    {
        if (key.empty())
            return text;
        // "Press the ENTER key to ..." -> "Press A to ..."; other languages: only the key's name changes
        auto english = L"the " + key + L" key";
        if (auto at = text.find(english); at != std::wstring::npos)
            return text.replace(at, english.size(), button);
        if (auto at = text.find(key); at != std::wstring::npos)
            text.replace(at, key.size(), button);
        return text;
    }

    const wchar_t* __fastcall Text(void* texts, int, const char* key)
    {
        auto native = TextHook.thiscall<const wchar_t*>(texts, key);
        if (!InputDevices::PadLast || !key || !native)
            return native;
        static const std::pair<const char*, WORD> hints[] = {
            { "esc_i", XINPUT_GAMEPAD_B }, { "del_i", XINPUT_GAMEPAD_X }, { "ent_i", XINPUT_GAMEPAD_A }, { "bspc_i", XINPUT_GAMEPAD_X } };
        for (auto& [name, button] : hints)
        {
            if (strcmp(key, name) == 0)
                return InputDevices::ButtonName(button);
        }
        // The quit question: A quits, B goes back to the game
        bool quit = strcmp(key, "quit2") == 0, resume = strcmp(key, "quit3") == 0;
        if (!quit && !resume)
            return native;
        static std::wstring prompt[2];
        auto keyName = TextHook.thiscall<const wchar_t*>(texts, quit ? "ent_i" : "esc_i");
        prompt[resume] = Replace(native, keyName ? keyName : L"", InputDevices::ButtonName(quit ? XINPUT_GAMEPAD_A : XINPUT_GAMEPAD_B));
        return prompt[resume].c_str();
    }

    void Install()
    {
        auto text = hook::pattern("8B 41 04 8B 09 8B 54 24 04 68 ? ? ? ? 6A 0C 50 51 52 E8");
        TextHook = safetyhook::create_inline(text.get_first(), Text);
    }
}
