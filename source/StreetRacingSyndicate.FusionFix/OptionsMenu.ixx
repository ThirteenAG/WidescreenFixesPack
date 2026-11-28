module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/RWGTA/FixMenu.hpp"
#include "Shared/PadInput.hpp"
#include <array>
#include <deque>
#include <map>
#include <set>

// Options > Plugins: one more button in the game's Options menu (OptionsState). It opens the plugin list, each
// plugin's page and its categories in the same menu, built from the game's own buttons and selector rows. Every list
// shows a window of rows that scrolls by one row; like the game's own lists it doesn't wrap and Esc goes back. The
// help line, the restart notice and the scroll marks are drawn in the menu's font.

export module OptionsMenu;

import ComVars;
import States;


export namespace SRSMenu
{
    // The game's string: bit 0 of the flags = 16-bit characters, bit 1 = '#' format codes
    struct NativeString
    {
        const wchar_t* text{};
        uint8_t encoding{ 1 };
        uint8_t allocated{};
        uint16_t index{ 0xFFFF };
    };
    static_assert(sizeof(NativeString) == 8);

    struct Bindings
    {
        void* (__cdecl* allocate)(size_t){};
        void (__thiscall* initialize)(void*){};
        void (__thiscall* finish)(void*){};
        int (__thiscall* clear)(void*, bool){};
        void* (__thiscall* button)(void*, int, const NativeString*, const char*, bool, bool){};
        void* (__thiscall* slider)(void*, int, const NativeString*, const char*, const int*, const NativeString* const*, int, int, bool, bool, bool, int){};
        int (__thiscall* add)(void*, void*){};
        void* (__thiscall* drawText)(void*, void*, const NativeString*, const int*, int){};
    };
    inline Bindings api;
    struct Item
    {
        RWGTAMenu::Row row{};
        std::string label, help;
        void* widget{};
        int id{ -1 };
    };
    using RestartKey = std::pair<uintptr_t, std::string>;
    struct RestartSetting
    {
        RestartKey key;
        const AsiSettingsApi* api;
    };
    struct State
    {
        HMODULE module{};
        void* root{};
        void* building{};
        bool open{}, rebuilding{}, rebuild{};
        uint64_t generation{};
        size_t page{ RWGTAMenu::Model::RootPage };
        int pending{ -1 }, direction{};
        int pluginsId{ -1 };
        // From the window procedure (the game's thread): wheel notches, page keys, clicks; the menu's last mouse point
        int wheel{}, pageKeys{}, edgeKeys{};
        int marked{};        // a scroll mark under the mouse when its button went down: -1 up, +1 down
        bool button{};
        int mouseX{ -1 }, mouseY{ -1 };
        MenuList::WheelNotches notches;
        MenuList::Repeat shoulderRepeat[2], markRepeat;
        // The scroll marks in menu space (left, top, right, bottom), empty when not shown
        std::array<int, 4> upMark{}, downMark{};
        std::string language;
        std::wstring heading;
        std::wstring notice;
        ULONGLONG lastCaption{}, noticeUntil{};
        NativeString title;
        std::deque<std::wstring> texts;
        std::deque<NativeString> values;
        std::deque<std::array<const NativeString*, 3>> choices;
        std::vector<Item> items;
        std::vector<RestartSetting> restartSettings;
        bool restartPending{};
        RWGTAMenu::Model model;
        SafetyHookInline buildHook, updateHook, showHook, eventHook, sliderHook, lookupHook, captionHook, destroyHook, moveHook, pointHook, pollHook;
        uint8_t* moved{};       // the focus manager's first byte: the selection moved, the menu plays its move sound
    };
    State state;
    inline std::map<RestartKey, std::string> runningValues;
    constexpr int TitleId{ 0x7F000001 };
    constexpr size_t Slots{ 6 };     // the rows the Options frame holds above the prompts
    constexpr size_t MenuOffset{ 2092 };
    constexpr int MarkHeight{ 12 };
    constexpr uint32_t MarkColor{ 0xFFA0A0A0 };

