module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "../Shared/NFS/TextLanguage.hpp"
#include "../Shared/OtherPlugins.hpp"
#include "../Shared/NFS/PluginsMenu.hpp"
#include "../Shared/NFS/StickDeadzone.hpp"
#include "../Shared/NFS/Text.hpp"
#include "../Shared/NFS/LegacyText.hpp"
#include "../Shared/NFS/MenuLayout.hpp"
#include "StaticText.hpp"

export module Options;

import ComVars;
import Input;
import Frontend;

// The plugins' settings in the game's options menus, with the game's own categories left as they are. The main menu's
// Credits entry becomes "Plugins" (its text and description only) and opens the game's options screen
// (MU_Options_PC.fng, ten rows with a left/right/accept handler each) with a category of the fix; the pause menu gets
// a "Plugins" entry below HUD that opens the pause options screen (MU_PauseOptions_PC.fng, eight rows) the same way.
// The category first lists the plugins (the fix first, then every other plugin with settings in menus, and in the main
// menu the game's credits); accepting one shows its page: a page selector in the first row when the page needs more
// than one screen, then its rows (WFPPlugins: the Preset row, the settings, Restore Defaults). Back returns to the
// list. Native rows below each plugin page show complete setting help and separate restart notes; long text continues
// on timed pages at the original font size. The plugin list is paged when necessary.

namespace
{
    constexpr int RowCount = 10;
    // The game's categories are 0 to 10 (dword_735E20); every table of them is a bounded switch
    constexpr int CategoryAdvanced = 11;
    constexpr uint32_t MessageLeft = 0x9120409E;
    constexpr uint32_t MessageRight = 0xB5971BF1;
    constexpr uint32_t MessageAccept = 0x0C407210;
    constexpr uint32_t MessageBack = 0x911AB364;            // the main options screen leaves for the selector
    constexpr uint32_t MessagePauseBack = 0xF2D0D887;       // the pause options screen leaves for its selector

    int32_t* Category = nullptr;
    void* RowShow = nullptr;        // usercall: ebx = screen; row, show data, selectable
    void* RowArrows = nullptr;      // usercall: ebx = row; screen, slider
    void* Printf = nullptr;         // usercall: eax = package, edx = object; format, ...
    void* PlaySound = nullptr;      // usercall: eax = sound; package
    void* PrintfByHash = nullptr;   // usercall: eax = package, edi = object hash; format, ...
    void* ShowObject = nullptr;     // usercall: edx = object name, edi = package name
    void* SetButtonEnabled = nullptr;   // usercall: eax = package name, edi = button name hash; FEngine, enabled
    void* SetCurrentButton = nullptr;   // usercall: eax = package name, ecx = button name hash; FEngine
    uint32_t TitleHash = 0;         // the options screen's title

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

    std::string LanguageName()
    {
        int32_t id = CurrentLanguage ? *CurrentLanguage : 0;
        for (auto info = Languages; info && info < LanguagesEnd; ++info)
        {
            if (info->id == id && info->name)
            {
                std::string name = info->name;
                for (auto& c : name)
                    c = char(tolower(uint8_t(c)));
                return WFPMenu::TextLanguage(FixSettings(), name);
            }
        }
        return WFPMenu::TextLanguage(FixSettings(), "english");
    }

    // Latin font bytes with readable replacements; Russian releases keep their native Cyrillic code page
    std::string GameText(const std::string& text, bool capitals = false)
    {
        auto language = LanguageName();
        WFPText::SetLanguage(language);
        auto converted = capitals ? WFPText::Upper(text) : text;
        if (language == "russian")
            return WFPText::ToCodePage(converted, 1251);
        return WFPLegacyText::Encode(converted);
    }

    // The language's text of a key, in the game's font
    std::string KeyText(const char* key, bool capitals = false)
    {
        WFPText::SetLanguage(LanguageName());
        return GameText(WFPText::Get(key), capitals);
    }

    const char* PackageName(void* screen)
    {
        return *reinterpret_cast<const char**>(reinterpret_cast<uintptr_t>(screen) + 0x0C);
    }

    void PrintText(void* screen, const std::string& object, const std::string& text)
    {
        auto package = PackageName(screen);
        auto name = object.c_str();
        auto value = text.c_str();
        static const char format[] = "%s";
        auto fn = Printf;
        __asm
        {
            push value
            push offset format
            mov eax, package
            mov edx, name
            call fn
            add esp, 8
        }
    }

    void PrintTitle(void* screen, const std::string& text)
    {
        auto package = PackageName(screen);
        auto value = text.c_str();
        static const char format[] = "%s";
        auto fn = PrintfByHash;
        auto hash = TitleHash;
        __asm
        {
            push edi
            push value
            push offset format
            mov eax, package
            mov edi, hash
            call fn
            add esp, 8
            pop edi
        }
    }

    void Sound(void* screen, int sound)
    {
        auto package = PackageName(screen);
        auto fn = PlaySound;
        __asm
        {
            push package
            mov eax, sound
            call fn
            add esp, 4
        }
    }

    uint32_t HashUpper(const std::string& text)
    {
        auto name = text.c_str();
        uint32_t result = 0;
        auto fn = FEHashUpper;
        __asm
        {
            mov edx, name
            call fn
            mov result, eax
        }
        return result;
    }

    void Show(void* screen, const std::string& object)
    {
        auto package = PackageName(screen);
        auto name = object.c_str();
        auto fn = ShowObject;
        __asm
        {
            push edi
            mov edx, name
            mov edi, package
            call fn
            pop edi
        }
    }

    void Hide(void* screen, const std::string& object)
    {
        auto package = PackageName(screen);
        auto hash = HashUpper(object);
        auto fn = FEngSetInvisible;
        __asm
        {
            push esi
            push edi
            mov esi, package
            mov edi, hash
            call fn
            pop edi
            pop esi
        }
    }

    void EnableButton(void* screen, int row, bool enabled)
    {
        auto package = PackageName(screen);
        auto button = HashUpper("OptionName_" + std::to_string(row));
        auto feng = cFEng::pInstance ? *cFEng::pInstance : nullptr;
        int state = enabled ? 1 : 0;
        auto fn = SetButtonEnabled;
        __asm
        {
            push edi
            push state
            push feng
            mov eax, package
            mov edi, button
            call fn
            pop edi
        }
    }

    // Moves the pad's and the mouse's highlight to a row, as the game does between its buttons
    void Focus(void* screen, int row)
    {
        auto package = PackageName(screen);
        auto button = HashUpper("OptionName_" + std::to_string(row));
        auto feng = cFEng::pInstance ? *cFEng::pInstance : nullptr;
        if (!feng)
            return;
        auto fn = SetCurrentButton;
        __asm
        {
            push feng
            mov eax, package
            mov ecx, button
            call fn
        }
    }

    uint8_t* FindPackageByName(const char* name)
    {
        uint8_t* result = nullptr;
        auto fn = FEPkgMgr_FindPackage;
        __asm
        {
            mov eax, name
            call fn
            mov result, eax
        }
        return result;
    }

    uint8_t* FindObjectByHash(uint8_t* package, uint32_t hash)
    {
        uint8_t* object = nullptr;
        auto findObject = FEngFindObject;
        __asm
        {
            mov ecx, hash
            mov edx, package
            call findObject
            mov object, eax
        }
        return object;
    }

    // An object's state (colour, pivot, position at [7], rotation, size at [14]), when it's at least this big
    float* ObjectData(uint8_t* object, size_t minimumSize)
    {
        if (!object || *reinterpret_cast<uint32_t*>(object + 0x30) < minimumSize)
            return nullptr;
        return *reinterpret_cast<float**>(object + 0x2C);
    }

    // The object's state changed: the game makes its drawing again
    void MarkChanged(uint8_t* object)
    {
        *reinterpret_cast<uint32_t*>(object + 0x1C) |= 0x400000;
    }

    bool FontMetricsPending{};
    void* FontLookup{};              // usercall: eax = font hash
    double (__fastcall* FontWidth)(uint32_t flags, void* edx, void* font, const wchar_t* text, uint32_t wrap){};
    std::map<std::pair<std::string, std::string>, float> OriginalX;

    uint8_t* TextObject(void* screen, const std::string& name)
    {
        auto package = FindPackageByName(PackageName(screen));
        return package ? FindObjectByHash(package, HashUpper(name)) : nullptr;
    }

    template<typename Function>
    void PositionTracks(uint8_t* object, Function&& function)
    {
        auto script = *reinterpret_cast<uint8_t**>(object + 0x4C);
        for (size_t depth = 0; script && depth < 64; ++depth, script = *reinterpret_cast<uint8_t**>(script + 4))
        {
            auto count = *reinterpret_cast<uint32_t*>(script + 0x1C);
            auto tracks = *reinterpret_cast<uint8_t**>(script + 0x20);
            if (!tracks || count > 512)
                continue;
            for (size_t i = 0; i < count; ++i)
            {
                auto track = tracks + i * 0x38;
                if (track[0] == 4 && track[7] == 7)
                    function(*reinterpret_cast<float*>(track + 0x18));
            }
        }
    }

    float BaseX(uint8_t* object, float* data)
    {
        float x = data[7];
        bool found{};
        PositionTracks(object, [&](float base)
        {
            if (!found)
            {
                x = base;
                found = true;
            }
        });
        return x;
    }

    float SavedX(void* screen, const std::string& name, float* data)
    {
        return OriginalX.try_emplace(std::pair{ std::string(PackageName(screen)), name }, BaseX(TextObject(screen, name), data)).first->second;
    }

    void SetX(uint8_t* object, float* data, float x)
    {
        auto delta = x - BaseX(object, data);
        if (delta != 0.0f)
        {
            data[7] += delta;
            PositionTracks(object, [delta](float& base)
            {
                base += delta;
            });
            MarkChanged(object);
        }
    }

    void* ObjectFont(uint8_t* object)
    {
        if (!object || !FontLookup)
            return nullptr;
        auto hash = *reinterpret_cast<uint32_t*>(object + 0x24);
        void* font{};
        auto find = FontLookup;
        __asm
        {
            mov eax, hash
            call find
            mov font, eax
        }
        return font;
    }

    float MeasureText(void* screen, const std::string& name, const std::string& text)
    {
        PrintText(screen, name, text);
        auto object = TextObject(screen, name);
        auto data = ObjectData(object, 68);
        auto font = ObjectFont(object);
        auto wide = object ? *reinterpret_cast<const wchar_t**>(object + 0x60) : nullptr;
        if (!data || !font || !FontWidth || !wide || !*reinterpret_cast<uintptr_t*>(static_cast<uint8_t*>(font) + 12))
        {
            FontMetricsPending = true;
            return std::numeric_limits<float>::quiet_NaN();
        }
        return float(FontWidth(0, nullptr, font, wide, 0)) * std::abs(data[14]);
    }

