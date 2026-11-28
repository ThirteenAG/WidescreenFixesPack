module;

#include <stdafx.h>
#include <stdexcept>
#include "FixSettings.hpp"
#include "Shared/RWGTA/FixMenu.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"
#include "Shared/MenuList.hpp"
#include <charconv>
#include <set>

export module NativeMenu;

import ComVars;

// The movie owns its Page/ListBox/Label objects. The bridge only responds to
// that movie's FSCommands, using the native Invoke/string protocol.
export namespace SCCMenu
{
    inline SafetyHookInline commandHook, retireHook;
    inline HMODULE module = nullptr;
    inline bool ready = false, noticeSource = false;
    inline std::atomic<bool> helpRequested{ false };
    inline const char* (__cdecl* nativeLanguage)() = nullptr;
    inline void* (__cdecl* currentMovie)() = nullptr;
    inline void* (__cdecl* currentManager)() = nullptr;
    inline void* callbackManager = nullptr;
    inline void* callbackMovie = nullptr;
    inline void (__fastcall* stringCtor)(void*, void*, const char*) = nullptr;
    inline void (__fastcall* stringDtor)(void*, void*) = nullptr;
    inline void (__fastcall* wideDtor)(void*, void*) = nullptr;
    inline uintptr_t invokeAddress = 0;
    inline std::set<std::string> logged;
    // The movie shows seven rows
    inline constexpr size_t Slots = 7;
    inline MenuList::WheelNotches wheel;
    inline std::atomic<int> wheelDelta{ 0 }, pendingKey{ 0 };
    inline const std::string Root = "_root.FlashMenu.callbackObject.privateMenu.";

