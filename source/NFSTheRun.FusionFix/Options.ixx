module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/NFS/PluginsMenu.hpp"
#include "Shared/NFS/TextLanguage.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"
#include <unordered_set>

export module Options;

import ComVars;

// The Plugins row in the game's Settings menu. The game's menus are tree menus from its data (UITreeMenuAsset): a list of
// items, each a label, a submenu (UIMenuItem), a toggle with its choices (UIToggleTreeMenuItem) or a slider, that the
// game sends to its menu screen as a whole. The rows show values from the UI's data store, by key, and write the value
// the player picks back there. The fix adds one submenu to the Settings menu, made of copies of the menu's own items:
// Plugins, a page for the fix and each other plugin with settings in menus, each with its Preset row and its settings
// (sections in submenus when there are several), every setting a toggle with its values. The values the player picks
// are read back from the data store and set through the plugin.
namespace
{
    // The engine's type information of a data object, for its type name and size
    struct ClassInfo
    {
        const char* name;
        uint16_t flags;
        uint16_t size;
    };

    const ClassInfo* InfoOf(uintptr_t object)
    {
        auto typeInfo = (*(uintptr_t(__thiscall**)(uintptr_t))(*(uintptr_t*)object))(object);
        return typeInfo ? *(const ClassInfo**)typeInfo : nullptr;
    }

    bool IsType(uintptr_t object, std::string_view name)
    {
        auto info = InfoOf(object);
        return info && info->name && name == info->name;
    }

    // Field offsets of the menu objects
    namespace Menu
    {
        constexpr size_t Items = 0x18;              // UITreeMenuAsset: array of items
        constexpr size_t DefaultHighlighted = 0x1C;
    }

    namespace Item
    {
        constexpr size_t Label = 0x0C;              // UITreeMenuItem
        constexpr size_t Data = 0x10;
        constexpr size_t Description = 0x14;
        constexpr size_t IsLocked = 0x18;           // UIBoolDataSourceInfo: category, key, invert
        constexpr size_t IsLockedHelpText = 0x24;
        constexpr size_t IsEnabled = 0x28;
        constexpr size_t IsDisabledHelpText = 0x34;
        constexpr size_t Visibility = 0x38;
        constexpr size_t SubMenu = 0x44;            // UIMenuItem
        constexpr size_t StaticItems = 0x44;        // UIToggleTreeMenuItem: array of { label, data }
        constexpr size_t DynamicItems = 0x48;
        constexpr size_t CurrentValue = 0x50;       // UIDataSourceInfo: category, key
    }

    template<typename T>
    T& At(uintptr_t object, size_t offset)
    {
        return *(T*)(object + offset);
    }

    // The engine's arrays: the element count before the first element
    template<typename T>
    uint32_t Count(T* array)
    {
        return array ? ((uint32_t*)array)[-1] : 0;
    }

    // The UI's data store: values by key, read by the menu screens
    namespace UIData
    {
        void** pManager = nullptr;
        void* (__thiscall* MakeString)(void* value, const char* text) = nullptr;
        void (__thiscall* Destroy)(void* value) = nullptr;

        // A value of the store: a variant with its own string
        struct Value
        {
            alignas(8) uint8_t bytes[0x30] = {};

            explicit Value(const char* text = "")
            {
                MakeString(bytes, text);
            }

            ~Value()
            {
                Destroy(bytes);
            }

            int Type() const
            {
                return *(const int*)(bytes + 0x18);
            }

            std::string String() const
            {
                if (Type() != 7)
                    return {};
                auto begin = *(const char* const*)(bytes + 8);
                auto end = *(const char* const*)(bytes + 0xC);
                return begin && end > begin ? std::string(begin, end) : std::string();
            }
        };

        bool Ready()
        {
            return pManager && *pManager && MakeString && Destroy;
        }

        bool Get(uint32_t key, std::string& text)
        {
            if (!Ready())
                return false;
            Value value;
            auto manager = *pManager;
            (*(void(__thiscall**)(void*, uint32_t, void*))(*(uintptr_t*)manager + 0x48))(manager, key, value.bytes);
            if (value.Type() != 7)
                return false;
            text = value.String();
            return true;
        }

