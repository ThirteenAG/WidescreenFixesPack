#pragma once
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "AsiSettings/asi_settings.hpp"

// The other plugins' settings in the fix's Plugins menu (ASI Settings menu hosts): each time the
// options screen is opened, the fix claims the menu host role, finds every loaded module with a description and the
// AsiSettings_GetApi export, and shows their pages after its own, in the order of the specification (priority, then
// name). Values go through each plugin's table (Get, Set, GetChoices, GetState, Invoke, Subscribe); the plugin
// checks, applies and stores them. Texts are the plugin's own, in the game's language. Nothing here knows the game:
// the host's menu code draws the rows.
namespace OtherPlugins
{
    using asi_settings::LoadedPlugin;
    using asi_settings::Setting;

    // The plugins found when the menu was opened; the pages point into it, so it's kept as long as they are shown
    struct Snapshot
    {
        std::vector<LoadedPlugin> plugins;
    };

    // Set by plugins (from any thread) after a value or state changed; the menu shows the page again
    inline std::atomic<bool>& Changed()
    {
        static std::atomic<bool> changed = false;
        return changed;
    }

    inline void __cdecl OnChanged(void*, const char*)
    {
        Changed() = true;
    }

    // Functions past the plugin's table don't exist (older plugins have shorter tables)
    inline bool HasFunction(const AsiSettingsApi* api, size_t offset)
    {
        return api && api->size >= offset + sizeof(void*) && *reinterpret_cast<void* const*>(reinterpret_cast<const char*>(api) + offset);
    }

    // Every other plugin with a description, without the host itself. Without the host role (another plugin draws
    // the other plugins' pages) there are none.
    inline std::shared_ptr<Snapshot> Find(HMODULE host, std::string_view language)
    {
        auto snapshot = std::make_shared<Snapshot>();
        if (!asi_settings::ClaimHostRole())
            return snapshot;
        static std::map<HMODULE, int32_t> subscriptions;
        for (auto& plugin : asi_settings::FindLoadedPlugins(host, language))
        {
            if (plugin.module == host)
                continue;
            if (!subscriptions.count(plugin.module) && HasFunction(plugin.api, offsetof(AsiSettingsApi, Subscribe)))
            {
                int32_t token = plugin.api->Subscribe(OnChanged, nullptr);
                if (token > 0)
                    subscriptions[plugin.module] = token;
            }
            snapshot->plugins.push_back(std::move(plugin));
        }
        return snapshot;
    }

    inline std::string Text(const LoadedPlugin& plugin, const std::string& field)
    {
        return plugin.file.texts.Resolve(field);
    }

    inline std::string Name(const LoadedPlugin& plugin)
    {
        return Text(plugin, plugin.file.description.name);
    }

    // snprintf-like functions of the table, into a string
    template<typename F>
    inline std::string Fetch(F&& call)
    {
        char buffer[256];
        int32_t length = call(buffer, uint32_t(sizeof(buffer)));
        if (length < 0)
            return {};
        if (length < int32_t(sizeof(buffer)))
            return std::string(buffer, size_t(length));
        std::string text(size_t(length) + 1, '\0');
        length = call(text.data(), uint32_t(text.size()));
        text.resize(length >= 0 ? std::min(size_t(length), text.size() - 1) : 0);
        return text;
    }

    inline std::string Get(const LoadedPlugin& plugin, const Setting& setting)
    {
        return Fetch([&](char* buffer, uint32_t size) { return plugin.api->Get(setting.id.c_str(), buffer, size); });
    }

    inline int32_t Set(const LoadedPlugin& plugin, const Setting& setting, const std::string& value)
    {
        return plugin.api->Set(setting.id.c_str(), value.c_str());
    }

    // The plugin's values as the menu sees them, for conditions and presets
    inline std::vector<asi_settings::Slot> Values(const LoadedPlugin& plugin)
    {
        std::vector<asi_settings::Slot> slots;
        for (auto& setting : plugin.file.description.settings)
        {
            if (!setting.HasValue())
                continue;
            asi_settings::Slot slot;
            slot.setting = &setting;
            asi_settings::SetDefault(slot);
            asi_settings::ParseValue(slot, Get(plugin, setting), false);
            slots.push_back(std::move(slot));
        }
        return slots;
    }

    inline bool Evaluate(const asi_settings::Condition& condition, const std::vector<asi_settings::Slot>& slots)
    {
        return asi_settings::Evaluate(condition, [&](std::string_view id) -> const asi_settings::Slot*
        {
            for (auto& slot : slots)
            {
                if (asi_settings::text::IEquals(slot.setting->id, id))
                    return &slot;
            }
            return nullptr;
        });
    }

