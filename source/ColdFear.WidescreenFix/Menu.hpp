#pragma once
#include <algorithm>
#include <cctype>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>
#include "../Shared/RWGTA/FixMenu.hpp"

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
//   Preset row, its settings, Restore defaults (RWGTAMenu::Model, Plugins layout). A plugin with more rows than fit
//   continues on further parts, which the row on top switches (left, right). Back returns to the list.
// - Rows show "Label: Value"; left and right step through the values, Enter steps forward. The help of the row
//   selected shows below the rows.
// - The rows have no text key: an item whose key is unknown draws the text the code puts into it, which the fix
//   writes every frame. Their width too, since the page's setup sizes the items after their keys' texts.
namespace ColdFearMenu
{
    constexpr int PageSlot = 1;             // Language_EUROPE, never shown on PC
    constexpr int OptionPage = 6;           // Option
    constexpr const char* PageName = "WFPOptions";
    constexpr float EntryCode = 7000.0f;    // the Plugins row of Options
    constexpr float SwitchCode = 7001.0f;   // the part row
    constexpr float FirstRowCode = 7002.0f;
    constexpr float BackCode = 7020.0f;
    constexpr float DescriptionCode = 17.0f;
    constexpr int RowCount = 6;
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
        std::vector<size_t> entries;        // the first page of each plugin, in the list's order
        std::vector<const RWGTAMenu::Row*> rows; // the rows of the part shown
        size_t part = 0;                    // the model's page shown
        bool list = false;                  // the list of plugins is shown
        bool open = false;                  // the page is shown; it's set up again when it's opened the next time
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

    // The parts of the plugin of the model's page shown: first and count
    inline std::pair<size_t, size_t> Parts()
    {
        auto& s = Get();
        auto& pages = s.model.Pages();
        if (s.part >= pages.size())
            return { 0, 0 };
        auto plugin = pages[s.part].plugin;
        size_t first = s.part, last = s.part;
        while (first > 0 && pages[first - 1].plugin == plugin)
            --first;
        while (last + 1 < pages.size() && pages[last + 1].plugin == plugin)
            ++last;
        return { first, last - first + 1 };
    }

    // The plugins with settings, the fix first: each time the page is opened, since Ultimate ASI Loader loads them one
    // after another and some only after the fix's init
    inline bool Build()
    {
        auto& s = Get();
        s.model.OpenPlugins(s.host, Language(), RowCount);
        s.entries.clear();
        auto& pages = s.model.Pages();
        for (size_t i = 0; i < pages.size(); ++i)
        {
            if (i == 0 || pages[i].plugin != pages[i - 1].plugin)
                s.entries.push_back(i);
        }
        s.entries.resize(std::min(s.entries.size(), size_t(RowCount)));
        s.part = 0;
        s.list = true;
        return !pages.empty();
    }

    // The name of a plugin in the list
    inline std::string EntryText(size_t entry)
    {
        auto& s = Get();
        auto& page = s.model.Pages()[s.entries[entry]];
        return OtherPlugins::Name(*page.plugin);
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
        s.rows.clear();
        s.model.Close();
    }

