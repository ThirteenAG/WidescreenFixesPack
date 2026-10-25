module;

#include <stdafx.h>
#include "common.h"
#include "FixSettings.hpp"
#include "../Shared/RWGTA/FixMenu.hpp"
#include "../Shared/RWGTA/RootPages.hpp"
#include "MenuText.hpp"

export module Options;

import Skeleton;

// The fix's settings in the game's frontend: "Plugins" at the end of the Options screen shows the list of plugins (the
// fix first, then the other plugins with ASI Settings, see Shared/RWGTA/FixMenu.hpp), and each page its settings with
// their values like Display Setup. Console gamma is a row of Display Setup (after Brightness), the field of view and
// SMAA rows of its Advanced screen (after Widescreen and the game's Anti Aliasing); the game's Restore Defaults of
// those screens resets them too. In the main menu and in the pause menu, which share the frontend.
//
// San Andreas' screen table (MenuPages) has no screen the PC game leaves unused, so the Options screen shows the fix's
// rows while they are open: its rows and title are saved, replaced, and put back when the fix's pages are left (Back,
// Escape, or any other way off the screen). Nothing else of the frontend changes, and only one row is added to the
// Options screen, before its Back row; when another mod changed that row, the fix leaves the menu alone and the
// settings stay in the .cfg and FusionFixHub.
//
// The rows are answered through call sites, each calling the previous target (another mod's hook or the game): the
// row's and the screen's text (CText::Get), CMenuManager::UserInput (to fill the rows), CMenuManager::ProcessUserInput
// (Escape on the fix's pages goes back to their list) and CMenuManager::ProcessMenuOptions (accept, and left/right on
// rows with values). The value at the right of a row comes through Menu.ixx's hook at the end of the right text's
// switch in CMenuManager::DrawStandardMenus (RightText).

namespace
{
#pragma pack(push, 1)
    struct CMenuEntry
    {
        uint8_t action;
        char name[8];
        uint8_t type;           // 11: a button, 12: a row with a value (left and right change it)
        int8_t targetMenu;
        uint8_t pad1;
        int16_t x;
        int16_t y;
        uint8_t align;          // 1: left, 2: right, else centered
        uint8_t pad2;
    };

    struct CMenuScreen
    {
        char name[8];
        int8_t previousPage;
        int8_t startingButton;
        CMenuEntry entries[12];
    };
#pragma pack(pop)
    static_assert(sizeof(CMenuEntry) == 18 && sizeof(CMenuScreen) == 226);

    constexpr int RowCount = 12;
    constexpr int PageRows = 9;             // settings visible at once; one more slot includes Back
    constexpr int RootRows = 9;             // links of the list; Back follows
    constexpr int ScreenOptions = 33;
    constexpr int ScreenDisplay = 4;
    constexpr int ScreenAdvanced = 27;
    constexpr int16_t LineHeight = 30;      // the game's distance between rows placed one after another
    constexpr uint8_t ActionBase = 0xC0;    // the fix's rows: ActionBase + row; far from the game's ids (up to 0x41)
    constexpr uint8_t ActionOpen = 0xD0;
    constexpr uint8_t ActionJoined = 0xE0;  // the fix's rows on the game's screens: ActionJoined + index of Joined
    constexpr uint8_t TypeButton = 11, TypeValue = 12;
    constexpr uint8_t AlignLeft = 1, AlignCenter = 3;

    // CMenuManager
    constexpr size_t SelectedItem = 0x54;
    constexpr size_t Language = 0x84;
    constexpr size_t MainMenu = 0xE9;
    constexpr size_t CurrentPage = 0x15D;

    CMenuScreen* aScreens = nullptr;
    uint8_t* Manager = nullptr;
    bool bReady = false;

    enum class Mode { None, Root, Page };
    Mode State = Mode::None;
    CMenuScreen SavedOptions{};
    int OpenRow = 0;
    int CurrentShownPage = 0;
    int RootRowCount = 0;