    bool GlyphBounds(uint8_t* object, float& top, float& bottom)
    {
        auto data = ObjectData(object, 68);
        auto font = static_cast<uint8_t*>(ObjectFont(object));
        if (!data || !font || !std::isfinite(data[15]) || data[15] <= 0.0f)
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

    float BaseY(uint8_t* object, float* data)
    {
        float y = data[8];
        bool found{};
        PositionTracks(object, [&](float& base)
        {
            if (!found)
            {
                y = (&base)[1];
                found = true;
            }
        });
        return y;
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

    void* OpenPackage = nullptr;            // usercall: edi = package to open; current package name
    void Open(const char* package, const char* current)
    {
        auto fn = OpenPackage;
        __asm
        {
            push edi
            push current
            mov edi, package
            call fn
            add esp, 4
            pop edi
        }
    }

    // The game's credits, reached from the fix's category since the menu's Credits entry opens the fix's category
    const char* CreditsPackage = nullptr;   // "Credits.fng"
    uint32_t CreditsTextHash = 0xF55C86CE;  // MU_CREDITS, the Credits entry's own text (read from it when it's found)
    constexpr int CategoryCredits = 9;
    const char* (__fastcall* GameSearchForString)(void*, uint32_t) = nullptr;   // the game's text lookup, unhooked

    std::string CreditsText()
    {
        auto text = GameSearchForString ? GameSearchForString(nullptr, CreditsTextHash) : nullptr;
        return text ? text : "";
    }

    // The menu's entries are in capitals, the rows of the options screens aren't: the first letter of each word stays
    std::string TitleCase(std::string text)
    {
        bool first = true;
        for (auto& c : text)
        {
            auto u = uint8_t(c);
            bool upper = (u >= 'A' && u <= 'Z') || (u >= 0xC0 && u <= 0xDE && u != 0xD7);
            bool letter = upper || (u >= 'a' && u <= 'z') || u >= 0xDF;
            if (upper && !first)
                c = char(u + 0x20);
            first = !letter && u != '\'';
        }
        return text;
    }

    // The category's screen: the list of plugins, or one plugin's page
    enum class RowKind { None, Plugin, Credits, RootScreen, Screen, Item, Note };
    struct Row
    {
        RowKind kind = RowKind::None;
        size_t plugin = 0;              // Plugin: its index in the snapshot
        WFPPlugins::Row item;           // Item: the plugin's row
    };
    // How a row is shown: hidden, its name only, its name and value, with the arrows, or a note (no button)
    enum class Style { Hidden, Plain, Data, Arrows, Note };

    // The main menu's options screen (MU_Options_PC.fng) has ten rows with their handlers at +0x40; the pause
    // menu's (MU_PauseOptions_PC.fng) has eight with their handlers at +0x48 and no credits
    struct Menu
    {
        int rowCount;
        size_t handlers;
        bool pause;
        bool page = false;              // a plugin's page is shown, not the list
        size_t plugin = 0;
        size_t screen = 0;
        size_t rootPage = 0;
        std::vector<std::string> joinedSettings;
        std::vector<uint32_t> helpIds;
        float helpWidth{};
        std::string helpText;
        std::vector<std::string> helpLines;
        uint64_t helpStarted{};
        size_t helpPage{ std::numeric_limits<size_t>::max() };
        std::vector<WFPPlugins::Screen> screens;
        size_t reserved = 0;            // native rows kept free for complete help and restart notes
    };
    Menu MainMenu = { RowCount, 0x40, false };
    Menu PauseMenu = { 8, 0x48, true };
    Menu* Active = &MainMenu;   // the options screen being shown
    Row Rows[RowCount + 1];
    Style RowStyle[RowCount + 1] = {};
    std::atomic<bool> OwnSettingsChanged = false;

    // The plugins, found again each time the category is opened, so that the order the plugins were loaded in
    // doesn't matter. The rows point into them.
    std::shared_ptr<WFPPlugins::Snapshot> Plugins;
    WFPPlugins::PresetRow Preset;
    WFPPlugins::RestoreRow Restore;

    // The game's Display category (4 in both menus) has free rows below its own: the fix's picture settings are
    // shown there, next to the game's HUD and display options, and not on the fix's page
    constexpr int CategoryDisplay = 4;
    const std::vector<std::string> DisplaySettings = { "MAIN.HudAspectRatioConstraint", "MAIN.FOVFactor", "GRAPHICS.Scaling", "GRAPHICS.ConsoleGamma", "MAIN.FMVWidescreenMode" };
    int GameRowsFrom = 0;
    bool GameRowsInstalled = false; // the Display category's rows could be set up           // the first of the fix's rows in a game category, 0 when the category is the fix's

    HMODULE Self()
    {
        static HMODULE self = nullptr;
        if (!self)
            GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCWSTR>(&Self), &self);
        return self;
    }

    // The settings a plugin's page leaves out because the game's own screens show them
    const std::vector<std::string>& Excluded(const asi_settings::LoadedPlugin& plugin)
    {
        static const std::vector<std::string> none;
        return plugin.module == Self() && GameRowsInstalled ? Active->joinedSettings : none;
    }

    void FindPlugins()
    {
        auto tag = WFPText::Detail::Tag(LanguageName());
        Plugins = WFPPlugins::Find(Self(), tag ? asi_settings::text::Narrow(tag) : "en");
        WFPPlugins::Changed() = false;
    }

    void ShowMainRow(void* screen, int row, bool data, bool arrows)
    {
        auto fn = RowShow;
        int showData = data ? 1 : 0;
        __asm
        {
            push ebx
            mov ebx, screen
            push 0
            push showData
            push row
            call fn
            pop ebx
        }
        if (!arrows)
            return;
        auto show = RowArrows;
        __asm
        {
            push ebx
            mov ebx, row
            push 0
            push screen
            call show
            pop ebx
        }
    }

    void* PauseRowShow = nullptr;           // usercall: ebx = screen; row, show data

    void ShowPauseRow(void* screen, int row, bool data, bool arrows)
    {
        auto fn = PauseRowShow;
        int showData = data ? 1 : 0;
        __asm
        {
            push ebx
            mov ebx, screen
            push showData
            push row
            call fn
            pop ebx
        }
        if (!arrows)
            return;
        // The arrows, as the game's toggle rows show them
        for (auto format : { "LeftArrow_%d", "option_buttonL_%02d", "RightArrow_%d", "option_buttonR_%02d" })
        {
            char name[32];
            snprintf(name, sizeof(name), format, row);
            Show(screen, name);
        }
    }

    // Shows a row in a style; the objects of the style it had are hidden first, the way the game hides the rows it
    // doesn't use, and an unused row is out of the pad and mouse navigation (OptionName_n is its button)
    void SetStyle(void* screen, int row, Style style)
    {
        if (RowStyle[row] == style)
            return;
        static const char* const names[] = { "OptionData_%d", "OptionName_%d", "SelectButton_%d", "LeftArrow_%d", "RightArrow_%d", "OptionNametext_%d", "option_buttonR_%02d", "option_buttonL_%02d", "minus_%02d", "plus_%02d" };
        for (auto format : names)
        {
            char name[32];
            snprintf(name, sizeof(name), format, row);
            Hide(screen, name);
        }
        EnableButton(screen, row, false);
        RowStyle[row] = style;
        switch (style)
        {
            case Style::Plain:
            case Style::Data:
            case Style::Arrows:
                if (Active->pause)
                    ShowPauseRow(screen, row, style != Style::Plain, style == Style::Arrows);
                else
                    ShowMainRow(screen, row, style != Style::Plain, style == Style::Arrows);
                break;
            case Style::Note:
                Show(screen, "OptionNametext_" + std::to_string(row));
                break;
            default:
                break;
        }
    }

    // Preserve native font sizes and the complete value; labels use the space left beside the arrows.
    void DrawRow(void* screen, int row, const std::string& label, const std::string& value, bool fullWidth = false)
    {
        auto suffix = std::to_string(row);
        auto name = "OptionNametext_" + suffix;
        auto valueName = "OptionData_" + suffix;
        auto labelObject = TextObject(screen, name);
        auto valueObject = TextObject(screen, valueName);
        auto labelData = ObjectData(labelObject, 68);
        auto valueData = ObjectData(valueObject, 68);
        auto buttonObject = TextObject(screen, "OptionName_" + suffix);
        auto buttonData = ObjectData(buttonObject, 68);
        auto leftName = "LeftArrow_" + suffix;
        auto rightObject = TextObject(screen, "RightArrow_" + suffix);
        auto rightData = ObjectData(rightObject, 68);
        auto leftData = ObjectData(TextObject(screen, leftName), 68);
        auto singleValue = SingleLine(value);
        auto valueWidth = MeasureText(screen, valueName, singleValue);
        float width{};
        if (labelData && buttonData)
        {
            auto right = BaseX(buttonObject, buttonData) + std::abs(buttonData[14]) * 0.5f - 8.0f;
            if (!fullWidth && !singleValue.empty() && valueData && leftData && rightData && std::isfinite(valueWidth))
            {
                auto justify = *reinterpret_cast<uint32_t*>(valueObject + 0x68);
                auto anchor = justify & 1 ? 0.5f : justify & 2 ? 1.0f : 0.0f;
                auto original = SavedX(screen, valueName, valueData);
                auto valueRight = original + valueWidth * (1.0f - anchor);
                auto limit = BaseX(rightObject, rightData) - std::abs(rightData[14]) * 0.5f - 8.0f;
                auto x = original + std::min(0.0f, limit - valueRight);
                SetX(valueObject, valueData, x);
                auto left = x - valueWidth * anchor;
                auto arrowOriginal = SavedX(screen, leftName, leftData);
                auto arrow = std::min(arrowOriginal, left - 18.0f);
                auto delta = arrow - arrowOriginal;
                char button[32];
                snprintf(button, sizeof(button), "option_buttonL_%02d", row);
                char minus[32];
                snprintf(minus, sizeof(minus), "minus_%02d", row);
                for (const auto& objectName : { leftName, std::string(button), std::string(minus) })
                {
                    auto object = TextObject(screen, objectName);
                    if (auto data = ObjectData(object, 68))
                        SetX(object, data, SavedX(screen, objectName, data) + delta);
                }
                right = arrow - std::abs(leftData[14]) * 0.5f - 8.0f;
            }
            else if (valueData)
                SetX(valueObject, valueData, SavedX(screen, valueName, valueData));
            width = right - BaseX(labelObject, labelData);
        }
        auto fitted = WFPMenuLayout::Fit(SingleLine(label), width, [&](const std::string& text)
        {
            return MeasureText(screen, name, text);
        });
        PrintText(screen, name, fitted);
        PrintText(screen, valueName, singleValue);
    }

    std::string SectionTitle(const WFPPlugins::Screen& screen)
    {
        // The screen's title is "Plugin: Section (1/2)"; the selector shows what follows the plugin's name
        auto name = WFPPlugins::Name(*screen.plugin);
        if (!screen.title.starts_with(name) || screen.title.size() == name.size())
            return screen.title;
        // "Plugin (1/2)": the screen's number without the parentheses
        auto rest = screen.title.substr(name.size());
        auto first = rest.find_first_not_of(": (");
        rest = first == std::string::npos ? "" : rest.substr(first);
        if (!rest.empty() && rest.back() == ')' && rest.find('(') == std::string::npos)
            rest.pop_back();
        return rest;
    }

    constexpr size_t NoteRows = 2;
    std::vector<std::string> NoteLines;
    std::vector<std::string> RestartLines;
    size_t NoteOffset{};
    size_t RestartOffset{};
    std::string NoteText;
    uint64_t NoteStarted{};
    size_t NotePage{ std::numeric_limits<size_t>::max() };
    int SelectedRow(void* screen);
    void RefreshNotes(void* screen);

    float NoteWidth(void* screen, int row)
    {
        auto text = TextObject(screen, "OptionNametext_" + std::to_string(row));
        auto button = TextObject(screen, "OptionName_" + std::to_string(row));
        auto data = ObjectData(text, 68);
        auto box = ObjectData(button, 68);
        return data && box ? BaseX(button, box) + std::abs(box[14]) * 0.5f - 8.0f - BaseX(text, data) : 0.0f;
    }

    void DrawItem(void* screen, int row)
    {
        auto& entry = Rows[row];
        switch (entry.kind)
        {
            case RowKind::Plugin:
                DrawRow(screen, row, GameText(WFPPlugins::Name(Plugins->plugins[entry.plugin])), "");
                break;
            case RowKind::Credits:
                DrawRow(screen, row, TitleCase(CreditsText()), "");   // already in the game's font
                break;
            case RowKind::RootScreen:
            {
                auto total = Plugins->plugins.size() + (!Active->pause && CreditsPackage ? 1 : 0);
                auto slice = WFPMenuLayout::Root(total, Active->rowCount - Active->reserved, Active->rootPage);
                DrawRow(screen, row, KeyText("WFP_SECTION"), std::to_string(slice.page + 1) + "/" + std::to_string(slice.pages));
                break;
            }
            case RowKind::Screen:
                DrawRow(screen, row, KeyText("WFP_SECTION"), GameText(SectionTitle(Active->screens[Active->screen])));
                break;
            case RowKind::Note:
                // The whole width of the row is the note's
            {
                auto helpRows = Active->page ? NoteRows : 0;
                auto help = entry.plugin < helpRows;
                const auto& lines = help ? NoteLines : RestartLines;
                auto index = help ? NoteOffset + entry.plugin : RestartOffset + entry.plugin - helpRows;
                DrawRow(screen, row, index < lines.size() ? lines[index] : "", "", true);
                break;
            }
            case RowKind::Item:
            {
                auto& item = entry.item;
                std::string value;
                if (item.kind == WFPPlugins::Row::Kind::Preset)
                    value = Preset.ValueText();
                else if (item.kind == WFPPlugins::Row::Kind::RestoreDefaults)
                    value = Restore.ValueText();
                else
                    value = WFPPlugins::ValueText(item);
                // In the game's categories the rows' names end with a colon, like the game's own
                auto label = WFPPlugins::Label(item) + (GameRowsFrom && row >= GameRowsFrom ? ":" : "");
                DrawRow(screen, row, GameText(label), GameText(value));
                break;
            }
            default:
                break;
        }
    }

    Style StyleOf(const Row& row)
    {
        switch (row.kind)
        {
            case RowKind::Plugin:
            case RowKind::Credits:
                return Style::Plain;
            case RowKind::RootScreen:
            case RowKind::Screen:
                return Style::Arrows;
            case RowKind::Note:
                return Style::Note;
            case RowKind::Item:
                switch (row.item.kind)
                {
                    case WFPPlugins::Row::Kind::Action:
                        return Style::Plain;
                    case WFPPlugins::Row::Kind::RestoreDefaults:
                        return Style::Data;
                    case WFPPlugins::Row::Kind::Setting:
                        return WFPPlugins::Enabled(row.item) ? Style::Arrows : Style::Data;
                    default:
                        return Style::Arrows;
                }
            default:
                return Style::Hidden;
        }
    }

    // The plugin's page in screens of the rows below the page selector, less the rows of the restart warning
    void BuildScreens(Menu& menu)
    {
        auto& plugin = Plugins->plugins[menu.plugin];
        menu.reserved = NoteRows + (WFPPlugins::RestartNeeded() ? NoteRows : 0);
        menu.screens = WFPPlugins::Screens(plugin, size_t(std::max(1, menu.rowCount - 1 - int(menu.reserved))), true, Excluded(plugin));
        if (menu.screen >= menu.screens.size())
            menu.screen = 0;
    }

    // Shows the list or the page: the rows, the title and the restart warning
    void Layout(void* screen)
    {
        auto& menu = *Active;
        WFPPlugins::Row selected{};
        bool keepSelection{};
        if (menu.page && menu.reserved != NoteRows + (WFPPlugins::RestartNeeded() ? NoteRows : 0))
        {
            auto row = SelectedRow(screen);
            if (row > 0 && Rows[row].kind == RowKind::Item)
            {
                selected = Rows[row].item;
                keepSelection = true;
            }
            BuildScreens(menu);
            if (keepSelection)
            {
                for (size_t i = 0; i < menu.screens.size(); ++i)
                {
                    for (const auto& item : menu.screens[i].rows)
                    {
                        if (item.kind == selected.kind && item.plugin == selected.plugin && item.setting == selected.setting)
                            menu.screen = i;
                    }
                }
            }
        }
        std::vector<Row> rows;
        std::string title;
        if (!menu.page)
        {
            title = KeyText("WFP_PLUGINS", !menu.pause);
            menu.reserved = WFPPlugins::RestartNeeded() ? NoteRows : 0;
            auto count = Plugins->plugins.size() + (!menu.pause && CreditsPackage ? 1 : 0);
            auto slice = WFPMenuLayout::Root(count, menu.rowCount - menu.reserved, menu.rootPage);
            menu.rootPage = slice.page;
            if (slice.selector)
                rows.push_back({ RowKind::RootScreen });
            for (size_t i = slice.start; i < slice.start + slice.count; ++i)
                rows.push_back(i < Plugins->plugins.size() ? Row{ RowKind::Plugin, i } : Row{ RowKind::Credits });
        }
        else
        {
            auto& plugin = Plugins->plugins[menu.plugin];
            title = GameText(WFPPlugins::Name(plugin), !menu.pause);
            if (menu.screen >= menu.screens.size())
                menu.screen = 0;
            if (menu.screens.size() > 1)
                rows.push_back({ RowKind::Screen });
            if (!menu.screens.empty())
            {
                // Rows hidden now (visibleIf, or the plugin has no usable value) leave no gap
                for (auto& item : menu.screens[menu.screen].rows)
                {
                    if (WFPPlugins::Visible(item))
                        rows.push_back({ RowKind::Item, 0, item });
                }
            }
        }
        if (menu.reserved)
        {
            // Notes use fixed native rows; longer descriptions continue without shrinking the font.
            rows.resize(size_t(menu.rowCount) - menu.reserved);
            for (size_t i = 0; i < menu.reserved; ++i)
                rows.push_back({ RowKind::Note, i });
        }

        if (menu.pause)
            PrintText(screen, "OptionTitle", title);
        else
            PrintTitle(screen, title);
        for (int row = 1; row <= menu.rowCount; ++row)
        {
            size_t index = size_t(row - 1);
            Rows[row] = index < rows.size() ? rows[index] : Row{};
            SetStyle(screen, row, StyleOf(Rows[row]));
            DrawItem(screen, row);
        }
        if (keepSelection)
        {
            for (int row = 1; row <= menu.rowCount; ++row)
            {
                const auto& item = Rows[row].item;
                if (Rows[row].kind == RowKind::Item && item.kind == selected.kind &&
                    item.plugin == selected.plugin && item.setting == selected.setting)
                {
                    Focus(screen, row);
                    break;
                }
            }
        }
        NoteText.clear();
        NoteLines.clear();
        RestartLines.clear();
        NoteOffset = 0;
        RestartOffset = 0;
        NoteStarted = GetTickCount64();
        NotePage = std::numeric_limits<size_t>::max();
        RefreshNotes(screen);
    }

    int SelectedRow(void* screen)
    {
        auto package = FindPackageByName(PackageName(screen));
        auto current = package ? *reinterpret_cast<uint8_t**>(package + 0x80) : nullptr;
        if (!current)
            return 0;
        auto hash = *reinterpret_cast<uint32_t*>(current + 0x10);
        for (int row = 1; row <= Active->rowCount; ++row)
        {
            if (hash == HashUpper("OptionName_" + std::to_string(row)))
                return row;
        }
        return 0;
    }

    void RefreshNotes(void* screen)
    {
        if (GameRowsFrom || !Active->reserved)
            return;
        std::string text;
        auto selected = SelectedRow(screen);
        if (selected > 0 && Rows[selected].kind == RowKind::Item)
        {
            const auto& item = Rows[selected].item;
            auto value = item.kind == WFPPlugins::Row::Kind::Preset ? Preset.ValueText() :
                item.kind == WFPPlugins::Row::Kind::RestoreDefaults ? Restore.ValueText() : WFPPlugins::ValueText(item);
            text = WFPPlugins::Label(item);
            if (!value.empty())
                text += ": " + value;
            auto help = WFPPlugins::Help(item);
            if (!help.empty())
                text += "\n" + help;
        }
        auto warning = WFPPlugins::RestartNeeded() ? GameText(WFPText::Get("WFP_RESTART_WARNING")) : "";
        text = GameText(text);
        auto key = text + (warning.empty() ? "" : "\n" + warning);
        auto now = GetTickCount64();
        bool changed = key != NoteText;
        if (changed)
        {
            NoteText = key;
            NoteStarted = now;
            auto width = std::numeric_limits<float>::max();
            for (int i = 0; i < int(Active->reserved); ++i)
                width = std::min(width, NoteWidth(screen, Active->rowCount - i));
            auto object = "OptionNametext_" + std::to_string(Active->rowCount);
            auto measure = [&](const std::string& candidate)
            {
                return MeasureText(screen, object, candidate);
            };
            NoteLines = Active->page ? WFPMenuLayout::Wrap(text, width, measure) : std::vector<std::string>{};
            RestartLines = WFPMenuLayout::Wrap(warning, width, measure);
        }
        auto page = size_t((now - NoteStarted) / 12000);
        if (!changed && page == NotePage)
            return;
        NotePage = page;
        auto pages = std::max<size_t>(1, (NoteLines.size() + NoteRows - 1) / NoteRows);
        auto warningPages = std::max<size_t>(1, (RestartLines.size() + NoteRows - 1) / NoteRows);
        NoteOffset = (page % pages) * NoteRows;
        RestartOffset = (page % warningPages) * NoteRows;
        for (int row = Active->rowCount - int(Active->reserved) + 1; row <= Active->rowCount; ++row)
            DrawItem(screen, row);
    }

    // The highlighted row isn't one any more (a shorter screen): the first row is
    void KeepFocus(void* screen)
    {
        auto package = FindPackageByName(PackageName(screen));
        auto current = package ? *reinterpret_cast<uint8_t**>(package + 0x80) : nullptr;
        if (!current)
            return;
        auto name = *reinterpret_cast<uint32_t*>(current + 0x10);
        for (int row = 1; row <= Active->rowCount; ++row)
        {
            if (name == HashUpper("OptionName_" + std::to_string(row)))
            {
                if (RowStyle[row] == Style::Hidden || RowStyle[row] == Style::Note)
                    Focus(screen, 1);
                return;
            }
        }
    }

    void ShowList(void* screen, size_t plugin)
    {
        Active->page = false;
        Preset.Cancel();
        Restore.Cancel();
        auto count = Plugins->plugins.size() + (!Active->pause && CreditsPackage ? 1 : 0);
        auto capacity = Active->rowCount - (WFPPlugins::RestartNeeded() ? NoteRows : 0);
        auto slice = WFPMenuLayout::Root(count, capacity, 0);
        if (slice.pages > 1)
            Active->rootPage = plugin / std::max<size_t>(1, capacity - 1);
        Layout(screen);
        slice = WFPMenuLayout::Root(count, capacity, Active->rootPage);
        Focus(screen, int(plugin - slice.start) + 1 + (slice.selector ? 1 : 0));
    }

    void ShowPage(void* screen, size_t plugin)
    {
        auto& menu = *Active;
        menu.page = true;
        menu.plugin = plugin;
        menu.screen = 0;
        auto& entry = Plugins->plugins[plugin];
        BuildScreens(menu);
        Preset.Reset(entry);
        Restore.Cancel();
        Layout(screen);
        Focus(screen, 1);
    }

    // The rows show the values in effect: the page again (its rows can depend on them), or the fix's rows of a game
    // category
    void Update(void* screen)
    {
        if (!GameRowsFrom)
        {
            Layout(screen);
            KeepFocus(screen);
            return;
        }
        for (int row = GameRowsFrom; row <= Active->rowCount; ++row)
        {
            if (Rows[row].kind != RowKind::Item)
                continue;
            SetStyle(screen, row, StyleOf(Rows[row]));
            DrawItem(screen, row);
        }
    }

    // Values changed elsewhere (an editor, a preset, the plugin itself): the rows show them
    void Refresh(void* screen)
    {
        bool own = OwnSettingsChanged.exchange(false);
        bool other = WFPPlugins::Changed().exchange(false);
        bool metrics = std::exchange(FontMetricsPending, false);
        if (metrics || ((own || other) && (Active->page || GameRowsFrom)))
            Update(screen);
    }

    // The confirmations of the Preset and Restore Defaults rows end with any other input
    void CancelConfirmations(void* screen, const Row& entry)
    {
        bool preset = entry.kind == RowKind::Item && entry.item.kind == WFPPlugins::Row::Kind::Preset;
        bool restore = entry.kind == RowKind::Item && entry.item.kind == WFPPlugins::Row::Kind::RestoreDefaults;
        if ((!preset && Preset.Cancel()) || (!restore && Restore.Cancel()))
        {
            for (int row = 1; row <= Active->rowCount; ++row)
                DrawItem(screen, row);
        }
    }

    template<int RowIndex>
    void __fastcall RowHandler(void* screen, void* edx, uint32_t message)
    {
        if (!Plugins)
            return;
        Refresh(screen);
        auto entry = Rows[RowIndex];
        RefreshNotes(screen);
        CancelConfirmations(screen, entry);
        int direction = message == MessageLeft ? -1 : message == MessageRight ? 1 : 0;
        bool accept = message == MessageAccept;
        switch (entry.kind)
        {
            case RowKind::Credits:
                // As the menu's Credits entry does; leaving the credits returns to the menu, at the Credits entry
                if (accept)
                {
                    *Category = CategoryCredits;
                    Open(CreditsPackage, PackageName(screen));
                }
                return;
            case RowKind::Plugin:
                if (accept && entry.plugin < Plugins->plugins.size())
                {
                    ShowPage(screen, entry.plugin);
                    Sound(screen, 11);
                }
                return;
            case RowKind::RootScreen:
            {
                if (!direction)
                    return;
                auto count = Plugins->plugins.size() + (!Active->pause && CreditsPackage ? 1 : 0);
                auto slice = WFPMenuLayout::Root(count, Active->rowCount - Active->reserved, Active->rootPage);
                Active->rootPage = (slice.page + slice.pages + direction) % slice.pages;
                Layout(screen);
                Focus(screen, 1);
                Sound(screen, direction < 0 ? 10 : 11);
                return;
            }
            case RowKind::Screen:
            {
                if (!direction || Active->screens.empty())
                    return;
                size_t count = Active->screens.size();
                Active->screen = (Active->screen + count + direction) % count;
                Preset.Cancel();
                Restore.Cancel();
                Layout(screen);
                Sound(screen, direction < 0 ? 10 : 11);
                return;
            }
            case RowKind::Item:
                break;
            default:
                return;
        }

        auto& item = entry.item;
        auto& plugin = *item.plugin;
        switch (item.kind)
        {
            case WFPPlugins::Row::Kind::Preset:
                if (direction)
                {
                    Preset.Cycle(direction);
                    DrawItem(screen, RowIndex);
                    Sound(screen, direction < 0 ? 10 : 11);
                }
                else if (accept && Preset.Accept())
                {
                    // Applied: the page shows the new values (its rows can depend on them)
                    WFPPlugins::Changed() = false;
                    OwnSettingsChanged = false;
                    Update(screen);
                    Sound(screen, 11);
                }
                return;
            case WFPPlugins::Row::Kind::RestoreDefaults:
                if (accept && Restore.Accept(plugin, Excluded(plugin)))
                {
                    if (!Restore.confirm)
                        Preset.Reset(plugin);
                    WFPPlugins::Changed() = false;
                    OwnSettingsChanged = false;
                    Update(screen);
                    Sound(screen, 11);
                }
                return;
            case WFPPlugins::Row::Kind::Action:
                if (accept && OtherPlugins::Invoke(plugin, *item.setting) >= 0)
                {
                    WFPPlugins::Changed() = false;
                    Update(screen);
                    Sound(screen, 11);
                }
                return;
            default:
                break;
        }

        // A setting; settings that depend on another one being on (enabledIf) keep their value while it's off
        if (!direction || !WFPPlugins::Step(item, direction))
            return;
        // The rows that depend on it (visibleIf, enabledIf), the preset in use and the restart warning follow
        WFPPlugins::Changed() = false;
        OwnSettingsChanged = false;
        Preset.Reset(plugin);
        Update(screen);
        Sound(screen, direction < 0 ? 10 : 11);
    }

    template<int... Indices>
    constexpr auto MakeHandlers(std::integer_sequence<int, Indices...>)
    {
        return std::array<void*, sizeof...(Indices)>{ reinterpret_cast<void*>(&RowHandler<Indices + 1>)... };
    }
    const auto Handlers = MakeHandlers(std::make_integer_sequence<int, RowCount>{});

    // The category's rows: the game has hidden all rows and set none up, so all are the fix's
    void SetupRows(void* screen, Menu& menu)
    {
        Active = &menu;
        GameRowsFrom = 0;
        FindPlugins();
        auto handlers = reinterpret_cast<void**>(reinterpret_cast<uintptr_t>(screen) + menu.handlers);
        for (int row = 1; row <= menu.rowCount; ++row)
        {
            handlers[row - 1] = Handlers[row - 1];
            RowStyle[row] = Style::Hidden;
        }
        // The page stays as it was when the screen comes back (from the credits, after the pause menu)
        if (menu.page && menu.plugin < Plugins->plugins.size())
        {
            auto& plugin = Plugins->plugins[menu.plugin];
            BuildScreens(menu);
            Preset.Reset(plugin);
            Restore.Cancel();
        }
        else
            menu.page = false;
        Layout(screen);
    }

    // The fix's rows below the game's own in its Display category: the rows after the last one the category shows
    void AddGameRows(void* screen, Menu& menu)
    {
        menu.joinedSettings.clear();
        GameRowsFrom = 0;
        auto package = FindPackageByName(PackageName(screen));
        if (!GameRowsInstalled || !package)
            return;
        int first = 1;
        for (int row = 1; row <= menu.rowCount; ++row)
        {
            auto text = FindObjectByHash(package, HashUpper("OptionNametext_" + std::to_string(row)));
            if (!text)
                return;
            if (!(*reinterpret_cast<uint32_t*>(text + 0x1C) & 1))
                first = row + 1;
        }
        Active = &menu;
        FindPlugins();
        if (Plugins->plugins.empty() || Plugins->plugins.front().module != Self())
            return;
        auto& fix = Plugins->plugins.front();
        auto handlers = reinterpret_cast<void**>(reinterpret_cast<uintptr_t>(screen) + menu.handlers);
        int row = first;
        for (auto& id : DisplaySettings)
        {
            auto& settings = fix.file.description.settings;
            auto setting = std::find_if(settings.begin(), settings.end(), [&](const asi_settings::Setting& s) { return asi_settings::text::IEquals(s.id, id); });
            if (setting == settings.end() || !WFPPlugins::Shown(*setting) || row > menu.rowCount)
                continue;
            auto item = WFPPlugins::Row{ WFPPlugins::Row::Kind::Setting, &fix, &*setting };
            if (!WFPPlugins::Visible(item))
                continue;
            handlers[row - 1] = Handlers[row - 1];
            Rows[row] = { RowKind::Item, 0, item };
            RowStyle[row] = Style::Hidden;
            menu.joinedSettings.push_back(setting->id);
            ++row;
        }
        for (int i = row; i <= menu.rowCount; ++i)
            Rows[i] = {};
        if (row == first)
            return;
        GameRowsFrom = first;
        Update(screen);
    }

    const std::array<std::string, 2> JoinedHelpNames{ "WFP_DisplayHelp_0", "WFP_DisplayHelp_1" };

    uint8_t* JoinedCaption(void* screen, const Menu& menu, size_t index)
    {
        if (index >= menu.helpIds.size())
            return nullptr;
        auto object = TextObject(screen, JoinedHelpNames[index]);
        return object && *reinterpret_cast<uint32_t*>(object + 0x0C) == menu.helpIds[index] &&
            *reinterpret_cast<uint32_t*>(object + 0x18) == 2 ? object : nullptr;
    }

    void HideJoinedHelp(void* screen, Menu& menu)
    {
        for (size_t i = 0; i < menu.helpIds.size(); ++i)
        {
            if (auto object = JoinedCaption(screen, menu, i))
            {
                auto& flags = *reinterpret_cast<uint32_t*>(object + 0x1C);
                if (!(flags & 1))
                    flags |= 0x400001;
            }
        }
        menu.helpText.clear();
        menu.helpPage = std::numeric_limits<size_t>::max();
    }

    bool EnsureJoinedHelp(void* screen, Menu& menu)
    {
        auto package = FindPackageByName(PackageName(screen));
        if (!package)
            return false;
        bool any{};
        bool complete = !menu.helpIds.empty();
        for (size_t i = 0; i < JoinedHelpNames.size(); ++i)
        {
            auto object = TextObject(screen, JoinedHelpNames[i]);
            any |= object != nullptr;
            if (i < menu.helpIds.size() && !JoinedCaption(screen, menu, i))
                complete = false;
            if (object && i >= menu.helpIds.size())
                complete = false;
        }
        if (complete)
            return true;
        if (any)
            return false;
        menu.helpIds.clear();
        auto suffix = std::to_string(menu.rowCount);
        auto original = TextObject(screen, "OptionData_" + suffix);
        auto label = TextObject(screen, "OptionNametext_" + suffix);
        auto button = TextObject(screen, "OptionName_" + suffix);
        auto state = ObjectData(original, 68);
        auto labelState = ObjectData(label, 68);
        auto buttonState = ObjectData(button, 68);
        float top{}, bottom{}, labelTop{}, labelBottom{};
        if (!state || !labelState || !buttonState || !GlyphBounds(original, top, bottom) ||
            !GlyphBounds(label, labelTop, labelBottom))
        {
            FontMetricsPending = true;
            return false;
        }
        auto x = BaseX(label, labelState);
        auto y = BaseY(label, labelState) + labelBottom + 2.0f - top;
        auto pitch = bottom - top + 2.0f;
        auto width = BaseX(button, buttonState) + std::abs(buttonState[14]) * 0.5f - 8.0f - x;
        size_t count{};
        // Keep the game's bottom input hints clear; the main screen fits one native-size line, pause fits two.
        while (count < JoinedHelpNames.size() && y + count * pitch + bottom <= 190.0f)
            ++count;
        if (!count || !std::isfinite(width) || width <= 0.0f)
            return false;
        std::array<uint8_t*, 2> staged{};
        for (size_t i = 0; i < count; ++i)
        {
            staged[i] = NFSUStaticText::Clone(original);
            if (!staged[i])
            {
                for (auto object : staged)
                    NFSUStaticText::Destroy(object);
                return false;
            }
            auto data = ObjectData(staged[i], 68);
            data[7] = x;
            data[8] = y + i * pitch;
            reinterpret_cast<uint32_t*>(data)[3] = 255;
            *reinterpret_cast<uint32_t*>(staged[i] + 0x10) = HashUpper(JoinedHelpNames[i]);
            *reinterpret_cast<uint32_t*>(staged[i] + 0x68) &= ~3u;
            *reinterpret_cast<uint32_t*>(staged[i] + 0x1C) |= 0x4400001;
        }
        // Publish only complete, noninteractive captions. The native package owns and retires them.
        auto tail = *reinterpret_cast<uint8_t**>(package + 0x44);
        for (size_t i = 0; i < count; ++i)
        {
            auto object = staged[i];
            *reinterpret_cast<uint8_t**>(object + 4) = nullptr;
            *reinterpret_cast<uint8_t**>(object + 8) = tail;
            if (tail)
                *reinterpret_cast<uint8_t**>(tail + 4) = object;
            else
                *reinterpret_cast<uint8_t**>(package + 0x40) = object;
            tail = object;
            ++*reinterpret_cast<uint32_t*>(package + 0x3C);
            menu.helpIds.push_back(*reinterpret_cast<uint32_t*>(object + 0x0C));
            AddHudLayoutCopy(BaseX(original, state), BaseY(original, state), x, y + i * pitch);
        }
        *reinterpret_cast<uint8_t**>(package + 0x44) = tail;
        menu.helpWidth = width;
        menu.helpText.clear();
        menu.helpPage = std::numeric_limits<size_t>::max();
        return true;
    }

    void RefreshJoinedHelp(void* screen)
    {
        auto& menu = *Active;
        auto selected = SelectedRow(screen);
        if (!GameRowsFrom || selected < GameRowsFrom || selected > menu.rowCount || Rows[selected].kind != RowKind::Item)
        {
            HideJoinedHelp(screen, menu);
            return;
        }
        if (!EnsureJoinedHelp(screen, menu))
        {
            // The Plugins page retains full help if the native layout cannot host a caption.
            menu.joinedSettings.clear();
            HideJoinedHelp(screen, menu);
            return;
        }
        menu.joinedSettings.clear();
        for (int row = GameRowsFrom; row <= menu.rowCount; ++row)
        {
            if (Rows[row].kind == RowKind::Item && Rows[row].item.setting)
                menu.joinedSettings.push_back(Rows[row].item.setting->id);
        }
        const auto& item = Rows[selected].item;
        auto text = WFPPlugins::Label(item) + ": " + WFPPlugins::ValueText(item) + "\n" + WFPPlugins::Help(item);
        if (WFPPlugins::RestartNeeded())
            text = WFPText::Get("WFP_RESTART_WARNING") + "\n" + text;
        text = GameText(text);
        auto now = GetTickCount64();
        bool changed = text != menu.helpText;
        if (changed)
        {
            menu.helpText = text;
            menu.helpStarted = now;
            menu.helpLines = WFPMenuLayout::Wrap(text, menu.helpWidth, [&](const std::string& candidate)
            {
                return MeasureText(screen, JoinedHelpNames[0], candidate);
            });
        }
        auto pages = std::max<size_t>(1, (menu.helpLines.size() + menu.helpIds.size() - 1) / menu.helpIds.size());
        auto page = size_t((now - menu.helpStarted) / 12000) % pages;
        if (changed || page != menu.helpPage)
        {
            menu.helpPage = page;
            for (size_t i = 0; i < menu.helpIds.size(); ++i)
            {
                auto line = page * menu.helpIds.size() + i;
                PrintText(screen, JoinedHelpNames[i], line < menu.helpLines.size() ? menu.helpLines[line] : "");
            }
        }
        for (size_t i = 0; i < menu.helpIds.size(); ++i)
        {
            if (auto object = JoinedCaption(screen, menu, i))
                *reinterpret_cast<uint32_t*>(object + 0x1C) &= ~1u;
        }
    }

    // The main menu's options screen is set up (the game's own categories are left alone)
    void AddRows(void* screen)
    {
        GameRowsFrom = 0;
        if (*Category == CategoryDisplay)
            AddGameRows(screen, MainMenu);
        if (*Category != CategoryAdvanced)
            return;
        SetupRows(screen, MainMenu);
    }

    // The pause menu's options screen; its title is the OptionTitle string
    int32_t* PauseCategory = nullptr;       // dword_777CB8: 0 to 7 are the game's
    constexpr int PauseCategoryAdvanced = 8;
    // The pause options screen's panel (IconCover, an image centred at x 22.6,
    // y -66, 691 by 192) ends at the sixth row (y 26), which the game's categories don't go below. For the fix's
    // eight rows it's made taller, down to below the eighth row (OptionNametext_8, y 89): only its height and its
    // centre's y change, so its top edge, its width and its x stay as they are (cFEngRender::RenderImage draws a
    // centred quad scaled by the object's size on each axis). The 45-degree corner pieces at its bottom left
    // (IconCover2/3, in the panel's colour) stay where they are, inside the taller panel. The game's categories get
    // the panel as it was.
    constexpr uint32_t PanelName = 0x84C0B267;          // IconCover
    constexpr float PanelY = -65.98379516601562f;       // the layout's centre and height
    constexpr float PanelHeight = 191.71759033203125f;
    constexpr float RowsBottom = 89.0f + 23.0f;         // OptionNametext_8 at y 89 and the margin below it

    void SizePausePanel(void* screen, bool tall)
    {
        auto package = FindPackageByName(PackageName(screen));
        if (!package)
            return;
        auto object = FindObjectByHash(package, PanelName);
        auto data = ObjectData(object, 64);
        if (!data)
            return;
        float extra = RowsBottom - (PanelY + PanelHeight * 0.5f);
        float tallY = PanelY + extra * 0.5f;
        // Only the layout's own panel is changed: a different layout is left as it is
        if (std::fabs(data[8] - PanelY) > 0.01f && std::fabs(data[8] - tallY) > 0.01f)
            return;
        data[8] = tall ? tallY : PanelY;
        data[15] = tall ? PanelHeight + extra : PanelHeight;
        MarkChanged(object);
    }

    void AddPauseRows(void* screen)
    {
        bool fix = *PauseCategory == PauseCategoryAdvanced;
        GameRowsFrom = 0;
        if (*PauseCategory == CategoryDisplay)
            AddGameRows(screen, PauseMenu);
        // The game's panel ends at the sixth row
        bool tall = fix || (GameRowsFrom && Rows[7].kind != RowKind::None);
        SizePausePanel(screen, tall);
        if (!fix)
            return;
        SetupRows(screen, PauseMenu);
    }

    // The fix's category is shown in this menu's options screen; or with game rows, the Display category
    bool IsShown(void* screen, const Menu& menu, bool gameRows = false)
    {
        if (&menu != Active || !screen)
            return false;
        auto package = PackageName(screen);
        if (!package || _stricmp(package, menu.pause ? "MU_PauseOptions_PC.fng" : "MU_Options_PC.fng") != 0)
            return false;
        auto category = menu.pause ? PauseCategory : Category;
        if (!category)
            return false;
        return *category == (menu.pause ? PauseCategoryAdvanced : CategoryAdvanced) || (gameRows && GameRowsFrom && *category == CategoryDisplay);
    }

    // Back on a plugin's page returns to the list, at the plugin's row; on the list it leaves as the game's back does
    bool Back(void* screen, Menu& menu)
    {
        if (!IsShown(screen, menu) || !menu.page)
            return false;
        Restore.Cancel();
        ShowList(screen, menu.plugin);
        Sound(screen, 10);
        return true;
    }

    SafetyHookInline shMainNotify = {};
    uint8_t __fastcall MainNotify(void* screen, void* edx, uint32_t message, void* object, int a4, int a5)
    {
        if (message == MessageBack && Back(screen, MainMenu))
            return 0;
        return shMainNotify.unsafe_fastcall<uint8_t>(screen, edx, message, object, a4, a5);
    }

    SafetyHookInline shPauseNotify = {};
    void __fastcall PauseNotify(void* screen, void* edx, uint32_t message, void* object, int a4, int a5)
    {
        if (message == MessagePauseBack && Back(screen, PauseMenu))
            return;
        shPauseNotify.unsafe_fastcall(screen, edx, message, object, a4, a5);
    }
}


// The options menu: its Credits entry becomes "Plugins" and opens the options screen with the fix's category instead
// of the credits, which are a row of the fix's category. Its position, highlight bar, navigation
// and mouse area stay the game's own.
namespace Selector
{
    constexpr uint32_t MessageHighlight = 0xABC08912;
    constexpr uint32_t CreditsName = 0xFD8A3B6D;            // the Credits entry's name hash, as the menu compares it
    constexpr int32_t TypeString = 2;
    constexpr size_t ObjNameHash = 0x10, ObjType = 0x18;
    constexpr size_t StringLanguageHash = 0x5C;             // FEString: the text's hash, looked up when it's drawn
    constexpr size_t ScreenDescription = 0x40;              // the menu screen's description text object

