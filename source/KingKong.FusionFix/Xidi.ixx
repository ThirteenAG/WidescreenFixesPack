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

// The controller works without SettingsApplication: a character without saved pad bindings gets the console layout
// (Jack: A use/help, B magazines, LT arm, RT fire, RB reload, LS crouch, RS precise aim; Kong: A hit, B grab, X jump,
// Y rage; the left stick moves). Saved bindings stay as they are.
class PadDefaults
{
public:
    PadDefaults()
    {
        WFP::onInitEvent() += []()
        {
            if (Screen.bSignatureEdition)
                return;
            // Per character 14 button masks: crouch, inventory up, inventory down, run toggle, up, down, left, right,
            // run, look mode, use object 2, super action, speed up, primary action
            auto masks = hook::pattern("68 ? ? ? ? BE 04 00 00 00 51 89 74 24 28 FF D5 85 C0 75 0F 8B 44 24 14 8B D7 6B D2 38 89 82");
            // Per character 20 button names of 65 characters, in the order of the settings' action names
            auto names = hook::pattern("8B DF 69 DB 14 05 00 00 8D 04 BF C1 E0 07 81 C3");
            auto read = hook::pattern("B8 01 00 00 00 E8 ? ? ? ? 8B 55 08 52 89 1D");
            static auto Masks = *masks.get_first<uint32_t*>(32);
            static auto Names = *names.get_first<char*>(16);
            static auto ReadHook = safetyhook::create_mid(read.get_first(10), [](SafetyHookContext&)
            {
                constexpr uint32_t Up = 0x100000, Down = 0x200000, Left = 0x400000, Right = 0x800000;
                constexpr std::array<uint32_t, 14> defaults[2] =
                {
                    { 0x001, 0x002, 0x400, 0, Up, Down, Left, Right, 0, 0x080, 0x040, 0x020, 0, 0x800 },
                    { 0x001, 0x002, 0x004, 0x008, Up, Down, Left, Right, 0, 0, 0, 0, 0, 0 },
                };
                // Name slots: 0 primary action, 3 super action, 4 look mode, 6 crouch, 7 inventory up,
                // 8 inventory down, 9-12 up/down/left/right, 13 run toggle, 19 use object 2
                struct Label
                {
                    int slot;
                    const char* text;
                };
                static constexpr Label jack[] = { { 0, "(RS)" }, { 3, "(RB)" }, { 4, "(RT)" }, { 6, "(A)" }, { 7, "(B)" },
                    { 8, "(LS)" }, { 9, "Stick up" }, { 10, "Stick down" }, { 11, "Stick left" }, { 12, "Stick right" }, { 19, "(LT)" } };
                static constexpr Label kong[] = { { 6, "(A)" }, { 7, "(B)" }, { 8, "(X)" }, { 13, "(Y)" }, { 9, "Stick up" },
                    { 10, "Stick down" }, { 11, "Stick left" }, { 12, "Stick right" } };
                const std::span<const Label> labels[2] = { jack, kong };
                for (int character = 0; character < 2; ++character)
                {
                    auto* masks = Masks + 14 * character;
                    if (std::any_of(masks, masks + 14, [](uint32_t mask) { return mask != 0; }))
                        continue;
                    std::copy(defaults[character].begin(), defaults[character].end(), masks);
                    for (const auto& label : labels[character])
                        strcpy_s(Names + 1300 * character + 65 * label.slot, 65, label.text);
                }
            });
        };
    }
} PadDefaults;
