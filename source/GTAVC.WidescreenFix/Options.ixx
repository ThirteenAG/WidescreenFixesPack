module;

#include <stdafx.h>
#include <intrin.h>
#include "common.h"
#include "FixSettings.hpp"
#include "../Shared/RWGTA/FixMenu.hpp"
#include "../Shared/RWGTA/RootPages.hpp"

export module Options;

import Skeleton;

// The fix's settings in the game's frontend: "Plugins" at the end of the Options screen shows the list of plugins (the
// fix first, then the other plugins with ASI Settings, see Shared/RWGTA/FixMenu.hpp), and each page its settings with
// their values like Display Setup. In the main menu and in the pause menu, which share the frontend. Display Setup
// already has the twelve rows a screen can hold, so all the fix's settings are on its page.
//
// Vice City's screen table (aScreens) has no screen the PC game leaves unused, so the Options screen shows the fix's
// rows while they are open: its rows and title are saved, replaced, and put back when the fix's pages are left (Back,
// Escape, or any other way off the screen). Nothing else of the frontend changes, and only one row is added to the
// Options screen, before its Back row; when another mod changed that row, the fix leaves the menu alone and the
// settings stay in the .cfg and FusionFixHub.
//
// The rows are answered through call sites, each calling the previous target (another mod's hook or the game): the
// text of the fix's keys (CText::Get, asked in several places of CMenuManager::Draw), CMenuManager::UserInput (to fill the rows), CMenuManager::ProcessUserInput
// (Escape on the fix's pages goes back to their list) and the value changes (CMenuManager::ProcessOnOffMenuOptions,
// whose default case ignores the fix's actions). The value at the right of a row comes through Menu.ixx's hook at
// the end of the right text's switch in CMenuManager::Draw (RightText).

namespace
{
#pragma pack(push, 1)
    struct CMenuEntry
    {
        int16_t action;
        char name[8];
        int8_t saveSlot;
        int8_t targetMenu;
        uint16_t x;
        uint16_t y;
        uint8_t align;
        uint8_t pad;
    };

    struct CMenuScreen
    {
        char name[8];
        int8_t previousPage;
        int8_t parentEntry;
        CMenuEntry entries[12];
    };
#pragma pack(pop)
    static_assert(sizeof(CMenuEntry) == 18 && sizeof(CMenuScreen) == 226);

    constexpr int RowCount = 12;
    constexpr int PageRows = 10;            // settings visible at once; one more slot includes Back
    constexpr int RootRows = 9;             // links of the list; Back follows
    constexpr int ScreenOptions = 27;
    constexpr int16_t ActionChangeMenu = 4;
    constexpr int16_t ActionBase = 0x5740;  // the fix's rows: ActionBase + row; far from the game's and other mods' ids
    constexpr int16_t ActionOpen = ActionBase + 0x80;
    constexpr uint8_t AlignLeft = 1, AlignCenter = 3;

    // CMenuManager
    constexpr size_t CurrentOption = 0x30;
    constexpr size_t GameNotLoaded = 0x6C;
    constexpr size_t CurrentScreen = 0xF8;
    constexpr size_t FadeAlpha = 0x108;     // the screen fades in from 0 after a switch

    CMenuScreen* aScreens = nullptr;
    int32_t* pLanguage = nullptr;
    bool bReady = false;

    enum class Mode { None, Root, Page };
    Mode State = Mode::None;
    CMenuScreen SavedOptions{};
    int OpenRow = 0;                        // the "Plugins" row of the Options screen
    int CurrentPage = 0;
    int RootRowCount = 0;                   // links shown in the list

    RWGTAMenu::Model Model;
    RWGTAMenu::WheelInput Wheel;
    RWGTAMenu::ValueAreas ValueAreas;
    void* Audio = nullptr;
    void(__fastcall* NavigationSound)(void*, void*, uint16_t, uint32_t) = nullptr;
    void(__cdecl* DrawRect)(const float*, const uint8_t*) = nullptr;
    using ValuePrintFn = void(__cdecl*)(float, float, const wchar_t*);
    ValuePrintFn ValuePrintPrevious = nullptr;
    float(__cdecl* ValueStringWidth)(const wchar_t*, bool) = nullptr;

