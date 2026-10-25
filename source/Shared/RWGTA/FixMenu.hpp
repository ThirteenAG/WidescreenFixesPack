#pragma once
#include <algorithm>
#include <atomic>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <thread>
#include <vector>
#include <shellapi.h>
#include "../AsiSettings/asi_settings.hpp"
#include "../OtherPlugins.hpp"

// The settings of the fix and of the other plugins with an ASI Settings description, as pages
// of the game's own frontend. Nothing here knows the game: each fix's menu module puts the rows into its menu screens
// and converts the text (UTF-8) to its font.
//
// Plugins layout (the one row "Plugins" added to the game's options):
//   model.OpenPlugins(module, language, pageRows, { "MAIN.FOVFactor", ... }, true);
// - Pages() are the entries of the Plugins list: the fix first, then every other plugin with settings, one page each
//   with a scrolling window of rows, including Back as its last row (scrolling = true).
//   Older hosts retain their fixed-page layout until they provide scrolling input and drawing.
// - A plugin's page: its Preset row (only when it has presets; left/right choose, accept twice applies), its settings
//   by page and order, compatibility settings last, then Restore defaults (accept twice; resets that plugin's rows
//   of this list to their defaults). No preset of all plugins.
// - The ids passed to OpenPlugins are the fix's settings drawn next to the game's own options: they are left out of
//   the fix's page; JoinedRows() gives their rows (Label/Value/Change work on them), and RestoreDefaults(plugin, ids)
//   resets them from the game's own Restore Defaults, with or without the model open.
// - Labels carry no restart marks: RestartRequired() tells when a changed setting applies at the next start, and
//   Text("menu.restartwarning") is the line to show in the game's own style. Help(row) is the row's help text.
//
// Pages layout (Open, older menus): the first page is a list of links: Presets (game-wide when several plugins have
// presets, the fix's preset, the settings file), the fix's pages of the description, then the other plugins' pages.
//
// Every plugin, the fix too, is used through its runtime interface (AsiSettings_GetApi), so all of them behave the
// same: Set checks, applies and stores, and subscribers learn of changes from editors (the live link) or presets.
namespace RWGTAMenu
{
    using asi_settings::LoadedPlugin;
    using asi_settings::Setting;

    enum class RowKind { Setting, Action, Preset, ApplyPreset, SettingsFile, RestoreDefaults };

    // Resets the settings of the given ids to their defaults (the game's own Restore Defaults on a page with the fix's
    // rows); true when one of the changed settings applies after a restart
    inline bool RestoreDefaults(asi_settings::Plugin& plugin, const std::set<std::string>& ids)
    {
        bool restart = false;
        for (auto& setting : plugin.GetDescription().settings)
        {
            if (!setting.HasValue() || !ids.count(setting.id))
                continue;
            asi_settings::Slot slot;
            slot.setting = &setting;
            asi_settings::SetDefault(slot);
            auto value = asi_settings::FormatValue(slot);
            if (asi_settings::text::IEquals(plugin.Wanted(setting.id), value))
                continue;
            if (plugin.Set(setting.id, value, true) == ASI_SETTINGS_OK)
                restart |= setting.apply == "restart";
        }
        return restart;
    }

    struct Row
    {
        RowKind kind = RowKind::Setting;
        const LoadedPlugin* plugin = nullptr;
        const Setting* setting = nullptr;
    };

    struct Page
    {
        std::string title;      // UTF-8
        const LoadedPlugin* plugin = nullptr;
        std::vector<Row> rows;
        size_t scroll{ 0 };
    };

    class Model
    {
    public:
        // The menu's own words (UTF-8) for on and off, and the host's text for a key of its translations ("preset"...)
        std::string Text(const std::string& key) const
        {
            std::string text = host ? host->file.texts.Resolve("@" + key) : key;
            if (text == key)
            {
                // Keys of the Plugins layout that a fix's texts may not have yet
                static const std::map<std::string, std::string> English =
                {
                    { "menu.plugins", "Plugins" },
                    { "menu.restoredefaults", "Restore defaults" },
                    { "menu.restoredefaultshelp", "Sets the settings of this page to their defaults" },
                    { "menu.restartwarning", "The game must be restarted in order to apply settings." },
                    { "menu.confirm", "Confirm?" },
                    { "menu.presethelp", "Vanilla: as released. Enhanced: everything turned up" },
                    { "menu.settingsfilehelp", "Shows the settings file in File Explorer" },
                };
                if (auto it = English.find(key); it != English.end())
                    return it->second;
            }
            return text;
        }