        void Set(uint32_t key, Value& value)
        {
            auto manager = *pManager;
            (*(void(__thiscall**)(void*, uint32_t, void*, int))(*(uintptr_t*)manager + 0x4C))(manager, key, value.bytes, 0);
        }

        void SetString(uint32_t key, const std::string& text)
        {
            if (!Ready())
                return;
            Value value(text.c_str());
            Set(key, value);
        }
    }

    // Texts and objects the game's menus point to; they live as long as the game. The menus' text is 8-bit,
    // Windows-1252.
    const char* Text(const std::string& utf8)
    {
        static std::unordered_set<std::string> strings;
        return strings.insert(WFPText::ToCodePage(utf8, 1252)).first->c_str();
    }

    const char* Intern(const std::string& text)
    {
        static std::unordered_set<std::string> strings;
        return strings.insert(text).first->c_str();
    }

    uintptr_t Clone(uintptr_t object)
    {
        auto info = InfoOf(object);
        auto size = info ? info->size : 0;
        auto copy = (uintptr_t)_aligned_malloc(std::max<size_t>(size, 0x60), 16);
        memcpy((void*)copy, (void*)object, size);
        return copy;
    }

    // An engine array of count elements (kept: the menus may still point to an older one)
    template<typename T>
    T* MakeArray(const std::vector<T>& elements)
    {
        auto memory = (uint8_t*)_aligned_malloc(16 + sizeof(T) * std::max<size_t>(elements.size(), 1), 16);
        *(uint32_t*)(memory + 12) = uint32_t(elements.size());
        auto array = (T*)(memory + 16);
        std::copy(elements.begin(), elements.end(), array);
        return array;
    }

    uint32_t KeyOf(std::string_view name)
    {
        // FNV-1a, in a range the game's own keys aren't likely to use
        uint32_t hash = 2166136261u;
        for (char c : name)
            hash = (hash ^ uint8_t(c)) * 16777619u;
        return hash;
    }

    std::string LanguageName(int language)
    {
        static const char* names[] = { "english", "french", "german", "spanish", "italian", "japanese", "russian",
            "polish", "dutch", "portuguese", "chinese", "korean", "czech", "spanish" };
        return language >= 0 && language < int(std::size(names)) ? names[language] : "english";
    }

    std::string LanguageTag(int language)
    {
        static const char* tags[] = { "en", "fr", "de", "es", "it", "ja", "ru", "pl", "nl", "pt", "zh-TW", "ko", "cs", "es-MX" };
        return language >= 0 && language < int(std::size(tags)) ? tags[language] : "en";
    }

    int (__cdecl* GetGameLanguage)() = nullptr;

    // A row of the Plugins pages with a value in the data store
    struct Row
    {
        enum class Kind { Preset, Setting } kind = Kind::Setting;
        const asi_settings::LoadedPlugin* plugin = nullptr;
        const asi_settings::Setting* setting = nullptr;
        uint32_t key = 0;               // the value: the data of one of the choices
        uintptr_t item = 0;
        std::vector<std::string> data;  // the choices' data, in their order
        std::string written;            // the data last written to the store
    };

    struct Pages
    {
        std::shared_ptr<WFPPlugins::Snapshot> snapshot;
        std::vector<Row> rows;
        uintptr_t pluginsItem = 0;
    };

    Pages Current;
    std::recursive_mutex Mutex;

    // Templates: the Settings menu, one of its submenus and a toggle row
    uintptr_t MenuTemplate = 0, SubMenuTemplate = 0, ToggleTemplate = 0;

    // The objects of the Plugins pages, kept by name so that a menu built again uses the same ones
    uintptr_t Object(uintptr_t templ, const std::string& name)
    {
        static std::map<std::string, uintptr_t> objects;
        auto& object = objects[name];
        if (!object)
            object = Clone(templ);
        else
            memcpy((void*)object, (void*)templ, InfoOf(templ)->size);
        return object;
    }

    void Clear(uintptr_t item)
    {
        At<const char*>(item, Item::Data) = "";
        At<const char*>(item, Item::Description) = "";
        memset((void*)(item + Item::IsLocked), 0, 12);
        At<const char*>(item, Item::IsLockedHelpText) = "";
        memset((void*)(item + Item::IsEnabled), 0, 12);
        At<const char*>(item, Item::IsDisabledHelpText) = "";
        memset((void*)(item + Item::Visibility), 0, 12);
    }

