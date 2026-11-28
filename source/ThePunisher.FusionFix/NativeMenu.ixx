module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/RWGTA/FixMenu.hpp"
#include "Shared/D3D8Compatibility.hpp"
#include "Shared/RainDroplets.hpp"
#include <array>
#include <deque>

export module NativeMenu;

import ComVars;
import NativeMenuContracts;
import MenuLayout;

export namespace PunMenu
{
    inline uintptr_t Options{}, OptionsList{}, OptionsBackground{};
    inline uintptr_t GameSettings{}, GameLabels{}, GameValues{};

    inline const std::set<std::string> joined{ "MAIN.HudAspectRatioConstraint", "MAIN.FOVFactor" };
    // The lists hold up to 20 rows; every page of the menu is shown whole (rows, then help lines when they fit)
    constexpr size_t Capacity{ 18 };
    constexpr size_t Rows{ Capacity };
    constexpr int ValueRoom{ 90 };
    // Widgets the game hasn't created yet are null
    template <class T> T Read(const void* base, size_t offset = 0)
    {
        return base ? *reinterpret_cast<const T*>(reinterpret_cast<uintptr_t>(base) + offset) : T{};
    }
    template <class T> T Read(uintptr_t base, size_t offset = 0) { return Read<T>(reinterpret_cast<void*>(base), offset); }
    template <class T> void Write(uintptr_t base, size_t offset, T value)
    {
        injector::WriteMemory<T>(reinterpret_cast<T*>(base + offset), value, true);
    }
    inline std::string NativeText(const std::string& text)
    {
        auto count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), nullptr, 0);
        if (count <= 0 || count > 8192) return {};
        std::wstring wide(count, L'\0');
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), wide.data(), count);
        CharUpperBuffW(wide.data(), count);
        count = WideCharToMultiByte(1252, WC_NO_BEST_FIT_CHARS, wide.data(), static_cast<int>(wide.size()), nullptr, 0, "?", nullptr);
        std::string result(count, '\0');
        WideCharToMultiByte(1252, WC_NO_BEST_FIT_CHARS, wide.data(), static_cast<int>(wide.size()), result.data(), count, "?", nullptr);
        return result;
    }
    inline std::pair<int, int> Measure(const std::string& text, uintptr_t list)
    {
        int width{}, height{};
        auto font = Read<int>(Read<void*>(list, 76));
        reinterpret_cast<void(__cdecl*)(int*, int*, const char*, int, int)>(Native().measure)(&width, &height, text.c_str(), -1, font);
        return { width, height };
    }
    inline std::vector<std::string> Wrap(const std::string& text, uintptr_t list, int width)
    {
        std::vector<std::string> lines;
        auto native = NativeText(text);
        while (!native.empty())
        {
            size_t length = native.find('\n');
            if (length == std::string::npos) length = native.size();
            while (length && Measure(native.substr(0, length), list).first > width) --length;
            if (!length) length = 1;
            if (length < native.size() && native[length] != '\n')
            {
                auto space = native.rfind(' ', length);
                if (space != std::string::npos && space > 0) length = space;
            }
            lines.push_back(native.substr(0, length));
            native.erase(0, length);
            while (!native.empty() && (native.front() == '\n' || native.front() == ' ')) native.erase(0, 1);
        }
        if (lines.empty()) lines.emplace_back();
        return lines;
    }
    struct Binding
    {
        uintptr_t widget{}, provider{};
        int count{};
        const char*** source{};
        uint8_t* availability{};
        std::array<uint8_t, 20> disabled{};
        std::array<int, 4> rectangle{};
        std::vector<std::string> text;
        std::vector<const char*> values;
        std::vector<const char**> slots;
        std::vector<uint8_t> enabled;
        bool bound{};
        bool Capture(uintptr_t list)
        {
            if (bound) return widget == list;
            auto data = Read<uintptr_t>(list, 100);
            if (Read<uintptr_t>(list) != Native().listTable || Read<uintptr_t>(data) != Native().dataTable) return false;
            int rows = Read<int>(data, 360);
            auto labels = Read<const char***>(data, 376);
            if (rows < 0 || rows > 18 || !labels) return false;
            widget = list;
            provider = data;
            count = rows;
            source = labels;
            availability = Read<uint8_t*>(data, 388);
            for (size_t i = 0; i < disabled.size(); ++i) disabled[i] = Read<uint8_t>(data, 4 + i);
            for (size_t i = 0; i < rectangle.size(); ++i) rectangle[i] = Read<int>(list, 44 + i * 4);
            bound = true;
            return true;
        }
        const char** Original(size_t index) const { return index < static_cast<size_t>(count) ? Read<const char**>(source, index * 4) : nullptr; }
        void Publish(std::vector<std::string> labels, std::vector<bool> selectable, size_t originals = 0)
        {
            text = std::move(labels);
            values.clear();
            slots.clear();
            enabled.clear();
            values.reserve(text.size());
            slots.reserve(text.size());
            enabled.reserve(text.size());
            for (const auto& label : text) values.push_back(label.c_str());
            for (size_t i = 0; i < text.size(); ++i)
            {
                auto original = i < originals ? Original(i) : nullptr;
                slots.push_back(original ? original : &values[i]);
                bool available = selectable[i];
                if (i < originals && i < static_cast<size_t>(count))
                    available &= !disabled[i] && (!availability || Read<uint8_t>(availability, i) != 0);
                enabled.push_back(available ? 1 : 0);
            }
            for (size_t i = 0; i < text.size(); ++i) Write<uint8_t>(provider, 4 + i, enabled[i] ? 0 : 1);
            Write<int>(provider, 360, static_cast<int>(text.size()));
            Write<const char***>(provider, 376, slots.data());
            Write<uint8_t*>(provider, 388, enabled.data());
            Write<int>(widget, 56, std::max(rectangle[3], static_cast<int>(text.size()) * Read<int>(widget, 92)));
        }
        void Restore()
        {
            if (!bound) return;
            Write<int>(provider, 360, count);
            Write<const char***>(provider, 376, source);
            Write<uint8_t*>(provider, 388, availability);
            for (size_t i = 0; i < disabled.size(); ++i) Write<uint8_t>(provider, 4 + i, disabled[i]);
            for (size_t i = 0; i < rectangle.size(); ++i) Write<int>(widget, 44 + i * 4, rectangle[i]);
            bound = false;
        }
    };
    // A row of a shown list: a model row, or a line of text (help, restart notice, update notice)
    struct Item
    {
        const RWGTAMenu::Row* row{};
        std::string text, value;
        bool selectable{};
    };
    // Where the Plugins menu is: the plugins list on the Options screen, a plugin's page or a category on the Game
    // Settings screen (labels and values with the game's arrows), as the game shows its own settings
    enum class Screen { None, List, Settings };
    struct State
    {
        HMODULE module{};
        RWGTAMenu::Model model;
        SafetyHookInline optionsInit, optionsInput, gameInit, gameInput, defaults;
        SafetyHookInline draw;
        Binding options, labels, values;
        std::vector<Item> items;
        uintptr_t activeRoot{};
        Screen screen{};
        size_t page{ RWGTAMenu::Model::RootPage };
        bool entering{};        // the Game Settings screen the game is opening shows a page of the menu
        bool returning{};       // the Options screen the game is going back to shows the plugins list
        std::string title, notice;
        const char* titlePointer{};
        const char** originalTitle{};
        uintptr_t titleRoot{};
        std::array<int, 2> originalTitleSize{};
        std::array<int, 2> originalBackgroundSize{};
        ULONGLONG lastDiscrete{};
        bool enabled{}, busy{}, open{}, pause{}, capturedTitle{}, modelOpen{};
    };
    inline State& Get() { static auto* state = new State; return *state; }
    inline void (*Tick)(){};
    inline bool Current(uintptr_t root)
    {
        return Get().activeRoot == root && Read<int>(root, 164) == 0;
    }
    inline int Selected(uintptr_t list) { return Read<int>(list, 96); }
    inline void Select(uintptr_t list, int index)
    {
        reinterpret_cast<void(__thiscall*)(void*, int)>(Native().select)(reinterpret_cast<void*>(list), index);
    }
    inline void Model(bool force = false)
    {
        auto& state = Get();
        if (state.modelOpen && !force) return;
        auto language = FixSettings().Wanted("MAIN.TextLanguage");
        if (language == "auto")
        {
            switch (PRIMARYLANGID(GetUserDefaultUILanguage()))
            {
                case LANG_FRENCH: language = "fr"; break;
                case LANG_GERMAN: language = "de"; break;
                case LANG_ITALIAN: language = "it"; break;
                case LANG_SPANISH: language = "es"; break;
                default: language = "en"; break;
            }
        }
        state.model.OpenMenu(state.module, language, Rows, joined);
        state.modelOpen = true;
    }
    inline void RestoreTitle()
    {
        auto& state = Get();
        if (!state.capturedTitle) return;
        Write<const char**>(state.titleRoot, 132, state.originalTitle);
        Write<int>(state.titleRoot, 152, state.originalTitleSize[0]);
        Write<int>(state.titleRoot, 156, state.originalTitleSize[1]);
        if (state.titleRoot == Options)
        {
            Write<int>(OptionsBackground, 52, state.originalBackgroundSize[0]);
            Write<int>(OptionsBackground, 56, state.originalBackgroundSize[1]);
        }
        state.capturedTitle = false;
    }
    // The screen's title in its header, in the game's capitals
    inline void Title(uintptr_t root, const std::string& title)
    {
        auto& state = Get();
        if (state.capturedTitle && state.titleRoot != root)
            RestoreTitle();
        if (!state.capturedTitle)
        {
            state.titleRoot = root;
            state.originalTitle = Read<const char**>(root, 132);
            state.originalTitleSize = { Read<int>(root, 152), Read<int>(root, 156) };
            state.originalBackgroundSize = { Read<int>(OptionsBackground, 52), Read<int>(OptionsBackground, 56) };
            state.capturedTitle = true;
        }
        state.title = NativeText(title);
        state.titlePointer = state.title.c_str();
        Write<const char**>(root, 132, &state.titlePointer);
        int width{}, height{};
        reinterpret_cast<void(__cdecl*)(int*, int*, const char*, int, int)>(Native().measure)(&width, &height, state.titlePointer, -1, MenuFont.get());
        Write<int>(root, 152, width);
        Write<int>(root, 156, height);
    }
    inline bool Entry()
    {
        auto& state = Get();
        if (!state.options.Capture(OptionsList) || state.options.count != 3) return false;
        Model();
        std::vector<std::string> text(3);
        text.push_back(NativeText(state.model.Text("menu.plugins")));
        std::vector<bool> enabled(4, true);
        state.options.Publish(std::move(text), std::move(enabled), 3);
        return true;
    }
    inline void Joined()
    {
        auto& state = Get();
        if (!state.labels.Capture(GameLabels) || !state.values.Capture(GameValues) || state.labels.count != 6) return;
        Model();
        const auto& rows = state.model.JoinedRows();
        std::vector<std::string> labels(6), values(6);
        for (const auto& row : rows)
        {
            labels.push_back(NativeText(state.model.Label(row)));
            values.push_back(NativeText(state.model.Value(row)));
        }
        state.labels.Publish(std::move(labels), std::vector<bool>(6 + rows.size(), true), 6);
        state.values.Publish(std::move(values), std::vector<bool>(6 + rows.size(), true), 6);
    }
    // Leaves the Plugins menu on the Options screen, on its Plugins entry
    inline void Close()
    {
        auto& state = Get();
        state.open = false;
        state.screen = Screen::None;
        state.notice.clear();
        if (Tick) Tick();
        state.items.clear();
        RestoreTitle();
        Entry();
        Select(OptionsList, 3);
    }
    // Lines of text under the rows: the selected row's help, the restart notice
    inline void Notes(std::vector<Item>& items, const RWGTAMenu::Row* selected, int width)
    {
        auto& state = Get();
        std::vector<std::string> lines;
        if (selected && !RWGTAMenu::Model::Opens(*selected))
            for (auto& line : Wrap(state.model.Help(*selected), OptionsList, width))
                if (!line.empty()) lines.push_back(line);
        if (state.model.RestartRequired())
            for (auto& line : Wrap(state.model.Text("menu.restartwarning"), OptionsList, width))
                lines.push_back(line);
        if (lines.empty())
            return;
        items.push_back({ nullptr, {}, {}, false });
        for (auto& line : lines)
            items.push_back({ nullptr, line, {}, false });
    }
    inline const RWGTAMenu::Row* SelectedRow(uintptr_t list)
    {
        auto& state = Get();
        const int index = Selected(list);
        return index >= 0 && static_cast<size_t>(index) < state.items.size() ? state.items[index].row : nullptr;
    }
    // The shown list again; selection: the row to keep selected (or the window's)
    inline bool Build(const RWGTAMenu::Row* selection = nullptr)
    {
        auto& state = Get();
        const bool settings = state.screen == Screen::Settings;
        const auto root = settings ? GameSettings : Options;
        const auto list = settings ? GameLabels : OptionsList;
        if (!Current(root) || !(settings ? state.labels.bound && state.values.bound : state.options.bound)) return false;
        state.busy = true;
        const auto& pages = state.model.Pages();
        if (state.page >= pages.size())
            state.page = RWGTAMenu::Model::RootPage;
        auto& window = state.model.Window(state.page);
        if (selection)
            window.SelectKey(reinterpret_cast<MenuList::Key>(selection));
        std::vector<Item> all;
        if (!state.notice.empty() && !settings)
        {
            for (const auto& line : Wrap(state.notice, OptionsList, 510))
                all.push_back({ nullptr, line, {}, false });
        }
        else
        {
            for (auto row : state.model.Shown(state.page, false))
                all.push_back({ row, NativeText(state.model.Label(*row)), settings ? NativeText(state.model.Value(*row)) : std::string{}, true });
            const auto shown = all.size();
            // On Game Settings the lines stay left of the values; there they span the labels and the values
            const int width = settings ? state.values.rectangle[0] - state.labels.rectangle[0] - 8 : 510;
            Notes(all, reinterpret_cast<const RWGTAMenu::Row*>(window.SelectedKey()), width);
            if (all.size() > Capacity)
                all.resize(std::max(shown, Capacity));
        }
        Title(root, pages[state.page].title);
        state.items = std::move(all);
        std::vector<std::string> labels, values;
        std::vector<bool> enabled;
        for (const auto& item : state.items)
        {
            labels.push_back(item.text);
            enabled.push_back(item.selectable);
            // The values (with the game's arrows) only for the rows; the lines under them have none
            if (item.row)
                values.push_back(item.value);
        }
        if (settings)
        {
            std::vector<bool> valued(values.size(), true);
            state.labels.Publish(std::move(labels), std::move(enabled));
            state.values.Publish(std::move(values), std::move(valued));
            // Room for the longer values of the plugins between the arrows, further right of the labels
            Write<int>(GameValues, 52, state.values.rectangle[2] + ValueRoom);
        }
        else
        {
            state.options.Publish(std::move(labels), std::move(enabled));
            Write<int>(OptionsList, 52, 520);
            Write<int>(OptionsBackground, 52, 580);
            Write<int>(OptionsBackground, 56, 30 + static_cast<int>(state.items.size()) * Read<int>(OptionsList, 92));
        }
        const bool selectable = window.Count() && window.Slot() < state.items.size() && state.items[window.Slot()].selectable;
        Select(list, selectable ? static_cast<int>(window.Slot()) : 0);
        state.busy = false;
        return true;
    }
    // The Options screen's entries: the accept sound and the opening of another screen (13: Game Settings)
    struct ScreenCalls
    {
        void(__cdecl* sound)(int, int, int, float){};
        const int* accept{};
        int(__thiscall* open)(uintptr_t, int, const char*, char){};
        char(__cdecl* paused)(){};
    };
    constexpr int GameSettingsScreen{ 13 };
    inline const ScreenCalls& Screens()
    {
        static const auto calls = []()
        {
            ScreenCalls c;
            auto entries = reinterpret_cast<uintptr_t>(hook::pattern("8B 15 ? ? ? ? 68 00 00 80 3F 6A FF 6A 00 52 E8 ? ? ? ? 83 C4 10 B9 ? ? ? ? E8 ? ? ? ? 83 E8 00 74 3C 48 74 20").get_first());
            c.accept = *reinterpret_cast<const int**>(entries + 2);
            c.sound = reinterpret_cast<decltype(c.sound)>(injector::GetBranchDestination(entries + 16).as_int());
            c.paused = reinterpret_cast<decltype(c.paused)>(injector::GetBranchDestination(entries + 0x4A).as_int());
            c.open = reinterpret_cast<decltype(c.open)>(injector::GetBranchDestination(entries + 0x59).as_int());
            return c;
        }();
        return calls;
    }
    inline bool Press(int input, int key) { return reinterpret_cast<int(__cdecl*)(int, int)>(Native().press)(input, key) != 0; }
    inline bool Repeat(int input, int key) { return reinterpret_cast<int(__cdecl*)(int, int)>(Native().repeat)(input, key) != 0; }
    struct Input { bool back{}, choose{}, left{}, right{}; };
    inline Input Events(int input, uintptr_t root, uintptr_t list)
    {
        int back = -1, row = -1, direction = -1;
        auto* hit = reinterpret_cast<void*(__thiscall*)(void*, int*, int*, int*, int)>(Native().hit)(reinterpret_cast<void*>(root), &back, &row, &direction, 0);
        if (reinterpret_cast<uintptr_t>(hit) == list && row >= 0) Select(list, row);
        return { Press(input, 1) || back == 0, Press(input, 28) || back == 1 || row >= 0,
                 Repeat(input, 203) || direction == 0, Repeat(input, 205) || direction == 1 };
    }
    // Opens a page of the menu: the plugins list on the Options screen; anything else on the Game Settings screen,
    // which the game opens as from its own Game Settings entry (input: the press that opened it)
    inline uintptr_t Show(size_t page, int input, void* origin)
    {
        auto& state = Get();
        state.page = page;
        if (page == RWGTAMenu::Model::RootPage)
        {
            state.screen = Screen::List;
            if (!Build()) Close();
            return 0;
        }
        state.entering = true;
        Screens().sound(*Screens().accept, 0, -1, 1.0f);
        Screens().open(Options, GameSettingsScreen, nullptr, Screens().paused());
        return 0;
    }
    inline uintptr_t __cdecl OptionsInput(int input, void* origin)
    {
        auto& state = Get();
        if (state.busy || !Current(Options)) return state.optionsInput.ccall<uintptr_t>(input, origin);
        auto event = Events(input, Options, OptionsList);
        if (!state.open)
        {
            if (Selected(OptionsList) != 3 || !event.choose) return state.optionsInput.ccall<uintptr_t>(input, origin);
            if (GetTickCount64() - state.lastDiscrete < 180) return 0;
            state.lastDiscrete = GetTickCount64();
            Model(true);
            state.open = true;
            state.model.Window(RWGTAMenu::Model::RootPage).Reset();
            return Show(RWGTAMenu::Model::RootPage, input, origin);
        }
        if (state.entering) return 0;
        if ((event.choose || event.back) && GetTickCount64() - state.lastDiscrete < 180) return 0;
        if (!(event.choose || event.back)) return 0;
        state.lastDiscrete = GetTickCount64();
        if (event.back)
        {
            if (!state.notice.empty())
            {
                state.notice.clear();
                if (!Build()) Close();
            }
            else
                Close();
            return 0;
        }
        const auto row = SelectedRow(OptionsList);
        if (!row || !RWGTAMenu::Model::Opens(*row)) return 0;
        state.model.Window(state.page).SelectKey(reinterpret_cast<MenuList::Key>(row));
        const auto target = state.model.Enter(*row);
        if (target == SIZE_MAX) return 0;
        return Show(target, input, origin);
    }
    inline uintptr_t __cdecl GameInput(int input, void* origin)
    {
        auto& state = Get();
        if (state.busy || !Current(GameSettings) || !state.labels.bound)
            return state.gameInput.ccall<uintptr_t>(input, origin);
        auto event = Events(input, GameSettings, GameLabels);
        if (state.screen == Screen::Settings)
        {
            if ((event.choose || event.back) && GetTickCount64() - state.lastDiscrete < 180) return 0;
            if (event.choose || event.back) state.lastDiscrete = GetTickCount64();
            const auto row = SelectedRow(GameLabels);
            if (row) state.model.Window(state.page).SelectKey(reinterpret_cast<MenuList::Key>(row));
            if (event.back)
            {
                const auto parent = state.model.Back(state.page);
                if (parent != RWGTAMenu::Model::RootPage && parent != SIZE_MAX)
                {
                    // A category: back to its plugin's page on the same screen, on the row it came from
                    state.page = parent;
                    if (!Build()) Close();
                    return 0;
                }
                // The plugin's page: the game goes back to Options, which shows the plugins list again
                state.page = RWGTAMenu::Model::RootPage;
                state.screen = Screen::None;
                state.returning = true;
                state.labels.Restore();
                state.values.Restore();
                RestoreTitle();
                return state.gameInput.ccall<uintptr_t>(input, origin);
            }
            if (!row) return 0;
            if (RWGTAMenu::Model::Opens(*row))
            {
                if (!event.choose) return 0;
                const auto target = state.model.Enter(*row);
                if (target == SIZE_MAX) return 0;
                state.page = target;
                if (!Build()) Close();
                return 0;
            }
            if (!(event.choose || event.left || event.right)) return 0;
            if (state.model.Enabled(*row, state.pause))
                state.model.Change(*row, event.left ? -1 : event.right ? 1 : 0, state.pause);
            if (state.open && Current(GameSettings) && !Build(row)) Close();
            return 0;
        }
        int row = Selected(GameLabels) - state.labels.count;
        if (row < 0 || static_cast<size_t>(row) >= state.model.JoinedRows().size() || event.back)
            return state.gameInput.ccall<uintptr_t>(input, origin);
        if (event.choose || event.left || event.right)
        {
            state.model.Change(state.model.JoinedRows()[row], event.left ? -1 : event.right ? 1 : 0, state.pause);
            Joined();
        }
        return 0;
    }
    inline uintptr_t __cdecl OptionsInit(int* previous)
    {
        auto& state = Get();
        const bool returning = std::exchange(state.returning, false) && state.open;
        state.options.Restore();
        RestoreTitle();
        if (!returning)
        {
            state.open = false;
            state.screen = Screen::None;
            state.notice.clear();
        }
        if (Tick) Tick();
        auto result = state.optionsInit.ccall<uintptr_t>(previous);
        state.activeRoot = Options;
        if (!returning)
        {
            state.pause = previous && *previous == 55;
            Model(true);
        }
        Entry();
        if (returning)
        {
            state.page = RWGTAMenu::Model::RootPage;
            state.screen = Screen::List;
            if (!Build()) Close();
        }
        return result;
    }
    inline uintptr_t __cdecl GameInit()
    {
        auto& state = Get();
        state.labels.Restore();
        state.values.Restore();
        RestoreTitle();
        auto result = state.gameInit.ccall<uintptr_t>();
        state.activeRoot = GameSettings;
        Model();
        if (std::exchange(state.entering, false) && state.open)
        {
            state.screen = Screen::Settings;
            if (!state.labels.Capture(GameLabels) || !state.values.Capture(GameValues) || !Build())
                Close();
        }
        else
        {
            state.screen = state.open ? state.screen : Screen::None;
            Joined();
        }
        return result;
    }
    inline uintptr_t __cdecl Defaults()
    {
        auto result = Get().defaults.ccall<uintptr_t>();
        RWGTAMenu::RestoreDefaults(FixSettings(), joined);
        return result;
    }
    inline void UpdateDraw(uintptr_t root)
    {
        auto& state = Get();
        if (state.busy) return;
        state.activeRoot = root;
        if (root != Options && root != GameSettings && state.open && !state.entering && !state.returning)
        {
            // The game left the Options screens
            state.open = false;
            state.screen = Screen::None;
            state.notice.clear();
        }
        if (root != Options)
        {
            state.options.Restore();
            if (state.titleRoot == Options) RestoreTitle();
        }
        else if (!state.options.bound)
        {
            Entry();
        }
        if (root != GameSettings)
        {
            state.labels.Restore();
            state.values.Restore();
            if (state.titleRoot == GameSettings) RestoreTitle();
        }
        else if (!state.labels.bound && state.screen != Screen::Settings)
        {
            Joined();
        }
        if (Tick) Tick();
        const bool shown = state.open && ((root == Options && state.screen == Screen::List) || (root == GameSettings && state.screen == Screen::Settings));
        if (shown)
        {
            // The help under the rows follows the selection
            const auto list = root == Options ? OptionsList : GameLabels;
            const auto row = SelectedRow(list);
            auto& window = state.model.Window(state.page);
            if (row && reinterpret_cast<MenuList::Key>(row) != window.SelectedKey() && state.notice.empty())
            {
                window.SelectKey(reinterpret_cast<MenuList::Key>(row));
                Build(row);
            }
        }
        if (RWGTAMenu::Model::Changed().exchange(false))
        {
            if (shown)
            {
                if (!Build(SelectedRow(root == Options ? OptionsList : GameLabels))) Close();
            }
            else if (root == GameSettings && state.screen != Screen::Settings)
            {
                Joined();
            }
        }
    }
    // The game's screens (the apartment, the briefing, the in-game menu over a paused game) draw through Draw;
    // play and cutscenes draw none. Decided once a frame, before it is shown.
    inline ULONGLONG lastScreenDraw{};
    inline void (*NextFrame)(IDirect3DDevice9*) = nullptr;
    inline void Presented(IDirect3DDevice9* device)
    {
        RainDroplets::MenuOpen(GetTickCount64() - lastScreenDraw < 250);
        if (NextFrame)
            NextFrame(device);
    }
    inline uintptr_t __fastcall Draw(void* root, void*)
    {
        const auto address = reinterpret_cast<uintptr_t>(root);
        lastScreenDraw = GetTickCount64();
        UpdateDraw(address);
        PunMenuLayout::Scope layout(address);
        auto& state = Get();
        if (address != Options || state.open || !state.options.bound)
            return state.draw.fastcall<uintptr_t>(root, nullptr);

        // The pause menu clips the list at the bottom of its paper background.
        const int addedHeight = Read<int>(OptionsList, 56) - state.options.rectangle[3];
        const int backgroundHeight = Read<int>(OptionsBackground, 56);
        Write<int>(OptionsBackground, 56, backgroundHeight + addedHeight);
        const auto result = state.draw.fastcall<uintptr_t>(root, nullptr);
        Write<int>(OptionsBackground, 56, backgroundHeight);
        return result;
    }
    inline void Init()
    {
        NextFrame = std::exchange(D3D8Compatibility::Frame, Presented);
        auto& state = Get();
        const auto& native = Native();
        Options = native.options;
        OptionsList = native.optionsList;
        OptionsBackground = native.optionsBackground;
        GameSettings = native.gameSettings;
        GameLabels = native.gameLabels;
        GameValues = native.gameValues;

        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN, reinterpret_cast<LPCWSTR>(&Init), &state.module);
        Screens();
        state.optionsInit = safetyhook::create_inline(reinterpret_cast<void*>(native.optionsInit), OptionsInit);
        state.optionsInput = safetyhook::create_inline(reinterpret_cast<void*>(native.optionsInput), OptionsInput);
        state.gameInit = safetyhook::create_inline(reinterpret_cast<void*>(native.gameInit), GameInit);
        state.gameInput = safetyhook::create_inline(reinterpret_cast<void*>(native.gameInput), GameInput);
        state.defaults = safetyhook::create_inline(reinterpret_cast<void*>(native.defaults), Defaults);
        state.draw = safetyhook::create_inline(reinterpret_cast<void*>(native.draw), Draw);
        state.enabled = true;
    }
} // namespace PunMenu
