module;

#include <stdafx.h>
#include <deque>
#include <map>
#include <unordered_set>
#include "FixSettings.hpp"
#include "Help.hpp"
#include "RowLayout.hpp"
#include "../Shared/NFS/PluginsMenu.hpp"
#include "../Shared/NFS/MenuOwner.hpp"
#include "../Shared/NFS/MenuLayout.hpp"
#include "../Shared/NFS/LegacyText.hpp"
#include "../Shared/NFS/Text.hpp"
#include "../Shared/NFS/TextLanguage.hpp"

export module Options;

import ComVars;
import Input;

// The settings in the game's options menu. Settings that belong to an option of the game are rows of that option's
// screen, made with the game's widgets: the display mode under the resolution, the picture settings under the
// brightness, the field of view under the car camera and the HUD aspect ratio on the interface screen. Everything
// else is under one category of its own, "Plugins": the fix first, then every other loaded plugin with settings.
//
// The rows' values go through the game's options data layer (DALOptions) under ids of their own, and the plugins'
// runtime tables (WFPPlugins) behind it. The native package has fourteen option slots. The host-first plugin list
// and each plugin's sections page within those slots; Back from a plugin restores its position in the list.
//
// The Plugins category reuses the options state 1: the console versions' video screen, which the PC version builds as
// an empty screen and never lists. Selecting it opens FeOptionsScreen in that state and Back returns to the options
// menu, like the game's other option screens.
//
// The game's widgets only react to left and right; the Preset, Restore Defaults and action rows are choice widgets with
// a copy of the widget's vtable whose message handler also takes accept (the scroller passes every message to the
// highlighted widget).

namespace
{
    constexpr int CategoryState = 1;
    constexpr int MaxRows = 14;
    constexpr int PluginRowId = 5900;       // the Plugins screen's rows
    constexpr int PageRowId = 5901;
    constexpr int FirstPluginScreenId = 5902;
    constexpr int FirstGameScreenId = 5950; // the rows on the game's screens

    // The fix's settings shown on the game's screens, after the game's option of that id
    struct Placement
    {
        int after;
        const char* setting;
    };
    constexpr Placement Placements[] =
    {
        { 5068, "MAIN.WindowMode" },              // Resolution (basic and advanced video)
        { 5083, "GRAPHICS.ConsoleGamma" },          // Brightness (basic video)
        { 5083, "GRAPHICS.AntiAliasing" },
        { 5083, "GRAPHICS.Sharpness" },
        { 5083, "MAIN.GraphicsAPI" },
        { 5083, "MAIN.FMVWidescreenMode" },
        { 5083, "MAIN.CutsceneBorders" },
        { 5034, "MAIN.FOVFactor" },                 // Car Camera (gameplay)
        { 5034, "GRAPHICS.Scaling" },
        { 5040, "MAIN.HudAspectRatioConstraint" },  // Picture in Picture (interface)
    };

    std::vector<std::string> PlacedSettings();

    // Rows and queued operations keep their originating plugin description alive until their view retires.
    std::shared_ptr<WFPPlugins::Snapshot> Plugins;
    WFPMenu::Owner ScreenOwner;
    WFPMenu::Owner RowsOwner;
    using ScreenToken = WFPMenu::Owner::Token;
    uintptr_t* pOptionsScreen = nullptr;    // the FeOptionsScreen that is open, or null
    uintptr_t PluginsScreen = 0;            // the screen showing the Plugins rows
    uintptr_t PendingRebuild = 0;
    bool KeepPresetChoice{};
    unsigned NativeDepth{};
    unsigned DefaultsDepth{};
    bool Servicing{};
    bool RefreshNeeded{};
    bool SnapshotLayoutChanged{};
    bool DiscoverNeeded{};
    bool StockRebuildNeeded{};
    void* SetupManager{};
    int BuildingVideoPage = -1;
    std::array<std::array<std::vector<std::string>, 4>, 2> JoinedSettings;

    int PlacementPage()
    {
        if (!SetupManager)
            return -1;
        auto manager = static_cast<uint8_t*>(SetupManager);
        auto state = *reinterpret_cast<int*>(manager + 4);
        return state == 2 ? (BuildingVideoPage >= 0 ? BuildingVideoPage : manager[0xE0] ? 0 : 1) :
            state == 3 ? 2 : state == 4 ? 3 : -1;
    }

    size_t MenuMode()
    {
        return SetupManager && *reinterpret_cast<int*>(static_cast<uint8_t*>(SetupManager) + 8) == 1 ? 1 : 0;
    }

    std::vector<std::string> PlacedSettings()
    {
        std::vector<std::string> ids;
        for (const auto& page : JoinedSettings[MenuMode()])
        {
            for (const auto& id : page)
            {
                if (std::find(ids.begin(), ids.end(), id) == ids.end())
                    ids.push_back(id);
            }
        }
        return ids;
    }

    void ServiceScreen();

    bool Current(ScreenToken token)
    {
        return RowsOwner.Matches(token) && ScreenOwner.Get() == token.screen;
    }

    struct SettingState
    {
        std::string value;
        std::string display;
        std::vector<std::string> choices;
        bool visible{};
        bool enabled{};
    };
    std::unordered_map<const asi_settings::Setting*, SettingState> CachedSettings;

    std::string CachedValue(const WFPPlugins::Row& row)
    {
        auto found = CachedSettings.find(row.setting);
        return found != CachedSettings.end() ? found->second.value :
            row.setting ? asi_settings::detail::JsonToText(row.setting->defaultValue) : std::string();
    }

    bool CachedVisible(const WFPPlugins::Row& row)
    {
        if (row.kind != WFPPlugins::Row::Kind::Setting && row.kind != WFPPlugins::Row::Kind::Action)
            return true;
        auto found = CachedSettings.find(row.setting);
        return found != CachedSettings.end() && found->second.visible;
    }

    bool CacheSnapshot(ScreenToken lifetime)
    {
        auto pin = Plugins;
        auto rowsGeneration = RowsOwner.Capture().generation;
        std::unordered_map<const asi_settings::Setting*, SettingState> next;
        bool layoutChanged = false;
        if (!pin || !ScreenOwner.Matches(lifetime))
            return false;
        for (const auto& plugin : pin->plugins)
        {
            for (const auto& setting : plugin.file.description.settings)
            {
                if (!WFPPlugins::Shown(setting))
                    continue;
                SettingState state;
                state.value = OtherPlugins::Get(plugin, setting);
                if (!ScreenOwner.Matches(lifetime) || RowsOwner.Capture().generation != rowsGeneration || Plugins != pin)
                    return false;
                state.display = OtherPlugins::ValueText(plugin, setting, WFPText::Get("WFP_ON"), WFPText::Get("WFP_OFF"));
                if (!ScreenOwner.Matches(lifetime) || RowsOwner.Capture().generation != rowsGeneration || Plugins != pin)
                    return false;
                state.choices = OtherPlugins::ListedValues(plugin, setting);
                if (!ScreenOwner.Matches(lifetime) || RowsOwner.Capture().generation != rowsGeneration || Plugins != pin)
                    return false;
                state.visible = OtherPlugins::Visible(plugin, setting);
                if (!ScreenOwner.Matches(lifetime) || RowsOwner.Capture().generation != rowsGeneration || Plugins != pin)
                    return false;
                state.enabled = OtherPlugins::Enabled(plugin, setting, true);
                if (!ScreenOwner.Matches(lifetime) || RowsOwner.Capture().generation != rowsGeneration || Plugins != pin)
                    return false;
                auto old = CachedSettings.find(&setting);
                layoutChanged |= old != CachedSettings.end() &&
                    (old->second.visible != state.visible || old->second.choices != state.choices);
                next.emplace(&setting, std::move(state));
            }
        }
        CachedSettings = std::move(next);
        SnapshotLayoutChanged |= layoutChanged;
        return true;
    }
    HMODULE Self = nullptr;

    // The game's language: its id and the table of languages
    int32_t* CurrentLanguage = nullptr;
    struct LanguageInfo
    {
        int32_t id;
        const char* name;
        const char* files[3];
        uint32_t unknown[2];
    };
    LanguageInfo* Languages = nullptr;
    LanguageInfo* LanguagesEnd = nullptr;

    std::string LanguageName(int32_t id)
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

    // The language of the rows' texts: the game's, or the one chosen for the plugins' texts. The game's fonts have no
    // Cyrillic letters, so Russian shows in English.
    std::string TextLanguageName()
    {
        auto name = WFPMenu::TextLanguage(FixSettings(), LanguageName(CurrentLanguage ? *CurrentLanguage : 0));
        return name == "russian" ? "english" : name;
    }

    void FindPlugins()
    {
        auto lifetime = ScreenOwner.Capture();
        auto rowsGeneration = RowsOwner.Capture().generation;
        auto tag = WFPText::Detail::Tag(TextLanguageName());
        auto next = WFPPlugins::Find(Self, tag ? asi_settings::text::Narrow(tag) : "en");
        if (!ScreenOwner.Matches(lifetime) || RowsOwner.Capture().generation != rowsGeneration)
            return;
        Plugins = std::move(next);
        WFPPlugins::Changed() = false;
    }

    const asi_settings::LoadedPlugin* Fix()
    {
        if (!Plugins || Plugins->plugins.empty() || Plugins->plugins.front().module != Self)
            return nullptr;
        return &Plugins->plugins.front();
    }

    // Text: hash -> text in the font's encoding. The game may hold on to a text, so an entry is changed in place and
    // never removed.
    std::unordered_map<uint32_t, std::string> Texts;

    uint32_t TextHash(const std::string& key)
    {
        return bStringHash(("WFP_TEXT_" + key).c_str());
    }

    void SetText(uint32_t hash, const std::string& utf8)
    {
        auto text = WFPText::ToEAFont(utf8, WFPText::Language());
        // Native rendered-string buffers hold 1024 wide characters; complete originals stay in the row/help cache.
        text.resize(std::min<size_t>(text.size(), 1023));
        Texts[hash] = std::move(text);
    }

    float RenderScale(uint32_t font)
    {
        auto language = CurrentLanguage ? *CurrentLanguage : 0;
        return NFSCStaticText::RenderScale(font, language);
    }

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
            if (!flags && (c == L'\n' || c == L'^'))
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

    uint32_t SetKeyText(const std::string& key)
    {
        auto hash = TextHash(key);
        SetText(hash, WFPText::Get(key));
        return hash;
    }

    // The rows
    enum class Shape { Toggle, Choice, ValueText, Slider, Button };
    struct MenuRow
    {
        WFPPlugins::Row item;
        Shape shape = Shape::Button;
        std::vector<std::string> values;   // a choice's values, in the order of its labels
        void* widget = nullptr;
        void* option = nullptr;
        bool enabled = true;
        int rootPlugin = -1;
        bool pageSelector{};
    };
    MenuRow Pager{};
    std::map<int, MenuRow> Rows;
    NFSCRowLayout::Layout ValueLayout;
    struct ValuePage
    {
        std::string raw;
        std::vector<std::string> lines;
        std::string language;
        void* fontObject{};
        void* charset{};
        uint32_t rawCharset{};
        uint32_t font{};
        int gameLanguage{};
        float scale{};
        float width{};
    };
    std::unordered_map<void*, ValuePage> ValuePages;
    std::unordered_set<void*> ContinuedValues;
    std::unordered_set<void*> PendingFonts;
    uint64_t ValueTick{};
    uint64_t ValueFramePage{};
    uint64_t FontRetryAt{};
    void LayoutRowValue(const MenuRow& row, ScreenToken token);
    std::unordered_map<void*, std::pair<uint32_t, std::string>> PublishedTexts;
    std::unordered_map<uint8_t*, uint32_t> TextMaximums;
    std::unordered_map<uint8_t*, uint32_t> TextFormatFlags;

    enum class CommandKind { Value, Step, Accept, CyclePreset, Defaults };
    struct Command
    {
        CommandKind kind;
        ScreenToken token;
        ScreenToken lifetime;
        std::shared_ptr<WFPPlugins::Snapshot> snapshot;
        int id{};
        int direction{};
        std::string value;
        std::vector<WFPPlugins::Row> defaults;
    };
    std::deque<Command> Commands;

    void Queue(CommandKind kind, int id, std::string value = {}, int direction = 0)
    {
        auto token = RowsOwner.Capture();
        if (!Current(token) || !Rows.count(id) || DefaultsDepth)
            return;
        Commands.push_back({ kind, token, ScreenOwner.Capture(), Plugins, id, direction, std::move(value) });
    }

    void ScreensRetired();
    void RetireHelp(void* screen);

    void RetireRows(void* screen)
    {
        if (!RowsOwner.Clear(screen))
            return;
        ValueLayout.RestoreAll();
        ValuePages.clear();
        ContinuedValues.clear();
        PendingFonts.clear();
        for (const auto& [object, maximum] : TextMaximums)
        {
            *reinterpret_cast<uint32_t*>(object + 0x74) = maximum;
            auto& flags = *reinterpret_cast<uint32_t*>(object + 0x1C);
            flags = (flags & ~0x800u) | (TextFormatFlags.count(object) ? TextFormatFlags.at(object) : 0) | 0x2000000;

        }
        TextMaximums.clear();
        TextFormatFlags.clear();
        RetireHelp(screen);
        Rows.clear();
        Pager = {};
        PublishedTexts.clear();
        std::erase_if(Commands, [&](const Command& command)
        {
            return command.kind != CommandKind::Defaults && command.token.screen == screen;
        });
        if (PendingRebuild == reinterpret_cast<uintptr_t>(screen))
            PendingRebuild = 0;
        ScreensRetired();
    }