    const char* OptionsPackage = nullptr;   // the game's "MU_Options_PC.fng"
    void* SetLanguageHash = nullptr;        // usercall: eax = hash, esi = string object; clears the own-text flag
    uint32_t LabelHash = 0;
    uint32_t DescriptionHash = 0;
    uint32_t PauseLabelHash = 0;            // the pause menu's entry, whose buttons aren't in capitals

    template<typename T> T& At(uint8_t* base, size_t offset) { return *reinterpret_cast<T*>(base + offset); }

    uint8_t* FindObject(const char* packageName, uint32_t hash)
    {
        uint8_t* package = nullptr;
        auto findPackage = FEPkgMgr_FindPackage;
        __asm
        {
            mov eax, packageName
            call findPackage
            mov package, eax
        }
        if (!package)
            return nullptr;
        uint8_t* object = nullptr;
        auto findObject = FEngFindObject;
        __asm
        {
            mov ecx, hash
            mov edx, package
            call findObject
            mov object, eax
        }
        return object;
    }

    void SetText(uint8_t* object, uint32_t hash)
    {
        auto fn = SetLanguageHash;
        __asm
        {
            push esi
            mov eax, hash
            mov esi, object
            call fn
            pop esi
        }
    }

    bool IsCredits(uint8_t* object)
    {
        return object && At<uint32_t>(object, ObjNameHash) == CreditsName;
    }

