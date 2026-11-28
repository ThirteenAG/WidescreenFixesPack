module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <cstdint>
#include "Hooking.Patterns.h"
#include <Windows.h>
#include <cstring>
#include <cmath>
#include <string_view>
#include <string>
#include <vector>
#include <algorithm>
#include "Shared/RWGTA/FixMenu.hpp"
#include <memory>
#include <array>
#include <unordered_map>
#include "Shared/RWGTA/UpdateCheck.hpp"
#include "Shared/RainDroplets.hpp"
#include "Shared/PadInput.hpp"
#include <set>

export module Frontend;

import ComVars;


export namespace DeerMenu::Native
{
    // Native font functions of the supported executable.
    inline constexpr int GlyphWidth = 8;
    inline constexpr int GlyphHeight = 14;
    inline constexpr unsigned FirstGlyph = 32;
    inline constexpr unsigned GlyphCount = 91;

    inline void* CreateTypeface(const uint32_t* foreground, const uint32_t* background)
    {
        // Native cdecl wrapper owns the constructed raster. Its internal helper
        // uses a custom register ABI; calling that interior helper is prohibited.
        return reinterpret_cast<void* (__cdecl*)(const uint32_t*, const uint32_t*)>(Game().createTypeface)(
            foreground, background);
    }

    inline void DestroyTypeface(void* raster)
    {
        reinterpret_cast<int (__cdecl*)(void*)>(Game().destroyTypeface)(raster);
    }

    inline bool DrawText(void* raster, const char* text, int x, int y)
    {
        // cdecl with four arguments.
        return reinterpret_cast<void* (__cdecl*)(void*, const char*, int, int)>(Game().drawText)(
            raster, text, x, y) == raster;
    }

    using KeyHandler = int (__cdecl*)(int event, const int* key);
    using Frame = void (__cdecl*)();
    using Render = int (__cdecl*)(void* world, void* camera);
}

export namespace DeerMenu
{
    inline thread_local unsigned pauseSpriteDepth = 0;

    template<class T> inline T Read(uintptr_t address)
    {
        return *reinterpret_cast<const T*>(address);
    }

    template<class T> inline T Global(uintptr_t address)
    {
        return *reinterpret_cast<const volatile T*>(address);
    }

    struct PauseContext
    {
        void* world{};
        void* camera{};
        void* engine{};
        void* raster{};
        int width{};
        int height{};

        bool Alive() const
        {
            return world && camera && engine && raster &&
                (DeerMenu::Game(), game_world.get()) == world && (DeerMenu::Game(), game_camera.get()) == camera &&
                (DeerMenu::Game(), game_engine.get()) == engine && (DeerMenu::Game(), game_worldState.get()) == 1 &&
                Read<void*>(reinterpret_cast<uintptr_t>(camera) + 96) == raster &&
                Read<int>(reinterpret_cast<uintptr_t>(raster) + 12) == width &&
                Read<int>(reinterpret_cast<uintptr_t>(raster) + 16) == height;
        }

        bool Current() const
        {
            // Ordinary in-world pause only. Loading/cutscene/exhibition, console,
            // deactivation and stock confirmations are independently excluded.
            return (DeerMenu::Game(), game_demoState.get()) == 0 && (DeerMenu::Game(), game_levelState.get()) != 0 &&
                (DeerMenu::Game(), game_menuOpen.get()) == 1 && (DeerMenu::Game(), game_menuActive.get()) == 1 &&
                (DeerMenu::Game(), game_menuPage.get()) == 5 && (DeerMenu::Game(), game_transitionState.get()) == 0 &&
                (DeerMenu::Game(), game_consoleState.get()) == 0 && (DeerMenu::Game(), game_introDone.get()) == 1 && Alive();
        }

        bool Rendering() const
        {
            // Font geometry reads the currently begun RenderWare camera. This
            // stronger predicate belongs to the actual draw boundary, not the
            // outer-frame logical queue where EndUpdate may clear that camera.
            if (!Current())
                return false;
            const float nearClip = Read<float>(reinterpret_cast<uintptr_t>(camera) + 128);
            const float screenZ = Read<float>(reinterpret_cast<uintptr_t>(engine) + 24);
            return Read<void*>(reinterpret_cast<uintptr_t>(engine)) == camera &&
                std::isfinite(nearClip) && nearClip > 0.0f && std::isfinite(screenZ);
        }

        // Cheap pre-check before Capture: only the in-world pause menu qualifies.
        static bool PauseShown()
        {
            return (DeerMenu::Game(), game_menuPage.get()) == 5 && (DeerMenu::Game(), game_menuOpen.get()) == 1 && (DeerMenu::Game(), game_menuActive.get()) == 1 &&
                (DeerMenu::Game(), game_worldState.get()) == 1;
        }

        bool Capture(void* renderWorld, void* renderCamera)
        {
            world = renderWorld;
            camera = renderCamera;
            engine = (DeerMenu::Game(), game_engine.get());
            if (!camera || !engine)
                return false;
            raster = Read<void*>(reinterpret_cast<uintptr_t>(camera) + 96);
            if (!raster)
                return false;
            width = Read<int>(reinterpret_cast<uintptr_t>(raster) + 12);
            height = Read<int>(reinterpret_cast<uintptr_t>(raster) + 16);
            const float nearClip = Read<float>(reinterpret_cast<uintptr_t>(camera) + 128);
            const float screenZ = Read<float>(reinterpret_cast<uintptr_t>(engine) + 24);
            return width >= 320 && height >= 240 && width <= 16384 && height <= 16384 &&
                std::isfinite(nearClip) && nearClip > 0.0f && std::isfinite(screenZ) &&
                Rendering();
        }
    };
}

export namespace DeerMenu
{
    // 4C9810 exposes91 embedded8x14 glyphs. 4C9E40 sign-extends each byte and
    // subtracts32 without validating it. No Unicode/replacement font is assumed.
    inline std::string PlainText(std::string_view utf8)
    {
        if (utf8.size() > 16384) return "?";
        std::string result;
        for (size_t i = 0; i < utf8.size();)
        {
            uint32_t code = static_cast<uint8_t>(utf8[i++]);
            if (code >= 0x80)
            {
                size_t count = code >= 0xC2 && code <= 0xDF ? 1 : code >= 0xE0 && code <= 0xEF ? 2 : code >= 0xF0 && code <= 0xF4 ? 3 : 0;
                uint32_t minimum = count == 1 ? 0x80 : count == 2 ? 0x800 : 0x10000;
                bool valid = count && count <= utf8.size() - i;
                uint32_t decoded = code & (count == 1 ? 0x1F : count == 2 ? 0x0F : 0x07);
                for (size_t k = 0; valid && k < count; ++k)
                {
                    auto byte = static_cast<uint8_t>(utf8[i + k]);
                    valid = (byte & 0xC0) == 0x80;
                    decoded = (decoded << 6) | (byte & 0x3F);
                }
                if (valid)
                {
                    valid = decoded >= minimum && decoded <= 0x10FFFF && !(decoded >= 0xD800 && decoded <= 0xDFFF);
                    if (valid)
                        i += count;
                }
                code = valid ? decoded : '?';
            }
            if (code == 0x2018 || code == 0x2019)
                code = '\'';
            else if (code == 0x201C || code == 0x201D)
                code = '"';
            else if (code == 0x2013 || code == 0x2014)
                code = '-';
            else if (code == 0xA0 || code == '\r' || code == '\n' || code == '\t')
                code = ' ';
            if (code == 0xDF) { result += "ss"; continue; }
            if (code == 0xC6 || code == 0xE6) { result += code == 0xC6 ? "AE" : "ae"; continue; }
            if (code == 0x152 || code == 0x153) { result += code == 0x152 ? "OE" : "oe"; continue; }
            if (code == 0xD8 || code == 0xF8) code = code == 0xD8 ? 'O' : 'o';
            if (code >= 0x80 && code <= 0xFFFF)
            {
                wchar_t original = wchar_t(code), decomposed[8]{};
                if (FoldStringW(MAP_COMPOSITE, &original, 1, decomposed, int(std::size(decomposed))) > 0 && decomposed[0] >= 32 && decomposed[0] <= 126)
                    code = uint32_t(decomposed[0]);
            }
            if (code < Native::FirstGlyph || code >= Native::FirstGlyph + Native::GlyphCount)
                code = '?';
            result += static_cast<char>(code);
        }
        return result;
    }

    inline bool NativeText(std::string_view text)
    {
        for (unsigned char ch : text)
            if (ch < Native::FirstGlyph || ch >= Native::FirstGlyph + Native::GlyphCount)
                return false;
        return true;
    }

    inline bool TextFits(std::string_view text, int width, int height)
    {
        const auto plain = PlainText(text);
        return NativeText(plain) && width >= 0 && height >= Native::GlyphHeight &&
            plain.size() <= static_cast<std::size_t>(width / Native::GlyphWidth);
    }

    inline bool FitText(std::string_view text, int width, std::string& output)
    {
        output.clear();
        const auto plain = PlainText(text); text = plain;
        if (!NativeText(text) || width < 0)
            return false;
        const auto columns = size_t(width / Native::GlyphWidth);
        if (text.size() <= columns)
            output.assign(text);
        else if (columns <= 3)
            output.assign(columns, '.');
        else
            output = std::string(text.substr(0, columns - 3)) + "...";
        return true;
    }

    inline bool SplitText(std::string_view text, int width, std::vector<std::string>& pieces)
    {
        pieces.clear();
        const auto plain = PlainText(text); text = plain;
        if (!NativeText(text) || width < Native::GlyphWidth)
            return false;
        const auto columns = static_cast<std::size_t>(width / Native::GlyphWidth);
        while (!text.empty())
        {
            auto count = std::min(columns, text.size());
            if (count < text.size())
            {
                // Whole words/path components only. Preserve the delimiter and
                // every original character; refuse a component wider than a row.
                const auto split = text.substr(0, count).find_last_of(" /\\:._-");
                if (split == std::string_view::npos)
                    return false;
                count = split + 1;
            }
            pieces.emplace_back(text.substr(0, count));
            text.remove_prefix(count);
            if (pieces.size() > 2048)
                return false;
        }
        if (pieces.empty())
            pieces.emplace_back();
        return true;
    }

}

export namespace DeerMenu
{
    enum class Kind { Link, Row, Back };

    struct Item
    {
        Kind kind = Kind::Back;
        std::string text;
        RWGTAMenu::Row row{};
        size_t target = 0;
        bool enabled = true;
    };

    // A page of the menu: its rows in a window that scrolls a row at a time (window: the rows shown; all of them in the
    // launcher's list box, which scrolls itself), then Back
    struct View
    {
        size_t page = RWGTAMenu::Model::RootPage;
        size_t first = 0;
        size_t window = SIZE_MAX;
        size_t count = 0;           // the rows of the page
        std::string title;
        std::vector<std::string> headings;
        std::vector<Item> items;

