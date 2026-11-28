module;

#include <stdafx.h>
#include <d3d9.h>
#include "FixSettings.hpp"
#include <algorithm>
#include <array>
#include <cctype>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>
#include "Shared/RWGTA/FixMenu.hpp"
#include "Shared/MenuList.hpp"
#include <atomic>

export module Frontend;


// The settings of the fix and of the other plugins with an ASI Settings description in Cold Fear's own menu: one
// "Plugins" row in Options. The game has no options in its pause menu (the inventory), so the
// main menu is the only place. The Screen Configuration page is a fixed grid the code fills (codes 200 to 233), with no
// room for more rows, so all of the fix's settings are on its own page.
//
// The front end reads its pages from Conf\MenuPC.cfg: [Page] and [Page.ItemN] sections with TYPE, TEXT (a key of
// menu.lng), DEST (a page name), RETURN_FLOAT (an ID the code asks for) and the layout. There are 14 pages, whose names
// the exe keeps in a table; the PC version starts on Language_US and never shows Language_EUROPE, so the fix takes that
// slot, names it WFPOptions and adds the page and the Options row to the cfg text as the parser reads it.
//
// - The page first lists the plugins (the fix first), even when it is alone; Enter opens one. A plugin's page: its
//   Preset row, its settings or a row per category opening that category, Restore defaults (RWGTAMenu::Model, menu
//   layout). Every list scrolls through the page's rows a row at a time: moving past the first or last row shown, or
//   the wheel. Back returns to the list the page was opened from, on the row it came from.
// - Rows show "Label: Value"; left and right step through the values, Enter steps forward. The help of the row
//   selected shows below the rows.
// - The rows have no text key: an item whose key is unknown draws the text the code puts into it, which the fix
//   writes every frame. Their width too, since the page's setup sizes the items after their keys' texts.
export namespace ColdFearMenu
{
    constexpr int PageSlot = 1;             // Language_EUROPE, never shown on PC
    constexpr int OptionPage = 6;           // Option
    constexpr const char* PageName = "WFPOptions";
    constexpr float EntryCode = 7000.0f;    // the Plugins row of Options
    constexpr float SwitchCode = 7001.0f;   // the line above the rows: the help pages' title, the mark of rows above
    constexpr float FirstRowCode = 7002.0f;
    constexpr float BackCode = 7020.0f;
    constexpr float NoticeCode = 7030.0f;
    constexpr float DescriptionCode = 17.0f;    // the game's help line
    constexpr int RowCount = 6;
    constexpr int BelowItem = RowCount + 2;         // the line below the rows: the mark of rows below
    constexpr int DescriptionItem = RowCount + 3;   // the help lines' places on the fix's page
    constexpr size_t DescriptionLines = 3;
    constexpr size_t DescriptionLength = 78;        // the game draws its small text in pieces of 80 letters
    constexpr int RowWidth = 470;

    // A page of the menu (the code's "this") and its items
    namespace PageField
    {
        constexpr ptrdiff_t Count = 8, Items = 12, Selected = 50572, Index = 50580;
    }
    namespace ItemField
    {
        constexpr ptrdiff_t Size = 1264, Selectable = 24, Visible = 25, Text = 92, Width = 636, Code = 1248;
        constexpr size_t TextLength = 250;  // wchar_t, the item's own text ends before its layout
    }

