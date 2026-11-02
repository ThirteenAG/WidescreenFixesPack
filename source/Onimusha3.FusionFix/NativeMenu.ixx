module;
#include <stdafx.h>
#include "Shared/RWGTA/FixMenu.hpp"
#include "FixSettings.hpp"

export module NativeMenu;
import ComVars;
import NativeMenuView;
import NativeMenuText;
import Resolution;


export namespace OnimushaMenu
{
    inline RWGTAMenu::Model Model;
    inline View Page;
    inline SafetyHookInline FrameHook;
    inline SafetyHookInline NativeTextHook;
    inline SafetyHookInline PanelHook, CursorHook, RowsHook;
    inline thread_local bool BackgroundOnly = false;
    inline void* Font = nullptr;
    inline void* Owner = nullptr;
    inline bool Opened = false;
    inline bool MainShown = false;
    inline std::string ShownLanguage;
    inline std::map<std::string, std::string> RestartValues;
    inline std::map<std::pair<HMODULE, std::string>, std::string> OtherRestartValues;
    inline int(__cdecl* Print)(void*, int16_t, int16_t, char, uint8_t, char, char, const char*, ...) = nullptr;
    inline uint32_t(__fastcall* Shade)(int, int, int, char, int) = nullptr;
    inline void (*Tick)() = nullptr;

    inline char __fastcall NativeOptionsText(void* owner, void*)
    {
        return BackgroundOnly ? 0 : NativeTextHook.unsafe_thiscall<char>(owner);
    }

    inline int __fastcall NativePanel(void* sprites, void*, int texture, char count, int table, int colour, int flags)
    {
        return BackgroundOnly ? 0 : PanelHook.unsafe_thiscall<int>(sprites, texture, count, table, colour, flags);
    }

    inline int __fastcall NativeCursor(void* sprites, void*, int texture, char count, int table, int colour, int flags)
    {
        return BackgroundOnly ? 0 : CursorHook.unsafe_thiscall<int>(sprites, texture, count, table, colour, flags);
    }

    inline int __fastcall NativeRows(void* sprites, void*, int texture, char count, int table, int flags)
    {
        return BackgroundOnly ? 0 : RowsHook.unsafe_thiscall<int>(sprites, texture, count, table, flags);
    }