        // Caller stages a candidate View and publishes it only with the matching
        // native widget/text transaction. No stale visible-row mapping survives.
        bool Prepare(RWGTAMenu::Model& model, bool pause,
                     const std::function<bool()>& current, int width)
        {
            if (!current() || !model.IsOpen() || model.Pages().empty() || width < 80)
                return false;
            std::vector<Item> content;
            auto append = [&](Item item, const std::string& text)
            {
                if (!FitText(text, width, item.text) || !current() || content.size() >= 2048)
                    return false;
                content.push_back(std::move(item));
                return true;
            };
            {
                if (page >= model.Pages().size())
                    return false;
                title = model.Pages()[page].title;
                const auto visible = model.VisibleRows(page);
                if (!current())
                    return false;
                std::vector<RWGTAMenu::Row> copied;
                for (auto row : visible)
                    copied.push_back(*row);
                for (auto row : copied)
                {
                    auto label = model.Label(row);
                    if (!current())
                        return false;
                    if (RWGTAMenu::Model::Opens(row))
                    {
                        if (!append(Item{Kind::Link, {}, row, row.target, true}, label))
                            return false;
                        continue;
                    }
                    if (!row.plugin && (row.kind == RWGTAMenu::RowKind::Preset ||
                        row.kind == RWGTAMenu::RowKind::ApplyPreset))
                        continue;
                    auto value = model.Value(row);
                    if (!current())
                        return false;
                    const auto enabled = model.Enabled(row, pause);
                    if (!current())
                        return false;
                    std::string suffix = value.empty() ? std::string{} : ": " + value;
                    if (!enabled)
                        suffix += " " + model.Text("menu.unavailable");
                    std::string fittedSuffix, fittedLabel;
                    if (!FitText(suffix, width, fittedSuffix) ||
                        !FitText(label, std::max(0, width - int(fittedSuffix.size()) * Native::GlyphWidth), fittedLabel))
                        return false;
                    label = fittedLabel + fittedSuffix;
                    if (!append(Item{Kind::Row, {}, row, 0, enabled}, label))
                        return false;
                }
            }
            if (model.RestartRequired())
                title = model.Text("menu.restartwarning");
            if (!current() || !SplitText(title, width, headings) || headings.size() > 4)
                return false;
            count = content.size();
            first = count > window ? std::min(first, count - window) : 0;
            items.clear();
            for (size_t i = first; i < content.size() && i - first < window; ++i)
                items.push_back(std::move(content[i]));
            items.push_back(Item{Kind::Back, model.Text("menu.back"), {}, 0, true});
            for (auto& item : items)
            {
                item.text = PlainText(item.text);
                if (!TextFits(item.text, width, Native::GlyphHeight)) return false;
            }
            return current() && !items.empty() && (window == SIZE_MAX || items.size() <= window + 1);
        }

        // The rows of the page in the window
        size_t Shown() const { return items.empty() ? 0 : items.size() - 1; }
        bool Above() const { return first > 0; }
        bool Below() const { return window != SIZE_MAX && first + window < count; }
    };
}

export namespace DeerMenu
{
    struct Session : std::enable_shared_from_this<Session>
    {
        struct Pending
        {
            uint64_t generation = 0;
            uint64_t serial = 0;
            size_t page = RWGTAMenu::Model::RootPage;
            size_t first = 0;
            size_t selected = 0;
            int direction = 0;
            bool back = false;
            Item item;
        };

        struct Busy
        {
            bool& flag;
            bool previous;
            explicit Busy(bool& value) : flag(value), previous(value) { flag = true; }
            ~Busy() { flag = previous; }
        };

        HMODULE module = nullptr;
        RWGTAMenu::Model model;
        View view;
        std::function<bool()> owner;
        std::vector<Pending> pending;
        uint64_t generation = 0;
        uint64_t serial = 0;
        size_t selected = 0;
        int width = 0;
        bool pause = true;
        bool open = false;
        bool opening = false;
        bool ready = false;
        bool busy = false;
        bool invalid = false;
        bool draining = false;

        bool Current(uint64_t expected) const
        {
            return !invalid && open && generation == expected && owner && owner();
        }

        void Close() noexcept
        {
            // Logical cancellation is immediate even inside a Get/Set/preset
            // burst; native window/font cleanup belongs to the outer bridge.
            ++generation;
            ++serial;
            open = opening = ready = false;
            pending.clear();
            try { model.Close(); } catch (...) { invalid = true; }
        }

        void Retire() noexcept
        {
            invalid = true;
            Close();
        }

        static bool Same(const Item& a, const Item& b)
        {
            return a.kind == b.kind && a.target == b.target && a.text == b.text &&
                a.row.kind == b.row.kind && a.row.plugin == b.row.plugin && a.row.setting == b.row.setting;
        }

        bool Populate(bool preserveSelection)
        {
            auto lease = shared_from_this();
            const auto expected = generation;
            if (!Current(expected) || !model.IsOpen())
                return false;
            const bool hadItem = preserveSelection && selected < view.items.size();
            const Item old = hadItem ? view.items[selected] : Item{};
            View candidate = view;
            Busy reading(busy);
            if (!candidate.Prepare(model, pause, [this, expected] { return Current(expected); }, width) ||
                !Current(expected))
                return false;
            size_t next = 0;
            if (hadItem)
            {
                for (size_t i = 0; i < candidate.items.size(); ++i)
                    if (Same(candidate.items[i], old))
                    {
                        next = i;
                        break;
                    }
                if (next == 0 && !Same(candidate.items[0], old))
                    next = std::min(selected, candidate.items.size() - 1);
            }
            // The text/widget bridge publishes this committed snapshot as one
            // unit. Old queued row identities are invalid from this point.
            view = std::move(candidate);
            selected = next;
            ++serial;
            ready = true;
            return true;
        }

        bool ReadHelp(std::string& text)
        {
            const auto expected = generation;
            const auto expectedSerial = serial;
            const auto expectedSelected = selected;
            if (!Current(expected) || !ready || selected >= view.items.size() || busy)
                return false;
            const Item item = view.items[selected];
            Busy reading(busy);
            text = item.kind == Kind::Row ? PlainText(model.Help(item.row)) : std::string{};
            return Current(expected) && serial == expectedSerial && selected == expectedSelected;
        }

        bool Open(const std::string& language)
        {
            auto lease = shared_from_this();
            if (invalid || !module || !owner || !owner())
                return false;
            Close();
            if (invalid || !owner())
                return false;
            open = opening = true;
            ready = false;
            selected = 0;
            view = View{ RWGTAMenu::Model::RootPage, 0, view.window };
            const auto expected = generation;
            Busy discovery(busy);
            model.OpenMenu(module, language, 4096);
            if (!Current(expected) || !model.IsOpen() || !Populate(false))
            {
                if (generation == expected)
                    Close();
                return false;
            }
            opening = false;
            return Current(expected) && ready;
        }

        bool Queue(int direction, bool physicalBack, size_t actualSelected)
        {
            auto lease = shared_from_this();
            const auto expected = generation;
            if (!Current(expected) || (!opening && (!ready || actualSelected != selected ||
                selected >= view.items.size())))
                return false;
            // Physical Back is the only accepted construction-phase action.
            if (opening)
            {
                if (!physicalBack || direction)
                    return false;
                Close();
                return true;
            }
            const Item item = view.items[selected];
            const bool back = physicalBack || (direction == 0 && item.kind == Kind::Back);
            if (busy && back && direction == 0)
            {
                Close();
                return true;
            }
            if (pending.size() >= 32 || (physicalBack && direction))
                return false;
            pending.push_back(Pending{expected, serial, view.page, view.first, selected, direction, back, item});
            return true;
        }

        // Up/Down: past the first or last row shown the list scrolls a row; from the first row up and from Back down
        // the list wraps to its other end
        bool Move(int direction)
        {
            if (!Current(generation) || !ready || busy || view.items.empty())
                return false;
            const size_t shown = view.Shown(), back = view.items.size() - 1;
            auto show = [&](size_t first, size_t row)
            {
                view.first = first;
                if (!Populate(false))
                    return false;
                selected = std::min(row, view.items.size() - 1);
                return true;
            };
            if (direction < 0)
            {
                if (selected == back && view.Below())
                    return show(view.count - view.window, back);
                if (selected)
                    --selected;
                else if (view.Above())
                    return show(view.first - 1, 0);
                else if (view.Below())
                    return show(view.count - view.window, back);
                else
                    selected = back;
            }
            else if (direction > 0)
            {
                if (selected + 1 == shown && view.Below())
                    return show(view.first + 1, selected);
                if (selected < back)
                    ++selected;
                else if (view.Above())
                    return show(0, 0);
                else
                    selected = 0;
            }
            return true;
        }

        bool Activate(const Pending& command)
        {
            auto lease = shared_from_this();
            const auto expected = generation;
            if (!Current(expected) || !ready || selected >= view.items.size())
                return false;
            const Item item = command.item;
            if (command.back)
            {
                if (command.direction)
                    return true;
                const auto parent = model.Back(view.page);
                if (parent == SIZE_MAX)
                {
                    Close();
                    return true;
                }
                // Back on the row of the page it was opened from
                const auto rows = model.VisibleRows(parent);
                const auto found = std::find_if(rows.begin(), rows.end(), [&](const RWGTAMenu::Row* row)
                    { return row && row->target == view.page; });
                const size_t index = found == rows.end() ? 0 : size_t(found - rows.begin());
                view.page = parent;
                view.first = view.window != SIZE_MAX && index >= view.window ? index - view.window + 1 : 0;
                if (!Populate(false))
                    return false;
                selected = std::min(index - view.first, view.items.size() - 1);
                return true;
            }
            if (item.kind != Kind::Row && command.direction)
                return true;
            if (item.kind == Kind::Link)
            {
                const auto target = model.Enter(item.row);
                if (target == SIZE_MAX)
                    return false;
                view.page = target;
                view.first = 0;
                return Current(expected) && Populate(false);
            }
            if (item.kind == Kind::Row && item.enabled)
            {
                Busy changing(busy);
                const auto expectedSerial = serial;
                const auto expectedSelected = selected;
                const auto expectedPage = view.page;
                const auto expectedFirst = view.first;
                const bool enabled = model.Enabled(item.row, pause);
                if (!Current(expected) || serial != expectedSerial || selected != expectedSelected ||
                    view.page != expectedPage || view.first != expectedFirst)
                    return false;
                if (!enabled)
                    return Populate(true);
                model.Change(item.row, command.direction, pause);
                if (!Current(expected))
                    return false;
                return Populate(true);
            }
            return true;
        }

        void Frame()
        {
            auto lease = shared_from_this();
            if (draining)
                return;
            Busy drain(draining);
            if (open && (!owner || !owner()))
                Close();
            if (!Current(generation) || !ready)
                return;
            auto commands = std::move(pending);
            pending.clear();
            for (const auto& command : commands)
            {
                if (!Current(command.generation) || command.serial != serial ||
                    command.page != view.page || command.first != view.first ||
                    command.selected != selected || selected >= view.items.size() ||
                    !Same(command.item, view.items[selected]))
                    continue;
                if (!Activate(command))
                {
                    Close();
                    return;
                }
            }
            if (Current(generation) && RWGTAMenu::Model::Changed().exchange(false) && !Populate(true))
                Close();
        }
    };
}

export namespace DeerMenu
{
    struct TitleContext
    {
        inline static constexpr std::array<int, 6> IDs{1, 2, 1002, 1003, 1000, 1001};
        HWND dialog = nullptr;
        std::array<HWND, 6> controls{};
        DWORD thread = 0;

        static bool Class(HWND window, const wchar_t* name)
        {
            wchar_t actual[32]{};
            return window && GetClassNameW(window, actual, int(std::size(actual))) > 0 &&
                lstrcmpiW(actual, name) == 0;
        }

