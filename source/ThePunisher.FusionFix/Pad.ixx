module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/D3D8Compatibility.hpp"
#include "Shared/ActivePad.hpp"
#include "Shared/RWGTA/FixMenu.hpp"
#include "MouseInput.hpp"

export module Pad;

import NativeMenu;
import Window;
import Cutscene;

// The PC game reads only the keyboard and the mouse. The bundled Xidi (Xidi.32.dll and Xidi.ini next to pun.exe)
// turns the controller into the default keys and mouse: profiles for play, cutscenes and the game's screens. While
// the controller was used last, prompts, tutorials and the screens' footers show its buttons with the console
// versions' button glyphs, which the PC fonts still have; after keyboard or mouse input they show the keys again.
export namespace PunisherPad
{
    enum Button { A, B, X, Y, LB, RB, LT, RT, LS, RS, Back, Start, Up, Down, Left, Right, Count };

    // Xbox (Black and White stand for RB and LB, as on the Xbox 360), PlayStation 2, Nintendo
    inline const char* const Labels[3][Count] =
    {
        { "\x82", "\x85", "\x83", "\x84", "\x87", "\x86", "\x88", "\x8A", "\x89", "\x8B", "BACK", "START", "\x8D", "\x8E", "\x8F", "\x90" },
        { "\xB3", "\xB6", "\xB4", "\xB5", "\xB7", "\xBA", "\xB8", "\xBB", "\xB9", "\xBC", "SELECT", "START", "\xAF", "\xB0", "\xB2", "\xB1" },
        { "B", "A", "Y", "X", "L", "R", "ZL", "ZR", "\x89", "\x8B", "-", "+", "\x8D", "\x8E", "\x8F", "\x90" },
    };

    // What each button sends in play (Xidi.ini, profile Main): a DirectInput key code or a mouse button
    struct Binding
    {
        int key, mouse;
        Button button;
    };
    constexpr Binding Play[] =
    {
        { 0x12, -1, A }, { 0x1C, -1, A }, { 0x10, -1, B }, { 0x13, -1, X }, { 0x14, -1, Y }, { 0x2E, -1, LB },
        { 0x39, -1, RB }, { -1, 1, LT }, { -1, 0, RT }, { 0x31, -1, LS }, { 0x21, -1, RS }, { 0x3B, -1, Back },
        { 0x22, -1, Up }, { 0x0F, -1, Down }, { 0x02, -1, Left }, { 0x03, -1, Right },
        { 0x11, -1, LS }, { 0x1F, -1, LS }, { 0x1E, -1, LS }, { 0x20, -1, LS },
    };

    inline std::atomic_bool PadLast{ false };
    inline std::atomic<const wchar_t*> Profile{ L"Menu" };
    inline ULONGLONG PadActive{};
    inline std::atomic_int IconSet{};
    inline PadInput::ActivePad Reader;
    // Xidi's time of the last controller input, kept by its own polling thread
    inline uint64_t (*LastActivity)(unsigned){};
    inline uint64_t SeenActivity{};

    inline ULONGLONG PadTime()
    {
        ULONGLONG time = PadActive;
        for (unsigned i = 0; LastActivity && i < XUSER_MAX_COUNT; ++i)
            time = std::max<ULONGLONG>(time, LastActivity(i));
        return time;
    }
    inline void (*NextFrame)(IDirect3DDevice9*) = nullptr;
    inline SafetyHookInline BindingName;
    inline SafetyHookMid TokenHook;

    // The footers of the game's screens ("Enter: Select", "Esc: Back") and of the tutorial boxes ("Enter: Continue")
    struct Footer
    {
        const char* key;
        Button button;
        char text[16];
    };
    inline Footer Footers[] =
    {
        { "Enter:", A }, { "Esc:", B }, { "PgUp:", LB }, { "PgDn:", RB }, { "Bksp:", X }, { "Arrow Keys:", Up },
    };
    inline char ContinueText[16]{};
    inline const char* Continue{};
    inline const char* const* ContinueEntry{};
    inline char Message[256]{};

    inline bool Pad() { return PadLast.load(std::memory_order_relaxed); }
    inline const char* Label(Button button) { return Labels[IconSet.load(std::memory_order_relaxed)][button]; }