    uint32_t LabelHash(int id) { return TextHash("ROW_LABEL_" + std::to_string(id)); }
    uint32_t ChoiceHash(int id, size_t i) { return TextHash("ROW_CHOICE_" + std::to_string(id) + "_" + std::to_string(i)); }
    uint32_t ValueHash(int id) { return TextHash("ROW_VALUE_" + std::to_string(id)); }

    // A choice widget has five labels at most; a choice with more values shows its value under a label of its own whose
    // text follows the value, and the widget always shows that one
    constexpr size_t ChoiceLabels = 5;

    bool SameValue(const asi_settings::Setting& setting, const std::string& a, const std::string& b)
    {
        if (setting.StringValued())
            return asi_settings::text::IEquals(a, b);
        return std::atof(a.c_str()) == std::atof(b.c_str());
    }

    int ChoiceIndex(const MenuRow& row)
    {
        auto value = CachedValue(row.item);
        for (size_t i = 0; i < row.values.size(); ++i)
        {
            if (SameValue(*row.item.setting, row.values[i], value))
                return int(i);
        }
        return 0;
    }

    // The texts of a choice's value labels
    std::string ChoiceText(const MenuRow& row, const std::string& value)
    {
        auto& setting = *row.item.setting;
        if (setting.type == asi_settings::Type::Bool)
        {
            int b = 0;
            return WFPText::Get(asi_settings::detail::ParseBool(value, b) && b ? "WFP_ON" : "WFP_OFF");
        }
        bool numeric = !setting.StringValued();
        for (auto list : { &setting.choices, &setting.special, &setting.suggestions })
        {
            for (auto& entry : *list)
            {
                if (!entry.label.empty() && asi_settings::detail::JsonEquals(entry.value, value, numeric))
                    return OtherPlugins::Text(*row.item.plugin, entry.label);
            }
        }
        return value;
    }

    // The Preset, Restore Defaults and action rows
    WFPPlugins::PresetRow Preset;
    WFPPlugins::RestoreRow Restore;
    constexpr const char* PresetLabels[] = { "WFP_PRESET_VANILLA", "WFP_PRESET_RECOMMENDED", "WFP_PRESET_ENHANCED", "WFP_PRESET_CUSTOM", "WFP_PRESET_CONFIRM" };

    int PresetIndex()
    {
        if (Preset.confirm)
            return 4;
        return Preset.shown < 0 ? 3 : Preset.shown;
    }

    // The plugin list and the selected plugin's section
    int CurrentPlugin = -1;
    size_t CurrentScreen = 0;
    size_t CurrentRootPage = 0;
    size_t ReturnPlugin = 0;
    std::string RestoreSetting;
    uint64_t RetryBuildAt{};

    size_t RootCapacity()
    {
        return Plugins && Plugins->plugins.size() > MaxRows ? MaxRows - 1 : MaxRows;
    }

    size_t RootPageCount()
    {
        auto count = Plugins ? Plugins->plugins.size() : 0;
        return std::max<size_t>(1, (count + RootCapacity() - 1) / RootCapacity());
    }
    std::vector<WFPPlugins::Screen> Screens;
    std::vector<bool> BuiltVisibility;

    void ScreensRetired()
    {
        Screens.clear();
        BuiltVisibility.clear();
        Preset = {};
        Restore = {};
        KeepPresetChoice = false;
        RetryBuildAt = 0;
    }

    void ResetRestartNotice();

    void ConstructScreen(void* screen)
    {
        RetireRows(RowsOwner.Get());
        ScreenOwner.Bind(screen);
        ResetRestartNotice();
        CurrentPlugin = -1;
        CurrentScreen = CurrentRootPage = ReturnPlugin = 0;
        RestoreSetting.clear();
        PluginsScreen = PendingRebuild = 0;
        Commands.clear();
        Plugins.reset();
        CachedSettings.clear();
        SetupManager = nullptr;
        DiscoverNeeded = RefreshNeeded = true;
        StockRebuildNeeded = SnapshotLayoutChanged = false;
    }

    SafetyHookInline shDestroyScreen, shClearRows, shScreenSetup, shNativeDefaults, shVideoBasic, shVideoAdvanced, shBack;

    uintptr_t __fastcall DestroyScreen(void* screen, void* edx)
    {
        RetireRows(screen);
        if (ScreenOwner.Clear(screen))
        {
            PluginsScreen = PendingRebuild = 0;
            Commands.clear();
            Plugins.reset();
            CachedSettings.clear();
            SetupManager = nullptr;
            DiscoverNeeded = RefreshNeeded = StockRebuildNeeded = false;
        }
        return shDestroyScreen.unsafe_fastcall<uintptr_t>(screen, edx);
    }

    uintptr_t __fastcall ClearRows(void* list, void* edx)
    {
        RetireRows(static_cast<uint8_t*>(list) - 0x28);
        ++NativeDepth;
        auto result = shClearRows.unsafe_fastcall<uintptr_t>(list, edx);
        --NativeDepth;
        return result;
    }

    uintptr_t __fastcall ScreenSetup(void* manager, void* edx)
    {
        SetupManager = manager;
        auto page = PlacementPage();
        if (page >= 2)
            JoinedSettings[MenuMode()][page].clear();
        ++NativeDepth;
        auto result = shScreenSetup.unsafe_fastcall<uintptr_t>(manager, edx);
        --NativeDepth;
        return result;
    }

    uintptr_t BuildVideo(void* manager, void* edx, int page)
    {
        SetupManager = manager;
        auto previous = std::exchange(BuildingVideoPage, page);
        JoinedSettings[MenuMode()][page].clear();
        ++NativeDepth;
        auto result = page == 0 ? shVideoBasic.unsafe_fastcall<uintptr_t>(manager, edx) :
            shVideoAdvanced.unsafe_fastcall<uintptr_t>(manager, edx);
        --NativeDepth;
        BuildingVideoPage = previous;
        return result;
    }

    uintptr_t __fastcall BuildVideoBasic(void* manager, void* edx)
    {
        return BuildVideo(manager, edx, 0);
    }

    uintptr_t __fastcall BuildVideoAdvanced(void* manager, void* edx)
    {
        return BuildVideo(manager, edx, 1);
    }

    void RequestRebuild();

    uintptr_t __fastcall Back(void* manager, void* edx)
    {
        if (manager == SetupManager && *reinterpret_cast<int*>(static_cast<uint8_t*>(manager) + 4) == CategoryState &&
            pOptionsScreen && *pOptionsScreen == PluginsScreen && CurrentPlugin >= 0)
        {
            CurrentPlugin = -1;
            CurrentRootPage = ReturnPlugin / RootCapacity();
            RestoreSetting.clear();
            RequestRebuild();
            return 0;
        }
        return shBack.unsafe_fastcall<uintptr_t>(manager, edx);
    }

    uintptr_t __fastcall NativeDefaults(void* screen, void* edx)
    {
        Command command{ CommandKind::Defaults, RowsOwner.Capture(), ScreenOwner.Capture(), Plugins };
        for (const auto& [id, row] : Rows)
        {
            if (row.widget && row.item.kind == WFPPlugins::Row::Kind::Setting &&
                !asi_settings::text::IEquals(row.item.setting->section, "LANGUAGE"))
                command.defaults.push_back(row.item);
        }
        ++NativeDepth;
        ++DefaultsDepth;
        auto result = shNativeDefaults.unsafe_fastcall<uintptr_t>(screen, edx);
        --DefaultsDepth;
        --NativeDepth;
        if (command.lifetime.screen == screen && ScreenOwner.Matches(command.lifetime))
            Commands.push_back(std::move(command));
        return result;
    }

    std::vector<std::string> Exclude(const asi_settings::LoadedPlugin& plugin)
    {
        return plugin.module == Self ? PlacedSettings() : std::vector<std::string>{};
    }

    std::string ScreenName(const WFPPlugins::Screen& screen)
    {
        // The screen's title without the plugin's name, which the first row shows
        auto name = WFPPlugins::Name(*screen.plugin);
        if (screen.title.size() > name.size() + 2 && screen.title.compare(0, name.size() + 2, name + ": ") == 0)
            return screen.title.substr(name.size() + 2);
        return screen.title;
    }

    // The texts of a row, rebuilt with the row and when the language changes
    void UpdateValueText(int id, const MenuRow& row)
    {
        if (row.shape == Shape::ValueText && row.item.setting)
        {
            auto found = CachedSettings.find(row.item.setting);
            SetText(ValueHash(id), found != CachedSettings.end() ? found->second.display : CachedValue(row.item));
        }
    }

    void UpdateRowTexts(int id, const MenuRow& row)
    {
        if (row.rootPlugin >= 0)
        {
            SetText(LabelHash(id), WFPPlugins::Name(*row.item.plugin));
            return;
        }
        SetText(LabelHash(id), WFPPlugins::Label(row.item));
        if (row.shape == Shape::Choice)
        {
            for (size_t i = 0; i < row.values.size() && i < ChoiceLabels; ++i)
                SetText(ChoiceHash(id, i), ChoiceText(row, row.values[i]));
        }
        UpdateValueText(id, row);
    }

    void UpdatePluginTexts()
    {
        SetText(LabelHash(PageRowId), WFPText::Get("WFP_SECTION"));
        auto title = WFPText::Get("WFP_PLUGINS");
        if (Plugins && CurrentPlugin >= 0 && size_t(CurrentPlugin) < Plugins->plugins.size())
            title = WFPPlugins::Name(Plugins->plugins[CurrentPlugin]);
        SetText(TextHash("WFP_SCREEN_TITLE"), WFPText::Upper(title));
        if (CurrentPlugin < 0)
            SetText(ValueHash(PageRowId), std::to_string(CurrentRootPage + 1) + "/" + std::to_string(RootPageCount()));
        else if (CurrentScreen < Screens.size())
            SetText(ValueHash(PageRowId), ScreenName(Screens[CurrentScreen]));
    }

    // The texts, rebuilt when the game's language changes
    void UpdateTexts()
    {
        static std::string current;
        auto name = TextLanguageName();
        if (name == current)
            return;
        current = name;
        DiscoverNeeded = RefreshNeeded = true;
        WFPText::SetLanguage(name);

        SetKeyText("WFP_ON");
        SetKeyText("WFP_OFF");
        for (auto label : PresetLabels)
            SetKeyText(label);
        Texts[TextHash("WFP_EMPTY")].reserve(256);
        UpdatePluginTexts();
        for (auto& [id, row] : Rows)
            UpdateRowTexts(id, row);

        // The category in the options menu and the screen's title, in capitals like the game's
        SetText(TextHash("WFP_PLUGINS"), WFPText::Upper(WFPText::Get("WFP_PLUGINS")));
    }

    // Widgets of the game
    void* (__cdecl* GameMalloc)(size_t size) = nullptr;
    uint32_t (__cdecl* HashString)(const char* format, ...) = nullptr;
    void* (__fastcall* ToggleWidget)(void* widget, void* edx, uint32_t* title, int id, uint32_t on, uint32_t off, bool enabled) = nullptr;
    void* (__fastcall* ChoiceWidget)(void* widget, void* edx, uint32_t* title, int id, uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e) = nullptr;
    void* (__fastcall* SliderWidget)(void* widget, void* edx, uint32_t* title, int id, float minimum, float maximum, float step, bool unknown) = nullptr;
    void (__fastcall* AddWidget)(void* screen, void* edx, uint32_t name, void* widget) = nullptr;
    void* (__fastcall* WidgetOptionCtor)(void* option, void* edx, uint32_t name, void* widget) = nullptr;
    void* (__fastcall* InsertOption)(void* screen, void* edx, void* option) = nullptr;
    void (__cdecl* SetHeaderHash)(const char* package, uint32_t object, uint32_t language) = nullptr;
    uint32_t HeaderObjectHash{};

    void DestroyWidget(void* widget)
    {
        if (widget)
            reinterpret_cast<void(__thiscall*)(void*, int)>((*reinterpret_cast<void***>(widget))[0])(widget, 1);
    }

    void* PublishWidget(void* screen, void* widget)
    {
        auto token = RowsOwner.Capture();
        if (!Current(token) || token.screen != screen)
        {
            DestroyWidget(widget);
            return nullptr;
        }
        auto& counter = *reinterpret_cast<int*>(static_cast<uint8_t*>(screen) + 0x164);
        if (!widget || counter >= MaxRows ||
            !WidgetOptionCtor || !InsertOption)
        {
            DestroyWidget(widget);
            return nullptr;
        }
        auto memory = GameMalloc(80);
        if (!memory)
        {
            DestroyWidget(widget);
            return nullptr;
        }
        auto option = WidgetOptionCtor(memory, nullptr, HashString("OPTION_%d", counter + 1), widget);
        if (!Current(token))
        {
            reinterpret_cast<void(__thiscall*)(void*, int)>((*reinterpret_cast<void***>(option))[0])(option, 1);
            return nullptr;
        }
        // Native insertion deletes the wrapper and its widget when the package slot cannot bind.
        if (!InsertOption(screen, nullptr, option) || !Current(token))
            return nullptr;
        ++counter;
        return option;
    }