        bool Alive() const
        {
            DWORD process = 0;
            if (!dialog || !IsWindow(dialog) || !Class(dialog, L"#32770") ||
                GetWindowThreadProcessId(dialog, &process) != thread || process != GetCurrentProcessId() ||
                reinterpret_cast<HMODULE>(GetWindowLongPtrW(dialog, GWLP_HINSTANCE)) != GetModuleHandleW(nullptr))
                return false;
            for (size_t i = 0; i < IDs.size(); ++i)
                if (!controls[i] || GetDlgItem(dialog, IDs[i]) != controls[i] ||
                    GetParent(controls[i]) != dialog || GetDlgCtrlID(controls[i]) != IDs[i] ||
                    !Class(controls[i], i < 3 ? L"Button" : L"ComboBox"))
                    return false;
            return true;
        }

        bool Capture(HWND source)
        {
            dialog = source;
            DWORD process = 0;
            thread = GetWindowThreadProcessId(source, &process);
            for (size_t i = 0; i < IDs.size(); ++i)
                controls[i] = GetDlgItem(source, IDs[i]);
            return thread == GetCurrentThreadId() && process == GetCurrentProcessId() && Alive();
        }

        bool Space(RECT& result) const
        {
            if (!Alive())
                return false;
            RECT client{}, exit{}, modes{};
            if (!GetClientRect(dialog, &client) || !GetWindowRect(controls[2], &exit) ||
                !GetWindowRect(controls[5], &modes))
                return false;
            SetLastError(0);
            if (!MapWindowPoints(nullptr, dialog, reinterpret_cast<POINT*>(&exit), 2) && GetLastError())
                return false;
            SetLastError(0);
            if (!MapWindowPoints(nullptr, dialog, reinterpret_cast<POINT*>(&modes), 2) && GetLastError())
                return false;
            const auto gap = std::max(4L, (exit.bottom - exit.top) / 2);
            result = {std::max(exit.right + gap, client.left), std::max(exit.top, client.top),
                std::min(modes.left - gap, client.right), std::min(exit.bottom, client.bottom)};
            // The native Exit button sits at 11,344 (62x22); the mode combo starts at 250,336.
            // Derive the spare horizontal band from actual child rectangles so
            // window/DPI coordinates stay in the same Win32 native space; keep the new row inside the client.
            return Alive() && result.left >= client.left && result.top >= client.top &&
                result.right <= client.right && result.bottom <= client.bottom &&
                result.right - result.left >= 80 && result.bottom - result.top >= 12 &&
                result.bottom - result.top <= 128;
        }
    };
}

export namespace DeerMenu
{
    inline std::string Language()
    {
        // Native glyph bounds remain fixed; translated text is folded to ASCII.
        const auto selected = FixSettings().String("MAIN.TextLanguage");
        return selected.empty() || selected == "auto" ? "en" : selected;
    }

    struct Popup;
    struct ListBinding
    {
        std::weak_ptr<Popup> owner;
        HWND window = nullptr;
        WNDPROC original = nullptr;
        bool expectedDestroy = false;
    };
    inline std::unordered_map<HWND, std::shared_ptr<Popup>> popups;
    inline std::unordered_map<HWND, std::shared_ptr<ListBinding>> listBindings;
    inline INT_PTR CALLBACK PopupProcedure(HWND, UINT, WPARAM, LPARAM);
    inline LRESULT CALLBACK ListProcedure(HWND, UINT, WPARAM, LPARAM);

    struct Popup : std::enable_shared_from_this<Popup>
    {
        inline static constexpr UINT ServiceMessage = WM_APP + 0x5D;
        TitleContext source;
        HMODULE module = nullptr;
        HWND window = nullptr;
        HWND heading = nullptr;     // the group box of the list, titled with the page
        HWND help = nullptr;
        HWND helpBox = nullptr;     // the group box of the help
        HWND confirm = nullptr;     // OK, like the launcher's own dialogs
        uint64_t helpSerial = 0;
        size_t helpSelected = SIZE_MAX;
        HWND list = nullptr;
        HFONT font = nullptr; // the launcher's own control font (or a stock object); never deleted by this host
        std::shared_ptr<Session> session;
        uint64_t paintedSerial = 0;
        HWND expectedFocus = nullptr;
        bool servicing = false;
        bool publishing = false;
        bool closing = false;
        bool invalid = false;
        bool started = false;

        bool Current() const
        {
            return !invalid && !closing && window && IsWindow(window) && source.Alive() &&
                (!servicing || publishing || GetFocus() == expectedFocus);
        }

        bool Input(HWND sender) const
        {
            return Current() && sender == list && list && IsWindow(list) &&
                GetFocus() == list && GetForegroundWindow() == window &&
                session && session->open;
        }

        void Cancel() noexcept
        {
            if (session)
                session->Close();
            if (window && IsWindow(window))
                PostMessageW(window, ServiceMessage, 0, 0);
        }

        void Retire() noexcept
        {
            invalid = true;
            if (session)
                session->Retire();
            if (window && IsWindow(window))
                PostMessageW(window, ServiceMessage, 0, 0);
        }

        bool Measure(HDC dc, const std::string& text, int width, int height) const
        {
            if (!NativeText(text) || text.size() > 4096)
                return false;
            const std::wstring wide(text.begin(), text.end());
            SIZE size{};
            return GetTextExtentPoint32W(dc, wide.data(), int(wide.size()), &size) &&
                size.cx <= width && size.cy <= height;
        }

        static void DestroyList(HWND control)
        {
            const auto it = listBindings.find(control);
            const auto binding = it == listBindings.end() ? std::shared_ptr<ListBinding>{} : it->second;
            if (it != listBindings.end())
                it->second->expectedDestroy = true;
            if (control && IsWindow(control))
                DestroyWindow(control);
            // Failed subclass installation keeps the original WNDPROC, so
            // our WM_NCDESTROY cannot remove this exact weak binding.
            const auto remaining = listBindings.find(control);
            if (!IsWindow(control) && remaining != listBindings.end() && remaining->second == binding)
                listBindings.erase(remaining);
        }

        bool Publish()
        {
            auto lease = shared_from_this();
            if (!Current() || !session || !session->ready || !session->open)
                return false;
            const auto generation = session->generation;
            const auto serial = session->serial;
            const View snapshot = session->view;
            const auto current = [&] { return Current() && session->Current(generation) && session->serial == serial; };
            RECT client{};
            if (!GetClientRect(window, &client) || !current())
                return false;
            HDC dc = GetDC(window);
            if (!dc)
                return false;
            const auto previous = SelectObject(dc, font);
            struct DCLease
            {
                HWND window;
                HDC dc;
                HGDIOBJ previous;
                ~DCLease() { if (previous) SelectObject(dc, previous); ReleaseDC(window, dc); }
            } dcLease{window, dc, previous};
            TEXTMETRICW metrics{};
            const bool metricOK = GetTextMetricsW(dc, &metrics) && metrics.tmHeight >= 12 && metrics.tmHeight <= 96;
            const int rowHeight = metricOK ? metrics.tmHeight + 4 : 0;
            const int width = client.right - client.left - 32;
            bool fits = metricOK && current();
            for (const auto& line : snapshot.headings)
                fits = fits && Measure(dc, line, width, metrics.tmHeight + 2);
            for (const auto& item : snapshot.items)
                fits = fits && Measure(dc, item.text, width - 8, metrics.tmHeight + 2);
            if (!fits || !current())
                return false;
            // Like the launcher's own dialogs: group boxes in the dialog's font, OK centred at the bottom. The list's
            // group takes the room the help and OK leave; the list box scrolls the rows that don't fit.
            const Layout layout = Arrange(client, metrics.tmHeight);
            const int top = layout.list.top + metrics.tmHeight + 6;
            const int listHeight = layout.list.bottom - 8 - top;
            if (listHeight < 3 * rowHeight || width < 80)
                return false;
            Session::Busy building(publishing);
            const auto next = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", L"",
                WS_CHILD | WS_TABSTOP | WS_VSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT,
                layout.list.left + 8, top, layout.list.right - layout.list.left - 16, listHeight, window,
                reinterpret_cast<HMENU>(0x7100), module, nullptr);
            struct PendingWindow
            {
                HWND value;
                ~PendingWindow() { if (value) Popup::DestroyList(value); }
            } pendingWindow{next};
            if (!next || !current())
            {
                return false;
            }
            auto binding = std::make_shared<ListBinding>();
            binding->owner = lease;
            binding->window = next;
            binding->original = reinterpret_cast<WNDPROC>(GetWindowLongPtrW(next, GWLP_WNDPROC));
            if (!binding->original)
                return false;
            listBindings.emplace(next, binding);
            SetLastError(0);
            const auto original = SetWindowLongPtrW(next, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(ListProcedure));
            if (!original && GetLastError())
            {
                return false;
            }
            SendMessageW(next, WM_SETFONT, reinterpret_cast<WPARAM>(font), FALSE);
            for (size_t i = 0; i < snapshot.items.size(); ++i)
            {
                const auto& text = snapshot.items[i].text;
                const std::wstring wide(text.begin(), text.end());
                if (!current() || SendMessageW(next, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(wide.c_str())) != LRESULT(i))
                {
                    return false;
                }
            }
            if (!current() || session->selected >= snapshot.items.size())
            {
                return false;
            }
            SendMessageW(next, LB_SETCURSEL, session->selected, 0);
            std::wstring title;
            for (const auto& line : snapshot.headings)
            {
                if (!title.empty())
                    title += L" ";
                title.append(line.begin(), line.end());
            }
            MoveWindow(heading, layout.list.left, layout.list.top, layout.list.right - layout.list.left,
                layout.list.bottom - layout.list.top, TRUE);
            SetWindowTextW(heading, title.c_str());
            MoveWindow(helpBox, layout.help.left, layout.help.top, layout.help.right - layout.help.left,
                layout.help.bottom - layout.help.top, TRUE);
            MoveWindow(confirm, layout.confirm.left, layout.confirm.top, layout.confirm.right - layout.confirm.left,
                layout.confirm.bottom - layout.confirm.top, TRUE);
            if (!current())
            {
                return false;
            }
            const auto old = list;
            list = next;
            pendingWindow.value = nullptr;
            paintedSerial = serial;
            ShowWindow(next, SW_SHOW);
            expectedFocus = next;
            SetFocus(next);
            DestroyList(old);
            return current() && GetFocus() == next;
        }

        struct Layout
        {
            RECT list{}, help{}, confirm{};
        };

        // The places of the list's group, the help's group (four lines) and OK in the client area
        static Layout Arrange(const RECT& client, int textHeight)
        {
            constexpr int Margin = 8;
            Layout layout;
            const int buttonHeight = textHeight + 10, buttonWidth = 6 * textHeight;
            layout.confirm = { (client.right - buttonWidth) / 2, client.bottom - Margin - buttonHeight,
                (client.right + buttonWidth) / 2, client.bottom - Margin };
            layout.help = { Margin, layout.confirm.top - Margin - (4 * textHeight + textHeight + 14), client.right - Margin,
                layout.confirm.top - Margin };
            layout.list = { Margin, Margin / 2, client.right - Margin, layout.help.top - Margin / 2 };
            return layout;
        }

