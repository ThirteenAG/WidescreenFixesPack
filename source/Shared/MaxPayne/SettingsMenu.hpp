#pragma once
#include <algorithm>
#include <cctype>
#include <cstring>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>
#include "../AsiSettings/asi_settings.hpp"
#include "../OtherPlugins.hpp"

// The settings of the fix and of the other plugins with an ASI Settings description in the Max Payne games' own options
// menu (the same menu system in both games):
// - Settings that belong next to the game's own options go into those menus (Video, Generic, Controls, Mouse
//   Settings), before the game's Restore Defaults, which then resets them too.
// - One "Plugins" row at the end of Options lists the fix first, then every other plugin with settings. Each opens its
//   page: its preset row and the confirmation to apply it, its remaining settings, compatibility settings last, and
//   Restore Defaults for that page's rows.
//
// The MaxFX menus are scripts: menu.txt describes every menu and its rows ([Menu], [Item], [Toggle], [Slider],
// [Back]), with a numeric ID per row that the game's menu mode answers through virtual functions: getSlider/setSlider
// (0..1) and setItem/isItemEnabled. menu.txt is read through R_File (rlmfc), so the fix adds its rows to the text as the
// script loader reads it, with IDs of its own, and answers those IDs in the menu mode.
//
// - Numbers with a range and a step are sliders with a tick per step; everything else (switches, choices, special
//   values, suggestions, aspect ratios) is an item that steps to the next usable value when pressed. The game's own
//   toggles aren't used: their box sits at a fixed distance from the middle of the menu, over longer names.
//   Sliders and items show their value in their name, "Field of View: 1.10 x", which follows every change (the menu
//   mode updates every line every frame, the fix prints the name again when its text changed).
// - Confirmations use the game's own "Are you sure?" menu with its Yes and No.
// - Values go through each plugin's runtime interface, the fix's own included, so the menu, editors and the live link
//   all change them the same way.
// - The menu's fonts are bitmaps of the Latin-1 letters; other characters are replaced. Tooltips are one line.
namespace MaxPayneMenu
{
    using asi_settings::LoadedPlugin;
    using asi_settings::Setting;

    // Row IDs of the fix, far above the game's own (menuid.h: 0..45)
    constexpr int32_t FirstId = 7000;

    enum class Widget { Slider, Item };

    // Where a group of the fix's settings goes in a menu of the game
    enum class Spot
    {
        BeforeLastMenu,     // before the menu's last submenu (the game's Restore Defaults)
        BeforeFirstItem,    // after the game's toggles and sliders, before its first item
        BeforeBack,         // at the end
    };

    struct Placement
    {
        std::string headline;               // the game's menu, by the string ID of its headline (GCID_VIDEOU)
        Spot spot = Spot::BeforeBack;
        int32_t defaultsId = 0;             // the ID of the game's Restore Defaults that resets these settings too
        std::vector<std::string> settings;  // the fix's setting IDs, in their order
    };

    struct Row
    {
        enum class Kind { Setting, Action, Preset, ApplyPreset, RestoreDefaults } kind = Kind::Setting;
        int plugin = -1;                    // index into State::plugins
        const Setting* setting = nullptr;
        std::vector<const Setting*> page;   // Restore Defaults: the settings it resets
        Widget widget = Widget::Item;
        int ticks = 0;                      // sliders: the number of positions
        std::string shown;                  // the name last printed on the line
    };

    struct State
    {
        HMODULE host = nullptr;
        std::vector<Placement> placements;
        std::vector<LoadedPlugin> plugins;  // the fix first; not resized after the rows point into it
        std::vector<Row> rows;
        std::vector<int> presets;           // per plugin: the preset picked in its preset row, -1 Custom
        std::map<int32_t, std::vector<const Setting*>> gameDefaults;   // the game's Restore Defaults ID: the fix's settings
        std::map<std::string, std::string> started;                     // values when first seen, by module and ID
        asi_settings::Texts hostTexts;      // the fix's own texts, for the rows of the menu itself
        void(__fastcall* textClear)(void* text, void* edx) = nullptr;
        void(__fastcall* textPrint)(void* text, void* edx, const char* string) = nullptr;
        std::string menuText;               // menu.txt with the fix's rows, between R_File::size and R_File::load
    };

    inline State& Get()
    {
        static State state;
        return state;
    }

    // ---------------------------------------------------------------------------------------------------------------
    // Text

    // The fix's own words (text/MaxPayne/<language>.json, keys WFP_...)
    inline std::string HostText(std::string_view key)
    {
        return Get().hostTexts.Resolve("@" + std::string(key));
    }

