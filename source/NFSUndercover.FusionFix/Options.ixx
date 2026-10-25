module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "../Shared/NFS/PluginsMenu.hpp"
#include "../Shared/NFS/SettingsMenu.hpp"
#include "../Shared/NFS/Text.hpp"
#include "../Shared/NFS/TextLanguage.hpp"

export module Options;

import ComVars;
import Input;

// The plugins' settings in the game's options menus. The menus are a table of entries ({id, parent, label, unknown,
// data}) and every menu lists the entries of one parent, so the fix's entries go into a copy of the table:
//
// - Settings that belong next to one of the game's options are listed right after it (display mode under Resolution,
//   console gamma, SMAA and the frame limit in Advanced, the scenery and shadow settings in World, field of view and
//   HUD in Gameplay, button icons and dead zones in Controls). The game's Default Settings of Graphic Preferences also
//   sets the fix's settings under it to their defaults.
// - One "Plugins" entry goes into both lists of option categories (45, and 46 that the options screen shows in some
//   cases); the pause menu opens the same options. It lists the fix and then every other loaded plugin with settings
//   in menus; each opens its page: Preset (when it has presets), its settings by the pages of its description (as
//   sub-categories when they don't fit one list), Restore Defaults. This part of the table is built when Plugins is
//   opened.
//
// Categories need nothing else: the options' "selected" handler lists the children of any entry it doesn't know.
// Selecting a setting opens the game's page for one option with the state that's left unused (a page with a list of
// values), filled with the setting's values; accepting the page sets the value. Preset opens a page with the presets;
// accepting another one, and Restore Defaults, ask with the game's yes/no dialog of Default Settings. The highlighted
// entry's description, and the note that a changed setting applies after a restart, are shown in the menu's
// description line.

namespace
{
    struct TreeEntry
    {
        int32_t id;
        int32_t parent;
        uint32_t label;
        int32_t param;
        int32_t data;
    };
    static_assert(sizeof(TreeEntry) == 0x14);

    constexpr int32_t OptionsMenus[] = { 45, 46 };   // the lists of option categories
    constexpr int32_t PluginsId = 900;
    constexpr int32_t FirstPlacedId = 901;           // the fix's settings next to the game's options
    constexpr int32_t FirstPluginsId = 1000;         // the Plugins part, built when it's opened
    constexpr size_t MaxPluginsEntries = 2048;
    constexpr size_t MaxListRows = 9;                // a plugin's page larger than this is split by its pages
    constexpr int32_t PageState = 18;                // an option page no menu entry opens
    constexpr int32_t ConfirmState = 50;             // the yes/no dialog of Default Settings
    constexpr int32_t GraphicsMenus[] = { 62, 117, 118 };   // Graphic Preferences and its sub-categories
    constexpr uint32_t GameplayLabel = 0x0CA2ECA4;

    // The fix's settings next to the game's options: the menu, the game's entry they follow, the setting
    struct Placement
    {
        int32_t parent;
        int32_t after;
        const char* setting;
    };
    constexpr Placement Placements[] = {
        { 62, 116, "MAIN.WindowMode" },                   // Graphic Preferences: Resolution
        { 117, 126, "GRAPHICS.ConsoleGamma" },              // Advanced: Brightness
        { 117, 127, "GRAPHICS.AntiAliasing" },                      // Anti-Aliasing
        { 117, 129, "MAIN.FPSLimit" },                      // V-Sync
        { 118, 130, "GRAPHICS.ImproveSceneryLOD" },         // World: World Detail
        { 118, 130, "GRAPHICS.DisablePreculler" },
        { 118, 130, "GRAPHICS.BruteforceCulling" },
        { 118, 131, "GRAPHICS.ShadowResolution" },                // Shadows
        { 118, 132, "GRAPHICS.DisableMotionBlur" },         // World Effects
        { 66, 78, "MAIN.FOVFactor" },                       // Gameplay: Default Camera
        { 66, 78, "GRAPHICS.Scaling" },
        { 66, 84, "MAIN.HudAspectRatioConstraint" },        // GPS Mini-Map
        { 54, 67, "MAIN.GamepadIcons" },                    // Controls (keyboard): Configuration
        { 54, 67, "GAMEPAD.LeftStickDeadzone" },
        { 54, 67, "GAMEPAD.RightStickDeadzone" },
        { 55, 68, "MAIN.GamepadIcons" },                    // Controls (gamepad): Vibration
        { 55, 68, "GAMEPAD.LeftStickDeadzone" },
        { 55, 68, "GAMEPAD.RightStickDeadzone" },
        { 56, 70, "MAIN.GamepadIcons" },                    // Controls (wheel): Overall Force
        { 56, 70, "GAMEPAD.LeftStickDeadzone" },
        { 56, 70, "GAMEPAD.RightStickDeadzone" },
    };

    using asi_settings::LoadedPlugin;
    using asi_settings::Setting;

