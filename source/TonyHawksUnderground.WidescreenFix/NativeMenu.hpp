#pragma once
#include "NativeMenuABI.hpp"
#include "NativeMenuContracts.hpp"
#include "../Shared/RWGTA/FixMenu.hpp"
#include <cmath>

// The "Plugins" row of the Options page and its pages, built with the game's own themed menu scripts (title
// Options: themed scrolling menu, pause Options: themed option menu). Choose, left/right and back are taken from
// the controller events while one of our pages (or the Plugins row) has focus; up/down stay the game's.
namespace THUGMenu
{
    constexpr auto EntryID = Checksum("wfp_thug_entry");

    struct Weak
    {
        void* element = nullptr;
        void* previous = nullptr;
        void* next = nullptr;
        Weak() = default;
        Weak(const Weak&) = delete;
        Weak& operator=(const Weak&) = delete;
        ~Weak() { Reset(); }
        // At process exit the elements may already be gone: drop the pointer without unlinking
        void Forget() { element = previous = next = nullptr; }
        void Reset(void* value = nullptr)
        {
            if (element && Readable(element, 8))
                reinterpret_cast<void(__thiscall*)(void*, Weak*)>(0x420F30)(element, this);
            element = value;
            previous = next = nullptr;
            if (element)
                reinterpret_cast<void(__thiscall*)(void*, Weak*)>(0x420F10)(element, this);
        }
    };
    static_assert(sizeof(Weak) == 12);

    enum class Kind { Setting, Page, Back, Info };
    struct Item { Kind kind; uint32_t id, textId; size_t index; RWGTAMenu::Row row{}; std::string key, text; bool enabled = true; };
    enum class Command { Open, Accept, Left, Right, Back };
    struct Pending { Command command; uint32_t generation, row, selected; void* owner; };
    struct Busy
    {
        bool& flag;
        bool previous;
        explicit Busy(bool& value) : flag(value), previous(value) { flag = true; }
        ~Busy() { flag = previous; }
    };
    struct State
    {
        HMODULE module = nullptr;
        RWGTAMenu::Model model;
        Weak base, baseList, entry, root, list;
        std::vector<Item> items;
        std::vector<Pending> pending;
        SafetyHookInline createHook, taskHook;
        uint8_t(__thiscall* launch)(void*, uint32_t, uint32_t, uint32_t, Structure*, int) = nullptr;
        uint32_t generation = 1, baseID = 0;
        std::string focusKey;       // the row to select again once a rebuilt page has focus
        bool fitPending = false;    // title rows are measured after they were drawn once
        bool returnToEntry = false; // select the Plugins row when the stock page is back
        int page = -1;
        bool pause = false, open = false, busy = false, draining = false;
        unsigned frameDepth = 0;
    };
    inline State state;

