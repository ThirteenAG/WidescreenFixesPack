module;

#include <stdafx.h>
#include <deque>
#include <unordered_set>
#include "../Shared/NFS/MenuOwner.hpp"
#include "../Shared/NFS/MenuLayout.hpp"
#include "../Shared/NFS/LegacyText.hpp"
#include "FixSettings.hpp"
#include "Help.hpp"
#include "Dialog.hpp"
#include "../Shared/NFS/PluginsMenu.hpp"
#include "../Shared/NFS/SettingsMenu.hpp"
#include "../Shared/NFS/Text.hpp"
#include "../Shared/NFS/TextLanguage.hpp"

export module Options;

import ComVars;

// The plugins' settings in the game's options menu. The menus are a table of entries ({id, parent, label, type, DAL
// id}, at 0xA52AE8) and every screen lists the entries of one parent, so the fix adds entries of its own:
//
// - Settings that belong next to one of the game's options are listed on that option's screen: console gamma, SMAA,
//   the frame rate limit and the display mode under Graphics > Advanced, the shadow resolution and motion blur under
//   Graphics > World, the HUD width under Interface.
// - One "Plugins" entry lists the fix first and then every other loaded plugin with settings in menus (ASI Settings
//   menu hosts). Each opens its page: the Preset row, its settings by the description's pages, Restore Defaults last.
//   A screen shows nine entries at most (its front end package has the rows ITEM_GROUP_1 to ITEM_GROUP_9), so a
//   page with more is split into screens, one for each of the description's pages.
//
// The options menu already has nine entries (audio, autosave, gameplay, online options, controls, interface,
// graphics, EA Trax jukebox and credits), so Plugins takes the place of the credits. That options menu (entry 30) is
// the main menu's and the career hub's. The pause menu in a race has an options menu of its own (entry 60: audio,
// gameplay, controls, interface and graphics, which the pause menu drops in a race, 0x5A7000), so Plugins is added at
// its end, and there the fix's page also has the settings of the graphics screens.
//
// Values go through the options DAL like the game's, under DAL ids of their own, and through each plugin's runtime
// table (AsiSettings_GetApi), the fix's too. Lists get their items and sliders their range from the game's option
// commands. A list opens on accept like the game's; the Preset row picks with left/right instead, and it and Restore
// Defaults ask to confirm on the first accept. Labels are upper case like the game's, in the game's language.

namespace
{
    enum EntryType : int32_t
    {
        TypeMenu = 0,
        TypeSlider = 1,
        TypeList = 2,
    };

    struct TreeEntry
    {
        int32_t id;
        int32_t parent;
        uint32_t label;
        int32_t type;
        int32_t dalId;
    };
    static_assert(sizeof(TreeEntry) == 0x14);

    constexpr int32_t OptionsMenu = 30;         // the game's options menu
    constexpr int32_t PauseOptionsMenu = 60;    // the options menu of the pause menu in a race
    constexpr uint32_t GraphicsTitle = 0x5DD8F55B;
    bool bPauseOptions = false;                 // the table has that menu
    constexpr int32_t CreditsEntry = 181;       // its credits, opened by the options screen's handler for this id
    constexpr uint32_t CreditsTitle = 0xDDEEEDE5;

    // The game's screens that get settings of the fix, each after one of the game's entries
    struct Placement
    {
        int32_t screen;
        int32_t after;
        const char* setting;
    };
    constexpr Placement Placements[] = {
        { 186, 201, "GRAPHICS.ConsoleGamma" },          // Graphics > Advanced: after brightness
        { 186, 272, "GRAPHICS.AntiAliasing" },          // after anti aliasing
        { 186, 274, "MAIN.FPSLimit" },                  // after v-sync
        { 186, 274, "MAIN.WindowMode" },
        { 184, 265, "GRAPHICS.ShadowResolution" },            // Graphics > World: after world shadows
        { 184, 266, "GRAPHICS.DisableMotionBlur" },     // after world FX
        { 178, 261, "MAIN.HudAspectRatioConstraint" },  // Interface: after the leaderboard
    };
    constexpr int32_t InterfaceScreen = 178;    // the one of those screens the pause menu has too

    // The game's table of menu entries
    TreeEntry* Table = nullptr;
    TreeEntry* TableEnd = nullptr;

    // The Plugins entries: the options menu's and the pause menu's
    constexpr int32_t PluginsMenu = 900;
    constexpr int32_t PausePluginsMenu = 901;
    constexpr size_t ScreenRows = 9;

    enum class Kind { Menu, Setting, Action, Preset, RestoreDefaults };

    struct MenuEntry
    {
        TreeEntry entry;
        Kind kind = Kind::Menu;
        size_t plugin = 0;          // index in the plugins found
        std::string setting;        // the setting's id
        std::string title;          // a menu's title (UTF-8)
        std::string titleKey;
        std::string titleSuffix;
        HMODULE module = nullptr;
        int32_t root = 0;           // the Plugins entry it's under, 0 for the settings on the game's screens
    };
    std::deque<MenuEntry> Entries;  // node based: the game keeps pointers to the tree entries

    // The plugins, discovered by the frame service after initialization (the fix first)
    std::shared_ptr<WFPPlugins::Snapshot> Plugins;
    std::string PluginsLanguage;
    std::string PluginsFailedLanguage;

    MenuEntry* FindById(int32_t id)
    {
        for (auto& entry : Entries)
        {
            if (entry.entry.id == id)
                return &entry;
        }
        return nullptr;
    }

    MenuEntry* FindByDalId(int32_t dalId)
    {
        if (dalId == 0)
            return nullptr;
        for (auto& entry : Entries)
        {
            if (entry.entry.dalId == dalId)
                return &entry;
        }
        return nullptr;
    }

    const asi_settings::LoadedPlugin* PluginOf(const MenuEntry& entry)
    {
        if (Plugins)
        {
            for (auto& plugin : Plugins->plugins)
            {
                if (plugin.module == entry.module)
                    return &plugin;
            }
        }
        return nullptr;
    }

    // The entry's row of the plugin's page
    WFPPlugins::Row RowOf(const MenuEntry& entry)
    {
        WFPPlugins::Row row;
        row.plugin = PluginOf(entry);
        switch (entry.kind)
        {
            case Kind::Preset:
                row.kind = WFPPlugins::Row::Kind::Preset;
                break;
            case Kind::RestoreDefaults:
                row.kind = WFPPlugins::Row::Kind::RestoreDefaults;
                break;
            case Kind::Action:
                row.kind = WFPPlugins::Row::Kind::Action;
                break;
            default:
                row.kind = WFPPlugins::Row::Kind::Setting;
                break;
        }
        if (row.plugin && !entry.setting.empty())
            row.setting = row.plugin->file.description.Find(entry.setting);
        return row;
    }

    struct CachedSetting
    {
        std::string value;
        std::string text;
        std::vector<std::string> values;
        std::vector<std::string> texts;
        bool visible = false;
        bool enabled = false;
    };
    std::unordered_map<int32_t, CachedSetting> Cached;
    std::unordered_map<int32_t, CachedSetting> ListedSnapshots;
    std::unordered_map<uint8_t*, CachedSetting> PublishedChoices;
    std::unordered_map<HMODULE, int> CachedPresets;
    WFPMenu::Owner PageOwner;
    thread_local uint32_t NativeDepth = 0;
    uint64_t Lifetime = 0;
    struct DefaultsContext
    {
        WFPMenu::Owner::Token owner{};
        int32_t parent = -1;
        bool pause = false;
        uint8_t* context = nullptr;
        WFPMenu::Owner::Token replacement{};
    };
    thread_local DefaultsContext NativeDefaults;
    uint8_t** DefaultsMainContext = nullptr;
    uint8_t** DefaultsPauseContext = nullptr;

    bool InGameDefaults()
    {
        return NativeDefaults.parent != -1 && PageOwner.Matches(NativeDefaults.owner);
    }

    struct NativeScope
    {
        NativeScope()
        {
            ++NativeDepth;
        }
        ~NativeScope()
        {
            --NativeDepth;
        }
        NativeScope(const NativeScope&) = delete;
        NativeScope& operator=(const NativeScope&) = delete;
    };
    std::unordered_set<void*> Pages;
    uintptr_t* OptionsPageTable = nullptr;
    uintptr_t* BasePageTable = nullptr;
    std::unordered_map<uint8_t*, std::string> PageFormats;
    void RestoreLayout();
    void HideHelp();
    void UpdateHelp(uint8_t* page);
    void RestartNotice(uint8_t* page);
    void TouchPage(uint8_t* page);
    void SyncActiveContext(uint8_t* context);
    bool ReconcileRows(uint8_t* page);
    bool RefreshNeeded = true;

    struct Command
    {
        enum class Kind { Set, Accept, RestorePage } kind;
        WFPMenu::Owner::Token owner;
        std::shared_ptr<WFPPlugins::Snapshot> snapshot;
        int32_t id;
        std::string value;
        int preset = -1;
    };
    std::deque<Command> Commands;

    const CachedSetting* Data(const MenuEntry& entry)
    {
        auto found = Cached.find(entry.entry.id);
        return found == Cached.end() ? nullptr : &found->second;
    }

    bool Enabled(const MenuEntry& entry)
    {
        auto data = Data(entry);
        return data && data->visible && data->enabled;
    }

    void Queue(const MenuEntry& entry, Command::Kind kind, std::string value = {}, int preset = -1)
    {
        auto owner = PageOwner.Capture();
        if (owner.screen && Commands.size() < 64)
            Commands.push_back({ kind, owner, Plugins, entry.entry.id, std::move(value), preset });
    }

    bool IsRow(const MenuEntry& entry)
    {
        return entry.kind != Kind::Menu;
    }

    bool PauseMenuActive = false;
    std::map<int32_t, std::vector<std::string>> JoinedSettings[2];

    std::vector<std::string> Excluded(int32_t root)
    {
        std::vector<std::string> exclude;
        for (auto& [screen, settings] : JoinedSettings[root == PausePluginsMenu ? 1 : 0])
            exclude.insert(exclude.end(), settings.begin(), settings.end());
        return exclude;
    }

    // A setting shown as a list: a switch, a choice, or a number with values of its own (special values and
    // suggestions); the other numbers are sliders
    bool IsListed(const asi_settings::Setting& setting)
    {
        return setting.type == asi_settings::Type::Bool || setting.type == asi_settings::Type::Choice || !setting.special.empty() ||
            !setting.suggestions.empty() || (setting.type != asi_settings::Type::Int && setting.type != asi_settings::Type::Float);
    }

    bool IsSlider(const MenuEntry& entry)
    {
        auto row = RowOf(entry);
        return row.kind == WFPPlugins::Row::Kind::Setting && row.setting && !IsListed(*row.setting);
    }

    // Text
    std::unordered_map<uint32_t, std::string> Texts;     // label hash -> text in the font's encoding

    uint32_t TextHash(const std::string& key)
    {
        return bStringHash(("WFP_TEXT_" + key).c_str());
    }

    // The game's language: its id and the table of languages
    int32_t* CurrentLanguage = nullptr;
    struct LanguageInfo
    {
        int32_t id;
        const char* name;
        const char* code;
        const char* files[4];
        uint32_t unknown[2];
    };
    static_assert(sizeof(LanguageInfo) == 0x24);
    LanguageInfo* Languages = nullptr;
    LanguageInfo* LanguagesEnd = nullptr;
    std::string LanguageName;

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

    // The language of the plugins' texts. The Russian font has its own letter codes, which aren't known, but it has
    // the Latin letters: the texts are English there.
    std::string TextLanguage()
    {
        auto name = WFPMenu::TextLanguage(FixSettings(), FindLanguageName(CurrentLanguage ? *CurrentLanguage : 0));
        return name == "russian" ? "english" : name;
    }

    // UTF-8 text in the game's font, upper case like the game's rows
    std::string GameText(const std::string& text)
    {
        auto result = WFPText::ToEAFont(WFPText::Upper(text), LanguageName);
        // Native rendered-string buffers have room for 1024 wide characters; complete originals stay in the cache.
        result.resize(std::min<size_t>(result.size(), 1023));
        return result;
    }

    float RenderScale(uint32_t font)
    {
        auto language = CurrentLanguage ? *CurrentLanguage : 0;
        return (language == 11 || language == 12 || language == 13) && font == 0x9583AA1A ? 2.0f : 1.0f;
    }

    ProtectedGameRef<uint8_t> NoDollar;
    ProtectedGameRef<uint8_t> LiteralFontControls;
    bool FontModesReady = false;

    class FontModeScope
    {
        inline static thread_local uint32_t depth = 0;
        inline static thread_local uint8_t baselineDollar = 0;
        inline static thread_local uint8_t baselineControls = 0;
        uint8_t previousDollar = 0;
        uint8_t previousControls = 0;
        bool enabled = false;

    public:
        explicit FontModeScope(bool own)
        {
            if (!FontModesReady)
                return;
            enabled = true;
            previousDollar = NoDollar.get();
            previousControls = LiteralFontControls.get();
            if (!depth)
            {
                baselineDollar = previousDollar;
                baselineControls = previousControls;
            }
            ++depth;
            NoDollar = own ? 1 : baselineDollar;
            LiteralFontControls = own ? 1 : baselineControls;
        }

        ~FontModeScope()
        {
            if (enabled)
            {
                --depth;
                NoDollar = previousDollar;
                LiteralFontControls = previousControls;
            }
        }
    };

    void* (__cdecl* FindFont)(uint32_t hash) = nullptr;
    double (__thiscall* GlyphAdvance)(void* font, uint32_t code, uint16_t previous, uint32_t flags) = nullptr;
    uint8_t (__cdecl* LocalizedWide)(wchar_t* destination, int capacity, uint32_t hash) = nullptr;

    std::string SingleLine(std::string text)
    {
        for (auto& c : text)
        {
            if (c == '\r' || c == '\n' || c == '\t' || c == '^')
                c = ' ';
        }
        return text;
    }

    bool DecodeText(const std::string& text, std::vector<wchar_t>& wide)
    {
        if (!LocalizedWide || text.size() >= size_t(INT_MAX - 1))
            return false;
        auto hash = TextHash("MEASURE");
        Texts[hash] = text + '\0';
        wide.assign(text.size() + 1, L'\0');
        bool decoded = LocalizedWide(wide.data(), int(wide.size()), hash) != 0;
        wide.back() = L'\0';
        return decoded;
    }

    std::string FontText(void* object, const std::string& text, bool uppercase = true)
    {
        auto fallback = [&]() { return uppercase ? WFPText::ToEAFont(WFPText::Upper(text), WFPText::Language()) : WFPText::ToEAFont(text, WFPText::Language()); };
        if (!object || !FindFont)
            return fallback();
        auto font = static_cast<uint8_t*>(FindFont(*reinterpret_cast<uint32_t*>(static_cast<uint8_t*>(object) + 0x24)));
        auto header = font ? *reinterpret_cast<uint8_t**>(font + 24) : nullptr;
        if (!header || *reinterpret_cast<uint32_t*>(header) != 0x46746E46)
            return fallback();
        auto count = *reinterpret_cast<uint16_t*>(header + 10);
        auto offset = *reinterpret_cast<uint32_t*>(header + 20);
        if (!count || offset > 0x1000000)
            return fallback();
        auto stride = *reinterpret_cast<uint32_t*>(header + 12) & 0x40000 ? 16 : 12;
        auto hasGlyph = [&](uint16_t code)
        {
            if (code == 0x99)
                code = 0x2122;
            else if (code == 0x9C)
                code = 0x153;
            else if (code == 0xA0)
                code = ' ';
            size_t first{}, last = count;
            while (first < last)
            {
                auto middle = first + (last - first) / 2;
                auto glyph = *reinterpret_cast<uint16_t*>(header + offset + middle * stride);
                if (glyph == code)
                    return true;
                if (glyph < code)
                    first = middle + 1;
                else
                    last = middle;
            }
            return false;
        };
        std::string result;
        for (auto c : WFPText::ToWide(uppercase ? WFPText::Upper(text) : text))
        {
            char bytes[4]{};
            auto size = WideCharToMultiByte(CP_UTF8, 0, &c, 1, bytes, sizeof(bytes), nullptr, nullptr);
            auto utf8 = std::string(bytes, size);
            auto encoded = WFPText::ToEAFont(utf8, WFPText::Language());
            if (encoded == "?" && c != L'?')
                encoded = WFPLegacyText::Encode(utf8);
            auto supported = [&](const std::string& value)
            {
                std::vector<wchar_t> wide;
                if (!DecodeText(value, wide) || std::wcslen(wide.data()) != value.size())
                    return false;
                for (size_t i = 0; i < value.size(); ++i)
                {
                    auto code = uint16_t(wide[i]);
                    if (code >= 0xFF80)
                        code = uint8_t(code);
                    if (code == '_' && value[i] != '_')
                        return false;
                    auto expected = uint16_t(uint8_t(value[i]));
                    if (expected == 0x99)
                        expected = 0x2122;
                    else if (expected == 0x9C)
                        expected = 0x153;
                    else if (expected == 0xA0)
                        expected = ' ';
                    auto actual = code == 0x99 ? 0x2122 : code == 0x9C ? 0x153 : code == 0xA0 ? ' ' : code;
                    if (value.size() == 1 && uint8_t(value[i]) >= 0x80 && actual != expected && actual != uint16_t(c))
                        return false;
                    if (code != '\n' && code != '\r' && code != '\t' && !hasGlyph(code))
                        return false;
                }
                return true;
            };
            if (!supported(encoded))
                encoded = WFPLegacyText::Encode(utf8);
            if (!supported(encoded))
            {
                wchar_t decomposed[8]{};
                if (NormalizeString(NormalizationD, &c, 1, decomposed, 8) > 0 &&
                    decomposed[0] >= 0x20 && decomposed[0] < 0x80)
                    encoded.assign(1, char(decomposed[0]));
            }
            result += supported(encoded) ? encoded : (hasGlyph('?') ? "?" : " ");
        }
        return result;
    }