    enum class Kind { Category, Setting, Action, Preset, RestoreDefaults };

    struct Entry
    {
        int32_t id;
        Kind kind;
        const LoadedPlugin* plugin = nullptr;
        const Setting* setting = nullptr;
        std::string label;      // UTF-8, for the categories that aren't settings
        bool graphics = false;  // placed under Graphic Preferences
    };

    // The fix as a plugin of the menus, for the settings next to the game's options
    LoadedPlugin FixPlugin;
    std::vector<Entry> Placed;
    std::vector<std::string> PlacedIds;

    // The Plugins part, from the plugins found when it was opened
    std::shared_ptr<WFPPlugins::Snapshot> Plugins;
    std::vector<Entry> PluginEntries;

    Entry* FindEntry(int32_t id)
    {
        for (auto list : { &Placed, &PluginEntries })
        {
            for (auto& entry : *list)
            {
                if (entry.id == id)
                    return &entry;
            }
        }
        return nullptr;
    }

    // The game's language: its id and the table of languages
    int32_t* CurrentLanguage = nullptr;
    struct LanguageInfo
    {
        int32_t id;
        int32_t type;
        const char* name;
        const char* filter;
        void* locale;
    };
    static_assert(sizeof(LanguageInfo) == 0x14);
    LanguageInfo* Languages = nullptr;
    LanguageInfo* LanguagesEnd = nullptr;

    std::string FindLanguageName(int32_t id)
    {
        for (auto info = Languages; info && info < LanguagesEnd; ++info)
        {
            if (info->id == id && info->name)
            {
                std::string name = info->name;
                for (auto& c : name)
                    c = char(tolower(uint8_t(c)));
                return name == "mexican" ? "spanish" : name;
            }
        }
        return "english";
    }

    // The plugins' texts are chosen by language tag
    std::string LanguageTag(const std::string& language)
    {
        auto tag = WFPText::Detail::Tag(language);
        if (!tag)
            return "en";
        std::string result;
        for (auto p = tag; *p; ++p)
            result += char(*p);
        return result;
    }

    // Text
    std::string LanguageName = "english";
    std::unordered_map<uint32_t, std::string> Texts;     // label hash -> text in the font's encoding

    uint32_t TextHash(const std::string& key)
    {
        return bStringHash(("WFP_TEXT_" + key).c_str());
    }

    uint32_t EntryHash(int32_t id)
    {
        return TextHash("ENTRY_" + std::to_string(id));
    }

    // The Russian font has its own letters at 0x81 and up, in an order of its own
    std::string ToRussianFont(const std::string& text);

    std::string GameText(const std::string& text)
    {
        if (LanguageName == "russian")
            return ToRussianFont(text);
        return WFPText::ToEAFont(text, LanguageName);
    }

    std::string EntryLabel(const Entry& entry)
    {
        switch (entry.kind)
        {
            case Kind::Setting:
            case Kind::Action:
                return WFPPlugins::Label({ WFPPlugins::Row::Kind::Setting, entry.plugin, entry.setting });
            case Kind::Preset:
                return WFPText::Get("WFP_PRESET");
            case Kind::RestoreDefaults:
                return WFPText::Get("WFP_RESTOREDEFAULTS");
            default:
                return entry.label;
        }
    }

    void SetEntryTexts(const std::vector<Entry>& entries)
    {
        for (auto& entry : entries)
            Texts[EntryHash(entry.id)] = GameText(EntryLabel(entry));
    }

    // The texts follow the game's language (or the one chosen for the menus' texts)
    bool UpdateTexts()
    {
        static std::string current;
        auto name = WFPMenu::TextLanguage(FixSettings(), FindLanguageName(CurrentLanguage ? *CurrentLanguage : 0));
        if (name == current)
            return false;
        current = name;
        LanguageName = name;
        WFPText::SetLanguage(LanguageName);
        FixPlugin.file.texts.Select(LanguageTag(LanguageName));

        Texts.clear();
        Texts[TextHash("WFP_PLUGINS")] = GameText(WFPText::Get("WFP_PLUGINS"));
        SetEntryTexts(Placed);
        SetEntryTexts(PluginEntries);
        return true;
    }

    // The text shown in the description line and the dialog's question, set before the game asks for them
    std::string HelpText;
    std::string DialogText;

    // The search of a language table for a text by its hash
    SafetyHookInline shSearchString = {};
    const char* __cdecl SearchString(uint32_t hash, void* table)
    {
        UpdateTexts();
        if (hash == TextHash("HELP"))
            return HelpText.c_str();
        if (auto it = Texts.find(hash); it != Texts.end())
            return it->second.c_str();
        return shSearchString.unsafe_ccall<const char*>(hash, table);
    }

    // Values

    bool IsNumber(const Setting& setting)
    {
        return setting.type == asi_settings::Type::Float || setting.type == asi_settings::Type::Int;
    }

