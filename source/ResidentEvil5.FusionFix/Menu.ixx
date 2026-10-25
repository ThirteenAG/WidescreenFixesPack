module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "../Shared/NFS/PluginsMenu.hpp"

export module Menu;

// The fix's settings in the game's own Options menu, at the title screen and in the pause menu.
//
// Options gets one more row, PLUGINS, above DEFAULT. It opens a list page: the fix first, then every other loaded
// plugin with settings (ASI Settings menu host). Each plugin's page has its Preset row (when it has presets), its
// settings, its compatibility settings last and Restore defaults (that page only). Values change live through each
// plugin's runtime interface.
//
// The menu screens are units with a small state machine (byte +0x14): 0/1 fade in, 2 input, then fade out and the
// next screen. They draw every frame with the menu's text and layout functions. The pages are drawn on the System
// Settings screen: the PLUGINS row opens that screen as the game does for SYSTEM SETTINGS, and while it shows the
// plugins its input and drawing are replaced, so the pages get the game's bars, highlight, arrows, help line,
// button prompts and mouse areas. The game's raw text function knows ASCII only: accented letters lose their accent.

namespace
{
    using Page = uintptr_t;

    template<class T>
    T& Field(uintptr_t object, uint32_t offset)
    {
        return *reinterpret_cast<T*>(object + offset);
    }

    // Menu screen fields
    constexpr uint32_t State = 0x14;        // byte: state machine
    constexpr uint32_t Player = 0x34;       // player the menu belongs to
    constexpr uint32_t Blink = 0x3D;        // byte: accepted row blinks
    constexpr uint32_t Leaving = 0x3E;      // byte: leaving without a next screen
    constexpr uint32_t Timer = 0x44;        // float
    constexpr uint32_t Cursor = 0x48;
    constexpr uint32_t Scroll = 0x50;
    constexpr uint32_t FadeLevel = 0x78;    // 0..9
    constexpr uint32_t PadIndex = 0x188;
    constexpr uint32_t InGame = 0x18C;      // the pause menu's options
    constexpr uint32_t Popup = 0x190;       // top Options only: 1 restore defaults question, 2 benchmark

    constexpr int StateInput = 2;
    constexpr int StateAccepted = 3;
    constexpr int StateLeave = 0x5A;

    constexpr int TopRows = 8;              // the game's rows of the Options list
    constexpr int VisibleRows = 7;          // rows of the System Settings list
    constexpr int TopRowSpacing = 38;       // nine rows in the space of eight
    constexpr int TopFirstRow = 147;
    constexpr int TopMouseScreen = 140;     // mouse areas of the Options list at the title screen
    constexpr int LabelX = 380;
    constexpr int ValueX = 830;
    constexpr int FontSize = 36;
    constexpr int TextLayer = 96;
    constexpr uint32_t White = 0xFFFFFFFF;
    constexpr uint32_t Grey = 0xFFB2B2B2;

    namespace Game
    {
        // thiscall functions, called as fastcall with an unused edx
        int (__fastcall* Pressed)(Page, void*, int pad, int player);
        int (__fastcall* Repeated)(Page, void*, int pad, int player);
        void (__fastcall* Housekeep)(Page, void*);
        void (__fastcall* HideElements)(Page, void*);
        void (__fastcall* ResetLayout)(Page, void*, int, int);
        void (__fastcall* MouseScreen)(Page, void*, int menu, int screen, int layer);
        bool (__fastcall* BackPressed)(Page, void*);
        void (__fastcall* StopInput)(Page, void*);
        int (__fastcall* ScrollNav)(Page, void*, int pressed, int repeated, int cursor, int count, int* scroll, int visible);
        int (__fastcall* MouseRow)(Page, void*, int cursor, int count, int, int);
        int (__fastcall* Wheel)(uintptr_t, void*, int scroll, int min, int max, int, int, int);
        bool (__fastcall* MouseClick)(uintptr_t, void*, int area, int kind, int layer);
        bool (__fastcall* MouseClickRange)(uintptr_t, void*, int first, int last, int kind, int layer);
        int (__fastcall* MouseHover)(uintptr_t, void*, int layer);
        bool (__fastcall* KeyboardMenus)(uintptr_t, void*, int player);
        void (__fastcall* Title)(Page, void*, int message);
        void (__fastcall* Highlight)(Page, void*, int element, int message, bool selected, int y);
        void (__fastcall* HighlightScale)(Page, void*, const char* name, float scale);
        void (__fastcall* Prompt)(Page, void*, int prompt, int);
        void (__fastcall* PausePrompt)(Page, void*, int prompt);
        void (__fastcall* Arrows)(Page, void*, int element, int mask);
        void (__fastcall* ElementAlpha)(Page, void*, const char* name, int alpha);
        void (__fastcall* ElementShow)(Page, void*, const char* name);
        void (__fastcall* ElementMove)(Page, void*, const char* name, int x, int y);
        int (__fastcall* Fade)(Page, void*, int from, int to, int index);
        bool (__fastcall* FadedIn)(Page, void*, int index);
        bool (__fastcall* LayoutBusy)(uintptr_t, void*);
        void (__fastcall* DrawMessageAt)(Page, void*, int size, int x, int y, uint32_t* color, int message, int layer, int);
        void (__fastcall* DrawMessageCentered)(Page, void*, int size, int x, int y, uint32_t* color, int message, int layer);
        void (__fastcall* DrawRawText)(Page, void*, int size, int x, int yTop, uint32_t* color, const char* text, int layer);
        float (__fastcall* TextWidth)(uintptr_t, void*, int font, const char* text, float size);
        int (__cdecl* PauseFontSize)();
        void (__fastcall* SoundMove)(Page, void*);
        void (__fastcall* SoundValue)(Page, void*);
        void (__fastcall* SoundAccept)(Page, void*);
        void (__fastcall* SoundAcceptInGame)(Page, void*);
        void (__fastcall* SoundBack)(Page, void*);
        void (__fastcall* SoundBackInGame)(Page, void*);
        void (__fastcall* SoundError)(Page, void*);

        uintptr_t* Pad;         // +0x5B4 accept buttons, +0x614 player on keyboard/mouse
        uintptr_t* Mouse;       // +0x2E0/+0x2E4 cursor in 1280x720, +0x318 areas by screen
        uintptr_t* Fonts;
        uintptr_t* System;      // +0x10428 input locked, +0x10434 busy flags
        uintptr_t* Main;        // +0x2584 language
        uintptr_t* Session;
        uintptr_t* Layouts;
        uintptr_t* MouseDevices;   // the mouse states, next to the pad's

        int AcceptMask()
        {
            return Field<int>(*Pad, 0x5B4);
        }

        bool KeyboardPlayer(Page page)
        {
            return Field<int>(page, Player) == Field<int>(*Pad, 0x614);
        }

        // The language's English name for the fix's texts (the game's order: jpn, eng, ger, fra, spa, ita, ...)
        std::string Language()
        {
            static const char* names[] = { "japanese", "english", "german", "french", "spanish", "italian", "russian", "polish", "czech" };
            uint32_t index = Main && *Main ? Field<uint32_t>(*Main, 0x2584) : 1;
            return index < std::size(names) ? names[index] : "english";
        }

        // An empty message of the menu texts, for the title bars without the game's title text
        int EmptyMessage()
        {
            static uintptr_t table = 0;
            static int empty = -1;
            if (!Fonts || !*Fonts)
                return -1;
            uintptr_t slot = Field<uintptr_t>(*Fonts, 0x84);
            uintptr_t texts = slot ? Field<uintptr_t>(slot, 0x12C) : 0;
            if (!texts || !Field<uintptr_t>(texts, 0x60) || !Field<uintptr_t>(texts, 0x64))
                return -1;
            if (texts == table)
                return empty;
            table = texts;
            empty = -1;
            auto messages = Field<uint32_t*>(texts, 0x60);
            int count = Field<uint16_t>(Field<uintptr_t>(texts, 0x64), 0x1C);
            for (int i = 0; i < count; ++i)
            {
                if (messages[i] && *reinterpret_cast<uint32_t*>(messages[i]) == 0x04010000)
                {
                    empty = i;
                    break;
                }
            }
            return empty;
        }

