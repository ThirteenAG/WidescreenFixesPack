module;

#include <stdafx.h>
#include <atomic>
#include <stdexcept>

export module Xidi;

import ComVars;
import GUI;
import HudIDs;

namespace
{
    struct NativeString
    {
        const wchar_t* data;
        int count;
        int capacity;
    };

    struct Joystick
    {
        std::byte device[48];
        uint16_t vendor, product;
        std::byte properties[12];
        int strafe, forward, turn, look, fire, altFire;
        NativeString* buttons;
        int count, capacity;
        NativeString dpad[4];
        int profileIndex, flags;
    };
    static_assert(offsetof(Joystick, buttons) == 0x58 && offsetof(Joystick, dpad) == 0x64);
    static_assert(sizeof(Joystick) == 0x9C);

    struct JoystickManager
    {
        std::byte properties[0x3C];
        Joystick* devices;
        int count;
    };

    struct BindingList
    {
        NativeString* data;
        int count, capacity;
    };

    NativeString* (__thiscall* assignString)(NativeString*, const NativeString&);
    int (__thiscall* readCurrentBindings)(JoystickManager*, int, BindingList*);
    SafetyHookInline readDefaults;

    void SetBinding(NativeString& target, std::wstring_view binding)
    {
        const int length = binding.empty() ? 0 : int(binding.size()) + 1;
        assignString(&target, { binding.data(), length, length });
    }

    bool IsXidiController(const Joystick& joystick)
    {
        return joystick.vendor == 0xFFFF && joystick.product >= 1 && joystick.product <= 4;
    }

    void SetDefaultBindings(NativeString* buttons, int count, NativeString* dpad)
    {
        constexpr std::wstring_view bindings[] = {
            L"Interaction", L"Duck", L"Scope", L"Jump", L"QuickInventory", L"CoopActionWhistle",
            L"AltFire", L"Fire", L"OpsatMenu", L"PauseMenu", L"SwitchHandZoom", L"ZoomToggle",
            L"EEVMode", L"StickyCamMode", L"NightVision", L"ThermalVision"
        };
        for (int i = 0; i < std::min(count, int(std::size(bindings))); ++i)
            SetBinding(buttons[i], bindings[i]);
        for (int i = 0; i < 4; ++i)
            SetBinding(dpad[i], L"");
    }

    void SetControllerDefaults(Joystick& joystick)
    {
        if (!IsXidiController(joystick))
            return;

        // The native axis identifiers for X, Y, Z and Z rotation.
        joystick.strafe = 1;
        joystick.forward = 2;
        joystick.turn = 3;
        joystick.look = 6;
        SetDefaultBindings(joystick.buttons, joystick.count, joystick.dpad);
    }

    int __fastcall ReadDefaultBindings(JoystickManager* manager, void*, int index, BindingList* pending)
    {
        if (readDefaults.thiscall<int>(manager, index, pending))
            return 1;
        if (index < 0 || index >= manager->count || !IsXidiController(manager->devices[index]))
            return 0;

        // The native getter pads the editable list to sixteen buttons and appends four hat directions.
        // Only the menu's Save action should change the player's active bindings.
        if (!readCurrentBindings(manager, index, pending))
            return 0;
        const int buttonCount = pending->count - 4;
        SetDefaultBindings(pending->data, buttonCount, pending->data + buttonCount);
        return 1;
    }
}

export void InitControllerDefaults()
{
    auto pattern = hook::pattern("8D 44 24 14 50 8D 4D 64 E8 ? ? ? ? 8B 0D ? ? ? ? 8B 11 56");
    assignString = reinterpret_cast<decltype(assignString)>(injector::GetBranchDestination(pattern.get_first(8)).as_int());

    pattern = hook::pattern("56 57 8B 7C 24 0C 85 FF 8B F1 7C ? 3B 7E 40 7D ? 53 8B 5C 24 14 8B CB");
    readCurrentBindings = reinterpret_cast<decltype(readCurrentBindings)>(pattern.get_first());
    pattern = hook::pattern("8B 44 24 04 83 EC 0C 85 C0 56 57 0F 8C ? ? ? ? 3B 41 40 0F 8D ? ? ? ? 8B 51 3C 69 C0 9C 00 00 00");
    readDefaults = safetyhook::create_inline(pattern.get_first(), ReadDefaultBindings);

    // The game has populated the button array from its generic fallback because neither profile matched.
    pattern = hook::pattern("8B 44 24 28 68 ? ? ? ? 89 BB 94 00 00 00 8B 0D");
    static auto newController = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        SetControllerDefaults(*reinterpret_cast<Joystick*>(regs.ebx));
    });

    // Restore Defaults normally reads the device's section from JoysticksDefault.
    pattern = hook::pattern("FF 52 18 85 C0 5F 74 ? 8B 44 24 08 85 C0 8B 44 24 04");
    static auto resetController = safetyhook::create_mid(pattern.get_first(3), [](SafetyHookContext& regs)
    {
        if (!regs.eax)
            SetControllerDefaults(*reinterpret_cast<Joystick*>(regs.esi));
    });
}

typedef bool (*XidiSendVibrationFunc)(short, unsigned short, unsigned short);
export std::atomic<XidiSendVibrationFunc> XidiSendVibration{ nullptr };

// Chosen on the game thread, which owns the menu state it depends on, and read by Xidi's polling
// thread. No preference (the configured mapper) until the first frame.
std::atomic<const wchar_t*> XidiProfile = nullptr;

const wchar_t* SelectXidiProfile()
{
    if (CMenusManager::IsMenuDisplayed(Page::P_Controls_joystick) && !CMenusManager::IsMenuDisplayed(Page::P_Controls_Popup_Joy_Selection))
        return L"P_Controls_joystick";
    else if (!CMenusManager::IsMenuDisplayed(Page::P_Map) && (CMenusManager::IsOpsatDisplayed() || CMenusManager::IsMenuDisplayed(Page::P_Controls_Popup_Joy_Selection)))
        return L"Opsat";
    else if (CMenusManager::IsMainMenuDisplayed())
        return L"Menu";
    return L"Main";
}

// Called once per frame from the game thread
export void UpdateXidiProfile()
{
    XidiProfile.store(SelectXidiProfile(), std::memory_order_relaxed);
}

export void InitXidi()
{
    typedef bool (*XidiRegisterProfileCallbackFunc)(const wchar_t* (*callback)());
    auto xidiModule = GetModuleHandleW(L"Xidi.32.dll");

    if (!xidiModule)
        throw std::runtime_error("Controller proxy unavailable");
    const auto registerProfile = reinterpret_cast<XidiRegisterProfileCallbackFunc>(GetProcAddress(xidiModule, "XidiRegisterProfileCallback"));
    const auto sendVibration = reinterpret_cast<XidiSendVibrationFunc>(GetProcAddress(xidiModule, "XidiSendVibration"));
    if (!registerProfile || !sendVibration)
        throw std::runtime_error("Controller proxy exports unavailable");
    if (!registerProfile([]() -> const wchar_t*
    {
        return XidiProfile.load(std::memory_order_relaxed);
    }))
        throw std::runtime_error("Controller profile callback rejected");
    XidiSendVibration.store(sendVibration, std::memory_order_release);
}
