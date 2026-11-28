module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/RWGTA/FixMenu.hpp"
#include "Shared/PadInput.hpp"
#include "Shared/RainDroplets.hpp"
#include <array>
#include <atomic>
#include <memory>

export module NativeMenu;

import ComVars;

export namespace SufferingMenu
{
    using Object = uintptr_t;
    template<class T> T& Field(Object object, size_t offset)
    {
        return *reinterpret_cast<T*>(object + offset);
    }
    template<class T> void SetField(Object object, size_t offset, T value)
    {
        Field<T>(object, offset) = value;
    }
    inline void* find_pattern(HMODULE module, const char* bytes)
    {
        return hook::module_pattern(module, bytes).get_first();
    }
    struct Bindings
    {
        void*(__cdecl* allocate)(int, size_t){};
        Object(__thiscall* button)(Object, int){};
        Object(__thiscall* text)(Object, int){};
        int(__thiscall* setText)(Object, const wchar_t*){};
        int(__thiscall* addChild)(Object, Object){};
        int(__thiscall* select)(Object, int){};
        int(__thiscall* pending)(Object, int, int){}; // a menu stack's input: is this message waiting
    };
    struct MenuLayout
    {
        size_t tableSize;
        size_t updateSlot;
        // The menu stack pops the top page on Escape (polled, not sent to the page) when the page's slot 12 allows it
        size_t escapeSlot;
        int escape;
        bool joinedGraphics;
        int left, right, accept, back; // the sequel numbers its menu messages differently; it has no Back message
        // (Up and Down are the two messages before Left)
        // The page's rows, title and help line as fractions of the Options page. The sequel's page is the whole title
        // screen: its rows go in two columns on the paper strip below the logo, the title and help line on the logo's
        // paper, where its dark text reads.
        struct Box
        {
            float x, y, width;
        };
        Box rows;
        float rowStep, columnStep;
        int rowsPerColumn;
        Box title, help;
        int helpTemplate{ -1 };    // the template of the game's help lines (the Graphics page's), -1: the rows' one
    };
    inline constexpr MenuLayout Retail{ 29, 23, 12, 1027, true, 1006, 1007, 1020, 1021,
        { 0.12f, 0.29f, 0.76f }, 0.075f, 0.0f, 7, { 0.08f, 0.185f, 0.84f }, { 0.1f, 0.85f, 0.8f }, 1017 };
    inline constexpr MenuLayout TiesThatBind{ 32, 26, 12, 1057, false, 1036, 1037, 1050, 0,
        { 0.12f, 0.735f, 0.38f }, 0.047f, 0.4f, 4, { 0.62f, 0.245f, 0.25f }, { 0.2f, 0.458f, 1.05f } };
    inline const MenuLayout* Layout = &Retail;
    inline Bindings api;
    inline Object* menuStack{}; // the game's menu stack: its pages at +80, their count at +88, flags at +4
    inline HMODULE host{};
    inline std::string notice; // an update notice, shown in the fix page's help line while the row has no help
    inline std::string Language()
    {
        const auto language = FixSettings().String("MAIN.TextLanguage");
        if (language != "auto")
            return language;
        switch (languageCode.is_initialized() ? languageCode.get() : 0)
        {
            case 0x5247: return "de";
            case 0x5053: return "es";
            case 0x5246: return "fr";
            case 0x5449: return "it";
            default: return "en";
        }
    }
    inline SafetyHookInline optionsHook, videoHook, defaultsHook;
    // A row of the shown window: a model row, or Back (no row)
    struct Item
    {
        bool back{};
        RWGTAMenu::Row row{};
    };
    struct Page;
    struct Listener
    {
        void** vtable{};
        Page* page{};
    };
    struct Page
    {
        Object root{}, entry{}, help{}, title{}, up{}, down{}, nativeTitle{};
        uintptr_t* originalTable{};
        std::array<uintptr_t, TiesThatBind.tableSize> table{};
        Listener listener;
        RWGTAMenu::Model model;
        std::vector<Item> items;
        std::vector<Object> controls, privateChildren;
        std::vector<std::pair<Object, int>> stockStates;
        Object stockChildren{};
        int stockCapacity{}, stockCount{}, stockSelection{}, stockOrientation{};
        size_t page{ RWGTAMenu::Model::RootPage };
        int command{ -1 }, direction{};
        bool open{}, video{}, escapeHeld{};
        MenuList::Repeat shoulders[2], held[2];
        MenuList::StickAxis stick;
        std::string shownHelp;
        std::vector<std::pair<Object, RWGTAMenu::Row>> joined;
    };
    inline std::map<Object, std::unique_ptr<Page>> pages;
    // Wheel notches from the game's DirectInput mouse (its raw lZ, the remainder kept); Page Up/Down and Home/End
    // from the window procedure
    inline std::atomic<int> wheel{}, pageKeys{}, edgeKeys{};
    inline MenuList::WheelNotches notches;
    inline SafetyHookInline mouseHook;
    inline int __fastcall PollMouse(Object mouse, void*, float elapsed, int reset)
    {
        const int result = mouseHook.thiscall<int>(mouse, elapsed, reset);
        // 2: read; the DIMOUSESTATE at +16 (lZ +24)
        if (result == 2 && std::any_of(pages.begin(), pages.end(), [](const auto& page) { return page.second->open; }))
            wheel += notches.Take(Field<int>(mouse, 24));
        return result;
    }
    inline void Message(UINT message, WPARAM wParam)
    {
        if (message == WM_KEYDOWN && (wParam == VK_PRIOR || wParam == VK_NEXT))
            pageKeys += wParam == VK_NEXT ? 1 : -1;
        else if (message == WM_KEYDOWN && (wParam == VK_HOME || wParam == VK_END))
            edgeKeys = wParam == VK_END ? 1 : -1;
    }
    inline std::wstring FontText(std::string text, bool label = true)
    {
        if (label && Language() == "en")
            for (size_t start = 0; start < text.size();)
            {
                auto end = text.find(' ', start);
                if (end == std::string::npos) end = text.size();
                auto word = text.substr(start, end - start);
                bool minor = start && (word == "of" || word == "the" || word == "and" || word == "to");
                if (!minor && start < end && text[start] >= 'a' && text[start] <= 'z')
                    text[start] -= 'a' - 'A';
                start = end + 1;
            }
        int length = MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
        std::wstring wide(length, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), wide.data(), length);
        return wide;
    }
    inline std::string Help(const RWGTAMenu::Model& model, const RWGTAMenu::Row& row)
    {
        std::string text;
        if (row.plugin && row.plugin->module == host && row.setting && row.setting->id.starts_with("MAIN."))
            text = OtherPlugins::Text(*row.plugin, "@menu." + row.setting->id.substr(5) + ".help");
        else
            text = model.Help(row);
        if (text.size() > 85)
        {
            auto end = text.rfind(' ', 82);
            text.resize(end == std::string::npos ? 82 : end);
            text += "...";
        }
        return text;
    }
    inline std::string Text(const char* key)
    {
        return RWGTAMenu::PluginText(FixSettings(), Language(), key);
    }
    inline void SetText(Object control, const std::string& text, bool label = true)
    {
        api.setText(control, FontText(text, label).c_str());
    }
    inline void Geometry(Object object, float x, float y, float width, float height)
    {
        SetField<int>(object, 48, 0);
        SetField<int>(object, 52, 0);
        SetField<float>(object, 56, x);
        SetField<float>(object, 60, y);
        SetField<float>(object, 64, width);
        SetField<float>(object, 68, height);
    }
    // A control made from a template of the menu data (font, size, colour): the source control's, or the one given
    inline Object Create(Page& page, Object source, bool button, int templateIndex = -1)
    {
        auto memory = reinterpret_cast<Object>(api.allocate(0, button ? 0xC4 : 0xC0));
        if (!memory)
            return 0;
        auto object = (button ? api.button : api.text)(memory, templateIndex >= 0 ? templateIndex : Field<int>(source, 12));
        SetField<Object>(object, 96, reinterpret_cast<Object>(&page.listener));
        SetField<Object>(object, 100, page.root);
        SetField<int>(object, 36, 4);
        return object;
    }
    // The Options rows the Plugins entry goes between: retail puts it above Back, after Credits (Audio on the pause
    // menu's page, which has no Credits); the sequel (Escape for Back) in the gap between Gameplay and Graphics on its
    // upper row
    inline std::pair<Object, Object> Neighbours(Object root)
    {
        if (!Layout->joinedGraphics)
            return { Field<Object>(root, 124), Field<Object>(root, 144) };
        // The pause menu's page creates Credits but doesn't list it
        const auto credits = Field<Object>(root, 144);
        const auto children = Field<Object*>(root, 80);
        const auto end = children + std::max(0, Field<int>(root, 88));
        const bool listed = credits && children && std::find(children, end, credits) != end;
        return { listed ? credits : Field<Object>(root, 132), Field<Object>(root, 148) };
    }
    // The sequel lays its rows out after the constructor, so its entry follows its neighbours every frame
    inline void Place(Page& page)
    {
        const auto [previous, next] = Neighbours(page.root);
        const auto source = Field<Object>(page.root, 120);
        float x = Field<float>(source, 56);
        float width = Field<float>(source, 64);
        float y = (Field<float>(previous, 60) + Field<float>(next, 60)) * 0.5f;
        if (!Layout->joinedGraphics)
        {
            // Labels start at their row's x; +168 is the measured label width, +184 its scale. The entry's label is
            // centred between the end of the Gameplay label and the start of the Graphics label.
            const auto label = [](Object control) { return Field<float>(control, 168) * Field<float>(control, 184); };
            x = (Field<float>(previous, 56) + label(previous) + Field<float>(next, 56) - label(page.entry)) * 0.5f;
            width = Field<float>(next, 56) - x;
            if (width <= 0.0f)
                return;
        }
        Geometry(page.entry, x, y, width, Field<float>(source, 68));
    }
    inline void Restore(Page& page)
    {
        if (!page.open)
            return;
        SetField<Object>(page.root, 80, page.stockChildren);
        SetField<int>(page.root, 84, page.stockCapacity);
        SetField<int>(page.root, 88, page.stockCount);
        SetField<int>(page.root, 44, page.stockOrientation);
        for (auto& [control, state] : page.stockStates)
            SetField<int>(control, 36, state);
        page.stockStates.clear();
        SetField<int>(page.root, 8, -1);
        page.open = false;
        api.select(page.root, page.stockSelection);
        page.model.Close();
    }
    // Back is the last row where the game's Options has one (the sequel goes back with Escape)
    inline bool BackRow()
    {
        return Layout->back != 0;
    }
    inline std::string Title(const Page& page)
    {
        const auto& pages = page.model.Pages();
        if (page.page == RWGTAMenu::Model::RootPage || page.page >= pages.size())
            return Text("menu.plugins");
        const auto& shown = pages[page.page];
        // The fix's own page is named after the game; its categories keep their titles
        if (shown.plugin && shown.plugin->module == host && shown.parent == RWGTAMenu::Model::RootPage)
            return "The Suffering";
        return shown.title;
    }
    inline std::string Label(const Page& page, const Item& item)
    {
        if (item.back)
            return Text("menu.back");
        if (item.row.kind == RWGTAMenu::RowKind::Plugin && item.row.plugin && item.row.plugin->module == host)
            return "The Suffering";
        auto label = page.model.Label(item.row);
        auto value = page.model.Value(item.row);
        return value.empty() ? label : label + ": " + value;
    }
    // The rows of the page's window in the controls; sound: the move sound even when the selected control stays
    inline void Build(Page& page, bool sound = false)
    {
        page.items.clear();
        for (auto row : page.model.Shown(page.page, BackRow()))
            page.items.push_back(row ? Item{ false, *row } : Item{ true });
        const auto& window = page.model.Window(page.page);
        const auto count = std::min(page.items.size(), page.controls.size());
        for (size_t i = 0; i < page.controls.size(); ++i)
        {
            SetField<int>(page.controls[i], 36, i < count ? 4 : 1);
            if (i < count)
                SetText(page.controls[i], Label(page, page.items[i]));
        }
        SetText(page.title, Title(page));
        // The marks of more rows above and below, in the rows' font, centred over the list
        SetField<int>(page.up, 36, window.Above() ? 4 : 1);
        SetField<int>(page.down, 36, window.Below() ? 4 : 1);
        const auto slot = static_cast<int>(std::min(window.Slot(), count ? count - 1 : 0));
        if (sound || Field<int>(page.root, 8) != slot)
            SetField<int>(page.root, 8, -1);
        api.select(page.root, slot);
    }
    inline void Open(Page& page)
    {
        if (Layout->joinedGraphics)
            page.model.OpenMenu(host, Language(), page.controls.size(), { "MAIN.FOVFactor", "MAIN.CutsceneBorders" });
        else
            page.model.OpenMenu(host, Language(), page.controls.size());
        page.stockChildren = Field<Object>(page.root, 80);
        page.stockCapacity = Field<int>(page.root, 84);
        page.stockCount = Field<int>(page.root, 88);
        page.stockSelection = Field<int>(page.root, 8);
        page.stockOrientation = Field<int>(page.root, 44);
        SetField<int>(page.root, 44, 1); // a vertical list: Up/Down move, Left/Right change values
        auto stock = reinterpret_cast<Object*>(page.stockChildren);
        for (int i = 0; i < page.stockCount; ++i)
        {
            if (stock[i] == page.nativeTitle)
                continue;
            page.stockStates.emplace_back(stock[i], Field<int>(stock[i], 36));
            SetField<int>(stock[i], 36, 1);
        }
        SetField<Object>(page.root, 80, reinterpret_cast<Object>(page.privateChildren.data()));
        SetField<int>(page.root, 84, static_cast<int>(page.privateChildren.size()));
        SetField<int>(page.root, 88, static_cast<int>(page.privateChildren.size()));
        page.open = true;
        page.page = RWGTAMenu::Model::RootPage;
        page.model.Window(page.page).Reset();
        notches.Clear();
        wheel = pageKeys = edgeKeys = 0;
        Build(page);
    }
    // Esc, B or the Back row: the list it came from, on the row it came from; the game's Options from the plugins list
    inline void Back(Page& page)
    {
        const auto parent = page.model.Back(page.page);
        if (parent == SIZE_MAX)
        {
            Restore(page);
            return;
        }
        page.page = parent;
        Build(page, true);
    }
    inline void Act(Page& page, int index, int direction)
    {
        if (!page.open)
        {
            if (!page.video)
                Open(page);
            else if (index >= 0 && static_cast<size_t>(index) < page.joined.size())
            {
                page.model.Change(page.joined[index].second, direction, true);
                auto& row = page.joined[index];
                SetText(row.first, page.model.Label(row.second) + ": " + page.model.Value(row.second));
            }
            return;
        }
        if (index < 0 || static_cast<size_t>(index) >= page.items.size())
            return;
        auto item = page.items[index];
        page.model.Window(page.page).Point(index);
        if (item.back)
        {
            if (!direction)
                Back(page);
            return;
        }
        if (RWGTAMenu::Model::Opens(item.row))
        {
            if (direction)
                return;
            const auto target = page.model.Enter(item.row);
            if (target == SIZE_MAX)
                return;
            page.page = target;
            Build(page, true);
            return;
        }
        page.model.Change(item.row, direction, true);
        Build(page);
    }
    // Up/Down: one row, the window following by one row; the game's own wrap
    inline void Move(Page& page, int direction)
    {
        auto& window = page.model.Window(page.page);
        const auto first = window.First();
        if (window.Move(direction, (Field<int>(page.root, 4) & 2) != 0))
            Build(page, window.First() != first);
    }
    // The input the game's menu has no use for: wheel, page keys, shoulder buttons
    inline void ListInput(Page& page)
    {
        auto& window = page.model.Window(page.page);
        // The selection follows a row the mouse picked
        const int selected = Field<int>(page.root, 8);
        if (selected >= 0 && static_cast<size_t>(selected) < page.items.size() && static_cast<size_t>(selected) != window.Slot())
            window.Point(selected);
        auto scrolled = [&](bool changed)
        {
            if (changed)
                Build(page, true);
        };
        const int notchesNow = wheel.exchange(0);
        for (int i = 0; i < std::abs(notchesNow); ++i)
            scrolled(window.Scroll(notchesNow > 0 ? -1 : 1));
        const int pagesNow = pageKeys.exchange(0);
        for (int i = 0; i < std::abs(pagesNow); ++i)
            scrolled(window.Page(pagesNow > 0 ? 1 : -1));
        if (const int edge = edgeKeys.exchange(0))
            scrolled(window.Edge(edge));
        XINPUT_STATE pad{};
        DWORD slot{};
        DWORD process{};
        GetWindowThreadProcessId(GetForegroundWindow(), &process);
        const bool focused = process == GetCurrentProcessId();
        const bool read = focused && PadInput::Read(pad, slot);
        const auto now = GetTickCount64();
        for (int i = 0; i < 2; ++i)
        {
            const WORD button = i ? XINPUT_GAMEPAD_RIGHT_SHOULDER : XINPUT_GAMEPAD_LEFT_SHOULDER;
            if (page.shoulders[i].Step(read && (pad.Gamepad.wButtons & button), now))
                scrolled(window.Page(i ? 1 : -1));
        }
        // A held Up/Down (keys, d-pad, stick) repeats; the game acts on the press itself only
        const int axis = read ? page.stick.Direction(pad.Gamepad.sThumbLY) : 0;
        for (int i = 0; i < 2; ++i)
        {
            // The menu stack's input: Up or Down waiting while its key or button is held
            const bool waiting = api.pending(Field<Object>(page.root, 100) + 108, Layout->left - 2 + i, 1) > 0;
            const bool down = waiting || (pad.Gamepad.wButtons & (i ? XINPUT_GAMEPAD_DPAD_DOWN : XINPUT_GAMEPAD_DPAD_UP)) ||
                axis == (i ? -1 : 1);
            if (page.held[i].Step(focused && down, now, false))
                Move(page, i ? 1 : -1);
        }
    }
    inline int __fastcall Notify(Listener* listener, void*, Object control)
    {
        auto& page = *listener->page;
        if (page.open)
        {
            auto found = std::find(page.controls.begin(), page.controls.end(), control);
            if (found != page.controls.end())
                page.command = static_cast<int>(found - page.controls.begin());
            else if (control == page.up || control == page.down)
            {
                // A click on a mark scrolls the list by a row
                auto& window = page.model.Window(page.page);
                if (window.Scroll(control == page.up ? -1 : 1))
                    Build(page, true);
                return 0;
            }
        }
        else if (control == page.entry)
            page.command = 0;
        else
        {
            for (size_t i = 0; i < page.joined.size(); ++i)
                if (page.joined[i].first == control)
                    page.command = static_cast<int>(i);
        }
        page.direction = 0;
        return 0;
    }
    inline void* listenerTable[]{ nullptr, reinterpret_cast<void*>(Notify) };
    inline Object __fastcall Destroy(Object self, void*, int flags)
    {
        auto found = pages.find(self);
        if (found == pages.end())
            return 0;
        auto page = std::move(found->second);
        pages.erase(found);
        Restore(*page);
        SetField<uintptr_t*>(self, 0, page->originalTable);
        for (auto control : page->privateChildren)
            reinterpret_cast<Object(__thiscall*)(Object, int)>(Field<uintptr_t*>(control, 0)[0])(control, 1);
        return reinterpret_cast<Object(__thiscall*)(Object, int)>(page->originalTable[0])(self, flags);
    }
    inline int __fastcall Input(Object self, void*, int message)
    {
        auto& page = *pages.at(self);
        int index = Field<int>(self, 8);
        auto children = Field<Object*>(self, 80);
        Object selected = index >= 0 && index < Field<int>(self, 88) ? children[index] : 0;
        if (page.open)
        {
            if (Layout->back && message == Layout->back)
            {
                page.command = -2;
                return 1;
            }
            if (message == Layout->left - 2 || message == Layout->left - 1)
            {
                Move(page, message == Layout->left - 2 ? -1 : 1);
                return 1;
            }
            if ((message == Layout->left || message == Layout->right) && index >= 0 && static_cast<size_t>(index) < page.items.size()
                && !page.items[index].back)
            {
                page.command = index;
                page.direction = message == Layout->left ? -1 : 1;
                return 1;
            }
        }
        else if (page.video)
        {
            for (size_t i = 0; i < page.joined.size(); ++i)
            {
                if (selected == page.joined[i].first && (message == Layout->accept || message == Layout->left || message == Layout->right))
                {
                    page.command = static_cast<int>(i);
                    page.direction = message == Layout->left ? -1 : message == Layout->right ? 1 : 0;
                    return 1;
                }
            }
        }
        return reinterpret_cast<int(__thiscall*)(Object, int)>(page.originalTable[5])(self, message);
    }
    inline int __fastcall Update(Object self, void*)
    {
        auto& page = *pages.at(self);
        if (page.open || page.escapeHeld)
        {
            // Escape on the open fix page is its Back; the stack keeps the page until the key is let go
            const bool pressed = api.pending(Field<Object>(self, 100) + 108, Layout->escape, 1) > 0;
            if (pressed && page.open && !page.escapeHeld)
                page.command = -2;
            page.escapeHeld = pressed;
        }
        int result = reinterpret_cast<int(__thiscall*)(Object)>(page.originalTable[Layout->updateSlot])(self);
        if (page.command == -2)
        {
            page.command = -1;
            Back(page);
        }
        if (page.command >= 0)
        {
            int command = std::exchange(page.command, -1);
            Act(page, command, page.direction);
        }
        if (page.video)
        {
            auto index = Field<int>(self, 8);
            auto children = Field<Object*>(self, 80);
            for (auto& [control, row] : page.joined)
                if (Field<int>(control, 36) == 5)
                    for (int i = 0; i < Field<int>(self, 88); ++i)
                        if (children[i] == control && index != i)
                        {
                            index = i;
                            SetField<int>(self, 8, i);
                        }
            auto selected = index >= 0 && index < Field<int>(self, 88) ? children[index] : 0;
            std::string help;
            for (auto& [control, row] : page.joined)
            {
                auto label = FontText(page.model.Label(row) + ": " + page.model.Value(row));
                auto text = Field<const wchar_t*>(control, 108);
                if (!text || label != text)
                    api.setText(control, label.c_str());
                if (selected == control)
                    help = Help(page.model, row);
            }
            if (!help.empty())
                for (size_t offset = 192; offset <= 216; offset += 4)
                    if (auto control = Field<Object>(self, offset))
                        SetField<int>(control, 36, 1);
            SetField<int>(page.help, 36, help.empty() ? 1 : 3);
            if (help != page.shownHelp)
            {
                SetText(page.help, help, false);
                page.shownHelp = std::move(help);
            }
        }
        if (!page.open && page.entry && !Layout->joinedGraphics)
            Place(page);
        if (page.open)
        {
            if (RWGTAMenu::Model::Changed().exchange(false))
                Build(page);
            ListInput(page);
            int selected = Field<int>(self, 8);
            std::string help;
            if (selected >= 0 && static_cast<size_t>(selected) < page.items.size())
            {
                auto& item = page.items[selected];
                if (!item.back && !RWGTAMenu::Model::Opens(item.row))
                    help = Help(page.model, item.row);
            }
            if (page.model.RestartRequired())
                help = Text("menu.restartwarning");
            else if (help.empty())
                help = notice;
            if (help.size() > 100)
            {
                auto end = help.rfind(' ', 97);
                help.resize(end == std::string::npos ? 97 : end);
                help += "...";
            }
            if (help != page.shownHelp)
            {
                SetText(page.help, help, false);
                page.shownHelp = std::move(help);
            }
        }
        return result;
    }
    inline int __fastcall CanEscape(Object self, void*)
    {
        auto& page = *pages.at(self);
        if (page.open || page.escapeHeld)
            return 0;
        return reinterpret_cast<int(__thiscall*)(Object)>(page.originalTable[Layout->escapeSlot])(self);
    }
    inline void Attach(Object root, bool video)
    {
        if (pages.count(root))
            return;
        auto page = std::make_unique<Page>();
        page->root = root;
        page->video = video;
        page->listener = { listenerTable, page.get() };
        auto source = Field<Object>(root, 120);
        if (!source)
            return;
        if (video)
        {
            page->model.OpenMenu(host, Language(), 5, { "MAIN.FOVFactor", "MAIN.CutsceneBorders" });
            auto rows = page->model.JoinedRows();
            float x = Field<float>(root, 56) + Field<float>(root, 64) * 0.12f;
            float width = Field<float>(root, 64) * 0.76f;
            float height = Field<float>(source, 68);
            float top = Field<float>(source, 60);
            auto children = Field<Object*>(root, 80);
            for (int i = 0; i < Field<int>(root, 88); ++i)
            {
                float y = Field<float>(children[i], 60);
                if (y >= top && y < Field<float>(root, 68) * 0.72f)
                    SetField<float>(children[i], 60, top + (y - top) * 0.8f);
            }
            float y = top + height * (8.0f / 9.0f) * 0.8f * 6.0f;
            for (auto& row : rows)
            {
                auto control = Create(*page, source, true);
                if (!control)
                    continue;
                Geometry(control, x, y, width, height);
                SetText(control, page->model.Label(row) + ": " + page->model.Value(row));
                api.addChild(root, control);
                children = Field<Object*>(root, 80);
                auto accept = Field<Object>(root, 220);
                for (int i = 0; i < Field<int>(root, 88) - 1; ++i)
                    if (children[i] == accept)
                    {
                        for (int j = Field<int>(root, 88) - 1; j > i; --j)
                            children[j] = children[j - 1];
                        children[i] = control;
                        break;
                    }
                page->joined.emplace_back(control, row);
                y += height * (8.0f / 9.0f) * 0.8f;
            }
            auto sourceHelp = Field<Object>(root, 216);
            page->help = Create(*page, sourceHelp ? sourceHelp : source, false);
            if (!page->help)
                return;
            Geometry(page->help, Field<float>(root, 56) + Field<float>(root, 64) * 0.1f,
                Field<float>(root, 60) + Field<float>(root, 68) * 0.85f,
                Field<float>(root, 64) * 0.8f, Field<float>(root, 68) * 0.18f);
            SetField<int>(page->help, 36, 1);
            api.addChild(root, page->help);
        }
        else
        {
            auto children = Field<Object*>(root, 80);
            const auto count = Field<int>(root, 88);
            if (!children || count <= 0)
                return;
            const auto end = children + count;
            const auto [previous, next] = Neighbours(root);
            const auto insertion = std::find(children, end, next);
            if (!previous || !next || insertion == end || std::find(children, end, previous) == end)
                return;
            const auto entryIndex = insertion - children;
            page->entry = Create(*page, source, true);
            if (!page->entry)
                return;
            Place(*page);
            SetText(page->entry, Text("menu.plugins"));
            for (int i = 0; i < Layout->rowsPerColumn * (Layout->columnStep > 0.0f ? 2 : 1); ++i)
            {
                auto control = Create(*page, source, true);
                if (!control)
                    return;
                const auto column = i / Layout->rowsPerColumn, row = i % Layout->rowsPerColumn;
                Geometry(control, Field<float>(root, 56) + Field<float>(root, 64) * (Layout->rows.x + column * Layout->columnStep),
                    Field<float>(root, 60) + Field<float>(root, 68) * (Layout->rows.y + row * Layout->rowStep),
                    Field<float>(root, 64) * Layout->rows.width, Field<float>(source, 68));
                page->controls.push_back(control);
                page->privateChildren.push_back(control);
            }
            page->help = Create(*page, source, false, Layout->helpTemplate);
            // The page's title is a picture of its name ("Options Menu", its first control; the game has no font for
            // it): it stays over the Plugins pages, which are part of Options, and their names go under it. The
            // sequel's title screen has none.
            page->nativeTitle = Layout->joinedGraphics && Field<int>(root, 88) > 0 ? Field<Object*>(root, 80)[0] : 0;
            page->title = Create(*page, source, false);
            page->up = Create(*page, source, true);
            page->down = Create(*page, source, true);
            if (!page->help || !page->title || !page->up || !page->down)
                return;
            // The marks: an ellipsis (the font has no arrows) above the first and below the last row, clickable, not
            // reached by the keys
            const auto& first = Layout->rows;
            const auto lastRow = (Layout->rowsPerColumn - 1) * Layout->rowStep;
            for (auto [mark, y] : { std::pair{ page->up, first.y - Layout->rowStep * 0.55f }, std::pair{ page->down, first.y + lastRow + Layout->rowStep * 0.55f } })
            {
                SetText(mark, "...", false);
                Geometry(mark, Field<float>(root, 56) + Field<float>(root, 64) * (first.x + first.width * 0.45f),
                    Field<float>(root, 60) + Field<float>(root, 68) * y, Field<float>(root, 64) * first.width * 0.1f, Field<float>(source, 68) * 0.6f);
                SetField<int>(mark, 36, 1);
            }
            SetText(page->help, "", false);
            Geometry(page->help, Field<float>(root, 56) + Field<float>(root, 64) * Layout->help.x,
                Field<float>(root, 60) + Field<float>(root, 68) * Layout->help.y,
                Field<float>(root, 64) * Layout->help.width, Field<float>(root, 68) * (Layout->helpTemplate >= 0 ? 0.18f : 0.12f));
            Geometry(page->title, Field<float>(root, 56) + Field<float>(root, 64) * Layout->title.x,
                Field<float>(root, 60) + Field<float>(root, 68) * Layout->title.y,
                Field<float>(root, 64) * Layout->title.width, Field<float>(source, 68));
            if (page->nativeTitle)
                page->privateChildren.push_back(page->nativeTitle);
            page->privateChildren.push_back(page->help);
            page->privateChildren.push_back(page->title);
            page->privateChildren.push_back(page->up);
            page->privateChildren.push_back(page->down);

            api.addChild(root, page->entry);
            children = Field<Object*>(root, 80);
            for (auto i = Field<int>(root, 88) - 1; i > entryIndex; --i)
                children[i] = children[i - 1];
            children[entryIndex] = page->entry;
        }
        page->originalTable = Field<uintptr_t*>(root, 0);
        std::copy_n(page->originalTable, Layout->tableSize, page->table.begin());
        page->table[0] = reinterpret_cast<uintptr_t>(Destroy);
        page->table[5] = reinterpret_cast<uintptr_t>(Input);
        page->table[Layout->updateSlot] = reinterpret_cast<uintptr_t>(Update);
        page->table[Layout->escapeSlot] = reinterpret_cast<uintptr_t>(CanEscape);
        SetField<uintptr_t*>(root, 0, page->table.data());
        pages.emplace(root, std::move(page));
    }
    // Every frame: while a menu page (title, pause, options...) is the menu stack's top page, which takes the input
    // (flag 1; the HUD's pages under it don't), no drops are drawn over it
    inline void Frame()
    {
        const auto stack = menuStack ? *menuStack : 0;
        const int count = stack ? Field<int>(stack, 88) : 0;
        const auto top = count > 0 ? Field<Object*>(stack, 80)[count - 1] : 0;
        RainDroplets::MenuOpen(top && (Field<int>(top, 4) & 1));
    }
    inline Object __fastcall Options(Object self, void*, int object)
    {
        auto result = optionsHook.thiscall<Object>(self, object);
        Attach(self, false);
        return result;
    }
    inline Object __fastcall Video(Object self, void*, int object)
    {
        auto result = videoHook.thiscall<Object>(self, object);
        Attach(self, true);
        return result;
    }
    inline int __fastcall Defaults(Object self, void*)
    {
        RWGTAMenu::RestoreDefaults(FixSettings(), { "MAIN.FOVFactor", "MAIN.CutsceneBorders" });
        return defaultsHook.thiscall<int>(self);
    }
    inline void Init(const wchar_t* moduleName, const MenuLayout& layout)
    {
        auto module = GetModuleHandleW(moduleName);
        if (!module)
            return;
        Layout = &layout;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<const wchar_t*>(Init), &host);
        auto language = find_pattern(GetModuleHandleW(nullptr), "8D 44 24 10 50 6A 23 55 89 15 ? ? ? ? FF D6 8A 54 24 0C 33 C9 80 FA 30");
        languageCode.SetAddress(*reinterpret_cast<const uint32_t**>(static_cast<uint8_t*>(language) + 10));
        api.allocate = reinterpret_cast<decltype(api.allocate)>(find_pattern(module, "81 EC 80 00 00 00 53 8B 1D ? ? ? ? 55 56 57 8B BC 24 98 00 00 00 33 ED"));
        api.button = reinterpret_cast<decltype(api.button)>(find_pattern(module, "8B 44 24 04 56 50 8B F1 E8 ? ? ? ? C7 06 ? ? ? ? C7 86 C0 00 00 00 00 00 00 00 8B C6"));
        api.text = reinterpret_cast<decltype(api.text)>(find_pattern(module, Layout->joinedGraphics
            ? "6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 51 8B 44 24 14 53 56 8B F1 50 89 74 24 0C"
            : "8B 44 24 04 53 56 50 8B F1 E8 ? ? ? ? 33 DB C7 06 ? ? ? ? 89 5E 70 89 5E 74 89 5E 6C 89 5E 7C"));
        api.setText = reinterpret_cast<decltype(api.setText)>(find_pattern(module, "53 55 8B 6C 24 0C 56 57 55 8B D9 FF 15 ? ? ? ? 8B F0 8B 43 6C 83 C4 04"));
        api.addChild = reinterpret_cast<decltype(api.addChild)>(find_pattern(module, "56 57 8B 7C 24 0C 8D 71 50 89 4F 64 8B 46 08 8B 4E 04 40 3B C8"));
        api.select = reinterpret_cast<decltype(api.select)>(find_pattern(module, "53 56 8B F1 8B 46 58 57 8B 7C 24 10 48 3B F8"));
        api.pending = reinterpret_cast<decltype(api.pending)>(find_pattern(module, Layout->joinedGraphics
            ? "56 8B 74 24 0C 56 8B 74 24 0C 8B C1 8B 0D ? ? ? ? 8B 11 56 50 FF 92 AC 00 00 00 5E C2 08 00"
            : "56 8B 74 24 0C 56 8B 74 24 0C 8B C1 8B 0D ? ? ? ? 8B 11 56 50 FF 92 B0 00 00 00 5E C2 08 00"));
        auto options = find_pattern(module, Layout->joinedGraphics
            ? "6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 51 8B 44 24 14 53 56 57 8B F1 50 89 74 24 10 E8 ? ? ? ? C7 46 6C ? ? ? ? 8B 4E 0C 33 DB 3B CB"
            : "8B 44 24 04 55 56 50 8B F1 E8 ? ? ? ? C7 46 6C ? ? ? ? 8B 46 0C 85 C0 8D 6E 6C C7 06 ? ? ? ? C7 45 00");
        HMODULE pinned{};
        if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
            reinterpret_cast<const wchar_t*>(module), &pinned))
            return;
        optionsHook = safetyhook::create_inline(options, Options);
        // The game's mouse poll (both exes)
        auto mouse = hook::pattern("83 EC 08 53 56 8B F1 E8 ? ? ? ? 8B D8 83 FB 02 74 08 5E 5B 83 C4 08 C2 08 00 8B 46 0C 8B 08 57 8D 7E 10 57 6A 14 50 FF 51 24").get_first();
        mouseHook = safetyhook::create_inline(mouse, PollMouse);
        // The menu stack is made at start and kept in a global
        auto stack = static_cast<uint8_t*>(find_pattern(module, Layout->joinedGraphics
            ? "E8 ? ? ? ? EB 02 33 C0 C7 84 24 ? ? 00 00 FF FF FF FF A3"
            : "68 DC 00 00 00 6A 00 E8 ? ? ? ? 83 C4 08 85 C0 74 EC 8B C8 E8 ? ? ? ? A3"));
        menuStack = *reinterpret_cast<Object**>(stack + (Layout->joinedGraphics ? 21 : 27));
        if (!Layout->joinedGraphics)
            return;
        auto video = find_pattern(module, "6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 51 8B 44 24 14 53 56 57 8B F1 50 89 74 24 10 E8 ? ? ? ? C7 46 6C ? ? ? ? 33 DB C7 06");
        auto defaults = find_pattern(module, "B8 01 00 00 00 C7 81 24 01 00 00 20 03 00 00 C7 81 28 01 00 00 58 02 00 00");
        videoHook = safetyhook::create_inline(video, Video);
        defaultsHook = safetyhook::create_inline(defaults, Defaults);
    }
}