        bool PublishHelp()
        {
            if (!Current() || !session || !session->ready || !list || !help)
                return false;
            if (helpSerial == session->serial && helpSelected == session->selected)
                return true;
            const auto serial = session->serial;
            const auto selected = session->selected;
            std::string text;
            if (!session->ReadHelp(text) || !Current() || session->serial != serial || session->selected != selected)
                return false;
            RECT client{};
            if (!GetClientRect(window, &client))
                return false;
            TEXTMETRICW metrics{};
            HDC dc = GetDC(window);
            if (!dc)
                return false;
            const auto previous = SelectObject(dc, font);
            const bool metricOK = GetTextMetricsW(dc, &metrics) != FALSE;
            SelectObject(dc, previous);
            ReleaseDC(window, dc);
            if (!metricOK || client.right < 104)
                return false;
            const auto box = Arrange(client, metrics.tmHeight).help;
            const std::wstring wide(text.begin(), text.end());
            MoveWindow(help, box.left + 8, box.top + metrics.tmHeight + 4, box.right - box.left - 16,
                box.bottom - box.top - metrics.tmHeight - 10, TRUE);
            SetWindowTextW(help, wide.c_str());
            if (!Current() || session->serial != serial || session->selected != selected)
                return false;
            helpSerial = serial;
            helpSelected = selected;
            return true;
        }

        void Service()
        {
            auto lease = shared_from_this();
            if (servicing)
                return;
            Session::Busy service(servicing);
            expectedFocus = GetFocus();
            if (!Current() || !session)
            {
                Finish();
                return;
            }
            if (!session->open)
            {
                if (closing || started)
                {
                    Finish();
                    return;
                }
                started = true;
                if (!session->Open(Language()))
                {
                    Finish();
                    return;
                }
            }
            else
                session->Frame();
            if (!Current() || !session->open || !session->ready ||
                (paintedSerial != session->serial && !Publish()) || !PublishHelp())
                Finish();
        }

        void Finish() noexcept
        {
            closing = true;
            if (session)
                session->Close();
            if (window && IsWindow(window))
                EndDialog(window, 0);
        }

        bool Queue(int direction, bool back)
        {
            if (!session || !Current())
                return false;
            const auto index = list && IsWindow(list) ? SendMessageW(list, LB_GETCURSEL, 0, 0) : LB_ERR;
            if (!session->opening && (index == LB_ERR || size_t(index) != session->selected ||
                paintedSerial != session->serial))
                return false;
            if (!session->Queue(direction, back, session->selected))
                return false;
            PostMessageW(window, ServiceMessage, 0, 0);
            return true;
        }
    };

    inline LRESULT CALLBACK ListProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        const auto it = listBindings.find(window);
        if (it == listBindings.end())
            return DefWindowProcW(window, message, wParam, lParam);
        const auto binding = it->second;
        const auto owner = binding->owner.lock();
        try
        {
            if (owner && window == owner->list && message == WM_KILLFOCUS &&
                !binding->expectedDestroy && !owner->publishing && owner->session && owner->session->busy)
                owner->Cancel();
            if (owner && window == owner->list && owner->Input(window))
            {
                if (message == WM_GETDLGCODE)
                    return CallWindowProcW(binding->original, window, message, wParam, lParam) | DLGC_WANTARROWS | DLGC_WANTALLKEYS;
                if (message == WM_KEYDOWN && (wParam == VK_RETURN || wParam == VK_ESCAPE || wParam == VK_LEFT || wParam == VK_RIGHT))
                {
                    const int direction = wParam == VK_LEFT ? -1 : wParam == VK_RIGHT ? 1 : 0;
                    if (owner->Queue(direction, wParam == VK_ESCAPE))
                        return 0;
                }
                if (message == WM_CHAR && (wParam == VK_RETURN || wParam == VK_ESCAPE))
                    return 0;
            }
        }
        catch (...) { if (owner) owner->Retire(); }
        const auto result = CallWindowProcW(binding->original, window, message, wParam, lParam);
        if (message == WM_NCDESTROY)
        {
            listBindings.erase(window);
            if (owner && !binding->expectedDestroy && window == owner->list)
                owner->Retire();
        }
        return result;
    }

    inline INT_PTR CALLBACK PopupProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        std::shared_ptr<Popup> popup;
        const auto it = popups.find(window);
        if (it != popups.end())
            popup = it->second;
        try
        {
            if (message == WM_INITDIALOG)
            {
                popup = reinterpret_cast<Popup*>(lParam)->shared_from_this();
                popup->window = window;
                popups.emplace(window, popup);
                // The dialog font of the launcher's own dialogs, from the template
                popup->font = reinterpret_cast<HFONT>(SendMessageW(window, WM_GETFONT, 0, 0));
                if (!popup->font)
                    popup->font = reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
                popup->heading = CreateWindowExW(0, L"BUTTON", L"Plugins", WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                    8, 4, 300, 200, window, nullptr, popup->module, nullptr);
                popup->helpBox = CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                    8, 210, 300, 60, window, nullptr, popup->module, nullptr);
                popup->help = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOPREFIX,
                    16, 224, 284, 40, window, reinterpret_cast<HMENU>(0x7102), popup->module, nullptr);
                popup->confirm = CreateWindowExW(0, L"BUTTON", L"OK", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                    120, 280, 75, 23, window, reinterpret_cast<HMENU>(IDOK), popup->module, nullptr);
                if (!popup->font || !popup->heading || !popup->helpBox || !popup->help || !popup->confirm ||
                    !popup->source.Alive())
                {
                    popup->Finish();
                    return TRUE;
                }
                for (auto control : { popup->heading, popup->helpBox, popup->help, popup->confirm })
                    SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(popup->font), FALSE);
                RECT client{};
                GetClientRect(window, &client);
                popup->session = std::make_shared<Session>();
                popup->session->module = popup->module;
                popup->session->pause = false;
                // Rows are fitted in letters of the game's 8-pixel font: the list's width in the dialog font's letters
                {
                    HDC dc = GetDC(window);
                    const auto previous = dc ? SelectObject(dc, popup->font) : nullptr;
                    TEXTMETRICW metrics{};
                    const bool measured = dc && GetTextMetricsW(dc, &metrics) && metrics.tmAveCharWidth > 0;
                    if (previous) SelectObject(dc, previous);
                    if (dc) ReleaseDC(window, dc);
                    const int letters = measured ? (client.right - 40) / (metrics.tmAveCharWidth + 1) : (client.right - 48) / Native::GlyphWidth;
                    popup->session->width = letters * Native::GlyphWidth;
                }
                popup->session->owner = [weak = std::weak_ptr<Popup>(popup)]
                {
                    const auto current = weak.lock();
                    return current && current->Current();
                };
                PostMessageW(window, Popup::ServiceMessage, 0, 0);
                SetTimer(window, 1, 100, nullptr);
                return TRUE;
            }
            if (!popup)
                return FALSE;
            if (message == Popup::ServiceMessage || (message == WM_TIMER && wParam == 1))
            {
                popup->Service();
                return TRUE;
            }
            if (message == WM_COMMAND && LOWORD(wParam) == IDCANCEL)
            {
                popup->Queue(0, true);
                return TRUE;
            }
            // OK closes the dialog like the launcher's own
            if (message == WM_COMMAND && LOWORD(wParam) == IDOK && reinterpret_cast<HWND>(lParam) == popup->confirm)
            {
                popup->Finish();
                return TRUE;
            }
            if (message == WM_COMMAND && reinterpret_cast<HWND>(lParam) == popup->list && popup->Input(popup->list))
            {
                if (HIWORD(wParam) == LBN_SELCHANGE)
                {
                    const auto selected = SendMessageW(popup->list, LB_GETCURSEL, 0, 0);
                    if (popup->session->busy)
                        popup->Cancel();
                    else if (selected != LB_ERR && size_t(selected) < popup->session->view.items.size())
                    {
                        popup->session->selected = size_t(selected);
                        PostMessageW(window, Popup::ServiceMessage, 0, 0);
                    }
                }
                else if (HIWORD(wParam) == LBN_DBLCLK)
                    popup->Queue(0, false);
                return TRUE;
            }
            if (message == WM_CLOSE)
            {
                popup->Cancel();
                popup->closing = true;
                return TRUE;
            }
            if (message == WM_NCDESTROY)
            {
                popup->invalid = true;
                popup->window = nullptr;
                if (popup->session)
                    popup->session->Retire();
                popups.erase(window);
            }
        }
        catch (...)
        {
            if (popup)
                popup->Finish();
            else
                EndDialog(window, -1);
            return TRUE;
        }
        return FALSE;
    }
}

export namespace DeerMenu
{
    struct Launcher : std::enable_shared_from_this<Launcher>
    {
        inline static constexpr int EntryID = 0x7101;
        inline static constexpr UINT OpenMessage = WM_APP + 0x5E;
        TitleContext context;
        HMODULE module = nullptr;
        HWND entry = nullptr;
        bool opening = false;
        bool requested = false;
        bool invalid = false;
        const char* refusal = "title entry contract refused";

        void Remove() noexcept
        {
            const auto owned = entry;
            entry = nullptr;
            if (owned && IsWindow(owned) && GetParent(owned) == context.dialog &&
                GetDlgCtrlID(owned) == EntryID && GetDlgItem(context.dialog, EntryID) == owned &&
                TitleContext::Class(owned, L"Button") &&
                reinterpret_cast<HMODULE>(GetWindowLongPtrW(owned, GWLP_HINSTANCE)) == module)
                DestroyWindow(owned);
        }

        bool Alive() const
        {
            return !invalid && context.Alive() && entry && IsWindow(entry) &&
                GetDlgItem(context.dialog, EntryID) == entry && GetParent(entry) == context.dialog &&
                TitleContext::Class(entry, L"Button");
        }

        void Retire() noexcept
        {
            invalid = true;
            requested = false;
            for (const auto& item : popups)
                if (item.second->source.dialog == context.dialog)
                    item.second->Retire();
        }

        bool Build(HWND dialog, HMODULE owner)
        {
            auto lease = shared_from_this();
            module = owner;
            RECT band{};
            if (!owner)
            {
                refusal = "title module unavailable";
                return false;
            }
            if (!context.Capture(dialog))
            {
                refusal = "title native control identity refused";
                return false;
            }
            if (GetDlgItem(dialog, EntryID))
            {
                refusal = "title entry ID already in use";
                return false;
            }
            if (!context.Space(band))
            {
                refusal = "title client space refused";
                return false;
            }
            auto font = reinterpret_cast<HFONT>(SendMessageW(context.controls[2], WM_GETFONT, 0, 0));
            // WM_GETFONT returns null when the stock control uses SYSTEM_FONT.
            if (!font) font = reinterpret_cast<HFONT>(GetStockObject(SYSTEM_FONT));
            HDC dc = GetDC(dialog);
            if (!dc || !font)
            {
                if (dc) ReleaseDC(dialog, dc);
                refusal = "title native font unavailable";
                return false;
            }
            const auto previous = SelectObject(dc, font);
            SIZE extent{};
            TEXTMETRICW metric{};
            const bool fits = GetTextMetricsW(dc, &metric) && metric.tmHeight >= 12 &&
                GetTextExtentPoint32W(dc, L"Plugins", 7, &extent) &&
                extent.cx + 12 <= band.right - band.left && extent.cy + 4 <= band.bottom - band.top;
            if (previous) SelectObject(dc, previous);
            ReleaseDC(dialog, dc);
            if (!fits || !context.Alive())
            {
                refusal = fits ? "title source retired during text measurement" : "title native text does not fit";
                return false;
            }
            entry = CreateWindowExW(0, L"BUTTON", L"Plugins", WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
                band.left, band.top, band.right - band.left, band.bottom - band.top, dialog,
                reinterpret_cast<HMENU>(EntryID), owner, nullptr);
            if (!Alive())
            {
                Remove();
                return false;
            }
            SendMessageW(entry, WM_SETFONT, reinterpret_cast<WPARAM>(font), FALSE);
            if (!Alive())
            {
                Remove();
                return false;
            }
            ShowWindow(entry, SW_SHOW);
            if (!Alive()) { Remove(); return false; }
            return true;
        }