        // The Plugins layout (see the top of the file); joined: ids of the fix's settings shown on the game's own pages
        void OpenPlugins(HMODULE hostModule, const std::string& language, size_t pageRows, const std::set<std::string>& joined = {}, bool scrolling = false)
        {
            Open(hostModule, language, pageRows, Layout::Plugins, joined);
            if (!scrolling)
                SplitPluginPages(pageRows);
        }

        // Finds the plugins and builds the pages; host: the fix's module, language: the game's language tag
        void Open(HMODULE hostModule, const std::string& language, size_t pageRows)
        {
            Open(hostModule, language, pageRows, Layout::Pages, {});
        }

        bool PluginsLayout() const { return layout == Layout::Plugins; }

        // The rows of the fix's settings passed to OpenPlugins, in their order (Plugins layout)
        const std::vector<Row>& JoinedRows() const { return joinedRows; }

        // A changed setting applies at the next start (Plugins layout: shown once, in the game's style)
        bool RestartRequired() const { return restartRequired; }

        // The row's help text: the setting's description, or the model's own for its rows
        std::string Help(const Row& row) const
        {
            switch (row.kind)
            {
                case RowKind::Preset: case RowKind::ApplyPreset: return Text("menu.presethelp");
                case RowKind::SettingsFile: return Text("menu.settingsfilehelp");
                case RowKind::RestoreDefaults: return Text("menu.restoredefaultshelp");
                default: break;
            }
            if (!row.plugin || !row.setting || row.setting->description.empty())
                return {};
            return OtherPlugins::Text(*row.plugin, row.setting->description);
        }

    private:
        enum class Layout { Pages, Plugins };

        void Open(HMODULE hostModule, const std::string& language, size_t pageRows, Layout chosen, const std::set<std::string>& joined)
        {
            Close();
            layout = chosen;
            windowSize = std::max(size_t(1), pageRows + 1);
            joinedIds = joined;
            plugins = std::make_shared<std::vector<LoadedPlugin>>();
            auto snapshot = plugins;
            bool others = asi_settings::ClaimHostRole();
            auto discovered = asi_settings::FindLoadedPlugins(hostModule, language);
            if (snapshot != plugins)
                return;
            for (auto& plugin : discovered)
            {
                if (plugin.module != hostModule && !others)
                    continue;
                if (!subscriptions.count(plugin.module) && OtherPlugins::HasFunction(plugin.api, offsetof(AsiSettingsApi, Subscribe)))
                {
                    // Discovery callbacks can reopen this model. Reserve the
                    // module before subscribing so the nested open reuses it.
                    subscriptions[plugin.module] = 0;
                    int32_t token = plugin.api->Subscribe(OnChanged, nullptr);
                    if (token > 0)
                        subscriptions[plugin.module] = token;
                    else
                        subscriptions.erase(plugin.module);
                }
                if (snapshot != plugins)
                    return;
                snapshot->push_back(std::move(plugin));
            }
            host = nullptr;
            for (auto& plugin : *plugins)
            {
                if (plugin.module == hostModule)
                    host = &plugin;
            }
            if (layout == Layout::Plugins)
                BuildPlugins(pageRows);
            else
                Build(pageRows);
            if (snapshot != plugins)
                return;
            Changed() = false;
            presetShown.clear();
            confirm = nullptr;
            confirmActive = restartActive = false;
            restoreConfirm = nullptr;
        }

    public:
        void Close()
        {
            pages.clear();
            joinedRows.clear();
            restoreConfirm = nullptr;
            host = nullptr;
            confirm = nullptr;
            confirmActive = restartActive = false;
            restartNeeded = nullptr;
            presetShown.clear();
            plugins.reset();
        }

        bool IsOpen() const { return plugins != nullptr; }
        const std::vector<Page>& Pages() const { return pages; }

        // Set by plugins (from any thread) after a value or state changed, whoever changed it
        static std::atomic<bool>& Changed()
        {
            static std::atomic<bool> changed = false;
            return changed;
        }

        // The rows shown now: settings whose visibleIf holds and that the plugin doesn't hide
        std::vector<const Row*> VisibleRows(size_t page) const
        {
            std::vector<const Row*> rows;
            auto snapshot = plugins;
            if (!snapshot || page >= pages.size())
                return rows;
            auto described = pages[page].rows;
            for (size_t i = 0; i < described.size(); ++i)
            {
                auto& row = described[i];
                bool visible = (row.kind != RowKind::Setting && row.kind != RowKind::Action) || OtherPlugins::Visible(*row.plugin, *row.setting);
                if (snapshot != plugins)
                    return {};
                if (!visible)
                    continue;
                rows.push_back(&pages[page].rows[i]);
            }
            return rows;
        }