    // SearchForString: every text by its hash; strings with a hash look up their text each time they're drawn
    SafetyHookInline shSearchForString = {};
    std::string LabelText, DescriptionText;
    const char* __fastcall SearchForString(void* ecx, uint32_t hash)
    {
        if (hash == LabelHash)
        {
            LabelText = KeyText("WFP_PLUGINS", true);
            return LabelText.c_str();
        }
        if (hash == PauseLabelHash)
        {
            DescriptionText = KeyText("WFP_PLUGINS");
            return DescriptionText.c_str();
        }
        if (hash == DescriptionHash)
        {
            DescriptionText = KeyText("WFP_NFSU_PLUGINS_DESCRIPTION");
            return DescriptionText.c_str();
        }
        return shSearchForString.unsafe_fastcall<const char*>(ecx, hash);
    }

    // The menu screen's constructor highlights the entry of the last category; it also gets the entry's new text.
    // The fix's category is the Credits entry's.
    SafetyHookInline shFocusLastCategory = {};
    void __stdcall FocusLastCategory(uint8_t* screen)
    {
        if (auto credits = FindObject(PackageName(screen), CreditsName); credits && At<int32_t>(credits, ObjType) == TypeString)
        {
            if (At<uint32_t>(credits, StringLanguageHash) != LabelHash)
                CreditsTextHash = At<uint32_t>(credits, StringLanguageHash);
            SetText(credits, LabelHash);
        }

        int category = *Category;
        if (category == CategoryAdvanced)
            *Category = CategoryCredits;
        shFocusLastCategory.unsafe_stdcall(screen);
        *Category = category;
    }