    constexpr uint32_t NoLabel = 0x881F8EFA;   // the game's empty entry

    // Widget messages
    constexpr uint32_t MessageAccept = 0x0C407210;
    constexpr uint32_t MessageLeft = 0x9120409E;
    constexpr uint32_t MessageRight = 0xB5971BF1;

    // A widget's vtable: slot 1 shows its value (read from DALOptions), slot 4 handles a message
    void (__cdecl* SetLanguageHash)(void* object, uint32_t hash) = nullptr;

    std::vector<uint8_t*> StringLeaves(void* object)
    {
        std::vector<uint8_t*> leaves;
        auto visit = [&](auto&& self, uint8_t* current, size_t depth) -> void
        {
            if (!current || depth > 32 || leaves.size() >= 4096)
                return;
            auto type = *reinterpret_cast<uint32_t*>(current + 0x18);
            if (type == 2)
                leaves.push_back(current);
            else if (type == 5)
            {
                size_t count{};
                for (auto child = *reinterpret_cast<uint8_t**>(current + 0x64); child && count++ < 4096;
                    child = *reinterpret_cast<uint8_t**>(child + 4))
                    self(self, child, depth + 1);
            }
        };
        visit(visit, static_cast<uint8_t*>(object), 0);
        return leaves;
    }

    bool PublishEncoded(uint8_t* object, uint32_t hash, std::string encoded, ScreenToken token)
    {
        if (!Current(token) || !SetLanguageHash)
            return false;
        auto previous = PublishedTexts.find(object);
        bool changed = previous == PublishedTexts.end() || previous->second.first != hash || previous->second.second != encoded ||
            *reinterpret_cast<uint32_t*>(object + 0x60) != hash || (*reinterpret_cast<uint32_t*>(object + 0x1C) & 2) != 0 ||
            (*reinterpret_cast<uint32_t*>(object + 0x1C) & 0x800) == 0 || *reinterpret_cast<uint32_t*>(object + 0x74) != 0;
        encoded.resize(std::min<size_t>(encoded.size(), 1023));
        Texts[hash] = encoded;
        // Restore the authored limit when the row retires and the slot returns to the game.
        TextMaximums.try_emplace(object, *reinterpret_cast<uint32_t*>(object + 0x74));
        TextFormatFlags.try_emplace(object, *reinterpret_cast<uint32_t*>(object + 0x1C) & 0x800);
        *reinterpret_cast<uint32_t*>(object + 0x1C) |= 0x800;
        *reinterpret_cast<uint32_t*>(object + 0x74) = 0;
        if (changed)
        {
            SetLanguageHash(object, hash);
            if (!Current(token))
                return false;
            *reinterpret_cast<uint32_t*>(object + 0x1C) |= 0x2000000;
            PublishedTexts[object] = { hash, std::move(encoded) };
        }
        return Current(token);
    }

    std::string ValueLine(uint8_t* package, const MenuRow& row, uint8_t* object, const std::string& text, ScreenToken token);

    bool PublishGroup(void* group, const std::string& key, const std::string& text, ScreenToken token,
        float width = std::numeric_limits<float>::quiet_NaN(), const MenuRow* valueRow = nullptr)
    {
        if (!SetLanguageHash || !Current(token))
            return Current(token);
        auto leaves = StringLeaves(group);
        for (size_t i = 0; i < leaves.size(); ++i)
        {
            if (!Current(token))
                return false;
            auto object = leaves[i];
            auto encoded = SingleLine(FontText(object, text, false));
            if (valueRow)
                encoded = ValueLine(nullptr, *valueRow, object, text, token);
            if (std::isfinite(width))
            {
                encoded = WFPMenuLayout::Fit(std::move(encoded), width, [&](const std::string& candidate)
                {
                    return MeasureText(object, candidate);
                });
            }
            auto hash = TextHash(key + "_" + std::to_string(i));
            if (!PublishEncoded(object, hash, std::move(encoded), token))
                return false;
        }
        return Current(token);
    }

    std::string ShownValue(const MenuRow& row)
    {
        if (row.pageSelector)
        {
            if (CurrentPlugin < 0)
                return std::to_string(CurrentRootPage + 1) + "/" + std::to_string(RootPageCount());
            return CurrentScreen < Screens.size() ? ScreenName(Screens[CurrentScreen]) : std::string();
        }
        if (row.rootPlugin >= 0)
            return {};
        if (row.item.kind == WFPPlugins::Row::Kind::Preset)
            return WFPText::Get(PresetLabels[std::clamp(PresetIndex(), 0, int(std::size(PresetLabels)) - 1)]);
        if (row.item.kind == WFPPlugins::Row::Kind::RestoreDefaults)
            return Restore.confirm ? WFPText::Get("WFP_PRESET_CONFIRM") : std::string();
        if (!row.item.setting)
            return {};
        auto cached = CachedSettings.find(row.item.setting);
        return cached != CachedSettings.end() ? cached->second.display : CachedValue(row.item);
    }

    std::string RowLabel(const MenuRow& row)
    {
        if (row.pageSelector)
            return WFPText::Get("WFP_SECTION");
        return row.rootPlugin >= 0 ? WFPPlugins::Name(*row.item.plugin) : WFPPlugins::Label(row.item);
    }

    struct WidgetTable
    {
        std::array<uintptr_t, 16> entries{};
        void (__thiscall* draw)(void*){};
        void (__thiscall* initialize)(void*){};
    };
    std::map<uintptr_t*, WidgetTable> WidgetTables;

    float ColumnWidth(void* widget)
    {
        auto value = *reinterpret_cast<uint8_t**>(static_cast<uint8_t*>(widget) + 32);
        if (!value || *reinterpret_cast<uint32_t*>(value + 0x18) != 2 ||
            *reinterpret_cast<uint32_t*>(value + 0x30) < 68)
            return std::numeric_limits<float>::quiet_NaN();
        auto state = *reinterpret_cast<float**>(value + 0x2C);
        auto maximum = *reinterpret_cast<uint32_t*>(value + 0x74);
        auto saved = TextMaximums.find(value);
        if (saved != TextMaximums.end())
            maximum = saved->second;
        // The authored data string defines the column before its native font scale is applied.
        auto width = state ? float(maximum) * std::abs(state[14]) *
            RenderScale(*reinterpret_cast<uint32_t*>(value + 0x24)) : 0.0f;
        if (!std::isfinite(width) || width <= 0.0f)
            return std::numeric_limits<float>::quiet_NaN();
        return width;
    }

    void __fastcall DrawWidget(void* widget, void*)
    {
        auto table = *reinterpret_cast<uintptr_t**>(widget);
        auto original = std::find_if(WidgetTables.begin(), WidgetTables.end(), [&](const auto& entry)
        {
            return entry.second.entries.data() + 1 == table;
        });
        if (original == WidgetTables.end())
            return;
        auto draw = original->second.draw;
        auto token = RowsOwner.Capture();
        auto pin = Plugins;
        auto found = std::find_if(Rows.begin(), Rows.end(), [&](const auto& entry) { return entry.second.widget == widget; });
        bool pager = widget == Pager.widget;
        if ((!pager && found == Rows.end()) || !Current(token))
        {
            draw(widget);
            return;
        }
        auto id = pager ? PageRowId : found->first;
        auto row = pager ? Pager : found->second;
        ValueTick = GetTickCount64();
        ContinuedValues.erase(widget);
        PendingFonts.erase(widget);
        auto column = ColumnWidth(widget);
        LayoutRowValue(row, token);
        if (!Current(token) || Plugins != pin)
            return;
        auto bytes = static_cast<uint8_t*>(widget);
        auto title = *reinterpret_cast<uint32_t*>(bytes + 12);
        // Native label drawing treats localized text as a printf format. Owned labels use hashes instead.
        *reinterpret_cast<uint32_t*>(bytes + 12) = 0;
        ++NativeDepth;
        draw(widget);
        --NativeDepth;
        if (!Current(token) || Plugins != pin)
            return;
        *reinterpret_cast<uint32_t*>(bytes + 12) = title;
        auto label = RowLabel(row);
        if (!PublishGroup(*reinterpret_cast<void**>(bytes + 24), "DRAW_LABEL_" + std::to_string(id), label, token, column))
            return;
        auto value = *reinterpret_cast<void**>(bytes + (row.shape == Shape::Slider ? 64 : 28));
        PublishGroup(value, "DRAW_VALUE_" + std::to_string(id), ShownValue(row), token,
            std::numeric_limits<float>::quiet_NaN(), &row);
    }

    void** FrontendInstance{};
    uint8_t* (__thiscall* FindPackage)(void*, const char*){};
    void** ActiveCharset{};
    uint32_t* ActiveCharsetMode{};
    NFSCHelp::Captions HelpCaptions;
    struct HelpPage
    {
        std::string raw;
        std::vector<std::string> lines;
        std::string language;
        void* fontObject{};
        void* charset{};
        uint32_t rawCharset{};
        uint32_t font{};
        int gameLanguage{};
        int row{};
        float width{};
        float scale{};
        size_t count{};
        uint64_t started{};
    } Help;

    uint8_t* ScreenPackage(void* screen, std::string& name)
    {
        auto frontend = FrontendInstance ? *FrontendInstance : nullptr;
        auto package = screen ? *reinterpret_cast<const char**>(static_cast<uint8_t*>(screen) + 12) : nullptr;
        if (!FindPackage || !frontend || !package)
            return nullptr;
        name = package;
        return FindPackage(frontend, package);
    }

    void RefreshTitle(ScreenToken token)
    {
        if (!Current(token) || reinterpret_cast<uintptr_t>(token.screen) != PluginsScreen || !HeaderObjectHash)
            return;
        std::string name;
        auto package = ScreenPackage(token.screen, name);
        if (!Current(token) || !package)
            return;
        size_t budget{ 4096 };
        auto find = [&](auto&& self, uint8_t* head, size_t depth) -> uint8_t*
        {
            for (auto object = head; object && budget && depth < 32;
                object = *reinterpret_cast<uint8_t**>(object + 4))
            {
                --budget;
                if (*reinterpret_cast<uint32_t*>(object + 0x10) == HeaderObjectHash)
                    return object;
                if (*reinterpret_cast<uint32_t*>(object + 0x18) == 5)
                {
                    if (auto child = self(self, *reinterpret_cast<uint8_t**>(object + 0x64), depth + 1))
                        return child;
                }
            }
            return nullptr;
        };
        auto header = find(find, *reinterpret_cast<uint8_t**>(package + 0x4C), 0);
        auto leaves = StringLeaves(header);
        if (leaves.empty())
            return;
        auto state = *reinterpret_cast<float**>(leaves.front() + 0x2C);
        auto maximum = *reinterpret_cast<uint32_t*>(leaves.front() + 0x74);
        auto saved = TextMaximums.find(leaves.front());
        if (saved != TextMaximums.end())
            maximum = saved->second;
        auto width = state && maximum ? float(maximum) * std::abs(state[14]) *
            RenderScale(*reinterpret_cast<uint32_t*>(leaves.front() + 0x24)) : std::numeric_limits<float>::quiet_NaN();
        auto title = WFPText::Get("WFP_PLUGINS");
        if (Plugins && CurrentPlugin >= 0 && size_t(CurrentPlugin) < Plugins->plugins.size())
            title = WFPPlugins::Name(Plugins->plugins[CurrentPlugin]);
        PublishGroup(header, "DRAW_HEADER", WFPText::Upper(title), token, width);
    }

    std::string ValueLine(uint8_t* package, const MenuRow& row, uint8_t* object, const std::string& text, ScreenToken token)
    {
        if (!Current(token))
            return {};
        std::string name;
        if (!package)
            package = ScreenPackage(token.screen, name);
        if (!Current(token))
            return {};
        auto state = *reinterpret_cast<float**>(object + 0x2C);
        auto width = package ? NFSCRowLayout::Layout::Capacity(package, static_cast<uint8_t*>(row.widget), object,
            row.shape == Shape::Slider) : std::numeric_limits<float>::quiet_NaN();
        auto font = *reinterpret_cast<uint32_t*>(object + 0x24);
        auto fontObject = FindFont ? FindFont(font) : nullptr;
        auto charset = ActiveCharset ? *ActiveCharset : nullptr;
        auto rawCharset = ActiveCharsetMode ? *ActiveCharsetMode : 0;
        auto language = CurrentLanguage ? *CurrentLanguage : 0;
        auto scale = state ? state[14] : 0.0f;
        auto& page = ValuePages[object];
        if (page.raw != text || page.width != width || page.font != font || page.fontObject != fontObject ||
            page.scale != scale || page.charset != charset || page.rawCharset != rawCharset || page.gameLanguage != language || page.language != WFPText::Language())
        {
            page.raw = text;
            page.width = width;
            page.font = font;
            page.fontObject = fontObject;
            page.scale = scale;
            page.charset = charset;
            page.rawCharset = rawCharset;
            page.gameLanguage = language;
            page.language = WFPText::Language();
            auto encoded = SingleLine(FontText(object, text, false));
            page.lines.clear();
            auto measured = MeasureText(object, encoded);
            if (std::isfinite(width) && width > 0.0f && std::isfinite(measured))
            {
                page.lines = WFPMenuLayout::Wrap(encoded, width, [&](const std::string& candidate)
                {
                    return MeasureText(object, candidate);
                });
            }
            if (page.lines.empty())
            {
                encoded.resize(std::min<size_t>(encoded.size(), 1023));
                page.lines.push_back(std::move(encoded));
            }
        }
        if (!fontObject || !std::isfinite(width) || width <= 0.0f)
            PendingFonts.insert(row.widget);
        if (page.lines.size() > 1)
            ContinuedValues.insert(row.widget);
        return page.lines[size_t(ValueTick / 12000) % page.lines.size()];
    }