        // The pause menu stops while the other player's menu or a system message is up
        bool InputBlocked(Page page)
        {
            if (!Field<int>(page, InGame))
                return false;
            uintptr_t session = *Session;
            if (Field<int>(session, 0x3973C) == 38 && Field<int>(page, Player) != Field<int>(session + 0x5940 * Field<int>(session, 0x20), 0x273C4))
                return true;
            return (Field<uint32_t>(*System, 0x10434) & 0x500) != 0;
        }
    }

    // Text

    // The game's text function draws ASCII: other letters without their accents
    std::string Ascii(const std::string& utf8)
    {
        std::string result;
        for (wchar_t c : WFPText::ToWide(utf8))
        {
            if (c >= 0x20 && c < 0x7F && c != '#' && c != '*' && c != '<' && c != '\\' && c != '`')
            {
                result += char(c);
                continue;
            }
            switch (c)
            {
                case L'\x00DF': result += "ss"; continue;
                case L'\x00C6': result += "AE"; continue;
                case L'\x00E6': result += "ae"; continue;
                case L'\x0152': result += "OE"; continue;
                case L'\x0153': result += "oe"; continue;
                case L'\x2018': case L'\x2019': result += '\''; continue;
                case L'\x201C': case L'\x201D': case L'\x00AB': case L'\x00BB': result += '"'; continue;
                case L'\x2013': case L'\x2014': result += '-'; continue;
                case L'\x2026': result += "..."; continue;
                case L'\x00A0': case L'\x202F': result += ' '; continue;
                case L'\x00BF': case L'\x00A1': continue;
                case L'\n': result += ' '; continue;
            }
            wchar_t decomposed[8] = {};
            if (NormalizeString(NormalizationD, &c, 1, decomposed, 8) > 0 && decomposed[0] >= 0x20 && decomposed[0] < 0x7F)
                result += char(decomposed[0]);
            else
                result += '?';
        }
        return result;
    }

    float Width(const std::string& text, float size)
    {
        return Game::TextWidth(*Game::Fonts, nullptr, 0, text.c_str(), size);
    }

    // Centered on x and y; smaller when wider than maxWidth
    void Text(Page page, const std::string& text, int x, int y, uint32_t color, float size, float maxWidth = 0.0f)
    {
        if (text.empty())
            return;
        if (maxWidth > 0.0f)
        {
            float width = Width(text, size);
            if (width > maxWidth)
                size *= maxWidth / width;
        }
        int pixels = int(size);
        Game::DrawRawText(page, nullptr, pixels, x, y - pixels / 2, &color, text.c_str(), TextLayer);
    }

    // Words wrapped to lines of at most maxWidth
    std::vector<std::string> Wrap(const std::string& text, float size, float maxWidth)
    {
        std::vector<std::string> lines;
        std::string line;
        size_t start = 0;
        while (start < text.size())
        {
            size_t end = text.find(' ', start);
            if (end == std::string::npos)
                end = text.size();
            auto word = text.substr(start, end - start);
            auto candidate = line.empty() ? word : line + " " + word;
            if (!line.empty() && Width(candidate, size) > maxWidth)
            {
                lines.push_back(line);
                line = word;
            }
            else
                line = candidate;
            start = end + 1;
        }
        if (!line.empty())
            lines.push_back(line);
        return lines;
    }

    // The help line under the list: two lines as the game's; a longer text gets a smaller font for a third line
    void Help(Page page, const std::string& text, int centerY, uint32_t color, float size, bool fromTop = false)
    {
        constexpr float maxWidth = 1000.0f;
        auto lines = Wrap(text, size, maxWidth);
        if (lines.size() > 2)
        {
            size *= 0.8f;
            lines = Wrap(text, size, maxWidth);
        }
        if (lines.size() > 3)
        {
            lines.resize(3);
            lines.back() += "...";
        }
        float lineHeight = size;
        float first = fromTop ? centerY + lineHeight / 2 : centerY - (lines.size() - 1) * lineHeight / 2;
        for (size_t i = 0; i < lines.size(); ++i)
            Text(page, lines[i], 640, int(first + i * lineHeight), color, size, maxWidth);
    }

    uint32_t WithAlpha(uint32_t color, int alpha)
    {
        return (color & 0x00FFFFFF) | (uint32_t(std::clamp(alpha, 0, 255)) << 24);
    }

    // The game's colours: white for the selected row, grey for the others, faded with the screen
    uint32_t RowColor(Page page, bool selected)
    {
        int alpha = Field<int>(page, FadeLevel) * 255 / 9;
        return WithAlpha(selected ? White : Grey, alpha);
    }

    std::string Upper(const std::string& utf8)
    {
        return Ascii(WFPText::Upper(utf8));
    }

    // Plugins pages

    struct Pages
    {
        bool pending = false;           // the next System Settings screen shows the plugins
        Page page = 0;                  // the System Settings screen that shows them
        bool topSelected = false;       // PLUGINS is the selected row of Options
        bool topReturn = false;

        std::shared_ptr<WFPPlugins::Snapshot> snapshot;
        int level = 0;                  // 0 the list of plugins, 1 a plugin's page
        size_t plugin = 0;
        int listCursor = 0;
        std::vector<WFPPlugins::Row> rows;
        WFPPlugins::PresetRow preset;
        WFPPlugins::RestoreRow restore;
    } pages;

    std::string PluginTitle(size_t index)
    {
        return Upper(WFPPlugins::Name(pages.snapshot->plugins[index]));
    }

    int RowCount()
    {
        if (!pages.snapshot)
            return 0;
        return pages.level == 0 ? int(pages.snapshot->plugins.size()) : int(pages.rows.size());
    }