    std::string Upper(const std::string& text)
    {
        return WFPText::Upper(text);
    }

    // The help line is one line in a box as wide as its text: a longer text ends at a word with an ellipsis
    std::string HelpText(const std::string& text, size_t width = 140)
    {
        size_t letters = 0;
        size_t lastSpace = std::string::npos;
        for (size_t i = 0; i < text.size(); ++i)
        {
            if ((uint8_t(text[i]) & 0xC0) == 0x80)
                continue;
            if (text[i] == ' ')
                lastSpace = i;
            if (++letters > width)
                return text.substr(0, lastSpace != std::string::npos ? lastSpace : i) + "...";
        }
        return text;
    }

    // MAIN.CheckForUpdates: a newer release of the fix
    std::mutex NoticeMutex;
    std::string UpdateNotice;

    // The help of the Plugins row and the plugins' rows: a newer release of the fix, or the restart warning when a
    // changed setting waits for a restart
    std::string MenuHelp(const std::string& text)
    {
        {
            std::lock_guard lock(NoticeMutex);
            if (!UpdateNotice.empty())
                return HelpText(UpdateNotice);
        }
        auto warning = WFPPlugins::RestartWarning();
        return HelpText(warning.empty() ? text : warning);
    }

    // The values a setting's row steps through: the listed ones, or its range (at most about a hundred steps), and the
    // value in effect
    std::vector<std::string> ValuesOf(const asi_settings::LoadedPlugin& plugin, const asi_settings::Setting& setting)
    {
        auto values = OtherPlugins::ListedValues(plugin, setting);
        auto current = OtherPlugins::Get(plugin, setting);
        bool numeric = !setting.StringValued();
        if (values.empty() && numeric && setting.min && setting.max && setting.step && *setting.step > 0.0)
        {
            double min = *setting.min, max = *setting.max, step = *setting.step;
            size_t count = size_t(std::floor((max - min) / step + 0.5)) + 1;
            size_t stride = std::max<size_t>(1, (count + 99) / 100);
            for (size_t i = 0; i < count; i += stride)
                values.push_back(asi_settings::text::FormatNumber(min + double(i) * step));
            if ((count - 1) % stride)
                values.push_back(asi_settings::text::FormatNumber(max));
        }
        auto equal = [&](const std::string& a, const std::string& b)
        {
            if (numeric)
                return std::fabs(std::atof(a.c_str()) - std::atof(b.c_str())) <= 1e-6 * std::max(1.0, std::fabs(std::atof(a.c_str())));
            return asi_settings::text::IEquals(a, b);
        };
        if (std::none_of(values.begin(), values.end(), [&](const std::string& v) { return equal(v, current); }))
        {
            auto at = values.end();
            if (numeric)
                at = std::find_if(values.begin(), values.end(), [&](const std::string& v) { return std::atof(v.c_str()) > std::atof(current.c_str()); });
            values.insert(at, current);
        }
        return values;
    }

    // The choice of a row that the value in effect is
    std::string CurrentData(const Row& row)
    {
        if (row.kind == Row::Kind::Preset)
            return std::to_string(OtherPlugins::CurrentPreset(*row.plugin));
        auto current = OtherPlugins::Get(*row.plugin, *row.setting);
        bool numeric = !row.setting->StringValued();
        for (auto& data : row.data)
        {
            if (numeric ? std::fabs(std::atof(data.c_str()) - std::atof(current.c_str())) <= 1e-6 * std::max(1.0, std::fabs(std::atof(current.c_str())))
                : asi_settings::text::IEquals(data, current))
                return data;
        }
        return current;
    }

    // The store's values of the rows, after the menus were built or a value changed
    void WriteValues()
    {
        for (auto& row : Current.rows)
        {
            row.written = CurrentData(row);
            UIData::SetString(row.key, row.written);
        }
    }

    // UIToggleTreeOption: a choice of a toggle row, its text and its value in the data store
    struct Choice
    {
        const char* label;
        const char* data;
    };