    // The game's strings (the localized table, the English text when it has fewer entries)
    inline const char* const* Strings{};
    inline const int* StringCount{};
    inline const char* GameString(int index, const char* english)
    {
        return *StringCount > index ? Strings[index] : english;
    }

    inline std::string Language()
    {
        auto language = FixSettings().Wanted("MAIN.TextLanguage");
        if (language != "auto")
            return language;
        switch (PRIMARYLANGID(GetUserDefaultUILanguage()))
        {
            case LANG_FRENCH: return "fr";
            case LANG_GERMAN: return "de";
            case LANG_ITALIAN: return "it";
            case LANG_SPANISH: return "es";
            default: return "en";
        }
    }

    inline void UpdateTexts()
    {
        const bool pad = Pad();
        for (auto& footer : Footers)
        {
            if (pad)
                std::snprintf(footer.text, sizeof(footer.text), "%s:", footer.button == Up ? Label(LS) : Label(footer.button));
            else
                std::snprintf(footer.text, sizeof(footer.text), "%s", footer.key);
        }
        std::snprintf(ContinueText, sizeof(ContinueText), "%s:", Label(A));
        Continue = pad ? ContinueText : *ContinueEntry;
    }

    inline void Poll()
    {
        if (const auto time = PadTime(); time > SeenActivity)
        {
            SeenActivity = time;
            PadLast.store(true, std::memory_order_relaxed);
        }
        XINPUT_STATE state{};
        if (!Reader.Read(state))
            return;
        const auto& pad = state.Gamepad;
        const auto moved = [](SHORT value) { return std::abs(int(value)) > 12000; };
        if (pad.wButtons || pad.bLeftTrigger > 40 || pad.bRightTrigger > 40 || moved(pad.sThumbLX) || moved(pad.sThumbLY) ||
            moved(pad.sThumbRX) || moved(pad.sThumbRY))
        {
            PadActive = GetTickCount64();
            PadLast.store(true, std::memory_order_relaxed);
        }
    }