    // The fix's settings drawn next to the game's own options are left out of its page
    std::vector<std::string> Excluded(const WFPPlugins::LoadedPlugin& plugin)
    {
        HMODULE self = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCWSTR>(&Ascii), &self);
        if (plugin.module != self)
            return {};
        return { "MAIN.FOVFactor", "MAIN.ModernControls", "MAIN.ForceLaserSight", "MAIN.KnifeParry" };
    }

    void OpenPlugin(size_t index)
    {
        pages.level = 1;
        pages.plugin = index;
        auto& plugin = pages.snapshot->plugins[index];
        pages.rows = WFPPlugins::Rows(plugin, Excluded(plugin));
        std::erase_if(pages.rows, [](const WFPPlugins::Row& row) { return !WFPPlugins::Visible(row); });
        pages.preset.Reset(plugin);
        pages.restore = {};
    }

    void OpenList()
    {
        WFPText::SetLanguage(Game::Language());
        std::string tag = "en";
        if (auto wide = WFPText::Detail::Tag(WFPText::Language()))
            tag.assign(wide, wide + wcslen(wide));
        HMODULE self = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCWSTR>(&Ascii), &self);
        pages.snapshot = WFPPlugins::Find(self, tag);
        pages.level = 0;
        pages.listCursor = 0;
        pages.rows.clear();
    }

    std::string RowLabel(const WFPPlugins::Row& row)
    {
        return Ascii(WFPPlugins::Label(row));
    }

    std::string RowValue(const WFPPlugins::Row& row)
    {
        switch (row.kind)
        {
            case WFPPlugins::Row::Kind::Preset:
                return Upper(pages.preset.ValueText());
            case WFPPlugins::Row::Kind::RestoreDefaults:
                return Upper(pages.restore.ValueText());
            case WFPPlugins::Row::Kind::Setting:
                // Words in capitals as the game's values, numbers with their units as they are
                if (row.setting->type == asi_settings::Type::Float || row.setting->type == asi_settings::Type::Int)
                {
                    auto value = WFPPlugins::ValueText(row);
                    bool listed = std::any_of(row.setting->choices.begin(), row.setting->choices.end(), [&](const auto& choice)
                    {
                        return OtherPlugins::Text(*row.plugin, choice.label) == value;
                    });
                    if (!listed)
                        return Ascii(value);
                }
                return Upper(WFPPlugins::ValueText(row));
            default:
                return {};
        }
    }

    std::string RowHelp(int index)
    {
        if (pages.level == 0)
        {
            return Ascii(WFPText::Get("WFP_PLUGINS_HELP"));
        }
        auto& row = pages.rows[index];
        auto text = WFPPlugins::Help(row);
        if (row.setting && row.setting->apply == "restart")
        {
            auto warning = WFPPlugins::RestartWarning();
            if (!warning.empty())
                text = warning;
        }
        return Ascii(text);
    }

    // Arrows of the selected row: 1 left, 2 right
    int RowArrows(int index)
    {
        if (pages.level == 0)
            return 0;
        auto& row = pages.rows[index];
        if (row.kind == WFPPlugins::Row::Kind::Preset)
            return 3;
        if (row.kind != WFPPlugins::Row::Kind::Setting || !WFPPlugins::Enabled(row))
            return 0;
        auto& setting = *row.setting;
        bool listed = !setting.choices.empty() || setting.type == asi_settings::Type::Bool;
        if (listed || !setting.step)
            return 3;
        double value = std::atof(WFPPlugins::Get(row).c_str());
        int mask = 0;
        if (!setting.min || value > *setting.min + 1e-6)
            mask |= 1;
        if (!setting.max || value < *setting.max - 1e-6)
            mask |= 2;
        return mask;
    }

    // Left/right on a row; true when something changed
    bool Step(int index, int direction)
    {
        if (pages.level == 0)
            return false;
        auto& row = pages.rows[index];
        pages.restore.Cancel();
        if (row.kind == WFPPlugins::Row::Kind::Preset)
        {
            pages.preset.Cycle(direction);
            return true;
        }
        pages.preset.Cancel();
        return WFPPlugins::Step(row, direction);
    }

    // Accept on a row; true when something happened
    bool Accept(int index)
    {
        if (pages.level == 0)
        {
            pages.listCursor = index;
            OpenPlugin(index);
            return true;
        }
        auto& row = pages.rows[index];
        auto& plugin = pages.snapshot->plugins[pages.plugin];
        switch (row.kind)
        {
            case WFPPlugins::Row::Kind::Preset:
                pages.restore.Cancel();
                return pages.preset.Accept();
            case WFPPlugins::Row::Kind::RestoreDefaults:
                pages.preset.Cancel();
                if (pages.restore.Accept(plugin, Excluded(plugin)))
                {
                    pages.preset.Reset(plugin);
                    return true;
                }
                return false;
            case WFPPlugins::Row::Kind::Action:
                pages.preset.Cancel();
                pages.restore.Cancel();
                return OtherPlugins::Invoke(plugin, *row.setting) == ASI_SETTINGS_OK;
            default:
                // Accept or a click steps through the values, as the arrows do
                return Step(index, 1);
        }
    }

    // The pages on the System Settings screen

    void DrawPage(Page page)
    {
        int count = RowCount();
        int cursor = Field<int>(page, Cursor);
        int scroll = Field<int>(page, Scroll);
        bool inGame = Field<int>(page, InGame) != 0;
        std::string title = pages.level == 0 ? Upper(WFPText::Get("WFP_PLUGINS")) : PluginTitle(pages.plugin);

        if (!inGame)
        {
            int barAlpha = Game::Fade(page, nullptr, 0, 255, 0);
            Game::Title(page, nullptr, Game::EmptyMessage());
            Text(page, title, 640, 96, RowColor(page, true), 45.0f, 1100.0f);
            for (int i = 0; i < VisibleRows && scroll + i < count; ++i)
            {
                int index = scroll + i;
                int y = 150 + 50 * i;
                bool selected = i == cursor;
                uint32_t color = RowColor(page, selected);
                float size = FontSize * (selected ? 1.0f : 0.8f);
                std::string label, value;
                if (pages.level == 0)
                {
                    label = PluginTitle(index);
                    auto& plugin = pages.snapshot->plugins[index];
                    int preset = OtherPlugins::HasPresets(plugin) ? OtherPlugins::CurrentPreset(plugin) : -2;
                    if (preset > -2)
                        value = Upper(WFPText::Get(preset < 0 ? "WFP_PRESET_CUSTOM" : WFPPlugins::PresetRow::Names[preset]));
                }
                else
                {
                    label = RowLabel(pages.rows[index]);
                    value = RowValue(pages.rows[index]);
                }
                if (pages.level == 1 && !WFPPlugins::Enabled(pages.rows[index]))
                    color = WithAlpha(color & 0xFF606060, color >> 24);
                Text(page, label, LabelX, y, color, size, 440.0f);
                Text(page, value, ValueX, y, color, size, 360.0f);
                char name[32];
                sprintf_s(name, "bar_%02d", i + 1);
                Game::ElementAlpha(page, nullptr, name, barAlpha);
                if (selected && Field<int>(page, FadeLevel) == 9)
                {
                    sprintf_s(name, "sel_%02d", i + 1);
                    float width = value.empty() ? 0.0f : std::min(Width(value, FontSize), 360.0f);
                    Game::HighlightScale(page, nullptr, name, (width + 150.0f) / 300.0f);
                }
            }
            if (Game::FadedIn(page, nullptr, 0) && cursor + scroll < count)
            {
                Game::Arrows(page, nullptr, cursor + 1, RowArrows(cursor + scroll));
                Help(page, RowHelp(cursor + scroll), 525, RowColor(page, true), FontSize);
                if (scroll > 0)
                    Game::ElementShow(page, nullptr, "up");
                if (scroll < count - VisibleRows)
                    Game::ElementShow(page, nullptr, "down");
                Game::Prompt(page, nullptr, Game::KeyboardMenus(*Game::Pad, nullptr, -1) ? 0 : 14, -1);
            }
            return;
        }

        // Pause menu: a list of text rows under the title, as the game's pause options
        int size = Game::PauseFontSize();
        int top = 180;
        Text(page, title, 640, top, White, float(size), 1100.0f);
        int y = top + size + 18;
        for (int i = 0; i < VisibleRows && scroll + i < count; ++i, y += size)
        {
            int index = scroll + i;
            bool selected = i == cursor;
            float rowSize = selected ? float(size) : float(86 * size / 100);
            uint32_t color = selected ? White : Grey;
            std::string label, value;
            if (pages.level == 0)
                label = PluginTitle(index);
            else
            {
                label = RowLabel(pages.rows[index]);
                value = RowValue(pages.rows[index]);
                if (!WFPPlugins::Enabled(pages.rows[index]))
                    color &= 0xFF606060;
            }
            Text(page, label, pages.level == 0 ? 640 : LabelX, y, color, rowSize, pages.level == 0 ? 900.0f : 440.0f);
            Text(page, value, ValueX, y, color, rowSize, 360.0f);
            if (selected)
            {
                int arrows = RowArrows(index);
                if (arrows & 1)
                    Game::ElementMove(page, nullptr, "left_03", 640, y);
                if (arrows & 2)
                    Game::ElementMove(page, nullptr, "right_03", 1020, y);
            }
        }
        if (scroll > 0)
            Game::ElementMove(page, nullptr, "up", ValueX, top + 25);
        if (scroll < count - VisibleRows)
            Game::ElementMove(page, nullptr, "down", ValueX, y - 10);
        if (cursor + scroll < count)
            Help(page, RowHelp(cursor + scroll), y + size / 2, White, float(size), true);
        Game::PausePrompt(page, nullptr, Game::KeyboardMenus(*Game::Pad, nullptr, Field<int>(page, Player)) ? 2 : 5);
    }

    void LeavePage(Page page)
    {
        Game::StopInput(page, nullptr);
        if (Field<int>(page, InGame))
            Game::SoundBackInGame(page, nullptr);
        else
            Game::SoundBack(page, nullptr);
        Field<uint8_t>(page, Leaving) = 1;
        Field<float>(page, Timer) = 0.0f;
        Field<uint8_t>(page, State) = StateLeave;
        pages.topReturn = true;
    }

    void UpdatePage(Page page)
    {
        bool inGame = Field<int>(page, InGame) != 0;
        int pressed = Game::Pressed(page, nullptr, Field<int>(page, PadIndex), Field<int>(page, Player));
        int repeated = Game::Repeated(page, nullptr, Field<int>(page, PadIndex), Field<int>(page, Player));
        if (Field<uint8_t>(*Game::System, 0x10428))
            pressed = repeated = 0;
        Game::Housekeep(page, nullptr);
        Game::HideElements(page, nullptr);
        Field<int>(page, 0x12C) = 0;
        Game::ResetLayout(page, nullptr, Field<int>(page, 0x2C), 0);
        Game::MouseScreen(page, nullptr, 6, inGame ? 173 : 143, 0);
        Game::MouseScreen(page, nullptr, 6, inGame ? 164 : 144, 1);
        Game::MouseScreen(page, nullptr, 6, 0, 2);

        if (OtherPlugins::Changed().exchange(false) && pages.level == 1)
        {
            int cursor = Field<int>(page, Cursor) + Field<int>(page, Scroll);
            OpenPlugin(pages.plugin);
            int count = RowCount();
            if (cursor >= count)
            {
                Field<int>(page, Cursor) = 0;
                Field<int>(page, Scroll) = 0;
            }
        }

        int count = RowCount();
        bool mouse = Game::KeyboardPlayer(page);
        uintptr_t mouseAreas = *Game::Mouse;

        if (Game::BackPressed(page, nullptr))
        {
            if (pages.level == 1)
            {
                pages.level = 0;
                pages.rows.clear();
                int index = pages.listCursor;
                Field<int>(page, Scroll) = std::max(0, index - (VisibleRows - 1));
                Field<int>(page, Cursor) = index - Field<int>(page, Scroll);
                inGame ? Game::SoundBackInGame(page, nullptr) : Game::SoundBack(page, nullptr);
            }
            else
                LeavePage(page);
            DrawPage(page);
            return;
        }

        if (count > 0)
        {
            int index = Field<int>(page, Cursor) + Field<int>(page, Scroll);
            bool accept = (pressed & Game::AcceptMask()) != 0 ||
                (mouse && (Game::MouseClick(mouseAreas, nullptr, 8, 0, 0) ||
                (Game::MouseClickRange(mouseAreas, nullptr, 0, VisibleRows - 1, 0, 0) && Game::MouseHover(mouseAreas, nullptr, 1) == -1)));
            if (accept)
            {
                int level = pages.level;
                if (Accept(index))
                {
                    if (level != pages.level)
                    {
                        Field<int>(page, Cursor) = 0;
                        Field<int>(page, Scroll) = 0;
                        inGame ? Game::SoundAcceptInGame(page, nullptr) : Game::SoundAccept(page, nullptr);
                    }
                    else
                        Game::SoundValue(page, nullptr);
                }
                else
                    Game::SoundError(page, nullptr);
                DrawPage(page);
                return;
            }

            int scroll = Field<int>(page, Scroll);
            int cursor = Game::ScrollNav(page, nullptr, pressed, repeated, Field<int>(page, Cursor), count, &scroll, VisibleRows);
            cursor = Game::MouseRow(page, nullptr, cursor, std::min(count, VisibleRows), 0, 0);
            if (mouse)
                scroll = Game::Wheel(mouseAreas, nullptr, scroll, 0, std::max(0, count - VisibleRows), 0, 1, 2);
            if (cursor != Field<int>(page, Cursor) || scroll != Field<int>(page, Scroll))
            {
                Field<int>(page, Cursor) = cursor;
                Field<int>(page, Scroll) = scroll;
                pages.preset.Cancel();
                pages.restore.Cancel();
                Game::SoundMove(page, nullptr);
                DrawPage(page);
                return;
            }

            int direction = 0;
            if ((repeated & 0x20020) || (mouse && Game::MouseClick(mouseAreas, nullptr, 1, 0, 1)))
                direction = 1;
            else if ((repeated & 0x80080) || (mouse && Game::MouseClick(mouseAreas, nullptr, 0, 0, 1)))
                direction = -1;
            if (direction && RowArrows(index) & (direction > 0 ? 2 : 1) && Step(index, direction))
                Game::SoundValue(page, nullptr);
        }
        DrawPage(page);
    }

    // System Settings screen

    SafetyHookInline SystemInitHook;
    void __fastcall SystemInit(Page page, void*)
    {
        SystemInitHook.unsafe_thiscall(page);
        if (pages.pending)
        {
            pages.pending = false;
            pages.page = page;
            Field<int>(page, Cursor) = 0;
            Field<int>(page, Scroll) = 0;
            OpenList();
        }
        else if (pages.page == page)
            pages.page = 0;
    }

    SafetyHookInline SystemUpdateHook;
    void __fastcall SystemUpdate(Page page, void*)
    {
        if (page != pages.page)
            return SystemUpdateHook.unsafe_thiscall(page);
        int state = Field<uint8_t>(page, State);
        if ((state == StateInput || state == StateAccepted) && !Game::InputBlocked(page))
            return UpdatePage(page);
        SystemUpdateHook.unsafe_thiscall(page);
        if (Field<uint8_t>(page, 0x3C))
            pages.page = 0;
    }

    SafetyHookInline SystemDrawHook;
    void __fastcall SystemDraw(Page page, void*)
    {
        if (page != pages.page)
            return SystemDrawHook.unsafe_thiscall(page);
        if (pages.snapshot)
            DrawPage(page);
    }

    // Options: the PLUGINS row above DEFAULT, which stays last

    constexpr int DefaultRow = TopRows - 1;     // the game's DEFAULT row
    constexpr int PluginsSlot = DefaultRow;     // where PLUGINS is drawn; DEFAULT moves one slot down
    constexpr int PauseMouseScreen = 160;       // mouse areas of the Options list in the pause menu

    // The slot (0..8) a row of the game is drawn in
    int TopSlot(int row)
    {
        return row < PluginsSlot ? row : row + 1;
    }

    int TopRowY(int slot)
    {
        return TopFirstRow + TopRowSpacing * slot;
    }

    int PauseRowY(int slot)
    {
        int size = Game::PauseFontSize();
        return 180 + size + size / 2 + slot * size;
    }

    uintptr_t* MouseAreas(int screen, int count)
    {
        uintptr_t mouse = *Game::Mouse;
        if (!mouse || screen >= Field<int>(mouse, 0x30C))
            return nullptr;
        uintptr_t areas = Field<uintptr_t*>(mouse, 0x318)[screen];
        if (!areas || Field<uint32_t>(areas, 12) < uint32_t(count))
            return nullptr;
        return Field<uintptr_t*>(areas, 24);
    }

    // The title screen's Options list: nine rows in the space of eight, with the mouse areas moved along
    void LayoutTopRows(uint16_t* rowY)
    {
        for (int i = 0; i < TopRows; ++i)
            rowY[i] = uint16_t(TopRowY(TopSlot(i)));
        if (auto list = MouseAreas(TopMouseScreen, TopRows))
        {
            for (int i = 0; i < TopRows; ++i)
            {
                if (!list[i])
                    continue;
                Field<int>(list[i], 8) = TopRowY(TopSlot(i)) - 18;
                Field<int>(list[i], 16) = TopRowY(TopSlot(i)) + 19;
            }
        }
    }

    // The pause menu's list: DEFAULT's mouse area one row down
    void LayoutPauseRows()
    {
        auto list = MouseAreas(PauseMouseScreen, TopRows);
        if (!list || !list[DefaultRow])
            return;
        int size = Game::PauseFontSize();
        int y = PauseRowY(TopSlot(DefaultRow));
        Field<int>(list[DefaultRow], 8) = y - size / 2;
        Field<int>(list[DefaultRow], 16) = y + size / 2;
    }

    uint16_t* TopRowTable;
    uint16_t* TopLabelTable;
    int TopLabels[TopRows];
    int TopHelps[TopRows];
    POINT LastMouse{ -1, -1 };

    // Mouse over the PLUGINS row
    bool MouseOverPlugins(Page page, bool& moved)
    {
        moved = false;
        if (!Game::KeyboardPlayer(page) || !*Game::Mouse)
            return false;
        POINT mouse{ Field<int>(*Game::Mouse, 0x2E0), Field<int>(*Game::Mouse, 0x2E4) };
        moved = mouse.x != LastMouse.x || mouse.y != LastMouse.y;
        LastMouse = mouse;
        if (Field<int>(page, InGame))
        {
            int size = Game::PauseFontSize();
            int y = PauseRowY(PluginsSlot);
            return mouse.x >= 335 && mouse.x <= 967 && mouse.y >= y - size / 2 && mouse.y < y + size / 2;
        }
        int y = TopRowY(PluginsSlot);
        return mouse.x >= 335 && mouse.x <= 967 && mouse.y >= y - 18 && mouse.y <= y + 19;
    }

    // A click of the menu's mouse button this frame
    bool LeftClick()
    {
        uintptr_t mouse = *Game::Mouse;
        uintptr_t devices = *Game::MouseDevices;
        if (!mouse || !devices)
            return false;
        int device = Field<int>(mouse, 0x31C);
        return (Field<uint32_t>(devices + 0x70 * device, 0x38) & Field<uint32_t>(mouse, 0x2FC)) != 0;
    }

    SafetyHookInline TopDrawHook;
    void __fastcall TopDraw(Page page, void*)
    {
        bool inGame = Field<int>(page, InGame) != 0;
        if (Field<int>(page, Popup) != 0)
            return TopDrawHook.unsafe_thiscall(page);
        if (inGame)
            LayoutPauseRows();
        else
            LayoutTopRows(TopRowTable);

        // The game's rows without the selection, help and prompts while PLUGINS is selected. In the pause menu the
        // game's rows follow each other: it draws an empty DEFAULT, which is drawn one row lower here, with the help
        // under it
        int cursor = Field<int>(page, Cursor);
        int empty = inGame ? Game::EmptyMessage() : -1;
        if (pages.topSelected)
            Field<int>(page, Cursor) = TopRows;
        if (pages.topSelected || inGame)
            Field<int>(page, Popup) = 3;
        if (empty >= 0)
            TopLabelTable[DefaultRow] = uint16_t(empty);
        TopDrawHook.unsafe_thiscall(page);
        TopLabelTable[DefaultRow] = uint16_t(TopLabels[DefaultRow]);
        Field<int>(page, Cursor) = cursor;
        Field<int>(page, Popup) = 0;

        WFPText::SetLanguage(Game::Language());
        auto label = Upper(WFPText::Get("WFP_PLUGINS"));
        auto help = Ascii(WFPText::Get("WFP_PLUGINS_HELP"));
        bool selected = pages.topSelected;
        if (!inGame)
        {
            int y = TopRowY(PluginsSlot);
            Text(page, label, 640, y, RowColor(page, selected), FontSize * (selected ? 1.0f : 0.8f));
            if (!selected)
                return;
            if (!Game::LayoutBusy(*Game::Layouts, nullptr))
                Game::Highlight(page, nullptr, 1, TopLabels[DefaultRow], true, y);
            Help(page, help, 525, RowColor(page, true), FontSize);
            Game::Prompt(page, nullptr, 1, -1);
            if (Game::FadedIn(page, nullptr, 0))
            {
                uint32_t color = White;
                Game::DrawMessageAt(page, nullptr, 30, 128, 618, &color, Game::KeyboardMenus(*Game::Pad, nullptr, -1) ? 2362 : 2363, TextLayer, -1);
            }
            return;
        }

        // Pause menu: rows below the title, one font size apart; the help line after them
        int size = Game::PauseFontSize();
        int smaller = 86 * size / 100;
        Text(page, label, 640, PauseRowY(PluginsSlot), selected ? White : Grey, selected ? float(size) : float(smaller));
        if (empty >= 0)
        {
            bool defaultSelected = !selected && cursor == DefaultRow;
            uint32_t color = defaultSelected ? White : Grey;
            Game::DrawMessageCentered(page, nullptr, defaultSelected ? size : smaller, 640, PauseRowY(TopSlot(DefaultRow)), &color, TopLabels[DefaultRow], TextLayer);
        }
        int helpY = PauseRowY(TopSlot(DefaultRow)) + size + size / 2;
        if (selected)
            Help(page, help, helpY, White, float(size));
        else
        {
            uint32_t color = White;
            Game::DrawMessageCentered(page, nullptr, size, 640, helpY, &color, TopHelps[std::clamp(cursor, 0, TopRows - 1)], TextLayer);
        }
        Game::PausePrompt(page, nullptr, 1);
    }

    void OpenPlugins(Page page)
    {
        Game::StopInput(page, nullptr);
        if (Field<int>(page, InGame))
            Game::SoundAcceptInGame(page, nullptr);
        else
            Game::SoundAccept(page, nullptr);
        pages.pending = true;
        // Opens SYSTEM SETTINGS, whose screen then shows the plugins
        Field<int>(page, Cursor) = 5;
        Field<uint8_t>(page, State) = StateAccepted;
        Field<uint8_t>(page, Blink) = 1;
        Field<float>(page, Timer) = 0.25f;
    }

    SafetyHookInline TopUpdateHook;
    void __fastcall TopUpdate(Page page, void*)
    {
        // While PLUGINS is selected the game's cursor stays on the row above it
        constexpr int RowAbove = PluginsSlot - 1;
        int state = Field<uint8_t>(page, State);
        if (state == 0 && !pages.topReturn && !pages.pending)
            pages.topSelected = false;
        if (pages.topReturn && state == StateInput)
        {
            pages.topReturn = false;
            pages.topSelected = true;
            Field<int>(page, Cursor) = RowAbove;
        }
        if (state != StateInput || Field<int>(page, Popup) != 0 || Game::InputBlocked(page))
            return TopUpdateHook.unsafe_thiscall(page);

        int pressed = Game::Pressed(page, nullptr, Field<int>(page, PadIndex), Field<int>(page, Player));
        int repeated = Game::Repeated(page, nullptr, Field<int>(page, PadIndex), Field<int>(page, Player));
        if (Field<uint8_t>(*Game::System, 0x10428))
            pressed = repeated = 0;
        constexpr int Down = 0x40040, Up = 0x10010;
        bool down = (pressed | repeated) & Down, up = (pressed | repeated) & Up;
        bool moved = false;
        bool over = MouseOverPlugins(page, moved);
        bool click = LeftClick() && over;
        int cursor = Field<int>(page, Cursor);

        auto select = [&](bool plugins, int row)
        {
            pages.topSelected = plugins;
            Field<int>(page, Cursor) = row;
            Game::SoundMove(page, nullptr);
            TopDraw(page, nullptr);
        };

        if (pages.topSelected)
        {
            if ((pressed & Game::AcceptMask()) || click)
            {
                OpenPlugins(page);
                TopDraw(page, nullptr);
                return;
            }
            if (up && !down)
                return select(false, RowAbove);
            if (down && !up)
                return select(false, DefaultRow);
            // The mouse on one of the game's rows, the back button and the rest: the game's
            TopUpdateHook.unsafe_thiscall(page);
            if (Field<int>(page, Cursor) != cursor || Field<uint8_t>(page, State) != StateInput)
                pages.topSelected = false;
            else if (Game::KeyboardPlayer(page) && *Game::Mouse && moved && !over)
            {
                int hover = Field<int>(*Game::Mouse, 0x2C);
                if (hover >= 0 && hover < TopRows)
                {
                    pages.topSelected = false;
                    Field<int>(page, Cursor) = hover;
                }
            }
            return;
        }

        if ((down && !up && cursor == RowAbove) || (up && !down && cursor == DefaultRow) || (over && moved) || click)
        {
            if (click)
            {
                pages.topSelected = true;
                Field<int>(page, Cursor) = RowAbove;
                OpenPlugins(page);
                TopDraw(page, nullptr);
                return;
            }
            return select(true, RowAbove);
        }
        TopUpdateHook.unsafe_thiscall(page);
    }

    // GAME SETTINGS: the fix's camera, controls and gameplay rows under the game's four

    // The same settings Excluded() leaves out of the fix's page
    constexpr const char* JoinedIds[] = { "MAIN.FOVFactor", "MAIN.ModernControls", "MAIN.ForceLaserSight", "MAIN.KnifeParry" };
    constexpr int GameRows = 4;
    constexpr int JoinedRows = int(std::size(JoinedIds));
    constexpr int GameMouseScreen = 145;

    struct Joined
    {
        std::shared_ptr<WFPPlugins::Snapshot> snapshot;
        std::vector<WFPPlugins::Row> rows;
        int selected = -1;              // a row of the fix, or -1 for the game's cursor
    } joined;

    struct GameTables
    {
        uint16_t* labels;
        uint16_t* helps;
        uint16_t* values[3];            // rows 0 and 1, row 2, row 3
    } gameTables;

    void (__fastcall* DrawMessage)(Page, void*, int message, int x, int y, uint32_t* color, float scale);
    void (__fastcall* HelpMessage)(Page, void*, int message, int y);
    void (__fastcall* PauseRestorePrompt)(Page, void*);

    HMODULE Self()
    {
        HMODULE self = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCWSTR>(&Ascii), &self);
        return self;
    }

    void LoadJoined()
    {
        WFPText::SetLanguage(Game::Language());
        std::string tag = "en";
        if (auto wide = WFPText::Detail::Tag(WFPText::Language()))
            tag.assign(wide, wide + wcslen(wide));
        joined.snapshot = WFPPlugins::Find(Self(), tag);
        joined.rows.clear();
        joined.selected = -1;
        if (joined.snapshot->plugins.empty() || joined.snapshot->plugins.front().module != Self())
            return;
        auto& plugin = joined.snapshot->plugins.front();
        for (auto id : JoinedIds)
        {
            if (auto setting = plugin.file.description.Find(id))
                joined.rows.push_back({ WFPPlugins::Row::Kind::Setting, &plugin, setting });
        }
    }

    int GameRowY(int row)
    {
        return 150 + 43 * row;
    }

    int GameValue(Page page, int row)
    {
        return Field<int>(page, 0x190 + 4 * row);
    }

    int GameValueCount(Page page, int row)
    {
        return Field<int>(page, 0x1A0 + 4 * row);
    }

    int GameValueMessage(Page page, int row)
    {
        auto table = gameTables.values[row < 2 ? 0 : row - 1];
        return table[GameValue(page, row)];
    }

    // Arrows of a row: 1 left, 2 right
    int JoinedArrows(int index)
    {
        auto& row = joined.rows[index];
        auto& setting = *row.setting;
        if (setting.type == asi_settings::Type::Bool || !setting.choices.empty() || !setting.step)
            return 3;
        double value = std::atof(WFPPlugins::Get(row).c_str());
        return (!setting.min || value > *setting.min + 1e-6 ? 1 : 0) | (!setting.max || value < *setting.max - 1e-6 ? 2 : 0);
    }

    std::string JoinedValue(int index)
    {
        auto& row = joined.rows[index];
        if (row.setting->type == asi_settings::Type::Float || row.setting->type == asi_settings::Type::Int)
            return Ascii(WFPPlugins::ValueText(row));
        return Upper(WFPPlugins::ValueText(row));
    }

    // The game's mouse areas of its four rows, moved to their new places
    void LayoutGameRows()
    {
        uintptr_t mouse = *Game::Mouse;
        if (!mouse || GameMouseScreen >= Field<int>(mouse, 0x30C))
            return;
        uintptr_t areas = Field<uintptr_t*>(mouse, 0x318)[GameMouseScreen];
        if (!areas || Field<uint32_t>(areas, 12) < uint32_t(GameRows))
            return;
        auto list = Field<uintptr_t*>(areas, 24);
        for (int i = 0; i < GameRows; ++i)
        {
            if (!list[i])
                continue;
            Field<int>(list[i], 8) = GameRowY(i) - 21;
            Field<int>(list[i], 16) = GameRowY(i) + 21;
        }
    }

    void DrawGameSettings(Page page)
    {
        int cursor = Field<int>(page, Cursor);
        int count = GameRows + int(joined.rows.size());
        int selectedRow = joined.selected >= 0 ? GameRows + joined.selected : cursor;
        char name[32];

        if (!Field<int>(page, InGame))
        {
            LayoutGameRows();
            Game::Title(page, nullptr, 1405);
            for (int i = 0; i < count; ++i)
            {
                int y = GameRowY(i);
                bool selected = i == selectedRow;
                uint32_t color = RowColor(page, selected);
                float scale = selected ? 1.0f : 0.8f;
                int arrows = 0;
                int widthMessage = gameTables.values[0][1];
                if (i < GameRows)
                {
                    widthMessage = GameValueMessage(page, i);
                    DrawMessage(page, nullptr, gameTables.labels[i], LabelX, y, &color, scale);
                    DrawMessage(page, nullptr, widthMessage, ValueX, y, &color, scale);
                    arrows = (GameValue(page, i) > 0 ? 1 : 0) | (GameValue(page, i) < GameValueCount(page, i) - 1 ? 2 : 0);
                }
                else
                {
                    Text(page, Ascii(WFPPlugins::Label(joined.rows[i - GameRows])), LabelX, y, color, FontSize * scale, 440.0f);
                    Text(page, JoinedValue(i - GameRows), ValueX, y, color, FontSize * scale, 360.0f);
                    arrows = JoinedArrows(i - GameRows);
                }
                if (!selected)
                    continue;
                if (Field<int>(page, FadeLevel) == 9)
                    Game::Highlight(page, nullptr, 4, widthMessage, true, y);
                sprintf_s(name, "left_%02d", 4);
                if (arrows & 1)
                    Game::ElementMove(page, nullptr, name, 640, y);
                sprintf_s(name, "right_%02d", 4);
                if (arrows & 2)
                    Game::ElementMove(page, nullptr, name, 1020, y);
            }
            if (selectedRow < GameRows)
                HelpMessage(page, nullptr, gameTables.helps[selectedRow], 525);
            else
                Help(page, Ascii(WFPPlugins::Help(joined.rows[selectedRow - GameRows])), 525, RowColor(page, true), FontSize);
            Game::Prompt(page, nullptr, 2, -1);
            Game::Prompt(page, nullptr, 0x10, -1);
            return;
        }

        // Pause menu: the game's text list, closer together for the added rows
        int size = Game::PauseFontSize();
        constexpr int spacing = 36;
        int top = 216;
        uint32_t white = White;
        Game::DrawMessageCentered(page, nullptr, size, 640, top, &white, 1405, TextLayer);
        int y = top + size + 18;
        for (int i = 0; i < count; ++i, y += spacing)
        {
            bool selected = i == selectedRow;
            int rowSize = selected ? size : 86 * size / 100;
            uint32_t color = selected ? White : Grey;
            int arrows = 0;
            if (i < GameRows)
            {
                Game::DrawMessageCentered(page, nullptr, rowSize, LabelX, y, &color, gameTables.labels[i], TextLayer);
                Game::DrawMessageCentered(page, nullptr, rowSize, ValueX, y, &color, GameValueMessage(page, i), TextLayer);
                arrows = (GameValue(page, i) > 0 ? 1 : 0) | (GameValue(page, i) < GameValueCount(page, i) - 1 ? 2 : 0);
            }
            else
            {
                Text(page, Ascii(WFPPlugins::Label(joined.rows[i - GameRows])), LabelX, y, color, float(rowSize), 440.0f);
                Text(page, JoinedValue(i - GameRows), ValueX, y, color, float(rowSize), 360.0f);
                arrows = JoinedArrows(i - GameRows);
            }
            if (selected && (arrows & 1))
                Game::ElementMove(page, nullptr, "left_02", 640, y);
            if (selected && (arrows & 2))
                Game::ElementMove(page, nullptr, "right_02", 1020, y);
        }
        int helpY = y - spacing + size;
        if (selectedRow < GameRows)
            Game::DrawMessageCentered(page, nullptr, size, 640, helpY, &white, gameTables.helps[selectedRow], TextLayer);
        else
            Help(page, Ascii(WFPPlugins::Help(joined.rows[selectedRow - GameRows])), helpY - size * 4 / 10, White, size * 0.8f, true);
        Game::PausePrompt(page, nullptr, 2);
        PauseRestorePrompt(page, nullptr);
    }

    SafetyHookInline GameDrawHook;
    void __fastcall GameDraw(Page page, void*)
    {
        if (joined.rows.empty())
            return GameDrawHook.unsafe_thiscall(page);
        DrawGameSettings(page);
    }

    // The game's Restore to default also resets the fix's rows
    SafetyHookInline GameRestoreHook;
    void __fastcall GameRestore(Page page, void*)
    {
        GameRestoreHook.unsafe_thiscall(page);
        if (joined.rows.empty())
            return;
        std::vector<const asi_settings::Setting*> settings;
        for (auto& row : joined.rows)
            settings.push_back(row.setting);
        WFPPlugins::RestoreDefaults(joined.snapshot->plugins.front(), settings);
    }

    // The fix's row under the mouse, or -1
    int MouseOverJoined(Page page, bool& moved)
    {
        moved = false;
        if (!Game::KeyboardPlayer(page) || !*Game::Mouse || Field<int>(page, InGame))
            return -1;
        POINT mouse{ Field<int>(*Game::Mouse, 0x2E0), Field<int>(*Game::Mouse, 0x2E4) };
        moved = mouse.x != LastMouse.x || mouse.y != LastMouse.y;
        LastMouse = mouse;
        for (int i = 0; i < int(joined.rows.size()); ++i)
        {
            int y = GameRowY(GameRows + i);
            if (mouse.x >= 200 && mouse.x <= 1080 && mouse.y >= y - 21 && mouse.y < y + 21)
                return i;
        }
        return -1;
    }

    SafetyHookInline GameUpdateHook;
    void __fastcall GameUpdate(Page page, void*)
    {
        int state = Field<uint8_t>(page, State);
        if (state == 0)
            LoadJoined();
        if (joined.rows.empty() || state != StateInput || Game::InputBlocked(page))
            return GameUpdateHook.unsafe_thiscall(page);

        int pressed = Game::Pressed(page, nullptr, Field<int>(page, PadIndex), Field<int>(page, Player));
        int repeated = Game::Repeated(page, nullptr, Field<int>(page, PadIndex), Field<int>(page, Player));
        if (Field<uint8_t>(*Game::System, 0x10428))
            pressed = repeated = 0;
        constexpr int Down = 0x40040, Up = 0x10010;
        bool down = (pressed | repeated) & Down, up = (pressed | repeated) & Up;
        bool moved = false;
        int over = MouseOverJoined(page, moved);
        bool click = over >= 0 && LeftClick();
        int rows = int(joined.rows.size());
        int cursor = Field<int>(page, Cursor);

        auto redraw = [&]()
        {
            // The frame's layout and mouse work of the game's update, then the rows
            bool inGame = Field<int>(page, InGame) != 0;
            Game::MouseScreen(page, nullptr, 4, inGame ? 165 : 145, 0);
            Game::MouseScreen(page, nullptr, 4, inGame ? 166 : 146, 1);
            Game::Housekeep(page, nullptr);
            Game::HideElements(page, nullptr);
            Field<int>(page, 0x12C) = 0;
            Game::ResetLayout(page, nullptr, Field<int>(page, 0x2C), 0);
            DrawGameSettings(page);
        };
        auto select = [&](int row, int gameCursor)
        {
            joined.selected = row;
            Field<int>(page, Cursor) = gameCursor;
            Game::SoundMove(page, nullptr);
            redraw();
        };

        if (joined.selected >= 0)
        {
            int index = joined.selected;
            if (up && !down)
                return index == 0 ? select(-1, GameRows - 1) : select(index - 1, cursor);
            if (down && !up)
            {
                if (index < rows - 1)
                    return select(index + 1, cursor);
                if (pressed & Down)
                    return select(-1, 0);
                return redraw();
            }
            int direction = 0;
            bool mouse = Game::KeyboardPlayer(page);
            if ((repeated & 0x20020) || (mouse && Game::MouseClick(*Game::Mouse, nullptr, 1, 0, 1)) || (pressed & Game::AcceptMask()) || (click && over == index))
                direction = 1;
            else if ((repeated & 0x80080) || (mouse && Game::MouseClick(*Game::Mouse, nullptr, 0, 0, 1)))
                direction = -1;
            if (direction)
            {
                bool step = (pressed & Game::AcceptMask()) || click ? JoinedArrows(index) == 3 : (JoinedArrows(index) & (direction > 0 ? 2 : 1)) != 0;
                if (step && WFPPlugins::Step(joined.rows[index], direction))
                    Game::SoundValue(page, nullptr);
                return redraw();
            }
            if (over >= 0 && over != index && moved)
                return select(over, cursor);
            // Back, restore and the mouse on the game's rows: the game's
            GameUpdateHook.unsafe_thiscall(page);
            if (Field<int>(page, Cursor) != cursor)
                joined.selected = -1;
            else if (Game::KeyboardPlayer(page) && *Game::Mouse && moved && over < 0)
            {
                int hover = Field<int>(*Game::Mouse, 0x2C);
                if (hover >= 0 && hover < GameRows)
                {
                    joined.selected = -1;
                    Field<int>(page, Cursor) = hover;
                }
            }
            return;
        }

        if (down && !up && cursor == GameRows - 1)
            return select(0, cursor);
        if ((pressed & Up) && !down && cursor == 0)
            return select(rows - 1, cursor);
        if (over >= 0 && (moved || click))
        {
            select(over, cursor);
            if (click && JoinedArrows(over) == 3 && WFPPlugins::Step(joined.rows[over], 1))
                Game::SoundValue(page, nullptr);
            return;
        }
        GameUpdateHook.unsafe_thiscall(page);
    }

    template<class T>
    T Call(uintptr_t function, uint32_t offset)
    {
        uintptr_t site = function + offset;
        if (*reinterpret_cast<uint8_t*>(site) != 0xE8 && *reinterpret_cast<uint8_t*>(site) != 0xE9)
            return T{};
        return (T)injector::GetBranchDestination(site).as_int();
    }

    template<class T>
    T Pointer(uintptr_t address)
    {
        return *reinterpret_cast<T*>(address);
    }
}

