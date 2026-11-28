module;

#include <stdafx.h>
#include <intrin.h>
#include "common.h"
#include "FixSettings.hpp"
#include "Shared/RWGTA/FixMenu.hpp"
#include "Shared/PadInput.hpp"

export module Options;

import Skeleton;

// The fix's settings in the game's frontend: "Plugins" at the end of the Options screen opens the list of plugins
// (the fix first, then the other plugins with ASI Settings, see Shared/RWGTA/FixMenu.hpp). A plugin opens its settings
// with their values like the Display Setup screen, or, when its settings come in several categories, the list of them,
// each opening its settings. Every list scrolls through a window of rows (Back scrolls with them): Up/Down move and
// scroll by one row and wrap at the ends, Page Up/Page Down move by a window, Home/End go to the ends, the mouse wheel
// and the arrows of the game's scroll bar scroll, the mouse selects the row under it while it moves. Console gamma,
// the field of view and anti-aliasing are rows of Display Setup itself, next to brightness, draw distance and the
// screen resolution; the game's Restore Defaults there resets them too. In the main menu and in the pause menu, which
// share the frontend.
//
// Kept minimal so that it works next to other mods of the frontend (SilentPatch, SkyGfx, menu and widescreen mods):
// - No code of the game is replaced. The lists are three screens of the game's own screen table (aScreens) that the
//   PC game never shows, the PlayStation 2 memory card screens 19 and 20 and the multiplayer screen 21, and one row
//   added to the Options screen. Each is checked to be as the game shipped it; when another mod changed it, the fix
//   leaves the menu alone and the settings stay in the .cfg and FusionFixHub. The game moves between them as between
//   its own screens (its sounds, fades and Back), so the plugins list, a plugin and a category are one screen each.
// - Rows are answered through call sites, each calling the previous target (another mod's hook or the game): the
//   row's and the screen's text (CText::Get), the button processing (CMenuManager::ProcessButtonPresses, to fill the
//   rows) and the value changes (CMenuManager::ProcessOnOffMenuOptions, whose default case ignores the fix's actions).
// - The value at the right of a row comes from a mid hook at the end of the right text's switch in CMenuManager::Draw.
// - The plugin and category screens use Display Setup's layout: their entries of Draw's layout jump table are set to
//   Display Setup's; the plugins list keeps the layout of the Options screen. On a plugin's screen with categories,
//   the rows that open a screen, Restore Defaults and Back are centred like the Options screen's, below the preset.
// - Display Setup gets its rows only when its anchors are there and it keeps room (13 rows, as the lists use).

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
    constexpr int PageSlots = 12;           // rows of a settings list shown at once, Back included (13 with the restart line, as Display Setup)
    constexpr int RootSlots = 9;            // plugins shown at once on the Options layout, Back included
    constexpr int ScreenDisplay = 6;
    constexpr int ScreenOptions = 41;
    constexpr int ScreenRoot = 19;          // FEM_MCM: the PS2 memory card menu
    constexpr int ScreenPage = 20;          // FEM_MC2
    constexpr int ScreenCategory = 21;      // FET_MP: the multiplayer menu of the PS2 game
    constexpr int32_t ActionLabel = 1;      // a line of text above the rows (the first entry only)
    constexpr int32_t ActionChangeMenu = 2;
    constexpr int32_t ActionBase = 0x57460; // the fix's rows: ActionBase + row; far from the game's and other mods' ids
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
    constexpr size_t MainMenuSwitch = 0x116;
    constexpr size_t MousePos = 0x118;      // m_nMousePosX, m_nMousePosY
    constexpr size_t MouseTempPos = 0x120;  // the newest position, read by the next button processing
    constexpr size_t MenuSprites = 0x4C8;   // m_aMenuSprites: a CSprite2d (its texture) each
    constexpr size_t FadeAlpha = 0x51C;
    constexpr size_t MouseOldPos = 0x53C;
    constexpr size_t HoverOption = 0x544;
    constexpr size_t CurrentScreen = 0x548;
    constexpr size_t CurrentEntry = 0x54C;
    constexpr size_t MouseHoverEntry = 0x550;
    constexpr int32_t HoverRow = 2;         // HOVEROPTION_RANDOM_ITEM: the mouse is over a row
    enum MenuSprite { SpriteDownOff = 14, SpriteDownOn, SpriteUpOff, SpriteUpOn };

    CMenuScreen* aScreens = nullptr;
    GameRef<int32_t> pLanguage;
    size_t RightTextOffset = 0xC;           // Draw's right text pointer on the stack
    bool bReady = false;

    void* DMAudio = nullptr;
    void(__fastcall* PlayFrontEndSound)(void* audio, void* edx, uint16_t sound, uint32_t volume) = nullptr;
    constexpr uint16_t SoundNavigation = 0x97;
    constexpr uint16_t SoundSettingChange = 0x98;

    RWGTAMenu::Model Model;
    RWGTAMenu::WheelInput Wheel;
    RWGTAMenu::ValueAreas ValueAreas;
    void(__cdecl* DrawRect)(const float*, const uint8_t*) = nullptr;
    void(__fastcall* DrawSprite)(void* sprite, void* edx, const float* rect, const uint8_t* colour) = nullptr;
    using ValuePrintFn = void(__cdecl*)(float, float, const wchar_t*);
    ValuePrintFn ValuePrintPrevious = nullptr;
    float(__cdecl* ValueStringWidth)(const wchar_t*, bool) = nullptr;

    size_t PluginPage = SIZE_MAX;           // the plugin shown on ScreenPage
    size_t CategoryPage = SIZE_MAX;         // the category shown on ScreenCategory
    int LastScreen = -1;
    int ScreenAtStart = -1;                 // the screen when the game began processing the buttons
    uint8_t* HelpManager = nullptr;         // the menu, for its selected row's help
    const RWGTAMenu::Row* HelpRow = nullptr;    // the row whose help is in WFPHELP
    std::vector<const RWGTAMenu::Row*> ShownRows;   // the rows of the list shown, slot by slot (null: Back)
    bool LabelShown = false;                // the list starts with the restart line
    bool CentredRows = false;               // the plugin's screen lists categories: they are centred
    bool CentreLabel = false;               // the row whose label is drawn next is centred
    uint8_t* FontCentre = nullptr;          // CFont::Details.centre
    bool LabelReady = false;                // the label's text is answered
    std::map<std::string, std::u16string, std::less<>> Texts;   // the rows' and titles' text by key, in the game's font

    int32_t& Field(uint8_t* manager, size_t offset) { return *reinterpret_cast<int32_t*>(manager + offset); }
    int32_t& Screen(uint8_t* manager) { return Field(manager, CurrentScreen); }
    int32_t& Entry(uint8_t* manager) { return Field(manager, CurrentEntry); }
    bool InGame(uint8_t* manager) { return !*(manager + MainMenuSwitch); }
    bool OurList(int screen) { return screen == ScreenRoot || screen == ScreenPage || screen == ScreenCategory; }

    // The language of the fix's texts and of the other plugins' pages: MAIN.TextLanguage (cfg and editors only, for
    // translations of the game that replaced one of its languages), or the game's language
    std::string LanguageTag()
    {
        const auto chosen = FixSettings().Get("MAIN.TextLanguage");
        if (!chosen.empty() && chosen != "auto")
            return chosen;
        static const char* tags[] = { "en", "fr", "de", "it", "es" };
        int language = pLanguage.is_initialized() ? *pLanguage : 0;
        return language >= 0 && language < int(std::size(tags)) ? tags[language] : "en";
    }

    void __cdecl ValuePrint(float x, float y, const wchar_t* text)
    {
        if (bReady && (LastScreen == ScreenPage || LastScreen == ScreenCategory) && text && *text && ValueStringWidth)
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

    // The model's page shown on a screen of the fix
    size_t PageOf(int screen)
    {
        return screen == ScreenRoot ? RWGTAMenu::Model::RootPage : screen == ScreenPage ? PluginPage :
            screen == ScreenCategory ? CategoryPage : SIZE_MAX;
    }

    // The list shown: the rows in its window, Back among them. A plugin or category row opens the next screen like the
    // game's own rows that open a screen; settings have their values like Display Setup.
    void FillList(uint8_t* manager, int screenId)
    {
        size_t page = PageOf(screenId);
        auto& screen = aScreens[screenId];
        ClearEntries(screen);
        bool root = screenId == ScreenRoot;
        ShownRows = page < Model.Pages().size() ? Model.Shown(page, true) : std::vector<const RWGTAMenu::Row*>{};
        auto& window = Model.Window(page);
        // A changed setting that applies at the next start: the game's own line of text above the rows says so
        bool label = !root && LabelReady && Model.RestartRequired();
        LabelShown = label;
        int first = label ? 1 : 0;
        if (label)
        {
            SetText("WFPWARN", Model.Text("menu.restartwarning"));
            SetEntry(screen.entries[0], ActionLabel, "WFPWARN", screenId);
        }
        int count = int(ShownRows.size());
        if (screenId == ScreenPage)
        {
            auto rows = Model.VisibleRows(page);
            CentredRows = std::any_of(rows.begin(), rows.end(), [](auto row) { return row && row->kind == RWGTAMenu::RowKind::Category; });
        }
        for (int i = 0; i < count; ++i)
        {
            char key[8];
            sprintf_s(key, root ? "WFPL%02d" : "WFPR%02d", i);
            auto row = ShownRows[i];
            if (!row)
            {
                SetEntry(screen.entries[first + i], ActionChangeMenu, "FEDS_TB", 0);
                continue;
            }
            SetText(key, Model.Label(*row));
            if (RWGTAMenu::Model::Opens(*row))
            {
                SetEntry(screen.entries[first + i], ActionChangeMenu, key, root ? ScreenPage : ScreenCategory);
                continue;
            }
            SetText(std::string(key) + "V", Model.Value(*row));
            SetEntry(screen.entries[first + i], ActionBase + i, key, screenId);
        }
        Entry(manager) = count ? first + int(window.Slot()) : first;
        // Back from the next screen returns to this row
        if (screenId == ScreenRoot)
            aScreens[ScreenPage].parentEntry[0] = aScreens[ScreenPage].parentEntry[1] = Entry(manager);
        else if (screenId == ScreenPage)
            aScreens[ScreenCategory].parentEntry[0] = aScreens[ScreenCategory].parentEntry[1] = Entry(manager);
        if (page < Model.Pages().size())
            SetText(root ? "WFPT0" : screenId == ScreenPage ? "WFPT1" : "WFPT2", Model.Pages()[page].title);
    }

    // The row of the list selected on a screen (the one a plugin or category row opened)
    const RWGTAMenu::Row* SelectedListRow(size_t page)
    {
        auto rows = Model.ListRows(page, true);
        auto& window = Model.Window(page);
        return window.Selected() < rows.size() ? rows[window.Selected()] : nullptr;
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

    // Before and after the game processes the buttons: the rows of the screen shown, from the settings in effect. The
    // game opens the plugin's or the category's screen of the row accepted (the row selected in the list it left), and
    // Back returns to the list's screen, where the list kept its row.
    void Update(uint8_t* manager)
    {
        HelpManager = manager;
        int screen = Screen(manager);
        if (screen != LastScreen)
            HelpRow = nullptr;
        bool ours = OurList(screen) || (bJoined && screen == ScreenDisplay);
        if (screen == ScreenOptions)
            SetText("WFPOPT", RWGTAMenu::PluginText(FixSettings(), LanguageTag(), "menu.plugins"));
        if (ours && !Model.IsOpen())
        {
            HMODULE self = nullptr;
            GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCWSTR>(&Update), &self);
            Model.OpenMenu(self, LanguageTag(), PageSlots, JoinedIds());
            Model.Window(RWGTAMenu::Model::RootPage).SetSlots(RootSlots);
            PluginPage = CategoryPage = SIZE_MAX;
        }
        else if (!ours && screen != LastScreen && Model.IsOpen())
        {
            Model.Close();
        }
        // The menu's language changed while the lists are open (an editor): their texts follow at once
        else if (ours && Model.SelectLanguage(LanguageTag()))
        {
            ShownRows.clear();
            HelpRow = nullptr;
        }

        if (screen == ScreenRoot && LastScreen == ScreenOptions)
        {
            Model.Window(RWGTAMenu::Model::RootPage).Reset();
        }
        else if (screen == ScreenPage && LastScreen == ScreenRoot)
        {
            auto row = SelectedListRow(RWGTAMenu::Model::RootPage);
            PluginPage = row ? Model.Enter(*row) : SIZE_MAX;
        }
        else if (screen == ScreenCategory && LastScreen == ScreenPage)
        {
            auto row = SelectedListRow(PluginPage);
            CategoryPage = row ? Model.Enter(*row) : SIZE_MAX;
        }
        else if (screen == ScreenPage && LastScreen == ScreenCategory)
        {
            Model.Back(CategoryPage);
        }
        else if (screen == ScreenRoot && LastScreen == ScreenPage)
        {
            Model.Back(PluginPage);
        }

        if (OurList(screen) && Model.IsOpen())
            FillList(manager, screen);
        else if (screen == ScreenDisplay && bJoined)
            FillJoined();
        LastScreen = screen;
    }

    // The scroll bar of the lists, like the one of the game's skin list on Player Setup: its arrows above and below a
    // bar on a dark track, in the right margin of the rows. In the menu's 640x480 units.
    constexpr float BarRight = 33.0f;       // the arrows' left edge from the right, as the skin list's
    constexpr float BarWidth = 16.0f;
    constexpr float ArrowHeight = 17.0f;

    struct ListGeometry
    {
        float top, pitch;                   // the first slot's top and the distance between rows
    };

    // The rows of the Options layout (the plugins list) and of Display Setup's (plugins and categories)
    ListGeometry Geometry(int screen)
    {
        if (screen == ScreenRoot)
            return { 120.0f, 25.7f };
        // The restart line is printed above the rows; they keep their places
        return { 77.0f, 21.4f };
    }

    float BarTop(int screen) { return Geometry(screen).top; }
    float BarBottom(int screen, const MenuList::Window& window)
    {
        auto geometry = Geometry(screen);
        return geometry.top + geometry.pitch * float(window.Slots());
    }

    // Where the mouse was on the scroll bar when the menu was last drawn (the mouse and the bar in the same units as
    // the game's own hit tests there): -1 / +1 the up / down arrow, 2 the bar between them, 0 off it
    int BarHit = 0;

    // The scroll bar is shown: the list has more rows than its window
    bool BarShown(uint8_t* manager)
    {
        int screen = Screen(manager);
        if (!bReady || !Model.IsOpen() || !OurList(screen) || screen != LastScreen)
            return false;
        auto& window = Model.Window(PageOf(screen));
        return window.Above() || window.Below();
    }

    // The mouse is on the scroll bar (its rows don't take the mouse there)
    bool MouseOnBar(uint8_t* manager)
    {
        return BarHit && BarShown(manager);
    }

    // -1 / +1 when the mouse is on the up / down arrow, 0 otherwise
    int ArrowUnderMouse(uint8_t* manager)
    {
        return MouseOnBar(manager) && BarHit != 2 ? BarHit : 0;
    }

    // Up/Down, Page Up/Page Down, Home/End: the selection moves and the window follows; true when it changed. The
    // game plays the sound of its own presses; repeats and the other keys play it here.
    bool MoveSelection(uint8_t* manager, int direction, bool page, bool edge, bool sound)
    {
        int screen = Screen(manager);
        auto& window = Model.Window(PageOf(screen));
        bool moved = edge ? window.Edge(direction) : page ? window.Page(direction) : window.Move(direction, true);
        if (!moved)
            return false;
        if (sound && PlayFrontEndSound)
            PlayFrontEndSound(DMAudio, nullptr, SoundNavigation, 0);
        FillList(manager, screen);
        return true;
    }

    // The wheel and the arrows: the window moves by a row, the selection stays on its row while it is shown
    void ScrollList(uint8_t* manager, int direction)
    {
        int screen = Screen(manager);
        if (Model.Window(PageOf(screen)).Scroll(direction))
        {
            if (PlayFrontEndSound)
                PlayFrontEndSound(DMAudio, nullptr, SoundNavigation, 0);
            FillList(manager, screen);
        }
    }

    // The mouse moved over a row (the game selects the entry under the mouse only while it moves)
    void FollowHover(uint8_t* manager)
    {
        int screen = Screen(manager);
        if (!Model.IsOpen() || !OurList(screen) || screen != LastScreen || MouseOnBar(manager))
            return;
        int slot = Entry(manager) - (LabelShown ? 1 : 0);
        auto& window = Model.Window(PageOf(screen));
        if (slot >= 0 && size_t(slot) < window.Shown() && size_t(slot) != window.Slot())
            window.Point(size_t(slot));
    }

    int32_t WheelDelta = 0;                 // the mouse wheel's movement read by the game since the last frame
    uint8_t* MouseLeft = nullptr;           // CPad's left mouse button as read this frame (TempMouseControllerState)
    MenuList::WheelNotches WheelNotches;
    MenuList::Repeat PageUpKey, PageDownKey, ArrowClick, UpHeld, DownHeld;
    MenuList::StickAxis StickY;
    bool HomeDown = false, EndDown = false;
    int ArrowHeld = 0;                      // the arrow clicked and held (drawn pressed)

    bool GameInFront()
    {
        DWORD pid = 0;
        GetWindowThreadProcessId(GetForegroundWindow(), &pid);
        return pid == GetCurrentProcessId();
    }

    // What the game's menu doesn't read: held Up/Down (keys, d-pad, stick) repeat, Page Up/Page Down and the shoulder
    // buttons move by a window, Home/End go to the ends, a held click on an arrow scrolls
    void ExtraInput(uint8_t* manager)
    {
        bool active = Model.IsOpen() && OurList(Screen(manager)) && Screen(manager) == LastScreen && GameInFront();
        auto held = [&](int key) { return active && (GetAsyncKeyState(key) & 0x8000) != 0; };
        const uint64_t now = GetTickCount64();
        XINPUT_STATE pad{};
        DWORD slot = 0;
        bool padRead = active && PadInput::Read(pad, slot);
        int stick = padRead ? StickY.Direction(pad.Gamepad.sThumbLY) : 0;
        WORD buttons = padRead ? pad.Gamepad.wButtons : 0;
        if (DownHeld.Step(held(VK_DOWN) || (buttons & XINPUT_GAMEPAD_DPAD_DOWN) || stick < 0, now, false))
            MoveSelection(manager, 1, false, false, true);
        if (UpHeld.Step(held(VK_UP) || (buttons & XINPUT_GAMEPAD_DPAD_UP) || stick > 0, now, false))
            MoveSelection(manager, -1, false, false, true);
        if (PageUpKey.Step(held(VK_PRIOR) || (buttons & XINPUT_GAMEPAD_LEFT_SHOULDER), now))
            MoveSelection(manager, -1, true, false, true);
        if (PageDownKey.Step(held(VK_NEXT) || (buttons & XINPUT_GAMEPAD_RIGHT_SHOULDER), now))
            MoveSelection(manager, 1, true, false, true);
        bool home = held(VK_HOME), end = held(VK_END);
        if (home && !HomeDown)
            MoveSelection(manager, -1, false, true, true);
        if (end && !EndDown)
            MoveSelection(manager, 1, false, true, true);
        HomeDown = home;
        EndDown = end;
        int arrow = active && MouseLeft && *MouseLeft ? ArrowUnderMouse(manager) : 0;
        if (arrow != ArrowHeld)
            ArrowClick.Step(false, now);
        ArrowHeld = arrow;
        if (ArrowClick.Step(arrow != 0, now))
            ScrollList(manager, arrow);
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
        const auto x = std::clamp(field(0x120), 0, std::max(0, RsGlobal->width));
        const auto y = std::clamp(field(0x124), 0, std::max(0, RsGlobal->height));
        field(0x118) = field(0x53C) = x;
        field(0x11C) = field(0x540) = y;
    }

    void* ProcessButtonPressesPrevious = nullptr;
    void __fastcall ProcessButtonPresses(uint8_t* manager, void* edx)
    {
        SyncResolutionCursor(manager);
        ScreenAtStart = Screen(manager);
        if (bReady)
        {
            FollowHover(manager);
            Update(manager);
            ExtraInput(manager);
        }
        bool list = bReady && Model.IsOpen() && OurList(Screen(manager)) && Screen(manager) == LastScreen;
        bool overValue = ValueAreas.Contains(float(Field(manager, MouseTempPos)), float(Field(manager, MouseTempPos + 4)));
        bool listWheel = list && !overValue;
        Wheel.Begin(listWheel);
        // Every notch the mouse reported since the last frame scrolls one row (the game's flag would show one per frame)
        int notches = listWheel ? WheelNotches.Take(std::exchange(WheelDelta, 0)) : (WheelNotches.Clear(), WheelDelta = 0, 0);
        for (; notches; notches += notches > 0 ? -1 : 1)
            ScrollList(manager, notches > 0 ? -1 : 1);
        bool onBar = list && MouseOnBar(manager);
        if (onBar)
        {
            // A click on the scroll bar doesn't choose the row at its height
            Field(manager, HoverOption) = 0;
        }
        else if (list && Field(manager, HoverOption) == HoverRow && MouseLeft && *MouseLeft)
        {
            // A click chooses the row under the mouse: it is the list's selection, also for the screen it opens
            int slot = Field(manager, MouseHoverEntry) - (LabelShown ? 1 : 0);
            if (slot >= 0 && Model.Window(PageOf(Screen(manager))).Point(size_t(slot)))
                FillList(manager, Screen(manager));
        }
        reinterpret_cast<void(__fastcall*)(uint8_t*, void*)>(ProcessButtonPressesPrevious)(manager, edx);
        Wheel.End();
        if (bReady)
        {
            Update(manager);
            // The rows don't follow the mouse while it is on the scroll bar
            if (onBar && Screen(manager) == ScreenAtStart)
            {
                Field(manager, MouseOldPos) = Field(manager, MousePos);
                Field(manager, MouseOldPos + 4) = Field(manager, MousePos + 4);
            }
        }
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
        if (bJoined && Screen(manager) == ScreenDisplay)
        {
            auto joined = JoinedRow(aScreens[ScreenDisplay].entries[entry].action - ActionJoined);
            if (!joined)
                return false;
            if (Model.Change(*joined, step, InGame(manager)) && dir != 0 && PlayFrontEndSound)
                PlayFrontEndSound(DMAudio, nullptr, SoundSettingChange, 0);
            FillJoined();
            return true;
        }
        // The game also calls it after accepting a row that opened another screen: not a row of that screen
        int screen = Screen(manager);
        if ((screen != ScreenPage && screen != ScreenCategory) || screen != ScreenAtStart || !Model.IsOpen())
            return false;
        int row = aScreens[screen].entries[entry].action - ActionBase;
        if (row < 0 || row >= int(ShownRows.size()) || !ShownRows[row] || RWGTAMenu::Model::Opens(*ShownRows[row]))
            return false;
        bool changed = Model.Change(*ShownRows[row], step, InGame(manager));
        // Accept has its sound from the game already
        if (changed && dir != 0 && PlayFrontEndSound)
            PlayFrontEndSound(DMAudio, nullptr, SoundSettingChange, 0);
        FillList(manager, screen);
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

    // A row of the plugin's screen that is centred: one that opens a category, Restore Defaults and Back
    bool Centred(const char* key)
    {
        if (!bReady || !CentredRows || !HelpManager || Screen(HelpManager) != ScreenPage || LastScreen != ScreenPage || !key)
            return false;
        if (strncmp(key, "FEDS_TB", 8) == 0)
            return true;
        int row = -1;
        if (sscanf_s(key, "WFPR%02d", &row) != 1 || row < 0 || row >= int(ShownRows.size()) || !ShownRows[row])
            return false;
        return RWGTAMenu::Model::Opens(*ShownRows[row]) || ShownRows[row]->kind == RWGTAMenu::RowKind::RestoreDefaults;
    }

    const wchar_t* __fastcall RowText(void* text, void* edx, const char* key)
    {
        CentreLabel = Centred(key);
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

    // CFont::PrintString of a row's label in CMenuManager::Draw (its shadow, then the text): a centred row is printed
    // at the middle of the screen with the font centred, as the Options screen's rows are
    using LabelPrintFn = void(__cdecl*)(float, float, const wchar_t*);
    LabelPrintFn LabelPrintPrevious = nullptr;
    float(__fastcall* StretchX)(uint8_t* manager, void* edx, float x) = nullptr;

    void __cdecl LabelPrint(float x, float y, const wchar_t* text)
    {
        if (!CentreLabel || !FontCentre || !StretchX || *FontCentre)
            return LabelPrintPrevious(x, y, text);
        *FontCentre = 1;
        LabelPrintPrevious(x + StretchX(HelpManager, nullptr, 320.0f) - StretchX(HelpManager, nullptr, 50.0f), y, text);
        *FontCentre = 0;
    }

    // CMenuManager::DisplayHelperText: the selected row's help in place of the game's general hint
    void* HelpTextPrevious = nullptr;
    GameRef<float> CentreSize;              // CFont::Details.centreSize
    float HelpCentreSize = -1.0f;           // the centre size before the help's, put back after it

    const RWGTAMenu::Row* SelectedRow(uint8_t* manager)
    {
        int screen = Screen(manager);
        int entry = Entry(manager);
        if (entry < 0 || entry >= int(std::size(aScreens[screen].entries)))
            return nullptr;
        int action = aScreens[screen].entries[entry].action;
        if ((screen == ScreenPage || screen == ScreenCategory) && LastScreen == screen)
        {
            // Lists of plugins and categories have no help, like the game's screens of screens
            int row = action - ActionBase;
            return row >= 0 && row < int(ShownRows.size()) && ShownRows[row] && !RWGTAMenu::Model::Opens(*ShownRows[row]) ? ShownRows[row] : nullptr;
        }
        if (screen == ScreenDisplay && bJoined)
            return JoinedRow(action - ActionJoined);
        return nullptr;
    }

    const wchar_t* __fastcall HelpText(void* text, void* edx, const char* key)
    {
        if (bReady && HelpManager && Model.IsOpen())
        {
            if (auto row = SelectedRow(HelpManager))
            {
                if (row != HelpRow)
                {
                    HelpRow = row;
                    // The menu font draws an arrow for ';'
                    auto help = Model.Help(*row);
                    std::replace(help.begin(), help.end(), ';', ',');
                    SetText("WFPHELP", help);
                }
                if (auto answer = Answer("WFPHELP"); answer && *answer)
                {
                    // Wrapped inside the menu's frame
                    if (CentreSize.is_initialized() && StretchX)
                    {
                        HelpCentreSize = *CentreSize;
                        *CentreSize = 2.0f * (StretchX(HelpManager, nullptr, 320.0f) - StretchX(HelpManager, nullptr, 40.0f));
                    }
                    return answer;
                }
            }
        }
        return reinterpret_cast<const wchar_t* (__fastcall*)(void*, void*, const char*)>(HelpTextPrevious)(text, edx, key);
    }

    // Checks the game's screens before they are changed; false when another mod changed them
    bool ScreensAsShipped()
    {
        auto& root = aScreens[ScreenRoot];
        auto& page = aScreens[ScreenPage];
        auto& category = aScreens[ScreenCategory];
        if (strncmp(root.name, "FEM_MCM", 8) != 0 || strncmp(page.name, "FEM_MC2", 8) != 0 || strncmp(category.name, "FET_MP", 8) != 0)
            return false;
        // Nothing of the game leads to them (Back rows go to their screen's previous one, whatever their target)
        for (int s = 0; s < 57; ++s)
        {
            for (auto& entry : aScreens[s].entries)
            {
                if (entry.action == ActionChangeMenu && OurList(entry.targetMenu) && !OurList(s) && strncmp(entry.name, "FEDS_TB", 8) != 0)
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
        auto& category = aScreens[ScreenCategory];
        category.previousPage[0] = category.previousPage[1] = ScreenPage;
        category.parentEntry[0] = category.parentEntry[1] = 0;
        strncpy_s(root.name, "WFPT0", _TRUNCATE);
        strncpy_s(page.name, "WFPT1", _TRUNCATE);
        strncpy_s(category.name, "WFPT2", _TRUNCATE);
        ClearEntries(root);
        ClearEntries(page);
        ClearEntries(category);
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
        if (count + int(std::size(Joined)) > 13)
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
    void RememberResolutionCursor()
    {
        ResolutionCursorHeld = GetCursorPos(&ResolutionCursor) != FALSE;
    }

    // A text of the fix in the game's language, upper case in the game's font (empty when it has none)
    const wchar_t* GameText(const std::string& key)
    {
        static std::map<std::string, std::u16string, std::less<>> cache;
        auto language = LanguageTag();
        auto& text = cache[language + ":" + key];
        if (text.empty())
        {
            auto utf8 = RWGTAMenu::PluginText(FixSettings(), language, key);
            if (utf8 != key && utf8 != "@" + key)
                text = RWGTAMenu::ToGameFont(utf8, true, false);
        }
        return reinterpret_cast<const wchar_t*>(text.c_str());
    }

    void BeginMenuDraw()
    {
        ValueAreas.bounds.clear();
        BarHit = 0;
    }

    // The scroll bar of a list longer than its window, drawn like the skin list's: the arrows (pressed while held),
    // the track, and the bar for the rows shown with its shadow, fading in with the menu
    void DrawScrollIndicators()
    {
        auto manager = HelpManager;
        if (!manager || !DrawRect || !DrawSprite || !BarShown(manager))
            return;
        int screen = Screen(manager);
        auto& window = Model.Window(PageOf(screen));
        const float sx = SCREEN_WIDTH / 640.0f, sy = SCREEN_HEIGHT / 480.0f;
        const uint8_t alpha = uint8_t(std::clamp(Field(manager, FadeAlpha), 0, 255));
        auto rect = [&](float left, float top, float right, float bottom, const uint8_t* colour)
        {
            const float r[] = { left * sx, bottom * sy, right * sx, top * sy };
            DrawRect(r, colour);
        };
        auto sprite = [&](int index, float left, float top)
        {
            const float r[] = { left * sx, (top + 64.0f) * sy, (left + 64.0f) * sx, top * sy };
            const uint8_t white[] = { 255, 255, 255, alpha };
            DrawSprite(manager + MenuSprites + index * 4, nullptr, r, white);
        };
        const float left = 640.0f - BarRight;
        const float top = BarTop(screen), bottom = BarBottom(screen, window);
        // The mouse over the bar, in the units the menu is drawn in now
        const float mouseX = float(Field(manager, MousePos)) / sx, mouseY = float(Field(manager, MousePos + 4)) / sy;
        if (mouseX >= left - 2.0f && mouseX <= left + BarWidth + 2.0f && mouseY >= top - 2.0f && mouseY <= bottom + 2.0f)
            BarHit = mouseY <= top + ArrowHeight ? -1 : mouseY >= bottom - ArrowHeight ? 1 : 2;
        const uint8_t track[] = { 100, 100, 66, uint8_t(alpha * 205 / 255) };
        const uint8_t shadow[] = { 50, 50, 50, alpha };
        const uint8_t bar[] = { 235, 170, 50, alpha };
        rect(left - 1.0f, top, left + BarWidth - 1.0f, bottom, track);
        const float bodyTop = top + ArrowHeight, bodyBottom = bottom - ArrowHeight;
        const float count = float(std::max<size_t>(1, window.Count()));
        const float barTop = bodyTop + (bodyBottom - bodyTop) * float(window.First()) / count;
        const float barBottom = bodyTop + (bodyBottom - bodyTop) * float(window.First() + window.Shown()) / count;
        rect(left, barTop, left + BarWidth - 3.0f, barBottom + 1.0f, shadow);
        rect(left, barTop, left + BarWidth - 4.0f, barBottom, bar);
        sprite(ArrowHeld < 0 ? SpriteUpOn : SpriteUpOff, left, top);
        sprite(ArrowHeld > 0 ? SpriteDownOn : SpriteDownOff, left, bottom - ArrowHeight);
    }
}

class Options
{
public:
    Options()
    {
        WFP::onInitEvent() += []() { Model.RememberRunningSettings(FixSettings()); };
        WFP::onGameInitEvent() += []()
        {
            // CMenuManager::ProcessOnOffMenuOptions: the screen table, from the current entry's action
            auto pattern = hook::pattern("8B 8B ? ? ? ? 8B 83 ? ? ? ? 6B C9 61 8D 04 80 01 C1 8B 04 8D");
            auto language = hook::pattern("C7 05 ? ? ? ? 04 00 00 00 C6 05");
            auto rowText = hook::pattern("E8 ? ? ? ? 89 44 24 ? 8B B5");
            auto titleText = hook::pattern("8B 85 ? ? ? ? 69 C0 84 01 00 00 05 ? ? ? ? 50 E8 ? ? ? ? 50 FF B4 24");
            auto rightText = hook::pattern("8B 44 24 ? 89 84 24 ? ? ? ? DB 84 24 ? ? ? ? D8 44 24 ? D9 9C 24");
            auto rightTextStore = hook::pattern("E8 ? ? ? ? 89 44 24 ? E9 ? ? ? ? 0F BE 05");
            auto layout = hook::pattern("8B 85 ? ? ? ? 59 48 8B 8D ? ? ? ? 83 F8 37 0F 87 ? ? ? ? FF 24 85");
            auto buttons = hook::pattern("89 E9 E8 ? ? ? ? 83 3D ? ? ? ? 00 74");
            ProcessButtonPressesPrevious = injector::MakeCALL(buttons.get_first(2), ProcessButtonPresses, true).get();
            auto accept = hook::pattern("89 E9 E8 ? ? ? ? 80 7C 24 ? 00 0F 84");
            auto change = hook::pattern("89 E9 E8 ? ? ? ? 83 BD ? ? ? ? 37");
            auto sound = hook::pattern("B9 ? ? ? ? A2 ? ? ? ? 6A 00 68 98 00 00 00 E8");

            aScreens = reinterpret_cast<CMenuScreen*>(*pattern.get_first<uint8_t*>(23) - offsetof(CMenuScreen, entries));
            pLanguage.SetAddress(*language.get_first<int32_t*>(2));
            RightTextOffset = *rightTextStore.get_first<uint8_t>(8);
            DMAudio = *sound.get_first<void*>(1);
            PlayFrontEndSound = reinterpret_cast<decltype(PlayFrontEndSound)>(injector::GetBranchDestination(sound.get_first(17)).as_int());

            // Both call sites must reach the same function
            auto onOff = injector::GetBranchDestination(accept.get_first(2)).as_int();

            // The layout of the plugin and category screens: Display Setup's, when theirs is still the default one
            auto table = *layout.get_first<uintptr_t*>(26);
            if (table[ScreenPage - 1] != table[ScreenOptions - 1] || table[ScreenRoot - 1] != table[ScreenOptions - 1] ||
                table[ScreenCategory - 1] != table[ScreenOptions - 1])
                return;

            if (!ScreensAsShipped() || !AddOptionsEntry())
                return;
            injector::WriteMemory<uintptr_t>(&table[ScreenPage - 1], table[ScreenDisplay - 1], true);
            injector::WriteMemory<uintptr_t>(&table[ScreenCategory - 1], table[ScreenDisplay - 1], true);

            // The fix's rows on Display Setup, reset by the game's own Restore Defaults there
            auto restore = hook::pattern("C6 05 ? ? ? ? 01 C7 05 ? ? ? ? 00 01 00 00 C6 05 ? ? ? ? 01 C7 05 ? ? ? ? 9A 99 99 3F");
            if (AddJoinedEntries())
            {
                bJoined = true;
                static auto RestoreHook = safetyhook::create_mid(restore.get_first(), [](SafetyHookContext&)
                {
                    // With the game's own rows of Display Setup that keep their values in the fix's settings
                    auto ids = JoinedIds();
                    ids.insert({ "MAIN.CutsceneBorders", "MAIN.FPSLimit" });
                    RWGTAMenu::RestoreDefaults(FixSettings(), ids);
                    if (Model.IsOpen())
                        FillJoined();
                });
            }

            RowTextPrevious = injector::MakeCALL(rowText.get_first(0), RowText, true).get();
            // The rows' labels (the font's centre flag is tested just before)
            FontCentre = *hook::pattern("80 3D ? ? ? ? 00 75 05 E8 ? ? ? ? 0F BE 44 24 08 89 E9").get_first<uint8_t*>(2);
            auto labelPrint = hook::pattern("FF 74 24 24 FF B4 24 ? ? ? ? FF B4 24 ? ? ? ? E8 ? ? ? ? 83 C4 0C 83 7C 24 0C 00 0F 84");
            LabelPrintPrevious = reinterpret_cast<LabelPrintFn>(injector::MakeCALL(labelPrint.get_first(18), LabelPrint, true).as_int());
            TitleTextPrevious = injector::MakeCALL(titleText.get_first(18), TitleText, true).get();
            OnOffAcceptPrevious = injector::MakeCALL(accept.get_first(2), OnOffAccept, true).get();
            OnOffChangePrevious = injector::MakeCALL(change.get_first(2), OnOffChange, true).get();

            // The text of a page's first entry when it is a label (the restart line)
            auto labelText = hook::pattern("BE ? ? ? ? B9 ? ? ? ? 01 C6 83 C6 20 56 E8 ? ? ? ? 50");
            LabelTextPrevious = injector::MakeCALL(labelText.get_first(16), LabelText, true).get();
            LabelReady = true;

            // The general hint of DisplayHelperText: the selected row's help
            auto image = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
            auto headers = reinterpret_cast<PIMAGE_NT_HEADERS>(image + reinterpret_cast<PIMAGE_DOS_HEADER>(image)->e_lfanew);
            auto helpKey = reinterpret_cast<uintptr_t>(hook::range_pattern(image, image + headers->OptionalHeader.SizeOfImage,
                "46 45 54 5F 4D 49 47 00").get_first());
            char helpSignature[128]{};
            sprintf_s(helpSignature, "B9 ? ? ? ? 68 %02X %02X %02X %02X E8 ? ? ? ? 89 E9 89 C3",
                unsigned(helpKey & 255), unsigned((helpKey >> 8) & 255), unsigned((helpKey >> 16) & 255), unsigned(helpKey >> 24));
            auto helpText = hook::pattern(helpSignature).get_first<uint8_t>();
            HelpTextPrevious = injector::MakeCALL(helpText + 10, HelpText, true).get();
            const auto stretchAt = reinterpret_cast<uintptr_t>(helpText + 0x30);
            auto stretchX = hook::range_pattern(stretchAt, stretchAt + 5, "E8 ? ? ? ?").get_first();
            StretchX = reinterpret_cast<decltype(StretchX)>(injector::GetBranchDestination(stretchX).as_int());
            CentreSize.SetAddress(reinterpret_cast<float*>(*hook::pattern("68 ? ? ? ? 52 0F BF 05").get_first<uint8_t*>(1) + 0x2C));

            // CMenuManager::Draw shows the helper text on a few setup screens only: on the fix's pages too
            auto helper = hook::pattern("8B 85 ? ? ? ? 8B 9D ? ? ? ? 83 E8 05 83 F8 01 76 ? 83 E8 1E 74 ? 83 E8 15 75 ? 89 E9 E8");
            {
                static auto DisplayHelperText = reinterpret_cast<void(__fastcall*)(uint8_t*, void*)>(injector::GetBranchDestination(helper.get_first(32)).as_int());
                static auto HelperHook = safetyhook::create_mid(helper.get_first(37), [](SafetyHookContext& regs)
                {
                    auto manager = reinterpret_cast<uint8_t*>(regs.ebp);
                    if (bReady && (Screen(manager) == ScreenPage || Screen(manager) == ScreenCategory) && SelectedRow(manager))
                        DisplayHelperText(manager, nullptr);
                    if (HelpCentreSize >= 0.0f)
                    {
                        *CentreSize = HelpCentreSize;
                        HelpCentreSize = -1.0f;
                    }
                });
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
                    if (Screen(manager) != ScreenPage && Screen(manager) != ScreenCategory)
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

            // Up/Down on the fix's lists: the selection moves in the list's window (the game moved through the entries)
            auto navigation = hook::pattern("80 7C 24 0C 00 74 ? 83 BD ? ? ? ? 1E 74 ? FF 85");
            static auto NavigationHook = safetyhook::create_mid(navigation.get_first(), [](SafetyHookContext& regs)
            {
                auto manager = reinterpret_cast<uint8_t*>(regs.ebp);
                if (!bReady || !Model.IsOpen() || !OurList(Screen(manager)) || Screen(manager) != LastScreen)
                    return;
                auto& down = *reinterpret_cast<uint8_t*>(regs.esp + 0xC);
                auto& up = *reinterpret_cast<uint8_t*>(regs.esp + 0x10);
                if (down || up)
                {
                    MoveSelection(manager, down ? 1 : -1, false, false, false);
                    down = up = 0;
                }
            });

            // CPad::UpdateMouse: the DirectInput state's wheel movement (eax), before it becomes a flag; the buttons
            // before it (left, right, middle, wheel up)
            auto mouse = hook::pattern("8B 44 24 ? 85 C0 7E ? C6 05 ? ? ? ? 01");
            MouseLeft = *mouse.get_first<uint8_t*>(10) - 3;
            static auto WheelRead = safetyhook::create_mid(mouse.get_first(4), [](SafetyHookContext& regs)
            {
                if (Model.IsOpen() && OurList(LastScreen))
                    WheelDelta += int32_t(regs.eax);
            });

            // CSprite2d::Draw(const CRect&, const CRGBA&), for the scroll bar's arrows
            DrawSprite = reinterpret_cast<decltype(DrawSprite)>(injector::GetBranchDestination(hook::pattern("E8 ? ? ? ? EB ? 8D 44 20 ? B9").get_first()).as_int());

            auto valuePrint = hook::pattern("FF 74 24 0C FF B4 24 ? ? ? ? D9 9C 24 ? ? ? ? FF B4 24 ? ? ? ? E8");
            ValueStringWidth = reinterpret_cast<decltype(ValueStringWidth)>(0x5018A0);
            ValuePrintPrevious = reinterpret_cast<ValuePrintFn>(injector::MakeCALL(valuePrint.get_first(25), ValuePrint, true).as_int());
            auto wheelButtons = hook::pattern("80 3D ? ? ? ? 00 59 75 11 6A 00 E8 ? ? ? ? 80 3D ? ? ? ? 00 59");
            for (size_t i = 0; i < wheelButtons.size(); ++i)
            {
                auto match = wheelButtons.get(i);
                auto up = *match.get<uint8_t*>(2);
                if (*match.get<uint8_t*>(19) == up + 1)
                    Wheel.buttons = up;
            }
            auto rectangle = find_pattern("8B 44 24 04 53 8B 5C 24 0C 6A 00 53 53 53 53 50 E8", "E9 ? ? ? ? 8B 5C 24 0C 6A 00 53 53 53 53 50 E8");
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