    // UTF-8 to the menu fonts' Latin-1; quotes would end the script's string, so they become apostrophes
    inline std::string ToFont(std::string_view text)
    {
        std::string out;
        for (size_t i = 0; i < text.size(); ++i)
        {
            auto c = uint8_t(text[i]);
            uint32_t code = c;
            if (c >= 0x80)
            {
                int extra = c >= 0xF0 ? 3 : c >= 0xE0 ? 2 : c >= 0xC0 ? 1 : 0;
                code = extra == 3 ? c & 0x07 : extra == 2 ? c & 0x0F : c & 0x1F;
                for (int k = 0; k < extra && i + 1 < text.size(); ++k)
                    code = (code << 6) | (uint8_t(text[++i]) & 0x3F);
                if (code == 0x2019 || code == 0x2018)
                    code = '\'';
                else if (code == 0x2013 || code == 0x2014)
                    code = '-';
                else if (code == 0x201C || code == 0x201D)
                    code = '\'';
                else if (code == 0x2026)
                    code = '.';
                else if (code > 0xFF)
                    code = '?';
            }
            if (code == '"')
                code = '\'';
            if (code == '\n' || code == '\r' || code == '\t')
                code = ' ';
            out += char(code);
        }
        return out;
    }

    // Upper case of Latin-1 text, as the game's headlines are
    inline std::string UpperFont(std::string text)
    {
        for (auto& c : text)
        {
            auto code = uint8_t(c);
            if (code < 0x80)
                c = char(std::toupper(code));
            else if (code >= 0xE0 && code <= 0xFE && code != 0xF7)
                c = char(code - 0x20);
        }
        return text;
    }

    // English labels in title case, as the game's rows are ("Rain droplets" -> "Rain Droplets")
    inline std::string TitleCase(std::string text)
    {
        static constexpr std::string_view Small[] = { "a", "an", "and", "as", "at", "by", "for", "in", "of", "on", "or", "the", "to", "vs", "with" };
        size_t start = 0;
        while (start < text.size())
        {
            auto end = text.find(' ', start);
            if (end == std::string::npos)
                end = text.size();
            std::string_view word(text.data() + start, end - start);
            bool minor = start > 0 && std::any_of(std::begin(Small), std::end(Small), [&](std::string_view s) { return asi_settings::text::IEquals(s, word); });
            if (!minor && !word.empty() && uint8_t(word[0]) < 0x80)
                text[start] = char(std::toupper(uint8_t(text[start])));
            start = end + 1;
        }
        return text;
    }

    // A string of the menu script
    inline std::string Quoted(std::string_view text)
    {
        return "\"" + ToFont(text) + "\"";
    }

    // Tooltips are one line of the small font, about as long as the game's longest: whole sentences
    // that fit, otherwise words
    constexpr size_t MaxTip = 105;

    inline std::string FitTip(std::string text)
    {
        if (text.size() <= MaxTip)
            return text;
        auto sentence = text.rfind(". ", MaxTip - 1);
        if (sentence != std::string::npos && sentence > MaxTip / 3)
            return text.substr(0, sentence + 1);
        auto space = text.rfind(' ', MaxTip - 3);
        return text.substr(0, space == std::string::npos ? MaxTip - 3 : space) + "...";
    }

    // ---------------------------------------------------------------------------------------------------------------
    // Rows

    inline const LoadedPlugin* PluginOf(const Row& row)
    {
        auto& s = Get();
        return row.plugin >= 0 && row.plugin < int(s.plugins.size()) ? &s.plugins[row.plugin] : nullptr;
    }

    inline Row* Find(int32_t id)
    {
        auto& s = Get();
        int index = id - FirstId;
        return index >= 0 && index < int(s.rows.size()) ? &s.rows[index] : nullptr;
    }

    // How a setting shows: a slider through its range, or an item stepping through its values
    inline Widget WidgetOf(const Setting& setting, int& ticks)
    {
        ticks = 0;
        bool numeric = setting.type == asi_settings::Type::Int || setting.type == asi_settings::Type::Float;
        bool listed = !setting.special.empty() || !setting.suggestions.empty();
        if (numeric && !listed && setting.min && setting.max && setting.step && *setting.step > 0.0)
        {
            double steps = std::round((*setting.max - *setting.min) / *setting.step);
            if (steps >= 1.0 && steps <= 200.0)
            {
                ticks = int(steps) + 1;
                return Widget::Slider;
            }
        }
        return Widget::Item;
    }

    inline std::string PresetName(int preset)
    {
        static constexpr const char* Names[] = { "WFP_PRESET_VANILLA", "WFP_PRESET_RECOMMENDED", "WFP_PRESET_ENHANCED" };
        return HostText(preset < 0 || preset > 2 ? "WFP_PRESET_CUSTOM" : Names[preset]);
    }

    inline std::string StartedKey(const LoadedPlugin& plugin, const Setting& setting)
    {
        return std::to_string(uintptr_t(plugin.module)) + ":" + setting.id;
    }

    // The longest slider name with its value that stays inside the menu (about "Horizontal Aiming Speed")
    constexpr size_t MaxSliderName = 23;