export namespace Menu
{
    void Init()
    {
        auto topUpdate = hook::pattern("53 56 8B F1 33 DB 39 9E 8C 01 00 00");
        auto topDraw = hook::pattern("83 EC 0C 56 8B F1 83 BE 8C 01 00 00 00");
        auto systemUpdate = hook::pattern("83 EC 08 56 8B F1 83 BE 8C 01 00 00 00 74 40");
        auto systemDraw = hook::pattern("A1 ? ? ? ? 83 EC 68 80 B8 28 05 01 00 00");
        auto systemInit = hook::pattern("83 EC 08 53 55 56 8B F1 8B 0D ? ? ? ? 57");
        auto keyboardUpdate = hook::pattern("83 EC 10 56 8B F1 83 BE 8C 01 00 00 00");
        auto drawText = hook::pattern("53 8B 5C 24 18 56 57 8B 7C 24 10 51 F3 0F 2A C7 F3 0F 11 04 24 53 8B F1 8B 0D ? ? ? ? 6A 00 E8 ? ? ? ? E8 ? ? ? ? 8B 4C 24 24 8B 54 24 1C 51 8B 4C 24 1C 53 52 99");
        auto textWidth = hook::pattern("56 8B 74 24 0C 85 F6 75 06 D9 EE 5E");
        auto highlightScale = hook::pattern("8B 41 18 83 F8 5C 73 3A 50 8B 44 24 08 51 8B CC 50 E8 ? ? ? ? 8B 0D ? ? ? ? E8 ? ? ? ? 85 C0 74 1D F3 0F 10 44 24 08");
        auto elementMove = hook::pattern("8B 41 18 83 F8 5C 73 45 50 8B 44 24 08");
        auto pausePrompt = hook::pattern("51 A1 ? ? ? ? 53 55 56 8B D9 8B 88 2C 04 01 00 57 E8 ? ? ? ? 83 F8 01 75 1C 8B 0D ? ? ? ? F3 0F 10 81 70 30 00 00 F3 0F 59 05 ? ? ? ? F3 0F 2C E8 EB 0E");
        auto fontSize = hook::pattern("A1 ? ? ? ? 8B 88 2C 04 01 00 E8 ? ? ? ? 83 F8 01 75 23");
        auto drawCentered = hook::pattern("83 EC 10 53 8B 5C 24 28 56 8B 74 24 1C 57 6A FF 56 56 6A 00 8D 44 24 1C 50 53 8B F9 8B 0D ? ? ? ? 6A 00 E8 ? ? ? ? 8B 4C 24 34 F3 0F 10 05");
        if (topUpdate.size() != 1 || topDraw.size() != 1 || systemUpdate.size() != 1 || systemDraw.size() != 1 || systemInit.size() != 1 ||
            keyboardUpdate.size() != 1 || drawText.size() != 1 || textWidth.size() != 1 || highlightScale.size() != 1 || elementMove.size() != 1 ||
            pausePrompt.size() != 1 || fontSize.size() != 1 || drawCentered.size() != 1)
            return;

        uintptr_t top = uintptr_t(topDraw.get_first());
        uintptr_t update = uintptr_t(systemUpdate.get_first());
        uintptr_t draw = uintptr_t(systemDraw.get_first());
        uintptr_t topInput = uintptr_t(topUpdate.get_first());
        uintptr_t keyboard = uintptr_t(keyboardUpdate.get_first());

        using namespace Game;
        Pressed = Call<decltype(Pressed)>(update, 0x5E);
        Repeated = Call<decltype(Repeated)>(update, 0x72);
        Housekeep = Call<decltype(Housekeep)>(update, 0x97);
        HideElements = Call<decltype(HideElements)>(update, 0x9E);
        ResetLayout = Call<decltype(ResetLayout)>(update, 0xB5);
        MouseScreen = Call<decltype(MouseScreen)>(update, 0x17E);
        BackPressed = Call<decltype(BackPressed)>(update, 0x1B7);
        StopInput = Call<decltype(StopInput)>(update, 0x1C6);
        MouseClick = Call<decltype(MouseClick)>(update, 0x2AE);
        ScrollNav = Call<decltype(ScrollNav)>(update, 0x320);
        MouseRow = Call<decltype(MouseRow)>(update, 0x32E);
        Wheel = Call<decltype(Wheel)>(update, 0x35B);
        SoundBackInGame = Call<decltype(SoundBackInGame)>(update, 0x25A);
        SoundBack = Call<decltype(SoundBack)>(update, 0x261);
        SoundValue = Call<decltype(SoundValue)>(update, 0x464);
        SoundMove = Call<decltype(SoundMove)>(update, 0x480);
        SoundAcceptInGame = Call<decltype(SoundAcceptInGame)>(update, 0x4BA);
        MouseClickRange = Call<decltype(MouseClickRange)>(keyboard, 0x19E);
        MouseHover = Call<decltype(MouseHover)>(keyboard, 0x1AF);
        SoundError = Call<decltype(SoundError)>(keyboard, 0x1BB);
        SoundAccept = Call<decltype(SoundAccept)>(topInput, 0x294);
        Title = Call<decltype(Title)>(top, 0xB8);
        Highlight = Call<decltype(Highlight)>(top, 0x193);
        LayoutBusy = Call<decltype(LayoutBusy)>(top, 0x177);
        Prompt = Call<decltype(Prompt)>(top, 0x27D);
        FadedIn = Call<decltype(FadedIn)>(top, 0x374);
        KeyboardMenus = Call<decltype(KeyboardMenus)>(top, 0x395);
        DrawMessageAt = Call<decltype(DrawMessageAt)>(top, 0x3C3);
        Fade = Call<decltype(Fade)>(draw, 0x37);
        ElementAlpha = Call<decltype(ElementAlpha)>(draw, 0x30B);
        Arrows = Call<decltype(Arrows)>(draw, 0x407);
        ElementShow = Call<decltype(ElementShow)>(draw, 0x42F);
        DrawRawText = (decltype(DrawRawText))drawText.get_first();
        TextWidth = (decltype(TextWidth))textWidth.get_first();
        HighlightScale = (decltype(HighlightScale))highlightScale.get_first();
        ElementMove = (decltype(ElementMove))elementMove.get_first();
        PausePrompt = (decltype(PausePrompt))pausePrompt.get_first();
        PauseFontSize = (decltype(PauseFontSize))fontSize.get_first();

        Pad = Pointer<uintptr_t*>(top + 0x388 + 2);
        MouseDevices = reinterpret_cast<uintptr_t*>(uintptr_t(Pad) + 0x38);
        Layouts = Pointer<uintptr_t*>(top + 0x171 + 2);
        Mouse = Pointer<uintptr_t*>(update + 0x2A2 + 2);
        Fonts = Pointer<uintptr_t*>(uintptr_t(drawText.get_first()) + 0x18 + 2);
        System = Pointer<uintptr_t*>(update + 0x3E + 1);
        Session = Pointer<uintptr_t*>(update + 0x0F + 1);
        TopRowTable = Pointer<uint16_t*>(top + 0xC4 + 4);
        auto labels = Pointer<uint16_t*>(top + 0x149 + 4);
        TopLabelTable = labels;
        for (int i = 0; i < TopRows; ++i)
            TopLabels[i] = labels[i];
        Main = Pointer<uintptr_t*>(update + 0x150 + 2);
        auto helps = Pointer<uint16_t*>(top + 0x1B0 + 4);
        for (int i = 0; i < TopRows; ++i)
            TopHelps[i] = helps[i];
        DrawMessageCentered = (decltype(DrawMessageCentered))drawCentered.get_first();

        for (auto function : { (void*)Pressed, (void*)Repeated, (void*)Housekeep, (void*)HideElements, (void*)ResetLayout, (void*)MouseScreen,
            (void*)BackPressed, (void*)StopInput, (void*)MouseClick, (void*)ScrollNav, (void*)MouseRow, (void*)Wheel, (void*)SoundBackInGame,
            (void*)SoundBack, (void*)SoundValue, (void*)SoundMove, (void*)SoundAcceptInGame, (void*)MouseClickRange, (void*)MouseHover,
            (void*)SoundError, (void*)SoundAccept, (void*)Title, (void*)Highlight, (void*)LayoutBusy, (void*)Prompt, (void*)FadedIn,
            (void*)KeyboardMenus, (void*)DrawMessageAt, (void*)Fade, (void*)ElementAlpha, (void*)Arrows, (void*)ElementShow })
        {
            if (!function)
                return;
        }

        auto gameUpdate = hook::pattern("68 A5 00 00 00 6A 04 E8 ? ? ? ? 6A 01 68 A6 00 00 00");
        auto gameDraw = hook::pattern("83 EC 30 56 8B F1 83 BE 8C 01 00 00 00");
        auto gameRestore = hook::pattern("53 56 8B F1 57 33 FF 8D 9E 90 01 00 00");
        if (gameUpdate.size() == 1 && gameDraw.size() == 1 && gameRestore.size() == 1)
        {
            uintptr_t settingsDraw = uintptr_t(gameDraw.get_first());
            DrawMessage = Call<decltype(DrawMessage)>(top, 0x163);
            HelpMessage = Call<decltype(HelpMessage)>(top, 0x1C0);
            auto pauseDraw = Call<uintptr_t>(settingsDraw, 0x0F);
            PauseRestorePrompt = pauseDraw ? Call<decltype(PauseRestorePrompt)>(pauseDraw, 0x32D) : nullptr;
            gameTables.labels = Pointer<uint16_t*>(settingsDraw + 0x7A + 4);
            gameTables.values[0] = Pointer<uint16_t*>(settingsDraw + 0xB4 + 4);
            gameTables.values[1] = Pointer<uint16_t*>(settingsDraw + 0xD1 + 4);
            gameTables.values[2] = Pointer<uint16_t*>(settingsDraw + 0xEE + 4);
            gameTables.helps = Pointer<uint16_t*>(settingsDraw + 0x1A5 + 4);
            if (DrawMessage && HelpMessage && PauseRestorePrompt)
            {
                GameUpdateHook = safetyhook::create_inline(gameUpdate.get_first(-0x105), GameUpdate);
                GameDrawHook = safetyhook::create_inline(gameDraw.get_first(), GameDraw);
                GameRestoreHook = safetyhook::create_inline(gameRestore.get_first(), GameRestore);
            }
        }

        SystemInitHook = safetyhook::create_inline(systemInit.get_first(), SystemInit);
        SystemUpdateHook = safetyhook::create_inline(systemUpdate.get_first(), SystemUpdate);
        SystemDrawHook = safetyhook::create_inline(systemDraw.get_first(), SystemDraw);
        TopUpdateHook = safetyhook::create_inline(topUpdate.get_first(), TopUpdate);
        TopDrawHook = safetyhook::create_inline(topDraw.get_first(), TopDraw);
    }
}