    inline void Log(const std::string& message)
    {
        if (!logged.insert(message).second)
        {
            return;
        }
        OutputDebugStringA(("SCC native menu: " + message + "\n").c_str());
    }
    inline std::string Language()
    {
        auto chosen = FixSettings().String("MAIN.TextLanguage");
        if (!chosen.empty() && chosen != "auto") return chosen;
        auto language = nativeLanguage ? nativeLanguage() : nullptr;
        std::string tag = language ? language : "int";
        std::transform(tag.begin(), tag.end(), tag.begin(), [](unsigned char c) { return char(std::tolower(c)); });
        static const std::map<std::string, std::string> tags = {
            {"int","en"},{"eng","en"},{"fra","fr"},{"fre","fr"},{"deu","de"},{"ger","de"},
            {"ita","it"},{"esp","es"},{"rus","ru"},{"pol","pl"},{"dan","da"},{"nl","nl"},
            {"nor","no"},{"swe","sv"},{"jpn","ja"},{"kor","ko"},{"chn","zh-CN"},{"cht","zh-TW"}
        };
        auto found = tags.find(tag); return found == tags.end() ? tag : found->second;
    }
    inline std::wstring Wide(const std::string& text)
    {
        int count = MultiByteToWideChar(CP_UTF8, 0, text.data(), int(text.size()), nullptr, 0);
        std::wstring value(size_t(std::max(count, 0)), L'\0');
        if (count) MultiByteToWideChar(CP_UTF8, 0, text.data(), int(text.size()), value.data(), count);
        auto tag = Language();
        std::wstring locale(tag.begin(), tag.end());
        int upperSize = LCMapStringEx(locale.c_str(), LCMAP_UPPERCASE, value.data(), int(value.size()), nullptr, 0, nullptr, nullptr, 0);
        if (upperSize > 0)
        {
            std::wstring upper(upperSize, L'\0');
            if (LCMapStringEx(locale.c_str(), LCMAP_UPPERCASE, value.data(), int(value.size()), upper.data(), upperSize, nullptr, nullptr, 0)) value = std::move(upper);
        }
        if (value.size() > 4096) { value.resize(4096); if (value.back() >= 0xD800 && value.back() <= 0xDBFF) value.pop_back(); }
        return value;
    }
    inline std::string Utf8(const std::wstring& text)
    {
        int count = WideCharToMultiByte(CP_UTF8, 0, text.data(), int(text.size()), nullptr, 0, nullptr, nullptr);
        std::string value(size_t(std::max(count, 0)), '\0');
        if (count) WideCharToMultiByte(CP_UTF8, 0, text.data(), int(text.size()), value.data(), count, nullptr, nullptr);
        return value;
    }
    inline bool MovieContract(void* movie)
    {
        return movie && (*reinterpret_cast<uintptr_t**>(movie))[0x38 / 4] == invokeAddress;
    }
    struct CallbackScope
    {
        void* previousManager = callbackManager;
        void* previousMovie = callbackMovie;
        CallbackScope(void* manager, void* movie)
        {
            callbackManager = manager;
            callbackMovie = movie;
        }
        ~CallbackScope()
        {
            callbackManager = previousManager;
            callbackMovie = previousMovie;
        }
    };
    inline bool MovieValid(void* movie)
    {
        // Projected title movies have their own callback source. The native
        // Localize handler invokes that source, not the manager's primary movie.
        // Admit it only during its active manager's native callback.
        return ready && callbackManager && currentManager() == callbackManager && movie == callbackMovie && MovieContract(movie);
    }
    template<class... Args> bool Invoke(void* movie, const char* method, const char* format, Args... args)
    {
        if (!MovieValid(movie)) return false;
        uint8_t path[20] = {}, result[64] = {};
        std::string name = "_root.FlashMenu.callbackObject."; name += method;
        stringCtor(path, nullptr, name.c_str());
        reinterpret_cast<void (__cdecl*)(void*, void*, void*, const char*, Args...)>(invokeAddress)(movie, result, path, format, args...);
        wideDtor(result + 0x20, nullptr); stringDtor(result + 0x0C, nullptr); stringDtor(path, nullptr);
        return result[0] == 4 && (result[4] & 1) && MovieValid(movie);
    }
    // FlashMovie virtuals: +68h reads a variable as a number, +78h sets a boolean, +7Ch a number, +80h a string
    inline double GetNumber(void* movie, const std::string& name)
    {
        if (!MovieValid(movie)) return NAN;
        uint8_t path[20] = {};
        stringCtor(path, nullptr, name.c_str());
        const auto value = reinterpret_cast<double(__thiscall*)(void*, void*)>((*reinterpret_cast<uintptr_t**>(movie))[0x68 / 4])(movie, path);
        stringDtor(path, nullptr);
        return value;
    }
    inline void SetBool(void* movie, const std::string& name, bool value)
    {
        if (!MovieValid(movie)) return;
        uint8_t path[20] = {};
        stringCtor(path, nullptr, name.c_str());
        reinterpret_cast<void(__thiscall*)(void*, void*, int)>((*reinterpret_cast<uintptr_t**>(movie))[0x78 / 4])(movie, path, value ? 1 : 0);
        stringDtor(path, nullptr);
    }
    inline void SetNumber(void* movie, const std::string& name, float value)
    {
        if (!MovieValid(movie)) return;
        uint8_t path[20] = {};
        stringCtor(path, nullptr, name.c_str());
        reinterpret_cast<void(__thiscall*)(void*, void*, float)>((*reinterpret_cast<uintptr_t**>(movie))[0x7C / 4])(movie, path, value);
        stringDtor(path, nullptr);
    }
    inline void SetString(void* movie, const std::string& name, const std::string& value)
    {
        if (!MovieValid(movie)) return;
        uint8_t path[20] = {}, text[20] = {};
        stringCtor(path, nullptr, name.c_str());
        stringCtor(text, nullptr, value.c_str());
        reinterpret_cast<void(__thiscall*)(void*, void*, void*)>((*reinterpret_cast<uintptr_t**>(movie))[0x80 / 4])(movie, path, text);
        stringDtor(text, nullptr); stringDtor(path, nullptr);
    }