    struct State
    {
        HMODULE host = nullptr;
        RWGTAMenu::Model model;
        std::vector<const RWGTAMenu::Row*> rows; // the rows in the page's window, slot by slot
        size_t view = RWGTAMenu::Model::RootPage; // the model's page shown
        int lastSelected = -1;              // the item selected in the last frame
        bool below = false;                 // rows below the window: the mark under the rows
        std::atomic<int> wheelDelta{ 0 };
        MenuList::WheelNotches wheel;
        bool open = false;                  // the page is shown; it's set up again when it's opened the next time
        bool frontend = true;
        HWND window = nullptr;
        bool details = false;
        bool helpKeyDown = false;
        int helpSelection = 0;
        size_t helpPage = 0;
        std::wstring helpTitle;
        std::vector<std::wstring> helpLines;
        std::string notice;
        std::array<std::wstring, DescriptionLines> description; // the help below the rows, written again after the game's frame
        bool startSkipped = false;
        void (*skipStart)(uint8_t*) = nullptr;
        std::string cfg;                    // MenuPC.cfg with the fix's page, while the parser reads it
        void(__fastcall* playSound)(void* sound, void* edx, int32_t id) = nullptr;
        void* sound = nullptr;
        uint8_t* language = nullptr;        // dword_876918: the texts of menu.lng and their character map
    };

    inline State& Get()
    {
        static State state;
        return state;
    }

    // ---------------------------------------------------------------------------------------------------------------
    // Text

    inline std::string Language()
    {
        auto language = FixSettings().String("MAIN.TextLanguage");
        return language.empty() || language == "auto" ? "en" : language;
    }

    // The fix's own texts, also while the page isn't open (the row of Options)
    inline std::string HostText(std::string_view key)
    {
        auto& s = Get();
        if (s.model.IsOpen())
            return s.model.Text(std::string(key));
        static asi_settings::Texts texts;
        static std::string loadedLanguage;
        auto language = Language();
        if (loadedLanguage != language)
        {
            loadedLanguage = language;
            for (auto& plugin : asi_settings::FindLoadedPlugins(s.host, language))
            {
                if (plugin.module == s.host)
                    texts = plugin.file.texts;
            }
        }
        return texts.Resolve("@" + std::string(key));
    }

    inline std::wstring Wide(std::string_view text)
    {
        if (text.empty())
            return {};
        int length = MultiByteToWideChar(CP_UTF8, 0, text.data(), int(text.size()), nullptr, 0);
        std::wstring wide(size_t(std::max(length, 0)), L'\0');
        if (length > 0)
            MultiByteToWideChar(CP_UTF8, 0, text.data(), int(text.size()), wide.data(), length);
        return wide;
    }

    // Upper case like the game's titles (towupper follows the accented letters of the European languages too)
    inline std::wstring Upper(std::wstring text)
    {
        for (auto& c : text)
            c = wchar_t(towupper(c));
        return text;
    }

    // ---------------------------------------------------------------------------------------------------------------
    // Pages and rows

    // The plugins with settings, the fix first: each time the page is opened, since Ultimate ASI Loader loads them one
    // after another and some only after the fix's init
    inline bool Build()
    {
        auto& s = Get();
        s.model.OpenMenu(s.host, Language(), RowCount);
        s.view = RWGTAMenu::Model::RootPage;
        s.model.Window(s.view).Reset();
        s.lastSelected = -1;
        s.wheel.Clear();
        s.wheelDelta = 0;
        return !s.model.Pages().empty();
    }

    inline bool List()
    {
        return Get().view == RWGTAMenu::Model::RootPage;
    }

    // The row's line: its label and its value
    inline std::string RowText(const RWGTAMenu::Row& row)
    {
        auto& s = Get();
        auto label = s.model.Label(row);
        auto value = s.model.Value(row);
        return value.empty() ? label : label + ": " + value;
    }

    // The help line of the row selected; the restart notice after a change that applies at the next start
    inline std::string RowHelp(const RWGTAMenu::Row& row)
    {
        auto& s = Get();
        if (s.model.RestartRequired())
        {
            bool restartRow = row.kind != RWGTAMenu::RowKind::Setting || (row.setting && row.setting->apply == "restart");
            if (restartRow && row.kind != RWGTAMenu::RowKind::Action)
                return s.model.Text("menu.restartwarning");
        }
        return s.model.Help(row);
    }

    // ---------------------------------------------------------------------------------------------------------------
    // The game's menu

    inline uint8_t* ItemAt(uint8_t* page, int index)
    {
        return page + PageField::Items + index * ItemField::Size;
    }