    // The line's name: the setting's label and, for sliders and items, its value. A setting that applies after a
    // restart says so while its value isn't the one the game started with.
    inline std::string RowText(const Row& row)
    {
        auto& s = Get();
        auto plugin = PluginOf(row);
        if (row.kind == Row::Kind::Preset)
            return HostText("WFP_PRESET") + ": " + PresetName(s.presets[row.plugin]);
        if (!plugin || !row.setting)
            return {};
        auto label = TitleCase(OtherPlugins::Text(*plugin, row.setting->label));
        if (row.kind == Row::Kind::Action)
            return label;
        auto text = label + ": " + OtherPlugins::ValueText(*plugin, *row.setting, HostText("WFP_ON"), HostText("WFP_OFF"));
        // A slider's name is right-aligned to the slider: a long one would run past the menu's left side, so it shows
        // only the label then, the slider shows the value
        if (row.widget == Widget::Slider && text.size() > MaxSliderName)
            text = label;
        if (row.setting->apply == "restart")
        {
            auto started = s.started.find(StartedKey(*plugin, *row.setting));
            if (started != s.started.end() && started->second != OtherPlugins::Get(*plugin, *row.setting))
                text += " " + HostText("WFP_RESTART");
        }
        return text;
    }

    // ---------------------------------------------------------------------------------------------------------------
    // The rows in the script

    // The rows of a menu, one script entry each
    using Rows = std::vector<std::string>;

    // A menu shows about a dozen rows between its headline and the tooltip
    constexpr size_t MaxRows = 12;

    inline int32_t AddRow(Row row)
    {
        auto& s = Get();
        row.shown = RowText(row);
        s.rows.push_back(std::move(row));
        return FirstId + int32_t(s.rows.size()) - 1;
    }

    inline std::string RowLine(Row row, const std::string& tip)
    {
        auto widget = row.widget;
        auto ticks = row.ticks;
        auto id = AddRow(std::move(row));
        auto& name = Get().rows.back().shown;
        auto common = "\tName = " + Quoted(name) + ";\r\n\tToolTip = " + Quoted(FitTip(tip)) + ";\r\n\tID = " + std::to_string(id) + ";\r\n";
        if (widget == Widget::Item)
            return "\t[Item]\r\n\t{\r\n\t[General]\r\n" + common + "\t}\r\n";
        return "\t[Slider]\r\n" + common + "\tTicks = " + std::to_string(ticks) + ";\r\n";
    }

    // A submenu asking the game's "Are you sure?" with Yes (the row) and No. name: a string of the script or a string ID.
    inline std::string ConfirmLine(Row row, const std::string& name, const std::string& tip)
    {
        auto id = AddRow(std::move(row));
        return "\t[Menu]\r\n\t{\r\n\t[Properties]\r\n\tName = " + name + ";\r\n\tToolTip = " + Quoted(FitTip(tip)) +
            ";\r\n\tHeadline = GCID_AREYOUSURE;\r\n\tInitialPosition = 1;\r\n"
            "\t[Back]\r\n\tName = GCID_YES;\r\n\tToolTip = GCID_YES_TIP;\r\n\tID = " + std::to_string(id) + ";\r\n"
            "\t[Back]\r\n\tName = GCID_NO;\r\n\tToolTip = GCID_NO_TIP;\r\n\tID = 0;\r\n\t}\r\n";
    }

    inline std::string Submenu(const std::string& title, const std::string& tip, const Rows& rows)
    {
        std::string text = "\t[Menu]\r\n\t{\r\n\t[Properties]\r\n\tName = " + Quoted(title) + ";\r\n\tToolTip = " + Quoted(FitTip(tip)) +
            ";\r\n\tHeadline = \"" + UpperFont(ToFont(title)) + "\";\r\n";
        for (auto& row : rows)
            text += row;
        return text + "\t[Back]\r\n\tName = GCID_BACK;\r\n\tToolTip = GCID_BACK_TIP;\r\n\tID = 0;\r\n\t}\r\n";
    }

    // Submenus of at most MaxRows rows; more continue in the next ones, "Name (2/3)"
    inline Rows Submenus(const std::string& title, const std::string& tip, const Rows& rows)
    {
        if (rows.size() <= MaxRows)
            return { Submenu(title, tip, rows) };
        Rows menus;
        size_t count = (rows.size() + MaxRows - 1) / MaxRows;
        for (size_t i = 0; i < count; ++i)
        {
            Rows part(rows.begin() + i * MaxRows, rows.begin() + std::min(rows.size(), (i + 1) * MaxRows));
            menus.push_back(Submenu(title + " (" + std::to_string(i + 1) + "/" + std::to_string(count) + ")", tip, part));
        }
        return menus;
    }

    inline bool Shown(const Setting& setting)
    {
        if (!setting.InMenu() || setting.type == asi_settings::Type::Info || setting.type == asi_settings::Type::Key)
            return false;
        return setting.HasValue() || setting.type == asi_settings::Type::Action;
    }

    inline bool Placed(const Setting& setting)
    {
        for (auto& placement : Get().placements)
        {
            for (auto& id : placement.settings)
            {
                if (asi_settings::text::IEquals(id, setting.id))
                    return true;
            }
        }
        return false;
    }

