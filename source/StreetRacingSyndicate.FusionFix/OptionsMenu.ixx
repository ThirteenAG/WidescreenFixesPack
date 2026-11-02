module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/RWGTA/FixMenu.hpp"
#include <array>
#include <deque>

// Options > Plugins: one more button in the game's Options menu (OptionsState). It opens the plugin list and each
// plugin's page in the same menu, built from the game's own buttons and selector rows; the help line and the restart
// notice are drawn under the menu in the menu's font.

export module OptionsMenu;

import ComVars;


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
    enum class Kind
    {
        Plugin,
        Setting,
        Back,
        Previous,
        Next
    };
    struct Item
    {
        Kind kind{};
        size_t index{};
        RWGTAMenu::Row row{};
        std::string label, help;
        void* widget{};
        int id{ -1 };
    };
    struct State
    {
        HMODULE module{};
        void* root{};
        void* building{};
        bool open{}, rebuilding{};
        uint64_t generation{};
        int page{ -1 }, pending{ -1 }, direction{};
        size_t start{}, selected{};
        int pluginsId{ -1 };
        std::string language;
        std::wstring heading;
        NativeString title;
        std::deque<std::wstring> texts;
        std::deque<NativeString> values;
        std::deque<std::array<const NativeString*, 3>> choices;
        std::vector<Item> items;
        RWGTAMenu::Model model;
        SafetyHookInline buildHook, updateHook, showHook, eventHook, sliderHook, lookupHook, captionHook, destroyHook;
    };
    State state;
    constexpr int TitleId{ 0x7F000001 };
    constexpr size_t PageRows{ 6 };

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
        return state.root ? reinterpret_cast<uint8_t*>(state.root) + 2092 : nullptr;
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
    inline void OpenModel()
    {
        state.language = Language();
        state.model.OpenPlugins(state.module, state.language, PageRows);
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
    inline void Build()
    {
        Clear();
        api.initialize(Menu());
        const auto& pages = state.model.Pages();
        if (state.page < 0)
        {
            for (size_t i = state.start; i < std::min(state.start + 5, pages.size()); ++i)
                state.items.push_back({ Kind::Plugin, i, {}, pages[i].title, {} });
            if (state.start)
                state.items.push_back({ Kind::Previous, state.start >= 5 ? state.start - 5 : 0, {}, state.model.Text("menu.previous"), {} });
            if (state.start + 5 < pages.size())
                state.items.push_back({ Kind::Next, state.start + 5, {}, state.model.Text("menu.next"), {} });
        }
        else if (static_cast<size_t>(state.page) < pages.size())
        {
            for (auto row : state.model.VisibleRows(state.page))
                state.items.push_back({ Kind::Setting, 0, *row, state.model.Label(*row), state.model.Help(*row) });
        }
        state.items.push_back({ Kind::Back, 0, {}, state.model.Text("menu.back"), {} });
        state.selected = std::min(state.selected, state.items.size() - 1);
        state.heading = Wide(state.page < 0 ? state.model.Text("menu.plugins") : pages[state.page].title);
        state.title = { state.heading.c_str() };
        Write<int>(Menu(), 212, TitleId);
        for (auto& item : state.items)
        {
            const bool selector = item.kind == Kind::Setting &&
                (item.row.kind == RWGTAMenu::RowKind::Setting || item.row.kind == RWGTAMenu::RowKind::Preset);
            item.widget = selector ? Slider(item) : Button(item.label);
            if (item.widget)
                item.id = api.add(Menu(), item.widget);
        }
        api.finish(Menu());
        state.showHook.thiscall<void>(Menu());
        Write<int>(Menu(), 896, static_cast<int>(state.selected));
        RWGTAMenu::Model::Changed() = false;
    }
    inline void Stock()
    {
        Clear();
        state.open = false;
        state.page = -1;
        state.start = state.selected = 0;
        state.pluginsId = -1;
        Write<int>(Menu(), 212, 443);
        state.building = state.root;
        state.buildHook.thiscall<int>(state.root);
        state.building = nullptr;
    }
    inline void Activate(size_t index, int direction)
    {
        if (index >= state.items.size())
            return;
        const auto item = state.items[index];
        const auto generation = state.generation;
        state.selected = index;
        if (item.kind == Kind::Back)
        {
            if (state.page < 0)
            {
                Stock();
                return;
            }
            state.page = -1;
            state.selected = 0;
        }
        else if (item.kind == Kind::Plugin)
        {
            state.page = static_cast<int>(item.index);
            state.selected = 0;
            state.model.PageShown(item.index);
        }
        else if (item.kind == Kind::Previous || item.kind == Kind::Next)
        {
            state.start = item.index;
            state.selected = 0;
        }
        else if (state.model.Enabled(item.row, false))
            state.model.Change(item.row, direction, false);
        if (state.root && state.open && generation == state.generation)
            Build();
    }
    inline int __fastcall BuildStock(void* root, void*)
    {
        if (state.root != root)
        {
            ++state.generation;
            state.root = root;
            state.open = false;
            state.page = -1;
            state.pending = -1;
            state.start = state.selected = 0;
            state.items.clear();
            state.texts.clear();
            state.choices.clear();
        state.values.clear();
            OpenModel();
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
        // +3660: an item was chosen, +3656: its id (1 = Back)
        if (Read<uint8_t>(root, 3660))
        {
            const auto id = Read<int>(root, 3656);
            if (!state.open && id == state.pluginsId)
            {
                Write<uint8_t>(root, 3660, 0);
                state.open = true;
                state.page = -1;
                state.selected = 0;
                Build();
            }
            else if (state.open)
            {
                Write<uint8_t>(root, 3660, 0);
                if (id == 1)
                    Activate(state.items.size() - 1, 0);
                else
                    for (size_t i = 0; i < state.items.size(); ++i)
                        if (state.items[i].id == id)
                        {
                            Activate(i, 0);
                            break;
                        }
            }
        }
        if (RWGTAMenu::Model::Changed().exchange(false) && state.open)
            Build();
        return state.updateHook.thiscall<int>(root);
    }
    inline void* __cdecl Lookup(int id)
    {
        return id == TitleId ? &state.title : state.lookupHook.ccall<void*>(id);
    }
    inline char __fastcall Caption(void* menu, void*)
    {
        const auto result = state.captionHook.thiscall<char>(menu);
        if (!state.open || menu != Menu() || state.items.empty() || Read<float>(menu, 220) < 0.05f)
            return result;
        auto font = Font();
        auto table = Read<void**>(font);
        auto attributes = Read<void*>(font, 36);
        if (!table || !attributes)
            return result;
        for (size_t i = 0; i < state.items.size(); ++i)
            if (Read<uint8_t>(state.items[i].widget, 268))
                state.selected = i;
        auto text = state.model.RestartRequired() ? state.model.Text("menu.restartwarning") : state.items[state.selected].help;
        const auto help = Wide(text);
        const auto flags = Read<int>(attributes, 12);
        const auto color = Read<uint32_t>(attributes, 16);
        auto align = reinterpret_cast<void(__thiscall*)(void*, int)>(table[3]);
        auto tint = reinterpret_cast<void(__thiscall*)(void*, uint32_t)>(table[4]);
        align(font, 0);
        tint(font, 0xFFFFFFFF);
        // Above the menu's banner, where the Options pages have no items; the game wraps it to the width
        const NativeString string{ help.c_str() };
        const int rectangle[]{ 25, 4, 590, 0 };
        int extent[2]{};
        api.drawText(font, extent, &string, rectangle, Read<int>(menu, 48) + 3);
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
            state.items.clear();
            state.choices.clear();
        state.values.clear();
            state.texts.clear();
            state.model.Close();
        }
        return result;
    }

    inline void Init()
    {
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_PIN | GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                           reinterpret_cast<LPCWSTR>(&Init), &state.module);
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
    }
}
