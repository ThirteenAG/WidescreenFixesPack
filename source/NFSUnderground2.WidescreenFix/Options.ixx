module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "StaticText.hpp"
#include "../Shared/NFS/LegacyText.hpp"
#include "../Shared/NFS/TextLanguage.hpp"
#include "../Shared/NFS/PluginsMenu.hpp"
#include "../Shared/NFS/MenuLayout.hpp"
#include "../Shared/NFS/Text.hpp"

export module Options;

import ComVars;

// The fix adds one category to the options menu and to the pause menu's options: "Plugins". It opens the game's options
// screen (UI_Options.fng, in the pause menu UI_PauseOptions.fng) with the list of plugins with settings, the fix first;
// accepting one shows its page on the same screen, back returns to the list, including when the fix is the only plugin.
// A page starts with a row that names the plugin and the part of its page shown; left and right on it go through the
// parts. The game's screens have twelve rows with arrows (the pause screen eleven) and scroll eight of them, so every
// part of a page fits into the rows below the first. The parts are the description's pages (settings.json), the
// Preset row first, Restore Defaults last (see Shared/NFS/PluginsMenu.hpp).
// Every option of the game is an object of its own class, so the fix's rows are objects of the game's toggle class with
// a class of their own: left, right and accept change the row's setting, and the row shows its name and value. The
// rows of a screen are made once; showing other rows relinks them into the screen's list of options.
// The display options (UI_Options and UI_PauseOptions of the "Display" category) get the window mode under the
// resolution and the field of view, HUD aspect ratio and console gamma under the level of detail; the game's
// "Default Settings" there resets them too.

namespace
{
    // Options types (dword_83AA04): 0 audio, 1 video, 2 gameplay, 3 player, 4 controller, 5 EA Trax, 6 trailers,
    // 7 the options of an online lobby, 8 credits. The fix's category is a type of its own.
    constexpr int32_t VideoOptionsType = 1;
    constexpr int32_t FixOptionsType = 9;
    int32_t* OptionsType = nullptr;

    constexpr uint32_t MessageLeft = 0x9120409E;
    constexpr uint32_t MessageRight = 0xB5971BF1;
    constexpr uint32_t MessageAccept = 0x0C407210;
    constexpr uint32_t MessageBack = 0x911AB364;
    constexpr uint32_t MessageDialogYes = 0xD05FC3A3;      // "Default Settings" confirmed