    uintptr_t MakeToggle(const std::string& name, const std::string& label, const std::string& help,
        const std::vector<std::pair<std::string, std::string>>& choices, uint32_t key)
    {
        auto item = Object(ToggleTemplate, name);
        Clear(item);
        At<const char*>(item, Item::Label) = Text(Upper(label));
        At<const char*>(item, Item::Description) = Text(help);
        std::vector<Choice> list;
        for (auto& [text, data] : choices)
            list.push_back({ Text(Upper(text)), Intern(data) });
        At<Choice*>(item, Item::StaticItems) = MakeArray(list);
        At<uint32_t>(item, Item::DynamicItems) = 0;
        At<uint32_t>(item, Item::DynamicItems + 4) = 0;
        At<uint32_t>(item, Item::CurrentValue + 4) = key;
        return item;
    }

    uintptr_t MakeSubMenu(const std::string& name, const std::string& label, const std::string& help, const std::vector<uintptr_t>& items)
    {
        auto menu = Object(MenuTemplate, name + "#menu");
        At<uintptr_t*>(menu, Menu::Items) = MakeArray(items);
        At<int>(menu, Menu::DefaultHighlighted) = 0;
        At<uint64_t>(menu, 0x10) = 0;
        At<uint64_t>(menu, 0x20) = 0;
        auto item = Object(SubMenuTemplate, name);
        Clear(item);
        At<const char*>(item, Item::Label) = Text(Upper(label));
        At<const char*>(item, Item::Description) = Text(help);
        At<uintptr_t>(item, Item::SubMenu) = menu;
        return item;
    }

    uintptr_t MakeRow(Pages& pages, const asi_settings::LoadedPlugin& plugin, const WFPPlugins::Row& source)
    {
        auto id = plugin.file.id + "|" + (source.setting ? source.setting->id : std::string("@preset"));
        Row row;
        row.plugin = &plugin;
        row.setting = source.setting;
        row.key = KeyOf("WFPMenu|" + id);
        std::vector<std::pair<std::string, std::string>> choices;
        std::string label, help;
        if (source.kind == WFPPlugins::Row::Kind::Preset)
        {
            row.kind = Row::Kind::Preset;
            label = WFPText::Get("WFP_PRESET");
            help = WFPText::Get("WFP_PRESET_HELP");
            if (OtherPlugins::CurrentPreset(plugin) < 0)
                choices.push_back({ WFPText::Get("WFP_PRESET_CUSTOM"), "-1" });
            for (int i = 0; i < 3; ++i)
                choices.push_back({ WFPText::Get(WFPPlugins::PresetRow::Names[i]), std::to_string(i) });
        }
        else
        {
            label = WFPPlugins::Label(source);
            help = WFPPlugins::Help(source);
            for (auto& value : ValuesOf(plugin, *source.setting))
                choices.push_back({ OtherPlugins::ValueText(plugin, *source.setting, value, WFPText::Get("WFP_ON"), WFPText::Get("WFP_OFF")), value });
        }
        for (auto& choice : choices)
            row.data.push_back(choice.second);
        row.item = MakeToggle(id, label, HelpText(help), choices, row.key);
        pages.rows.push_back(std::move(row));
        return pages.rows.back().item;
    }

    // The rows a menu screen shows at once
    constexpr size_t ScreenRows = 8;

