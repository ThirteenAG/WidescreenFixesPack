module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "../Shared/RWGTA/FixMenu.hpp"

export module Menu;

// The fourth Options page lists plugins with settings, with this fix first. Each plugin opens its own
// numbered settings pages, per-plugin Preset and Restore Defaults. Native text, colours and navigation
// stay the same; values use the same runtime interface as FusionFixHub.
//
// The native Options screen keeps fixed tables for its three pages, so the new page is a "virtual" page: while it
// shows, the fix runs the page frame instead of the native one and the native page stays where it was.

namespace
{
    uint8_t* Options = nullptr;     // the options object: +252 page, +248 page change request, +176 the options block

    int(__cdecl* PadHeld)(int) = nullptr;
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

    // Our page: the list of pages (View = -1) or one of them
    bool PageActive = false;
    RWGTAMenu::Model Model;
    int View = -1;
    int Row = 0;                    // 0..RowCount() - 1, the last one is Exit (list) or Back (page)
    int ListRow = 0;                // the list's row while a page shows
    int Scroll = 0;                 // the list's first row shown, when it has more rows than fit
    int HelpFrames = 0;
    std::vector<const RWGTAMenu::Row*> Rows;

    constexpr int PageRows = 8;     // settings per page
    constexpr int VisibleRows = 9;  // rows above the help line
    constexpr int BarSegments = 10; // the volume bar
    constexpr int SoundMove = 40000, SoundCancel = 40002, SoundChange = 40003, SoundSlider = 40005, SoundPage = 40089, SoundEnter = 42001;
    constexpr uint32_t ColourNormal = 0x80606060, ColourSelected = 0x80404060;
    constexpr uintptr_t MsgOn = 0x31, MsgOff = 0x32, MsgExit = 0x30;     // the game's own words

    int RowCount()
    {
        return (View < 0 ? int(Model.Pages().size()) : int(Rows.size())) + 1;
    }

    // The native spacing, tightened when the rows would reach the help line
    int RowY(int slot)
    {
        int rows = std::min(RowCount(), VisibleRows);
        return 96 + std::min(32, 232 / std::max(1, rows - 1)) * slot;
    }