        bool Request(HWND sender)
        {
            if (!Alive() || sender != entry || opening || requested || !IsWindowEnabled(context.dialog))
                return false;
            requested = true;
            if (!PostMessageW(context.dialog, OpenMessage, 0, 0))
            {
                requested = false;
                return false;
            }
            return true;
        }

        void Open()
        {
            auto lease = shared_from_this();
            if (!requested || opening || !Alive() || !IsWindowEnabled(context.dialog))
                return;
            requested = false;
            Session::Busy scope(opening);
            auto popup = std::make_shared<Popup>();
            popup->source = context;
            popup->module = module;
            // An owned empty Win32 dialog template; all row controls are built
            // transactionally by Popup. The native launcher remains its owner.
            std::vector<WORD> bytes;
            bytes.resize((sizeof(DLGTEMPLATE) + 1) / 2);
            auto* definition = reinterpret_cast<DLGTEMPLATE*>(bytes.data());
            // The font and size of the launcher's own Options dialog (MS Sans Serif 8, 186 x 151 dialog units)
            definition->style = WS_POPUP | WS_CAPTION | WS_SYSMENU | DS_MODALFRAME | DS_SETFONT | DS_CENTER;
            definition->dwExtendedStyle = 0;
            definition->cdit = 0;
            definition->x = 0; definition->y = 0;
            definition->cx = 230; definition->cy = 190;
            bytes.push_back(0); // no menu
            bytes.push_back(0); // native dialog class
            for (const wchar_t c : std::wstring(L"Plugins")) bytes.push_back(WORD(c));
            bytes.push_back(0);
            bytes.push_back(8);
            for (const wchar_t c : std::wstring(L"MS Sans Serif")) bytes.push_back(WORD(c));
            bytes.push_back(0);
            if (!Alive())
                return;
            DialogBoxIndirectParamW(module, reinterpret_cast<const DLGTEMPLATE*>(bytes.data()),
                context.dialog, PopupProcedure, reinterpret_cast<LPARAM>(popup.get()));
            if (Alive())
                SetFocus(entry);
        }
    };

    inline std::unordered_map<HWND, std::shared_ptr<Launcher>> launchers;
}

export namespace DeerMenu::Sprites
{
    // Letters of the game's own menu words (menu texture) and its digits (interface2 texture, three quarters size),
    // cut in three bands that follow the italic slant; letters the words lack are put together from parts of others
    struct GlyphPiece { uint8_t texture, x0, y0, x1, y1; float dx, dy, scale; };
    struct Glyph { char code; uint8_t first, count; float advance; };
    inline constexpr GlyphPiece GlyphPieces[] =
    {
        { 0, 22, 105, 26, 111, 0.0f, 0.0f, 1.0f }, { 0, 20, 111, 27, 115, -2.0f, 6.0f, 1.0f },
        { 0, 165, 219, 170, 224, 0.0f, 8.0f, 1.0f }, { 1, 233, 0, 247, 16, -1.5f, 1.5f, 0.75f },
        { 0, 22, 105, 26, 111, 0.0f, 0.0f, 1.0f }, { 0, 130, 165, 136, 171, -1.0f, 0.0f, 1.0f },
        { 0, 129, 171, 135, 175, -2.0f, 6.0f, 1.0f }, { 0, 129, 175, 135, 180, -2.0f, 10.0f, 1.0f },
        { 0, 10, 210, 15, 216, 1.0f, 0.0f, 1.0f }, { 0, 10, 216, 14, 220, 1.0f, 6.0f, 1.0f },
        { 0, 9, 220, 14, 225, 0.0f, 10.0f, 1.0f }, { 0, 165, 219, 170, 224, 0.0f, 8.0f, 1.0f },
        { 0, 7, 111, 14, 115, 0.0f, 6.0f, 1.0f }, { 0, 165, 219, 170, 224, 0.0f, 8.0f, 1.0f },
        { 0, 7, 111, 14, 115, 0.0f, 6.0f, 1.0f }, { 1, 49, 0, 65, 16, -1.5f, 1.5f, 0.75f },
        { 1, 68, 0, 77, 16, -1.5f, 1.5f, 0.75f }, { 1, 79, 0, 95, 16, -1.5f, 1.5f, 0.75f },
        { 1, 98, 0, 114, 16, -1.5f, 1.5f, 0.75f }, { 1, 118, 0, 134, 16, -1.5f, 1.5f, 0.75f },
        { 1, 137, 0, 153, 16, -1.5f, 1.5f, 0.75f }, { 1, 156, 0, 172, 16, -1.5f, 1.5f, 0.75f },
        { 1, 175, 0, 190, 16, -1.5f, 1.5f, 0.75f }, { 1, 194, 0, 210, 16, -1.5f, 1.5f, 0.75f },
        { 1, 212, 0, 228, 16, -1.5f, 1.5f, 0.75f }, { 0, 165, 219, 170, 224, 0.0f, 3.0f, 1.0f },
        { 0, 165, 219, 170, 224, 0.0f, 8.0f, 1.0f }, { 0, 166, 210, 175, 216, 0.0f, 0.0f, 1.0f },
        { 0, 164, 216, 175, 220, -2.0f, 6.0f, 1.0f }, { 0, 164, 220, 170, 225, -2.0f, 10.0f, 1.0f },
        { 0, 156, 120, 165, 126, 0.0f, 0.0f, 1.0f }, { 0, 155, 126, 166, 130, -1.0f, 6.0f, 1.0f },
        { 0, 155, 130, 166, 135, -1.0f, 10.0f, 1.0f }, { 0, 40, 165, 52, 173, -1.0f, 0.0f, 1.0f },
        { 0, 3, 218, 14, 225, -2.0f, 8.0f, 1.0f }, { 0, 130, 165, 141, 171, -1.0f, 0.0f, 1.0f },
        { 0, 129, 171, 140, 175, -2.0f, 6.0f, 1.0f }, { 0, 129, 175, 140, 180, -2.0f, 10.0f, 1.0f },
        { 0, 4, 210, 15, 216, -1.0f, 0.0f, 1.0f }, { 0, 3, 216, 14, 220, -2.0f, 6.0f, 1.0f },
        { 0, 3, 220, 14, 225, -2.0f, 10.0f, 1.0f }, { 0, 3, 105, 14, 111, -1.0f, 0.0f, 1.0f },
        { 0, 2, 111, 14, 115, -2.0f, 6.0f, 1.0f }, { 0, 2, 115, 13, 120, -2.0f, 10.0f, 1.0f },
        { 0, 3, 105, 14, 111, -1.0f, 0.0f, 1.0f }, { 0, 2, 111, 14, 115, -2.0f, 6.0f, 1.0f },
        { 0, 21, 115, 26, 120, -1.0f, 10.0f, 1.0f }, { 0, 40, 225, 53, 231, -1.0f, 0.0f, 1.0f },
        { 0, 40, 231, 51, 235, -1.0f, 6.0f, 1.0f }, { 0, 39, 235, 50, 240, -2.0f, 10.0f, 1.0f },
        { 0, 101, 90, 111, 96, 0.0f, 0.0f, 1.0f }, { 0, 100, 96, 112, 100, -1.0f, 6.0f, 1.0f },
        { 0, 99, 100, 111, 105, -2.0f, 10.0f, 1.0f }, { 0, 22, 105, 26, 111, 0.0f, 0.0f, 1.0f },
        { 0, 20, 111, 27, 115, -2.0f, 6.0f, 1.0f }, { 0, 21, 115, 26, 120, -1.0f, 10.0f, 1.0f },
        { 0, 53, 210, 59, 216, 5.0f, 0.0f, 1.0f }, { 0, 52, 216, 58, 220, 4.0f, 6.0f, 1.0f },
        { 0, 47, 220, 58, 225, -1.0f, 10.0f, 1.0f }, { 0, 22, 105, 26, 111, 0.0f, 0.0f, 1.0f },
        { 0, 20, 111, 27, 115, -2.0f, 6.0f, 1.0f }, { 0, 21, 115, 26, 120, -1.0f, 10.0f, 1.0f },
        { 0, 19, 195, 24, 201, 5.0f, 0.0f, 1.0f }, { 0, 18, 201, 23, 205, 4.0f, 6.0f, 1.0f },
        { 0, 18, 205, 23, 210, 4.0f, 10.0f, 1.0f }, { 0, 136, 75, 142, 81, -1.0f, 0.0f, 1.0f },
        { 0, 136, 81, 146, 85, -1.0f, 6.0f, 1.0f }, { 0, 136, 85, 146, 90, -1.0f, 10.0f, 1.0f },
        { 0, 42, 180, 55, 186, -1.0f, 0.0f, 1.0f }, { 0, 42, 186, 55, 190, -1.0f, 6.0f, 1.0f },
        { 0, 41, 190, 54, 195, -2.0f, 10.0f, 1.0f }, { 0, 187, 120, 198, 126, 0.0f, 0.0f, 1.0f },
        { 0, 186, 126, 198, 130, -1.0f, 6.0f, 1.0f }, { 0, 186, 130, 197, 135, -1.0f, 10.0f, 1.0f },
        { 0, 119, 210, 130, 216, 1.0f, 0.0f, 1.0f }, { 0, 117, 216, 129, 220, -1.0f, 6.0f, 1.0f },
        { 0, 116, 220, 128, 225, -2.0f, 10.0f, 1.0f }, { 0, 41, 165, 51, 171, 0.0f, 0.0f, 1.0f },
        { 0, 40, 171, 52, 175, -1.0f, 6.0f, 1.0f }, { 0, 40, 175, 45, 180, -1.0f, 10.0f, 1.0f },
        { 0, 132, 210, 144, 216, -1.0f, 0.0f, 1.0f }, { 0, 132, 216, 143, 220, -1.0f, 6.0f, 1.0f },
        { 0, 131, 220, 143, 225, -2.0f, 10.0f, 1.0f }, { 0, 234, 195, 244, 201, 0.0f, 0.0f, 1.0f },
        { 0, 233, 201, 244, 205, -1.0f, 6.0f, 1.0f }, { 0, 233, 205, 244, 210, -1.0f, 10.0f, 1.0f },
        { 0, 198, 135, 210, 141, -1.0f, 0.0f, 1.0f }, { 0, 198, 141, 209, 145, -1.0f, 6.0f, 1.0f },
        { 0, 197, 145, 209, 150, -2.0f, 10.0f, 1.0f }, { 0, 43, 75, 54, 81, -2.0f, 0.0f, 1.0f },
        { 0, 45, 81, 52, 85, 0.0f, 6.0f, 1.0f }, { 0, 45, 85, 51, 90, 0.0f, 10.0f, 1.0f },
        { 0, 48, 210, 59, 216, 0.0f, 0.0f, 1.0f }, { 0, 47, 216, 58, 220, -1.0f, 6.0f, 1.0f },
        { 0, 47, 220, 58, 225, -1.0f, 10.0f, 1.0f }, { 0, 3, 75, 13, 81, -2.0f, 0.0f, 1.0f },
        { 0, 3, 81, 12, 85, -2.0f, 6.0f, 1.0f }, { 0, 3, 85, 12, 90, -2.0f, 10.0f, 1.0f },
        { 0, 101, 105, 118, 111, -2.0f, 0.0f, 1.0f }, { 0, 101, 111, 116, 115, -2.0f, 6.0f, 1.0f },
        { 0, 101, 115, 115, 120, -2.0f, 10.0f, 1.0f }, { 0, 14, 195, 24, 201, 0.0f, 0.0f, 1.0f },
        { 0, 14, 201, 23, 205, 0.0f, 6.0f, 1.0f }, { 0, 13, 205, 23, 210, -1.0f, 10.0f, 1.0f },
        { 0, 28, 210, 39, 216, -2.0f, 0.0f, 1.0f }, { 0, 29, 216, 37, 220, -1.0f, 6.0f, 1.0f },
        { 0, 29, 220, 36, 225, -1.0f, 10.0f, 1.0f }, { 1, 79, 0, 95, 16, -1.5f, 1.5f, 0.75f },
    };
    inline constexpr Glyph Glyphs[] =
    {
        { '!', 0, 3, 4.0f }, { '%', 3, 1, 7.5f }, { '\'', 4, 1, 4.0f }, { '(', 5, 3, 5.0f }, { ')', 8, 3, 5.0f },
        { ',', 11, 1, 3.0f }, { '-', 12, 1, 6.0f }, { '.', 13, 1, 3.0f }, { '/', 14, 1, 6.0f }, { '0', 15, 1, 9.0f },
        { '1', 16, 1, 3.75f }, { '2', 17, 1, 9.0f }, { '3', 18, 1, 9.0f }, { '4', 19, 1, 9.0f }, { '5', 20, 1, 9.0f },
        { '6', 21, 1, 9.0f }, { '7', 22, 1, 8.25f }, { '8', 23, 1, 9.0f }, { '9', 24, 1, 9.0f }, { ':', 25, 2, 3.0f },
        { '?', 27, 3, 7.0f }, { 'A', 30, 3, 8.0f }, { 'B', 33, 2, 9.0f }, { 'C', 35, 3, 9.0f }, { 'D', 38, 3, 9.0f },
        { 'E', 41, 3, 9.0f }, { 'F', 44, 3, 9.0f }, { 'G', 47, 3, 10.0f }, { 'H', 50, 3, 9.0f }, { 'I', 53, 3, 4.0f },
        { 'J', 56, 3, 9.0f }, { 'K', 59, 6, 10.0f }, { 'L', 65, 3, 7.0f }, { 'M', 68, 3, 11.0f }, { 'N', 71, 3, 9.0f },
        { 'O', 74, 3, 10.0f }, { 'P', 77, 3, 9.0f }, { 'Q', 80, 3, 10.0f }, { 'R', 83, 3, 9.0f }, { 'S', 86, 3, 10.0f },
        { 'T', 89, 3, 8.0f }, { 'U', 92, 3, 9.0f }, { 'V', 95, 3, 8.0f }, { 'W', 98, 3, 13.0f }, { 'X', 101, 3, 9.0f },
        { 'Y', 104, 3, 7.0f }, { 'Z', 107, 1, 9.0f },
    };