    // A plugin's settings for the menu, in its page order with compatibility pages last; for the fix, without the
    // ones placed into the game's menus
    inline std::vector<const Setting*> MenuSettings(int pluginIndex)
    {
        auto& description = Get().plugins[pluginIndex].file.description;
        std::vector<const asi_settings::Page*> pages;
        for (auto& page : description.pages)
            pages.push_back(&page);
        std::stable_sort(pages.begin(), pages.end(), [](auto a, auto b) { return a->order < b->order; });
        std::stable_partition(pages.begin(), pages.end(), [](auto page) { return !asi_settings::text::IEquals(page->section, "compatibility"); });
        auto pageIndex = [&](const Setting& setting)
        {
            for (size_t i = 0; i < pages.size(); ++i)
            {
                if (asi_settings::text::IEquals(pages[i]->id, setting.page))
                    return int(i);
            }
            return 0;
        };

        std::vector<const Setting*> settings;
        for (auto& setting : description.settings)
        {
            if (Shown(setting) && !(pluginIndex == 0 && Placed(setting)))
                settings.push_back(&setting);
        }
        std::stable_sort(settings.begin(), settings.end(), [&](auto a, auto b)
        {
            auto key = [&](const Setting* s) { return std::make_tuple(s->compatibility ? 1 : 0, pageIndex(*s), s->order); };
            return key(a) < key(b);
        });
        return settings;
    }

    inline std::string SettingRow(int pluginIndex, const Setting& setting)
    {
        Row row;
        row.plugin = pluginIndex;
        row.setting = &setting;
        if (setting.type == asi_settings::Type::Action)
            row.kind = Row::Kind::Action;
        else
            row.widget = WidgetOf(setting, row.ticks);
        return RowLine(std::move(row), OtherPlugins::Text(Get().plugins[pluginIndex], setting.description));
    }

    // A plugin's page in Plugins: its preset, its settings, Restore Defaults for them
    inline Rows PluginPage(int pluginIndex)
    {
        auto& plugin = Get().plugins[pluginIndex];
        Rows rows;
        std::vector<const Setting*> page;
        if (OtherPlugins::HasPresets(plugin))
        {
            Row preset;
            preset.kind = Row::Kind::Preset;
            preset.plugin = pluginIndex;
            rows.push_back(RowLine(std::move(preset), HostText("WFP_PRESET_DESCRIPTION")));
            Row apply;
            apply.kind = Row::Kind::ApplyPreset;
            apply.plugin = pluginIndex;
            rows.push_back(ConfirmLine(std::move(apply), Quoted(HostText("WFP_PRESET_APPLY")), HostText("WFP_PRESET_APPLY_DESCRIPTION")));
        }
        for (auto setting : MenuSettings(pluginIndex))
        {
            rows.push_back(SettingRow(pluginIndex, *setting));
            if (setting->HasValue())
                page.push_back(setting);
        }
        if (!page.empty())
        {
            Row restore;
            restore.kind = Row::Kind::RestoreDefaults;
            restore.plugin = pluginIndex;
            restore.page = page;
            rows.push_back(ConfirmLine(std::move(restore), "GCID_RESTOREDEFAULTS", HostText("WFP_RESTOREDEFAULTS_DESCRIPTION")));
        }
        auto name = TitleCase(OtherPlugins::Name(plugin));
        return rows.empty() ? Rows() : Submenus(name, name, rows);
    }

    // The plugins with settings, the fix first: when the menu is read, since Ultimate ASI Loader loads them one after
    // another and some only after the fix's init. Without the menu host role (another plugin draws the other plugins'
    // pages) only the fix's own.
    inline bool FindPlugins()
    {
        auto& s = Get();
        if (!s.plugins.empty())
            return true;
        bool hostRole = asi_settings::ClaimHostRole();
        for (auto& plugin : asi_settings::FindLoadedPlugins(s.host, "en"))
        {
            if (plugin.module == s.host || hostRole)
                s.plugins.push_back(std::move(plugin));
        }
        std::stable_partition(s.plugins.begin(), s.plugins.end(), [&](const LoadedPlugin& p) { return p.module == s.host; });
        if (s.plugins.empty() || s.plugins.front().module != s.host)
        {
            s.plugins.clear();
            return false;
        }
        s.hostTexts = s.plugins.front().file.texts;
        for (auto& plugin : s.plugins)
        {
            for (auto& setting : plugin.file.description.settings)
            {
                if (setting.HasValue() && setting.apply == "restart")
                    s.started.try_emplace(StartedKey(plugin, setting), OtherPlugins::Get(plugin, setting));
            }
        }
        return true;
    }

    // ---------------------------------------------------------------------------------------------------------------
    // menu.txt

    // A menu of the script ({ ... }): its headline's string ID and where its rows are. Comments and strings are skipped.
    struct Block
    {
        std::string headline;
        size_t lastMenu = std::string_view::npos;
        size_t firstItem = std::string_view::npos;
        size_t lastBack = std::string_view::npos;
        size_t close = std::string_view::npos;
    };

