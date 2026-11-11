module;

#include "stdafx.h"
#include "FixSettings.hpp"
#include "Shared/RWGTA/FixMenu.hpp"
#include <array>
#include <map>
#include <memory>

// The settings in the game's own journal: a "Plugins" row on the Options page opens a journal page of native text
// rows. The first page lists the plugins (the fix first); a plugin's page lists its rows. Clicking a row (or Enter,
// or the pad's accept button on it) moves to its next value; Left/Right step either way while a row is under the
// cursor. The preset row chooses a preset and the row after it applies it (twice, to confirm).
export module NativeMenu;

import ComVars;


export namespace PsychonautsMenu
{
    using Object = uint8_t*;

    template<class T> T& Field(Object object, size_t offset)
    {
        return *reinterpret_cast<T*>(object + offset);
    }

    template<class R = void, class... Args> R Virtual(Object object, size_t slot, Args... args)
    {
        auto function = reinterpret_cast<R(__thiscall*)(Object, Args...)>((*reinterpret_cast<void***>(object))[slot]);
        return function(object, args...);
    }

    // Widget methods by their slot
    constexpr size_t SlotActivated = 6, SlotPosition = 11, SlotSize = 12, SlotVisible = 18, SlotText = 19, SlotShow = 19, SlotClose = 20;
    constexpr size_t TableSize = 21;
    constexpr float PageWidth = 540.0f, PageHeight = 480.0f;
    constexpr size_t WindowRows = 11;

    struct Api
    {
        void*(__cdecl* allocate)(size_t, size_t){};
        Object(__thiscall* text)(Object, int){};
        Object(__thiscall* container)(Object){};
        int(__thiscall* addChild)(Object, Object){};
        int(__thiscall* layout)(Object){};
        int(__thiscall* show)(Object){};
        char(__cdecl* pressed)(int){};
        void** containerTable{};
    };
    inline Api api;
    inline SafetyHookInline optionsHook, activatedHook, lookupHook;
    inline std::array<void*, TableSize> pageTable{};
    inline HMODULE host{};
    inline std::string language = "en";
    inline std::string SettingsLanguage()
    {
        const auto selected = FixSettings().String("MAIN.TextLanguage");
        return selected == "auto" ? language : selected;
    }
    inline std::map<std::string, std::string> strings;     // key (upper case, no slashes) -> game text
    inline std::map<Object, std::string> keys;              // text widget -> its "/KEY/"
    inline std::map<Object, float> widths;                  // rows that take the whole line under the cursor
    inline std::string notice;                              // an update notice, shown while no row is under the cursor

    struct Line
    {
        enum class Kind { Plugin, Row, Apply, Back } kind{};
        size_t index{};
        RWGTAMenu::Row row{};
    };

    struct Menu
    {
        Object options{}, plugins{}, page{};
        Object title{}, help{}, up{}, down{};
        std::vector<Object> rows;
        std::vector<Line> lines;
        RWGTAMenu::Model model;
        int plugin{ -1 };
        std::string shownHelp;
    };
    inline std::map<Object, std::unique_ptr<Menu>> menus;  // by Options widget
    inline std::map<Object, Menu*> pages;                   // by page widget

    // UTF-8 to the game's fonts (Windows-1252)
    inline std::string Encode(const std::string& text)
    {
        const int wide = MultiByteToWideChar(CP_UTF8, 0, text.data(), int(text.size()), nullptr, 0);
        std::wstring buffer(wide, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, text.data(), int(text.size()), buffer.data(), wide);
        const int size = WideCharToMultiByte(1252, 0, buffer.data(), wide, nullptr, 0, "?", nullptr);
        std::string result(size, '\0');
        WideCharToMultiByte(1252, 0, buffer.data(), wide, result.data(), size, "?", nullptr);
        return result;
    }

    inline std::string Text(const std::string& key)
    {
        return RWGTAMenu::PluginText(FixSettings(), SettingsLanguage(), key);
    }

