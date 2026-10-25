#pragma once
#if __has_include("../AsiSettings/asi_settings.hpp")
#include "../AsiSettings/asi_settings.hpp"
#else
#include "../asi_settings/asi_settings.hpp"
#endif
#include <set>

namespace asi_overlay
{
    // No storage implementation here: even preset changes go through the live API.
    inline bool HasApi(const AsiSettingsApi* api, size_t end)
    {
        return api && api->version / 1000 == ASI_SETTINGS_VERSION_MAJOR && api->size >= end;
    }
#define OVERLAY_HAS(api, member) (HasApi(api, offsetof(AsiSettingsApi, member) + sizeof((api)->member)) && (api)->member)

    struct Change
    {
        size_t plugin;
        std::string id, label, before, after, description;
        bool restart;
        std::string beforeText, afterText; // display only; `after` is the value that is set
    };

    class SettingsModel
    {
        // A plugin's settings file, parsed again only when it was written
        struct StoredFile
        {
            asi_settings::StorageFiles storage;
            mutable std::filesystem::path path;
            mutable FILETIME written{};
            mutable std::optional<asi_settings::CfgFile> cfg;

            mutable ULONGLONG checked = 0;

            const asi_settings::CfgFile* Read() const
            {
                // Called per setting and frame: look at the files at most four times a second
                if (checked && GetTickCount64() - checked < 250) return cfg ? &*cfg : nullptr;
                checked = GetTickCount64();
                auto newest = asi_settings::NewestCfg(storage);
                WIN32_FILE_ATTRIBUTE_DATA data{};
                if (newest.empty() || !GetFileAttributesExW(newest.c_str(), GetFileExInfoStandard, &data)) { cfg.reset(); return nullptr; }
                if (!cfg || newest != path || CompareFileTime(&data.ftLastWriteTime, &written) != 0)
                {
                    path = newest; written = data.ftLastWriteTime;
                    auto text = asi_settings::ReadFileText(newest);
                    if (text) cfg = asi_settings::ParseCfg(*text); else cfg.reset();
                }
                return cfg ? &*cfg : nullptr;
            }
        };
        std::vector<StoredFile> files;

    public:
        std::vector<asi_settings::LoadedPlugin> plugins;
        std::string language = "en", message;
        std::set<std::string> restartChanges;

        static bool Read(const AsiSettingsApi* api, const std::string& id, std::string& value)
        {
            if (!OVERLAY_HAS(api, Get)) return false;
            return asi_settings::detail::CallText([&](char* b, uint32_t n) { return api->Get(id.c_str(), b, n); }, value) >= 0;
        }

        void Discover(HMODULE host, std::string fallback)
        {
            plugins = asi_settings::FindLoadedPlugins(host, fallback);
            // Where each plugin keeps its settings file, for the stored values (Stored)
            files.assign(plugins.size(), {});
            wchar_t path[MAX_PATH * 4] = {};
            GetModuleFileNameW(nullptr, path, DWORD(std::size(path)));
            asi_settings::Locations at;
            at.exe = path;
            for (size_t i = 0; i < plugins.size(); ++i)
            {
                GetModuleFileNameW(plugins[i].module, path, DWORD(std::size(path)));
                at.plugin = path;
                files[i].storage = asi_settings::ResolveStorageFiles(plugins[i].file.description, at);
            }
            for (auto& p : plugins)
            {
                if (p.module != host) continue;
                std::string override;
                if (Read(p.api, "MAIN.TextLanguage", override) && !override.empty() && override != "auto") fallback = override;
            }
            SetLanguage(fallback);
        }

        void SetLanguage(const std::string& tag)
        {
            language = tag;
            for (auto& p : plugins) p.file.texts.Select(tag);
        }

        // Discover resolves the files; hosts that fill `plugins` themselves (and tests) set them here
        void UseStorage(size_t index, asi_settings::StorageFiles storage)
        {
            if (files.size() < plugins.size()) files.resize(plugins.size());
            if (index < files.size()) { files[index] = {}; files[index].storage = std::move(storage); }
        }

        // The player's choice of a setting: Get returns the value in effect, which differs
        // when the choice can't be used now (DLAA without its DLLs runs as SMAA). The runtime interface has no call
        // for the choice, so it is read from the plugin's settings file, read-only, as editors do; the plugin wrote
        // it. Settings not in the file (or not stored in a cfg) fall back to the value in effect.
        bool Stored(size_t index, const asi_settings::Setting& s, std::string& value) const
        {
            if (index < files.size() && s.storage == "cfg")
                if (auto cfg = files[index].Read())
                {
                    const std::string* text = cfg->Find(s.section, s.key);
                    for (size_t i = 0; !text && i < s.aliases.size(); ++i)
                        if (auto dot = s.aliases[i].find('.'); dot != std::string::npos)
                            text = cfg->Find(s.aliases[i].substr(0, dot), s.aliases[i].substr(dot + 1));
                    asi_settings::Slot slot;
                    slot.setting = &s;
                    if (text && asi_settings::ParseValue(slot, *text, true)) { value = asi_settings::FormatValue(slot); return true; }
                }
            return Read(plugins[index].api, s.id, value);
        }

        // Values in effect (stored = false; for conditions and controls) or the player's choices (for presets)
        std::vector<asi_settings::Slot> Snapshot(size_t index, bool stored = false) const
        {
            std::vector<asi_settings::Slot> values;
            for (auto& s : plugins[index].file.description.settings)
            {
                if (!s.HasValue()) continue;
                std::string value;
                if (!(stored ? Stored(index, s, value) : Read(plugins[index].api, s.id, value))) continue;
                asi_settings::Slot slot;
                slot.setting = &s;
                if (asi_settings::ParseValue(slot, value, false)) values.push_back(std::move(slot));
            }
            return values;
        }

