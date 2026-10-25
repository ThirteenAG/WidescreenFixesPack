#pragma once
#include "FixSettings.hpp"
#include "NativeMenuBindings.hpp"
#include "../Shared/RWGTA/FixMenu.hpp"
#include <memory>
#include <set>

namespace THUG2Menu
{
    inline SafetyHookInline taskHook, callHook, destructorHook;
    inline char(__thiscall* launch)(void*, uint32_t, uint32_t, uint32_t, Structure*, int, float) = nullptr;
    inline HMODULE module = nullptr;
    inline bool ready = false, registered = false, draining = false;
    inline unsigned taskDepth = 0;
    inline DWORD nativeThread = 0;
    inline uint32_t nextSerial = 0;
    constexpr uint32_t privateAnchor = Checksum("wfp_thug2_anchor");
    constexpr uint32_t privateMenu = Checksum("wfp_thug2_vmenu");
    constexpr uint32_t entryId = Checksum("wfp_thug2_entry");
    constexpr uint32_t titleId = Checksum("wfp_thug2_title");

    inline void LogOnce(const std::string& message)
    {
        static std::set<std::string> seen;
        if (seen.size() >= 32 || !seen.insert(message).second) return;
        OutputDebugStringA(("THUG2 native menu: " + message + "\n").c_str());
    }
    inline uint32_t Serial()
    {
        nextSerial = (nextSerial + 1) & 0x7FFFFFFF;
        if (!nextSerial) ++nextSerial;
        return nextSerial;
    }
    inline std::string Language()
    {
        auto chosen = FixSettings().String("MAIN.TextLanguage");
        if (!chosen.empty() && chosen != "auto") return chosen;
        switch (Read<uint32_t>(reinterpret_cast<void*>(0x7CFFA0)))
        {
            case 1: return "ja";
            case 2: return "en";
            case 3: return "fr";
            case 4: return "es";
            case 5: return "de";
            case 6: return "it";
            case 7: return "nl";
            case 8: return "pt";
            default:
                LogOnce("native language has no verified tag; auto falls back to English");
                return "en";
        }
    }
    inline std::string NativeText(const std::string& utf8)
    {
        auto count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.c_str(), -1, nullptr, 0);
        if (count <= 0 || count > 4096) return "?";
        std::wstring wide(size_t(count), L'\0');
        if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.c_str(), -1, wide.data(), count)) return "?";
        for (auto& c : wide) if (c < 32 && c) c = L' ';
        // The installed fan font uses the same single-byte Russian glyph slots
        // as its original menu strings. A locale9 tag is still unknown above.
        auto page = Read<uint32_t>(reinterpret_cast<void*>(0x7CFFA0)) == 9 ? 1251u : 1252u;
        auto bytes = WideCharToMultiByte(page, WC_NO_BEST_FIT_CHARS, wide.c_str(), -1, nullptr, 0, "?", nullptr);
        if (bytes <= 0 || bytes > 4096) return "?";
        std::string result(size_t(bytes), '\0');
        if (!WideCharToMultiByte(page, WC_NO_BEST_FIT_CHARS, wide.c_str(), -1, result.data(), bytes, "?", nullptr)) return "?";
        result.pop_back();
        return result;
    }
    inline std::string Fit(const std::string& label, const std::string& value, float width)
    {
        auto left = NativeText(label);
        auto suffix = value.empty() ? std::string() : ": " + NativeText(value);
        Extent measured;
        auto full = left + suffix;
        if (!Measure(full, measured)) return {};
        if (measured.width <= width) return full;
        // Keep the entire value. The native text node may subsequently scale
        // horizontally if the value alone is wider than the available column.
        while (!left.empty())
        {
            left.pop_back();
            full = left + "..." + suffix;
            if (!Measure(full, measured)) return {};
            if (measured.width <= width) return full;
        }
        return suffix.empty() ? "..." : suffix.substr(2);
    }

    struct Host
    {
        Weak anchor, menu, window;
        uint32_t serial = 0;
        bool pause = false;
    };
    enum class Kind { Link, Previous, Next, Row, Back };
    struct Item { Kind kind; size_t index = 0; bool enabled = true; std::string label, value, description; uint32_t id = 0; };
    struct Context
    {
        RWGTAMenu::Model model;
        Weak anchor, menu, window;
        uint32_t serial = 0;
        bool pause = false, retired = false, rebuilding = false, constructing = false, returnPending = false;
        size_t page = SIZE_MAX, rootStart = 0, selected = 0;
        uint32_t focused = 0;
        std::string language;
        std::vector<Item> items;
    };
    inline std::unique_ptr<Host> host;
    inline std::shared_ptr<Context> context;
    enum class Action { Open, Accept, Left, Right, Back, Focus };
    struct Pending { Action action; uint32_t serial; size_t index; };
    inline std::deque<Pending> pending;

    inline bool Modal()
    {
        // 7CF46F includes ordinary menus, so it is not a modal-only blocker.
        for (auto name : { "keyboard_anchor", "dialog_box_anchor", "quit_dialog_anchor" })
        {
            Weak object;
            object.Find(Checksum(name));
            if (object.Get()) return true;
        }
        return false;
    }
    inline bool NativeOwned(const std::shared_ptr<Context>& owner)
    {
        if (!owner || context != owner || !owner->anchor.Get() || !owner->menu.Get() || !owner->window.Get()) return false;
        Weak window;
        window.Find(Checksum("root_window"));
        return window.Get() == owner->window.Get();
    }
    inline bool Owned(const std::shared_ptr<Context>& owner)
    {
        return NativeOwned(owner) && !owner->retired && owner->model.IsOpen();
    }
    inline bool Current(const std::shared_ptr<Context>& owner)
    {
        if (!Owned(owner) || Modal()) return false;
        Weak menu, anchor;
        menu.Find(Checksum("current_menu"));
        anchor.Find(Checksum("current_menu_anchor"));
        return menu.Get() == owner->menu.Get() && anchor.Get() == owner->anchor.Get();
    }
    inline void Retire(const std::shared_ptr<Context>& owner);
    // Controller events go to root_window and down the focus chain to the selected row. While our page or the
    // Plugins row is focused, choose/left/right/back are queued here and run after the frame (Task).
    inline bool Intercept(uint32_t type)
    {
        Action action;
        if (type == Checksum("pad_choose") || type == Checksum("pad_start")) action = Action::Accept;
        else if (type == Checksum("pad_left")) action = Action::Left;
        else if (type == Checksum("pad_right")) action = Action::Right;
        else if (type == Checksum("pad_back")) action = Action::Back;
        else return false;
        if (!ready || draining || nativeThread != GetCurrentThreadId() || pending.size() >= 32 || Modal()) return false;
        auto owner = context;
        if (!owner)
        {
            if (action != Action::Accept || !host || !host->menu.Get()) return false;
            Weak current, anchor;
            current.Find(Checksum("current_menu"));
            anchor.Find(Checksum("current_menu_anchor"));
            if (current.Get() != host->menu.Get() || anchor.Get() != host->anchor.Get()
                || Read<uint8_t>(host->menu.Get(), 384) == 0 || Read<uint32_t>(host->menu.Get(), 380) != entryId) return false;
            pending.push_back({ Action::Open, host->serial, 0 });
            return true;
        }
        if (!Current(owner)) return false;
        auto selected = Read<uint32_t>(owner->menu.Get(), 380);
        size_t index = 0;
        while (index < owner->items.size() && owner->items[index].id != selected) ++index;
        if (action == Action::Back || index < owner->items.size()) pending.push_back({ action, owner->serial, index });
        return true;
    }
    inline char __fastcall Launch(void* manager, void*, uint32_t type, uint32_t target, uint32_t source, Structure* parameters, int flag, float time)
    {
        if (Intercept(type)) return 1;
        return launch(manager, type, target, source, parameters, flag, time);
    }
    inline void Color(uint32_t id, bool selected, bool enabled)
    {
        // Native skutils do_scale_up/down palette, without their spawned blur
        // loop. These remain the game's own text-control colors and renderer.
        ArrayView rgba(1, enabled ? selected ? std::vector<uint32_t>{127,102,0,100} : std::vector<uint32_t>{88,105,112,128} : std::vector<uint32_t>{70,70,70,120});
        Parameters parameters;
        parameters.Name("id", id);
        parameters.List("rgba", &rgba.value);
        Properties(parameters);
    }

    inline void Retire(const std::shared_ptr<Context>& owner)
    {
        if (!owner || owner->retired) return;
        owner->retired = true;
        // Logical retirement is safe even inside a foreign API callback: the
        // shared Model holds its plugin snapshot and aborts later preset sets.
        // Native controls are destroyed only after the outer task has returned.
        owner->model.Close();
    }
    inline void __fastcall ElementDestroyed(void* object, void*)
    {
        auto owner = context;
        if (owner && (object == owner->window.Get() || (!owner->rebuilding && (object == owner->anchor.Get() || object == owner->menu.Get())))) Retire(owner);
        destructorHook.thiscall<void>(object);
    }
    inline bool Help(const std::shared_ptr<Context>& owner, const std::string& text)
    {
        if (!Current(owner)) return false;
        const auto native = Fit(text, "", 500);
        Parameters line;
        line.Text("text", EscapeNativeText(native, true));
        ArrayView lines(10, { Parameters::Pointer(&line.value) });
        Parameters parameters;
        parameters.List("helper_text_elements", &lines.value);
        return Run("create_helper_text", parameters) && Current(owner);
    }
    inline bool Build(const std::shared_ptr<Context>& owner)
    {
        if (!Current(owner))
        {
            LogOnce("private page lost native focus");
            return false;
        }
        std::vector<Item> items;
        std::string heading = owner->model.Text("menu.plugins");
        if (owner->page == SIZE_MAX)
        {
            auto count = std::min(size_t(5), owner->model.Pages().size() - std::min(owner->rootStart, owner->model.Pages().size()));
            for (size_t i = 0; i < count; ++i)
            {
                const auto& full = owner->model.Pages()[owner->rootStart + i].title;
                auto label = full;
                std::string number;
                const auto suffix = label.rfind(" (");
                if (suffix != std::string::npos && label.back() == ')')
                {
                    number = label.substr(suffix + 2, label.size() - suffix - 3);
                    label.resize(suffix);
                }
                items.push_back({ Kind::Link, owner->rootStart + i, true, std::move(label), std::move(number), full });
            }
            if (owner->rootStart) items.push_back({ Kind::Previous, 0, true, owner->model.Text("menu.previous") });
            if (owner->rootStart + count < owner->model.Pages().size()) items.push_back({ Kind::Next, count, true, owner->model.Text("menu.next") });
        }
        else
        {
            if (owner->page >= owner->model.Pages().size()) return false;
            heading = owner->model.Pages()[owner->page].title;
            auto rows = owner->model.VisibleRows(owner->page);
            if (!Current(owner))
            {
                LogOnce("private page lost native focus");
                return false;
            }
            for (size_t i = 0; i < rows.size(); ++i)
            {
                auto row = *rows[i];
                auto enabled = owner->model.Enabled(row, owner->pause);
                if (!Current(owner))
                {
                    LogOnce("private page lost native focus");
                    return false;
                }
                auto label = owner->model.Label(row);
                auto value = enabled ? owner->model.Value(row) : owner->model.Text("menu.unavailable");
                if (!Current(owner))
                {
                    LogOnce("private page lost native focus");
                    return false;
                }
                auto description = owner->model.Help(row);
                items.push_back({ Kind::Row, i, enabled, std::move(label), std::move(value), std::move(description) });
            }
        }
        items.push_back({ Kind::Back, 0, true, owner->model.Text("menu.back") });
        if (!Current(owner) || items.empty() || items.size() > 8) return false;
        owner->serial = Serial(); // Row-pool generation rejects queued old-page input.
        owner->rebuilding = true;
        owner->constructing = true;
        auto oldAnchor = Read<uint32_t>(owner->anchor.Get(), 20);
        Destroy(oldAnchor);
        if (context != owner || owner->retired || !owner->window.Get() || Modal())
        {
            owner->rebuilding = false;
            LogOnce("private page anchor cleanup rejected");
            return false;
        }
        Weak replacement;
        replacement.Find(Checksum("current_menu_anchor"));
        if (replacement.Get())
        {
            owner->rebuilding = false;
            LogOnce("private page anchor cleanup rejected");
            return false;
        }
        Parameters root;
        root.Name("menu_id", privateAnchor);
        root.Name("vmenu_id", privateMenu);
        const auto title = EscapeNativeText(NativeText(heading), true);
        root.Text("title", title);
        root.Number("title_scale", std::min(2.0f, 44.0f / std::max(1.0f, static_cast<float>(title.size()))));
        root.Flag("no_helper_text");
        if (owner->pause)
        {
            root.XY("pos", 120, 108);
            root.XY("dims", 440, 252);
        }
        else
        {
            root.XY("dims", 300, 252);
        }
        auto issued = Run(owner->pause ? "make_new_roundbar_menu" : "make_new_themed_sub_menu", root);
        // The themed title script owns a parts container outside the menu itself.
        // Its current anchor alias is the lifetime/cleanup root for both styles.
        owner->anchor.Find(Checksum("current_menu_anchor"));
        owner->menu.Find(privateMenu);
        owner->rebuilding = false;
        if (!issued || !Current(owner) || Read<uint32_t>(owner->menu.Get(), 24) != Checksum("VMenu") || Read<uintptr_t>(owner->menu.Get()) != 0x64B048)
        {
            LogOnce(!issued ? "private page factory unavailable" : !owner->anchor.Get() ? "private page anchor missing" : !owner->menu.Get() ? "private page menu missing" : !Current(owner) ? "private page aliases mismatch" : "private page menu type mismatch");
            return false;
        }
        owner->items = std::move(items);
        owner->selected = std::min(owner->selected, owner->items.size() - 1);
        for (size_t i = 0; i < owner->items.size(); ++i)
        {
            auto& item = owner->items[i];
            auto idText = "wfp_thug2_row" + std::to_string(i);
            item.id = Checksum(idText.c_str());
            const auto width = owner->pause ? 440.0f : 300.0f;
            auto fitted = Fit(item.label, item.value, width);
            Extent extent;
            if (!Measure(fitted, extent))
            {
                LogOnce("private row metrics unavailable");
                return false;
            }
            auto xScale = extent.width > width ? width / extent.width : 1.0f;
            // The stock row keeps its own focus visuals; input is taken in Intercept
            Parameters row;
            row.Name("id", item.id);
            row.Name("parent", privateMenu);
            row.Name("text_id", Checksum((idText + "_text").c_str()));
            row.Text("text", EscapeNativeText(fitted, true));
            row.Flag("no_choose_sound");
            if (!owner->pause)
            {
                row.Flag("centered");
                row.Flag("no_bg");
            }
            if (!item.enabled) row.Flag("not_focusable");
            if (!Run(owner->pause ? "add_roundbar_menu_item" : "theme_menu_add_item", row) || !Current(owner))
            {
                LogOnce("private row factory rejected");
                return false;
            }
            Weak created;
            created.Find(item.id);
            if (!created.Get() || Read<uint32_t>(created.Get(), 24) != Checksum("ContainerElement"))
            {
                LogOnce("private row type mismatch");
                return false;
            }
            Parameters scale;
            scale.Name("id", item.id);
            scale.XY("scale", xScale, 1.0f);
            if (!Properties(scale))
            {
                LogOnce("private row scale rejected");
                return false;
            }
        }
        if (!Help(owner, owner->model.RestartRequired() ? owner->model.Text("menu.restartwarning") : owner->items[owner->selected].description))
        {
            LogOnce("native helper text creation rejected");
            return false;
        }
        Parameters finish;
        finish.Name("menu", privateAnchor);
        if (!Spawn(owner->pause ? "roundbar_menu_finish" : "finish_themed_sub_menu", finish, Checksum("wfp_thug2_finish")))
        {
            LogOnce("private menu finish rejected");
            return false;
        }
        if (!Current(owner))
        {
            LogOnce("private page lost native focus");
            return false;
        }
        owner->focused = owner->items[owner->selected].id;
        Parameters focus;
        focus.Name("child_id", owner->items[owner->selected].id);
        if (!Fire("focus", privateMenu, &focus.value) || !Current(owner))
        {
            LogOnce("private initial focus rejected");
            return false;
        }
        RWGTAMenu::Model::Changed() = false;
        owner->constructing = false;
        return true;
    }

    inline bool Active(const std::shared_ptr<Context>& owner, size_t index)
    {
        return Current(owner) && !Modal() && index < owner->items.size()
            && Read<uint8_t>(owner->menu.Get(), 384) != 0
            && Read<uint32_t>(owner->menu.Get(), 380) == owner->items[index].id;
    }
    inline void Entry(void* script, uint32_t callee)
    {
        if (!ready || nativeThread != GetCurrentThreadId() || context || !registered) return;
        auto caller = Read<uint32_t>(script, 200);
        auto pause = caller == Checksum("create_options_menu") && callee == Checksum("roundbar_menu_finish");
        auto title = caller == Checksum("create_gamesettings_menu") && callee == Checksum("finish_themed_sub_menu");
        if (!pause && !title) return;
        Weak existing;
        existing.Find(entryId);
        if (existing.Get()) return;
        auto source = std::make_unique<Host>();
        source->menu.Find(Checksum("current_menu"));
        source->anchor.Find(Checksum("current_menu_anchor"));
        source->window.Find(Checksum("root_window"));
        if (!source->menu.Get() || !source->anchor.Get() || !source->window.Get()
            || Read<uint32_t>(source->menu.Get(), 24) != Checksum("VMenu")
            || Read<uintptr_t>(source->menu.Get()) != 0x64B048 || Modal())
        {
            LogOnce("stock Options root contract rejected");
            return;
        }
        source->serial = Serial();
        source->pause = pause;
        Parameters row;
        row.Name("id", entryId);
        row.Text("text", EscapeNativeText(NativeText(RWGTAMenu::PluginText(FixSettings(), Language(), "menu.plugins")), true));
        // These are the installed roots' own row factories, before their native
        // finish/layout/focus call. Stock options and action handlers stay intact.
        auto issued = Run(pause ? "add_roundbar_menu_item" : "theme_menu_add_item", row);
        existing.Find(entryId);
        if (!issued || !existing.Get() || !source->menu.Get() || !source->anchor.Get())
        {
            LogOnce("native Plugins row creation rejected");
            return;
        }
        host = std::move(source);
    }
    inline char __fastcall ScriptCall(void* script, void*, uint32_t name, void* code, Structure* parameters, void* object, char flag)
    {
        Entry(script, name);
        return callHook.thiscall<char>(script, name, code, parameters, object, flag);
    }
    inline void Cleanup(const std::shared_ptr<Context>& owner, bool restore)
    {
        if (!owner) return;
        if (Modal() && NativeOwned(owner))
        {
            // A covering native modal may be opened by a foreign getter. Keep
            // its underlying controls and logical owner intact; rebuild after
            // focus returns rather than treating suspension as commit failure.
            if (owner->retired) owner->returnPending = true;
            else RWGTAMenu::Model::Changed() = true;
            return;
        }
        Weak current, window;
        current.Find(Checksum("current_menu_anchor"));
        window.Find(Checksum("root_window"));
        auto own = current.Get() && current.Get() == owner->anchor.Get();
        auto gap = owner->constructing && !current.Get();
        // Issue the waiting parent script before retiring the usable owner.
        // A constructing gap is ours only while the exact native window remains
        // current and no foreign anchor/modal has replaced it.
        if (restore && (own || gap) && owner->window.Get() && window.Get() == owner->window.Get() && !Modal())
        {
            Parameters parameters;
            if (!Spawn(owner->pause ? "create_options_menu" : "create_gamesettings_menu", parameters, Checksum("wfp_thug2_return")))
            {
                LogOnce("stock Options asynchronous return rejected");
                owner->returnPending = true;
                return; // Retain context/Back and retry after the next outer task.
            }
            if (gap && !Modal())
            {
                Weak partial, alias;
                partial.Find(privateAnchor);
                alias.Find(Checksum("current_menu_anchor"));
                if (!alias.Get() && partial.Get() && partial.Get() == owner->anchor.Get()) Destroy(privateAnchor);
            }
        }
        Retire(owner);
        if (!restore && own && !Modal()) Destroy(privateAnchor);
        if (context == owner) context.reset();
    }
    inline void Open(uint32_t serial)
    {
        if (context || !host || host->serial != serial || !host->anchor.Get() || !host->menu.Get() || !host->window.Get() || Modal()) return;
        Weak current, window, anchor;
        current.Find(Checksum("current_menu"));
        anchor.Find(Checksum("current_menu_anchor"));
        window.Find(Checksum("root_window"));
        if (current.Get() != host->menu.Get() || anchor.Get() != host->anchor.Get() || window.Get() != host->window.Get()
            || Read<uint8_t>(host->menu.Get(), 384) == 0 || Read<uint32_t>(host->menu.Get(), 380) != entryId
            || !ScriptAvailable(host->pause ? "create_options_menu" : "create_gamesettings_menu")) return;
        auto sourceMenu = host->menu.Get();
        auto sourceAnchor = host->anchor.Get();
        auto sourceWindow = host->window.Get();
        auto owner = std::make_shared<Context>();
        owner->serial = Serial();
        owner->pause = host->pause;
        owner->language = Language();
        owner->model.OpenPlugins(module, owner->language, 5);
        current.Find(Checksum("current_menu"));
        anchor.Find(Checksum("current_menu_anchor"));
        window.Find(Checksum("root_window"));
        if (!host || host->serial != serial || host->menu.Get() != sourceMenu || host->anchor.Get() != sourceAnchor || host->window.Get() != sourceWindow || current.Get() != sourceMenu || anchor.Get() != sourceAnchor || window.Get() != sourceWindow || Modal()
            || Read<uint8_t>(sourceMenu, 384) == 0 || Read<uint32_t>(sourceMenu, 380) != entryId)
        {
            owner->model.Close();
            return;
        }
        owner->anchor.Find(Read<uint32_t>(sourceAnchor, 20));
        owner->menu.Find(Read<uint32_t>(sourceMenu, 20));
        owner->window.Find(Checksum("root_window"));
        context = owner;
        if (!Build(owner))
        {
            LogOnce("private page commit rejected");
            Cleanup(owner, true);
            return;
        }
    }
    inline void Act(const Pending& action)
    {
        if (action.action == Action::Open) { Open(action.serial); return; }
        auto owner = context;
        if (!owner || owner->serial != action.serial) return;
        if (action.action == Action::Back)
        {
            if (owner->retired) { Cleanup(owner, true); return; }
            if (!Current(owner) || Modal() || Read<uint8_t>(owner->menu.Get(), 384) == 0) return;
            if (owner->page != SIZE_MAX)
            {
                owner->page = SIZE_MAX;
                owner->selected = 0;
                if (!Build(owner)) Cleanup(owner, true);
            }
            else Cleanup(owner, true);
            return;
        }
        if (!Current(owner) || Modal() || action.index >= owner->items.size()) return;
        auto item = owner->items[action.index];
        if (!Active(owner, action.index)) return;
        owner->selected = action.index;
        if (action.action == Action::Focus)
        {
            owner->focused = item.id;
            Help(owner, owner->model.RestartRequired() ? owner->model.Text("menu.restartwarning") : item.description);
            return;
        }
        if (!item.enabled) return;
        auto accept = action.action == Action::Accept;
        if (item.kind != Kind::Row && !accept) return;
        if (item.kind == Kind::Back)
        {
            Pending back{ Action::Back, action.serial, 0 };
            Act(back);
            return;
        }
        if (item.kind == Kind::Link)
        {
            owner->page = item.index;
            owner->selected = 0;
            owner->model.PageShown(item.index);
        }
        else if (item.kind == Kind::Previous)
        {
            owner->rootStart = owner->rootStart >= 5 ? owner->rootStart - 5 : 0;
            owner->selected = 0;
        }
        else if (item.kind == Kind::Next)
        {
            owner->rootStart += item.index;
            owner->selected = 0;
        }
        else
        {
            auto rows = owner->model.VisibleRows(owner->page);
            if (!Current(owner) || item.index >= rows.size()) return;
            auto row = *rows[item.index];
            if (!owner->model.Enabled(row, owner->pause) || !Current(owner)) return;
            if (!accept && row.kind != RWGTAMenu::RowKind::Setting && row.kind != RWGTAMenu::RowKind::Preset) return;
            owner->model.Change(row, accept ? 0 : action.action == Action::Left ? -1 : 1, owner->pause);
            if (!Current(owner)) return;
        }
        if (!Build(owner)) Cleanup(owner, true);
    }
    inline int __cdecl Task(void* task)
    {
        if (!nativeThread) nativeThread = GetCurrentThreadId();
        if (nativeThread != GetCurrentThreadId()) return taskHook.call<int>(task);
        ++taskDepth;
        if (ready && !registered && ScriptAvailable("make_new_themed_sub_menu") && ScriptAvailable("make_new_roundbar_menu")
            && ScriptAvailable("finish_themed_sub_menu") && ScriptAvailable("roundbar_menu_finish")
            && ScriptAvailable("theme_menu_add_item") && ScriptAvailable("add_roundbar_menu_item")) registered = true;
        auto result = taskHook.call<int>(task);
        --taskDepth;
        if (!ready || taskDepth || draining) return result;
        draining = true;
        for (size_t n = 0; !pending.empty() && n < 32; ++n)
        {
            auto action = pending.front();
            pending.pop_front();
            Act(action);
        }
        auto owner = context;
        if (owner && Current(owner) && !Modal())
        {
            // The selection moved (native up/down): show the row's description
            auto selected = Read<uint32_t>(owner->menu.Get(), 380);
            for (size_t i = 0; selected != owner->focused && i < owner->items.size(); ++i)
            {
                if (owner->items[i].id == selected) Act({ Action::Focus, owner->serial, i });
            }
        }
        if (owner && owner->returnPending && !Modal()) Cleanup(owner, true);
        else if (owner && !Current(owner) && (!Modal() || !Owned(owner))) Cleanup(owner, false);
        else if (owner && !Modal() && RWGTAMenu::Model::Changed() && !Build(owner)) Cleanup(owner, true);
        draining = false;
        return result;
    }
    inline void Init()
    {
        if (!Contracts()) { LogOnce("native executable contract rejected"); return; }
        if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_PIN | GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, reinterpret_cast<LPCWSTR>(&Init), &module))
        {
            LogOnce("native bridge module pin rejected");
            return;
        }
        // The game keeps calling into this module (hooks, the controller event call); unloading it is unsupported.
        auto pattern = hook::pattern("68 74 EE B2 36 68 74 EE B2 36 51 8B 4C 24 ? E8");
        if (pattern.size() != 1)
        {
            LogOnce("controller event call not found");
            return;
        }
        destructorHook = safetyhook::create_inline(reinterpret_cast<void*>(0x4AABF0), ElementDestroyed);
        callHook = safetyhook::create_inline(reinterpret_cast<void*>(0x4748C0), ScriptCall);
        taskHook = safetyhook::create_inline(reinterpret_cast<void*>(0x54FE10), Task);
        if (!destructorHook || !callHook || !taskHook)
        {
            taskHook.reset(); callHook.reset(); destructorHook.reset();
            LogOnce("native hook transaction rejected");
            return;
        }
        launch = reinterpret_cast<decltype(launch)>(injector::GetBranchDestination(pattern.get_first(15)).as_int());
        injector::MakeCALL(pattern.get_first(15), Launch, true);
        ready = true;
    }
}
