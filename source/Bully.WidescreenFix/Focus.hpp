#pragma once

namespace BullyFocus
{
    SafetyHookInline ActivationHook, ActiveHook;
    SafetyHookMid PumpHook;
    const int* PauseOnFocusLoss = nullptr;
    uint8_t* Active = nullptr;
    uint8_t* TimerPaused = nullptr;
    uint8_t* FocusMuted = nullptr;
    uint8_t* AudioReady = nullptr;
    uint8_t* Audio = nullptr;
    uint8_t* MutexOwned = nullptr;
    HANDLE* StreamingMutex = nullptr;
    int(__thiscall* PauseAudio)(void*) = nullptr;
    int(__thiscall* ResumeAudio)(void*) = nullptr;
    uintptr_t PumpReturn = 0;
    bool HaveFocusEvent = false;

    bool BackgroundRunning()
    {
        return HaveFocusEvent && Active && !*Active && PauseOnFocusLoss && !*PauseOnFocusLoss;
    }

    void Update()
    {
        if (!HaveFocusEvent || !Active || *Active)
            return;
        const bool pause = !BackgroundRunning();
        *TimerPaused = pause;
        if (pause && !*MutexOwned && *StreamingMutex)
        {
            const auto result = WaitForSingleObject(*StreamingMutex, INFINITE);
            if (result == WAIT_OBJECT_0 || result == WAIT_ABANDONED)
                *MutexOwned = 1;
        }
        else if (!pause && *MutexOwned)
        {
            if (ReleaseMutex(*StreamingMutex))
                *MutexOwned = 0;
        }
        if (!*AudioReady)
            return;
        if (pause && !*FocusMuted && !Audio[53])
        {
            PauseAudio(Audio);
            *FocusMuted = 1;
        }
        else if (!pause && *FocusMuted)
        {
            ResumeAudio(Audio);
            *FocusMuted = 0;
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
        else if (Active && !*Active)
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
        if (activation.size() != 1 || active.size() != 1 || pump.size() != 1 || state.size() != 1 || mutex.size() != 1 || resume.size() != 1)
        {
            OutputDebugStringA("Bully: background pause patterns unavailable.\n");
            return;
        }
        PauseOnFocusLoss = &pause;
        Active = *pump.get_first<uint8_t*>(2);
        if (*active.get_first<uint8_t*>(17) != Active)
        {
            OutputDebugStringA("Bully: background activity binding unavailable.\n");
            return;
        }
        AudioReady = *state.get_first<uint8_t*>(2);
        TimerPaused = *state.get_first<uint8_t*>(14);
        Audio = *state.get_first<uint8_t*>(35);
        FocusMuted = *state.get_first<uint8_t*>(46);
        MutexOwned = *mutex.get_first<uint8_t*>(2);
        StreamingMutex = *mutex.get_first<HANDLE*>(17);
        PauseAudio = reinterpret_cast<decltype(PauseAudio)>(injector::GetBranchDestination(state.get_first(39)).as_int());
        ResumeAudio = reinterpret_cast<decltype(ResumeAudio)>(injector::GetBranchDestination(resume.get_first(23)).as_int());
        PumpReturn = reinterpret_cast<uintptr_t>(pump.get_first(9));
        ActivationHook = safetyhook::create_inline(activation.get_first(), Activation);
        ActiveHook = safetyhook::create_inline(active.get_first(16), IsActive);
        PumpHook = safetyhook::create_mid(pump.get_first(), Pump);
        if (!ActivationHook || !ActiveHook || !PumpHook)
        {
            ResetHooks();
            OutputDebugStringA("Bully: background pause hooks unavailable.\n");
        }
    }
}