    inline float Width(Object widget)
    {
        return std::fabs(Field<float>(widget, 20) - Field<float>(widget, 12));
    }

    inline void Place(Object widget, float x, float y)
    {
        float position[2]{ x, y };
        Virtual(widget, SlotPosition, position);
    }

    inline void SetText(Object widget, const std::string& text, bool center = false)
    {
        auto key = keys[widget];
        strings[key.substr(1, key.size() - 2)] = Encode(text);
        api.layout(widget);
        if (auto wide = widths.find(widget); wide != widths.end())
        {
            float size[2]{ wide->second, std::fabs(Field<float>(widget, 24) - Field<float>(widget, 16)) };
            Virtual(widget, SlotSize, size);
        }
        if (center)
            Place(widget, (PageWidth - Width(widget)) * 0.5f, Field<float>(widget, 16));
    }

    inline void SetVisible(Object widget, bool visible)
    {
        Virtual(widget, SlotVisible, char(visible));
    }

    inline Object NewText(Object parent, float x, float y, float scale, bool selectable)
    {
        auto widget = static_cast<Object>(api.allocate(0x98, 0x10));
        if (!widget)
            return nullptr;
        api.text(widget, 0);
        auto& key = keys[widget];
        key = "/WSFIX" + std::to_string(keys.size()) + "/";
        strings[key.substr(1, key.size() - 2)] = {};
        Virtual(widget, SlotText, key.c_str());
        Field<float>(widget, 76) = scale;
        Field<float>(widget, 80) = scale;
        Field<uint8_t>(widget, 148) = 0;
        if (!selectable)
            std::memcpy(widget + 60, widget + 44, 16);
        api.layout(widget);
        Place(widget, x, y);
        api.addChild(parent, widget);
        return widget;
    }

    inline std::string Help(const Menu& menu, const Line& line)
    {
        switch (line.kind)
        {
            case Line::Kind::Row:
            {
                auto& row = line.row;
                if (row.plugin && row.plugin->module == host && row.setting && row.setting->id.starts_with("MAIN."))
                {
                    auto help = OtherPlugins::Text(*row.plugin, "@menu." + row.setting->id.substr(5) + ".help");
                    if (!help.empty() && !help.starts_with("menu."))
                        return help;
                }
                return menu.model.Help(row);
            }
            case Line::Kind::Apply:
                return menu.model.Help({ RWGTAMenu::RowKind::ApplyPreset, line.row.plugin });
            default:
                return {};
        }
    }

    // Splits a help text into lines that fit the page
    inline std::string Wrap(std::string text, size_t width, size_t lines)
    {
        std::string result;
        for (size_t line = 0; line < lines && !text.empty(); ++line)
        {
            if (line)
                result += '\n';
            if (text.size() <= width)
            {
                result += text;
                text.clear();
                break;
            }
            auto end = text.rfind(' ', width);
            if (end == std::string::npos || end == 0)
                end = width;
            if (line + 1 == lines)
            {
                end = text.rfind(' ', width - 3);
                result += text.substr(0, end == std::string::npos ? width - 3 : end) + "...";
                break;
            }
            result += text.substr(0, end);
            text.erase(0, std::min(text.size(), end + 1));
        }
        return result;
    }