    bool SameValue(const Setting& setting, const std::string& a, const std::string& b)
    {
        if (IsNumber(setting) || (setting.type == asi_settings::Type::Choice && !setting.StringValued()) || setting.type == asi_settings::Type::Bool)
            return std::fabs(std::atof(a.c_str()) - std::atof(b.c_str())) < 1e-4;
        return asi_settings::text::IEquals(a, b);
    }

    // The setting's values for its page: the listed ones, or every step of its range. The value in effect comes first
    // when it isn't among them.
    std::vector<std::string> SettingValues(const LoadedPlugin& plugin, const Setting& setting)
    {
        auto values = OtherPlugins::ListedValues(plugin, setting);
        if (values.empty() && IsNumber(setting) && setting.min && setting.max && setting.step && *setting.step > 0.0)
        {
            int count = int(std::round((*setting.max - *setting.min) / *setting.step)) + 1;
            for (int i = 0; i < count && i < 1000; ++i)
                values.push_back(asi_settings::text::FormatNumber(std::round((*setting.min + i * *setting.step) * 1000.0) / 1000.0));
        }
        auto current = OtherPlugins::Get(plugin, setting);
        if (std::none_of(values.begin(), values.end(), [&](const std::string& v) { return SameValue(setting, v, current); }))
            values.insert(values.begin(), current);
        return values;
    }

    // The text of one of the setting's values (UTF-8)
    std::string ValueLabel(const LoadedPlugin& plugin, const Setting& setting, const std::string& value)
    {
        if (setting.type == asi_settings::Type::Bool)
            return WFPText::Get(std::atof(value.c_str()) != 0.0 ? "WFP_ON" : "WFP_OFF");
        bool numeric = !setting.StringValued();
        for (auto list : { &setting.choices, &setting.special, &setting.suggestions })
        {
            for (auto& entry : *list)
            {
                if (!entry.label.empty() && asi_settings::detail::JsonEquals(entry.value, value, numeric))
                    return OtherPlugins::Text(plugin, entry.label);
            }
        }
        if (!IsNumber(setting))
            return value;
        std::string text = value;
        if (setting.type == asi_settings::Type::Float)
        {
            int decimals = setting.decimals >= 0 ? setting.decimals : (setting.step && *setting.step < 1.0 ? 2 : 0);
            char buffer[32];
            snprintf(buffer, sizeof(buffer), "%.*f", decimals, std::atof(value.c_str()));
            text = buffer;
        }
        if (!setting.unit.empty())
            text += setting.unit == "%" || setting.unit == "\xC2\xB0" ? setting.unit : " " + setting.unit;
        return text;
    }

    // The open page: its entry, values and texts
    Entry* ActiveEntry = nullptr;
    bool bPageOpened = false;           // the page is opening: its value is the setting's
    bool bPresetPageCustom = false;     // the preset page lists "Custom" first
    std::vector<std::string> PageTexts;
    std::vector<const char*> PageLabels;
    std::vector<std::string> PageValues;
    uint8_t* PageScreen = nullptr;

    void BuildPage(const Entry& entry)
    {
        UpdateTexts();
        PageTexts.clear();
        PageValues.clear();
        if (entry.kind == Kind::Preset)
        {
            if (bPresetPageCustom)
                PageTexts.push_back(GameText(WFPText::Get("WFP_PRESET_CUSTOM")));
            for (auto name : WFPPlugins::PresetRow::Names)
                PageTexts.push_back(GameText(WFPText::Get(name)));
        }
        else if (entry.kind == Kind::Setting)
        {
            PageValues = SettingValues(*entry.plugin, *entry.setting);
            for (auto& value : PageValues)
                PageTexts.push_back(GameText(ValueLabel(*entry.plugin, *entry.setting, value)));
        }
        PageLabels.clear();
        for (auto& text : PageTexts)
            PageLabels.push_back(text.c_str());
    }

    int PageValueIndex(const Entry& entry)
    {
        if (entry.kind == Kind::Preset)
        {
            int preset = OtherPlugins::CurrentPreset(*entry.plugin);
            return preset < 0 ? 0 : preset + (bPresetPageCustom ? 1 : 0);
        }
        auto current = OtherPlugins::Get(*entry.plugin, *entry.setting);
        for (size_t i = 0; i < PageValues.size(); ++i)
        {
            if (SameValue(*entry.setting, PageValues[i], current))
                return int(i);
        }
        return 0;
    }

    // The preset a value of the preset page stands for, -1 for "Custom"
    int PagePreset(int index)
    {
        return index - (bPresetPageCustom ? 1 : 0);
    }

    // The options menus' state manager: its state (+4) is the open page
    void** StateManager = nullptr;
    int32_t& StateOf(void* manager)
    {
        return *reinterpret_cast<int32_t*>(reinterpret_cast<uintptr_t>(manager) + 4);
    }

    // FEOptionsScreen: the page's value when it opened and the value picked
    constexpr uintptr_t OriginalValue = 0x84;
    constexpr uintptr_t EditedValue = 0x88;

