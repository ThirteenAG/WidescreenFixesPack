module;
#include <stdafx.h>
#include "Shared/PadInput.hpp"
#include <unordered_map>
#include "FixSettings.hpp"

export module PadLabels;
import ComVars;
import Frame;
import DeveloperOverlay;
import PadBindings;
import Camera;
import Window;


export namespace KnightRider2PadLabels
{
    inline SafetyHookInline NameHook, DestroyHook;
    inline void* (__thiscall* Construct)(void*, const char*) = nullptr;
    inline void (__thiscall* Refresh)(void*) = nullptr;
    inline uintptr_t FirstLabelReturn = 0;
    inline void* CurrentMenu = nullptr;
    inline std::atomic<bool> UsingPad{ false }, RefreshRequested{ false };
    inline DWORD LastPadSlot = 0;
    inline SafetyHookInline FooterHook, FormDestroyHook;
    inline void* FooterWindow = nullptr;
    inline void (__thiscall* FreeString)(void*) = nullptr;
    inline const char* Label(int key);
    inline SafetyHookInline LocalizedHook;
    inline SafetyHookInline TextUpdateHook, TextDestroyHook;
    inline std::unordered_map<void*, int> TextDevices;
    inline void* LocalizedCaption[2]{};
    inline std::string PreviousCaption[2];
    inline WORD Buttons = 0;
    inline WORD QuitHeld = 0;
    inline SafetyHookMid QuitOpenHook, QuitPromptHook;
    inline void (__thiscall* PlayClick)(void*, int) = nullptr;

    // The pause menu's quit question only reads the Enter and Esc keys; A answers yes and B no.
    inline void QuitPrompt(uint8_t* pause)
    {
        const WORD pressed = Buttons & ~QuitHeld;
        QuitHeld = Buttons;
        auto& answer = *reinterpret_cast<int*>(pause + 144);
        if (answer != 2 || !(pressed & (XINPUT_GAMEPAD_A | XINPUT_GAMEPAD_B)))
            return;
        if (const auto click = *reinterpret_cast<void**>(pause + 132))
            PlayClick(click, 0);
        answer = (pressed & XINPUT_GAMEPAD_A) ? 1 : 0;
    }

    inline char __fastcall TextUpdate(uint8_t* text, void*)
    {
        const int key = *reinterpret_cast<int*>(text + 76);
        if (key == 30 || key == 5701)
        {
            const int device = UsingPad.load(std::memory_order_relaxed)
                ? static_cast<int>(PadInput::ResolveFamily(std::clamp(FixSettings().Int("MAIN.GamepadIcons"), 0, 3))) : 0;
            auto& previous = TextDevices[text];
            if (previous != device)
            {
                text[81] = 1;
                previous = device;
            }
        }
        return TextUpdateHook.thiscall<char>(text);
    }

    inline int __fastcall TextDestroy(void* text, void*)
    {
        TextDevices.erase(text);
        return TextDestroyHook.thiscall<int>(text);
    }

    inline void* __fastcall Localized(void* dictionary, void*, int key)
    {
        if (!UsingPad.load(std::memory_order_relaxed) || (key != 30 && key != 5701))
            return LocalizedHook.thiscall<void*>(dictionary, key);
        const int index = key == 30 ? 0 : 1;
        const auto native = LocalizedHook.thiscall<char**>(dictionary, key == 30 ? 26 : 107);
        const auto caption = std::format("{} {}", Label(key == 30 ? 16 : 7), *native + 6);
        if (caption != PreviousCaption[index])
        {
            if (LocalizedCaption[index])
                FreeString(&LocalizedCaption[index]);
            Construct(&LocalizedCaption[index], caption.c_str());
            PreviousCaption[index] = caption;
        }
        return &LocalizedCaption[index];
    }

    inline int __fastcall Footer(void* window, void*, int index, char** value)
    {
        FooterWindow = window;
        if (!UsingPad.load(std::memory_order_relaxed) || index < 0 || index > 2 || !value || !*value)
            return FooterHook.thiscall<int>(window, index, value);
        const auto caption = std::format("{} {}", Label(index == 0 ? 7 : index == 1 ? 8 : 10), *value + 6);
        void* temporary = nullptr;
        Construct(&temporary, caption.c_str());
        const auto result = FooterHook.thiscall<int>(window, index, &temporary);
        FreeString(&temporary);
        return result;
    }

    inline void __fastcall FormDestroy(void* window, void*)
    {
        if (FooterWindow == window)
            FooterWindow = nullptr;
        FormDestroyHook.thiscall<void>(window);
    }