        // A null row is Back, which scrolls with the settings.
        std::vector<const Row*> WindowRows(size_t page)
        {
            auto rows = VisibleRows(page);
            if (page >= pages.size())
                return {};
            rows.push_back(nullptr);
            auto& offset = pages[page].scroll;
            offset = std::min(offset, rows.size() > windowSize ? rows.size() - windowSize : 0);
            size_t end = std::min(rows.size(), offset + windowSize);
            return { rows.begin() + offset, rows.begin() + end };
        }

        bool HiddenAbove(size_t page) const
        {
            return page < pages.size() && pages[page].scroll > 0;
        }

        bool HiddenBelow(size_t page) const
        {
            return page < pages.size() && pages[page].scroll + windowSize < VisibleRows(page).size() + 1;
        }

        // Native directional input only: hovering a row never changes the window.
        int Move(size_t page, int slot, int direction)
        {
            auto rows = WindowRows(page);
            if (rows.empty() || !direction)
                return slot;
            auto snapshot = plugins;
            size_t total = VisibleRows(page).size() + 1;
            if (snapshot != plugins || page >= pages.size())
                return slot;
            auto& offset = pages[page].scroll;
            int selected = int(offset) + std::clamp(slot, 0, int(rows.size()) - 1);
            selected = (selected + (direction > 0 ? 1 : -1) + int(total)) % int(total);
            if (size_t(selected) < offset)
                offset = size_t(selected);
            else if (size_t(selected) >= offset + windowSize)
                offset = size_t(selected) - windowSize + 1;
            return selected - int(offset);
        }

        void Scroll(size_t page, int direction)
        {
            auto rows = VisibleRows(page);
            if (page >= pages.size())
                return;
            size_t maximum = rows.size() + 1 > windowSize ? rows.size() + 1 - windowSize : 0;
            auto& offset = pages[page].scroll;
            offset = std::min(offset, maximum);
            if (direction < 0 && offset > 0)
                --offset;
            else if (direction > 0 && offset < maximum)
                ++offset;
        }

        // The row's name; settings that apply at the next start say so (Pages layout)
        std::string Label(const Row& row) const
        {
            switch (row.kind)
            {
                case RowKind::Preset: return Text(row.plugin ? "menu.preset" : "menu.gamepreset");
                case RowKind::ApplyPreset: return Text("menu.applypreset");
                case RowKind::SettingsFile: return Text("menu.settingsfile");
                case RowKind::RestoreDefaults: return Text("menu.restoredefaults");
                default: break;
            }
            auto label = OtherPlugins::Text(*row.plugin, row.setting->label);
            if (layout == Layout::Pages && row.kind == RowKind::Setting && row.setting->apply == "restart")
                label += " " + Text("menu.restart");
            return label;
        }

        // The value shown at the right of the row (empty: none)
        std::string Value(const Row& source) const
        {
            Row row = source;
            auto snapshot = plugins;
            if (!snapshot)
                return {};
            auto read = [&]() -> std::string
            {
                switch (row.kind)
                {
                    case RowKind::Setting:
                        return OtherPlugins::ValueText(*row.plugin, *row.setting, Text("menu.on"), Text("menu.off"));
                    case RowKind::Preset:
                    {
                        if (layout == Layout::Plugins && confirmActive && confirm == row.plugin)
                            return Text("menu.confirm");
                        int shown = PresetShown(row.plugin);
                        return Text(shown < 0 ? "menu.custom" : PresetNames[shown]);
                    }
                    case RowKind::RestoreDefaults:
                        return restoreConfirm == row.plugin ? Text("menu.confirm") : std::string{};
                    case RowKind::ApplyPreset:
                    {
                        if (confirmActive && confirm == row.plugin)
                            return Text("menu.confirm");
                        if (restartActive && restartNeeded == row.plugin)
                            return Text("menu.restartneeded");
                        int shown = PresetShown(row.plugin);
                        if (shown < 0)
                            return {};
                        return Text(MatchesPreset(row.plugin, shown) ? "menu.inuse" : "menu.press");
                    }
                    case RowKind::SettingsFile:
                        return Text("menu.show");
                    default:
                        return {};
                }
            };
            auto value = read();
            return snapshot == plugins ? value : std::string{};
        }

