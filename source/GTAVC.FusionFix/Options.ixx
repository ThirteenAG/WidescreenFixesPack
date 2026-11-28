module;

#include <stdafx.h>
#include <intrin.h>
#include "common.h"
#include "FixSettings.hpp"
#include "Shared/RWGTA/FixMenu.hpp"
#include "Shared/PadInput.hpp"

export module Options;

import Skeleton;

// The fix's settings in the game's frontend: "Plugins" at the end of the Options screen shows the list of plugins (the
// fix first, then the other plugins with ASI Settings, see Shared/RWGTA/FixMenu.hpp). A plugin opens its settings with
// their values like Display Setup, or, when its settings come in several categories, the list of them, each opening
// its settings. In the main menu and in the pause menu, which share the frontend. A screen holds twelve rows, so
// every list scrolls through a window of rows (Back scrolls with them): Up/Down move and scroll by one row and wrap at
// the ends like the game's lists, Page Up/Page Down move by a window, Home/End go to the ends, the mouse wheel and the
// arrows in the right margin scroll, the mouse selects the row under it while it moves.
//
// Vice City's screen table (aScreens) has no screen the PC game leaves unused, so the Options screen shows the fix's
// rows while they are open: its rows and title are saved, replaced, and put back when the fix's pages are left (Back,
// Escape, or any other way off the screen). Nothing else of the frontend changes, and only one row is added to the
// Options screen, before its Back row; when another mod changed that row, the fix leaves the menu alone and the
// settings stay in the .cfg and FusionFixHub.
//
// A changed setting that applies at the next start puts the game's own label line above the page's rows. Vice City's
// helper text is for short notices only, so the rows have no description line, like the game's own settings.
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
    constexpr int PageRows = 11;            // rows of a settings list shown at once, Back included
    constexpr int ListRows = 10;            // rows of a list of plugins or categories shown at once, Back included
    constexpr int ScreenOptions = 27;
    constexpr int16_t ActionLabel = 1;      // the line of text above the rows (the first entry only)
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
    GameRef<int32_t> pLanguage;
    bool bReady = false;

    enum class Mode { None, Page };
    Mode State = Mode::None;
    CMenuScreen SavedOptions{};
    int OpenRow = 0;                        // the "Plugins" row of the Options screen
    size_t CurrentPage = 0;
    bool CenteredPage = false;              // a list of plugins or categories: centred like the Options screen

    RWGTAMenu::Model Model;
    RWGTAMenu::WheelInput Wheel;
    RWGTAMenu::ValueAreas ValueAreas;
    void* Audio = nullptr;
    void(__fastcall* NavigationSound)(void*, void*, uint16_t, uint32_t) = nullptr;
    void(__cdecl* DrawRect)(const float*, const uint8_t*) = nullptr;
    void(__fastcall* DrawSprite)(void* sprite, void* edx, const float* rect, const uint8_t* colour) = nullptr;
    using ValuePrintFn = void(__cdecl*)(float, float, const wchar_t*);
    ValuePrintFn ValuePrintPrevious = nullptr;
    float(__cdecl* ValueStringWidth)(const wchar_t*, bool) = nullptr;

    std::vector<const RWGTAMenu::Row*> ShownRows;
    bool LabelShown = false;                // the page starts with the restart line
    uint8_t* FontDetails = nullptr;         // CFont::Details: scaleX +4, scaleY +8, wrapX +0x2C
    ValuePrintFn LabelPrintPrevious = nullptr;
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
        const auto chosen = FixSettings().Get("MAIN.TextLanguage");
        if (!chosen.empty() && chosen != "auto")
            return chosen;
        static const char* tags[] = { "en", "fr", "de", "it", "es" };
        int language = pLanguage.is_initialized() ? *pLanguage : 0;
        return language >= 0 && language < int(std::size(tags)) ? tags[language] : "en";
    }

    void __cdecl ValuePrint(float x, float y, const wchar_t* text)
    {
        if (bReady && State == Mode::Page && text && *text)
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

    int IndicatorUnderMouse(uint8_t* manager);

    // The mouse moved over a row of the window (the game selects the entry under the mouse only while it moves)
    void FollowHover(uint8_t* manager)
    {
        if (State != Mode::Page || Screen(manager) != ScreenOptions || IndicatorUnderMouse(manager))
            return;
        int slot = Option(manager) - FirstRowEntry();
        auto& window = Model.Window(CurrentPage);
        if (slot >= 0 && size_t(slot) < window.Shown() && size_t(slot) != window.Slot())
            window.Point(size_t(slot));
    }

    // The page shown: the rows in its window with their values, Back among them
    void FillPage(uint8_t* manager)
    {
        auto& screen = aScreens[ScreenOptions];
        for (auto& entry : screen.entries)
            entry = {};
        strncpy_s(screen.name, "WFPT1", _TRUNCATE);
        CenteredPage = Centered(CurrentPage);
        ShownRows = Model.Shown(CurrentPage, true);
        auto& window = Model.Window(CurrentPage);
        // A changed setting that applies at the next start: the game's own label line above the rows says so
        bool label = !CenteredPage && Model.RestartRequired();
        if (label != LabelShown)
        {
            LabelShown = label;
            // The highlight moves with its row (m_nOptionHighlightTransitionBlend)
            *reinterpret_cast<int32_t*>(manager + 0x10C) = 0;
        }
        int first = FirstRowEntry();
        if (label)
        {
            SetText("WFPWARN", Model.Text("menu.restartwarning"));
            SetEntry(screen.entries[0], ActionLabel, "WFPWARN", 320, 97, AlignCenter);
        }
        int count = int(ShownRows.size());
        // Below the label the rows are a little closer, so that Back stays on the menu's card
        uint16_t y = CenteredPage ? 132 : label ? 125 : 78;
        uint16_t step = CenteredPage ? 29 : label ? 24 : 25;
        for (int i = 0; i < count; ++i, y += step)
        {
            char key[8];
            sprintf_s(key, "WFPR%02d", i);
            if (!ShownRows[i])
            {
                SetEntry(screen.entries[first + i], ActionBase + i, "FEDS_TB", 320, CenteredPage ? y : y + 10, AlignCenter);
                continue;
            }
            SetText(key, Model.Label(*ShownRows[i]));
            SetText(std::string(key) + "V", Model.Value(*ShownRows[i]));
            // Rows without a value (categories, Restore defaults) are centred like the game's own on Display Setup
            auto kind = ShownRows[i]->kind;
            if (CenteredPage || (kind != RWGTAMenu::RowKind::Setting && kind != RWGTAMenu::RowKind::Preset))
                SetEntry(screen.entries[first + i], ActionBase + i, key, 320, y, AlignCenter);
            else
                SetEntry(screen.entries[first + i], ActionBase + i, key, 40, y, AlignLeft);
        }
        Option(manager) = count ? first + int(window.Slot()) : first;
        if (CurrentPage < Model.Pages().size())
            SetText("WFPT1", Model.Pages()[CurrentPage].title);
    }

    void ShowPage(uint8_t* manager, size_t page)
    {
        CurrentPage = page;
        State = Mode::Page;
        Model.Window(page).SetSlots(Centered(page) ? ListRows : PageRows);
        FillPage(manager);
        FadeIn(manager);
    }

    void Open(uint8_t* manager)
    {
        HMODULE self = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCWSTR>(&Open), &self);
        Model.OpenMenu(self, LanguageTag(), PageRows);
        SavedOptions = aScreens[ScreenOptions];
        LabelShown = false;
        ShowPage(manager, RWGTAMenu::Model::RootPage);
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
        LabelShown = false;
    }

    // Back from a page: to the list it was opened from, with that row selected, or from the plugins list to the
    // Options screen
    void Back(uint8_t* manager)
    {
        if (State != Mode::Page)
            return;
        size_t parent = Model.Back(CurrentPage);
        if (parent == SIZE_MAX)
        {
            Close();
            Option(manager) = OpenRow;
            FadeIn(manager);
            return;
        }
        ShowPage(manager, parent);
    }

    // Up/Down, Page Up/Page Down, Home/End, the wheel and the arrows: the window moves; true when it changed
    bool MoveSelection(uint8_t* manager, int direction, bool page, bool edge)
    {
        auto& window = Model.Window(CurrentPage);
        bool moved = edge ? window.Edge(direction) : page ? window.Page(direction) : window.Move(direction, true);
        if (!moved)
            return false;
        NavigationSound(Audio, nullptr, 0xC3, 0);
        *reinterpret_cast<int32_t*>(manager + 0x10C) = 0;
        FillPage(manager);
        return true;
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
        const auto x = std::clamp(field(0x64), 0, std::max(0, RsGlobal->width));
        const auto y = std::clamp(field(0x68), 0, std::max(0, RsGlobal->height));
        field(0x12C) = field(0x134) = x;
        field(0x130) = field(0x138) = y;
    }

    // The scroll bar of the lists, like the one of the game's skin list on Player Setup: its arrows (the frontend's
    // sprites) above and below a bar on a dark track, in the right margin. In the menu's 640x480 units: x from the right
    // edge, the top and bottom lines.
    constexpr float BarRight = 33.0f;       // the arrows' left edge from the right, as the skin list's
    constexpr float BarWidth = 16.0f;
    constexpr float ArrowHeight = 17.0f;
    constexpr size_t MenuSprites = 0x84;    // m_aFrontEndSprites: a CSprite2d (its texture) each
    constexpr size_t MousePos = 0x12C;      // m_nMousePosX, m_nMousePosY
    enum MenuSprite { SpriteDownOff = 22, SpriteDownOn, SpriteUpOff, SpriteUpOn };
    float IndicatorTop() { return CenteredPage ? 122.0f : LabelShown ? 115.0f : 68.0f; }
    float IndicatorBottom() { return CenteredPage ? 418.0f : LabelShown ? 407.0f : 370.0f; }

    // Where the mouse was on the scroll bar when the menu was last drawn (the mouse and the bar in the same units as
    // the game's own hit tests there): -1 / +1 the up / down arrow, 2 the bar between them, 0 off it
    int BarHit = 0;
    int ArrowHeld = 0;                      // the arrow clicked and held (drawn pressed)

    // -1 / +1 when the mouse is on the up / down arrow, 2 on the bar between them, 0 otherwise
    int IndicatorUnderMouse(uint8_t* manager)
    {
        if (State != Mode::Page || Screen(manager) != ScreenOptions)
            return 0;
        return BarHit;
    }

    // Keys the game's menu doesn't read: Page Up/Page Down move by a window, Home/End to the ends; a held click on an
    // arrow scrolls. Each repeats while held.
    int32_t WheelDelta = 0;                 // the mouse wheel's movement read by the game since the last frame
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
        if (PageUpKey.Step(held(VK_PRIOR), now))
            MoveSelection(manager, -1, true, false);
        if (PageDownKey.Step(held(VK_NEXT), now))
            MoveSelection(manager, 1, true, false);
        bool home = held(VK_HOME), end = held(VK_END);
        if (home && !HomeDown)
            MoveSelection(manager, -1, false, true);
        if (end && !EndDown)
            MoveSelection(manager, 1, false, true);
        HomeDown = home;
        EndDown = end;
        int arrow = held(VK_LBUTTON) ? IndicatorUnderMouse(manager) : 0;
        arrow = arrow == 2 ? 0 : arrow;
        if (arrow != ArrowHeld)
            ArrowClick.Step(false, now);
        ArrowHeld = arrow;
        if (ArrowClick.Step(arrow != 0, now) && Model.Window(CurrentPage).Scroll(arrow))
        {
            NavigationSound(Audio, nullptr, 0xC3, 0);
            *reinterpret_cast<int32_t*>(manager + 0x10C) = 0;
            FillPage(manager);
        }
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
            float(*reinterpret_cast<int32_t*>(manager + 0x64)), float(*reinterpret_cast<int32_t*>(manager + 0x68)));
        bool listWheel = bReady && State == Mode::Page && Screen(manager) == ScreenOptions && !overValue;
        Wheel.Begin(listWheel);
        // Every notch the mouse reported since the last frame scrolls one row (the game's flag would show one per frame)
        int notches = listWheel ? WheelNotches.Take(std::exchange(WheelDelta, 0)) : (WheelNotches.Clear(), WheelDelta = 0);
        bool scrolled = false;
        for (; notches; notches += notches > 0 ? -1 : 1)
            scrolled |= Model.Window(CurrentPage).Scroll(notches > 0 ? -1 : 1);
        if (scrolled)
        {
            // Like the game's lists: the highlight sound, the highlight starts on its row again
            NavigationSound(Audio, nullptr, 0xC3, 0);
            *reinterpret_cast<int32_t*>(manager + 0x10C) = 0;
            FillPage(manager);
        }
        reinterpret_cast<void(__fastcall*)(uint8_t*, void*)>(UserInputPrevious)(manager, edx);
        Wheel.End();
        if (bReady)
        {
            FollowHover(manager);
            Update(manager);
        }
    }

    // Escape (and the game's other ways back) on the fix's pages: back to their list, or to the Options screen
    void* ProcessUserInputPrevious = nullptr;
    int32_t OnOffDirection = 0;
    void __fastcall ProcessUserInput(uint8_t* manager, void* edx, int32_t down, int32_t up, int32_t accept, int32_t back, int32_t change)
    {
        if (bReady && State != Mode::None && Screen(manager) == ScreenOptions && int8_t(back & 0xFF))
        {
            Back(manager);
            back = 0;
        }
        if (bReady && State == Mode::Page && Screen(manager) == ScreenOptions)
        {
            if (uint8_t(down) || uint8_t(up))
            {
                MoveSelection(manager, uint8_t(down) ? 1 : -1, false, false);
                down = up = 0;
            }
            // A click on the scroll bar scrolls (ExtraInput) and doesn't choose the row at its height
            if (uint8_t(accept) && IndicatorUnderMouse(manager))
                accept = 0;
        }
        const auto previousDirection = std::exchange(OnOffDirection, change);
        reinterpret_cast<void(__fastcall*)(uint8_t*, void*, int32_t, int32_t, int32_t, int32_t, int32_t)>(ProcessUserInputPrevious)(manager, edx, down, up, accept, back, change);
        OnOffDirection = previousDirection;
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

    // The outer input hook receives the direction as a native argument.
    uint32_t __fastcall OnOffChange(uint8_t* manager, void* edx)
    {
        if (ProcessOnOff(manager, OnOffDirection))
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

    // CFont::PrintString of the label line in CMenuManager::Draw: the game wraps it at its right margin, and a second
    // line would cover the first row. A narrow menu or a long translation gets a smaller font that fits one line.
    void __cdecl LabelPrint(float x, float y, const wchar_t* text)
    {
        auto restart = LabelShown && State == Mode::Page ? Answer("WFPWARN") : nullptr;
        if (FontDetails && restart && text && wcscmp(text, restart) == 0)
        {
            auto& scaleX = *reinterpret_cast<float*>(FontDetails + 0x4);
            auto& scaleY = *reinterpret_cast<float*>(FontDetails + 0x8);
            const float wrapX = *reinterpret_cast<float*>(FontDetails + 0x2C);
            const float width = ValueStringWidth(text, true);
            const float room = (wrapX - x) * 0.98f;
            if (width > room && room > 0.0f)
            {
                const float savedX = scaleX, savedY = scaleY;
                const float factor = room / width;
                scaleX *= factor;
                scaleY *= factor;
                LabelPrintPrevious(x, y, text);
                scaleX = savedX;
                scaleY = savedY;
                return;
            }
        }
        LabelPrintPrevious(x, y, text);
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
    void RememberResolutionCursor()
    {
        ResolutionCursorHeld = GetCursorPos(&ResolutionCursor) != FALSE;
    }

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
    }

    // The scroll bar of a list longer than its window, drawn like the skin list's: the arrows (pressed while held), the
    // track, and the bar for the rows shown with its shadow, fading in with the menu
    void DrawScrollIndicators(uint8_t* manager)
    {
        BarHit = 0;
        if (!bReady || State != Mode::Page || !DrawSprite || !manager)
            return;
        auto& window = Model.Window(CurrentPage);
        if (!window.Above() && !window.Below())
            return;
        const float scale = SCREEN_HEIGHT / 480.0f;
        const uint8_t alpha = uint8_t(std::clamp(*reinterpret_cast<int32_t*>(manager + FadeAlpha), 0, 255));
        // x from the right edge, y from the top
        auto rect = [&](float right, float top, float left, float bottom, const uint8_t* colour)
        {
            const float r[] = { SCREEN_WIDTH - right * scale, bottom * scale, SCREEN_WIDTH - left * scale, top * scale };
            DrawRect(r, colour);
        };
        auto sprite = [&](int index, float right, float top)
        {
            const float r[] = { SCREEN_WIDTH - right * scale, (top + 64.0f) * scale, SCREEN_WIDTH - (right - 64.0f) * scale, top * scale };
            const uint8_t white[] = { 255, 255, 255, alpha };
            DrawSprite(manager + MenuSprites + index * 4, nullptr, r, white);
        };
        const float top = IndicatorTop(), bottom = IndicatorBottom();
        // The mouse over the bar, in the units the menu is drawn in now
        const float mouseX = (SCREEN_WIDTH - float(*reinterpret_cast<int32_t*>(manager + MousePos))) / scale;
        const float mouseY = float(*reinterpret_cast<int32_t*>(manager + MousePos + 4)) / scale;
        if (mouseX <= BarRight + 2.0f && mouseX >= BarRight - BarWidth - 2.0f && mouseY >= top - 2.0f && mouseY <= bottom + 2.0f)
            BarHit = mouseY <= top + ArrowHeight ? -1 : mouseY >= bottom - ArrowHeight ? 1 : 2;
        const uint8_t track[] = { 100, 100, 66, uint8_t(alpha * 205 / 255) };
        const uint8_t shadow[] = { 50, 50, 50, alpha };
        const uint8_t bar[] = { 255, 150, 225, alpha };
        rect(BarRight + 1.0f, top, BarRight + 1.0f - BarWidth, bottom, track);
        const float bodyTop = top + ArrowHeight, bodyBottom = bottom - ArrowHeight;
        const float count = float(std::max<size_t>(1, window.Count()));
        const float barTop = bodyTop + (bodyBottom - bodyTop) * float(window.First()) / count;
        const float barBottom = bodyTop + (bodyBottom - bodyTop) * float(window.First() + window.Shown()) / count;
        rect(BarRight, barTop, BarRight - BarWidth + 3.0f, barBottom + 1.0f, shadow);
        rect(BarRight, barTop, BarRight - BarWidth + 4.0f, barBottom, bar);
        sprite(ArrowHeld < 0 ? SpriteUpOn : SpriteUpOff, BarRight, top);
        sprite(ArrowHeld > 0 ? SpriteDownOn : SpriteDownOff, BarRight, bottom - ArrowHeight);
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
            // Menu.ixx may have hooked the code right after the call already
            auto rowText = find_pattern("B9 ? ? ? ? E8 ? ? ? ? 89 44 24 ? 80 7D", "B9 ? ? ? ? E8 ? ? ? ? 89 44 24 ? E9 ? ? ? ? ? 8B B5");
            auto userInput = hook::pattern("80 7B 38 00 74 ? 89 D9 E8");
            UserInputPrevious = injector::MakeCALL(userInput.get_first(8), UserInput, true).get();
            auto processUserInput = hook::pattern("89 E9 FF 74 24 ? FF 74 24 ? FF 74 24 ? FF 74 24 ? FF 74 24 ? E8");
            auto accept = hook::pattern("89 E9 E8 ? ? ? ? 80 BC 24");
            auto change = hook::pattern("50 E8 ? ? ? ? 89 E9 E8 ? ? ? ? 83 BD");

            aScreens = reinterpret_cast<CMenuScreen*>(*hook::pattern("8B 93 ? ? ? ? 8B 4B 30 6B D2 71 8D 0C C9 01 CA 0F BF 04 55").get_first<uint8_t*>(21) - offsetof(CMenuScreen, entries));
            pLanguage.SetAddress(*hook::pattern("A1 ? ? ? ? 83 C4 14 83 F8 03 74 0A 83 F8 01").get_first<int32_t*>(1));

            // The change call site is the one that reaches the same function as the accept one
            auto onOff = injector::GetBranchDestination(accept.get_first(2)).as_int();
            void* changeCall = nullptr;
            for (size_t i = 0; i < change.size(); ++i)
            {
                if (injector::GetBranchDestination(change.get(i).get<void>(8)).as_int() == onOff)
                    changeCall = change.get(i).get<void>(8);
            }
            if (!AddOptionsEntry())
                return;

            HookTextCalls(rowText.get_first<uint8_t>(5));
            ProcessUserInputPrevious = injector::MakeCALL(processUserInput.get_first(22), ProcessUserInput, true).get();
            OnOffAcceptPrevious = injector::MakeCALL(accept.get_first(2), OnOffAccept, true).get();
            OnOffChangePrevious = injector::MakeCALL(changeCall, OnOffChange, true).get();

            ValueStringWidth = reinterpret_cast<decltype(ValueStringWidth)>(hook::pattern("D9 05 ? ? ? ? 53 56 83").get_first());
            auto valuePrint = hook::pattern("53 50 D9 14 24 50 D9 C1 D9 1C 24 DE D9 E8");
            ValuePrintPrevious = reinterpret_cast<ValuePrintFn>(injector::MakeCALL(valuePrint.get_first(13), ValuePrint, true).as_int());
            auto wheelButtons = hook::pattern("80 3D ? ? ? ? 00 59 75 11 6A 00 E8 ? ? ? ? 80 3D ? ? ? ? 00 59");
            for (size_t i = 0; i < wheelButtons.size(); ++i)
            {
                auto match = wheelButtons.get(i);
                auto up = *match.get<uint8_t*>(2);
                if (*match.get<uint8_t*>(19) == up + 1)
                    Wheel.buttons = up;
            }
            // CSprite2d::Draw(const CRect&, const CRGBA&), for the scroll bar's arrows
            DrawSprite = reinterpret_cast<decltype(DrawSprite)>(injector::GetBranchDestination(hook::pattern("E8 ? ? ? ? 83 BB ? ? ? ? ? 0F 85 ? ? ? ? 8B 83").get_first()).as_int());
            DrawRect = reinterpret_cast<decltype(DrawRect)>(find_pattern("8B 44 24 04 53 8B 5C 24 0C 53 53 53 53 50 E8", "E9 ? ? ? ? 8B 5C 24 0C 53 53 53 53 50 E8").get_first());

            auto details = hook::pattern("80 3D ? ? ? ? ? 75 ? 80 3D ? ? ? ? ? 75 ? 80 3D ? ? ? ? ? 0F 84");

            // The label line's print (CMenuManager::Draw: ... call CFont::PrintString; mov eax, RsGlobal.width;
            // add esp, 0Ch; add eax, -10 for the next SetWrapx)
            FontDetails = *details.get_first<uint8_t*>(2);
            auto labelPrint = hook::pattern("D9 C1 D9 1C 24 DE D9 E8 ? ? ? ? A1 ? ? ? ? 83 C4 0C 83 C0 F6");
            LabelPrintPrevious = reinterpret_cast<ValuePrintFn>(injector::MakeCALL(labelPrint.get_first(7), LabelPrint, true).as_int());

            auto navigationSound = hook::pattern("B9 ? ? ? ? 6A 00 68 C3 00 00 00 E8 ? ? ? ? FF 45 30");
            Audio = *navigationSound.get_first<void*>(1);
            NavigationSound = reinterpret_cast<decltype(NavigationSound)>(injector::GetBranchDestination(navigationSound.get_first(12)).as_int());

            // CPad::UpdateMouse: the DirectInput state's wheel movement (eax), before it becomes a flag
            static auto WheelRead = safetyhook::create_mid(hook::pattern("8B 44 24 ? 85 C0 7E ? C6 05 ? ? ? ? 01").get_first(4), [](SafetyHookContext& regs)
            {
                if (State == Mode::Page)
                    WheelDelta += int32_t(regs.eax);
            });

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
