#pragma once
#include "FixSettings.hpp"
#include "../Shared/RWGTA/FixMenu.hpp"
#include <charconv>
#include <set>

// The movie owns its Page/ListBox/Label objects. The bridge only responds to
// that movie's FSCommands, using the native Invoke/string protocol.
namespace SCCMenu
{
    inline SafetyHookInline commandHook, retireHook;
    inline HMODULE module = nullptr;
    inline bool ready = false;
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
        if (value.size() > 512) { value.resize(512); if (value.back() >= 0xD800 && value.back() <= 0xDBFF) value.pop_back(); }
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

    enum class Kind { Page, Next, Previous, Back, Setting };
    struct Link { Kind kind; size_t index = 0; RWGTAMenu::Row row; };
    struct Context
    {
        RWGTAMenu::Model model;
        void* movie = nullptr;
        void* manager = nullptr;
        bool pause = false;
        bool opening = false;
        bool committed = false;
        int page = -1;
        size_t offset = 0, rowOffset = 0;
        std::vector<Link> links;
    };
    inline std::shared_ptr<Context> context;
    inline bool Current(const std::shared_ptr<Context>& ctx)
    { return context == ctx && ctx && ctx->manager == callbackManager && ctx->model.IsOpen() && MovieValid(ctx->movie); }
    inline void Close()
    {
        auto old = std::exchange(context, {});
        // Discovery can call foreign descriptor/getter APIs. Keep its Model
        // alive until Open unwinds if that callback retires the source movie.
        if (old && !old->opening) old->model.Close();
    }
    inline void Populate(const std::shared_ptr<Context>& ctx, int selection = 0)
    {
        if (!Current(ctx)) return;
        ctx->committed = false;
        struct RowText { std::string label, value; bool enabled; Link link; };
        std::vector<RowText> rows;
        auto title = ctx->model.Text("menu.plugins");
        if (ctx->page < 0)
        {
            const auto& pages = ctx->model.Pages();
            for (size_t i = ctx->offset; i < pages.size() && i < ctx->offset + 4; ++i)
                rows.push_back({pages[i].title, {}, true, {Kind::Page, i, {}}});
            if (ctx->offset) rows.push_back({ctx->model.Text("menu.previous"), {}, true, {Kind::Previous, 0, {}}});
            if (ctx->offset + 4 < pages.size()) rows.push_back({ctx->model.Text("menu.next"), {}, true, {Kind::Next, 0, {}}});
        }
        else
        {
            auto visible = ctx->model.VisibleRows(size_t(ctx->page));
            if (!Current(ctx)) return;
            if (size_t(ctx->page) >= ctx->model.Pages().size()) return;
            title = ctx->model.Pages()[size_t(ctx->page)].title;
            if (ctx->rowOffset >= visible.size()) ctx->rowOffset = 0;
            for (size_t i = ctx->rowOffset; i < visible.size() && i < ctx->rowOffset + 4; ++i)
            {
                auto row = *visible[i];
                auto label = ctx->model.Label(row);
                auto enabled = ctx->model.Enabled(row, ctx->pause);
                if (!Current(ctx)) return;
                auto value = ctx->model.Value(row);
                if (!Current(ctx)) return;
                if (!enabled) value = ctx->model.Text("menu.unavailable");
                rows.push_back({std::move(label), std::move(value), enabled, {Kind::Setting, 0, row}});
            }
            if (ctx->rowOffset) rows.push_back({ctx->model.Text("menu.previous"), {}, true, {Kind::Previous, 0, {}}});
            if (ctx->rowOffset + 4 < visible.size()) rows.push_back({ctx->model.Text("menu.next"), {}, true, {Kind::Next, 0, {}}});
        }
        rows.push_back({ctx->model.Text("menu.back"), {}, true, {Kind::Back, 0, {}}});
        // Four plugin links or settings, plus previous/next and Back.
        if (rows.size() > 7) { Log("private row bound rejected"); return; }
        if (!Invoke(ctx->movie, "resetFixMenu", "W", Wide(title).c_str()) || !Current(ctx)) return;
        ctx->links.clear();
        for (const auto& row : rows)
        {
            if (!Invoke(ctx->movie, "appendFixMenu", "WWB", Wide(row.label).c_str(), Wide(row.value).c_str(), int(row.enabled)) || !Current(ctx)) return;
            ctx->links.push_back(row.link);
        }
        if (!Invoke(ctx->movie, "finishFixMenu", "i", selection) || !Current(ctx))
        {
            Log("private page native commit rejected");
            if (context == ctx) { Close(); Invoke(ctx->movie, "leaveFixMenu", ""); }
            return;
        }
        RWGTAMenu::Model::Changed() = false;
        ctx->committed = true;
    }
    inline int Index(const char* args)
    {
        if (!args) return -1;
        int index = -1; auto end = args + strlen(args);
        auto result = std::from_chars(args, end, index);
        return result.ec == std::errc{} ? index : -1;
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
            ctx->model.OpenPlugins(module, Language(), 4096);
            ctx->opening = false;
            if (context != ctx) { ctx->model.Close(); return 1; }
            if (Current(ctx))
            {
                Populate(ctx);
            }
            return 1;
        }
        auto ctx = context;
        if (!Current(ctx) || ctx->movie != movie || ctx->manager != manager) return 1;
        if (cmd == "FixMenuClose") { Close(); return 1; }
        if (cmd == "FixMenuRefresh") { if (RWGTAMenu::Model::Changed()) Populate(ctx, Index(args)); return 1; }
        if (cmd == "FixMenuBack")
        {
            if (ctx->page >= 0) { ctx->model.PageShown(size_t(ctx->page)); ctx->page = -1; ctx->rowOffset = 0; Populate(ctx); }
            else { Close(); Invoke(movie, "leaveFixMenu", ""); }
            return 1;
        }
        auto index = Index(args);
        if (index < 0 || size_t(index) >= ctx->links.size()) return 1;
        auto link = ctx->links[size_t(index)];
        if (cmd == "FixMenuDescription")
        {
            std::string description;
            if (link.kind == Kind::Setting) description = ctx->model.Help(link.row);
            if (ctx->model.RestartRequired())
            {
                auto warning = ctx->model.Text("menu.restartwarning");
                if (!description.empty()) warning += " - " + description;
                description = std::move(warning);
            }
            Invoke(movie, "describeFixMenu", "W", Wide(description).c_str()); return 1;
        }
        if (cmd != "FixMenuBack" && cmd != "FixMenuAccept" && cmd != "FixMenuLeft" && cmd != "FixMenuRight") return 1;
        if (link.kind != Kind::Setting && cmd != "FixMenuAccept") return 1;
        if (link.kind == Kind::Back)
        {
            if (ctx->page >= 0) { ctx->model.PageShown(size_t(ctx->page)); ctx->page = -1; ctx->rowOffset = 0; Populate(ctx); }
            else { Close(); Invoke(movie, "leaveFixMenu", ""); }
        }
        else if (link.kind == Kind::Page) { ctx->page = int(link.index); ctx->rowOffset = 0; ctx->model.PageShown(link.index); Populate(ctx); }
        else if (link.kind == Kind::Next) { (ctx->page < 0 ? ctx->offset : ctx->rowOffset) += 4; Populate(ctx); }
        else if (link.kind == Kind::Previous) { auto& offset = ctx->page < 0 ? ctx->offset : ctx->rowOffset; offset = offset >= 4 ? offset - 4 : 0; Populate(ctx); }
        else
        {
            int direction = cmd == "FixMenuLeft" ? -1 : cmd == "FixMenuRight" ? 1 : 0;
            ctx->model.Change(link.row, direction, ctx->pause);
            if (Current(ctx)) Populate(ctx, index);
        }
        return 1;
    }
    inline int __fastcall Retire(void* manager, void* edx)
    {
        if (context && context->manager == manager) Close();
        return retireHook.fastcall<int>(manager, edx);
    }
    inline uintptr_t Unique(const char* pattern)
    {
        auto found = hook::pattern(pattern); return found.size() == 1 ? reinterpret_cast<uintptr_t>(found.get_first()) : 0;
    }
    inline void Init()
    {
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&Init), &module);
        auto command = Unique("55 8D 6C 24 94 81 EC C0 00 00 00 53 56 57 89 4D 68 33 DB 33 FF E8 ? ? ? ? 8B F0");
        auto retire = Unique("56 8B F1 8B 4E 4C 85 C9 74 0B 8B 01 6A 01 FF 50 04 83 66 4C 00 33 C0 40 5E C3");
        nativeLanguage = reinterpret_cast<decltype(nativeLanguage)>(Unique("B8 ? ? ? ? C3 8B 41 0C 85 C0 74 19 8B 40 18"));
        currentMovie = reinterpret_cast<decltype(currentMovie)>(Unique("E8 ? ? ? ? 85 C0 74 ? E8 ? ? ? ? 8B 40 4C C3"));
        if (currentMovie)
        {
            auto getter = reinterpret_cast<uintptr_t>(currentMovie);
            auto first = injector::GetBranchDestination(getter).as_int();
            auto second = injector::GetBranchDestination(getter + 9).as_int();
            if (first == second) currentManager = reinterpret_cast<decltype(currentManager)>(first);
        }
        stringCtor = reinterpret_cast<decltype(stringCtor)>(Unique("33 C0 56 8B F1 57 8B 7C 24 0C 89 46 04 89 46 08 89 06 89 46 0C 38 07 74"));
        stringDtor = reinterpret_cast<decltype(stringDtor)>(Unique("56 8B F1 33 C0 39 46 08 89 46 04 74 0A 6A 01 89 46 08 E8"));
        wideDtor = reinterpret_cast<decltype(wideDtor)>(Unique("56 8B F1 33 C0 39 46 08 89 46 04 74 0A 6A 02 89 46 08 E8"));
        invokeAddress = Unique("55 8B EC 83 EC 34 56 8B 75 0C 8B CE E8 ? ? ? ? 8B 4D 08 8D 55 18 52 FF 75 14 8D 55 CC FF 75 10 C6 06 00 8B 01 52 FF 50 34");
        if (!command || !retire || !nativeLanguage || !currentMovie || !currentManager || !stringCtor || !stringDtor || !wideDtor || !invokeAddress)
        { Log("native Flash bridge signatures unavailable; category hidden"); return; }
        retireHook = safetyhook::create_inline(reinterpret_cast<void*>(retire), Retire);
        if (retireHook) commandHook = safetyhook::create_inline(reinterpret_cast<void*>(command), Command);
        if (!retireHook || !commandHook) { commandHook.reset(); retireHook.reset(); Log("native Flash bridge hook transaction failed"); return; }
        ready = true;
    }
}