    inline void DrawNavigation(uint8_t* camera)
    {
        std::vector<std::pair<uint8_t*, uint8_t>> hidden;
        if (UsingPad.load(std::memory_order_relaxed))
        {
            const auto visit = [&](auto&& self, uint8_t* object, bool navigation, int depth) -> void
            {
                if (!object || depth > 8)
                    return;
                const auto name = *reinterpret_cast<const char**>(object + 8);
                navigation = navigation || (name && std::strcmp(name, "Navigation") == 0);
                if (navigation && name && (std::strcmp(name, "ObjSquare") == 0
                    || std::strcmp(name, "ObjTriangle") == 0 || std::strcmp(name, "ObjCross") == 0))
                {
                    hidden.emplace_back(object, object[204]);
                    object[204] |= 1;
                    return;
                }
                struct Type { const char* Name; const Type* Base; };
                const auto getType = reinterpret_cast<const Type* (__thiscall*)(void*)>((*reinterpret_cast<void***>(object))[1]);
                for (auto type = getType(object); type; type = type->Base)
                {
                    if (std::strcmp(type->Name, "NiNode") != 0)
                        continue;
                    const auto children = *reinterpret_cast<uint8_t***>(object + 208);
                    const auto count = *reinterpret_cast<uint16_t*>(object + 214);
                    for (uint16_t i = 0; i < count; ++i)
                        self(self, children[i], navigation, depth + 1);
                    break;
                }
            };
            visit(visit, *reinterpret_cast<uint8_t**>(camera + 364), false, 0);
        }
        KnightRider2Camera::DrawHook.thiscall<void>(camera);
        for (const auto& [object, flags] : hidden)
            object[204] = static_cast<uint8_t>((object[204] & ~uint8_t{ 1 }) | (flags & 1));
    }

    inline const char* Label(int key)
    {
        static constexpr const char* buttons[3][16]
        {
            { "[A]", "[B]", "[X]", "[Y]", "[LB]", "[RB]", "[RS LEFT]", "[RS RIGHT]", "[VIEW]", "[MENU]", "[LS]", "[RS]", "[RS UP]", "[RS DOWN]", "[UNUSED]", "[UNUSED]" },
            { "[CROSS]", "[CIRCLE]", "[SQUARE]", "[TRIANGLE]", "[L1]", "[R1]", "[RS LEFT]", "[RS RIGHT]", "[SHARE]", "[OPTIONS]", "[L3]", "[R3]", "[RS UP]", "[RS DOWN]", "[UNUSED]", "[UNUSED]" },
            { "[B]", "[A]", "[Y]", "[X]", "[L]", "[R]", "[RS LEFT]", "[RS RIGHT]", "[-]", "[+]", "[LS]", "[RS]", "[RS UP]", "[RS DOWN]", "[UNUSED]", "[UNUSED]" }
        };
        const auto family = PadInput::ResolveFamily(std::clamp(FixSettings().Int("MAIN.GamepadIcons"), 0, 3));
        if (key >= 23 && key <= 26)
        {
            static constexpr const char* directions[]{ "[DPAD UP]", "[DPAD DOWN]", "[DPAD LEFT]", "[DPAD RIGHT]" };
            return directions[key - 23];
        }
        if (key >= 7 && key <= 22)
            return buttons[static_cast<int>(family) - 1][key - 7];
        if (key == 3)
            return "[LS LEFT]";
        if (key == 4)
            return "[LS RIGHT]";
        if (key == 2)
            return static_cast<int>(family) == 2 ? "[L2]" : static_cast<int>(family) == 3 ? "[ZL]" : "[LT]";
        if (key == 5 || key == 1)
            return static_cast<int>(family) == 2 ? "[R2]" : static_cast<int>(family) == 3 ? "[ZR]" : "[RT]";
        return nullptr;
    }