    // A setting shows when the description puts it into menus, its visibleIf holds and the plugin doesn't hide it
    inline bool Visible(const LoadedPlugin& plugin, const Setting& setting)
    {
        if (setting.visibility != "menu")
            return false;
        if (HasFunction(plugin.api, offsetof(AsiSettingsApi, GetState)) &&
            plugin.api->GetState(setting.id.c_str(), nullptr, 0) == ASI_SETTINGS_STATE_HIDDEN)
            return false;
        return !setting.visibleIf || Evaluate(*setting.visibleIf, Values(plugin));
    }

    // The plugin checks at run time whether a value can change.
    inline bool Enabled(const LoadedPlugin& plugin, const Setting& setting, bool)
    {
        return !setting.enabledIf || Evaluate(*setting.enabledIf, Values(plugin));
    }

    // The values a menu can step through, as text: the plugin's usable values (GetChoices), otherwise the choices
    // (or both values of a bool), special values and suggestions. Empty for a number that steps through its range.
    inline std::vector<std::string> ListedValues(const LoadedPlugin& plugin, const Setting& setting)
    {
        using asi_settings::detail::JsonToText;
        std::vector<std::string> values;
        bool listed = setting.type == asi_settings::Type::Bool || setting.type == asi_settings::Type::Choice ||
            !setting.special.empty() || !setting.suggestions.empty();
        if (!listed)
            return values;
        if (HasFunction(plugin.api, offsetof(AsiSettingsApi, GetChoices)))
        {
            auto json = Fetch([&](char* buffer, uint32_t size) { return plugin.api->GetChoices(setting.id.c_str(), buffer, size); });
            asi_settings::json::Value root;
            if (!json.empty() && asi_settings::json::Parse(json, root) && root.IsArray())
            {
                for (auto& item : root.items)
                {
                    if (auto value = item.Find("value"))
                        values.push_back(JsonToText(*value));
                }
                return values;
            }
        }
        if (setting.type == asi_settings::Type::Bool)
            return { "0", "1" };
        for (auto list : { &setting.choices, &setting.special, &setting.suggestions })
        {
            for (auto& entry : *list)
                values.push_back(JsonToText(entry.value));
        }
        return values;
    }

    // The value left or right of the one in effect: through the listed values, otherwise a step within the range
    inline std::string NextValue(const LoadedPlugin& plugin, const Setting& setting, int direction)
    {
        auto current = Get(plugin, setting);
        bool numeric = !setting.StringValued();
        auto values = ListedValues(plugin, setting);
        if (!values.empty())
        {
            auto it = std::find_if(values.begin(), values.end(), [&](const std::string& v)
            {
                asi_settings::json::Value value;
                if (numeric)
                {
                    value.kind = asi_settings::json::Value::Kind::Number;
                    value.number = std::atof(v.c_str());
                }
                else
                {
                    value.kind = asi_settings::json::Value::Kind::String;
                    value.string = v;
                }
                return asi_settings::detail::JsonEquals(value, current, numeric);
            });
            int count = int(values.size());
            // A value that isn't listed (a custom one from the settings file) continues from the first or the last
            int index = it != values.end() ? int(it - values.begin()) : (direction > 0 ? -1 : 0);
            return values[((index + direction) % count + count) % count];
        }
        if ((setting.type == asi_settings::Type::Float || setting.type == asi_settings::Type::Int) && setting.step)
        {
            double step = *setting.step;
            double value = std::round((std::atof(current.c_str()) + direction * step) / step) * step;
            if (setting.min)
                value = std::max(value, *setting.min);
            if (setting.max)
                value = std::min(value, *setting.max);
            return asi_settings::text::FormatNumber(value);
        }
        return current;
    }

    // The value in effect as the menu shows it (UTF-8); on and off are the host's own words
    inline std::string ValueText(const LoadedPlugin& plugin, const Setting& setting, const std::string& on, const std::string& off)
    {
        auto value = Get(plugin, setting);
        if (setting.type == asi_settings::Type::Bool)
        {
            int b = 0;
            return asi_settings::detail::ParseBool(value, b) && b ? on : off;
        }
        bool numeric = !setting.StringValued();
        for (auto list : { &setting.choices, &setting.special, &setting.suggestions })
        {
            for (auto& entry : *list)
            {
                if (!entry.label.empty() && asi_settings::detail::JsonEquals(entry.value, value, numeric))
                    return Text(plugin, entry.label);
            }
        }
        if (setting.type == asi_settings::Type::Float || setting.type == asi_settings::Type::Int)
        {
            if (setting.type == asi_settings::Type::Float)
            {
                int decimals = setting.decimals >= 0 ? setting.decimals : (setting.step && *setting.step < 1.0 ? 2 : 0);
                char text[32];
                snprintf(text, sizeof(text), "%.*f", decimals, std::atof(value.c_str()));
                value = text;
            }
            if (!setting.unit.empty())
                value += setting.unit == "%" || setting.unit == "\xC2\xB0" ? setting.unit : " " + setting.unit;
        }
        return value;
    }