    // The menu's messages: the object is the entry
    SafetyHookInline shNotify = {};
    void __fastcall Notify(uint8_t* screen, void* edx, uint32_t message, uint8_t* object, int a4, int a5)
    {
        if (message == MessageAccept && IsCredits(object))
        {
            *Category = CategoryAdvanced;
            Open(OptionsPackage, PackageName(screen));
            return;
        }

        shNotify.unsafe_fastcall(screen, edx, message, object, a4, a5);

        // The game sets the description of the highlighted entry by its name
        if (message == MessageHighlight && IsCredits(object))
        {
            if (auto description = At<uint8_t*>(screen, ScreenDescription))
                SetText(description, DescriptionHash);
        }
    }

    // Everything is checked before anything is hooked; without the menu entry the fix's settings stay in the .ini
    // and .cfg and the game's menus are untouched
    bool Install()
    {
        auto focus = hook::pattern("A1 ? ? ? ? 83 EC 20 83 F8 0A 56 77 ? FF 24 85");
        auto notify = hook::pattern("8B 44 24 04 3D 0B 62 F8 35 57 8B F9 0F 87");
        auto highlight = hook::pattern("8B 54 24 0C 8B 42 10 56 57 E8 ? ? ? ? 8B 77 40 E8 ? ? ? ? 8B 7F 40 6A 01 68 ? ? ? ? E8");
        auto open = hook::pattern("8B 4F 0C 51 BF ? ? ? ? E8 ? ? ? ? 83 C4 04 5F C2 10 00");
        auto credits = hook::pattern("8B 47 0C 50 BF ? ? ? ? C7 05 ? ? ? ? 09 00 00 00 E8");
        auto title = hook::pattern("BA E9 EA 28 A9 E8 ? ? ? ? 85 C0 75 ? BA 63 6F 1A C0 E8 ? ? ? ? 50 8B 45 0C BF ? ? ? ? E8");
        auto buttonEnabled = hook::pattern("68 ? ? ? ? E8 ? ? ? ? 85 C0 74 4C 8B 40 18 85 C0 74 45 8B 90 88 00 00 00 56 33 F6 85 D2");
        if (focus.empty() || notify.empty() || highlight.empty() || open.empty() || credits.empty() || title.empty() || buttonEnabled.empty())
            return false;
        if (!FEPkgMgr_FindPackage || !FEngFindObject || !FEngSetInvisible || !FEHashUpper || !cFEng::pInstance)
            return false;
        SetButtonEnabled = buttonEnabled.get_first();

        // The menu's category is the options screen's, the entries open MU_Options_PC.fng and Credits.fng
        if (*focus.get_first<int32_t*>(1) != Category || *credits.get_first<int32_t*>(11) != Category)
            return false;
        OptionsPackage = *open.get_first<const char*>(5);
        CreditsPackage = *credits.get_first<const char*>(5);
        if (!OptionsPackage || _stricmp(OptionsPackage, "MU_Options_PC.fng") != 0 || !CreditsPackage || _stricmp(CreditsPackage, "Credits.fng") != 0)
            return false;

        SetLanguageHash = injector::GetBranchDestination(highlight.get_first(17)).get_raw<void>();
        OpenPackage = injector::GetBranchDestination(open.get_first(9)).get_raw<void>();
        if (injector::GetBranchDestination(credits.get_first(19)).get_raw<void>() != OpenPackage)
            return false;
        PrintfByHash = injector::GetBranchDestination(title.get_first(33)).get_raw<void>();
        TitleHash = *title.get_first<uint32_t>(29);
        auto searchForString = injector::GetBranchDestination(title.get_first(5)).get_raw<void>();
        if (!SetLanguageHash || !OpenPackage || !PrintfByHash || !searchForString)
            return false;

        LabelHash = bStringHash("WFP_PLUGINS_LABEL");
        DescriptionHash = bStringHash("WFP_PLUGINS_DESCRIPTION");
        PauseLabelHash = bStringHash("WFP_PLUGINS_PAUSE");

        shSearchForString = safetyhook::create_inline(searchForString, SearchForString);
        shFocusLastCategory = safetyhook::create_inline(focus.get_first(), FocusLastCategory);
        shNotify = safetyhook::create_inline(notify.get_first(), Notify);
        if (shSearchForString && shFocusLastCategory && shNotify)
        {
            GameSearchForString = shSearchForString.original<decltype(GameSearchForString)>();
            return true;
        }
        shSearchForString = {};
        shFocusLastCategory = {};
        shNotify = {};
        return false;
    }