    inline int FindItem(uint8_t* page, float code)
    {
        int count = *(int32_t*)(page + PageField::Count);
        for (int i = 0; i < count; ++i)
        {
            if (*(float*)(ItemAt(page, i) + ItemField::Code) == code)
                return i;
        }
        return -1;
    }

    // The menu's fonts draw the codes of menu.lng, not Unicode: the file's character map (pairs of a code and its
    // character, at +12 with their count at +8) turns the text into them. Characters the fonts don't have become the
    // plain letter, or '?'.
    inline wchar_t ToFont(wchar_t c)
    {
        auto& s = Get();
        if (!s.language)
            return c;
        auto count = *(int32_t*)(s.language + 8);
        auto map = *(uint16_t**)(s.language + 12);
        for (int32_t i = 0; map && i < count; ++i)
        {
            if (map[i * 4 + 2] == c)
                return wchar_t(map[i * 4]);
        }
        return 0;
    }

    inline wchar_t Plain(wchar_t c)
    {
        switch (c)
        {
            case L'\x2013': case L'\x2014': return L'-';
            case L'\x2018': case L'\x2019': return L'\'';
            case L'\x201C': case L'\x201D': return L'"';
            case L'\xB0': return L'o';
            default: break;
        }
        // The letter without its accent
        wchar_t decomposed[4] = {};
        if (FoldStringW(MAP_COMPOSITE, &c, 1, decomposed, 4) > 0 && decomposed[0] < 0x80)
            return decomposed[0];
        return L'?';
    }

    inline void SetText(uint8_t* item, const std::wstring& text)
    {
        auto target = (wchar_t*)(item + ItemField::Text);
        size_t length = 0;
        for (auto c : text)
        {
            if (length + 1 >= ItemField::TextLength)
                break;
            if (auto code = ToFont(c))
                target[length++] = code;
            else if (auto plain = ToFont(Plain(c)))
                target[length++] = plain;
        }
        target[length] = 0;
    }

    inline void Sound(int32_t id)
    {
        auto& s = Get();
        if (s.playSound && s.sound)
            s.playSound(s.sound, nullptr, id);
    }

    // The row index (0 to RowCount - 1) an item stands for, or -1
    inline int RowIndex(uint8_t* item)
    {
        int index = int(*(float*)(item + ItemField::Code) - FirstRowCode);
        return *(float*)(item + ItemField::Code) >= FirstRowCode && index < RowCount ? index : -1;
    }

    inline void Select(uint8_t* page, float code)
    {
        if (int item = FindItem(page, code); item >= 0)
            *(int32_t*)(page + PageField::Selected) = item;
    }

    inline void Close()
    {
        auto& s = Get();
        s.open = false;
        s.details = false;
        s.helpLines.clear();
        s.helpKeyDown = false;
        s.rows.clear();
        s.model.Close();
    }

    // The help below the rows in lines of the game's small text, broken at words; a longer help ends with "..." on the
    // last line and F1 shows the whole of it
    inline std::array<std::wstring, DescriptionLines> Short(std::wstring text)
    {
        std::array<std::wstring, DescriptionLines> lines;
        for (auto& c : text)
        {
            if (c == L'\n' || c == L'\r')
                c = L' ';
        }
        for (size_t line = 0; line < DescriptionLines && !text.empty(); ++line)
        {
            if (text.size() <= DescriptionLength)
            {
                lines[line] = text;
                break;
            }
            const size_t room = line + 1 == DescriptionLines ? DescriptionLength - 3 : DescriptionLength;
            auto end = text.rfind(L' ', room);
            if (end == std::wstring::npos || end == 0)
                end = room;
            lines[line] = text.substr(0, end);
            text.erase(0, end);
            while (!text.empty() && text.front() == L' ')
                text.erase(text.begin());
            if (line + 1 == DescriptionLines && !text.empty())
            {
                auto& last = lines[line];
                while (!last.empty() && (last.back() == L',' || last.back() == L'.' || last.back() == L' '))
                    last.pop_back();
                last += L"...";
            }
        }
        return lines;
    }

