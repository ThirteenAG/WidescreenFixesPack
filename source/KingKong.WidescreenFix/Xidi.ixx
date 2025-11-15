module;

#include <stdafx.h>

export module Xidi;

import ComVars;

typedef bool (*XidiSendVibrationFunc)(unsigned int, unsigned short, unsigned short);
export XidiSendVibrationFunc XidiSendVibration = nullptr;

SafetyHookInline shUpdateRumble = {};
void __cdecl UpdateRumble(int a1)
{
    if (!XidiSendVibration)
        return;

    bool bVibration = false;
    if (pSettings)
    {
        auto Settings = *(uintptr_t*)pSettings;
        if (Settings)
        {
            bVibration = *(int*)(*(uintptr_t*)pSettings + 0x4DB4);
            if (!bVibration)
                return;
        }
    }

    XidiSendVibration(-1, static_cast<WORD>((float)a1 * 257.0f), static_cast<WORD>((float)a1 * 257.0f));
}

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

        if (XidiSendVibration)
        {
            auto pattern = hook::pattern("51 8B 44 24 ? 85 C0 7F");
            if (!pattern.empty())
                shUpdateRumble = safetyhook::create_inline(pattern.get_first(), UpdateRumble);
        }

        if (XidiRegisterProfileCallback)
        {
            static bool bPlayingVideo = false;
            auto pattern = hook::pattern("FF 15 ? ? ? ? 39 1D");
            if (!pattern.empty())
            {
                static auto VideosKeyPressCheck = safetyhook::create_mid(pattern.get_first(0), [](SafetyHookContext& regs)
                {
                    bPlayingVideo = true;
                });
            }

            pattern = hook::pattern("E8 ? ? ? ? A1 ? ? ? ? 3B C3 74");
            if (!pattern.empty())
            {
                static auto VideosHookEnd = safetyhook::create_mid(pattern.get_first(0), [](SafetyHookContext& regs)
                {
                    bPlayingVideo = false;
                });
            }

            XidiRegisterProfileCallback([]() -> const wchar_t*
            {
                if (bPlayingVideo)
                    return L"Video";
                return L"Main";
            });
        }
    }
}