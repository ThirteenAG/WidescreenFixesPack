module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/RWGTA/FixMenu.hpp"
#include "Shared/PadInput.hpp"
#include "Shared/MenuList.hpp"

export module Menu;

import ComVars;

// The fix's settings in the game's Options (pause) and Video (title) screens: "Plugins" opens the list of plugins (the
// fix first, then the other plugins with ASI Settings, see Shared/RWGTA/FixMenu.hpp), drawn with the screens' own font,
// colours and layout. A plugin opens its settings with their values, or, when they come in several categories, the
// list of them, each opening its settings. Every list scrolls by a row through the seven rows the screen shows (Up/Down
// wrap at its ends, Page Up/Page Down and the shoulder buttons move by a screen, Home/End go to the ends, the wheel
// scrolls), small arrows mark the rows above and below, and Back returns to the row a list was opened from.
namespace BullyMenu
{
    using ScreenFn = void(__thiscall*)(void*);
    static ScreenFn optionsEnter, optionsLeave, optionsProcess, displayEnter, displayLeave, displayProcess, displayDraw;
    static void* optionsDraw;
    static SafetyHookInline headingHook, hintsHook;
    static std::string updateNotice;
    static ULONGLONG noticeUntil = 0;
    static int(__cdecl* drawText)(int, int, const wchar_t*, int, int, int, char);
    static int(__cdecl* style)(int, int, int, char);
    static double(__cdecl* measure)(const wchar_t*, int);
    static int(__cdecl* bounds)(float*, float, float, const wchar_t*);
    static char(__cdecl* pointerAccept)(int, int, int);
    static char(__stdcall* accept)(int);
    static char(__stdcall* back)(int);
    static BOOL(__cdecl* repeat)(int);
    static void(__cdecl* consume)();
    static void(__cdecl* delay)(int);
    static void(__cdecl* controls)(const char*);
    static void(__cdecl* background)();
    static void(__cdecl* divider)(float, float, float, void*, float);
    static float previousMouseX, previousMouseY;
    static bool mouseMode;
    static RWGTAMenu::Model model;
    export std::function<bool()> UsingPad, PadHelp, BlocksInput;
    static void* owner;
    static bool paused;
    static bool showingHelp = false, helpKeyDown = false;
    static size_t helpFirst = 0;
    static size_t page = RWGTAMenu::Model::RootPage;   // the model's page shown
    constexpr size_t Rows = 5;          // rows shown at once
    // The Video screen's rows: their font, the first one's height and the distance between them
    constexpr int RowFont = 3;
    constexpr float RowTop = 172.0f, RowStep = 40.0f;
    float RowY(size_t slot) { return RowTop + RowStep * float(slot); }
    static MenuList::WheelNotches wheelNotches;
    static MenuList::Repeat pageUpKey, pageDownKey;
    static bool homeDown, endDown;

    template<class T> T& Field(void* object, size_t offset)
    {
        return *reinterpret_cast<T*>(static_cast<uint8_t*>(object) + offset);
    }

    static std::string Language()
    {
        auto override = FixSettings().String("MAIN.TextLanguage");
        if (!override.empty() && override != "auto")
            return override;
        static constexpr const char* languages[] = { "en", "fr", "de", "it", "es", "en", "ru", "ja" };
        return MenuLanguage >= 0 && MenuLanguage < int(std::size(languages)) ? languages[MenuLanguage] : "en";
    }

