module;

#include <stdafx.h>

export module Focus;

import ComVars;

export namespace BullyFocus
{
    SafetyHookInline ActivationHook, ActiveHook;
    SafetyHookMid PumpHook;
    const int* PauseOnFocusLoss = nullptr;
    int(__thiscall* PauseAudio)(void*) = nullptr;
    int(__thiscall* ResumeAudio)(void*) = nullptr;
    uintptr_t PumpReturn = 0;
    bool HaveFocusEvent = false;

    bool BackgroundRunning()
    {
        return HaveFocusEvent && FocusActive.is_initialized() && !FocusActive && PauseOnFocusLoss && !*PauseOnFocusLoss;
    }

    void Update()
    {
        if (!HaveFocusEvent || !FocusActive.is_initialized() || FocusActive)
            return;
        const bool pause = !BackgroundRunning();
        FocusTimerPaused = pause;
        if (pause && !FocusMutexOwned && FocusStreamingMutex)
        {
            const auto result = WaitForSingleObject(FocusStreamingMutex, INFINITE);
            if (result == WAIT_OBJECT_0 || result == WAIT_ABANDONED)
                FocusMutexOwned = 1;
        }
        else if (!pause && FocusMutexOwned)
        {
            if (ReleaseMutex(FocusStreamingMutex))
                FocusMutexOwned = 0;
        }
        if (!FocusAudioReady)
            return;
        if (pause && !FocusFocusMuted && !FocusAudio[53])
        {
            PauseAudio(FocusAudio.get().data());
            FocusFocusMuted = 1;
        }
        else if (!pause && FocusFocusMuted)
        {
            ResumeAudio(FocusAudio.get().data());
            FocusFocusMuted = 0;
        }
    }

    LONG __cdecl Activation(char active)
    {
        const auto result = ActivationHook.ccall<LONG>(active);
        HaveFocusEvent = true;
        Update();
        return result;
    }

    char __cdecl IsActive()
    {
        return ActiveHook.ccall<char>() || BackgroundRunning();
    }

    void Pump(SafetyHookContext& regs)
    {
        Update();
        if (BackgroundRunning())
            regs.eip = PumpReturn;
        else if (FocusActive.is_initialized() && !FocusActive)
            MsgWaitForMultipleObjectsEx(0, nullptr, 50, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
    }

    void ResetHooks()
    {
        ActivationHook.reset();
        ActiveHook.reset();
        PumpHook.reset();
        HaveFocusEvent = false;
    }

    void Install(const int& pause, HMODULE module = GetModuleHandle(nullptr))
    {
        auto activation = hook::module_pattern(module, "83 EC 08 53 8A 5C 24 10 84 DB 74 18 80 3D ? ? ? ? 00 75 0F 68 ? ? ? ? E8");
        auto active = hook::module_pattern(module, "A1 ? ? ? ? C3 CC CC CC CC CC CC CC CC CC CC A0 ? ? ? ? C3");
        auto pump = hook::module_pattern(module, "80 3D ? ? ? ? 00 74 C3 5F 5E 5B C3");
        auto state = hook::module_pattern(module, "80 3D ? ? ? ? 00 A1 ? ? ? ? 88 1D ? ? ? ? A3 ? ? ? ? 74 19 80 3D ? ? ? ? 00 75 10 B9 ? ? ? ? E8 ? ? ? ? 88 1D ? ? ? ?");
        auto mutex = hook::module_pattern(module, "80 3D ? ? ? ? 00 C6 05 ? ? ? ? 00 74 13 A1 ? ? ? ? 50 FF 15 ? ? ? ? C6 05 ? ? ? ? 00");
        auto resume = hook::module_pattern(module, "80 3D ? ? ? ? 00 74 1A 80 3D ? ? ? ? 00 74 11 B9 ? ? ? ? E8 ? ? ? ? C6 05 ? ? ? ? 00");
        PauseOnFocusLoss = &pause;
        FocusActive.SetAddress(*pump.get_first<uint8_t*>(2));
        FocusAudioReady.SetAddress(*state.get_first<uint8_t*>(2));
        FocusTimerPaused.SetAddress(*state.get_first<uint8_t*>(14));
        FocusAudio.SetAddress(*state.get_first<uint8_t*>(35));
        FocusFocusMuted.SetAddress(*state.get_first<uint8_t*>(46));
        FocusMutexOwned.SetAddress(*mutex.get_first<uint8_t*>(2));
        FocusStreamingMutex.SetAddress(*mutex.get_first<HANDLE*>(17));
        PauseAudio = reinterpret_cast<decltype(PauseAudio)>(injector::GetBranchDestination(state.get_first(39)).as_int());
        ResumeAudio = reinterpret_cast<decltype(ResumeAudio)>(injector::GetBranchDestination(resume.get_first(23)).as_int());
        PumpReturn = reinterpret_cast<uintptr_t>(pump.get_first(9));
        ActivationHook = safetyhook::create_inline(activation.get_first(), Activation);
        ActiveHook = safetyhook::create_inline(active.get_first(16), IsActive);
        PumpHook = safetyhook::create_mid(pump.get_first(), Pump);
    }
}