    inline size_t HelpPages()
    {
        return std::max(size_t(1), (Get().helpLines.size() + RowCount - 1) / RowCount);
    }

    inline void OpenHelp(uint8_t* page)
    {
        auto& s = Get();
        const int selected = *(int32_t*)(page + PageField::Selected);
        if (s.details || selected < 0)
            return;
        const int row = RowIndex(ItemAt(page, selected));
        const bool notice = row < 0 || row >= int(s.rows.size()) || RWGTAMenu::Model::Opens(*s.rows[row]);
        if (notice && (!List() || s.notice.empty()))
            return;
        auto text = Wide(notice ? s.notice : s.model.Help(*s.rows[row]));
        if (text.empty())
            return;
        s.helpLines.clear();
        while (!text.empty())
        {
            size_t end = std::min(size_t(48), text.size());
            const auto newline = text.find(L'\n');
            if (newline < end)
                end = newline;
            else if (end < text.size())
            {
                const auto space = text.rfind(L' ', end);
                if (space != std::wstring::npos && space > 0)
                    end = space;
            }
            s.helpLines.push_back(text.substr(0, end));
            text.erase(0, end);
            while (!text.empty() && (text.front() == L' ' || text.front() == L'\n' || text.front() == L'\r'))
                text.erase(text.begin());
        }
        s.helpTitle = Wide(notice ? HostText("menu.plugins") : s.model.Label(*s.rows[row]));
        s.helpTitle.resize(std::min(size_t(48), s.helpTitle.size()));
        s.helpSelection = selected;
        s.helpPage = 0;
        s.details = true;
        Select(page, HelpPages() > 1 ? SwitchCode : BackCode);
        Sound(2);
    }

    inline void UpdateHelp(uint8_t* page)
    {
        auto& s = Get();
        SetText(ItemAt(page, 0), Upper(Wide(HostText("menu.details"))));
        if (const int index = FindItem(page, SwitchCode); index >= 0)
        {
            auto item = ItemAt(page, index);
            auto title = s.helpTitle + L"  " + std::to_wstring(s.helpPage + 1) + L"/" + std::to_wstring(HelpPages());
            SetText(item, title);
            *(uint8_t*)(item + ItemField::Visible) = 1;
            *(uint8_t*)(item + ItemField::Selectable) = HelpPages() > 1;
            *(int32_t*)(item + ItemField::Width) = RowWidth;
        }
        for (int row = 0; row < RowCount; ++row)
        {
            const int index = FindItem(page, FirstRowCode + float(row));
            if (index < 0)
                continue;
            auto item = ItemAt(page, index);
            const size_t line = s.helpPage * RowCount + row;
            const bool visible = line < s.helpLines.size();
            SetText(item, visible ? s.helpLines[line] : L"");
            *(uint8_t*)(item + ItemField::Visible) = visible;
            *(uint8_t*)(item + ItemField::Selectable) = 0;
            *(int32_t*)(item + ItemField::Width) = visible ? RowWidth : 0;
        }

        s.description = Short(Wide(HostText("menu.detailsnav")));
    }