        // Whether the row can be changed now; pause: the game is running behind the menu
        bool Enabled(const Row& source, bool pause) const
        {
            Row row = source;
            auto snapshot = plugins;
            if (!snapshot)
                return false;
            if (row.kind != RowKind::Setting)
                return true;
            bool enabled = OtherPlugins::Enabled(*row.plugin, *row.setting, pause);
            return snapshot == plugins && enabled;
        }

        // Left (-1), right (+1) or accept (0) on a row; true when something changed
        bool Change(const Row& source, int direction, bool pause)
        {
            // Set/Invoke may synchronously close or rebuild the native menu.
            // Keep the row's description and plugin API snapshot alive until
            // the call unwinds, and never publish state into its replacement.
            Row row = source;
            auto snapshot = plugins;
            auto current = [&]() { return snapshot && snapshot == plugins; };
            if (!current())
                return false;
            bool presetAccept = layout == Layout::Plugins && row.kind == RowKind::Preset && direction == 0;
            if (row.kind != RowKind::ApplyPreset && !presetAccept)
            {
                confirm = nullptr;
                confirmActive = false;
            }
            if (row.kind != RowKind::RestoreDefaults || direction != 0)
                restoreConfirm = nullptr;
            if (presetAccept)
                return ApplyShownPreset(row.plugin, current);
            switch (row.kind)
            {
                case RowKind::Setting:
                {
                    if (!Enabled(row, pause) || !current())
                        return false;
                    auto value = OtherPlugins::NextValue(*row.plugin, *row.setting, direction ? direction : 1);
                    if (!current())
                        return false;
                    if (value == OtherPlugins::Get(*row.plugin, *row.setting))
                        return false;
                    if (!current())
                        return false;
                    bool changed = OtherPlugins::Set(*row.plugin, *row.setting, value) == ASI_SETTINGS_OK;
                    if (changed && row.setting->apply == "restart")
                        restartRequired = true;
                    if (!current())
                        return changed;
                    restartNeeded = nullptr;
                    restartActive = false;
                    return changed;
                }
                case RowKind::Action:
                    return direction == 0 && OtherPlugins::Invoke(*row.plugin, *row.setting) >= 0;
                case RowKind::Preset:
                {
                    int count = int(std::size(PresetNames));
                    int shown = PresetShown(row.plugin);
                    if (!current())
                        return false;
                    int step = direction ? direction : 1;
                    shown = shown < 0 ? (step > 0 ? 0 : count - 1) : ((shown + step) % count + count) % count;
                    presetShown[row.plugin] = shown;
                    restartNeeded = nullptr;
                    restartActive = false;
                    return true;
                }
                case RowKind::ApplyPreset:
                {
                    if (direction != 0)
                    {
                        bool cancelled = confirmActive;
                        confirm = nullptr;
                        confirmActive = false;
                        return cancelled;
                    }
                    return ApplyShownPreset(row.plugin, current);
                }
                case RowKind::SettingsFile:
                    if (direction == 0 && host)
                        ShowSettingsFile();
                    return direction == 0;
                case RowKind::RestoreDefaults:
                {
                    if (direction != 0 || !row.plugin)
                        return false;
                    if (restoreConfirm != row.plugin)
                    {
                        restoreConfirm = row.plugin;
                        return true;
                    }
                    restoreConfirm = nullptr;
                    // Every setting row of the plugin's page; joined rows stay as they are
                    std::vector<const Setting*> settings;
                    for (auto& page : pages)
                    {
                        if (page.plugin != row.plugin)
                            continue;
                        for (auto& entry : page.rows)
                        {
                            if (entry.kind == RowKind::Setting)
                                settings.push_back(entry.setting);
                        }
                    }
                    for (auto setting : settings)
                    {
                        if (!current())
                            return true;
                        asi_settings::Slot slot;
                        slot.setting = setting;
                        asi_settings::SetDefault(slot);
                        auto value = asi_settings::FormatValue(slot);
                        if (asi_settings::text::IEquals(OtherPlugins::Get(*row.plugin, *setting), value))
                            continue;
                        if (OtherPlugins::Set(*row.plugin, *setting, value) == ASI_SETTINGS_OK && setting->apply == "restart")
                            restartRequired = true;
                    }
                    return true;
                }
            }
            return false;
        }