    RWGTAMenu::Model Model;
    RWGTAMenu::WheelInput Wheel;
    RWGTAMenu::ValueAreas ValueAreas;
    void* Audio = nullptr;
    void(__fastcall* NavigationSound)(void*, void*, int, float, float) = nullptr;
    void(__cdecl* DrawRect)(const float*, const uint8_t*) = nullptr;
    RWGTAMenu::RootPages Root;
    std::vector<const RWGTAMenu::Row*> ShownRows;

    // The fix's settings on the game's own screens, each after the game's row of that key
    struct JoinedSetting
    {
        const char* id;
        int screen;
        const char* after;
    };
    constexpr JoinedSetting Joined[] =
    {
        { "GRAPHICS.ConsoleGamma", ScreenDisplay, "FED_BRI" },
        { "MAIN.FOVFactor", ScreenAdvanced, "FED_WIS" },
        { "GRAPHICS.AntiAliasing", ScreenAdvanced, "FED_AAS" },
    };
    bool bJoined = false;                   // the rows are on the game's screens

    std::set<std::string> JoinedIds(int screen = -1)
    {
        std::set<std::string> ids;
        for (auto& joined : Joined)
        {
            if (bJoined && (screen < 0 || joined.screen == screen))
                ids.insert(joined.id);
        }
        return ids;
    }
    std::map<std::string, std::string, std::less<>> Texts;  // the rows' and titles' text by key, in the game's font

    int Screen(uint8_t* manager) { return *reinterpret_cast<int8_t*>(manager + CurrentPage); }
    int32_t& Option(uint8_t* manager) { return *reinterpret_cast<int32_t*>(manager + SelectedItem); }
    bool InGame(uint8_t* manager) { return !*(manager + MainMenu); }

    // The language of the fix's texts and of the other plugins' pages: MAIN.TextLanguage (cfg and editors only, for
    // translations of the game that replaced one of its languages), or the game's language
    std::string LanguageTag()
    {
        auto& chosen = FixSettings().String("MAIN.TextLanguage");
        if (!chosen.empty() && chosen != "auto")
            return chosen;
        static const char* tags[] = { "en", "fr", "de", "it", "es" };
        int language = Manager ? *reinterpret_cast<int8_t*>(Manager + Language) : 0;
        return language >= 0 && language < int(std::size(tags)) ? tags[language] : "en";
    }

    // San Andreas' font is 8 bits with the same table as III and Vice City
    void SetText(const std::string& key, const std::string& utf8)
    {
        std::string text;
        for (auto c : RWGTAMenu::ToGameFont(utf8, false))
            text += char(c < 0x100 ? c : '?');
        Texts[key] = text;
    }

    void SetEntry(CMenuEntry& entry, uint8_t action, const char* name, uint8_t type, int16_t x, int16_t y, uint8_t align)
    {
        entry = {};
        entry.action = action;
        strncpy_s(entry.name, name, _TRUNCATE);
        entry.type = type;
        entry.targetMenu = ScreenOptions;
        entry.x = x;
        entry.y = y;
        entry.align = align;
    }

    void FillRoot()
    {
        auto& screen = aScreens[ScreenOptions];
        for (auto& entry : screen.entries)
            entry = {};
        strncpy_s(screen.name, "WFPT0", _TRUNCATE);
        auto& pages = Model.Pages();
        RootRowCount = int(Root.Rows());
        int16_t y = 130;
        for (int i = 0; i < RootRowCount; ++i, y += 30)
        {
            char key[8];
            sprintf_s(key, "WFPL%02d", i);
            int page = Root.Page(size_t(i));
            SetText(key, page >= 0 ? pages[page].title : Model.Text(page == RWGTAMenu::RootPages::Previous ? "menu.previous" : "menu.next"));
            SetEntry(screen.entries[i], ActionBase + i, key, TypeButton, 320, y, AlignCenter);
        }
        SetEntry(screen.entries[RootRowCount], ActionBase + RootRowCount, "FEDS_TB", TypeButton, 320, y, AlignCenter);
    }

