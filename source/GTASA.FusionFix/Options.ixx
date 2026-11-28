module;

#include <stdafx.h>
#include "common.h"
#include "FixSettings.hpp"
#include "Shared/RWGTA/FixMenu.hpp"
#include "Shared/PadInput.hpp"
#include <cmath>
#include <string>

export module Options;

import Skeleton;
import Draw;

namespace MenuText
{
    template<typename Measure>
    std::string FitLabel(std::string label, float room, Measure&& measure)
    {
        if (!std::isfinite(room) || room <= 0.0f)
            return {};
        if (measure(label) <= room)
            return label;
        const std::string suffix{ "..." };
        if (measure(suffix) > room)
            return {};
        while (!label.empty())
        {
            label.pop_back();
            if (measure(label + suffix) <= room)
                return label + suffix;
        }
        return suffix;
    }
}

// The fix's settings in the game's frontend: "Plugins" at the end of the Options screen shows the list of plugins (the
// fix first, then the other plugins with ASI Settings, see Shared/RWGTA/FixMenu.hpp). A plugin opens its settings with
// their values like Display Setup, or, when its settings come in several categories, the list of them, each opening
// its settings. A screen holds twelve rows, so every list scrolls through a window of rows (Back scrolls with them):
// Up/Down move and scroll by one row and wrap at the ends like the game's lists, Page Up/Page Down move by a window,
// Home/End go to the ends, the mouse wheel and the arrows (the Brief screen's) scroll, the mouse selects the row under
// it while it moves. Console gamma is a row of Display Setup (after Brightness), the field of view and
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
//
// A changed setting that applies at the next start puts the game's own label line (a first row of action 1, drawn
// at 60, 97 like the save screens' questions) above the page's rows, and the selected row's description, also of the
// fix's rows on Display Setup and Advanced, is the game's helper text at the bottom right (DisplayHelperText).

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
    constexpr int PageRows = 10;            // rows of a settings list shown at once, Back included (11 with the restart line)
    constexpr int ListRows = 8;             // rows of a list of plugins or categories shown at once, Back included
    constexpr int ScreenOptions = 33;
    constexpr int ScreenDisplay = 4;
    constexpr int ScreenAdvanced = 27;
    constexpr int16_t LineHeight = 30;      // the game's distance between rows placed one after another
    constexpr uint8_t ActionLabel = 1;      // the line of text above the rows (the first entry only)
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
    constexpr size_t HelperMessage = 0x1B18;    // the game's own notice in the helper text (0: none)

    CMenuScreen* aScreens = nullptr;
    uint8_t* Manager = nullptr;
    bool bReady = false;

    enum class Mode { None, Page };
    Mode State = Mode::None;
    CMenuScreen SavedOptions{};
    int OpenRow = 0;
    size_t CurrentShownPage = 0;
    bool CenteredPage = false;              // a list of plugins or categories: centred like the Options screen
    bool DisplayLayout = false;             // a plugin's settings and categories, placed like Display Setup's rows

    RWGTAMenu::Model Model;
    RWGTAMenu::WheelInput Wheel;
    RWGTAMenu::ValueAreas ValueAreas;
    void* Audio = nullptr;
    void(__fastcall* NavigationSound)(void*, void*, int, float, float) = nullptr;
    void(__cdecl* DrawRect)(const float*, const uint8_t*) = nullptr;
    void(__cdecl* Draw2DPolygon)(float, float, float, float, float, float, float, float, const uint8_t*) = nullptr;
    std::vector<const RWGTAMenu::Row*> ShownRows;
    bool LabelShown = false;                // the page starts with the restart line
    float HelpLeft = -1.0f;                 // the left edge of the fix's helper text while it is printed (-1: none)
    float(__fastcall* StretchX)(uint8_t*, void*, float) = nullptr;

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
        const auto chosen = FixSettings().Get("MAIN.TextLanguage");
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

    // Lists of plugins and categories where the Options screen has its rows: its first row's place, a line apart
    int16_t ListX() { return SavedOptions.entries[0].x ? SavedOptions.entries[0].x : 320; }
    int16_t ListY() { return SavedOptions.entries[0].y ? SavedOptions.entries[0].y : 130; }

    // The entry of the page's first row: after the restart line when it is shown
    int FirstRowEntry()
    {
        return LabelShown ? 1 : 0;
    }

    // A list of plugins or categories (no values): centred like the Options screen; a list of settings like Display
    // Setup, names at the left and values at the right
    bool Centered(size_t page)
    {
        auto& rows = Model.Pages()[page].rows;
        return std::none_of(rows.begin(), rows.end(), [](const RWGTAMenu::Row& row)
        {
            return row.kind == RWGTAMenu::RowKind::Setting || row.kind == RWGTAMenu::RowKind::Preset ||
                row.kind == RWGTAMenu::RowKind::Action;
        });
    }

    // The places of Display Setup's Advanced, Restore Defaults and Back (as the game draws them: Back follows Restore
    // Defaults a line lower); false when another mod moved them
    bool DisplayAnchors(int16_t& advanced, int16_t& defaults, int16_t& back)
    {
        auto& display = aScreens[ScreenDisplay];
        auto find = [&](const char* name) { return std::find_if(std::begin(display.entries), std::end(display.entries), [&](const CMenuEntry& e) { return strncmp(e.name, name, 8) == 0; }); };
        auto a = find("FED_ADV"), d = find("FET_DEF"), b = find("FEDS_TB");
        if (a == std::end(display.entries) || d == std::end(display.entries) || b == std::end(display.entries) || !a->y || !d->y)
            return false;
        advanced = a->y;
        defaults = d->y;
        back = b->y ? b->y : int16_t(d->y + LineHeight);
        return true;
    }

    // The page shown: the rows in its window with their values, Back among them
    void FillPage(uint8_t* manager)
    {
        auto& screen = aScreens[ScreenOptions];
        for (auto& entry : screen.entries)
            entry = {};
        strncpy_s(screen.name, "WFPT1", _TRUNCATE);
        CenteredPage = Centered(CurrentShownPage);
        ShownRows = Model.Shown(CurrentShownPage, true);
        auto& window = Model.Window(CurrentShownPage);
        // A changed setting that applies at the next start: the game's own label line above the rows says so
        bool label = !CenteredPage && Model.RestartRequired();
        LabelShown = label;
        int first = FirstRowEntry();
        if (label)
        {
            SetText("WFPWARN", Model.Text("menu.restartwarning"));
            SetEntry(screen.entries[0], ActionLabel, "WFPWARN", 0, 0, 0, 0);
        }
        // A little closer than the game's rows, so that Back stays above the helper text; below the label too. Lists
        // of plugins and categories like the Options screen.
        int count = int(ShownRows.size());
        int16_t y = CenteredPage ? ListY() : label ? 136 : 127;
        int16_t step = CenteredPage ? LineHeight : label ? 27 : 28;
        // A list that fits has its Back where the Options screen has its own
        const auto back = std::find_if(std::begin(SavedOptions.entries), std::end(SavedOptions.entries), [](const CMenuEntry& e) { return strncmp(e.name, "FEDS_TB", 8) == 0; });
        const bool nativeBack = CenteredPage && !window.Above() && !window.Below() && back != std::end(SavedOptions.entries) && back->x && back->y;
        // A plugin's page with settings and categories that fits on the screen is laid out like Display Setup: the
        // settings at the left from the top, then the rows that open a screen (centred, ending where Advanced is),
        // Restore Defaults and Back centred where Display Setup has its own
        std::vector<int16_t> places;
        if (!CenteredPage && !window.Above() && !window.Below())
        {
            int16_t advanced = 0, defaults = 0, backY = 0;
            auto isValue = [](const RWGTAMenu::Row* row) { return row && (row->kind == RWGTAMenu::RowKind::Setting || row->kind == RWGTAMenu::RowKind::Preset); };
            bool categories = std::any_of(ShownRows.begin(), ShownRows.end(), [](auto row) { return row && row->kind == RWGTAMenu::RowKind::Category; });
            if (categories && DisplayAnchors(advanced, defaults, backY))
            {
                int values = int(std::count_if(ShownRows.begin(), ShownRows.end(), isValue));
                int stacked = int(std::count_if(ShownRows.begin(), ShownRows.end(), [&](auto row) { return row && !isValue(row) && row->kind != RWGTAMenu::RowKind::RestoreDefaults; }));
                int16_t next = int16_t(advanced - LineHeight * (stacked - 1)), valueY = y;
                if (next >= y + step * values + 13)
                {
                    for (auto row : ShownRows)
                    {
                        if (!row)
                            places.push_back(backY);
                        else if (isValue(row))
                            places.push_back(std::exchange(valueY, int16_t(valueY + step)));
                        else if (row->kind == RWGTAMenu::RowKind::RestoreDefaults)
                            places.push_back(defaults);
                        else
                            places.push_back(std::exchange(next, int16_t(next + LineHeight)));
                    }
                }
            }
        }
        DisplayLayout = !places.empty();
        for (int i = 0; i < count; ++i, y += step)
        {
            if (DisplayLayout)
                y = places[i];
            char key[8];
            sprintf_s(key, "WFPR%02d", i);
            if (!ShownRows[i])
            {
                if (nativeBack)
                    SetEntry(screen.entries[first + i], ActionBase + i, "FEDS_TB", TypeButton, back->x, back->y, back->align);
                else
                    SetEntry(screen.entries[first + i], ActionBase + i, "FEDS_TB", TypeButton, CenteredPage ? ListX() : 320, CenteredPage || DisplayLayout ? y : y + 5, AlignCenter);
                continue;
            }
            SetText(key, Model.Label(*ShownRows[i]));
            SetText(std::string(key) + "V", Model.Value(*ShownRows[i]));
            // Rows without a value (categories, Restore defaults) are centred like Advanced and Restore Defaults on
            // Display Setup
            auto kind = ShownRows[i]->kind;
            if (CenteredPage)
                SetEntry(screen.entries[first + i], ActionBase + i, key, TypeButton, ListX(), y, AlignCenter);
            else if (kind != RWGTAMenu::RowKind::Setting && kind != RWGTAMenu::RowKind::Preset)
                SetEntry(screen.entries[first + i], ActionBase + i, key, TypeButton, 320, y, AlignCenter);
            else
                SetEntry(screen.entries[first + i], ActionBase + i, key, TypeValue, 57, y, AlignLeft);
        }
        Option(manager) = count ? first + int(window.Slot()) : first;
        if (CurrentShownPage < Model.Pages().size())
            SetText("WFPT1", Model.Pages()[CurrentShownPage].title);
    }

    // The mouse moved over a row of the window (the game selects the entry under the mouse only while it moves)
    void FollowHover(uint8_t* manager)
    {
        if (State != Mode::Page || Screen(manager) != ScreenOptions)
            return;
        int slot = Option(manager) - FirstRowEntry();
        auto& window = Model.Window(CurrentShownPage);
        if (slot >= 0 && size_t(slot) < window.Shown() && size_t(slot) != window.Slot())
            window.Point(size_t(slot));
    }

    // Settings that are the game's own rows (Display Setup's Widescreen and Frame Limiter, Menu.ixx): not on the
    // fix's pages
    constexpr const char* NativeRows[] = { "MAIN.CutsceneBorders", "MAIN.FPSLimit" };

    void OpenModel()
    {
        HMODULE self = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCWSTR>(&OpenModel), &self);
        auto hidden = JoinedIds();
        hidden.insert(std::begin(NativeRows), std::end(NativeRows));
        Model.OpenMenu(self, LanguageTag(), PageRows, hidden);
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

    void ShowPage(uint8_t* manager, size_t page)
    {
        CurrentShownPage = page;
        State = Mode::Page;
        Model.Window(page).SetSlots(Centered(page) ? ListRows : PageRows);
        FillPage(manager);
    }

    void Open(uint8_t* manager)
    {
        OpenModel();
        SavedOptions = aScreens[ScreenOptions];
        LabelShown = false;
        ShowPage(manager, RWGTAMenu::Model::RootPage);
    }

    void Close()
    {
        if (State == Mode::None)
            return;
        aScreens[ScreenOptions] = SavedOptions;
        State = Mode::None;
        Model.Close();
        ShownRows.clear();
        LabelShown = false;
    }

    // Back from a page: to the list it was opened from, with that row selected, or from the plugins list to the
    // Options screen
    void Back(uint8_t* manager)
    {
        if (State != Mode::Page)
            return;
        size_t parent = Model.Back(CurrentShownPage);
        if (parent == SIZE_MAX)
        {
            Close();
            Option(manager) = OpenRow;
            return;
        }
        ShowPage(manager, parent);
    }

    // Up/Down, Page Up/Page Down, Home/End: the selection moves and the window follows; true when it changed
    bool MoveSelection(uint8_t* manager, int direction, bool page, bool edge)
    {
        auto& window = Model.Window(CurrentShownPage);
        bool moved = edge ? window.Edge(direction) : page ? window.Page(direction) : window.Move(direction, true);
        if (!moved)
            return false;
        if (NavigationSound)
            NavigationSound(Audio, nullptr, 3, 0.0f, 1.0f);
        FillPage(manager);
        return true;
    }

    // The wheel and the arrows: the window moves by a row, the selection stays on its row while it is shown
    void ScrollList(uint8_t* manager, int direction)
    {
        if (Model.Window(CurrentShownPage).Scroll(direction))
        {
            if (NavigationSound)
                NavigationSound(Audio, nullptr, 3, 0.0f, 1.0f);
            FillPage(manager);
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
                Model.SelectLanguage(LanguageTag());
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
        // The menu's language changed while the pages are open (an editor): their texts follow at once
        if (Model.SelectLanguage(LanguageTag()))
            ShownRows.clear();
        FillPage(manager);
    }

    POINT ResolutionCursor{};
    bool ResolutionCursorHeld{};

    void SyncResolutionCursor(uint8_t* manager)
    {
        if (!ResolutionCursorHeld)
            return;

        POINT cursor{};
        if (!GetCursorPos(&cursor) || cursor.x != ResolutionCursor.x || cursor.y != ResolutionCursor.y)
        {
            ResolutionCursorHeld = false;
            return;
        }

        // A window resize changes client coordinates even when the cursor stays at the same screen position.
        auto field = [manager](size_t offset) -> int32_t& { return *reinterpret_cast<int32_t*>(manager + offset); };
        const auto x = MenuCanvas::Depth && !MenuCanvas::Suspensions
            ? MenuCanvas::GetMouseX(field(0xE0))
            : std::clamp(field(0xE0), 0, std::max(0, RsGlobal->width));
        const auto y = std::clamp(field(0xE4), 0, std::max(0, RsGlobal->height));
        field(0xBC) = field(0x1AF8) = x;
        field(0xC0) = field(0x1AFC) = y;
    }

    // The arrows of the lists, like the Brief screen's: small light gray triangles that blink, left of a list of settings
    // as the Brief screen has them, above and below the middle of a centred list. In the menu's units (640x448).
    float ArrowX() { return CenteredPage ? float(ListX()) : 45.0f; }
    constexpr float ArrowHalf = 5.0f;
    constexpr float ArrowHeight = 10.0f;

    float ArrowTop() { return CenteredPage ? float(ListY()) - 14.0f : LabelShown ? 122.0f : 113.0f; }
    float ArrowBottom()
    {
        auto& window = Model.Window(CurrentShownPage);
        float first = CenteredPage ? float(ListY()) : LabelShown ? 136.0f : 127.0f;
        float step = CenteredPage ? float(LineHeight) : LabelShown ? 27.0f : 28.0f;
        return first + step * float(window.Slots()) + 2.0f;
    }

    // The arrow under the mouse when the menu was last drawn (the mouse and the arrows in the same units as the game's
    // own hit tests there): -1 / +1 the up / down arrow, 0 none
    int ArrowHit = 0;

    // -1 / +1 when the mouse is on the up / down arrow that is shown, 0 otherwise
    int IndicatorUnderMouse(uint8_t* manager)
    {
        if (State != Mode::Page || Screen(manager) != ScreenOptions)
            return 0;
        return ArrowHit;
    }

    // Keys the game's menu doesn't read: held Up/Down (keys, d-pad, stick) repeat, Page Up/Page Down and the shoulder
    // buttons move by a window, Home/End go to the ends; a held click on an arrow scrolls. Each repeats while held.
    int32_t WheelDelta = 0;                 // the mouse wheel's movement read by the game since the last frame
    uint8_t* MouseLeft = nullptr;           // CPad's left mouse button as read this frame (TempMouseControllerState)
    int ArrowClickFrames = 0;               // frames since the left button was down on an arrow (0: none)
    MenuList::WheelNotches WheelNotches;
    MenuList::Repeat PageUpKey, PageDownKey, ArrowClick, UpHeld, DownHeld;
    MenuList::StickAxis StickY;
    bool HomeDown = false, EndDown = false;

    bool GameInFront()
    {
        DWORD pid = 0;
        GetWindowThreadProcessId(GetForegroundWindow(), &pid);
        return pid == GetCurrentProcessId();
    }

    void ExtraInput(uint8_t* manager)
    {
        bool active = State == Mode::Page && Screen(manager) == ScreenOptions && GameInFront();
        auto held = [&](int key) { return active && (GetAsyncKeyState(key) & 0x8000) != 0; };
        const uint64_t now = GetTickCount64();
        // The game moves once per press (key, d-pad or stick); held, the selection repeats
        XINPUT_STATE pad{};
        DWORD slot = 0;
        bool padRead = active && PadInput::Read(pad, slot);
        int stick = padRead ? StickY.Direction(pad.Gamepad.sThumbLY) : 0;
        WORD buttons = padRead ? pad.Gamepad.wButtons : 0;
        if (DownHeld.Step(held(VK_DOWN) || (buttons & XINPUT_GAMEPAD_DPAD_DOWN) || stick < 0, now, false))
            MoveSelection(manager, 1, false, false);
        if (UpHeld.Step(held(VK_UP) || (buttons & XINPUT_GAMEPAD_DPAD_UP) || stick > 0, now, false))
            MoveSelection(manager, -1, false, false);
        if (PageUpKey.Step(held(VK_PRIOR) || (buttons & XINPUT_GAMEPAD_LEFT_SHOULDER), now))
            MoveSelection(manager, -1, true, false);
        if (PageDownKey.Step(held(VK_NEXT) || (buttons & XINPUT_GAMEPAD_RIGHT_SHOULDER), now))
            MoveSelection(manager, 1, true, false);
        bool home = held(VK_HOME), end = held(VK_END);
        if (home && !HomeDown)
            MoveSelection(manager, -1, false, true);
        if (end && !EndDown)
            MoveSelection(manager, 1, false, true);
        HomeDown = home;
        EndDown = end;
        int arrow = active && MouseLeft && *MouseLeft ? IndicatorUnderMouse(manager) : 0;
        // The game takes the click on its release, a frame or two later: it doesn't reach the selected row
        ArrowClickFrames = arrow ? 4 : ArrowClickFrames > 0 ? ArrowClickFrames - 1 : 0;
        if (ArrowClick.Step(arrow != 0, now))
            ScrollList(manager, arrow);
    }

    void* UserInputPrevious = nullptr;
    void __fastcall UserInput(uint8_t* manager, void* edx)
    {
        SyncResolutionCursor(manager);
        if (bReady)
        {
            Update(manager);
            ExtraInput(manager);
        }
        bool overValue = ValueAreas.Contains(
            float(*reinterpret_cast<int32_t*>(manager + 0xE0)), float(*reinterpret_cast<int32_t*>(manager + 0xE4)));
        bool listWheel = bReady && State == Mode::Page && Screen(manager) == ScreenOptions && !overValue;
        Wheel.Begin(listWheel);
        // Every notch the mouse reported since the last frame scrolls one row (the game's flag would show one per frame)
        int notches = listWheel ? WheelNotches.Take(std::exchange(WheelDelta, 0)) : (WheelNotches.Clear(), WheelDelta = 0, 0);
        for (; notches; notches += notches > 0 ? -1 : 1)
            ScrollList(manager, notches > 0 ? -1 : 1);
        reinterpret_cast<void(__fastcall*)(uint8_t*, void*)>(UserInputPrevious)(manager, edx);
        Wheel.End();
        if (bReady)
        {
            FollowHover(manager);
            Update(manager);
        }
    }

    // Escape on the fix's pages: back to the list they were opened from, or to the Options screen; Up/Down move in the
    // list's window
    void* ProcessUserInputPrevious = nullptr;
    void __fastcall ProcessUserInput(uint8_t* manager, void* edx, int32_t down, int32_t up, int32_t enter, int32_t exit, int32_t wheel)
    {
        if (bReady && State != Mode::None && Screen(manager) == ScreenOptions && uint8_t(exit & 0xFF))
        {
            Back(manager);
            exit = 0;
        }
        if (bReady && State == Mode::Page && Screen(manager) == ScreenOptions)
        {
            if (uint8_t(down) || uint8_t(up))
            {
                MoveSelection(manager, uint8_t(down) ? 1 : -1, false, false);
                down = up = 0;
            }
            // A click on an arrow scrolls (ExtraInput) and doesn't choose the row at its height
            if (uint8_t(enter) && ArrowClickFrames)
                enter = 0;
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
        // A click on an arrow only scrolls
        if (State == Mode::Page && screen == ScreenOptions && ArrowClickFrames)
            return true;
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
        if (row >= int(ShownRows.size()) || !ShownRows[row])
        {
            if (dir == 0)
                Back(manager);
            return true;
        }
        if (RWGTAMenu::Model::Opens(*ShownRows[row]))
        {
            if (size_t page = dir == 0 ? Model.Enter(*ShownRows[row]) : SIZE_MAX; page != SIZE_MAX)
                ShowPage(manager, page);
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

    // CText::Get at the rows, the title, the label line and the helper text
    void* RowTextPrevious = nullptr;
    void* TitleTextPrevious = nullptr;
    void* LabelTextPrevious = nullptr;
    void* HelperKeyPrevious = nullptr;

    template<void*& Previous>
    const char* __fastcall AnsweredText(void* text, void* edx, const char* key)
    {
        if (auto answer = Answer(key))
            return answer;
        return reinterpret_cast<const char* (__fastcall*)(void*, void*, const char*)>(Previous)(text, edx, key);
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
    GameRef<float> FontScaleX;
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
        // Rows without a value (lists, centred rows) keep their whole label
        if (!value || !*value)
            return LabelPrintPrevious(x, y, text);
        float room = ValueRightEdge - x - GetStringWidth(value, true, false) - GetStringWidth("WW", true, false);
        auto fitted = MenuText::FitLabel(text, room, [](const std::string& label)
        {
            return GetStringWidth(label.c_str(), true, false);
        });
        LabelPrintPrevious(x, y, fitted.c_str());
    }

    // The selected row of the fix: on its pages, or on Display Setup and Advanced while the game shows no notice of
    // its own there
    const RWGTAMenu::Row* SelectedRow(uint8_t* manager)
    {
        int option = Option(manager);
        if (option < 0 || option >= RowCount)
            return nullptr;
        int screen = Screen(manager);
        if (State == Mode::Page && screen == ScreenOptions)
        {
            // Lists of plugins and categories have no helper text, like the Options screen
            int row = aScreens[ScreenOptions].entries[option].action - ActionBase;
            return row >= 0 && row < int(ShownRows.size()) && ShownRows[row] && !RWGTAMenu::Model::Opens(*ShownRows[row]) ? ShownRows[row] : nullptr;
        }
        if (State == Mode::None && JoinedScreen(screen) && Model.IsOpen() && !*reinterpret_cast<int32_t*>(manager + HelperMessage))
            return JoinedRow(aScreens[screen].entries[option].action - ActionJoined);
        return nullptr;
    }

    // CMenuManager::DisplayHelperText at the end of DrawStandardMenus
    using HelperFn = void(__fastcall*)(uint8_t*, void*, const char*);
    void* HelperTextPrevious = nullptr;

    // The selected row's description as the game's helper text: right-aligned at the bottom right, where the game's
    // own hints are, wrapped right of Back where Back is centred at the bottom (Display Setup and the pages laid out
    // like it) and right of the corner's version text elsewhere
    bool RowHelp(uint8_t* manager, void* edx)
    {
        auto row = bReady ? SelectedRow(manager) : nullptr;
        if (!row)
            return false;
        SetText("WFPHELP", Model.Help(*row));
        if (Texts["WFPHELP"].empty())
            return false;
        if (StretchX)
            HelpLeft = StretchX(manager, nullptr, (State == Mode::Page && DisplayLayout) || Screen(manager) == ScreenDisplay ? 400.0f : 100.0f);
        reinterpret_cast<HelperFn>(HelperTextPrevious)(manager, edx, "WFPHELP");
        HelpLeft = -1.0f;
        return true;
    }

    // CFont::PrintStringFromBottom of DisplayHelperText (its font is set): the fix's description in lines that fit
    // between its left edge and the right-aligned x, broken at spaces with the font's line break
    PrintStringFn HelperPrintPrevious = nullptr;
    void __cdecl HelperPrint(float x, float y, const char* text)
    {
        if (HelpLeft < 0.0f || !text || !GetStringWidth)
            return HelperPrintPrevious(x, y, text);
        std::string out, line;
        std::string_view rest(text);
        while (!rest.empty())
        {
            const auto space = rest.find(' ');
            std::string word(rest.substr(0, space));
            rest = space == std::string_view::npos ? std::string_view{} : rest.substr(space + 1);
            auto candidate = line.empty() ? word : line + " " + word;
            if (!line.empty() && GetStringWidth(candidate.c_str(), true, false) > x - HelpLeft)
            {
                out += line + "~n~";
                line = std::move(word);
            }
            else
                line = std::move(candidate);
        }
        out += line;
        HelperPrintPrevious(x, y, out.c_str());
    }

    // The game's general notice (no key) gives way to the description of the fix's selected row
    void __fastcall HelperText(uint8_t* manager, void* edx, const char* key)
    {
        if (!key && RowHelp(manager, edx))
            return;
        reinterpret_cast<HelperFn>(HelperTextPrevious)(manager, edx, key);
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
    void RememberResolutionCursor()
    {
        ResolutionCursorHeld = GetCursorPos(&ResolutionCursor) != FALSE;
    }

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
    // A text of the fix in the game's font, in the language of the menus
    std::string GameText(const char* key, bool upper = false)
    {
        std::string text;
        for (auto c : RWGTAMenu::ToGameFont(RWGTAMenu::PluginText(FixSettings(), LanguageTag(), key), upper))
            text += char(c < 0x100 ? c : '?');
        return text;
    }

    void BeginMenuDraw()
    {
        ValueAreas.bounds.clear();
    }

    // The arrows where the list has more rows, like the Brief screen's: light gray triangles that blink every 700 ms
    void DrawScrollIndicators()
    {
        if (!bReady || !Draw2DPolygon || State != Mode::Page)
            return;
        auto& window = Model.Window(CurrentShownPage);
        ArrowHit = 0;
        if (!window.Above() && !window.Below())
            return;
        const float sx = SCREEN_WIDTH / 640.0f, sy = SCREEN_HEIGHT / 448.0f;
        if (Manager)
        {
            // The mouse over an arrow that is shown, blinking or not
            const float x = float(*reinterpret_cast<int32_t*>(Manager + 0xBC)) / sx;
            const float y = float(*reinterpret_cast<int32_t*>(Manager + 0xC0)) / sy;
            if (std::fabs(x - ArrowX()) <= ArrowHalf + 6.0f)
            {
                if (window.Above() && y >= ArrowTop() - 6.0f && y <= ArrowTop() + ArrowHeight + 6.0f)
                    ArrowHit = -1;
                else if (window.Below() && y >= ArrowBottom() - 6.0f && y <= ArrowBottom() + ArrowHeight + 6.0f)
                    ArrowHit = 1;
            }
        }
        if ((GetTickCount64() / 700) % 2)
            return;
        const uint8_t colour[] = { 225, 225, 225, 255 };
        auto triangle = [&](float apex, float base)
        {
            const float x = ArrowX();
            Draw2DPolygon(x * sx, apex * sy, (x + ArrowHalf) * sx, base * sy, (x - ArrowHalf) * sx, base * sy, x * sx, apex * sy, colour);
        };
        if (window.Above())
            triangle(ArrowTop(), ArrowTop() + ArrowHeight);
        if (window.Below())
            triangle(ArrowBottom() + ArrowHeight, ArrowBottom());
    }
}

class OptionsInit
{
public:
    OptionsInit()
    {
        WFP::onInitEvent() += []() { Model.RememberRunningSettings(FixSettings()); };
        WFP::onGameInitEvent() += []()
        {
            auto screens = hook::pattern("0F BE 86 ? ? ? ? 8B 4E 54 69 C0 E2 00 00 00 8D 14 C9 0F B6 94 50");
            auto rowText = hook::pattern("57 E8 ? ? ? ? 8B D8 89 5C 24");
            auto titleText = hook::pattern("81 C2 ? ? ? ? 52 B9 ? ? ? ? E8 ? ? ? ? 8B 4C 24");
            auto userInput = hook::pattern("8A 46 5B 50 E8 ? ? ? ? 8B CE E8");
            UserInputPrevious = injector::MakeCALL(userInput.get_first(11), UserInput, true).get();
            auto processUserInput = hook::pattern("50 51 52 8B CE E8 ? ? ? ? 5E 83 C4 18 C3");
            auto accept = hook::pattern("8D 54 24 ? 52 6A 00 8B CE E8 ? ? ? ? 8A 44 24");
            auto change = hook::pattern("8D 44 24 ? 50 53 8B CE E8 ? ? ? ? 53 8B CE E8");
            auto labelPrint = hook::pattern("53 51 52 E8 ? ? ? ? 83 C4 0C");
            auto valuePrint = hook::pattern("56 50 D8 E1 51 D9 1C 24 DD D8 E8");
            auto setScale = hook::pattern("52 50 E8 ? ? ? ? 6A 02");
            auto stringWidth = hook::pattern("53 56 8B B4 24 A4 01 00 00 56 C7 44 24");

            aScreens = reinterpret_cast<CMenuScreen*>(*screens.get_first<uint8_t*>(23) - offsetof(CMenuScreen, entries));
            if (!AddOptionsEntry())
                return;

            // The fix's rows on Display Setup and Advanced, reset by the game's Restore Defaults there
            auto restore = hook::pattern("33 D2 8A 90 ? ? ? ? 8B CE 52 E8 ? ? ? ? 0F BE 86 ? ? ? ? 69 C0 E2 00 00 00");
            if (AddJoinedEntries())
            {
                bJoined = true;
                SetDefaultsPrevious = injector::MakeCALL(restore.get_first(11), SetDefaults, true).get();
            }

            RowTextPrevious = injector::MakeCALL(rowText.get_first(1), AnsweredText<RowTextPrevious>, true).get();
            TitleTextPrevious = injector::MakeCALL(titleText.get_first(12), AnsweredText<TitleTextPrevious>, true).get();
            auto renderFontBuffer = hook::pattern("A1 ? ? ? ? 83 EC 18 3D ? ? ? ? 0F 84 ? ? ? ? 0F BF 0D");
            RenderFontBuffer = reinterpret_cast<decltype(RenderFontBuffer)>(renderFontBuffer.get_first());
            // Menu mods that hide the page title replace its print call with NOPs; it stays hidden then.
            if (*titleText.get_first<uint8_t>(28) == 0xE8)
                TitlePrintPrevious = reinterpret_cast<PrintStringFn>(injector::MakeCALL(titleText.get_first(28), TitlePrint, true).as_int());
            ProcessUserInputPrevious = injector::MakeCALL(processUserInput.get_first(5), ProcessUserInput, true).get();
            OptionsAcceptPrevious = injector::MakeCALL(accept.get_first(9), OptionsAccept, true).get();
            OptionsChangePrevious = injector::MakeCALL(change.get_first(8), OptionsChange, true).get();
            auto scale = injector::GetBranchDestination(setScale.get_first(2)).get<uint8_t>();
            const auto scaleAt = reinterpret_cast<uintptr_t>(scale + 8);
            hook::range_pattern(scaleAt, scaleAt + 5, "A3 ? ? ? ?").get_first();
            {
                FontScaleX.SetAddress(*reinterpret_cast<float**>(scale + 9));
                GetStringWidth = reinterpret_cast<decltype(GetStringWidth)>(stringWidth.get_first<uint8_t>(-6));
                LabelPrintPrevious = reinterpret_cast<PrintStringFn>(injector::MakeCALL(labelPrint.get_first(3), LabelPrint, true).as_int());
                ValuePrintPrevious = reinterpret_cast<PrintStringFn>(injector::MakeCALL(valuePrint.get_first(10), ValuePrint, true).as_int());
            }
            auto wheelButtons = hook::pattern("A0 ? ? ? ? 83 C4 04 84 C0 74 4D B0 01 C2 04 00 6A 00 E8");
            Wheel.buttons = *wheelButtons.get_first<uint8_t*>(1);
            // CPad::UpdateMouse: the DirectInput state's wheel movement (eax), before it becomes a flag; the buttons
            // before it (left, right, middle, wheel up)
            auto mouse = hook::pattern("85 C0 C6 05 ? ? ? ? 00 C6 05 ? ? ? ? 00 88 0D ? ? ? ? 88 15 ? ? ? ? 7E");
            MouseLeft = *mouse.get_first<uint8_t*>(4) - 3;
            static auto WheelRead = safetyhook::create_mid(mouse.get_first(), [](SafetyHookContext& regs)
            {
                if (State == Mode::Page)
                    WheelDelta += int32_t(regs.eax);
            });

            // CSprite2d::Draw2DPolygon, for the arrows of the lists
            Draw2DPolygon = reinterpret_cast<decltype(Draw2DPolygon)>(hook::pattern("8B 44 24 20 8B 4C 24 1C 8B 54 24 18 56 8B 74 24").get_first());

            // CSprite2d::DrawRect, at the end of the user tracks scan's bar in CMenuManager::DrawBackground
            auto rectangle = hook::pattern("50 8D 54 24 ? 52 E8 ? ? ? ? 83 C4 08 E8 ? ? ? ? 8B CE E8");
            DrawRect = reinterpret_cast<decltype(DrawRect)>(injector::GetBranchDestination(rectangle.get_first(6)).as_int());

            // The restart line: CText::Get of the label's key in DrawStandardMenus (before y = 97.0)
            auto labelText = hook::pattern("E8 ? ? ? ? 39 35 ? ? ? ? 75 ? C7 44 24 ? 00 00 C2 42");
            LabelTextPrevious = injector::MakeCALL(labelText.get_first(), AnsweredText<LabelTextPrevious>, true).get();

            // The helper text: DrawStandardMenus' switch of the screens with one (movsx eax, [ebp+15Dh]; ... push 0;
            // mov ecx, ebp; call DisplayHelperText), the CText::Get of a key given to DisplayHelperText, its x
            // (StretchX(610.0)) and its print
            auto helper = hook::pattern("0F BE 85 5D 01 00 00 83 F8 28 77 ? 0F B6 90 ? ? ? ? FF 24 95 ? ? ? ? 6A 00 8B CD E8");
            auto helperKey = hook::pattern("85 C0 74 ? 50 B9 ? ? ? ? E8 ? ? ? ? 68 FF 00 00 00");
            auto helperX = hook::pattern("68 00 80 18 44 E8 ? ? ? ? 51 D9 1C 24");
            HelperKeyPrevious = injector::MakeCALL(helperKey.get_first(10), AnsweredText<HelperKeyPrevious>, true).get();
            HelperTextPrevious = injector::MakeCALL(helper.get_first(30), HelperText, true).get();
            // The Options screen has no helper text of its own: the fix's pages get theirs before the switch
            static auto PageHelpHook = safetyhook::create_mid(helper.get_first(), [](SafetyHookContext& regs)
            {
                auto manager = reinterpret_cast<uint8_t*>(regs.ebp);
                if (bReady && State == Mode::Page && Screen(manager) == ScreenOptions)
                    RowHelp(manager, nullptr);
            });
            StretchX = reinterpret_cast<decltype(StretchX)>(injector::GetBranchDestination(helperX.get_first(5)).as_int());
            // Menu mods that hide the helper text replace its print call with NOPs; it stays hidden then.
            if (*helperX.get_first<uint8_t>(14) == 0xE8)
                HelperPrintPrevious = reinterpret_cast<PrintStringFn>(injector::MakeCALL(helperX.get_first(14), HelperPrint, true).as_int());

            auto navigationSound = hook::pattern("68 00 00 80 3F 6A 00 6A 03 B9 ? ? ? ? E8 ? ? ? ? 0F BE 96");
            Audio = *navigationSound.get_first<void*>(10);
            NavigationSound = reinterpret_cast<decltype(NavigationSound)>(injector::GetBranchDestination(navigationSound.get_first(14)).as_int());

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