    float MeasureText(void* object, const std::string& text)
    {
        FontModeScope literal(true);
        if (!object || !FindFont || !GlyphAdvance || !LocalizedWide)
            return std::numeric_limits<float>::quiet_NaN();
        auto bytes = static_cast<uint8_t*>(object);
        auto data = *reinterpret_cast<float**>(bytes + 0x2C);
        auto font = FindFont(*reinterpret_cast<uint32_t*>(bytes + 0x24));
        if (!data || *reinterpret_cast<uint32_t*>(bytes + 0x30) < 68 || !font ||
            !*reinterpret_cast<uintptr_t*>(static_cast<uint8_t*>(font) + 24) || !std::isfinite(data[14]))
            return std::numeric_limits<float>::quiet_NaN();
        std::vector<wchar_t> wide;
        if (!DecodeText(text, wide))
            return std::numeric_limits<float>::quiet_NaN();
        // Match the renderer's glyph advances and kerning before applying the authored text scale.
        float width{}, maximum{};
        auto flags = *reinterpret_cast<uint32_t*>(bytes + 0x1C) & 0x20;
        uint16_t previous{};
        for (auto c : wide)
        {
            if (!c)
                break;
            if (c == L'\n')
            {
                maximum = std::max(maximum, width);
                width = 0.0f;
                previous = 0;
                continue;
            }
            width += float(GlyphAdvance(font, uint16_t(c), previous, flags));
            previous = uint16_t(c);
        }
        auto scale = RenderScale(*reinterpret_cast<uint32_t*>(bytes + 0x24));
        return std::max(maximum, width) * std::abs(data[14]) * scale;
    }

    // The plugins and the entries of their pages
    int32_t NextId = 910;
    int32_t NextDalId = 5900;

    void AddRow(int32_t parent, int32_t root, size_t plugin, const WFPPlugins::Row& row)
    {
        MenuEntry entry;
        entry.root = root;
        entry.plugin = plugin;
        entry.module = Plugins->plugins[plugin].module;
        if (row.setting)
            entry.setting = row.setting->id;
        switch (row.kind)
        {
            case WFPPlugins::Row::Kind::Preset:
                entry.kind = Kind::Preset;
                break;
            case WFPPlugins::Row::Kind::RestoreDefaults:
                entry.kind = Kind::RestoreDefaults;
                break;
            case WFPPlugins::Row::Kind::Action:
                entry.kind = Kind::Action;
                break;
            default:
                entry.kind = Kind::Setting;
                break;
        }
        int32_t id = NextId++;
        int32_t type = row.setting && row.kind == WFPPlugins::Row::Kind::Setting && !IsListed(*row.setting) ? TypeSlider : TypeList;
        entry.entry = { id, parent, TextHash("ENTRY_" + std::to_string(id)), type, NextDalId++ };
        Entries.push_back(std::move(entry));
    }

    int32_t AddMenu(int32_t parent, int32_t root, size_t plugin, const std::string& title)
    {
        MenuEntry entry;
        int32_t id = NextId++;
        entry.entry = { id, parent, TextHash("ENTRY_" + std::to_string(id)), TypeMenu, 0 };
        entry.root = root;
        entry.plugin = plugin;
        entry.module = Plugins->plugins[plugin].module;
        entry.title = title;
        Entries.push_back(std::move(entry));
        return id;
    }

    void ArrangeMenus(int32_t parent, int32_t root, size_t plugin, const std::vector<int32_t>& menus, size_t first = 0, bool hostFirst = false)
    {
        if (menus.size() <= ScreenRows)
        {
            for (auto id : menus)
                FindById(id)->entry.parent = parent;
            return;
        }
        size_t begin = hostFirst ? 1 : 0;
        if (hostFirst)
            FindById(menus.front())->entry.parent = parent;
        size_t capacity = ScreenRows - begin;
        size_t groupSize = (menus.size() - begin + capacity - 1) / capacity;
        for (size_t i = begin; i < menus.size(); i += groupSize)
        {
            size_t end = std::min(menus.size(), i + groupSize);
            auto title = WFPText::Get("WFP_NEXT") + " (" + std::to_string(first + i + 1) + "-" + std::to_string(first + end) + ")";
            int32_t group = AddMenu(parent, root, plugin, title);
            FindById(group)->titleKey = "@next";
            FindById(group)->titleSuffix = title.substr(WFPText::Get("WFP_NEXT").size());
            std::vector<int32_t> children(menus.begin() + i, menus.begin() + end);
            ArrangeMenus(group, root, plugin, children, first + i);
        }
    }

    void AddPages(int32_t root)
    {
        std::vector<int32_t> pluginMenus;
        for (size_t i = 0; i < Plugins->plugins.size(); ++i)
        {
            auto& plugin = Plugins->plugins[i];
            auto sections = WFPPlugins::Sections(plugin);
            struct Screen
            {
                std::string title;
                std::string key;
                std::string suffix;
                std::vector<WFPPlugins::Row> rows;
            };
            std::vector<Screen> screens;
            for (auto& section : sections)
            {
                std::erase_if(section.rows, [](const WFPPlugins::Row& row)
                {
                    return row.kind == WFPPlugins::Row::Kind::RestoreDefaults;
                });
                std::string key = "@plugin";
                for (auto& row : section.rows)
                {
                    if (!row.setting)
                        continue;
                    auto page = std::find_if(plugin.file.description.pages.begin(), plugin.file.description.pages.end(), [&](const asi_settings::Page& page)
                    {
                        return asi_settings::text::IEquals(page.id, row.setting->page);
                    });
                    if (page != plugin.file.description.pages.end())
                        key = page->title;
                    break;
                }
                size_t count = (section.rows.size() + ScreenRows - 2) / (ScreenRows - 1);
                for (size_t k = 0; k < count; ++k)
                {
                    auto title = section.title;
                    std::string suffix;
                    if (count > 1)
                        suffix = " (" + std::to_string(k + 1) + "/" + std::to_string(count) + ")";
                    title += suffix;
                    auto begin = section.rows.begin() + k * (ScreenRows - 1);
                    std::vector<WFPPlugins::Row> rows(begin, begin + std::min(ScreenRows - 1, section.rows.size() - k * (ScreenRows - 1)));
                    if (std::any_of(rows.begin(), rows.end(), [](const WFPPlugins::Row& row)
                    {
                        return row.kind == WFPPlugins::Row::Kind::Setting;
                    }))
                        rows.push_back({ WFPPlugins::Row::Kind::RestoreDefaults, &plugin });
                    screens.push_back({ std::move(title), key, std::move(suffix), std::move(rows) });
                }
            }
            int32_t page = AddMenu(root, root, i, WFPPlugins::Name(plugin));
            FindById(page)->titleKey = "@plugin";
            pluginMenus.push_back(page);
            std::vector<int32_t> sectionMenus;
            for (size_t j = 0; j < screens.size(); ++j)
            {
                auto& [title, key, suffix, rows] = screens[j];
                int32_t parent = screens.size() > 1 ? AddMenu(page, root, i, title) : page;
                if (parent != page)
                {
                    FindById(parent)->titleKey = key;
                    FindById(parent)->titleSuffix = suffix;
                    sectionMenus.push_back(parent);
                }
                for (auto& row : rows)
                    AddRow(parent, root, i, row);
            }
            ArrangeMenus(page, root, i, sectionMenus);
        }
        ArrangeMenus(root, root, 0, pluginMenus, 0, true);
    }