    void FillPage(uint8_t* manager)
    {
        auto& screen = aScreens[ScreenOptions];
        for (auto& entry : screen.entries)
            entry = {};
        strncpy_s(screen.name, "WFPT1", _TRUNCATE);
        ShownRows = Model.WindowRows(size_t(CurrentShownPage));
        int count = int(ShownRows.size());
        int16_t y = 127;
        for (int i = 0; i < count; ++i, y += 30)
        {
            char key[8];
            sprintf_s(key, "WFPR%02d", i);
            if (!ShownRows[i])
            {
                SetEntry(screen.entries[i], ActionBase + i, "FEDS_TB", TypeButton, 320, y + 5, AlignCenter);
                continue;
            }
            SetText(key, Model.Label(*ShownRows[i]));
            SetText(std::string(key) + "V", Model.Value(*ShownRows[i]));
            auto type = ShownRows[i]->kind == RWGTAMenu::RowKind::Setting || ShownRows[i]->kind == RWGTAMenu::RowKind::Preset ? TypeValue : TypeButton;
            SetEntry(screen.entries[i], ActionBase + i, key, type, 57, y, AlignLeft);
        }
        if (Option(manager) >= count)
            Option(manager) = std::max(0, count - 1);
        if (size_t(CurrentShownPage) < Model.Pages().size())
            SetText("WFPT1", Model.Pages()[CurrentShownPage].title);
    }