        static bool Condition(const std::optional<asi_settings::Condition>& condition, const std::vector<asi_settings::Slot>& values)
        {
            return !condition || asi_settings::Evaluate(*condition, [&](std::string_view id) -> const asi_settings::Slot*
            {
                for (auto& v : values) if (asi_settings::text::IEquals(v.setting->id, id)) return &v;
                return nullptr;
            });
        }

        static bool Hidden(const AsiSettingsApi* api, const asi_settings::Setting& s)
        {
            if (!OVERLAY_HAS(api, GetState)) return false;
            char reason[4097]{};
            auto state = api->GetState(s.id.c_str(), reason, sizeof(reason));
            return state >= 0 && (state & ASI_SETTINGS_STATE_HIDDEN);
        }

        static std::vector<asi_settings::Labelled> Choices(const AsiSettingsApi* api, const asi_settings::Setting& s)
        {
            // Without a runtime list, a choice's special values join its dropdown (no separate list is drawn).
            auto described = s.choices;
            if (s.type == asi_settings::Type::Choice) described.insert(described.end(), s.special.begin(), s.special.end());
            if (!OVERLAY_HAS(api, GetChoices)) return described;
            std::string source;
            auto result = asi_settings::detail::CallText([&](char* b, uint32_t n) { return api->GetChoices(s.id.c_str(), b, n); }, source);
            if (result < 0) return s.dynamic ? std::vector<asi_settings::Labelled>{} : described;
            asi_settings::json::Value array;
            if (!asi_settings::json::Parse(source, array) || !array.IsArray()) return {};
            std::vector<asi_settings::Labelled> choices;
            for (auto& item : array.items)
            {
                asi_settings::Labelled choice;
                if (item.IsObject())
                {
                    auto value = item.Find("value");
                    if (!value) continue;
                    choice.value = *value;
                    if (auto label = item.Find("label"); label && label->IsString()) choice.label = label->string;
                }
                else choice.value = item;
                // Runtime lists may return special values (e.g. a debug view) without labels.
                if (choice.label.empty())
                {
                    for (auto list : { &s.choices, &s.special, &s.suggestions })
                        for (auto& defined : *list)
                            if (choice.label.empty() && asi_settings::detail::JsonToText(defined.value) == asi_settings::detail::JsonToText(choice.value)) choice.label = defined.label;
                }
                choices.push_back(std::move(choice));
            }
            return choices;
        }

        bool Set(size_t index, const asi_settings::Setting& s, const std::string& value)
        {
            auto api = plugins[index].api;
            auto result = OVERLAY_HAS(api, Set) ? api->Set(s.id.c_str(), value.c_str()) : ASI_SETTINGS_UNSUPPORTED;
            if (result == ASI_SETTINGS_OK)
            {
                message.clear();
                if (s.apply == "restart") restartChanges.insert(plugins[index].file.id + ":" + s.id);
                return true;
            }
            message = plugins[index].file.texts.Resolve(s.label) + ": " +
                (result == ASI_SETTINGS_NOT_NOW ? "Cannot be used now." : result == ASI_SETTINGS_INVALID_VALUE ? "Invalid value." : "The plugin refused this change.");
            return false;
        }

        // Readable value for change lists: On/Off and choice labels instead of raw numbers.
        std::string Display(size_t index, const asi_settings::Setting& s, const std::string& value) const
        {
            auto& texts = plugins[index].file.texts;
            if (s.type == asi_settings::Type::Bool)
            {
                int b = 0;
                if (asi_settings::detail::ParseBool(value, b)) return b ? "On" : "Off";
            }
            for (auto list : { &s.choices, &s.special, &s.suggestions })
                for (auto& c : *list)
                    if (!c.label.empty() && asi_settings::detail::JsonEquals(c.value, value, !s.StringValued())) return texts.Resolve(c.label);
            return value.empty() ? "\"\"" : value + (s.unit.empty() ? "" : " " + s.unit);
        }

        std::vector<Change> PresetChanges(int preset, int onlyPlugin = -1) const
        {
            std::vector<Change> changes;
            for (size_t i = 0; i < plugins.size(); ++i)
            {
                if (onlyPlugin >= 0 && i != size_t(onlyPlugin)) continue;
                auto& p = plugins[i];
                for (auto& s : p.file.description.settings)
                {
                    if (!s.InPresets()) continue;
                    // From the player's choice, as the plugin's own preset compares; the value in effect only as a note
                    std::string before, running;
                    if (!Stored(i, s, before)) continue;
                    auto after = asi_settings::detail::JsonToText((*s.presets)[preset]);
                    if (asi_settings::detail::JsonEquals((*s.presets)[preset], before, !s.StringValued())) continue;
                    auto beforeText = Display(i, s, before);
                    if (Read(p.api, s.id, running) && Display(i, s, running) != beforeText)
                        beforeText += " (running: " + Display(i, s, running) + ")";
                    changes.push_back({ i, s.id, p.file.texts.Resolve(s.label), before, after, p.file.texts.Resolve(s.description), s.apply == "restart",
                        beforeText, Display(i, s, after) });
                }
            }
            return changes;
        }

        void Apply(const std::vector<Change>& changes)
        {
            std::string errors;
            for (auto& change : changes)
                if (auto s = plugins[change.plugin].file.description.Find(change.id); s && !Set(change.plugin, *s, change.after)) errors += message + "\n";
            message = errors;
        }
    };
}