    // Text: every text lookup by hash ends in the game's search, the fix's texts are answered by their hashes
    std::unordered_map<uint32_t, std::string> Texts;     // hash -> text in the font's encoding

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
        const char* file;
        void* charset;
    };
    LanguageInfo* Languages = nullptr;
    LanguageInfo* LanguagesEnd = nullptr;
    std::string LanguageName = "english";

    std::string FindLanguageName(int32_t id)
    {
        for (auto info = Languages; info && info < LanguagesEnd; ++info)
        {
            if (info->id == id && info->name)
            {
                std::string name = info->name;
                for (auto& c : name)
                    c = char(tolower(uint8_t(c)));
                return name;
            }
        }
        return "english";
    }

    // The language of the menus' texts: the fix's choice, otherwise the game's. The game's fonts have no Cyrillic
    // letters, so Russian texts are shown in English.
    void UpdateLanguage()
    {
        auto language = WFPMenu::TextLanguage(FixSettings(), FindLanguageName(CurrentLanguage ? *CurrentLanguage : 0));
        if (language == "russian")
            language = "english";
        if (language == LanguageName && !Texts.empty())
            return;
        LanguageName = language;
        WFPText::SetLanguage(LanguageName);
        Texts[TextHash("WFP_PLUGINS")] = WFPLegacyText::Encode(WFPText::Get("WFP_PLUGINS"));
    }

    std::string LanguageTag()
    {
        auto tag = WFPText::Detail::Tag(LanguageName);
        return tag ? asi_settings::text::Narrow(tag) : "en";
    }

    std::string FontText(const std::string& text)
    {
        return WFPLegacyText::Encode(text);
    }

    // The game's functions
    void* (__cdecl* GameMalloc)(size_t size) = nullptr;
    void* (__fastcall* ToggleOption)(void* option, void* edx, void* a, void* b, void* c, void* d, int enabled) = nullptr;
    void (__fastcall* AddToggleOption)(void* screen, void* edx, void* option, int arrows) = nullptr;
    void (__fastcall* SelectOption)(void* screen, void* edx, int index) = nullptr;     // index from 1, 0: the first
    void (__cdecl* SetLanguageHash)(void* object, uint32_t hash) = nullptr;
    void* (__cdecl* FindString)(const char* package, uint32_t hash) = nullptr;     // an FEString of a package
    void (__cdecl* HideObject)(void* object) = nullptr;
    void (__cdecl* ShowObject)(void* object) = nullptr;

    // The screens (UIWidgetMenu): their list of options and the state of the selection
    namespace Screen
    {
        constexpr uintptr_t Package = 0x04;
        constexpr uintptr_t ListHead = 0x4C;        // the options' links (next, previous), the screen's own is the end
        constexpr uintptr_t ListTail = 0x50;
        constexpr uintptr_t Current = 0x54;         // the highlighted option
        constexpr uintptr_t FirstShown = 0x58;      // the first option of the rows shown
        constexpr uintptr_t Count = 0x12C;          // options + 1
        constexpr uintptr_t Selected = 0x145;       // an option was picked

        template<typename T> T& At(void* screen, uintptr_t offset) { return *reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(screen) + offset); }
        const char* PackageName(void* screen) { return At<const char*>(screen, Package); }
    }

    // A row of the fix's: the list's plugin, the first row of a page, or a row of a plugin's page
    struct Entry
    {
        enum class Kind { Empty, RootPage, Plugin, Title, Row } kind = Kind::Empty;
        size_t plugin = 0;
        WFPPlugins::Row row;
        uint32_t labelHash = 0;
        uint32_t valueHash = 0;
    };

    constexpr size_t OptionSize = 0x54;
    struct Option
    {
        void** vtable;
        uint8_t data[OptionSize - 4];
        Entry* entry;   // the fix's, after the game's part
        float dataX{};
        float dataWidth{};
        bool metricsSaved{};

        template<typename T> T& At(uintptr_t offset) { return *reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(this) + offset); }
        void* Title() { return At<void*>(0x2C); }
        void* Value() { return At<void*>(0x30); }
        void* LeftArrow() { return At<void*>(0x4C); }
        void* RightArrow() { return At<void*>(0x50); }
        uintptr_t Link() { return reinterpret_cast<uintptr_t>(this) + 4; }
    };

    // The option's methods (UIWidget): 1 act, 3 draw, 5 show, 6 hide, 10 unhighlight, 13 move vertically
    template<typename R = void, typename... Args>
    R CallMethod(void* object, size_t slot, Args... args)
    {
        auto vtable = *reinterpret_cast<void***>(object);
        return reinterpret_cast<R(__fastcall*)(void*, void*, Args...)>(vtable[slot])(object, nullptr, args...);
    }

    Option* FromLink(uintptr_t link)
    {
        return link ? reinterpret_cast<Option*>(link - 4) : nullptr;
    }

    // The screen's options in their order
    std::vector<Option*> ListOptions(void* screen)
    {
        std::vector<Option*> options;
        auto end = reinterpret_cast<uintptr_t>(screen) + Screen::ListHead;
        for (auto link = Screen::At<uintptr_t>(screen, Screen::ListHead); link && link != end; link = *reinterpret_cast<uintptr_t*>(link))
            options.push_back(FromLink(link));
        return options;
    }

    // The screen's list of options made of these, in this order; the others are moved away and hidden
    void Relink(void* screen, const std::vector<Option*>& options, const std::vector<Option*>& hidden = {})
    {
        auto end = reinterpret_cast<uintptr_t>(screen) + Screen::ListHead;
        auto tail = end;
        Screen::At<uintptr_t>(screen, Screen::ListHead) = end;
        Screen::At<uintptr_t>(screen, Screen::ListTail) = end;
        for (auto option : options)
        {
            auto link = option->Link();
            *reinterpret_cast<uintptr_t*>(tail) = link;
            Screen::At<uintptr_t>(screen, Screen::ListTail) = link;
            reinterpret_cast<uintptr_t*>(link)[0] = end;
            reinterpret_cast<uintptr_t*>(link)[1] = tail;
            tail = link;
        }
        Screen::At<int32_t>(screen, Screen::Count) = int32_t(options.size()) + 1;
        Screen::At<Option*>(screen, Screen::FirstShown) = options.empty() ? nullptr : options.front();
        for (auto option : hidden)
        {
            CallMethod(option, 13, 6000.0f);
            CallMethod(option, 6);
        }
    }

    // The fix's rows of the screen being shown
    struct FixScreen
    {
        void* screen = nullptr;
        size_t rowsWithArrows = 0;
        std::vector<Option*> options;   // made once per screen, all with arrows
        std::vector<Entry> entries;     // one per option
        bool video = false;             // the display options' rows
        bool pause = false;             // UI_PauseOptions.fng
    };
    FixScreen Active;

    struct HelpState
    {
        std::vector<uint32_t> ids;
        float width{};
        std::string text;
        std::vector<std::string> lines;
        uint64_t started{};
        size_t page{ std::numeric_limits<size_t>::max() };
        std::string warning;
        std::vector<std::string> warningLines;
        uint64_t warningStarted{};
    };
    HelpState MainHelp, PauseHelp;
    HelpState& CurrentHelp()
    {
        return Active.pause ? PauseHelp : MainHelp;
    }
    uint8_t* (__cdecl* FindPackage)(const char* name) = nullptr;
    uint8_t* (__thiscall* FindPackageObject)(void* package, uint32_t hash) = nullptr;

    // The plugins and the page shown
    std::shared_ptr<WFPPlugins::Snapshot> Plugins;
    enum class Level { List, Page } CurrentLevel = Level::List;
    size_t CurrentPlugin = 0;
    size_t RootPage = 0;
    size_t CurrentPart = 0;
    WFPPlugins::PresetRow PresetRow;
    WFPPlugins::RestoreRow RestoreRow;

    // The settings the display options show, left out of the fix's page; empty when they can't be shown there
    std::vector<std::string> VideoSettings;
    std::vector<std::string> MainJoinedSettings;
    std::vector<std::string> PauseJoinedSettings;
    constexpr const char* VideoSettingIds[] = { "MAIN.WindowMode", "MAIN.FOVFactor", "MAIN.HudAspectRatioConstraint", "GRAPHICS.ConsoleGamma" };

    HMODULE Self()
    {
        HMODULE module = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCWSTR>(&Self), &module);
        return module;
    }

    bool IsFix(const WFPPlugins::LoadedPlugin& plugin)
    {
        return plugin.module == Self();
    }

    std::vector<std::string> Exclude(const WFPPlugins::LoadedPlugin& plugin)
    {
        return IsFix(plugin) ? (Active.pause ? PauseJoinedSettings : MainJoinedSettings) : std::vector<std::string>{};
    }

    // A part of a plugin's page: a description page, split when it has more rows than fit below the first row
    struct Part
    {
        std::string title;      // UTF-8
        std::vector<WFPPlugins::Row> rows;
    };

    std::vector<Part> Parts(const WFPPlugins::LoadedPlugin& plugin, size_t pageRows)
    {
        std::vector<Part> parts;
        for (auto& section : WFPPlugins::Sections(plugin, Exclude(plugin)))
        {
            size_t count = std::max<size_t>(1, (section.rows.size() + pageRows - 1) / pageRows);
            for (size_t i = 0; i < count; ++i)
            {
                Part part{ section.title, {} };
                if (count > 1)
                    part.title += " (" + std::to_string(i + 1) + "/" + std::to_string(count) + ")";
                auto begin = section.rows.begin() + std::min(section.rows.size(), i * pageRows);
                auto end = section.rows.begin() + std::min(section.rows.size(), (i + 1) * pageRows);
                part.rows.assign(begin, end);
                parts.push_back(std::move(part));
            }
        }
        return parts;
    }

    const WFPPlugins::LoadedPlugin* PluginOf(const Entry& entry)
    {
        if (entry.kind == Entry::Kind::Row)
            return entry.row.plugin;
        if (Plugins && entry.plugin < Plugins->plugins.size())
            return &Plugins->plugins[entry.plugin];
        return nullptr;
    }

    // The texts of a row
    std::string LabelText(const Entry& entry)
    {
        switch (entry.kind)
        {
            case Entry::Kind::RootPage:
                return WFPText::Get("WFP_PLUGINS");
            case Entry::Kind::Plugin:
            case Entry::Kind::Title:
                return PluginOf(entry) ? WFPPlugins::Name(*PluginOf(entry)) : std::string();
            case Entry::Kind::Row:
                return WFPPlugins::Label(entry.row);
            default:
                return {};
        }
    }

    std::string ValueText(const Entry& entry)
    {
        switch (entry.kind)
        {
            case Entry::Kind::RootPage:
            {
                auto slice = WFPMenuLayout::Root(Plugins ? Plugins->plugins.size() : 0, Active.options.size(), RootPage);
                return std::to_string(slice.page + 1) + "/" + std::to_string(slice.pages);
            }
            case Entry::Kind::Plugin:
            {
                // The preset in use, for plugins with presets
                auto plugin = PluginOf(entry);
                if (!plugin || !OtherPlugins::HasPresets(*plugin))
                    return {};
                int preset = OtherPlugins::CurrentPreset(*plugin);
                return WFPText::Get(preset < 0 ? "WFP_PRESET_CUSTOM" : WFPPlugins::PresetRow::Names[preset]);
            }
            case Entry::Kind::Title:
            {
                auto plugin = PluginOf(entry);
                auto parts = plugin ? Parts(*plugin, Active.rowsWithArrows - 1) : std::vector<Part>{};
                return CurrentPart < parts.size() ? parts[CurrentPart].title : std::string();
            }
            case Entry::Kind::Row:
                switch (entry.row.kind)
                {
                    case WFPPlugins::Row::Kind::Preset:
                        return PresetRow.ValueText();
                    case WFPPlugins::Row::Kind::RestoreDefaults:
                        return RestoreRow.ValueText();
                    default:
                        return WFPPlugins::ValueText(entry.row);
                }
            default:
                return {};
        }
    }

    // Left and right change the row: plugins in the list, actions and Restore Defaults have no arrows
    bool HasArrows(const Entry& entry)
    {
        if (entry.kind == Entry::Kind::Title || entry.kind == Entry::Kind::RootPage)
            return true;
        if (entry.kind != Entry::Kind::Row)
            return false;
        return entry.row.kind == WFPPlugins::Row::Kind::Preset || entry.row.kind == WFPPlugins::Row::Kind::Setting;
    }

    void SetText(void* object, uint32_t hash, const std::string& text)
    {
        if (!object)
            return;
        auto bytes = static_cast<uint8_t*>(object);
        auto previous = Texts.find(hash);
        bool changed = previous == Texts.end() || previous->second != text ||
            *reinterpret_cast<uint32_t*>(bytes + 0x60) != hash || (*reinterpret_cast<uint32_t*>(bytes + 0x1C) & 2);
        Texts[hash] = text;
        SetLanguageHash(object, hash);
        if (changed)
            *reinterpret_cast<uint32_t*>(bytes + 0x1C) |= 0x2400000;
    }

    float MeasureText(void* object, const std::string& text);
    std::string SupportedText(void* object, std::string text);

    // The restart warning in the native header
    constexpr uint32_t InfoLine = 0x7326E44C;      // "PLAYER"
    constexpr uint32_t PauseInfoLine = 0x98643DFE;  // "Header_text2"

    void ShowInfo()
    {
        if (!Active.screen)
            return;
        auto warning = WFPPlugins::RestartWarning();
        auto pause = Active.pause;
        // The display options keep the game's line until a change waits for a restart
        if (Active.video && warning.empty())
            return;
        if (auto line = FindString(Screen::PackageName(Active.screen), pause ? PauseInfoLine : InfoLine))
        {
            auto text = SupportedText(line, FontText(warning.empty() && pause ? WFPText::Get("WFP_PLUGINS") : warning));
            auto& state = CurrentHelp();
            auto now = GetTickCount64();
            if (!warning.empty() && std::isfinite(MeasureText(line, text)))
            {
                if (state.warning != text)
                {
                    state.warning = text;
                    state.warningStarted = now;
                    state.warningLines = WFPMenuLayout::Wrap(text, 260.0f, [&](const std::string& candidate)
                    {
                        return MeasureText(line, candidate);
                    });
                }
                if (!state.warningLines.empty())
                    text = state.warningLines[size_t((now - state.warningStarted) / 12000) % state.warningLines.size()];
            }
            else
                state.warning.clear();
            SetText(line, TextHash("INFO"), text);
            if (warning.empty() && !pause)
                HideObject(line);
            else
                ShowObject(line);
        }
    }

    void* (__cdecl* FindFont)(uint32_t hash) = nullptr;
    double (__thiscall* StringWidth)(void* font, const wchar_t* text, uint32_t flags, uint32_t wrap, uint8_t* wrapped) = nullptr;
    uint8_t (__cdecl* LocalizedWide)(wchar_t* text, int capacity, uint32_t hash) = nullptr;

    bool DecodeText(const std::string& text, std::vector<wchar_t>& wide)
    {
        if (!LocalizedWide || text.size() >= size_t(INT_MAX / 2 - 1))
            return false;
        auto hash = TextHash("MEASURE");
        Texts[hash] = text;
        wide.assign(text.size() + 1, L'\0');
        return LocalizedWide(wide.data(), int(wide.size()), hash) != 0;
    }

    std::string SupportedText(void* object, std::string text)
    {
        if (!object || !FindFont)
            return text;
        auto bytes = static_cast<uint8_t*>(object);
        auto font = static_cast<uint8_t*>(FindFont(*reinterpret_cast<uint32_t*>(bytes + 0x24)));
        auto header = font ? *reinterpret_cast<uint8_t**>(font + 12) : nullptr;
        if (!header || *reinterpret_cast<uint32_t*>(header) != 0x46544E46)
            return text;
        auto count = *reinterpret_cast<uint16_t*>(header + 10);
        auto offset = *reinterpret_cast<uint32_t*>(header + 20);
        if (!count || count > 4096 || offset > 0x100000)
            return text;
        auto stride = *reinterpret_cast<uint32_t*>(header + 12) & 0x40000 ? 16 : 12;
        auto hasGlyph = [&](uint16_t code)
        {
            size_t first{}, last = count;
            while (first < last)
            {
                auto middle = first + (last - first) / 2;
                auto value = *reinterpret_cast<uint16_t*>(header + offset + middle * stride);
                if (value == code)
                    return true;
                if (value < code)
                    first = middle + 1;
                else
                    last = middle;
            }
            return false;
        };
        std::vector<wchar_t> wide;
        bool decoded = DecodeText(text, wide) && std::wcslen(wide.data()) == text.size();
        for (size_t i = 0; i < text.size(); ++i)
        {
            auto c = uint8_t(text[i]);
            if (c == '\r' || c == '\n' || c == '\t')
                continue;
            auto code = decoded ? uint16_t(wide[i]) : uint16_t(c);
            if (code >= 0xFF80)
                code = uint8_t(code);
            if (code == 153)
                code = 8482;
            else if (code == 156)
                code = 339;
            if (decoded && code == c && hasGlyph(code))
                continue;
            wchar_t original = c;
            wchar_t decomposed[8]{};
            if (c >= 0x80 && NormalizeString(NormalizationD, &original, 1, decomposed, 8) > 0 &&
                decomposed[0] >= 0x20 && decomposed[0] < 0x80 && hasGlyph(uint16_t(decomposed[0])))
                text[i] = char(decomposed[0]);
            else if (c >= 0x80 || !hasGlyph(code))
                text[i] = hasGlyph('?') ? '?' : ' ';
        }
        return text;
    }

    float MeasureText(void* object, const std::string& text)
    {
        if (!object || !FindFont || !StringWidth || !LocalizedWide || text.size() >= size_t(INT_MAX / 2 - 1))
            return std::numeric_limits<float>::quiet_NaN();
        auto bytes = static_cast<uint8_t*>(object);
        auto data = *reinterpret_cast<float**>(bytes + 0x2C);
        auto font = FindFont(*reinterpret_cast<uint32_t*>(bytes + 0x24));
        if (!data || *reinterpret_cast<uint32_t*>(bytes + 0x30) < 68 || !font ||
            !*reinterpret_cast<uintptr_t*>(static_cast<uint8_t*>(font) + 12) || !std::isfinite(data[14]))
            return std::numeric_limits<float>::quiet_NaN();
        std::vector<wchar_t> wide;
        if (!DecodeText(text, wide))
            return std::numeric_limits<float>::quiet_NaN();
        return float(StringWidth(font, wide.data(), *reinterpret_cast<uint32_t*>(bytes + 0x1C), 0, nullptr)) * std::abs(data[14]);
    }

    bool GlyphBounds(void* object, float& top, float& bottom)
    {
        if (!object || !FindFont)
            return false;
        auto bytes = static_cast<uint8_t*>(object);
        auto data = *reinterpret_cast<float**>(bytes + 0x2C);
        auto font = static_cast<uint8_t*>(FindFont(*reinterpret_cast<uint32_t*>(bytes + 0x24)));
        if (!data || *reinterpret_cast<uint32_t*>(bytes + 0x30) < 68 || !font ||
            !std::isfinite(data[15]) || data[15] <= 0.0f)
            return false;
        auto header = *reinterpret_cast<uint8_t**>(font + 12);
        if (!header || *reinterpret_cast<uint32_t*>(header) != 0x46544E46)
            return false;
        auto count = *reinterpret_cast<uint16_t*>(header + 10);
        auto offset = *reinterpret_cast<uint32_t*>(header + 20);
        if (!count || count > 4096 || offset > 0x100000)
            return false;
        auto stride = *reinterpret_cast<uint32_t*>(header + 12) & 0x40000 ? 16 : 12;
        float minimum = std::numeric_limits<float>::max();
        float maximum = std::numeric_limits<float>::lowest();
        for (size_t i = 0; i < count; ++i)
        {
            auto glyph = header + offset + i * stride;
            float y = *reinterpret_cast<int8_t*>(glyph + 10);
            minimum = std::min(minimum, y);
            maximum = std::max(maximum, y + glyph[3]);
        }
        auto origin = *reinterpret_cast<float*>(font + 40);
        top = (minimum + origin) * data[15];
        bottom = (maximum + origin) * data[15];
        return std::isfinite(top) && std::isfinite(bottom) && bottom > top;
    }

    std::string SingleLine(std::string text)
    {
        for (auto& c : text)
        {
            if (c == '\r' || c == '\n' || c == '\t' || c == '^')
                c = ' ';
        }
        return text;
    }

    void FitOption(Option* option, std::string& label, const std::string& value)
    {
        if (!option->metricsSaved)
        {
            option->dataX = option->At<float>(0x44);
            option->dataWidth = option->At<float>(0x3C);
            option->metricsSaved = true;
        }
        option->At<float>(0x44) = option->dataX;
        option->At<float>(0x3C) = option->dataWidth;
        auto valueWidth = MeasureText(option->Value(), value);
        auto labelWidth = MeasureText(option->Title(), label);
        auto arrow = static_cast<uint8_t*>(option->LeftArrow());
        auto arrowData = arrow ? *reinterpret_cast<float**>(arrow + 0x2C) : nullptr;
        if (!std::isfinite(valueWidth) || !std::isfinite(labelWidth) || !arrowData ||
            *reinterpret_cast<uint32_t*>(arrow + 0x30) < 68 || !std::isfinite(arrowData[14]) ||
            !std::isfinite(option->dataX) || !std::isfinite(option->dataWidth) || option->dataWidth < 0.0f)
            return;
        auto width = std::max(option->dataWidth, valueWidth + 8.0f);
        option->At<float>(0x44) = option->dataX + option->dataWidth - width;
        option->At<float>(0x3C) = width;
        auto available = option->At<float>(0x44) - std::abs(arrowData[14]) - 8.0f - option->At<float>(0x0C);
        label = WFPMenuLayout::Fit(std::move(label), available, [&](const std::string& candidate)
        {
            return MeasureText(option->Title(), candidate);
        });
        // The native widget positions the value and both arrows together, including their animation bases.
        CallMethod(option, 4);
    }

    void __fastcall DrawOption(Option* option)
    {
        UpdateLanguage();
        auto entry = option->entry;
        if (!entry)
            return;
        auto label = SupportedText(option->Title(), SingleLine(FontText(LabelText(*entry))));
        auto value = SupportedText(option->Value(), SingleLine(FontText(ValueText(*entry))));
        FitOption(option, label, value);
        SetText(option->Title(), entry->labelHash, label);
        SetText(option->Value(), entry->valueHash, value);
    }

    void* OptionVTableStorage[18] = {};
    void** OptionVTable = &OptionVTableStorage[1];
    void* BaseShowOption = nullptr;

    // Showing a row: rows that left and right don't change have no arrows
    void __fastcall ShowOption(Option* option)
    {
        reinterpret_cast<void(__fastcall*)(Option*, void*)>(BaseShowOption)(option, nullptr);
        if (option->entry && !HasArrows(*option->entry))
        {
            HideObject(option->LeftArrow());
            HideObject(option->RightArrow());
        }
    }

    void DrawAll()
    {
        for (auto option : Active.options)
        {
            if (option->entry && option->entry->kind != Entry::Kind::Empty)
                DrawOption(option);
        }
        ShowInfo();
    }

    bool SameRow(const Entry& a, const Entry& b)
    {
        return a.kind == b.kind && a.plugin == b.plugin && a.row.kind == b.row.kind && a.row.plugin == b.row.plugin && a.row.setting == b.row.setting;
    }

    // The rows of the list or of the page's part shown, in the screen's options; select: the row to highlight, from 1.
    // Without force, rows that stay the same are only drawn again.
    void ShowRows(int select, bool force = true)
    {
        if (!Active.screen || Active.video || !Plugins || Plugins->plugins.empty() || Active.options.empty())
            return;
        std::vector<Entry> entries;
        if (CurrentLevel == Level::List)
        {
            auto slice = WFPMenuLayout::Root(Plugins->plugins.size(), Active.options.size(), RootPage);
            RootPage = slice.page;
            if (slice.selector)
                entries.push_back({ Entry::Kind::RootPage });
            for (size_t i = 0; i < slice.count; ++i)
                entries.push_back({ Entry::Kind::Plugin, slice.start + i });
        }
        else
        {
            auto& plugin = Plugins->plugins[CurrentPlugin];
            auto parts = Parts(plugin, Active.rowsWithArrows - 1);
            CurrentPart = std::min(CurrentPart, parts.empty() ? 0 : parts.size() - 1);
            entries.push_back({ Entry::Kind::Title, CurrentPlugin });
            if (CurrentPart < parts.size())
            {
                for (auto& row : parts[CurrentPart].rows)
                {
                    if (WFPPlugins::Visible(row) && entries.size() < Active.options.size())
                        entries.push_back({ Entry::Kind::Row, CurrentPlugin, row });
                }
            }
        }

        if (!force)
        {
            bool same = true;
            for (size_t i = 0; i < Active.options.size() && same; ++i)
                same = SameRow(Active.entries[i], i < entries.size() ? entries[i] : Entry{});
            if (same)
            {
                DrawAll();
                return;
            }
        }

        // The options keep their texts' hashes
        std::vector<Option*> shown, hidden;
        for (size_t i = 0; i < Active.options.size(); ++i)
        {
            auto& entry = Active.entries[i];
            auto label = entry.labelHash, value = entry.valueHash;
            entry = i < entries.size() ? entries[i] : Entry{};
            entry.labelHash = label;
            entry.valueHash = value;
            (i < entries.size() ? shown : hidden).push_back(Active.options[i]);
        }

        if (auto current = Screen::At<Option*>(Active.screen, Screen::Current))
            CallMethod(current, 10);
        Screen::At<Option*>(Active.screen, Screen::Current) = nullptr;
        Relink(Active.screen, shown, hidden);
        DrawAll();
        Screen::At<uint8_t>(Active.screen, Screen::Selected) = 0;
        SelectOption(Active.screen, nullptr, std::clamp(select, 1, int(std::max<size_t>(1, shown.size()))));
    }

    int CurrentRow()
    {
        auto current = Active.screen ? Screen::At<Option*>(Active.screen, Screen::Current) : nullptr;
        auto options = Active.screen ? ListOptions(Active.screen) : std::vector<Option*>{};
        auto it = std::find(options.begin(), options.end(), current);
        return it != options.end() ? int(it - options.begin()) + 1 : 1;
    }

    void OpenPlugin(size_t index)
    {
        CurrentLevel = Level::Page;
        CurrentPlugin = index;
        CurrentPart = 0;
        PresetRow.Reset(Plugins->plugins[index]);
        RestoreRow.Cancel();
        ShowRows(1);
    }

    int RootSelection()
    {
        auto count = Plugins ? Plugins->plugins.size() : 0;
        if (!count || Active.options.empty())
        {
            RootPage = 0;
            return 1;
        }
        CurrentPlugin = std::min(CurrentPlugin, count - 1);
        auto first = WFPMenuLayout::Root(count, Active.options.size(), 0);
        auto rows = first.selector ? Active.options.size() - 1 : Active.options.size();
        RootPage = CurrentPlugin / rows;
        auto slice = WFPMenuLayout::Root(count, Active.options.size(), RootPage);
        return int(CurrentPlugin - slice.start + 1 + size_t(slice.selector));
    }

    void OpenList()
    {
        CurrentLevel = Level::List;
        PresetRow.Cancel();
        RestoreRow.Cancel();
        ShowRows(RootSelection());
    }

    // After a change: the rows again (a setting can show or hide others), the selection stays on its row
    void Refresh()
    {
        if (Active.video)
            DrawAll();
        else
            ShowRows(CurrentRow(), false);
    }

    // The screen sends left, right and accept to the highlighted option, and "back" before it leaves
    void __fastcall ActOption(Option* option, void* edx, const char* package, uint32_t message)
    {
        auto entry = option->entry;
        if (!entry || (message != MessageLeft && message != MessageRight && message != MessageAccept))
        {
            DrawOption(option);
            return;
        }

        bool preset = entry->kind == Entry::Kind::Row && entry->row.kind == WFPPlugins::Row::Kind::Preset;
        bool restore = entry->kind == Entry::Kind::Row && entry->row.kind == WFPPlugins::Row::Kind::RestoreDefaults;
        bool cancelled = (!preset || message != MessageAccept) && PresetRow.Cancel();
        cancelled |= (!restore || message != MessageAccept) && RestoreRow.Cancel();
        int direction = message == MessageLeft ? -1 : 1;

        switch (entry->kind)
        {
            case Entry::Kind::RootPage:
                if (message != MessageAccept)
                {
                    auto slice = WFPMenuLayout::Root(Plugins->plugins.size(), Active.options.size(), RootPage);
                    RootPage = message == MessageLeft ? (slice.page ? slice.page - 1 : slice.pages - 1) :
                        (slice.page + 1) % slice.pages;
                    ShowRows(1);
                }
                return;
            case Entry::Kind::Plugin:
                if (message == MessageAccept)
                    OpenPlugin(entry->plugin);
                return;
            case Entry::Kind::Title:
                if (message != MessageAccept)
                {
                    auto parts = Parts(Plugins->plugins[CurrentPlugin], Active.rowsWithArrows - 1);
                    int count = int(std::max<size_t>(1, parts.size()));
                    CurrentPart = size_t(((int(CurrentPart) + direction) % count + count) % count);
                    ShowRows(1);
                    return;
                }
                break;
            case Entry::Kind::Row:
                switch (entry->row.kind)
                {
                    case WFPPlugins::Row::Kind::Preset:
                        if (message == MessageAccept)
                        {
                            if (PresetRow.Accept() && !PresetRow.confirm)
                            {
                                Refresh();
                                return;
                            }
                        }
                        else
                            PresetRow.Cycle(direction);
                        break;
                    case WFPPlugins::Row::Kind::RestoreDefaults:
                        if (message == MessageAccept && RestoreRow.Accept(*entry->row.plugin, Exclude(*entry->row.plugin)) && !RestoreRow.confirm)
                        {
                            Refresh();
                            return;
                        }
                        break;
                    case WFPPlugins::Row::Kind::Action:
                        if (message == MessageAccept)
                            OtherPlugins::Invoke(*entry->row.plugin, *entry->row.setting);
                        break;
                    default:
                        if (message != MessageAccept && WFPPlugins::Step(entry->row, direction))
                        {
                            Refresh();
                            return;
                        }
                        break;
                }
                break;
            default:
                break;
        }
        if (cancelled)
            DrawAll();
        else
        {
            DrawOption(option);
            ShowInfo();
        }
    }

    // The screen's arguments for an option's constructor
    void* ScreenField(void* screen, uintptr_t offset)
    {
        return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(screen) + offset);
    }

    Option* AddRow(void* screen)
    {
        auto option = static_cast<Option*>(GameMalloc(sizeof(Option)));
        if (!option)
            return nullptr;
        ToggleOption(option, nullptr, ScreenField(screen, 0xFC), ScreenField(screen, 0x10C), ScreenField(screen, 0x114), ScreenField(screen, 0x11C), 1);
        if (!OptionVTable[0])
        {
            // The toggle class with the methods of the fix's rows (and its type information before the table)
            std::copy_n(option->vtable - 1, std::size(OptionVTableStorage), OptionVTableStorage);
            OptionVTable[1] = reinterpret_cast<void*>(&ActOption);
            OptionVTable[3] = reinterpret_cast<void*>(&DrawOption);
            BaseShowOption = std::exchange(OptionVTable[5], reinterpret_cast<void*>(&ShowOption));
        }
        option->vtable = OptionVTable;
        option->entry = nullptr;
        option->metricsSaved = false;
        AddToggleOption(screen, nullptr, option, 1);
        return option;
    }

    void NewScreen(void* screen, size_t rows, bool video, bool pause)
    {
        Active = {};
        Active.screen = screen;
        Active.rowsWithArrows = rows;
        Active.video = video;
        Active.pause = pause;
        Active.entries.resize(rows);
        for (size_t i = 0; i < rows; ++i)
        {
            auto prefix = std::string(video ? "VIDEO_" : "ROW_") + std::to_string(i);
            Active.entries[i].labelHash = TextHash(prefix + "_LABEL");
            Active.entries[i].valueHash = TextHash(prefix + "_VALUE");
        }
    }

    // The options screen always opens the host-first list of plugins.
    constexpr uint32_t ScreenTitle = 0x42ADB44C;    // "Header_text"

    void BuildFixScreen(void* screen, size_t rows)
    {
        UpdateLanguage();
        NewScreen(screen, rows, false, rows < 12);
        Plugins = WFPPlugins::Find(Self(), LanguageTag());
        if (Plugins->plugins.empty())
            return;
        for (size_t i = 0; i < rows; ++i)
        {
            auto option = AddRow(screen);
            if (!option)
                break;
            option->entry = &Active.entries[i];
            Active.options.push_back(option);
        }
        Active.entries.resize(Active.options.size());
        Active.rowsWithArrows = std::max<size_t>(2, Active.options.size());

        // The list after the pages of the same menu were shown stays on the plugin shown last
        CurrentPlugin = std::min(CurrentPlugin, Plugins->plugins.size() - 1);
        CurrentLevel = Level::List;
        CurrentPart = 0;
        PresetRow.Reset(Plugins->plugins[CurrentPlugin]);
        RestoreRow.Cancel();

    }

    // After the game's setup of the fix's screen: the rows of the list or the page
    void ShowFixScreen()
    {
        if (Active.screen && !Active.video && Plugins && !Plugins->plugins.empty())
            ShowRows(CurrentLevel == Level::List ? RootSelection() : 1);
    }

    void SetTitles(void* screen, bool pause)
    {
        auto package = Screen::PackageName(screen);
        if (!pause)
        {
            if (auto title = FindString(package, ScreenTitle))
                SetLanguageHash(title, TextHash("WFP_PLUGINS"));
            return;
        }
        // The pause options screen's texts, set by each of the game's categories: the title ("Options"), the
        // category below it and the buttons'
        constexpr uint32_t PauseHeaderText = 0x1D7BB6C9;
        constexpr uint32_t PauseBackButton = 0x1C7FCF8B;    // "Back_button"
        constexpr uint32_t PauseBackButtonText = 0x933DD695;
        if (auto header = FindString(package, ScreenTitle))
            SetLanguageHash(header, PauseHeaderText);
        if (auto back = FindString(package, PauseBackButton))
            SetLanguageHash(back, PauseBackButtonText);
    }

    // The display options: the fix's rows after the game's, then placed among them
    void AddVideoRows(void* screen, size_t rows)
    {
        auto& joined = rows < 12 ? PauseJoinedSettings : MainJoinedSettings;
        joined.clear();
        if (VideoSettings.empty())
            return;
        auto gameOptions = ListOptions(screen);
        if (gameOptions.size() != 4 || gameOptions.size() + VideoSettings.size() > rows)
            return;
        UpdateLanguage();
        NewScreen(screen, VideoSettings.size(), true, rows < 12);
        Plugins = WFPPlugins::Find(Self(), LanguageTag());
        if (Plugins->plugins.empty() || !IsFix(Plugins->plugins.front()))
            return;
        auto& fix = Plugins->plugins.front();
        for (size_t i = 0; i < VideoSettings.size(); ++i)
        {
            auto setting = fix.file.description.Find(VideoSettings[i]);
            auto option = setting ? AddRow(screen) : nullptr;
            if (!option)
                break;
            auto& entry = Active.entries[i];
            entry.kind = Entry::Kind::Row;
            entry.row = { WFPPlugins::Row::Kind::Setting, &fix, setting };
            option->entry = &entry;
            Active.options.push_back(option);
            joined.push_back(setting->id);
            DrawOption(option);
        }
        // Resolution, window mode, level of detail, field of view, HUD, gamma, customize, default settings
        auto ours = Active.options;
        std::vector<Option*> order{ gameOptions[0] };
        if (!ours.empty())
            order.push_back(ours[0]);
        order.push_back(gameOptions[1]);
        order.insert(order.end(), ours.begin() + std::min<size_t>(1, ours.size()), ours.end());
        order.insert(order.end(), gameOptions.begin() + 2, gameOptions.end());
        Relink(screen, order);
    }

    // The screens' messages: back on a plugin's page returns to the list; "Default Settings" of the display options
    // resets the fix's rows there too
    bool BeforeNotify(void* screen, uint32_t message)
    {
        if (screen != Active.screen || !OptionsType)
            return false;
        if (!Active.video && *OptionsType == FixOptionsType && message == MessageBack && CurrentLevel == Level::Page &&
            Plugins && !Plugins->plugins.empty())
        {
            OpenList();
            return true;
        }
        return false;
    }

    void AfterNotify(void* screen, uint32_t message)
    {
        if (screen != Active.screen || !Active.video || !OptionsType || *OptionsType != VideoOptionsType || message != MessageDialogYes)
            return;
        std::vector<const asi_settings::Setting*> settings;
        for (auto& entry : Active.entries)
        {
            if (entry.kind == Entry::Kind::Row)
                settings.push_back(entry.row.setting);
        }
        if (Plugins && !Plugins->plugins.empty())
            WFPPlugins::RestoreDefaults(Plugins->plugins.front(), settings);
        DrawAll();
    }

    SafetyHookInline NotifyHooks[2] = {};
    template<size_t N>
    void __fastcall Notify(void* screen, void* edx, uint32_t message, int a3, int a4, int a5)
    {
        if (BeforeNotify(screen, message))
            return;
        NotifyHooks[N].unsafe_fastcall(screen, edx, message, a3, a4, a5);
        AfterNotify(screen, message);
    }

    SafetyHookInline DestructorHooks[2] = {};
    template<size_t N>
    void* __fastcall Destructor(void* screen, void* edx, int flags)
    {
        if (screen == Active.screen)
            Active = {};
        return DestructorHooks[N].unsafe_fastcall<void*>(screen, edx, flags);
    }

    // The category in the options menu: the game's icon option class (0x48 bytes) with a class of its own, whose
    // only method besides the destructor answers "accept" by setting the options type
    constexpr uint32_t CategoryIcon = 0x4DF98FB2;       // the gameplay category's icon
    void* (__fastcall* IconOptionCtor)(void* option, void* edx, uint32_t icon, uint32_t text, uint32_t unknown) = nullptr;
    void* CategoryVTable[2] = {};

    void __fastcall CategoryReact(void* option, void* edx, uint32_t unknown, uint32_t message, uint32_t a3, uint32_t a4, uint32_t a5)
    {
        if (message == MessageAccept)
            *OptionsType = FixOptionsType;
    }

    void AddCategory(void* menu)
    {
        auto option = GameMalloc(0x48);
        if (!option)
            return;
        UpdateLanguage();
        IconOptionCtor(option, nullptr, CategoryIcon, TextHash("WFP_PLUGINS"), 0);
        *reinterpret_cast<void***>(option) = CategoryVTable;
        // IconScrollerMenu::AddOption
        reinterpret_cast<void(__fastcall*)(void*, void*, void*)>((*reinterpret_cast<void***>(menu))[6])(menu, nullptr, option);
    }

    const std::array<std::string, 2> HelpNames{ "WFP_HELP_0", "WFP_HELP_1" };

    uint8_t* FindHelpObject(size_t index)
    {
        if (!Active.screen || !FindPackage || !FindPackageObject || index >= HelpNames.size())
            return nullptr;
        auto package = FindPackage(Screen::PackageName(Active.screen));
        return package ? FindPackageObject(package, bStringHash(HelpNames[index].c_str())) : nullptr;
    }

    uint8_t* HelpCaption(HelpState& state, size_t index)
    {
        if (index >= state.ids.size())
            return nullptr;
        auto object = FindHelpObject(index);
        return object && *reinterpret_cast<uint32_t*>(object + 0x18) == 2 &&
            *reinterpret_cast<uint32_t*>(object + 0x0C) == state.ids[index] ? object : nullptr;
    }

    void HideHelp(HelpState& state)
    {
        for (size_t i = 0; i < state.ids.size(); ++i)
        {
            if (auto object = HelpCaption(state, i))
                HideObject(object);
        }
        state.text.clear();
        state.page = std::numeric_limits<size_t>::max();
    }

    bool EnsureHelp(HelpState& state)
    {
        if (!Active.screen || !FindPackage || !FindPackageObject || Active.options.empty())
            return false;
        auto package = FindPackage(Screen::PackageName(Active.screen));
        if (!package)
            return false;
        bool any{};
        bool complete = !state.ids.empty();
        for (size_t i = 0; i < HelpNames.size(); ++i)
        {
            auto object = FindHelpObject(i);
            any |= object != nullptr;
            if (i < state.ids.size() && !HelpCaption(state, i))
                complete = false;
            if (object && i >= state.ids.size())
                complete = false;
        }
        if (complete)
            return true;
        if (any)
            return false;
        state.ids.clear();
        auto original = static_cast<uint8_t*>(Active.options.front()->Value());
        auto label = Active.options.front()->Title();
        auto arrow = static_cast<uint8_t*>(Active.options.front()->RightArrow());
        auto arrowData = arrow ? *reinterpret_cast<float**>(arrow + 0x2C) : nullptr;
        float top{}, bottom{}, labelTop{}, labelBottom{};
        if (!original || !arrowData || *reinterpret_cast<uint32_t*>(arrow + 0x30) < 68 ||
            !GlyphBounds(original, top, bottom) || !GlyphBounds(label, labelTop, labelBottom))
            return false;
        auto visible = Screen::At<uint32_t>(Active.screen, 0x134);
        auto firstY = Screen::At<float>(Active.screen, 0xF8);
        auto rowHeight = Screen::At<float>(Active.screen, 0x108);
        auto x = Screen::At<float>(Active.screen, 0xF4);
        auto right = Screen::At<float>(Active.screen, 0x11C) + Screen::At<float>(Active.screen, 0x114);
        if (!visible || visible > 32 || !std::isfinite(firstY) || !std::isfinite(rowHeight) || rowHeight <= 0.0f ||
            !std::isfinite(x) || !std::isfinite(right) || !std::isfinite(arrowData[14]))
            return false;
        auto y = firstY + (visible - 1) * rowHeight + labelBottom + 2.0f - top;
        auto pitch = bottom - top + 2.0f;
        auto width = right + std::abs(arrowData[14]) - 8.0f - x;
        size_t count{};
        // Keep the full native viewport and leave the bottom input hints clear.
        while (count < HelpNames.size() && y + count * pitch + bottom <= 190.0f)
            ++count;
        if (!count || !std::isfinite(width) || width <= 0.0f)
            return false;
        state.ids.reserve(count);
        std::array<uint8_t*, 2> staged{};
        for (size_t i = 0; i < count; ++i)
        {
            staged[i] = NFSU2StaticText::Clone(original);
            auto data = staged[i] ? *reinterpret_cast<float**>(staged[i] + 0x2C) : nullptr;
            if (!data)
            {
                for (auto object : staged)
                    NFSU2StaticText::Destroy(object);
                return false;
            }
            data[7] = x;
            data[8] = y + i * pitch;
            std::fill_n(reinterpret_cast<uint32_t*>(data), 4, 255u);
            *reinterpret_cast<uint32_t*>(staged[i] + 0x10) = bStringHash(HelpNames[i].c_str());
            *reinterpret_cast<uint32_t*>(staged[i] + 0x6C) &= ~3u;
            *reinterpret_cast<uint32_t*>(staged[i] + 0x1C) |= 0x4400001;
        }
        auto tail = *reinterpret_cast<uint8_t**>(package + 0x50);
        for (size_t i = 0; i < count; ++i)
        {
            auto object = staged[i];
            *reinterpret_cast<uint8_t**>(object + 4) = nullptr;
            *reinterpret_cast<uint8_t**>(object + 8) = tail;
            if (tail)
                *reinterpret_cast<uint8_t**>(tail + 4) = object;
            else
                *reinterpret_cast<uint8_t**>(package + 0x4C) = object;
            tail = object;
            ++*reinterpret_cast<uint32_t*>(package + 0x48);
            state.ids.push_back(*reinterpret_cast<uint32_t*>(object + 0x0C));
        }
        *reinterpret_cast<uint8_t**>(package + 0x50) = tail;
        state.width = width;
        state.text.clear();
        state.page = std::numeric_limits<size_t>::max();
        return true;
    }

    void RefreshHelp()
    {
        if (!Active.screen)
            return;
        auto& state = CurrentHelp();
        auto selected = Screen::At<Option*>(Active.screen, Screen::Current);
        if (std::find(Active.options.begin(), Active.options.end(), selected) == Active.options.end() ||
            !selected || !selected->entry || selected->entry->kind == Entry::Kind::Empty)
        {
            HideHelp(state);
            return;
        }
        if (!EnsureHelp(state))
        {
            if (Active.video)
                (Active.pause ? PauseJoinedSettings : MainJoinedSettings).clear();
            HideHelp(state);
            return;
        }
        if (Active.video)
        {
            auto& joined = Active.pause ? PauseJoinedSettings : MainJoinedSettings;
            joined.clear();
            for (const auto& entry : Active.entries)
            {
                if (entry.kind == Entry::Kind::Row && entry.row.setting)
                    joined.push_back(entry.row.setting->id);
            }
        }
        auto& entry = *selected->entry;
        auto text = LabelText(entry);
        auto value = ValueText(entry);
        if (!value.empty())
            text += ": " + value;
        if (entry.kind == Entry::Kind::Row)
            text += "\n" + WFPPlugins::Help(entry.row);
        if (WFPPlugins::RestartNeeded() && !FindString(Screen::PackageName(Active.screen), Active.pause ? PauseInfoLine : InfoLine))
            text = WFPPlugins::RestartWarning() + "\n" + text;
        text = SupportedText(HelpCaption(state, 0), FontText(text));
        auto now = GetTickCount64();
        bool changed = text != state.text;
        if (changed)
        {
            state.text = text;
            state.started = now;
            state.lines = WFPMenuLayout::Wrap(text, state.width, [&](const std::string& candidate)
            {
                return MeasureText(HelpCaption(state, 0), candidate);
            });
        }
        auto pages = std::max<size_t>(1, (state.lines.size() + state.ids.size() - 1) / state.ids.size());
        auto page = size_t((now - state.started) / 12000) % pages;
        state.page = page;
        for (size_t i = 0; i < state.ids.size(); ++i)
        {
            auto line = page * state.ids.size() + i;
            SetText(HelpCaption(state, i), TextHash(HelpNames[i]), line < state.lines.size() ? state.lines[line] : "");
        }
        for (size_t i = 0; i < state.ids.size(); ++i)
        {
            if (auto object = HelpCaption(state, i))
                ShowObject(object);
        }
    }

    // The screen being shown, while settings change elsewhere (live link, presets of other plugins)
    void OnFrame()
    {
        if (WFPPlugins::Changed().exchange(false) && Active.screen)
            DrawAll();
        if (Active.screen)
        {
            RefreshHelp();
            ShowInfo();
        }
    }
}