    void OpenModel()
    {
        HMODULE self = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCWSTR>(&OpenModel), &self);
        Model.OpenPlugins(self, LanguageTag(), PageRows, JoinedIds(), true);
    }

    // The model's row of a setting on the game's screens
    const RWGTAMenu::Row* JoinedRow(int index)
    {
        if (index < 0 || index >= int(std::size(Joined)))
            return nullptr;
        for (auto& row : Model.JoinedRows())
        {
            if (row.setting && row.setting->id == Joined[index].id)
                return &row;
        }
        return nullptr;
    }

    void FillJoined()
    {
        for (int i = 0; i < int(std::size(Joined)); ++i)
        {
            char key[8];
            sprintf_s(key, "WFPJ%02d", i);
            auto row = JoinedRow(i);
            SetText(key, row ? Model.Label(*row) : std::string{});
            SetText(std::string(key) + "V", row ? Model.Value(*row) : std::string{});
        }
    }

    bool JoinedScreen(int screen) { return bJoined && (screen == ScreenDisplay || screen == ScreenAdvanced); }

    void Open(uint8_t* manager)
    {
        OpenModel();
        SetText("WFPT0", Model.Text("menu.plugins"));
        Root.Reset(Model.Pages().size(), RootRows);
        SavedOptions = aScreens[ScreenOptions];
        State = Mode::Root;
        FillRoot();
        Option(manager) = 0;
    }

    void Close()
    {
        if (State == Mode::None)
            return;
        aScreens[ScreenOptions] = SavedOptions;
        State = Mode::None;
        Model.Close();
        ShownRows.clear();
    }

    void ShowPage(uint8_t* manager, int page)
    {
        CurrentShownPage = page;
        Model.PageShown(size_t(page));
        State = Mode::Page;
        Option(manager) = 0;
        FillPage(manager);
    }

    void Back(uint8_t* manager)
    {
        if (State == Mode::Page)
        {
            State = Mode::Root;
            FillRoot();
            Option(manager) = int(Root.Selection(size_t(CurrentShownPage)));
        }
        else if (State == Mode::Root)
        {
            Close();
            Option(manager) = OpenRow;
        }
    }

    void Update(uint8_t* manager)
    {
        Manager = manager;
        if (State == Mode::None)
        {
            int screen = Screen(manager);
            if (screen == ScreenOptions)
                SetText("WFPOPT", RWGTAMenu::PluginText(FixSettings(), LanguageTag(), "menu.plugins"));
            if (JoinedScreen(screen))
            {
                if (!Model.IsOpen())
                    OpenModel();
                FillJoined();
            }
            else if (Model.IsOpen())
            {
                Model.Close();
            }
            return;
        }
        if (Screen(manager) != ScreenOptions)
        {
            Close();
            return;
        }
        if (State == Mode::Page)
            FillPage(manager);
    }

    void* UserInputPrevious = nullptr;
    void __fastcall UserInput(uint8_t* manager, void* edx)
    {
        if (bReady)
            Update(manager);
        bool overValue = ValueAreas.Contains(
            float(*reinterpret_cast<int32_t*>(manager + 0xE0)), float(*reinterpret_cast<int32_t*>(manager + 0xE4)));
        int wheel{ Wheel.Begin(bReady && State == Mode::Page && Screen(manager) == ScreenOptions && !overValue) };
        if (wheel)
        {
            Model.Scroll(size_t(CurrentShownPage), wheel);
            FillPage(manager);
        }
        reinterpret_cast<void(__fastcall*)(uint8_t*, void*)>(UserInputPrevious)(manager, edx);
        Wheel.End();
        if (bReady)
            Update(manager);
    }

    // Escape on the fix's pages: back to their list, or to the Options screen
    void* ProcessUserInputPrevious = nullptr;
    void __fastcall ProcessUserInput(uint8_t* manager, void* edx, int32_t down, int32_t up, int32_t enter, int32_t exit, int32_t wheel)
    {
        if (bReady && State != Mode::None && Screen(manager) == ScreenOptions && uint8_t(exit & 0xFF))
        {
            Back(manager);
            exit = 0;
        }
        if (bReady && State == Mode::Page && Screen(manager) == ScreenOptions && (uint8_t(down) || uint8_t(up)))
        {
            if (NavigationSound)
                NavigationSound(Audio, nullptr, 3, 0.0f, 1.0f);
            Option(manager) = Model.Move(size_t(CurrentShownPage), Option(manager), uint8_t(down) ? 1 : -1);
            FillPage(manager);
            down = up = 0;
        }
        reinterpret_cast<void(__fastcall*)(uint8_t*, void*, int32_t, int32_t, int32_t, int32_t, int32_t)>(ProcessUserInputPrevious)(manager, edx, down, up, enter, exit, wheel);
    }

    // CMenuManager::ProcessMenuOptions(direction, &exit, enter); true when the row was the fix's
    bool __cdecl ProcessOptions(uint8_t* manager, int32_t direction, int32_t enter)
    {
        Manager = manager;
        if (!bReady)
            return false;
        int option = Option(manager);
        if (option < 0 || option >= RowCount)
            return false;
        int dir = uint8_t(enter & 0xFF) ? 0 : int(int8_t(direction & 0xFF));
        int screen = Screen(manager);
        if (State == Mode::None && JoinedScreen(screen))
        {
            auto joined = JoinedRow(aScreens[screen].entries[option].action - ActionJoined);
            if (!joined)
                return false;
            Model.Change(*joined, dir > 0 ? 1 : dir < 0 ? -1 : 0, InGame(manager));
            FillJoined();
            return true;
        }
        if (screen != ScreenOptions)
            return false;
        int action = aScreens[ScreenOptions].entries[option].action;
        if (State == Mode::None)
        {
            if (action != ActionOpen)
                return false;
            if (dir == 0)
                Open(manager);
            return true;
        }
        int row = action - ActionBase;
        if (row < 0 || row >= RowCount)
            return false;
        if (State == Mode::Root)
        {
            if (dir != 0)
                return true;
            if (row < RootRowCount)
            {
                int page = Root.Page(size_t(row));
                if (Root.Move(page))
                {
                    FillRoot();
                    Option(manager) = 0;
                }
                else if (page >= 0)
                {
                    ShowPage(manager, page);
                }
            }
            else
            {
                Back(manager);
            }
            return true;
        }
        if (row >= int(ShownRows.size()) || !ShownRows[row])
        {
            if (dir == 0)
                Back(manager);
            return true;
        }
        Model.Change(*ShownRows[row], dir > 0 ? 1 : dir < 0 ? -1 : 0, InGame(manager));
        FillPage(manager);
        return true;
    }

    void* OptionsAcceptPrevious = nullptr;
    void* OptionsChangePrevious = nullptr;

    // thiscall (direction, &exit, enter), callee cleans 12 bytes: called as fastcall with three stack arguments, the
    // previous target gets ecx, edx and the arguments as they were
    using OptionsFunction = uint32_t(__fastcall*)(uint8_t*, void*, uint32_t, uint32_t, uint32_t);

    uint32_t __fastcall OptionsAccept(uint8_t* manager, void* edx, uint32_t direction, uint32_t exit, uint32_t enter)
    {
        if (ProcessOptions(manager, int32_t(direction), int32_t(enter)))
            return 1;
        return reinterpret_cast<OptionsFunction>(OptionsAcceptPrevious)(manager, edx, direction, exit, enter);
    }

    uint32_t __fastcall OptionsChange(uint8_t* manager, void* edx, uint32_t direction, uint32_t exit, uint32_t enter)
    {
        if (ProcessOptions(manager, int32_t(direction), int32_t(enter)))
            return 1;
        return reinterpret_cast<OptionsFunction>(OptionsChangePrevious)(manager, edx, direction, exit, enter);
    }

    const char* Answer(const char* key)
    {
        if (!bReady || !key || strncmp(key, "WFP", 3) != 0)
            return nullptr;
        auto it = Texts.find(std::string_view(key, strnlen(key, 8)));
        return it != Texts.end() ? it->second.c_str() : nullptr;
    }

    void* RowTextPrevious = nullptr;
    void* TitleTextPrevious = nullptr;

    const char* __fastcall RowText(void* text, void* edx, const char* key)
    {
        if (auto answer = Answer(key))
            return answer;
        return reinterpret_cast<const char* (__fastcall*)(void*, void*, const char*)>(RowTextPrevious)(text, edx, key);
    }

    const char* __fastcall TitleText(void* text, void* edx, const char* key)
    {
        if (auto answer = Answer(key))
            return answer;
        return reinterpret_cast<const char* (__fastcall*)(void*, void*, const char*)>(TitleTextPrevious)(text, edx, key);
    }

    // Keep labels within the space left by the right-aligned value, at the native font scale.
    using PrintStringFn = void(__cdecl*)(float, float, const char*);
    PrintStringFn TitlePrintPrevious = nullptr;
    void(__cdecl* RenderFontBuffer)() = nullptr;

    void __cdecl TitlePrint(float x, float y, const char* text)
    {
        TitlePrintPrevious(x, y, text);
        // Keep the title's font state before the setting labels are measured and drawn.
        if (bReady && State != Mode::None && RenderFontBuffer)
            RenderFontBuffer();
    }

    PrintStringFn LabelPrintPrevious = nullptr;
    PrintStringFn ValuePrintPrevious = nullptr;
    float(__cdecl* GetStringWidth)(const char*, bool, bool) = nullptr;
    float* FontScaleX = nullptr;
    float ValueRightEdge = 0.0f;

    void __cdecl ValuePrint(float x, float y, const char* text)
    {
        if (bReady && (State == Mode::Page || (Manager && JoinedScreen(Screen(Manager)))))
        {
            ValueRightEdge = x;
            if (State == Mode::Page && text && *text && GetStringWidth)
                ValueAreas.Add(x, y, GetStringWidth(text, true, false), 30.0f * SCREEN_HEIGHT / 448.0f, 8.0f * SCREEN_HEIGHT / 480.0f);
        }
        ValuePrintPrevious(x, y, text);
    }

    void __cdecl LabelPrint(float x, float y, const char* text)
    {
        bool joined = State == Mode::None && Manager && JoinedScreen(Screen(Manager));
        if (!bReady || (State != Mode::Page && !joined) || ValueRightEdge <= x || !text)
            return LabelPrintPrevious(x, y, text);
        const char* value = nullptr;
        size_t count = joined ? std::size(Joined) : ShownRows.size();
        for (size_t i = 0; i < count && !value; ++i)
        {
            char key[8];
            sprintf_s(key, joined ? "WFPJ%02d" : "WFPR%02d", int(i));
            auto label = Texts.find(std::string_view(key));
            if (label != Texts.end() && label->second.c_str() == text)
            {
                auto it = Texts.find(std::string(key) + "V");
                value = it != Texts.end() ? it->second.c_str() : "";
            }
        }
        if (!value)
            return LabelPrintPrevious(x, y, text);
        float room = ValueRightEdge - x - GetStringWidth(value, true, false) - GetStringWidth("WW", true, false);
        auto fitted = MenuText::FitLabel(text, room, [](const std::string& label)
        {
            return GetStringWidth(label.c_str(), true, false);
        });
        LabelPrintPrevious(x, y, fitted.c_str());
    }

    bool AddOptionsEntry()
    {
        auto& options = aScreens[ScreenOptions];
        if (strncmp(options.name, "FET_OPT", 8) != 0)
            return false;
        int back = -1;
        for (int i = 0; i < RowCount; ++i)
        {
            if (!options.entries[i].action)
                break;
            if (strncmp(options.entries[i].name, "WFPOPT", 8) == 0)
                return false;
            if (strncmp(options.entries[i].name, "FEDS_TB", 8) == 0)
                back = i;
        }
        if (back < 1 || back + 1 >= RowCount || options.entries[back + 1].action)
            return false;
        options.entries[back + 1] = options.entries[back];
        options.entries[back] = options.entries[back - 1];
        options.entries[back].action = ActionOpen;
        strncpy_s(options.entries[back].name, "WFPOPT", _TRUNCATE);
        options.entries[back].type = TypeButton;
        options.entries[back].targetMenu = ScreenOptions;
        options.entries[back].x = options.entries[back].y = 0;
        OpenRow = back;
        return true;
    }

    // The fix's rows on Display Setup and its Advanced screen, each after its game row, as rows with a value placed
    // one line below the previous; the rows the game places at a fixed height below them move down by as much. Left
    // alone unless every anchor is there and each screen keeps room for them.
    bool AddJoinedEntries()
    {
        for (int screen : { ScreenDisplay, ScreenAdvanced })
        {
            auto& page = aScreens[screen];
            if (strncmp(page.name, "FEH_DIS", 8) != 0)
                return false;
            int count = 0;
            while (count < RowCount && page.entries[count].action)
                ++count;
            int added = int(std::count_if(std::begin(Joined), std::end(Joined), [&](auto& j) { return j.screen == screen; }));
            if (count + added > RowCount)
                return false;
            for (auto& joined : Joined)
            {
                if (joined.screen != screen)
                    continue;
                auto it = std::find_if(page.entries, page.entries + count, [&](auto& e) { return strncmp(e.name, joined.after, 8) == 0; });
                if (it == page.entries + count)
                    return false;
            }
        }
        for (int i = 0; i < int(std::size(Joined)); ++i)
        {
            auto& page = aScreens[Joined[i].screen];
            int count = 0;
            while (count < RowCount && page.entries[count].action)
                ++count;
            int at = int(std::find_if(page.entries, page.entries + count, [&](auto& e) { return strncmp(e.name, Joined[i].after, 8) == 0; }) - page.entries) + 1;
            for (int k = count; k > at; --k)
            {
                page.entries[k] = page.entries[k - 1];
                if (page.entries[k].y)
                    page.entries[k].y += LineHeight;
            }
            char key[8];
            sprintf_s(key, "WFPJ%02d", i);
            SetEntry(page.entries[at], ActionJoined + i, key, TypeValue, 0, 0, page.entries[at - 1].align);
            page.entries[at].targetMenu = int8_t(Joined[i].screen);
        }
        return true;
    }

    // CMenuManager::SetDefaultPreferences at the game's Restore Defaults (after its question): the fix's rows of that
    // screen too
    void* SetDefaultsPrevious = nullptr;
    void __fastcall SetDefaults(uint8_t* manager, void* edx, int8_t screen)
    {
        reinterpret_cast<void(__fastcall*)(uint8_t*, void*, int8_t)>(SetDefaultsPrevious)(manager, edx, screen);
        auto ids = JoinedIds(screen);
        if (!ids.empty())
        {
            RWGTAMenu::RestoreDefaults(FixSettings(), ids);
            if (Model.IsOpen())
                FillJoined();
        }
    }
}

