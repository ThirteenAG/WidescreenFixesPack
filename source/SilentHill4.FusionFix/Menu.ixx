module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/RWGTA/FixMenu.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"
#include "Shared/SilentHill/Display.hpp"
#include "Shared/MenuList.hpp"
#include "Shared/RainDroplets.hpp"

export module Menu;

import ComVars;

// The fourth Options page lists plugins with settings, with this fix first. Each plugin opens its page: Preset, its
// settings or a row per category opening that category, and Restore Defaults. Every list scrolls row by row. Native
// text, colours and navigation stay the same; values use the same runtime interface as FusionFixHub.
//
// The native Options screen keeps fixed tables for its three pages, so the new page is a "virtual" page: while it
// shows, the fix runs the page frame instead of the native one and the native page stays where it was.

namespace
{
    uint8_t* Options = nullptr;     // the options object: +252 page, +248 page change request, +176 the options block

    int(__cdecl* PadTrigger)(int) = nullptr;
    int(__cdecl* PadRepeat)(int) = nullptr;
    int(__cdecl* PadAction)(int, int) = nullptr;
    int(__cdecl* MouseLeftClick)() = nullptr;
    int(__cdecl* MouseRightClick)() = nullptr;
    int(__cdecl* MousePreviousPage)() = nullptr;
    int(__cdecl* MouseNextPage)() = nullptr;
    int(__cdecl* MouseHitRect)(int32_t*) = nullptr;
    int(__cdecl* MouseMoved)() = nullptr;
    int(__cdecl* PlaySound)(int) = nullptr;
    int(__cdecl* Fps)() = nullptr;
    void(__cdecl* DrawMessage)(int bank, const uint16_t* text, int x, int y) = nullptr;   // bank -1: raw text
    void(__cdecl* ColorPreset)(int) = nullptr;
    void(__cdecl* TextAlpha)(int) = nullptr;
    void(__cdecl* TextFont)(int) = nullptr;
    void(__cdecl* DrawArrow)(float x, float y, int direction, float scale, uint32_t colour) = nullptr;
    void* DrawBarFn = nullptr;      // usercall eax = x; filled, y, colour
    void(__cdecl* PrimColour)(void*, uint32_t, uint32_t, uint32_t) = nullptr;
    void(__cdecl* PrimTriangle)(void*, float, float, float, float, float, float) = nullptr;
    void(__cdecl* DrawPrim)(void*, int) = nullptr;
    void* Prim = nullptr;

    SafetyHookInline FrameHook;

    using RWGTAMenu::RowKind;

    // Our page: the plugins list (View = RootPage), a plugin's page or a category
    bool PageActive = false;
    bool HelpActive = false, HelpRequested = false;
    int HelpScroll = 0;
    std::string HelpText, UpdateNotice;
    std::string DisplayLanguage, UpdateLanguage, UpdateVersion;
    bool UpdateViaHub = false;
    RWGTAMenu::Model Model;
    std::map<std::pair<uintptr_t, std::string>, std::string> RestartValues;
    size_t View = RWGTAMenu::Model::RootPage;
    int HelpFrames = 0;
    const RWGTAMenu::Row* HelpRow = nullptr;     // the row the sliding help line belongs to
    bool HelpRowBack = false;
    // Wheel notches from the window, taken by the next frame
    std::atomic<int> WheelDelta{ 0 };
    MenuList::WheelNotches Wheel;
    // The Options frame ran since the last Present: a menu of the game is on the screen
    bool OptionsFrame = false;

    constexpr int VisibleRows = 9;  // rows above the help line, Back or Exit among them
    constexpr int BarSegments = 10; // the volume bar
    constexpr int SoundMove = 40000, SoundCancel = 40002, SoundChange = 40003, SoundSlider = 40005, SoundPage = 40089, SoundEnter = 42001;
    constexpr uint32_t ColourNormal = 0x80606060, ColourSelected = 0x80404060;

