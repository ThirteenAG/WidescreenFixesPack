module;

#include <stdafx.h>
#include <Xinput.h>
#include "FixSettings.hpp"

export module Gamepad;

import DeveloperOverlay;
import ComVars;

namespace
{
    using ReadState = DWORD(WINAPI*)(DWORD, XINPUT_STATE*);
    ReadState NativeReadState = nullptr;

    DWORD WINAPI Read(DWORD index, XINPUT_STATE* state)
    {
        const auto result = NativeReadState(index, state);
        if (result == ERROR_SUCCESS && state && DeveloperOverlay::BlocksInput())
            state->Gamepad = {};
        return result;
    }
}

export std::optional<std::string> GamepadArtworkOverride(std::string_view path)
{
    const auto file = path.substr(path.find_last_of("\\/") + 1);
    const std::string name(file);
    if (_stricmp(name.c_str(), "HUD_ImageSubstitution.gfx") != 0 &&
        _stricmp(name.c_str(), "HUD_ImageSubstitution.swf") != 0)
        return std::nullopt;

    static const int family = FixSettings().Int("MAIN.GamepadIcons");
    if (family != 2 && family != 3)
        return std::nullopt;

    const std::string result = std::string("..\\..\\data\\Flash\\Ingame\\buttons-") +
        (family == 2 ? "playstation.gfx" : "nintendo.gfx");
    if (!GetOverloadedFilePathA || !GetOverloadedFilePathA(result.c_str(), nullptr, 0))
        return std::nullopt;
    return result;
}

export void InitGamepad()
{
    // The ordinal import may already contain the native split-screen player mapper.
    auto pattern = hook::pattern("8D 4C 24 04 E9 ? ? ? ? CC FF 25 ? ? ? ? FF 25 ? ? ? ?");
    auto slot = *pattern.get_first<ReadState*>(18);
    if (slot && *slot && *slot != Read)
    {
        NativeReadState = *slot;
        injector::WriteMemory<ReadState>(slot, Read, true);
    }
}