    // Presets: the preset the plugin's preset settings all match, or -1 for Custom
    inline bool HasPresets(const LoadedPlugin& plugin)
    {
        auto& settings = plugin.file.description.settings;
        return std::any_of(settings.begin(), settings.end(), [](const Setting& s) { return s.InPresets() && s.HasValue(); });
    }

    // Whether a value of the description is the slot's, as the setting's type reads it
    inline bool Matches(const asi_settings::Slot& slot, const asi_settings::json::Value& value)
    {
        asi_settings::Slot wanted;
        wanted.setting = slot.setting;
        asi_settings::SetDefault(wanted);
        return asi_settings::ParseValue(wanted, asi_settings::detail::JsonToText(value), false) &&
            asi_settings::text::IEquals(asi_settings::FormatValue(wanted), asi_settings::FormatValue(slot));
    }

    inline int CurrentPreset(const LoadedPlugin& plugin)
    {
        if (!HasPresets(plugin))
            return -1;
        auto slots = Values(plugin);
        for (int preset = 0; preset < 3; ++preset)
        {
            bool all = std::all_of(slots.begin(), slots.end(), [&](const asi_settings::Slot& slot)
            {
                return !slot.setting->InPresets() || Matches(slot, (*slot.setting->presets)[preset]);
            });
            if (all)
                return preset;
        }
        return -1;
    }

    inline bool MatchesPreset(const LoadedPlugin& plugin, int preset)
    {
        if (preset < 0 || preset >= 3 || !HasPresets(plugin))
            return false;
        auto slots = Values(plugin);
        return std::all_of(slots.begin(), slots.end(), [&](const asi_settings::Slot& slot)
        {
            return !slot.setting->InPresets() || Matches(slot, (*slot.setting->presets)[preset]);
        });
    }

    // Sets the preset's values; true when a changed setting applies after a restart
    template<class Continue>
    inline bool ApplyPreset(const LoadedPlugin& plugin, int preset, Continue active)
    {
        bool restart = false;
        for (auto& slot : Values(plugin))
        {
            if (!active())
                break;
            auto& presets = slot.setting->presets;
            if (!slot.setting->InPresets() || Matches(slot, (*presets)[preset]))
                continue;
            if (plugin.api->Set(slot.setting->id.c_str(), asi_settings::detail::JsonToText((*presets)[preset]).c_str()) == ASI_SETTINGS_OK)
                restart |= slot.setting->apply == "restart";
        }
        return restart;
    }

    inline bool ApplyPreset(const LoadedPlugin& plugin, int preset)
    {
        return ApplyPreset(plugin, preset, []() { return true; });
    }

    inline int32_t Invoke(const LoadedPlugin& plugin, const Setting& setting)
    {
        return HasFunction(plugin.api, offsetof(AsiSettingsApi, Invoke)) ? plugin.api->Invoke(setting.id.c_str()) : ASI_SETTINGS_UNSUPPORTED;
    }

    // The rows of the plugins' pages, in the menu's page size: a plugin's first page starts with its preset selector
    // and its apply row, then come its settings by page and order. A page with more rows than fit continues on the
    // next page ("Name (2/3)"). Rows that are hidden when the page is shown leave their place empty.
    struct Row
    {
        enum class Kind { Preset, ApplyPreset, Setting, Action } kind = Kind::Setting;
        const LoadedPlugin* plugin = nullptr;
        const Setting* setting = nullptr;
    };
    struct Page
    {
        std::string title;      // UTF-8
        const LoadedPlugin* plugin = nullptr;
        std::vector<Row> rows;
    };

    inline std::vector<Page> Pages(const Snapshot& snapshot, size_t pageRows)
    {
        std::vector<Page> pages;
        if (pageRows < 3)
            return pages;
        for (auto& plugin : snapshot.plugins)
        {
            auto& description = plugin.file.description;
            auto name = Name(plugin);
            // The description's pages (one for a description without pages), each a list of rows
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
                    rows.push_back({ setting->type == asi_settings::Type::Action ? Row::Kind::Action : Row::Kind::Setting, &plugin, setting });
                if (!rows.empty())
                    lists.emplace_back(title, std::move(rows));
            };
            if (description.pages.empty())
                addPage("", name);
            for (auto& page : description.pages)
            {
                auto title = Text(plugin, page.title);
                addPage(page.id, title);
            }
            if (lists.empty())
                continue;
            if (HasPresets(plugin))
            {
                auto& first = lists.front().second;
                first.insert(first.begin(), { Row{ Row::Kind::Preset, &plugin }, Row{ Row::Kind::ApplyPreset, &plugin } });
            }

            for (auto& [title, rows] : lists)
            {
                // The plugin's name, and the page's title when the plugin has more than one and it says more
                std::string pageTitle = name;
                if (lists.size() > 1 && !asi_settings::text::IEquals(title, name))
                    pageTitle += ": " + title;
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
        return pages;
    }
}
