module;
#include <stdafx.h>
#include "Shared/PadInput.hpp"
#include "FixSettings.hpp"

export module PadLabels;
import Frame;
import DeveloperOverlay;
import PadBindings;


export namespace KnightRiderPadLabels
{
    inline SafetyHookInline NameHook, DestroyHook;
    inline void* (__thiscall* Construct)(void*, const char*) = nullptr;
    inline void (__thiscall* Refresh)(void*) = nullptr;
    inline uintptr_t FirstLabelReturn = 0;
    inline void* CurrentMenu = nullptr;
    inline std::atomic<bool> UsingPad{ false }, RefreshRequested{ false };
    inline DWORD LastPadSlot = XUSER_MAX_COUNT;
    inline SafetyHookInline SetTextHook, NavigationHook, NavigationDestroyHook;
    inline void (__thiscall* FreeString)(void*) = nullptr;
    inline uint8_t* Navigation = nullptr;
    inline std::string NavigationText[2];

    inline const char* Label(int key)
    {
        static constexpr const char* buttons[3][16]
        {
            { "[A]", "[B]", "[X]", "[Y]", "[LB]", "[RB]", "[LT]", "[RT]", "[VIEW]", "[MENU]", "[LS]", "[RS]", "[DPAD UP]", "[DPAD DOWN]", "[DPAD LEFT]", "[DPAD RIGHT]" },
            { "[CROSS]", "[CIRCLE]", "[SQUARE]", "[TRIANGLE]", "[L1]", "[R1]", "[L2]", "[R2]", "[SHARE]", "[OPTIONS]", "[L3]", "[R3]", "[DPAD UP]", "[DPAD DOWN]", "[DPAD LEFT]", "[DPAD RIGHT]" },
            { "[B]", "[A]", "[Y]", "[X]", "[L]", "[R]", "[ZL]", "[ZR]", "[-]", "[+]", "[LS]", "[RS]", "[DPAD UP]", "[DPAD DOWN]", "[DPAD LEFT]", "[DPAD RIGHT]" }
        };
        const auto family = PadInput::ResolveFamily(std::clamp(FixSettings().Int("MAIN.GamepadIcons"), 0, 3));
        if (key >= 7 && key <= 22)
            return buttons[static_cast<int>(family) - 1][key - 7];
        if (key == 3)
            return "[LS LEFT]";
        if (key == 4)
            return "[LS RIGHT]";
        if (key == 2)
            return buttons[static_cast<int>(family) - 1][6];
        if (key == 5 || key == 1)
            return buttons[static_cast<int>(family) - 1][7];
        return nullptr;
    }

    inline void* __fastcall Name(void* menu, void*, void* output, int key, bool keyboard)
    {
        CurrentMenu = menu;
        const char* label = nullptr;
        if (!keyboard)
            label = Label(key);
        else if (UsingPad.load(std::memory_order_relaxed))
        {
            const auto address = reinterpret_cast<uintptr_t>(_ReturnAddress());
            // The native page builds its normal and highlighted labels in fourteen pairs.
            if (address >= FirstLabelReturn && address < FirstLabelReturn + 28 * 57 && (address - FirstLabelReturn) % 57 == 0)
            {
                static constexpr int actions[14]{ 5, 6, 14, 3, 4, 13, 11, 12, 15, 17, 16, 18, 19, 2 };
                label = Label(KnightRiderPadBindings::BoundKey(actions[(address - FirstLabelReturn) / (2 * 57)], LastPadSlot));
            }
        }
        if (!label)
            return NameHook.thiscall<void*>(menu, output, key, keyboard);
        return Construct(output, label);
    }

    // Frontend footer: the accept label (OK, Yes, ...) and the back label (No, Back, ...) of the shared
    // navigation form. Their key pictures stay; the pad button is written before the text while a pad is used.
    inline void* NavigationLabel(int slot)
    {
        if (!Navigation)
            return nullptr;
        const auto label = *reinterpret_cast<uint8_t**>(Navigation + (slot == 0 ? 0x2D4 : 0x2C0));
        return label ? label + 0x628 : nullptr;
    }

    inline void __fastcall SetText(void* text, void*, char* value)
    {
        int slot = -1;
        for (int i = 0; i < 2; ++i)
        {
            if (text && text == NavigationLabel(i))
                slot = i;
        }
        if (slot < 0 || !value)
            return SetTextHook.thiscall<void>(text, value);
        NavigationText[slot] = value + 6;
        if (!UsingPad.load(std::memory_order_relaxed) || NavigationText[slot].empty())
            return SetTextHook.thiscall<void>(text, value);
        const auto caption = std::format("{} {}", Label(slot == 0 ? 7 : 8), NavigationText[slot]);
        char* replaced = nullptr;
        Construct(&replaced, caption.c_str());
        FreeString(&value);
        // The callee takes over the by-value string reference.
        SetTextHook.thiscall<void>(text, replaced);
    }

    inline void RefreshNavigation()
    {
        for (int slot = 0; slot < 2; ++slot)
        {
            const auto text = NavigationLabel(slot);
            if (!text || NavigationText[slot].empty())
                continue;
            char* value = nullptr;
            Construct(&value, std::string(NavigationText[slot]).c_str());
            SetText(text, nullptr, value);
        }
    }

    inline void __fastcall NavigationReset(uint8_t* navigation, void*)
    {
        Navigation = navigation;
        NavigationHook.thiscall<void>(navigation);
    }

    inline void* __fastcall NavigationDestroy(uint8_t* navigation, void*, int flags)
    {
        if (Navigation == navigation)
        {
            Navigation = nullptr;
            NavigationText[0].clear();
            NavigationText[1].clear();
        }
        return NavigationDestroyHook.thiscall<void*>(navigation, flags);
    }