    inline int BoundKey(int action)
    {
        const auto application = KnightRider2Window::Application.is_initialized() ? KnightRider2Window::Application.get() : nullptr;
        if (!application)
            return KnightRider2PadBindings::DefaultKey(action);
        const auto manager = application + 124;
        const auto actions = *reinterpret_cast<uint8_t***>(manager + 20);
        const auto count = *reinterpret_cast<uint16_t*>(manager + 24);
        for (uint16_t i = 0; i < count; ++i)
        {
            const auto entry = actions[i];
            if (*reinterpret_cast<int*>(entry + 12) != action)
                continue;
            const auto combos = *reinterpret_cast<uint8_t***>(entry + 16);
            const auto comboCount = *reinterpret_cast<uint16_t*>(entry + 20);
            for (uint16_t j = 0; j < comboCount; ++j)
            {
                const auto properties = *reinterpret_cast<uint8_t***>(combos[j] + 8);
                const auto propertyCount = *reinterpret_cast<uint16_t*>(combos[j] + 12);
                for (uint16_t k = 0; k < propertyCount; ++k)
                {
                    const auto property = properties[k];
                    const auto device = *reinterpret_cast<uint8_t**>(property + 12);
                    if (*reinterpret_cast<uint16_t*>(device + 8) == 3
                        && *reinterpret_cast<uint16_t*>(device + 10) == LastPadSlot)
                        return *reinterpret_cast<int*>(property + 8);
                }
            }
        }
        return KnightRider2PadBindings::DefaultKey(action);
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
            // The native page builds its normal and highlighted labels in twenty-three pairs.
            if (address >= FirstLabelReturn && address < FirstLabelReturn + 46 * 59 && (address - FirstLabelReturn) % 59 == 0)
            {
                static constexpr int actions[23]{ 3, 4, 5, 1, 2, 13, 16, 17, 38, 15, 40, 36, 37, 21, 22, 12, 14, 0, 18, 8, 9, 10, 11 };
                label = Label(BoundKey(actions[(address - FirstLabelReturn) / (2 * 59)]));
            }
        }
        if (!label)
            return NameHook.thiscall<void*>(menu, output, key, keyboard);
        return Construct(output, label);
    }

    inline void Keyboard()
    {
        if (UsingPad.exchange(false))
            RefreshRequested = true;
    }

    inline void Frame()
    {
        XINPUT_STATE state{};
        DWORD slot = 0;
        static DWORD previousPacket = 0;
        static bool connected = false;
        const bool present = GetModuleHandleW(L"Xidi.32.dll") && PadInput::Read(state, slot);
        Buttons = present ? state.Gamepad.wButtons : 0;
        if (present)
        {
            if (LastPadSlot != slot)
                RefreshRequested = true;
            LastPadSlot = slot;
        }
        uint16_t overlayButtons = 0;
        if (present)
        {
            const auto buttons = state.Gamepad.wButtons;
            const auto map = [&](WORD source, uint16_t target)
            {
                if (buttons & source)
                    overlayButtons |= target;
            };
            map(XINPUT_GAMEPAD_BACK, 0x1);
            map(XINPUT_GAMEPAD_START, 0x8);
            map(XINPUT_GAMEPAD_DPAD_UP, 0x10);
            map(XINPUT_GAMEPAD_DPAD_RIGHT, 0x20);
            map(XINPUT_GAMEPAD_DPAD_DOWN, 0x40);
            map(XINPUT_GAMEPAD_DPAD_LEFT, 0x80);
            map(XINPUT_GAMEPAD_LEFT_SHOULDER, 0x400);
            map(XINPUT_GAMEPAD_RIGHT_SHOULDER, 0x800);
            map(XINPUT_GAMEPAD_Y, 0x1000);
            map(XINPUT_GAMEPAD_B, 0x2000);
            map(XINPUT_GAMEPAD_A, 0x4000);
            map(XINPUT_GAMEPAD_X, 0x8000);
        }
        KnightRider2Overlay::Overlay.Gamepad(overlayButtons,
            present ? static_cast<int8_t>(state.Gamepad.sThumbLX / 256) : 0,
            present ? static_cast<int8_t>(-state.Gamepad.sThumbLY / 256) : 0);
        if (present)
        {
            const auto& pad = state.Gamepad;
            const bool active = pad.wButtons || pad.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD
                || pad.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD
                || std::abs(static_cast<int>(pad.sThumbLX)) > 4915
                || std::abs(static_cast<int>(pad.sThumbLY)) > 4915
                || std::abs(static_cast<int>(pad.sThumbRX)) > XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE
                || std::abs(static_cast<int>(pad.sThumbRY)) > XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE;
            if (active && (!connected || previousPacket != state.dwPacketNumber)
                && GetForegroundWindow() == WindowedModeWrapper::GameHWND && !UsingPad.exchange(true))
                RefreshRequested = true;
            previousPacket = state.dwPacketNumber;
        }
        else if (connected)
            Keyboard();
        connected = present;
        if (RefreshRequested.exchange(false))
        {
            const int device = UsingPad.load(std::memory_order_relaxed)
                ? static_cast<int>(PadInput::ResolveFamily(std::clamp(FixSettings().Int("MAIN.GamepadIcons"), 0, 3))) : 0;
            for (auto& [object, previous] : TextDevices)
            {
                if (previous == device)
                    continue;
                previous = device;
                const auto text = static_cast<uint8_t*>(object);
                text[81] = 1;
                TextUpdateHook.thiscall<char>(text);
            }
            if (CurrentMenu)
                Refresh(CurrentMenu);
            if (FooterWindow)
                reinterpret_cast<void (__thiscall*)(void*)>((*reinterpret_cast<void***>(FooterWindow))[0xE8 / 4])(FooterWindow);
        }
    }

    inline void* __fastcall Destroy(void* menu, void*, int flags)
    {
        if (CurrentMenu == menu)
            CurrentMenu = nullptr;
        return DestroyHook.thiscall<void*>(menu, flags);
    }

    inline void Init()
    {
        auto construct = hook::pattern("8B 54 24 04 85 D2 56 8B F1 74 10 57 83 C9 FF 33 C0 8B FA F2 AE F7 D1 49 5F 75 29");
        Construct = reinterpret_cast<decltype(Construct)>(construct.get_first());
        auto freeString = hook::pattern("56 8B F1 8B 06 66 FF 08 8B 06 66 83 38 00 75 23 57 0F B7 78 04 A1 ? ? ? ? 83 C7 06 85 C0 75 05 E8 ? ? ? ? 8B 06 57 50 E8 ? ? ? ? 83 C4 08 5F 5E C3 90 90 90 90 90 90 90 90 90 90 90 53");
        FreeString = reinterpret_cast<decltype(FreeString)>(freeString.get_first());
        auto localized = hook::pattern("83 EC 08 66 8B 41 08 66 85 C0 53 55 8D 59 04 56 57 89 4C 24 14 74 5D 8B 53 08");
        LocalizedHook = safetyhook::create_inline(localized.get_first(), Localized);
        auto textUpdate = hook::pattern("81 EC 70 01 00 00 56 8B F1 8B 46 6C 57 83 CF FF 85 C0 0F 85 91 00 00 00");
        TextUpdateHook = safetyhook::create_inline(textUpdate.get_first(), TextUpdate);
        auto textDestroy = hook::pattern("56 8B F1 57 C7 06 ? ? ? ? 8B 4E 3C 33 FF 3B CF 74 12");
        TextDestroyHook = safetyhook::create_inline(textDestroy.get_first(), TextDestroy);
        auto footer = hook::pattern("51 56 8B F1 8B 86 F0 01 00 00 85 C0 75 33 68 ? ? ? ? 8D 4C 24 08 E8 ? ? ? ? 8D 44 24 04 50 68 7E 02 00 00");
        FooterHook = safetyhook::create_inline(footer.get_first(), Footer);
        auto formDestroy = hook::pattern("C7 01 ? ? ? ? E9 ? ? ? ? 90 90 90 90 90 8B 01 6A 00 FF 90 ? ? ? ?");
        FormDestroyHook = safetyhook::create_inline(formDestroy.get_first(), FormDestroy);
        KnightRider2Camera::FrontendDraw = DrawNavigation;
        auto name = hook::pattern("51 56 57 8B F9 8D 4C 24 08 E8 ? ? ? ? 8A 44 24 18 8B 35 ? ? ? ?");
        NameHook = safetyhook::create_inline(name.get_first(), Name);
        auto first = hook::pattern("53 56 57 8B 7C 24 10 8B F1 8B CF E8 ? ? ? ? 8B CF 8B D8 E8 ? ? ? ?");
        FirstLabelReturn = reinterpret_cast<uintptr_t>(first.get_first(0x35));
        auto refresh = hook::pattern("8B 81 08 02 00 00 05 FC 00 00 00 50 E8 ? ? ? ? C3 90 90 90 90 90 90");
        Refresh = reinterpret_cast<decltype(Refresh)>(refresh.get_first());
        auto destroy = hook::pattern("56 8B F1 E8 ? ? ? ? F6 44 24 08 01 74 09 56 E8 ? ? ? ? 83 C4 04 8B C6 5E C2 04 00 90 90 51 53 56 8B F1 33 C0 57 8B BE 74 0B 00 00");
        DestroyHook = safetyhook::create_inline(destroy.get_first(), Destroy);
        FixSettings().OnChange("MAIN.GamepadIcons", []() { RefreshRequested = true; });
        auto quitKey = hook::pattern("56 8B F1 D9 86 94 00 00 00 D8 1D ? ? ? ? DF E0 25 00 41 00 00 74 4E 8B 44 24 0C 8B 08 8B 41 08 85 C0 74 26 83 F8 1B 75 3C");
        PlayClick = reinterpret_cast<decltype(PlayClick)>(injector::GetBranchDestination(quitKey.get_first(0x36)).as_int());
        auto quitOpen = hook::pattern("8B 4F 48 C7 87 90 00 00 00 02 00 00 00 E8");
        // The button that opened the question must be released first.
        QuitOpenHook = safetyhook::create_mid(quitOpen.get_first(3), [](SafetyHookContext&) { QuitHeld = 0xFFFF; });
        auto quitPrompt = hook::pattern("83 EC 14 56 8B F1 57 8B 46 34 85 C0 0F 84 ? ? ? ? 8D 4E 4C E8");
        QuitPromptHook = safetyhook::create_mid(quitPrompt.get_first(), [](SafetyHookContext& context)
        {
            QuitPrompt(reinterpret_cast<uint8_t*>(context.ecx));
        });
        KnightRider2Frame::InputFrame = Frame;
        KnightRider2Frame::KeyboardInput = Keyboard;
    }
}