    int32_t& ScreenValue(uint8_t* screen, uintptr_t offset)
    {
        return *reinterpret_cast<int32_t*>(screen + offset);
    }

    // FEOptionsScreen::CreateItem: the page's control, with its list of values
    SafetyHookInline shCreateItem = {};
    void* __fastcall CreateItem(uint8_t* screen, void* edx, int type, const char** labels, int count, int id, int selected)
    {
        if (ActiveEntry && StateManager && StateOf(*StateManager) == PageState)
        {
            PageScreen = screen;
            BuildPage(*ActiveEntry);
            if (bPageOpened)
            {
                bPageOpened = false;
                ScreenValue(screen, OriginalValue) = ScreenValue(screen, EditedValue) = PageValueIndex(*ActiveEntry);
            }
            return shCreateItem.unsafe_thiscall<void*>(screen, 0, PageLabels.data(), int(PageLabels.size()), id, ScreenValue(screen, EditedValue));
        }
        return shCreateItem.unsafe_thiscall<void*>(screen, type, labels, count, id, selected);
    }

    // FEOptionsScreen::CommitChanges: the page's value is accepted
    SafetyHookInline shCommitChanges = {};
    void __fastcall CommitChanges(uint8_t* screen)
    {
        if (ActiveEntry && ActiveEntry->kind == Kind::Setting && StateManager && StateOf(*StateManager) == PageState)
        {
            auto edited = ScreenValue(screen, EditedValue);
            if (edited != ScreenValue(screen, OriginalValue) && edited >= 0 && edited < int(PageValues.size()))
                WFPPlugins::Set(*ActiveEntry->plugin, *ActiveEntry->setting, PageValues[edited]);
        }
        shCommitChanges.unsafe_thiscall(screen);
    }

    // The game's functions
    void* (__fastcall* GetScreen)(void* manager, void* edx, int id) = nullptr;
    void (__fastcall* AddScreenLevel)(void* screen, void* edx, int unknown) = nullptr;
    bool (__cdecl* CallScreenFunction)(void* screen, const char* name, char* result, unsigned count, ...) = nullptr;
    void (__cdecl* ShowYesNoDialog)(const char* text, bool unknown) = nullptr;
    void** ScreenManager = nullptr;

    // The state manager's handlers (vtable slots)
    constexpr int DialogResultSlot = 17;
    constexpr int HighlightSlot = 37;
    constexpr int SelectedSlot = 38;
    constexpr int AcceptSlot = 39;
    constexpr int ShowDialogSlot = 81;
    constexpr int StateChangedSlot = 83;
    void* Originals[84] = {};

    template<typename F>
    F Original(int slot)
    {
        return reinterpret_cast<F>(Originals[slot]);
    }

    // The game's texts break lines with a backslash and "n"
    constexpr const char* LineBreak = "\\n";

    // The description line: the entry's description; the restart warning while a change waits for a restart
    void ShowHelp(Entry* entry)
    {
        UpdateTexts();
        std::string text;
        if (entry)
        {
            if (entry->kind == Kind::Setting || entry->kind == Kind::Action)
                text = WFPPlugins::Help({ WFPPlugins::Row::Kind::Setting, entry->plugin, entry->setting });
            else if (entry->kind == Kind::Preset)
                text = WFPPlugins::Help({ WFPPlugins::Row::Kind::Preset, entry->plugin });
            else if (entry->kind == Kind::RestoreDefaults)
                text = WFPPlugins::Help({ WFPPlugins::Row::Kind::RestoreDefaults, entry->plugin });
        }
        auto warning = WFPPlugins::RestartWarning();
        if (!warning.empty())
            text = text.empty() ? warning : text + LineBreak + warning;
        if (text.empty() && !entry)
            return;
        HelpText = GameText(text);
        if (auto screen = GetScreen(*ScreenManager, nullptr, 39))
            CallScreenFunction(screen, "UpdatePauseMenuDesc", nullptr, 1, "~WFP_TEXT_HELP");
    }

    void __fastcall Highlight(void** manager, void* edx, int32_t id, int32_t data)
    {
        Original<void(__fastcall*)(void**, void*, int32_t, int32_t)>(HighlightSlot)(manager, edx, id, data);
        ShowHelp(FindEntry(id));
    }

    // The yes/no dialog of Default Settings, for a preset or Restore Defaults
    struct Confirmation
    {
        Kind kind = Kind::Category;     // Category: none
        const LoadedPlugin* plugin = nullptr;
        int preset = -1;
    } Pending;