        // When a page is shown: its preset row starts at the preset the values match
        void PageShown(size_t page)
        {
            restoreConfirm = nullptr;
            confirm = nullptr;
            confirmActive = restartActive = false;
            restartNeeded = nullptr;
            if (page < pages.size())
            {
                pages[page].scroll = 0;
                presetShown.erase(pages[page].plugin);
                presetShown.erase(nullptr);
            }
        }

    private:
        static constexpr const char* PresetNames[] = { "menu.vanilla", "menu.recommended", "menu.enhanced" };

        std::shared_ptr<std::vector<LoadedPlugin>> plugins;
        const LoadedPlugin* host = nullptr;
        std::vector<Page> pages;
        Layout layout = Layout::Pages;
        size_t windowSize{ 1 };
        std::set<std::string> joinedIds;
        std::vector<Row> joinedRows;
        const LoadedPlugin* restoreConfirm = nullptr;   // the plugin whose Restore defaults row asks to confirm
        bool restartRequired = false;
        std::map<const LoadedPlugin*, int> presetShown;
        const LoadedPlugin* confirm = nullptr;          // the plugin whose apply row asks to confirm
        const LoadedPlugin* restartNeeded = nullptr;    // the plugin whose last preset needs a restart
        bool confirmActive = false, restartActive = false; // nullptr scopes the game-wide preset
        static inline std::map<HMODULE, int32_t> subscriptions;

        static void __cdecl OnChanged(void*, const char*)
        {
            Changed() = true;
        }

        // Accept on the preset (apply) row: the first press asks to confirm, the second applies the shown preset
        template<class Current>
        bool ApplyShownPreset(const LoadedPlugin* plugin, Current current)
        {
            int shown = PresetShown(plugin);
            bool matches = shown >= 0 && MatchesPreset(plugin, shown);
            if (!current())
                return false;
            if (shown < 0 || ((!confirmActive || confirm != plugin) && matches))
                return false;
            if (!confirmActive || confirm != plugin)
            {
                confirm = plugin;
                confirmActive = true;
                return true;
            }
            confirm = nullptr;
            confirmActive = false;
            bool restart = false;
            if (plugin)
                restart = OtherPlugins::ApplyPreset(*plugin, shown, current);
            else
            {
                auto snapshot = plugins;
                for (auto& loaded : *snapshot)
                {
                    if (OtherPlugins::HasPresets(loaded))
                        restart |= OtherPlugins::ApplyPreset(loaded, shown, current);
                    if (!current())
                        return true;
                }
            }
            restartRequired |= restart;
            if (!current())
                return true;
            restartActive = restart;
            restartNeeded = plugin;
            return true;
        }

        int PresetShown(const LoadedPlugin* plugin) const
        {
            auto it = presetShown.find(plugin);
            return it != presetShown.end() ? it->second : CurrentPreset(plugin);
        }

        int CurrentPreset(const LoadedPlugin* plugin) const
        {
            if (plugin)
                return OtherPlugins::CurrentPreset(*plugin);
            if (!plugins)
                return -1;
            for (int preset = 0; preset < 3; ++preset)
            {
                if (MatchesPreset(nullptr, preset))
                    return preset;
            }
            return -1;
        }

        bool MatchesPreset(const LoadedPlugin* plugin, int preset) const
        {
            if (plugin)
                return OtherPlugins::MatchesPreset(*plugin, preset);
            auto snapshot = plugins;
            if (!snapshot)
                return false;
            bool found = false;
            for (auto& loaded : *snapshot)
            {
                if (!OtherPlugins::HasPresets(loaded))
                    continue;
                found = true;
                if (!OtherPlugins::MatchesPreset(loaded, preset))
                    return false;
                if (snapshot != plugins)
                    return false;
            }
            return found;
        }

        void ShowSettingsFile() const
        {
            // The host's file, from its own runtime: the path the plugin uses (fallback included)
            wchar_t path[MAX_PATH * 4] = {};
            GetModuleFileNameW(host->module, path, DWORD(std::size(path)));
            asi_settings::Locations at;
            at.plugin = path;
            GetModuleFileNameW(nullptr, path, DWORD(std::size(path)));
            at.exe = path;
            auto file = asi_settings::NewestCfg(asi_settings::ResolveStorageFiles(host->file.description, at));
            std::thread([file]()
            {
                CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
                auto arguments = L"/select,\"" + file.wstring() + L"\"";
                ShellExecuteW(nullptr, L"open", L"explorer.exe", arguments.c_str(), nullptr, SW_SHOWNORMAL);
                CoUninitialize();
            }).detach();
        }