    struct Context
    {
        RWGTAMenu::Model model;
        void* movie = nullptr;
        void* manager = nullptr;
        bool pause = false;
        bool opening = false;
        bool committed = false;
        bool details = false, notice = false;
        bool marks = false;
        size_t helpSlot = 0;
        size_t page = RWGTAMenu::Model::RootPage;
        size_t lastSlot = 0;
        ULONGLONG populated = 0;
        // When the page was shown: then the movie hovers the row under the mouse, as on its own pages
        ULONGLONG entered = GetTickCount64();
        // The rows in the window, as the movie shows them
        std::vector<RWGTAMenu::Row> rows;
    };
    inline std::shared_ptr<Context> context;
    inline bool Current(const std::shared_ptr<Context>& ctx)
    { return context == ctx && ctx && ctx->manager == callbackManager && ctx->model.IsOpen() && MovieValid(ctx->movie); }
    inline bool RequestHelp(bool pressed)
    {
        if (!context || !context->committed || !context->model.IsOpen())
            return false;
        if (pressed) helpRequested = true;
        return true;
    }
    inline std::atomic<ULONGLONG> lastMouse{ 0 }, lastKey{ 0 };
    inline void Input(UINT message)
    {
        if (message == WM_KEYDOWN) { lastKey = GetTickCount64(); return; }
        static POINT last{};
        POINT now{};
        if (GetCursorPos(&now) && (now.x != last.x || now.y != last.y)) { last = now; lastMouse = GetTickCount64(); }
    }
    inline bool TakeWheel(int delta)
    {
        if (!context || !context->committed || context->details || !context->model.IsOpen()) return false;
        wheelDelta += delta;
        return true;
    }
    inline bool TakeKey(int key)
    {
        if (!context || !context->committed || context->details || !context->model.IsOpen()) return false;
        pendingKey = key;
        return true;
    }
    inline void Close()
    {
        helpRequested = false;
        auto old = std::exchange(context, {});
        // Discovery can call foreign descriptor/getter APIs. Keep its Model
        // alive until Open unwinds if that callback retires the source movie.
        if (old && !old->opening) old->model.Close();
    }
    // Scroll marks: two copies of the movie's own row text, beside the first and the last row, showing the font's
    // plainest arrows while more rows are above or below (it has no triangles)
    inline void Marks(const std::shared_ptr<Context>& ctx)
    {
        if (!Current(ctx)) return;
        if (ctx->details)
        {
            // The help page has no list window
            if (ctx->marks)
                for (auto clip : { "FixMarkUp.", "FixMarkDown." })
                {
                    SetString(ctx->movie, Root + clip + "MenuItem.txtField.text", "");
                    SetBool(ctx->movie, Root + clip + "_visible", false);
                }
            return;
        }
        const auto first = GetNumber(ctx->movie, Root + "Menu0._y"), second = GetNumber(ctx->movie, Root + "Menu1._y");
        const auto width = GetNumber(ctx->movie, Root + "Menu0.MenuItem.txtField._width");
        if (!std::isfinite(first) || !std::isfinite(second) || !std::isfinite(width) || second <= first || width < 64) return;
        if (!ctx->marks)
        {
            Invoke(ctx->movie, "privateMenu.Menu0.duplicateMovieClip", "Wi", L"FixMarkUp", 9001);
            Invoke(ctx->movie, "privateMenu.Menu0.duplicateMovieClip", "Wi", L"FixMarkDown", 9002);
            if (!Current(ctx)) return;
            ctx->marks = true;
        }
        auto& window = ctx->model.Window(ctx->page);
        const auto spacing = second - first;
        const std::pair<const char*, bool> marks[] = { { "FixMarkUp.", window.Above() && !ctx->details },
            { "FixMarkDown.", window.Below() && !ctx->details } };
        for (size_t i = 0; i < 2; ++i)
        {
            const std::string clip = Root + marks[i].first;
            SetNumber(ctx->movie, clip + "_y", float(first + spacing * (i ? double(window.Shown() - 1) : 0.0)));
            SetNumber(ctx->movie, clip + "_x", float(GetNumber(ctx->movie, Root + "Menu0._x") + width + spacing * 0.25));
            SetBool(ctx->movie, clip + "Background._visible", false);
            SetBool(ctx->movie, clip + "hitTestDisable", true);
            SetString(ctx->movie, clip + "MenuItem.txtField.text", marks[i].second ? (i ? "v" : "^") : "");
            SetBool(ctx->movie, clip + "_visible", marks[i].second);
        }
    }
    // The help under the list: all of it, wrapped, when it fits the space below the rows; else its whole sentences
    // that do (F1 / Y shows all of it)
    inline void Describe(const std::shared_ptr<Context>& ctx, size_t slot)
    {
        if (!Current(ctx) || ctx->details) return;
        std::string description;
        if (slot < ctx->rows.size() && !RWGTAMenu::Model::Opens(ctx->rows[slot])) description = ctx->model.Help(ctx->rows[slot]);
        if (!Current(ctx)) return;
        if (ctx->model.RestartRequired())
        {
            auto warning = ctx->model.Text("menu.restartwarning");
            if (!description.empty()) warning += " - " + description;
            description = std::move(warning);
        }
        if (!Invoke(ctx->movie, "describeFixMenu", "W", L"") || !Current(ctx)) return;
        const std::string field = Root + "FixDescription.MenuItem.txtField";
        const auto spacing = GetNumber(ctx->movie, Root + "Menu1._y") - GetNumber(ctx->movie, Root + "Menu0._y");
        if (!std::isfinite(spacing) || spacing <= 0) return;
        SetBool(ctx->movie, field + ".multiline", true);
        SetBool(ctx->movie, field + ".wordWrap", true);
        SetNumber(ctx->movie, field + "._height", float(spacing * 4));
        const auto text = Utf8(Wide(description));
        auto shown = text;
        for (size_t cut = text.size();;)
        {
            SetString(ctx->movie, field + ".text", shown);
            SetNumber(ctx->movie, field + ".scroll", 1);
            const auto overflow = GetNumber(ctx->movie, field + ".maxscroll");
            if (!std::isfinite(overflow) || overflow <= 1 || cut == 0) break;
            cut = text.find_last_of(".!?", cut - 1);
            if (cut == std::string::npos) { shown.clear(); break; }
            if (cut + 1 < text.size() && text[cut + 1] != ' ') continue;
            shown = text.substr(0, cut + 1);
        }
        SetBool(ctx->movie, Root + "FixDescription._visible", !shown.empty());
    }
    inline void Populate(const std::shared_ptr<Context>& ctx)
    {
        if (!Current(ctx)) return;
        ctx->committed = false;
        struct RowText { std::string label, value; bool enabled; };
        std::vector<RowText> rows;
        if (ctx->page >= ctx->model.Pages().size()) ctx->page = RWGTAMenu::Model::RootPage;
        auto title = ctx->page == RWGTAMenu::Model::RootPage ? ctx->model.Text("menu.plugins") : ctx->model.Pages()[ctx->page].title;
        // The game's own lists have no Back row: B / Esc go back a level
        std::vector<RWGTAMenu::Row> shown;
        for (auto row : ctx->model.Shown(ctx->page, false))
        {
            if (!row) continue;
            shown.push_back(*row);
            if (RWGTAMenu::Model::Opens(*row))
            {
                auto label = ctx->model.Label(*row);
                if (!Current(ctx)) return;
                rows.push_back({ std::move(label), {}, true });
                continue;
            }
            auto label = ctx->model.Label(*row);
            auto enabled = ctx->model.Enabled(*row, ctx->pause);
            if (!Current(ctx)) return;
            auto value = enabled ? ctx->model.Value(*row) : ctx->model.Text("menu.unavailable");
            if (!Current(ctx)) return;
            rows.push_back({ std::move(label), std::move(value), enabled });
        }
        if (rows.empty() || rows.size() > Slots) { Log("private row bound rejected"); return; }
        if (!Invoke(ctx->movie, "resetFixMenu", "W", Wide(title).c_str()) || !Current(ctx)) return;
        ctx->rows = std::move(shown);
        for (const auto& row : rows)
        {
            if (!Invoke(ctx->movie, "appendFixMenu", "WWB", Wide(row.label).c_str(), Wide(row.value).c_str(), int(row.enabled)) || !Current(ctx)) return;
        }
        const auto slot = ctx->model.Window(ctx->page).Slot();
        ctx->lastSlot = slot;
        if (!Invoke(ctx->movie, "finishFixMenu", "i", int(slot)) || !Current(ctx))
        {
            Log("private page native commit rejected");
            if (context == ctx) { Close(); Invoke(ctx->movie, "leaveFixMenu", ""); }
            return;
        }
        // the list shows all seven row clips, their background bars stayed below a shorter list
        // The copied Settings list brings more row clips than the seven rows; the unused ones stay hidden
        for (size_t i = 0; i < Slots + 8; ++i)
        {
            const auto clip = Root + "Menu" + std::to_string(i);
            SetBool(ctx->movie, clip + "._visible", i < rows.size());
            SetBool(ctx->movie, clip + ".Background._visible", i < rows.size());
        }
        Marks(ctx);
        RWGTAMenu::Model::Changed() = false;
        ctx->committed = true;
        ctx->populated = GetTickCount64();
        Describe(ctx, slot);
    }
    inline int Index(const char* args)
    {
        if (!args) return -1;
        int index = -1; auto end = args + strlen(args);
        auto result = std::from_chars(args, end, index);
        return result.ec == std::errc{} ? index : -1;
    }
    inline void Details(const std::shared_ptr<Context>& ctx, int index)
    {
        if (!Current(ctx) || index < 0 || size_t(index) >= ctx->rows.size()) return;
        const auto row = ctx->rows[size_t(index)];
        if (RWGTAMenu::Model::Opens(row)) return;
        auto help = ctx->model.Help(row);
        if (help.empty() || !Current(ctx)) return;
        // The movie's help page ends by sending a command of the frame, which moves the callback object off this page:
        // hide the scroll marks first
        const bool details = std::exchange(ctx->details, true);
        Marks(ctx);
        ctx->details = details;
        if (Invoke(ctx->movie, "showFixDetails", "WW", Wide(ctx->model.Label(row)).c_str(), Wide(help).c_str()) && Current(ctx))
        {
            ctx->details = true;
            ctx->helpSlot = size_t(index);
        }
    }
    // The movie's list moves within its rows and wraps there; a step off its last row (or its first) moves the window
    inline bool UnderMouse(const std::shared_ptr<Context>& ctx, size_t slot)
    {
        const auto mouse = GetNumber(ctx->movie, Root + "_ymouse"), top = GetNumber(ctx->movie, Root + "Menu0._y");
        const auto spacing = GetNumber(ctx->movie, Root + "Menu1._y") - top;
        const auto under = (mouse - top) / spacing;
        return std::isfinite(under) && spacing > 0 && under > -0.5 &&
            (size_t(std::max(0.0, std::floor(under))) == slot || size_t(std::max(0.0, std::round(under))) == slot);
    }
    // The movie moves the selection within its rows and wraps there. A key or pad step off its last row (or its first)
    // moves the window instead; the mouse just points at a row
    inline void Follow(const std::shared_ptr<Context>& ctx, size_t slot)
    {
        auto& window = ctx->model.Window(ctx->page);
        const size_t shown = ctx->rows.size();
        const size_t first = window.First();
        const auto now = GetTickCount64();
        const bool key = now - lastKey < 400, moving = now - lastMouse < 250;
        const bool pointed = !key && (moving || UnderMouse(ctx, slot));
        if (pointed && !moving && now - ctx->entered > 1500 && now - ctx->populated < 1000 && slot != window.Slot())
        {
            // The movie hovers the row under a resting mouse again when it rebuilds the list: keys don't fight that
            // mouse, the selection stays
            Invoke(ctx->movie, "menu.select", "i", int(window.Slot()));
            ctx->lastSlot = window.Slot();
            return;
        }
        if (!pointed && shown >= 3 && window.Count() > shown && ctx->lastSlot == shown - 1 && slot == 0) window.Move(1);
        else if (!pointed && shown >= 3 && window.Count() > shown && ctx->lastSlot == 0 && slot == shown - 1) window.Move(-1);
        else window.Point(slot);
        ctx->lastSlot = slot;
        if (window.First() != first || window.Slot() != slot) Populate(ctx);
        else Describe(ctx, slot);
    }
    inline void Navigate(const std::shared_ptr<Context>& ctx, int key, int notches)
    {
        auto& window = ctx->model.Window(ctx->page);
        window.Point(ctx->lastSlot < ctx->rows.size() ? ctx->lastSlot : 0);
        bool changed = false;
        for (; notches; notches += notches > 0 ? -1 : 1) changed |= window.Scroll(notches > 0 ? -1 : 1);
        if (key == VK_PRIOR || key == VK_NEXT) changed |= window.Page(key == VK_NEXT ? 1 : -1);
        if (key == VK_HOME || key == VK_END) changed |= window.Edge(key == VK_END ? 1 : -1);
        if (changed) Populate(ctx);
    }
    inline int __fastcall Command(void* manager, void* edx, const char* command, const char* args, void* movie)
    {
        if (!ready || !command || !std::string_view(command).starts_with("FixMenu"))
            return commandHook.fastcall<int>(manager, edx, command, args, movie);
        if (!manager || manager != currentManager() || !MovieContract(movie))
        {
            Log(!movie ? "native movie command has no source movie" : manager != currentManager() ?
                "native movie command owner is not active" : "native movie Invoke contract rejected");
            return 1;
        }
        CallbackScope source(manager, movie);
        std::string_view cmd(command);
        if (cmd == "FixMenuEntry")
        {
            auto& texts = FixSettings().GetTexts(); texts.Select(Language());
            if (!Invoke(movie, "fixMenuEntry", "W", Wide(texts.Resolve("@menu.plugins")).c_str()))
            {
                Log("native Settings category movie/geometry contract rejected");
            }
            return 1;
        }
        if (cmd == "FixMenuOpen")
        {
            Close();
            auto ctx = std::make_shared<Context>(); ctx->movie = movie; ctx->manager = manager;
            ctx->pause = args && std::string_view(args) == "pause"; context = ctx;
            ctx->opening = true;
            ctx->model.OpenMenu(module, Language(), Slots);
            ctx->opening = false;
            if (context != ctx) { ctx->model.Close(); return 1; }
            wheel.Clear(); wheelDelta = 0; pendingKey = 0;
            if (Current(ctx))
            {
                ctx->model.Window(ctx->page).Reset();
                Populate(ctx);
            }
            return 1;
        }
        if (cmd == "FixMenuNotice")
        {
            if (!context && ready)
            {
                noticeSource = true;
                RWGTA::UpdateCheck::Tick();
                noticeSource = false;
            }
            return 1;
        }
        auto ctx = context;
        if (!Current(ctx) || ctx->movie != movie || ctx->manager != manager) return 1;
        if (cmd == "FixMenuClose") { Close(); return 1; }
        if (cmd == "FixMenuHelpBack")
        {
            if (ctx->notice) { Close(); Invoke(movie, "leaveFixMenu", ""); }
            else { ctx->details = false; Populate(ctx); }
            return 1;
        }
        if (cmd == "FixMenuHelp") { Details(ctx, Index(args)); return 1; }
        if (cmd == "FixMenuRefresh")
        {
            // F1 opens the help of the row and closes it again
            if (helpRequested.exchange(false))
            {
                if (!ctx->details) Details(ctx, Index(args));
                else if (!ctx->notice) { ctx->details = false; Populate(ctx); }
                return 1;
            }
            if (ctx->details) return 1;
            const int notches = wheel.Take(wheelDelta.exchange(0));
            const int key = pendingKey.exchange(0);
            if (notches || key) { Navigate(ctx, key, notches); return 1; }
            if (RWGTAMenu::Model::Changed()) Populate(ctx);
            return 1;
        }
        if (cmd == "FixMenuBack")
        {
            const auto parent = ctx->model.Back(ctx->page);
            if (parent == SIZE_MAX) { Close(); Invoke(movie, "leaveFixMenu", ""); }
            else { ctx->page = parent; ctx->entered = GetTickCount64(); Populate(ctx); }
            return 1;
        }
        auto index = Index(args);
        if (index < 0 || size_t(index) >= ctx->rows.size()) return 1;
        if (cmd == "FixMenuDescription") { Follow(ctx, size_t(index)); return 1; }
        if (cmd != "FixMenuAccept" && cmd != "FixMenuLeft" && cmd != "FixMenuRight") return 1;
        const auto row = ctx->rows[size_t(index)];
        ctx->model.Window(ctx->page).Point(size_t(index));
        if (RWGTAMenu::Model::Opens(row))
        {
            if (cmd != "FixMenuAccept") return 1;
            const auto page = ctx->model.Enter(row);
            if (page == SIZE_MAX) return 1;
            ctx->page = page;
            ctx->entered = GetTickCount64();
            Populate(ctx);
            return 1;
        }
        int direction = cmd == "FixMenuLeft" ? -1 : cmd == "FixMenuRight" ? 1 : 0;
        ctx->model.Change(row, direction, ctx->pause);
        if (Current(ctx)) Populate(ctx);
        return 1;
    }
    inline int __fastcall Retire(void* manager, void* edx)
    {
        if (context && context->manager == manager) Close();
        return retireHook.fastcall<int>(manager, edx);
    }
    inline void WINAPI ExitNotice(UINT code)
    {
        RWGTA::UpdateCheck::OnExit();
        ExitProcess(code);
    }
    inline uintptr_t Find(const char* pattern)
    {
        return reinterpret_cast<uintptr_t>(hook::pattern(pattern).get_first());
    }
    inline void Init()
    {
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&Init), &module);
        auto command = Find("55 8D 6C 24 94 81 EC C0 00 00 00 53 56 57 89 4D 68 33 DB 33 FF E8 ? ? ? ? 8B F0");
        auto retire = Find("56 8B F1 8B 4E 4C 85 C9 74 0B 8B 01 6A 01 FF 50 04 83 66 4C 00 33 C0 40 5E C3");
        nativeLanguage = reinterpret_cast<decltype(nativeLanguage)>(Find("B8 ? ? ? ? C3 8B 41 0C 85 C0 74 19 8B 40 18"));
        currentMovie = reinterpret_cast<decltype(currentMovie)>(Find("E8 ? ? ? ? 85 C0 74 ? E8 ? ? ? ? 8B 40 4C C3"));
        currentManager = reinterpret_cast<decltype(currentManager)>(injector::GetBranchDestination(currentMovie).as_int());
        stringCtor = reinterpret_cast<decltype(stringCtor)>(Find("33 C0 56 8B F1 57 8B 7C 24 0C 89 46 04 89 46 08 89 06 89 46 0C 38 07 74"));
        stringDtor = reinterpret_cast<decltype(stringDtor)>(Find("8B CE 5E E9 ? ? ? ? 56 8B F1 33 C0 39 46 08 89 46 04 74 0A 6A 01 89 46 08 E8") + 8);
        wideDtor = reinterpret_cast<decltype(wideDtor)>(Find("56 8B F1 33 C0 39 46 08 89 46 04 74 0A 6A 02 89 46 08 E8"));
        invokeAddress = Find("55 8B EC 83 EC 34 56 8B 75 0C 8B CE E8 ? ? ? ? 8B 4D 08 8D 55 18 52 FF 75 14 8D 55 CC FF 75 10 C6 06 00 8B 01 52 FF 50 34");
        retireHook = safetyhook::create_inline(reinterpret_cast<void*>(retire), Retire);
        commandHook = safetyhook::create_inline(reinterpret_cast<void*>(command), Command);
        ready = true;
        RWGTA::UpdateCheck::Config update;
        update.pluginId = "SplinterCellConviction.FusionFix";
        update.fileVersion = rsc_FileVersion;
        update.settings = &FixSettings();
        update.playable = []() { return noticeSource && ready && !context && MovieValid(callbackMovie); };
        update.show = [](const std::wstring& text)
        {
            auto title = RWGTAMenu::PluginText(FixSettings(), Language(), "update.title");
            if (Invoke(callbackMovie, "openFixNotice", "WW", Wide(title).c_str(), text.c_str()) && context)
                context->details = context->notice = true;
        };
        RWGTA::UpdateCheck::Init(std::move(update));
        IATHook::Replace(GetModuleHandleW(nullptr), "KERNEL32.dll", std::forward_as_tuple("ExitProcess", ExitNotice));
    }
}