    void Confirm(void** manager, Kind kind, const LoadedPlugin* plugin, int preset)
    {
        Pending = { kind, plugin, preset };
        if (kind == Kind::Preset)
            DialogText = WFPText::Get("WFP_PRESET") + ": " + WFPText::Get(WFPPlugins::PresetRow::Names[preset]) + LineBreak + WFPText::Get("WFP_PRESET_CONFIRM");
        else
            DialogText = WFPText::Get("WFP_RESTOREDEFAULTS_HELP") + LineBreak + WFPText::Get("WFP_PRESET_CONFIRM");
        DialogText = GameText(DialogText);

        // As the game's Default Settings: back to this state after the dialog
        auto vtable = *reinterpret_cast<uintptr_t**>(manager);
        auto state = reinterpret_cast<int32_t*>(manager);
        state[19] = state[1];
        state[5] = state[1];
        state[1] = ConfirmState;
        reinterpret_cast<void(__fastcall*)(void**, void*)>(vtable[StateChangedSlot])(manager, nullptr);
        reinterpret_cast<void(__fastcall*)(void**, void*)>(vtable[ShowDialogSlot])(manager, nullptr);
    }

    void __fastcall ShowDialog(void** manager, void* edx)
    {
        if (Pending.kind != Kind::Category && StateOf(manager) == ConfirmState)
        {
            ShowYesNoDialog(DialogText.c_str(), true);
            return;
        }
        Original<void(__fastcall*)(void**, void*)>(ShowDialogSlot)(manager, edx);
    }

    void __fastcall DialogResult(void** manager, void* edx, int32_t result)
    {
        if (StateOf(manager) == ConfirmState)
        {
            if (Pending.kind != Kind::Category)
            {
                auto pending = std::exchange(Pending, {});
                if (result)
                {
                    if (pending.kind == Kind::Preset)
                    {
                        if (OtherPlugins::ApplyPreset(*pending.plugin, pending.preset))
                            WFPPlugins::RestartNeeded() = true;
                    }
                    else
                    {
                        auto exclude = pending.plugin->module == FixPlugin.module ? PlacedIds : std::vector<std::string>{};
                        WFPPlugins::RestoreDefaults(*pending.plugin, exclude);
                    }
                    // The preset page goes back to the preset that's now in use, without asking to keep a change
                    if (PageScreen && ActiveEntry && ActiveEntry->kind == Kind::Preset)
                        ScreenValue(PageScreen, OriginalValue) = ScreenValue(PageScreen, EditedValue);
                }
                // The dialog's "no": back to the state before it
                Original<void(__fastcall*)(void**, void*, int32_t)>(DialogResultSlot)(manager, edx, 0);
                return;
            }
            if (result)
            {
                // Default Settings: the fix's settings under Graphic Preferences too
                std::vector<const Setting*> settings;
                for (auto& entry : Placed)
                {
                    if (entry.graphics && std::find(settings.begin(), settings.end(), entry.setting) == settings.end())
                        settings.push_back(entry.setting);
                }
                WFPPlugins::RestoreDefaults(FixPlugin, settings);
            }
        }
        Original<void(__fastcall*)(void**, void*, int32_t)>(DialogResultSlot)(manager, edx, result);
    }

    // The table of menu entries: the game's with the fix's settings next to its options, then the Plugins part
    std::vector<TreeEntry> Tree;
    size_t GameTreeSize = 0;
    ProtectedGameRef<uintptr_t> TreeEnds[2];

    void SetTreeEnd()
    {
        auto end = reinterpret_cast<uintptr_t>(Tree.data() + Tree.size());
        for (auto& ref : TreeEnds)
            ref = end;
    }

    void AddPluginEntry(int32_t parent, Entry entry)
    {
        if (Tree.size() >= Tree.capacity())
            return;
        entry.id = FirstPluginsId + int32_t(PluginEntries.size());
        Tree.push_back({ entry.id, parent, EntryHash(entry.id), 0, 0 });
        PluginEntries.push_back(std::move(entry));
    }

    // The plugins found now and their pages
    void BuildPlugins()
    {
        UpdateTexts();
        ActiveEntry = nullptr;
        Tree.resize(GameTreeSize);
        PluginEntries.clear();
        PluginEntries.reserve(MaxPluginsEntries);
        Plugins = WFPPlugins::Find(FixPlugin.module, LanguageTag(LanguageName));
        for (auto& plugin : Plugins->plugins)
        {
            AddPluginEntry(PluginsId, { 0, Kind::Category, &plugin, nullptr, WFPPlugins::Name(plugin) });
            int32_t pluginId = PluginEntries.back().id;
            auto exclude = plugin.module == FixPlugin.module ? PlacedIds : std::vector<std::string>{};
            auto sections = WFPPlugins::Sections(plugin, exclude);
            size_t count = 0;
            for (auto& section : sections)
            {
                std::erase_if(section.rows, [](const WFPPlugins::Row& row) { return !WFPPlugins::Visible(row); });
                count += section.rows.size();
            }

            auto add = [&](int32_t parent, const WFPPlugins::Row& row)
            {
                switch (row.kind)
                {
                    case WFPPlugins::Row::Kind::Preset:
                        AddPluginEntry(parent, { 0, Kind::Preset, &plugin });
                        break;
                    case WFPPlugins::Row::Kind::RestoreDefaults:
                        AddPluginEntry(parent, { 0, Kind::RestoreDefaults, &plugin });
                        break;
                    default:
                        AddPluginEntry(parent, { 0, row.kind == WFPPlugins::Row::Kind::Action ? Kind::Action : Kind::Setting, &plugin, row.setting });
                        break;
                }
            };

            // One list when it fits, else Preset, a sub-category for each page and Restore Defaults
            if (sections.size() == 1 || count <= MaxListRows)
            {
                for (auto& section : sections)
                {
                    for (auto& row : section.rows)
                        add(pluginId, row);
                }
                continue;
            }
            for (auto& section : sections)
            {
                int32_t sectionId = -1;
                for (auto& row : section.rows)
                {
                    if (row.kind != WFPPlugins::Row::Kind::Setting && row.kind != WFPPlugins::Row::Kind::Action)
                    {
                        add(pluginId, row);
                        continue;
                    }
                    if (sectionId < 0)
                    {
                        AddPluginEntry(pluginId, { 0, Kind::Category, &plugin, nullptr, section.title });
                        sectionId = PluginEntries.back().id;
                    }
                    add(sectionId, row);
                }
            }
        }
        SetEntryTexts(PluginEntries);
        SetTreeEnd();
    }

