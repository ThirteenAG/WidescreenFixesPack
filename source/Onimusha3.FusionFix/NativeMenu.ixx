module;
#include <stdafx.h>
#include "Shared/RWGTA/FixMenu.hpp"
#include "FixSettings.hpp"
#include "Shared/RainDroplets.hpp"
#include <array>
#include <cstring>

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
    inline int(__cdecl* PrintCentred)(void*, int16_t, int16_t, char, uint8_t, char, char, const char*, ...) = nullptr;

    // The sprites of the game's Options screen: the plaque, the row bars (eight pieces a row), the bullets, the arrows
    // around a value and the frame of the selected row. A table entry: +0 shown, +4 x, +6 y (16 bits each).
    namespace Sprites
    {
        inline void* Batch = nullptr;
        inline uint8_t* Plaque = nullptr;
        inline uint8_t* WesternBars = nullptr;      // 24 byte entries, rows 23 apart
        inline uint8_t* JapaneseBars = nullptr;     // 20 byte entries, rows 30 apart
        inline uint8_t** Bullets = nullptr;         // 20 byte entries: the selected bullet, then one a row
        inline uint8_t** Arrows = nullptr;          // 24 byte entries: left and right of each value row
        inline uint8_t** Frame = nullptr;           // 24 byte entries: the four pieces of the selection frame
        inline uint16_t* ValueCentre = nullptr;
        inline uint32_t* TextPitch = nullptr;

        template<size_t Size> struct Entry
        {
            std::array<uint8_t, Size> bytes{};
            void Move(int x, int y)
            {
                std::memcpy(&bytes[4], &x, 2);
                std::memcpy(&bytes[6], &y, 2);
            }
            int16_t X() const { int16_t v; std::memcpy(&v, &bytes[4], 2); return v; }
            int16_t Y() const { int16_t v; std::memcpy(&v, &bytes[6], 2); return v; }
        };

        template<size_t Size> Entry<Size> Copy(const uint8_t* table, size_t index)
        {
            Entry<Size> entry;
            std::memcpy(entry.bytes.data(), table + index * Size, Size);
            return entry;
        }
    }
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

    inline void DrawSprites(void* batch, int texture);

    inline int __fastcall NativeRows(void* sprites, void*, int texture, char count, int table, int flags)
    {
        if (!BackgroundOnly)
            return RowsHook.unsafe_thiscall<int>(sprites, texture, count, table, flags);
        if (Opened)
            DrawSprites(sprites, texture);
        return 0;
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

    // A line in the game's Options text style: colour 0 normal, 1 unavailable
    inline void Line(int x, int y, const std::wstring& text, int colour = 0, bool centred = false, int pitch = 0, int size = 19)
    {
        const auto encoded = Text::Encode(text, NativeLanguage == 9 || NativeLanguage == 10);
        if (!pitch) pitch = Sprites::TextPitch && *Sprites::TextPitch ? int(*Sprites::TextPitch) : 18;
        (centred ? PrintCentred : Print)(Font, int16_t(x), int16_t(y), char(colour), 128, char(size), char(pitch), "%s", encoded.c_str());
    }

    // The rows the Options screen has room for, below the page title row
    inline size_t ListRows()
    {
        return Western() ? 11 : 9;
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
        Model.OpenMenu(Module(), ShownLanguage, ListRows());
        for (const auto& page : Model.Pages())
            if (page.plugin && page.plugin->module != Module())
                for (const auto& setting : page.plugin->file.description.settings)
                    if (setting.apply == "restart" && setting.HasValue())
                        OtherRestartValues.try_emplace(std::make_pair(page.plugin->module, setting.id), OtherPlugins::Get(*page.plugin, setting));
        Page = {};
        Page.Lines = ListRows();
        Opened = Page.Build(Model, true);
    }

    // The rows of the page in the game's Options layout: the plaque, the page title on the first row, then the list
    struct Layout
    {
        bool western = true;
        int pitch = 23, barTop = 57, textTop = 59;

        static Layout Now()
        {
            Layout layout;
            layout.western = Western();
            if (!layout.western)
            {
                layout.pitch = 30;
                layout.barTop = 61;
                layout.textTop = 65;
            }
            return layout;
        }

        int Text(size_t row) const { return textTop + int(row) * pitch; }
        int Bar(size_t row) const { return barTop + int(row) * pitch; }
    };

    // A value sits centred between its arrows; a long one moves the left arrow out (as the game does for the screen
    // resolution) and, past that, gets a smaller size, like a long name left of it
    struct Fit
    {
        int shift = 0, centre = 382, valueSize = 19, labelSize = 19;
    };

    // The game's font advances about two thirds of its size a letter
    inline int Width(const std::wstring& text, int size)
    {
        return int(text.size()) * size * 25 / 38;
    }

    inline Fit Place(const std::wstring& label, const std::wstring& value)
    {
        Fit fit;
        fit.centre = *Sprites::ValueCentre;
        // Between the arrows of the game's value rows
        const auto left = Sprites::Copy<24>(*Sprites::Arrows, 0), right = Sprites::Copy<24>(*Sprites::Arrows, 1);
        const int room = right.X() - left.X() - 18 - 42;
        const int width = Width(value, 19);
        if (width > room)
        {
            const int extra = std::min(width - room, 55);
            fit.shift = extra;
            fit.centre -= (extra + 1) / 2;
            if (width > room + extra && !value.empty())
                fit.valueSize = std::max(12, (room + extra) * 38 / (int(value.size()) * 25));
        }
        const int labelRoom = left.X() - fit.shift - 8 - 60;
        if (Width(label, 19) > labelRoom && !label.empty())
            fit.labelSize = std::max(12, labelRoom * 38 / (int(label.size()) * 25));
        return fit;
    }

    // A list line starts a row of the page when it is the row's first line
    inline bool RowStart(size_t slot)
    {
        return Page.first + slot == Page.items[slot].head;
    }

    inline void DrawSprites(void* batch, int texture)
    {
        using namespace Sprites;
        const auto layout = Layout::Now();
        const uint32_t pulse = *reinterpret_cast<uint32_t*>(static_cast<uint8_t*>(Owner) + 200);
        PanelHook.unsafe_thiscall<int>(batch, texture, 1, Plaque, 0x80808080, 0);
        const size_t rows = Page.items.size() + 1;
        if (layout.western)
            CursorHook.unsafe_thiscall<int>(batch, texture, uint8_t(rows * 8), WesternBars, 0x80808080, 0);
        else
            PanelHook.unsafe_thiscall<int>(batch, texture, uint8_t(rows * 8), JapaneseBars, 0x80808080, 0);
        const auto bullets = *Bullets;
        const auto arrows = *Arrows;
        for (size_t i = 0; i < Page.items.size(); ++i)
        {
            const auto& item = Page.items[i];
            if (item.kind == View::Kind::Text || !RowStart(i))
                continue;
            // Bullets: entry 0 is the selected one, then one a row (the title row has none)
            auto bullet = Copy<20>(bullets, 2 + i);
            PanelHook.unsafe_thiscall<int>(batch, texture, 1, bullet.bytes.data(), 0x80808080, 0);
            if (item.kind == View::Kind::Row && !item.value.empty())
            {
                std::array<Entry<24>, 2> pair{ Copy<24>(arrows, 0), Copy<24>(arrows, 1) };
                for (auto& arrow : pair) arrow.Move(arrow.X(), layout.Text(i + 1));
                pair[0].Move(pair[0].X() - Place(item.text, item.value).shift, layout.Text(i + 1));
                RowsHook.unsafe_thiscall<int>(batch, texture, uint8_t(pair.size()), pair[0].bytes.data(), 0);
            }
            if (item.head != Page.selected)
                continue;
            auto orb = Copy<20>(bullets, 0);
            orb.Move(bullet.X(), bullet.Y());
            PanelHook.unsafe_thiscall<int>(batch, texture, 1, orb.bytes.data(), (pulse & 0xFFF0F0F0) | 0xF0F0F0, 0);
            std::array<Entry<24>, 4> frame{};
            for (size_t part = 0; part < frame.size(); ++part)
            {
                frame[part] = Copy<24>(*Frame, part);
                frame[part].Move(frame[part].X(), layout.Bar(i + 1) - 1);
            }
            CursorHook.unsafe_thiscall<int>(batch, texture, uint8_t(frame.size()), frame[0].bytes.data(), pulse, 0);
        }
    }

    inline void Draw()
    {
        const auto layout = Layout::Now();
        const auto title = Page.page < Model.Pages().size() ? Model.Pages()[Page.page].title : Model.Text("menu.plugins");
        std::wstring heading;
        Text::Convert(title, heading);
        Line(256, layout.Text(0), heading, 0, true);
        // Rows above or below the shown ones: the font's own small arrows at the end of the title row
        if (Page.Above()) Line(446, layout.Text(0), L"\u2191");
        if (Page.Below()) Line(462, layout.Text(0), L"\u2193");
        for (size_t i = 0; i < Page.items.size(); ++i)
        {
            const auto& item = Page.items[i];
            const auto& head = Page.all[item.head];
            const int colour = item.kind == View::Kind::Text || !head.enabled ? 1 : 0;
            if (item.kind == View::Kind::Row && RowStart(i) && !item.value.empty())
            {
                const auto fit = Place(item.text, item.value);
                Line(60, layout.Text(i + 1), item.text, colour, false, 0, fit.labelSize);
                Line(fit.centre, layout.Text(i + 1), item.value, colour, true, 0, fit.valueSize);
            }
            else
                Line(60, layout.Text(i + 1), item.text, colour);
        }
        // The help line under the rows, like the game's: the selected setting's description, then a restart notice
        std::vector<std::wstring> help;
        const auto& selected = Page.selected < Page.all.size() ? Page.all[Page.selected] : View::Item{};
        if (!Page.details && Page.notice.empty() && selected.kind == View::Kind::Row && selected.row)
            Text::Lines(Model.Help(*selected.row), help, 40);
        if (RestartPending())
        {
            std::vector<std::wstring> warning;
            Text::Lines(Model.Text("menu.restartshort"), warning, 40);
            help.resize(std::min(help.size(), size_t(1)));
            help.insert(help.end(), warning.begin(), warning.end());
        }
        for (size_t i = 0; i < std::min(help.size(), size_t(2)); ++i)
            Line(40, 375 + int(i) * 22, help[i], 0, false, 21);
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
        RainDroplets::MenuOpen(true);
        if (Opened)
        {
            if (ShownLanguage != Language())
                Open();
            Page.Lines = ListRows();
            if (Opened && Page.Build(Model, true))
            {
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
            }
            else
                Close();
            const bool shown = Opened && Page.Build(Model, true);
            // The game draws its Options background; the page's sprites go in with it, where its own rows would be
            const uint32_t savedPressed = Pressed;
            const uint32_t savedRepeated = Repeated;
            Pressed = 0;
            Repeated = 0;
            BackgroundOnly = true;
            FrameHook.unsafe_thiscall<int>(owner);
            BackgroundOnly = false;
            Pressed = savedPressed;
            Repeated = savedRepeated;
            if (shown)
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
            Line(60, y, asi_settings::text::Widen(Words("menu.plugins")));
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
        {
            auto plaque = hook::pattern("8B 45 00 53 68 80 80 80 80 68 ? ? ? ? 6A 01 50 B9 ? ? ? ? E8");
            Sprites::Plaque = *plaque.get_first<uint8_t*>(10);
            Sprites::Batch = *plaque.get_first<void*>(18);
            Sprites::WesternBars = *hook::pattern("68 ? ? ? ? 6A 70 51 B9").get_first<uint8_t*>(1);
            Sprites::JapaneseBars = *hook::pattern("68 ? ? ? ? 6A 58 50 B9").get_first<uint8_t*>(1);
            Sprites::Bullets = *hook::pattern("8B 0D ? ? ? ? 8B 55 00 53 25 F0 F0 F0 FF").get_first<uint8_t**>(2);
            Sprites::Arrows = *hook::pattern("8B 15 ? ? ? ? 0F B6 05 ? ? ? ? 8B 4D 00 53 52 50 51 B9").get_first<uint8_t**>(2);
            Sprites::Frame = *hook::pattern("A1 ? ? ? ? 0F B6 0D ? ? ? ? 52 8B 55 00 50 51 52 B9").get_first<uint8_t**>(1);
            Sprites::ValueCentre = *hook::pattern("0F B7 05 ? ? ? ? 68 ? ? ? ? 52 6A 13").get_first<uint16_t*>(3);
            Sprites::TextPitch = *hook::pattern("0F B6 15 ? ? ? ? 50 0F B7 05 ? ? ? ? 68").get_first<uint32_t*>(3);
            PrintCentred = reinterpret_cast<decltype(PrintCentred)>(hook::pattern("8B 4C 24 04 8D 81 F0 F0 03 00 8D 91 3F D8 03 00 39 10 72 03 33 C0 C3 50 8B 54 24 24 8D 44 24 28 50 33 C0 8A 64 24 24 52 0F B6 54 24 20 81 CA 00 01 00 00 8A 44 24 24 50 0F B6 44 24 20 83 E0 0F 52 8B 54 24 20 83 C8 10").get_first());
        }
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
