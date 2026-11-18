module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/PadInput.hpp"

export module Xidi;

// The device used last decides whether button prompts name keys or controller buttons; the mouse counts only when
// it really moves.
std::atomic<bool> PadLast{ false };
POINT LastCursor{ LONG_MIN, LONG_MIN };

export int PadIcons()
{
    return std::clamp(FixSettings().Int("MAIN.GamepadIcons"), 0, 3);
}

export bool PadUsedLast()
{
    return PadLast.load(std::memory_order_relaxed);
}

// Once per frame
export void PollPad()
{
    XINPUT_STATE state{};
    DWORD slot = 0;
    if (PadInput::Read(state, slot))
    {
        const auto& pad = state.Gamepad;
        const auto moved = [](SHORT value) { return std::abs(int(value)) > 16000; };
        if (pad.wButtons || pad.bLeftTrigger > 64 || pad.bRightTrigger > 64 || moved(pad.sThumbLX) || moved(pad.sThumbLY) ||
            moved(pad.sThumbRX) || moved(pad.sThumbRY))
            PadLast.store(true, std::memory_order_relaxed);
    }
    POINT cursor{};
    if (GetCursorPos(&cursor))
    {
        if (LastCursor.x != LONG_MIN && (std::abs(cursor.x - LastCursor.x) > 8 || std::abs(cursor.y - LastCursor.y) > 8))
            PadLast.store(false, std::memory_order_relaxed);
        LastCursor = cursor;
    }
}

// From the game's window procedure
export void InputMessage(UINT message)
{
    switch (message)
    {
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_XBUTTONDOWN:
        case WM_MOUSEWHEEL:
            PadLast.store(false, std::memory_order_relaxed);
            break;
    }
}

typedef bool (*XidiSendVibrationFunc)(short, unsigned short, unsigned short);
export XidiSendVibrationFunc XidiSendVibration = nullptr;
// Set by the game thread, read by Xidi's polling thread
export std::atomic<bool> bShowingCursor = false;
export std::atomic<bool> bKeyAssignmentInProgress = false;

std::atomic_bool XidiPending{ false };

export void InitXidi()
{
    // Module-load notification can precede the library's CRT initialization.
    XidiPending.store(true, std::memory_order_release);
}

export void UpdateXidi()
{
    if (!XidiPending.load(std::memory_order_acquire))
        return;
    static std::mutex setupMutex;
    const std::lock_guard lock(setupMutex);
    if (!XidiPending.exchange(false, std::memory_order_acquire))
        return;
    typedef void (*XidiSetButtonLabelFunc)(int index, const char* label);
    typedef bool (*XidiRegisterProfileCallbackFunc)(const wchar_t* (*callback)());
    HMODULE xidiModule = nullptr;

    if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_PIN, L"Xidi.32.dll", &xidiModule))
    {
        auto XidiRegisterProfileCallback = (XidiRegisterProfileCallbackFunc)GetProcAddress(xidiModule, "XidiRegisterProfileCallback");
        auto XidiSetButtonLabel = (XidiSetButtonLabelFunc)GetProcAddress(xidiModule, "XidiSetButtonLabel");
        XidiSendVibration = (XidiSendVibrationFunc)GetProcAddress(xidiModule, "XidiSendVibration");

        if (XidiRegisterProfileCallback)
        {
            XidiRegisterProfileCallback([]() -> const wchar_t*
            {
                if (bShowingCursor && !bKeyAssignmentInProgress)
                    return L"Menu";
                return L"Main";
            });
        }

        if (XidiSetButtonLabel)
        {
            const char* labelStrs[] = {
                "(Left Stick X)",
                "(Left Stick Y)",
                "(Right Stick X)",
                "(Right Stick Y)",
                "(D-pad Up)",
                "(D-pad Down)",
                "(D-pad Left)",
                "(D-pad Right)",
                "(LT)",
                "(RT)",
                "(A)",
                "(B)",
                "(X)",
                "(Y)",
                "(LB)",
                "(RB)",
                "(Back)",
                "(Start)",
                "(LS)",
                "(RS)",
            };

            for (auto [index, label] : std::views::enumerate(labelStrs))
            {
                XidiSetButtonLabel(index, label);
            }
        }
    }
    else
    {
        XidiPending.store(true, std::memory_order_release);
    }
}