    void Uninstall()
    {
        shNotify = {};
        shFocusLastCategory = {};
        shSearchForString = {};
    }
}

// The pause menu's options list (MU_PauseOptionsSelector.fng): Audio, Camera, Car, Controller, Display and HUD,
// 48 units apart, each a text button with a highlight marker (PulseNN, which the button's highlight response sends the
// highlight message to) and a row background (Button_N); the hidden Debug button follows HUD. A "Plugins"
// entry is made below HUD by copying HUD's button, marker and background one row lower. The pad and the mouse move
// between the package's buttons by their positions, so the copy only has to be in the package's button list. Selecting
// it opens the pause options screen (MU_PauseOptions_PC.fng) with the fix's category.
namespace PauseSelector
{
    constexpr uint32_t MessageHighlight = 0xABC08912;
    constexpr uint32_t MessageOpen = 0x406415E3;            // the menu also opens an entry on this message
    constexpr uint32_t HudName = 0xB120;                    // "Hud", as the menu compares it
    constexpr uint32_t DisplayName = 0x368A0075;            // "Display"
    constexpr uint32_t DebugName = 0x02A053C6;              // "Debug", disabled and hidden by the menu
    constexpr uint32_t HudBackgroundName = 0xCDA0A670;      // "Button_6"
    constexpr int PauseCategoryHud = 5;
    constexpr int32_t TypeString = 2;
    constexpr int32_t TypeGroup = 5;
    constexpr uint32_t FlagInvisible = 0x1;
    constexpr uint32_t FlagDirty = 0x400000;                // the object's state changed: its drawing is made again
    constexpr uint32_t FlagDisabled = 0x4000000;            // not a target of the pad and the mouse

    // FEObject (NFSU's FEng)
    constexpr size_t ObjNext = 0x04, ObjPrev = 0x08, ObjGuid = 0x0C, ObjNameHash = 0x10, ObjType = 0x18, ObjFlags = 0x1C;
    constexpr size_t ObjData = 0x2C, ObjDataSize = 0x30;   // colour (4 ints), pivot, position (+28), rotation, size
    constexpr size_t ObjResponses = 0x3C;                   // head of the message responses
    constexpr size_t ObjScripts = 0x4C;                     // head of the scripts
    constexpr size_t GroupChildren = 0x5C;                  // count, head, tail
    // FEResponse: next +4, message +0x0C, command count +0x10, commands +0x14 (command, two parameters; command 1
    // sends the message in the first parameter to the object with the id in the second)
    // FEScript: next +4, track count +0x1C, tracks +0x20 (0x38 bytes: value type +0, offset in the object's state in
    // dwords +7, base key value +0x18, which the other keys are added to), event count +0x24, events +0x28 (message,
    // object id, time)
    // FEPackage: objects (count, head, tail) +0x3C, buttons +0x84 and their count +0x88
    constexpr size_t PkgObjects = 0x3C, PkgButtons = 0x84, PkgButtonCount = 0x88;

    template<typename T> T& At(uint8_t* base, size_t offset) { return *reinterpret_cast<T*>(base + offset); }

    const char* SelectorPackage = "MU_PauseOptionsSelector.fng";
    uint32_t EntryName = 0;     // the copy's name hash
    uint32_t LabelHash = 0;
    void* (__cdecl* GameMalloc)(size_t) = nullptr;
    void (__cdecl* GameFree)(void*) = nullptr;

    uint8_t* FindPackage(const char* name)
    {
        uint8_t* result = nullptr;
        auto fn = FEPkgMgr_FindPackage;
        __asm
        {
            mov eax, name
            call fn
            mov result, eax
        }
        return result;
    }

