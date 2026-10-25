module;

#include <stdafx.h>
#include "../Shared/CheckedPattern.hpp"
#include <intrin.h>
#include "common.h"
#include "FixSettings.hpp"
#include "../Shared/RWGTA/FixMenu.hpp"
#include "../Shared/RWGTA/RootPages.hpp"

export module Options;

import Skeleton;

// The fix's settings in the game's frontend: "Plugins" at the end of the Options screen opens the list of plugins
// (the fix first, then the other plugins with ASI Settings, see Shared/RWGTA/FixMenu.hpp), and each page shows its
// settings with their values like the Display Setup screen. Console gamma, the field of view and anti-aliasing are
// rows of Display Setup itself, next to brightness, draw distance and the screen resolution; the game's Restore
// Defaults there resets them too. In the main menu and in the pause menu, which share the frontend.
//
// Kept minimal so that it works next to other mods of the frontend (SilentPatch, SkyGfx, menu and widescreen mods):
// - No code of the game is replaced. The pages are two screens of the game's own screen table (aScreens) that the PC
//   game never shows, the PlayStation 2 memory card screens 19 and 20, and one row added to the Options screen. Each
//   is checked to be as the game shipped it; when another mod changed it, the fix leaves the menu alone and the
//   settings stay in the .cfg and FusionFixHub.
// - Rows are answered through call sites, each calling the previous target (another mod's hook or the game): the
//   row's and the screen's text (CText::Get), the button processing (CMenuManager::ProcessButtonPresses, to fill the
//   rows) and the value changes (CMenuManager::ProcessOnOffMenuOptions, whose default case ignores the fix's actions).
// - The value at the right of a row comes from a mid hook at the end of the right text's switch in CMenuManager::Draw.
// - The page screen uses Display Setup's layout: its entry of Draw's layout jump table is set to Display Setup's.
// - Display Setup gets its rows only when its anchors are there and it keeps room (13 rows, as the pages use).

namespace
{
    struct CMenuEntry
    {
        int32_t action;
        char name[8];
        int32_t saveSlot;
        int32_t targetMenu;
    };

    struct CMenuScreen
    {
        char name[8];
        int32_t unknown;
        int32_t previousPage[2];    // [0] in the main menu (no game started), [1] in the pause menu
        int32_t parentEntry[2];
        CMenuEntry entries[18];
    };
    static_assert(sizeof(CMenuScreen) == 0x184);

    constexpr int RowCount = 18;
    constexpr int PageRows = 12;            // settings per page; Back follows
    constexpr int ScreenDisplay = 6;
    constexpr int ScreenOptions = 41;
    constexpr int ScreenRoot = 19;          // FEM_MCM: the PS2 memory card menu
    constexpr int ScreenPage = 20;          // FEM_MC2
    constexpr int32_t ActionLabel = 1;      // a line of text above the rows (the first entry only)
    constexpr int32_t ActionChangeMenu = 2;
    constexpr int32_t ActionBase = 0x57460; // the fix's rows: ActionBase + row; far from the game's and other mods' ids
    constexpr int32_t ActionRoot = ActionBase + 0x80;
    constexpr int32_t ActionJoined = ActionBase + 0x40; // the fix's rows on Display Setup

    // The fix's settings on Display Setup, each after the game's row of that key
    struct JoinedSetting
    {
        const char* id;
        const char* after;
    };
    constexpr JoinedSetting Joined[] =
    {
        { "GRAPHICS.ConsoleGamma", "FED_BRI" },
        { "MAIN.FOVFactor", "FEM_LOD" },
        { "GRAPHICS.AntiAliasing", "FED_RES" },
    };
    bool bJoined = false;                   // the rows are on Display Setup

    std::set<std::string> JoinedIds()
    {
        std::set<std::string> ids;
        if (bJoined)
        {
            for (auto& joined : Joined)
                ids.insert(joined.id);
        }
        return ids;
    }

    // CMenuManager
    constexpr size_t CurrentScreen = 0x548;
    constexpr size_t CurrentEntry = 0x54C;
    constexpr size_t MainMenuSwitch = 0x116;

