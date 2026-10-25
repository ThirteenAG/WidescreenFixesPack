#pragma once
#include "../Shared/RWGTA/FixMenu.hpp"

namespace BullyMenu
{
    using ScreenFn = void(__thiscall*)(void*);
    static ScreenFn optionsEnter, optionsLeave, optionsProcess, displayEnter, displayLeave, displayProcess, displayDraw;
    static void* optionsDraw;
    static SafetyHookInline headingHook;
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
    static void* dividerColor;
    static uint8_t *up, *down, *left, *right;
    static float* scaleX;
    static float *scaleY, *mouseX, *mouseY;
    static bool* widescreen;
    static float previousMouseX, previousMouseY;
    static bool mouseMode;
    static int* nativeLanguage;
    static RWGTAMenu::Model model;
    static void* owner;
    static bool paused;
    static size_t page, selected, first;
    constexpr size_t PageRows = 64;     // a plugin's rows on one page; the page scrolls 7 rows at a time

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
        return *nativeLanguage >= 0 && *nativeLanguage < int(std::size(languages)) ? languages[*nativeLanguage] : "en";
    }

    static std::wstring FontText(const std::string& text)
    {
        // The native font takes UTF-16. Keep each draw below the native formatter's 512-WORD buffer.
        auto count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), int(text.size()), nullptr, 0);
        if (count <= 0)
            return {};
        std::wstring out(count, L'\0');
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), int(text.size()), out.data(), count);
        if (out.size() > 180)
            out.resize(180);
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
        return (*widescreen ? 853.0f : 640.0f) - 102.0f;
    }

    static void Print(float x, float y, const std::string& text, bool highlight = false, bool enabled = true, int align = 0, float width = 440.0f, int font = 1, Casing casing = AsWritten)
    {
        auto converted = FontText(text);
        ApplyCasing(converted, casing);
        int color = !enabled ? 2 : highlight ? 3 : 11;
        style(color, align, font, 0);
        while (!converted.empty() && measure(converted.c_str(), 1) > width * *scaleX)
            converted.pop_back();
        drawText(int(x * *scaleX), int(y), converted.c_str(), color, align, font, 0);
    }

    // Splits text into at most two lines of the given width; the second ends with "..." when the text is longer
    static std::vector<std::wstring> Wrap(const std::string& text, float width)
    {
        std::vector<std::wstring> lines;
        auto converted = FontText(text);
        style(11, 0, 1, 0);
        float limit = width * *scaleX;
        size_t start = 0;
        while (start < converted.size() && lines.size() < 2)
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
                    end = converted.size();
            }
            auto line = converted.substr(start, end - start);
            if (lines.size() == 1 && end < converted.size())
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

    static void Close()
    {
        model.Close();
        owner = nullptr;
        page = selected = first = 0;
        mouseMode = false;
    }

    static void Open(void* screen, bool pause)
    {
        Close();
        HMODULE module = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&Open), &module);
        model.OpenPlugins(module, Language(), PageRows);
        owner = screen;
        paused = pause;
        previousMouseX = *mouseX;
        previousMouseY = *mouseY;
        consume();
    }

    static void UpdateMouse()
    {
        if (std::isfinite(*mouseX) && std::isfinite(*mouseY) && (*mouseX != previousMouseX || *mouseY != previousMouseY))
            mouseMode = true;
        previousMouseX = *mouseX;
        previousMouseY = *mouseY;
    }

    static bool Hover(float x, float y, const std::string& text, int align = 0, float width = 440.0f, Casing casing = AsWritten)
    {
        if (!mouseMode) return false;
        auto converted = FontText(text);
        ApplyCasing(converted, casing);
        style(3, align, 1, 0);
        while (!converted.empty() && measure(converted.c_str(), 1) > width * *scaleX)
            converted.pop_back();
        float rectangle[4] = {};
        // Use the same centered coordinates and native font bounds as the stock title mouse hit-test.
        bounds(rectangle, (x * *scaleX - (*widescreen ? 853.0f : 640.0f) * 0.5f) * *scaleX,
            (y - 240.0f) * *scaleY, converted.c_str());
        return *mouseX >= rectangle[0] && *mouseX <= rectangle[2] && *mouseY >= rectangle[1] && *mouseY <= rectangle[3];
    }

    static size_t RowCount()
    {
        return page == 0 ? model.Pages().size() : model.VisibleRows(page - 1).size();
    }

    static void ProcessPage()
    {
        UpdateMouse();
        bool pointer = pointerAccept(28, 0, 1) != 0;
        if (back(0) || (pointer && Hover(100, 395, Text("menu.back"))))
        {
            consume();
            if (page != 0)
            {
                page = selected = first = 0;
                model.PageShown(size_t(-1));
            }
            else Close();
            return;
        }
        auto count = RowCount();
        if (count == 0)
            return;
        selected = std::min(selected, count - 1);
        bool rowHovered = false;
        auto visible = page == 0 ? std::vector<const RWGTAMenu::Row*>{} : model.VisibleRows(page - 1);
        for (size_t i = first; i < std::min(count, first + 7); ++i)
        {
            float y = 145.0f + float(i - first) * 30.0f;
            if (Hover(100, y, page == 0 ? model.Pages()[i].title : model.Label(*visible[i]), 0, page == 0 ? 440.0f : 300.0f, TitleCase)
                || (page != 0 && Hover(ValueX(), y, model.Value(*visible[i]), 1, 220.0f, Capitals)))
            {
                selected = i;
                rowHovered = true;
                break;
            }
        }
        if (repeat(0) && (*up || *down))
        {
            selected = *up ? (selected + count - 1) % count : (selected + 1) % count;
            delay(0);
            mouseMode = false;
        }
        bool pressed = accept(0) != 0;
        if (pointer && !rowHovered) { consume(); return; }
        int direction = repeat(0) ? (*left ? -1 : *right ? 1 : 0) : 0;
        if (!pressed && !direction)
            return;
        if (page == 0)
        {
            if (pressed)
            {
                page = selected + 1;
                selected = first = 0;
                model.PageShown(page - 1);
            }
        }
        else
        {
            auto rows = model.VisibleRows(page - 1);
            if (selected < rows.size())
                model.Change(*rows[selected], pressed ? 0 : direction, paused);
        }
        if (pressed) consume();
        else delay(0);
    }

    static void DrawPage()
    {
        // Repaint the same native menu background before replacing its heading/content.
        background();
        auto title = FontText(page == 0 ? Text("menu.plugins") : model.Pages()[page - 1].title);
        ApplyCasing(title, Capitals);
        style(8, 2, 4, 0);
        drawText(int((*widescreen ? 853.0f : 640.0f) * 0.5f * *scaleX), 50, title.c_str(), 8, 2, 4, 0);
        divider(320.0f, 440.0f, 110.0f, dividerColor, 2.0f);
        auto count = RowCount();
        if (count)
        {
            selected = std::min(selected, count - 1);
            if (selected < first) first = selected;
            if (selected >= first + 7) first = selected - 6;
            auto rows = page == 0 ? std::vector<const RWGTAMenu::Row*>{} : model.VisibleRows(page - 1);
            for (size_t i = first; i < std::min(count, first + 7); ++i)
            {
                float y = 145.0f + float(i - first) * 30.0f;
                bool enabled = page == 0 || model.Enabled(*rows[i], paused);
                Print(100, y, page == 0 ? model.Pages()[i].title : model.Label(*rows[i]), i == selected, enabled, 0, page == 0 ? 440.0f : 300.0f, 1, TitleCase);
                if (page != 0)
                    Print(ValueX(), y, model.Value(*rows[i]), i == selected, enabled, 1, 220.0f, 1, Capitals);
            }
            if (page != 0 && selected < rows.size())
            {
                auto lines = Wrap(model.Help(*rows[selected]), ValueX() - 100.0f);
                for (size_t i = 0; i < lines.size(); ++i)
                {
                    style(11, 0, 1, 0);
                    drawText(int(100.0f * *scaleX), int(352.0f + 18.0f * float(i)), lines[i].c_str(), 11, 0, 1, 0);
                }
            }
        }
        // A changed setting that applies at the next start: one line under the heading
        if (model.RestartRequired())
        {
            auto warning = FontText(model.Text("menu.restartwarning"));
            style(8, 2, 1, 0);
            drawText(int((*widescreen ? 426.5f : 320.0f) * *scaleX), 122, warning.c_str(), 8, 2, 1, 0);
        }
        Print(100, 395, Text("menu.back"), false, true, 0, 440.0f, 1, TitleCase);
        if (count > 7) Print(ValueX(), 395, std::to_string(first + 1) + "/" + std::to_string(count), false, true, 1);
        controls("FEO_CHGCTRL");
    }

    struct Widget { void** vtable; uint32_t type = 2; };
    // Native callers push x, y, data offset, selected and editing; widget draw cleans exactly 0x14 bytes.
    static void __fastcall WidgetDraw(Widget*, void*, float x, float y, float, uint32_t highlight, uint32_t)
    {
        // The native caller pushes the selected flag as a bool: only its low byte is set
        Print(x / *scaleX, y, Text("menu.plugins"), (highlight & 0xFF) != 0, true, 0, 440.0f, 1, TitleCase);
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
        bool negative = *left || *up;
        bool positive = *right || *down;
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
        auto matches = hook::pattern(signature);
        return matches.size() == 1 ? reinterpret_cast<uintptr_t>(matches.get_first()) : 0;
    }

    static void Init()
    {
        auto op = Find("56 8B F1 80 BE 58 01 00 00 00 57 0F 85 ? ? ? ? 8D BE 6C 01 00 00 55 8B CF");
        auto oe = Find("56 8B F1 33 C0 89 86 54 01 00 00 88 86 58 01 00 00 E8 ? ? ? ? 8D 8E 6C 01 00 00 5E E9");
        auto ol = Find("56 8B F1 E8 ? ? ? ? 80 BE 58 01 00 00 00 74 2D 8D 8E 6C 01 00 00 E8");
        auto dp = Find("51 80 3D ? ? ? ? 00 53 56 8B F1 75 09 80 3D ? ? ? ? 00 74 43 6A 00");
        auto dd = Find("83 EC 70 53 55 56 8B F1 83 7E 10 02 57 75 17 E8 ? ? ? ? 84 C0 75 0E");
        auto de = Find("33 C0 89 41 10 89 41 0C E9 ? ? ? ? CC CC CC 51 80 3D");
        auto acceptSite = Find("6A 00 B9 ? ? ? ? E8 ? ? ? ? 84 C0 74 36 8B CF E8 ? ? ? ? 8B 96 54 01");
        auto a = acceptSite ? acceptSite + 12 + *reinterpret_cast<int32_t*>(acceptSite + 8) : 0;
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
        if (!op || !oe || !ol || !dp || !dd || !de || !a || !b || !r || !c || !d || !font || !coordinates || !language || !hints || !metrics || !box || !mx || !my || !wide || !bg)
            return;
        if (*reinterpret_cast<uint8_t*>(dd + 100) != 0xE8 || *reinterpret_cast<uint8_t*>(dd + 63) != 0x68
            || *reinterpret_cast<uint8_t*>(dd + 49) != 0xE8 || *reinterpret_cast<uint8_t*>(dd + 48) != 0x50
            || *reinterpret_cast<uint8_t*>(dd + 43) != 0xA1)
            return;
        if (*reinterpret_cast<uint8_t*>(a + 41) != 0xE8)
            return;
        auto image = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
        auto header = reinterpret_cast<IMAGE_NT_HEADERS*>(image + reinterpret_cast<IMAGE_DOS_HEADER*>(image)->e_lfanew);
        auto end = image + header->OptionalHeader.SizeOfImage;
        // The default pattern scanner ends at executable sections; native vtables are in read-only data.
        auto optionTables = hook::range_pattern(image, end,
            pattern_str(to_bytes(uint32_t(oe))) + pattern_str(to_bytes(uint32_t(ol))) + pattern_str(to_bytes(uint32_t(op))));
        auto displayTables = hook::range_pattern(image, end, pattern_str(to_bytes(uint32_t(dp))) + pattern_str(to_bytes(uint32_t(dd))));
        if (optionTables.size() != 1 || displayTables.size() != 1)
            return;
        auto ot = optionTables.get_first<void*>(-12);
        auto dt = displayTables.get_first<void*>(-20);
        if (reinterpret_cast<uintptr_t>(dt[3]) != de)
            return;
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
        dividerColor = *reinterpret_cast<void**>(dd + 64);
        right = *reinterpret_cast<uint8_t**>(dp + 3); // DIK_RIGHT (205), native 50E040.
        up = right - 4;
        down = right - 8;
        left = right + 4; // DIK_LEFT (203), native 50DFE0.
        scaleX = *reinterpret_cast<float**>(coordinates + 5);
        scaleY = *reinterpret_cast<float**>(hints + 3);
        mouseX = *reinterpret_cast<float**>(mx + 2);
        mouseY = *reinterpret_cast<float**>(my + 2);
        widescreen = *reinterpret_cast<bool**>(wide + 2);
        nativeLanguage = *reinterpret_cast<int**>(language + 20);
        headingHook = safetyhook::create_inline(reinterpret_cast<void*>(dd + 54 + *reinterpret_cast<int32_t*>(dd + 50)), NativeHeading);
        if (!headingHook) return;
        ProtectedGameRef<void*>(ot + 3) = reinterpret_cast<void*>(&OptionsEnter);
        ProtectedGameRef<void*>(ot + 4) = reinterpret_cast<void*>(&OptionsLeave);
        ProtectedGameRef<void*>(ot + 5) = reinterpret_cast<void*>(&OptionsProcess);
        ProtectedGameRef<void*>(ot + 6) = reinterpret_cast<void*>(&OptionsDraw);
        ProtectedGameRef<void*>(dt + 3) = reinterpret_cast<void*>(&DisplayEnter);
        ProtectedGameRef<void*>(dt + 4) = reinterpret_cast<void*>(&DisplayLeave);
        ProtectedGameRef<void*>(dt + 5) = reinterpret_cast<void*>(&DisplayProcess);
        ProtectedGameRef<void*>(dt + 6) = reinterpret_cast<void*>(&DisplayDraw);
    }
}