export namespace FixOptions
{
    // From Menu.ixx's hook at the end of CMenuManager::DrawStandardMenus' right text switch
    const char* RightText(int action)
    {
        if (bReady && State == Mode::None && action >= ActionJoined && action < ActionJoined + int(std::size(Joined)))
        {
            char key[8];
            sprintf_s(key, "WFPJ%02dV", action - ActionJoined);
            auto it = Texts.find(std::string_view(key));
            return it != Texts.end() && !it->second.empty() ? it->second.c_str() : nullptr;
        }
        if (!bReady || State != Mode::Page)
            return nullptr;
        int row = action - ActionBase;
        if (row < 0 || row >= int(ShownRows.size()) || !ShownRows[row])
            return nullptr;
        char key[8];
        sprintf_s(key, "WFPR%02dV", row);
        auto it = Texts.find(std::string_view(key));
        return it != Texts.end() && !it->second.empty() ? it->second.c_str() : nullptr;
    }
}

export namespace FixOptions
{
    void BeginMenuDraw()
    {
        ValueAreas.bounds.clear();
    }

    void DrawScrollIndicators()
    {
        if (!bReady || !DrawRect || State != Mode::Page)
            return;
        const uint8_t colour[] = { 225, 225, 225, 255 };
        RWGTAMenu::DrawIndicators(Model.HiddenAbove(size_t(CurrentShownPage)), Model.HiddenBelow(size_t(CurrentShownPage)),
            12.0f, 113.0f, 430.0f, SCREEN_WIDTH, SCREEN_HEIGHT / 480.0f, colour, DrawRect);
    }
}