    // The Plugins submenu: a page for each plugin
    uintptr_t BuildPlugins(Pages& pages)
    {
        HMODULE module = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCWSTR)&BuildPlugins, &module);
        const int language = GetGameLanguage ? GetGameLanguage() : 0;
        const auto selected = FixSettings().String("MAIN.TextLanguage");
        WFPText::SetLanguage(WFPMenu::TextLanguage(FixSettings(), LanguageName(language)));
        pages.snapshot = WFPPlugins::Find(module, selected == "auto" ? LanguageTag(language) : selected);
        pages.rows.clear();
        pages.rows.reserve(256);
        std::vector<uintptr_t> pluginItems;
        for (auto& plugin : pages.snapshot->plugins)
        {
            auto sections = WFPPlugins::Sections(plugin);
            std::vector<uintptr_t> items;
            for (auto& section : sections)
            {
                std::erase_if(section.rows, [](const WFPPlugins::Row& row)
                {
                    return row.kind == WFPPlugins::Row::Kind::RestoreDefaults || row.kind == WFPPlugins::Row::Kind::Action || !WFPPlugins::Visible(row);
                });
            }
            std::erase_if(sections, [](const WFPPlugins::Section& section) { return section.rows.empty(); });

            // The Preset row first, then the rows: all on the page when they fit, else a submenu for each section, of at
            // most a screen of rows each
            std::vector<uintptr_t> preset;
            std::vector<std::pair<std::string, std::vector<uintptr_t>>> lists;
            size_t count = 0;
            for (auto& section : sections)
            {
                lists.push_back({ section.title, {} });
                for (auto& row : section.rows)
                {
                    if (row.kind == WFPPlugins::Row::Kind::Preset)
                        preset.push_back(MakeRow(pages, plugin, row));
                    else
                        lists.back().second.push_back(MakeRow(pages, plugin, row));
                }
                count += lists.back().second.size();
            }
            items = preset;
            if (count + preset.size() <= ScreenRows)
            {
                for (auto& [title, rows] : lists)
                    items.insert(items.end(), rows.begin(), rows.end());
            }
            else
            {
                for (auto& [title, rows] : lists)
                {
                    // Screens of about the same size
                    size_t parts = (rows.size() + ScreenRows - 1) / ScreenRows;
                    size_t size = parts ? (rows.size() + parts - 1) / parts : 0;
                    for (size_t part = 0; part < parts; ++part)
                    {
                        auto name = parts > 1 ? title + " (" + std::to_string(part + 1) + "/" + std::to_string(parts) + ")" : title;
                        auto begin = rows.begin() + part * size;
                        std::vector<uintptr_t> screen(begin, begin + std::min(size, rows.size() - part * size));
                        if (!screen.empty())
                            items.push_back(MakeSubMenu(plugin.file.id + "|section|" + name, name, "", screen));
                    }
                }
            }
            pluginItems.push_back(MakeSubMenu(plugin.file.id, WFPPlugins::Name(plugin), MenuHelp(""), items));
        }
        return MakeSubMenu("WFPPlugins", WFPText::Get("WFP_PLUGINS"), MenuHelp(WFPText::Get("WFP_PLUGINS_HELP")), pluginItems);
    }

    // The game's Settings menu: the one with its Audio and Display rows
    bool IsSettingsMenu(uintptr_t menu)
    {
        auto items = At<uintptr_t*>(menu, Menu::Items);
        bool audio = false, display = false;
        for (uint32_t i = 0; i < Count(items); ++i)
        {
            auto label = At<const char*>(items[i], Item::Label);
            if (!label || items[i] == Current.pluginsItem)
                continue;
            audio |= !strcmp(label, "ID_SETTINGS_AUDIO");
            display |= !strcmp(label, "ID_OPTIONS_DISPLAY");
        }
        return audio && display;
    }

    bool FindTemplates(uintptr_t menu)
    {
        // From the menu being sent: the menus of a level go away with it
        auto items = At<uintptr_t*>(menu, Menu::Items);
        MenuTemplate = menu;
        SubMenuTemplate = ToggleTemplate = 0;
        for (uint32_t i = 0; i < Count(items); ++i)
        {
            if (!IsType(items[i], "UIMenuItem"))
                continue;
            SubMenuTemplate = items[i];
            auto sub = At<uintptr_t>(items[i], Item::SubMenu);
            auto rows = sub ? At<uintptr_t*>(sub, Menu::Items) : nullptr;
            for (uint32_t k = 0; k < Count(rows) && !ToggleTemplate; ++k)
            {
                if (IsType(rows[k], "UIToggleTreeMenuItem") && At<uint32_t>(rows[k], Item::CurrentValue))
                    ToggleTemplate = rows[k];
            }
        }
        return SubMenuTemplate && ToggleTemplate;
    }

    // The Settings menu with the Plugins row added
    void AddPluginsRow(uintptr_t menu)
    {
        static std::map<uintptr_t, std::pair<uintptr_t*, uintptr_t*>> menus;   // menu: the game's items, the items with Plugins
        auto items = At<uintptr_t*>(menu, Menu::Items);
        auto& known = menus[menu];
        if (items != known.second)
            known = { items, nullptr };
        if (!FindTemplates(menu))
            return;

        std::lock_guard lock(Mutex);
        Current.pluginsItem = BuildPlugins(Current);
        std::vector<uintptr_t> list(known.first, known.first + Count(known.first));
        list.push_back(Current.pluginsItem);
        known.second = MakeArray(list);
        At<uintptr_t*>(menu, Menu::Items) = known.second;
        WriteValues();
    }

    // A value the player picked on a row
    void Picked(Row& row, const std::string& data)
    {
        if (row.kind == Row::Kind::Preset)
        {
            int preset = std::atoi(data.c_str());
            if (preset >= 0 && preset < 3 && preset != OtherPlugins::CurrentPreset(*row.plugin) && OtherPlugins::ApplyPreset(*row.plugin, preset))
                WFPPlugins::RestartNeeded() = true;
            return;
        }
        WFPPlugins::Set(*row.plugin, *row.setting, data);
    }

    // The race's pause menu: the one with its Resume row
    bool IsPauseMenu(uintptr_t menu)
    {
        auto items = At<uintptr_t*>(menu, Menu::Items);
        for (uint32_t i = 0; i < Count(items); ++i)
        {
            auto label = At<const char*>(items[i], Item::Label);
            if (label && !strcmp(label, "ID_MENU_RESUME"))
                return true;
        }
        return false;
    }

    SafetyHookInline shSerializeMenu = {};
    int __cdecl SerializeMenu(uintptr_t menu, void* out)
    {
        if (menu && IsPauseMenu(menu))
            fb::PauseMenu::OpenedAt = GetTickCount64();
        if (menu && IsSettingsMenu(menu))
            AddPluginsRow(menu);
        return shSerializeMenu.ccall<int>(menu, out);
    }

}