    // The options menus' "selected" handler: the plugins' settings open their page
    void __fastcall OptionSelected(void** manager, void* edx, int32_t id, int32_t data, int32_t button)
    {
        if (id == PluginsId)
            BuildPlugins();

        auto entry = FindEntry(id);
        if (entry && entry->kind != Kind::Category)
        {
            if (entry->kind == Kind::Action)
            {
                OtherPlugins::Invoke(*entry->plugin, *entry->setting);
                return;
            }
            if (entry->kind == Kind::RestoreDefaults)
            {
                Confirm(manager, Kind::RestoreDefaults, entry->plugin, -1);
                return;
            }
            // A setting that depends on another one being on keeps its value while it's off
            if (entry->kind == Kind::Setting && !WFPPlugins::Enabled({ WFPPlugins::Row::Kind::Setting, entry->plugin, entry->setting }))
                return;
            if (entry->kind == Kind::Preset)
                bPresetPageCustom = OtherPlugins::CurrentPreset(*entry->plugin) < 0;

            ActiveEntry = entry;
            bPageOpened = true;
            if (auto screen = GetScreen(*ScreenManager, nullptr, 39))
                AddScreenLevel(screen, nullptr, 0);
            using PushFn = void(__fastcall*)(void**, void*, int, int, int);
            reinterpret_cast<PushFn>((*reinterpret_cast<void***>(manager))[12])(manager, nullptr, 3, PageState, -1);
            return;
        }
        Original<void(__fastcall*)(void**, void*, int32_t, int32_t, int32_t)>(SelectedSlot)(manager, edx, id, data, button);
    }

    // The options menus' "accept" handler: on an option page it keeps the edited value and goes back to the list. On
    // the preset page another preset is applied after the dialog's yes.
    void __fastcall PadAccept(void** manager, void* edx)
    {
        if (ActiveEntry && ActiveEntry->kind == Kind::Preset && StateOf(manager) == PageState && PageScreen)
        {
            int preset = PagePreset(ScreenValue(PageScreen, EditedValue));
            if (preset >= 0 && preset != OtherPlugins::CurrentPreset(*ActiveEntry->plugin))
            {
                Confirm(manager, Kind::Preset, ActiveEntry->plugin, preset);
                return;
            }
            ScreenValue(PageScreen, EditedValue) = ScreenValue(PageScreen, OriginalValue);
        }
        Original<void(__fastcall*)(void**, void*)>(AcceptSlot)(manager, edx);
    }

    // The Russian font's letters, А..Я and а..я, at 0x81 and up; some are 0x80 and a second byte. Ё and ё are drawn
    // as Е and е, Ъ as ъ.
    std::string ToRussianFont(const std::string& text)
    {
        static const char* const upper[32] = {
            "\x9B", "\xD0", "\x9F", "\xB7", "\x9A", "\xA3", "\x80\x83", "\xB3", "\x98", "\xFE", "\xAC", "\xB6", "\xAF", "\x9D", "\x96", "\xA2",
            "\xA8", "\xA5", "\x9C", "\xB5", "\x80\x81", "\xF7", "\xFF", "\xDE", "\x80\x80", "\x80\x87", "\x80\x89", "\xBE", "\xF5", "\x80\x84", "\x80\x88", "\xD5",
        };
        static const char* const lower[32] = {
            "\x82", "\x95", "\x89", "\x94", "\x8C", "\x84", "\x9E", "\x90", "\x83", "\x97", "\x8B", "\x8A", "\x8F", "\x85", "\x81", "\x8D",
            "\x87", "\x88", "\x86", "\x8E", "\xF0", "\xAD", "\xB2", "\x99", "\xA6", "\xB8", "\x80\x89", "\x91", "\x93", "\xB9", "\xB1", "\x92",
        };
        std::string result;
        std::wstring other;
        auto flush = [&]()
        {
            if (other.empty())
                return;
            int size = WideCharToMultiByte(CP_UTF8, 0, other.data(), int(other.size()), nullptr, 0, nullptr, nullptr);
            std::string utf8(size, '\0');
            WideCharToMultiByte(CP_UTF8, 0, other.data(), int(other.size()), utf8.data(), size, nullptr, nullptr);
            result += WFPText::ToEAFont(utf8, "english");
            other.clear();
        };
        for (wchar_t c : WFPText::ToWide(text))
        {
            if (c == L'Ё')
                c = L'Е';
            else if (c == L'ё')
                c = L'е';
            if (c >= L'А' && c <= L'Я')
            {
                flush();
                result += upper[c - L'А'];
            }
            else if (c >= L'а' && c <= L'я')
            {
                flush();
                result += lower[c - L'а'];
            }
            else
                other += c;
        }
        flush();
        return result;
    }
}

