#pragma once

#include <Windows.h>
#include <Xinput.h>
#include <initializer_list>

namespace PadInput
{
    struct XInputApi
    {
        decltype(&XInputGetState) getState = nullptr;
        decltype(&XInputSetState) setState = nullptr;
    };

    inline const XInputApi& XInput()
    {
        static XInputApi api = []
        {
            XInputApi found;
            for (auto name : { L"xinput1_4.dll", L"xinput9_1_0.dll", L"xinput1_3.dll" })
            {
                auto dll = LoadLibraryW(name);
                auto getState = dll ? GetProcAddress(dll, "XInputGetState") : nullptr;
                if (!getState)
                    continue;
                found.getState = reinterpret_cast<decltype(&XInputGetState)>(getState);
                found.setState = reinterpret_cast<decltype(&XInputSetState)>(GetProcAddress(dll, "XInputSetState"));
                break;
            }
            return found;
        }();
        return api;
    }

    inline bool Read(XINPUT_STATE& state, DWORD& slot)
    {
        state = {};
        auto getState = XInput().getState;
        if (!getState)
            return false;
        for (DWORD i = 0; i < XUSER_MAX_COUNT; ++i)
        {
            if (getState(i, &state) == ERROR_SUCCESS)
            {
                slot = i;
                return true;
            }
        }
        return false;
    }

    enum class Family { Xbox = 1, PlayStation = 2, Nintendo = 3 };

    inline Family ResolveFamily(int selected)
    {
        return selected >= 1 && selected <= 3 ? Family(selected) : Family::Xbox;
    }

    inline const wchar_t* ButtonName(WORD button, Family family)
    {
        bool ps = family == Family::PlayStation;
        bool nintendo = family == Family::Nintendo;
        switch (button)
        {
            case XINPUT_GAMEPAD_A: return ps ? L"CROSS" : nintendo ? L"B" : L"A";
            case XINPUT_GAMEPAD_B: return ps ? L"CIRCLE" : nintendo ? L"A" : L"B";
            case XINPUT_GAMEPAD_X: return ps ? L"SQUARE" : nintendo ? L"Y" : L"X";
            case XINPUT_GAMEPAD_Y: return ps ? L"TRIANGLE" : nintendo ? L"X" : L"Y";
            case XINPUT_GAMEPAD_START: return ps ? L"OPTIONS" : nintendo ? L"PLUS" : L"MENU";
            case XINPUT_GAMEPAD_BACK: return ps ? L"SHARE" : nintendo ? L"MINUS" : L"VIEW";
        }
        return L"?";
    }
}
