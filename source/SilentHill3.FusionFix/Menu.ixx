module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/RWGTA/FixMenu.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"
#include "Shared/SilentHill/Display.hpp"
#include "Shared/SilentHill/Gamepad.hpp"

export module Menu;

import ComVars;

// The fourth Options page lists plugins with settings, with this fix first. Each plugin opens its own
// numbered settings pages, per-plugin Preset and Restore Defaults. Native text, colours and navigation
// stay the same; values use the same runtime interface as FusionFixHub.
//
// The native Options screen keeps one selection for its 28 rows across its three pages; the page boundaries are
// hard-coded all over it. The new page therefore is a "virtual" page: while it shows, the native selection stays
// parked on the first OPTIONS row and the fix runs the frame instead of the native frame function.

namespace
{


    void(__cdecl* TextReset)() = nullptr;
    void(__cdecl* ReadOptInput)() = nullptr;
    void(__cdecl* SetTextBg)(int, int, int) = nullptr;
    void(__cdecl* TextLayerFront)() = nullptr;
    void(__cdecl* TextLayerBack)() = nullptr;
    void(__cdecl* OptBackground)() = nullptr;
    void* DrawOptTextFn = nullptr;      // usercall eax = colour, edx = row, ecx = kind; stack: message id
    void(__cdecl* SetColor)(int, int, int, int) = nullptr;
    const uint16_t* (__cdecl* Encode)(const char*) = nullptr;
    void(__cdecl* CentreOn)() = nullptr;
    void(__cdecl* CentreOff)() = nullptr;
    void(__cdecl* SetGlyphSize)(int, int) = nullptr;
    void(__cdecl* ClearGlyphSize)() = nullptr;
    int(__cdecl* MsgWidth)(int) = nullptr;
    void(__cdecl* ClearRects)() = nullptr;
    void(__cdecl* AddRect)(int, int, int, int, int) = nullptr;
    int(__cdecl* HitTest)() = nullptr;
    int(__cdecl* MouseMoved)() = nullptr;
    int(__cdecl* MouseClicked)() = nullptr;
    int(__cdecl* MouseHeld)() = nullptr;
    void(__cdecl* SliderRects)(int, int) = nullptr;
    void(__cdecl* PlaySE)(int, float, float) = nullptr;
    void(__cdecl* QueueDraw)(void*, void*, int) = nullptr;
    void(__cdecl* SliderDraw)(void*) = nullptr;

    SafetyHookInline FrameHook, PageNextHook, PagePrevHook, ReadInputHook;
    SafetyHookMid DrawOptTextHook, OptInitHook;

    using RWGTAMenu::RowKind;

    // Our page: the list of pages (View = -1) or one of them
    bool PageActive = false;
    bool HelpActive = false, HelpRequested = false;
    int HelpScroll = 0;
    std::string HelpText, UpdateNotice;
    bool OptionsSeen = false;
    RWGTAMenu::Model Model;
    int View = -1;
    int Row = 1;                    // 1..RowCount(), the last one is Exit (list) or Back (page)
    int ListRow = 1;                // the list's row while a page shows
    int Scroll = 0;                 // the list's first row shown, when it has more rows than fit
    std::vector<const RWGTAMenu::Row*> Rows;
    std::map<std::pair<uintptr_t, std::string>, std::string> RestartValues;     // restart settings as the game started

    constexpr int PageRows = 9;     // settings per page, as many as the OPTIONS page has
    constexpr int VisibleRows = 10; // rows above the help line
    constexpr int MsgOn = 39, MsgOff = 40, MsgExit = 104, MsgExitHelp = 105;
    constexpr int SoundMove = 1, SoundAccept = 3, SoundChange = 5, SoundSlider = 7;

    int RowCount()
    {
        return (View < 0 ? int(Model.Pages().size()) : int(Rows.size())) + 1;
    }

    // Screen row of a row of the view
    int Slot(int row)
    {
        return row - Scroll;
    }

    int RowY(int slot)
    {
        return 25 * slot + 71;
    }

    void Sound(int id)
    {
        if (PlaySE)
            PlaySE(id, 1.0f, 0.0f);
    }

    // The native row text: a native message at the row's label, value, help or centred position
    void DrawOptText(int colour, int row, int kind, int message)
    {
        auto fn = DrawOptTextFn;
        __asm
        {
            push message
            mov eax, colour
            mov edx, row
            mov ecx, kind
            call fn
            add esp, 4
        }
    }