    HMODULE Self()
    {
        HMODULE self = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCWSTR>(&Self), &self);
        return self;
    }

    // Discovery runs outside native menu callbacks. Language refresh preserves native entry identities.
    // True when they were found again
    bool UpdatePlugins()
    {
        auto language = TextLanguage();
        if (language == PluginsLanguage || language == PluginsFailedLanguage)
            return false;
        auto lifetime = Lifetime;
        auto previous = Plugins;
        auto tag = WFPText::Detail::Tag(language);
        auto plugins = WFPPlugins::Find(Self(), tag ? asi_settings::text::Narrow(tag) : "en");
        if (Lifetime != lifetime || Plugins != previous)
            return false;
        if (plugins->plugins.empty() || plugins->plugins.front().module != Self())
        {
            // The fix's own description can't be read in this language: don't look again every frame
            PluginsLanguage.clear();
            PluginsFailedLanguage = language;
            return false;
        }
        PluginsLanguage = language;
        WFPText::SetLanguage(language);
        if (Plugins)
        {
            Plugins = plugins;
            return true;
        }
        Plugins = plugins;

        // The settings on the game's screens
        for (auto& placement : Placements)
        {
            auto setting = Plugins->plugins.front().file.description.Find(placement.setting);
            if (!setting || !WFPPlugins::Shown(*setting))
                continue;
            AddRow(placement.screen, 0, 0, { WFPPlugins::Row::Kind::Setting, &Plugins->plugins.front(), setting });
        }
        AddPages(PluginsMenu);
        if (bPauseOptions)
            AddPages(PausePluginsMenu);
        return true;
    }

    std::string EntryLabel(const MenuEntry& entry)
    {
        if (entry.kind != Kind::Menu)
        {
            auto row = RowOf(entry);
            return row.plugin && (row.setting || entry.kind != Kind::Setting) ? WFPPlugins::Label(row) : std::string();
        }
        std::string title = entry.title;
        if (entry.entry.id == PluginsMenu || entry.entry.id == PausePluginsMenu)
            title = WFPText::Get("WFP_PLUGINS");
        else if (entry.titleKey == "@next")
            title = WFPText::Get("WFP_NEXT");
        else if (auto plugin = PluginOf(entry))
        {
            if (entry.titleKey == "@plugin")
                title = WFPPlugins::Name(*plugin);
            else if (!entry.titleKey.empty())
                title = OtherPlugins::Text(*plugin, entry.titleKey);
        }
        return title + entry.titleSuffix;
    }

    void UpdateTexts(bool found = false)
    {
        static std::string current;
        auto name = TextLanguage();
        if (name == current && !found)
            return;
        current = name;
        LanguageName = name;
        WFPText::SetLanguage(LanguageName);

        Texts.clear();
        Texts[TextHash("WFP_PLUGINS")] = GameText(WFPText::Get("WFP_PLUGINS"));
        for (auto& entry : Entries)
            Texts[entry.entry.label] = GameText(EntryLabel(entry));
    }

    // The search of a language table for a text by its hash
    SafetyHookInline shSearchString = {};
    const char* __cdecl SearchString(uint32_t hash, int table)
    {
        NativeScope scope;
        if (auto it = Texts.find(hash); it != Texts.end())
            return it->second.c_str();
        return shSearchString.unsafe_ccall<const char*>(hash, table);
    }

    // The Preset and Restore Defaults rows of the page shown
    WFPPlugins::PresetRow PresetRow;
    WFPPlugins::RestoreRow RestoreRow;

    // The tree
    void* (__cdecl* ListNode)(TreeEntry* entry) = nullptr;
    void* (__cdecl* SliderNode)(TreeEntry* entry) = nullptr;
    void* (__cdecl* StandardNode)(TreeEntry* entry) = nullptr;

    struct ListLink
    {
        ListLink* next;
        ListLink* prev;
    };

    struct PageSource
    {
        int32_t parent;
        bool pause;
    };
    std::unordered_map<ListLink*, PageSource> ChildrenParents;
    std::unordered_map<uint8_t*, PageSource> PageParents;
    thread_local uint32_t PreviewDepth = 0;
    uint8_t* LastContext = nullptr;
    void (__thiscall* RepaintRows)(uint8_t*) = nullptr;
    SafetyHookInline shCreatePreview;

    void __fastcall CreatePreview(void* context, void*, ListLink* nodes)
    {
        NativeScope native;
        ++PreviewDepth;
        struct Finish
        {
            ~Finish()
            {
                --PreviewDepth;
            }
        } finish;
        shCreatePreview.thiscall<void>(context, nodes);
    }

    void SyncActiveContext(uint8_t* context)
    {
        if (!context)
            return;
        auto list = reinterpret_cast<ListLink*>(context + 0x20);
        if (!list->next || list->next == list || !list->prev)
            return;
        auto page = reinterpret_cast<uint8_t*>(list->prev) - 4;
        if (Pages.count(page) && *reinterpret_cast<uint8_t**>(page + 0x14) == context)
            TouchPage(page);
    }

    void Append(ListLink* list, ListLink* node)
    {
        auto tail = list->prev;
        tail->next = node;
        list->prev = node;
        node->prev = tail;
        node->next = list;
    }

    // A node of the menu, made from an entry
    void* MakeNode(TreeEntry* entry)
    {
        switch (entry->type)
        {
            case TypeList: return ListNode(entry);
            case TypeSlider: return SliderNode(entry);
            default: return StandardNode(entry);
        }
    }

    bool AppendNode(ListLink* list, TreeEntry* entry)
    {
        if (auto node = MakeNode(entry))
        {
            Append(list, reinterpret_cast<ListLink*>(reinterpret_cast<uintptr_t>(node) + 4));
            return true;
        }
        return false;
    }

    bool Shown(const MenuEntry& entry)
    {
        auto data = Data(entry);
        if (!data || !data->visible)
            return false;
        auto row = RowOf(entry);
        return !row.setting || !Plugins || Plugins->plugins.empty() || entry.module != Plugins->plugins.front().module ||
            !WFPPlugins::Excluded(*row.setting, Excluded(entry.root));
    }

    // The entries of the plugins' pages that are shown: rows the plugin hides (visibleIf, its state) are left out
    void AppendEntries(ListLink* list, int32_t parent)
    {
        for (auto& entry : Entries)
        {
            if (entry.entry.parent != parent || entry.root == 0)
                continue;
            if ((entry.kind == Kind::Setting || entry.kind == Kind::Action) && !Shown(entry))
                continue;
            if (entry.kind == Kind::RestoreDefaults && std::none_of(Entries.begin(), Entries.end(), [&](const MenuEntry& other)
            {
                return other.entry.parent == parent && other.kind == Kind::Setting && Shown(other);
            }))
                continue;
            if (entry.kind == Kind::Preset)
            {
                PresetRow.plugin = PluginOf(entry);
                auto found = CachedPresets.find(entry.module);
                PresetRow.shown = found != CachedPresets.end() ? found->second : -1;
                PresetRow.confirm = false;
            }
            if (entry.kind == Kind::RestoreDefaults)
                RestoreRow.Cancel();
            AppendNode(list, &entry.entry);
        }
    }

    bool HasPlacements(int32_t screen)
    {
        return std::any_of(std::begin(Placements), std::end(Placements), [&](const Placement& p) { return p.screen == screen; });
    }

    SafetyHookInline shGetChildren = {};
    void __cdecl GetChildren(int32_t parent, ListLink* list)
    {
        NativeScope scope;
        if (InGameDefaults())
        {
            if (parent == -1)
                parent = NativeDefaults.parent;
            PauseMenuActive = NativeDefaults.pause;
        }
        if (parent == OptionsMenu || parent == PluginsMenu)
            PauseMenuActive = false;
        else if (parent == PauseOptionsMenu || parent == PausePluginsMenu)
            PauseMenuActive = true;
        ChildrenParents[list] = { parent, PauseMenuActive };
        if (!Plugins)
        {
            shGetChildren.unsafe_ccall(parent, list);
            return;
        }

        // The options menu: the game's entries in its order, with Plugins in the place of the credits
        if (parent == OptionsMenu)
        {
            auto plugins = FindById(PluginsMenu);
            for (auto entry = Table; entry < TableEnd; ++entry)
            {
                if (entry->parent == OptionsMenu)
                    AppendNode(list, entry->id == CreditsEntry ? &plugins->entry : entry);
            }
            return;
        }

        // The race's pause menu: its options, then Plugins
        if (parent == PauseOptionsMenu && bPauseOptions && Plugins)
        {
            shGetChildren.unsafe_ccall(parent, list);
            AppendNode(list, &FindById(PausePluginsMenu)->entry);
            return;
        }

        // A game screen with settings of the fix: its entries in its order, the fix's after the game's they follow
        if (HasPlacements(parent) && Plugins)
        {
            auto& joined = JoinedSettings[PauseMenuActive ? 1 : 0][parent];
            joined.clear();
            for (auto entry = Table; entry < TableEnd; ++entry)
            {
                if (entry->parent != parent)
                    continue;
                AppendNode(list, entry);
                for (auto& placement : Placements)
                {
                    if (placement.screen != parent || placement.after != entry->id)
                        continue;
                    for (auto& own : Entries)
                    {
                        if (own.root == 0 && own.entry.parent == parent && own.setting == placement.setting && Data(own) && Data(own)->visible)
                        {
                            if (AppendNode(list, &own.entry))
                                joined.push_back(own.setting);
                        }
                    }
                }
            }
            return;
        }

        shGetChildren.unsafe_ccall(parent, list);
        AppendEntries(list, parent);
    }

    SafetyHookInline shGetDataNode = {};
    void* __cdecl GetDataNode(int32_t id)
    {
        NativeScope scope;
        if (auto entry = FindById(id))
            return MakeNode(&entry->entry);
        return shGetDataNode.unsafe_ccall<void*>(id);
    }

    SafetyHookInline shGetParentID = {};
    int32_t __cdecl GetParentID(int32_t id)
    {
        NativeScope scope;
        if (auto entry = FindById(id))
            return entry->entry.parent;
        return shGetParentID.unsafe_ccall<int32_t>(id);
    }

    int ChoiceIndex(const asi_settings::Setting& setting, const std::vector<std::string>& values, const std::string& value)
    {
        bool numeric = !setting.StringValued();
        for (size_t i = 0; i < values.size(); ++i)
        {
            if (numeric ? std::atof(values[i].c_str()) == std::atof(value.c_str()) : asi_settings::text::IEquals(values[i], value))
                return int(i);
        }
        return -1;
    }

    // The value of a row with a DAL id, as the game's node holds it
    float NodeValue(const MenuEntry& entry)
    {
        switch (entry.kind)
        {
            case Kind::Preset:
                return float(PresetRow.shown);
            case Kind::Setting:
            {
                auto row = RowOf(entry);
                if (!row.setting)
                    return 0.0f;
                if (!IsListed(*row.setting))
                    return Data(entry) ? float(std::atof(Data(entry)->value.c_str())) : 0.0f;
                return Data(entry) ? float(ChoiceIndex(*row.setting, Data(entry)->values, Data(entry)->value)) : 0.0f;
            }
            default:
                return 0.0f;
        }
    }

    std::string NodeText(const MenuEntry& entry)
    {
        switch (entry.kind)
        {
            case Kind::Preset:
                return PresetRow.ValueText();
            case Kind::RestoreDefaults:
                return RestoreRow.ValueText();
            case Kind::Setting:
                return Data(entry) ? Data(entry)->text : std::string();
            default:
                return {};
        }
    }

    // DALOptions: the values. The game reads a float first and an integer when that fails, and writes the same way.
    using GetIntFn = bool(__fastcall*)(void*, void*, int, int*, int, int, int);
    using SetIntFn = bool(__fastcall*)(void*, void*, int, int, int, int, int);
    using GetFloatFn = bool(__fastcall*)(void*, void*, int, float*, int, int, int);
    using SetFloatFn = bool(__fastcall*)(void*, void*, int, float, int, int, int);
    using GetStringFn = bool(__fastcall*)(void*, void*, int, char*, int, int, int, int);
    GetFloatFn OriginalGetFloat = nullptr;
    SetFloatFn OriginalSetFloat = nullptr;
    GetIntFn OriginalGetInt = nullptr;
    SetIntFn OriginalSetInt = nullptr;
    GetStringFn OriginalGetString = nullptr;
    GetIntFn OriginalGetDefaultInt = nullptr;
    GetFloatFn OriginalGetDefaultFloat = nullptr;

    bool __fastcall GetFloat(void* dal, void* edx, int id, float* value, int a, int b, int c)
    {
        NativeScope scope;
        if (auto entry = FindByDalId(id))
        {
            if (!IsSlider(*entry))
                return false;
            *value = NodeValue(*entry);
            return true;
        }
        return OriginalGetFloat(dal, edx, id, value, a, b, c);
    }

    bool __fastcall SetFloat(void* dal, void* edx, int id, float value, int a, int b, int c)
    {
        NativeScope scope;
        if (auto entry = FindByDalId(id))
        {
            if (!IsSlider(*entry))
                return false;
            if (InGameDefaults())
                return true;
            auto row = RowOf(*entry);
            auto& setting = *row.setting;
            double result = value;
            if (setting.step)
                result = std::round(result / *setting.step) * *setting.step;
            if (setting.min)
                result = std::max(result, *setting.min);
            if (setting.max)
                result = std::min(result, *setting.max);
            if (Enabled(*entry))
                Queue(*entry, Command::Kind::Set, asi_settings::text::FormatNumber(result));
            return true;
        }
        return OriginalSetFloat(dal, edx, id, value, a, b, c);
    }

    bool __fastcall GetInt(void* dal, void* edx, int id, int* value, int a, int b, int c)
    {
        NativeScope scope;
        if (auto entry = FindByDalId(id))
        {
            if (IsSlider(*entry))
                return false;
            *value = int(NodeValue(*entry));
            return true;
        }
        return OriginalGetInt(dal, edx, id, value, a, b, c);
    }

    bool __fastcall SetInt(void* dal, void* edx, int id, int value, int a, int b, int c)
    {
        NativeScope scope;
        if (auto entry = FindByDalId(id))
        {
            if (IsSlider(*entry))
                return false;
            if (entry->kind != Kind::Setting)
                return true;
            if (InGameDefaults())
                return true;
            auto row = RowOf(*entry);
            if (!row.setting || !Enabled(*entry))
                return true;
            auto page = static_cast<uint8_t*>(PageOwner.Get());
            auto node = page ? *reinterpret_cast<uint8_t**>(page + 0x24) : nullptr;
            auto published = PublishedChoices.find(node);
            if (published != PublishedChoices.end() && node &&
                *reinterpret_cast<int32_t*>(node + 0x18) == id && value >= 0 && value < int(published->second.values.size()))
                Queue(*entry, Command::Kind::Set, published->second.values[value]);
            return true;
        }
        return OriginalSetInt(dal, edx, id, value, a, b, c);
    }

    // A slider's value as text
    bool __fastcall GetString(void* dal, void* edx, int id, char* buffer, int size, int a, int b, int c)
    {
        NativeScope scope;
        if (auto entry = FindByDalId(id); entry && IsSlider(*entry))
        {
            if (buffer && size > 0)
                snprintf(buffer, size_t(size), "%s", GameText(NodeText(*entry)).c_str());
            return true;
        }
        return OriginalGetString(dal, edx, id, buffer, size, a, b, c);
    }

    // The defaults, for the game's own restoring of a screen's defaults
    bool __fastcall GetDefaultInt(void* dal, void* edx, int id, int* value, int a, int b, int c)
    {
        NativeScope scope;
        if (auto entry = FindByDalId(id))
        {
            if (IsSlider(*entry))
                return false;
            auto row = RowOf(*entry);
            if (entry->kind != Kind::Setting || !row.setting)
                *value = int(NodeValue(*entry));
            else
            {
                auto data = Data(*entry);
                auto page = static_cast<uint8_t*>(PageOwner.Get());
                auto node = page ? *reinterpret_cast<uint8_t**>(page + 0x24) : nullptr;
                auto published = PublishedChoices.find(node);
                if (published != PublishedChoices.end() && node && *reinterpret_cast<int32_t*>(node + 0x18) == id)
                    data = &published->second;
                // The language keeps the installer's selection rather than imposing a description default.
                auto initial = row.setting->section == "LANGUAGE" && data ? data->value :
                    asi_settings::detail::JsonToText(row.setting->defaultValue);
                *value = data ? ChoiceIndex(*row.setting, data->values, initial) : -1;
            }
            return true;
        }
        return OriginalGetDefaultInt(dal, edx, id, value, a, b, c);
    }

    bool __fastcall GetDefaultFloat(void* dal, void* edx, int id, float* value, int a, int b, int c)
    {
        NativeScope scope;
        if (auto entry = FindByDalId(id))
        {
            if (!IsSlider(*entry))
                return false;
            *value = float(RowOf(*entry).setting->defaultValue.number);
            return true;
        }
        return OriginalGetDefaultFloat(dal, edx, id, value, a, b, c);
    }

    // The option commands: a list's items and a slider's range
    void* FastMem = nullptr;
    void* (__fastcall* FastMemAlloc)(void* fastMem, void* edx, size_t size, int unknown) = nullptr;
    void* (__fastcall* ListItem)(void* item, void* edx, const char* text, float value) = nullptr;

    struct OptionListCommand
    {
        void* vtable;
        uint32_t unknown;
        int32_t dalId;
        float value;
        ListLink* items;
    };

    struct OptionRangeCommand
    {
        void* vtable;
        uint32_t unknown;
        float minimum;
        float maximum;
        float step;
        int32_t dalId;
    };

    void AddItem(ListLink* items, const std::string& text, float value)
    {
        auto item = FastMemAlloc(FastMem, nullptr, 0x14, 0);
        if (!item)
            return;
        ListItem(item, nullptr, GameText(text).c_str(), value);
        Append(items, reinterpret_cast<ListLink*>(reinterpret_cast<uintptr_t>(item) + 4));
    }

    void* OriginalListExecute = nullptr;
    void __fastcall ListExecute(OptionListCommand* command)
    {
        NativeScope scope;
        if (auto entry = FindByDalId(command->dalId))
        {
            switch (entry->kind)
            {
                case Kind::Preset:
                    // Never opened: left and right pick, the item only gives the row its text
                    AddItem(command->items, PresetRow.ValueText(), float(PresetRow.shown));
                    return;
                case Kind::Setting:
                    break;
                default:
                    AddItem(command->items, NodeText(*entry), 0.0f);
                    return;
            }
            auto row = RowOf(*entry);
            if (!row.setting)
                return;
            auto data = Data(*entry);
            auto values = data ? data->values : std::vector<std::string>{};
            if (data)
                ListedSnapshots[entry->entry.id] = *data;
            auto current = data ? data->value : std::string();
            if (ChoiceIndex(*row.setting, values, current) < 0)
                AddItem(command->items, data ? data->text : current, -1.0f);
            for (size_t i = 0; i < values.size(); ++i)
            {
                const auto& text = data && i < data->texts.size() ? data->texts[i] : values[i];
                AddItem(command->items, text, float(i));
            }
            return;
        }
        reinterpret_cast<void(__fastcall*)(OptionListCommand*)>(OriginalListExecute)(command);
    }

    void* OriginalRangeExecute = nullptr;
    void __fastcall RangeExecute(OptionRangeCommand* command)
    {
        NativeScope scope;
        if (auto entry = FindByDalId(command->dalId))
        {
            if (auto setting = RowOf(*entry).setting)
            {
                command->minimum = float(setting->min.value_or(0.0));
                command->maximum = float(setting->max.value_or(1.0));
                command->step = float(setting->step.value_or(1.0));
                return;
            }
        }
        reinterpret_cast<void(__fastcall*)(OptionRangeCommand*)>(OriginalRangeExecute)(command);
    }

    template<typename T>
    void HookSlot(uintptr_t* vtable, int slot, T replacement, T& original)
    {
        ProtectedGameRef<uintptr_t> ref(&vtable[slot]);
        original = reinterpret_cast<T>(vtable[slot]);
        ref = reinterpret_cast<uintptr_t>(replacement);
    }

    void HookSlot(uintptr_t* vtable, int slot, void* replacement, void*& original)
    {
        ProtectedGameRef<uintptr_t> ref(&vtable[slot]);
        original = reinterpret_cast<void*>(vtable[slot]);
        ref = reinterpret_cast<uintptr_t>(replacement);
    }

    // An option page ({vtable, ...}, 0x975F18) holds copies of the entries' nodes ({vtable, link, id, label, label
    // hash, DAL id, value text, value, type, items}), linked at +4 in its list at +0x0C, the highlighted one at +0x24,
    // its state at +0x2C (0 browsing, 1 a slider, 2 a list open). Its slot 11 shows a node in a row. Texts in nodes
    // are the game's CRT allocations.
    constexpr size_t PageNodes = 0x0C;
    constexpr size_t PageCurrent = 0x24;
    constexpr size_t PageState = 0x2C;
    constexpr size_t NodeDalId = 0x18;
    constexpr size_t NodeValueText = 0x1C;
    constexpr size_t NodeValueAt = 0x20;

    std::vector<std::string> PageExclusions(const MenuEntry& entry)
    {
        std::vector<std::string> included;
        auto page = static_cast<uint8_t*>(PageOwner.Get());
        if (page && Pages.count(page))
        {
            auto nodes = reinterpret_cast<ListLink*>(page + PageNodes);
            for (auto link = nodes->next; link && link != nodes; link = link->next)
            {
                auto node = reinterpret_cast<uint8_t*>(link) - 4;
                auto other = FindByDalId(*reinterpret_cast<int32_t*>(node + NodeDalId));
                if (other && other->module == entry.module && other->entry.parent == entry.entry.parent && other->kind == Kind::Setting)
                    included.push_back(other->setting);
            }
        }
        else
        {
            for (auto& other : Entries)
            {
                if (other.entry.parent == entry.entry.parent && other.module == entry.module && other.kind == Kind::Setting)
                    included.push_back(other.setting);
            }
        }
        std::vector<std::string> exclude;
        if (auto plugin = PluginOf(entry))
        {
            for (auto& setting : plugin->file.description.settings)
            {
                if (!WFPPlugins::Excluded(setting, included))
                    exclude.push_back(setting.id);
            }
        }
        return exclude;
    }

    thread_local const std::unordered_map<const char*, std::string>* LiteralStrings = nullptr;

    class LiteralScope
    {
    public:
        explicit LiteralScope(uint8_t* node) : previous(LiteralStrings)
        {
            if (node)
            {
                Add(*reinterpret_cast<const char**>(node + 0x10));
                Add(*reinterpret_cast<const char**>(node + NodeValueText));
                if (*reinterpret_cast<int32_t*>(node + 0x24) == TypeList)
                {
                    auto items = reinterpret_cast<ListLink*>(node + 0x28);
                    size_t remaining = 4096;
                    for (auto link = items->next; link && link != items && remaining; link = link->next, --remaining)
                        Add(*reinterpret_cast<const char**>(reinterpret_cast<uint8_t*>(link) + 8));
                }
            }
            LiteralStrings = &strings;
        }

        LiteralScope(const char* text, bool) : previous(LiteralStrings)
        {
            Add(text);
            LiteralStrings = &strings;
        }

        ~LiteralScope()
        {
            LiteralStrings = previous;
        }

    private:
        void Add(const char* text)
        {
            if (text)
                strings.emplace(text, std::string(text, strnlen(text, 1023)));
        }

        std::unordered_map<const char*, std::string> strings;
        const std::unordered_map<const char*, std::string>* previous;
    };

    uint8_t* OwnNode(uint8_t* page)
    {
        auto node = *reinterpret_cast<uint8_t**>(page + PageCurrent);
        return node && FindById(*reinterpret_cast<int32_t*>(node + 0x0C)) ? node : nullptr;
    }

    SafetyHookInline shDrawSlider;
    using DrawSliderFn = int(__fastcall*)(uint8_t*, void*);
    DrawSliderFn OriginalDrawSlider = nullptr;

    int __fastcall DrawSlider(uint8_t* slider, void* edx)
    {
        NativeScope scope;
        auto page = static_cast<uint8_t*>(PageOwner.Get());
        auto node = page ? OwnNode(page) : nullptr;
        auto entry = node ? FindByDalId(*reinterpret_cast<int32_t*>(node + NodeDalId)) : nullptr;
        if (entry && IsSlider(*entry) && *reinterpret_cast<uintptr_t**>(page) == OptionsPageTable &&
            *reinterpret_cast<int32_t*>(page + PageState) == 1 && *reinterpret_cast<uint8_t**>(page + 0x38) == slider)
        {
            auto minimum = *reinterpret_cast<float*>(slider + 4);
            auto maximum = *reinterpret_cast<float*>(slider + 8);
            auto value = *reinterpret_cast<float*>(slider + 16);
            if (std::isfinite(minimum) && std::isfinite(maximum) && std::isfinite(value) && maximum > minimum)
            {
                // Native drawing adds the minimum instead of subtracting it.
                std::array<uint8_t, 40> draw;
                std::memcpy(draw.data(), slider, draw.size());
                *reinterpret_cast<float*>(draw.data() + 4) = 0.0f;
                *reinterpret_cast<float*>(draw.data() + 8) = 1.0f;
                *reinterpret_cast<float*>(draw.data() + 16) = std::clamp((value - minimum) / (maximum - minimum), 0.0f, 1.0f);
                return OriginalDrawSlider(draw.data(), edx);
            }
        }
        return OriginalDrawSlider(slider, edx);
    }

    SafetyHookInline shFormat;
    int __cdecl Format(char* destination, const char* text, void* arguments)
    {
        if (LiteralStrings)
        {
            if (auto found = LiteralStrings->find(text); found != LiteralStrings->end())
            {
                if (destination)
                    std::memcpy(destination, found->second.c_str(), found->second.size() + 1);
                return int(found->second.size());
            }
        }
        return shFormat.unsafe_ccall<int>(destination, text, arguments);
    }

    void** PackageManager = nullptr;
    uint8_t* (__thiscall* FindPackage)(void*, const char*) = nullptr;
    uint8_t* (__thiscall* FindObject)(void*, uint32_t) = nullptr;
    int (__cdecl* SetLanguageHash)(void*, uint32_t) = nullptr;

    uint8_t* LoadedPackage(const std::string& name)
    {
        auto manager = PackageManager ? *PackageManager : nullptr;
        if (!manager || !FindPackage)
            return nullptr;
        auto entry = FindPackage(manager, name.c_str());
        return entry ? *reinterpret_cast<uint8_t**>(entry + 0x20) : nullptr;
    }

    struct LayoutRecord
    {
        std::string package;
        uint8_t* context{};
        uint8_t* object{};
        float* state{};
        uint32_t guid{};
        uint32_t name{};
        uint32_t maximum{};
        float x{};
        float width{};
        float appliedX{};
        float appliedWidth{};
        bool moved{};
        bool resized{};
        bool maximumCleared{};
        std::array<uint32_t, 3> color{};
        std::array<uint32_t, 3> appliedColor{};
        bool dimmed{};
        int32_t availabilityId{};
    };
    std::unordered_map<uint8_t*, LayoutRecord> LayoutRecords;

    LayoutRecord& LayoutState(const std::string& package, uint8_t* context, uint8_t* object)
    {
        auto [it, inserted] = LayoutRecords.try_emplace(object);
        auto& record = it->second;
        auto state = *reinterpret_cast<float**>(object + 0x2C);
        if (!inserted && (record.context != context || record.guid != *reinterpret_cast<uint32_t*>(object + 0x0C) ||
            record.state != state || record.name != *reinterpret_cast<uint32_t*>(object + 0x10)))
        {
            record = {};
            inserted = true;
        }
        if (inserted)
        {
            record.package = package;
            record.context = context;
            record.object = object;
            record.state = state;
            record.guid = *reinterpret_cast<uint32_t*>(object + 0x0C);
            record.name = *reinterpret_cast<uint32_t*>(object + 0x10);
            record.x = state[7];
            record.width = state[14];
            if (*reinterpret_cast<uint32_t*>(object + 0x18) == 2)
                record.maximum = *reinterpret_cast<uint32_t*>(object + 0x74);
        }
        if (record.moved && state[7] != record.appliedX)
            record.x = state[7];
        if (record.resized && state[14] != record.appliedWidth)
            record.width = state[14];
        if (record.maximumCleared && *reinterpret_cast<uint32_t*>(object + 0x74) != 0)
            record.maximum = *reinterpret_cast<uint32_t*>(object + 0x74);
        return record;
    }

    void ClearValuePages();

    void RestoreLayout()
    {
        HideHelp();
        ClearValuePages();
        auto records = std::exchange(LayoutRecords, {});
        auto owner = PageOwner.Capture();
        auto active = [&]()
        {
            auto current = PageOwner.Capture();
            return current.screen == owner.screen && current.generation == owner.generation;
        };
        for (auto& [address, record] : records)
        {
            auto context = LoadedPackage(record.package);
            if (!active())
                return;
            if (context != record.context || !FindObject)
                continue;
            auto object = FindObject(context, record.name);
            if (!active())
                return;
            if (object != address || *reinterpret_cast<uint32_t*>(object + 0x0C) != record.guid ||
                *reinterpret_cast<float**>(object + 0x2C) != record.state)
                continue;
            if (record.moved && record.state[7] == record.appliedX)
                record.state[7] = record.x;
            if (record.resized && record.state[14] == record.appliedWidth)
                record.state[14] = record.width;
            if (record.maximumCleared && *reinterpret_cast<uint32_t*>(object + 0x74) == 0)
                *reinterpret_cast<uint32_t*>(object + 0x74) = record.maximum;
            if (record.dimmed)
            {
                auto color = reinterpret_cast<uint32_t*>(record.state);
                for (size_t i = 0; i < record.color.size(); ++i)
                {
                    if (color[i] == record.appliedColor[i])
                        color[i] = record.color[i];
                }
            }
            *reinterpret_cast<uint32_t*>(object + 0x1C) |= 0x2400000;
        }
    }

    bool TextState(uint8_t* object)
    {
        if (!object || *reinterpret_cast<uint32_t*>(object + 0x18) != 2 || *reinterpret_cast<uint32_t*>(object + 0x30) < 68)
            return false;
        auto state = *reinterpret_cast<float**>(object + 0x2C);
        if (!state || !std::isfinite(state[7]) || !std::isfinite(state[14]) || state[14] <= 0.0f)
            return false;
        for (size_t i : { 4u, 5u, 6u, 10u, 11u, 12u })
        {
            if (!std::isfinite(state[i]) || std::abs(state[i]) > 0.001f)
                return false;
        }
        return std::isfinite(state[13]) && std::abs(state[13] - 1.0f) < 0.001f;
    }

    void SetAvailability(const std::string& package, uint8_t* context, uint8_t* object, bool enabled, int32_t entryId = 0)
    {
        if (!TextState(object))
            return;
        auto& record = LayoutState(package, context, object);
        if (entryId)
            record.availabilityId = entryId;
        auto color = reinterpret_cast<uint32_t*>(record.state);
        for (size_t i = 0; i < record.color.size(); ++i)
        {
            if (!record.dimmed || color[i] != record.appliedColor[i])
                record.color[i] = color[i];
        }
        // Some native selected rows use black ink on a light panel.
        auto target = *std::max_element(record.color.begin(), record.color.end()) < 64 ? 128u : 0u;
        bool changed = false;
        for (size_t i = 0; i < record.color.size(); ++i)
        {
            auto next = enabled ? record.color[i] : (record.color[i] + target + 1) / 2;
            changed |= color[i] != next;
            color[i] = record.appliedColor[i] = next;
        }
        record.dimmed = !enabled;
        if (changed)
            *reinterpret_cast<uint32_t*>(object + 0x1C) |= 0x2400000;
    }

    std::vector<uint8_t*> StringLeaves(uint8_t* object)
    {
        std::vector<uint8_t*> strings;
        size_t remaining = 4096;
        auto visit = [&](auto&& self, uint8_t* item, size_t depth) -> void
        {
            if (!item || !remaining || depth >= 32)
                return;
            --remaining;
            auto type = *reinterpret_cast<uint32_t*>(item + 0x18);
            if (type == 2)
                strings.push_back(item);
            else if (type == 5)
            {
                for (auto child = *reinterpret_cast<uint8_t**>(item + 0x64); child && remaining;
                    child = *reinterpret_cast<uint8_t**>(child + 4))
                    self(self, child, depth + 1);
            }
        };
        visit(visit, object, 0);
        return strings;
    }

    bool PublishCaption(uint8_t* object, uint32_t hash, std::string encoded, WFPMenu::Owner::Token owner)
    {
        if (!PageOwner.Matches(owner) || !SetLanguageHash)
            return false;
        encoded.resize(std::min<size_t>(encoded.size(), 1023));
        bool changed = Texts[hash] != encoded || *reinterpret_cast<uint32_t*>(object + 0x60) != hash ||
            (*reinterpret_cast<uint32_t*>(object + 0x1C) & 2) != 0 || *reinterpret_cast<uint32_t*>(object + 0x74) != 0;
        Texts[hash] = std::move(encoded);
        *reinterpret_cast<uint32_t*>(object + 0x74) = 0;
        if (changed)
        {
            *reinterpret_cast<uint32_t*>(object + 0x1C) |= 0x400000;
            SetLanguageHash(object, hash);
        }
        return PageOwner.Matches(owner);
    }

    struct ValuePages
    {
        std::string raw;
        std::string language;
        std::vector<std::string> lines;
        void* font{};
        float scale{};
        float width{};
        int gameLanguage{};
        ULONGLONG started{};
        void* charset{};
        uint32_t rawCharset{};
        void* header{};
    };
    std::unordered_map<uint8_t*, ValuePages> ContinuedValues;
    uint8_t** ActiveCharset = nullptr;
    uint32_t* RawCharset = nullptr;

    void ClearValuePages()
    {
        ContinuedValues.clear();
    }

    std::string ValueLine(uint8_t* object, const std::string& raw, float width)
    {
        auto state = *reinterpret_cast<float**>(object + 0x2C);
        auto font = FindFont ? FindFont(*reinterpret_cast<uint32_t*>(object + 0x24)) : nullptr;
        auto gameLanguage = CurrentLanguage ? *CurrentLanguage : 0;
        auto& pages = ContinuedValues[object];
        auto charset = ActiveCharset ? *ActiveCharset : nullptr;
        auto rawCharset = RawCharset ? *RawCharset : 0;
        auto header = font ? *reinterpret_cast<void**>(static_cast<uint8_t*>(font) + 24) : nullptr;
        if (pages.raw != raw || pages.language != LanguageName || pages.font != font || pages.scale != state[14] ||
            pages.width != width || pages.gameLanguage != gameLanguage || pages.charset != charset ||
            pages.rawCharset != rawCharset || pages.header != header)
        {
            pages = { raw, LanguageName, {}, font, state[14], width, gameLanguage, GetTickCount64(), charset, rawCharset, header };
            auto encoded = SingleLine(FontText(object, raw));
            for (size_t begin = 0; begin < encoded.size();)
            {
                size_t low = 1;
                size_t high = std::min<size_t>(1023, encoded.size() - begin);
                size_t count = 1;
                while (low <= high)
                {
                    auto middle = low + (high - low) / 2;
                    auto measured = MeasureText(object, encoded.substr(begin, middle));
                    if (std::isfinite(measured) && measured <= width)
                    {
                        count = middle;
                        low = middle + 1;
                    }
                    else
                        high = middle - 1;
                }
                if (begin + count < encoded.size())
                {
                    auto space = encoded.substr(begin, count).find_last_of(' ');
                    if (space != std::string::npos && space > 0)
                        count = space + 1;
                }
                pages.lines.push_back(encoded.substr(begin, count));
                begin += count;
            }
        }
        return pages.lines.empty() ? std::string() : pages.lines[size_t((GetTickCount64() - pages.started) / 12000) % pages.lines.size()];
    }

    void FitRow(uint8_t* page, int index, const char* format, uint8_t* node, bool settings, WFPMenu::Owner::Token owner)
    {
        if (!PageOwner.Matches(owner) || !FindObject || !SetLanguageHash)
            return;
        if (owner.screen != page)
        {
            auto context = *reinterpret_cast<uint8_t**>(page + 0x14);
            auto active = static_cast<uint8_t*>(owner.screen);
            if (!Pages.count(page) || !context || !active ||
                *reinterpret_cast<uint8_t**>(active + 0x14) != context || *reinterpret_cast<uint8_t**>(context + 0x34) != page)
                return;
        }
        auto entry = FindById(*reinterpret_cast<int32_t*>(node + 0x0C));
        auto screen = *reinterpret_cast<uint8_t**>(page + 0x14);
        auto name = screen ? *reinterpret_cast<const char**>(screen + 0x0C) : nullptr;
        if (!entry || !name)
            return;
        std::string package = name;
        auto context = LoadedPackage(package);
        if (!PageOwner.Matches(owner) || !context)
            return;
        char objectName[96]{};
        snprintf(objectName, sizeof(objectName), format, index);
        auto label = FindObject(context, bStringHash(objectName));
        if (!PageOwner.Matches(owner))
            return;
        auto leaves = StringLeaves(label);
        uint8_t* value = nullptr;
        uint8_t* background = nullptr;
        float valueLeft = std::numeric_limits<float>::max();
        auto available = (entry->kind != Kind::Setting && entry->kind != Kind::Action) || Enabled(*entry);
        for (auto leaf : leaves)
            SetAvailability(package, context, leaf, available, entry->entry.id);
        if (settings)
        {
            snprintf(objectName, sizeof(objectName), "OPTION_DATA_%d", index);
            value = FindObject(context, bStringHash(objectName));
            if (!PageOwner.Matches(owner))
                return;
            SetAvailability(package, context, value, available, entry->entry.id);
            snprintf(objectName, sizeof(objectName), "OPTION_DATA_BASE_%d", index);
            background = FindObject(context, bStringHash(objectName));
            if (!PageOwner.Matches(owner))
                return;
            if (TextState(value) && !leaves.empty() && TextState(leaves.front()) &&
                (*reinterpret_cast<uint32_t*>(leaves.front() + 0x6C) & 3) == 0 &&
                *reinterpret_cast<uint16_t*>(value + 0x20) == *reinterpret_cast<uint16_t*>(leaves.front() + 0x20))
            {
                auto valueState = *reinterpret_cast<float**>(value + 0x2C);
                auto& original = LayoutState(package, context, value);
                float column = float(original.maximum) * std::abs(valueState[14]);
                auto& first = LayoutState(package, context, leaves.front());
                auto right = original.x + column * 0.5f;
                auto labelText = SingleLine(FontText(leaves.front(), EntryLabel(*entry)));
                auto minimum = MeasureText(leaves.front(), labelText.substr(0, 1) + "...");
                if (!std::isfinite(minimum))
                    return;
                auto capacity = right - first.x - std::max(16.0f, minimum) - 8.0f;
                if (column > 0.0f && capacity > 0.0f && (*reinterpret_cast<uint32_t*>(value + 0x6C) & 3) == 1)
                {
                    auto text = ValueLine(value, NodeText(*entry), capacity);
                    auto width = MeasureText(value, text);
                    if (std::isfinite(width) && width <= capacity + 0.01f)
                    {
                        auto x = original.x - std::max(0.0f, width - column) * 0.5f;
                        valueState[7] = x;
                        original.appliedX = x;
                        original.moved = true;
                        original.maximumCleared = true;
                        valueLeft = x - width * 0.5f;
                        if (background && *reinterpret_cast<uint32_t*>(background + 0x18) == 1 &&
                            *reinterpret_cast<uint32_t*>(background + 0x30) >= 84 &&
                            *reinterpret_cast<uint16_t*>(background + 0x20) == *reinterpret_cast<uint16_t*>(value + 0x20))
                        {
                            auto image = *reinterpret_cast<float**>(background + 0x2C);
                            if (image && std::isfinite(image[7]) && std::isfinite(image[14]) && image[14] > 0.0f &&
                                std::isfinite(image[19]) && image[19] > 0.0f)
                            {
                                auto& box = LayoutState(package, context, background);
                                auto expanded = std::max(box.width, (width + 4.0f) / image[19]);
                                image[7] = box.x - (expanded - box.width) * image[19] * 0.5f;
                                image[14] = expanded;
                                box.appliedX = image[7];
                                box.appliedWidth = expanded;
                                box.moved = box.resized = true;
                                *reinterpret_cast<uint32_t*>(background + 0x1C) |= 0x2400000;
                            }
                        }
                        if (!PublishCaption(value, TextHash("VALUE_" + std::to_string(entry->entry.id)), text, owner))
                            return;
                    }
                }
            }
        }
        size_t ordinal = 0;
        for (auto leaf : leaves)
        {
            if (!TextState(leaf))
                continue;
            auto state = *reinterpret_cast<float**>(leaf + 0x2C);
            auto& original = LayoutState(package, context, leaf);
            float width = float(original.maximum) * std::abs(state[14]);
            if (settings && std::isfinite(valueLeft) && valueLeft != std::numeric_limits<float>::max())
                width = std::min(width, valueLeft - state[7] - 8.0f);
            if (width <= 0.0f || !std::isfinite(MeasureText(leaf, "W")))
                continue;
            auto text = WFPMenuLayout::Fit(SingleLine(FontText(leaf, EntryLabel(*entry))), width, [&](const std::string& text)
            {
                return MeasureText(leaf, text);
            });
            original.maximumCleared = true;
            auto hash = TextHash("LABEL_" + std::to_string(entry->entry.id) + "_" + std::to_string(ordinal++));
            if (!PublishCaption(leaf, hash, text, owner))
                return;
        }
    }

    double __fastcall StockGlyphAdvance(void* font, void*, uint32_t code, uint16_t previous, uint32_t flags)
    {
        FontModeScope stock(false);
        return GlyphAdvance(font, code, previous, flags);
    }

    NFSPSHelp::Captions HelpCaptions;
    struct HelpView
    {
        std::string package;
        std::string text;
        std::string language;
        std::vector<std::string> lines;
        uint64_t generation = 0;
        int32_t id = 0;
        float width = 0.0f;
        float scale = 0.0f;
        size_t rows = 0;
        void* font = nullptr;
        void* header = nullptr;
        void* charset = nullptr;
        uint32_t rawCharset = 0;
        int gameLanguage = 0;
        ULONGLONG started = 0;
    } Help;

    void HideHelp()
    {
        auto package = std::move(Help.package);
        Help = {};
        if (!package.empty())
            HelpCaptions.Hide(LoadedPackage(package), package);
    }

    std::string HelpText(const MenuEntry& entry)
    {
        auto text = EntryLabel(entry);
        auto value = NodeText(entry);
        if (!value.empty())
            text += ": " + value;
        if (entry.kind != Kind::Menu)
        {
            auto row = RowOf(entry);
            if (row.plugin && (row.setting || entry.kind == Kind::Preset || entry.kind == Kind::RestoreDefaults))
            {
                auto description = WFPPlugins::Help(row);
                if (!description.empty())
                    text += " - " + description;
            }
        }
        return text;
    }

    void UpdateHelp(uint8_t* page)
    {
        static std::string metricsLanguage;
        static int metricsGameLanguage = INT_MIN;
        auto gameLanguage = CurrentLanguage ? *CurrentLanguage : 0;
        if (metricsLanguage != LanguageName || metricsGameLanguage != gameLanguage)
        {
            metricsLanguage = LanguageName;
            metricsGameLanguage = gameLanguage;
            HelpCaptions.ResetMetrics();
            HideHelp();
        }
        auto owner = PageOwner.Capture();
        auto pin = Plugins;
        auto active = [&]() { return PageOwner.Matches(owner) && Plugins == pin; };
        if ((NFSPSDialog::Pending && NFSPSDialog::Busy()) ||
            !page || owner.screen != page || !Pages.count(page) || !FindObject ||
            *reinterpret_cast<int32_t*>(page + 0x1C) <= 0 ||
            (*reinterpret_cast<uintptr_t**>(page) == OptionsPageTable && *reinterpret_cast<int32_t*>(page + PageState) != 0))
        {
            HideHelp();
            return;
        }
        auto node = OwnNode(page);
        auto entry = node ? FindById(*reinterpret_cast<int32_t*>(node + 0x0C)) : nullptr;
        auto selectedIndex = *reinterpret_cast<int32_t*>(page + 0x20);
        auto format = PageFormats.find(page);
        auto screen = *reinterpret_cast<uint8_t**>(page + 0x14);
        auto name = screen ? *reinterpret_cast<const char**>(screen + 12) : nullptr;
        if (!entry || selectedIndex < 0 || selectedIndex >= int32_t(ScreenRows) || format == PageFormats.end() || !name)
        {
            HideHelp();
            return;
        }
        std::string packageName = name;
        auto package = LoadedPackage(packageName);
        if (!active())
            return;
        if (!package)
        {
            HideHelp();
            return;
        }
        auto rowFormat = format->second;
        auto groupFormat = rowFormat.starts_with("OPTION_HEADER_") ? "OPTION_GROUP_%d" : rowFormat.c_str();
        char nameBuffer[96];
        snprintf(nameBuffer, sizeof(nameBuffer), groupFormat, selectedIndex + 1);
        auto group = FindObject(package, bStringHash(nameBuffer));
        if (!active())
            return;
        auto rowCount = std::min(int32_t(ScreenRows), *reinterpret_cast<int32_t*>(page + 0x1C));
        snprintf(nameBuffer, sizeof(nameBuffer), groupFormat, rowCount);
        auto lastGroup = FindObject(package, bStringHash(nameBuffer));
        if (!active())
            return;
        snprintf(nameBuffer, sizeof(nameBuffer), rowFormat.c_str(), selectedIndex + 1);
        auto label = FindObject(package, bStringHash(nameBuffer));
        if (!active())
            return;
        auto leaves = StringLeaves(label);
        auto selected = std::find_if(leaves.begin(), leaves.end(), [](uint8_t* object)
        {
            return (*reinterpret_cast<uint32_t*>(object + 0x1C) & 0x41) == 0;
        });
        if (selected == leaves.end() || !group || !lastGroup)
        {
            HideHelp();
            return;
        }
        if (!Help.package.empty() && Help.package != packageName)
            HideHelp();
        if (!active())
            return;
        auto maximum = [&](uint8_t* object)
        {
            auto found = LayoutRecords.find(object);
            return found != LayoutRecords.end() && found->second.guid == *reinterpret_cast<uint32_t*>(object + 0x0C) &&
                found->second.state == *reinterpret_cast<float**>(object + 0x2C) ?
                found->second.maximum : *reinterpret_cast<uint32_t*>(object + 0x74);
        };
        float width = 0.0f;
        auto captions = HelpCaptions.Prepare(package, packageName, *selected, group, lastGroup,
            FindFont, gameLanguage, width, maximum, active);
        if (!active())
            return;
        if (captions.empty())
        {
            // Native animation and cache updates can temporarily invalidate placement.
            // Keep the selected row's continuation clock until it can be drawn again.
            HelpCaptions.Hide(package, packageName);
            return;
        }
        Help.package = packageName;
        // Font conversion can call native language hooks; measuring uses a captured caption state.
        std::array<uint8_t, 120> measure;
        std::array<float, 17> state;
        std::memcpy(measure.data(), captions.front(), measure.size());
        std::memcpy(state.data(), *reinterpret_cast<void**>(measure.data() + 0x2C), sizeof(state));
        *reinterpret_cast<float**>(measure.data() + 0x2C) = state.data();
        auto text = HelpText(*entry);
        auto font = FindFont ? FindFont(*reinterpret_cast<uint32_t*>(measure.data() + 0x24)) : nullptr;
        if (!active())
            return;
        auto header = font ? *reinterpret_cast<void**>(static_cast<uint8_t*>(font) + 24) : nullptr;
        auto charset = ActiveCharset ? *ActiveCharset : nullptr;
        auto rawCharset = RawCharset ? *RawCharset : 0;
        if (Help.text != text || Help.language != LanguageName || Help.generation != owner.generation || Help.id != entry->entry.id ||
            Help.width != width || Help.scale != state[14] || Help.rows != captions.size() || Help.font != font ||
            Help.header != header || Help.charset != charset || Help.rawCharset != rawCharset || Help.gameLanguage != gameLanguage)
        {
            auto encoded = FontText(measure.data(), text, false);
            if (!active())
                return;
            auto lines = WFPMenuLayout::Wrap(encoded, width, [&](const std::string& line)
            {
                return active() ? MeasureText(measure.data(), line) : INFINITY;
            });
            if (!active())
                return;
            Help = { packageName, std::move(text), LanguageName, std::move(lines), owner.generation, entry->entry.id,
                width, state[14], captions.size(), font, header, charset, rawCharset, gameLanguage, GetTickCount64() };
        }
        auto pages = std::max<size_t>(1, (Help.lines.size() + captions.size() - 1) / captions.size());
        auto first = size_t((GetTickCount64() - Help.started) / 12000) % pages * captions.size();
        for (size_t i = 0; i < captions.size(); ++i)
        {
            auto line = first + i < Help.lines.size() ? Help.lines[first + i] : std::string();
            auto object = captions[i];
            if (!PublishCaption(object, TextHash("HELP_" + std::to_string(i)), line, owner) || !active())
                return;
            if (line.empty())
                *reinterpret_cast<uint32_t*>(object + 0x1C) |= 1;
            else
                *reinterpret_cast<uint32_t*>(object + 0x1C) &= ~1u;
        }
    }

    thread_local uint8_t* PopupSource = nullptr;
    struct PopupScope
    {
        uint8_t* previous;
        explicit PopupScope(uint8_t* node) : previous(PopupSource)
        {
            PopupSource = node;
        }
        ~PopupScope()
        {
            PopupSource = previous;
        }
    };

    struct PopupCaption
    {
        std::string text;
        WFPMenu::Owner::Token owner;
    };
    std::unordered_map<uint8_t*, PopupCaption> PopupCaptions;
    std::array<uintptr_t, 4> PopupItemTable{};
    using PopupDeleteFn = void*(__fastcall*)(uint8_t*, void*, uint8_t);
    PopupDeleteFn OriginalPopupDelete = nullptr;
    void* __fastcall DeletePopupItem(uint8_t* item, void* edx, uint8_t flags)
    {
        NativeScope native;
        PopupCaptions.erase(item);
        return OriginalPopupDelete(item, edx, flags);
    }

    SafetyHookInline shPopupItem;
    uint8_t* __fastcall ConstructPopupItem(uint8_t* item, void* edx, const char* text, int value)
    {
        NativeScope native;
        if (!PopupSource || !LiteralStrings || !LiteralStrings->count(text))
            return shPopupItem.unsafe_fastcall<uint8_t*>(item, edx, text, value);
        std::string full = LiteralStrings->at(text);
        if (auto published = PublishedChoices.find(PopupSource); published != PublishedChoices.end())
        {
            auto& data = published->second;
            if (value >= 0 && value < int(data.texts.size()))
                full = data.texts[value];
            else if (value == -1)
                full = data.text;
        }
        // The native constructor copies into an inline char[64]. Drawing uses the full owned text.
        auto bounded = GameText(full).substr(0, 63);
        auto result = shPopupItem.unsafe_fastcall<uint8_t*>(item, edx, bounded.c_str(), value);
        if (!PopupItemTable[0])
        {
            auto table = *reinterpret_cast<uintptr_t**>(result);
            std::copy_n(table, PopupItemTable.size(), PopupItemTable.begin());
            OriginalPopupDelete = reinterpret_cast<PopupDeleteFn>(PopupItemTable[0]);
            PopupItemTable[0] = reinterpret_cast<uintptr_t>(&DeletePopupItem);
        }
        PopupCaptions[result] = { std::move(full), PageOwner.Capture() };
        *reinterpret_cast<uintptr_t**>(result) = PopupItemTable.data();
        return result;
    }

    SafetyHookInline shPopupDraw;
    int __fastcall DrawPopupItem(uint8_t* renderer, void* edx, uint8_t* item, uint32_t selected, int animation)
    {
        NativeScope native;
        auto found = PopupCaptions.find(item);
        if (found == PopupCaptions.end())
            return shPopupDraw.unsafe_fastcall<int>(renderer, edx, item, selected, animation);
        if (!PageOwner.Matches(found->second.owner))
            return 0;
        auto caption = found->second;
        auto object = *reinterpret_cast<uint8_t**>(renderer + 12);
        auto text = GameText(caption.text);
        std::string package;
        uint8_t* context = nullptr;
        auto page = static_cast<uint8_t*>(caption.owner.screen);
        auto screen = *reinterpret_cast<uint8_t**>(page + 0x14);
        auto name = screen ? *reinterpret_cast<const char**>(screen + 12) : nullptr;
        if (name)
        {
            package = name;
            context = LoadedPackage(package);
            if (!PageOwner.Matches(caption.owner))
                return 0;
        }
        if (TextState(object))
        {
            auto state = *reinterpret_cast<float**>(object + 0x2C);
            auto maximum = *reinterpret_cast<uint32_t*>(object + 0x74);
            if (context)
                maximum = LayoutState(package, context, object).maximum;
            if (maximum)
                text = ValueLine(object, caption.text, float(maximum) * std::abs(state[14]));
        }
        // The native renderer reads the datum synchronously; it never retains this caption proxy.
        std::vector<uint8_t> proxy(std::max<size_t>(88, 24 + text.size() + 1));
        std::memcpy(proxy.data(), item, 24);
        std::memcpy(proxy.data() + 24, text.c_str(), text.size() + 1);
        LiteralScope literal(reinterpret_cast<const char*>(proxy.data() + 24), true);
        auto result = shPopupDraw.unsafe_fastcall<int>(renderer, edx, proxy.data(), selected, animation);
        if (PageOwner.Matches(caption.owner) && context && TextState(object))
        {
            auto& layout = LayoutState(package, context, object);
            layout.maximumCleared = true;
            PublishCaption(object, TextHash("POPUP_" + std::to_string(*reinterpret_cast<uint32_t*>(object + 0x10))), text, caption.owner);
        }
        return result;
    }

    void* (__thiscall* CreateFontCache)(void*, uint8_t*, void*) = nullptr;
    SafetyHookInline shRenderString;
    uintptr_t __fastcall RenderString(void* renderer, void* edx, uint8_t* object, void* cache)
    {
        NativeScope scope;
        bool own = object && (*reinterpret_cast<uint32_t*>(object + 0x1C) & 2) == 0 &&
            Texts.contains(*reinterpret_cast<uint32_t*>(object + 0x60));
        FontModeScope modes(own);
        if (own)
        {
            auto layout = LayoutRecords.find(object);
            if (layout != LayoutRecords.end() && layout->second.availabilityId &&
                layout->second.guid == *reinterpret_cast<uint32_t*>(object + 0x0C) &&
                layout->second.name == *reinterpret_cast<uint32_t*>(object + 0x10) &&
                layout->second.state == *reinterpret_cast<float**>(object + 0x2C))
            {
                if (auto entry = FindById(layout->second.availabilityId))
                {
                    auto available = (entry->kind != Kind::Setting && entry->kind != Kind::Action) || Enabled(*entry);
                    SetAvailability(layout->second.package, layout->second.context, object, available);
                }
            }
        }
        uintptr_t result = 0;
        if (own && !cache && CreateFontCache)
        {
            auto font = FindFont ? static_cast<uint8_t*>(FindFont(*reinterpret_cast<uint32_t*>(object + 0x24))) : nullptr;
            auto texture = font ? *reinterpret_cast<void**>(font + 20) : nullptr;
            if (texture)
                cache = CreateFontCache(renderer, object, texture);
            // The native glyph loop writes through its cache without checking allocation failure.
            if (cache)
                result = shRenderString.unsafe_fastcall<uintptr_t>(renderer, edx, object, cache);
        }
        else
            result = shRenderString.unsafe_fastcall<uintptr_t>(renderer, edx, object, cache);
        return result;
    }


    using ShowFn = int(__fastcall*)(uint8_t*, void*, int, const char*, uint8_t*);
    ShowFn OriginalBaseShow = nullptr;
    ShowFn OriginalShow = nullptr;

    int __fastcall BaseShow(uint8_t* page, void* edx, int index, const char* format, uint8_t* node)
    {
        NativeScope native;
        SyncActiveContext(*reinterpret_cast<uint8_t**>(page + 0x14));
        LiteralScope literal(node && FindById(*reinterpret_cast<int32_t*>(node + 0x0C)) ? node : nullptr);
        auto owner = PageOwner.Capture();
        PageFormats[page] = format;
        auto result = OriginalBaseShow(page, edx, index, format, node);
        if (PageOwner.Matches(owner))
            FitRow(page, index, format, node, false, owner);
        return result;
    }

    int __fastcall Show(uint8_t* page, void* edx, int index, const char* format, uint8_t* node)
    {
        NativeScope native;
        SyncActiveContext(*reinterpret_cast<uint8_t**>(page + 0x14));
        if (node && !PublishedChoices.count(node))
        {
            auto entry = FindByDalId(*reinterpret_cast<int32_t*>(node + NodeDalId));
            if (entry && ListedSnapshots.count(entry->entry.id))
                PublishedChoices.emplace(node, ListedSnapshots.at(entry->entry.id));
        }
        LiteralScope literal(node && FindByDalId(*reinterpret_cast<int32_t*>(node + NodeDalId)) ? node : nullptr);
        auto owner = PageOwner.Capture();
        // Popup close also redraws hidden base rows on this derived page.
        constexpr auto rowFormat = "OPTION_HEADER_%d";
        PageFormats[page] = rowFormat;
        auto result = OriginalShow(page, edx, index, format, node);
        if (PageOwner.Matches(owner))
            FitRow(page, index, rowFormat, node, true, owner);
        return result;
    }

    void* (__cdecl* CrtMalloc)(size_t) = nullptr;
    void (__cdecl* CrtFree)(void*) = nullptr;

    SafetyHookInline shConstructDialog, shDialogNotify;
    int (__stdcall* CloseDialog)(uint32_t) = nullptr;

    uint8_t* __fastcall ConstructDialog(uint8_t* dialog, void*, void* screen)
    {
        auto own = NFSPSDialog::Pending && NFSPSDialog::Publishing && *NFSPSDialog::Pending == NFSPSDialog::Publishing;
        NFSPSDialog::OwnedInstance = own ? dialog : nullptr;
        if (own)
            NFSPSDialog::Publishing = nullptr;
        return shConstructDialog.thiscall<uint8_t*>(dialog, screen);
    }

    int __fastcall DialogNotify(uint8_t* dialog, void*, uint32_t message, uintptr_t data, int controller, int extra)
    {
        if (dialog == NFSPSDialog::OwnedInstance && NFSPSDialog::Instance && *NFSPSDialog::Instance == dialog &&
            (message == 0x911AB364 || message == 0x0C407210 || message == 0x406415E3) && dialog[0x28] == 1 && CloseDialog)
        {
            // The stock acknowledgement belongs to its Options controller's current state.
            NFSPSDialog::OwnedInstance = nullptr;
            return CloseDialog(0x93E8A57C);
        }
        return shDialogNotify.thiscall<int>(dialog, message, data, controller, extra);
    }

    bool RestartChange = false;
    bool RestartWarned = false;

    void RestartNotice(uint8_t* page)
    {
        auto owner = PageOwner.Capture();
        auto pin = Plugins;
        auto current = [&]() { return owner.screen == page && PageOwner.Matches(owner) && Plugins == pin; };
        if (!RestartChange || RestartWarned || !page || !current() || !Pages.count(page) || NFSPSDialog::Busy() ||
            !LocalizedWide || !FindFont || !FindFont(0x94AF4029) || !current() ||
            (*reinterpret_cast<uintptr_t**>(page) == OptionsPageTable && *reinterpret_cast<int32_t*>(page + PageState) != 0))
            return;
        std::array<uint8_t, 120> style{};
        *reinterpret_cast<uint32_t*>(style.data() + 0x24) = 0x94AF4029;
        auto encoded = FontText(style.data(), WFPText::Get("WFP_RESTART_WARNING"), false);
        std::vector<wchar_t> body;
        std::array<wchar_t, 256> button{};
        if (!current() || !DecodeText(encoded, body) || !current() ||
            !LocalizedWide(button.data(), int(button.size()), 0x13950307) || !current())
            return;
        button.back() = L'\0';
        HideHelp();
        if (current() && NFSPSDialog::Show(body.data(), button.data(), current))
            RestartWarned = true;
    }

    bool SetText(char*& text, const std::string& value, WFPMenu::Owner::Token owner)
    {
        auto copy = static_cast<char*>(CrtMalloc(value.size() + 1));
        if (!copy)
            return PageOwner.Matches(owner);
        memcpy(copy, value.c_str(), value.size() + 1);
        if (!PageOwner.Matches(owner))
        {
            CrtFree(copy);
            return false;
        }
        auto previous = std::exchange(text, copy);
        CrtFree(previous);
        return PageOwner.Matches(owner);
    }

    // Shows the values of the page's rows again, after a preset, Restore Defaults or another editor changed them
    void SwapItems(ListLink* first, ListLink* second)
    {
        auto a = *first;
        auto b = *second;
        bool aEmpty = a.next == first;
        bool bEmpty = b.next == second;
        first->next = first->prev = first;
        second->next = second->prev = second;
        if (!bEmpty)
        {
            *first = b;
            first->next->prev = first;
            first->prev->next = first;
        }
        if (!aEmpty)
        {
            *second = a;
            second->next->prev = second;
            second->prev->next = second;
        }
    }

    bool RefreshChoices(uint8_t* node, const MenuEntry& entry, WFPMenu::Owner::Token owner)
    {
        if (entry.kind != Kind::Setting || *reinterpret_cast<int32_t*>(node + 0x24) != TypeList)
            return true;
        auto data = Data(entry);
        if (!data)
            return false;
        auto current = PublishedChoices.find(node);
        if (current != PublishedChoices.end() && current->second.values == data->values && current->second.texts == data->texts &&
            (ChoiceIndex(*RowOf(entry).setting, data->values, data->value) >= 0 || (current->second.value == data->value && current->second.text == data->text)))
            return true;
        auto snapshot = *data;
        auto replacement = static_cast<uint8_t*>(MakeNode(const_cast<TreeEntry*>(&entry.entry)));
        if (!replacement)
            return false;
        auto destroy = reinterpret_cast<void(__fastcall*)(void*, void*, int)>((*reinterpret_cast<uintptr_t**>(replacement))[0]);
        if (PageOwner.Matches(owner))
        {
            SwapItems(reinterpret_cast<ListLink*>(node + 0x28), reinterpret_cast<ListLink*>(replacement + 0x28));
            PublishedChoices[node] = std::move(snapshot);
        }
        destroy(replacement, nullptr, 1);
        return PageOwner.Matches(owner);
    }

    void Refresh(uint8_t* page)
    {
        NativeScope scope;
        if (!CrtMalloc || !CrtFree)
            return;
        auto format = PageFormats.find(page);
        if (format == PageFormats.end())
            return;
        auto rowFormat = format->second;
        auto owner = PageOwner.Capture();
        if (owner.screen != page)
            return;
        auto nodes = reinterpret_cast<ListLink*>(page + PageNodes);
        int row = 0;
        for (auto link = nodes->next; link && link != nodes; link = link->next, ++row)
        {
            auto node = reinterpret_cast<uint8_t*>(link) - 4;
            auto entry = FindById(*reinterpret_cast<int32_t*>(node + 0x0C));
            if (!entry)
                continue;
            if (!RefreshChoices(node, *entry, owner))
                return;
            if (entry->kind != Kind::Menu)
            {
                if (!SetText(*reinterpret_cast<char**>(node + NodeValueText), GameText(NodeText(*entry)), owner))
                    return;
                *reinterpret_cast<float*>(node + NodeValueAt) = NodeValue(*entry);
            }
            auto show = reinterpret_cast<void(__fastcall*)(void*, void*, int, const char*, void*)>((*reinterpret_cast<uintptr_t**>(page))[11]);
            show(page, nullptr, row + 1, rowFormat.c_str(), node);
            if (!PageOwner.Matches(owner))
                return;
        }
    }

    SafetyHookInline shGameDefaults;
    using GameDefaultsFn = void(__fastcall*)(uint8_t*, void*);
    GameDefaultsFn OriginalGameDefaults = nullptr;

    void __fastcall GameDefaults(uint8_t* manager, void* edx)
    {
        NativeScope scope;
        auto owner = PageOwner.Capture();
        auto page = static_cast<uint8_t*>(owner.screen);
        auto origin = PageParents.find(page);
        auto context = *reinterpret_cast<int32_t*>(manager + 8) == 2 ? DefaultsPauseContext : DefaultsMainContext;
        auto pin = Plugins;
        std::vector<int32_t> entries;
        if (page && Pages.count(page) && origin != PageParents.end() && context && *context &&
            *reinterpret_cast<uint8_t**>(page + 0x14) == *context && *reinterpret_cast<int32_t*>(page + PageState) == 0)
        {
            auto list = reinterpret_cast<ListLink*>(page + PageNodes);
            for (auto link = list->next; link && link != list; link = link->next)
            {
                auto node = reinterpret_cast<uint8_t*>(link) - 4;
                auto entry = FindByDalId(*reinterpret_cast<int32_t*>(node + NodeDalId));
                if (entry && entry->kind == Kind::Setting)
                    entries.push_back(entry->entry.id);
            }
        }
        if (entries.empty())
        {
            OriginalGameDefaults(manager, edx);
            return;
        }
        auto source = origin->second;
        auto screen = *context;
        auto target = owner;
        {
            struct Restore
            {
                DefaultsContext previous = NativeDefaults;
                ~Restore()
                {
                    NativeDefaults = previous;
                }
            } restore;
            // The native fixed table cannot resolve the parent of an added row.
            NativeDefaults = { owner, source.parent, source.pause, screen };
            OriginalGameDefaults(manager, edx);
            if (NativeDefaults.replacement.screen)
                target = NativeDefaults.replacement;
        }
        if (!PageOwner.Matches(target) || Plugins != pin)
            return;
        page = static_cast<uint8_t*>(target.screen);
        auto currentSource = PageParents.find(page);
        auto active = reinterpret_cast<ListLink*>(screen + 0x20);
        if (currentSource == PageParents.end() || currentSource->second.parent != source.parent || currentSource->second.pause != source.pause ||
            *reinterpret_cast<uint8_t**>(page + 0x14) != screen || active->prev != reinterpret_cast<ListLink*>(page + 4))
            return;
        std::unordered_set<int32_t> present;
        auto list = reinterpret_cast<ListLink*>(page + PageNodes);
        for (auto link = list->next; link && link != list; link = link->next)
            present.insert(*reinterpret_cast<int32_t*>(reinterpret_cast<uint8_t*>(link) - 4 + 0x0C));
        std::unordered_set<HMODULE> queued;
        for (auto id : entries)
        {
            auto entry = FindById(id);
            if (entry && present.count(id) && queued.insert(entry->module).second)
                Queue(*entry, Command::Kind::RestorePage);
        }
    }

    void InitializePreset(uint8_t* page)
    {
        PresetRow = {};
        RestoreRow.Cancel();
        auto nodes = reinterpret_cast<ListLink*>(page + PageNodes);
        for (auto link = nodes->next; link && link != nodes; link = link->next)
        {
            auto node = reinterpret_cast<uint8_t*>(link) - 4;
            auto entry = FindByDalId(*reinterpret_cast<int32_t*>(node + NodeDalId));
            if (entry && entry->kind == Kind::Preset)
            {
                PresetRow.plugin = PluginOf(*entry);
                auto found = CachedPresets.find(entry->module);
                PresetRow.shown = found != CachedPresets.end() ? found->second : -1;
                break;
            }
        }
    }

    void DeleteRow(uint8_t* node)
    {
        if (node)
            reinterpret_cast<void (__thiscall*)(uint8_t*, int)>((*reinterpret_cast<uintptr_t**>(node))[0])(node, 1);
    }

    bool RetirePreview(uint8_t* page, WFPMenu::Owner::Token owner)
    {
        auto context = *reinterpret_cast<uint8_t**>(page + 0x14);
        if (!context)
            return false;
        auto preview = *reinterpret_cast<uint8_t**>(context + 0x34);
        if (!preview)
            return true;
        reinterpret_cast<void (__thiscall*)(uint8_t*)>((*reinterpret_cast<uintptr_t**>(preview))[4])(preview);
        if (!PageOwner.Matches(owner))
            return false;
        if (*reinterpret_cast<uint8_t**>(context + 0x34) != preview)
            return true;
        DeleteRow(preview);
        if (!PageOwner.Matches(owner))
            return false;
        if (*reinterpret_cast<uint8_t**>(context + 0x34) == preview)
            *reinterpret_cast<uint8_t**>(context + 0x34) = nullptr;
        return true;
    }

    void ShowRowGroups(uint8_t* page, size_t count, WFPMenu::Owner::Token owner)
    {
        auto format = PageFormats.find(page);
        if (format == PageFormats.end() || !FindObject)
            return;
        auto rowFormat = format->second.starts_with("OPTION_HEADER_") ? std::string("OPTION_GROUP_%d") : format->second;
        auto screen = *reinterpret_cast<uint8_t**>(page + 0x14);
        auto name = screen ? *reinterpret_cast<const char**>(screen + 12) : nullptr;
        if (!name)
            return;
        auto package = LoadedPackage(name);
        if (!package || !PageOwner.Matches(owner))
            return;
        for (size_t i = 0; i < ScreenRows; ++i)
        {
            char rowName[96];
            snprintf(rowName, sizeof(rowName), rowFormat.c_str(), int(i + 1));
            auto group = FindObject(package, bStringHash(rowName));
            if (!PageOwner.Matches(owner))
                return;
            if (!group || *reinterpret_cast<uint32_t*>(group + 0x18) != 5)
                continue;
            auto& flags = *reinterpret_cast<uint32_t*>(group + 0x1C);
            auto next = i < count ? flags & ~1u : flags | 1u;
            if (next != flags)
                flags = next | 0x2400000;
        }
    }

    bool ReconcileRows(uint8_t* page)
    {
        NativeScope native;
        auto source = PageParents.find(page);
        if (!page || !Pages.count(page) || source == PageParents.end() || !RepaintRows ||
            (NFSPSDialog::Pending && NFSPSDialog::Busy()))
            return true;
        auto table = *reinterpret_cast<uintptr_t**>(page);
        if ((table != BasePageTable && table != OptionsPageTable) ||
            (table == OptionsPageTable && *reinterpret_cast<int32_t*>(page + PageState) != 0))
            return true;
        auto screen = *reinterpret_cast<uint8_t**>(page + 0x14);
        if (!screen || screen[0x32])
            return true;
        auto origin = source->second;
        bool ownPage = FindById(origin.parent) != nullptr;
        bool joined = !ownPage && HasPlacements(origin.parent);
        if (!ownPage && !joined)
            return true;
        auto owner = PageOwner.Capture();
        auto pin = Plugins;
        auto active = [&]() { return owner.screen == page && PageOwner.Matches(owner) && Plugins == pin; };
        if (!active())
            return false;
        auto list = reinterpret_cast<ListLink*>(page + PageNodes);
        std::vector<uint8_t*> previous;
        for (auto link = list->next; link && link != list; link = link->next)
        {
            if (previous.size() == ScreenRows)
                return false;
            previous.push_back(reinterpret_cast<uint8_t*>(link) - 4);
        }
        std::vector<int32_t> desired;
        auto visible = [&](const MenuEntry& entry)
        {
            if (entry.kind == Kind::Setting || entry.kind == Kind::Action)
                return Shown(entry);
            if (entry.kind == Kind::RestoreDefaults)
            {
                return std::any_of(Entries.begin(), Entries.end(), [&](const MenuEntry& other)
                {
                    return other.entry.parent == origin.parent && other.kind == Kind::Setting && Shown(other);
                });
            }
            return true;
        };
        if (ownPage)
        {
            for (auto& entry : Entries)
            {
                if (entry.entry.parent == origin.parent && entry.root && visible(entry))
                    desired.push_back(entry.entry.id);
            }
        }
        else
        {
            for (auto node : previous)
            {
                auto id = *reinterpret_cast<int32_t*>(node + 0x0C);
                if (FindById(id))
                    continue;
                desired.push_back(id);
                for (auto& placement : Placements)
                {
                    if (placement.screen != origin.parent || placement.after != id)
                        continue;
                    for (auto& entry : Entries)
                    {
                        if (!entry.root && entry.entry.parent == origin.parent && entry.setting == placement.setting &&
                            Data(entry) && Data(entry)->visible)
                            desired.push_back(entry.entry.id);
                    }
                }
            }
        }
        if (desired.size() > ScreenRows)
            return false;
        if (desired.size() == previous.size() && std::equal(desired.begin(), desired.end(), previous.begin(), [](int32_t id, uint8_t* node)
        {
            return id == *reinterpret_cast<int32_t*>(node + 0x0C);
        }))
            return true;
        std::vector<uint8_t*> next, added, removed;
        auto rollback = [&]()
        {
            for (auto node : added)
                DeleteRow(node);
        };
        for (auto id : desired)
        {
            auto found = std::find_if(previous.begin(), previous.end(), [&](uint8_t* node)
            {
                return *reinterpret_cast<int32_t*>(node + 0x0C) == id;
            });
            if (found != previous.end())
                next.push_back(*found);
            else
            {
                auto entry = FindById(id);
                auto node = entry ? static_cast<uint8_t*>(MakeNode(&entry->entry)) : nullptr;
                if (node)
                    added.push_back(node);
                if (!node || !active())
                {
                    rollback();
                    return false;
                }
                next.push_back(node);
            }
        }
        for (auto node : previous)
        {
            if (std::find(next.begin(), next.end(), node) == next.end())
                removed.push_back(node);
        }
        auto selected = *reinterpret_cast<uint8_t**>(page + PageCurrent);
        auto selectedId = selected ? *reinterpret_cast<int32_t*>(selected + 0x0C) : -1;
        auto index = size_t(std::max(0, *reinterpret_cast<int32_t*>(page + 0x20)));
        if (auto found = std::find(desired.begin(), desired.end(), selectedId); found != desired.end())
            index = size_t(found - desired.begin());
        else
            index = next.empty() ? 0 : std::min(index, next.size() - 1);
        std::vector<std::string> published;
        if (joined)
        {
            for (auto id : desired)
            {
                if (auto entry = FindById(id); entry && !entry->root && entry->kind == Kind::Setting)
                    published.push_back(entry->setting);
            }
        }
        RestoreLayout();
        if (!active() || !RetirePreview(page, owner) || !active())
        {
            rollback();
            return false;
        }
        if (!previous.empty())
        {
            reinterpret_cast<void (__thiscall*)(uint8_t*)>(table[13])(page);
            if (!active())
            {
                rollback();
                return false;
            }
        }
        // All allocations succeeded. Surviving nodes keep their original native ownership and pointers.
        ++Lifetime;
        PageOwner.Bind(page);
        owner = PageOwner.Capture();
        PauseMenuActive = origin.pause;
        std::erase_if(Commands, [&](const Command& command) { return command.owner.screen == page; });
        list->next = list->prev = list;
        for (auto node : next)
            Append(list, reinterpret_cast<ListLink*>(node + 4));
        *reinterpret_cast<int32_t*>(page + 0x1C) = int32_t(next.size());
        *reinterpret_cast<int32_t*>(page + 0x20) = int32_t(index);
        *reinterpret_cast<uint8_t**>(page + PageCurrent) = next.empty() ? nullptr : next[index];
        if (joined)
            JoinedSettings[origin.pause ? 1 : 0][origin.parent] = std::move(published);
        for (auto node : removed)
        {
            PublishedChoices.erase(node);
            auto link = reinterpret_cast<ListLink*>(node + 4);
            link->next = link->prev = link;
            DeleteRow(node);
        }
        if (!active())
            return false;
        InitializePreset(page);
        if (!PageFormats.count(page))
            PageFormats[page] = table == OptionsPageTable ? "OPTION_HEADER_%d" : "ITEM_GROUP_%d";
        if (!next.empty())
            RepaintRows(page);
        else
        {
            reinterpret_cast<void (__thiscall*)(uint8_t*, int)>(table[2])(page, 0);
            if (active())
                screen[0x32] = 0;
        }
        if (!active())
            return false;
        ShowRowGroups(page, next.size(), owner);
        return active();
    }

    void TouchPage(uint8_t* page)
    {
        if (auto source = PageParents.find(page); source != PageParents.end())
            PauseMenuActive = source->second.pause;
        if (PageOwner.Get() == page)
            return;
        RestoreLayout();
        PageOwner.Bind(page);
        LastContext = *reinterpret_cast<uint8_t**>(page + 0x14);
        RefreshNeeded = true;
        InitializePreset(page);
    }

    MenuEntry* Highlighted(uint8_t* page)
    {
        if (*reinterpret_cast<int32_t*>(page + PageState) != 0)
            return nullptr;
        auto node = *reinterpret_cast<uint8_t**>(page + PageCurrent);
        return node ? FindByDalId(*reinterpret_cast<int32_t*>(node + NodeDalId)) : nullptr;
    }

    // Accept on a row: the Preset and Restore Defaults rows ask first and act on the second accept; lists of settings
    // that can't change now don't open
    using ActivateFn = int(__fastcall*)(uint8_t*, void*);
    ActivateFn OriginalActivate = nullptr;
    int __fastcall Activate(uint8_t* page, void* edx)
    {
        NativeScope scope;
        TouchPage(page);
        if (*reinterpret_cast<int32_t*>(page + 0x1C) <= 0)
            return 0;
        if (auto entry = Highlighted(page))
        {
            auto row = RowOf(*entry);
            switch (entry->kind)
            {
                case Kind::Preset:
                    Queue(*entry, Command::Kind::Accept, {}, PresetRow.shown);
                    return 0;
                case Kind::RestoreDefaults:
                    if (row.plugin)
                    {
                        Queue(*entry, Command::Kind::Accept);
                    }
                    return 0;
                case Kind::Action:
                    if (row.plugin && row.setting && Enabled(*entry))
                        Queue(*entry, Command::Kind::Accept);
                    return 0;
                case Kind::Setting:
                    if (!row.setting || !Enabled(*entry))
                        return 0;
                    if (auto node = OwnNode(page); node && *reinterpret_cast<int32_t*>(node + 0x24) == TypeList)
                    {
                        auto items = reinterpret_cast<ListLink*>(node + 0x28);
                        if (!items->next || items->next == items)
                            return 0;
                    }
                    break;
                default:
                    break;
            }
        }
        LiteralScope literal(OwnNode(page));
        PopupScope popup(OwnNode(page));
        return OriginalActivate(page, edx);
    }

    // The page's messages: left and right pick a preset on the Preset row; moving away cancels a pending confirmation
    constexpr uint32_t PadLeft = 0x9120409E;
    constexpr uint32_t PadRight = 0xB5971BF1;
    constexpr uint32_t PadUp = 0x72619778;
    constexpr uint32_t PadDown = 0x911C0A4B;
    constexpr uint32_t PadBack = 0x911AB364;

    using NotifyFn = int(__fastcall*)(uint8_t*, void*, uint32_t, int, int, int);
    NotifyFn OriginalNotify = nullptr;
    int __fastcall Notify(uint8_t* page, void* edx, uint32_t message, int a, int b, int c)
    {
        NativeScope scope;
        TouchPage(page);
        auto owner = PageOwner.Capture();
        if (message == PadLeft || message == PadRight || message == PadUp || message == PadDown || message == PadBack)
        {
            std::erase_if(Commands, [&](const Command& command)
            {
                return command.owner.screen == page && command.kind == Command::Kind::Accept;
            });
        }
        if (*reinterpret_cast<int32_t*>(page + 0x1C) <= 0 &&
            (message == PadUp || message == PadDown || message == PadLeft || message == PadRight))
            return 0;
        bool own = false;
        auto nodes = reinterpret_cast<ListLink*>(page + PageNodes);
        for (auto link = nodes->next; link && link != nodes && !own; link = link->next)
            own = FindByDalId(*reinterpret_cast<int32_t*>(reinterpret_cast<uint8_t*>(link) - 4 + NodeDalId)) != nullptr;
        if (own && *reinterpret_cast<int32_t*>(page + PageState) == 0)
        {
            auto entry = Highlighted(page);
            bool refresh = false;
            if (entry && entry->kind == Kind::Preset && (message == PadLeft || message == PadRight))
            {
                PresetRow.Cycle(message == PadLeft ? -1 : 1);
                refresh = true;
            }
            else if (message == PadLeft || message == PadRight || message == PadUp || message == PadDown || message == PadBack)
            {
                refresh |= PresetRow.Cancel();
                refresh |= RestoreRow.Cancel();
            }
            if (refresh)
                Refresh(page);
        }
        if (!PageOwner.Matches(owner))
            return 0;
        if ((message == PadLeft || message == PadRight) && *reinterpret_cast<int32_t*>(page + PageState) == 0)
        {
            auto entry = Highlighted(page);
            if (entry && (entry->kind == Kind::Setting || entry->kind == Kind::Action) && !Enabled(*entry))
                return 0;
        }
        LiteralScope literal(OwnNode(page));
        PopupScope popup(OwnNode(page));
        return OriginalNotify(page, edx, message, a, b, c);
    }

    SafetyHookInline shContextNotify;
    using ContextNotifyFn = void(__fastcall*)(uint8_t*, void*, uint32_t, int, int, int);
    ContextNotifyFn OriginalContextNotify = nullptr;

    void __fastcall ContextNotify(uint8_t* context, void* edx, uint32_t message, int a, int b, int c)
    {
        NativeScope scope;
        if ((message == PadLeft || message == PadRight) && context[0x31] && !context[0x32])
        {
            auto list = reinterpret_cast<ListLink*>(context + 0x20);
            auto page = list->next && list->next != list && list->prev ? reinterpret_cast<uint8_t*>(list->prev) - 4 : nullptr;
            if (page && Pages.count(page) && *reinterpret_cast<uintptr_t**>(page) == OptionsPageTable &&
                *reinterpret_cast<uint8_t**>(page + 0x14) == context)
            {
                auto entry = Highlighted(page);
                if (entry && entry->kind == Kind::Preset)
                {
                    // Native tree input consumes these before the option page sees them.
                    Notify(page, nullptr, message, a, b, c);
                    return;
                }
            }
        }
        OriginalContextNotify(context, edx, message, a, b, c);
    }

    SafetyHookInline shConstructPage, shClearNodes;
    uint8_t* __fastcall ConstructPage(uint8_t* page, void* edx, ListLink* nodes, void* screen)
    {
        NativeScope scope;
        auto source = ChildrenParents.find(nodes);
        auto parent = source != ChildrenParents.end() ? std::optional<PageSource>(source->second) : std::nullopt;
        auto preview = PreviewDepth != 0;
        if (!preview)
        {
            RestoreLayout();
            ++Lifetime;
        }
        Pages.insert(page);
        if (parent)
            PageParents[page] = *parent;
        else
            PageParents.erase(page);
        if (!preview)
        {
            PageOwner.Bind(page);
            LastContext = static_cast<uint8_t*>(screen);
        }
        RefreshNeeded = true;
        auto owner = PageOwner.Capture();
        auto result = shConstructPage.unsafe_fastcall<uint8_t*>(page, edx, nodes, screen);
        if (!preview && PageOwner.Matches(owner))
        {
            if (parent)
                PauseMenuActive = parent->pause;
            InitializePreset(page);
            if (NativeDefaults.parent != -1 && NativeDefaults.context == screen && parent &&
                parent->parent == NativeDefaults.parent && parent->pause == NativeDefaults.pause)
                NativeDefaults.replacement = owner;
        }
        return result;
    }

    void __fastcall ClearNodes(ListLink* list, void* edx)
    {
        NativeScope scope;
        ChildrenParents.erase(list);
        auto page = reinterpret_cast<uint8_t*>(list) - PageNodes;
        if (Pages.erase(page))
        {
            auto context = *reinterpret_cast<uint8_t**>(page + 0x14);
            if (LastContext == context && std::none_of(Pages.begin(), Pages.end(), [&](void* other)
            {
                return *reinterpret_cast<uint8_t**>(static_cast<uint8_t*>(other) + 0x14) == context;
            }))
                LastContext = nullptr;
            PageParents.erase(page);
            if (PageOwner.Get() == page)
                RestoreLayout();
            for (auto link = list->next; link && link != list; link = link->next)
                PublishedChoices.erase(reinterpret_cast<uint8_t*>(link) - 4);
            PageFormats.erase(page);
            ++Lifetime;
            PageOwner.Clear(page);
            std::erase_if(Commands, [&](const Command& command)
            {
                return command.owner.screen == page;
            });
            RefreshNeeded = true;
        }
        shClearNodes.unsafe_fastcall(list, edx);
    }

    NotifyFn OriginalBaseNotify = nullptr;
    ActivateFn OriginalBaseActivate = nullptr;
    int __fastcall BaseNotify(uint8_t* page, void* edx, uint32_t message, int a, int b, int c)
    {
        NativeScope scope;
        TouchPage(page);
        if (*reinterpret_cast<int32_t*>(page + 0x1C) <= 0 &&
            (message == PadUp || message == PadDown || message == PadLeft || message == PadRight))
            return 0;
        LiteralScope literal(OwnNode(page));
        return OriginalBaseNotify(page, edx, message, a, b, c);
    }

    int __fastcall BaseActivate(uint8_t* page, void* edx)
    {
        NativeScope scope;
        TouchPage(page);
        if (*reinterpret_cast<int32_t*>(page + 0x1C) <= 0)
            return 0;
        LiteralScope literal(OwnNode(page));
        return OriginalBaseActivate(page, edx);
    }

    bool CacheSettings()
    {
        auto pin = Plugins;
        if (!pin)
            return false;
        auto lifetime = Lifetime;
        auto owner = PageOwner.Capture();
        auto active = [&]() { return Plugins == pin && Lifetime == lifetime && PageOwner.Capture().generation == owner.generation; };
        std::unordered_map<int32_t, CachedSetting> next;
        for (auto& entry : Entries)
        {
            auto row = RowOf(entry);
            CachedSetting data;
            data.visible = data.enabled = row.plugin != nullptr;
            if (row.plugin && row.setting)
            {
                data.value = OtherPlugins::Get(*row.plugin, *row.setting);
                if (!active())
                    return false;
                data.text = WFPPlugins::ValueText(row);
                if (!active())
                    return false;
                data.values = OtherPlugins::ListedValues(*row.plugin, *row.setting);
                if (!active())
                    return false;
                data.visible = WFPPlugins::Visible(row);
                if (!active())
                    return false;
                data.enabled = WFPPlugins::Enabled(row);
                if (!active())
                    return false;
                for (auto& value : data.values)
                {
                    auto label = WFPMenu::ValueLabel(*row.setting, value);
                    data.texts.push_back(row.setting->type == asi_settings::Type::Bool ?
                        WFPText::Get(value != "0" ? "WFP_ON" : "WFP_OFF") :
                        label && !label->label.empty() ? OtherPlugins::Text(*row.plugin, label->label) : value);
                }
            }
            next.emplace(entry.entry.id, std::move(data));
        }
        if (!active())
            return false;
        std::unordered_map<HMODULE, int> presets;
        for (auto& plugin : pin->plugins)
        {
            if (OtherPlugins::HasPresets(plugin))
            {
                presets[plugin.module] = OtherPlugins::CurrentPreset(plugin);
                if (!active())
                    return false;
            }
        }
        Cached = std::move(next);
        CachedPresets = std::move(presets);
        return true;
    }

    void RetireHelp(uint8_t* page)
    {
        if (!Help.id)
            return;
        auto node = page ? OwnNode(page) : nullptr;
        if (!node || Help.generation != PageOwner.Capture().generation ||
            Help.id != *reinterpret_cast<int32_t*>(node + 0x0C) ||
            (NFSPSDialog::Pending && NFSPSDialog::Busy()) ||
            (*reinterpret_cast<uintptr_t**>(page) == OptionsPageTable && *reinterpret_cast<int32_t*>(page + PageState) != 0))
            HideHelp();
    }

    void ServiceHelp()
    {
        if (NativeDepth)
            return;
        SyncActiveContext(LastContext);
        // Native glyph matrices are valid after the front end has drawn its captions.
        UpdateHelp(static_cast<uint8_t*>(PageOwner.Get()));
    }

    void Service()
    {
        static bool running = false;
        static ULONGLONG last = 0;
        if (NativeDepth || running)
            return;
        running = true;
        struct Finish
        {
            bool& running;
            ~Finish()
            {
                running = false;
            }
        } finish{ running };
        SyncActiveContext(LastContext);
        auto previous = Plugins;
        bool found = UpdatePlugins();
        if (!Plugins)
            return;
        if (previous != Plugins)
        {
            PresetRow = {};
            RestoreRow = {};
            RefreshNeeded = true;
        }
        UpdateTexts(found);
        if (!PageOwner.Get() && previous && !found)
        {
            Commands.clear();
            HideHelp();
            return;
        }
        bool resetPreset = false;
        while (!Commands.empty())
        {
            auto command = std::move(Commands.front());
            Commands.pop_front();
            auto pin = command.snapshot;
            auto active = [&]() { return PageOwner.Matches(command.owner) && Plugins == pin; };
            auto entry = FindById(command.id);
            if (!active() || !entry)
                continue;
            if (command.kind == Command::Kind::Accept && Highlighted(static_cast<uint8_t*>(command.owner.screen)) != entry)
                continue;
            auto row = RowOf(*entry);
            if (!row.plugin)
                continue;
            if (command.kind == Command::Kind::RestorePage)
            {
                WFPPlugins::RestoreDefaults(*row.plugin, PageExclusions(*entry), active);
                resetPreset = true;
            }
            else if (command.kind == Command::Kind::Set)
            {
                if (!row.setting || !WFPPlugins::Visible(row) || !active() || !WFPPlugins::Enabled(row) || !active())
                    continue;
                WFPPlugins::Set(*row.plugin, *row.setting, command.value, active);
            }
            else if (entry->kind == Kind::Preset)
            {
                PresetRow.plugin = row.plugin;
                PresetRow.shown = command.preset;
                PresetRow.Accept(active);
            }
            else if (entry->kind == Kind::RestoreDefaults)
                RestoreRow.Accept(*row.plugin, PageExclusions(*entry), active);
            else if (entry->kind == Kind::Action && row.setting && WFPPlugins::Visible(row) && active() && WFPPlugins::Enabled(row) && active())
                OtherPlugins::Invoke(*row.plugin, *row.setting);
            if (!active())
                return;
            RestartChange |= WFPPlugins::RestartNeeded();
            RefreshNeeded = true;
        }
        auto now = GetTickCount64();
        RefreshNeeded |= WFPPlugins::Changed().exchange(false);
        if (!RefreshNeeded && now - last < 250)
        {
            auto page = static_cast<uint8_t*>(PageOwner.Get());
            RetireHelp(page);
            RestartNotice(page);
            return;
        }
        if (!CacheSettings())
            return;
        if (!ReconcileRows(static_cast<uint8_t*>(PageOwner.Get())))
            return;
        auto owner = PageOwner.Capture();
        auto page = static_cast<uint8_t*>(owner.screen);
        if (page && (previous != Plugins || resetPreset))
            InitializePreset(page);
        RetireHelp(page);
        if (page && *reinterpret_cast<uintptr_t**>(page) == OptionsPageTable &&
            *reinterpret_cast<int32_t*>(page + PageState) == 1)
        {
            auto node = OwnNode(page);
            auto entry = node ? FindByDalId(*reinterpret_cast<int32_t*>(node + NodeDalId)) : nullptr;
            if (entry && IsSlider(*entry))
                FitRow(page, *reinterpret_cast<int32_t*>(page + 0x20) + 1, "OPTION_HEADER_%d", node, true, owner);
        }
        if (!PageOwner.Matches(owner))
            return;
        if (page && !(NFSPSDialog::Pending && NFSPSDialog::Busy()) && ((*reinterpret_cast<uintptr_t**>(page) == OptionsPageTable &&
            *reinterpret_cast<int32_t*>(page + PageState) == 0) || *reinterpret_cast<uintptr_t**>(page) == BasePageTable))
            Refresh(page);
        if (PageOwner.Matches(owner))
            RestartNotice(page);
        last = now;
        RefreshNeeded = false;
    }

}