        // Plugins layout: one scrolling page per plugin
        void BuildPlugins(size_t pageRows)
        {
            pages.clear();
            joinedRows.clear();
            if (pageRows < 3)
                return;
            for (auto& plugin : *plugins)
            {
                auto& description = plugin.file.description;
                bool isHost = &plugin == host;

                // The description's pages by order; a setting on a page it doesn't list goes before the compatibility ones
                std::vector<const asi_settings::Page*> order;
                for (auto& page : description.pages)
                    order.push_back(&page);
                std::stable_sort(order.begin(), order.end(), [](auto a, auto b) { return a->order < b->order; });
                auto rank = [&](const Setting& setting) -> size_t
                {
                    for (size_t i = 0; i < order.size(); ++i)
                    {
                        if (asi_settings::text::IEquals(order[i]->id, setting.page))
                            return i;
                    }
                    return order.size();
                };
                std::vector<const Setting*> settings;
                for (auto& setting : description.settings)
                {
                    bool shown = setting.visibility == "menu" && (setting.HasValue() || setting.type == asi_settings::Type::Action);
                    if (shown && setting.type != asi_settings::Type::Info)
                        settings.push_back(&setting);
                }
                std::stable_sort(settings.begin(), settings.end(), [&](auto a, auto b)
                {
                    if (a->compatibility != b->compatibility)
                        return b->compatibility;
                    size_t ra = rank(*a), rb = rank(*b);
                    return ra != rb ? ra < rb : a->order < b->order;
                });

                std::vector<Row> rows;
                if (OtherPlugins::HasPresets(plugin))
                    rows.push_back({ RowKind::Preset, &plugin });
                bool resettable = false;
                for (auto setting : settings)
                {
                    Row row{ setting->type == asi_settings::Type::Action ? RowKind::Action : RowKind::Setting, &plugin, setting };
                    if (isHost && joinedIds.count(setting->id))
                    {
                        joinedRows.push_back(row);
                        continue;
                    }
                    resettable |= row.kind == RowKind::Setting;
                    rows.push_back(row);
                }
                if (resettable)
                    rows.push_back({ RowKind::RestoreDefaults, &plugin });
                if (rows.empty())
                    continue;

                auto name = OtherPlugins::Name(plugin);
                pages.push_back({ name, &plugin, std::move(rows) });
            }
        }

        // Older native hosts still consume whole pages through VisibleRows.
        void SplitPluginPages(size_t pageRows)
        {
            if (!pageRows)
                return;
            auto complete = std::move(pages);
            pages.clear();
            for (auto& source : complete)
            {
                size_t count = (source.rows.size() + pageRows - 1) / pageRows;
                for (size_t part = 0; part < count; ++part)
                {
                    Page page{ source.title, source.plugin, {} };
                    if (count > 1)
                        page.title += " (" + std::to_string(part + 1) + "/" + std::to_string(count) + ")";
                    auto begin = source.rows.begin() + part * pageRows;
                    page.rows.assign(begin, begin + std::min(pageRows, source.rows.size() - part * pageRows));
                    pages.push_back(std::move(page));
                }
            }
        }