    // The game's menu sprites: a record names a texture, its place and size (x scaled by 4:3), four texture corners
    // and a colour; setup applies the parts the flags name, draw fills it
    constexpr size_t RecordSize = 124;
    constexpr int SetPosition = 1, SetColour = 2, SetShape = 4, SetTexture = 8, SetCorners = 16;
    constexpr size_t MenuTexture = 0, InterfaceTexture = 1;
    // One texel of the pause panel on screen
    constexpr float TexelX = 0.399f / 256.0f, TexelY = 0.203f / 96.0f;
    // The pause panel in its texture: the column and row that stretch, and the inside where its words are
    constexpr float PanelTop = 160.0f, StretchX = 123.0f, StretchY = 213.0f;
    constexpr float InsideX0 = 56.0f, InsideY0 = 177.0f, InsideX1 = 190.0f, InsideY1 = 232.0f;
    constexpr float RowPitch = 20.0f, FirstRow = 16.0f, LetterGap = 1.0f, SpaceWidth = 6.0f;

    struct Api
    {
        int (__cdecl* setup)(uint8_t*, int){};
        int (__cdecl* draw)(uint8_t*, int){};
        int (__cdecl* state)(int, int){};
        uint8_t* panel{};
        std::array<uint8_t*, 3> buttons{};      // up, down, enter
        std::array<uint8_t*, 3> highlights{};   // the bright words of the three items
    };
    inline Api api;
    alignas(16) inline uint8_t records[2][RecordSize]{};
    inline void* textures[2]{};

    inline float& Value(uint8_t* record, size_t offset)
    {
        return *reinterpret_cast<float*>(record + offset);
    }

    constexpr uint32_t Colour(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
    {
        return uint32_t(r) | uint32_t(g) << 8 | uint32_t(b) << 16 | uint32_t(a) << 24;
    }
    constexpr uint32_t Bright = Colour(255, 255, 255, 255);
    // The game's unselected words: grey green on the panel
    constexpr uint32_t Dim = Colour(209, 222, 209, 178);
    constexpr uint32_t Unavailable = Colour(209, 222, 209, 100);

    // Our two records take the textures the game's own pause records loaded
    inline bool Ready()
    {
        void* const sources[2] = { *reinterpret_cast<void**>(api.buttons[0] + 100), *reinterpret_cast<void**>(api.panel + 100) };
        for (size_t i = 0; i < 2; ++i)
        {
            if (!sources[i])
                return false;
            if (textures[i] == sources[i])
                continue;
            auto record = records[i];
            std::memcpy(record, i == MenuTexture ? api.buttons[0] : api.panel, 32);
            *reinterpret_cast<void**>(record + 100) = sources[i];
            Value(record, 40) = 0.1f;
            Value(record, 44) = 0.1f;
            api.setup(record, SetPosition | SetColour | SetShape | SetTexture | SetCorners);
            textures[i] = sources[i];
        }
        return true;
    }

    // A texel rectangle of a texture to a rectangle on screen (top left, size)
    inline void Quad(size_t texture, float u0, float v0, float u1, float v1, float x, float y, float w, float h, uint32_t colour)
    {
        auto record = records[texture];
        Value(record, 32) = x;
        Value(record, 36) = y;
        Value(record, 40) = w;
        Value(record, 44) = h;
        Value(record, 48) = 0.0f;
        const float corners[8] = { u0 / 256.0f, v1 / 256.0f, u1 / 256.0f, v1 / 256.0f, u1 / 256.0f, v0 / 256.0f, u0 / 256.0f, v0 / 256.0f };
        std::memcpy(record + 52, corners, sizeof(corners));
        std::memcpy(record + 84, &colour, 4);
        api.setup(record, SetPosition | SetColour | SetShape | SetCorners);
        api.draw(record, 1);
    }

    // A game record moved up or down
    inline void Moved(const uint8_t* source, float dy)
    {
        alignas(16) uint8_t copy[RecordSize];
        std::memcpy(copy, source, RecordSize);
        Value(copy, 36) += dy;
        api.setup(copy, SetPosition);
        api.draw(copy, 1);
    }

    // A panel like the game's pause panel, wider and taller by stretching its middle column and row; empty or with
    // the game's own three words
    struct Panel
    {
        float left{}, top{}, wider{}, taller{};

        float X(float local) const { return left + local * TexelX; }
        float Y(float local) const { return top - local * TexelY; }
        float Width() const { return 256.0f + wider; }

        void Piece(size_t texture, float u0, float v0, float u1, float v1, float x0, float y0, float x1, float y1, uint32_t colour) const
        {
            if (x1 > x0 && y1 > y0)
                Quad(texture, u0, v0, u1, v1, X(x0), Y(y0), (x1 - x0) * TexelX, (y1 - y0) * TexelY, colour);
        }

        void Draw(bool empty) const
        {
            struct Segment { float s0, s1, d0, d1; };
            struct Rect { float x0, y0, x1, y1; };
            const Segment columns[3] = { { 0.0f, StretchX, 0.0f, StretchX }, { StretchX, StretchX + 1.0f, StretchX, StretchX + wider },
                { StretchX, 256.0f, StretchX + wider, 256.0f + wider } };
            const float row = StretchY - PanelTop;
            const Segment rows[3] = { { PanelTop, StretchY, 0.0f, row }, { StretchY, StretchY + 1.0f, row, row + taller },
                { StretchY, 256.0f, row + taller, 96.0f + taller } };
            const auto map = [](const Segment& segment, float s)
            {
                return segment.d0 + (s - segment.s0) * (segment.d1 - segment.d0) / (segment.s1 - segment.s0);
            };
            for (const auto& c : columns)
            {
                for (const auto& r : rows)
                {
                    if (c.d1 <= c.d0 || r.d1 <= r.d0)
                        continue;
                    // The parts of this cell outside the panel's words
                    std::array<Rect, 4> parts{};
                    size_t count = 0;
                    const Rect cell{ c.s0, r.s0, c.s1, r.s1 };
                    if (!empty || cell.x0 >= InsideX1 || cell.x1 <= InsideX0 || cell.y0 >= InsideY1 || cell.y1 <= InsideY0)
                        parts[count++] = cell;
                    else
                    {
                        const float y0 = std::max(cell.y0, InsideY0), y1 = std::min(cell.y1, InsideY1);
                        if (cell.y0 < InsideY0)
                            parts[count++] = { cell.x0, cell.y0, cell.x1, InsideY0 };
                        if (cell.y1 > InsideY1)
                            parts[count++] = { cell.x0, InsideY1, cell.x1, cell.y1 };
                        if (cell.x0 < InsideX0)
                            parts[count++] = { cell.x0, y0, InsideX0, y1 };
                        if (cell.x1 > InsideX1)
                            parts[count++] = { InsideX1, y0, cell.x1, y1 };
                    }
                    for (size_t i = 0; i < count; ++i)
                    {
                        const auto& p = parts[i];
                        Piece(InterfaceTexture, p.x0, p.y0, p.x1, p.y1, map(c, p.x0), map(r, p.y0), map(c, p.x1), map(r, p.y1), Bright);
                    }
                }
            }
            // The inside is one even colour: a single texel of it fills the stretched inside
            if (empty)
                Piece(InterfaceTexture, 100.25f, 213.25f, 100.75f, 213.75f, InsideX0, InsideY0 - PanelTop,
                    InsideX1 + wider, InsideY1 - PanelTop + taller, Bright);
        }
    };

    inline const Glyph& Find(char code)
    {
        if (code >= 'a' && code <= 'z')
            code = char(code - 'a' + 'A');
        const Glyph* dash = nullptr;
        for (const auto& glyph : Glyphs)
        {
            if (glyph.code == code)
                return glyph;
            if (glyph.code == '-')
                dash = &glyph;
        }
        return *dash;
    }

    inline float TextWidth(std::string_view text)
    {
        float width = 0.0f;
        for (char ch : text)
            width += ch == ' ' ? SpaceWidth : Find(ch).advance + LetterGap;
        return std::max(0.0f, width - LetterGap);
    }

    // Text with its letter cells' top at a panel row (panel texels), centred at a panel column
    inline void Text(const Panel& panel, std::string_view text, float centre, float top, float scale, uint32_t colour)
    {
        float pen = centre - TextWidth(text) * scale * 0.5f;
        for (char ch : text)
        {
            if (ch == ' ')
            {
                pen += SpaceWidth * scale;
                continue;
            }
            const auto& glyph = Find(ch);
            for (size_t i = glyph.first; i < size_t(glyph.first + glyph.count); ++i)
            {
                const auto& piece = GlyphPieces[i];
                const float x = pen + piece.dx * scale, y = top + piece.dy * scale;
                panel.Piece(piece.texture, piece.x0, piece.y0, piece.x1, piece.y1, x, y,
                    x + (piece.x1 - piece.x0) * piece.scale * scale, y + (piece.y1 - piece.y0) * piece.scale * scale, colour);
            }
            pen += (glyph.advance + LetterGap) * scale;
        }
    }

    // A key cap of the menu texture (the up or down arrow) at a panel spot, a row high
    inline void Arrow(const Panel& panel, bool up, float x, float top)
    {
        const float* corners = reinterpret_cast<const float*>(api.buttons[up ? 0 : 1] + 52);
        constexpr float size = 13.0f;
        panel.Piece(MenuTexture, corners[0] * 256.0f, corners[5] * 256.0f, corners[2] * 256.0f, corners[1] * 256.0f,
            x, top + 1.0f, x + size, top + 1.0f + size, Bright);
    }

    inline void Init(uintptr_t menuDraw)
    {
        api.state = reinterpret_cast<decltype(api.state)>(injector::GetBranchDestination(menuDraw + 0x16).as_int());
        auto page = hook::pattern("6A 01 68 ? ? ? ? E8 ? ? ? ? 6A 01 68 ? ? ? ? E8 ? ? ? ? 6A 01 68 ? ? ? ? E8 ? ? ? ? "
            "6A 01 68 ? ? ? ? E8 ? ? ? ? A1 ? ? ? ? 83 C4 20 2B C7 74 32 48 74 1B 48 0F 85 ? ? ? ? "
            "6A 01 68 ? ? ? ? E8 ? ? ? ? 83 C4 08 E9 ? ? ? ? 6A 01 68 ? ? ? ? E8 ? ? ? ? 83 C4 08 E9 ? ? ? ? 6A 01 68");
        api.panel = *page.get_first<uint8_t*>(3);
        api.draw = reinterpret_cast<decltype(api.draw)>(injector::GetBranchDestination(page.get_first(7)).as_int());
        api.buttons = { *page.get_first<uint8_t*>(15), *page.get_first<uint8_t*>(27), *page.get_first<uint8_t*>(39) };
        api.highlights = { *page.get_first<uint8_t*>(0x71), *page.get_first<uint8_t*>(0x5D), *page.get_first<uint8_t*>(0x49) };
        auto setup = hook::pattern("6A 02 68 ? ? ? ? A2 ? ? ? ? A2 ? ? ? ? A2 ? ? ? ? A2 ? ? ? ? E8");
        api.setup = reinterpret_cast<decltype(api.setup)>(injector::GetBranchDestination(setup.get_first(27)).as_int());
    }
}

export namespace DeerMenu
{
    struct Pause : std::enable_shared_from_this<Pause>
    {
        PauseContext context;
        HMODULE module = nullptr;
        void* font = nullptr;
        std::shared_ptr<Session> session;
        bool requested = false;
        bool retired = false;
        bool entryPainted = false;
        bool padBack = false;
        std::string notice;
        uint64_t paintedGeneration = 0, paintedSerial = 0;

