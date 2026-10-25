module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "StaticText.hpp"
#include "Help.hpp"
#include "../Shared/NFS/PluginsMenu.hpp"
#include "../Shared/NFS/MenuOwner.hpp"
#include "../Shared/NFS/MenuLayout.hpp"
#include "../Shared/NFS/LegacyText.hpp"
#include "../Shared/NFS/Text.hpp"
#include "../Shared/NFS/TextLanguage.hpp"

export module Options;

import ComVars;
import Input;

// One category of the game's options menu, "Plugins" (next to Audio, Video, Gameplay, Player and Controls), in the main
// menu and in the pause menu. It opens the game's options screen with a row for each plugin with settings, the fix
// first; accept opens the plugin's page on the same screen (its Preset row, its settings, Restore Defaults) and back
// returns to the list. Every option of the game is an object of its own class, so the rows are objects of the game's
// toggle class with a class of their own: left and right change the setting, accept acts on the Preset, Restore
// Defaults and action rows. Labels and values come from each plugin's texts in the game's language.

namespace
{
    // The options screen shows the options of a type: 0 audio, 1 video, 3 gameplay, 4 player, 9 online. The fix's
    // category is a type of its own.
    constexpr int32_t FixOptionsType = 10;
    uintptr_t* OptionsDatabase = nullptr;   // the front end database; +0x10 the options settings, the type at +0x24

    int32_t& OptionsType()
    {
        return *reinterpret_cast<int32_t*>(*reinterpret_cast<uintptr_t*>(*OptionsDatabase + 0x10) + 0x24);
    }

    // Text
    std::unordered_map<uint32_t, std::string> Texts;     // text hash -> text in the font's encoding

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
        void* unknown[2];
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

    void UpdateLanguage()
    {
        LanguageName = WFPMenu::TextLanguage(FixSettings(), FindLanguageName(CurrentLanguage ? *CurrentLanguage : 0));
        // The 8-bit fonts' Cyrillic encoding differs between the Russian releases: Russian menus show the English texts
        if (LanguageName == "russian")
            LanguageName = "english";
        WFPText::SetLanguage(LanguageName);
    }

    // UTF-8 text as the options screen draws it: in capitals, like the game's rows
    std::string GameText(const std::string& text)
    {
        return WFPText::ToEAFont(WFPText::Upper(text), LanguageName);
    }

    void SetText(uint32_t hash, const std::string& text)
    {
        Texts[hash] = GameText(text);
    }

    // SearchForString: the lookup of every text by its hash
    SafetyHookInline shSearchForString = {};
    const char* __fastcall SearchForString(void* ecx, uint32_t hash)
    {
        if (auto it = Texts.find(hash); it != Texts.end())
            return it->second.c_str();
        return shSearchForString.unsafe_fastcall<const char*>(ecx, hash);
    }

    // The game's options
    void* (__cdecl* GameMalloc)(size_t size) = nullptr;
    void* (__fastcall* ToggleOption)(void* option, void* edx, bool enabled) = nullptr;
    void (__fastcall* AddOption)(void* screen, void* edx, void* option, int unknown) = nullptr;
    void (__cdecl* SetLanguageHash)(void* object, uint32_t hash) = nullptr;
    void* (__fastcall* RowObject)(void* screen, void* edx, const char* prefix) = nullptr;    // the next option's title
    void (__fastcall* ClearOptions)(void* screen, void* edx) = nullptr;                     // deletes the screen's options
    void (__fastcall* FinishOptions)(void* screen, void* edx, int highlight) = nullptr;     // lays them out, highlights one (1-based, 0 = first)

    // The screen's layout has a row of objects for each option: OPTION_NAME_1, OPTION_DATA_1, LEFT_ARROW_1 and so on,
    // with the prefixes in the screen. The options screen has ten rows, so the fix makes more by copying the last one.
    // The copies of the arrows show at the row they were copied from, so the added rows share the last row's arrows,
    // which move to the highlighted option.
    void** FEngInstance = nullptr;
    uint8_t* (__fastcall* FindPackage)(void* feng, void* edx, const char* name) = nullptr;
    void (__fastcall* BuildMouseObjects)(uint8_t* package, void* edx) = nullptr;   // the package's list of clickable objects
    void (__fastcall* BindAnimationTracks)(void* object, void* edx) = nullptr;

    struct FEList
    {
        int32_t count;
        uint8_t* head;
        uint8_t* tail;
    };

    // An object of the package by its name's hash, and the list it's in
    uint8_t* FindObject(FEList* list, uint32_t hash, FEList** owner)
    {
        for (auto node = list->head; node; node = *reinterpret_cast<uint8_t**>(node + 4))
        {
            if (*reinterpret_cast<uint32_t*>(node + 0x10) == hash)
            {
                *owner = list;
                return node;
            }
            if (*reinterpret_cast<int32_t*>(node + 0x18) == 5)  // a group
            {
                if (auto found = FindObject(reinterpret_cast<FEList*>(node + 0x60), hash, owner))
                    return found;
            }
        }
        return nullptr;
    }

    const char* RowPrefix(void* screen, int index)
    {
        auto prefix = *reinterpret_cast<const char**>(reinterpret_cast<uintptr_t>(screen) + 0xB0 + index * 4);
        return prefix && *prefix ? prefix : nullptr;
    }

    constexpr int ArrowPrefix = 4;  // LEFT_ARROW_, then RIGHT_ARROW_