    CMenuScreen* aScreens = nullptr;
    int32_t* pLanguage = nullptr;
    size_t RightTextOffset = 0xC;           // Draw's right text pointer on the stack
    bool bReady = false;

    void* DMAudio = nullptr;
    void(__fastcall* PlayFrontEndSound)(void* audio, void* edx, uint16_t sound, uint32_t volume) = nullptr;

    RWGTAMenu::Model Model;
    RWGTAMenu::WheelInput Wheel;
    RWGTAMenu::ValueAreas ValueAreas;
    void(__cdecl* DrawRect)(const float*, const uint8_t*) = nullptr;
    using ValuePrintFn = void(__cdecl*)(float, float, const wchar_t*);
    ValuePrintFn ValuePrintPrevious = nullptr;
    float(__cdecl* ValueStringWidth)(const wchar_t*, bool) = nullptr;

    RWGTAMenu::RootPages Root;
    int CurrentPage = -1;                   // the page shown on ScreenPage
    int RootEntry = 0;                      // the row of the list last selected
    int LastScreen = -1;
    std::vector<const RWGTAMenu::Row*> ShownRows;
    bool LabelShown = false;                // the page starts with the restart line
    bool LabelReady = false;                // the label's text is answered
    std::map<std::string, std::u16string, std::less<>> Texts;   // the rows' and titles' text by key, in the game's font