    void LayoutRowValue(const MenuRow& row, ScreenToken token)
    {
        if (!Current(token) || !row.widget || !row.option || !FindFont || !GlyphAdvance || !LocalizedWide)
            return;
        auto pin = Plugins;
        std::string name;
        auto package = ScreenPackage(token.screen, name);
        if (!Current(token) || Plugins != pin || !package)
            return;
        auto selected = *reinterpret_cast<void**>(static_cast<uint8_t*>(token.screen) + 0x34) == row.option;
        auto text = ShownValue(row);
        ValueLayout.Place(package, static_cast<uint8_t*>(row.widget), row.shape == Shape::Slider, selected,
            [&](uint8_t* object)
            {
                return MeasureText(object, ValueLine(package, row, object, text, token));
            });
    }

    void RetireHelp(void* screen)
    {
        if (HelpCaptions.HasPackages())
        {
            std::string name;
            auto package = ScreenPackage(screen, name);
            HelpCaptions.Hide(package, name);
        }
        Help = {};
        HelpCaptions.ResetMetrics();
    }

    void RefreshHelp(ScreenToken token)
    {
        if (!Current(token))
            return;
        auto selected = *reinterpret_cast<void**>(static_cast<uint8_t*>(token.screen) + 0x34);
        auto found = std::find_if(Rows.begin(), Rows.end(), [&](const auto& entry) { return entry.second.option == selected; });
        std::string name;
        auto package = ScreenPackage(token.screen, name);
        if (!Current(token))
            return;
        bool pager = selected && selected == Pager.option;
        if ((!pager && found == Rows.end()) || !package)
        {
            HelpCaptions.Hide(package, name);
            Help = {};
            return;
        }
        auto id = pager ? PageRowId : found->first;
        auto row = pager ? Pager : found->second;
        auto pin = Plugins;
        auto label = RowLabel(row);
        auto value = ShownValue(row);
        auto text = label + (value.empty() ? std::string() : ": " + value);
        if (!row.pageSelector && row.rootPlugin < 0)
            text += "\n" + WFPPlugins::Help(row.item);
        auto group = row.widget ? *reinterpret_cast<void**>(static_cast<uint8_t*>(row.widget) + 24) : nullptr;
        auto leaves = StringLeaves(group);
        float width{};
        HelpCaptions.ResetMetrics();
        auto captions = leaves.empty() ? std::vector<uint8_t*>{} : HelpCaptions.Prepare(token.screen, package, name,
            leaves.front(), FindFont, CurrentLanguage ? *CurrentLanguage : 0, width,
            [&]() { return Current(token) && Plugins == pin; });
        if (!Current(token) || Plugins != pin)
            return;
        if (captions.empty())
        {
            HelpCaptions.Hide(package, name);
            return;
        }
        auto state = *reinterpret_cast<float**>(captions.front() + 0x2C);
        auto font = *reinterpret_cast<uint32_t*>(captions.front() + 0x24);
        auto fontObject = FindFont ? FindFont(font) : nullptr;
        auto charset = ActiveCharset ? *ActiveCharset : nullptr;
        auto rawCharset = ActiveCharsetMode ? *ActiveCharsetMode : 0;
        auto language = CurrentLanguage ? *CurrentLanguage : 0;
        if (Help.raw != text || Help.row != id || Help.width != width || Help.count != captions.size() ||
            Help.scale != state[14] || Help.font != font || Help.fontObject != fontObject || Help.charset != charset || Help.rawCharset != rawCharset ||
            Help.gameLanguage != language || Help.language != WFPText::Language())
        {
            Help.raw = text;
            Help.row = id;
            Help.width = width;
            Help.count = captions.size();
            Help.scale = state[14];
            Help.font = font;
            Help.fontObject = fontObject;
            Help.charset = charset;
            Help.rawCharset = rawCharset;
            Help.gameLanguage = language;
            Help.language = WFPText::Language();
            auto encoded = FontText(captions.front(), text, false);
            Help.lines = WFPMenuLayout::Wrap(encoded, width, [&](const std::string& candidate)
            {
                return MeasureText(captions.front(), candidate);
            });
            Help.started = GetTickCount64();
        }
        auto pages = std::max<size_t>(1, (Help.lines.size() + captions.size() - 1) / captions.size());
        auto page = size_t((GetTickCount64() - Help.started) / 12000) % pages;
        for (size_t i = 0; i < captions.size(); ++i)
        {
            auto index = page * captions.size() + i;
            auto line = index < Help.lines.size() ? Help.lines[index] : std::string();
            auto caption = captions[i];
            auto& flags = *reinterpret_cast<uint32_t*>(caption + 0x1C);
            if (line.empty())
                flags |= 0x2400001;
            else if ((flags & 1) != 0)
                flags = (flags & ~1u) | 0x2400000;
            if (!PublishEncoded(caption, TextHash("HELP_" + std::to_string(i)), std::move(line), token))
                return;
        }
    }

    void (__cdecl* ShowMessage)(const char*){};
    void** DialogConfig{};
    void** DialogInstance{};
    bool RestartChange{};
    bool RestartWarned{};

    void ResetRestartNotice()
    {
        RestartChange = RestartWarned = false;
    }

    void RestartNotice(ScreenToken token)
    {
        if (!Current(token) || !RestartChange || RestartWarned || !ShowMessage || !DialogConfig || !DialogInstance || !SetupManager || *DialogConfig || *DialogInstance)
            return;
        uint8_t* style{};
        auto selected = *reinterpret_cast<void**>(static_cast<uint8_t*>(token.screen) + 0x34);
        for (const auto& [id, row] : Rows)
        {
            if (row.option == selected && row.widget)
            {
                auto leaves = StringLeaves(*reinterpret_cast<void**>(static_cast<uint8_t*>(row.widget) + 24));
                if (!leaves.empty())
                    style = leaves.front();
                break;
            }
        }
        auto warning = FontText(style, WFPText::Get("WFP_RESTART_WARNING"), false);
        if (!Current(token))
            return;
        auto manager = static_cast<uint8_t*>(SetupManager);
        // The native acknowledgement returns through this field, as its own Options dialogs do.
        *reinterpret_cast<int*>(manager + 0xDC) = *reinterpret_cast<int*>(manager + 4);
        RestartWarned = true;
        ShowMessage(warning.c_str());
    }

    void __fastcall InitializeWidget(void* widget, void*)
    {
        auto table = *reinterpret_cast<uintptr_t**>(widget);
        auto original = std::find_if(WidgetTables.begin(), WidgetTables.end(), [&](const auto& entry)
        {
            return entry.second.entries.data() + 1 == table;
        });
        if (original == WidgetTables.end())
            return;
        auto initialize = original->second.initialize;
        auto token = RowsOwner.Capture();
        auto owned = widget == Pager.widget || std::any_of(Rows.begin(), Rows.end(), [&](const auto& entry) { return entry.second.widget == widget; });
        if (!owned || !Current(token))
        {
            initialize(widget);
            return;
        }
        auto bytes = static_cast<uint8_t*>(widget);
        auto title = *reinterpret_cast<uint32_t*>(bytes + 12);
        *reinterpret_cast<uint32_t*>(bytes + 12) = 0;
        ++NativeDepth;
        initialize(widget);
        --NativeDepth;
        if (Current(token))
            *reinterpret_cast<uint32_t*>(bytes + 12) = title;
    }

    void OwnWidget(void* widget)
    {
        if (!widget)
            return;
        auto table = *reinterpret_cast<uintptr_t**>(widget);
        auto [found, added] = WidgetTables.try_emplace(table);
        auto& owned = found->second;
        if (added)
        {
            std::copy_n(table - 1, owned.entries.size(), owned.entries.data());
            owned.draw = reinterpret_cast<decltype(owned.draw)>(table[1]);
            owned.initialize = reinterpret_cast<decltype(owned.initialize)>(table[2]);
            owned.entries[2] = reinterpret_cast<uintptr_t>(&DrawWidget);
            owned.entries[3] = reinterpret_cast<uintptr_t>(&InitializeWidget);
        }
        *reinterpret_cast<uintptr_t**>(widget) = owned.entries.data() + 1;
    }

    void RedrawWidget(void* widget)
    {
        if (widget)
            reinterpret_cast<void(__fastcall*)(void*, void*)>((*reinterpret_cast<uintptr_t**>(widget))[1])(widget, nullptr);
    }

    // The Plugins screen's rows, after a preset or the defaults were applied or another plugin changed a value
    void RedrawRows()
    {
        auto token = RowsOwner.Capture();
        auto pin = Plugins;
        std::vector<int> ids;
        for (const auto& [id, row] : Rows)
            ids.push_back(id);
        for (int id : ids)
        {
            if (!Current(token) || Plugins != pin)
                return;
            auto found = Rows.find(id);
            if (found == Rows.end() || !found->second.widget)
                continue;
            auto row = found->second;
            if (row.item.kind == WFPPlugins::Row::Kind::Preset && !KeepPresetChoice)
            {
                auto shown = Preset.Current();
                if (!Current(token) || Plugins != pin)
                    return;
                Preset.shown = shown;
            }
            auto cached = CachedSettings.find(row.item.setting);
            bool enabled = cached == CachedSettings.end() || cached->second.enabled;
            if (enabled != row.enabled)
            {
                Rows.at(id).enabled = enabled;
                *reinterpret_cast<uint8_t*>(static_cast<uint8_t*>(row.widget) + 100) = uint8_t(enabled);
                if (row.option)
                    *reinterpret_cast<uint8_t*>(static_cast<uint8_t*>(row.option) + 64) = uint8_t(enabled);
                auto vtable = *reinterpret_cast<void***>(row.widget);
                if (enabled)
                    reinterpret_cast<void(__thiscall*)(void*, bool)>(vtable[9])(row.widget, true);
                else
                    reinterpret_cast<void(__thiscall*)(void*)>(vtable[10])(row.widget);
            }
            UpdateRowTexts(id, row);
            RedrawWidget(row.widget);
        }
    }

    // Any other row's input cancels a pending confirmation
    void CancelConfirmations(int except)
    {
        auto token = RowsOwner.Capture();
        std::vector<int> ids;
        for (const auto& [id, row] : Rows)
            ids.push_back(id);
        for (int id : ids)
        {
            if (!Current(token))
                return;
            auto found = Rows.find(id);
            if (found == Rows.end() || id == except || !found->second.widget)
                continue;
            auto row = found->second;
            bool cancelled = false;
            if (row.item.kind == WFPPlugins::Row::Kind::Preset)
                cancelled = Preset.Cancel();
            else if (row.item.kind == WFPPlugins::Row::Kind::RestoreDefaults)
                cancelled = Restore.Cancel();
            if (cancelled)
                RedrawWidget(row.widget);
        }
    }

    // The button rows' widgets: a choice widget's vtable (15 slots, the type info before them) with a message handler
    // that takes accept as well
    using MessageFn = int(__fastcall*)(void* widget, void* edx, int unknown, uint32_t message);
    MessageFn OriginalChoiceMessage = nullptr;
    uintptr_t ButtonVtable[16] = {};

    int __fastcall ButtonMessage(void* widget, void* edx, int unknown, uint32_t message)
    {
        int id = *reinterpret_cast<int*>(static_cast<uint8_t*>(widget) + 0x74);
        auto it = Rows.find(id);
        if (it == Rows.end())
            return OriginalChoiceMessage(widget, edx, unknown, message);
        if (message == MessageAccept)
        {
            Queue(CommandKind::Accept, id);
            return 0;
        }
        return it->second.item.kind == WFPPlugins::Row::Kind::Preset ?
            OriginalChoiceMessage(widget, edx, unknown, message) : 0;
    }

    void* MakeChoice(int id, const uint32_t (&labels)[5])
    {
        uint32_t title = LabelHash(id);
        auto memory = GameMalloc(140);
        return memory ? ChoiceWidget(memory, nullptr, &title, id, labels[0], labels[1], labels[2], labels[3], labels[4]) : nullptr;
    }