    // Keys and mouse buttons of the game window; Xidi's own keys and mouse buttons come at the time of the controller
    // input they stand for, so only input that came without controller input counts
    inline void Input(UINT message)
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
                // Messages wait in the queue while videos and loading screens play: compare the time of the input
                if (Pad() && int32_t(GetMessageTime() - DWORD(PadTime())) > 300)
                    PadLast.store(false, std::memory_order_relaxed);
                break;
        }
    }

    inline void Presented(IDirect3DDevice9* device)
    {
        Poll();
        if (PunisherMouse::deviceInput.exchange(false, std::memory_order_relaxed))
            Input(WM_MOUSEWHEEL);
        UpdateTexts();
        const bool cutscene = Cutscene::Active && Cutscene::Active() && Cutscene::Bars();
        Profile.store(PunMenu::ScreenShown() ? L"Menu" : cutscene ? L"Cutscene" : L"Main", std::memory_order_relaxed);
        if (NextFrame)
            NextFrame(device);
    }

    // The name of an action's binding in prompts ("%s kick door", "Press %s"): the key first, then the mouse button
    inline const char* __cdecl ActionName(uintptr_t bindings, int action)
    {
        if (Pad())
        {
            const auto entry = reinterpret_cast<const int*>(bindings + 40 + 16 * action);
            for (const int key : { entry[0], entry[1] })
            {
                for (const auto& binding : Play)
                {
                    if (key >= 0 && binding.key == key)
                        return Label(binding.button);
                }
            }
            for (const auto& binding : Play)
            {
                if (entry[2] >= 0 && binding.mouse == entry[2])
                    return Label(binding.button);
            }
        }
        return BindingName.ccall<const char*>(bindings, action);
    }

    // Tokens of messages ("$STICK_CONFIG_MSG$"); null leaves them to the game
    inline const char* __cdecl Token(const char* token)
    {
        if (!Pad())
            return nullptr;
        const bool playStation = IconSet.load(std::memory_order_relaxed) == 1;
        const auto stick = playStation ? GameString(15, "Analog Stick") : GameString(16, "Thumbstick");
        if (!std::strcmp(token, "STICK_CONFIG_MSG"))
        {
            std::snprintf(Message, sizeof(Message), GameString(19, "Use the left %s to move around. Use the right %s to look around."), stick, stick);
            return Message;
        }
        if (!std::strcmp(token, "STICKS") || !std::strcmp(token, "LEFT_STICK"))
            return stick;
        if (!std::strcmp(token, "INTERROGATION_INSTRUCTIONS"))
        {
            const auto text = PunMenu::NativeText(RWGTAMenu::PluginText(FixSettings(), Language(), "pad.interrogation"));
            std::snprintf(Message, sizeof(Message), "%s", text.c_str());
            return Message;
        }
        if (!std::strcmp(token, "DPAD_UP"))
            return Label(Up);
        if (!std::strcmp(token, "DPAD_DOWN"))
            return Label(Down);
        if (!std::strcmp(token, "DPAD_LEFT"))
            return Label(Left);
        if (!std::strcmp(token, "DPAD_RIGHT"))
            return Label(Right);
        return nullptr;
    }

    inline void LoadXidi()
    {
        wchar_t path[MAX_PATH]{};
        GetModuleFileNameW(nullptr, path, DWORD(std::size(path)));
        const auto library = std::filesystem::path(path).parent_path() / L"Xidi.32.dll";
        const auto xidi = LoadLibraryW(library.c_str());
        if (!xidi)
            return;
        using RegisterProfileCallback = bool (*)(const wchar_t* (*)());
        using SendVibration = bool (*)(short, unsigned short, unsigned short);
        if (const auto registerCallback = reinterpret_cast<RegisterProfileCallback>(GetProcAddress(xidi, "XidiRegisterProfileCallback")))
            registerCallback([]() { return Profile.load(std::memory_order_relaxed); });
        // The game opens no controller device itself: this starts Xidi's controller polling
        if (const auto vibration = reinterpret_cast<SendVibration>(GetProcAddress(xidi, "XidiSendVibration")))
            vibration(-1, 0, 0);
        LastActivity = reinterpret_cast<decltype(LastActivity)>(GetProcAddress(xidi, "XidiGetLastControllerActivity"));
    }

    inline void Init()
    {
        ObservePunisherSetting("MAIN.GamepadIcons", []()
        {
            const int set = std::clamp(PunisherNumericSetting("MAIN.GamepadIcons", 0), 0, 3);
            IconSet.store(set == 0 ? 0 : set - 1, std::memory_order_relaxed);
        });

        auto name = hook::pattern("81 EC 84 00 00 00 A1 ? ? ? ? 8B 8C 24 88 00 00 00 56 8B B4 24 90 00 00 00 89 84 24 84 00 00 00 8B C6 C1 E0 04");
        BindingName = safetyhook::create_inline(name.get_first(), ActionName);

        // Message tokens: the token function takes the token in ebx (kept by the caller) and returns its text in eax
        auto token = hook::pattern("8D 5C 24 ? E8 ? ? ? ? 8B C8 8D 79 01 8A 11 41 84 D2 75 F9");
        TokenHook = safetyhook::create_mid(token.get_first(9), [](SafetyHookContext& regs)
        {
            if (const auto text = Token(reinterpret_cast<const char*>(regs.ebx)))
                regs.eax = reinterpret_cast<uintptr_t>(text);
        });

        // The screens' footer: Enter, Esc, PgUp, PgDn, Bksp and the arrow keys (the 6th to 11th case of its switch)
        auto footer = hook::pattern("81 F9 D2 00 00 00 77 ? 0F B6 89 ? ? ? ? FF 24 8D ? ? ? ? C7 44 84 28");
        for (size_t i = 0; i < std::size(Footers); ++i)
            injector::WriteMemory<const char*>(footer.get_first(22 + 10 * (5 + i) + 4), Footers[i].text, true);

        // The tutorial box's "Enter:", read twice from the string table
        auto tutorial = hook::pattern("BD BF 00 00 00 8D 74 0E EC 3B C5 A1");
        auto entry = tutorial.get_first<uint8_t>(11);
        ContinueEntry = *reinterpret_cast<const char* const**>(entry + 1);
        Strings = ContinueEntry - 191;
        StringCount = *reinterpret_cast<const int**>(entry + 0x3C);
        Continue = *ContinueEntry;
        injector::WriteMemory<const char**>(entry + 1, &Continue, true);
        injector::WriteMemory<const char**>(entry + 0x45 + 1, &Continue, true);

        PunisherWindow::Input = Input;
        NextFrame = std::exchange(D3D8Compatibility::Frame, Presented);
        LoadXidi();
    }
}
