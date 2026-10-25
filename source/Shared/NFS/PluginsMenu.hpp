#pragma once
#include <algorithm>
#include <atomic>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "../OtherPlugins.hpp"
#include "Text.hpp"

// The one row the NFS fixes add to a game's options menu: "Plugins" (WFP_PLUGINS). It lists the fix first, then every
// other loaded plugin with settings in menus (ASI Settings menu hosts); each opens its page:
//
//   Preset              only when the plugin has presets; left/right picks one, accept asks, accept again applies
//   its settings        by the description's pages and order, compatibility pages last
//   Restore Defaults    accept asks, accept again sets this page's settings to their defaults
//
// Every plugin, the fix too, is used through its runtime table (AsiSettings_GetApi): Set checks, applies and stores the
// value, and subscribers learn about changes from presets or editors. A changed setting that applies at the next start
// sets RestartNeeded(), for the game's one-line warning (WFP_RESTART_WARNING) instead of a mark on each row.
// Settings a game draws next to its own options are left out of the fix's page (exclude). Nothing here knows the game:
// each Options module draws the rows and converts the text (UTF-8) to its font.
namespace WFPPlugins
{
    using asi_settings::LoadedPlugin;
    using asi_settings::Setting;

    // The plugins found when the menu was opened (the fix first); rows point into it, so it's kept while they're shown
    struct Snapshot
    {
        std::vector<LoadedPlugin> plugins;
    };

    // A setting that applies at the next start was changed in the menu
    inline std::atomic<bool>& RestartNeeded()
    {
        static std::atomic<bool> restart = false;
        return restart;
    }

    inline bool Shown(const Setting& setting)
    {
        return setting.visibility == "menu" && setting.type != asi_settings::Type::Info &&
            (setting.HasValue() || setting.type == asi_settings::Type::Action);
    }

    // The fix (host) and, when this fix is the process's menu host, every other plugin with settings in menus
    inline std::shared_ptr<Snapshot> Find(HMODULE host, std::string_view language)
    {
        auto snapshot = std::make_shared<Snapshot>();
        bool others = asi_settings::ClaimHostRole();
        static std::map<HMODULE, int32_t> subscriptions;
        for (auto& plugin : asi_settings::FindLoadedPlugins(host, language))
        {
            if (plugin.module != host && !others)
                continue;
            auto& settings = plugin.file.description.settings;
            if (plugin.module != host && std::none_of(settings.begin(), settings.end(), [](const Setting& s) { return Shown(s); }))
                continue;
            if (!subscriptions.count(plugin.module) && OtherPlugins::HasFunction(plugin.api, offsetof(AsiSettingsApi, Subscribe)))
            {
                subscriptions[plugin.module] = 0;
                int32_t token = plugin.api->Subscribe(OtherPlugins::OnChanged, nullptr);
                if (token > 0)
                    subscriptions[plugin.module] = token;
                else
                    subscriptions.erase(plugin.module);
            }
            snapshot->plugins.push_back(std::move(plugin));
        }
        // The fix is always first, also when FindLoadedPlugins couldn't read it
        std::stable_partition(snapshot->plugins.begin(), snapshot->plugins.end(), [&](const LoadedPlugin& p) { return p.module == host; });
        return snapshot;
    }

    // Set by plugins (from any thread) after a value changed; the menu shows its rows again
    inline std::atomic<bool>& Changed()
    {
        return OtherPlugins::Changed();
    }

    inline std::string Name(const LoadedPlugin& plugin)
    {
        return OtherPlugins::Name(plugin);
    }

    struct Row
    {
        enum class Kind { Preset, Setting, Action, RestoreDefaults } kind = Kind::Setting;
        const LoadedPlugin* plugin = nullptr;
        const Setting* setting = nullptr;
    };

    // A part of a plugin's page: a description page's title (UTF-8) and its rows
    struct Section
    {
        std::string title;
        std::vector<Row> rows;
    };