class Options
{
public:
    Options()
    {
        WFP::onInitEvent() += []()
        {
            auto children = GetPattern("8B 7C 24 28 8B 6C 24 24 BE ? ? ? ? 39 6E 04");
            auto highlight = GetPattern("33 C9 B8 ? ? ? ? 8D 9B 00 00 00 00 39 38 74 ? 83 C0 14 83 C1 01 3D ? ? ? ? 7C");
            auto highlightParent = GetPattern("8D 14 89 8B 04 95 ? ? ? ?");
            auto stateManager = GetPattern("C7 06 ? ? ? ? E8 ? ? ? ? 88 5E 7C");
            auto commit = GetPattern("51 53 56 8B F1 57 8B BE 88 00 00 00");
            auto createItem = GetPattern("81 EC D8 04 00 00 A1 ? ? ? ? 33 C4 89 84 24 D4 04 00 00");
            auto addScreenLevel = GetPattern("6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 51 53 55 56 57 A1 ? ? ? ? 33 C4 50 8D 44 24 18 64 A3 00 00 00 00 8B E9 33 DB");
            auto getScreen = GetPattern("56 8B B1 30 01 00 00 33 C0 85 F6 57 7C 1B");
            // The screen manager: the global that's passed to the screen lookup, here for screen 39 (the code around it
            // differs between the builds)
            auto screenLookups = GetPattern("8B 0D ? ? ? ? 6A 27 E8");
            auto search = GetPattern("53 55 56 8B 74 24 14 8B 4E 10 83 E9 01 8B C1 99 2B C2 57 8B 7E 18");
            auto languages = GetPattern("8B 54 24 04 33 C9 B8 ? ? ? ? EB 03 8D 49 00 39 10 74 12");
            auto language = GetPattern("89 3D ? ? ? ? A3 ? ? ? ? E8");
            // The description line's update (a call of the screen's script function) and the Default Settings dialog
            auto description = GetPattern("68 ? ? ? ? 6A 01 6A 00 68 ? ? ? ? 55 E8");
            auto dialog = GetPattern("68 71 90 CC 7E E8 ? ? ? ? 83 C4 04 50 E8");
            if (children.empty() || highlight.empty() || highlightParent.empty() || stateManager.empty() || commit.empty() || createItem.empty() ||
                addScreenLevel.empty() || getScreen.empty() || screenLookups.empty() || search.empty() || languages.empty() || language.empty() ||
                description.empty() || dialog.empty())
                return;

            auto getChildren = reinterpret_cast<uintptr_t>(children.get_first(-0x24));
            auto tableBegin = *children.get_first<TreeEntry*>(9);
            auto tableEnd = *reinterpret_cast<TreeEntry**>(getChildren + 0x8C);
            if (*highlight.get_first<TreeEntry*>(3) != tableBegin || *highlight.get_first<TreeEntry*>(0x18) != tableEnd ||
                *highlightParent.get_first<uintptr_t>(6) != reinterpret_cast<uintptr_t>(tableBegin) + 4)
                return;

            AddScreenLevel = reinterpret_cast<decltype(AddScreenLevel)>(addScreenLevel.get_first());
            GetScreen = reinterpret_cast<decltype(GetScreen)>(getScreen.get_first());
            CallScreenFunction = reinterpret_cast<decltype(CallScreenFunction)>(injector::GetBranchDestination(description.get_first(15)).as_int());
            ShowYesNoDialog = reinterpret_cast<decltype(ShowYesNoDialog)>(injector::GetBranchDestination(dialog.get_first(14)).as_int());
            for (size_t i = 0; i < screenLookups.size() && !ScreenManager; ++i)
            {
                if (injector::GetBranchDestination(screenLookups.get(i).get<void>(8)).as_int() == reinterpret_cast<uintptr_t>(getScreen.get_first()))
                    ScreenManager = *screenLookups.get(i).get<void**>(2);
            }
            if (!ScreenManager)
                return;
            StateManager = *stateManager.get_first<void**>(0x16);
            Languages = *languages.get_first<LanguageInfo*>(7);
            LanguagesEnd = *languages.get_first<LanguageInfo*>(0x1B);
            CurrentLanguage = *language.get_first<int32_t*>(2);

            // Both lists of option categories must be the game's: gameplay is among them
            for (int32_t menu : OptionsMenus)
            {
                if (std::none_of(tableBegin, tableEnd, [&](const TreeEntry& entry) { return entry.parent == menu && entry.label == GameplayLabel; }))
                    return;
            }

            // The fix as a plugin of the menus
            HMODULE module = nullptr;
            GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCWSTR>(&FindEntry), &module);
            for (auto& plugin : asi_settings::FindLoadedPlugins(module, "en"))
            {
                if (plugin.module == module)
                    FixPlugin = std::move(plugin);
            }
            if (!FixPlugin.module)
                return;

            // The settings next to the game's options, each with one id wherever it's listed
            auto& settings = FixPlugin.file.description.settings;
            std::vector<std::pair<const Placement*, int32_t>> placements;
            for (auto& placement : Placements)
            {
                auto setting = std::find_if(settings.begin(), settings.end(), [&](const Setting& s) { return s.id == placement.setting; });
                bool listed = std::any_of(tableBegin, tableEnd, [&](const TreeEntry& e) { return e.id == placement.after && e.parent == placement.parent; });
                if (setting == settings.end() || !WFPPlugins::Shown(*setting) || !listed)
                    continue;
                auto entry = std::find_if(Placed.begin(), Placed.end(), [&](const Entry& e) { return e.setting == &*setting; });
                if (entry == Placed.end())
                {
                    Placed.push_back({ FirstPlacedId + int32_t(Placed.size()), Kind::Setting, &FixPlugin, &*setting });
                    PlacedIds.push_back(setting->id);
                    entry = Placed.end() - 1;
                }
                if (std::find(std::begin(GraphicsMenus), std::end(GraphicsMenus), placement.parent) != std::end(GraphicsMenus))
                    entry->graphics = true;
                placements.emplace_back(&placement, entry->id);
            }

            // The new table: the game's entries with the placed settings after theirs, Plugins in both lists of
            // categories, then room for the Plugins part
            Tree.reserve(size_t(tableEnd - tableBegin) + std::size(Placements) + std::size(OptionsMenus) + MaxPluginsEntries);
            for (auto entry = tableBegin; entry < tableEnd; ++entry)
            {
                Tree.push_back(*entry);
                for (auto& [placement, id] : placements)
                {
                    if (placement->parent == entry->parent && placement->after == entry->id)
                        Tree.push_back({ id, entry->parent, EntryHash(id), 0, 0 });
                }
            }
            for (int32_t menu : OptionsMenus)
                Tree.push_back({ PluginsId, menu, TextHash("WFP_PLUGINS"), 0, 0 });
            GameTreeSize = Tree.size();

            // The hooks first: without them the game keeps its own table
            shSearchString = safetyhook::create_inline(search.get_first(), SearchString);
            shCreateItem = safetyhook::create_inline(createItem.get_first(), CreateItem);
            shCommitChanges = safetyhook::create_inline(commit.get_first(), CommitChanges);
            if (!shSearchString || !shCreateItem || !shCommitChanges)
            {
                shCommitChanges.reset();
                shCreateItem.reset();
                shSearchString.reset();
                return;
            }

            auto begin = reinterpret_cast<uintptr_t>(Tree.data());
            for (auto address : { children.get_first<uintptr_t>(9), highlight.get_first<uintptr_t>(3) })
            {
                ProtectedGameRef<uintptr_t> ref(address);
                ref = begin;
            }
            ProtectedGameRef<uintptr_t> parentRef(highlightParent.get_first<uintptr_t>(6));
            parentRef = begin + 4;
            TreeEnds[0].SetAddress(reinterpret_cast<uintptr_t*>(getChildren + 0x8C));
            TreeEnds[1].SetAddress(highlight.get_first<uintptr_t>(0x18));
            SetTreeEnd();

            auto vtable = *stateManager.get_first<uintptr_t*>(2);
            for (auto [slot, function] : { std::pair{ SelectedSlot, reinterpret_cast<uintptr_t>(&OptionSelected) },
                { AcceptSlot, reinterpret_cast<uintptr_t>(&PadAccept) }, { HighlightSlot, reinterpret_cast<uintptr_t>(&Highlight) },
                { ShowDialogSlot, reinterpret_cast<uintptr_t>(&ShowDialog) }, { DialogResultSlot, reinterpret_cast<uintptr_t>(&DialogResult) } })
            {
                ProtectedGameRef<uintptr_t> ref(&vtable[slot]);
                Originals[slot] = reinterpret_cast<void*>(vtable[slot]);
                ref = function;
            }

        };
    }
} Options;