class Options
{
public:
    Options()
    {
        WFP::onInitEvent() += []()
        {
            // A toggle of the gameplay screen: allocation, the toggle class' constructor and adding it
            auto option = hook::pattern("6A 54 8B F1 E8 ? ? ? ? 8B F8 83 C4 04 89 7C 24 0C 85 FF C7 44 24 18 00 00 00 00 74 2D 6A 01 8D 86 1C 01 00 00 50 8D 8E 14 01 00 00 51 8D 96 0C 01 00 00 52 8D 86 FC 00 00 00 50 8B CF E8 ? ? ? ? C7 07 ? ? ? ? EB 02 33 FF 6A 01 83 CB FF 57 8B CE 89 5C 24 20 E8");
            auto setHash = hook::pattern("8B 46 2C 68 BF 5C 6A F2 50 E8");
            auto search = hook::pattern("33 FF 4E 8B C6 D1 E8 8B 0C C3 3B CA");
            auto found = hook::pattern("8B 44 C3 04 5F 5E 5D 5B C3");
            auto languages = hook::pattern("B9 ? ? ? ? EB 03 8D 49 00 39 11 74 12 83 C1 10 40 81 F9");
            auto select = hook::pattern("56 8B F1 8A 86 45 01 00 00 84 C0 0F 85");
            auto hide = hook::pattern("8B 44 24 04 85 C0 74 ? 8B 0D ? ? ? ? 85 C9 75 ? F6 40 1C 01 75 ? 81 48 1C 01 00 40 00");
            auto show = hook::pattern("8B 44 24 04 85 C0 74 ? 8B 0D ? ? ? ? 85 C9 75 ? F6 40 1C 01 74 ? 8B 48 1C 83 E1 FE");
            // The options menu (UI_OptionsMain.fng): after the last category is added; the icon option constructor;
            // the switch that opens the screen of the chosen category; the options screen's setup by the type; the
            // options screen's title, set for the game's types; the screen's class
            auto categories = hook::pattern("C7 07 ? ? ? ? EB 02 33 FF 8B 06 8B CE 57 89 5C 24 ? FF 50 18 8B 46 04 8B 5E 4C");
            auto iconCtor = hook::pattern("68 44 F1 7A F3 8B CF E8");
            auto menuSwitch = hook::pattern("A1 ? ? ? ? 83 F8 08 0F 87 ? ? ? ? FF 24 85 ? ? ? ? 8B 56 04 6A 00 68");
            auto screenSwitch = hook::pattern("A1 ? ? ? ? 83 C4 38 83 F8 07 0F 87 ? ? ? ? FF 24 85");
            auto title = hook::pattern("8B 46 04 68 83 D7 6C E7 68 4C B4 AD 42 50 E8");
            auto screenClass = hook::pattern("C7 06 ? ? ? ? 88 9E 4C 01 00 00 88 9E 4D 01 00 00 88 9E 4E 01 00 00");
            // Without the category the settings stay in the .cfg only: the game's own screens aren't changed
            if (option.empty() || setHash.empty() || search.empty() || found.empty() || languages.empty() || select.empty() ||
                hide.empty() || show.empty() || categories.empty() || iconCtor.empty() || menuSwitch.empty() ||
                screenSwitch.empty() || title.empty() || screenClass.empty())
                return;

            GameMalloc = reinterpret_cast<decltype(GameMalloc)>(injector::GetBranchDestination(option.get_first(4)).as_int());
            ToggleOption = reinterpret_cast<decltype(ToggleOption)>(injector::GetBranchDestination(option.get_first(0x3E)).as_int());
            AddToggleOption = reinterpret_cast<decltype(AddToggleOption)>(injector::GetBranchDestination(option.get_first(0x59)).as_int());
            SelectOption = reinterpret_cast<decltype(SelectOption)>(select.get_first());
            SetLanguageHash = reinterpret_cast<decltype(SetLanguageHash)>(injector::GetBranchDestination(setHash.get_first(9)).as_int());
            FindString = reinterpret_cast<decltype(FindString)>(injector::GetBranchDestination(title.get_first(0xE)).as_int());
            HideObject = reinterpret_cast<decltype(HideObject)>(hide.get_first());
            ShowObject = reinterpret_cast<decltype(ShowObject)>(show.get_first());
            IconOptionCtor = reinterpret_cast<decltype(IconOptionCtor)>(injector::GetBranchDestination(iconCtor.get_first(7)).as_int());
            auto font = hook::pattern("56 57 B9 ? ? ? ? E8 ? ? ? ? 8B 3D ? ? ? ? 8B 54 24 0C 8B F0 8B C7 3B C6");
            auto width = hook::pattern("83 EC 14 57 8B 7C 24 1C 85 FF 89 4C 24 04 75 0D D9 05 ? ? ? ? 5F 83 C4 14 C2 10 00 53 55 56");
            auto wide = hook::pattern("8B 54 24 0C E8 ? ? ? ? 85 C0 74 2D 8B 0D ? ? ? ? 85 C9 50 74 12");
            if (font.size() == 1 && width.size() == 1 && wide.size() == 1)
            {
                FindFont = reinterpret_cast<decltype(FindFont)>(font.get_first());
                StringWidth = reinterpret_cast<decltype(StringWidth)>(width.get_first());
                LocalizedWide = reinterpret_cast<decltype(LocalizedWide)>(wide.get_first());
                auto package = hook::pattern("56 8B 74 24 08 85 F6 57 8B 7C 24 10 74 19 56 E8 ? ? ? ? 83 C4 04 85 C0 74 0C 57 8B C8 E8 ? ? ? ? 85 C0 75 17 A1 ? ? ? ? 85 C0 74 14 8B 48 08 56 57 E8 ? ? ? ? 85 C0 74 06 83 78 18 02 74 02 33 C0 5F 5E C3");
                if (package.size() == 1)
                {
                    FindPackage = reinterpret_cast<decltype(FindPackage)>(injector::GetBranchDestination(package.get_first(0x0F)).as_int());
                    FindPackageObject = reinterpret_cast<decltype(FindPackageObject)>(injector::GetBranchDestination(package.get_first(0x1E)).as_int());
                }
            }
            Languages = *languages.get_first<LanguageInfo*>(1);
            CurrentLanguage = *languages.get_first<int32_t*>(0x14);
            LanguagesEnd = reinterpret_cast<LanguageInfo*>(CurrentLanguage);
            OptionsType = *menuSwitch.get_first<int32_t*>(1);

            // The category's class: the destructor of the game's categories and the fix's answer to "accept"
            auto gameCategoryVTable = *categories.get_first<void**>(2);
            CategoryVTable[0] = gameCategoryVTable[0];
            CategoryVTable[1] = reinterpret_cast<void*>(&CategoryReact);

            // Every text lookup by hash ends in this search (edx = hash); the fix's texts leave through its "found" exit
            static auto foundExit = reinterpret_cast<uintptr_t>(found.get_first(4));
            static auto shSearchString = safetyhook::create_mid(search.get_first(), [](SafetyHookContext& regs)
            {
                if (auto it = Texts.find(regs.edx); it != Texts.end())
                {
                    regs.eax = reinterpret_cast<uintptr_t>(it->second.c_str());
                    regs.eip = foundExit;
                }
            });

            // The options menu's setup: the fix's category after the game's (esi = the menu)
            static auto CategoriesHook = safetyhook::create_mid(categories.get_first(0x16), [](SafetyHookContext& regs)
            {
                AddCategory(reinterpret_cast<void*>(regs.esi));
            });

            // Choosing a category: the fix's opens UI_Options.fng, like the game's audio, video, gameplay and player
            static auto MenuSwitchHook = safetyhook::create_mid(menuSwitch.get_first(5), [](SafetyHookContext& regs)
            {
                if (int32_t(regs.eax) == FixOptionsType)
                    regs.eax = 0;
            });

            // The options screen's setup (esi = the screen): the fix's type has no list of the game's, its rows are
            // added before the screen picks its first option (the switch's default case), then set to the list or page
            // The screen's class: its messages and its destructor
            auto mainVTable = *screenClass.get_first<void**>(2);
            NotifyHooks[0] = safetyhook::create_inline(mainVTable[2], &Notify<0>);
            DestructorHooks[0] = safetyhook::create_inline(mainVTable[0], &Destructor<0>);

            // The switch's cases: the display options' setup, then the first option picked (0); the default case
            auto table = *screenSwitch.get_first<uintptr_t*>(0x14);
            auto videoCase = reinterpret_cast<uint8_t*>(table[VideoOptionsType]);
            auto defaultCase = screenSwitch.get_first<uint8_t>(0x0B);
            auto defaultTarget = defaultCase + 6 + *reinterpret_cast<int32_t*>(defaultCase + 2);
            bool video = videoCase[0] == 0x8B && videoCase[1] == 0xCE && videoCase[2] == 0xE8 && videoCase[7] == 0x6A && videoCase[8] == 0x00;
            static SafetyHookMid DefaultCaseHook = {};
            if (defaultCase[0] == 0x0F && defaultCase[1] == 0x87 && defaultTarget[0] == 0x6A && defaultTarget[1] == 0x00)
            {
                DefaultCaseHook = safetyhook::create_mid(defaultTarget, [](SafetyHookContext& regs)
                {
                    if (*OptionsType == FixOptionsType)
                        ShowFixScreen();
                });
            }

            static auto ScreenSwitchHook = safetyhook::create_mid(screenSwitch.get_first(8), [](SafetyHookContext& regs)
            {
                if (int32_t(regs.eax) == FixOptionsType)
                {
                    auto screen = reinterpret_cast<void*>(regs.esi);
                    SetTitles(screen, false);
                    BuildFixScreen(screen, 12);
                }
            });

            // Without any of them the menu is left out completely
            if (!shSearchString || !CategoriesHook || !MenuSwitchHook || !NotifyHooks[0] || !DestructorHooks[0] || !ScreenSwitchHook)
            {
                ScreenSwitchHook = {};
                DefaultCaseHook = {};
                DestructorHooks[0] = {};
                NotifyHooks[0] = {};
                MenuSwitchHook = {};
                CategoriesHook = {};
                shSearchString = {};
                return;
            }

            // The pause menu's options (UI_PauseOptionsMain.fng): after its last category (player) is added; the
            // pause options screen's setup by the type (cases 0 to 3), the common end of its cases and its class.
            // Every category of the pause menu opens UI_PauseOptions.fng, the fix's too; leaving it goes back to the
            // pause menu's options for any type.
            auto pauseCategories = hook::pattern("C7 07 ? ? ? ? EB 02 33 FF 8B 16 57 8B CE 89 5C 24 ? FF 52 18 8B 46 04 8B 5E 4C");
            auto pauseScreenSwitch = hook::pattern("A1 ? ? ? ? 83 C4 18 83 F8 03 77 ? FF 24 85");
            auto pauseClass = hook::pattern("C7 06 ? ? ? ? 88 9E 50 01 00 00 89 9E 54 01 00 00 88 9E 58 01 00 00");
            bool pause = !pauseCategories.empty() && !pauseScreenSwitch.empty() && !pauseClass.empty() &&
                *pauseScreenSwitch.get_first<int32_t*>(1) == OptionsType;
            uint8_t* pauseEnd = nullptr;
            if (pause)
            {
                auto pauseDefault = pauseScreenSwitch.get_first<uint8_t>(0x0B);
                pauseEnd = pauseDefault + 2 + *reinterpret_cast<int8_t*>(pauseDefault + 1);
                pause = pauseEnd[0] == 0x8B && pauseEnd[1] == 0x46 && pauseEnd[2] == 0x04;
            }

            // The display options' rows, in both menus or in none
            static SafetyHookMid VideoHook = {};
            if (video && pause)
            {
                for (auto id : VideoSettingIds)
                {
                    if (FixSettings().GetDescription().Find(id))
                        VideoSettings.push_back(id);
                }
                VideoHook = safetyhook::create_mid(videoCase + 7, [](SafetyHookContext& regs)
                {
                    AddVideoRows(reinterpret_cast<void*>(regs.esi), 12);
                });
            }

            WFP::onGameProcessEvent() += []()
            {
                OnFrame();
            };

            if (!pause)
                return;

            static auto PauseCategoriesHook = safetyhook::create_mid(pauseCategories.get_first(0x16), [](SafetyHookContext& regs)
            {
                AddCategory(reinterpret_cast<void*>(regs.esi));
            });

            // The pause options screen's setup (esi = the screen): the fix's type isn't one of the game's, its texts
            // and rows are set before the screen picks its first option
            static auto PauseScreenSwitchHook = safetyhook::create_mid(pauseScreenSwitch.get_first(8), [](SafetyHookContext& regs)
            {
                if (int32_t(regs.eax) == FixOptionsType)
                {
                    auto screen = reinterpret_cast<void*>(regs.esi);
                    SetTitles(screen, true);
                    BuildFixScreen(screen, 11);
                }
            });

            static auto PauseEndHook = safetyhook::create_mid(pauseEnd, [](SafetyHookContext& regs)
            {
                if (*OptionsType == FixOptionsType)
                    ShowFixScreen();
                else if (*OptionsType == VideoOptionsType)
                    AddVideoRows(reinterpret_cast<void*>(regs.esi), 11);
            });

            auto pauseVTable = *pauseClass.get_first<void**>(2);
            NotifyHooks[1] = safetyhook::create_inline(pauseVTable[2], &Notify<1>);
            DestructorHooks[1] = safetyhook::create_inline(pauseVTable[0], &Destructor<1>);

            // Without any of them the pause menu stays as it is (the display rows too, both menus or none)
            if (!PauseCategoriesHook || !PauseScreenSwitchHook || !PauseEndHook || !NotifyHooks[1] || !DestructorHooks[1])
            {
                DestructorHooks[1] = {};
                NotifyHooks[1] = {};
                PauseEndHook = {};
                PauseScreenSwitchHook = {};
                PauseCategoriesHook = {};
                VideoHook = {};
                VideoSettings.clear();
            }
        };
    }
} Options;