    // The language of the texts: MAIN.TextLanguage, or the game's (the options block)
    std::string LanguageTag()
    {
        auto& chosen = FixSettings().String("MAIN.TextLanguage");
        if (!chosen.empty() && chosen != "auto")
            return chosen;
        static const char* tags[] = { "en", "en", "fr", "de", "it", "es", "en" };
        int language = Options ? Options[176 + 17] : 1;
        return tags[std::clamp(language, 0, 6)];
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

    // Text with the native drop shadow; bank -1 = our own text, 0 = a message of the game's common bank
    void Text(const uint16_t* text, int x, int y, int preset, int bank = -1)
    {
        ColorPreset(8);
        DrawMessage(bank, text, x + 2, y + 2);
        ColorPreset(preset);
        DrawMessage(bank, text, x, y);
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

    // UTF-8 text split into lines of at most width characters (the help font is about as wide as the native help)
    std::vector<std::string> Wrap(const std::string& utf8, size_t width)
    {
        std::vector<std::string> lines;
        std::string line;
        size_t start = 0, letters = 0;
        auto count = [](const std::string& s) { return size_t(std::count_if(s.begin(), s.end(), [](char c) { return (uint8_t(c) & 0xC0) != 0x80; })); };
        while (start <= utf8.size())
        {
            size_t end = utf8.find(' ', start);
            if (end == std::string::npos)
                end = utf8.size();
            auto word = utf8.substr(start, end - start);
            if (!line.empty() && letters + 1 + count(word) > width)
            {
                lines.push_back(line);
                line.clear();
                letters = 0;
            }
            letters += (line.empty() ? 0 : 1) + count(word);
            line += (line.empty() ? "" : " ") + word;
            start = end + 1;
        }
        if (!line.empty())
            lines.push_back(line);
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

    void ShowRows()
    {
        Rows = Model.VisibleRows(size_t(View));
        if (Rows.size() > size_t(PageRows))
            Rows.resize(PageRows);
    }

    void Select(int row)
    {
        if (row != Row)
            HelpFrames = 0;
        Row = row;
    }

    void OpenList()
    {
        HMODULE self = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCWSTR>(&OpenList), &self);
        Model.OpenPlugins(self, LanguageTag(), PageRows);
        PageActive = true;
        View = -1;
        Row = ListRow = Scroll = HelpFrames = 0;
        PlaySound(SoundPage);
    }

    void OpenPage(int page)
    {
        ListRow = Row;
        View = page;
        Row = Scroll = HelpFrames = 0;
        Model.PageShown(size_t(page));
        ShowRows();
    }

    void CloseView()
    {
        View = -1;
        Row = ListRow;
        HelpFrames = 0;
    }

    void LeavePage(int page)
    {
        PageActive = false;
        Model.Close();
        Rows.clear();
        *reinterpret_cast<int32_t*>(Options + 252) = page;
        PlaySound(SoundPage);
    }

    int PageFrame()
    {
        if (View >= 0)
            ShowRows();
        int rows = RowCount();
        int result = 0;
        int trigger = PadTrigger(0);
        int repeat = PadRepeat(0);
        auto clamp = [&]()
        {
            Row = std::clamp(Row, 0, rows - 1);
            Scroll = std::clamp(Scroll, std::max(0, Row - VisibleRows + 1), Row);
        };
        clamp();

        // Leaving: the shoulder buttons or the page arrows
        bool next = (trigger & 0x4000) || (MouseLeftClick() && MouseNextPage());
        bool previous = (trigger & 0x1000) || (MouseLeftClick() && MousePreviousPage());
        if (next || previous)
        {
            LeavePage(next ? 0 : 2);
            return 0;
        }

        if (MouseRightClick() || PadAction(0, 1))
        {
            // Cancel: from a page back to the list, in the list to Exit like the native pages
            if (View >= 0)
                CloseView();
            else
                Select(rows - 1);
            PlaySound(SoundCancel);
        }
        else if (repeat & 1)
        {
            Select((Row + rows - 1) % rows);
            PlaySound(SoundMove);
        }
        else if (repeat & 2)
        {
            Select((Row + 1) % rows);
            PlaySound(SoundMove);
        }
        else
        {
            // Mouse: the row under the cursor, and its value or arrows
            int shown = std::min(rows - Scroll, VisibleRows);
            if (MouseMoved())
            {
                for (int slot = 0; slot < shown; ++slot)
                {
                    int32_t rect[4] = { 40, RowY(slot), 424, 24 };
                    if (MouseHitRect(rect) && slot + Scroll != Row)
                    {
                        Select(slot + Scroll);
                        PlaySound(SoundMove);
                    }
                }
            }
            int y = RowY(Row - Scroll);
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
            if (Row == rows - 1)
            {
                if (enter)
                {
                    PlaySound(SoundEnter);
                    if (View >= 0)
                        CloseView();
                    else
                        result = 1;     // native save confirmation and exit
                }
            }
            else if (View < 0)
            {
                if (enter)
                {
                    PlaySound(SoundEnter);
                    OpenPage(Row);
                }
            }
            else if (direction || enter)
            {
                auto& row = *Rows[Row];
                bool bar = BarPositions(row) != 0;
                if (!(bar && !direction) && Model.Change(row, direction, false))
                    PlaySound(bar ? SoundSlider : SoundChange);
            }
        }

        if (View >= 0)
            ShowRows();
        rows = RowCount();
        clamp();
        int shown = std::min(rows - Scroll, VisibleRows);

        // Title
        TextFont(0);
        auto title = Model.Text("menu.page4");
        if (View >= 0)
        {
            // "Option 4 (Camera)", like the native page titles
            auto page = Model.Text("menu.page4.page");
            auto at = page.find("{0}");
            title = at == std::string::npos ? Model.Pages()[View].title : page.replace(at, 3, Model.Pages()[View].title);
        }
        Text(GameText(title), 140, 36, 3);

        // Rows
        for (int slot = 0; slot < shown; ++slot)
        {
            int row = slot + Scroll;
            bool selected = row == Row;
            int preset = selected ? 2 : 0;
            uint32_t colour = selected ? ColourSelected : ColourNormal;
            int y = RowY(slot);
            if (row == rows - 1)
            {
                if (View < 0)
                    Text(reinterpret_cast<const uint16_t*>(MsgExit), 40, y, preset, 0);
                else
                    Text(GameText(Model.Text("menu.back")), 40, y, preset);
                continue;
            }
            if (View < 0)
            {
                Text(GameText(Model.Pages()[row].title), 40, y, preset);
                continue;
            }
            auto& entry = *Rows[row];
            Text(GameText(Label(entry)), 40, y, preset);
            if (BarPositions(entry))
                DrawBar(364, BarFilled(entry), y + 8, colour);
            else if (entry.kind == RowKind::Setting && entry.setting->type == asi_settings::Type::Bool)
                Text(reinterpret_cast<const uint16_t*>(OtherPlugins::Get(*entry.plugin, *entry.setting) == "1" ? MsgOn : MsgOff), 364, y, preset, 0);
            else
                Text(GameText(Model.Value(entry)), 364, y, preset);
            if (selected && entry.kind != RowKind::SettingsFile && entry.kind != RowKind::Action)
            {
                // The right arrow after values longer than the native ones (about 13 units a letter)
                int width = BarPositions(entry) ? 0 : int(Letters(Model.Value(entry))) * 13;
                DrawArrow(344.0f, float(y + 8), 2, 1.0f, 0x80808080);
                DrawArrow(float(std::max(464, 364 + width + 6)), float(y + 8), 3, 1.0f, 0x80808080);
            }
        }
        ColorPreset(0);

        // Help line, sliding in like the native one
        if (View >= 0 && Row < rows - 1)
        {
            int frames = std::max(1, Fps() / 5);
            float t = std::min(1.0f, float(HelpFrames) / float(frames));
            float x = 528.0f - 464.0f * t;
            TextFont(1);
            TextAlpha(int(t * t * 128.0f));
            auto lines = Wrap(Help(*Rows[Row]), 44);
            for (size_t i = 0; i < lines.size() && i < 3; ++i)
                Text(GameText(lines[i]), int(x), 360 + int(i) * 22, 0);
            TextAlpha(128);
            TextFont(0);
            ++HelpFrames;
        }

        // Page arrows
        Triangle(40, 54, 48, 42, 48, 66);
        Triangle(464, 42, 472, 54, 464, 66);
        return result;
    }

    // The native page frame takes the options object in eax
    int __cdecl Frame(uint8_t* options)
    {
        Options = options;
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
            int result = PageFrame();
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

export namespace Menu
{
    void Init()
    {
        auto frame = hook::pattern("83 EC 18 53 55 56 8B F0 8B 86 FC 00 00 00");
        auto input = hook::pattern("83 EC 74 53 56 33 F6 57 56 8B D8 89 74 24 10");
        auto drawMessage = hook::pattern("8B 44 24 10 8B 4C 24 0C 8B 54 24 08 50 8B 44 24 08 51 52 50");
        auto preset = hook::pattern("A1 ? ? ? ? 8B 4C 24 04 C1 E0 18");
        auto alpha = hook::pattern("8B 44 24 04 8B 0D ? ? ? ? 8B 15 ? ? ? ?");
        auto font = hook::pattern("8B 44 24 04 A3 ? ? ? ? C3 90 90 90 90 90 90 A1 ? ? ? ? 8B 4C 24 04");
        auto arrow = hook::pattern("51 A0 ? ? ? ? 84 C0 75 3E 6A 01");
        auto moved = hook::pattern("A1 ? ? ? ? 85 C0 75 0C A1 ? ? ? ? 85 C0 75 03 33 C0 C3");
        if (frame.size() != 1 || input.size() != 1 || drawMessage.size() != 1 || preset.size() != 1 || alpha.size() != 1 ||
            font.size() != 1 || arrow.size() != 1 || moved.empty())
            return;

        auto in = input.get_first<uint8_t>();
        Call(PadHeld, in + 0x0F);
        Call(PadTrigger, in + 0x19);
        Call(PadRepeat, in + 0x21);
        Call(MouseRightClick, in + 0x8E);
        Call(PadAction, in + 0xA2);
        Call(MouseLeftClick, in + 0xB2);
        Call(MousePreviousPage, in + 0xBB);
        Call(MouseNextPage, in + 0xC4);
        Call(MouseHitRect, in + 0x301);
        Call(PlaySound, in + 0x4BD);
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
        // The "mouse moved" test reads the mouse deltas (another function has the same code for other values)
        auto mouseX = hook::pattern("8B 0D ? ? ? ? 33 C0 85 C9 74 05 B8 01 00 00");
        for (size_t i = 0; i < moved.size() && !mouseX.empty(); ++i)
        {
            if (*moved.get(i).get<void*>(1) == *mouseX.get(0).get<void*>(2))
                MouseMoved = reinterpret_cast<decltype(MouseMoved)>(moved.get(i).get<void>(0));
        }
        if (!MouseMoved)
            return;
        // The volume bar
        auto barFunction = hook::pattern("53 55 8B 6C 24 10 56 57 33 F6 8B D8");
        if (barFunction.size() == 1)
            DrawBarFn = barFunction.get_first();
        if (!DrawBarFn || fr[0x24C] != 0x68 || fr[0x251] != 0xE8 || fr[0x283] != 0xE8 || fr[0x28F] != 0xE8 || fr[0xFF] != 0xE8)
            return;

        FrameHook = safetyhook::create_inline(frame.get_first(), &FrameThunk);
    }
}