    inline void Build(Menu& menu)
    {
        menu.lines.clear();
        auto& plugins = menu.model.Pages();
        if (menu.plugin < 0 || size_t(menu.plugin) >= plugins.size())
        {
            menu.plugin = -1;
            for (size_t i = 0; i < plugins.size() && menu.lines.size() + 1 < menu.rows.size(); ++i)
                menu.lines.push_back({ Line::Kind::Plugin, i });
        }
        else
        {
            for (auto row : menu.model.WindowRows(size_t(menu.plugin)))
            {
                if (!row)
                    break;
                menu.lines.push_back({ Line::Kind::Row, 0, *row });
                if (row->kind == RWGTAMenu::RowKind::Preset)
                    menu.lines.push_back({ Line::Kind::Apply, 0, { RWGTAMenu::RowKind::ApplyPreset, row->plugin } });
            }
            while (menu.lines.size() + 1 > menu.rows.size())
                menu.lines.pop_back();
        }
        menu.lines.push_back({ Line::Kind::Back });

        for (size_t i = 0; i < menu.rows.size(); ++i)
        {
            const bool shown = i < menu.lines.size();
            SetVisible(menu.rows[i], shown);
            if (!shown)
                continue;
            auto& line = menu.lines[i];
            std::string text;
            switch (line.kind)
            {
                case Line::Kind::Plugin:
                    text = plugins[line.index].title;
                    break;
                case Line::Kind::Row:
                case Line::Kind::Apply:
                {
                    auto row = line.kind == Line::Kind::Row ? line.row : RWGTAMenu::Row{ RWGTAMenu::RowKind::ApplyPreset, line.row.plugin };
                    text = line.kind == Line::Kind::Row ? menu.model.Label(row) : Text("menu.applypreset");
                    if (text.starts_with("menu."))
                        text = "Apply";
                    auto value = menu.model.Value(row);
                    if (!value.empty())
                        text += ": " + value;
                    break;
                }
                case Line::Kind::Back:
                    text = Text("menu.back");
                    break;
            }
            SetText(menu.rows[i], text);
        }
        const bool page = menu.plugin >= 0;
        SetVisible(menu.up, page && menu.model.HiddenAbove(size_t(menu.plugin)));
        SetVisible(menu.down, page && menu.model.HiddenBelow(size_t(menu.plugin)));
        SetText(menu.title, page ? plugins[size_t(menu.plugin)].title : Text("menu.plugins"), true);
        menu.shownHelp = "\x01";
    }

    inline void Leave(Menu& menu)
    {
        menu.model.Close();
        menu.plugin = -1;
        Virtual(menu.page, SlotClose, char(1));
    }

    inline void Act(Menu& menu, Object child, int direction)
    {
        if (child == menu.up || child == menu.down)
        {
            if (menu.plugin >= 0)
                menu.model.Scroll(size_t(menu.plugin), child == menu.up ? -1 : 1);
            Build(menu);
            return;
        }
        auto found = std::find(menu.rows.begin(), menu.rows.end(), child);
        if (found == menu.rows.end())
            return;
        const auto index = size_t(found - menu.rows.begin());
        if (index >= menu.lines.size())
            return;
        auto line = menu.lines[index];
        switch (line.kind)
        {
            case Line::Kind::Plugin:
                if (direction)
                    return;
                menu.plugin = int(line.index);
                menu.model.PageShown(line.index);
                break;
            case Line::Kind::Row:
                // A click moves forward; on the preset row it only chooses
                menu.model.Change(line.row, direction ? direction : 1, true);
                break;
            case Line::Kind::Apply:
                menu.model.Change({ RWGTAMenu::RowKind::Preset, line.row.plugin }, direction, true);
                break;
            case Line::Kind::Back:
                if (direction)
                    return;
                if (menu.plugin < 0)
                {
                    Leave(menu);
                    return;
                }
                menu.plugin = -1;
                break;
        }
        Build(menu);
    }

    inline int __fastcall PageActivated(Object self, void*, Object child)
    {
        if (auto found = pages.find(self); found != pages.end())
            Act(*found->second, child, 0);
        return 0;
    }

    inline int __fastcall PageShow(Object self, void*)
    {
        if (auto found = pages.find(self); found != pages.end())
        {
            auto& menu = *found->second;
            menu.model.OpenPlugins(host, SettingsLanguage(), WindowRows - 2, {}, true);
            menu.plugin = -1;
            Build(menu);
        }
        return api.show(self);
    }