    inline bool Excluded(const Setting& setting, const std::vector<std::string>& exclude)
    {
        return std::any_of(exclude.begin(), exclude.end(), [&](const std::string& id) { return asi_settings::text::IEquals(id, setting.id); });
    }

    // The plugin's page by the description's pages: the Preset row first, Restore Defaults last
    inline std::vector<Section> Sections(const LoadedPlugin& plugin, const std::vector<std::string>& exclude = {})
    {
        auto& description = plugin.file.description;
        std::vector<const asi_settings::Page*> pages;
        for (auto& page : description.pages)
            pages.push_back(&page);
        std::stable_sort(pages.begin(), pages.end(), [](auto a, auto b) { return a->order < b->order; });
        auto rank = [&](const Setting& setting)
        {
            for (size_t i = 0; i < pages.size(); ++i)
            {
                if (asi_settings::text::IEquals(pages[i]->id, setting.page))
                    return i;
            }
            return pages.size();
        };
        std::vector<const Setting*> settings;
        for (auto& setting : description.settings)
        {
            if (Shown(setting) && !Excluded(setting, exclude))
                settings.push_back(&setting);
        }
        std::stable_sort(settings.begin(), settings.end(), [&](auto a, auto b)
        {
            if (a->compatibility != b->compatibility)
                return b->compatibility;
            auto ra = rank(*a), rb = rank(*b);
            return ra != rb ? ra < rb : a->order < b->order;
        });
        std::vector<Section> sections;
        size_t previous = pages.size();
        bool compatibility = false;
        bool resettable = false;
        for (auto setting : settings)
        {
            size_t page = rank(*setting);
            if (sections.empty() || page != previous || setting->compatibility != compatibility)
            {
                auto title = page < pages.size() ? OtherPlugins::Text(plugin, pages[page]->title) : Name(plugin);
                sections.push_back({ title, {} });
            }
            auto kind = setting->type == asi_settings::Type::Action ? Row::Kind::Action : Row::Kind::Setting;
            sections.back().rows.push_back({ kind, &plugin, setting });
            resettable |= kind == Row::Kind::Setting;
            previous = page;
            compatibility = setting->compatibility;
        }
        if (sections.empty())
            sections.push_back({ Name(plugin), {} });
        if (OtherPlugins::HasPresets(plugin))
            sections.front().rows.insert(sections.front().rows.begin(), Row{ Row::Kind::Preset, &plugin });
        if (resettable)
            sections.back().rows.push_back(Row{ Row::Kind::RestoreDefaults, &plugin });
        return sections;
    }

    // The rows of the whole page in one list
    inline std::vector<Row> Rows(const LoadedPlugin& plugin, const std::vector<std::string>& exclude = {})
    {
        std::vector<Row> rows;
        for (auto& section : Sections(plugin, exclude))
            rows.insert(rows.end(), section.rows.begin(), section.rows.end());
        return rows;
    }

    // For menus with a fixed number of rows: the page split into screens of at most pageRows rows, each section on
    // screens of its own when separate is set. Titles: the plugin's name, ": section" when there are several,
    // " (2/3)" when a section needs more than one screen.
    struct Screen
    {
        std::string title;
        const LoadedPlugin* plugin = nullptr;
        std::vector<Row> rows;
    };

    inline std::vector<Screen> Screens(const LoadedPlugin& plugin, size_t pageRows, bool separate, const std::vector<std::string>& exclude = {})
    {
        std::vector<Screen> screens;
        if (pageRows == 0)
            return screens;
        auto name = Name(plugin);
        auto sections = Sections(plugin, exclude);
        if (!separate)
            sections = { Section{ name, Rows(plugin, exclude) } };
        for (auto& section : sections)
        {
            std::string title = name;
            if (sections.size() > 1 && !asi_settings::text::IEquals(section.title, name))
                title += ": " + section.title;
            size_t count = (section.rows.size() + pageRows - 1) / pageRows;
            for (size_t i = 0; i < count; ++i)
            {
                Screen screen{ title, &plugin, {} };
                if (count > 1)
                    screen.title += " (" + std::to_string(i + 1) + "/" + std::to_string(count) + ")";
                auto begin = section.rows.begin() + i * pageRows;
                screen.rows.assign(begin, begin + std::min(pageRows, section.rows.size() - i * pageRows));
                screens.push_back(std::move(screen));
            }
        }
        return screens;
    }