    // Every frame of a page of the front end, before the game draws it
    inline void Update(uint8_t* page)
    {
        auto& s = Get();
        int index = *(int32_t*)(page + PageField::Index);
        if (index == 3 && FixSettings().Bool("MAIN.SkipPressStart") && !s.startSkipped && s.skipStart)
        {
            s.startSkipped = true;
            s.skipStart(page);
        }
        else if (index != 3)
            s.startSkipped = false;
        if (index != PageSlot)
        {
            if (s.open)
                Close();
            if (index == OptionPage)
            {
                if (int entry = FindItem(page, EntryCode); entry >= 0)
                    SetText(ItemAt(page, entry), Wide(HostText("menu.plugins")));
            }
            if (index == 4)
            {
                if (const int entry = FindItem(page, NoticeCode); entry >= 0)
                {
                    auto item = ItemAt(page, entry);
                    SetText(item, Wide(s.notice));
                    *(uint8_t*)(item + ItemField::Visible) = !s.notice.empty();
                    *(int32_t*)(item + ItemField::Width) = RowWidth;
                }
            }
            return;
        }
        if (!s.open)
        {
            s.open = true;
            if (!Build())
                return;
            Select(page, FirstRowCode);
        }
        auto& pages = s.model.Pages();
        if (pages.empty())
            return;

        const bool helpDown = s.window && GetForegroundWindow() == s.window && (GetAsyncKeyState(VK_F1) & 0x8000);
        if (helpDown && !s.helpKeyDown)
            OpenHelp(page);
        s.helpKeyDown = helpDown;
        if (s.details)
        {
            UpdateHelp(page);
            return;
        }

        // The title: the page's name (the plugins list, a plugin, a category), upper case like the game's titles
        const auto& title = s.view == RWGTAMenu::Model::RootPage ? HostText("menu.plugins") : pages[s.view].title;
        SetText(ItemAt(page, 0), Upper(Wide(title)));
        *(int32_t*)(ItemAt(page, 0) + ItemField::Width) = RowWidth;

        auto& window = s.model.Window(s.view);
        s.model.Shown(s.view, false);
        auto& selected = *(int32_t*)(page + PageField::Selected);
        const auto slotOf = [&](int item) { return item >= 0 ? RowIndex(ItemAt(page, item)) : -1; };
        const auto backItem = FindItem(page, BackCode);
        // Moving past the first or last row shown scrolls the list instead, like the game's longer lists; from Back
        // the list wraps to its ends
        const int previous = slotOf(s.lastSelected), current = slotOf(selected);
        const int shownRows = int(window.Shown());
        if (s.lastSelected >= 0 && selected != s.lastSelected)
        {
            if (previous == 0 && current != 1 && window.Above())
            {
                window.Move(-1);
                Select(page, FirstRowCode);
            }
            else if (previous == shownRows - 1 && current < 0 && window.Below())
            {
                window.Move(1);
                Select(page, FirstRowCode + float(shownRows - 1));
            }
            else if (s.lastSelected == backItem && current == 0 && window.Above())
            {
                window.Edge(-1);
                Select(page, FirstRowCode);
            }
            else if (s.lastSelected == backItem && current >= 0 && window.Below())
            {
                window.Edge(1);
                Select(page, FirstRowCode + float(window.Slot()));
            }
            else if (current >= 0)
                window.Point(size_t(current));
        }
        else if (current >= 0 && size_t(current) != window.Slot())
            window.Point(size_t(current));
        // The wheel moves the list a row per notch; the selection stays on the rows shown
        bool scrolled = false;
        for (int notches = s.wheel.Take(s.wheelDelta.exchange(0)); notches; notches += notches > 0 ? -1 : 1)
            scrolled |= window.Scroll(notches > 0 ? -1 : 1);
        if (scrolled)
        {
            Sound(0);
            if (slotOf(selected) >= 0)
                Select(page, FirstRowCode + float(window.Slot()));
        }
        s.lastSelected = selected;

        s.rows = s.model.Shown(s.view, false);
        const int used = int(s.rows.size());
        // Rows above or below the window: a small mark above or below the rows
        const auto mark = [&](float code, bool shown, const wchar_t* text)
        {
            if (int item = FindItem(page, code); item >= 0)
            {
                auto pItem = ItemAt(page, item);
                SetText(pItem, shown ? text : L"");
                *(int32_t*)(pItem + ItemField::Width) = shown ? RowWidth : 0;
                *(uint8_t*)(pItem + ItemField::Selectable) = 0;
                *(uint8_t*)(pItem + ItemField::Visible) = shown;
            }
        };
        mark(SwitchCode, window.Above(), L"...");
        s.below = window.Below();
        for (int i = 0; i < RowCount; ++i)
        {
            int item = FindItem(page, FirstRowCode + float(i));
            if (item < 0)
                continue;
            auto pItem = ItemAt(page, item);
            bool shown = i < used;
            SetText(pItem, Wide(shown ? RowText(*s.rows[i]) : std::string()));
            *(int32_t*)(pItem + ItemField::Width) = shown ? RowWidth : 0;
            *(uint8_t*)(pItem + ItemField::Selectable) = shown;
            *(uint8_t*)(pItem + ItemField::Visible) = shown;
        }

        // A row that isn't there anymore (a shorter page): the first row there is
        if (selected >= 0 && !*(uint8_t*)(ItemAt(page, selected) + ItemField::Selectable))
            Select(page, used > 0 ? FirstRowCode : BackCode);

        std::string description;
        if (selected >= 0)
        {
            auto pSelected = ItemAt(page, selected);
            int row = RowIndex(pSelected);
            if (List() && row >= 0)
                description = s.notice.empty() ? HostText("WFP_PLUGINS_DESCRIPTION") : s.notice;
            else if (row >= 0 && row < int(s.rows.size()) && !RWGTAMenu::Model::Opens(*s.rows[row]))
                description = RowHelp(*s.rows[row]);
        }
        s.description = Short(Wide(description));
    }