    template<class T> T Read(const void* base, size_t offset = 0)
    {
        // Menu objects the game hasn't created yet are null
        return base ? *reinterpret_cast<const T*>(reinterpret_cast<uintptr_t>(base) + offset) : T{};
    }
    template<class T> void Write(void* base, size_t offset, T value)
    {
        *reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(base) + offset) = value;
    }
    inline void* Menu()
    {
        return state.root ? reinterpret_cast<uint8_t*>(state.root) + MenuOffset : nullptr;
    }
    inline std::wstring Wide(const std::string& text)
    {
        return asi_settings::text::Widen(text);
    }
    inline std::string Language()
    {
        const auto selected = FixSettings().Get("MAIN.TextLanguage");
        if (selected != "auto")
            return selected;
        // The game's text language, by the name in its language table
        const std::string_view name = LanguageTable.get_ptr()[MenuLanguage.get()];
        if (name == "FRENCH")
            return "fr";
        if (name == "GERMAN")
            return "de";
        if (name == "ITALIAN")
            return "it";
        if (name == "SPANISH")
            return "es";
        return "en";
    }
    inline void* Font()
    {
        return Read<void*>(MenuManager.is_initialized() ? MenuManager.get() : nullptr, 2944);
    }
    inline int Width(void* font, const std::wstring& text)
    {
        const NativeString string{ text.c_str() };
        auto table = Read<void**>(font);
        return table ? reinterpret_cast<int(__thiscall*)(void*, const NativeString*)>(table[1])(font, &string) : 0;
    }
    inline NativeString Text(const std::string& text, int maximum = 450)
    {
        auto value = Wide(text);
        auto font = Font();
        if (font && Width(font, value) > maximum)
        {
            while (!value.empty() && Width(font, value + L"...") > maximum)
                value.pop_back();
            value += L"...";
        }
        state.texts.push_back(std::move(value));
        return { state.texts.back().c_str() };
    }
    inline std::string ReadRestartSetting(const RestartSetting& setting)
    {
        return OtherPlugins::Fetch([&](char* buffer, uint32_t size)
        {
            return setting.api->Get(setting.key.second.c_str(), buffer, size);
        });
    }
    inline void RememberRestartSettings()
    {
        const auto generation = state.generation;
        std::vector<RestartSetting> settings;
        std::set<RestartKey> keys;
        for (const auto& page : state.model.Pages())
        {
            if (!page.plugin)
                continue;
            for (const auto& setting : page.plugin->file.description.settings)
            {
                if (!setting.HasValue() || setting.apply != "restart")
                    continue;
                RestartKey key{ reinterpret_cast<uintptr_t>(page.plugin->module), setting.id };
                if (keys.insert(key).second)
                    settings.push_back({ std::move(key), page.plugin->api });
            }
        }
        for (const auto& setting : settings)
        {
            if (runningValues.contains(setting.key))
                continue;
            auto value = ReadRestartSetting(setting);
            if (generation != state.generation)
                return;
            runningValues.try_emplace(setting.key, std::move(value));
        }
        state.restartSettings = std::move(settings);
    }
    inline bool UpdateRestartState()
    {
        const auto generation = state.generation;
        const auto settings = state.restartSettings;
        state.restartPending = false;
        for (const auto& setting : settings)
        {
            const auto current = ReadRestartSetting(setting);
            if (generation != state.generation || !state.open)
                return false;
            const auto running = runningValues.find(setting.key);
            if (running != runningValues.end() && running->second != current)
            {
                state.restartPending = true;
                break;
            }
        }
        return true;
    }
    inline void OpenModel()
    {
        state.language = Language();
        state.model.OpenMenu(state.module, state.language, Slots);
        RememberRestartSettings();
    }
    inline void Clear()
    {
        state.rebuilding = true;
        api.clear(Menu(), true);
        state.items.clear();
        state.choices.clear();
        state.values.clear();
        state.texts.clear();
        state.rebuilding = false;
    }
    inline void* Button(const std::string& label)
    {
        auto memory = api.allocate(0x414);
        if (!memory)
            return nullptr;
        const auto text = Text(label);
        return api.button(memory, 0, &text, reinterpret_cast<const char*>(state.root) + 4, false, false);
    }
    inline void* Slider(const Item& item)
    {
        auto memory = api.allocate(0x558);
        if (!memory)
            return nullptr;
        const auto label = Text(state.model.Label(item.row), 200);
        const auto value = Text(state.model.Value(item.row), 200);
        // Three choices showing the value, the middle one selected: Left/Right moves off the middle, read as a step
        state.values.push_back(value);
        state.choices.push_back({ &state.values.back(), &state.values.back(), &state.values.back() });
        return api.slider(memory, 0, &label, reinterpret_cast<const char*>(state.root) + 4, nullptr,
                          state.choices.back().data(), 3, 1, false, false, true, 235);
    }
    inline void AddPlugins()
    {
        // In capitals like the game's own Options items
        auto label = Wide(state.model.Text("menu.plugins"));
        CharUpperBuffW(label.data(), static_cast<DWORD>(label.size()));
        if (auto widget = Button(asi_settings::text::Narrow(label)))
            state.pluginsId = api.add(Menu(), widget);
    }
    inline void __fastcall Show(void* menu, void*)
    {
        if (state.building == state.root && menu == Menu() && !state.open)
            AddPlugins();
        state.showHook.thiscall<void>(menu);
    }
    inline std::wstring Upper(const std::string& text)
    {
        auto wide = Wide(text);
        CharUpperBuffW(wide.data(), static_cast<DWORD>(wide.size()));
        return wide;
    }
    // A new list comes in with the menu's own animation; the same list after a scroll or a change only gets new rows
    inline void Build(bool animate = true)
    {
        state.rebuild = false;
        RWGTAMenu::Model::Changed().exchange(false);
        if (!UpdateRestartState())
            return;
        Clear();
        if (animate)
            api.initialize(Menu());
        const auto& pages = state.model.Pages();
        if (state.page >= pages.size())
            state.page = RWGTAMenu::Model::RootPage;
        for (auto row : state.model.Shown(state.page, false))
        {
            // Plugins and categories open lists, like the game's own Options entries (in capitals)
            auto label = RWGTAMenu::Model::Opens(*row) ? asi_settings::text::Narrow(Upper(state.model.Label(*row))) : state.model.Label(*row);
            state.items.push_back({ *row, std::move(label), state.model.Help(*row) });
        }
        auto& window = state.model.Window(state.page);
        state.heading = Upper(pages[state.page].title);
        state.title = { state.heading.c_str() };
        Write<int>(Menu(), 212, TitleId);
        for (auto& item : state.items)
        {
            const bool selector = (item.row.kind == RWGTAMenu::RowKind::Setting || item.row.kind == RWGTAMenu::RowKind::Preset ||
                !state.model.Value(item.row).empty()) && !RWGTAMenu::Model::Opens(item.row);
            item.widget = selector ? Slider(item) : Button(item.label);
            if (item.widget)
                item.id = api.add(Menu(), item.widget);
        }
        api.finish(Menu());
        state.showHook.thiscall<void>(Menu());
        Write<int>(Menu(), 896, state.items.empty() ? 0 : static_cast<int>(window.Slot()));
    }
    inline void Stock()
    {
        Clear();
        state.open = false;
        state.page = RWGTAMenu::Model::RootPage;
        state.pluginsId = -1;
        Write<int>(Menu(), 212, 443);
        state.building = state.root;
        state.buildHook.thiscall<int>(state.root);
        state.building = nullptr;
        // Back on the Plugins entry (the last one)
        if (const int count = Read<int>(Menu(), 456); count > 0)
            Write<int>(Menu(), 896, count - 1);
    }
    // Esc: the list it came from, at the row it came from; the game's Options from the plugins list
    inline void Back()
    {
        const auto parent = state.model.Back(state.page);
        if (parent == SIZE_MAX)
        {
            Stock();
            return;
        }
        state.page = parent;
        Build();
    }
    inline void Activate(size_t slot, int direction)
    {
        if (slot >= state.items.size())
            return;
        const auto item = state.items[slot];
        const auto generation = state.generation;
        state.model.Window(state.page).Point(slot);
        const bool opened = RWGTAMenu::Model::Opens(item.row);
        if (opened)
        {
            if (direction)
                return;
            const auto target = state.model.Enter(item.row);
            if (target == SIZE_MAX)
                return;
            state.page = target;
        }
        else if (state.model.Enabled(item.row, false))
            state.model.Change(item.row, direction, false);
        if (state.root && state.open && generation == state.generation)
            Build(opened);
    }
    // The window moved (wheel, page keys, scroll marks): new rows and the move sound
    inline void Scrolled(bool changed)
    {
        if (!changed)
            return;
        Build(false);
        if (state.moved)
            *state.moved = 1;
    }
    // Up/Down from the keys, the pad and the stick (the game's focus manager, with its own repeat): one row; at the
    // window's edge the window scrolls by one row
    inline void __fastcall Move(void* focus, void*, float dx, float dy)
    {
        state.moved = static_cast<uint8_t*>(focus);
        if (state.open && state.root && dy != 0.0f && dx == 0.0f && !state.items.empty())
        {
            auto& window = state.model.Window(state.page);
            const auto first = window.First();
            if (!window.Move(dy > 0.0f ? 1 : -1, false))
                return;
            if (window.First() != first)
            {
                state.rebuild = true;
                if (state.moved)
                    *state.moved = 1;
                return;
            }
        }
        state.moveHook.thiscall<void>(focus, dx, dy);
    }
    inline bool Inside(const std::array<int, 4>& mark);
    // The game's DirectInput poll: while a list of the fix is open the wheel (the mouse's raw lZ, notches kept whole)
    // scrolls it and doesn't also step the selected row's value; the left button on a scroll mark scrolls (held: with
    // the game's repeat) and doesn't also press the selected row, as the game takes any click for it
    inline int __fastcall Poll(void* adapter, void*)
    {
        const auto result = state.pollHook.thiscall<int>(adapter);
        if (state.open)
        {
            state.wheel += state.notches.Take(Read<LONG>(adapter, 408));
            Write<LONG>(adapter, 408, 0);
            const bool down = Read<uint8_t>(adapter, 412) & 0x80;
            if (down && (!state.button || state.marked))
            {
                const int mark = Inside(state.upMark) ? -1 : Inside(state.downMark) ? 1 : 0;
                if (!state.button)
                    state.marked = mark;
                if (state.marked)
                    Write<uint8_t>(adapter, 412, 0);
            }
            if (!down)
                state.marked = 0;
            state.button = down;
        }
        return result;
    }
    // The mouse over the menu, in its own coordinates
    inline void __fastcall Point(void* focus, void*, int x, int y)
    {
        state.mouseX = x;
        state.mouseY = y;
        state.pointHook.thiscall<void>(focus, x, y);
    }
    inline bool Inside(const std::array<int, 4>& mark)
    {
        return mark[2] > mark[0] && state.mouseX >= mark[0] && state.mouseX < mark[2] && state.mouseY >= mark[1] && state.mouseY < mark[3];
    }
    // Window messages: Page Up/Down, Home/End
    inline void Message(UINT message, WPARAM wParam, LPARAM)
    {
        if (!state.open)
            return;
        if (message == WM_KEYDOWN && (wParam == VK_PRIOR || wParam == VK_NEXT))
            state.pageKeys += wParam == VK_NEXT ? 1 : -1;
        else if (message == WM_KEYDOWN && (wParam == VK_HOME || wParam == VK_END))
            state.edgeKeys = wParam == VK_END ? 1 : -1;
    }
    // The input the game's menu has no use for: wheel, page keys, shoulder buttons, the scroll marks
    inline void Input()
    {
        auto& window = state.model.Window(state.page);
        // The selection follows the row the mouse put the focus on
        if (!Read<uint8_t>(Menu(), 900))
        {
            for (size_t i = 0; i < state.items.size(); ++i)
                if (state.items[i].widget && Read<uint8_t>(state.items[i].widget, 268) && i != window.Slot())
                    window.Point(i);
        }
        const auto wheel = std::exchange(state.wheel, 0);
        for (int i = 0; i < std::abs(wheel); ++i)
            Scrolled(window.Scroll(wheel > 0 ? -1 : 1));
        const auto pages = std::exchange(state.pageKeys, 0);
        for (int i = 0; i < std::abs(pages); ++i)
            Scrolled(window.Page(pages > 0 ? 1 : -1));
        if (const auto edge = std::exchange(state.edgeKeys, 0))
            Scrolled(window.Edge(edge));
        if (state.markRepeat.Step(state.marked != 0, GetTickCount64()))
            Scrolled(window.Scroll(state.marked));
        XINPUT_STATE pad{};
        DWORD slot{};
        const bool read = PadInput::Read(pad, slot);
        const auto now = GetTickCount64();
        for (int i = 0; i < 2; ++i)
        {
            const WORD button = i ? XINPUT_GAMEPAD_RIGHT_SHOULDER : XINPUT_GAMEPAD_LEFT_SHOULDER;
            if (state.shoulderRepeat[i].Step(read && (pad.Gamepad.wButtons & button), now))
                Scrolled(window.Page(i ? 1 : -1));
        }
    }
    inline int __fastcall BuildStock(void* root, void*)
    {
        if (state.root != root)
        {
            ++state.generation;
            state.root = root;
            state.open = false;
            state.page = RWGTAMenu::Model::RootPage;
            state.pending = -1;
            state.items.clear();
            state.texts.clear();
            state.choices.clear();
            state.values.clear();
            OpenModel();
            if (state.root != root)
                return 0;
        }
        state.building = root;
        const auto result = state.buildHook.thiscall<int>(root);
        state.building = nullptr;
        return result;
    }
    inline int __fastcall Event(void* root, void*, int id, int type, int data)
    {
        // Type 8: an item was pressed (id 1 = Back). The game answers its own items once their menu has animated out;
        // the fix's items and Back on the fix's pages are taken at once, by the next update.
        if (state.root == root && type == 8 && (state.open || id == state.pluginsId))
        {
            Write<int>(root, 3656, id);
            Write<uint8_t>(root, 3660, 1);
            return 0;
        }
        return state.eventHook.thiscall<int>(root, id, type, data);
    }
    inline void __fastcall SliderUpdate(void* widget, void*)
    {
        state.sliderHook.thiscall<void>(widget);
        if (!state.open || state.rebuilding || !Read<uint8_t>(widget, 1316))
            return;
        for (size_t i = 0; i < state.items.size(); ++i)
        {
            if (state.items[i].widget == widget)
            {
                state.pending = static_cast<int>(i);
                state.direction = Read<int>(widget, 1204) - 1;
                return;
            }
        }
    }
    inline int __fastcall Update(void* root, void*)
    {
        if (state.root != root)
            return state.updateHook.thiscall<int>(root);
        if (state.pending >= 0 && state.open)
        {
            const auto command = std::exchange(state.pending, -1);
            Activate(command, state.direction);
        }
        if (state.open && state.rebuild)
            Build(false);
        if (state.open)
            Input();
        // +3660: an item was chosen, +3656: its id (1 = Back)
        if (Read<uint8_t>(root, 3660))
        {
            const auto id = Read<int>(root, 3656);
            if (!state.open && id == state.pluginsId)
            {
                Write<uint8_t>(root, 3660, 0);
                const auto language = Language();
                if (language != state.language)
                {
                    state.model.SelectLanguage(language);
                    state.language = language;
                }
                state.open = true;
                state.notches.Clear();
                state.page = RWGTAMenu::Model::RootPage;
                state.model.Window(state.page).Reset();
                Build();
            }
            else if (state.open)
            {
                Write<uint8_t>(root, 3660, 0);
                if (id == 1)
                    Back();
                else
                    for (size_t i = 0; i < state.items.size(); ++i)
                        if (state.items[i].id == id)
                        {
                            Activate(i, 0);
                            break;
                        }
            }
        }
        if (state.open && RWGTAMenu::Model::Changed())
            Build(false);
        return state.updateHook.thiscall<int>(root);
    }
    inline void* __cdecl Lookup(int id)
    {
        return id == TitleId ? &state.title : state.lookupHook.ccall<void*>(id);
    }
    inline bool NoticeReady()
    {
        return state.lastCaption && GetTickCount64() - state.lastCaption < 250;
    }
    inline void ShowNotice(const std::wstring& text)
    {
        state.notice = text;
        state.noticeUntil = GetTickCount64() + 12000;
    }
    inline char __fastcall Caption(void* menu, void*)
    {
        const auto result = state.captionHook.thiscall<char>(menu);
        const bool options = menu == Menu();
        const auto deepest = SRSStates::Deepest();
        const bool main = deepest && std::string_view(SRSStates::Name(deepest)) == "MainMenuState" &&
            menu == deepest + MenuOffset;
        if ((!options && !main) || Read<float>(menu, 220) < 0.05f)
            return result;
        auto font = Font();
        auto table = Read<void**>(font);
        auto attributes = Read<void*>(font, 36);
        if (!table || !attributes)
            return result;
        state.lastCaption = GetTickCount64();
        if (!state.notice.empty() && (state.lastCaption >= state.noticeUntil || FixSettings().Get("MAIN.CheckForUpdates") == "0"))
            state.notice.clear();
        auto help = state.notice;
        const bool list = options && state.open && !state.items.empty();
        if (help.empty())
        {
            if (!list)
                return result;
            const auto slot = std::min(state.model.Window(state.page).Slot(), state.items.size() - 1);
            help = Wide(state.restartPending ? state.model.Text("menu.restartwarning") : state.items[slot].help);
        }
        const auto flags = Read<int>(attributes, 12);
        const auto color = Read<uint32_t>(attributes, 16);
        auto align = reinterpret_cast<void(__thiscall*)(void*, int)>(table[3]);
        auto tint = reinterpret_cast<void(__thiscall*)(void*, uint32_t)>(table[4]);
        const int depth = Read<int>(menu, 48) + 3;
        align(font, 0);
        tint(font, 0xFFFFFFFF);
        // Leave room for the Main Menu's sign-in status above the notice.
        const NativeString string{ help.c_str() };
        const int rectangle[]{ 25, main ? 28 : 4, 590, 0 };
        int extent[2]{};
        api.drawText(font, extent, &string, rectangle, depth);
        state.upMark = state.downMark = {};
        if (list)
        {
            // The font has no arrows: an ellipsis above the first and below the last row, centred on the rows (where
            // labels and values meet), in their grey, says there are more rows that way
            const auto& window = state.model.Window(state.page);
            const auto top = state.items.front().widget, bottom = state.items.back().widget;
            // Widgets: centre x +8, centre y +12, height +32, width +40
            const int centre = static_cast<int>(Read<float>(top, 8));
            tint(font, MarkColor);
            auto mark = [&](int y, std::array<int, 4>& area)
            {
                const NativeString text{ L"..." };
                const int w = Width(font, text.text);
                const int place[]{ centre - w / 2, y, centre + w / 2 + 2, 0 };
                int size[2]{};
                api.drawText(font, size, &text, place, depth);
                area = { centre - w / 2 - 12, y - 4, centre + w / 2 + 12, y + MarkHeight + 4 };
            };
            if (window.Above())
                mark(static_cast<int>(Read<float>(top, 12) - Read<float>(top, 32) / 2) - MarkHeight - 4, state.upMark);
            if (window.Below())
                mark(static_cast<int>(Read<float>(bottom, 12) + Read<float>(bottom, 32) / 2) - 4, state.downMark);
        }
        tint(font, color);
        align(font, flags);
        return result;
    }
    inline void* __fastcall Destroy(void* root, void*, char flags)
    {
        const auto result = state.destroyHook.thiscall<void*>(root, flags);
        if (state.root == root)
        {
            ++state.generation;
            state.root = state.building = nullptr;
            state.open = false;
            state.pending = -1;
            state.lastCaption = 0;
            state.items.clear();
            state.choices.clear();
            state.values.clear();
            state.texts.clear();
            state.restartSettings.clear();
            state.model.Close();
        }
        return result;
    }

    inline void Init()
    {
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_PIN | GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                           reinterpret_cast<LPCWSTR>(&Init), &state.module);
        auto& settings = FixSettings();
        for (const auto& setting : settings.GetDescription().settings)
            if (setting.HasValue() && setting.apply == "restart")
                runningValues.try_emplace(RestartKey{ reinterpret_cast<uintptr_t>(state.module), setting.id }, settings.Get(setting.id));
        auto build = hook::pattern("56 8B F1 57 8D BE 2C 08 00 00 8B CF E8 ? ? ? ? 68 14 04 00 00 E8 ? ? ? ? 83 C4 04 85 C0 74 18 6A 00 6A 00 8D 4E 04 51 6A 00 68 BD 01 00 00").get_first();
        auto update = hook::pattern("56 8B F1 E8 ? ? ? ? 84 C0 75 21 8A 86 4C 0E 00 00").get_first();
        auto show = hook::pattern("56 8B F1 83 BE B4 00 00 00 01 C7 86 70 03 00 00 FF FF FF FF C6 86 84 03 00 00 01").get_first();
        auto event = hook::pattern("8B 44 24 08 83 E8 03 74 1A 48 74 17 83 E8 04 75 19 8B 44 24 04 89 81 48 0E 00 00").get_first();
        auto sliderUpdate = hook::pattern("56 57 8B F1 E8 ? ? ? ? 8B CE E8 ? ? ? ? 8D BE 2C 04 00 00 8B CF E8 ? ? ? ? 8A 86 0C").get_first();
        auto lookup = hook::pattern("8A 0D ? ? ? ? B8 01 00 00 00 84 C8 75 2C 8B 0D ? ? ? ? 0B C8 6A 00").get_first();
        auto caption = hook::pattern("83 EC 30 57 8B F9 8A 87 BA 03 00 00 84 C0 0F 85 ? ? ? ? 8D 4C 24 14").get_first();
        // OptionsState's destructor: the first entry of the method table its constructor sets
        auto methods = *hook::pattern("8B CE C7 06 ? ? ? ? E8 ? ? ? ? 8D 46 04 50 8D 8E 2C 08 00 00 E8 ? ? ? ? 8A 4C 24 08 C7 86 00 09 00 00 BB 01 00 00").get_first<void**>(4);
        auto destroy = methods[0];
        api.clear = reinterpret_cast<decltype(api.clear)>(hook::pattern("53 57 8B F9 8B 87 B0 00 00 00 8B 8F B4 00 00 00 33 DB 3B C1 75 25").get_first());
        api.button = reinterpret_cast<decltype(api.button)>(hook::pattern("8B 44 24 0C 53 55 56 57 50 8B F1 E8 ? ? ? ? 8D BE EC 02 00 00 8B CF").get_first());
        api.slider = reinterpret_cast<decltype(api.slider)>(hook::pattern("8B 44 24 2C 8B 54 24 24 53 8B 5C 24 08 56 57 8B 7C 24 14 50 8B 44 24 30 8B F1").get_first());
        api.add = reinterpret_cast<decltype(api.add)>(hook::pattern("83 EC 18 55 8B 6C 24 20 85 ED 56 8B F1 75 18 68 ? ? ? ? E8 ? ? ? ? 83 C4 04 5E 83 C8 FF").get_first());
        api.drawText = reinterpret_cast<decltype(api.drawText)>(hook::pattern("83 EC 44 53 55 33 C0 56 57 8B F9 8D 4C 24 44 89 44 24 1C 89 44 24 18 89 44 24 30 89 44 24 14").get_first());
        api.finish = reinterpret_cast<decltype(api.finish)>(hook::pattern("56 8B F1 6A 00 6A 00 8D 8E E8 00 00 00 E8 ? ? ? ? 6A 00 6A 00 8D 8E 90 02 00 00 E8 ? ? ? ? 5E C3").get_first());
        api.initialize = reinterpret_cast<decltype(api.initialize)>(injector::GetBranchDestination(static_cast<uint8_t*>(build) + 12).as_int());
        api.allocate = reinterpret_cast<decltype(api.allocate)>(injector::GetBranchDestination(static_cast<uint8_t*>(build) + 22).as_int());
        if (auto* manager = Read<void**>(reinterpret_cast<void*>(api.initialize), 1))
            MenuManager.SetAddress(manager);
        auto language = hook::pattern("A1 ? ? ? ? 8B 0C 85 ? ? ? ? 51 68");
        MenuLanguage.SetAddress(*language.get_first<int32_t*>(1));
        LanguageTable.SetAddress(*language.get_first<const char**>(8));
        state.buildHook = safetyhook::create_inline(build, BuildStock);
        state.updateHook = safetyhook::create_inline(update, Update);
        state.showHook = safetyhook::create_inline(show, Show);
        state.eventHook = safetyhook::create_inline(event, Event);
        state.sliderHook = safetyhook::create_inline(sliderUpdate, SliderUpdate);
        state.lookupHook = safetyhook::create_inline(lookup, Lookup);
        state.captionHook = safetyhook::create_inline(caption, Caption);
        state.destroyHook = safetyhook::create_inline(destroy, Destroy);
        // The focus manager: a step in a direction, and the focus under the mouse
        auto move = hook::pattern("56 8B 71 04 85 F6 74 3A 8B 86 88 00 00 00 85 C0 74 30 8B 44 24 0C 8B 4C 24 08 50 51 8B CE E8").get_first();
        auto point = hook::pattern("51 53 8B D9 8B 83 88 00 00 00 57 33 FF 85 C0 89 44 24 08 74 04 C6 40 0C 00 56 8B B3 8C 00 00 00").get_first();
        state.moveHook = safetyhook::create_inline(move, Move);
        state.pointHook = safetyhook::create_inline(point, Point);
        // The DirectInput adapter's poll; the mouse state (DIMOUSESTATE) is at +400
        auto poll = hook::pattern("51 53 56 8B F1 8B 46 08 8B 08 57 50 FF 51 1C 8B 46 1C 8B 10 50 FF 52 1C 8B 46 08 8B 08 50 FF 51 64").get_first();
        state.pollHook = safetyhook::create_inline(poll, Poll);
    }
}