    // The language of the texts: MAIN.TextLanguage, or the one picked in the OPTIONS page (which may not be
    // applied yet)
    std::string LanguageTag()
    {
        auto& chosen = FixSettings().String("MAIN.TextLanguage");
        if (!chosen.empty() && chosen != "auto")
            return chosen;
        static const char* tags[] = { "en", "en", "fr", "de", "it", "es", "en", "en" };
        int language = aOptionsPendingValues.is_initialized() ? (*aOptionsPendingValues)[2] : 1;
        return tags[std::clamp(language, 0, 7)];
    }

    // UTF-8 to the game's text: Windows-1252, "\h" for the proportional font, no backslash (an escape)
    std::string ToGame(const std::string& utf8)
    {
        int length = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), int(utf8.size()), nullptr, 0);
        std::wstring wide(length, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, utf8.data(), int(utf8.size()), wide.data(), length);
        int size = WideCharToMultiByte(1252, 0, wide.data(), length, nullptr, 0, nullptr, nullptr);
        std::string text(size, '\0');
        WideCharToMultiByte(1252, 0, wide.data(), length, text.data(), size, nullptr, nullptr);
        std::replace(text.begin(), text.end(), '\\', '/');
        return "\\h" + text;
    }

    int Width(const std::string& text)
    {
        int widths[8] = {};
        MeasureLines(widths, Encode(text.c_str()));
        return widths[0];
    }

    std::string FitLine(std::string text, int width)
    {
        for (auto& c : text)
        {
            if (c == '\n' || c == '\r')
                c = ' ';
        }
        if (Width(text) <= width)
            return text;
        while (text.size() > 2 && Width(text + "...") > width)
            text.pop_back();
        return text + "...";
    }

    void SetColour(int index)
    {
        auto c = aTextColours->data() + index * 4;
        SetColor(c[0], c[1], c[2], c[3]);
    }

    // Text in a native colour; the selected (5) and title (4) colours get the raised look with a drop shadow
    void DrawText(int colour, const std::string& text, int x, int y, bool centred = false)
    {
        if (colour == 0)
            return;
        if (centred)
            CentreOn();
        if (colour == 4 || colour == 5)
        {
            SetColour(6);
            DrawGlyphs(Encode(text.c_str()), x + 2, y + 2);
            SetColour(colour);
            DrawGlyphs(Encode(text.c_str()), x - 2, y - 2);
        }
        else
        {
            SetColour(colour);
            DrawGlyphs(Encode(text.c_str()), x, y);
        }
        if (centred)
            CentreOff();
    }

    // UTF-8 text split into lines of the game's text no wider than width
    std::vector<std::string> Wrap(const std::string& utf8, int width)
    {
        std::vector<std::string> lines;
        std::string line;
        size_t start = 0;
        while (start <= utf8.size())
        {
            size_t end = utf8.find(' ', start);
            if (end == std::string::npos)
                end = utf8.size();
            auto word = utf8.substr(start, end - start);
            auto candidate = line.empty() ? word : line + " " + word;
            if (!line.empty() && Width(ToGame(candidate)) > width)
            {
                lines.push_back(ToGame(line));
                line = word;
            }
            else
                line = candidate;
            start = end + 1;
        }
        if (!line.empty())
            lines.push_back(ToGame(line));
        return lines;
    }

    // The volume slider of the OPTIONS page, for numbers with a few steps: the game draws it later in the frame from
    // the row's pending value, so that value is swapped in while it draws
    struct SliderData
    {
        uint8_t row;
        uint8_t colour;
        uint8_t value;
    } Sliders[VisibleRows + 1];

    void __cdecl DrawSlider(void* data)
    {
        auto slider = static_cast<SliderData*>(data);
        int8_t saved = (*aOptionsPendingValues)[slider->row];
        (*aOptionsPendingValues)[slider->row] = slider->value;
        SliderDraw(data);
        (*aOptionsPendingValues)[slider->row] = saved;
    }

    constexpr int SliderTicks = 16;

    // The number of positions of a number shown as a slider, 0 for other rows
    int SliderPositions(const RWGTAMenu::Row& row)
    {
        if (row.kind != RowKind::Setting)
            return 0;
        auto& s = *row.setting;
        bool number = s.type == asi_settings::Type::Int || s.type == asi_settings::Type::Float;
        if (!number || !s.min || !s.max || !s.step || *s.step <= 0 || !s.special.empty() || !s.suggestions.empty())
            return 0;
        int positions = int(std::lround((*s.max - *s.min) / *s.step)) + 1;
        return positions >= 2 && positions <= SliderTicks ? positions : 0;
    }

    // The slider's tick of the value in effect, 0..15
    int SliderTick(const RWGTAMenu::Row& row)
    {
        auto& s = *row.setting;
        int positions = SliderPositions(row);
        double value = std::atof(OtherPlugins::Get(*row.plugin, s).c_str());
        int position = std::clamp(int(std::lround((value - *s.min) / *s.step)), 0, positions - 1);
        return int(std::lround(double(position) * (SliderTicks - 1) / (positions - 1)));
    }

    // Sets a slider's value from a tick; true when it changed
    bool SetSliderTick(const RWGTAMenu::Row& row, int tick)
    {
        auto& s = *row.setting;
        int positions = SliderPositions(row);
        int position = int(std::lround(double(tick) * (positions - 1) / (SliderTicks - 1)));
        auto value = asi_settings::text::FormatNumber(*s.min + position * *s.step);
        if (!OtherPlugins::Enabled(*row.plugin, s, false) || value == OtherPlugins::Get(*row.plugin, s))
            return false;
        return OtherPlugins::Set(*row.plugin, s, value) == ASI_SETTINGS_OK;
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
    }

    void RememberRestartValues(const asi_settings::LoadedPlugin& plugin)
    {
        for (auto& setting : plugin.file.description.settings)
        {
            if (setting.HasValue() && setting.apply == "restart")
                RestartValues.try_emplace({ reinterpret_cast<uintptr_t>(plugin.module), setting.id }, OtherPlugins::Get(plugin, setting));
        }
    }

    // A restart setting differs from the value the game started with
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

    void OpenPage(int page)
    {
        ListRow = Row;
        View = page;
        Row = 1;
        Scroll = 0;
        Model.PageShown(size_t(page));
        ShowRows();
    }

    void CloseView()
    {
        View = -1;
        Row = ListRow;
    }

    void OpenList()
    {
        HMODULE self = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCWSTR>(&OpenList), &self);
        Model.OpenPlugins(self, LanguageTag(), PageRows, {}, true);
        for (auto& page : Model.Pages())
        {
            if (page.plugin)
                RememberRestartValues(*page.plugin);
        }
        View = -1;
        Row = ListRow = 1;
        Scroll = 0;
        PageActive = true;
        Sound(SoundMove);
    }

    void LeavePage(int nativeRow)
    {
        PageActive = HelpActive = false;
        Model.Close();
        Rows.clear();
        nOptionsSelection = nOptionsSelectionNext = int8_t(nativeRow);
    }

    // The native title arrows, shared by all pages
    int TitleArrowWidth()
    {
        return std::max({ MsgWidth(45), MsgWidth(46), MsgWidth(136) });
    }

    int __cdecl Frame()
    {
        OptionsSeen = true;
        if (!PageActive || nOptionsMode != 0)
        {
            if (PageActive)
            {
                PageActive = false;
                Model.Close();
            }
            int result = FrameHook.ccall<int>();
            if (!UpdateNotice.empty() && nOptionsMode == 0)
            {
                TextLayerFront();
                SetGlyphSize(18, 18);
                DrawText(1, FitLine(ToGame(UpdateNotice), 440), 256, 425, true);
                ClearGlyphSize();
                TextLayerBack();
            }
            return result;
        }

        TextReset();
        ReadOptInput();
        nOptionsModeNext = nOptionsMode.get();
        nOptionsSelectionNext = nOptionsSelection.get();
        SetTextBg((*aTextBackground)[0], (*aTextBackground)[1], (*aTextBackground)[2]);
        TextLayerFront();

        // Rows that appear or disappear with other values (visibleIf) follow at once
        if (View >= 0)
            ShowRows();
        int rows = RowCount();
        Row = std::clamp(Row, 1, rows);
        Scroll = std::clamp(Scroll, std::max(0, Row - VisibleRows), Row - 1);
        int shown = std::min(rows - Scroll, VisibleRows);
        const RWGTAMenu::Row* selected = View >= 0 && Row < rows ? Rows[Row - 1] : nullptr;
        int action = nOptionsAction;
        bool leave = false;
        const auto pad = SHPad::Get();
        static bool helpHeld = false;
        bool held = (pad.buttons & SHPad::Y) != 0;
        bool helpPressed = HelpRequested || (held && !helpHeld);
        HelpRequested = false;
        helpHeld = held;
        if (helpPressed && !HelpActive && (selected || !UpdateNotice.empty()))
        {
            HelpText = selected ? Label(*selected) + ": " + Help(*selected) : UpdateNotice;
            HelpActive = true;
            HelpScroll = 0;
            Sound(SoundAccept);
        }
        if (HelpActive)
        {
            auto lines = Wrap(HelpText, 470);
            int maximum = std::max(0, int(lines.size()) - 12);
            if (action == 3) --HelpScroll;
            if (action == 4) ++HelpScroll;
            HelpScroll = std::clamp(HelpScroll, 0, maximum);
            if (action == 1 || action == 2 || helpPressed)
            {
                // The opening help press belongs only to this view.
                if (action == 1 || action == 2)
                    HelpActive = false;
            }
            OptBackground();
            DrawText(4, ToGame(Model.Text("menu.help")), 256, 56, true);
            SetGlyphSize(20, 20);
            for (int i = 0; i < 12 && i + HelpScroll < int(lines.size()); ++i)
                DrawText(1, lines[i + HelpScroll], 256, 105 + i * 21, true);
            ClearGlyphSize();
            DrawText(5, ToGame(Model.Text("menu.back")), 256, 369, true);
            SetGlyphSize(18, 18);
            if (RestartRequired())
                DrawText(1, FitLine(ToGame(Model.Text("menu.restartwarning")), 480), 256, 425, true);
            ClearGlyphSize();
            ClearRects();
            AddRect(196, 365, 316, 395, 1);
            if (HitTest() == 1 && MouseClicked())
                HelpActive = false;
            TextLayerBack();
            return 0;
        }

        // Mouse: rows, the title arrows and the slider
        ClearRects();
        int arrowWidth = TitleArrowWidth();
        AddRect(241 - arrowWidth / 2 - 30, 54, 241 - arrowWidth / 2, 82, 31);
        AddRect(arrowWidth / 2 + 271, 54, arrowWidth / 2 + 301, 82, 30);
        bool sliderSelected = selected && SliderPositions(*selected);
        if (sliderSelected)
            SliderRects(8, RowY(Slot(Row)) - 248);
        else
        {
            for (int slot = 1; slot <= shown; ++slot)
            {
                int row = slot + Scroll, y = RowY(slot);
                if (View >= 0 && row < rows)
                {
                    int labelWidth = Width(FitLine(ToGame(Label(*Rows[row - 1])), 228));
                    AddRect(244 - labelWidth, y, 268 + 200, y + 25, slot);
                }
                else
                {
                    int w = row == rows && View < 0 ? MsgWidth(MsgExit) : 200;
                    AddRect(256 - w / 2, y, 256 + w / 2, y + 25, slot);
                }
            }
        }
        int hit = HitTest();
        if ((hit == 30 || hit == 31) && MouseClicked())
            action = hit == 30 ? 7 : 8;
        else if (sliderSelected && hit > 0 && hit <= SliderTicks && MouseHeld())
        {
            if (SetSliderTick(*selected, hit - 1))
                Sound(SoundSlider);
        }
        else if (!sliderSelected && hit > 0 && hit <= shown && MouseMoved() && hit + Scroll != Row)
        {
            Row = hit + Scroll;
            Sound(SoundMove);
        }

        switch (action)
        {
        case 2:     // Cancel: from a page back to the list, from the list out of Options like the native pages
            if (View >= 0)
            {
                CloseView();
                Sound(SoundMove);
            }
            else
                leave = true;
            break;
        case 3:
            Row = Row > 1 ? Row - 1 : rows;
            Sound(SoundMove);
            break;
        case 4:
            Row = Row < rows ? Row + 1 : 1;
            Sound(SoundMove);
            break;
        case 7:     // next page: OPTIONS
            Sound(SoundMove);
            LeavePage(1);
            break;
        case 8:     // previous page: Configuration
            Sound(SoundMove);
            LeavePage(22);
            break;
        case 1:
        case 5:
        case 6:
            if (Row == rows)
            {
                if (action == 1)
                {
                    Sound(SoundAccept);
                    if (View >= 0)
                        CloseView();
                    else
                        leave = true;
                }
            }
            else if (View < 0)
            {
                if (action == 1)
                {
                    Sound(SoundAccept);
                    OpenPage(Row - 1);
                }
            }
            else if (sliderSelected)
            {
                if (action != 1 && Model.Change(*selected, action == 5 ? 1 : -1, false))
                    Sound(SoundSlider);
            }
            else if (Model.Change(*selected, action == 1 ? 0 : action == 5 ? 1 : -1, false))
                Sound(action == 1 ? SoundAccept : SoundChange);
            break;
        }

        if (PageActive)
        {
            if (View >= 0)
                ShowRows();
            rows = RowCount();
            Row = std::clamp(Row, 1, rows);
            Scroll = std::clamp(Scroll, std::max(0, Row - VisibleRows), Row - 1);
            shown = std::min(rows - Scroll, VisibleRows);
            selected = View >= 0 && Row < rows ? Rows[Row - 1] : nullptr;

            // Background and the native title arrows; the native title itself is hidden by the DrawOptText hook
            OptBackground();
            // The title between the arrows, narrower when it's longer than the game's titles
            auto title = ToGame(View < 0 ? Model.Text("menu.title") : Model.Pages()[View].title);
            SetGlyphSize(30, 30);
            int titleWidth = Width(title), room = arrowWidth + 20;
            if (titleWidth > room)
                SetGlyphSize(std::max(16, 30 * room / titleWidth), 30);
            DrawText(4, title, 256, 56, true);
            ClearGlyphSize();

            for (int slot = 1; slot <= shown; ++slot)
            {
                int row = slot + Scroll;
                int colour = row == Row ? 5 : 1;
                int y = RowY(slot);
                if (row == rows)
                {
                    if (View < 0)
                        DrawOptText(colour, slot, 0, MsgExit);
                    else
                        DrawText(colour, ToGame(Model.Text("menu.back")), 256, y, true);
                    continue;
                }
                if (View < 0)
                {
                    DrawText(colour, ToGame(Model.Pages()[row - 1].title), 256, y, true);
                    continue;
                }
                auto& entry = *Rows[row - 1];
                auto label = FitLine(ToGame(Label(entry)), 228);
                // Labels never show raised except when selected, like the native rows
                DrawText(colour, label, 244 - Width(label), y);
                if (SliderPositions(entry))
                {
                    auto& slider = Sliders[slot];
                    slider.row = uint8_t(slot);
                    slider.colour = uint8_t(colour);
                    slider.value = uint8_t(SliderTick(entry));
                    QueueDraw(reinterpret_cast<void*>(&DrawSlider), &slider, 1);
                }
                else if (entry.kind == RowKind::Setting && entry.setting->type == asi_settings::Type::Bool)
                    DrawOptText(colour, slot, 4, OtherPlugins::Get(*entry.plugin, *entry.setting) == "1" ? MsgOn : MsgOff);
                else
                    DrawText(colour, FitLine(ToGame(Model.Value(entry)), 228), 268, y);
            }

            SetGlyphSize(18, 18);
            if (RestartRequired())
                DrawText(1, FitLine(ToGame(Model.Text("menu.restartwarning")), 480), 256, 425, true);
            ClearGlyphSize();

            // The complete description opens separately; the footer remains a hint.
            SetGlyphSize(18, 18);
            DrawText(1, ToGame(Model.Text("menu.helphint")), 256, 401, true);
            ClearGlyphSize();
            // Help line of the selected row
            if (selected)
            {
                auto lines = Wrap(Help(*selected), 480);
                for (size_t i = 0; i < lines.size() && i < 1; ++i)
                    DrawText(1, lines[i], 256, 374 + int(i) * 20, true);
            }
            else if (View < 0 && Row == rows)
                DrawOptText(1, 0, 5, MsgExitHelp);
        }

        TextLayerBack();

        if (leave)
        {
            // Native exit: the confirm dialog for native changes (it closes at once when there are none)
            LeavePage(10);
            nOptionsMode = nOptionsModeNext = 3;
        }
        return 0;
    }

    template<class T>
    void Resolve(T& target, const char* signature, int offset = 0)
    {
        target = reinterpret_cast<T>(hook::get_pattern(signature, offset));
    }

    template<class T>
    void ResolveCall(T& target, const char* signature, int offset)
    {
        target = reinterpret_cast<T>(injector::GetBranchDestination(hook::get_pattern(signature, offset)).as_int());
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

            auto frame = hook::pattern("53 56 57 E8 ? ? ? ? E8 ? ? ? ? A0 ? ? ? ? 8A 0D ? ? ? ?");
            nOptionsMode.SetAddress(*frame.get_first<int8_t*>(0x0E));
            nOptionsSelection.SetAddress(*frame.get_first<int8_t*>(0x14));
            nOptionsSelectionNext.SetAddress(*frame.get_first<int8_t*>(0x14) + 1);
            nOptionsModeNext.SetAddress(*frame.get_first<int8_t*>(0x0E) + 1);
            nOptionsAction.SetAddress(*frame.get_first<int8_t*>(0x0E) + 2);
            aOptionsPendingValues.SetAddress(*frame.get_first<int8_t*>(0x14) + 0x27);
            TextReset = reinterpret_cast<decltype(TextReset)>(injector::GetBranchDestination(frame.get_first(3)).as_int());
            ReadOptInput = reinterpret_cast<decltype(ReadOptInput)>(injector::GetBranchDestination(frame.get_first(8)).as_int());

            auto frameStart = frame.get_first<uint8_t>();
            aTextBackground.SetAddress(*reinterpret_cast<uint8_t**>(frameStart + 0x36));     // mov cl, [text background red]
            SetTextBg = reinterpret_cast<decltype(SetTextBg)>(injector::GetBranchDestination(frameStart + 0x3D).as_int());
            TextLayerFront = reinterpret_cast<decltype(TextLayerFront)>(injector::GetBranchDestination(frameStart + 0x45).as_int());

            Resolve(TextLayerBack, "B8 04 00 00 00 66 39 05 ? ? ? ? 7D");
            Resolve(OptBackground, "A0 ? ? ? ? 83 EC 14 3C 01 0F 84");
            Resolve(DrawOptTextFn, "51 53 56 8B D8 85 DB 57 8B F1 8B FA");
            Resolve(SetColor, "0F B6 4C 24 04 0F B6 54 24 10 33 C0");
            Resolve(Encode, "83 EC 0C 8B 44 24 10 53 8A 18 55 56");
            // DrawGlyphs and MeasureLines are bound in Init, before the button icons hook them
            Resolve(CentreOn, "81 0D ? ? ? ? 00 01 00 00 C3 90");
            Resolve(CentreOff, "81 25 ? ? ? ? FF FE FF FF C3 90 90 90 90 90 81");
            Resolve(SetGlyphSize, "A1 ? ? ? ? 66 8B 4C 24 08 0D 00");
            Resolve(ClearGlyphSize, "81 25 ? ? ? ? FF F7 FF FF C3 90");
            Resolve(MsgWidth, "83 EC 20 57 33 C0 B9 08 00 00 00 8D");
            Resolve(ClearRects, "A1 ? ? ? ? 85 C0 74 1A 56 8D 9B");
            Resolve(AddRect, "E8 ? ? ? ? 6A 18 E8 ? ? ? ? 8B");
            Resolve(HitTest, "E8 ? ? ? ? 85 C0 75 01 C3 56 E8");
            Resolve(MouseMoved, "8B 0D ? ? ? ? 33 C0 83 F9 03 0F");
            ResolveCall(SliderRects, "6A FE 6A 08 E8 ? ? ? ? 83 C4 08 E8 ? ? ? ? 85 C0 74 2D E8", 4);
            ResolveCall(MouseHeld, "6A FE 6A 08 E8 ? ? ? ? 83 C4 08 E8 ? ? ? ? 85 C0 74 2D E8", 0x0C);
            auto slider = hook::pattern("6A 01 68 ? ? ? ? 68 ? ? ? ? C6 05 ? ? ? ? 07 88 1D ? ? ? ? E8");
            SliderDraw = *slider.get_first<decltype(SliderDraw)>(8);
            QueueDraw = reinterpret_cast<decltype(QueueDraw)>(injector::GetBranchDestination(slider.get_first(0x19)).as_int());
            auto colours = hook::pattern("33 C9 8A 0C 85 ? ? ? ? 33 D2 8A");
            aTextColours.SetAddress(*colours.get_first<uint8_t*>(5) - 3);
            MouseClicked = reinterpret_cast<decltype(MouseClicked)>(injector::GetBranchDestination(frameStart + 0xEE).as_int());

            auto pageNext = hook::pattern("53 8A 1D ? ? ? ? 0F BE C3 33 C9 83 F8 0A 7E 07 83 F8 15 0F 9F C1 41 83 E9 00 74 24");
            auto pagePrev = hook::pattern("53 8A 1D ? ? ? ? 0F BE C3 33 C9 83 F8 0A 7E 07 83 F8 15 0F 9F C1 41 83 E9 00 74 2B");
            PlaySE = reinterpret_cast<decltype(PlaySE)>(injector::GetBranchDestination(pageNext.get_first(0x68)).as_int());
            auto optInitEnd = hook::pattern("C6 05 ? ? ? ? 00 C7 05 ? ? ? ? 00 00 00 00 C6 05 ? ? ? ? 00 5B C3");

            FrameHook = safetyhook::create_inline(frame.get_first(), &Frame);
            SHDisplay::OnWindowMessage().push_back([](HWND, UINT message, WPARAM key, LPARAM flags, LRESULT& result)
            {
                if (PageActive && message == WM_KEYDOWN && key == VK_F1)
                {
                    if (!(flags & (1L << 30)))
                        HelpRequested = true;
                    result = 0;
                    return true;
                }
                return false;
            });
            RWGTA::UpdateCheck::Config updates;
            updates.pluginId = "SilentHill3.FusionFix";
            updates.fileVersion = rsc_FileVersion;
            updates.settings = &FixSettings();
            updates.playable = []() { return OptionsSeen && nOptionsMode == 0; };
            updates.show = [](const std::wstring& notice) { UpdateNotice = asi_settings::text::Narrow(notice); };
            updates.format = [](const std::string& version, bool hub)
            {
                auto texts = FixSettings().GetTexts();
                texts.Select(LanguageTag());
                auto line = texts.Resolve(hub ? "@update.availablehub" : "@update.availablesite");
                auto replace = [&line](const char* key, const std::string& value)
                {
                    const auto at = line.find(key);
                    if (at != std::string::npos)
                        line.replace(at, strlen(key), value);
                };
                replace("{name}", texts.Resolve(FixSettings().GetDescription().name));
                replace("{version}", version);
                return asi_settings::text::Widen(line);
            };
            RWGTA::UpdateCheck::Init(std::move(updates));
            SHDisplay::OnPresent().push_back([]() { RWGTA::UpdateCheck::Tick(); });

            // Page Up / Page Down switch pages too (the game only uses its step keys and the shoulder buttons)
            ReadInputHook = safetyhook::create_inline(ReadOptInput, static_cast<void(__cdecl*)()>([]()
            {
                ReadInputHook.ccall<void>();
                static bool previousHeld = false, nextHeld = false, helpHeld = false;
                DWORD process = 0;
                GetWindowThreadProcessId(GetForegroundWindow(), &process);
                bool focused = process == GetCurrentProcessId();
                bool previous = focused && (GetAsyncKeyState(VK_PRIOR) & 0x8000);
                bool next = focused && (GetAsyncKeyState(VK_NEXT) & 0x8000);
                bool help = focused && (GetAsyncKeyState(VK_F1) & 0x8000);
                if (PageActive && help && !helpHeld)
                    HelpRequested = true;
                if (previous && !previousHeld)
                    nOptionsAction = 8;
                else if (next && !nextHeld)
                    nOptionsAction = 7;
                previousHeld = previous;
                nextHeld = next;
                helpHeld = help;
            }));

            // From Configuration the next page is ours, and from OPTIONS the previous one
            PageNextHook = safetyhook::create_inline(pageNext.get_first(), static_cast<int(__cdecl*)()>([]() -> int
            {
                if (nOptionsSelection > 21 && !PageActive)
                {
                    OpenList();
                    return 0;
                }
                return PageNextHook.ccall<int>();
            }));
            PagePrevHook = safetyhook::create_inline(pagePrev.get_first(), static_cast<int(__cdecl*)()>([]() -> int
            {
                if (nOptionsSelection <= 10 && !PageActive)
                {
                    OpenList();
                    return 0;
                }
                return PagePrevHook.ccall<int>();
            }));

            // The native page title isn't drawn under ours
            DrawOptTextHook = safetyhook::create_mid(DrawOptTextFn, [](SafetyHookContext& ctx)
            {
                if (PageActive && ctx.ecx == 1)
                    ctx.eax = 0;
            });

            // Options always opens on the native first page
            OptInitHook = safetyhook::create_mid(optInitEnd.get_first(0x11), [](SafetyHookContext&)
            {
                PageActive = HelpActive = false;
                Model.Close();
            });
        };
    }
} Menu;