    inline void CreatePage(Menu& menu)
    {
        auto page = static_cast<Object>(api.allocate(0xC4, 0x10));
        if (!page)
            return;
        api.container(page);
        Field<void*>(page, 0) = pageTable.data();
        float size[2]{ PageWidth, PageHeight };
        Virtual(page, SlotSize, size);
        Place(page, (640.0f - PageWidth) * 0.5f, (480.0f - PageHeight) * 0.5f);
        SetVisible(page, false);
        Field<Object>(page, 124) = menu.options;
        menu.page = page;
        pages[page] = &menu;

        menu.title = NewText(page, 0.0f, 62.0f, 0.75f, false);
        for (size_t i = 0; i < WindowRows; ++i)
        {
            menu.rows.push_back(NewText(page, 70.0f, 100.0f + float(i) * 24.0f, 0.5f, true));
            widths[menu.rows.back()] = 340.0f;
        }
        menu.up = NewText(page, 415.0f, 100.0f, 0.5f, true);
        menu.down = NewText(page, 415.0f, 100.0f + float(WindowRows - 1) * 24.0f, 0.5f, true);
        SetText(menu.up, Text("menu.previous"));
        SetText(menu.down, Text("menu.next"));
        menu.help = NewText(page, 70.0f, 372.0f, 0.42f, false);
    }

    // Under the cursor: a row's help; else the restart warning or the update notice
    inline void Frame()
    {
        if (!currentMenu.is_initialized())
            return;
        auto found = pages.find(currentMenu.get());
        if (found == pages.end())
            return;
        auto& menu = *found->second;
        if (RWGTAMenu::Model::Changed().exchange(false))
            Build(menu);
        std::string help;
        for (size_t i = 0; i < menu.rows.size() && i < menu.lines.size(); ++i)
        {
            if (!(Field<uint32_t>(menu.rows[i], 8) & 2))
                continue;
            help = Help(menu, menu.lines[i]);
            // Left and Right on the row under the cursor
            for (auto [key, direction] : { std::pair{ 203, -1 }, std::pair{ 205, 1 } })
            {
                if (api.pressed(key))
                {
                    Act(menu, menu.rows[i], direction);
                    help = Help(menu, menu.lines[i]);
                }
            }
            break;
        }
        if (help.empty())
            help = menu.model.RestartRequired() ? Text("menu.restartwarning") : notice;
        if (help != menu.shownHelp)
        {
            SetText(menu.help, Wrap(help, 78, 3));
            menu.shownHelp = std::move(help);
        }
    }

    // The Options page: room for one more row between Sound and Back
    inline Object __fastcall OptionsCreated(Object self, void*, int owner)
    {
        auto result = optionsHook.thiscall<Object>(self, owner);
        auto menu = std::make_unique<Menu>();
        menu->options = self;
        for (auto node = Field<Object>(self, 112); node; node = Field<Object>(node, 0))
        {
            auto child = Field<Object>(node, 8);
            const float x = Field<float>(child, 12), y = Field<float>(child, 16);
            const float shift = y < 60.0f ? 0.0f : y < 150.0f ? -20.0f : y < 230.0f ? -35.0f : y < 300.0f ? -50.0f : 0.0f;
            if (shift)
                Place(child, x, y + shift);
        }
        menu->plugins = NewText(self, 211.0f, 274.0f, 0.75f, true);
        if (menu->plugins)
            SetText(menu->plugins, Text("menu.plugins"));
        for (auto it = pages.begin(); it != pages.end();)
            it = it->second->options == self ? pages.erase(it) : std::next(it);
        menus[self] = std::move(menu);
        return result;
    }

    inline int __fastcall OptionsActivated(Object self, void*, Object child)
    {
        if (auto found = menus.find(self); found != menus.end() && child && child == found->second->plugins)
        {
            auto& menu = *found->second;
            if (!menu.page)
                CreatePage(menu);
            if (menu.page)
            {
                Field<Object>(self, 140) = menu.page;
                Virtual(self, SlotClose, char(0));
            }
            return 0;
        }
        return activatedHook.thiscall<int>(self, child);
    }