    void* MakeButton(int id, const uint32_t (&labels)[5])
    {
        auto widget = MakeChoice(id, labels);
        if (!widget)
            return nullptr;
        auto& vtable = *reinterpret_cast<uintptr_t**>(widget);
        if (!OriginalChoiceMessage)
        {
            std::copy_n(vtable - 1, std::size(ButtonVtable), ButtonVtable);
            OriginalChoiceMessage = reinterpret_cast<MessageFn>(ButtonVtable[1 + 4]);
            ButtonVtable[1 + 4] = reinterpret_cast<uintptr_t>(&ButtonMessage);
        }
        vtable = ButtonVtable + 1;
        return widget;
    }

    // Adds a row with the game's widget for it; false when it has none
    bool AddRow(void* screen, int id, const WFPPlugins::Row& item)
    {
        auto token = RowsOwner.Capture();
        auto pin = Plugins;
        if (!Current(token) || token.screen != screen)
            return false;
        MenuRow row{ item };
        if (item.kind == WFPPlugins::Row::Kind::Setting)
        {
            auto& setting = *item.setting;
            auto cached = CachedSettings.find(&setting);
            if (cached != CachedSettings.end())
                row.values = cached->second.choices;
            if (setting.type == asi_settings::Type::Bool)
                row.shape = Shape::Toggle;
            else if (!row.values.empty())
                row.shape = row.values.size() > ChoiceLabels ? Shape::ValueText : Shape::Choice;
            else if ((setting.type == asi_settings::Type::Float || setting.type == asi_settings::Type::Int) && setting.min && setting.max && setting.step)
                row.shape = Shape::Slider;
            else if (setting.type == asi_settings::Type::String)
                row.shape = Shape::ValueText;
            else
                return false;
        }

        auto& stored = Rows[id] = std::move(row);
        UpdateRowTexts(id, stored);

        uint32_t title = LabelHash(id);
        uint32_t labels[5] = { NoLabel, NoLabel, NoLabel, NoLabel, NoLabel };
        void* widget = nullptr;
        switch (item.kind)
        {
            case WFPPlugins::Row::Kind::Preset:
                for (size_t i = 0; i < std::size(PresetLabels); ++i)
                    labels[i] = TextHash(PresetLabels[i]);
                widget = MakeButton(id, labels);
                break;
            case WFPPlugins::Row::Kind::RestoreDefaults:
                labels[0] = TextHash("WFP_EMPTY");
                labels[1] = TextHash("WFP_PRESET_CONFIRM");
                widget = MakeButton(id, labels);
                break;
            case WFPPlugins::Row::Kind::Action:
                labels[0] = TextHash("WFP_EMPTY");
                widget = MakeButton(id, labels);
                break;
            default:
                switch (stored.shape)
                {
                    case Shape::Toggle:
                        if (auto memory = GameMalloc(128))
                            widget = ToggleWidget(memory, nullptr, &title, id, TextHash("WFP_ON"), TextHash("WFP_OFF"), true);
                        break;
                    case Shape::Choice:
                        for (size_t i = 0; i < stored.values.size() && i < ChoiceLabels; ++i)
                            labels[i] = ChoiceHash(id, i);
                        widget = MakeChoice(id, labels);
                        break;
                    case Shape::ValueText:
                        labels[0] = ValueHash(id);
                        widget = MakeChoice(id, labels);
                        break;
                    case Shape::Slider:
                    {
                        auto& setting = *item.setting;
                        if (auto memory = GameMalloc(188))
                            widget = SliderWidget(memory, nullptr, &title, id, float(*setting.min), float(*setting.max), float(*setting.step), true);
                        break;
                    }
                    default:
                        break;
                }
                break;
        }
        if (!Current(token) || Plugins != pin)
        {
            DestroyWidget(widget);
            return false;
        }
        if (!widget)
        {
            Rows.erase(id);
            return false;
        }

        stored.widget = widget;
        OwnWidget(widget);
        auto cached = CachedSettings.find(item.setting);
        stored.enabled = cached == CachedSettings.end() || cached->second.enabled;
        *reinterpret_cast<uint8_t*>(static_cast<uint8_t*>(widget) + 100) = uint8_t(stored.enabled);
        auto option = PublishWidget(screen, widget);
        if (!Current(token) || Plugins != pin)
            return false;
        if (!option)
        {
            Rows.erase(id);
            return false;
        }
        stored.widget = widget;
        stored.option = option;
        RedrawWidget(widget);
        if (!Current(token) || Plugins != pin)
            return false;
        if (id >= FirstGameScreenId && item.setting)
        {
            auto page = PlacementPage();
            if (page >= 0)
            {
                auto& joined = JoinedSettings[MenuMode()][page];
                if (std::find(joined.begin(), joined.end(), item.setting->id) == joined.end())
                    joined.push_back(item.setting->id);
            }
        }
        return true;
    }

    void AddValueRow(void* screen, int id)
    {
        auto token = RowsOwner.Capture();
        auto pin = Plugins;
        Rows.erase(id);
        uint32_t labels[5] = { ValueHash(id), NoLabel, NoLabel, NoLabel, NoLabel };
        auto widget = MakeChoice(id, labels);
        if (!Current(token) || Plugins != pin)
        {
            DestroyWidget(widget);
            return;
        }
        Pager = {};
        Pager.shape = Shape::ValueText;
        Pager.pageSelector = true;
        Pager.widget = widget;
        OwnWidget(widget);
        auto option = PublishWidget(screen, widget);
        if (!Current(token) || Plugins != pin)
            return;
        if (!option)
        {
            Pager = {};
            RetryBuildAt = GetTickCount64() + 250;
            return;
        }
        Pager.option = option;
        RedrawWidget(widget);
    }

    bool AddRootRow(void* screen, int id, size_t plugin)
    {
        auto token = RowsOwner.Capture();
        auto pin = Plugins;
        if (!Current(token) || !pin || plugin >= pin->plugins.size())
            return false;
        MenuRow row{ { WFPPlugins::Row::Kind::Action, &pin->plugins[plugin], nullptr } };
        row.rootPlugin = int(plugin);
        Rows[id] = row;
        UpdateRowTexts(id, row);
        uint32_t labels[5] = { TextHash("WFP_EMPTY"), NoLabel, NoLabel, NoLabel, NoLabel };
        auto widget = MakeButton(id, labels);
        if (!Current(token) || Plugins != pin)
        {
            DestroyWidget(widget);
            return false;
        }
        Rows.at(id).widget = widget;
        OwnWidget(widget);
        auto option = PublishWidget(screen, widget);
        if (!Current(token) || Plugins != pin)
            return false;
        if (!option)
        {
            Rows.erase(id);
            return false;
        }
        Rows.at(id).widget = widget;
        Rows.at(id).option = option;
        return true;
    }

    // Which rows of the shown screen are visible, to rebuild it when a value hides or shows one
    std::vector<bool> VisibleRows()
    {
        std::vector<bool> visible;
        if (CurrentScreen < Screens.size())
        {
            for (auto& item : Screens[CurrentScreen].rows)
                visible.push_back(CachedVisible(item));
        }
        return visible;
    }

    // The plugin list, or the selected plugin's section and its own preset/defaults
    void BuildScreen(void* screen)
    {
        auto token = RowsOwner.Capture();
        auto pin = Plugins;
        UpdateTexts();
        if (!Current(token) || !pin || pin->plugins.empty())
            return;
        CurrentRootPage = std::min(CurrentRootPage, RootPageCount() - 1);
        if (CurrentPlugin < 0)
        {
            Screens.clear();
            BuiltVisibility.clear();
            UpdatePluginTexts();
            bool pager = RootPageCount() > 1;
            if (pager)
            {
                AddValueRow(screen, PageRowId);
                if (!Current(token) || RetryBuildAt)
                    return;
            }
            auto first = CurrentRootPage * RootCapacity();
            auto end = std::min(first + RootCapacity(), pin->plugins.size());
            for (size_t plugin = first; plugin < end; ++plugin)
            {
                if (!AddRootRow(screen, FirstPluginScreenId + int(plugin - first), plugin))
                {
                    if (Current(token))
                        RetryBuildAt = GetTickCount64() + 250;
                    return;
                }
            }
            return;
        }
        CurrentPlugin = std::min(CurrentPlugin, int(pin->plugins.size() - 1));
        auto& plugin = pin->plugins[CurrentPlugin];
        Screens = WFPPlugins::Screens(plugin, MaxRows - 1, true, Exclude(plugin));
        CurrentScreen = std::min(CurrentScreen, Screens.empty() ? 0 : Screens.size() - 1);
        WFPPlugins::PresetRow preset;
        preset.Reset(plugin);
        if (!Current(token) || Plugins != pin)
            return;
        Preset = std::move(preset);
        Restore.Cancel();
        UpdatePluginTexts();
        if (Screens.size() > 1)
        {
            AddValueRow(screen, PageRowId);
            if (!Current(token) || RetryBuildAt)
                return;
        }
        if (Screens.empty())
            return;
        BuiltVisibility = VisibleRows();
        int id = FirstPluginScreenId;
        for (size_t i = 0; i < Screens[CurrentScreen].rows.size(); ++i)
        {
            if (BuiltVisibility[i] && !AddRow(screen, id++, Screens[CurrentScreen].rows[i]))
            {
                if (Current(token))
                    RetryBuildAt = GetTickCount64() + 250;
                return;
            }
            if (!Current(token))
                return;
        }
    }

    // Another plugin or page was picked, or the rows to show changed: the list is rebuilt on the next frame, outside
    // the widget's own input handling, the way the game swaps its basic and advanced video lists (clear the list, add
    // the rows, finish)

    void RequestRebuild()
    {
        if (!pOptionsScreen || !*pOptionsScreen || *pOptionsScreen != PluginsScreen)
            return;
        auto screen = reinterpret_cast<uint8_t*>(*pOptionsScreen);
        auto selected = *reinterpret_cast<void**>(screen + 0x34);
        for (const auto& [id, row] : Rows)
        {
            if (row.option == selected && row.item.setting)
                RestoreSetting = row.item.setting->id;
        }
        PendingRebuild = PluginsScreen;
    }

    void RebuildScreen()
    {
        uintptr_t screen = std::exchange(PendingRebuild, 0);
        if (!screen || !pOptionsScreen || *pOptionsScreen != screen)
            return;

        uintptr_t list = screen + 0x28;
        reinterpret_cast<void(__fastcall*)(uintptr_t, void*)>(*reinterpret_cast<uintptr_t*>(*reinterpret_cast<uintptr_t*>(list) + 0x10))(list, nullptr);
        if (ScreenOwner.Get() != reinterpret_cast<void*>(screen))
            return;
        // The native clear retires options but leaves the screen's package-slot counter intact.
        *reinterpret_cast<int*>(screen + 0x164) = 0;
        RowsOwner.Bind(reinterpret_cast<void*>(screen));
        auto token = RowsOwner.Capture();
        BuildScreen(reinterpret_cast<void*>(screen));
        if (!Current(token))
            return;
        auto package = *reinterpret_cast<const char**>(screen + 0x0C);
        if (SetHeaderHash && package)
            SetHeaderHash(package, HeaderObjectHash, TextHash("WFP_SCREEN_TITLE"));
        if (!Current(token))
            return;
        reinterpret_cast<void(__fastcall*)(uintptr_t, void*)>(*reinterpret_cast<uintptr_t*>(*reinterpret_cast<uintptr_t*>(screen) + 0x1C))(screen, nullptr);
        if (!Current(token))
            return;
        if (!Plugins || Plugins->plugins.empty())
            return;
        size_t selection = 0;
        if (CurrentPlugin < 0)
        {
            auto first = CurrentRootPage * RootCapacity();
            auto returned = std::clamp(ReturnPlugin, first, std::min(first + RootCapacity(), Plugins->plugins.size()) - 1);
            selection = returned - first + (RootPageCount() > 1 ? 1 : 0);
        }
        else if (!RestoreSetting.empty())
        {
            size_t index = Screens.size() > 1 ? 1 : 0;
            for (const auto& [id, row] : Rows)
            {
                if (row.item.setting && row.item.setting->id == RestoreSetting)
                {
                    selection = index;
                    break;
                }
                ++index;
            }
        }
        if (*reinterpret_cast<int*>(screen + 0x4C) > int(selection))
            reinterpret_cast<void(__thiscall*)(uintptr_t, int)>((*reinterpret_cast<uintptr_t**>(screen))[3])(screen, int(selection));
    }

    // A value changed through a row: the Plugins screen's preset row follows, and the list is rebuilt when a value hides
    // or shows a row
    void AfterChange(int id, const MenuRow& row)
    {
        RefreshNeeded = true;
    }

    // The game's options menu: the Plugins category, a text option like the game's own, made the way the menu's
    // FEOptionsStateManager adds its categories (0x5B64C0), with the state it opens as its id
    void* (__fastcall* TextOption)(void* option, void* edx, uint32_t name, int id, bool* unknown1, bool* unknown2) = nullptr;
    void (__fastcall* AddMenuOption)(void* menu, void* edx, void* option) = nullptr;

    void AddCategory(void* menu)
    {
        UpdateTexts();
        bool one = true, zero = false;     // what the game passes for its categories
        if (auto memory = GameMalloc(152))
        {
            if (auto option = TextOption(memory, nullptr, TextHash("WFP_PLUGINS"), CategoryState, &one, &zero))
                AddMenuOption(menu, nullptr, option);
        }
    }

