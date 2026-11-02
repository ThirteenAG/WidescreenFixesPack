module;

#include <stdafx.h>

export module Xidi;

import ComVars;

typedef bool (*XidiSendVibrationFunc)(short, unsigned short, unsigned short);
std::atomic<XidiSendVibrationFunc> XidiSendVibration = nullptr;

SafetyHookInline shUpdateRumble = {};
void __cdecl UpdateRumble(int a1)
{
    const auto send = XidiSendVibration.load();
    if (!send)
        return;
    if (pSettings.is_initialized())
    {
        uintptr_t Settings = pSettings;
        if (Settings && !*(int*)(Settings + 0x4DB4))
            return;
    }
    send(-1, static_cast<WORD>((float)a1 * 257.0f), static_cast<WORD>((float)a1 * 257.0f));
}

export void InitXidi()
{
    auto xidiModule = GetModuleHandleW(L"Xidi.32.dll");
    if (!xidiModule)
        return;
    static std::once_flag once;
    std::call_once(once, [xidiModule]()
    {
        const auto send = (XidiSendVibrationFunc)GetProcAddress(xidiModule, "XidiSendVibration");
        XidiSendVibration.store(send);
        if (send)
            shUpdateRumble = safetyhook::create_inline(hook::pattern("51 8B 44 24 ? 85 C0 7F").get_first(), UpdateRumble);
    });
}