    RWGTAMenu::RootPages Root;
    std::vector<const RWGTAMenu::Row*> ShownRows;
    std::map<std::string, std::u16string, std::less<>> Texts;   // the rows' and titles' text by key, in the game's font

    int32_t& Screen(uint8_t* manager) { return *reinterpret_cast<int32_t*>(manager + CurrentScreen); }
    int32_t& Option(uint8_t* manager) { return *reinterpret_cast<int32_t*>(manager + CurrentOption); }
    bool InGame(uint8_t* manager) { return !*(manager + GameNotLoaded); }
    // A new page fades in like the game's own screens
    void FadeIn(uint8_t* manager) { *reinterpret_cast<int32_t*>(manager + FadeAlpha) = 0; }

    // The language of the fix's texts and of the other plugins' pages: MAIN.TextLanguage (cfg and editors only, for
    // translations of the game that replaced one of its languages), or the game's language
    std::string LanguageTag()
    {
        auto& chosen = FixSettings().String("MAIN.TextLanguage");
        if (!chosen.empty() && chosen != "auto")
            return chosen;
        static const char* tags[] = { "en", "fr", "de", "it", "es" };
        int language = pLanguage ? *pLanguage : 0;
        return language >= 0 && language < int(std::size(tags)) ? tags[language] : "en";
    }

    void __cdecl ValuePrint(float x, float y, const wchar_t* text)
    {
        if (bReady && State == Mode::Page && text && *text && ValueStringWidth)
            ValueAreas.Add(x, y, ValueStringWidth(text, true), 25.0f * SCREEN_HEIGHT / 448.0f, 8.0f * SCREEN_HEIGHT / 480.0f);
        ValuePrintPrevious(x, y, text);
    }

    void SetText(const std::string& key, const std::string& utf8)
    {
        Texts[key] = RWGTAMenu::ToGameFont(utf8, false);
    }

    void SetEntry(CMenuEntry& entry, int16_t action, const char* name, uint16_t x, uint16_t y, uint8_t align)
    {
        entry = {};
        entry.action = action;
        strncpy_s(entry.name, name, _TRUNCATE);
        entry.targetMenu = ScreenOptions;
        entry.x = x;
        entry.y = y;
        entry.align = align;
    }

    // The list of pages: a link per page, then Back
    void FillRoot()
    {
        auto& screen = aScreens[ScreenOptions];
        for (auto& entry : screen.entries)
            entry = {};
        strncpy_s(screen.name, "WFPT0", _TRUNCATE);
        auto& pages = Model.Pages();
        RootRowCount = int(Root.Rows());
        uint16_t y = 132;
        for (int i = 0; i < RootRowCount; ++i, y += 29)
        {
            char key[8];
            sprintf_s(key, "WFPL%02d", i);
            int page = Root.Page(size_t(i));
            SetText(key, page >= 0 ? pages[page].title : Model.Text(page == RWGTAMenu::RootPages::Previous ? "menu.previous" : "menu.next"));
            SetEntry(screen.entries[i], ActionBase + i, key, 320, y, AlignCenter);
        }
        SetEntry(screen.entries[RootRowCount], ActionBase + RootRowCount, "FEDS_TB", 320, y, AlignCenter);
    }

    // A page: its rows with their values, then Back
    void FillPage(uint8_t* manager)
    {
        auto& screen = aScreens[ScreenOptions];
        for (auto& entry : screen.entries)
            entry = {};
        strncpy_s(screen.name, "WFPT1", _TRUNCATE);
        ShownRows = Model.WindowRows(size_t(CurrentPage));
        int count = int(ShownRows.size());
        uint16_t y = 78;
        for (int i = 0; i < count; ++i, y += 25)
        {
            char key[8];
            sprintf_s(key, "WFPR%02d", i);
            if (!ShownRows[i])
            {
                SetEntry(screen.entries[i], ActionBase + i, "FEDS_TB", 320, y + 10, AlignCenter);
                continue;
            }
            SetText(key, Model.Label(*ShownRows[i]));
            SetText(std::string(key) + "V", Model.Value(*ShownRows[i]));
            SetEntry(screen.entries[i], ActionBase + i, key, 40, y, AlignLeft);
        }
        if (Option(manager) >= count)
            Option(manager) = std::max(0, count - 1);
        if (size_t(CurrentPage) < Model.Pages().size())
            SetText("WFPT1", Model.Pages()[CurrentPage].title);
    }