    inline void Keyboard()
    {
        if (UsingPad.exchange(false))
            RefreshRequested = true;
    }

    inline void Frame()
    {
        static std::array<DWORD, XUSER_MAX_COUNT> packets{};
        static std::array<bool, XUSER_MAX_COUNT> wasConnected{};
        const bool focused = GetForegroundWindow() == WindowedModeWrapper::GameHWND;
        std::array<XINPUT_STATE, XUSER_MAX_COUNT> states{};
        std::array<bool, XUSER_MAX_COUNT> connected{}, activity{};
        DWORD firstConnected = XUSER_MAX_COUNT;
        DWORD firstActive = XUSER_MAX_COUNT;
        const auto getState = GetModuleHandleW(L"Xidi.32.dll") ? PadInput::XInput().getState : nullptr;
        for (DWORD i = 0; getState && i < XUSER_MAX_COUNT; ++i)
        {
            if (getState(i, &states[i]) != ERROR_SUCCESS)
                continue;
            connected[i] = true;
            if (firstConnected == XUSER_MAX_COUNT)
                firstConnected = i;
            const auto& pad = states[i].Gamepad;
            const bool active = pad.wButtons || pad.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD
                || pad.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD
                || std::abs(static_cast<int>(pad.sThumbLX)) > 4915
                || std::abs(static_cast<int>(pad.sThumbLY)) > 4915
                || std::abs(static_cast<int>(pad.sThumbRX)) > XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE
                || std::abs(static_cast<int>(pad.sThumbRY)) > XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE;
            activity[i] = focused && active && (!wasConnected[i] || states[i].dwPacketNumber != packets[i]);
            packets[i] = states[i].dwPacketNumber;
            if (activity[i] && firstActive == XUSER_MAX_COUNT)
                firstActive = i;
        }
        wasConnected = connected;

        DWORD slot = LastPadSlot;
        if (firstActive != XUSER_MAX_COUNT && (slot == XUSER_MAX_COUNT || !activity[slot]))
            slot = firstActive;
        else if (slot == XUSER_MAX_COUNT || !connected[slot])
            slot = firstConnected;
        if (slot != LastPadSlot)
            RefreshRequested = true;
        LastPadSlot = slot;
        const bool present = slot != XUSER_MAX_COUNT;
        if constexpr (WFP::DeveloperOverlay)
            PadInput::ToOverlay(KnightRiderOverlay::Overlay, present && focused ? states[slot].Gamepad : XINPUT_GAMEPAD{});
        if (present && activity[slot] && !UsingPad.exchange(true))
            RefreshRequested = true;
        else if (!present)
            Keyboard();
        if (RefreshRequested.exchange(false))
        {
            if (CurrentMenu)
                Refresh(CurrentMenu);
            RefreshNavigation();
        }
    }

    inline void* __fastcall Destroy(void* menu, void*)
    {
        if (CurrentMenu == menu)
            CurrentMenu = nullptr;
        return DestroyHook.thiscall<void*>(menu);
    }

    inline void Init()
    {
        auto construct = hook::pattern("8B 54 24 04 56 85 D2 8B F1 74 10 57 8B FA 83 C9 FF");
        Construct = reinterpret_cast<decltype(Construct)>(construct.get_first());
        auto name = hook::pattern("51 56 57 8B F9 8D 4C 24 08 E8 ? ? ? ? 8A 44 24 18 84 C0");
        NameHook = safetyhook::create_inline(name.get_first(), Name);
        auto first = hook::pattern("53 56 57 8B 7C 24 10 8B F1 8B CF E8 ? ? ? ? 66 8B 00 66 3B 05");
        FirstLabelReturn = reinterpret_cast<uintptr_t>(first.get_first(0x33));
        auto refresh = hook::pattern("8B 81 C4 02 00 00 05 E0 00 00 00 50 E8 ? ? ? ? C3");
        Refresh = reinterpret_cast<decltype(Refresh)>(refresh.get_first());
        auto destroy = hook::pattern("53 56 8B F1 57 8D 8E 6C 08 00 00 E8");
        DestroyHook = safetyhook::create_inline(destroy.get_first(), Destroy);
        auto freeString = hook::pattern("57 8B F9 8B 07 66 FF 08 8B 07 66 83 38 00 75 24 56 33 F6 66 8B 70 04");
        FreeString = reinterpret_cast<decltype(FreeString)>(freeString.get_first());
        auto setText = hook::pattern("53 56 8B F1 57 8D 4C 24 10 8D 7E 34 57 E8 ? ? ? ? 84 C0 74 61 8A 46 10");
        SetTextHook = safetyhook::create_inline(setText.get_first(), SetText);
        auto navigation = hook::pattern("56 57 8B F9 68 ? ? ? ? 8D 8F A0 02 00 00 E8 ? ? ? ? 8D 87 90 02 00 00 8B CF 50 E8 ? ? ? ? 8B B7 AC 02 00 00");
        NavigationHook = safetyhook::create_inline(navigation.get_first(), NavigationReset);
        auto navigationDestroy = hook::pattern("56 8B F1 8D 8E CC 02 00 00 E8 ? ? ? ? 8D 8E B8 02 00 00 E8 ? ? ? ? 8D 8E A4 02 00 00 E8");
        NavigationDestroyHook = safetyhook::create_inline(navigationDestroy.get_first(), NavigationDestroy);
        FixSettings().OnChange("MAIN.GamepadIcons", []() { RefreshRequested = true; });
        KnightRiderFrame::InputFrame = Frame;
        KnightRiderFrame::KeyboardInput = Keyboard;
    }
}
