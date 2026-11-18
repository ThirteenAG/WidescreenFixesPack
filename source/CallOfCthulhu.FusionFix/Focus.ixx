module;

#include <stdafx.h>
#include <d3d9.h>
#include "FixSettings.hpp"

#ifndef DIRECTINPUT_VERSION
#define DIRECTINPUT_VERSION 0x0800
#endif
#include <dinput.h>

export module Focus;

import ComVars;
import Window;

export namespace CoCFocus
{
    inline std::atomic<bool> pause{ true };

    inline void Select(SafetyHookContext& context)
    {
        // Skip only the deactivation pause body. Native active-state and
        // keyboard-hook bookkeeping still receives the original wParam.
        if (!pause.load(std::memory_order_relaxed) && !context.esi)
            context.eflags &= ~uintptr_t(0x40);
    }

    // The game frame only runs while the app is marked active. A windowed device keeps working in the background,
    // so let the frame run there too; a full screen device is lost while away and stays gated.
    inline void Run(SafetyHookContext& context)
    {
        if (pause.load(std::memory_order_relaxed) || (context.eax & 0xFF))
            return;
        if (CoCWindow::hasPresentation && PresentationParameters->Windowed)
            context.eax |= 1;
    }

    inline void Init()
    {
        pause = FixSettings().Bool("MAIN.PauseOnFocusLoss");
        FixSettings().OnChange("MAIN.PauseOnFocusLoss", []()
        {
            pause = FixSettings().Bool("MAIN.PauseOnFocusLoss");
        });

        auto branch = hook::pattern("85 F6 75 69 E8 ? ? ? ? 84 C0 75 60 E8 ? ? ? ? 8B C8 E8 ? ? ? ? 84 C0 74 50");
        static auto FocusHook = safetyhook::create_mid(branch.get_first(2), Select);

        auto active = hook::pattern("83 EC 4C 57 8B F9 E8 ? ? ? ? 8B C8 E8 ? ? ? ? 84 C0 0F 84");
        static auto ActiveHook = safetyhook::create_mid(active.get_first(18), Run);
    }
}

// The game reads only the DirectInput keyboard and mouse. The bundled Xidi turns the controller into keys and mouse
// movement (Xidi.ini), and starts reading the controller once a game controller device exists, so one is opened on
// the game's DirectInput object.
export namespace CoCPad
{
    inline HRESULT(WINAPI* NativeCreate)(HINSTANCE, DWORD, REFIID, LPVOID*, LPUNKNOWN) = nullptr;
    inline IDirectInputDevice8A* controller = nullptr;

    inline BOOL CALLBACK First(LPCDIDEVICEINSTANCEA instance, LPVOID context)
    {
        *static_cast<GUID*>(context) = instance->guidInstance;
        return DIENUM_STOP;
    }

    inline HRESULT WINAPI Create(HINSTANCE instance, DWORD version, REFIID iid, LPVOID* output, LPUNKNOWN outer)
    {
        const auto result = NativeCreate(instance, version, iid, output, outer);
        if (SUCCEEDED(result) && output && *output && !controller && GetModuleHandleW(L"Xidi.32.dll"))
        {
            auto input = static_cast<IDirectInput8A*>(*output);
            GUID guid{};
            if (SUCCEEDED(input->EnumDevices(DI8DEVCLASS_GAMECTRL, First, &guid, DIEDFL_ATTACHEDONLY)) && guid != GUID{})
                input->CreateDevice(guid, &controller, nullptr);
        }
        return result;
    }

    inline void Init()
    {
        auto original = IATHook::Replace(GetModuleHandleW(nullptr), "DINPUT8.dll", std::forward_as_tuple("DirectInput8Create", Create));
        NativeCreate = reinterpret_cast<decltype(NativeCreate)>(original.at("DirectInput8Create").get());
    }
}
