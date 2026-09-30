module;

#include <stdafx.h>
#include <xinput.h>

export module Splitscreen;

import ComVars;

// Two instances of the game on one PC, connected over LAN, each in half of the screen:
// - the first instance starts the second one (SCC_SPLITSCREEN_INSTANCE=2 in its environment), which is closed with it
// - both are windowed, the resolution and window of each is its half of the primary monitor (Layout: top and bottom, left and right, set in Window.ixx)
// - WinMain quits when the "sc5_semaphore" semaphore exists, the second instance uses another one
// - each instance reads its own pad (GamepadInstance1/2), both see it as the first pad, Use Controller is set for the one with a pad
// - the pad is read without focus (UWindowsViewport reads it only when the window has focus), the mouse and keyboard stay with the focused window
namespace Splitscreen
{
    enum class Layout { TopBottom, LeftRight };
    Layout layout = Layout::TopBottom;

    SafetyHookInline shCreateSemaphoreA = {};
    HANDLE WINAPI CreateSemaphoreA(LPSECURITY_ATTRIBUTES attributes, LONG initialCount, LONG maximumCount, LPCSTR name)
    {
        if (!bInstance1 && name && strcmp(name, "sc5_semaphore") == 0)
            name = "sc5_semaphore2";
        return shCreateSemaphoreA.stdcall<HANDLE>(attributes, initialCount, maximumCount, name);
    }

    void LaunchSecondInstance()
    {
        wchar_t exePath[MAX_PATH] = {};
        GetModuleFileNameW(nullptr, exePath, MAX_PATH);
        std::wstring commandLine = GetCommandLineW();
        wchar_t currentDirectory[MAX_PATH] = {};
        GetCurrentDirectoryW(MAX_PATH, currentDirectory);

        // closed together with this instance
        static auto job = CreateJobObjectW(nullptr, nullptr);
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION info = {};
        info.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        SetInformationJobObject(job, JobObjectExtendedLimitInformation, &info, sizeof(info));

        SetEnvironmentVariableW(L"SCC_SPLITSCREEN_INSTANCE", L"2");
        STARTUPINFOW si = { sizeof(si) };
        PROCESS_INFORMATION pi = {};
        if (CreateProcessW(exePath, commandLine.data(), nullptr, nullptr, FALSE, CREATE_SUSPENDED, nullptr, currentDirectory, &si, &pi))
        {
            AssignProcessToJobObject(job, pi.hProcess);
            ResumeThread(pi.hThread);
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
        }
        SetEnvironmentVariableW(L"SCC_SPLITSCREEN_INSTANCE", nullptr);
    }

    decltype(&::XInputGetState) GetState = nullptr;
    decltype(&::XInputSetState) SetState = nullptr;

    // pad of each instance, 1-4, 0 for none
    int32_t padInstance1 = 1;
    int32_t padInstance2 = 2;

    DWORD Pad(DWORD userIndex)
    {
        auto pad = bInstance1 ? padInstance1 : padInstance2;
        return userIndex == 0 && pad >= 1 && pad <= XUSER_MAX_COUNT ? pad - 1 : XUSER_MAX_COUNT;
    }

    DWORD WINAPI XInputGetState(DWORD userIndex, XINPUT_STATE* state)
    {
        auto pad = Pad(userIndex);
        return pad < XUSER_MAX_COUNT && GetState ? GetState(pad, state) : ERROR_DEVICE_NOT_CONNECTED;
    }

    DWORD WINAPI XInputSetState(DWORD userIndex, XINPUT_VIBRATION* vibration)
    {
        auto pad = Pad(userIndex);
        return pad < XUSER_MAX_COUNT && SetState ? SetState(pad, vibration) : ERROR_DEVICE_NOT_CONNECTED;
    }

    // Pads are found with DirectInput (UWindowsViewport joystick init, sub_8F3E00), each one read with XInput or DirectInput,
    // only XInput is remapped per instance: XInput for every pad, and an instance without a pad doesn't find any
    uint32_t* padsFound = nullptr;  // dword_1431E4C
    uint32_t* padXInput = nullptr;  // dword_1431E34[2]

    // Profile settings (FProfileManager, sub_790ED4), Use Controller at +10h is on for the instance with a pad
    SafetyHookInline shGetProfileSettings = {};
    uint8_t* __fastcall GetProfileSettings(void* profileManager, void* edx)
    {
        auto settings = shGetProfileSettings.fastcall<uint8_t*>(profileManager, edx);
        if (settings)
            *reinterpret_cast<int32_t*>(settings + 0x10) = (bInstance1 ? padInstance1 : padInstance2) > 0;
        return settings;
    }