    // The native spacing, tightened when the rows would reach the help line
    int RowY(int slot, int shown)
    {
        return 96 + std::min(32, 232 / std::max(1, shown - 1)) * slot;
    }

    // The language of the texts: MAIN.TextLanguage, or the game's (the options block)
    std::string LanguageTag()
    {
        auto chosen = FixSettings().Get("MAIN.TextLanguage");
        if (!chosen.empty() && chosen != "auto")
            return chosen;
        static const char* tags[] = { "en", "en", "fr", "de", "it", "es", "en" };
        int language = Options ? Options[176 + 17] : 1;
        return tags[std::clamp(language, 0, 6)];
    }

    std::string FormatNotice(const std::string& version, bool hub, const std::string& language)
    {
        auto texts = FixSettings().GetTexts();
        texts.Select(language);
        auto line = texts.Resolve(hub ? "@update.availablehub" : "@update.availablesite");
        const auto replace = [&line](const char* key, const std::string& value)
        {
            const auto at = line.find(key);
            if (at != std::string::npos)
                line.replace(at, strlen(key), value);
        };
        replace("{name}", texts.Resolve(FixSettings().GetDescription().name));
        replace("{version}", version);
        replace("{page}", RWGTA::UpdateCheck::Page());
        return line;
    }

    // UTF-8 to the game's text: one 16-bit code per Latin-1 character (256 - c, space = 0), ended by FFFF 8000
    const uint16_t* GameText(const std::string& utf8)
    {
        static std::map<std::string, std::vector<uint16_t>> cache;
        auto& entry = cache[utf8];
        if (entry.empty())
        {
            int length = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), int(utf8.size()), nullptr, 0);
            std::wstring wide(length, L'\0');
            MultiByteToWideChar(CP_UTF8, 0, utf8.data(), int(utf8.size()), wide.data(), length);
            for (wchar_t c : wide)
            {
                if (c == L' ')
                    entry.push_back(0);
                else if (c > 0x20 && c < 0x100)
                    entry.push_back(uint16_t(256 - c));
                else if (c > 0x20)
                    entry.push_back(uint16_t(256 - '?'));
            }
            entry.push_back(0xFFFF);
            entry.push_back(0x8000);
        }
        return entry.data();
    }

    // Render the selected language with the native drop shadow.
    void Text(const uint16_t* text, int x, int y, int preset)
    {
        ColorPreset(8);
        DrawMessage(-1, text, x + 2, y + 2);
        ColorPreset(preset);
        DrawMessage(-1, text, x, y);
    }

    void DrawBar(int x, int filled, int y, uint32_t colour)
    {
        auto fn = DrawBarFn;
        __asm
        {
            push colour
            push y
            push filled
            mov eax, x
            call fn
            add esp, 12
        }
    }

    void Triangle(float x0, float y0, float x1, float y1, float x2, float y2)
    {
        PrimColour(Prim, 0x80808080, 0x80808080, 0x80808080);
        PrimTriangle(Prim, x0, y0, x1, y1, x2, y2);
        DrawPrim(Prim, 1);
    }

    // The number of letters of UTF-8 text
    size_t Letters(const std::string& s)
    {
        return size_t(std::count_if(s.begin(), s.end(), [](char c) { return (uint8_t(c) & 0xC0) != 0x80; }));
    }

    std::vector<std::string> Wrap(const std::string& text, size_t width)
    {
        std::vector<std::string> lines;
        std::string line, word;
        auto flush = [&]()
        {
            if (word.empty())
                return;
            if (!line.empty() && Letters(line) + 1 + Letters(word) > width)
            {
                lines.push_back(std::move(line));
                line.clear();
            }
            while (Letters(word) > width)
            {
                size_t end = 0, count = 0;
                while (end < word.size() && count < width)
                {
                    ++end;
                    while (end < word.size() && (uint8_t(word[end]) & 0xC0) == 0x80)
                        ++end;
                    ++count;
                }
                lines.push_back(word.substr(0, end));
                word.erase(0, end);
            }
            if (!word.empty())
            {
                if (!line.empty())
                    line += ' ';
                line += word;
                word.clear();
            }
        };
        for (char character : text)
        {
            if (character == ' ' || character == '\t' || character == '\r' || character == '\n')
            {
                flush();
                if (character == '\n')
                {
                    lines.push_back(std::move(line));
                    line.clear();
                }
            }
            else
                word += character;
        }
        flush();
        if (!line.empty())
            lines.push_back(std::move(line));
        return lines;
    }

    // The number of positions of a number shown as a volume bar, 0 for other rows
    int BarPositions(const RWGTAMenu::Row& row)
    {
        if (row.kind != RowKind::Setting)
            return 0;
        auto& s = *row.setting;
        bool number = s.type == asi_settings::Type::Int || s.type == asi_settings::Type::Float;
        if (!number || !s.min || !s.max || !s.step || *s.step <= 0 || !s.special.empty() || !s.suggestions.empty())
            return 0;
        int positions = int(std::lround((*s.max - *s.min) / *s.step)) + 1;
        return positions >= 2 && positions <= 21 ? positions : 0;
    }

    // The filled segments of the bar for the value in effect
    int BarFilled(const RWGTAMenu::Row& row)
    {
        auto& s = *row.setting;
        int positions = BarPositions(row);
        double value = std::atof(OtherPlugins::Get(*row.plugin, s).c_str());
        int position = std::clamp(int(std::lround((value - *s.min) / *s.step)), 0, positions - 1);
        return int(std::lround(double(position) * BarSegments / (positions - 1)));
    }

    // The row's name: settings by their label (a restart is mentioned in the help line), the rest from the model
    std::string Label(const RWGTAMenu::Row& row)
    {
        if (row.kind == RowKind::Setting || row.kind == RowKind::Action)
            return OtherPlugins::Text(*row.plugin, row.setting->label);
        return Model.Label(row);
    }

    // The help line of a row (UTF-8)
    std::string Help(const RWGTAMenu::Row& row)
    {
        switch (row.kind)
        {
        case RowKind::Preset:
        case RowKind::ApplyPreset:
            return Model.Text("menu.presethelp");
        case RowKind::SettingsFile:
            return Model.Text("menu.settingsfilehelp");
        case RowKind::RestoreDefaults:
            return Model.Help(row);
        default:
            break;
        }
        auto help = OtherPlugins::Text(*row.plugin, row.setting->description);
        if (row.kind == RowKind::Setting && row.setting->apply == "restart")
            help += (help.empty() ? "" : ". ") + Model.Text("menu.restarthelp");
        return help;
    }

    // The rows in the window, slot by slot; a null row is Exit (the plugins list) or Back
    std::vector<const RWGTAMenu::Row*> Shown()
    {
        return Model.Shown(View, true);
    }

    bool HasHelp(const RWGTAMenu::Row* row)
    {
        return row && !RWGTAMenu::Model::Opens(*row);
    }

    void RememberRestartValues(const asi_settings::LoadedPlugin& plugin)
    {
        for (auto& setting : plugin.file.description.settings)
        {
            if (setting.HasValue() && setting.apply == "restart")
                RestartValues.try_emplace({ reinterpret_cast<uintptr_t>(plugin.module), setting.id }, OtherPlugins::Get(plugin, setting));
        }
    }

    bool RestartRequired()
    {
        for (auto& page : Model.Pages())
        {
            if (!page.plugin)
                continue;
            for (auto& setting : page.plugin->file.description.settings)
            {
                if (!setting.HasValue() || setting.apply != "restart")
                    continue;
                auto initial = RestartValues.find({ reinterpret_cast<uintptr_t>(page.plugin->module), setting.id });
                if (initial != RestartValues.end() && initial->second != OtherPlugins::Get(*page.plugin, setting))
                    return true;
            }
        }
        return false;
    }

    void OpenList()
    {
        HMODULE self = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCWSTR>(&OpenList), &self);
        DisplayLanguage = LanguageTag();
        Model.OpenMenu(self, DisplayLanguage, VisibleRows);
        for (auto& page : Model.Pages())
        {
            if (page.plugin)
                RememberRestartValues(*page.plugin);
        }
        PageActive = true;
        HelpActive = false;
        HelpScroll = 0;
        View = RWGTAMenu::Model::RootPage;
        Model.Window(View).Reset();
        HelpFrames = 0;
        Wheel.Clear();
        WheelDelta = 0;
        PlaySound(SoundPage);
    }

    // Back from a page to the list it was opened from (that row selected); false on the plugins list
    bool CloseView()
    {
        const auto parent = Model.Back(View);
        if (parent == SIZE_MAX)
            return false;
        View = parent;
        return true;
    }

    void LeavePage(int page)
    {
        PageActive = false;
        HelpActive = false;
        Model.Close();
        *reinterpret_cast<int32_t*>(Options + 252) = page;
        PlaySound(SoundPage);
    }

    std::string FitLine(const std::string& text, size_t letters)
    {
        if (Letters(text) <= letters)
            return text;
        size_t end = 0, count = 0;
        while (end < text.size() && count < letters - 3)
        {
            ++end;
            while (end < text.size() && (uint8_t(text[end]) & 0xC0) == 0x80)
                ++end;
            ++count;
        }
        return text.substr(0, end) + "...";
    }

    void DrawFooter()
    {
        TextFont(1);
        Text(GameText(FitLine(Model.Text("menu.helphint"), 50)), 40, 401, 0);
        if (RestartRequired())
            Text(GameText(FitLine(Model.Text("menu.restartwarning"), 50)), 40, 425, 0);
        TextFont(0);
    }

    void DrawHelp(int repeat)
    {
        auto lines = Wrap(HelpText, 44);
        if (repeat & 1)
            --HelpScroll;
        else if (repeat & 2)
            ++HelpScroll;
        HelpScroll = std::clamp(HelpScroll, 0, std::max(0, int(lines.size()) - 12));
        TextFont(0);
        Text(GameText(Model.Text("menu.help")), 140, 36, 3);
        TextFont(1);
        for (int line = 0; line < 12 && HelpScroll + line < int(lines.size()); ++line)
            Text(GameText(lines[HelpScroll + line]), 40, 88 + line * 21, 0);
        Text(GameText(Model.Text("menu.back")), 40, 369, 2);
        TextFont(0);
        DrawFooter();
    }

    int PageFrame(bool languageChanged)
    {
        auto& window = Model.Window(View);
        auto rows = Shown();
        int shown = int(rows.size());
        const auto at = [&](int slot) { return slot >= 0 && slot < shown ? rows[slot] : nullptr; };
        const RWGTAMenu::Row* selected = at(int(window.Slot()));
        const bool freshLayout = !languageChanged;
        int result = 0;
        int trigger = PadTrigger(0);
        int repeat = PadRepeat(0);

        if ((freshLayout && HelpRequested) || HelpActive)
        {
            if (HasHelp(selected))
                HelpText = Label(*selected) + ": " + Help(*selected);
            else
                HelpText = UpdateNotice;
            if (!HelpActive && !HelpText.empty())
            {
                HelpActive = true;
                HelpScroll = 0;
                PlaySound(SoundEnter);
                DrawHelp(0);
                return 0;
            }
        }
        if (HelpActive)
        {
            int32_t back[4] = { 40, 357, 424, 28 };
            if (HelpRequested || MouseRightClick() || PadAction(0, 1) || (MouseLeftClick() && MouseHitRect(back)))
            {
                HelpActive = false;
                PlaySound(SoundCancel);
            }
            else
                DrawHelp(repeat);
            return 0;
        }

        // Leaving: the shoulder buttons or the page arrows
        bool next = (trigger & 0x4000) || (MouseLeftClick() && MouseNextPage());
        bool previous = (trigger & 0x1000) || (MouseLeftClick() && MousePreviousPage());
        if (next || previous)
        {
            LeavePage(next ? 0 : 2);
            return 0;
        }

        // Home and End go to the ends of the list, the wheel moves it a row per notch
        DWORD process = 0;
        GetWindowThreadProcessId(GetForegroundWindow(), &process);
        const bool focused = process == GetCurrentProcessId();
        static bool homeHeld = false, endHeld = false;
        const bool home = focused && (GetAsyncKeyState(VK_HOME) & 0x8000);
        const bool end = focused && (GetAsyncKeyState(VK_END) & 0x8000);
        bool moved = false;
        if (home && !homeHeld)
            moved |= window.Edge(-1);
        if (end && !endHeld)
            moved |= window.Edge(1);
        homeHeld = home;
        endHeld = end;
        for (int notches = Wheel.Take(WheelDelta.exchange(0)); notches; notches += notches > 0 ? -1 : 1)
            moved |= window.Scroll(notches > 0 ? -1 : 1);
        if (moved)
            PlaySound(SoundMove);

        if (MouseRightClick() || PadAction(0, 1))
        {
            // Cancel: from a page back to the list it was opened from, in the plugins list to Exit like the native pages
            if (!CloseView())
                window.Edge(1);
            PlaySound(SoundCancel);
        }
        else if (repeat & 1)
        {
            window.Move(-1);
            PlaySound(SoundMove);
        }
        else if (repeat & 2)
        {
            window.Move(1);
            PlaySound(SoundMove);
        }
        else if (freshLayout)
        {
            // Mouse: the row under the cursor while it moves, and its value or arrows
            if (MouseMoved())
            {
                for (int slot = 0; slot < shown; ++slot)
                {
                    int32_t rect[4] = { 40, RowY(slot, shown), 424, 24 };
                    if (MouseHitRect(rect) && window.Point(size_t(slot)))
                        PlaySound(SoundMove);
                }
            }
            selected = at(int(window.Slot()));
            int y = RowY(int(window.Slot()), shown);
            int32_t rowRect[4] = { 40, y, 424, 24 };
            int32_t leftRect[4] = { 344, y, 20, 24 };
            int32_t valueRect[4] = { 364, y, 100, 24 };
            int32_t rightRect[4] = { 464, y, 20, 24 };
            bool click = MouseLeftClick();
            bool enter = PadAction(0, 17) || (click && MouseHitRect(rowRect) && !MouseHitRect(leftRect) && !MouseHitRect(rightRect));
            int direction = 0;
            if ((trigger & 4) || (click && MouseHitRect(leftRect)))
                direction = -1;
            else if ((trigger & 8) || (click && (MouseHitRect(rightRect) || MouseHitRect(valueRect))))
                direction = 1;
            if (!selected)
            {
                if (enter)
                {
                    PlaySound(SoundEnter);
                    if (!CloseView())
                        result = 1;     // native save confirmation and exit
                }
            }
            else if (RWGTAMenu::Model::Opens(*selected))
            {
                const auto page = enter ? Model.Enter(*selected) : SIZE_MAX;
                if (page != SIZE_MAX)
                {
                    PlaySound(SoundEnter);
                    View = page;
                }
            }
            else if (direction || enter)
            {
                bool bar = BarPositions(*selected) != 0;
                if (!(bar && !direction) && Model.Change(*selected, direction, false))
                    PlaySound(bar ? SoundSlider : SoundChange);
            }
        }

        auto& view = Model.Window(View);
        rows = Shown();
        shown = int(rows.size());
        selected = at(int(view.Slot()));
        if (selected != HelpRow || !selected != HelpRowBack)
            HelpFrames = 0;
        HelpRow = selected;
        HelpRowBack = !selected;

        // Title
        TextFont(0);
        auto title = Model.Text("menu.page4");
        if (View != RWGTAMenu::Model::RootPage)
        {
            // "Option 4 (Camera)", like the native page titles
            auto page = Model.Text("menu.page4.page");
            auto at = page.find("{0}");
            title = at == std::string::npos ? Model.Pages()[View].title : page.replace(at, 3, Model.Pages()[View].title);
        }
        // A title longer than the room between the page arrows takes the game's smaller font
        if (Letters(title) > 24)
            TextFont(1);
        Text(GameText(FitLine(title, 36)), 140, 36, 3);
        TextFont(0);

        // Rows
        for (int slot = 0; slot < shown; ++slot)
        {
            const auto row = at(slot);
            bool isSelected = slot == int(view.Slot());
            int preset = isSelected ? 2 : 0;
            uint32_t colour = isSelected ? ColourSelected : ColourNormal;
            int y = RowY(slot, shown);
            if (!row)
            {
                Text(GameText(Model.Text(View == RWGTAMenu::Model::RootPage ? "menu.exit" : "menu.back")), 40, y, preset);
                continue;
            }
            // Labels longer than the label column take the game's smaller font, like its long values
            const auto label = Label(*row);
            const bool compactLabel = Letters(label) > (RWGTAMenu::Model::Opens(*row) ? 30u : 22u);
            if (compactLabel)
                TextFont(1);
            Text(GameText(label), 40, y, preset);
            TextFont(0);
            if (RWGTAMenu::Model::Opens(*row))
                continue;
            auto& entry = *row;
            if (BarPositions(entry))
                DrawBar(364, BarFilled(entry), y + 8, colour);
            else
            {
                const auto value = Model.Value(entry);
                const bool compact = Letters(value) > 8;
                if (compact)
                    TextFont(1);
                Text(GameText(FitLine(value, compact ? 12 : 8)), 364, y, preset);
                TextFont(0);
            }
            if (isSelected && entry.kind != RowKind::SettingsFile && entry.kind != RowKind::Action && !Model.Value(entry).empty())
            {
                // The right arrow after values longer than the native ones (about 13 units a letter)
                int width = BarPositions(entry) ? 0 : int(Letters(Model.Value(entry))) * 13;
                DrawArrow(344.0f, float(y + 8), 2, 1.0f, 0x80808080);
                DrawArrow(float(std::clamp(364 + width + 6, 464, 488)), float(y + 8), 3, 1.0f, 0x80808080);
            }
        }
        // More rows above or below the window: the native arrows, pointing up and down, right of the list
        if (view.Above())
            DrawArrow(496.0f, float(RowY(0, shown) + 8), 0, 1.0f, 0x80808080);
        if (view.Below())
            DrawArrow(496.0f, float(RowY(shown - 1, shown) + 8), 1, 1.0f, 0x80808080);
        ColorPreset(0);

        // Help line, sliding in like the native one
        if (HasHelp(selected))
        {
            int frames = std::max(1, Fps() / 5);
            float t = std::min(1.0f, float(HelpFrames) / float(frames));
            float x = 528.0f - 464.0f * t;
            TextFont(1);
            TextAlpha(int(t * t * 128.0f));
            auto lines = Wrap(Help(*selected), 44);
            for (size_t i = 0; i < lines.size() && i < 2; ++i)
                Text(GameText(lines[i]), int(x), 360 + int(i) * 22, 0);
            TextAlpha(128);
            TextFont(0);
            ++HelpFrames;
        }

        DrawFooter();

        // Page arrows
        Triangle(40, 54, 48, 42, 48, 66);
        Triangle(464, 42, 472, 54, 464, 66);
        return result;
    }

    // The native page frame takes the options object in eax
    int __cdecl Frame(uint8_t* options)
    {
        Options = options;
        const auto language = LanguageTag();
        if (FixSettings().Get("MAIN.CheckForUpdates") == "0")
        {
            UpdateVersion.clear();
            UpdateNotice.clear();
        }
        else if (!UpdateVersion.empty() && language != UpdateLanguage)
        {
            UpdateLanguage = language;
            UpdateNotice = FormatNotice(UpdateVersion, UpdateViaHub, language);
        }
        const bool languageChanged = PageActive && language != DisplayLanguage && Model.SelectLanguage(language);
        if (languageChanged)
            DisplayLanguage = language;
        OptionsFrame = true;
        RainDroplets::MenuOpen(true);
        DWORD process = 0;
        GetWindowThreadProcessId(GetForegroundWindow(), &process);
        const bool held = process == GetCurrentProcessId() && (GetAsyncKeyState(VK_F1) & 0x8000);
        static bool previousHelp = false;
        HelpRequested = (held && !previousHelp) || GameplayPad.mapPressed;
        previousHelp = held;
        auto page = *reinterpret_cast<int32_t*>(options + 252);
        auto request = *reinterpret_cast<int32_t*>(options + 248);
        if (!PageActive && request == 0)
        {
            int trigger = PadTrigger(0);
            bool next = (trigger & 0x4000) || (MouseLeftClick() && MouseNextPage());
            bool previous = (trigger & 0x1000) || (MouseLeftClick() && MousePreviousPage());
            if ((page == 2 && next) || (page == 0 && previous))
            {
                OpenList();
                return 0;
            }
        }
        if (PageActive)
        {
            int result = PageFrame(languageChanged);
            if (result)
            {
                PageActive = false;
                Model.Close();
            }
            return result;
        }
        auto original = FrameHook.original<void*>();
        int result;
        __asm
        {
            mov eax, options
            call original
            mov result, eax
        }
        if (result != 0)
            return result;

        // Deliver the response where its notice can be drawn on this frame.
        RWGTA::UpdateCheck::Tick();
        if (!UpdateNotice.empty())
        {
            TextFont(1);
            const auto lines = Wrap(UpdateNotice, 44);
            const int top = 425 - (static_cast<int>(lines.size()) - 1) * 20;
            for (size_t row = 0; row < lines.size(); ++row)
                Text(GameText(lines[row]), 40, top + static_cast<int>(row) * 20, 0);
            TextFont(0);
        }
        return result;
    }

    __declspec(naked) void FrameThunk()
    {
        __asm
        {
            push eax
            call Frame
            add esp, 4
            ret
        }
    }

    template<class T>
    void Call(T& target, uint8_t* at)
    {
        target = reinterpret_cast<T>(injector::GetBranchDestination(at).as_int());
    }
}