    inline void Log(const char* line)
    {
        OutputDebugStringA(line);
        OutputDebugStringA("\n");
    }
    inline void* Lookup(uint32_t id)
    {
        auto* manager = Read<void*>(reinterpret_cast<void*>(0x69B6C0));
        if (!Readable(manager, 16) || !Read<void*>(manager, 8) || !Read<void*>(manager, 12)) return nullptr;
        return reinterpret_cast<void*(__thiscall*)(void*, uint32_t)>(0x41FBB0)(manager, id);
    }
    inline void* Lookup(const char* name) { return Lookup(Checksum(name)); }
    inline void Run(const char* name, Parameters& params)
    {
        reinterpret_cast<void(__cdecl*)(uint32_t, Structure*, void*, int)>(0x40FD00)(Checksum(name), &params.value, nullptr, 0);
    }
    // Scripts that wait for frames run in the game's script VM, not synchronously
    inline bool Spawn(const char* name, Parameters& params)
    {
        return reinterpret_cast<void*(__cdecl*)(uint32_t, Structure*, uint32_t, Structure*, int, uint32_t, int, int, int, int)>(0x40FF60)
            (Checksum(name), &params.value, 0, nullptr, -1, 0, 0, 0, 0, 0) != nullptr;
    }
    inline void Emit(uint32_t type, uint32_t target, Structure* params = nullptr)
    {
        auto* manager = Read<void*>(reinterpret_cast<void*>(0x69B6C0));
        if (Readable(manager, 16))
            reinterpret_cast<uint8_t(__thiscall*)(void*, uint32_t, uint32_t, uint32_t, Structure*, int)>(0x4207B0)(manager, type, target, 0x36B2EE74, params, 0);
    }
    inline void* Symbol(const char* name)
    {
        auto* table = Read<void*>(reinterpret_cast<void*>(0x69851C));
        if (!Readable(table, 4096 * 20)) return nullptr;
        return reinterpret_cast<void*(__cdecl*)(uint32_t)>(0x4133A0)(Checksum(name));
    }
    inline bool Script(const char* name)
    {
        auto* symbol = Symbol(name);
        return Read<uint8_t>(symbol, 1) == 7 && Read<void*>(symbol, 12);
    }
    inline void Sound(const char* name)
    {
        Parameters params;
        if (Script(name)) Run(name, params);
    }
    inline std::string Language()
    {
        auto forced = FixSettings().String("MAIN.TextLanguage");
        if (!forced.empty() && forced != "auto") return forced;
        switch (Read<int>(reinterpret_cast<void*>(0x7CA180)))
        {
            case 1: return "ja"; case 3: return "fr"; case 4: return "es";
            case 5: return "de"; case 6: return "it"; case 7: return "nl";
            case 8: return "pt"; default: return "en";
        }
    }
    inline std::string NativeText(const std::string& text)
    {
        if (text.empty()) return {};
        if (text.size() > 2048) return "?";
        auto count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), nullptr, 0);
        if (count <= 0) return "?";
        std::wstring wide(count, L'\0');
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), wide.data(), count);
        count = WideCharToMultiByte(1252, WC_NO_BEST_FIT_CHARS, wide.data(), static_cast<int>(wide.size()), nullptr, 0, "?", nullptr);
        if (count <= 0) return "?";
        std::string converted(count, '\0');
        WideCharToMultiByte(1252, WC_NO_BEST_FIT_CHARS, wide.data(), static_cast<int>(wide.size()), converted.data(), count, "?", nullptr);
        return EscapeNativeText(converted);
    }
    inline bool Modal() { return Lookup("keyboard_anchor") || Lookup("dialog_box_anchor") || Lookup("quit_dialog_anchor"); }
    inline bool Current(bool privateMenu, bool requireFocus = true)
    {
        auto* root = privateMenu ? state.root.element : state.base.element;
        auto* list = privateMenu ? state.list.element : state.baseList.element;
        return root && list && Lookup("current_menu_anchor") == root && Lookup("current_menu") == list
            && Read<uint32_t>(list) == 0x632798 && (!requireFocus || Read<uint8_t>(list, 356) == 1) && !Modal();
    }
    inline bool Fit(uint32_t id, float maximum)
    {
        auto* element = Lookup(id);
        if (!element || Read<uint32_t>(element) != 0x632ABC) return false;
        // Measured text size
        float width = Read<float>(element, 0x148), height = Read<float>(element, 0x14C);
        if (!std::isfinite(width) || !std::isfinite(height) || width <= maximum || height <= 0) return true;
        auto scale = maximum / width;
        if (!std::isfinite(scale) || scale < 0.1f) return false;
        Parameters params; params.Name("id", id); params.Pair("scale", scale, 1.0f); params.Integer("time", 0);
        Run("DoScreenElementMorph", params);
        return Lookup(id) == element;
    }
    inline bool Origin(void* script, uint32_t id)
    {
        if (!Readable(script, 228)) return false;
        if (Read<uint32_t>(script, 188) == id) return true;
        auto count = Read<uint32_t>(script, 48);
        auto* frames = Read<uint8_t*>(script, 140);
        if (count > 16 || !Readable(frames, count * 44)) return false;
        for (uint32_t i = 0; i < count; ++i) if (Read<uint32_t>(frames, i * 44) == id) return true;
        return false;
    }

    // The Plugins row is added with the menu's own row helper while the stock page is built: before Done on the
    // title Options page, after Display Options on the pause Options page
    inline void AddEntry(bool pause)
    {
        const char* helper = pause ? "make_theme_menu_item" : "theme_menu_add_item";
        if (state.busy || Lookup(EntryID) || !Script(helper)) return;
        Busy busy(state.busy);
        Parameters params; params.Name("id", EntryID);
        params.Text("text", NativeText(RWGTAMenu::PluginText(FixSettings(), Language(), "menu.plugins")));
        if (!pause) { params.Flag("centered"); params.Flag("no_bg"); }
        Run(helper, params);
        if (Read<uint32_t>(Lookup(EntryID)) != 0x632864) Log("THUG native menu: Plugins row creation failed");
    }
    inline uint8_t __cdecl Created(Structure* params, void* script)
    {
        uint32_t id = 0;
        if (!script || !reinterpret_cast<uint8_t(__thiscall*)(Structure*, uint32_t, uint32_t*, int)>(0x4110B0)(params, Checksum("id"), &id, 0))
            return state.createHook.unsafe_ccall<uint8_t>(params, script);
        if (id == Checksum("options_done") && Origin(script, Checksum("create_setup_options_menu")))
            AddEntry(false);
        auto result = state.createHook.unsafe_ccall<uint8_t>(params, script);
        if (id == Checksum("menu_display_options") && Origin(script, Checksum("create_options_menu")))
            AddEntry(true);
        bool pause = id == Checksum("options_menu") && Origin(script, Checksum("create_options_menu"));
        bool title = id == Checksum("sub_menu") && Origin(script, Checksum("create_setup_options_menu"));
        if (!pause && !title) return result;
        auto* root = Lookup(id);
        if (!root || Read<uint32_t>(root) != 0x632864) return result;
        ++state.generation;
        state.base.Reset(root); state.baseList.Reset(); state.entry.Reset();
        state.baseID = id;
        state.pause = pause;
        return result;
    }

    inline void StopModel()
    {
        ++state.generation;
        state.open = false;
        state.pending.clear(); state.items.clear();
        state.list.Reset(); state.root.Reset();
        state.model.Close(); state.page = -1;
    }
    // Back from the Plugins list: the stock Options page is built again (it replaces our page)
    inline void Close()
    {
        Busy busy(state.busy);
        if (state.root.element && Lookup("current_menu_anchor") == state.root.element && !Modal())
        {
            const char* name = state.pause ? "create_options_menu" : "create_setup_options_menu";
            Parameters params;
            if (!Script(name) || !Spawn(name, params))
                Log("THUG native menu: stock Options page could not be rebuilt");
            else
                state.returnToEntry = true;
        }
        StopModel();
    }
    inline std::string Key(const RWGTAMenu::Row& row)
    {
        auto result = std::to_string(static_cast<int>(row.kind)) + ":" + std::to_string(row.plugin ? reinterpret_cast<uintptr_t>(row.plugin->module) : 0);
        if (row.setting) result += ":" + row.setting->id;
        return result;
    }
    inline void Add(std::vector<Item>& items, Kind kind, size_t index, std::string key, std::string text, bool enabled = true, RWGTAMenu::Row row = {})
    {
        auto n = std::to_string(items.size());
        items.push_back({ kind, Checksum(("wfp_thug_row_" + n).c_str()), Checksum(("wfp_thug_text_" + n).c_str()), index, row, std::move(key), std::move(text), enabled });
    }
    inline bool Prepare(std::vector<Item>& items)
    {
        const auto& pages = state.model.Pages();
        if (state.page < 0)
        {
            for (size_t i = 0; i < pages.size(); ++i)
                Add(items, Kind::Page, i, "page:" + std::to_string(i), pages[i].title);
        }
        else
        {
            if (static_cast<size_t>(state.page) >= pages.size()) return false;
            for (auto* source : state.model.VisibleRows(state.page))
            {
                RWGTAMenu::Row row = *source;
                auto text = state.model.Label(row);
                auto value = state.model.Value(row);
                bool enabled = state.model.Enabled(row, state.pause);
                if (!enabled) value = state.model.Text("menu.unavailable");
                if (!value.empty()) text += ": " + value;
                Add(items, Kind::Setting, 0, Key(row), std::move(text), enabled, row);
            }
            if (state.model.RestartRequired())
                Add(items, Kind::Info, 0, "restart", state.model.Text("menu.restartwarning"), false);
        }
        Add(items, Kind::Back, 0, "back", state.model.Text("menu.back"));
        return state.model.IsOpen();
    }
    // Builds the page from scratch with the game's themed menu scripts; keep: select the same row again
    inline bool Build(bool keep = false)
    {
        state.focusKey.clear();
        if (keep)
        {
            const auto selected = Read<uint32_t>(state.list.element, 352);
            for (const auto& item : state.items) if (item.id == selected) state.focusKey = item.key;
        }
        std::vector<Item> items;
        if (!Prepare(items)) return false;
        Busy busy(state.busy);
        ++state.generation;
        // The title font has capitals only
        auto title = NativeText(state.model.Text("menu.plugins"));
        CharUpperBuffA(title.data(), static_cast<DWORD>(title.size()));
        Parameters menu; menu.Text("title", title);
        if (state.pause) Run("make_new_themed_option_menu", menu);
        else { menu.Pair("dims", 300, 252); Run("make_new_themed_scrolling_menu", menu); }
        auto* root = Lookup("current_menu_anchor");
        auto* list = Lookup("current_menu");
        if (Read<uint32_t>(root) != 0x632864 || Read<uint32_t>(list) != 0x632798 || Modal()) return false;
        state.root.Reset(root); state.list.Reset(list);
        state.open = true;
        const char* helper = state.pause ? "make_theme_menu_item" : "theme_menu_add_item";
        for (const auto& item : items)
        {
            Parameters row; row.Name("id", item.id); row.Name("text_id", item.textId);
            row.Text("text", NativeText(item.text));
            if (!state.pause) { row.Flag("centered"); row.Flag("no_bg"); }
            if (!item.enabled) row.Flag("not_focusable");
            // Pause rows have no text id to measure: shrink long ones by the font's widest average advance
            if (state.pause && item.text.size() * 12.5f > 380.0f) row.Float("scale", 380.0f / (item.text.size() * 12.5f));
            Run(helper, row);
            if (Lookup("current_menu_anchor") != root) return false;
        }
        state.items = std::move(items);
        state.fitPending = !state.pause;
        if (state.pause)
            Emit(Checksum("focus"), Read<uint32_t>(root, 20));
        else
        {
            Parameters finish;
            Spawn("finish_themed_scrolling_menu", finish);
        }
        return true;
    }
    // After a value changed: the texts change in place, the page is built again only when its rows changed
    inline bool Refresh()
    {
        std::vector<Item> items;
        if (!Prepare(items)) return false;
        bool same = items.size() == state.items.size();
        for (size_t i = 0; same && i < items.size(); ++i) same = items[i].key == state.items[i].key && items[i].enabled == state.items[i].enabled;
        if (!same) return Build(true);
        Busy busy(state.busy);
        for (size_t i = 0; i < items.size(); ++i)
        {
            if (items[i].text == state.items[i].text) continue;
            // Title rows have their own text id; pause rows are addressed as { row child = 0 }
            Parameters id; id.Flag(items[i].id); id.Integer("child", 0);
            Parameters text;
            if (state.pause) text.Nested("id", &id.value);
            else text.Name("id", items[i].textId);
            text.Text("text", NativeText(items[i].text));
            Run("SetScreenElementProps", text);
            if (!state.pause) Fit(items[i].textId, 280);
        }
        state.items = std::move(items);
        return true;
    }
    inline void Open()
    {
        if (!Current(false) || !Script("make_new_themed_scrolling_menu") || !Script("make_new_themed_option_menu")
            || !Script("theme_menu_add_item") || !Script("make_theme_menu_item") || !Script("finish_themed_scrolling_menu")) return;
        state.model.OpenPlugins(state.module, Language(), 64);
        state.page = -1;
        if (!Build()) { Close(); return; }
    }
    inline void Activate(const Pending& input)
    {
        if (!Current(true) || input.generation != state.generation || input.owner != state.root.element) return;
        if (input.command == Command::Back)
        {
            Sound("generic_menu_pad_back_sound");
            if (state.page < 0) { Close(); return; }
            state.page = -1;
            if (!Build()) Close();
            return;
        }
        if (input.row >= state.items.size() || Read<uint32_t>(state.list.element, 352) != input.selected) return;
        auto item = state.items[input.row];
        int direction = input.command == Command::Left ? -1 : input.command == Command::Right ? 1 : 0;
        if ((item.kind != Kind::Setting && direction) || item.kind == Kind::Info || !item.enabled) return;
        Sound(direction ? "generic_menu_scroll_sideways_sound" : "generic_menu_pad_choose_sound");
        if (item.kind == Kind::Back)
        {
            if (state.page < 0) { Close(); return; }
            state.page = -1;
            if (!Build()) Close();
            return;
        }
        if (item.kind == Kind::Page)
        {
            state.page = static_cast<int>(item.index);
            state.model.PageShown(item.index);
            if (!Build()) Close();
            return;
        }
        state.model.Change(item.row, direction, state.pause);
        if (state.open && !Refresh()) Close();
    }
    // Controller events go to root_window and down the focus chain to the selected row; while our page or the
    // Plugins row has focus, choose/left/right/back are queued here and run after the frame (Task)
    inline bool Intercept(uint32_t type)
    {
        Command command;
        if (type == Checksum("pad_choose") || type == Checksum("pad_start")) command = Command::Accept;
        else if (type == Checksum("pad_left")) command = Command::Left;
        else if (type == Checksum("pad_right")) command = Command::Right;
        else if (type == Checksum("pad_back")) command = Command::Back;
        else return false;
        if (state.busy || state.pending.size() >= 32) return false;
        if (!state.open)
        {
            if (command != Command::Accept || !state.entry.element || !Current(false) || Read<uint32_t>(state.baseList.element, 352) != EntryID) return false;
            state.pending.push_back({ Command::Open, state.generation, 0, EntryID, state.base.element });
            return true;
        }
        if (!Current(true)) return false;
        const auto selected = Read<uint32_t>(state.list.element, 352);
        uint32_t row = 0;
        while (row < state.items.size() && state.items[row].id != selected) ++row;
        if (command == Command::Back || row < state.items.size())
            state.pending.push_back({ command, state.generation, row, selected, state.root.element });
        return true;
    }
    inline uint8_t __fastcall Launch(void* manager, void*, uint32_t type, uint32_t target, uint32_t source, Structure* params, int flag)
    {
        if (Intercept(type)) return 1;
        return state.launch(manager, type, target, source, params, flag);
    }
    // Finds the stock page's anchor and list once it has focus
    inline void Adopt()
    {
        if (!state.base.element || state.open || Modal()) return;
        // The themed title page creates its root inside the current menu anchor: own the anchor then
        if (auto* anchor = Lookup("current_menu_anchor"); anchor != state.base.element)
        {
            if (!anchor || Read<uint32_t>(anchor) != 0x632864 || Lookup(state.baseID) != state.base.element) return;
            state.base.Reset(anchor);
        }
        auto* list = Lookup("current_menu");
        if (Read<uint32_t>(list) != 0x632798 || Read<uint8_t>(list, 356) != 1) return;
        state.baseList.Reset(list);
        if (!state.entry.element)
        {
            if (auto* entry = Lookup(EntryID); Read<uint32_t>(entry) == 0x632864) state.entry.Reset(entry);
        }
        if (state.returnToEntry && state.entry.element)
        {
            state.returnToEntry = false;
            if (Read<uint32_t>(list, 352) == EntryID) return;
            Busy busy(state.busy);
            const auto id = Read<uint32_t>(list, 20);
            Emit(Checksum("unfocus"), id);
            Parameters focus; focus.Name("child_id", EntryID);
            Emit(Checksum("focus"), id, &focus.value);
        }
    }
    inline int __cdecl Task(void* task)
    {
        ++state.frameDepth;
        auto result = state.taskHook.unsafe_ccall<int>(task);
        --state.frameDepth;
        if (state.frameDepth || state.draining || state.busy) return result;
        Busy drain(state.draining);
        // Left by other means (a modal can cover the page for a while)
        if (state.open && (!state.root.element || (Lookup("current_menu_anchor") != state.root.element && !Modal()))) StopModel();
        auto pending = std::move(state.pending); state.pending.clear();
        for (const auto& input : pending)
        {
            if (input.command == Command::Open)
            {
                if (!state.open && input.generation == state.generation && input.owner == state.base.element
                    && Current(false) && Read<uint32_t>(state.baseList.element, 352) == EntryID) Open();
            }
            else Activate(input);
        }
        if (state.open && Current(true) && RWGTAMenu::Model::Changed().exchange(false))
            if (!Refresh()) Close();
        if (state.open && Current(true))
        {
            if (state.fitPending)
            {
                Busy busy(state.busy);
                state.fitPending = false;
                for (const auto& item : state.items) Fit(item.textId, 280);
            }
            if (!state.focusKey.empty())
            {
                auto key = std::move(state.focusKey);
                state.focusKey.clear();
                for (const auto& item : state.items)
                {
                    if (item.key != key || !item.enabled || Read<uint32_t>(state.list.element, 352) == item.id) continue;
                    Busy busy(state.busy);
                    const auto list = Read<uint32_t>(state.list.element, 20);
                    Emit(Checksum("unfocus"), list);
                    Parameters focus; focus.Name("child_id", item.id);
                    Emit(Checksum("focus"), list, &focus.value);
                }
            }
        }
        Adopt();
        return result;
    }
    inline void Init()
    {
        if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&Init), &state.module) || !Contracts()) { Log("THUG native menu: unsupported executable"); return; }
        auto pattern = hook::pattern("68 74 EE B2 36 68 74 EE B2 36 55 E8");
        if (pattern.size() != 1) { Log("THUG native menu: controller event call not found"); return; }
        // The game keeps calling into this module (hooks, the controller event call); unloading it is unsupported
        if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
            reinterpret_cast<LPCWSTR>(&Init), &state.module)) { Log("THUG native menu: module lifetime pin failed"); return; }
        state.createHook = safetyhook::create_inline(reinterpret_cast<void*>(0x490B50), Created);
        state.taskHook = safetyhook::create_inline(reinterpret_cast<void*>(0x503F90), Task);
        if (!state.createHook || !state.taskHook) { state.taskHook.reset(); state.createHook.reset(); Log("THUG native menu: hooks failed"); return; }
        state.launch = reinterpret_cast<decltype(state.launch)>(injector::GetBranchDestination(pattern.get_first(11)).as_int());
        injector::MakeCALL(pattern.get_first(11), Launch, true);
    }
    inline void Shutdown(bool terminating)
    {
        if (!state.taskHook && !state.createHook) return;
        state.busy = true;
        if (terminating)
        {
            for (auto* weak : { &state.base, &state.baseList, &state.entry, &state.root, &state.list }) weak->Forget();
            return;
        }
        state.taskHook.reset(); state.createHook.reset();
        StopModel(); state.entry.Reset(); state.baseList.Reset(); state.base.Reset();
    }
}