    // After the game's frame of the page: its frame empties the help line (it only describes the Screen Configuration's
    // rows), so the fix's goes in afterwards. The line is found by its place on the page (on the retail exe its
    // RETURN_FLOAT doesn't stay).
    inline void Describe(uint8_t* page)
    {
        auto& s = Get();
        if (*(int32_t*)(page + PageField::Index) != PageSlot || !s.open || s.model.Pages().empty() ||
            *(int32_t*)(page + PageField::Count) <= DescriptionItem + int(DescriptionLines) - 1)
            return;
        // The mark of rows below the window, a line like the help line (found by its place too)
        {
            auto mark = ItemAt(page, BelowItem);
            const bool shown = s.below && !s.details;
            SetText(mark, shown ? L"..." : L"");
            *(int32_t*)(mark + ItemField::Width) = shown ? 40 : 0;
            *(uint8_t*)(mark + ItemField::Selectable) = 0;
            *(uint8_t*)(mark + ItemField::Visible) = shown;
        }
        for (size_t line = 0; line < s.description.size(); ++line)
        {
            auto item = ItemAt(page, DescriptionItem + int(line));
            SetText(item, s.description[line]);
            *(uint8_t*)(item + ItemField::Visible) = !s.description[line].empty();
            *(int32_t*)(item + ItemField::Width) = RowWidth;
        }
    }

    // Enter (direction 0), left (-1) or right (1) on the item selected; false: not the fix's, the game handles it
    inline bool Use(uint8_t* page, int direction)
    {
        auto& s = Get();
        if (*(int32_t*)(page + PageField::Index) != PageSlot || !s.open || s.model.Pages().empty())
            return false;
        int selected = *(int32_t*)(page + PageField::Selected);
        if (selected < 0)
            return false;
        auto item = ItemAt(page, selected);
        float code = *(float*)(item + ItemField::Code);

        if (s.details)
        {
            if (code == BackCode && direction == 0)
            {
                s.details = false;
                *(int32_t*)(page + PageField::Selected) = s.helpSelection;
                Sound(1);
            }
            else if (code == SwitchCode)
            {
                const size_t count = HelpPages();
                s.helpPage = (s.helpPage + (direction < 0 ? count - 1 : 1)) % count;
                Sound(0);
            }
            return true;
        }

        // Back from a page to the list it was opened from, on the row it came from; from the plugins list to Options
        if (code == BackCode)
        {
            if (direction != 0)
                return false;
            const auto parent = s.model.Back(s.view);
            if (parent == SIZE_MAX)
                return false;
            s.view = parent;
            Select(page, FirstRowCode + float(s.model.Window(s.view).Slot()));
            s.lastSelected = -1;
            Sound(1);
            return true;
        }
        int row = RowIndex(item);
        if (row >= 0 && row < int(s.rows.size()) && RWGTAMenu::Model::Opens(*s.rows[row]))
        {
            if (direction != 0)
                return true;
            if (const auto target = s.model.Enter(*s.rows[row]); target != SIZE_MAX)
            {
                s.view = target;
                Select(page, FirstRowCode);
                s.lastSelected = -1;
                Sound(2);
            }
            return true;
        }
        if (row >= 0 && row < int(s.rows.size()))
        {
            s.model.Change(*s.rows[row], direction, false);
            Sound(direction == 0 ? 2 : 0);
            return true;
        }
        return false;
    }

