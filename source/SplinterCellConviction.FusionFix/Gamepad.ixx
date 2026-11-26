module;

#include <stdafx.h>
#include <Xinput.h>
#include <dinput.h>
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

    // UWindowsViewport joystick init (sub_8F3E00) finds up to two game controllers with DirectInput and reads a pad with XInput only
    // if its USB id is on its list (Xbox 360 pads, a few others) or it's unknown and UseXInputByDefault is set. Other Microsoft ids
    // (Xbox One and Series pads) are read with DirectInput: the camera ends up on the wrong sticks and triggers, the buttons shuffled.
    // A controller whose device path has "IG_" is an XInput pad, it's read with XInput like an Xbox 360 pad.
    SafetyHookInline shJoystickInit{};
    uint32_t* padDevices = nullptr; // dword_1431E28[2]
    uint32_t* padXInput = nullptr;  // dword_1431E34[2]
    uint32_t* padType = nullptr;    // dword_1431E10[2]
    uint32_t* padFound = nullptr;   // dword_1431E3C[2]

    bool IsXInputDevice(uint32_t device)
    {
        DIPROPGUIDANDPATH property = {};
        property.diph.dwSize = sizeof(property);
        property.diph.dwHeaderSize = sizeof(DIPROPHEADER);
        property.diph.dwHow = DIPH_DEVICE;
        auto getProperty = reinterpret_cast<HRESULT(__stdcall*)(uint32_t, REFGUID, DIPROPHEADER*)>((*reinterpret_cast<uintptr_t**>(device))[0x14 / 4]);
        return SUCCEEDED(getProperty(device, DIPROP_GUIDANDPATH, &property.diph)) && wcsstr(_wcsupr(property.wszPath), L"IG_");
    }

    void* __fastcall JoystickInit(void* viewport, void* edx)
    {
        auto result = shJoystickInit.fastcall<void*>(viewport, edx);
        for (int i = 0; i < 2; ++i)
        {
            if (padDevices[i] && padFound[i] && !padXInput[i] && IsXInputDevice(padDevices[i]))
            {
                padXInput[i] = 1;
                padType[i] = 5; // the Xbox 360 pad's
            }
        }
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

    pattern = hook::pattern("89 BE ? ? ? ? 89 3D ? ? ? ? EB 06 89 9E");
    padFound = *pattern.get_first<uint32_t*>(2);
    padXInput = padFound - 2;
    padDevices = padXInput - 3;
    padType = padXInput - 9;
    pattern = hook::pattern("55 8B EC 83 EC 1C A1 ? ? ? ? 53 56 57 33 FF 47 57 33 DB 53 68 ? ? ? ? 6A 04");
    shJoystickInit = safetyhook::create_inline(pattern.get_first(), JoystickInit);
}