    // FeOptionsScreen::AddDalOption: after the game's option a fix setting belongs to, its row
    SafetyHookInline shAddDalOption = {};
    int __fastcall AddDalOption(void* screen, void* edx, int id)
    {
        if (ScreenOwner.Get() == screen && !RowsOwner.Get())
            RowsOwner.Bind(screen);
        ++NativeDepth;
        int result = shAddDalOption.unsafe_fastcall<int>(screen, edx, id);
        --NativeDepth;
        if (CachedSettings.empty())
        {
            StockRebuildNeeded = true;
            return result;
        }
        if (!std::any_of(std::begin(Placements), std::end(Placements), [id](const Placement& p) { return p.after == id; }))
            return result;

        UpdateTexts();
        auto fix = Fix();
        if (!fix)
            return result;
        auto& settings = fix->file.description.settings;
        for (size_t i = 0; i < std::size(Placements); ++i)
        {
            if (Placements[i].after != id)
                continue;
            auto setting = std::find_if(settings.begin(), settings.end(), [&](const asi_settings::Setting& s) { return asi_settings::text::IEquals(s.id, Placements[i].setting); });
            if (setting == settings.end() || !setting->InMenu())
                continue;
            WFPPlugins::Row item{ WFPPlugins::Row::Kind::Setting, fix, &*setting };
            if (CachedVisible(item))
                AddRow(screen, FirstGameScreenId + int(i), item);
        }
        return result;
    }

    // The pause menu's options (FePauseOptionsMain.fng) has five category slots, OPTION_1 to OPTION_5, for its
    // five categories; a category binds to the slot of its position (0x5A1ED0) and is dropped without one. The
    // package data gets a sixth slot when it's loaded: a copy of the last slot's object under the next name and
    // a new GUID, counted as a button and added to the message targets of the original. The menu's scroller
    // places its categories itself, so the copy needs no position of its own.
    namespace PackageSlot
    {
        constexpr uint32_t TagRoot = 0xE76E4546;        // "FEn\xE7"
        constexpr uint32_t TagObjects = 0xCC6A624F;     // "Obj\xCC"
        constexpr uint32_t TagButtons = 0x6E747542;     // "Butn"
        constexpr uint32_t TagObject = 0xEA624F46;      // "FOb\xEA"
        constexpr uint32_t TagObjectData = 0x446A624F;  // "ObjD"
        constexpr uint32_t TagTargets = 0x67726154;     // "Targ"
        constexpr uint16_t RecordObject = 0x504F;       // "OP": GUID, name hash, flags, ...
        constexpr uint16_t RecordName = 0x684F;         // "Oh": name hash
        constexpr uint16_t RecordTargets = 0x744D;      // "Mt": message hash, then the GUIDs it goes to
        constexpr uint32_t ButtonFlag = 0x10000000;

        uint32_t Read(const std::vector<uint8_t>& data, size_t offset)
        {
            uint32_t value;
            memcpy(&value, data.data() + offset, 4);
            return value;
        }

        void Write(std::vector<uint8_t>& data, size_t offset, uint32_t value)
        {
            memcpy(data.data() + offset, &value, 4);
        }

        uint32_t SlotHash(int slot)
        {
            return bStringHash(std::format("OPTION_{}", slot).c_str());
        }

        // The records of an object's data chunk: calls f(offset of the record's value, tag, size)
        template<typename F>
        void ForObjectRecords(std::vector<uint8_t>& data, size_t object, F&& f)
        {
            size_t end = object + 8 + Read(data, object + 4);
            for (size_t p = object + 8; p + 8 <= end; p += 8 + Read(data, p + 4))
            {
                if (Read(data, p) != TagObjectData)
                    continue;
                size_t recordsEnd = std::min(end, p + 8 + Read(data, p + 4));
                for (size_t q = p + 8; q + 4 <= recordsEnd;)
                {
                    uint16_t tag, size;
                    memcpy(&tag, &data[q], 2);
                    memcpy(&size, &data[q + 2], 2);
                    if (q + 4 + size > recordsEnd)
                        break;
                    f(q + 4, tag, size);
                    q += 4 + size;
                }
            }
        }

        // The package with one more slot than it has, or empty when it isn't laid out as expected
        std::vector<uint8_t> AddSlot(const uint8_t* source)
        {
            uint32_t rootSize;
            memcpy(&rootSize, source + 4, 4);
            if (*reinterpret_cast<const uint32_t*>(source) != TagRoot || rootSize > 0x400000)
                return {};
            std::vector<uint8_t> data(source, source + 8 + rootSize);

            size_t objects = 0;
            for (size_t p = 8; p + 8 <= data.size(); p += 8 + Read(data, p + 4))
            {
                if (Read(data, p) == TagObjects)
                    objects = p;
            }
            if (!objects)
                return {};
            size_t objectsEnd = objects + 8 + Read(data, objects + 4);
            if (objectsEnd > data.size())
                return {};

            // The objects: the button count, the last slot and the highest GUID
            size_t buttons = 0, last = 0, lastSize = 0;
            uint32_t lastGuid = 0, lastFlags = 0, maxGuid = 0;
            int lastSlot = 0;
            for (size_t p = objects + 8; p + 8 <= objectsEnd; p += 8 + Read(data, p + 4))
            {
                if (Read(data, p) == TagButtons)
                    buttons = p + 8;
                if (Read(data, p) != TagObject)
                    continue;
                ForObjectRecords(data, p, [&](size_t value, uint16_t tag, uint16_t size)
                {
                    if (tag != RecordObject || size < 12)
                        return;
                    uint32_t guid = Read(data, value), hash = Read(data, value + 4), flags = Read(data, value + 8);
                    maxGuid = std::max(maxGuid, guid);
                    for (int slot = lastSlot + 1; slot <= 32; ++slot)
                    {
                        if (hash == SlotHash(slot))
                        {
                            lastSlot = slot;
                            last = p;
                            lastSize = 8 + Read(data, p + 4);
                            lastGuid = guid;
                            lastFlags = flags;
                        }
                    }
                });
            }
            if (!buttons || !last || last + lastSize > objectsEnd)
                return {};

            // The copy, under the next slot's name and a new GUID, right after the original
            std::vector<uint8_t> copy(data.begin() + last, data.begin() + last + lastSize);
            uint32_t newGuid = maxGuid + 1, newHash = SlotHash(lastSlot + 1);
            ForObjectRecords(copy, 0, [&](size_t value, uint16_t tag, uint16_t size)
            {
                if (tag == RecordObject && size >= 8)
                {
                    Write(copy, value, newGuid);
                    Write(copy, value + 4, newHash);
                }
                else if (tag == RecordName && size >= 4)
                    Write(copy, value, newHash);
            });
            data.insert(data.begin() + last + lastSize, copy.begin(), copy.end());
            Write(data, 4, Read(data, 4) + uint32_t(copy.size()));
            Write(data, objects + 4, Read(data, objects + 4) + uint32_t(copy.size()));
            if (lastFlags & ButtonFlag)
                Write(data, buttons, Read(data, buttons) + 1);

            // The messages the original receives go to the copy too
            for (size_t p = 8; p + 8 <= data.size(); p += 8 + Read(data, p + 4))
            {
                if (Read(data, p) != TagTargets)
                    continue;
                size_t q = p + 8;
                while (q + 4 <= p + 8 + Read(data, p + 4))
                {
                    uint16_t tag, size;
                    memcpy(&tag, &data[q], 2);
                    memcpy(&size, &data[q + 2], 2);
                    bool add = false;
                    if (tag == RecordTargets)
                    {
                        for (size_t g = q + 8; g + 4 <= q + 4 + size; g += 4)
                            add |= Read(data, g) == lastGuid;
                    }
                    if (add && size <= 0xFFFB)
                    {
                        size_t end = q + 4 + size;
                        uint8_t guid[4];
                        memcpy(guid, &newGuid, 4);
                        data.insert(data.begin() + end, guid, guid + 4);
                        size += 4;
                        memcpy(&data[q + 2], &size, 2);
                        Write(data, p + 4, Read(data, p + 4) + 4);
                        Write(data, 4, Read(data, 4) + 4);
                    }
                    q += 4 + size;
                }
                break;
            }
            return data;
        }

        // The game may keep pointers into a package's data, so the changed copies are never freed
        const uint8_t* Replace(const uint8_t* source)
        {
            static std::deque<std::pair<std::vector<uint8_t>, std::vector<uint8_t>>> copies;    // source, changed
            uint32_t rootSize;
            memcpy(&rootSize, source + 4, 4);
            for (auto& [original, changed] : copies)
            {
                if (original.size() == 8 + size_t(rootSize) && memcmp(original.data(), source, original.size()) == 0)
                    return changed.data();
            }

            auto changed = AddSlot(source);
            if (changed.empty())
                return source;
            copies.emplace_back(std::vector<uint8_t>(source, source + 8 + rootSize), std::move(changed));
            return copies.back().second.data();
        }
    }

    // DALOptions: the values of the rows
    using GetIntFn = bool(__fastcall*)(void*, void*, int, int*, int, int, int);
    using SetIntFn = bool(__fastcall*)(void*, void*, int, int, int, int, int);
    using GetFloatFn = bool(__fastcall*)(void*, void*, int, float*, int, int, int);
    using SetFloatFn = bool(__fastcall*)(void*, void*, int, float, int, int, int);
    using ScrollFn = bool(__fastcall*)(void*, void*, int, int, int, int, int);
    using GetDefaultIntFn = bool(__fastcall*)(void*, void*, int, int*, int, int, int);
    using GetDefaultFloatFn = bool(__fastcall*)(void*, void*, int, float*, int, int, int);
    ScrollFn OriginalScroll = nullptr;
    GetDefaultIntFn OriginalGetDefaultInt = nullptr;
    GetDefaultFloatFn OriginalGetDefaultFloat = nullptr;
    GetIntFn OriginalGetInt = nullptr;
    SetIntFn OriginalSetInt = nullptr;
    GetFloatFn OriginalGetFloat = nullptr;
    SetFloatFn OriginalSetFloat = nullptr;

    bool IsOwnId(int id)
    {
        return id >= PluginRowId && id < FirstGameScreenId + int(std::size(Placements));
    }

    MenuRow* FindRow(int id)
    {
        auto it = Rows.find(id);
        return it != Rows.end() ? &it->second : nullptr;
    }

    // The widget's value of a row: a switch's 0 or 1, a choice's position among its values
    int WidgetValue(const MenuRow& row)
    {
        switch (row.item.kind)
        {
            case WFPPlugins::Row::Kind::Preset:
                return PresetIndex();
            case WFPPlugins::Row::Kind::RestoreDefaults:
                return Restore.confirm ? 1 : 0;
            case WFPPlugins::Row::Kind::Setting:
                break;
            default:
                return 0;
        }
        switch (row.shape)
        {
            case Shape::Toggle:
            {
                int b = 0;
                return asi_settings::detail::ParseBool(CachedValue(row.item), b) && b ? 1 : 0;
            }
            case Shape::Choice:
                return ChoiceIndex(row);
            default:
                return 0;
        }
    }

    bool SetValue(int id, MenuRow& row, const std::string& value)
    {
        Queue(CommandKind::Value, id, value);
        return true;
    }

    bool __fastcall GetInt(void* dal, void* edx, int id, int* value, int a, int b, int c)
    {
        if (IsOwnId(id))
        {
            auto row = FindRow(id);
            *value = row ? WidgetValue(*row) : 0;
            return true;
        }
        return OriginalGetInt(dal, edx, id, value, a, b, c);
    }

    bool __fastcall SetInt(void* dal, void* edx, int id, int value, int a, int b, int c)
    {
        if (!IsOwnId(id))
            return OriginalSetInt(dal, edx, id, value, a, b, c);
        auto row = FindRow(id);
        if (!row || row->item.kind != WFPPlugins::Row::Kind::Setting)
            return true;
        if (row->shape == Shape::Toggle)
            SetValue(id, *row, value != 0 ? "1" : "0");
        else if (row->shape == Shape::Choice && value >= 0 && value < int(row->values.size()))
            SetValue(id, *row, row->values[value]);
        return true;
    }

    bool __fastcall GetFloat(void* dal, void* edx, int id, float* value, int a, int b, int c)
    {
        if (IsOwnId(id))
        {
            auto row = FindRow(id);
            *value = row && row->shape == Shape::Slider ? float(std::atof(CachedValue(row->item).c_str())) : 0.0f;
            return true;
        }
        return OriginalGetFloat(dal, edx, id, value, a, b, c);
    }

    bool __fastcall SetFloat(void* dal, void* edx, int id, float value, int a, int b, int c)
    {
        if (!IsOwnId(id))
            return OriginalSetFloat(dal, edx, id, value, a, b, c);
        auto row = FindRow(id);
        if (!row || row->shape != Shape::Slider || !std::isfinite(value))
            return true;
        auto& setting = *row->item.setting;
        double result = value;
        if (setting.step)
            result = std::round(result / *setting.step) * *setting.step;
        if (setting.min)
            result = std::max(result, *setting.min);
        if (setting.max)
            result = std::min(result, *setting.max);
        SetValue(id, *row, asi_settings::text::FormatNumber(result));
        return true;
    }