    inline std::vector<Block> ScanMenus(std::string_view text)
    {
        std::vector<Block> open, closed;
        bool afterHeadline = false;
        for (size_t i = 0; i < text.size(); )
        {
            char c = text[i];
            if (c == '/' && i + 1 < text.size() && text[i + 1] == '/')
            {
                while (i < text.size() && text[i] != '\n')
                    ++i;
                continue;
            }
            if (c == '/' && i + 1 < text.size() && text[i + 1] == '*')
            {
                auto end = text.find("*/", i + 2);
                i = end == std::string_view::npos ? text.size() : end + 2;
                continue;
            }
            if (c == '"')
            {
                ++i;
                while (i < text.size() && text[i] != '"')
                    i += text[i] == '\\' ? 2 : 1;
                ++i;
                continue;
            }
            if (c == '{')
            {
                open.push_back({});
                ++i;
                continue;
            }
            if (c == '}')
            {
                if (!open.empty())
                {
                    open.back().close = i;
                    closed.push_back(std::move(open.back()));
                    open.pop_back();
                }
                ++i;
                continue;
            }
            if (c == '[')
            {
                auto end = text.find(']', i);
                if (end == std::string_view::npos)
                    break;
                auto name = text.substr(i + 1, end - i - 1);
                if (!open.empty())
                {
                    auto& block = open.back();
                    if (asi_settings::text::IEquals(name, "back"))
                        block.lastBack = i;
                    else if (asi_settings::text::IEquals(name, "menu"))
                        block.lastMenu = i;
                    else if (asi_settings::text::IEquals(name, "item") && block.firstItem == std::string_view::npos)
                        block.firstItem = i;
                }
                i = end + 1;
                continue;
            }
            if (std::isalpha(uint8_t(c)) || c == '_')
            {
                size_t start = i;
                while (i < text.size() && (std::isalnum(uint8_t(text[i])) || text[i] == '_'))
                    ++i;
                auto word = text.substr(start, i - start);
                if (asi_settings::text::IEquals(word, "headline"))
                    afterHeadline = true;
                else if (afterHeadline)
                {
                    if (!open.empty() && open.back().headline.empty())
                        open.back().headline = std::string(word);
                    afterHeadline = false;
                }
                continue;
            }
            if (c == ';')
                afterHeadline = false;
            ++i;
        }
        return closed;
    }

    inline const Block* FindMenu(const std::vector<Block>& blocks, std::string_view headline)
    {
        for (auto& block : blocks)
        {
            if (asi_settings::text::IEquals(block.headline, headline))
                return &block;
        }
        return nullptr;
    }

    inline size_t Position(const Block& block, Spot spot)
    {
        auto end = block.lastBack != std::string_view::npos ? block.lastBack : block.close;
        if (spot == Spot::BeforeLastMenu && block.lastMenu != std::string_view::npos)
            return block.lastMenu;
        if (spot == Spot::BeforeFirstItem && block.firstItem != std::string_view::npos)
            return block.firstItem;
        return end;
    }

    // menu.txt with the fix's rows; false when the Options menu isn't found
    inline bool AddMenus(std::string& text)
    {
        auto& s = Get();
        s.rows.clear();
        s.rows.reserve(512);
        s.gameDefaults.clear();
        s.presets.assign(s.plugins.size(), -1);
        for (size_t i = 0; i < s.plugins.size(); ++i)
            s.presets[i] = OtherPlugins::CurrentPreset(s.plugins[i]);

        auto blocks = ScanMenus(text);
        auto options = FindMenu(blocks, "GCID_OPTIONSU");
        if (!options)
            return false;

        std::vector<std::pair<size_t, std::string>> inserts;

        // The fix's settings next to the game's options
        auto& fix = s.plugins.front();
        for (auto& placement : s.placements)
        {
            auto menu = FindMenu(blocks, placement.headline);
            if (!menu)
                continue;
            std::string rows;
            for (auto& id : placement.settings)
            {
                auto it = std::find_if(fix.file.description.settings.begin(), fix.file.description.settings.end(),
                    [&](const Setting& setting) { return asi_settings::text::IEquals(setting.id, id); });
                if (it == fix.file.description.settings.end() || !Shown(*it))
                    continue;
                rows += SettingRow(0, *it);
                if (placement.defaultsId && it->HasValue())
                    s.gameDefaults[placement.defaultsId].push_back(&*it);
            }
            if (!rows.empty())
                inserts.emplace_back(Position(*menu, placement.spot), rows);
        }

        // Plugins: the fix, then the others
        Rows pages;
        for (int i = 0; i < int(s.plugins.size()); ++i)
        {
            auto page = PluginPage(i);
            pages.insert(pages.end(), page.begin(), page.end());
        }
        if (!pages.empty())
            inserts.emplace_back(Position(*options, Spot::BeforeBack), Submenu(HostText("WFP_PLUGINS"), HostText("WFP_PLUGINS_DESCRIPTION"), pages));

        // From the end, so the positions before stay valid
        std::stable_sort(inserts.begin(), inserts.end(), [](auto& a, auto& b) { return a.first > b.first; });
        for (auto& [position, rows] : inserts)
            text.insert(position, rows);
        return true;
    }