        static void Write(uintptr_t address, int value)
        {
            *reinterpret_cast<int*>(address) = value;
        }

        bool Current() const
        {
            return !retired && context.Current();
        }

        bool Private() const
        {
            return Current() && session && session->open && (Game(), game_menuSelection.get()) == 3;
        }

        void Retire() noexcept
        {
            retired = true;
            requested = false;
            if (session) session->Retire();
        }

        // Only the complete bridge's zero-depth/pre-engine-retirement scope
        // may call this; no C++/DLL-detach destructor invokes native APIs.
        void Release()
        {
            Retire();
            if (font)
            {
                const auto owned = font;
                font = nullptr;
                Native::DestroyTypeface(owned);
            }
        }

        bool Prepare(HMODULE owner)
        {
            auto lease = shared_from_this();
            module = owner;
            if (retired || !owner || !context.Current()) return false;
            if (!session)
            {
                session = std::make_shared<Session>();
                session->module = module;
                session->pause = true;
                // Rows are fitted in letters; the panel grows to the longest
                session->width = 44 * Native::GlyphWidth;
                session->view.window = 5;
                session->owner = [weak = std::weak_ptr<Pause>(lease)]
                {
                    const auto host = weak.lock();
                    return host && host->Current() && (Game(), game_menuSelection.get()) == 3;
                };
            }
            return Current();
        }

        bool Key(int key)
        {
            auto lease = shared_from_this();
            if (!Current()) return false;
            const int nativeSelection = (Game(), game_menuSelection.get());
            if (Private())
            {
                if (!notice.empty())
                {
                    if (key == 128 || key == 171) notice.clear();
                    return true;
                }
                const bool painted = session->ready && paintedGeneration == session->generation &&
                    paintedSerial == session->serial;
                // Back can cancel construction/unseen changes; every other
                // owned command waits for that exact native view to be drawn.
                if (key != 128 && !painted &&
                    (key == 147 || key == 148 || key == 149 || key == 150 || key == 171))
                    return true;
                if (key == 147 || key == 148)
                {
                    if (session->busy) session->Close();
                    else session->Move(key == 147 ? -1 : 1);
                    return true;
                }
                if (key == 128 || key == 171 || key == 149 || key == 150)
                    return session->Queue(key == 149 ? -1 : key == 150 ? 1 : 0,
                        key == 128, session->selected);
                // Do not let confirmation aliases become latent stock input.
                if (key == 121 || key == 110) return true;
                return false;
            }
            if (!entryPainted) return false;
            if (nativeSelection < 0 || nativeSelection > 3) return false;
            if (key == 147 || key == 148)
            {
                // Shown in the order 0, 1, Plugins (3), 2
                constexpr int order[4] = { 0, 1, 3, 2 };
                const auto place = int(std::find(std::begin(order), std::end(order), nativeSelection) - std::begin(order));
                Write((Game(), reinterpret_cast<uintptr_t>(game_menuSelection.get_ptr())), order[std::clamp(place + (key == 147 ? -1 : 1), 0, 3)]);
                return true;
            }
            if (key == 171 && nativeSelection == 3)
            {
                requested = true;
                return true;
            }
            return false;
        }

        void Frame(const std::string& language)
        {
            auto lease = shared_from_this();
            if (!Current())
            {
                if (session) session->Close();
                requested = false;
                return;
            }
            if (requested)
            {
                requested = false;
                if ((Game(), game_menuSelection.get()) == 3 && session && !session->open)
                    session->Open(language);
            }
            else if (session && session->open)
                session->Frame();
            // The pad's back button goes up a level in our pages, as Esc does
            XINPUT_STATE pad{};
            DWORD slot = 0;
            const bool back = PadInput::Read(pad, slot) && (pad.Gamepad.wButtons & XINPUT_GAMEPAD_B);
            if (back && !padBack && Private())
                Key(128);
            padBack = back;
        }

        // The pause page in the game's menu sprites: its panel and words with Plugins added between its second and
        // third items, or our pages in a panel like it grown to their rows; the game's arrow and Enter keys below
        bool Paint()
        {
            auto lease = shared_from_this();
            if (!Current())
                return false;
            using namespace Sprites;
            const float nativeLeft = Value(api.panel, 32), nativeTop = Value(api.panel, 36);
            const auto frame = [&](float wider, float taller)
            {
                Panel result;
                result.wider = wider;
                result.taller = taller;
                result.left = nativeLeft - wider * TexelX * 0.5f;
                result.top = nativeTop + taller * TexelY * 0.5f;
                return result;
            };
            const auto keys = [&](const Panel& shown)
            {
                const float dy = shown.top - nativeTop - shown.taller * TexelY;
                for (auto button : api.buttons)
                    Moved(button, dy);
            };
            const float centre = (InsideX0 + InsideX1) * 0.5f;
            if (!Private())
            {
                const auto shown = frame(0.0f, RowPitch);
                shown.Draw(false);
                const int selection = (Game(), game_menuSelection.get());
                const float dy = shown.top - nativeTop;
                if (selection >= 0 && selection < 3)
                    Moved(api.highlights[selection], dy - (selection == 2 ? RowPitch * TexelY : 0.0f));
                // Where the third item was before the panel grew
                Text(shown, PlainText(RWGTAMenu::PluginText(FixSettings(), Language(), "menu.plugins")), centre,
                    FirstRow + 2.0f * RowPitch, 1.0f, selection == 3 ? Bright : Dim);
                keys(shown);
                entryPainted = true;
                return true;
            }
            if (!session->ready || session->view.items.size() > session->view.window + 1)
                return false;
            const auto serial = session->serial;
            const auto generation = session->generation;
            const View view = session->view;
            const size_t selected = session->selected;
            struct Line
            {
                std::string text;
                uint32_t colour;
                float scale;
            };
            std::vector<Line> lines;
            size_t firstItem = 0;
            if (!notice.empty())
            {
                std::vector<std::string> pieces;
                if (!SplitText(notice, session->width, pieces))
                    return false;
                for (auto& piece : pieces)
                    lines.push_back({ piece, Dim, 1.0f });
            }
            else
            {
                // Only the restart warning stands above the rows, smaller; the game's lists have no titles
                if (session->model.RestartRequired())
                    for (const auto& heading : view.headings)
                        lines.push_back({ heading, Dim, 0.8f });
                firstItem = lines.size();
                for (size_t i = 0; i < view.items.size(); ++i)
                {
                    const auto& item = view.items[i];
                    lines.push_back({ item.text, i == selected ? Bright : item.enabled ? Dim : Unavailable, 1.0f });
                }
            }
            float widest = 0.0f;
            for (const auto& line : lines)
                widest = std::max(widest, TextWidth(line.text) * line.scale);
            const bool marks = notice.empty() && (view.Above() || view.Below());
            const float room = InsideX1 - InsideX0 - (marks ? 44.0f : 12.0f);
            // The panel grows at most to keep its whole frame inside the picture with the game's margin; wider rows
            // get smaller letters
            constexpr float most = 200.0f;
            float scale = 1.0f, wider = std::max(0.0f, widest - room);
            if (wider > most)
            {
                scale = (room + most) / widest;
                wider = most;
            }
            const float taller = lines.size() > 3 ? float(lines.size() - 3) * RowPitch : 0.0f;
            const auto shown = frame(wider, taller);
            shown.Draw(true);
            const float top = FirstRow + (lines.size() < 3 ? float(3 - lines.size()) * RowPitch * 0.5f : 0.0f);
            for (size_t i = 0; i < lines.size(); ++i)
                Text(shown, lines[i].text, centre + wider * 0.5f, top + float(i) * RowPitch, scale * lines[i].scale, lines[i].colour);
            if (marks)
            {
                const float x = InsideX1 + wider - 18.0f;
                if (view.Above())
                    Arrow(shown, true, x, top + float(firstItem) * RowPitch);
                if (view.Below())
                    Arrow(shown, false, x, top + float(firstItem + view.Shown() - 1) * RowPitch);
            }
            keys(shown);
            paintedGeneration = generation;
            paintedSerial = serial;
            return true;
        }
    };
}

export namespace DeerMenu
{
    inline HMODULE module = nullptr;
    inline std::shared_ptr<Pause> pauseHost;
    inline SafetyHookInline titleHook, keyHook, menuHook, menuDrawHook, drawHook, frameHook, appHook;
    inline bool attempted = false, draining = false, stopping = false;
    inline unsigned frameDepth = 0, drawDepth = 0, inputDepth = 0, menuDepth = 0, appDepth = 0, titleDepth = 0;