    // Values

    inline std::string Get(const Row& row)
    {
        return row.setting ? OtherPlugins::Get(*row.plugin, *row.setting) : std::string();
    }

    // Sets a value through the plugin; true when it changed.
    template<class Continue>
    inline bool Set(const LoadedPlugin& plugin, const Setting& setting, const std::string& value, Continue active)
    {
        if (!active())
            return false;
        auto current = OtherPlugins::Get(plugin, setting);
        if (!active() || value == current || OtherPlugins::Set(plugin, setting, value) != ASI_SETTINGS_OK)
            return false;
        if (setting.apply == "restart")
            RestartNeeded() = true;
        return true;
    }

    inline bool Set(const LoadedPlugin& plugin, const Setting& setting, const std::string& value)
    {
        return Set(plugin, setting, value, []() { return true; });
    }

    inline bool Visible(const Row& row)
    {
        return !row.setting || OtherPlugins::Visible(*row.plugin, *row.setting);
    }

    inline bool Enabled(const Row& row)
    {
        return !row.setting || OtherPlugins::Enabled(*row.plugin, *row.setting, false);
    }

    // Left (-1) or right (+1) on a setting's row; false when nothing changed.
    template<class Continue>
    inline bool Step(const Row& row, int direction, Continue active)
    {
        if (!active() || row.kind != Row::Kind::Setting || !Enabled(row) || !active())
            return false;
        auto value = OtherPlugins::NextValue(*row.plugin, *row.setting, direction);
        return active() && Set(*row.plugin, *row.setting, value, active);
    }

    inline bool Step(const Row& row, int direction)
    {
        return Step(row, direction, []() { return true; });
    }

    // Defaults of the plugin's page, without settings joined to the game's own pages.
    template<class Continue>
    inline bool RestoreDefaults(const LoadedPlugin& plugin, const std::vector<std::string>& exclude, Continue active)
    {
        bool changed = false;
        for (auto& row : Rows(plugin, exclude))
        {
            if (!active())
                break;
            if (row.kind == Row::Kind::Setting && row.setting->HasValue())
                changed |= Set(plugin, *row.setting, asi_settings::detail::JsonToText(row.setting->defaultValue), active);
        }
        return changed;
    }

    inline bool RestoreDefaults(const LoadedPlugin& plugin, const std::vector<std::string>& exclude = {})
    {
        return RestoreDefaults(plugin, exclude, []() { return true; });
    }

    // Defaults of settings joined to a native category.
    template<class Continue>
    inline bool RestoreDefaults(const LoadedPlugin& plugin, const std::vector<const Setting*>& settings, Continue active)
    {
        bool changed = false;
        for (auto setting : settings)
        {
            if (!active())
                break;
            if (setting && setting->HasValue())
                changed |= Set(plugin, *setting, asi_settings::detail::JsonToText(setting->defaultValue), active);
        }
        return changed;
    }

    inline bool RestoreDefaults(const LoadedPlugin& plugin, const std::vector<const Setting*>& settings)
    {
        return RestoreDefaults(plugin, settings, []() { return true; });
    }

    // Text (UTF-8)

    inline std::string Label(const Row& row)
    {
        switch (row.kind)
        {
            case Row::Kind::Preset:
                return WFPText::Get("WFP_PRESET");
            case Row::Kind::RestoreDefaults:
                return WFPText::Get("WFP_RESTOREDEFAULTS");
            default:
                return OtherPlugins::Text(*row.plugin, row.setting->label);
        }
    }