    inline bool IsMenuScript(const char* name)
    {
        if (!name)
            return false;
        std::string_view path = name;
        auto slash = path.find_last_of("\\/");
        auto file = slash == std::string_view::npos ? path : path.substr(slash + 1);
        return asi_settings::text::IEquals(file, "menu.txt");
    }

    namespace R_File
    {
        inline SafetyHookInline shSize = {};
        inline SafetyHookInline shLoad = {};

        // menu.txt with the fix's rows, made from the original when its size is asked for (the script loader asks
        // for the size, then loads that many bytes)
        inline uint32_t __fastcall size(void* _this, void* edx, const char* name)
        {
            auto original = shSize.unsafe_fastcall<uint32_t>(_this, edx, name);
            auto& s = Get();
            s.menuText.clear();
            if (!IsMenuScript(name) || original == 0 || !FindPlugins())
                return original;

            std::string text(original, '\0');
            shLoad.unsafe_fastcall(_this, edx, text.data(), name, original);
            if (!AddMenus(text))
                return original;
            s.menuText = std::move(text);
            return uint32_t(s.menuText.size());
        }

        inline void __fastcall load(void* _this, void* edx, void* buffer, const char* name, uint32_t size)
        {
            auto& s = Get();
            if (!s.menuText.empty() && IsMenuScript(name) && size == s.menuText.size())
            {
                std::memcpy(buffer, s.menuText.data(), size);
                s.menuText.clear();
                return;
            }
            shLoad.unsafe_fastcall(_this, edx, buffer, name, size);
        }
    }

    // ---------------------------------------------------------------------------------------------------------------
    // The menu mode's answers for the fix's IDs

    inline void RefreshPresets()
    {
        auto& s = Get();
        for (size_t i = 0; i < s.plugins.size(); ++i)
            s.presets[i] = OtherPlugins::CurrentPreset(s.plugins[i]);
    }

    // The slider's position, 0..1
    inline std::optional<float> GetSlider(int32_t id)
    {
        auto row = Find(id);
        if (!row || !row->setting || row->widget != Widget::Slider)
            return std::nullopt;
        auto& setting = *row->setting;
        double value = std::atof(OtherPlugins::Get(*PluginOf(*row), setting).c_str());
        double range = *setting.max - *setting.min;
        return range > 0.0 ? float(std::clamp((value - *setting.min) / range, 0.0, 1.0)) : 0.0f;
    }

    inline bool SetSlider(int32_t id, float position)
    {
        auto row = Find(id);
        if (!row || !row->setting || row->widget != Widget::Slider)
            return false;
        auto& setting = *row->setting;
        auto plugin = PluginOf(*row);
        if (!OtherPlugins::Enabled(*plugin, setting, false))
            return true;
        double steps = std::round(std::clamp(double(position), 0.0, 1.0) * (row->ticks - 1));
        double value = std::clamp(*setting.min + steps * *setting.step, *setting.min, *setting.max);
        if (setting.type == asi_settings::Type::Int)
            value = std::round(value);
        auto text = asi_settings::text::FormatNumber(value);
        if (text != OtherPlugins::Get(*plugin, setting))
            OtherPlugins::Set(*plugin, setting, text);
        RefreshPresets();
        return true;
    }

    inline std::optional<bool> IsItemEnabled(int32_t id)
    {
        auto row = Find(id);
        if (!row)
            return std::nullopt;
        if (row->kind == Row::Kind::ApplyPreset)
            return Get().presets[row->plugin] >= 0;
        if (!row->setting)
            return true;
        return OtherPlugins::Enabled(*PluginOf(*row), *row->setting, false);
    }

    // Sets the settings to their defaults
    inline void RestoreDefaults(const LoadedPlugin& plugin, const std::vector<const Setting*>& settings)
    {
        for (auto setting : settings)
        {
            auto value = asi_settings::detail::JsonToText(setting->defaultValue);
            if (value != OtherPlugins::Get(plugin, *setting))
                OtherPlugins::Set(plugin, *setting, value);
        }
    }