    int32_t& Screen(uint8_t* manager) { return *reinterpret_cast<int32_t*>(manager + CurrentScreen); }
    int32_t& Entry(uint8_t* manager) { return *reinterpret_cast<int32_t*>(manager + CurrentEntry); }
    bool InGame(uint8_t* manager) { return !*(manager + MainMenuSwitch); }

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
        if (bReady && LastScreen == ScreenPage && text && *text && ValueStringWidth)
            ValueAreas.Add(x, y, ValueStringWidth(text, true), 22.0f * SCREEN_HEIGHT / 480.0f, 8.0f * SCREEN_HEIGHT / 480.0f);
        ValuePrintPrevious(x, y, text);
    }

    void SetText(const std::string& key, const std::string& utf8)
    {
        Texts[key] = RWGTAMenu::ToGameFont(utf8, true, false);
    }

    void SetEntry(CMenuEntry& entry, int32_t action, const char* name, int32_t target)
    {
        entry.action = action;
        strncpy_s(entry.name, name, _TRUNCATE);
        entry.saveSlot = 0;
        entry.targetMenu = target;
    }

    void ClearEntries(CMenuScreen& screen)
    {
        for (auto& entry : screen.entries)
            entry = {};
    }

    // The list of pages: a link per page, then Back
    void FillRoot()
    {
        auto& screen = aScreens[ScreenRoot];
        ClearEntries(screen);
        auto& pages = Model.Pages();
        int count = int(Root.Rows());
        for (int i = 0; i < count; ++i)
        {
            char key[8];
            sprintf_s(key, "WFPL%02d", i);
            int page = Root.Page(size_t(i));
            SetText(key, page >= 0 ? pages[page].title : Model.Text(page == RWGTAMenu::RootPages::Previous ? "menu.previous" : "menu.next"));
            SetEntry(screen.entries[i], page >= 0 ? ActionChangeMenu : ActionRoot + i, key, page >= 0 ? ScreenPage : ScreenRoot);
        }
        SetEntry(screen.entries[count], ActionChangeMenu, "FEDS_TB", 0);
    }

    // A page: its rows with their values, then Back
    void FillPage(uint8_t* manager)
    {
        auto& screen = aScreens[ScreenPage];
        ClearEntries(screen);
        ShownRows = Model.WindowRows(size_t(CurrentPage));
        // A changed setting that applies at the next start: the game's own line of text above the rows says so
        bool label = LabelReady && Model.RestartRequired();
        if (label != LabelShown)
        {
            Entry(manager) = std::max(int(label), Entry(manager) + (label ? 1 : -1));
            LabelShown = label;
        }
        int first = label ? 1 : 0;
        if (label)
        {
            SetText("WFPWARN", Model.Text("menu.restartwarning"));
            SetEntry(screen.entries[0], ActionLabel, "WFPWARN", ScreenPage);
        }
        int count = int(ShownRows.size());
        for (int i = 0; i < count; ++i)
        {
            char key[8];
            sprintf_s(key, "WFPR%02d", i);
            if (!ShownRows[i])
            {
                SetEntry(screen.entries[first + i], ActionChangeMenu, "FEDS_TB", 0);
                continue;
            }
            SetText(key, Model.Label(*ShownRows[i]));
            SetText(std::string(key) + "V", Model.Value(*ShownRows[i]));
            SetEntry(screen.entries[first + i], ActionBase + i, key, ScreenPage);
        }
        if (Entry(manager) >= first + count)
            Entry(manager) = std::max(first, first + count - 1);
        if (CurrentPage >= 0 && size_t(CurrentPage) < Model.Pages().size())
            SetText("WFPT1", Model.Pages()[CurrentPage].title);
    }

    // The model's row of a setting on Display Setup
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

    // The labels and values of the fix's rows on Display Setup
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

    // Before and after the game processes the buttons: the rows of the screen shown, from the settings in effect
    void Update(uint8_t* manager)
    {
        int screen = Screen(manager);
        bool ours = screen == ScreenRoot || screen == ScreenPage || (bJoined && screen == ScreenDisplay);
        if (screen == ScreenOptions)
            SetText("WFPOPT", RWGTAMenu::PluginText(FixSettings(), LanguageTag(), "menu.plugins"));
        if (ours && !Model.IsOpen())
        {
            HMODULE self = nullptr;
            GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCWSTR>(&Update), &self);
            Model.OpenPlugins(self, LanguageTag(), PageRows, JoinedIds(), true);
            SetText("WFPT0", Model.Text("menu.plugins"));
            Root.Reset(Model.Pages().size(), PageRows);
            FillRoot();
        }
        else if (!ours && screen != LastScreen && Model.IsOpen())
        {
            Model.Close();
        }

        if (screen == ScreenRoot)
        {
            RootEntry = Entry(manager);
            // Back from a page returns to its link
            aScreens[ScreenPage].parentEntry[0] = aScreens[ScreenPage].parentEntry[1] = RootEntry;
            if (LastScreen != ScreenRoot)
                FillRoot();
        }
        else if (screen == ScreenPage)
        {
            if (LastScreen != ScreenPage)
            {
                LabelShown = false;
                CurrentPage = Root.Page(size_t(RootEntry));
                Model.PageShown(size_t(CurrentPage));
            }
            FillPage(manager);
        }
        else if (screen == ScreenDisplay && bJoined)
        {
            FillJoined();
        }
        LastScreen = screen;
    }

    void* ProcessButtonPressesPrevious = nullptr;
    void __fastcall ProcessButtonPresses(uint8_t* manager, void* edx)
    {
        if (bReady)
            Update(manager);
        bool overValue = ValueAreas.Contains(
            float(*reinterpret_cast<int32_t*>(manager + 0x120)), float(*reinterpret_cast<int32_t*>(manager + 0x124)));
        int wheel{ Wheel.Begin(bReady && Screen(manager) == ScreenPage && !overValue) };
        if (wheel)
        {
            Model.Scroll(size_t(CurrentPage), wheel);
            FillPage(manager);
        }
        reinterpret_cast<void(__fastcall*)(uint8_t*, void*)>(ProcessButtonPressesPrevious)(manager, edx);
        Wheel.End();
        if (bReady)
            Update(manager);
    }

    // CMenuManager::ProcessOnOffMenuOptions runs on accept, left and right with the direction in a local of its
    // caller (0 for accept). The fix's rows are handled here; true when the row was one of them.
    bool __cdecl ProcessOnOff(uint8_t* manager, int32_t direction)
    {
        if (!bReady)
            return false;
        int entry = Entry(manager);
        if (entry < 0 || entry >= RowCount)
            return false;
        int dir = int(int8_t(direction & 0xFF));
        int step = dir > 0 ? 1 : dir < 0 ? -1 : 0;
        if (Screen(manager) == ScreenRoot)
        {
            int row = aScreens[ScreenRoot].entries[entry].action - ActionRoot;
            if (row < 0 || row >= int(Root.Rows()))
                return false;
            if (dir == 0 && Root.Move(Root.Page(size_t(row))))
            {
                FillRoot();
                Entry(manager) = 0;
            }
            return true;
        }
        if (bJoined && Screen(manager) == ScreenDisplay)
        {
            auto joined = JoinedRow(aScreens[ScreenDisplay].entries[entry].action - ActionJoined);
            if (!joined)
                return false;
            if (Model.Change(*joined, step, InGame(manager)) && dir != 0 && PlayFrontEndSound)
                PlayFrontEndSound(DMAudio, nullptr, 0x98, 0);
            FillJoined();
            return true;
        }
        if (Screen(manager) != ScreenPage)
            return false;
        int row = aScreens[ScreenPage].entries[entry].action - ActionBase;
        if (row < 0 || row >= int(ShownRows.size()) || !ShownRows[row])
            return false;
        bool changed = Model.Change(*ShownRows[row], step, InGame(manager));
        // Accept has its sound from the game already
        if (changed && dir != 0 && PlayFrontEndSound)
            PlayFrontEndSound(DMAudio, nullptr, 0x98, 0);
        FillPage(manager);
        return true;
    }

    // The two call sites (accept, and left/right), each with the target it had. Left/right: the direction is a local of
    // the caller at the top of its stack
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

    // Left or right: the direction is the caller's local at the top of its stack, just above the return address
    uint32_t __fastcall OnOffChange(uint8_t* manager, void* edx)
    {
        auto caller = reinterpret_cast<uint8_t*>(_AddressOfReturnAddress());
        if (ProcessOnOff(manager, *reinterpret_cast<int32_t*>(caller + 4)))
            return 1;
        return reinterpret_cast<OnOffFunction>(OnOffChangePrevious)(manager, edx);
    }

    // CText::Get at the rows' and the title's call sites in CMenuManager::Draw
    void* RowTextPrevious = nullptr;
    void* TitleTextPrevious = nullptr;
    void* LabelTextPrevious = nullptr;

    const wchar_t* Answer(const char* key)
    {
        if (!bReady || !key || strncmp(key, "WFP", 3) != 0)
            return nullptr;
        auto it = Texts.find(std::string_view(key, strnlen(key, 8)));
        return it != Texts.end() ? reinterpret_cast<const wchar_t*>(it->second.c_str()) : nullptr;
    }

    const wchar_t* __fastcall RowText(void* text, void* edx, const char* key)
    {
        if (auto answer = Answer(key))
            return answer;
        return reinterpret_cast<const wchar_t* (__fastcall*)(void*, void*, const char*)>(RowTextPrevious)(text, edx, key);
    }

    const wchar_t* __fastcall TitleText(void* text, void* edx, const char* key)
    {
        if (auto answer = Answer(key))
            return answer;
        return reinterpret_cast<const wchar_t* (__fastcall*)(void*, void*, const char*)>(TitleTextPrevious)(text, edx, key);
    }

    const wchar_t* __fastcall LabelText(void* text, void* edx, const char* key)
    {
        if (auto answer = Answer(key))
            return answer;
        return reinterpret_cast<const wchar_t* (__fastcall*)(void*, void*, const char*)>(LabelTextPrevious)(text, edx, key);
    }

    // Checks the game's screens before they are changed; false when another mod changed them
    bool ScreensAsShipped()
    {
        auto& root = aScreens[ScreenRoot];
        auto& page = aScreens[ScreenPage];
        if (strncmp(root.name, "FEM_MCM", 8) != 0 || strncmp(page.name, "FEM_MC2", 8) != 0)
            return false;
        // Nothing of the game leads to them
        for (int s = 0; s < 57; ++s)
        {
            for (auto& entry : aScreens[s].entries)
            {
                if (entry.action == ActionChangeMenu && (entry.targetMenu == ScreenRoot || entry.targetMenu == ScreenPage) && s != ScreenRoot && s != ScreenPage)
                    return false;
            }
        }
        return strncmp(aScreens[ScreenOptions].name, "FET_OPT", 8) == 0 && strncmp(aScreens[ScreenDisplay].name, "FET_DIS", 8) == 0;
    }

    // "Plugins" before the Options screen's Back row; the screen is left as it is when it has no room or
    // its Back row isn't where it was
    bool AddOptionsEntry()
    {
        auto& options = aScreens[ScreenOptions];
        int back = -1;
        for (int i = 0; i < RowCount; ++i)
        {
            if (!options.entries[i].action)
                break;
            if (strncmp(options.entries[i].name, "WFPOPT", 8) == 0)
                return true;
            if (strncmp(options.entries[i].name, "FEDS_TB", 8) == 0)
                back = i;
        }
        if (back < 0 || back + 1 >= RowCount || options.entries[back + 1].action)
            return false;
        options.entries[back + 1] = options.entries[back];
        SetEntry(options.entries[back], ActionChangeMenu, "WFPOPT", ScreenRoot);

        auto& root = aScreens[ScreenRoot];
        root.previousPage[0] = root.previousPage[1] = ScreenOptions;
        root.parentEntry[0] = root.parentEntry[1] = back;
        auto& page = aScreens[ScreenPage];
        page.previousPage[0] = page.previousPage[1] = ScreenRoot;
        page.parentEntry[0] = page.parentEntry[1] = 0;
        strncpy_s(root.name, "WFPT0", _TRUNCATE);
        strncpy_s(page.name, "WFPT1", _TRUNCATE);
        ClearEntries(root);
        ClearEntries(page);
        return true;
    }

    // The fix's rows on Display Setup, each after its game row; left alone unless every anchor is there and the
    // screen keeps room for them
    bool AddJoinedEntries()
    {
        auto& display = aScreens[ScreenDisplay];
        int count = 0;
        while (count < RowCount && display.entries[count].action)
            ++count;
        if (count + int(std::size(Joined)) > PageRows + 1)
            return false;
        auto find = [&](const char* name)
        {
            return int(std::find_if(display.entries, display.entries + count, [&](auto& e) { return strncmp(e.name, name, 8) == 0; }) - display.entries);
        };
        for (auto& joined : Joined)
        {
            if (find(joined.after) == count)
                return false;
        }
        for (int i = 0; i < int(std::size(Joined)); ++i)
        {
            int at = find(Joined[i].after) + 1;
            for (int k = count; k > at; --k)
                display.entries[k] = display.entries[k - 1];
            char key[8];
            sprintf_s(key, "WFPJ%02d", i);
            SetEntry(display.entries[at], ActionJoined + i, key, ScreenDisplay);
            ++count;
        }
        return true;
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
        if (!bReady || !DrawRect || !Model.IsOpen() || LastScreen != ScreenPage)
            return;
        const uint8_t colour[] = { 150, 170, 235, 255 };
        RWGTAMenu::DrawIndicators(Model.HiddenAbove(size_t(CurrentPage)), Model.HiddenBelow(size_t(CurrentPage)),
            12.0f, 68.0f, 356.0f, SCREEN_WIDTH, SCREEN_HEIGHT / 480.0f, colour, DrawRect);
    }
}