    // The game's Back (Escape, the pad's back button) on a page: on the fix's page it closes the details or returns one
    // level like its Back row; false: the game goes back to the page's parent
    inline bool Cancel(uint8_t* page)
    {
        auto& s = Get();
        if (*(int32_t*)(page + PageField::Index) != PageSlot || !s.open || s.model.Pages().empty())
            return false;
        if (s.details)
        {
            s.details = false;
            *(int32_t*)(page + PageField::Selected) = s.helpSelection;
            Sound(1);
            return true;
        }
        const auto parent = s.model.Back(s.view);
        if (parent == SIZE_MAX)
            return false;
        s.view = parent;
        Select(page, FirstRowCode + float(s.model.Window(s.view).Slot()));
        s.lastSelected = -1;
        Sound(1);
        return true;
    }

    // ---------------------------------------------------------------------------------------------------------------
    // MenuPC.cfg

    // A number of the cfg: the value of key in the section, or fallback
    inline int ReadNumber(std::string_view text, std::string_view section, std::string_view key, int fallback)
    {
        auto start = text.find("[" + std::string(section) + "]");
        if (start == text.npos)
            return fallback;
        auto end = text.find("\n[", start + 1);
        auto body = text.substr(start, end == text.npos ? text.npos : end - start);
        for (size_t pos = 0; (pos = body.find(key, pos)) != body.npos; pos += key.size())
        {
            if (pos > 0 && body[pos - 1] != '\n' && body[pos - 1] != '\t' && body[pos - 1] != ' ')
                continue;
            auto eq = body.find('=', pos);
            auto eol = body.find('\n', pos);
            auto name = body.substr(pos, eq - pos);
            while (!name.empty() && (name.back() == ' ' || name.back() == '\t'))
                name.remove_suffix(1);
            if (eq == body.npos || (eol != body.npos && eq > eol) || name != key)
                continue;
            return std::atoi(std::string(body.substr(eq + 1, eol == body.npos ? body.npos : eol - eq - 1)).c_str());
        }
        return fallback;
    }