    // Every frame of a page of the front end, before the game draws it
    inline void Update(uint8_t* page)
    {
        auto& s = Get();
        int index = *(int32_t*)(page + PageField::Index);
        if (index != PageSlot)
        {
            if (s.open)
                Close();
            if (index == OptionPage)
            {
                if (int entry = FindItem(page, EntryCode); entry >= 0)
                    SetText(ItemAt(page, entry), Wide(HostText("menu.plugins")));
            }
            return;
        }
        if (!s.open)
        {
            s.open = true;
            if (!Build())
                return;
            Select(page, s.list ? FirstRowCode : SwitchCode);
        }
        auto& pages = s.model.Pages();
        if (pages.empty())
            return;

        SetText(ItemAt(page, 0), Upper(Wide(HostText("menu.plugins"))));
        *(int32_t*)(ItemAt(page, 0) + ItemField::Width) = RowWidth;

        // The row on top: the plugin's name and its part (the fonts have no arrows)
        if (int item = FindItem(page, SwitchCode); item >= 0)
        {
            auto pItem = ItemAt(page, item);
            std::string text;
            if (!s.list)
            {
                auto [first, count] = Parts();
                text = OtherPlugins::Name(*pages[s.part].plugin);
                if (count > 1)
                    text += "  -  " + std::to_string(s.part - first + 1) + "/" + std::to_string(count);
            }
            SetText(pItem, Wide(text));
            *(int32_t*)(pItem + ItemField::Width) = s.list ? 0 : RowWidth;
            *(uint8_t*)(pItem + ItemField::Selectable) = !s.list;
            *(uint8_t*)(pItem + ItemField::Visible) = !s.list;
        }

        s.rows.clear();
        if (!s.list)
            s.rows = s.model.VisibleRows(s.part);
        int used = s.list ? int(s.entries.size()) : int(s.rows.size());
        for (int i = 0; i < RowCount; ++i)
        {
            int item = FindItem(page, FirstRowCode + float(i));
            if (item < 0)
                continue;
            auto pItem = ItemAt(page, item);
            bool shown = i < used;
            std::string text;
            if (shown)
                text = s.list ? EntryText(size_t(i)) : RowText(*s.rows[i]);
            SetText(pItem, Wide(text));
            *(int32_t*)(pItem + ItemField::Width) = shown ? RowWidth : 0;
            *(uint8_t*)(pItem + ItemField::Selectable) = shown;
            *(uint8_t*)(pItem + ItemField::Visible) = shown;
        }

        // A row that isn't there anymore (a shorter page): the first row there is
        auto& selected = *(int32_t*)(page + PageField::Selected);
        if (selected >= 0 && !*(uint8_t*)(ItemAt(page, selected) + ItemField::Selectable))
            Select(page, s.list || used > 0 ? FirstRowCode : BackCode);

        if (int item = FindItem(page, DescriptionCode); item >= 0)
        {
            std::string description;
            if (selected >= 0)
            {
                auto pSelected = ItemAt(page, selected);
                int row = RowIndex(pSelected);
                if (*(float*)(pSelected + ItemField::Code) == SwitchCode)
                    description = HostText("WFP_PAGE_DESCRIPTION");
                else if (s.list && row >= 0)
                    description = HostText("WFP_PLUGINS_DESCRIPTION");
                else if (row >= 0 && row < int(s.rows.size()))
                    description = RowHelp(*s.rows[row]);
            }
            SetText(ItemAt(page, item), Wide(description));
            *(int32_t*)(ItemAt(page, item) + ItemField::Width) = RowWidth;
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

        // Back from a plugin's page to the list
        if (code == BackCode)
        {
            if (direction != 0 || s.list || s.entries.empty())
                return false;
            s.list = true;
            Select(page, FirstRowCode);
            Sound(1);
            return true;
        }
        if (s.list)
        {
            int row = RowIndex(item);
            if (row < 0 || row >= int(s.entries.size()) || direction < 0)
                return false;
            s.list = false;
            s.part = s.entries[row];
            s.model.PageShown(s.part);
            Select(page, SwitchCode);
            Sound(2);
            return true;
        }
        if (code == SwitchCode)
        {
            auto [first, count] = Parts();
            if (count < 2)
                return true;
            size_t step = direction < 0 ? count - 1 : 1;
            s.part = first + (s.part - first + step) % count;
            s.model.PageShown(s.part);
            Sound(0);
            return true;
        }
        int row = RowIndex(item);
        if (row >= 0 && row < int(s.rows.size()))
        {
            s.model.Change(*s.rows[row], direction, false);
            Sound(direction == 0 ? 2 : 0);
            return true;
        }
        return false;
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

        auto line = [](std::string key, std::string value) { return key + " = " + value + "\r\n"; };
        std::string added = "\r\n[Option.Item" + std::to_string(count) + "]\r\n";
        added += line("Y", std::to_string(first + (count - 3) * (height + spacing)));
        added += line("DEST", PageName);
        added += line("RETURN_FLOAT", "7000");

        // The page: title, page row, the rows, the description, Back
        added += "\r\n[" + std::string(PageName) + "]\r\n";
        added += line("NUM_ITEM", std::to_string(RowCount + 4));
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
        added += item(RowCount + 2) + line("TYPE", "Static") + line("TEXT_SMALL", "true") + line("X", "84") + line("Y", "-100") + line("WIDTH", "470") + line("TEXT_ANIM_POS", "false") + line("TEXT_ANIM_ALPHA", "false") + line("RETURN_FLOAT", "17");
        added += item(RowCount + 3) + line("X", "135") + line("Y", "-70") + line("TEXT_ANIM_POS", "false") + line("TEXT", "STR_MN_MAIN_SCREEN_BACK") + line("DEST", "Option") + line("RETURN_FLOAT", "7020");
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