    static std::wstring FontText(const std::string& text, size_t limit = 180)
    {
        // The native font takes UTF-16. Keep each draw below the native formatter's 512-WORD buffer.
        auto count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), int(text.size()), nullptr, 0);
        if (count <= 0)
            return {};
        std::wstring out(count, L'\0');
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), int(text.size()), out.data(), count);
        if (out.size() > limit)
            out.resize(limit);
        for (auto& c : out)
        {
            if (c == L'~') c = L'-'; // Literal descriptor text must not become native formatter commands.
            if (c >= 0xD800 && c <= 0xDFFF) c = L'?';
        }
        return out;
    }

    static std::string Text(const char* key)
    {
        return RWGTAMenu::PluginText(FixSettings(), Language(), key);
    }

    static std::string HelpLabel()
    {
        if (!UsingPad || !UsingPad())
            return Text("menu.help");
        auto label = Text("menu.help.pad");
        const auto family = PadInput::ResolveFamily(FixSettings().Int("MAIN.GamepadIcons"));
        const auto button = family == PadInput::Family::PlayStation ? Text("button.triangle") :
            family == PadInput::Family::Nintendo ? "X" : "Y";
        if (auto position = label.find("{button}"); position != std::string::npos)
            label.replace(position, 8, button);
        return label;
    }

    enum Casing { AsWritten, Capitals, TitleCase };

    // The game's look: headings and values in capitals, English row labels in title case ("Vertical Sync")
    static void ApplyCasing(std::wstring& text, Casing casing)
    {
        if (casing == Capitals)
        {
            if (!text.empty())
                CharUpperBuffW(text.data(), DWORD(text.size()));
            return;
        }
        if (casing != TitleCase || Language() != "en")
            return;
        static const std::wstring minorWords[] = { L"a", L"an", L"and", L"at", L"by", L"for", L"in", L"of", L"on", L"or", L"the", L"to" };
        size_t start = 0;
        while (start < text.size())
        {
            size_t end = text.find(L' ', start);
            if (end == std::wstring::npos)
                end = text.size();
            auto word = text.substr(start, end - start);
            bool minor = start != 0 && std::find(std::begin(minorWords), std::end(minorWords), word) != std::end(minorWords);
            if (!word.empty() && !minor)
                CharUpperBuffW(&text[start], 1);
            start = end + 1;
        }
    }

    static float ValueX()
    {
        // Right edge of the Video screen's own values
        return (bWidescreen ? 853.0f : 640.0f) - 102.0f;
    }

    static void Print(float x, float y, const std::string& text, bool highlight = false, bool enabled = true, int align = 0, float width = 440.0f, int font = 1, Casing casing = AsWritten)
    {
        auto converted = FontText(text);
        ApplyCasing(converted, casing);
        int color = !enabled ? 2 : highlight ? 3 : 11;
        style(color, align, font, 0);
        while (!converted.empty() && measure(converted.c_str(), 1) > width * MenuScaleX)
            converted.pop_back();
        drawText(int(x * MenuScaleX), int(y), converted.c_str(), color, align, font, 0);
    }

    static std::vector<std::wstring> Wrap(const std::string& text, float width, size_t maximum = 2)
    {
        std::vector<std::wstring> lines;
        auto converted = FontText(text, 4096);
        style(11, 0, 1, 0);
        float limit = width * MenuScaleX;
        size_t start = 0;
        while (start < converted.size() && lines.size() < maximum)
        {
            while (start < converted.size() && converted[start] == L' ')
                ++start;
            size_t end = converted.size();
            if (measure(converted.substr(start).c_str(), 1) > limit)
            {
                // The last space that keeps the line inside the width
                end = start;
                for (size_t i = start; i <= converted.size(); ++i)
                {
                    if (i == converted.size() || converted[i] == L' ')
                    {
                        if (i > start && measure(converted.substr(start, i - start).c_str(), 1) > limit)
                            break;
                        end = i;
                    }
                }
                if (end == start)
                {
                    end = start + 1;
                    while (end < converted.size() && measure(converted.substr(start, end - start + 1).c_str(), 1) <= limit)
                        ++end;
                }
            }
            auto line = converted.substr(start, end - start);
            if (lines.size() + 1 == maximum && end < converted.size())
            {
                while (!line.empty() && measure((line + L"...").c_str(), 1) > limit)
                    line.pop_back();
                line += L"...";
            }
            lines.push_back(line);
            start = end;
        }
        return lines;
    }

    static int __cdecl NativeHeading(int key)
    {
        // Pause draws this heading outside the screen callback; the plugin page supplies its own.
        if (owner && paused) return 0;
        return headingHook.ccall<int>(key);
    }

    export bool NoticeReady()
    {
        return drawText && style && measure && MenuLanguage.is_initialized();
    }

    export void ShowNotice(const std::wstring& line)
    {
        updateNotice = asi_settings::text::Narrow(line);
        noticeUntil = 0;
    }

    static void __cdecl DrawHints(const char* key)
    {
        hintsHook.ccall<void>(key);
        if (updateNotice.empty() || showingHelp)
            return;
        const auto now = GetTickCount64();
        if (!noticeUntil)
            noticeUntil = now + 20000;
        if (now >= noticeUntil)
        {
            updateNotice.clear();
            return;
        }
        auto lines = Wrap(updateNotice, ValueX() - 100.0f);
        for (size_t i = 0; i < lines.size(); ++i)
        {
            style(8, 0, 1, 0);
            drawText(int(100.0f * MenuScaleX), (owner ? 352 : 400) + int(i) * 18, lines[i].c_str(), 8, 0, 1, 0);
        }
    }

    static void Close()
    {
        model.Close();
        owner = nullptr;
        page = RWGTAMenu::Model::RootPage;
        showingHelp = helpKeyDown = false;
        helpFirst = 0;
        mouseMode = false;
    }

    static void Open(void* screen, bool pause)
    {
        Close();
        HMODULE module = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&Open), &module);
        model.OpenMenu(module, Language(), Rows);
        owner = screen;
        MenuWheel.exchange(0);
        wheelNotches.Clear();
        paused = pause;
        previousMouseX = MenuMouseX;
        previousMouseY = MenuMouseY;
        consume();
    }

    static void UpdateMouse()
    {
        if (std::isfinite(MenuMouseX.get()) && std::isfinite(MenuMouseY.get()) && (MenuMouseX != previousMouseX || MenuMouseY != previousMouseY))
            mouseMode = true;
        previousMouseX = MenuMouseX;
        previousMouseY = MenuMouseY;
    }

    static bool Hover(float x, float y, const std::string& text, int align = 0, float width = 440.0f, Casing casing = AsWritten, int font = 1)
    {
        if (!mouseMode) return false;
        auto converted = FontText(text);
        ApplyCasing(converted, casing);
        style(3, align, font, 0);
        while (!converted.empty() && measure(converted.c_str(), font) > width * MenuScaleX)
            converted.pop_back();
        float rectangle[4] = {};
        // Use the same centered coordinates and native font bounds as the stock title mouse hit-test.
        bounds(rectangle, (x * MenuScaleX - (bWidescreen ? 853.0f : 640.0f) * 0.5f) * MenuScaleX,
            (y - 240.0f) * MenuScaleY, converted.c_str());
        return MenuMouseX >= rectangle[0] && MenuMouseX <= rectangle[2] && MenuMouseY >= rectangle[1] && MenuMouseY <= rectangle[3];
    }

    // The rows of the page shown, in its window
    static std::vector<const RWGTAMenu::Row*> ShownRows()
    {
        return model.Shown(page, false);
    }

    static MenuList::Window& Window()
    {
        return model.Window(page);
    }

    // The row a list shows at the selection (null when the list is empty)
    static const RWGTAMenu::Row* SelectedRow()
    {
        auto rows = model.ListRows(page, false);
        return Window().Selected() < rows.size() ? rows[Window().Selected()] : nullptr;
    }

    // Plugin and category rows open a list; they have no value
    static bool HasValue(const RWGTAMenu::Row* row)
    {
        return row && !RWGTAMenu::Model::Opens(*row);
    }

    static void ProcessPage()
    {
        if (BlocksInput && BlocksInput())
        {
            helpKeyDown = (GetAsyncKeyState(VK_F1) & 0x8000) != 0;
            return;
        }
        UpdateMouse();
        bool pointer = pointerAccept(28, 0, 1) != 0;
        const bool helpKey = (GetAsyncKeyState(VK_F1) & 0x8000) != 0;
        const bool helpPressed = helpKey && !helpKeyDown;
        helpKeyDown = helpKey;
        auto current = SelectedRow();
        const bool help = HasValue(current);
        if (help && (helpPressed || (PadHelp && PadHelp()) || (pointer && Hover(ValueX(), 420, HelpLabel(), 1))))
        {
            showingHelp = !showingHelp;
            helpFirst = 0;
            consume();
            return;
        }
        if (back(0) || (pointer && Hover(100, 395, Text("menu.back"))))
        {
            consume();
            if (showingHelp)
            {
                showingHelp = false;
                return;
            }
            // To the list the page was opened from, on its row, or out of the plugins list
            size_t parent = model.Back(page);
            if (parent == SIZE_MAX)
                Close();
            else
                page = parent;
            return;
        }
        auto& window = Window();
        auto rows = ShownRows();
        if (rows.empty())
            return;
        if (showingHelp)
        {
            const auto lines = current ? Wrap(model.Help(*current), ValueX() - 100.0f, 256) : std::vector<std::wstring>{};
            if (repeat(0) && (MenuUp || MenuDown))
            {
                const auto last = lines.size() > 12 ? lines.size() - 12 : 0;
                helpFirst = MenuUp ? (helpFirst ? helpFirst - 1 : 0) : std::min(helpFirst + 1, last);
                delay(0);
            }
            return;
        }
        // The wheel scrolls the list by a row per notch; the selection stays on its row while it is shown
        for (int notches = wheelNotches.Take(MenuWheel.exchange(0)); notches; notches += notches > 0 ? -1 : 1)
            window.Scroll(notches > 0 ? -1 : 1);
        rows = ShownRows();
        bool rowHovered = false;
        for (size_t slot = 0; slot < rows.size(); ++slot)
        {
            float y = RowY(slot);
            if (Hover(100, y, model.Label(*rows[slot]), 0, HasValue(rows[slot]) ? 300.0f : 440.0f, TitleCase, RowFont)
                || (HasValue(rows[slot]) && Hover(ValueX(), y, model.Value(*rows[slot]), 1, 220.0f, Capitals, RowFont)))
            {
                window.Point(slot);
                rowHovered = true;
                break;
            }
        }
        if (repeat(0) && (MenuUp || MenuDown))
        {
            window.Move(MenuUp ? -1 : 1, true);
            delay(0);
            mouseMode = false;
        }
        // A screen of rows up or down, or to the ends
        const auto now = GetTickCount64();
        XINPUT_STATE pad{};
        DWORD padSlot = 0;
        WORD buttons = UsingPad && UsingPad() && PadInput::Read(pad, padSlot) ? pad.Gamepad.wButtons : 0;
        auto held = [](int key) { return (GetAsyncKeyState(key) & 0x8000) != 0; };
        if (pageUpKey.Step(held(VK_PRIOR) || (buttons & XINPUT_GAMEPAD_LEFT_SHOULDER), now) && window.Page(-1))
            mouseMode = false;
        if (pageDownKey.Step(held(VK_NEXT) || (buttons & XINPUT_GAMEPAD_RIGHT_SHOULDER), now) && window.Page(1))
            mouseMode = false;
        bool home = held(VK_HOME), end = held(VK_END);
        if (home && !homeDown && window.Edge(-1))
            mouseMode = false;
        if (end && !endDown && window.Edge(1))
            mouseMode = false;
        homeDown = home;
        endDown = end;
        bool pressed = accept(0) != 0;
        if (pointer && !rowHovered) { consume(); return; }
        int direction = repeat(0) ? (MenuLeft ? -1 : MenuRight ? 1 : 0) : 0;
        if (!pressed && !direction)
            return;
        if (auto row = SelectedRow())
        {
            if (RWGTAMenu::Model::Opens(*row))
            {
                if (pressed)
                {
                    size_t next = model.Enter(*row);
                    if (next != SIZE_MAX)
                        page = next;
                }
            }
            else
            {
                model.Change(*row, pressed ? 0 : direction, paused);
            }
        }
        if (pressed) consume();
        else delay(0);
    }

    static void DrawPage()
    {
        // Repaint the same native menu background before replacing its heading/content.
        background();
        auto title = FontText(model.Pages()[page].title);
        ApplyCasing(title, Capitals);
        style(8, 2, 4, 0);
        drawText(int((bWidescreen ? 853.0f : 640.0f) * 0.5f * MenuScaleX), 50, title.c_str(), 8, 2, 4, 0);
        divider(320.0f, 440.0f, 110.0f, MenuDividerColor.get_ptr(), 2.0f);
        auto current = SelectedRow();
        if (showingHelp && HasValue(current))
        {
            Print(100, 122, model.Label(*current), false, true, 0, ValueX() - 100.0f, 1, TitleCase);
            const auto lines = Wrap(model.Help(*current), ValueX() - 100.0f, 256);
            helpFirst = std::min(helpFirst, lines.size() > 12 ? lines.size() - 12 : 0);
            for (size_t i = helpFirst; i < std::min(lines.size(), helpFirst + 12); ++i)
            {
                style(11, 0, 1, 0);
                drawText(int(100.0f * MenuScaleX), 150 + int(i - helpFirst) * 18, lines[i].c_str(), 11, 0, 1, 0);
            }
            Print(100, 395, Text("menu.back"), false, true, 0, 440.0f, 1, TitleCase);
            controls("FEO_CHGCTRL");
            return;
        }
        auto& window = Window();
        auto rows = ShownRows();
        for (size_t slot = 0; slot < rows.size(); ++slot)
        {
            float y = RowY(slot);
            auto row = rows[slot];
            bool highlight = slot == window.Slot();
            bool enabled = !HasValue(row) || model.Enabled(*row, paused);
            Print(100, y, model.Label(*row), highlight, enabled, 0, HasValue(row) ? 300.0f : 440.0f, RowFont, TitleCase);
            if (HasValue(row))
                Print(ValueX(), y, model.Value(*row), highlight, enabled, 1, 220.0f, RowFont, Capitals);
        }
        // Rows above and below: small arrows of the screen's font left of the first and last row shown
        if (window.Above())
            Print(80, RowY(0), "^", false, true, 0, 20.0f, RowFont);
        if (window.Below())
            Print(80, RowY(rows.size() - 1), "v", false, true, 0, 20.0f, RowFont);
        // Like the Video screen's rows, no description under them: Help shows the selected setting's
        // A changed setting that applies at the next start: one line under the heading
        if (model.RestartRequired())
        {
            auto warning = FontText(model.Text("menu.restartwarning"));
            style(8, 2, 1, 0);
            drawText(int((bWidescreen ? 426.5f : 320.0f) * MenuScaleX), 122, warning.c_str(), 8, 2, 1, 0);
        }
        Print(100, 395, Text("menu.back"), false, true, 0, 440.0f, 1, TitleCase);
        if (HasValue(current))
            Print(ValueX(), 420, HelpLabel(), false, true, 1, 220.0f);
        controls("FEO_CHGCTRL");
    }

    struct Widget { void** vtable; uint32_t type = 2; };
    // Native callers push x, y, data offset, selected and editing; widget draw cleans exactly 0x14 bytes.
    static void __fastcall WidgetDraw(Widget*, void*, float x, float y, float, uint32_t highlight, uint32_t)
    {
        // The native caller pushes the selected flag as a bool: only its low byte is set
        Print(x / MenuScaleX, y, Text("menu.plugins"), (highlight & 0xFF) != 0, true, 0, 440.0f, 1, TitleCase);
    }
    static void __fastcall WidgetNoop(Widget*, void*) {}
    static void* widgetTable[] = { reinterpret_cast<void*>(&WidgetNoop), reinterpret_cast<void*>(&WidgetDraw),
        reinterpret_cast<void*>(&WidgetNoop), reinterpret_cast<void*>(&WidgetNoop), reinterpret_cast<void*>(&WidgetNoop),
        reinterpret_cast<void*>(&WidgetNoop), reinterpret_cast<void*>(&WidgetNoop) };
    static Widget widget{ widgetTable };

    static void __fastcall OptionsEnter(void* screen, void*)
    {
        Close();
        optionsEnter(screen);
        // Display bank has ten pointers; only two are native. Preserve every native widget.
        if (Field<uint8_t>(screen, 337) == 2)
        {
            Field<Widget*>(screen, 264) = &widget;
            Field<uint8_t>(screen, 337) = 3;
        }
    }

    static void __fastcall OptionsLeave(void* screen, void*) { Close(); optionsLeave(screen); }
    static void __fastcall OptionsProcess(void* screen, void*)
    {
        if (owner == screen) { ProcessPage(); return; }
        if (Field<Widget*>(screen, 264) == &widget && Field<uint8_t>(screen, 468) == 1 && Field<int>(screen, 340) == 2
            && !Field<uint8_t>(screen, 344) && accept(0))
        {
            Open(screen, true);
            return;
        }
        optionsProcess(screen);
    }

    static bool __cdecl OptionsDrawPage(void* screen)
    {
        if (owner != screen) return false;
        DrawPage();
        return true;
    }

    // The original draw has an EBX live-in. Tail-jump preserves it, along with ECX and the original stack.
    static void __declspec(naked) OptionsDraw()
    {
        __asm
        {
            pushfd
            pushad
            push ecx
            call OptionsDrawPage
            add esp, 4
            test al, al
            jnz handled
            popad
            popfd
            jmp dword ptr [optionsDraw]
        handled:
            popad
            popfd
            ret
        }
    }

    static void __fastcall DisplayEnter(void* screen, void*) { Close(); displayEnter(screen); }
    static void __fastcall DisplayLeave(void* screen, void*) { Close(); displayLeave(screen); }
    static void __fastcall DisplayProcess(void* screen, void*)
    {
        if (owner == screen) { ProcessPage(); return; }
        if (Field<int>(screen, 16) != 0) { displayProcess(screen); return; }
        if (Field<int>(screen, 12) == 4 && accept(0)) { Open(screen, false); return; }
        bool negative = MenuLeft || MenuUp;
        bool positive = MenuRight || MenuDown;
        bool previous = Field<uint8_t>(screen, 20) != 0;
        if (!previous && repeat(0) && (negative || positive))
        {
            auto& row = Field<int>(screen, 12);
            row = (row + (negative ? 4 : 1)) % 5;
            Field<uint8_t>(screen, 20) = 1;
            consume();
            return;
        }
        if (Field<int>(screen, 12) == 4 && !back(0))
        {
            Field<uint8_t>(screen, 20) = negative || positive;
            return;
        }
        displayProcess(screen);
    }

    static void __fastcall DisplayDraw(void* screen, void*)
    {
        if (owner == screen) { DrawPage(); return; }
        displayDraw(screen);
        // The stock HDT rows are 40 virtual pixels apart; this row follows Shadows within the content region.
        Print(float(Field<int>(screen, 1148)), 240.0f + float(Field<int>(screen, 1172)) + 40.0f,
            Text("menu.plugins"), Field<int>(screen, 12) == 4, true, 0, 440.0f, 3);
    }

    static uintptr_t Find(const char* signature)
    {
        return reinterpret_cast<uintptr_t>(hook::get_pattern(signature));
    }

    export void Init()
    {
        auto op = Find("56 8B F1 80 BE 58 01 00 00 00 57 0F 85 ? ? ? ? 8D BE 6C 01 00 00 55 8B CF");
        auto oe = Find("56 8B F1 33 C0 89 86 54 01 00 00 88 86 58 01 00 00 E8 ? ? ? ? 8D 8E 6C 01 00 00 5E E9");
        auto ol = Find("56 8B F1 E8 ? ? ? ? 80 BE 58 01 00 00 00 74 2D 8D 8E 6C 01 00 00 E8");
        auto dp = Find("51 80 3D ? ? ? ? 00 53 56 8B F1 75 09 80 3D ? ? ? ? 00 74 43 6A 00");
        auto dd = Find("83 EC 70 53 55 56 8B F1 83 7E 10 02 57 75 17 E8 ? ? ? ? 84 C0 75 0E");
        auto de = Find("33 C0 89 41 10 89 41 0C E9 ? ? ? ? CC CC CC 51 80 3D");
        auto acceptSite = Find("6A 00 B9 ? ? ? ? E8 ? ? ? ? 84 C0 74 36 8B CF E8 ? ? ? ? 8B 96 54 01");
        auto a = acceptSite + 12 + *reinterpret_cast<int32_t*>(acceptSite + 8);
        auto b = Find("56 8B 74 24 08 56 6A 08 E8 ? ? ? ? 83 C4 08 84 C0 75 13 56 6A 0A");
        auto r = Find("80 3D ? ? ? ? 00 0F B7 0D ? ? ? ? 75 07 0F B7 0D ? ? ? ? 8B 44 24 04 8B 04 85");
        auto c = Find("C7 05 ? ? ? ? 00 00 00 00 C6 05 ? ? ? ? 01 C3 CC CC CC CC CC CC CC CC");
        auto d = Find("8B 44 24 04 85 C0 B1 01 75 0B A3 ? ? ? ? 88 0D ? ? ? ? 80 3D");
        auto font = Find("8B 44 24 1C 8B 4C 24 18 8B 54 24 14 81 EC 00 04 00 00 50 8B 84 24 14 04 00 00 51 52 50 E8");
        auto coordinates = Find("D9 41 04 D8 0D ? ? ? ? D8 04 24 D9 1C 24 D9 04 24 59 C3");
        auto language = Find("81 EC 0C 04 00 00 0F BF 15 ? ? ? ? D9 05 ? ? ? ? A1 ? ? ? ? D9 5C 24 08 83 F8 07 56 57");
        auto hints = Find("51 D9 05 ? ? ? ? 6A 00 DD 05 ? ? ? ? 6A 01 D8 C9 6A 02 6A 0B D9 5C 24 10");
        auto metrics = Find("51 56 8B 74 24 0C 85 F6 74 1B 66 81 3E FF FF 75 14 8B 44 24 10 50 83 C6 02 56 E8");
        auto box = Find("8B 44 24 10 85 C0 74 1D 66 81 38 FF FF 75 16 83 C0 02 68 ? ? ? ? 50 E8");
        auto mx = Find("D9 05 ? ? ? ? D9 44 24 18 D8 D1 DF E0 F6 C4 05 7A 3A D8 44 24 0C DE");
        auto my = Find("D9 05 ? ? ? ? D9 44 24 1C D8 D9 DF E0 F6 C4 05 7A 1C D9 44 24 24 DE");
        auto wide = Find("80 3D ? ? ? ? 00 57 75 0C 68");
        auto bg = Find("83 EC 20 E8 ? ? ? ? 80 3D ? ? ? ? 00 74 0C 6A 01 6A 14 E8 ? ? ? ? 83 C4 08 8B 0D ? ? ? ? 8B 01 8B 50 18");
        auto image = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
        auto header = reinterpret_cast<IMAGE_NT_HEADERS*>(image + reinterpret_cast<IMAGE_DOS_HEADER*>(image)->e_lfanew);
        auto end = image + header->OptionalHeader.SizeOfImage;
        // The default pattern scanner ends at executable sections; native vtables are in read-only data.
        auto optionTables = hook::range_pattern(image, end,
            pattern_str(to_bytes(uint32_t(oe))) + pattern_str(to_bytes(uint32_t(ol))) + pattern_str(to_bytes(uint32_t(op))));
        auto displayTables = hook::range_pattern(image, end, pattern_str(to_bytes(uint32_t(dp))) + pattern_str(to_bytes(uint32_t(dd))));
        auto ot = optionTables.get_first<void*>(-12);
        auto dt = displayTables.get_first<void*>(-20);
        optionsEnter = reinterpret_cast<ScreenFn>(oe);
        optionsLeave = reinterpret_cast<ScreenFn>(ol);
        optionsProcess = reinterpret_cast<ScreenFn>(op);
        optionsDraw = ot[6];
        displayEnter = reinterpret_cast<ScreenFn>(de);
        displayLeave = reinterpret_cast<ScreenFn>(dt[4]);
        displayProcess = reinterpret_cast<ScreenFn>(dp);
        displayDraw = reinterpret_cast<ScreenFn>(dd);
        drawText = reinterpret_cast<decltype(drawText)>(font);
        style = reinterpret_cast<decltype(style)>(font + 34 + *reinterpret_cast<int32_t*>(font + 30));
        measure = reinterpret_cast<decltype(measure)>(metrics);
        bounds = reinterpret_cast<decltype(bounds)>(box);
        pointerAccept = reinterpret_cast<decltype(pointerAccept)>(a + 46 + *reinterpret_cast<int32_t*>(a + 42));
        accept = reinterpret_cast<decltype(accept)>(a);
        back = reinterpret_cast<decltype(back)>(b);
        repeat = reinterpret_cast<decltype(repeat)>(r);
        consume = reinterpret_cast<decltype(consume)>(c);
        delay = reinterpret_cast<decltype(delay)>(d);
        controls = reinterpret_cast<decltype(controls)>(hints);
        background = reinterpret_cast<decltype(background)>(bg);
        divider = reinterpret_cast<decltype(divider)>(dd + 105 + *reinterpret_cast<int32_t*>(dd + 101));
        MenuDividerColor.SetAddress(*reinterpret_cast<void**>(dd + 64));
        auto right = *reinterpret_cast<uint8_t**>(dp + 3);
        MenuRight.SetAddress(right); // DIK_RIGHT (205), native 50E040.
        MenuUp.SetAddress(right - 4);
        MenuDown.SetAddress(right - 8);
        MenuLeft.SetAddress(right + 4); // DIK_LEFT (203), native 50DFE0.
        MenuScaleX.SetAddress(*reinterpret_cast<float**>(coordinates + 5));
        MenuScaleY.SetAddress(*reinterpret_cast<float**>(hints + 3));
        MenuMouseX.SetAddress(*reinterpret_cast<float**>(mx + 2));
        MenuMouseY.SetAddress(*reinterpret_cast<float**>(my + 2));
        bWidescreen.SetAddress(*reinterpret_cast<bool**>(wide + 2));
        MenuLanguage.SetAddress(*reinterpret_cast<int**>(language + 20));
        headingHook = safetyhook::create_inline(reinterpret_cast<void*>(dd + 54 + *reinterpret_cast<int32_t*>(dd + 50)), NativeHeading);
        hintsHook = safetyhook::create_inline(reinterpret_cast<void*>(hints), DrawHints);
        injector::WriteMemory<void*>(ot + 3, reinterpret_cast<void*>(&OptionsEnter), true);
        injector::WriteMemory<void*>(ot + 4, reinterpret_cast<void*>(&OptionsLeave), true);
        injector::WriteMemory<void*>(ot + 5, reinterpret_cast<void*>(&OptionsProcess), true);
        injector::WriteMemory<void*>(ot + 6, reinterpret_cast<void*>(&OptionsDraw), true);
        injector::WriteMemory<void*>(dt + 3, reinterpret_cast<void*>(&DisplayEnter), true);
        injector::WriteMemory<void*>(dt + 4, reinterpret_cast<void*>(&DisplayLeave), true);
        injector::WriteMemory<void*>(dt + 5, reinterpret_cast<void*>(&DisplayProcess), true);
        injector::WriteMemory<void*>(dt + 6, reinterpret_cast<void*>(&DisplayDraw), true);
    }
}
