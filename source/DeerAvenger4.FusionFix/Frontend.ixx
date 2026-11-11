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
    enum class Kind { Link, Row, Previous, Next, Back };

    struct Item
    {
        Kind kind = Kind::Back;
        std::string text;
        RWGTAMenu::Row row{};
        size_t target = 0;
        bool enabled = true;
    };

    struct View
    {
        // Keep two content lines plus Previous/Next/Back within five rows.
        static constexpr size_t ContentRows = 2;
        int page = -1;
        size_t physical = 0;
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
            if (page < 0)
            {
                title = model.Text("menu.plugins");
                const auto pages = model.Pages();
                for (size_t i = 0; i < pages.size(); ++i)
                    if (!append(Item{Kind::Link, {}, {}, i, true}, pages[i].title))
                        return false;
            }
            else
            {
                if (size_t(page) >= model.Pages().size())
                    return false;
                title = model.Pages()[size_t(page)].title;
                const auto visible = model.VisibleRows(size_t(page));
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
            const auto count = std::max(size_t(1), (content.size() + ContentRows - 1) / ContentRows);
            physical = std::min(physical, count - 1);
            items.clear();
            for (size_t i = physical * ContentRows; i < content.size() && i < (physical + 1) * ContentRows; ++i)
                items.push_back(std::move(content[i]));
            if (physical)
                items.push_back(Item{Kind::Previous, model.Text("menu.previous"), {}, 0, true});
            if (physical + 1 < count)
                items.push_back(Item{Kind::Next, model.Text("menu.next"), {}, 0, true});
            items.push_back(Item{Kind::Back, model.Text("menu.back"), {}, 0, true});
            for (auto& item : items)
            {
                item.text = PlainText(item.text);
                if (!TextFits(item.text, width, Native::GlyphHeight)) return false;
            }
            return current() && !items.empty() && items.size() <= 5;
        }
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
            int page = -1;
            size_t physical = 0;
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
            view = {};
            const auto expected = generation;
            Busy discovery(busy);
            model.OpenPlugins(module, language, 4096);
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
            pending.push_back(Pending{expected, serial, view.page, view.physical, selected, direction, back, item});
            return true;
        }

        bool Move(int direction)
        {
            if (!Current(generation) || !ready || busy || view.items.empty())
                return false;
            if (direction < 0 && selected)
                --selected;
            else if (direction > 0 && selected + 1 < view.items.size())
                ++selected;
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
                if (view.page < 0)
                {
                    Close();
                    return true;
                }
                model.PageShown(size_t(view.page));
                if (!Current(expected))
                    return false;
                view.page = -1;
                view.physical = 0;
                return Populate(false);
            }
            if (item.kind != Kind::Row && command.direction)
                return true;
            if (item.kind == Kind::Link)
            {
                if (item.target >= model.Pages().size())
                    return false;
                view.page = int(item.target);
                view.physical = 0;
                model.PageShown(item.target);
                return Current(expected) && Populate(false);
            }
            if (item.kind == Kind::Previous || item.kind == Kind::Next)
            {
                if (item.kind == Kind::Previous && view.physical)
                    --view.physical;
                else if (item.kind == Kind::Next)
                    ++view.physical;
                return Populate(false);
            }
            if (item.kind == Kind::Row && item.enabled)
            {
                Busy changing(busy);
                const auto expectedSerial = serial;
                const auto expectedSelected = selected;
                const auto expectedPage = view.page;
                const auto expectedPhysical = view.physical;
                const bool enabled = model.Enabled(item.row, pause);
                if (!Current(expected) || serial != expectedSerial || selected != expectedSelected ||
                    view.page != expectedPage || view.physical != expectedPhysical)
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
                    command.page != view.page || command.physical != view.physical ||
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
        HWND heading = nullptr;
        HWND help = nullptr;
        uint64_t helpSerial = 0;
        size_t helpSelected = SIZE_MAX;
        HWND list = nullptr;
        HFONT font = nullptr; // Stock GDI object; never deleted by this host.
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
            const int headingHeight = int(snapshot.headings.size()) * rowHeight;
            const int listHeight = int(snapshot.items.size()) * rowHeight + 8;
            const int top = 12 + headingHeight + 8;
            if (top + listHeight + 12 > client.bottom || width < 80)
                return false;
            Session::Busy building(publishing);
            const auto next = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", L"",
                WS_CHILD | WS_TABSTOP | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT,
                12, top, client.right - 24, listHeight, window, reinterpret_cast<HMENU>(0x7100), module, nullptr);
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
                    title += L"\r\n";
                title.append(line.begin(), line.end());
            }
            MoveWindow(heading, 12, 12, client.right - 24, headingHeight, TRUE);
            SetWindowTextW(heading, title.c_str());
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
            RECT client{}, bounds{};
            if (!GetClientRect(window, &client) || !GetWindowRect(list, &bounds))
                return false;
            MapWindowPoints(nullptr, window, reinterpret_cast<POINT*>(&bounds), 2);
            const int top = bounds.bottom + 12;
            if (client.right < 104 || client.bottom - top < 24)
                return false;
            const std::wstring wide(text.begin(), text.end());
            MoveWindow(help, 12, top, client.right - 24, client.bottom - top - 12, TRUE);
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
                popup->font = reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
                popup->heading = CreateWindowExW(0, L"STATIC", L"Plugins", WS_CHILD | WS_VISIBLE | SS_NOPREFIX,
                    12, 12, 300, 24, window, nullptr, popup->module, nullptr);
                popup->help = CreateWindowExW(0, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_READONLY,
                    12, 160, 300, 80, window, reinterpret_cast<HMENU>(0x7102), popup->module, nullptr);
                if (!popup->font || !popup->heading || !popup->help || !popup->source.Alive())
                {
                    popup->Finish();
                    return TRUE;
                }
                SendMessageW(popup->heading, WM_SETFONT, reinterpret_cast<WPARAM>(popup->font), FALSE);
                SendMessageW(popup->help, WM_SETFONT, reinterpret_cast<WPARAM>(popup->font), FALSE);
                RECT client{};
                GetClientRect(window, &client);
                popup->session = std::make_shared<Session>();
                popup->session->module = popup->module;
                popup->session->pause = false;
                popup->session->width = client.right - 48;
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
            definition->style = WS_POPUP | WS_CAPTION | WS_SYSMENU | DS_MODALFRAME;
            definition->dwExtendedStyle = 0;
            definition->cdit = 0;
            definition->x = 0; definition->y = 0;
            definition->cx = 330; definition->cy = 230;
            bytes.push_back(0); // no menu
            bytes.push_back(0); // native dialog class
            for (const wchar_t c : std::wstring(L"Plugins")) bytes.push_back(WORD(c));
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
        std::string notice;
        uint64_t paintedGeneration = 0, paintedSerial = 0;

        static void Write(uintptr_t address, int value)
        {
            *reinterpret_cast<int*>(address) = value;
        }

        bool Current() const
        {
            return !retired && font && context.Current();
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
            if (!font)
            {
                // RwRGBA bytes (r, g, b, a): opaque white text on a dark, translucent background.
                // The constructor copies these values into its own raster.
                const uint32_t foreground = 0xffffffff;
                const uint32_t background = 0xb0000000;
                font = Native::CreateTypeface(&foreground, &background);
            }
            if (!Current()) return false;
            if (!session)
            {
                session = std::make_shared<Session>();
                session->module = module;
                session->pause = true;
                session->width = context.width - 48;
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
                Write((Game(), reinterpret_cast<uintptr_t>(game_menuSelection.get_ptr())), std::clamp(nativeSelection + (key == 147 ? -1 : 1), 0, 3));
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
        }

        bool Draw()
        {
            auto lease = shared_from_this();
            if (!Current() || !context.Rendering()) return false;
            if (!Private())
            {
                const std::string caption = (Game(), game_menuSelection.get()) == 3 ? "> Plugins" : "  Plugins";
                const int y = context.height - Native::GlyphHeight - 16;
                const int x = (context.width - int(caption.size()) * Native::GlyphWidth) / 2;
                const bool result = x >= 16 && y >= 16 && Native::DrawText(font, caption.c_str(), x, y) && Current();
                entryPainted = result;
                return result;
            }
            if (!session->ready || session->view.items.size() > 5 || session->view.headings.size() > 4)
                return false;
            const auto serial = session->serial;
            const auto generation = session->generation;
            View snapshot = session->view;
            const size_t selected = session->selected;
            const auto current = [&] { return Private() && context.Rendering() &&
                session->Current(generation) && session->serial == serial; };
            std::string help;
            std::vector<std::string> helpLines;
            if (notice.empty())
            {
                if (!session->ReadHelp(help)) return false;
            }
            else
            {
                help = notice;
                snapshot.headings.clear();
                snapshot.items.clear();
            }
            if (!current() ||
                (!help.empty() && !SplitText(help, context.width - 32 - 2 * Native::GlyphWidth, helpLines)))
                return false;
            const auto lines = snapshot.headings.size() + snapshot.items.size() + helpLines.size();
            if (32 + 20 + int(lines) * (Native::GlyphHeight + 4) > context.height)
                return false;
            int y = 16;
            const auto line = [&](const std::string& text, bool focused)
            {
                const std::string display = focused ? "> " + text : "  " + text;
                if (!current() || !TextFits(display, context.width - 32, Native::GlyphHeight) ||
                    y + Native::GlyphHeight > context.height - 16) return false;
                const bool result = Native::DrawText(font, display.c_str(), 16, y);
                y += Native::GlyphHeight + 4;
                return result && current();
            };
            for (const auto& heading : snapshot.headings)
                if (!line(heading, false)) return false;
            y += 8;
            for (size_t i = 0; i < snapshot.items.size(); ++i)
                if (!line(snapshot.items[i].text, i == selected)) return false;
            y += 12;
            for (const auto& helpLine : helpLines)
                if (!line(helpLine, false)) return false;
            if (!current()) return false;
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
    inline bool installed = false, attempted = false, draining = false, stopping = false;
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
        if (!installed || !Outside() || draining) return;
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
        if (installed && host)
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
        if (installed && message == WM_INITDIALOG)
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
            if (installed && !stopping && event == 18 && host && data)
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
        if (installed && !stopping && host && host->Private())
        {
            // Clear the pause menu's command slots and return 0, as the game does
            // when no item is chosen; our commands are handled in Key.
            for (uintptr_t address = (Game(), reinterpret_cast<uintptr_t>(game_menuCommands.get_ptr())); address <= (Game(), reinterpret_cast<uintptr_t>(game_menuCommands.get_ptr())) + 28; address += 4)
                Pause::Write(address, 0);
            return 0;
        }
        if (installed && (DeerMenu::Game(), game_menuPage.get()) == 5 && (DeerMenu::Game(), game_menuSelection.get()) == 3 && (!host || !host->Current()))
            Pause::Write((Game(), reinterpret_cast<uintptr_t>(game_menuSelection.get_ptr())), 2);
        return menuHook.unsafe_ccall<int>();
    }

    inline int __cdecl MenuDraw()
    {
        Depth depth(drawDepth);
        auto host = pauseHost;
        // This function draws only menu sprites; hide the pause pane while our page replaces it.
        if (installed && !stopping && host && host->Private() && host->session->ready)
            return 0;
        if (PauseContext::PauseShown())
        {
            Depth pauseSprites(pauseSpriteDepth);
            return menuDrawHook.unsafe_ccall<int>();
        }
        return menuDrawHook.unsafe_ccall<int>();
    }

    inline int __cdecl Draw()
    {
        Depth depth(drawDepth);
        const int result = drawHook.unsafe_ccall<int>();
        if (!installed || stopping) return result;
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
            if (!host->Prepare(module) || !host->Draw())
                Cancel("pause source/font/viewport/text contract refused");
        }
        catch (...) { Cancel("pause font/draw failure"); }
        return result;
    }

    inline void __cdecl Frame()
    {
        static std::once_flag once;
        std::call_once(once, []() { WFP::onGameInitEvent().executeAll(); });
        WFP::onGameProcessEvent().executeAll();
        if (installed && Outside() && (DeerMenu::Game(), game_worldState.get()) != 1)
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
        if (installed && event == 13)
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

    inline void Rollback() noexcept
    {
        installed = false;
        titleHook.reset(); keyHook.reset(); menuHook.reset(); menuDrawHook.reset();
        drawHook.reset(); frameHook.reset(); appHook.reset();
    }

    inline void WINAPI Exit(UINT code)
    {
        RWGTA::UpdateCheck::OnExit();
        ExitProcess(code);
    }

    inline void InitUpdates()
    {
        FixSettings().Availability("MAIN.CheckForUpdates", [](std::string_view value)
        {
            return value == "0" || installed;
        });
        if (!installed) return;
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

    inline void Init(HMODULE owner) noexcept
    {
        if (attempted) return;
        attempted = true;
        module = owner;
        try
        {
            if (!owner) return;
            HMODULE pinned = nullptr;
            if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
                reinterpret_cast<LPCWSTR>(&Init), &pinned) || pinned != owner)
            { Log("process-lifetime pin failed; no exposure"); return; }
            titleHook = safetyhook::create_inline(reinterpret_cast<void*>(Game().title), Title, SafetyHookInline::StartDisabled);
            keyHook = safetyhook::create_inline(reinterpret_cast<void*>(Game().key), Key, SafetyHookInline::StartDisabled);
            menuHook = safetyhook::create_inline(reinterpret_cast<void*>(Game().menu), Menu, SafetyHookInline::StartDisabled);
            menuDrawHook = safetyhook::create_inline(reinterpret_cast<void*>(Game().menuDraw), MenuDraw, SafetyHookInline::StartDisabled);
            drawHook = safetyhook::create_inline(reinterpret_cast<void*>(Game().draw), Draw, SafetyHookInline::StartDisabled);
            frameHook = safetyhook::create_inline(reinterpret_cast<void*>(Game().frame), Frame, SafetyHookInline::StartDisabled);
            appHook = safetyhook::create_inline(reinterpret_cast<void*>(Game().app), App, SafetyHookInline::StartDisabled);
            for (auto hook : { &titleHook, &keyHook, &menuHook, &menuDrawHook, &drawHook, &frameHook, &appHook })
                (void)hook->enable();
            installed = true;
            InitUpdates();
        }
        catch (...) { Rollback(); Log("bridge allocation failed; originals restored"); }
    }
}