    void Open(uint8_t* manager)
    {
        HMODULE self = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCWSTR>(&Open), &self);
        Model.OpenPlugins(self, LanguageTag(), PageRows, {}, true);
        SetText("WFPT0", Model.Text("menu.plugins"));
        Root.Reset(Model.Pages().size(), RootRows);
        SavedOptions = aScreens[ScreenOptions];
        State = Mode::Root;
        FillRoot();
        Option(manager) = 0;
        FadeIn(manager);
    }

    // The Options screen as it was
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
        CurrentPage = page;
        Model.PageShown(size_t(page));
        State = Mode::Page;
        Option(manager) = 0;
        FillPage(manager);
        FadeIn(manager);
    }

    void ShowRoot(uint8_t* manager)
    {
        State = Mode::Root;
        FillRoot();
        Option(manager) = int(Root.Selection(size_t(CurrentPage)));
        FadeIn(manager);
    }

    void Back(uint8_t* manager)
    {
        if (State == Mode::Page)
        {
            ShowRoot(manager);
        }
        else if (State == Mode::Root)
        {
            Close();
            Option(manager) = OpenRow;
            FadeIn(manager);
        }
    }

    // Before and after the game's input: the rows of the page shown, from the settings in effect. Any other way off
    // the Options screen puts it back.
    void Update(uint8_t* manager)
    {
        if (State == Mode::None)
        {
            if (Screen(manager) == ScreenOptions)
                SetText("WFPOPT", RWGTAMenu::PluginText(FixSettings(), LanguageTag(), "menu.plugins"));
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
            float(*reinterpret_cast<int32_t*>(manager + 0x64)), float(*reinterpret_cast<int32_t*>(manager + 0x68)));
        int wheel{ Wheel.Begin(bReady && State == Mode::Page && Screen(manager) == ScreenOptions && !overValue) };
        if (wheel)
        {
            Model.Scroll(size_t(CurrentPage), wheel);
            FillPage(manager);
        }
        reinterpret_cast<void(__fastcall*)(uint8_t*, void*)>(UserInputPrevious)(manager, edx);
        Wheel.End();
        if (bReady)
            Update(manager);
    }

    // Escape (and the game's other ways back) on the fix's pages: back to their list, or to the Options screen
    void* ProcessUserInputPrevious = nullptr;
    void __fastcall ProcessUserInput(uint8_t* manager, void* edx, int32_t down, int32_t up, int32_t accept, int32_t back, int32_t change)
    {
        if (bReady && State != Mode::None && Screen(manager) == ScreenOptions && int8_t(back & 0xFF))
        {
            Back(manager);
            back = 0;
        }
        if (bReady && State == Mode::Page && Screen(manager) == ScreenOptions && (uint8_t(down) || uint8_t(up)))
        {
            if (NavigationSound)
                NavigationSound(Audio, nullptr, 0xC3, 0);
            *reinterpret_cast<int32_t*>(manager + 0x10C) = 0;
            Option(manager) = Model.Move(size_t(CurrentPage), Option(manager), uint8_t(down) ? 1 : -1);
            FillPage(manager);
            down = up = 0;
        }
        reinterpret_cast<void(__fastcall*)(uint8_t*, void*, int32_t, int32_t, int32_t, int32_t, int32_t)>(ProcessUserInputPrevious)(manager, edx, down, up, accept, back, change);
    }

    // CMenuManager::ProcessOnOffMenuOptions on accept (direction 0) and left/right; true when the row was the fix's
    bool __cdecl ProcessOnOff(uint8_t* manager, int32_t direction)
    {
        if (!bReady || Screen(manager) != ScreenOptions)
            return false;
        int option = Option(manager);
        if (option < 0 || option >= RowCount)
            return false;
        int action = aScreens[ScreenOptions].entries[option].action;
        int dir = int(int8_t(direction & 0xFF));
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

    void* OnOffAcceptPrevious = nullptr;
    void* OnOffChangePrevious = nullptr;

    // ProcessOnOffMenuOptions is thiscall without arguments: called as fastcall, the previous target gets ecx and edx
    // as they were
    using OnOffFunction = uint32_t(__fastcall*)(uint8_t*, void*);

    // Accept: no direction
    uint32_t __fastcall OnOffAccept(uint8_t* manager, void* edx)
    {
        if (ProcessOnOff(manager, 0))
            return 1;
        return reinterpret_cast<OnOffFunction>(OnOffAcceptPrevious)(manager, edx);
    }

    // Left or right: the direction is ProcessUserInput's last argument, 0x84 above the stack at the call (0x88 above
    // the return address)
    uint32_t __fastcall OnOffChange(uint8_t* manager, void* edx)
    {
        auto caller = reinterpret_cast<uint8_t*>(_AddressOfReturnAddress());
        if (ProcessOnOff(manager, *reinterpret_cast<int32_t*>(caller + 0x88)))
            return 1;
        return reinterpret_cast<OnOffFunction>(OnOffChangePrevious)(manager, edx);
    }

    const wchar_t* Answer(const char* key)
    {
        if (!bReady || !key || strncmp(key, "WFP", 3) != 0)
            return nullptr;
        auto it = Texts.find(std::string_view(key, strnlen(key, 8)));
        return it != Texts.end() ? reinterpret_cast<const wchar_t*>(it->second.c_str()) : nullptr;
    }

    // CText::Get at its call sites in CMenuManager::Draw that look up a computed key (the row, the screen's title, the
    // highlight's width): each keeps the target it had (another mod's hook or the game). Not at CText::Get itself:
    // CLEO replaces its start without calling what was there.
    std::map<uintptr_t, void*> TextPrevious;    // by return address
    void* TextDefault = nullptr;                // for calls from elsewhere (code that took the target from a hooked site)

    const wchar_t* __fastcall Text(void* text, void* edx, const char* key)
    {
        if (auto answer = Answer(key))
            return answer;
        auto previous = TextPrevious.find(reinterpret_cast<uintptr_t>(_ReturnAddress()));
        auto target = previous != TextPrevious.end() ? previous->second : TextDefault;
        return reinterpret_cast<const wchar_t* (__fastcall*)(void*, void*, const char*)>(target)(text, edx, key);
    }

    // The calls of CText::Get from a register-pushed key (push reg; call) around the row's call site
    void HookTextCalls(uint8_t* rowCall)
    {
        auto get = injector::GetBranchDestination(rowCall).as_int();
        TextDefault = reinterpret_cast<void*>(get);
        for (auto p = rowCall - 0xC00; p < rowCall + 0x1700; ++p)
        {
            if (p[0] != 0xE8 || injector::GetBranchDestination(p).as_int() != get)
                continue;
            bool registerKey = (p[-1] >= 0x50 && p[-1] <= 0x57) || (p[-6] >= 0x50 && p[-6] <= 0x57 && p[-5] == 0xB9);
            if (!registerKey)
                continue;
            TextPrevious[reinterpret_cast<uintptr_t>(p + 5)] = injector::MakeCALL(p, Text, true).get();
        }
    }

    // "Plugins" before the Options screen's Back row, when the screen is as the game shipped it
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
        options.entries[back].saveSlot = 0;
        options.entries[back].targetMenu = ScreenOptions;
        options.entries[back].x = options.entries[back].y = 0;
        OpenRow = back;
        return true;
    }
}