    inline HMODULE Module()
    {
        static const auto module = []()
        {
            HMODULE value = nullptr;
            GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                reinterpret_cast<LPCWSTR>(&Module), &value);
            return value;
        }();
        return module;
    }

    inline bool Western()
    {
        return NativeLanguage >= 2 && NativeLanguage <= 7;
    }

    inline std::string Language()
    {
        const auto chosen = FixSettings().String("MAIN.TextLanguage");
        if (chosen != "auto" && (chosen == "en" || Western()))
            return chosen;
        switch (NativeLanguage)
        {
            case 4: return "de";
            case 5: return "fr";
            case 6: return "it";
            case 7: return "es";
            default: return "en";
        }
    }

    inline std::string Words(const char* key)
    {
        return RWGTAMenu::PluginText(FixSettings(), Language(), key);
    }

    inline void Line(int x, int y, const std::wstring& text, int colour = 0)
    {
        const auto encoded = Text::Encode(text, NativeLanguage == 9 || NativeLanguage == 10);
        Print(Font, int16_t(x), int16_t(y), char(colour), 128, 14, 18, "%s", encoded.c_str());
    }

    inline bool RestartPending()
    {
        for (const auto& [id, value] : RestartValues)
            if (FixSettings().Wanted(id) != value)
                return true;
        for (const auto& page : Model.Pages())
        {
            if (!page.plugin || page.plugin->module == Module())
                continue;
            for (const auto& setting : page.plugin->file.description.settings)
            {
                const auto found = OtherRestartValues.find({ page.plugin->module, setting.id });
                if (found != OtherRestartValues.end() && OtherPlugins::Get(*page.plugin, setting) != found->second)
                    return true;
            }
        }
        return false;
    }

    inline void Close()
    {
        Opened = false;
        Model.Close();
        Page = {};
    }

    inline void Open()
    {
        ShownLanguage = Language();
        Model.OpenPlugins(Module(), ShownLanguage, 32, {}, true);
        for (const auto& page : Model.Pages())
            if (page.plugin && page.plugin->module != Module())
                for (const auto& setting : page.plugin->file.description.settings)
                    if (setting.apply == "restart" && setting.HasValue())
                        OtherRestartValues.try_emplace(std::make_pair(page.plugin->module, setting.id), OtherPlugins::Get(*page.plugin, setting));
        Page = {};
        Opened = Page.Build(Model, true);
    }

    inline void Draw()
    {
        Shade(0, 0, 0, char(96), 65520);
        const auto title = Page.page < Model.Pages().size() ? Model.Pages()[Page.page].title : Model.Text("menu.plugins");
        std::vector<std::wstring> lines;
        Text::Lines(title, lines);
        for (size_t i = 0; i < std::min(lines.size(), size_t(3)); ++i)
            Line(48, 32 + int(i) * 22, lines[i]);
        for (size_t i = 0; i < Page.items.size(); ++i)
            Line(48, 100 + int(i) * 28, Page.items[i].text, !Page.items[i].enabled ? 1 : i == Page.selected ? 2 : 0);
        if (RestartPending())
        {
            std::vector<std::wstring> warning;
            Text::Lines(Model.Text("menu.restartshort"), warning);
            for (size_t i = 0; i < std::min(warning.size(), size_t(2)); ++i)
                Line(48, 388 + int(i) * 20, warning[i], 2);
        }
    }

    inline int __fastcall Frame(uint8_t* owner, void*)
    {
        if (Owner != owner || *reinterpret_cast<uint32_t*>(owner + 188) != 4)
        {
            if (Opened)
                Close();
            Owner = owner;
        }
        if (Tick)
            Tick();
        if (Opened)
        {
            const uint32_t savedPressed = Pressed;
            const uint32_t savedRepeated = Repeated;
            Pressed = 0;
            Repeated = 0;
            BackgroundOnly = true;
            FrameHook.unsafe_thiscall<int>(owner);
            BackgroundOnly = false;
            Pressed = savedPressed;
            Repeated = savedRepeated;
            if (ShownLanguage != Language())
                Open();
            if (!Page.Build(Model, true))
            {
                Close();
                return 1;
            }
            const uint32_t pressed = Pressed;
            const uint32_t repeated = Repeated;
            if (repeated & 4) Page.Move(-1);
            else if (repeated & 8) Page.Move(1);
            if (pressed & 2)
            {
                if (!Page.Back(Model)) Close();
            }
            else if ((repeated & 0x30) || (pressed & 1))
            {
                const auto direction = repeated & 0x10 ? -1 : repeated & 0x20 ? 1 : 0;
                if (!Page.Activate(Model, true, direction)) Close();
            }
            if (Opened && Page.Build(Model, true))
                Draw();
            return 1;
        }

        const bool main = *reinterpret_cast<uint32_t*>(owner + 188) == 4 && owner[177] == 0;
        if (main && !MainShown)
            OnimushaResolution::Refresh();
        MainShown = main;
        const int selected = int8_t(owner[179]);
        const int next = Western() ? (owner[184] ? 9 : 12) : 9;
        const uint32_t repeated = Repeated;
        const bool stable = (owner[176] & 0x7F) == 1;
        if (main && stable && selected == 4 && (repeated & 0x30))
        {
            OnimushaResolution::Move(repeated & 0x10 ? -1 : 1);
            Repeated &= ~0x30u;
        }
        if (main && stable && selected == 7 && (Pressed & 1))
        {
            Open();
            if (Opened) Draw();
            return 1;
        }
        if (main && stable && selected == 7 && (repeated & 12))
        {
            owner[179] = repeated & 4 ? 6 : uint8_t(next);
            owner[176] = 5;
            Repeated &= ~12u;
        }
        const auto result = FrameHook.unsafe_thiscall<int>(owner);
        Repeated = repeated;
        if (main && stable && !(Pressed & 2) && ((selected == 6 && (repeated & 8)) || (selected == next && (repeated & 4))))
        {
            owner[179] = 7;
            owner[176] = 5;
        }
        if (*reinterpret_cast<uint32_t*>(owner + 188) == 4 && owner[177] == 0)
        {
            const auto y = Western() ? 220 : 273;
            Line(60, y, asi_settings::text::Widen(Words("menu.plugins")), owner[179] == 7 ? 2 : 0);
        }
        return result;
    }

    inline void Init()
    {
        auto frame = hook::pattern("53 56 8B F1 8B 86 BC 00 00 00 83 F8 07 BB 01 00 00 00 0F 87 ? ? ? ? FF 24 85 ? ? ? ? A0 ? ? ? ?");
        NativeLanguage.SetAddress(*frame.get_first<uint8_t*>(32));
        OnimushaResolution::Init();
        auto input = hook::pattern("83 EC 0C A1 ? ? ? ? 53 8B 1D ? ? ? ? 55 56 8B F1 8B 0D ? ? ? ? 57 89 4C 24 10 33 FF");
        Pressed.SetAddress(*input.get_first<uint32_t*>(4));
        Repeated.SetAddress(*input.get_first<uint32_t*>(21));
        // Both native language branches call this same font owner and formatter.
        auto font = hook::pattern("6A 3B 6A 3C 68 ? ? ? ? E8 ? ? ? ?");
        Font = *font.get_first<void*>(5);
        Print = reinterpret_cast<decltype(Print)>(injector::GetBranchDestination(font.get_first(9)).as_int());
        auto nativeText = hook::pattern("A0 ? ? ? ? 3C 01 56 8B F1 74 0C 3C 08 74 08 3C 09 74 04 3C 0A 75 0A E8");
        NativeTextHook = safetyhook::create_inline(nativeText.get_first(), NativeOptionsText);
        // The Options plaque, panels and cursor are left out behind the Plugins page; only the tiled background stays.
        const auto target = [](const char* pattern, ptrdiff_t call)
        {
            return reinterpret_cast<void*>(injector::GetBranchDestination(hook::pattern(pattern).get_first(call)).as_int());
        };
        PanelHook = safetyhook::create_inline(target("8B 45 00 53 68 80 80 80 80 68 ? ? ? ? 6A 01 50 B9 ? ? ? ? E8", 22), NativePanel);
        CursorHook = safetyhook::create_inline(target("A1 ? ? ? ? 0F B6 0D ? ? ? ? 52 8B 55 00 50 51 52 B9 ? ? ? ? E8", 24), NativeCursor);
        RowsHook = safetyhook::create_inline(target("8B 15 ? ? ? ? 0F B6 05 ? ? ? ? 8B 4D 00 53 52 50 51 B9 ? ? ? ? E8", 25), NativeRows);
        Shade = reinterpret_cast<decltype(Shade)>(hook::pattern("56 57 8B F9 6A 20 B9 ? ? ? ? 8B F2 E8 ? ? ? ? 85 C0 74 47 8B 4C 24 14 8A 54 24 10").get_first());
        for (const auto& setting : FixSettings().GetDescription().settings)
            if (setting.apply == "restart" && setting.HasValue())
                RestartValues.emplace(setting.id, FixSettings().Wanted(setting.id));
        FixSettings().Availability("MAIN.TextLanguage", [](std::string_view value)
        {
            return value == "auto" || value == "en" || Western();
        });
        FrameHook = safetyhook::create_inline(frame.get_first(), Frame);
    }
}