    struct Depth
    {
        unsigned& value;
        explicit Depth(unsigned& depth) : value(depth) { ++value; }
        ~Depth() { --value; }
    };

    // Failures only; each distinct message is reported once.
    inline void Log(const char* message) noexcept
    {
        try
        {
            static std::set<std::string> emitted;
            if (emitted.size() >= 24 || !emitted.emplace(message).second)
                return;
            OutputDebugStringA("Deer Avenger 4 native menu: ");
            OutputDebugStringA(message);
            OutputDebugStringA("\n");
        }
        catch (...) {}
    }

    inline bool Outside()
    {
        return frameDepth == 0 && drawDepth == 0 && inputDepth == 0 && menuDepth == 0 && appDepth == 0 && titleDepth == 0;
    }

    inline void Cancel(const char* reason) noexcept
    {
        auto host = pauseHost;
        if (host) host->Retire();
        Log(reason);
    }

    inline void ReleasePause()
    {
        auto host = pauseHost;
        pauseHost.reset();
        if (!host) return;
        // The stock pause menu has three items; never leave it on our fourth one.
        if (host->context.Alive() && (DeerMenu::Game(), game_menuPage.get()) == 5 && (DeerMenu::Game(), game_menuSelection.get()) == 3)
            Pause::Write((Game(), reinterpret_cast<uintptr_t>(game_menuSelection.get_ptr())), 2);
        host->Release();
    }

    inline void Drain()
    {
        if (!Outside() || draining) return;
        Session::Busy scope(draining);
        try
        {
            auto host = pauseHost;
            if (host && (host->retired || !host->context.Alive()))
            {
                ReleasePause();
                return;
            }
            if (host && !stopping)
                host->Frame(Language());
            RWGTA::UpdateCheck::Tick();
        }
        catch (...) { Cancel("deferred page failure; native pause restored"); }
    }

    inline INT_PTR __stdcall Title(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        Depth depth(titleDepth);
        std::shared_ptr<Launcher> host;
        const auto found = launchers.find(window);
        if (found != launchers.end()) host = found->second;
        if (host)
        {
            try
            {
                if (message == Launcher::OpenMessage)
                {
                    host->Open();
                    return TRUE;
                }
                if (message == WM_COMMAND && LOWORD(wParam) == Launcher::EntryID && HIWORD(wParam) == BN_CLICKED &&
                    host->Request(reinterpret_cast<HWND>(lParam))) return TRUE;
                if (message == WM_NCDESTROY) host->Retire();
            }
            catch (...) { host->Retire(); Log("title transaction refused; stock controls preserved"); }
        }
        const auto result = titleHook.unsafe_stdcall<INT_PTR>(window, message, wParam, lParam);
        if (message == WM_INITDIALOG)
        {
            std::shared_ptr<Launcher> created;
            try
            {
                created = std::make_shared<Launcher>();
                if (!launchers.emplace(window, created).second)
                    return result;
                if (!created->Build(window, module))
                {
                    created->Remove();
                    created->Retire();
                    launchers.erase(window);
                    Log(created->refusal);
                }
            }
            catch (...)
            {
                if (created)
                {
                    created->Remove();
                    created->Retire();
                    const auto current = launchers.find(window);
                    if (current != launchers.end() && current->second == created) launchers.erase(current);
                }
                Log("title allocation refused; exact entry rolled back");
            }
        }
        if (message == WM_NCDESTROY) launchers.erase(window);
        return result;
    }

    inline int __cdecl Key(int event, const int* data)
    {
        int result = 0;
        {
            Depth depth(inputDepth);
            auto host = pauseHost;
            bool consumed = false;
            if (!stopping && event == 18 && host && data)
            {
                try { consumed = host->Key(data[1]); }
                catch (...) { Cancel("input queue failure"); }
            }
            // Consumed keys return 1 like native handlers; everything else stays native.
            result = consumed ? 1 : keyHook.unsafe_ccall<int>(event, data);
        }
        Drain();
        return result;
    }

    inline int __cdecl Menu()
    {
        Depth depth(menuDepth);
        auto host = pauseHost;
        if (!stopping && host && host->Private())
        {
            // Clear the pause menu's command slots and return 0, as the game does
            // when no item is chosen; our commands are handled in Key.
            for (uintptr_t address = (Game(), reinterpret_cast<uintptr_t>(game_menuCommands.get_ptr())); address <= (Game(), reinterpret_cast<uintptr_t>(game_menuCommands.get_ptr())) + 28; address += 4)
                Pause::Write(address, 0);
            return 0;
        }
        if ((DeerMenu::Game(), game_menuPage.get()) == 5 && (DeerMenu::Game(), game_menuSelection.get()) == 3 && (!host || !host->Current()))
            Pause::Write((Game(), reinterpret_cast<uintptr_t>(game_menuSelection.get_ptr())), 2);
        return menuHook.unsafe_ccall<int>();
    }

    inline int __cdecl MenuDraw()
    {
        Depth depth(drawDepth);
        auto host = pauseHost;
        // The pause page in the game's sprites: its panel with our Plugins item, or our pages in a panel like it
        if (!stopping && host && host->Current() && PauseContext::PauseShown() && Sprites::Ready())
        {
            Depth pauseSprites(pauseSpriteDepth);
            Sprites::api.state(6, 0);
            Sprites::api.state(8, 0);
            bool painted = false;
            try { painted = host->Paint(); }
            catch (...) { Cancel("pause sprite failure"); }
            Sprites::api.state(6, 1);
            Sprites::api.state(8, 1);
            if (painted)
                return 0;
        }
        return menuDrawHook.unsafe_ccall<int>();
    }

    inline int __cdecl Draw()
    {
        Depth depth(drawDepth);
        const int result = drawHook.unsafe_ccall<int>();
        if (stopping) return result;
        try
        {
            if (!PauseContext::PauseShown())
                return result;
            PauseContext context;
            if (!context.Capture((DeerMenu::Game(), game_world.get()), (DeerMenu::Game(), game_camera.get())))
                return result;
            auto host = pauseHost;
            if (!host)
            {
                host = std::make_shared<Pause>();
                host->context = context;
                pauseHost = host;
            }
            if (!host->context.Alive())
            {
                host->Retire();
                return result;
            }
            if (!host->Prepare(module))
                Cancel("pause source/viewport contract refused");
        }
        catch (...) { Cancel("pause font/draw failure"); }
        return result;
    }

    inline void __cdecl Frame()
    {
        static std::once_flag once;
        std::call_once(once, []() { WFP::onGameInitEvent().executeAll(); });
        WFP::onGameProcessEvent().executeAll();
        // The droplets stay off the game's menus while one shows
        RainDroplets::MenuOpen((DeerMenu::Game(), game_menuOpen.get()) == 1);
        if (Outside() && (DeerMenu::Game(), game_worldState.get()) != 1)
        {
            try { ReleasePause(); } catch (...) { Cancel("pre-world retirement failure"); }
        }
        {
            Depth depth(frameDepth);
            frameHook.unsafe_ccall();
        }
        Drain();
    }

    inline int __cdecl App(int event, int* data)
    {
        if (event == 13)
        {
            stopping = true;
            Cancel("native engine retirement");
            for (const auto& item : launchers) item.second->Retire();
            for (const auto& item : popups) item.second->Retire();
            if (!Outside())
            {
                // Nested shutdown: forward it now and drop our font without
                // calling its destructor after the engine is gone.
                auto host = pauseHost;
                if (host) host->font = nullptr;
                pauseHost.reset();
                Log("nested shutdown: owned font quarantined; native teardown forwarded");
            }
            else try { ReleasePause(); } catch (...) { Log("pre-engine font retirement failure"); }
        }
        int result;
        {
            Depth depth(appDepth);
            result = appHook.unsafe_ccall<int>(event, data);
        }
        Drain();
        return result;
    }

    inline void WINAPI Exit(UINT code)
    {
        RWGTA::UpdateCheck::OnExit();
        ExitProcess(code);
    }

    inline void InitUpdates()
    {
        IATHook::Replace(GetModuleHandleW(nullptr), "KERNEL32.dll", std::forward_as_tuple("ExitProcess", Exit));
        RWGTA::UpdateCheck::Config config;
        config.pluginId = "DeerAvenger4.FusionFix";
        config.fileVersion = rsc_FileVersion;
        config.settings = &FixSettings();
        config.format = [](const std::string& latest, bool hub)
        {
            auto texts = FixSettings().GetTexts();
            texts.Select(Language());
            auto message = texts.Resolve(hub ? "@update.availablehub" : "@update.availablesite");
            for (const auto& [key, value] : { std::pair<std::string, std::string>{ "{name}", "Deer Avenger 4" }, { "{version}", latest } })
                if (const auto at = message.find(key); at != std::string::npos) message.replace(at, key.size(), value);
            return asi_settings::text::Widen(message);
        };
        config.playable = []()
        {
            auto host = pauseHost;
            return host && host->Private() && host->session->ready && host->notice.empty();
        };
        config.show = [](const std::wstring& message)
        {
            auto host = pauseHost;
            if (host && host->Private()) host->notice = asi_settings::text::Narrow(message);
        };
        RWGTA::UpdateCheck::Init(std::move(config));
    }

    inline void Init(HMODULE owner)
    {
        if (attempted) return;
        attempted = true;
        module = owner;
        HMODULE pinned = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN, reinterpret_cast<LPCWSTR>(&Init), &pinned);
        titleHook = safetyhook::create_inline(reinterpret_cast<void*>(Game().title), Title);
        keyHook = safetyhook::create_inline(reinterpret_cast<void*>(Game().key), Key);
        menuHook = safetyhook::create_inline(reinterpret_cast<void*>(Game().menu), Menu);
        Sprites::Init(Game().menuDraw);
        menuDrawHook = safetyhook::create_inline(reinterpret_cast<void*>(Game().menuDraw), MenuDraw);
        drawHook = safetyhook::create_inline(reinterpret_cast<void*>(Game().draw), Draw);
        frameHook = safetyhook::create_inline(reinterpret_cast<void*>(Game().frame), Frame);
        appHook = safetyhook::create_inline(reinterpret_cast<void*>(Game().app), App);
        InitUpdates();
    }
}