        void Build(size_t pageRows)
        {
            pages.clear();
            if (pageRows < 3)
                return;
            for (auto& plugin : *plugins)
            {
                auto& description = plugin.file.description;
                bool isHost = &plugin == host;
                auto name = OtherPlugins::Name(plugin);
                std::vector<std::pair<std::string, std::vector<Row>>> lists;
                auto addPage = [&](const std::string& pageId, const std::string& title)
                {
                    std::vector<const Setting*> settings;
                    for (auto& setting : description.settings)
                    {
                        bool shown = setting.visibility == "menu" && (setting.HasValue() || setting.type == asi_settings::Type::Action);
                        if (shown && setting.type != asi_settings::Type::Info && asi_settings::text::IEquals(setting.page, pageId))
                            settings.push_back(&setting);
                    }
                    std::stable_sort(settings.begin(), settings.end(), [](auto a, auto b) { return a->order < b->order; });
                    std::vector<Row> rows;
                    for (auto setting : settings)
                        rows.push_back({ setting->type == asi_settings::Type::Action ? RowKind::Action : RowKind::Setting, &plugin, setting });
                    if (!rows.empty())
                        lists.emplace_back(title, std::move(rows));
                };
                if (description.pages.empty())
                    addPage("", name);
                for (auto& page : description.pages)
                    addPage(page.id, OtherPlugins::Text(plugin, page.title));
                if (lists.empty() && !isHost)
                    continue;

                bool presets = std::any_of(description.settings.begin(), description.settings.end(), [](const Setting& s) { return s.InPresets(); });
                if (isHost)
                {
                    // The fix's presets and its settings file on a page of their own, first
                    std::vector<Row> rows;
                    if (std::count_if(plugins->begin(), plugins->end(), OtherPlugins::HasPresets) > 1)
                    {
                        rows.push_back({ RowKind::Preset, nullptr });
                        rows.push_back({ RowKind::ApplyPreset, nullptr });
                    }
                    if (presets)
                    {
                        rows.push_back({ RowKind::Preset, &plugin });
                        rows.push_back({ RowKind::ApplyPreset, &plugin });
                    }
                    rows.push_back({ RowKind::SettingsFile, &plugin });
                    lists.insert(lists.begin(), { Text("menu.presets"), std::move(rows) });
                }
                else if (presets)
                {
                    auto& first = lists.front().second;
                    first.insert(first.begin(), { Row{ RowKind::Preset, &plugin }, Row{ RowKind::ApplyPreset, &plugin } });
                }

                for (auto& [title, rows] : lists)
                {
                    // Other plugins: their name, and the page's title when they have more than one
                    std::string pageTitle = title;
                    if (!isHost)
                    {
                        pageTitle = name;
                        if (lists.size() > 1 && !asi_settings::text::IEquals(title, name))
                            pageTitle += ": " + title;
                    }
                    size_t count = (rows.size() + pageRows - 1) / pageRows;
                    for (size_t i = 0; i < count; ++i)
                    {
                        Page page{ pageTitle, &plugin, {} };
                        if (count > 1)
                            page.title += " (" + std::to_string(i + 1) + "/" + std::to_string(count) + ")";
                        auto begin = rows.begin() + i * pageRows;
                        page.rows.assign(begin, begin + std::min(pageRows, rows.size() - i * pageRows));
                        pages.push_back(std::move(page));
                    }
                }
            }
        }
    };

    // Temporarily reserve the native wheel buttons for scrolling, leaving other mouse input intact.
    // Value bounds come from the native font after its scale and alignment are set.
    struct ValueAreas
    {
        struct Rect
        {
            float left, top, right, bottom;
        };
        std::vector<Rect> bounds;

        void Add(float x, float y, float width, float height, float padding)
        {
            bounds.push_back({ x - width - padding, y, x + padding, y + height });
        }

        bool Contains(float x, float y) const
        {
            for (const auto& rect : bounds)
            {
                if (x >= rect.left && x <= rect.right && y >= rect.top && y < rect.bottom)
                    return true;
            }
            return false;
        }
    };

    class WheelInput
    {
    public:
        uint8_t* buttons{ nullptr };
        uint8_t* oldButtons{ nullptr };

        int Begin(bool active)
        {
            if (!buttons)
                return 0;
            up = buttons[0];
            down = buttons[1];
            int direction = down && !lastDown ? 1 : up && !lastUp ? -1 : 0;
            lastUp = up;
            lastDown = down;
            reserved = active;
            if (reserved)
            {
                buttons[0] = buttons[1] = 0;
                if (oldButtons)
                {
                    oldUp = oldButtons[0];
                    oldDown = oldButtons[1];
                    oldButtons[0] = oldButtons[1] = 0;
                }
            }
            return active ? direction : 0;
        }

        void End()
        {
            if (reserved)
            {
                buttons[0] = up;
                buttons[1] = down;
                if (oldButtons)
                {
                    oldButtons[0] = oldUp;
                    oldButtons[1] = oldDown;
                }
                reserved = false;
            }
        }

    private:
        uint8_t up{ 0 }, down{ 0 }, oldUp{ 0 }, oldDown{ 0 };
        bool lastUp = false, lastDown = false, reserved = false;
    };

    // Native sprite triangles in the right margin, with height-scaled spacing at every aspect ratio.
    template<class Draw>
    void DrawIndicators(bool above, bool below, float x, float top, float bottom, float screenWidth, float scaleY, const uint8_t* colour, Draw draw)
    {
        for (int line = 0; line < 5; ++line)
        {
            float half = float(line + 1);
            if (above)
            {
                float rect[] = { screenWidth - (x + half) * scaleY, (top + line + 1) * scaleY, screenWidth - (x - half) * scaleY, (top + line) * scaleY };
                draw(rect, colour);
            }
            if (below)
            {
                float rect[] = { screenWidth - (x + half) * scaleY, (bottom - line + 1) * scaleY, screenWidth - (x - half) * scaleY, (bottom - line) * scaleY };
                draw(rect, colour);
            }
        }
    }

