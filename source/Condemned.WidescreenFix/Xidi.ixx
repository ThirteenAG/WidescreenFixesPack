module;

#include <stdafx.h>

export module Xidi;

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
}