class Menu
{
public:
    Menu()
    {
        WFP::onInitEvent() += []()
        {
            HMODULE self = nullptr;
            GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCWSTR>(&OpenList), &self);
            for (auto& plugin : asi_settings::FindLoadedPlugins(self, "en"))
                RememberRestartValues(plugin);

            auto frame = hook::pattern("83 EC 18 53 55 56 8B F0 8B 86 FC 00 00 00");
            auto input = hook::pattern("83 EC 74 53 56 33 F6 57 56 8B D8 89 74 24 10");
            const bool alternate = input.size() == 0;
            auto in = alternate ?
                hook::pattern("83 EC 74 53 55 8B AC 24 80 00 00 00 56 57 33 FF 57 8B D8 89 7C 24 14").get_first<uint8_t>() :
                input.get_first<uint8_t>();
            auto drawMessage = hook::pattern("8B 44 24 10 8B 4C 24 0C 8B 54 24 08 50 8B 44 24 08 51 52 50");
            auto preset = hook::pattern("A1 ? ? ? ? 8B 4C 24 04 C1 E0 18");
            auto alpha = hook::pattern("8B 44 24 04 8B 0D ? ? ? ? 8B 15 ? ? ? ?");
            auto font = hook::pattern("8B 44 24 04 A3 ? ? ? ? C3 90 90 90 90 90 90 A1 ? ? ? ? 8B 4C 24 04");
            auto arrow = hook::pattern("51 A0 ? ? ? ? 84 C0 75 3E 6A 01");
            auto moved = hook::pattern("A1 ? ? ? ? 85 C0 75 0C A1 ? ? ? ? 85 C0 75 03 33 C0 C3 B8 01 00 00 00 C3 90 90 90 90 90 A1 ? ? ? ? C3");
            Call(PadTrigger, in + (alternate ? 0x21 : 0x19));
            Call(PadRepeat, in + (alternate ? 0x29 : 0x21));
            Call(MouseRightClick, in + (alternate ? 0x8F : 0x8E));
            Call(PadAction, in + (alternate ? 0xA3 : 0xA2));
            Call(MouseLeftClick, in + (alternate ? 0xB3 : 0xB2));
            Call(MousePreviousPage, in + (alternate ? 0xBC : 0xBB));
            Call(MouseNextPage, in + (alternate ? 0xC5 : 0xC4));
            Call(MouseHitRect, in + (alternate ? 0x360 : 0x301));
            Call(PlaySound, in + (alternate ? 0x51C : 0x4BD));
            auto fr = frame.get_first<uint8_t>();
            Call(Fps, fr + 0xFF);
            Prim = *reinterpret_cast<void**>(fr + 0x24D);
            Call(PrimColour, fr + 0x251);
            Call(PrimTriangle, fr + 0x283);
            Call(DrawPrim, fr + 0x28F);
            DrawMessage = reinterpret_cast<decltype(DrawMessage)>(drawMessage.get_first());
            ColorPreset = reinterpret_cast<decltype(ColorPreset)>(preset.get_first());
            TextAlpha = reinterpret_cast<decltype(TextAlpha)>(alpha.get_first());
            TextFont = reinterpret_cast<decltype(TextFont)>(font.get_first());
            DrawArrow = reinterpret_cast<decltype(DrawArrow)>(arrow.get_first());
            MouseMoved = reinterpret_cast<decltype(MouseMoved)>(moved.get_first());
            // The volume bar
            auto barFunction = hook::pattern("53 55 8B 6C 24 10 56 57 33 F6 8B D8");
            DrawBarFn = barFunction.get_first();
            RWGTA::UpdateCheck::Config updates;
            updates.pluginId = "SilentHill4.FusionFix";
            updates.fileVersion = rsc_FileVersion;
            updates.settings = &FixSettings();
            updates.show = [](const std::wstring& notice) { UpdateNotice = asi_settings::text::Narrow(notice); };
            updates.format = [](const std::string& version, bool hub)
            {
                UpdateVersion = version;
                UpdateViaHub = hub;
                UpdateLanguage = LanguageTag();
                return asi_settings::text::Widen(FormatNotice(version, hub, UpdateLanguage));
            };
            RWGTA::UpdateCheck::Init(std::move(updates));

            FrameHook = safetyhook::create_inline(frame.get_first(), &FrameThunk);
            SHDisplay::OnWindowMessage().push_back([](HWND, UINT message, WPARAM wParam, LPARAM, LRESULT&)
            {
                if (PageActive && message == WM_MOUSEWHEEL)
                    WheelDelta += GET_WHEEL_DELTA_WPARAM(wParam);
                return false;
            });
            SHDisplay::OnPresent().push_back([]()
            {
                // The droplets stay off the Options screen while it shows
                if (!OptionsFrame)
                    RainDroplets::MenuOpen(false);
                OptionsFrame = false;
            });
        };
    }
} Menu;