    // Left and right on a choice
    bool __fastcall Scroll(void* dal, void* edx, int id, int delta, int a, int b, int c)
    {
        if (!IsOwnId(id))
            return OriginalScroll(dal, edx, id, delta, a, b, c);
        if (!delta)
            return true;
        if (id == PageRowId)
        {
            auto count = CurrentPlugin < 0 ? RootPageCount() : Screens.size();
            if (!count)
                return true;
            auto& page = CurrentPlugin < 0 ? CurrentRootPage : CurrentScreen;
            page = (page + count + (delta > 0 ? 1 : count - 1)) % count;
            if (CurrentPlugin < 0)
                ReturnPlugin = page * RootCapacity();
            RestoreSetting.clear();
            UpdatePluginTexts();
            RequestRebuild();
            return true;
        }
        auto row = FindRow(id);
        if (!row)
            return true;
        if (row->item.kind == WFPPlugins::Row::Kind::Preset)
        {
            CancelConfirmations(id);
            Queue(CommandKind::CyclePreset, id, {}, delta);
            return true;
        }
        if (row->item.kind == WFPPlugins::Row::Kind::Setting && (row->shape == Shape::Choice || row->shape == Shape::ValueText))
            Queue(CommandKind::Step, id, {}, delta);
        return true;
    }

    // The game's Restore Defaults on a screen: the rows' defaults; the plugin and page rows, the Preset and Restore
    // Defaults rows and the language keep theirs
    bool __fastcall GetDefaultInt(void* dal, void* edx, int id, int* value, int a, int b, int c)
    {
        if (!IsOwnId(id))
            return OriginalGetDefaultInt(dal, edx, id, value, a, b, c);
        auto row = FindRow(id);
        if (!row || row->item.kind != WFPPlugins::Row::Kind::Setting || asi_settings::text::IEquals(row->item.setting->section, "LANGUAGE"))
            return GetInt(dal, edx, id, value, a, b, c);
        auto& setting = *row->item.setting;
        auto defaultValue = asi_settings::detail::JsonToText(setting.defaultValue);
        switch (row->shape)
        {
            case Shape::Toggle:
                *value = defaultValue != "0" ? 1 : 0;
                break;
            case Shape::Choice:
            {
                *value = 0;
                for (size_t i = 0; i < row->values.size(); ++i)
                {
                    if (SameValue(setting, row->values[i], defaultValue))
                        *value = int(i);
                }
                break;
            }
            default:
                // Shown as value 0, so the default is set here
                *value = 0;
                break;
        }
        return true;
    }

    bool __fastcall GetDefaultFloat(void* dal, void* edx, int id, float* value, int a, int b, int c)
    {
        if (!IsOwnId(id))
            return OriginalGetDefaultFloat(dal, edx, id, value, a, b, c);
        auto row = FindRow(id);
        if (row && row->shape == Shape::Slider)
            *value = float(row->item.setting->defaultValue.number);
        else
            return GetFloat(dal, edx, id, value, a, b, c);
        return true;
    }
    void ApplyCommand(Command command)
    {
        auto pin = command.snapshot;
        auto active = [&]()
        {
            return command.kind == CommandKind::Defaults ? ScreenOwner.Matches(command.lifetime) :
                Current(command.token) && Plugins == pin;
        };
        if (!pin || !active())
            return;
        if (command.kind == CommandKind::Defaults)
        {
            auto token = RowsOwner.Capture();
            auto current = [&]() { return active() && RowsOwner.Capture().generation == token.generation; };
            for (const auto& row : command.defaults)
            {
                if (!current())
                    return;
                WFPPlugins::Set(*row.plugin, *row.setting, asi_settings::detail::JsonToText(row.setting->defaultValue), current);
            }
            RestartChange |= WFPPlugins::RestartNeeded();
            RefreshNeeded = true;
            return;
        }
        auto found = Rows.find(command.id);
        if (found == Rows.end())
            return;
        auto row = found->second;
        if (row.rootPlugin >= 0)
        {
            if (command.kind == CommandKind::Accept && size_t(row.rootPlugin) < pin->plugins.size())
            {
                CurrentPlugin = row.rootPlugin;
                ReturnPlugin = size_t(CurrentPlugin);
                CurrentScreen = 0;
                RestoreSetting.clear();
                RequestRebuild();
            }
            return;
        }
        if (!WFPPlugins::Visible(row.item) || !active() || !WFPPlugins::Enabled(row.item) || !active())
            return;
        CancelConfirmations(command.id);
        if (!active())
            return;
        if (row.item.kind != WFPPlugins::Row::Kind::Preset)
            KeepPresetChoice = false;
        switch (command.kind)
        {
            case CommandKind::Value:
                if (row.item.kind == WFPPlugins::Row::Kind::Setting)
                    WFPPlugins::Set(*row.item.plugin, *row.item.setting, command.value, active);
                break;
            case CommandKind::Step:
                WFPPlugins::Step(row.item, command.direction, active);
                break;
            case CommandKind::CyclePreset:
                Preset.Cycle(command.direction);
                KeepPresetChoice = true;
                break;
            case CommandKind::Accept:
                if (row.item.kind == WFPPlugins::Row::Kind::Preset)
                {
                    auto preset = Preset;
                    bool apply = preset.confirm;
                    preset.Accept(active);
                    if (!active())
                        return;
                    Preset = std::move(preset);
                    KeepPresetChoice = !apply;
                }
                else if (row.item.kind == WFPPlugins::Row::Kind::RestoreDefaults)
                {
                    if (!Restore.confirm)
                        Restore.confirm = true;
                    else
                    {
                        Restore.confirm = false;
                        auto items = CurrentScreen < Screens.size() ? Screens[CurrentScreen].rows : std::vector<WFPPlugins::Row>{};
                        for (const auto& item : items)
                        {
                            if (!active())
                                return;
                            if (item.kind == WFPPlugins::Row::Kind::Setting)
                                WFPPlugins::Set(*item.plugin, *item.setting, asi_settings::detail::JsonToText(item.setting->defaultValue), active);
                        }
                    }
                }
                else if (row.item.kind == WFPPlugins::Row::Kind::Action)
                    OtherPlugins::Invoke(*row.item.plugin, *row.item.setting);
                break;
            default:
                break;
        }
        if (active())
        {
            if (command.kind != CommandKind::CyclePreset)
                RestartChange |= WFPPlugins::RestartNeeded();
            RefreshNeeded = true;
        }
    }

    void ServiceScreen()
    {
        if (NativeDepth || Servicing || !pOptionsScreen || !*pOptionsScreen ||
            ScreenOwner.Get() != reinterpret_cast<void*>(*pOptionsScreen))
            return;
        Servicing = true;
        struct Finish
        {
            ~Finish() { Servicing = false; }
        } finish;
        auto lifetime = ScreenOwner.Capture();
        UpdateTexts();
        if (DiscoverNeeded)
        {
            DiscoverNeeded = false;
            bool pluginsScreen = *pOptionsScreen == PluginsScreen;
            RetireRows(lifetime.screen);
            CachedSettings.clear();
            auto rowsGeneration = RowsOwner.Capture().generation;
            FindPlugins();
            if (!ScreenOwner.Matches(lifetime) || RowsOwner.Capture().generation != rowsGeneration)
                return;
            if (pluginsScreen)
                PendingRebuild = reinterpret_cast<uintptr_t>(lifetime.screen);
            else
                StockRebuildNeeded = true;
            RefreshNeeded = true;
        }
        while (!Commands.empty())
        {
            auto command = std::move(Commands.front());
            Commands.pop_front();
            ApplyCommand(std::move(command));
            if (!ScreenOwner.Matches(lifetime))
                return;
        }
        if (WFPPlugins::Changed().exchange(false))
            RefreshNeeded = true;
        if (RefreshNeeded)
        {
            if (!CacheSnapshot(lifetime))
                return;
            RefreshNeeded = false;
            if (std::exchange(SnapshotLayoutChanged, false))
            {
                if (*pOptionsScreen == PluginsScreen)
                    RequestRebuild();
                else
                    StockRebuildNeeded = true;
            }
            auto token = RowsOwner.Capture();
            if (Current(token))
            {
                RedrawRows();
                if (!Current(token))
                    return;
                if (VisibleRows() != BuiltVisibility)
                    RequestRebuild();
            }
        }
        if (StockRebuildNeeded && SetupManager && *pOptionsScreen != PluginsScreen && Plugins)
        {
            StockRebuildNeeded = false;
            auto screen = ScreenOwner.Get();
            auto page = PlacementPage();
            ClearRows(static_cast<uint8_t*>(screen) + 0x28, nullptr);
            if (!ScreenOwner.Matches(lifetime))
                return;
            if (page == 0)
                BuildVideoBasic(SetupManager, nullptr);
            else if (page == 1)
                BuildVideoAdvanced(SetupManager, nullptr);
            else
                ScreenSetup(SetupManager, nullptr);
        }
        if (RetryBuildAt && GetTickCount64() >= RetryBuildAt && ScreenOwner.Matches(lifetime))
        {
            RetryBuildAt = 0;
            RequestRebuild();
        }
        if (PendingRebuild && ScreenOwner.Matches(lifetime))
        {
            RebuildScreen();
        }
        auto token = RowsOwner.Capture();
        ValueTick = GetTickCount64();
        auto valuePage = ValueTick / 12000;
        bool pageChanged = ValueFramePage != valuePage;
        ValueFramePage = valuePage;
        bool retryFonts = !PendingFonts.empty() && ValueTick >= FontRetryAt;
        if (retryFonts)
            FontRetryAt = ValueTick + 250;
        if (pageChanged || retryFonts)
        {
            std::unordered_set<void*> redraw;
            if (pageChanged)
                redraw.insert(ContinuedValues.begin(), ContinuedValues.end());
            if (retryFonts)
                redraw.insert(PendingFonts.begin(), PendingFonts.end());
            for (auto widget : redraw)
            {
                if (!Current(token))
                    return;
                RedrawWidget(widget);
            }
        }
        ValueLayout.RestoreArrows();
        if (Current(token))
        {
            auto selected = *reinterpret_cast<void**>(static_cast<uint8_t*>(token.screen) + 0x34);
            auto found = std::find_if(Rows.begin(), Rows.end(), [&](const auto& entry) { return entry.second.option == selected; });
            if (found != Rows.end())
                LayoutRowValue(found->second, token);
            else if (selected && selected == Pager.option)
                LayoutRowValue(Pager, token);
        }
        RefreshTitle(token);
        RefreshHelp(token);
        if (Current(token))
            RestartNotice(token);
    }

}