    // An object of the package by its id, also inside groups
    uint8_t* FindById(uint8_t* head, uint32_t guid)
    {
        for (auto object = head; object; object = At<uint8_t*>(object, ObjNext))
        {
            if (At<uint32_t>(object, ObjGuid) == guid)
                return object;
            if (At<int32_t>(object, ObjType) == TypeGroup)
            {
                if (auto found = FindById(At<uint8_t*>(object, GroupChildren + 4), guid))
                    return found;
            }
        }
        return nullptr;
    }

    uint8_t* FindByName(uint8_t* package, uint32_t name)
    {
        for (auto object = At<uint8_t*>(package, PkgObjects + 4); object; object = At<uint8_t*>(object, ObjNext))
        {
            if (At<uint32_t>(object, ObjNameHash) == name)
                return object;
        }
        return nullptr;
    }

    // A button's highlight marker: where its highlight response sends the highlight message
    uint8_t* HighlightMarker(uint8_t* package, uint8_t* button)
    {
        for (auto response = At<uint8_t*>(button, ObjResponses); response; response = At<uint8_t*>(response, 4))
        {
            if (At<uint32_t>(response, 0x0C) != MessageHighlight)
                continue;
            int count = At<int32_t>(response, 0x10);
            auto commands = At<uint32_t*>(response, 0x14);
            for (int i = 0; commands && i < count; ++i)
            {
                if (commands[i * 3] == 1 && commands[i * 3 + 1] == MessageHighlight)
                    return FindById(At<uint8_t*>(package, PkgObjects + 4), commands[i * 3 + 2]);
            }
        }
        return nullptr;
    }

    bool HasPosition(uint8_t* object)
    {
        return object && At<uint8_t*>(object, ObjData) && At<uint32_t>(object, ObjDataSize) >= 40;
    }

    float& PositionY(uint8_t* object)
    {
        return *reinterpret_cast<float*>(At<uint8_t*>(object, ObjData) + 32);
    }

    // Marks an object and its children as changed, so their drawing is made again from the new state
    void MarkDirty(uint8_t* object)
    {
        At<uint32_t>(object, ObjFlags) |= FlagDirty;
        if (At<int32_t>(object, ObjType) == TypeGroup)
        {
            for (auto child = At<uint8_t*>(object, GroupChildren + 4); child; child = At<uint8_t*>(child, ObjNext))
                MarkDirty(child);
        }
    }

    // Moves an object down by dy: its position, and the base key of its scripts' position tracks, which the
    // animations are relative to
    void MoveY(uint8_t* object, float dy)
    {
        PositionY(object) += dy;
        for (auto script = At<uint8_t*>(object, ObjScripts); script; script = At<uint8_t*>(script, 4))
        {
            int count = At<int32_t>(script, 0x1C);
            auto tracks = At<uint8_t*>(script, 0x20);
            for (int i = 0; tracks && i < count; ++i)
            {
                auto track = tracks + i * 0x38;
                if (track[0] == 4 && track[7] == 7)     // a 3D vector at the position
                    *reinterpret_cast<float*>(track + 0x1C) += dy;
            }
        }
        MarkDirty(object);
    }

    // FEObject::Clone (vtable slot 1, 0 = the copy gets its own animation tracks); groups copy their children.
    // The copy's state starts at zero, so it's copied from the original.
    uint8_t* Clone(uint8_t* object)
    {
        using CloneFn = uint8_t* (__thiscall*)(uint8_t*, int);
        return reinterpret_cast<CloneFn>((*reinterpret_cast<void***>(object))[1])(object, 0);
    }

    void CopyState(uint8_t* from, uint8_t* to)
    {
        auto data = At<uint8_t*>(from, ObjData);
        auto copyData = At<uint8_t*>(to, ObjData);
        if (data && copyData && At<uint32_t>(from, ObjDataSize) == At<uint32_t>(to, ObjDataSize))
            memcpy(copyData, data, At<uint32_t>(from, ObjDataSize));
        if (At<int32_t>(from, ObjType) == TypeGroup && At<int32_t>(to, ObjType) == TypeGroup)
        {
            auto child = At<uint8_t*>(from, GroupChildren + 4);
            auto copy = At<uint8_t*>(to, GroupChildren + 4);
            for (; child && copy; child = At<uint8_t*>(child, ObjNext), copy = At<uint8_t*>(copy, ObjNext))
                CopyState(child, copy);
        }
    }

    // The copies get ids of their own (the game's are small numbers), and what refers to the originals by id in
    // their scripts' events and their responses refers to the copies instead. FEObject::Clone gives the copy a new
    // running number as its id, not the original's, so the originals' ids are taken from the originals.
    uint32_t NextGuid = 0x57460000;
    void NewIds(uint8_t* original, uint8_t* copy, std::map<uint32_t, uint32_t>& ids)
    {
        ids[At<uint32_t>(original, ObjGuid)] = NextGuid;
        At<uint32_t>(copy, ObjGuid) = NextGuid++;
        if (At<int32_t>(original, ObjType) == TypeGroup && At<int32_t>(copy, ObjType) == TypeGroup)
        {
            auto child = At<uint8_t*>(original, GroupChildren + 4);
            auto childCopy = At<uint8_t*>(copy, GroupChildren + 4);
            for (; child && childCopy; child = At<uint8_t*>(child, ObjNext), childCopy = At<uint8_t*>(childCopy, ObjNext))
                NewIds(child, childCopy, ids);
        }
    }

    void RemapIds(uint8_t* object, const std::map<uint32_t, uint32_t>& ids)
    {
        auto remap = [&](uint32_t& value)
        {
            if (auto it = ids.find(value); it != ids.end())
                value = it->second;
        };
        for (auto script = At<uint8_t*>(object, ObjScripts); script; script = At<uint8_t*>(script, 4))
        {
            int count = At<int32_t>(script, 0x24);
            auto events = At<uint32_t*>(script, 0x28);
            for (int i = 0; events && i < count; ++i)
                remap(events[i * 3 + 1]);
        }
        for (auto response = At<uint8_t*>(object, ObjResponses); response; response = At<uint8_t*>(response, 4))
        {
            int count = At<int32_t>(response, 0x10);
            auto commands = At<uint32_t*>(response, 0x14);
            for (int i = 0; commands && i < count; ++i)
            {
                remap(commands[i * 3 + 1]);
                remap(commands[i * 3 + 2]);
            }
        }
        if (At<int32_t>(object, ObjType) == TypeGroup)
        {
            for (auto child = At<uint8_t*>(object, GroupChildren + 4); child; child = At<uint8_t*>(child, ObjNext))
                RemapIds(child, ids);
        }
    }

    // Puts an object into the package's list after another one, so it's drawn in the same layer
    void InsertAfter(uint8_t* package, uint8_t* after, uint8_t* object)
    {
        auto next = At<uint8_t*>(after, ObjNext);
        At<uint8_t*>(object, ObjNext) = next;
        At<uint8_t*>(object, ObjPrev) = after;
        if (next)
            At<uint8_t*>(next, ObjPrev) = object;
        else
            At<uint8_t*>(package, PkgObjects + 8) = object;
        At<uint8_t*>(after, ObjNext) = object;
        ++At<int32_t>(package, PkgObjects);
    }

    void Setup(uint8_t* screen)
    {
        auto packageName = PackageName(screen);
        if (!packageName || _stricmp(packageName, SelectorPackage) != 0)
            return;
        auto package = FindPackage(packageName);
        if (!package)
            return;

        auto buttons = At<uint8_t**>(package, PkgButtons);
        int count = At<int32_t>(package, PkgButtonCount);
        if (!buttons || count <= 0 || count > 64)
            return;
        uint8_t *hud = nullptr, *display = nullptr, *debug = nullptr;
        for (int i = 0; i < count; ++i)
        {
            if (!buttons[i])
                return;
            auto name = At<uint32_t>(buttons[i], ObjNameHash);
            if (name == EntryName)
                return;     // a package that's still loaded has its entry
            if (name == HudName)
                hud = buttons[i];
            else if (name == DisplayName)
                display = buttons[i];
            else if (name == DebugName)
                debug = buttons[i];
        }

        // The row below HUD has to be free: Debug, which is there, is hidden and disabled by the menu
        if (!hud || !display || At<int32_t>(hud, ObjType) != TypeString || !HasPosition(hud) || !HasPosition(display))
            return;
        if (At<uint32_t>(hud, ObjFlags) & (FlagInvisible | FlagDisabled))
            return;
        if (debug && !(At<uint32_t>(debug, ObjFlags) & FlagDisabled))
            return;
        auto marker = HighlightMarker(package, hud);
        auto background = FindByName(package, HudBackgroundName);
        if (!HasPosition(marker) || !HasPosition(background))
            return;
        float step = PositionY(hud) - PositionY(display);
        if (step < 20.0f || step > 80.0f)
            return;

        auto newButtons = static_cast<uint8_t**>(GameMalloc(sizeof(uint8_t*) * (count + 1)));
        if (!newButtons)
            return;
        auto button = Clone(hud);
        auto markerCopy = button ? Clone(marker) : nullptr;
        auto backgroundCopy = markerCopy ? Clone(background) : nullptr;
        if (!button || !markerCopy || !backgroundCopy)
            return;     // not added to the package: unused
        CopyState(hud, button);
        CopyState(marker, markerCopy);
        CopyState(background, backgroundCopy);
        std::map<uint32_t, uint32_t> ids;
        NewIds(hud, button, ids);
        NewIds(marker, markerCopy, ids);
        NewIds(background, backgroundCopy, ids);
        RemapIds(button, ids);
        RemapIds(markerCopy, ids);
        RemapIds(backgroundCopy, ids);
        At<uint32_t>(button, ObjNameHash) = EntryName;

        // The fix moves the pause menu's objects to the side of a wide screen by their positions; the copies are
        // moved like their originals
        for (auto [from, to] : { std::pair{ hud, button }, { marker, markerCopy }, { background, backgroundCopy } })
        {
            auto position = reinterpret_cast<float*>(At<uint8_t*>(from, ObjData) + 28);
            AddHudLayoutCopy(position[0], position[1], position[0], position[1] + step);
            MoveY(to, step);
        }
        Selector::SetText(button, LabelHash);

        // Into the package after the originals, and into the buttons. The game allocated the button array with its
        // malloc and frees it with its free.
        InsertAfter(package, hud, button);
        InsertAfter(package, marker, markerCopy);
        InsertAfter(package, background, backgroundCopy);
        std::copy_n(buttons, count, newButtons);
        newButtons[count] = button;
        At<uint8_t**>(package, PkgButtons) = newButtons;
        At<int32_t>(package, PkgButtonCount) = count + 1;
        GameFree(buttons);
    }

    // The menu screen's constructor
    SafetyHookInline shConstruct = {};
    void* __stdcall Construct(uint8_t* screen, void* parameters)
    {
        auto result = shConstruct.unsafe_stdcall<void*>(screen, parameters);
        Setup(screen);
        return result;
    }