    // The fix's own text of a key in a language, while the pages aren't open (the "Plugins" row of the
    // Options screen); from the game's thread only
    inline std::string PluginText(asi_settings::Plugin& plugin, const std::string& language, const std::string& key)
    {
        static asi_settings::Texts texts = plugin.GetTexts();
        static std::string selected;
        if (selected != language)
        {
            texts.Select(language);
            selected = language;
        }
        return texts.Resolve("@" + key);
    }

    // The text of the GTA III and Vice City fonts (and of San Andreas, which uses the same table in 8 bits): ASCII
    // as it is, the accented letters of the European languages at the font's own positions (0x80 to 0xB0), anything
    // else as the nearest plain letter. upper: in capitals, like the games' own menu text. slash: the font has a "/"
    // (GTA III's menu font draws a cross there: a dash instead).
    inline std::u16string ToGameFont(const std::string& utf8, bool upper, bool slash = true)
    {
        static constexpr std::pair<char32_t, char16_t> Accents[] =
        {
            { U'À', 0x80 }, { U'Á', 0x81 }, { U'Â', 0x82 }, { U'Ä', 0x83 }, { U'Æ', 0x84 },
            { U'Ç', 0x85 }, { U'È', 0x86 }, { U'É', 0x87 }, { U'Ê', 0x88 }, { U'Ë', 0x89 },
            { U'Ì', 0x8A }, { U'Í', 0x8B }, { U'Î', 0x8C }, { U'Ï', 0x8D }, { U'Ò', 0x8E },
            { U'Ó', 0x8F }, { U'Ô', 0x90 }, { U'Ö', 0x91 }, { U'Ù', 0x92 }, { U'Ú', 0x93 },
            { U'Û', 0x94 }, { U'Ü', 0x95 }, { U'ß', 0x96 }, { U'à', 0x97 }, { U'á', 0x98 },
            { U'â', 0x99 }, { U'ä', 0x9A }, { U'æ', 0x9B }, { U'ç', 0x9C }, { U'è', 0x9D },
            { U'é', 0x9E }, { U'ê', 0x9F }, { U'ë', 0xA0 }, { U'ì', 0xA1 }, { U'í', 0xA2 },
            { U'î', 0xA3 }, { U'ï', 0xA4 }, { U'ò', 0xA5 }, { U'ó', 0xA6 }, { U'ô', 0xA7 },
            { U'ö', 0xA8 }, { U'ù', 0xA9 }, { U'ú', 0xAA }, { U'û', 0xAB }, { U'ü', 0xAC },
            { U'Ñ', 0xAD }, { U'ñ', 0xAE }, { U'¿', 0xAF }, { U'¡', 0xB0 },
        };
        std::u16string out;
        for (size_t i = 0; i < utf8.size();)
        {
            // UTF-8 to a code point
            char32_t c = uint8_t(utf8[i]);
            size_t length = c < 0x80 ? 1 : (c >> 5) == 6 ? 2 : (c >> 4) == 14 ? 3 : (c >> 3) == 30 ? 4 : 1;
            if (length > 1 && i + length <= utf8.size())
            {
                c &= (0x3F >> (length - 1));
                for (size_t k = 1; k < length; ++k)
                    c = (c << 6) | (uint8_t(utf8[i + k]) & 0x3F);
            }
            i += length;

            if (upper)
            {
                if (c >= U'a' && c <= U'z')
                    c -= 0x20;
                else if (c >= 0xE0 && c <= 0xFE && c != 0xF7)
                    c -= 0x20;
            }
            if (c == U'/' && !slash)
                c = U'-';
            if (c < 0x80)
            {
                out += char16_t(c);
                continue;
            }
            auto it = std::find_if(std::begin(Accents), std::end(Accents), [&](auto& a) { return a.first == c; });
            if (it != std::end(Accents))
                out += it->second;
            else if (c == 0x2013 || c == 0x2014)
                out += u'-';
            else if (c == 0x2019 || c == 0x2018)
                out += u'\'';
            else if (c == 0xB0)
                out += u'o';
            else
                out += u'?';
        }
        return out;
    }
}