    // An item was pressed
    inline bool SetItem(int32_t id)
    {
        auto row = Find(id);
        if (!row)
            return false;
        auto& s = Get();
        auto plugin = PluginOf(*row);
        switch (row->kind)
        {
            case Row::Kind::Preset:
            {
                auto& shown = s.presets[row->plugin];
                shown = shown < 0 ? 0 : (shown + 1) % 3;
                return true;
            }
            case Row::Kind::ApplyPreset:
                if (s.presets[row->plugin] >= 0)
                    OtherPlugins::ApplyPreset(*plugin, s.presets[row->plugin]);
                break;
            case Row::Kind::RestoreDefaults:
                RestoreDefaults(*plugin, row->page);
                break;
            case Row::Kind::Action:
                OtherPlugins::Invoke(*plugin, *row->setting);
                break;
            case Row::Kind::Setting:
                if (OtherPlugins::Enabled(*plugin, *row->setting, false))
                {
                    auto value = OtherPlugins::NextValue(*plugin, *row->setting, 1);
                    if (value != OtherPlugins::Get(*plugin, *row->setting))
                        OtherPlugins::Set(*plugin, *row->setting, value);
                }
                break;
        }
        RefreshPresets();
        return true;
    }

    // The game's Restore Defaults of a menu with the fix's rows resets them too
    inline void GameDefaults(int32_t id)
    {
        auto& s = Get();
        auto it = s.gameDefaults.find(id);
        if (it == s.gameDefaults.end() || s.plugins.empty())
            return;
        RestoreDefaults(s.plugins.front(), it->second);
        RefreshPresets();
    }

    // Every frame for every line of the page shown: the line's name follows its value
    inline void UpdateLine(int32_t id, void* text)
    {
        auto row = Find(id);
        auto& s = Get();
        if (!row || !text || !s.textClear || !s.textPrint)
            return;
        if (row->kind != Row::Kind::Setting && row->kind != Row::Kind::Preset)
            return;
        auto shown = RowText(*row);
        if (shown == row->shown)
            return;
        row->shown = shown;
        s.textClear(text, nullptr);
        s.textPrint(text, nullptr, ToFont(shown).c_str());
    }

    // ---------------------------------------------------------------------------------------------------------------
    // Settings

    // A key of the settings file: a virtual-key code (0x71) or a key name (F2, A, Space)
    inline int32_t KeyCode(std::string_view szKey)
    {
        if (szKey.empty())
            return 0;
        if (szKey.size() > 2 && szKey[0] == '0' && (szKey[1] == 'x' || szKey[1] == 'X'))
            return std::strtol(std::string(szKey).c_str(), nullptr, 16);
        if (szKey.size() == 1 && std::isalnum(uint8_t(szKey[0])))
            return std::toupper(uint8_t(szKey[0]));
        if ((szKey[0] == 'F' || szKey[0] == 'f') && szKey.size() <= 3 && std::all_of(szKey.begin() + 1, szKey.end(), ::isdigit))
        {
            auto n = std::atoi(std::string(szKey.substr(1)).c_str());
            return n >= 1 && n <= 24 ? VK_F1 + n - 1 : 0;
        }
        static const std::pair<std::string_view, int32_t> Names[] =
        {
            { "Space", VK_SPACE }, { "Tab", VK_TAB }, { "Enter", VK_RETURN }, { "Backspace", VK_BACK }, { "Insert", VK_INSERT },
            { "Delete", VK_DELETE }, { "Home", VK_HOME }, { "End", VK_END }, { "PageUp", VK_PRIOR }, { "PageDown", VK_NEXT },
            { "Pause", VK_PAUSE }, { "ScrollLock", VK_SCROLL }, { "CapsLock", VK_CAPITAL },
        };
        for (auto& [szName, nKey] : Names)
        {
            if (_strnicmp(szName.data(), szKey.data(), szKey.size()) == 0 && szName.size() == szKey.size())
                return nKey;
        }
        return std::isdigit(uint8_t(szKey[0])) ? std::atoi(std::string(szKey).c_str()) : 0;
    }

    // ---------------------------------------------------------------------------------------------------------------
    // Hooks

    // A class's virtual function table, found by its RTTI name (".?AVX_MenuItem@@") in the module that has it
    inline void** FindVTable(HMODULE module, const char* name)
    {
        auto base = reinterpret_cast<uint8_t*>(module);
        auto dos = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
        auto nt = reinterpret_cast<IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
        auto sections = IMAGE_FIRST_SECTION(nt);
        auto length = std::strlen(name) + 1;
        auto search = [&](auto&& match) -> uint8_t*
        {
            for (int i = 0; i < nt->FileHeader.NumberOfSections; ++i)
            {
                // All readable sections: packed executables keep their data in sections of any kind
                auto& section = sections[i];
                if (!(section.Characteristics & IMAGE_SCN_MEM_READ))
                    continue;
                auto begin = base + section.VirtualAddress;
                uint32_t size = section.Misc.VirtualSize;
                for (uint32_t offset = 0; offset + 4 <= size; offset += 4)
                {
                    if (auto found = match(begin + offset, size - offset))
                        return found;
                }
            }
            return nullptr;
        };
        // The type descriptor: its vtable pointer, a spare pointer, then the name
        auto descriptor = search([&](uint8_t* p, uint32_t left) -> uint8_t*
        {
            return left >= 8 + length && std::memcmp(p + 8, name, length) == 0 ? p : nullptr;
        });
        if (!descriptor)
            return nullptr;
        // The complete object locator of the class itself: signature 0, offset 0, ..., the type descriptor at +12
        auto locator = search([&](uint8_t* p, uint32_t left) -> uint8_t*
        {
            return left >= 20 && *reinterpret_cast<uint32_t*>(p) == 0 && *reinterpret_cast<uint32_t*>(p + 4) == 0 &&
                *reinterpret_cast<uint8_t**>(p + 12) == descriptor ? p : nullptr;
        });
        if (!locator)
            return nullptr;
        // The table follows the pointer to the locator
        auto table = search([&](uint8_t* p, uint32_t left) -> uint8_t*
        {
            return left >= 8 && *reinterpret_cast<uint8_t**>(p) == locator ? p + 4 : nullptr;
        });
        return reinterpret_cast<void**>(table);
    }

