module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/ActivePad.hpp"
#include "Shared/RWGTA/FixMenu.hpp"

export module Pad;

import Window;
import NativeMenu;
import Focus;
import Hud;

// The PC game reads only the keyboard and the mouse. The bundled Xidi (Xidi.32.dll and Xidi.ini next to PsiOps.exe)
// turns the controller into the default keys and mouse, laid out like the console version, with profiles for play,
// for held telekinesis and the sniper scope (the D-pad raises and lowers the object or zooms) and for the menus. While
// the controller was used last, prompts show the console layout's buttons (the PC fonts still have the PlayStation 2
// symbols); after keyboard or mouse input they show the keys again.
export namespace PsiOpsPad
{
    inline std::atomic_bool PadLast{ false };
    inline std::atomic<const wchar_t*> Profile{ L"Menu" };
    inline std::atomic_int IconSet{};
    inline ULONGLONG PadActive{};
    inline PadInput::ActivePad Reader;
    inline bool Telekinesis{};
    inline ULONGLONG ScopeTime{};
    // Xidi's time of the last controller input, kept by its own polling thread
    inline uint64_t (*LastActivity)(unsigned){};
    inline uint64_t SeenActivity{};

    // The PC version's first control layout table names the keys; the second and third are the console version's
    // layouts. They are loaded once like the game loads a layout.
    struct TableCalls
    {
        void*(__cdecl* allocate)(int){};
        void*(__thiscall* construct)(void*, char){};
        uintptr_t(__cdecl* resources)(){};
        const char*(__thiscall* resource)(uintptr_t, const char*){};
        void(__cdecl* fill)(void*, const char*){};
        const char* names[2]{};
    };
    inline TableCalls Calls;
    inline uintptr_t PadTables[2]{};
    inline SafetyHookInline FindString;

    // The console layout's button symbols (written as "\189" in the texts) and what stands for them in each set.
    // Xidi.ini puts L1/R1 (telekinesis, fire) on the triggers and L2/R2 (mind drain, weapon switch) on the bumpers,
    // like on the Xbox.
    struct Symbol
    {
        int code;
        const char* xbox;
        int playStation;
        const char* nintendo;
    };
    inline const Symbol Symbols[] =
    {
        { 189, "A", 189, "B" }, { 164, "B", 164, "A" }, { 163, "X", 163, "Y" }, { 162, "Y", 162, "X" },
        { 165, "LT", 167, "ZL" }, { 166, "RT", 168, "ZR" }, { 167, "LB", 165, "L" }, { 168, "RB", 166, "R" },
        { 186, "BACK", 186, "-" }, { 187, "START", 187, "+" },
    };
    inline std::map<std::string, std::string> Translated[3];

    // escaped: as the texts write a symbol; otherwise the font's character itself
    inline std::string Label(int code, bool escaped = false, int set = -1)
    {
        if (set < 0)
            set = IconSet.load(std::memory_order_relaxed);
        for (const auto& symbol : Symbols)
        {
            if (symbol.code == code)
            {
                if (set != 1)
                    return set == 2 ? symbol.nintendo : symbol.xbox;
                code = symbol.playStation;
                break;
            }
        }
        return escaped ? "\\" + std::to_string(code) : std::string(1, char(code));
    }

    inline std::string Translate(const char* text)
    {
        std::string result;
        for (const char* c = text; *c; ++c)
        {
            if (c[0] == '\\' && std::isdigit(static_cast<unsigned char>(c[1])))
            {
                int code = 0, digits = 0;
                while (digits < 3 && std::isdigit(static_cast<unsigned char>(c[1 + digits])))
                    code = code * 10 + (c[1 + digits++] - '0');
                result += Label(code, true);
                c += digits;
            }
            else
                result += *c;
        }
        return result;
    }

    inline bool Pad() { return PadLast.load(std::memory_order_relaxed); }