    // The menu's messages. Opening an entry: the menu sets the pause options' category by the entry's name, pushes
    // the pause options screen for the player's controller and hides itself (sub_4C2830). The copy's name isn't
    // the menu's, so the fix does the same with its category; every other entry, HUD too, is the game's.
    void* PushPackage = nullptr;            // usercall: eax = package; layer, controller mask
    void* ControllerMask = nullptr;         // usercall: eax = controller index, returns its mask
    const char* PauseOptionsPackage = nullptr;
    SafetyHookInline shNotify = {};
    void __fastcall Notify(uint8_t* screen, void* edx, uint32_t message, uint8_t* object, int a4, int a5)
    {
        bool entry = object && At<uint32_t>(object, ObjNameHash) == EntryName && (message == MessageAccept || message == MessageOpen);
        auto raceCoordinator = GameFlowManager::pRaceCoordinator ? reinterpret_cast<uint8_t*>(*GameFlowManager::pRaceCoordinator) : nullptr;
        if (!entry || !raceCoordinator)
            return shNotify.unsafe_fastcall(screen, edx, message, object, a4, a5);

        *PauseCategory = PauseCategoryAdvanced;
        int controller = At<int32_t>(raceCoordinator, 0x1C);
        int mask = 0;
        auto toMask = ControllerMask;
        __asm
        {
            mov eax, controller
            call toMask
            mov mask, eax
        }
        auto pushPackage = PushPackage;
        auto name = PauseOptionsPackage;
        __asm
        {
            push mask
            push 20
            mov eax, name
            call pushPackage
            add esp, 8
        }
        if (auto package = FindPackage(PackageName(screen)))
            At<int32_t>(package, 0x1C) = 0;
    }

    // Checks everything before anything is hooked; if something doesn't match, the pause menu stays as it is
    bool Install()
    {
        auto ctor = hook::pattern("C7 06 ? ? ? ? 89 6E 40 8B 41 1C 3B C5");
        auto mapping = hook::pattern("3D C6 53 A0 02 77 ? 74 ? 3D 55 99 00 00 74 ? 3D 20 B1 00 00 74");
        auto rows = hook::pattern("C7 05 ? ? ? ? 06 00 00 00 C6 86 1C 02 00 00 01 56 E8 ? ? ? ? E8");
        auto rowShow = hook::pattern("83 EC 20 55 8B 6C 24 28 56 57 55 68 ? ? ? ? 6A 20 8D 74 24 18 E8 ? ? ? ? 8B 7B 0C 8D 54 24 18 E8");
        auto memory = hook::pattern("8B 96 84 00 00 00 81 C6 84 00 00 00 52 E8 ? ? ? ? 83 C4 04 85 FF C7 06 00 00 00 00 74 ? 8D 04 BD 00 00 00 00 50 E8");
        auto open = hook::pattern("8B 41 1C E8 ? ? ? ? 50 6A 14 B8 ? ? ? ? E8 ? ? ? ? 8B 46 0C");
        if (ctor.empty() || mapping.empty() || rows.empty() || rowShow.empty() || memory.empty() || open.empty())
            return false;
        ControllerMask = injector::GetBranchDestination(open.get_first(3)).get_raw<void>();
        PushPackage = injector::GetBranchDestination(open.get_first(16)).get_raw<void>();
        PauseOptionsPackage = *open.get_first<const char*>(12);
        if (!ControllerMask || !PushPackage || !PauseOptionsPackage || _stricmp(PauseOptionsPackage, "MU_PauseOptions_PC.fng") != 0)
            return false;
        if (!GameFlowManager::pRaceCoordinator)
            return false;
        if (!FEPkgMgr_FindPackage || !FEngFindObject || !Selector::SetLanguageHash || !SetButtonEnabled || !Printf)
            return false;

        // The constructor starts 0x30 bytes before its vtable store; the vtable's notification has the name mapping
        auto ctorStart = ctor.get_first<uint8_t>(-0x30);
        static const uint8_t prologue[] = { 0x64, 0xA1, 0x00, 0x00, 0x00, 0x00, 0x6A, 0xFF, 0x68 };
        if (memcmp(ctorStart, prologue, sizeof(prologue)) != 0)
            return false;
        auto vtable = *ctor.get_first<uint8_t**>(2);
        if (!vtable)
            return false;
        auto notify = reinterpret_cast<uint8_t*>(reinterpret_cast<void**>(vtable)[2]);
        auto mappingAt = mapping.get_first<uint8_t>();
        auto openAt = open.get_first<uint8_t>();
        if (!notify || mappingAt < notify || mappingAt > notify + 0x100 || openAt < mappingAt || openAt > mappingAt + 0x100)
            return false;

        PauseCategory = *rows.get_first<int32_t*>(2);
        PauseRowShow = rowShow.get_first();
        GameFree = reinterpret_cast<decltype(GameFree)>(injector::GetBranchDestination(memory.get_first(13)).get_raw<void>());
        GameMalloc = reinterpret_cast<decltype(GameMalloc)>(injector::GetBranchDestination(memory.get_first(39)).get_raw<void>());
        auto pauseRowsReady = injector::GetBranchDestination(rows.get_first(23)).get_raw<void>();
        if (!PauseCategory || !GameFree || !GameMalloc || !pauseRowsReady)
            return false;

        EntryName = HashUpper("WFP_PAUSEADVANCEDOPTIONS");
        LabelHash = Selector::PauseLabelHash;

        shConstruct = safetyhook::create_inline(ctorStart, Construct);
        shNotify = safetyhook::create_inline(notify, Notify);
        static SafetyHookMid shRowsReady = {};
        shRowsReady = safetyhook::create_mid(pauseRowsReady, [](SafetyHookContext& regs)
        {
            AddPauseRows(reinterpret_cast<void*>(regs.esi));
        });
        if (shConstruct && shNotify && shRowsReady)
            return true;
        shConstruct = {};
        shNotify = {};
        shRowsReady = {};
        return false;
    }
}

class Options
{
public:
    Options()
    {
        WFP::onInitEvent() += []()
        {
            auto category = hook::pattern("A1 ? ? ? ? 83 F8 0A 0F 87 ? ? ? ? FF 24 85 ? ? ? ? 56 E8");
            auto rowShow = hook::pattern("83 EC 20 55 8B 6C 24 28 56 57 55 68 ? ? ? ? 6A 20 8D 74 24 18 E8 ? ? ? ? 8B 7B 0C 83 C4 0C 8D 54 24 0C E8");
            auto arrows = hook::pattern("6A 00 55 BB 01 00 00 00 E8 ? ? ? ? 8B FB 57 55 BB 02 00 00 00 E8");
            auto printf = hook::pattern("56 57 8B F0 E8 ? ? ? ? 85 F6 8B F8 74 18 8B C6 E8 ? ? ? ? 85 C0 74 0D 8B CF 8B D0 E8 ? ? ? ? 85 C0 75 14 A1 ? ? ? ? 85 C0 74 27");
            auto font = hook::pattern("56 8B F0 57 B8 ? ? ? ? E8 ? ? ? ? 8B 3D ? ? ? ? 8B D0 8B C7 3B C2 74 ? 8D 64 24 00 39 70 14");
            auto width = hook::pattern("83 EC 10 53 8B 5C 24 1C 85 DB 75 0D D9 05 ? ? ? ? 5B 83 C4 10 C2 0C 00 66 8B 03 83 C3 02");
            auto sound = hook::pattern("57 8B F8 39 3C FD ? ? ? ? 74 14 33 C0");
            auto language = hook::pattern("8B 0D ? ? ? ? 57 8B F8 3B F9 0F 84 ? ? ? ? 53 33 DB 83 F9 FF 0F 84 ? ? ? ? 56 33 F6 B8");
            auto currentButton = hook::pattern("57 68 ? ? ? ? 8B F9 E8 ? ? ? ? 85 C0 0F 84 ? ? ? ? 53 56 8B 70 18 85 F6 0F 84 ? ? ? ? 8B 86 88 00 00 00");
            auto mainNotify = hook::pattern("83 EC 38 53 57 8B F9 8A 87 E4 04 00 00 33 DB");
            auto pauseNotify = hook::pattern("8B 44 24 04 3D F1 1B 97 B5 57 8B F9 0F 87");
            if (category.empty() || rowShow.empty() || arrows.empty() || printf.empty() || sound.empty() || language.empty() ||
                currentButton.empty() || mainNotify.empty() || font.size() != 1 || width.size() != 1)
                return;

            Category = *category.get_first<int32_t*>(1);
            RowShow = rowShow.get_first();
            ShowObject = injector::GetBranchDestination(rowShow.get_first(0x25)).get_raw<void>();
            RowArrows = injector::GetBranchDestination(arrows.get_first(8)).get_raw<void>();
            Printf = printf.get_first();
            FontLookup = font.get_first();
            FontWidth = reinterpret_cast<decltype(FontWidth)>(width.get_first());
            PlaySound = sound.get_first();
            SetCurrentButton = currentButton.get_first();
            CurrentLanguage = *language.get_first<int32_t*>(2);
            Languages = *language.get_first<LanguageInfo*>(0x21);
            LanguagesEnd = reinterpret_cast<LanguageInfo*>(CurrentLanguage);
            if (!ShowObject || !RowArrows)
                return;

            FixSettings().Api()->Subscribe([](void*, const char*) { OwnSettingsChanged = true; }, nullptr);
            onMenuScreenBaseNotify() += [](MenuScreen* screen)
            {
                // A live screen notification keeps editor changes on the game thread, without retaining a screen
                // pointer after the package is closed. Native categories keep their own rows.
                if (Plugins && IsShown(screen, *Active, true))
                {
                    Refresh(screen);
                    RefreshNotes(screen);
                    if (GameRowsFrom)
                        RefreshJoinedHelp(screen);
                    else
                        HideJoinedHelp(screen, *Active);
                }
                else if (screen && screen->PackageFilename && _stricmp(screen->PackageFilename, "MU_Options_PC.fng") == 0)
                    HideJoinedHelp(screen, MainMenu);
                else if (screen && screen->PackageFilename && _stricmp(screen->PackageFilename, "MU_PauseOptions_PC.fng") == 0)
                    HideJoinedHelp(screen, PauseMenu);
            };

            // The menu entry; without it the settings remain available through the Hub and CFG
            if (!Selector::Install())
                return;

            // The rows' mouse areas are set up after the category's rows
            static auto shRowsReady = safetyhook::create_mid(injector::GetBranchDestination(category.get_first(0x1B)).get_raw<void>(), [](SafetyHookContext& regs)
            {
                AddRows(reinterpret_cast<void*>(regs.esi));
            });
            shMainNotify = safetyhook::create_inline(mainNotify.get_first(), MainNotify);
            if (!shRowsReady || !shMainNotify)
            {
                shMainNotify = {};
                shRowsReady = {};
                Selector::Uninstall();
                return;
            }
            GameRowsInstalled = true;

            // The pause menu's entry, which uses the main menu's text lookup; without it the pause menu stays as it is
            if (!pauseNotify.empty())
            {
                shPauseNotify = safetyhook::create_inline(pauseNotify.get_first(), PauseNotify);
                if (shPauseNotify && !PauseSelector::Install())
                    shPauseNotify = {};
            }
        };
    }
} Options;