    int& RowCount(void* screen)
    {
        return *reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(screen) + 0x104);   // the next option's row
    }

    bool HasRoom(void* screen)
    {
        auto title = RowPrefix(screen, 0);
        auto value = RowPrefix(screen, 1);
        return title && value && RowObject(screen, nullptr, title) && RowObject(screen, nullptr, value);
    }

    uint8_t* ScreenPackage(void* screen)
    {
        return FindPackage(*FEngInstance, nullptr, *reinterpret_cast<const char**>(reinterpret_cast<uintptr_t>(screen) + 0x10));
    }

    bool AddRow(void* screen)
    {
        if (!BindAnimationTracks)
            return false;
        auto package = ScreenPackage(screen);
        if (!package)
            return false;

        int row = RowCount(screen);
        struct Copy
        {
            uint8_t* original;
            uint8_t* object;
            FEList* owner;
        };
        std::vector<Copy> copies;
        auto discard = [&]()
        {
            for (auto& copy : copies)
                reinterpret_cast<void(__thiscall*)(void*, int)>((*reinterpret_cast<void***>(copy.object))[0])(copy.object, 1);
        };
        for (int i = 0; i < 7; ++i)
        {
            auto prefix = RowPrefix(screen, i);
            if (!prefix && i < 2)
            {
                discard();
                return false;
            }
            if (!prefix || i == ArrowPrefix || i == ArrowPrefix + 1)
                continue;

            FEList* list = nullptr;
            auto object = FindObject(reinterpret_cast<FEList*>(package + 0x48), bStringHash((prefix + std::to_string(row - 1)).c_str()), &list);
            if (!object)
            {
                if (i < 2)
                {
                    discard();
                    return false;
                }
                continue;
            }
            auto hash = bStringHash((prefix + std::to_string(row)).c_str());
            FEList* existingOwner = nullptr;
            if (FindObject(reinterpret_cast<FEList*>(package + 0x48), hash, &existingOwner))
            {
                discard();
                return false;
            }

            // FEObject::Clone, then the copy gets the next row's name and goes at the end of the same list
            auto copy = reinterpret_cast<uint8_t*(__fastcall*)(uint8_t*, void*, bool)>((*reinterpret_cast<void***>(object))[1])(object, nullptr, false);  // false: its own copy of the animation tracks
            if (!copy)
            {
                discard();
                return false;
            }
            *reinterpret_cast<uint32_t*>(copy + 0x10) = hash;
            *reinterpret_cast<void**>(copy + 0x58) = nullptr;     // render data, made when it's first drawn; the copy leaves it unset

            // The object's current state (position, size, colour), which the copy starts at zero
            auto data = *reinterpret_cast<void**>(object + 0x2C);
            auto copyData = *reinterpret_cast<void**>(copy + 0x2C);
            if (data && copyData && *reinterpret_cast<uint32_t*>(object + 0x30) == *reinterpret_cast<uint32_t*>(copy + 0x30))
                memcpy(copyData, data, *reinterpret_cast<uint32_t*>(object + 0x30));
            // Native cloning binds relative keys before its zeroed state receives the current row position.
            if (*reinterpret_cast<void**>(copy + 0x54))
                BindAnimationTracks(copy, nullptr);
            copies.push_back({ object, copy, list });
        }

        // The animations send events to the row's other objects (the name's highlight shows the arrows) by the
        // objects' ids: the copies' events go to the copies
        for (auto& [object, copy, list] : copies)
        {
            for (auto script = *reinterpret_cast<uint8_t**>(copy + 0x4C); script; script = *reinterpret_cast<uint8_t**>(script + 4))
            {
                int count = *reinterpret_cast<int*>(script + 0x24);
                auto events = *reinterpret_cast<uint32_t**>(script + 0x28);
                for (int e = 0; events && e < count; ++e)
                {
                    auto& target = events[e * 3 + 1];
                    for (auto& [from, to, targetList] : copies)
                    {
                        if (target == *reinterpret_cast<uint32_t*>(from + 0x0C))
                        {
                            target = *reinterpret_cast<uint32_t*>(to + 0x0C);
                            break;
                        }
                    }
                }
            }
        }
        // All objects and event targets are ready before the row becomes visible to the package.
        for (auto& [object, copy, list] : copies)
        {
            *reinterpret_cast<uint8_t**>(copy + 4) = nullptr;
            *reinterpret_cast<uint8_t**>(copy + 8) = list->tail;
            if (list->tail)
                *reinterpret_cast<uint8_t**>(list->tail + 4) = copy;
            else
                list->head = copy;
            list->tail = copy;
            ++list->count;
        }
        return !copies.empty();
    }

    constexpr size_t OptionSize = 0x5C;
    constexpr int32_t MessageLeft = -1860157282;
    constexpr int32_t MessageRight = -1248388111;
    constexpr int32_t MessageAccept = 0x0C407210;
    constexpr uint32_t MessageBack = 0x911AB364;

    // The screen's rows: a plugin of the list, or a row of a plugin's page
    struct MenuRow
    {
        int plugin = -1;            // the list: the plugin the row opens
        WFPPlugins::Row row;        // a plugin's page
        bool enabled = true;
        void* option = nullptr;     // the option showing the row
        std::string label;
        std::string value;
        std::string help;
    };

    std::shared_ptr<WFPPlugins::Snapshot> Plugins;     // the plugins found when the category was opened
    int ShownPlugin = -1;                               // -1: the list of plugins
    int ListPlugin = 0;                                 // the list's row to highlight
    std::vector<MenuRow> Rows;                          // the open screen's
    WFPMenu::Owner ScreenOwner;
    WFPMenu::Owner RowsOwner;
    using ScreenToken = WFPMenu::Owner::Token;
    enum class ScreenKind { Plugins, Video } ShownKind = ScreenKind::Plugins;
    struct BuildRequest
    {
        ScreenToken lifetime;
        ScreenKind kind{};
    };
    BuildRequest PendingBuild;


    // The fix's settings that go next to the game's own video options (the basic list, then the advanced one); the
    // fix's page leaves them out
    const std::vector<std::string> VideoBasic = { "MAIN.WindowMode", "MAIN.FOVFactor", "MAIN.HudAspectRatioConstraint", "GRAPHICS.ConsoleGamma" };
    const std::vector<std::string> VideoAdvanced = { "GRAPHICS.AntiAliasing", "GRAPHICS.Sharpness", "GRAPHICS.ShadowResolution", "GRAPHICS.SoftShadows" };
    bool VideoAdvancedShown = false;
    constexpr uint32_t InGameOffset = 292;
    bool VideoPauseShown{};
    std::array<std::array<std::vector<std::string>, 2>, 2> JoinedVideo;

    std::vector<std::string> JoinedVideoSettings()
    {
        auto ids = JoinedVideo[size_t(VideoPauseShown)][0];
        const auto& advanced = JoinedVideo[size_t(VideoPauseShown)][1];
        ids.insert(ids.end(), advanced.begin(), advanced.end());
        return ids;
    }

    std::vector<std::string> VideoSettings()
    {
        auto ids = VideoBasic;
        ids.insert(ids.end(), VideoAdvanced.begin(), VideoAdvanced.end());
        return ids;
    }

    HMODULE ThisModule()
    {
        HMODULE self = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCWSTR>(&ThisModule), &self);
        return self;
    }

    // The settings a plugin's page leaves out: the fix's video settings
    std::vector<std::string> Excluded(const WFPPlugins::LoadedPlugin& plugin)
    {
        return plugin.module == ThisModule() ? JoinedVideoSettings() : std::vector<std::string>{};
    }
    WFPPlugins::PresetRow PresetRow;
    WFPPlugins::RestoreRow RestoreRow;
    enum class Pending { None, Rebuild, Open, Back } PendingAction = Pending::None;
    int PendingPlugin = -1;
    int PendingHighlight = 0;
    bool RestartChange = false;     // a change on the open screen waits for a restart
    bool RestartWarned = false;     // the open screen showed the restart warning

    struct InputCommand
    {
        ScreenToken owner;
        size_t row{};
        int32_t message{};
        bool back{};
        bool defaults{};
        ScreenToken lifetime;
        std::shared_ptr<WFPPlugins::Snapshot> plugins;
        int plugin{ -1 };
        std::vector<std::string> settings;
    };
    std::vector<InputCommand> InputCommands;
    unsigned MessageDepth{};
    bool Servicing{};
    void ServiceScreen(bool frame = false);
    void ResetHelp();
    void HideScreenHelp(void* screen);

    bool Current(ScreenToken token)
    {
        return RowsOwner.Matches(token) && ScreenOwner.Get() == token.screen;
    }

    uint32_t LabelHash(size_t row) { return TextHash("ROW_LABEL_" + std::to_string(row)); }
    uint32_t ValueHash(size_t row) { return TextHash("ROW_VALUE_" + std::to_string(row)); }
    const uint32_t TitleHash = TextHash("TITLE");
    const uint32_t CategoryHash = TextHash("WFP_PLUGINS");

    struct Option
    {
        void** vtable;
        uint8_t data[OptionSize - 4];
        size_t row;     // the fix's, after the game's part
        float dataX{};
        float dataWidth{};
        float titleX{};
        bool metricsSaved{};

        void* Title() { return *reinterpret_cast<void**>(reinterpret_cast<uintptr_t>(this) + 0x2C); }
        void* Value() { return *reinterpret_cast<void**>(reinterpret_cast<uintptr_t>(this) + 0x30); }
        uint8_t& Changed() { return *reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(this) + 0x2A); }
    };

    void* OptionVTable[15] = {};
    void* BaseLayout = nullptr;     // positions the option's objects, the arrows too
    void* BaseShow = nullptr;
    void* BaseHide = nullptr;
    void* BaseHighlight = nullptr;
    void* BaseUnhighlight = nullptr;
    void* SharedArrows[2] = {};     // the layout's last row's arrows, used by the added rows
    bool WidthPending{};
    uint64_t WidthRetryAt{};
    Option* ArrowsOwner = nullptr;  // the highlighted option, which has the shared arrows

    std::unordered_map<void*, std::pair<uint32_t, std::string>> PublishedTexts;

    void RetireRows(void* screen)
    {
        auto token = RowsOwner.Capture();
        if (!RowsOwner.Clear(screen))
            return;
        std::erase_if(InputCommands, [&](const InputCommand& command)
        {
            return !command.defaults && command.owner.screen == token.screen && command.owner.generation == token.generation;
        });
        Rows.clear();
        PublishedTexts.clear();
        ResetHelp();
        WidthPending = false;
        ArrowsOwner = nullptr;
        SharedArrows[0] = SharedArrows[1] = nullptr;
        PendingAction = Pending::None;
        PendingPlugin = -1;
        PendingHighlight = 0;
        PresetRow = {};
        RestoreRow = {};
    }

    void ConstructScreen(void* screen)
    {
        PendingBuild = {};
        RetireRows(RowsOwner.Get());
        ScreenOwner.Bind(screen);
    }

    SafetyHookInline shDestroyScreen = {};
    uintptr_t __fastcall DestroyScreen(void* screen, void* edx)
    {
        if (PendingBuild.lifetime.screen == screen)
            PendingBuild = {};
        RetireRows(screen);
        ScreenOwner.Clear(screen);
        return shDestroyScreen.unsafe_fastcall<uintptr_t>(screen, edx);
    }

    SafetyHookInline shClearRows = {};
    uintptr_t __fastcall ClearRows(void* screen, void* edx)
    {
        HideScreenHelp(screen);
        if (PendingBuild.lifetime.screen == screen)
            PendingBuild = {};
        RetireRows(screen);
        return shClearRows.unsafe_fastcall<uintptr_t>(screen, edx);
    }

    void** Arrows(Option* option)
    {
        return reinterpret_cast<void**>(reinterpret_cast<uintptr_t>(option) + 0x4C);
    }

    // Shows, hides or unhighlights an option, leaving the shared arrows alone unless it's the highlighted option
    void CallWithoutSharedArrows(void* method, Option* option)
    {
        auto token = RowsOwner.Capture();
        if (!Current(token))
        {
            reinterpret_cast<void(__fastcall*)(Option*)>(method)(option);
            return;
        }
        auto arrows = Arrows(option);
        void* saved[2] = { arrows[0], arrows[1] };
        if (option != ArrowsOwner && arrows[0] == SharedArrows[0] && arrows[1] == SharedArrows[1])
            arrows[0] = arrows[1] = nullptr;
        reinterpret_cast<void(__fastcall*)(Option*)>(method)(option);
        if (!Current(token))
            return;
        arrows[0] = saved[0];
        arrows[1] = saved[1];
    }

    void __fastcall ShowOption(Option* option)
    {
        CallWithoutSharedArrows(BaseShow, option);
    }

    void __fastcall HideOption(Option* option)
    {
        CallWithoutSharedArrows(BaseHide, option);
    }

    void __fastcall HighlightOption(Option* option, void* edx, const char* package)
    {
        auto token = RowsOwner.Capture();
        if (!Current(token))
            return;
        ArrowsOwner = option;
        reinterpret_cast<void(__fastcall*)(Option*)>(BaseLayout)(option);
        if (Current(token))
            reinterpret_cast<void(__fastcall*)(Option*, void*, const char*)>(BaseHighlight)(option, nullptr, package);
    }

    void __fastcall UnhighlightOption(Option* option)
    {
        auto token = RowsOwner.Capture();
        CallWithoutSharedArrows(BaseUnhighlight, option);
        if (Current(token) && ArrowsOwner == option)
        {
            PresetRow.Cancel();
            RestoreRow.Cancel();
            ArrowsOwner = nullptr;
        }
    }

    // The screen lays out every option when it scrolls: the shared arrows go back to the highlighted one
    void __fastcall LayoutOption(Option* option)
    {
        auto token = RowsOwner.Capture();
        reinterpret_cast<void(__fastcall*)(Option*)>(BaseLayout)(option);
        if (Current(token) && ArrowsOwner && ArrowsOwner != option &&
            Arrows(option)[0] == SharedArrows[0] && Arrows(option)[1] == SharedArrows[1])
            reinterpret_cast<void(__fastcall*)(Option*)>(BaseLayout)(ArrowsOwner);
    }

    const WFPPlugins::LoadedPlugin* PluginAt(int index)
    {
        return Plugins && index >= 0 && index < int(Plugins->plugins.size()) ? &Plugins->plugins[index] : nullptr;
    }

    std::string RowLabel(const MenuRow& row)
    {
        if (row.plugin >= 0)
            return PluginAt(row.plugin) ? WFPPlugins::Name(*PluginAt(row.plugin)) : std::string();
        return WFPPlugins::Label(row.row);
    }

    std::string RowValue(const MenuRow& row)
    {
        if (row.plugin >= 0)
            return {};
        switch (row.row.kind)
        {
            case WFPPlugins::Row::Kind::Preset:
            {
                auto state = PresetRow;
                return state.ValueText();
            }
            case WFPPlugins::Row::Kind::RestoreDefaults:
            {
                auto state = RestoreRow;
                return state.ValueText();
            }
            case WFPPlugins::Row::Kind::Action:
                return {};
            default:
                return WFPPlugins::ValueText(row.row);
        }
    }

    // The screen's title: the category, or the plugin's name; the restart warning when a change waits for one
    void UpdateTitle()
    {
        if (ShownKind != ScreenKind::Plugins)
            return;
        std::string title = ShownPlugin >= 0 && PluginAt(ShownPlugin) ? WFPPlugins::Name(*PluginAt(ShownPlugin)) : WFPText::Get("WFP_PLUGINS");
        SetText(TitleHash, title);
    }

    void* (__cdecl* FindFont)(uint32_t hash) = nullptr;
    double (__thiscall* NativeStringWidth)(void* font, const wchar_t* text, uint32_t flags) = nullptr;
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
        auto fallback = [&]() { return uppercase ? GameText(text) : WFPText::ToEAFont(text, LanguageName); };
        if (!object || !FindFont)
            return fallback();
        auto font = static_cast<uint8_t*>(FindFont(*reinterpret_cast<uint32_t*>(static_cast<uint8_t*>(object) + 0x24)));
        auto header = font ? *reinterpret_cast<uint8_t**>(font + 12) : nullptr;
        if (!header || *reinterpret_cast<uint32_t*>(header) != 0x46544E46)
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
            auto encoded = WFPText::ToEAFont(utf8, LanguageName);
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
            !*reinterpret_cast<uintptr_t*>(static_cast<uint8_t*>(font) + 12) || !std::isfinite(data[14]))
            return std::numeric_limits<float>::quiet_NaN();
        std::vector<wchar_t> wide;
        if (!DecodeText(text, wide))
            return std::numeric_limits<float>::quiet_NaN();
        // The legacy whole-line helper passes a text pointer as the previous character. Measure the
        // renderer's native glyph advances with the actual preceding character instead.
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
        auto scale = NFSMWStaticText::RenderScale(*reinterpret_cast<uint32_t*>(bytes + 0x24), CurrentLanguage ? *CurrentLanguage : 0);
        return std::max(maximum, width) * std::abs(data[14]) * scale;
    }

    void FitOption(Option* option, std::string& label, const std::string& value)
    {
        if (!FindFont || !GlyphAdvance || !LocalizedWide || !NativeStringWidth)
            return;
        if (!option->metricsSaved)
        {
            option->dataX = *reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(option) + 0x44);
            option->dataWidth = *reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(option) + 0x3C);
            option->titleX = *reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(option) + 0x0C);
            option->metricsSaved = true;
        }
        auto valueWidth = MeasureText(option->Value(), value);
        auto labelWidth = MeasureText(option->Title(), label);
        auto arrow = static_cast<uint8_t*>(Arrows(option)[0]);
        auto data = arrow ? *reinterpret_cast<float**>(arrow + 0x2C) : nullptr;
        if (!std::isfinite(valueWidth) || !std::isfinite(labelWidth) || !data ||
            *reinterpret_cast<uint32_t*>(arrow + 0x30) < 68 || !std::isfinite(data[14]) ||
            !std::isfinite(option->dataX) || !std::isfinite(option->dataWidth) ||
            !std::isfinite(option->titleX) || option->dataWidth < 0.0f)
        {
            WidthPending = true;
            return;
        }
        auto width = std::max(option->dataWidth, valueWidth + 8.0f);
        auto x = option->dataX + option->dataWidth - width;
        *reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(option) + 0x44) = x;
        *reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(option) + 0x3C) = width;
        auto available = x - std::abs(data[14]) - 8.0f - option->titleX;
        label = WFPMenuLayout::Fit(std::move(label), available, [&](const std::string& candidate)
        {
            return MeasureText(option->Title(), candidate);
        });
        auto title = static_cast<uint8_t*>(option->Title());
        auto titleState = *reinterpret_cast<float**>(title + 0x2C);
        auto font = FindFont(*reinterpret_cast<uint32_t*>(title + 0x24));
        auto oldText = *reinterpret_cast<const wchar_t**>(title + 0x64);
        // The widget positions the existing literal's center even when the renderer uses a language hash.
        auto pivotWidth = oldText && font ? float(NativeStringWidth(font, oldText, 0)) * std::abs(titleState[14]) : 0.0f;
        auto justify = *reinterpret_cast<uint32_t*>(title + 0x6C);
        auto column = pivotWidth;
        auto titleX = option->titleX;
        if ((justify & 1) != 0)
            titleX += std::max(0.0f, available) * 0.5f;
        else if ((justify & 2) != 0)
            column = std::max(0.0f, available) - pivotWidth * 0.5f;
        *reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(option) + 0x0C) = titleX;
        *reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(option) + 0x34) = column;
        LayoutOption(option);
    }

    bool PublishText(void* object, uint32_t hash, const std::string& text, ScreenToken token)
    {
        if (!object)
            return Current(token);
        auto bytes = static_cast<uint8_t*>(object);
        auto previous = PublishedTexts.find(object);
        bool changed = previous == PublishedTexts.end() || previous->second.first != hash || previous->second.second != text ||
            *reinterpret_cast<uint32_t*>(bytes + 0x60) != hash || (*reinterpret_cast<uint32_t*>(bytes + 0x1C) & 2) != 0;
        Texts[hash] = text;
        if (!changed)
            return Current(token);
        SetLanguageHash(object, hash);
        if (!Current(token))
            return false;
        if (changed)
            *reinterpret_cast<uint32_t*>(bytes + 0x1C) |= 0x2000000;
        PublishedTexts[object] = { hash, text };
        return true;
    }

    void __fastcall DrawOption(Option* option)
    {
        auto token = RowsOwner.Capture();
        if (!Current(token))
            return;
        auto found = std::find_if(Rows.begin(), Rows.end(), [&](const MenuRow& row) { return row.option == option; });
        if (found == Rows.end())
            return;
        auto index = size_t(found - Rows.begin());
        auto plugins = Plugins;
        auto row = *found;
        auto label = row.label;
        auto value = row.value;
        // FEString's authored maximum width otherwise scales long text in the renderer.
        for (auto object : { option->Title(), option->Value() })
        {
            if (object)
            {
                *reinterpret_cast<uint32_t*>(static_cast<uint8_t*>(object) + 0x74) = 0;
                *reinterpret_cast<uint32_t*>(static_cast<uint8_t*>(object) + 0x1C) |= 0x800;
            }
        }
        label = SingleLine(FontText(option->Title(), label));
        value = SingleLine(FontText(option->Value(), value));
        FitOption(option, label, value);
        if (!Current(token))
            return;
        if (PublishText(option->Title(), LabelHash(index), label, token))
            PublishText(option->Value(), ValueHash(index), value, token);
    }

    bool UpdateRowText(size_t index, ScreenToken token)
    {
        if (!Current(token) || index >= Rows.size())
            return false;
        auto plugins = Plugins;
        auto row = Rows[index];
        auto label = RowLabel(row);
        if (!Current(token))
            return false;
        auto value = RowValue(row);
        if (!Current(token))
            return false;
        auto help = row.plugin >= 0 ? WFPText::Get("WFP_NFSU_PLUGINS_DESCRIPTION") : WFPPlugins::Help(row.row);
        if (!Current(token))
            return false;
        auto enabled = row.enabled;
        if (ShownKind == ScreenKind::Video)
        {
            enabled = WFPPlugins::Visible(row.row) && WFPPlugins::Enabled(row.row);
            if (!Current(token))
                return false;
        }
        Rows[index].label = std::move(label);
        Rows[index].value = std::move(value);
        Rows[index].help = std::move(help);
        Rows[index].enabled = enabled;
        if (row.option)
        {
            auto option = static_cast<Option*>(row.option);
            auto state = reinterpret_cast<uint8_t*>(option) + 0x28;
            if ((*state != 0) != enabled)
            {
                *state = uint8_t(enabled);
                CallWithoutSharedArrows(option->vtable[enabled ? 7 : 8], option);
            }
        }
        return Current(token);
    }

    void RedrawOptions()
    {
        auto token = RowsOwner.Capture();
        if (!Current(token))
            return;
        auto plugins = Plugins;
        auto rows = Rows;
        UpdateTitle();
        for (size_t i = 0; i < rows.size(); ++i)
        {
            if (!UpdateRowText(i, token))
                return;
            if (rows[i].option)
                DrawOption(static_cast<Option*>(rows[i].option));
        }
    }

    // The rows of the open page as they'd be built now: their visibility and enabled state follow other settings
    std::vector<MenuRow> PageRows(int plugin)
    {
        auto plugins = Plugins;
        std::vector<MenuRow> rows;
        if (ShownKind == ScreenKind::Video)
        {
            // Keep the native list intact; row availability is refreshed without rebuilding its stock options.
            auto fix = PluginAt(0);
            if (!fix || fix->module != ThisModule())
                return rows;
            for (auto& id : VideoAdvancedShown ? VideoAdvanced : VideoBasic)
            {
                auto& settings = fix->file.description.settings;
                auto it = std::find_if(settings.begin(), settings.end(), [&](const asi_settings::Setting& s) { return s.id == id; });
                if (it != settings.end() && WFPPlugins::Shown(*it))
                    rows.push_back({ -1, WFPPlugins::Row{ WFPPlugins::Row::Kind::Setting, fix, &*it } });
            }
            return rows;
        }
        if (plugin < 0)
        {
            for (int i = 0; Plugins && i < int(Plugins->plugins.size()); ++i)
                rows.push_back({ i });
            return rows;
        }
        if (auto loaded = PluginAt(plugin))
        {
            for (auto& row : WFPPlugins::Rows(*loaded, Excluded(*loaded)))
            {
                if (WFPPlugins::Visible(row))
                    rows.push_back({ -1, row, WFPPlugins::Enabled(row) });
            }
        }
        return rows;
    }

    bool SameRows(const std::vector<MenuRow>& a, const std::vector<MenuRow>& b)
    {
        return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin(), [](const MenuRow& x, const MenuRow& y)
        {
            return x.plugin == y.plugin && x.row.kind == y.row.kind && x.row.setting == y.row.setting && x.enabled == y.enabled;
        });
    }

    int HighlightedRow()
    {
        for (size_t i = 0; i < Rows.size(); ++i)
        {
            if (Rows[i].option && Rows[i].option == ArrowsOwner)
                return int(i);
        }
        return 0;
    }

    // After a change: rows that appear, disappear or change their enabled state need the page built again
    void AfterChange()
    {
        auto token = RowsOwner.Capture();
        if (!Current(token))
            return;
        auto plugins = Plugins;
        auto rows = PageRows(ShownPlugin);
        if (!Current(token))
            return;
        if (WFPPlugins::RestartNeeded())
            RestartChange = true;
        if (ShownKind == ScreenKind::Plugins && !SameRows(Rows, rows))
        {
            PendingAction = Pending::Rebuild;
            PendingHighlight = HighlightedRow();
        }
        else
            RedrawOptions();
    }

    void ActPresetRow(int32_t message)
    {
        auto token = RowsOwner.Capture();
        if (!Current(token))
            return;
        auto plugins = Plugins;
        auto selected = ArrowsOwner;
        auto state = PresetRow;
        bool applied{};
        if (message == MessageLeft || message == MessageRight)
            state.Cycle(message == MessageLeft ? -1 : 1);
        else if (message == MessageAccept)
        {
            bool confirming = state.confirm;
            applied = state.Accept([&]() { return Current(token); }) && confirming;
        }
        if (!Current(token))
            return;
        if (selected != ArrowsOwner)
            state.Cancel();
        PresetRow = std::move(state);
        if (applied)
            AfterChange();
        else
            RedrawOptions();
    }

    void ExecuteOption(const InputCommand& command)
    {
        if (!Current(command.owner) || command.row >= Rows.size())
            return;
        auto plugins = Plugins;
        auto row = Rows[command.row];
        auto message = command.message;
        using Kind = WFPPlugins::Row::Kind;
        if (row.plugin < 0 && (row.row.kind == Kind::Setting || row.row.kind == Kind::Action))
        {
            bool enabled = WFPPlugins::Visible(row.row) && WFPPlugins::Enabled(row.row);
            if (!Current(command.owner) || !enabled)
                return;
        }
        bool cancelled{};
        if (row.plugin >= 0 || row.row.kind != Kind::Preset)
            cancelled |= PresetRow.Cancel();
        if (row.plugin >= 0 || row.row.kind != Kind::RestoreDefaults)
            cancelled |= RestoreRow.Cancel();
        if (cancelled)
        {
            RedrawOptions();
            if (!Current(command.owner))
                return;
        }
        if (row.plugin >= 0)
        {
            if (message == MessageAccept)
            {
                PendingAction = Pending::Open;
                PendingPlugin = row.plugin;
            }
        }
        else if (row.row.kind == Kind::Preset)
            ActPresetRow(message);
        else if (row.row.kind == Kind::RestoreDefaults)
        {
            if (message != MessageAccept)
                return;
            auto state = RestoreRow;
            auto selected = ArrowsOwner;
            bool confirming = state.confirm;
            auto excluded = Excluded(*row.row.plugin);
            state.Accept(*row.row.plugin, excluded, [&]() { return Current(command.owner); });
            if (!Current(command.owner))
                return;
            if (selected != ArrowsOwner)
                state.Cancel();
            RestoreRow = std::move(state);
            if (confirming)
                AfterChange();
            else
                RedrawOptions();
        }
        else if (row.row.kind == Kind::Action)
        {
            if (message != MessageAccept)
                return;
            bool enabled = WFPPlugins::Enabled(row.row);
            if (!Current(command.owner))
                return;
            if (enabled)
            {
                OtherPlugins::Invoke(*row.row.plugin, *row.row.setting);
                if (Current(command.owner))
                    AfterChange();
            }
        }
        else if (message == MessageLeft || message == MessageRight)
        {
            bool changed = WFPPlugins::Step(row.row, message == MessageLeft ? -1 : 1, [&]() { return Current(command.owner); });
            if (!Current(command.owner))
                return;
            if (changed)
                AfterChange();
        }
    }

    uintptr_t __fastcall ActOption(Option* option, void* edx, int unknown, int32_t message)
    {
        auto token = RowsOwner.Capture();
        if (!Current(token))
            return 0;
        auto found = std::find_if(Rows.begin(), Rows.end(), [&](const MenuRow& row) { return row.option == option; });
        if (found == Rows.end() || !found->enabled)
            return 0;
        InputCommands.push_back({ token, size_t(found - Rows.begin()), message });
        option->Changed() = 1;
        return reinterpret_cast<uintptr_t(__fastcall*)(Option*, void*, int32_t)>(option->vtable[14])(option, nullptr, message);
    }

    // The arrows of the layout's last row that has them, for the added rows
    void FindSharedArrows(void* screen, int row)
    {
        auto package = ScreenPackage(screen);
        if (!package)
            return;
        for (int r = row; r > 0; --r)
        {
            FEList* list = nullptr;
            auto left = RowPrefix(screen, ArrowPrefix);
            auto right = RowPrefix(screen, ArrowPrefix + 1);
            auto leftObject = left ? FindObject(reinterpret_cast<FEList*>(package + 0x48), bStringHash((left + std::to_string(r)).c_str()), &list) : nullptr;
            auto rightObject = right ? FindObject(reinterpret_cast<FEList*>(package + 0x48), bStringHash((right + std::to_string(r)).c_str()), &list) : nullptr;
            if (leftObject || rightObject)
            {
                SharedArrows[0] = leftObject;
                SharedArrows[1] = rightObject;
                return;
            }
        }
    }

    bool AppendRows(void* screen)
    {
        ArrowsOwner = nullptr;      // an option of a screen that's gone
        SharedArrows[0] = SharedArrows[1] = nullptr;
        if (ScreenOwner.Get() != screen)
            ScreenOwner.Bind(screen);
        RowsOwner.Bind(screen);
        auto token = RowsOwner.Capture();
        VideoPauseShown = *reinterpret_cast<uint8_t*>(static_cast<uint8_t*>(screen) + InGameOffset) != 0;
        auto context = size_t(VideoPauseShown);
        auto advanced = size_t(VideoAdvancedShown);
        if (ShownKind == ScreenKind::Video)
            JoinedVideo[context][advanced].clear();
        auto plugins = Plugins;
        auto rows = PageRows(ShownPlugin);
        if (!Current(token))
            return false;
        Rows = std::move(rows);
        if (ShownKind == ScreenKind::Plugins && ShownPlugin >= 0)
        {
            auto state = PresetRow;
            state.Reset(*PluginAt(ShownPlugin));
            if (!Current(token))
                return false;
            PresetRow = std::move(state);
            RestoreRow = {};
        }
        UpdateTitle();
        if (!Current(token))
            return false;

        bool addedRows = false;
        for (size_t i = 0; i < Rows.size(); ++i)
        {
            if (!Current(token))
                return false;
            if (!UpdateRowText(i, token))
                return false;
            auto entry = Rows[i];
            if (!HasRoom(screen))
            {
                addedRows |= AddRow(screen);
                if (!Current(token))
                    return false;
            }
            bool room = HasRoom(screen);
            if (!Current(token))
                return false;
            if (!room)
            {
                Rows.resize(i);
                break;
            }
            auto option = static_cast<Option*>(GameMalloc(sizeof(Option)));
            if (!option)
            {
                Rows.resize(i);
                break;
            }
            ToggleOption(option, nullptr, entry.enabled);
            if (!OptionVTable[0])
            {
                // The base class, with the methods of the fix's options
                std::copy_n(option->vtable, std::size(OptionVTable), OptionVTable);
                OptionVTable[1] = reinterpret_cast<void*>(&ActOption);
                OptionVTable[3] = reinterpret_cast<void*>(&DrawOption);
                BaseLayout = std::exchange(OptionVTable[4], reinterpret_cast<void*>(&LayoutOption));
                BaseShow = std::exchange(OptionVTable[5], reinterpret_cast<void*>(&ShowOption));
                BaseHide = std::exchange(OptionVTable[6], reinterpret_cast<void*>(&HideOption));
                BaseHighlight = std::exchange(OptionVTable[9], reinterpret_cast<void*>(&HighlightOption));
                BaseUnhighlight = std::exchange(OptionVTable[10], reinterpret_cast<void*>(&UnhighlightOption));
            }
            option->vtable = OptionVTable;
            option->row = i;
            option->metricsSaved = false;
            Rows[i].option = option;
            int row = RowCount(screen);
            auto nextY = *reinterpret_cast<uint32_t*>(static_cast<uint8_t*>(screen) + 0xD8);
            auto nextDataY = *reinterpret_cast<uint32_t*>(static_cast<uint8_t*>(screen) + 0xF8);
            AddOption(screen, nullptr, option, 1);
            if (!Current(token))
                return false;
            if (!option->Title() || !option->Value())
            {
                HideOption(option);
                if (!Current(token))
                    return false;
                PublishedTexts.erase(option->Title());
                PublishedTexts.erase(option->Value());
                auto link = reinterpret_cast<uintptr_t>(option) + 4;
                auto next = *reinterpret_cast<uintptr_t*>(link);
                auto previous = *reinterpret_cast<uintptr_t*>(link + 4);
                *reinterpret_cast<uintptr_t*>(previous) = next;
                *reinterpret_cast<uintptr_t*>(next + 4) = previous;
                RowCount(screen) = row;
                *reinterpret_cast<uint32_t*>(static_cast<uint8_t*>(screen) + 0xD8) = nextY;
                *reinterpret_cast<uint32_t*>(static_cast<uint8_t*>(screen) + 0xF8) = nextDataY;
                reinterpret_cast<void(__fastcall*)(void*, void*, int)>(option->vtable[0])(option, nullptr, 1);
                if (!Current(token))
                    return false;
                Rows.resize(i);
                break;
            }
            if (ShownKind == ScreenKind::Video && entry.row.setting)
                JoinedVideo[context][advanced].push_back(entry.row.setting->id);

            // An added row: the last row's arrows
            auto arrows = Arrows(option);
            if (!arrows[0] && !arrows[1] && row > 1)
            {
                if (!SharedArrows[0] && !SharedArrows[1])
                    FindSharedArrows(screen, row - 1);
                arrows[0] = SharedArrows[0];
                arrows[1] = SharedArrows[1];
                reinterpret_cast<void(__fastcall*)(Option*)>(BaseLayout)(option);
            }
        }

        // The package's list of clickable objects has a fixed size, made for the objects it had
        if (addedRows)
        {
            if (auto package = ScreenPackage(screen))
                BuildMouseObjects(package, nullptr);
        }
        return Current(token);
    }

    // The options screen of the fix's category: the screen's icon and title, and the rows. The game's setup of the
    // options screen picks the list by the type and has nothing for the fix's type.
    constexpr uint32_t ScreenIcon = 0x0800814C;         // the category icon in the screen's corner
    constexpr uint32_t ScreenTitle = 0x42ADB44C;
    constexpr uint32_t CategoryIcon = 0x4DF98FB2;       // the gameplay category's icon

    // An object, or every object of a group of that name (the game's own title text goes to every string of a group)
    template <typename F>
    void ForEachObject(uint8_t* object, F&& visit)
    {
        if (*reinterpret_cast<int32_t*>(object + 0x18) != 5)
        {
            visit(object);
            return;
        }
        for (auto node = reinterpret_cast<FEList*>(object + 0x60)->head; node; node = *reinterpret_cast<uint8_t**>(node + 4))
            ForEachObject(node, visit);
    }

    bool FindPlugins(ScreenToken lifetime)
    {
        if (!ScreenOwner.Matches(lifetime))
            return false;
        auto rows = RowsOwner.Capture();
        auto tag = WFPText::Detail::Tag(LanguageName);
        auto plugins = WFPPlugins::Find(ThisModule(), tag ? asi_settings::text::Narrow(tag) : "en");
        auto current = RowsOwner.Capture();
        if (!ScreenOwner.Matches(lifetime) || rows.screen != current.screen || rows.generation != current.generation)
            return false;
        Plugins = std::move(plugins);
        WFPPlugins::Changed() = false;
        return true;
    }

    bool BuildFixScreenNow(void* screen)
    {
        auto lifetime = ScreenOwner.Capture();
        UpdateLanguage();
        if (!ScreenOwner.Matches(lifetime) || lifetime.screen != screen)
            return false;
        ShownKind = ScreenKind::Plugins;
        if (auto package = ScreenPackage(screen))
        {
            FEList* list = nullptr;
            if (auto icon = FindObject(reinterpret_cast<FEList*>(package + 0x48), ScreenIcon, &list))
            {
                ForEachObject(icon, [](uint8_t* object)
                {
                    auto image = reinterpret_cast<FEImage*>(object);
                    if (image->Type == FE_Image && image->Handle != CategoryIcon)
                    {
                        image->Handle = CategoryIcon;
                        image->Flags |= 0x2400000;
                    }
                });
            }
            if (auto title = FindObject(reinterpret_cast<FEList*>(package + 0x48), ScreenTitle, &list))
            {
                ForEachObject(title, [](uint8_t* object)
                {
                    if (*reinterpret_cast<int32_t*>(object + 0x18) == 2)    // a string
                        SetLanguageHash(object, TitleHash);
                });
            }
        }

        // The plugins, found again each time the category is opened, so that the order the plugins were loaded in
        // doesn't matter. The fix remains the first entry even when it is the only plugin.
        if (ShownPlugin < 0 || !Plugins)
        {
            if (!FindPlugins(lifetime))
                return false;
            ShownPlugin = -1;
            ListPlugin = 0;
        }
        return AppendRows(screen);
    }

    // The video options screen: the fix's video settings after the game's, in the basic and in the advanced list
    constexpr uint32_t AdvancedVideoOffset = 340;   // the screen's "advanced list shown"

    bool BuildVideoRowsNow(void* screen)
    {
        auto lifetime = ScreenOwner.Capture();
        UpdateLanguage();
        if (!ScreenOwner.Matches(lifetime) || lifetime.screen != screen)
            return false;
        ShownKind = ScreenKind::Video;
        VideoAdvancedShown = *reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(screen) + AdvancedVideoOffset) != 0;
        if (!FindPlugins(lifetime))
            return false;
        return AppendRows(screen);
    }

    // Public plugin APIs run after native construction/scroll/message parents have returned.
    void BuildFixScreen(void* screen)
    {
        if (ScreenOwner.Get() == screen)
            PendingBuild = { ScreenOwner.Capture(), ScreenKind::Plugins };
    }

    void BuildVideoRows(void* screen)
    {
        if (ScreenOwner.Get() == screen)
            PendingBuild = { ScreenOwner.Capture(), ScreenKind::Video };
    }

    // The game's Restore Defaults of an options screen: also the fix's rows on it
    SafetyHookInline shRestoreDefaults = {};
    void ExecuteDefaults(const InputCommand& command)
    {
        auto view = RowsOwner.Capture();
        auto active = [&]()
        {
            auto current = RowsOwner.Capture();
            return ScreenOwner.Matches(command.lifetime) && current.screen == view.screen && current.generation == view.generation;
        };
        if (!active() || !command.plugins || command.plugin < 0 || command.plugin >= int(command.plugins->plugins.size()))
            return;
        auto& plugin = command.plugins->plugins[command.plugin];
        std::vector<const asi_settings::Setting*> settings;
        for (const auto& id : command.settings)
        {
            auto found = std::find_if(plugin.file.description.settings.begin(), plugin.file.description.settings.end(),
                [&](const asi_settings::Setting& setting) { return setting.id == id; });
            if (found != plugin.file.description.settings.end())
                settings.push_back(&*found);
        }
        WFPPlugins::RestoreDefaults(plugin, settings, active);
        if (!active() || !Current(view))
            return;
        PresetRow.Cancel();
        RestoreRow.Cancel();
        AfterChange();
    }

    uintptr_t __fastcall RestoreDefaults(void* screen, void* edx)
    {
        auto lifetime = ScreenOwner.Capture();
        auto token = RowsOwner.Capture();
        if (screen == lifetime.screen && ScreenOwner.Matches(lifetime) && Current(token))
        {
            InputCommand command;
            command.defaults = true;
            command.lifetime = lifetime;
            command.plugins = Plugins;
            command.plugin = ShownKind == ScreenKind::Video ? 0 : ShownPlugin;
            if (command.plugins && command.plugin >= 0 && command.plugin < int(command.plugins->plugins.size()))
            {
                auto plugin = &command.plugins->plugins[command.plugin];
                for (const auto& row : Rows)
                {
                    if (row.option && row.row.plugin == plugin && row.row.setting && row.row.setting->HasValue())
                        command.settings.push_back(row.row.setting->id);
                }
                if (!command.settings.empty())
                    InputCommands.push_back(std::move(command));
            }
        }
        ++MessageDepth;
        auto result = shRestoreDefaults.unsafe_fastcall<uintptr_t>(screen, edx);
        --MessageDepth;
        return result;
    }

    // The page shown again on the same screen (another plugin, the list, or rows that changed)
    void RebuildScreen(void* screen, int highlight)
    {
        auto lifetime = ScreenOwner.Capture();
        ClearOptions(screen, nullptr);
        if (!ScreenOwner.Matches(lifetime))
            return;
        if (RowsOwner.Get() || !AppendRows(screen))
            return;
        if (RowsOwner.Get() != screen || !ScreenOwner.Matches(lifetime))
            return;
        auto token = RowsOwner.Capture();
        FinishOptions(screen, nullptr, highlight > 0 && highlight < int(Rows.size()) ? highlight + 1 : 0);
        if (Current(token))
            RedrawOptions();
    }

    // A dialog with one button: the parent package, the dialog's package, the button count, the text's hash
    int (__cdecl* ShowDialog)(const char* parent, const char* dialog, int buttons, uint32_t text, ...) = nullptr;
    const uint32_t RestartWarningHash = TextHash("WFP_RESTART_WARNING");

    // The options screen's messages: back on a plugin's page returns to the list; opening a page and building it
    // again wait until the screen handled the message, since the options are deleted
    void ProcessPending(ScreenToken token)
    {
        if (!Current(token))
            return;
        auto action = std::exchange(PendingAction, Pending::None);
        if (ShownKind != ScreenKind::Plugins)
            return;
        if (action == Pending::Open && PluginAt(PendingPlugin))
        {
            ShownPlugin = PendingPlugin;
            RebuildScreen(token.screen, 0);
        }
        else if (action == Pending::Back && ShownPlugin >= 0)
        {
            ListPlugin = ShownPlugin;
            ShownPlugin = -1;
            PresetRow.Cancel();
            RestoreRow.Cancel();
            RebuildScreen(token.screen, ListPlugin);
        }
        else if (action == Pending::Rebuild)
            RebuildScreen(token.screen, PendingHighlight);
    }

    NFSMWHelp::Captions HelpCaptions;
    struct HelpState
    {
        size_t row{ std::numeric_limits<size_t>::max() };
        std::string raw;
        std::vector<std::string> lines;
        float width{};
        float scale{};
        uint32_t font{};
        void* fontObject{};
        int language{};
        std::string languageName;
        size_t count{};
        uint64_t started{};
    } Help;

    void ResetHelp()
    {
        Help = {};
        HelpCaptions.ResetMetrics();
    }

    void HideScreenHelp(void* screen)
    {
        if (!HelpCaptions.HasPackages() || !FEngInstance || !*FEngInstance || !FindPackage)
            return;
        auto name = *reinterpret_cast<const char**>(static_cast<uint8_t*>(screen) + 0x10);
        if (name)
            HelpCaptions.Hide(FindPackage(*FEngInstance, nullptr, name), name);
    }

    void RefreshHelp(ScreenToken token)
    {
        if (!Current(token) || !FEngInstance || !*FEngInstance || !FindPackage)
            return;
        auto bytes = static_cast<uint8_t*>(token.screen);
        auto name = *reinterpret_cast<const char**>(bytes + 0x10);
        auto package = name ? FindPackage(*FEngInstance, nullptr, name) : nullptr;
        if (!package)
            return;
        auto selected = *reinterpret_cast<void**>(bytes + 0x34);
        auto row = std::find_if(Rows.begin(), Rows.end(), [&](const MenuRow& entry) { return entry.option == selected; });
        if (!selected || row == Rows.end())
        {
            HelpCaptions.Hide(package, name);
            return;
        }
        auto index = size_t(row - Rows.begin());
        auto text = row->label;
        if (!row->value.empty())
            text += ": " + row->value;
        if (!row->help.empty())
            text += "\n" + row->help;
        float width{};
        auto captions = HelpCaptions.Prepare(token.screen, package, name,
            static_cast<uint8_t*>(static_cast<Option*>(selected)->Title()), FindFont, CurrentLanguage ? *CurrentLanguage : 0, width);
        if (!Current(token))
            return;
        if (captions.empty())
        {
            HelpCaptions.Hide(package, name);
            return;
        }
        auto data = *reinterpret_cast<float**>(captions.front() + 0x2C);
        auto font = *reinterpret_cast<uint32_t*>(captions.front() + 0x24);
        auto fontObject = FindFont ? FindFont(font) : nullptr;
        auto language = CurrentLanguage ? *CurrentLanguage : 0;
        if (Help.raw != text || Help.row != index || Help.width != width || Help.count != captions.size() ||
            Help.scale != data[14] || Help.font != font || Help.fontObject != fontObject ||
            Help.language != language || Help.languageName != LanguageName)
        {
            Help.raw = text;
            Help.row = index;
            Help.width = width;
            Help.count = captions.size();
            Help.scale = data[14];
            Help.font = font;
            Help.fontObject = fontObject;
            Help.language = language;
            Help.languageName = LanguageName;
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
            auto line = page * captions.size() + i;
            auto text = line < Help.lines.size() ? Help.lines[line] : std::string();
            if (!PublishText(captions[i], TextHash("HELP_" + std::to_string(i)), text, token))
                return;
            auto& flags = *reinterpret_cast<uint32_t*>(captions[i] + 0x1C);
            if ((flags & 1) != 0)
                flags = (flags & ~1u) | 0x2400000;
        }
    }

    void ServiceScreen(bool frame)
    {
        if (MessageDepth || Servicing)
            return;
        Servicing = true;
        struct Finish
        {
            ~Finish()
            {
                Servicing = false;
            }
        } finish;
        auto build = std::exchange(PendingBuild, {});
        if (ScreenOwner.Matches(build.lifetime))
        {
            bool built = false;
            if (build.kind == ScreenKind::Plugins && OptionsType() == FixOptionsType)
                built = BuildFixScreenNow(build.lifetime.screen);
            else if (build.kind == ScreenKind::Video && OptionsType() == 1)
                built = BuildVideoRowsNow(build.lifetime.screen);
            auto token = RowsOwner.Capture();
            if (built && Current(token) && ScreenOwner.Matches(build.lifetime))
                FinishOptions(token.screen, nullptr, 0);
        }
        for (size_t count = 0; count < 32 && !InputCommands.empty(); ++count)
        {
            auto command = std::move(InputCommands.front());
            InputCommands.erase(InputCommands.begin());
            if (command.defaults)
            {
                auto view = RowsOwner.Capture();
                ExecuteDefaults(command);
                ProcessPending(view);
                continue;
            }
            if (!Current(command.owner))
                continue;
            if (command.back)
                PendingAction = Pending::Back;
            else
                ExecuteOption(command);
            ProcessPending(command.owner);
        }
        auto token = RowsOwner.Capture();
        if (!Current(token))
            return;
        if (WidthPending && GetTickCount64() >= WidthRetryAt)
        {
            WidthRetryAt = GetTickCount64() + 250;
            WidthPending = false;
            RedrawOptions();
            if (!Current(token))
                return;
        }
        if (WFPPlugins::Changed().exchange(false))
        {
            AfterChange();
            ProcessPending(token);
        }
        token = RowsOwner.Capture();
        if (frame && Current(token))
            RefreshHelp(token);
        if (!Current(token) || !RestartChange || RestartWarned || !ShowDialog)
            return;
        RestartWarned = true;
        void* fontObject = nullptr;
        for (const auto& row : Rows)
        {
            if (row.option && static_cast<Option*>(row.option)->Title())
            {
                fontObject = static_cast<Option*>(row.option)->Title();
                break;
            }
        }
        Texts[RestartWarningHash] = FontText(fontObject, WFPText::Get("WFP_RESTART_WARNING"), false);
        bool inGame = *reinterpret_cast<uint8_t*>(static_cast<uint8_t*>(token.screen) + InGameOffset) != 0;
        ShowDialog(*reinterpret_cast<const char**>(static_cast<uint8_t*>(token.screen) + 0x10),
            inGame ? "InGameDialog.fng" : "Dialog.fng", 1, RestartWarningHash);
    }

    SafetyHookInline shScreenMessage = {};
    uintptr_t __fastcall ScreenMessage(void* screen, void* edx, uint32_t message, int a3, int a4, int a5)
    {
        auto token = RowsOwner.Capture();
        bool ours = screen == token.screen && Current(token) &&
            (OptionsType() == FixOptionsType || OptionsType() == 1);
        bool back = ours && ShownKind == ScreenKind::Plugins && message == MessageBack &&
            ShownPlugin >= 0 && Plugins && !Plugins->plugins.empty();
        auto selected = ours ? *reinterpret_cast<void**>(static_cast<uint8_t*>(screen) + 0x34) : nullptr;
        auto row = std::find_if(Rows.begin(), Rows.end(), [&](const MenuRow& entry) { return entry.option == selected; });
        // Direct callbacks use the same deferred private-page actions.
        bool pluginRow = ours && ShownKind == ScreenKind::Plugins && selected && row != Rows.end() && row->enabled;
        bool accept = pluginRow && message == 0x406415E3;
        uintptr_t result = 1;
        if (back)
            InputCommands.push_back({ token, 0, int32_t(message), true });
        else if (accept)
            InputCommands.push_back({ token, size_t(row - Rows.begin()), MessageAccept });
        else
        {
            ++MessageDepth;
            result = shScreenMessage.unsafe_fastcall<uintptr_t>(screen, edx, message, a3, a4, a5);
            --MessageDepth;
        }
        // FEng may still dispatch row animations after this callback returns.
        // Rebuild the list at the next game tick, outside that traversal.
        return result;
    }

    SafetyHookInline shQueueMessage = {};
    int __fastcall QueueMessage(void* feng, void* edx, uint32_t message, void* object, void* package, int a4, int a5)
    {
        auto token = RowsOwner.Capture();
        if (Current(token) && ShownKind == ScreenKind::Plugins && OptionsType() == FixOptionsType &&
            package == ScreenPackage(token.screen))
        {
            if (message == 0x406415E3)
            {
                auto selected = *reinterpret_cast<void**>(static_cast<uint8_t*>(token.screen) + 0x34);
                auto row = std::find_if(Rows.begin(), Rows.end(), [&](const MenuRow& entry) { return entry.option == selected; });
                if (row != Rows.end() && row->enabled)
                    InputCommands.push_back({ token, size_t(row - Rows.begin()), MessageAccept });
                return 1;
            }
            if (message == MessageBack && ShownPlugin >= 0)
            {
                InputCommands.push_back({ token, 0, int32_t(message), true });
                return 1;
            }
        }
        return shQueueMessage.unsafe_fastcall<int>(feng, edx, message, object, package, a4, a5);
    }

    // The category in the options menu: the game's icon option class (0x4C bytes) with a class of its own, whose
    // only method besides the destructor answers "accept" by setting the options type
    void* (__fastcall* IconOptionCtor)(void* option, void* edx, uint32_t icon, uint32_t text, uint32_t unknown) = nullptr;
    void (__fastcall* AddIconOption)(void* menu, void* edx, void* option) = nullptr;
    void* CategoryVTable[2] = {};

    void __fastcall CategoryReact(void* option, void* edx, const char* package, uint32_t message, void* object, uint32_t param1, uint32_t param2)
    {
        if (message == MessageAccept)
        {
            OptionsType() = FixOptionsType;
            ShownPlugin = -1;
            RetireRows(RowsOwner.Get());
        }
    }

    void AddCategory(void* menu)
    {
        auto option = GameMalloc(0x4C);
        if (!option)
            return;
        UpdateLanguage();
        SetText(CategoryHash, WFPText::Get("WFP_PLUGINS"));
        IconOptionCtor(option, nullptr, CategoryIcon, CategoryHash, 0);
        *reinterpret_cast<void***>(option) = CategoryVTable;
        AddIconOption(menu, nullptr, option);
    }

    // "Did the options change": the settings are stored when they change, so leaving the screen doesn't ask whether
    // to keep the changes
    SafetyHookInline shOptionsUnchanged = {};
    bool __fastcall OptionsUnchanged(void* screen, void* edx)
    {
        if (OptionsType() == FixOptionsType)
            return true;
        return shOptionsUnchanged.unsafe_fastcall<bool>(screen, edx);
    }

}