class Options
{
public:
    Options()
    {
        WFP::onInitEvent() += []()
        {
            GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCWSTR>(&FindPlugins), &Self);

            auto fontLookup = hook::pattern("8B 54 24 04 56 8B 35 ? ? ? ? EB 03 8D 49 00 8B C6 3D ? ? ? ? 74 15");
            auto glyphAdvance = hook::pattern("51 8A 54 24 10 F6 C2 20 8B 44 24 08 C7 04 24 00 00 00 00 75 0C 66 3D 0A 00");
            // Resolve through the caller because Resolution already hooks the wide-text function.
            auto localizedWide = hook::pattern("E8 ? ? ? ? 83 C4 ? 84 C0 74 ? 8D 54 24 ? 89 54 24");
            auto stringHash = hook::pattern("56 8B 74 24 08 85 F6 74 30 8B 4E 1C 8B 44 24 0C 81 C9 00 00 40 00");
            if (fontLookup.size() == 1 && glyphAdvance.size() == 1 && localizedWide.size() == 1 && stringHash.size() == 1)
            {
                FindFont = reinterpret_cast<decltype(FindFont)>(fontLookup.get_first());
                GlyphAdvance = reinterpret_cast<decltype(GlyphAdvance)>(glyphAdvance.get_first());
                LocalizedWide = reinterpret_cast<decltype(LocalizedWide)>(injector::GetBranchDestination(localizedWide.get_first()).as_int());
                SetLanguageHash = reinterpret_cast<decltype(SetLanguageHash)>(stringHash.get_first());
                auto localizer = reinterpret_cast<uint8_t*>(LocalizedWide);
                if (localizer[0x3A] == 0x8B && localizer[0x3B] == 0x0D)
                    ActiveCharset = *reinterpret_cast<void***>(localizer + 0x3C);
                if (localizer[0x61] == 0xE8)
                {
                    auto converter = reinterpret_cast<uint8_t*>(injector::GetBranchDestination(localizer + 0x61).as_int());
                    if (converter[0x26] == 0xA1)
                        ActiveCharsetMode = *reinterpret_cast<uint32_t**>(converter + 0x27);
                }
            }

            // FeOptionsScreen::AddDalOption and the widgets it creates
            auto pattern = hook::pattern("6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 51 53 56 57 8B F9 8B 9F 64 01 00 00 43");
            if (pattern.size() != 1)
                return;
            auto addDalOption = reinterpret_cast<uintptr_t>(pattern.get_first());
            HashString = reinterpret_cast<decltype(HashString)>(injector::GetBranchDestination(addDalOption + 0x30).as_int());
            GameMalloc = reinterpret_cast<decltype(GameMalloc)>(injector::GetBranchDestination(addDalOption + 0x6B).as_int());

            auto toggle = hook::pattern("8D 4C 24 2C 56 51 8B C8 E8 ? ? ? ? EB 1F");
            auto choice = hook::pattern("68 39 DA DF 70 68 CB 13 00 00 8D 4C 24 38 51 8B C8 C7 44 24 3C ? ? ? ? E8");
            auto slider = hook::pattern("8B 54 24 20 8B 46 78 52 8B 54 24 30 52 8B 54 24 30 52 8B 54 24 30 8D 4E 78 52 FF 50 10");
            auto add = hook::pattern("50 53 8B CF E8 ? ? ? ? 8B 4C 24 10");
            if (toggle.size() != 1 || choice.size() != 1 || slider.size() != 1 || add.size() != 1)
                return;
            ToggleWidget = reinterpret_cast<decltype(ToggleWidget)>(injector::GetBranchDestination(toggle.get_first(8)).as_int());
            ChoiceWidget = reinterpret_cast<decltype(ChoiceWidget)>(injector::GetBranchDestination(choice.get_first(25)).as_int());
            SliderWidget = reinterpret_cast<decltype(SliderWidget)>(slider.get_first(-0x5E));
            AddWidget = reinterpret_cast<decltype(AddWidget)>(injector::GetBranchDestination(add.get_first(4)).as_int());
            auto publish = reinterpret_cast<uint8_t*>(AddWidget);
            if (publish[0x3F] != 0xE8 || publish[0x53] != 0xE8)
                return;
            WidgetOptionCtor = reinterpret_cast<decltype(WidgetOptionCtor)>(injector::GetBranchDestination(publish + 0x3F).as_int());
            InsertOption = reinterpret_cast<decltype(InsertOption)>(injector::GetBranchDestination(publish + 0x53).as_int());

            // DALOptions' vtable
            pattern = hook::pattern("8B 4C 24 04 89 48 04 C7 00 ? ? ? ? A3 ? ? ? ? C2 04 00 CC CC CC CC CC CC CC CC CC 56 8B F1");
            if (pattern.size() != 1)
                return;
            auto vtable = *pattern.get_first<uintptr_t*>(9);

            // The language tables' search and the languages
            auto search = hook::pattern("85 DB 75 03 33 C0 C3 55 56 8B F1 57 33 FF 4E");
            auto languages = hook::pattern("B9 ? ? ? ? EB 03 8D 49 00 39 11 74 0F 83 C1 1C 40 81 F9");
            if (search.size() != 1 || languages.size() != 1)
                return;
            Languages = *languages.get_first<LanguageInfo*>(1);
            CurrentLanguage = *languages.get_first<int32_t*>(0x14);
            LanguagesEnd = reinterpret_cast<LanguageInfo*>(CurrentLanguage);

            // The options menu's categories (FEOptionsStateManager's screen setup, state 13): the Plugins category
            // goes after the game's controls (state 6), before the ones the pause menu leaves out (cmp edi, 1)
            auto categories = hook::pattern("6A 06 8B CE E8 ? ? ? ? 83 FF 01 74");
            // State 1's screen: the title is pushed, then the empty list is finished; esi = the screen
            auto screen = hook::pattern("8B 35 ? ? ? ? 8B 56 0C 68 B6 AF 7F 56 68 6D 57 1B B7 52 E8 ? ? ? ? 8B 06 83 C4 0C 8B CE FF 50 1C");
            // The text option of a category and the menu's AddOption, in the function adding a category
            auto category = hook::pattern("C6 44 24 ? 01 E8 ? ? ? ? EB 02 33 C0 50 8B CD C7 44 24 ? FF FF FF FF E8");
            // The FE package reader (data at [esp+4]), for the pause menu's sixth category slot
            auto packageLoad = hook::pattern("8A 44 24 18 8A 54 24 14 53 56 8B F1 8A 4C 24 18 88 4E 09");
            if (categories.size() != 1 || screen.size() != 1 || category.size() != 1 || packageLoad.size() != 1)
                return;     // the settings stay in the .cfg
            TextOption = reinterpret_cast<decltype(TextOption)>(injector::GetBranchDestination(category.get_first(5)).as_int());
            AddMenuOption = reinterpret_cast<decltype(AddMenuOption)>(injector::GetBranchDestination(category.get_first(0x19)).as_int());
            pOptionsScreen = *screen.get_first<uintptr_t*>(2);
            SetHeaderHash = reinterpret_cast<decltype(SetHeaderHash)>(injector::GetBranchDestination(screen.get_first(0x14)).as_int());
            HeaderObjectHash = *screen.get_first<uint32_t>(15);
            auto header = reinterpret_cast<uint8_t*>(SetHeaderHash);
            if (header[0x20] == 0x8B && header[0x21] == 0x0D && header[0x27] == 0xE8)
            {
                FrontendInstance = *reinterpret_cast<void***>(header + 0x22);
                FindPackage = reinterpret_cast<decltype(FindPackage)>(injector::GetBranchDestination(header + 0x27).as_int());
            }
            auto message = hook::pattern("6A 00 6A 00 68 01 26 7B 41 E8 ? ? ? ? 83 C4 04 50 8B 44 24 10 50 E8");
            auto dialogConfig = hook::pattern("8B 74 24 10 56 A3 ? ? ? ? E8 ? ? ? ? 8D 44 00 02 50 E8 ? ? ? ? 8B 0D");
            auto dialog = hook::pattern("8B 0D ? ? ? ? 85 C9 74 05 E9 ? ? ? ? 8B 0D ? ? ? ? 56 68 ? ? ? ? 8B F1 E8");
            if (message.size() == 1 && dialogConfig.size() == 1 && dialog.size() == 1)
            {
                ShowMessage = reinterpret_cast<decltype(ShowMessage)>(message.get_first());
                DialogConfig = *dialogConfig.get_first<void**>(6);
                DialogInstance = *dialog.get_first<void**>(2);
            }


            auto ownerClass = hook::pattern("C7 06 ? ? ? ? 89 BE 64 01 00 00 89 BE 68 01 00 00");
            auto clearRows = hook::pattern("53 56 57 8B F9 8B 87 14 01 00 00 8B 00 33 DB 3B C3 74 05 8D 70 FC");
            auto screenSetup = hook::pattern("83 EC 08 57 8B F9 8B 47 04 83 F8 17 0F 87 ? ? ? ? 0F B6 80 ? ? ? ? 56 FF 24 85");
            auto nativeDefaults = hook::pattern("56 57 8B F9 8B 47 5C 8B 40 04 85 C0 74 05 8D 70 FC EB 02 33 F6 85 F6 74 0A");
            auto videoBasic = hook::pattern("53 55 56 8B 35 ? ? ? ? 8B 46 0C 57 68 B6 AF 7F 56 68 6D 57 1B B7 50 8B E9 E8");
            auto videoAdvanced = hook::pattern("53 55 56 8B 35 ? ? ? ? 8B 46 0C 57 68 6B FD 3D 87 68 6D 57 1B B7 50 8B E9 E8");
            auto back = hook::pattern("51 56 8B F1 8B 46 04 83 F8 1B 0F 87 ? ? ? ? 0F B6 88 ? ? ? ? FF 24 8D");
            if (ownerClass.size() != 1 || clearRows.size() != 1 || screenSetup.size() != 1 || nativeDefaults.size() != 1 || videoBasic.size() != 1 || videoAdvanced.size() != 1 || back.size() != 1)
                return;
            auto ownerVtable = *ownerClass.get_first<void**>(2);
            auto deletingDestructor = static_cast<uint8_t*>(ownerVtable[0]);
            const uint8_t destructorPrefix[]{ 0x56, 0x8B, 0xF1, 0xE8 };
            if (std::memcmp(deletingDestructor, destructorPrefix, sizeof(destructorPrefix)) != 0)
                return;
            shDestroyScreen = safetyhook::create_inline(injector::GetBranchDestination(deletingDestructor + 3).as_int(), DestroyScreen);
            shClearRows = safetyhook::create_inline(clearRows.get_first(), ClearRows);
            shScreenSetup = safetyhook::create_inline(screenSetup.get_first(), ScreenSetup);
            shNativeDefaults = safetyhook::create_inline(nativeDefaults.get_first(), NativeDefaults);
            shVideoBasic = safetyhook::create_inline(videoBasic.get_first(), BuildVideoBasic);
            shVideoAdvanced = safetyhook::create_inline(videoAdvanced.get_first(), BuildVideoAdvanced);
            shBack = safetyhook::create_inline(back.get_first(), Back);
            static auto ConstructHook = safetyhook::create_mid(ownerClass.get_first(), [](SafetyHookContext& regs)
            {
                ConstructScreen(reinterpret_cast<void*>(regs.esi));
            });
            if (!shDestroyScreen || !shClearRows || !shScreenSetup || !shNativeDefaults || !shVideoBasic || !shVideoAdvanced || !shBack || !ConstructHook)
            {
                ConstructHook.reset();
                shBack.reset();
                shVideoAdvanced.reset();
                shVideoBasic.reset();
                shNativeDefaults.reset();
                shScreenSetup.reset();
                shClearRows.reset();
                shDestroyScreen.reset();
                return;
            }
            shAddDalOption = safetyhook::create_inline(addDalOption, AddDalOption);

            static auto PackageLoadHook = safetyhook::create_mid(packageLoad.get_first(), [](SafetyHookContext& regs)
            {
                auto& data = *reinterpret_cast<const uint8_t**>(regs.esp + 4);
                if (data && *reinterpret_cast<const uint32_t*>(data) == PackageSlot::TagRoot &&
                    *reinterpret_cast<const uint32_t*>(data + 8) == 0x64486B50 &&   // "PkHd", the name follows at 0x28
                    _stricmp(reinterpret_cast<const char*>(data + 0x28), "FePauseOptionsMain.fng") == 0)
                    data = PackageSlot::Replace(data);
            });

            static auto CategoryHook = safetyhook::create_mid(categories.get_first(9), [](SafetyHookContext& regs)
            {
                AddCategory(reinterpret_cast<void*>(regs.esi));
            });

            // Opening the Plugins screen finds the plugins again
            static auto ScreenHook = safetyhook::create_mid(screen.get_first(0x19), [](SafetyHookContext& regs)
            {
                PendingRebuild = 0;
                PluginsScreen = regs.esi;
                CurrentPlugin = -1;
                CurrentScreen = CurrentRootPage = ReturnPlugin = 0;
                RestoreSetting.clear();
                if (ScreenOwner.Get() == reinterpret_cast<void*>(regs.esi))
                {
                    RowsOwner.Bind(reinterpret_cast<void*>(regs.esi));
                    DiscoverNeeded = RefreshNeeded = true;
                    PendingRebuild = regs.esi;
                }
            });

            // Every string lookup by hash ends in this search of the language tables (edx = hash), which returns
            // through its "not found" exit (ret) with the fix's text instead
            static auto returnAddress = reinterpret_cast<uintptr_t>(search.get_first(6));
            static auto shSearchString = safetyhook::create_mid(search.get_first(), [](SafetyHookContext& regs)
            {
                if (auto it = Texts.find(regs.edx); it != Texts.end())
                {
                    regs.eax = reinterpret_cast<uintptr_t>(it->second.c_str());
                    regs.eip = returnAddress;
                }
            });

            // Without any of them the menu is left out completely
            if (!shAddDalOption || !PackageLoadHook || !CategoryHook || !ScreenHook || !shSearchString)
            {
                shSearchString.reset();
                ScreenHook.reset();
                CategoryHook.reset();
                PackageLoadHook.reset();
                shAddDalOption.reset();
                ConstructHook.reset();
                shBack.reset();
                shVideoAdvanced.reset();
                shVideoBasic.reset();
                shNativeDefaults.reset();
                shScreenSetup.reset();
                shClearRows.reset();
                shDestroyScreen.reset();
                return;
            }

            static ProtectedGameRef<uint32_t> ScreenTitle;
            ScreenTitle.SetAddress(screen.get_first<uint32_t>(10));
            ScreenTitle = TextHash("WFP_SCREEN_TITLE");

            WFP::onGameProcessEvent() += []()
            {
                ServiceScreen();
            };

            auto hookSlot = [vtable](int slot, auto replacement, auto& original)
            {
                ProtectedGameRef<uintptr_t> ref(&vtable[slot]);
                original = reinterpret_cast<std::remove_reference_t<decltype(original)>>(vtable[slot]);
                ref = reinterpret_cast<uintptr_t>(replacement);
            };
            hookSlot(2, &GetFloat, OriginalGetFloat);
            hookSlot(3, &SetFloat, OriginalSetFloat);
            hookSlot(6, &GetInt, OriginalGetInt);
            hookSlot(7, &SetInt, OriginalSetInt);
            hookSlot(8, &Scroll, OriginalScroll);
            hookSlot(12, &GetDefaultInt, OriginalGetDefaultInt);
            hookSlot(13, &GetDefaultFloat, OriginalGetDefaultFloat);
        };
    }
} Options;