    // The menu mode's virtual functions the rows are answered through, by their slot
    struct MenuModeSlots
    {
        int setItem, isItemEnabled, setSlider, getSlider;
    };

    // Where a menu line keeps its name (P_Text*) and ID, and the slot of the function updating it every frame
    struct LineLayout
    {
        ptrdiff_t text, id;
        int update;
    };

    namespace MenuMode
    {
        inline SafetyHookInline shSetItem, shIsItemEnabled, shSetSlider, shGetSlider;

        inline void __fastcall setItem(void* _this, void* edx, int32_t id)
        {
            if (SetItem(id))
                return;
            shSetItem.unsafe_fastcall(_this, edx, id);
            GameDefaults(id);
        }

        inline bool __fastcall isItemEnabled(void* _this, void* edx, int32_t id)
        {
            auto enabled = IsItemEnabled(id);
            return enabled ? *enabled : shIsItemEnabled.unsafe_fastcall<bool>(_this, edx, id);
        }

        inline void __fastcall setSlider(void* _this, void* edx, int32_t id, float value)
        {
            if (!SetSlider(id, value))
                shSetSlider.unsafe_fastcall(_this, edx, id, value);
        }

        inline float __fastcall getSlider(void* _this, void* edx, int32_t id)
        {
            auto value = GetSlider(id);
            return value ? *value : shGetSlider.unsafe_fastcall<float>(_this, edx, id);
        }
    }

    namespace Lines
    {
        inline LineLayout Layout{};
        inline SafetyHookInline shUpdate[2];

        template<int N>
        void __fastcall update(uint8_t* _this, void* edx)
        {
            shUpdate[N].unsafe_fastcall(_this, edx);
            auto id = *reinterpret_cast<int32_t*>(_this + Layout.id);
            if (id >= FirstId)
                UpdateLine(id, *reinterpret_cast<void**>(_this + Layout.text));
        }
    }

    // Hooks the menu mode's functions (from its vtable) and the update of the item and slider lines (from theirs),
    // which may be in another module than the menu mode
    inline bool HookMenu(void** menuMode, const MenuModeSlots& slots, void** item, void** slider, const LineLayout& layout)
    {
        if (!menuMode || !item || !slider || !Get().host)
            return false;
        using namespace MenuMode;
        shSetItem = safetyhook::create_inline(menuMode[slots.setItem], setItem);
        shIsItemEnabled = safetyhook::create_inline(menuMode[slots.isItemEnabled], isItemEnabled);
        shSetSlider = safetyhook::create_inline(menuMode[slots.setSlider], setSlider);
        shGetSlider = safetyhook::create_inline(menuMode[slots.getSlider], getSlider);
        Lines::Layout = layout;
        Lines::shUpdate[0] = safetyhook::create_inline(item[layout.update], Lines::update<0>);
        Lines::shUpdate[1] = safetyhook::create_inline(slider[layout.update], Lines::update<1>);
        return true;
    }

    // Before the menu is read: hooks the script loader (rlmfc) and the text functions (e2mfc). host: the fix's module,
    // whose texts have the words of the menu's own rows. placements: the fix's settings that go into the game's menus.
    inline bool Init(HMODULE host, HMODULE rlmfc, HMODULE e2mfc, std::vector<Placement> placements)
    {
        auto& s = Get();
        s.textClear = reinterpret_cast<decltype(s.textClear)>(GetProcAddress(e2mfc, "?clear@P_Text@@QAEXXZ"));
        s.textPrint = reinterpret_cast<decltype(s.textPrint)>(GetProcAddress(e2mfc, "?print@P_Text@@QAEXPBD@Z"));
        auto size = GetProcAddress(rlmfc, "?size@R_File@@QAEIPBD@Z");
        auto load = GetProcAddress(rlmfc, "?load@R_File@@QAEXPAXPBDI@Z");
        if (!host || !s.textClear || !s.textPrint || !size || !load)
            return false;
        s.host = host;
        s.placements = std::move(placements);
        R_File::shSize = safetyhook::create_inline(size, R_File::size);
        R_File::shLoad = safetyhook::create_inline(load, R_File::load);
        return true;
    }
}