    inline const char* __fastcall Lookup(Object self, void*, const char* key)
    {
        static const bool known = [self]()
        {
            const auto name = reinterpret_cast<const char*>(self + 84);
            for (auto [game, tag] : { std::pair{ "French", "fr" }, std::pair{ "German", "de" }, std::pair{ "Italian", "it" },
                std::pair{ "Spanish", "es" } })
            {
                if (!_strnicmp(name, game, std::strlen(game) + 1))
                    language = tag;
            }
            return true;
        }();
        (void)known;
        if (key && !std::strncmp(key, "WSFIX", 5))
        {
            if (auto found = strings.find(key); found != strings.end())
                return found->second.c_str();
            return "";
        }
        return lookupHook.thiscall<const char*>(self, key);
    }

    inline void* find(const char* bytes)
    {
        return hook::pattern(bytes).get_first();
    }

    inline void Init()
    {
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&Init), &host);
        const auto runtime = GetModuleHandleW(L"MSVCR90.dll");
        api.allocate = reinterpret_cast<decltype(api.allocate)>(GetProcAddress(runtime, "_aligned_malloc"));
        auto options = find("55 8B EC 81 EC D0 00 00 00 89 8D 54 FF FF FF");
        auto activated = find("55 8B EC 51 89 4D FC 8B 45 08 50 8B 4D FC E8 ? ? ? ? 8B 4D FC 8B 55 08 3B 91 94 00 00 00");
        auto text = find("55 8B EC 83 EC 08 89 4D F8 8B 4D F8 E8 ? ? ? ? 8B 45 F8 C7 00 ? ? ? ? 8B 4D F8 C6 81 94 00 00 00 01");
        auto add = find("55 8B EC 51 89 4D FC 8B 45 08 50 8B 4D FC 83 C1 60");
        auto container = find("55 8B EC 51 89 4D FC 8B 4D FC E8 ? ? ? ? 8B 45 FC C7 00 ? ? ? ? 8B 4D FC C7 41 7C 00 00 00 00");
        auto lookup = find("55 8B EC 81 EC 1C 01 00 00 89 8D E4 FE FF FF");
        auto layout = find("55 8B EC 83 EC 2C 89 4D DC 8B 45 DC 83 B8 90 00 00 00 00");
        auto show = find("55 8B EC 51 89 4D FC 6A 00 E8 ? ? ? ? 83 C4 04 A1 ? ? ? ?");
        auto pressed = find("55 8B EC 8B 45 08 0F B6 80 ? ? ? ? 83 E0 02");
        api.text = reinterpret_cast<decltype(api.text)>(text);
        api.container = reinterpret_cast<decltype(api.container)>(container);
        api.addChild = reinterpret_cast<decltype(api.addChild)>(add);
        api.layout = reinterpret_cast<decltype(api.layout)>(layout);
        api.show = reinterpret_cast<decltype(api.show)>(show);
        api.pressed = reinterpret_cast<decltype(api.pressed)>(pressed);
        api.containerTable = *reinterpret_cast<void***>(static_cast<uint8_t*>(container) + 20);
        if (auto address = *reinterpret_cast<Object**>(static_cast<uint8_t*>(show) + 18))
            currentMenu.SetAddress(address);
        std::copy_n(api.containerTable, TableSize, pageTable.begin());
        pageTable[SlotActivated] = reinterpret_cast<void*>(&PageActivated);
        pageTable[SlotShow] = reinterpret_cast<void*>(&PageShow);
        lookupHook = safetyhook::create_inline(lookup, Lookup);
        optionsHook = safetyhook::create_inline(options, OptionsCreated);
        activatedHook = safetyhook::create_inline(activated, OptionsActivated);
    }
}