    // The cfg with the Options row and the fix's page; the text as it is if it isn't the menu's
    inline const char* AddPage(const char* text)
    {
        std::string_view original(text);
        if (original.find("[ScreenConfig]") == original.npos)
            return text;
        auto option = original.find("[Option]");
        if (option == original.npos)
            return text;

        // Options: one row more, below its last setting (Subtitles), in the column of the others
        auto& s = Get();
        int count = ReadNumber(original, "Option", "NUM_ITEM", 0);
        int height = ReadNumber(original, "Default Values", "ITEM_HEIGHT", 23);
        int spacing = ReadNumber(original, "Default Values", "ITEM_SPACING", 4);
        int first = ReadNumber(original, "Option.Item1", "Y", 160);
        if (count < 2 || count >= 39)
            return text;
        s.cfg.assign(original);
        auto end = s.cfg.find("\n[", option + 1);
        auto numItem = s.cfg.find("NUM_ITEM", option);
        if (numItem == s.cfg.npos || (end != s.cfg.npos && numItem > end))
            return text;
        auto eol = s.cfg.find_first_of("\r\n", numItem);
        s.cfg.replace(numItem, eol - numItem, "NUM_ITEM = " + std::to_string(count + 1));
        const auto main = s.cfg.find("\n[Main]");
        const auto mainCount = main == s.cfg.npos ? s.cfg.npos : s.cfg.find("NUM_ITEM", main);
        if (mainCount != s.cfg.npos)
        {
            const auto mainEnd = s.cfg.find_first_of("\r\n", mainCount);
            const int items = ReadNumber(original, "Main", "NUM_ITEM", 0);
            if (items == 7)
                s.cfg.replace(mainCount, mainEnd - mainCount, "NUM_ITEM = 8");
        }

        auto line = [](std::string key, std::string value) { return key + " = " + value + "\r\n"; };
        std::string added = "\r\n[Option.Item" + std::to_string(count) + "]\r\n";
        added += line("Y", std::to_string(first + (count - 3) * (height + spacing)));
        added += line("DEST", PageName);
        added += line("RETURN_FLOAT", "7000");
        added += "\r\n[Main.Item7]\r\n" + line("TYPE", "Static") + line("TEXT_SMALL", "true") +
            line("X", "84") + line("Y", "350") + line("WIDTH", "470") + line("HEIGHT", "50") +
            line("TEXT_ANIM_POS", "false") + line("TEXT_ANIM_ALPHA", "false") + line("RETURN_FLOAT", "7030");

        // The page: title, page row, the rows, the description, Back
        added += "\r\n[" + std::string(PageName) + "]\r\n";
        added += line("NUM_ITEM", std::to_string(RowCount + 4 + int(DescriptionLines)));
        added += line("ITEM_SELECT", "1");
        added += line("ITEM_SPACING", "2");
        auto item = [&](int index) { return "\r\n[" + std::string(PageName) + ".Item" + std::to_string(index) + "]\r\n"; };
        added += item(0) + line("TYPE", "TitlePage") + line("Y", "121");
        added += item(1) + line("Y", "150") + line("WIDTH", std::to_string(RowWidth)) + line("HEIGHT", "20") + line("TEXT_ANIM_POS", "false") + line("RETURN_FLOAT", "7001");
        for (int i = 0; i < RowCount; ++i)
        {
            added += item(2 + i);
            if (i == 0)
                added += line("Y", "178");
            added += line("WIDTH", std::to_string(RowWidth)) + line("HEIGHT", "20") + line("TEXT_ANIM_POS", "false") + line("RETURN_FLOAT", std::to_string(7002 + i));
        }
        added += item(RowCount + 2) + line("TYPE", "Static") + line("X", "520") + line("WIDTH", "40") + line("HEIGHT", "20") + line("TEXT_ANIM_POS", "false") + line("TEXT_ANIM_ALPHA", "false");
        // The help: lines of the game's small text between the rows and Back
        for (int i = 0; i < int(DescriptionLines); ++i)
            added += item(DescriptionItem + i) + line("TYPE", "Static") + line("TEXT_SMALL", "true") + line("X", "84") + line("Y", std::to_string(-112 + 13 * i)) + line("WIDTH", "470") + line("TEXT_ANIM_POS", "false") + line("TEXT_ANIM_ALPHA", "false") + line("RETURN_FLOAT", std::to_string(int(DescriptionCode)));
        added += item(DescriptionItem + int(DescriptionLines)) + line("X", "135") + line("Y", "-70") + line("TEXT_ANIM_POS", "false") + line("TEXT", "STR_MN_MAIN_SCREEN_BACK") + line("DEST", "Option") + line("RETURN_FLOAT", "7020");
        s.cfg += added;
        return s.cfg.c_str();
    }

    inline void Init(HMODULE host, void* playSound, void* sound, void* language)
    {
        auto& s = Get();
        s.language = (uint8_t*)language;
        s.host = host;
        s.playSound = (decltype(s.playSound))playSound;
        s.sound = sound;
    }
}
