module;

#include <stdafx.h>

export module Xidi;

typedef bool (*XidiSendVibrationFunc)(unsigned int, unsigned short, unsigned short);
export XidiSendVibrationFunc XidiSendVibration = nullptr;
export bool bShowingCursor = false;
export bool bActionKeyAssignmentInProgress = false;

export void InitXidi()
{
    typedef void (*XidiSetButtonLabelFunc)(int index, const char* label);
    typedef bool (*XidiRegisterProfileCallbackFunc)(const wchar_t* (*callback)());
    auto xidiModule = GetModuleHandleW(L"Xidi.32.dll");

    if (xidiModule)
    {
        auto XidiRegisterProfileCallback = (XidiRegisterProfileCallbackFunc)GetProcAddress(xidiModule, "XidiRegisterProfileCallback");
        auto XidiSetButtonLabel = (XidiSetButtonLabelFunc)GetProcAddress(xidiModule, "XidiSetButtonLabel");
        XidiSendVibration = (XidiSendVibrationFunc)GetProcAddress(xidiModule, "XidiSendVibration");

        if (XidiRegisterProfileCallback)
        {
            XidiRegisterProfileCallback([]() -> const wchar_t*
            {
                if (bShowingCursor && !bActionKeyAssignmentInProgress)
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