export namespace FixOptions
{
    // From Menu.ixx's hook at the end of CMenuManager::Draw's right text switch: the value of a row of the fix
    const wchar_t* RightText(int action)
    {
        if (!bReady || State != Mode::Page)
            return nullptr;
        int row = action - ActionBase;
        if (row < 0 || row >= int(ShownRows.size()) || !ShownRows[row])
            return nullptr;
        char key[8];
        sprintf_s(key, "WFPR%02dV", row);
        auto it = Texts.find(std::string_view(key));
        return it != Texts.end() && !it->second.empty() ? reinterpret_cast<const wchar_t*>(it->second.c_str()) : nullptr;
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
        const uint8_t colour[] = { 235, 175, 225, 255 };
        RWGTAMenu::DrawIndicators(Model.HiddenAbove(size_t(CurrentPage)), Model.HiddenBelow(size_t(CurrentPage)),
            12.0f, 68.0f, 370.0f, SCREEN_WIDTH, SCREEN_HEIGHT / 480.0f, colour, DrawRect);
    }
}

class OptionsInit
{
public:
    OptionsInit()
    {
        WFP::onGameInitEvent() += []()
        {
            auto screens = hook::pattern("8B 93 ? ? ? ? 8B 4B 30 6B D2 71 8D 0C C9 01 CA 0F BF 04 55");
            auto language = hook::pattern("A1 ? ? ? ? 83 C4 14 83 F8 03 74 0A 83 F8 01");
            auto rowText = hook::pattern("B9 ? ? ? ? E8 ? ? ? ? 89 44 24 ? 80 7D");
            auto userInput = hook::pattern("80 7B 38 00 74 ? 89 D9 E8");
            auto processUserInput = hook::pattern("89 E9 FF 74 24 ? FF 74 24 ? FF 74 24 ? FF 74 24 ? FF 74 24 ? E8");
            auto accept = hook::pattern("89 E9 E8 ? ? ? ? 80 BC 24");
            auto change = hook::pattern("50 E8 ? ? ? ? 89 E9 E8 ? ? ? ? 83 BD");
            if (screens.size() != 1 || language.empty() || rowText.size() != 1 || userInput.size() != 1 ||
                processUserInput.size() != 1 || accept.size() != 1 || change.empty())
                return;

            aScreens = reinterpret_cast<CMenuScreen*>(*screens.get_first<uint8_t*>(21) - offsetof(CMenuScreen, entries));
            pLanguage = *language.get_first<int32_t*>(1);

            // The change call site is the one that reaches the same function as the accept one
            auto onOff = injector::GetBranchDestination(accept.get_first(2)).as_int();
            void* changeCall = nullptr;
            for (size_t i = 0; i < change.size(); ++i)
            {
                if (injector::GetBranchDestination(change.get(i).get<void>(8)).as_int() == onOff)
                    changeCall = change.get(i).get<void>(8);
            }
            if (!changeCall || !AddOptionsEntry())
                return;

            HookTextCalls(rowText.get_first<uint8_t>(5));
            UserInputPrevious = injector::MakeCALL(userInput.get_first(8), UserInput, true).get();
            ProcessUserInputPrevious = injector::MakeCALL(processUserInput.get_first(22), ProcessUserInput, true).get();
            OnOffAcceptPrevious = injector::MakeCALL(accept.get_first(2), OnOffAccept, true).get();
            OnOffChangePrevious = injector::MakeCALL(changeCall, OnOffChange, true).get();

            auto valuePrint = hook::pattern("53 50 D9 14 24 50 D9 C1 D9 1C 24 DE D9 E8");
            if (valuePrint.size() == 1)
            {
                ValueStringWidth = reinterpret_cast<decltype(ValueStringWidth)>(0x550650);
                ValuePrintPrevious = reinterpret_cast<ValuePrintFn>(injector::MakeCALL(valuePrint.get_first(13), ValuePrint, true).as_int());
            }
            auto wheelButtons = hook::pattern("80 3D ? ? ? ? 00 59 75 11 6A 00 E8 ? ? ? ? 80 3D ? ? ? ? 00 59");
            for (size_t i = 0; i < wheelButtons.size(); ++i)
            {
                auto match = wheelButtons.get(i);
                auto up = *match.get<uint8_t*>(2);
                if (*match.get<uint8_t*>(19) == up + 1)
                    Wheel.buttons = up;
            }
            auto rectangle = hook::pattern("8B 44 24 04 53 8B 5C 24 0C 53 53 53 53 50 E8");
            if (rectangle.size() == 1)
                DrawRect = reinterpret_cast<decltype(DrawRect)>(rectangle.get_first());

            auto navigationSound = hook::pattern("B9 ? ? ? ? 6A 00 68 C3 00 00 00 E8 ? ? ? ? FF 45 30");
            if (navigationSound.size() == 1)
            {
                Audio = *navigationSound.get_first<void*>(1);
                NavigationSound = reinterpret_cast<decltype(NavigationSound)>(injector::GetBranchDestination(navigationSound.get_first(12)).as_int());
            }

            auto oldWheelButtons = hook::pattern("80 3D ? ? ? ? 00 59 75 01 40 84 C0 74 0B 80 3D ? ? ? ? 00");
            for (size_t i = 0; i < oldWheelButtons.size(); ++i)
            {
                auto match = oldWheelButtons.get(i);
                if (*match.get<uint8_t*>(2) == Wheel.buttons)
                    Wheel.oldButtons = *match.get<uint8_t*>(17);
            }

            bReady = true;
        };
    }
} OptionsInit;
