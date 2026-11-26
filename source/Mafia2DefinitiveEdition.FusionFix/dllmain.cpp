#include "stdafx.h"
#include "FixSettings.hpp"

import Mouse;

ASI_SETTINGS_EXPORT(FixSettings())

namespace SkipIntro
{
    // Fullscreen video of a GUI screen. A video that doesn't start counts as finished, and the screen moves on.
    SafetyHookInline PlayHook{};
    bool __fastcall Play(void* gui, const char* name, void* finished, uint8_t a, uint8_t b, uint8_t c, uint8_t d)
    {
        // The publisher, developer and port studio logos at startup
        if (name && (_stricmp(name, "Logos.bik") == 0 || _stricmp(name, "d3t.bik") == 0))
            return false;
        return PlayHook.fastcall<bool>(gui, name, finished, a, b, c, d);
    }

    // "Press any key or button to continue": the start screen waits for the PC input event that any key sends.
    // The fix sends that event once, on the frame after the screen first starts listening.
    using InputListenerFn = void(__fastcall*)(void* self, uint32_t key, int32_t event, int32_t state, void* device);
    InputListenerFn InputListener = nullptr;
    uint8_t* InputDevices = nullptr;
    bool startScreenPending = false;
    bool startScreenDone = false;

    SafetyHookInline StartInputHook{};
    bool __fastcall StartInput(void* command, void* args)
    {
        bool result = StartInputHook.fastcall<bool>(command, args);
        if (!startScreenDone)
            startScreenPending = true;
        return result;
    }

    void Frame()
    {
        if (!startScreenPending)
            return;
        // Any connected keyboard, mouse or pad of the game's input list
        auto first = *reinterpret_cast<uint8_t***>(InputDevices + 0x20);
        auto last = *reinterpret_cast<uint8_t***>(InputDevices + 0x28);
        for (auto device = first; device != last; ++device)
        {
            if (*device && *reinterpret_cast<int32_t*>(*device + 0x0C) != 5)
            {
                startScreenPending = false;
                startScreenDone = true;
                InputListener(nullptr, 0, 4, 0, *device);
                break;
            }
        }
    }

    void Init()
    {
        // Loading screen of the first load: the plain one instead of the legal notices
        auto pattern = hook::pattern("48 83 C1 70 BA 01 00 00 00 E9");
        injector::WriteMemory<int32_t>(pattern.get_first<int32_t>(5), 0, true);

        pattern = hook::pattern("48 89 5C 24 10 48 89 6C 24 18 48 89 74 24 20 57 48 83 EC 30 80 B9 D8 00 00 00 00 41 0F B6 F9 49 8B D8");
        PlayHook = safetyhook::create_inline(pattern.get_first(), Play);

        // PC input listener of the GUI; it calls the input device list getter first
        pattern = hook::pattern("48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 48 83 EC 20 48 8B 7C 24 50 41 8B E9 41 8B D8 8B F2 83 7F 0C 05 0F 84");
        InputListener = reinterpret_cast<InputListenerFn>(pattern.get_first());
        auto getDevices = reinterpret_cast<uint8_t* (*)()>(injector::GetBranchDestination(pattern.get_first(0x2B)).as_int());
        InputDevices = getDevices();

        // menu.StartInputNotify
        pattern = hook::pattern("48 83 EC 28 E8 ? ? ? ? 48 8B 88 78 01 00 00 E8 ? ? ? ? E8 ? ? ? ? 48 8B C8 48 8B 10 FF 52 38 E8");
        StartInputHook = safetyhook::create_inline(pattern.get_first(), StartInput);
    }
}

void Init(bool skipIntro)
{
    if (skipIntro)
        SkipIntro::Init();
    InitMouse();

    auto pump = hook::pattern("48 83 EC 68 45 33 C9 C7 44 24 20 01 00 00 00 45 33 C0 48 8D 4C 24 30 33 D2 FF 15");
    // The queue-empty return runs once per pass. The message loop jumps back into the prologue.
    static auto pumpHook = safetyhook::create_mid(pump.get_first(0x5C), [](SafetyHookContext&)
    {
        UpdateMouseRegistration();
        SkipIntro::Frame();
    });
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        const bool skipIntro = FixSettings().Get("MAIN.SkipIntro") == "1";
        asi_settings::LiveLink::Start();
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime([skipIntro] { Init(skipIntro); }, hook::pattern("48 83 EC 68 45 33 C9 C7 44 24 20 01 00 00 00 45 33 C0 48 8D 4C 24 30 33 D2 FF 15"));
    });
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        if (!IsUALPresent()) { InitializeASI(); }
    }
    return TRUE;
}