class Options
{
public:
    Options()
    {
        WFP::onInitEvent() += []()
        {
            // An option of the gameplay screen: allocation, the toggle class' constructor and adding it
            auto option = hook::pattern("6A 5C E8 ? ? ? ? 8B F8 83 C4 04 89 7C 24 0C 85 FF C7 44 24 18 00 00 00 00 74 11 6A 01 8B CF E8 ? ? ? ? C7 07 ? ? ? ? EB 02 33 FF 6A 01 57 8B CE 89 6C 24 20 E8");
            auto findObject = hook::pattern("8B 44 24 04 85 C0 74 26 8B 0D ? ? ? ? 50 E8 ? ? ? ? 85 C0 74 16 8B 4C 24 08 51 8B C8 E8 ? ? ? ? 85 C0 74 06 83 78 18 02 74 02 33 C0 C3");
            auto mouseObjects = hook::pattern("83 EC 14 55 56 8B F1 8B 86 AC 00 00 00 33 ED 3B C5 57 74 29 8B 48 FC 8D 78 FC 68");
            auto bindTracks = hook::pattern("83 EC 40 53 8B D9 8B 43 54 8B 48 20 8B 40 1C 85 C0 0F 86 ? ? ? ? 55 56 57 8D 71 18");
            auto setHash = hook::pattern("56 8B 74 24 08 85 F6 74 30 8B 4E 1C 8B 44 24 0C 81 C9 00 00 40 00 89 4E 1C 89 46 60");
            auto search = hook::pattern("53 8B 1D ? ? ? ? 85 DB 75 04 33 C0 5B C3 55 56 8B 35");
            auto languages = hook::pattern("B8 ? ? ? ? EB 03 8D 49 00 39 10 74 0E 83 C0 14 41 3D");
            auto fontLookup = hook::pattern("8B 54 24 04 56 8B 35 ? ? ? ? EB 03 8D 49 00 8B C6 3D");
            auto nativeWidth = hook::pattern("56 8B 74 24 08 57 6A 00 6A 00 6A 00 56 8B F9 E8 ? ? ? ? D9 5C 24 0C 66 8B 06");
            auto glyphAdvance = hook::pattern("51 8B 44 24 08 53 8B D9 8A 4C 24 14 F6 C1 20 C7 44 24 04 00 00 00 00");
            // Resolution hooks the callee before menu initialization; its caller remains unchanged.
            auto localizedWide = hook::pattern("E8 ? ? ? ? 83 C4 ? 84 C0 8D 6C 24 ? 75 ? 8B 6E ? 8B 56");
            if (fontLookup.size() == 1 && glyphAdvance.size() == 1 && localizedWide.size() == 1 && nativeWidth.size() == 1)
            {
                FindFont = reinterpret_cast<decltype(FindFont)>(fontLookup.get_first());
                NativeStringWidth = reinterpret_cast<decltype(NativeStringWidth)>(nativeWidth.get_first());
                GlyphAdvance = reinterpret_cast<decltype(GlyphAdvance)>(glyphAdvance.get_first());
                LocalizedWide = reinterpret_cast<decltype(LocalizedWide)>(injector::GetBranchDestination(localizedWide.get_first()).as_int());
            }
            else
                return;
            // The options menu's categories (after Controls), the switch that opens the options screen for a category,
            // the options screen's setup switch and its "did the options change"
            auto categories = hook::pattern("68 B5 6D DC 04 68 26 7B 4A A0 8B CF E8 ? ? ? ? C7 07 ? ? ? ? EB 02 33 FF 57 8B CE 89 6C 24 ? E8 ? ? ? ? A1");
            auto menuSwitch = hook::pattern("A1 ? ? ? ? 8B 48 10 8B 49 24 83 F9 09 0F 87 ? ? ? ? FF 24 8D");
            auto screenSwitch = hook::pattern("A1 ? ? ? ? 8B 48 10 8B 41 24 83 C4 04 83 F8 09 77 ? FF 24 85");
            auto unchanged = hook::pattern("56 8B 35 ? ? ? ? 8B C1 8B 4E 10 8B 51 24 83 FA 09 0F 87 ? ? ? ? FF 24 95 ? ? ? ? 8B 80 30 01 00 00 50 83 C1 4C E8");
            // The options screen's constructor (its class) and the gameplay list built again on the same screen
            auto screenClass = hook::pattern("C7 06 ? ? ? ? 39 5F 08 0F 95 C0 88 86 ? ? ? ? 8D 86");
            auto rebuild = hook::pattern("83 79 24 03 75 2B 8B CB E8 ? ? ? ? 8B CB E8 ? ? ? ? 6A 00 8B CB E8");
            // Consume private-page input before the package can start its native exit animation.
            auto queueMessage = hook::pattern("56 57 6A 20 8B F9 E8 ? ? ? ? 83 C4 04 85 C0 74 15 B9 FE CA AD AB 89 48 04 89 48 08 C7 00 ? ? ? ? 8B F0 EB 02 33 F6");
            // Without the category the settings stay in the .cfg only: the game's own screens aren't changed
            if (option.size() != 1 || findObject.size() != 1 || mouseObjects.size() != 1 || bindTracks.size() != 1 || setHash.size() != 1 || search.size() != 1 || languages.size() != 1 ||
                categories.size() != 1 || menuSwitch.size() != 1 || screenSwitch.size() != 1 || unchanged.size() != 1 || screenClass.size() != 1 || rebuild.size() != 1 || queueMessage.size() != 1)
                return;
            OptionsDatabase = *menuSwitch.get_first<uintptr_t*>(1);

            GameMalloc = reinterpret_cast<decltype(GameMalloc)>(injector::GetBranchDestination(option.get_first(2)).as_int());
            ToggleOption = reinterpret_cast<decltype(ToggleOption)>(injector::GetBranchDestination(option.get_first(0x20)).as_int());
            AddOption = reinterpret_cast<decltype(AddOption)>(injector::GetBranchDestination(option.get_first(0x38)).as_int());
            RowObject = reinterpret_cast<decltype(RowObject)>(injector::GetBranchDestination(reinterpret_cast<uintptr_t>(AddOption) + 0xE).as_int());
            SetLanguageHash = reinterpret_cast<decltype(SetLanguageHash)>(setHash.get_first());
            FEngInstance = *findObject.get_first<void**>(0xA);
            FindPackage = reinterpret_cast<decltype(FindPackage)>(injector::GetBranchDestination(findObject.get_first(0xF)).as_int());
            BuildMouseObjects = reinterpret_cast<decltype(BuildMouseObjects)>(mouseObjects.get_first(-0x15));
            BindAnimationTracks = reinterpret_cast<decltype(BindAnimationTracks)>(bindTracks.get_first());
            ClearOptions = reinterpret_cast<decltype(ClearOptions)>(injector::GetBranchDestination(rebuild.get_first(8)).as_int());
            FinishOptions = reinterpret_cast<decltype(FinishOptions)>(injector::GetBranchDestination(rebuild.get_first(0x18)).as_int());
            Languages = *languages.get_first<LanguageInfo*>(1);
            CurrentLanguage = *languages.get_first<int32_t*>(0x13);
            LanguagesEnd = reinterpret_cast<LanguageInfo*>(CurrentLanguage);

            auto screenVTable = *screenClass.get_first<void**>(2);
            auto deletingDestructor = static_cast<uint8_t*>(screenVTable[0]);
            const uint8_t destructorPrefix[]{ 0x56, 0x8B, 0xF1, 0xE8 };
            const uint8_t destructorFlags[]{ 0xF6, 0x44, 0x24, 0x08, 0x01 };
            if (std::memcmp(deletingDestructor, destructorPrefix, sizeof(destructorPrefix)) != 0 ||
                std::memcmp(deletingDestructor + 8, destructorFlags, sizeof(destructorFlags)) != 0)
                return;
            shDestroyScreen = safetyhook::create_inline(injector::GetBranchDestination(deletingDestructor + 3).as_int(), DestroyScreen);
            shClearRows = safetyhook::create_inline(reinterpret_cast<void*>(ClearOptions), ClearRows);
            static auto ScreenConstructHook = safetyhook::create_mid(screenClass.get_first(), [](SafetyHookContext& regs)
            {
                ConstructScreen(reinterpret_cast<void*>(regs.esi));
            });
            if (!shDestroyScreen || !shClearRows || !ScreenConstructHook)
            {
                ScreenConstructHook.reset();
                shClearRows.reset();
                shDestroyScreen.reset();
                return;
            }

            shSearchForString = safetyhook::create_inline(search.get_first(), SearchForString);
            shScreenMessage = safetyhook::create_inline(screenVTable[1], ScreenMessage);
            shQueueMessage = safetyhook::create_inline(queueMessage.get_first(), QueueMessage);
            shOptionsUnchanged = safetyhook::create_inline(unchanged.get_first(), OptionsUnchanged);
            if (!shSearchForString || !shScreenMessage || !shQueueMessage || !shOptionsUnchanged)
            {
                shOptionsUnchanged.reset();
                shQueueMessage.reset();
                shScreenMessage.reset();
                shSearchForString.reset();
                ScreenConstructHook.reset();
                shClearRows.reset();
                shDestroyScreen.reset();
                return;
            }

            // The options menu: the categories are added one by one, the fix's after Controls
            IconOptionCtor = reinterpret_cast<decltype(IconOptionCtor)>(injector::GetBranchDestination(categories.get_first(0xC)).as_int());
            AddIconOption = reinterpret_cast<decltype(AddIconOption)>(injector::GetBranchDestination(categories.get_first(0x22)).as_int());
            auto controlsVTable = *categories.get_first<void**>(0x13);
            CategoryVTable[0] = controlsVTable[0];     // the destructor
            CategoryVTable[1] = reinterpret_cast<void*>(&CategoryReact);
            static auto CategoriesHook = safetyhook::create_mid(categories.get_first(0x27), [](SafetyHookContext& regs)
            {
                AddCategory(reinterpret_cast<void*>(regs.esi));
            });

            // Picking a category: the fix's type opens the options screen like case 0 of the switch
            static uintptr_t OptionsScreenCase = **menuSwitch.get_first<uintptr_t*>(0x17);
            static auto MenuSwitchHook = safetyhook::create_mid(menuSwitch.get_first(8), [](SafetyHookContext& regs)
            {
                if (OptionsType() == FixOptionsType)
                    regs.eip = OptionsScreenCase;
            });

            // The options screen's setup, after its options were deleted: the fix's type goes to the default case,
            // which only finishes the list
            static auto ScreenSetupHook = safetyhook::create_mid(screenSwitch.get_first(5), [](SafetyHookContext& regs)
            {
                PendingBuild = {};
                RetireRows(RowsOwner.Get());
                Rows.clear();
                RestartChange = false;
                RestartWarned = false;
                ArrowsOwner = nullptr;
                PendingAction = Pending::None;
                if (OptionsType() == FixOptionsType)
                    BuildFixScreen(reinterpret_cast<void*>(regs.esi));
            });

            if (!CategoriesHook || !MenuSwitchHook || !ScreenSetupHook)
            {
                ScreenSetupHook.reset();
                MenuSwitchHook.reset();
                CategoriesHook.reset();
                shOptionsUnchanged.reset();
                shQueueMessage.reset();
                shScreenMessage.reset();
                shSearchForString.reset();
                ScreenConstructHook.reset();
                shClearRows.reset();
                shDestroyScreen.reset();
                return;
            }

            // The video screen's list (case 1 of the setup switch: the builder, then the end of the list) and the
            // game's Restore Defaults
            auto restoreDefaults = hook::pattern("53 55 8B E9 8B 0D ? ? ? ? 8B 41 10 8B 50 24 83 FA 09 56 57 0F 87");
            if (restoreDefaults.size() == 1)
                shRestoreDefaults = safetyhook::create_inline(restoreDefaults.get_first(), RestoreDefaults);

            auto videoCase = (*screenSwitch.get_first<uint8_t**>(0x16))[1];
            if (shRestoreDefaults && videoCase[0] == 0x8B && videoCase[1] == 0xCE && videoCase[2] == 0xE8)
            {
                static SafetyHookInline shVideoList = {};
                shVideoList = safetyhook::create_inline(injector::GetBranchDestination(videoCase + 2).as_int(), static_cast<void(__fastcall*)(void*, void*)>([](void* screen, void* edx)
                {
                    shVideoList.unsafe_fastcall(screen, edx);
                    BuildVideoRows(screen);
                }));
            }
            // A dialog with one button (the profile name check's message)
            auto dialog = hook::pattern("68 2D 45 6C 53 6A 01 68 ? ? ? ? 51 E8");
            if (dialog.size() == 1)
                ShowDialog = reinterpret_cast<decltype(ShowDialog)>(injector::GetBranchDestination(dialog.get_first(0xD)).as_int());

            WFP::onGameProcessEvent() += []()
            {
                ServiceScreen(true);
            };


        };
    }
} Options;