    inline ULONGLONG PadTime()
    {
        ULONGLONG time = PadActive;
        for (unsigned i = 0; LastActivity && i < XUSER_MAX_COUNT; ++i)
            time = std::max<ULONGLONG>(time, LastActivity(i));
        return time;
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
        {
            Telekinesis = false;
            return;
        }
        const auto& pad = state.Gamepad;
        Telekinesis = pad.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
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

    // The console layouts' texts: the default layout, and the second layout for wall moves, which the default layout
    // puts on the crouch button (the PC version has a key of its own for them, on B). The PC version jumps with the
    // action key.
    inline const char* ConsoleText(const char* key)
    {
        size_t layout = 0;
        if (!std::strcmp(key, "_control_jump"))
            key = "_control_action";
        else if (!std::strcmp(key, "_control_wall"))
            layout = 1;
        auto& table = PadTables[layout];
        if (!table)
        {
            if (const auto created = Calls.allocate(20))
            {
                Calls.construct(created, 1);
                Calls.fill(created, Calls.resource(Calls.resources(), Calls.names[layout]));
                table = reinterpret_cast<uintptr_t>(created);
            }
        }
        if (!table)
            return nullptr;
        using Find = const char*(__thiscall*)(uintptr_t, const char*);
        return reinterpret_cast<Find>((*reinterpret_cast<uintptr_t**>(table))[1])(table, key);
    }

    // The fix's text with a button: the PC version's texts that name keys and have no console counterpart
    inline std::string FixText(const char* key, const char* button)
    {
        auto text = PsiOpsMenu::Convert(RWGTAMenu::PluginText(FixSettings(), PsiOpsMenu::Language(), key), CP_UTF8, 1252);
        if (const auto at = text.find("%s"); at != std::string::npos)
            text.replace(at, 2, button);
        return text;
    }

    // Text by key from the loaded string tables. While the controller was used last the controls in prompts
    // ("_control_shoot") come from the console layout, with the buttons of the chosen set.
    inline const char* __cdecl String(const char* key)
    {
        if (Pad() && key)
        {
            const bool control = !std::strncmp(key, "_control_", 9);
            const bool own = !std::strcmp(key, "Press_action__space__to_continue_") || !std::strcmp(key, "Change_Targets");
            if (control || own)
            {
                auto& cache = Translated[IconSet.load(std::memory_order_relaxed)];
                if (const auto found = cache.find(key); found != cache.end())
                    return found->second.c_str();
                std::string text;
                if (control)
                {
                    const auto console = ConsoleText(key);
                    if (!console)
                        return FindString.ccall<const char*>(key);
                    text = Translate(console);
                }
                else if (!std::strcmp(key, "Change_Targets"))
                    text = FixText("pad.targets", "");
                else
                    text = FixText("pad.continue", Label(189).c_str());
                return cache.emplace(key, std::move(text)).first->second.c_str();
            }
        }
        return FindString.ccall<const char*>(key);
    }

    // The game's menus get their texts when they load: the inventory's key legends and the "(ESC)" of the menus'
    // Cancel and Back follow the device here
    struct LiveText
    {
        const char* key;
        const char* symbols;    // the font's symbols for the controller: the left stick and its directions
        std::string keys;
    };
    inline LiveText LiveTexts[] = { { "inventory_dpad_one", "\xB9 \xAC\xB3:" }, { "inventory_dpad_two", "\xB9 \xB2\xB5:" } };
    inline void TextDrawn(void* text)
    {
        using namespace PsiOpsMenu;
        const auto current = rawString(reinterpret_cast<GameString*>(reinterpret_cast<uintptr_t>(text) + 28));
        if (!current)
            return;
        const auto replace = [&](const std::string& value)
        {
            GameString string{};
            createString(&string, value.c_str());
            setText(text, &string, true);
            destroyString(&string);
        };
        const std::string_view shown(current);
        if (shown.ends_with(')'))
        {
            // " (ESC)", then the button of each set
            std::string suffixes[4]{ " (ESC)" };
            for (int set = 0; set < 3; ++set)
                suffixes[set + 1] = " (" + Label(164, false, set) + ")";
            const auto& wanted = suffixes[Pad() ? IconSet.load(std::memory_order_relaxed) + 1 : 0];
            for (const auto& suffix : suffixes)
            {
                if (shown.ends_with(suffix))
                {
                    if (suffix != wanted)
                        replace(std::string(shown.substr(0, shown.size() - suffix.size())) + wanted);
                    break;
                }
            }
            return;
        }
        for (auto& live : LiveTexts)
        {
            if (live.keys.empty())
                live.keys = FindString.ccall<const char*>(live.key);
            if (shown != live.keys && shown != live.symbols)
                continue;
            const std::string_view wanted = Pad() ? std::string_view(live.symbols) : std::string_view(live.keys);
            if (wanted != shown)
                replace(std::string(wanted));
            return;
        }
    }

    inline void Frame()
    {
        Poll();
        // In play the game isn't paused and shows no cursor; the sniper scope takes the D-pad for its zoom
        const auto now = GetTickCount64();
        const bool play = PsiOpsFocus::pauseLevel.load(std::memory_order_relaxed) == 0 && !PsiOpsWindow::cursorVisible;
        if (PsiOpsHud::scopeDrawn.load(std::memory_order_relaxed))
            ScopeTime = now;
        const bool scope = now - ScopeTime < 250;
        Profile.store(!play ? L"Menu" : Telekinesis ? L"Telekinesis" : scope ? L"Scope" : L"Main", std::memory_order_relaxed);
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
        const auto refresh = []()
        {
            const int set = std::clamp(FixSettings().Int("MAIN.GamepadIcons"), 0, 3);
            IconSet.store(set == 0 ? 0 : set - 1, std::memory_order_relaxed);
        };
        refresh();
        FixSettings().OnChange("MAIN.GamepadIcons", refresh);

        // The game's layout switch: it allocates and registers a table, finds the layout's resource and fills it
        auto layout = hook::pattern("83 EC 44 A1 ? ? ? ? 89 45 EC 89 4D BC 8B 45 BC 0F B6 88 B8 03 00 00");
        const auto branch = [&](ptrdiff_t offset) { return injector::GetBranchDestination(layout.get_first(offset)).as_int(); };
        Calls.allocate = reinterpret_cast<decltype(Calls.allocate)>(branch(0x78));
        Calls.construct = reinterpret_cast<decltype(Calls.construct)>(branch(0x95));
        Calls.resources = reinterpret_cast<decltype(Calls.resources)>(branch(0x13D));
        Calls.resource = reinterpret_cast<decltype(Calls.resource)>(branch(0x144));
        Calls.fill = reinterpret_cast<decltype(Calls.fill)>(branch(0x15A));
        Calls.names[0] = *layout.get_first<const char*>(0xF7 + 1);
        Calls.names[1] = *layout.get_first<const char*>(0x106 + 1);
        auto find = hook::pattern("55 8B EC 83 EC 10 C7 45 FC 00 00 00 00 8D 45 F8 50 B9 ? ? ? ? E8 ? ? ? ? EB 08");
        FindString = safetyhook::create_inline(find.get_first(), String);

        const auto message = PsiOpsWindow::message;
        PsiOpsWindow::message = [message](HWND window, UINT id, WPARAM wParam, LPARAM lParam, LRESULT& result)
        {
            Input(id);
            return message && message(window, id, wParam, lParam, result);
        };
        const auto frame = PsiOpsWindow::frame;
        PsiOpsWindow::frame = [frame](IDirect3DDevice9* device)
        {
            Frame();
            if (frame)
                frame(device);
        };
        PsiOpsMenu::textDrawn = TextDrawn;
        LoadXidi();
    }
}