    // UWindowsViewport, can the pad be read: has focus (+414h), pads were found, ...
    SafetyHookInline shCanReadPad = {};
    int __fastcall CanReadPad(uintptr_t viewport, void* edx)
    {
        auto& focus = *reinterpret_cast<int32_t*>(viewport + 0x414);
        auto hadFocus = std::exchange(focus, 1);
        auto result = shCanReadPad.fastcall<int>(viewport, edx);
        focus = hadFocus;
        return result;
    }
}

// Half of the primary monitor this instance uses
export RECT GetSplitscreenRect()
{
    MONITORINFO info = { sizeof(info) };
    GetMonitorInfo(MonitorFromPoint({ 0, 0 }, MONITOR_DEFAULTTOPRIMARY), &info);
    auto r = info.rcMonitor;
    if (Splitscreen::layout == Splitscreen::Layout::LeftRight)
    {
        auto middle = r.left + (r.right - r.left) / 2;
        return bInstance1 ? RECT{ r.left, r.top, middle, r.bottom } : RECT{ middle, r.top, r.right, r.bottom };
    }
    auto middle = r.top + (r.bottom - r.top) / 2;
    return bInstance1 ? RECT{ r.left, r.top, r.right, middle } : RECT{ r.left, middle, r.right, r.bottom };
}

export void InitSplitscreen()
{
    if (!bEnableSplitscreen)
        return;

    wchar_t instance[8] = {};
    bInstance1 = GetEnvironmentVariableW(L"SCC_SPLITSCREEN_INSTANCE", instance, 8) == 0 || wcscmp(instance, L"2") != 0;
    if (bInstance1)
        Splitscreen::LaunchSecondInstance();

    CIniReader iniReader("");
    Splitscreen::layout = iniReader.ReadInteger("2INSTANCESPLITSCREEN", "Layout", 0) == 1 ? Splitscreen::Layout::LeftRight : Splitscreen::Layout::TopBottom;
    Splitscreen::padInstance1 = iniReader.ReadInteger("2INSTANCESPLITSCREEN", "GamepadInstance1", 1);
    Splitscreen::padInstance2 = iniReader.ReadInteger("2INSTANCESPLITSCREEN", "GamepadInstance2", 2);

    Splitscreen::shCreateSemaphoreA = safetyhook::create_inline(::CreateSemaphoreA, Splitscreen::CreateSemaphoreA);

    if (auto xinput = LoadLibraryW(L"xinput1_3.dll"))
    {
        Splitscreen::GetState = reinterpret_cast<decltype(Splitscreen::GetState)>(GetProcAddress(xinput, "XInputGetState"));
        Splitscreen::SetState = reinterpret_cast<decltype(Splitscreen::SetState)>(GetProcAddress(xinput, "XInputSetState"));
    }
    IATHook::Replace(GetModuleHandleA(NULL), "XINPUT1_3.DLL",
        std::forward_as_tuple("XInputGetState", Splitscreen::XInputGetState),
        std::forward_as_tuple("XInputSetState", Splitscreen::XInputSetState)
    );

    auto pattern = hook::pattern("89 BE ? ? ? ? 89 3D ? ? ? ? EB 06 89 9E ? ? ? ? 83 C6 04 83 FE 08 0F 8C ? ? ? ? 5F 5E 5B C9 C3");
    Splitscreen::padXInput = *pattern.get_first<uint32_t*>(2) - 2; // dword_1431E3C - 8
    Splitscreen::padsFound = *pattern.get_first<uint32_t*>(8);
    static auto PadsFoundHook = safetyhook::create_mid(pattern.get_first(32), [](SafetyHookContext& regs)
    {
        if ((bInstance1 ? Splitscreen::padInstance1 : Splitscreen::padInstance2) <= 0)
            *Splitscreen::padsFound = 0;
        else
            Splitscreen::padXInput[0] = Splitscreen::padXInput[1] = 1;
    });

    pattern = hook::pattern("FF B1 30 02 00 00 E8 ? ? ? ? 50 E8 ? ? ? ? C3");
    Splitscreen::shGetProfileSettings = safetyhook::create_inline(pattern.get_first(), Splitscreen::GetProfileSettings);

    pattern = hook::pattern("33 C0 39 81 14 04 00 00 74 ? 39 05");
    Splitscreen::shCanReadPad = safetyhook::create_inline(pattern.get_first(), Splitscreen::CanReadPad);
}