    // The setting's value (on and off in the fix's words); the Preset and Restore Defaults rows draw their own
    inline std::string ValueText(const Row& row)
    {
        if (row.kind != Row::Kind::Setting)
            return {};
        return OtherPlugins::ValueText(*row.plugin, *row.setting, WFPText::Get("WFP_ON"), WFPText::Get("WFP_OFF"));
    }

    // The row's help text: the setting's description; the restart warning when a change waits for a restart
    inline std::string Help(const Row& row)
    {
        std::string text;
        switch (row.kind)
        {
            case Row::Kind::Preset:
                text = WFPText::Get("WFP_PRESET_HELP");
                break;
            case Row::Kind::RestoreDefaults:
                text = WFPText::Get("WFP_RESTOREDEFAULTS_HELP");
                break;
            default:
                text = OtherPlugins::Text(*row.plugin, row.setting->description);
                break;
        }
        return text;
    }

    inline std::string RestartWarning()
    {
        return RestartNeeded() ? WFPText::Get("WFP_RESTART_WARNING") : std::string();
    }

    // The Preset row: left/right picks a preset (shown, not applied), accept asks to confirm, accept again applies
    struct PresetRow
    {
        static constexpr const char* Names[] = { "WFP_PRESET_VANILLA", "WFP_PRESET_RECOMMENDED", "WFP_PRESET_ENHANCED" };

        const LoadedPlugin* plugin = nullptr;
        int shown = -1;             // -1: Custom (the values match no preset)
        bool confirm = false;

        // When the page is shown: the preset in use
        void Reset(const LoadedPlugin& p)
        {
            plugin = &p;
            shown = OtherPlugins::CurrentPreset(p);
            confirm = false;
        }

        int Current() const
        {
            return plugin ? OtherPlugins::CurrentPreset(*plugin) : -1;
        }

        std::string ValueText() const
        {
            if (confirm)
                return WFPText::Get("WFP_PRESET_CONFIRM");
            return WFPText::Get(shown < 0 ? "WFP_PRESET_CUSTOM" : Names[shown]);
        }

        // The preset's name without the confirmation, e.g. for an arrow-less value field
        std::string PresetText() const
        {
            return WFPText::Get(shown < 0 ? "WFP_PRESET_CUSTOM" : Names[shown]);
        }

        void Cycle(int direction)
        {
            int count = int(std::size(Names));
            shown = shown < 0 ? (direction > 0 ? 0 : count - 1) : ((shown + direction) % count + count) % count;
            confirm = false;
        }

        // True when something changed (redraw); the second press applies
        template<class Continue>
        bool Accept(Continue active)
        {
            if (!active())
                return false;
            if (!plugin || shown < 0 || (!confirm && shown == Current()) || !active())
                return false;
            if (!confirm)
            {
                confirm = true;
                return true;
            }
            confirm = false;
            if (OtherPlugins::ApplyPreset(*plugin, shown, active))
                RestartNeeded() = true;
            return true;
        }

        // Any other row's input cancels a pending confirmation; true when there was one
        bool Accept()
        {
            return Accept([]() { return true; });
        }

        bool Cancel()
        {
            return std::exchange(confirm, false);
        }
    };

    // The Restore Defaults row: accept asks to confirm, accept again restores
    struct RestoreRow
    {
        bool confirm = false;

        std::string ValueText() const
        {
            return confirm ? WFPText::Get("WFP_PRESET_CONFIRM") : std::string();
        }

        template<class Continue>
        bool Accept(const LoadedPlugin& plugin, const std::vector<std::string>& exclude, Continue active)
        {
            if (!active())
                return false;
            if (!confirm)
            {
                confirm = true;
                return true;
            }
            confirm = false;
            RestoreDefaults(plugin, exclude, active);
            return true;
        }

        bool Accept(const LoadedPlugin& plugin, const std::vector<std::string>& exclude = {})
        {
            return Accept(plugin, exclude, []() { return true; });
        }

        bool Cancel()
        {
            return std::exchange(confirm, false);
        }
    };
}