class Options
{
public:
    Options()
    {
        WFP::onInitEvent() += []()
        {
            auto children = hook::pattern("53 8B 5C 24 08 56 57 8B 7C 24 14 BE ? ? ? ? 39 5E 04 75");
            auto dataNode = hook::pattern("8B 54 24 04 33 C9 E9 ? ? ? ? EB 03 8D 49 00 39 10 74 10");
            auto parentId = hook::pattern("8B 54 24 04 33 C9 B8 ? ? ? ? EB 03 8D 49 00 39 10 74 11 83 C0 14 83 C1 01 3D ? ? ? ? 7C EF 83 C8 FF C3");
            auto dal = hook::pattern("C7 40 04 05 00 00 00 C7 00 ? ? ? ? A3");
            auto listCommand = hook::pattern("C7 44 24 18 ? ? ? ? 89 44 24 28 6A FF");
            auto rangeCommand = hook::pattern("8B 47 10 33 DB 89 5C 24 2C C7 44 24 28 ? ? ? ? 89 44 24");
            auto item = hook::pattern("F3 0F 10 44 24 08 53 8B 5C 24 08 85 DB 57 8B F9 C7 07");
            auto fastMem = hook::pattern("68 04 26 7B 41 E8 ? ? ? ? 68 C2 E5 DF 70 8B F0 E8 ? ? ? ? 83 C4 08 6A 00 6A 14 B9 ? ? ? ? 8B D8 E8");
            auto search = hook::pattern("8B 44 24 08 8B 15 ? ? ? ? 53 8D 0C C5 00 00 00 00 2B C8 55 56 8D 34 8A");
            auto language = hook::pattern("8B 15 ? ? ? ? 3B DA 0F 84 ? ? ? ? 33 FF 83 FA FF 0F 84 ? ? ? ? 33 C9 B8");
            auto languageEnd = hook::pattern("8B 54 24 04 33 C9 B8 ? ? ? ? EB 03 8D 49 00 39 10 74 10 83 C0 24 83 C1 01 3D ? ? ? ? 7C EF 33 C0 C3");
            auto page = hook::pattern("68 B4 00 00 00 C7 44 24 1C 00 00 00 00 C7 06 ? ? ? ? C7 46 2C 00 00 00 00");
            auto fontLookup = hook::pattern("8B 54 24 04 56 FF 25 ? ? ? ? EB 03 8D 49 00 8B C6 3D ? ? ? ? 74 15 8D A4 24 00 00 00 00");
            auto glyphAdvance = hook::pattern("66 3D 99 00 75 07 B8 22 21 00 00 EB 18 66 3D 9C 00 75 07 B8 53 01 00 00 EB 0B 66 3D A0 00 75 05");
            auto localizedWide = hook::pattern("8B 0D ? ? ? ? 85 C9 56 57 8B 7C 24 14 8B C7 74 ? 57 FF D1 83 C4 04 50 E8 ? ? ? ? 8B F0 83 C4 04");
            if (fontLookup.size() != 1 || glyphAdvance.size() != 1 || localizedWide.size() != 1)
                return;
            auto packageLookup = hook::pattern("8B 0D ? ? ? ? 57 56 E8 ? ? ? ? 85 C0 74 29 8B 78 20 85 FF 75 1A");
            auto objectLookup = hook::pattern("8B 44 24 04 85 C0 74 26 8B 0D ? ? ? ? 50 E8 ? ? ? ? 85 C0 74 16 8B 4C 24 08 51 8B C8 E8 ? ? ? ? 85 C0 74 06 83 78 18 02");
            auto setHash = hook::pattern("56 8B 74 24 08 85 F6 74 32 8B 46 60 8B 4C 24 0C 3B C1 BA 00 00 40 00 74 0A 09 56 1C");
            auto renderString = hook::pattern("55 8B EC 83 E4 F0 81 EC F4 08 00 00 A1 ? ? ? ? 33 C4 89 84 24 F0 08 00 00 8B 45 0C 53 56 8B 75 08 8B D9");
            auto stringConstructor = hook::pattern("56 8B F1 E8 ? ? ? ? 33 C0 89 46 5C C7 06 ? ? ? ? C7 46 60 FF FF FF FF 89 46 64 89 46 68 89 46 6C 89 46 70 89 46 74 C7 46 18 02 00 00 00");
            auto stateAllocation = hook::pattern("55 B9 ? ? ? ? 89 7E 2C E8 ? ? ? ? 89 46 2C 89 6E 30 8B 4B 30 8B 53 2C");
            if (renderString.size() != 1 || stringConstructor.size() != 1 || stateAllocation.size() != 1 || *renderString.get_first<uint8_t>(0xB4) != 0xE8)
                return;
            auto popupItem = hook::pattern("8B 4C 24 20 33 C0 89 46 0C 89 4E 10 C6 46 14 01 88 46 15 88 46 16 88 46 17");
            auto popupDraw = hook::pattern("8B 44 24 0C 56 8B 74 24 08 57 8B F9 8B 4C 24 10 50 51 56 8B CF E8 ? ? ? ? 85 F6 74 33 8B 46 0C");
            if (popupItem.size() != 1 || popupDraw.size() != 1)
                return;
            auto dialogConstructor = hook::pattern("53 56 57 6A 28 E8 ? ? ? ? 33 FF 83 C4 04 3B C7 74 26 C7 00 ? ? ? ? C6 40 04 00 C6 40 05 01 89 78 14 89 78 18 89 78 1C 89 78 24 C7 40 20 80 00 00 00 89 78 08 EB 02 33 C0 8B 74 24 10 56 A3");
            auto dialogOpen = hook::pattern("8B 0D ? ? ? ? 85 C9 74 05 E9 ? ? ? ? 8B 0D ? ? ? ? 56 68 ? ? ? ? 8B F1 E8");
            auto dialogScreen = hook::pattern("6A 15 6A 01 C7 44 24 20 00 00 00 00 C7 06 ? ? ? ? C7 46 20 00 00 00 00 E8 ? ? ? ? A1 ? ? ? ? 83 40 28 01 8B 50 28 8B C8 83 C0 28");
            auto dollarMode = hook::pattern("80 3D ? ? ? ? 00 75 3E 80 3D ? ? ? ? 00 75 35 66 83 F9 24");
            if (packageLookup.size() != 1 || objectLookup.size() != 1 || setHash.size() != 1 || renderString.size() != 1 || dollarMode.size() != 1)
                return;
            if (*localizedWide.get_first<uint8_t>(0x9A) != 0xE8)
                return;
            auto converter = injector::GetBranchDestination(localizedWide.get_first(0x9A)).as_int();
            if (*reinterpret_cast<uint16_t*>(converter + 0x32) != 0x3D83)
                return;
            auto formatter = hook::pattern("83 EC 10 8B 44 24 18 53 55 33 D2 56 32 DB 38 10 57 89 54 24 14 C7 44 24 10 FF FF FF FF 0F 84");
            if (formatter.size() != 1)
                return;
            auto basePage = hook::pattern("8D 7E 0C C7 06 ? ? ? ? 89 3F 89 7F 04 8B 4C 24 24 8B 44 24 28 33 DB 57 51 8B CE");
            auto repaintRows = hook::pattern("56 8B F1 8B 06 8B 50 08 6A 00 FF D2 8B 46 20 6A 01 50 8B CE E8 ? ? ? ? 8B 4E 14 C6 41 32 00 5E C3");
            if (repaintRows.size() != 1)
                return;
            RepaintRows = reinterpret_cast<decltype(RepaintRows)>(repaintRows.get_first());
            auto createPreview = hook::pattern("8B F9 8B 74 24 1C 85 F6 74 6C 39 36 74 68 83 7F 34 00 75 62 80 7F 30 00 74 5C 56 E8 ? ? ? ? 83 C4 04 84 C0");
            if (createPreview.size() != 1)
                return;
            auto clearNodes = hook::pattern("56 8B F1 8B 06 3B C6 74 26 8D A4 24 00 00 00 00 8B 08 8B 50 04 89 0A 89 51 04 8D 48 FC");
            if (basePage.size() != 1 || clearNodes.size() != 1)
                return;
            auto contextNotify = hook::pattern("53 55 56 57 8B 7C 24 14 81 FF 4B 0A 1C 91 8B F1 0F 87 ? ? ? ? 0F 84 ? ? ? ? 81 FF 10 9F 4D 5E");
            auto sliderDraw = hook::pattern("83 EC 18 56 57 8B F1 8B 56 20 8D 44 24 08 50 8D 4C 24 10 51 52 E8 ? ? ? ? D9 46 04");
            if (contextNotify.size() != 1 || sliderDraw.size() != 1)
                return;
            if (children.size() != 1 || dataNode.size() != 1 || parentId.size() != 1 || dal.size() != 1 || listCommand.size() != 1 || rangeCommand.size() != 1 ||
                item.size() != 1 || fastMem.size() != 1 || search.size() != 1 || language.size() != 1 || languageEnd.size() != 1 || page.size() != 1)
                return;

            // The credits entry of the options menu, in the table the parent ID lookup goes through
            Table = *parentId.get_first<TreeEntry*>(7);
            TableEnd = *parentId.get_first<TreeEntry*>(0x1B);
            if (std::none_of(Table, TableEnd, [](const TreeEntry& entry)
            {
                return entry.id == CreditsEntry && entry.parent == OptionsMenu && entry.label == CreditsTitle && entry.type == TypeMenu;
            }))
                return;
            bPauseOptions = std::any_of(Table, TableEnd, [](const TreeEntry& entry)
            {
                return entry.parent == PauseOptionsMenu && entry.label == GraphicsTitle;
            });

            auto getChildren = reinterpret_cast<uintptr_t>(children.get_first());
            ListNode = reinterpret_cast<decltype(ListNode)>(injector::GetBranchDestination(getChildren + 0x28).as_int());
            SliderNode = reinterpret_cast<decltype(SliderNode)>(injector::GetBranchDestination(getChildren + 0x30).as_int());
            StandardNode = reinterpret_cast<decltype(StandardNode)>(injector::GetBranchDestination(getChildren + 0x38).as_int());
            FindFont = reinterpret_cast<decltype(FindFont)>(fontLookup.get_first());
            GlyphAdvance = reinterpret_cast<decltype(GlyphAdvance)>(glyphAdvance.get_first(-0x3C));
            LocalizedWide = reinterpret_cast<decltype(LocalizedWide)>(localizedWide.get_first());
            ActiveCharset = *localizedWide.get_first<uint8_t**>(0x58);
            RawCharset = *reinterpret_cast<uint32_t**>(converter + 0x34);
            PackageManager = *packageLookup.get_first<void**>(2);
            FindPackage = reinterpret_cast<decltype(FindPackage)>(injector::GetBranchDestination(packageLookup.get_first(8)).as_int());
            FindObject = reinterpret_cast<decltype(FindObject)>(injector::GetBranchDestination(objectLookup.get_first(0x1F)).as_int());
            SetLanguageHash = reinterpret_cast<decltype(SetLanguageHash)>(setHash.get_first());
            if (*renderString.get_first<uint8_t>(0x56) != 0xE8)
                return;
            CreateFontCache = reinterpret_cast<decltype(CreateFontCache)>(injector::GetBranchDestination(renderString.get_first(0x56)).as_int());
            NFSPSStaticText::Construct = reinterpret_cast<decltype(NFSPSStaticText::Construct)>(stringConstructor.get_first());
            NFSPSStaticText::StatePool = *stateAllocation.get_first<void*>(2);
            NFSPSStaticText::AllocateState = reinterpret_cast<decltype(NFSPSStaticText::AllocateState)>(injector::GetBranchDestination(stateAllocation.get_first(9)).as_int());
            NFSPSHelp::ReadWide = reinterpret_cast<decltype(NFSPSHelp::ReadWide)>(injector::GetBranchDestination(renderString.get_first(0xB4)).as_int());
            NFSPSHelp::Advance = reinterpret_cast<decltype(NFSPSHelp::Advance)>(&StockGlyphAdvance);
            NoDollar.SetAddress(*dollarMode.get_first<uint8_t*>(0x0B));
            LiteralFontControls.SetAddress(*dollarMode.get_first<uint8_t*>(2));
            FontModesReady = true;
            ListItem = reinterpret_cast<decltype(ListItem)>(item.get_first());
            FastMem = *fastMem.get_first<void*>(0x1E);
            FastMemAlloc = reinterpret_cast<decltype(FastMemAlloc)>(injector::GetBranchDestination(fastMem.get_first(0x24)).as_int());
            CurrentLanguage = *language.get_first<int32_t*>(2);
            Languages = *languageEnd.get_first<LanguageInfo*>(7);
            LanguagesEnd = *languageEnd.get_first<LanguageInfo*>(0x1B);

            // Options > Plugins (listed in the place of the credits, and last in the race's pause menu); the plugins'
            // pages are added when a menu is first shown
            Entries.push_back({ { PluginsMenu, OptionsMenu, TextHash("WFP_PLUGINS"), TypeMenu, 0 }, Kind::Menu });
            Entries.back().root = PluginsMenu;
            if (bPauseOptions)
            {
                Entries.push_back({ { PausePluginsMenu, PauseOptionsMenu, TextHash("WFP_PLUGINS"), TypeMenu, 0 }, Kind::Menu });
                Entries.back().root = PausePluginsMenu;
            }

            shCreatePreview = safetyhook::create_inline(createPreview.get_first(-0x22), CreatePreview);
            shConstructPage = safetyhook::create_inline(basePage.get_first(-0x2A), ConstructPage);
            shClearNodes = safetyhook::create_inline(clearNodes.get_first(), ClearNodes);
            shPopupItem = safetyhook::create_inline(popupItem.get_first(-0x28), ConstructPopupItem);
            shPopupDraw = safetyhook::create_inline(popupDraw.get_first(), DrawPopupItem);
            shRenderString = safetyhook::create_inline(renderString.get_first(), RenderString);
            shFormat = safetyhook::create_inline(formatter.get_first(), Format);
            shSearchString = safetyhook::create_inline(search.get_first(), SearchString);
            shGetChildren = safetyhook::create_inline(getChildren, GetChildren);
            shGetDataNode = safetyhook::create_inline(dataNode.get_first(), GetDataNode);
            shGetParentID = safetyhook::create_inline(parentId.get_first(), GetParentID);
            shContextNotify = safetyhook::create_inline(contextNotify.get_first(), &ContextNotify);
            shDrawSlider = safetyhook::create_inline(sliderDraw.get_first(), &DrawSlider);
            if (!shCreatePreview || !shConstructPage || !shClearNodes || !shSearchString || !shGetChildren || !shGetDataNode || !shGetParentID || !shFormat || !shRenderString ||
                !shPopupItem || !shPopupDraw || !shContextNotify || !shDrawSlider)
            {
                shDrawSlider.reset();
                shContextNotify.reset();
                shGetParentID.reset();
                shGetDataNode.reset();
                shGetChildren.reset();
                shSearchString.reset();
                shFormat.reset();
                shPopupDraw.reset();
                shPopupItem.reset();
                shRenderString.reset();
                shClearNodes.reset();
                shConstructPage.reset();
                shCreatePreview.reset();
                return;
            }
            OriginalContextNotify = shContextNotify.original<ContextNotifyFn>();
            OriginalDrawSlider = shDrawSlider.original<DrawSliderFn>();
            auto baseTable = *basePage.get_first<uintptr_t*>(5);
            BasePageTable = baseTable;
            HookSlot(baseTable, 1, &BaseNotify, OriginalBaseNotify);
            HookSlot(baseTable, 5, &BaseActivate, OriginalBaseActivate);
            HookSlot(baseTable, 11, &BaseShow, OriginalBaseShow);

            auto dialogClose = hook::pattern("8B 15 ? ? ? ? 8B 02 05 D8 00 00 00 56 74 14 8B 70 0C 85 F6 74 0D 83 7E 20 00 75 13");
            if (dialogConstructor.size() == 1 && dialogOpen.size() == 1 && dialogScreen.size() == 1 && dialogClose.size() == 1)
            {
                auto table = *dialogScreen.get_first<uintptr_t*>(0x0E);
                shConstructDialog = safetyhook::create_inline(dialogScreen.get_first(-0x37), ConstructDialog);
                shDialogNotify = safetyhook::create_inline(reinterpret_cast<void*>(table[1]), DialogNotify);
                if (shConstructDialog && shDialogNotify)
                {
                    CloseDialog = reinterpret_cast<decltype(CloseDialog)>(dialogClose.get_first());
                    NFSPSDialog::Table = *dialogConstructor.get_first<void*>(0x15);
                    NFSPSDialog::Pending = *dialogConstructor.get_first<NFSPSDialog::Configuration**>(0x41);
                    NFSPSDialog::Instance = *dialogOpen.get_first<void**>(2);
                    NFSPSDialog::Open = reinterpret_cast<decltype(NFSPSDialog::Open)>(dialogOpen.get_first());
                }
                else
                {
                    shConstructDialog.reset();
                    shDialogNotify.reset();
                }
            }

            // The game's CRT, which allocates the texts of the menu's nodes
            if (auto crt = GetModuleHandleW(L"msvcr80.dll"))
            {
                CrtMalloc = reinterpret_cast<decltype(CrtMalloc)>(GetProcAddress(crt, "malloc"));
                CrtFree = reinterpret_cast<decltype(CrtFree)>(GetProcAddress(crt, "free"));
                NFSPSStaticText::Allocate = CrtMalloc;
                NFSPSStaticText::Free = CrtFree;
                NFSPSDialog::Allocate = CrtMalloc;
                NFSPSDialog::Free = CrtFree;
            }

            auto dalVTable = *dal.get_first<uintptr_t*>(9);
            HookSlot(dalVTable, 2, &GetFloat, OriginalGetFloat);
            HookSlot(dalVTable, 3, &SetFloat, OriginalSetFloat);
            HookSlot(dalVTable, 6, &GetInt, OriginalGetInt);
            HookSlot(dalVTable, 7, &SetInt, OriginalSetInt);
            HookSlot(dalVTable, 10, &GetString, OriginalGetString);
            HookSlot(dalVTable, 12, &GetDefaultInt, OriginalGetDefaultInt);
            HookSlot(dalVTable, 13, &GetDefaultFloat, OriginalGetDefaultFloat);
            HookSlot(*listCommand.get_first<uintptr_t*>(4), 2, reinterpret_cast<void*>(&ListExecute), OriginalListExecute);
            HookSlot(*rangeCommand.get_first<uintptr_t*>(0xD), 2, reinterpret_cast<void*>(&RangeExecute), OriginalRangeExecute);
            auto gameDefaults = hook::pattern("6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 83 EC 08 56 A1 ? ? ? ? 33 C4 50 8D 44 24 10 64 A3 00 00 00 00 8B F1 8D 44 24 08 8B C8 89 44 24 08 89 4C 24 0C 83 7E 08 02");
            if (gameDefaults.size() == 1)
            {
                DefaultsPauseContext = *gameDefaults.get_first<uint8_t**>(0x43);
                DefaultsMainContext = *gameDefaults.get_first<uint8_t**>(0x78);
                shGameDefaults = safetyhook::create_inline(gameDefaults.get_first(), &GameDefaults);
                OriginalGameDefaults = shGameDefaults.original<GameDefaultsFn>();
            }
            auto pageVTable = *page.get_first<uintptr_t*>(0x0F);
            OptionsPageTable = pageVTable;
            HookSlot(pageVTable, 1, &Notify, OriginalNotify);
            HookSlot(pageVTable, 5, &Activate, OriginalActivate);
            HookSlot(pageVTable, 11, &Show, OriginalShow);
            WFP::onGameProcessEvent() += []()
            {
                Service();
            };
            WFP::onEndScene() += []()
            {
                ServiceHelp();
            };
        };
    }
} Options;