class OptionsInit
{
public:
    OptionsInit()
    {
        WFP::onGameInitEvent() += []()
        {
            auto screens = hook::pattern("0F BE 86 ? ? ? ? 8B 4E 54 69 C0 E2 00 00 00 8D 14 C9 0F B6 94 50");
            auto rowText = hook::pattern("57 E8 ? ? ? ? 8B D8 89 5C 24");
            auto titleText = hook::pattern("81 C2 ? ? ? ? 52 B9 ? ? ? ? E8 ? ? ? ? 8B 4C 24");
            auto userInput = hook::pattern("8A 46 5B 50 E8 ? ? ? ? 8B CE E8");
            auto processUserInput = hook::pattern("50 51 52 8B CE E8 ? ? ? ? 5E 83 C4 18 C3");
            auto accept = hook::pattern("8D 54 24 ? 52 6A 00 8B CE E8 ? ? ? ? 8A 44 24");
            auto change = hook::pattern("8D 44 24 ? 50 53 8B CE E8 ? ? ? ? 53 8B CE E8");
            if (screens.size() != 1 || rowText.size() != 1 || titleText.size() != 1 || userInput.size() != 1 || processUserInput.size() != 1 ||
                accept.size() != 1 || change.size() != 1)
                return;
            if (injector::GetBranchDestination(accept.get_first(9)).as_int() != injector::GetBranchDestination(change.get_first(8)).as_int())
                return;
            auto labelPrint = hook::pattern("53 51 52 E8 ? ? ? ? 83 C4 0C");
            auto valuePrint = hook::pattern("56 50 D8 E1 51 D9 1C 24 DD D8 E8");
            auto setScale = hook::pattern("52 50 E8 ? ? ? ? 6A 02");
            auto stringWidth = hook::pattern("53 56 8B B4 24 A4 01 00 00 56 C7 44 24");

            aScreens = reinterpret_cast<CMenuScreen*>(*screens.get_first<uint8_t*>(23) - offsetof(CMenuScreen, entries));
            if (!AddOptionsEntry())
                return;

            // The fix's rows on Display Setup and Advanced, reset by the game's Restore Defaults there
            auto restore = hook::pattern("33 D2 8A 90 ? ? ? ? 8B CE 52 E8 ? ? ? ? 0F BE 86 ? ? ? ? 69 C0 E2 00 00 00");
            if (restore.size() == 1 && AddJoinedEntries())
            {
                bJoined = true;
                SetDefaultsPrevious = injector::MakeCALL(restore.get_first(11), SetDefaults, true).get();
            }

            RowTextPrevious = injector::MakeCALL(rowText.get_first(1), RowText, true).get();
            TitleTextPrevious = injector::MakeCALL(titleText.get_first(12), TitleText, true).get();
            if (*titleText.get_first<uint8_t>(28) == 0xE8)
            {
                RenderFontBuffer = reinterpret_cast<decltype(RenderFontBuffer)>(0x719840);
                TitlePrintPrevious = reinterpret_cast<PrintStringFn>(injector::MakeCALL(titleText.get_first(28), TitlePrint, true).as_int());
            }
            UserInputPrevious = injector::MakeCALL(userInput.get_first(11), UserInput, true).get();
            ProcessUserInputPrevious = injector::MakeCALL(processUserInput.get_first(5), ProcessUserInput, true).get();
            OptionsAcceptPrevious = injector::MakeCALL(accept.get_first(9), OptionsAccept, true).get();
            OptionsChangePrevious = injector::MakeCALL(change.get_first(8), OptionsChange, true).get();
            if (labelPrint.size() == 1 && valuePrint.size() == 1 && !setScale.empty() && stringWidth.size() == 1)
            {
                auto scale = injector::GetBranchDestination(setScale.get_first(2)).get<uint8_t>();
                if (scale[8] == 0xA3)
                {
                    FontScaleX = *reinterpret_cast<float**>(scale + 9);
                    GetStringWidth = reinterpret_cast<decltype(GetStringWidth)>(stringWidth.get_first<uint8_t>(-6));
                    LabelPrintPrevious = reinterpret_cast<PrintStringFn>(injector::MakeCALL(labelPrint.get_first(3), LabelPrint, true).as_int());
                    ValuePrintPrevious = reinterpret_cast<PrintStringFn>(injector::MakeCALL(valuePrint.get_first(10), ValuePrint, true).as_int());
                }
            }
            auto wheelButtons = hook::pattern("A0 ? ? ? ? 83 C4 04 84 C0 74 4D B0 01 C2 04 00 6A 00 E8");
            if (wheelButtons.size() == 1)
                Wheel.buttons = *wheelButtons.get_first<uint8_t*>(1);
            DrawRect = reinterpret_cast<decltype(DrawRect)>(0x727B60);

            auto navigationSound = hook::pattern("68 00 00 80 3F 6A 00 6A 03 B9 ? ? ? ? E8 ? ? ? ? 0F BE 96");
            if (navigationSound.size() == 1)
            {
                Audio = *navigationSound.get_first<void*>(10);
                NavigationSound = reinterpret_cast<decltype(NavigationSound)>(injector::GetBranchDestination(navigationSound.get_first(14)).as_int());
            }

            auto oldWheelButtons = hook::pattern("A0 ? ? ? ? 84 C0 74 0C A0 ? ? ? ? 84 C0 75 03 B0 01 C3");
            for (size_t i = 0; i < oldWheelButtons.size(); ++i)
            {
                auto match = oldWheelButtons.get(i);
                if (*match.get<uint8_t*>(1) == Wheel.buttons)
                    Wheel.oldButtons = *match.get<uint8_t*>(10);
            }

            bReady = true;
        };
    }
} OptionsInit;