class Options
{
public:
    Options()
    {
        WFP::onInitEvent() += []()
        {
            // UITreeMenuAsset to the menu screen, and in it the UI manager, the data store's string value and its end
            auto serialize = hook::pattern("83 EC 60 53 55 8B 6C 24 6C 33 DB 56 57 39 5D 10 74 17 E8");
            auto rows = hook::pattern("E8 ? ? ? ? 83 C4 08 39 5C 24 38 74 0E 8D 4C 24 20 51 8B 4C 24 7C E8 ? ? ? ? 8D 4C 24 20 E8");
            auto makeString = hook::pattern("55 56 8B F1 C7 46 14 00 00 00 00 8D 4E 08 C7 01 ? ? ? ? C7 41 04 ? ? ? ? C7 41 08");
            auto language = hook::pattern("55 8B EC 83 E4 F0 83 EC 1C 56 8D 44 24 0C 50 8D 4C 24 14 51 6A 00 6A 02 68 ? ? ? ? 68 ? ? ? ? 66 0F EF C0 68 02 00 00 80");

            auto getManager = injector::GetBranchDestination(serialize.get_first(18)).as_int();
            UIData::pManager = *hook::range_pattern(getManager, getManager + 5, "A1 ? ? ? ?").get_first<void**>(1);
            UIData::MakeString = (decltype(UIData::MakeString))makeString.get_first();
            UIData::Destroy = (decltype(UIData::Destroy))injector::GetBranchDestination(rows.get_first(32)).as_int();
            GetGameLanguage = (decltype(GetGameLanguage))language.get_first();
            shSerializeMenu = safetyhook::create_inline(serialize.get_first(), SerializeMenu);

            // The update notice is the Plugins row's help; FusionFixHub opens on its page when the game closes
            RWGTA::UpdateCheck::Config config;
            config.pluginId = "NFSTheRun.FusionFix";
            config.fileVersion = rsc_FileVersion;
            config.settings = &FixSettings();
            config.show = [](const std::wstring& notice)
            {
                std::lock_guard lock(NoticeMutex);
                UpdateNotice = asi_settings::text::Narrow(notice);
            };
            RWGTA::UpdateCheck::Init(std::move(config));
            WFP::onGameProcessEvent() += []() { RWGTA::UpdateCheck::Tick(); };
            WFP::onShutdownEvent() += []() { RWGTA::UpdateCheck::OnExit(); };

            // The values the player picks
            WFP::onGameProcessEvent() += []()
            {
                std::lock_guard lock(Mutex);
                if (Current.rows.empty())
                    return;
                bool changed = WFPPlugins::Changed().exchange(false);
                for (auto& row : Current.rows)
                {
                    std::string data;
                    if (UIData::Get(row.key, data) && data != row.written)
                    {
                        row.written = data;
                        Picked(row, data);
                        changed = true;
                    }
                }
                if (changed)
                    WriteValues();
            };
        };
    }
} Options;