class Options
{
public:
    Options()
    {
        WFP::onGameInitEvent() += []()
        {
            // CMenuManager::ProcessOnOffMenuOptions: the screen table, from the current entry's action
            auto pattern = hook::pattern("8B 8B ? ? ? ? 8B 83 ? ? ? ? 6B C9 61 8D 04 80 01 C1 8B 04 8D");
            if (pattern.size() < 1)
            {
                wfp::patterns::Report("GTA3.WidescreenFix: feature signature unavailable.\n");
                return;
            }
            auto language = hook::pattern("C7 05 ? ? ? ? 04 00 00 00 C6 05");
            auto rowText = hook::pattern("E8 ? ? ? ? 89 44 24 ? 8B B5");
            auto titleText = hook::pattern("8B 85 ? ? ? ? 69 C0 84 01 00 00 05 ? ? ? ? 50 E8 ? ? ? ? 50 FF B4 24");
            auto rightText = hook::pattern("8B 44 24 ? 89 84 24 ? ? ? ? DB 84 24 ? ? ? ? D8 44 24 ? D9 9C 24");
            auto rightTextStore = hook::pattern("E8 ? ? ? ? 89 44 24 ? E9 ? ? ? ? 0F BE 05");
            auto layout = hook::pattern("8B 85 ? ? ? ? 59 48 8B 8D ? ? ? ? 83 F8 37 0F 87 ? ? ? ? FF 24 85");
            auto buttons = hook::pattern("89 E9 E8 ? ? ? ? 83 3D ? ? ? ? 00 74");
            auto accept = hook::pattern("89 E9 E8 ? ? ? ? 80 7C 24 ? 00 0F 84");
            auto change = hook::pattern("89 E9 E8 ? ? ? ? 83 BD ? ? ? ? 37");
            auto sound = hook::pattern("B9 ? ? ? ? A2 ? ? ? ? 6A 00 68 98 00 00 00 E8");
            if (pattern.empty() || language.empty() || rowText.empty() || titleText.empty() || rightText.empty() || rightTextStore.empty() ||
                layout.empty() || buttons.empty() || accept.empty() || change.empty())
                return;

            aScreens = reinterpret_cast<CMenuScreen*>(*pattern.get_first<uint8_t*>(23) - offsetof(CMenuScreen, entries));
            pLanguage = *language.get_first<int32_t*>(2);
            RightTextOffset = *rightTextStore.get_first<uint8_t>(8);
            if (!sound.empty())
            {
                DMAudio = *sound.get_first<void*>(1);
                PlayFrontEndSound = reinterpret_cast<decltype(PlayFrontEndSound)>(injector::GetBranchDestination(sound.get_first(17)).as_int());
            }

            // Both call sites must reach the same function
            auto onOff = injector::GetBranchDestination(accept.get_first(2)).as_int();
            if (injector::GetBranchDestination(change.get_first(2)).as_int() != onOff)
                return;

            // The layout of the page screen: Display Setup's, when it's still the default one
            auto table = *layout.get_first<uintptr_t*>(26);
            if (table[ScreenPage - 1] != table[ScreenOptions - 1] || table[ScreenRoot - 1] != table[ScreenOptions - 1])
                return;

            if (!ScreensAsShipped() || !AddOptionsEntry())
                return;
            injector::WriteMemory<uintptr_t>(&table[ScreenPage - 1], table[ScreenDisplay - 1], true);

            // The fix's rows on Display Setup, reset by the game's own Restore Defaults there
            auto restore = hook::pattern("C6 05 ? ? ? ? 01 C7 05 ? ? ? ? 00 01 00 00 C6 05 ? ? ? ? 01 C7 05 ? ? ? ? 9A 99 99 3F");
            if (!restore.empty() && AddJoinedEntries())
            {
                bJoined = true;
                static auto RestoreHook = safetyhook::create_mid(restore.get_first(), [](SafetyHookContext&)
                {
                    RWGTAMenu::RestoreDefaults(FixSettings(), JoinedIds());
                    if (Model.IsOpen())
                        FillJoined();
                });
            }

            RowTextPrevious = injector::MakeCALL(rowText.get_first(0), RowText, true).get();
            TitleTextPrevious = injector::MakeCALL(titleText.get_first(18), TitleText, true).get();
            ProcessButtonPressesPrevious = injector::MakeCALL(buttons.get_first(2), ProcessButtonPresses, true).get();
            OnOffAcceptPrevious = injector::MakeCALL(accept.get_first(2), OnOffAccept, true).get();
            OnOffChangePrevious = injector::MakeCALL(change.get_first(2), OnOffChange, true).get();

            // The text of a page's first entry when it is a label (the restart line)
            auto labelText = hook::pattern("BE ? ? ? ? B9 ? ? ? ? 01 C6 83 C6 20 56 E8 ? ? ? ? 50");
            if (labelText.size() == 1)
            {
                LabelTextPrevious = injector::MakeCALL(labelText.get_first(16), LabelText, true).get();
                LabelReady = true;
            }

            // The value at the right of the fix's rows: the end of the right text's switch, where each case left it
            static auto RightTextHook = safetyhook::create_mid(rightText.get_first(), [](SafetyHookContext& regs)
            {
                if (!bReady || !aScreens)
                    return;
                auto manager = reinterpret_cast<uint8_t*>(regs.ebp);
                char key[8];
                // edi holds the row's action through the switch
                if (bJoined && Screen(manager) == ScreenDisplay)
                {
                    int row = int(regs.edi) - ActionJoined;
                    if (row < 0 || row >= int(std::size(Joined)))
                        return;
                    sprintf_s(key, "WFPJ%02dV", row);
                }
                else
                {
                    if (Screen(manager) != ScreenPage)
                        return;
                    int row = int(regs.edi) - ActionBase;
                    if (row < 0 || row >= int(ShownRows.size()) || !ShownRows[row])
                        return;
                    sprintf_s(key, "WFPR%02dV", row);
                }
                auto it = Texts.find(std::string_view(key));
                if (it != Texts.end() && !it->second.empty())
                    *reinterpret_cast<const wchar_t**>(regs.esp + RightTextOffset) = reinterpret_cast<const wchar_t*>(it->second.c_str());
            });

            auto navigation = hook::pattern("80 7C 24 0C 00 74 ? 83 BD ? ? ? ? 1E 74 ? FF 85");
            if (navigation.size() == 1)
            {
                static auto NavigationHook = safetyhook::create_mid(navigation.get_first(), [](SafetyHookContext& regs)
                {
                    auto manager = reinterpret_cast<uint8_t*>(regs.ebp);
                    if (!bReady || Screen(manager) != ScreenPage)
                        return;
                    auto& down = *reinterpret_cast<uint8_t*>(regs.esp + 0xC);
                    auto& up = *reinterpret_cast<uint8_t*>(regs.esp + 0x10);
                    if (down || up)
                    {
                        int first = LabelShown ? 1 : 0;
                        Entry(manager) = first + Model.Move(size_t(CurrentPage), Entry(manager) - first, down ? 1 : -1);
                        FillPage(manager);
                        down = up = 0;
                    }
                });
            }
            else
                return;

            auto valuePrint = hook::pattern("FF 74 24 0C FF B4 24 ? ? ? ? D9 9C 24 ? ? ? ? FF B4 24 ? ? ? ? E8");
            if (valuePrint.size() == 1)
            {
                ValueStringWidth = reinterpret_cast<decltype(ValueStringWidth)>(0x5018A0);
                ValuePrintPrevious = reinterpret_cast<ValuePrintFn>(injector::MakeCALL(valuePrint.get_first(25), ValuePrint, true).as_int());
            }
            auto wheelButtons = hook::pattern("80 3D ? ? ? ? 00 59 75 11 6A 00 E8 ? ? ? ? 80 3D ? ? ? ? 00 59");
            for (size_t i = 0; i < wheelButtons.size(); ++i)
            {
                auto match = wheelButtons.get(i);
                auto up = *match.get<uint8_t*>(2);
                if (*match.get<uint8_t*>(19) == up + 1)
                    Wheel.buttons = up;
            }
            auto rectangle = hook::pattern("8B 44 24 04 53 8B 5C 24 0C 6A 00 53 53 53 53 50 E8");
            if (rectangle.size() == 1)
                DrawRect = reinterpret_cast<decltype(DrawRect)>(rectangle.get_first());

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
} Options;
