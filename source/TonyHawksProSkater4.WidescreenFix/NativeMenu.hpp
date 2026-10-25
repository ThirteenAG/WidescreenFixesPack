#pragma once
#include "FixSettings.hpp"
#include "NativeMenuBindings.hpp"
#include "../Shared/RWGTA/FixMenu.hpp"
#include <algorithm>
#include <memory>
#include <set>

namespace THPS4Menu
{
    inline SafetyHookInline taskHook, sourceHook, destructorHook, brokerHook;
    inline HMODULE module = nullptr;
    inline bool ready = false, draining = false;
    inline unsigned taskDepth = 0, foreignDepth = 0;
    inline thread_local unsigned nativeDepth = 0;
    struct DepthScope
    {
        unsigned& depth;
        explicit DepthScope(unsigned& value) : depth(value) { ++depth; }
        ~DepthScope() { --depth; }
    };
    struct DrainScope
    {
        bool previous = draining;
        DrainScope() { draining = true; }
        ~DrainScope() { draining = previous; }
    };
    template<class F> auto Foreign(F&& call)
    {
        DepthScope scope(foreignDepth);
        return call();
    }
    inline DWORD nativeThread = 0;
    inline uint32_t nextSerial = 0;
    constexpr uint32_t privateAnchor = Checksum("wfp_thps4_anchor");
    constexpr uint32_t privateMenu = Checksum("wfp_thps4_vmenu");
    constexpr uint32_t entryId = Checksum("wfp_thps4_entry");
    constexpr uint32_t titleId = Checksum("wfp_thps4_title");
    constexpr uint32_t descriptionId = Checksum("wfp_thps4_description");

    inline void LogOnce(const std::string& message)
    {
        static std::set<std::string> seen;
        if (seen.size() >= 32 || !seen.insert(message).second) return;
        OutputDebugStringA(("THPS4 native menu: " + message + "\n").c_str());
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
        switch (Read<uint32_t>(reinterpret_cast<void*>(0xB085E8)))
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
        auto page = Read<uint32_t>(reinterpret_cast<void*>(0xB085E8)) == 9 ? 1251u : 1252u;
        auto bytes = WideCharToMultiByte(page, WC_NO_BEST_FIT_CHARS, wide.c_str(), -1, nullptr, 0, "?", nullptr);
        if (bytes <= 0 || bytes > 4096) return "?";
        std::string result(size_t(bytes), '\0');
        if (!WideCharToMultiByte(page, WC_NO_BEST_FIT_CHARS, wide.c_str(), -1, result.data(), bytes, "?", nullptr)) return "?";
        result.pop_back();
        return result;
    }
    template<class MeasureText> std::string FitLiteral(std::string left, std::string suffix, float width, MeasureText measure)
    {
        if (!std::isfinite(width) || width <= 0.0f) return {};
        Extent measured;
        auto full = left + suffix;
        if (!measure(full, measured)) return {};
        if (measured.width <= width) return full;
        if (!measure("..." + suffix, measured) || measured.width > width) return {};
        size_t low = 0, high = left.size();
        while (low < high)
        {
            auto middle = low + (high - low + 1) / 2;
            if (!measure(left.substr(0, middle) + "..." + suffix, measured)) return {};
            if (measured.width <= width) low = middle;
            else high = middle - 1;
        }
        return left.substr(0, low) + "..." + suffix;
    }
    inline std::string Fit(const std::string& label, const std::string& value, float width)
    {
        return FitLiteral(NativeText(label), value.empty() ? std::string() : ": " + NativeText(value), width, Measure);
    }

    struct Host
    {
        Weak anchor, menu, window, entry;
        uint32_t serial = 0;
        bool pause = false;
    };
    enum class Kind { Link, Previous, Next, Row, Back };
    struct Item { Kind kind; size_t index = 0; bool enabled = true; std::string label, value, description; uint32_t id = 0; std::shared_ptr<Weak> native; };
    struct Context
    {
        RWGTAMenu::Model model;
        Weak anchor, menu, window;
        uint32_t serial = 0;
        bool pause = false, retired = false, rebuilding = false, constructing = false, returnPending = false;
        size_t page = SIZE_MAX, rootStart = 0, rowStart = 0, selected = 0;
        std::string language;
        std::vector<Item> items;
    };
    inline std::unique_ptr<Host> host;
    inline std::shared_ptr<Context> context;
    enum class Action { Open, Accept, Left, Right, Back, Focus, Unfocus };
    struct Pending { Action action; uint32_t serial; size_t index; };
    inline std::deque<Pending> pending;

    inline bool Modal()
    {
        // Test actual native modal anchors; ordinary menu presence is allowed.
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
        return window.Get() == owner->window.Get()
            && Read<void*>(owner->anchor.Get(), 64) == owner->window.Get()
            && Read<void*>(owner->menu.Get(), 64) == owner->anchor.Get();
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
    inline bool Queue(Action action, uint32_t serial, size_t index = 0)
    {
        if (!ready || nativeThread != GetCurrentThreadId() || pending.size() >= 32) return false;
        pending.push_back({ action, serial, index });
        return true;
    }
    inline bool Active(const std::shared_ptr<Context>& owner, size_t index);
    inline char __fastcall ControlEvent(void* object, void*, Event* event);
    struct ControlTable
    {
        uintptr_t original;
        // Keep all nine native slots and the exact preceding original word.
        // Storage remains process-long even after a logical Context retires.
        std::array<uintptr_t, 10> words{};
        uintptr_t Pointer() const { return reinterpret_cast<uintptr_t>(words.data() + 1); }
        bool Prepare()
        {
            if (!Readable(reinterpret_cast<void*>(original - 4), sizeof(words))) return false;
            std::memcpy(words.data(), reinterpret_cast<void*>(original - 4), sizeof(words));
            words[5] = reinterpret_cast<uintptr_t>(&ControlEvent); // vfptr+16
            return true;
        }
        bool Bind(void* object) const
        {
            if (!Readable(object, 28) || Read<uintptr_t>(object) != original) return false;
            *reinterpret_cast<uintptr_t*>(object) = Pointer();
            return true;
        }
        char Delegate(void* object, Event* event) const
        {
            return reinterpret_cast<char (__thiscall*)(void*, Event*)>(Read<uintptr_t>(reinterpret_cast<void*>(original), 16))(object, event);
        }
    };
    inline ControlTable textTable{ 0x58AA20 }, containerTable{ 0x58A918 }, menuTable{ 0x58A320 };

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
        DepthScope native(nativeDepth);
        auto owner = context;
        if (owner && (object == owner->window.Get() || (!owner->rebuilding && (object == owner->anchor.Get() || object == owner->menu.Get())))) Retire(owner);
        destructorHook.thiscall<void>(object);
    }
    inline bool Caption(uint32_t id, const std::string& text, float x, float y, float scale)
    {
        auto native = Fit(text, "", 440 / scale);
        Extent extent;
        if ((!text.empty() && native.empty()) || !Measure(native, extent)) return false;
        auto xScale = extent.width * scale > 440 && extent.width > 0 ? 440 / extent.width : scale;
        Parameters parameters;
        parameters.Name("parent", privateAnchor);
        parameters.Name("font", "small");
        parameters.Text("text", EscapeNativeText(native, true));
        parameters.XY("pos", x, y);
        parameters.XY("scale", xScale, scale);
        parameters.Flag("not_focusable");
        return Create(Checksum("TextElement"), id, parameters);
    }
    inline bool Build(const std::shared_ptr<Context>& owner)
    {
        if (!Current(owner)) return false;
        RWGTAMenu::Model::Changed().exchange(false);
        std::vector<Item> items;
        std::string heading = owner->model.Text("menu.plugins");
        if (owner->page == SIZE_MAX)
        {
            auto count = std::min(size_t(5), owner->model.Pages().size() - std::min(owner->rootStart, owner->model.Pages().size()));
            for (size_t i = 0; i < count; ++i) items.push_back({ Kind::Link, owner->rootStart + i, true, owner->model.Pages()[owner->rootStart + i].title });
            if (owner->rootStart) items.push_back({ Kind::Previous, 0, true, owner->model.Text("menu.previous") });
            if (owner->rootStart + count < owner->model.Pages().size()) items.push_back({ Kind::Next, count, true, owner->model.Text("menu.next") });
        }
        else
        {
            if (owner->page >= owner->model.Pages().size()) return false;
            heading = owner->model.Pages()[owner->page].title;
            auto rows = Foreign([&] { return owner->model.VisibleRows(owner->page); });
            if (!Current(owner)) return false;
            auto end = std::min(rows.size(), owner->rowStart + 5);
            for (size_t i = owner->rowStart; i < end; ++i)
            {
                auto row = *rows[i];
                auto enabled = Foreign([&] { return owner->model.Enabled(row, owner->pause); });
                if (!Current(owner)) return false;
                auto label = owner->model.Label(row);
                auto value = enabled ? Foreign([&] { return owner->model.Value(row); }) : owner->model.Text("menu.unavailable");
                if (!Current(owner)) return false;
                auto description = owner->model.Help(row);
                items.push_back({ Kind::Row, i, enabled, std::move(label), std::move(value), std::move(description) });
            }
            if (owner->rowStart) items.push_back({ Kind::Previous, 0, true, owner->model.Text("menu.previous") });
            if (end < rows.size()) items.push_back({ Kind::Next, 5, true, owner->model.Text("menu.next") });
        }
        items.push_back({ Kind::Back, 0, true, owner->model.Text("menu.back") });
        if (!Current(owner) || items.empty() || items.size() > 8) return false;
        if (owner->model.RestartRequired())
            for (auto& item : items) item.description = owner->model.Text("menu.restartwarning") + "  " + item.description;
        owner->serial = Serial(); // Row-pool generation rejects queued old-page input.
        owner->rebuilding = true;
        owner->constructing = true;
        auto oldAnchor = Read<uint32_t>(owner->anchor.Get(), 20);
        Destroy(oldAnchor);
        if (context != owner || owner->retired || !owner->window.Get() || Modal())
        {
            owner->rebuilding = false;
            return false;
        }
        Weak replacement;
        replacement.Find(Checksum("current_menu_anchor"));
        if (replacement.Get())
        {
            owner->rebuilding = false;
            return false;
        }
        for (auto id : { privateAnchor, privateMenu, titleId, descriptionId })
        {
            Weak occupied;
            occupied.Find(id);
            if (occupied.Get()) return false;
        }
        Parameters root;
        root.Name("menu_id", privateAnchor);
        root.Name("vmenu_id", privateMenu);
        root.Name("parent", "root_window");
        root.XY("pos", 120, 108);
        root.XY("dims", 440, 280);
        root.Text("menu_title", "");
        auto issued = Run("make_new_menu", root);
        owner->anchor.Find(privateAnchor);
        owner->menu.Find(privateMenu);
        owner->rebuilding = false;
        if (!issued || !Current(owner) || Read<uint32_t>(owner->menu.Get(), 24) != Checksum("VMenu") || Read<uintptr_t>(owner->menu.Get()) != 0x58A320) return false;
        // Install escape before population; a later font/row failure can still
        // return through the native parent even with only a partial page.
        if (!containerTable.Bind(owner->anchor.Get()) || !menuTable.Bind(owner->menu.Get())) return false;
        owner->items = std::move(items);
        owner->selected = std::min(owner->selected, owner->items.size() - 1);
        for (size_t i = 0; i < owner->items.size(); ++i)
        {
            auto& item = owner->items[i];
            auto idText = "wfp_thps4_row" + std::to_string(i);
            item.id = Checksum(idText.c_str());
            auto fitted = Fit(item.label, item.value, 440);
            Extent extent;
            if (fitted.empty() || !Measure(fitted, extent)) { LogOnce("complete native row value cannot fit"); return false; }
            auto xScale = extent.width > 440 ? 440 / extent.width : 1.0f;
            Weak occupied;
            occupied.Find(item.id);
            if (occupied.Get()) return false;
            Parameters row;
            row.Name("id", item.id);
            row.Name("parent_menu_id", privateMenu);
            row.Name("focus_script", "nullscript");
            row.Name("unfocus_script", "nullscript");
            row.Name("pad_choose_script", "nullscript");
            row.Text("text", EscapeNativeText(fitted, true));
            row.Flag("no_choose_sound");
            auto issued = Run("make_text_sub_menu_item", row);
            Weak created;
            created.Find(item.id);
            if (!issued || !created.Get() || Read<uintptr_t>(created.Get()) != 0x58AA20 || Read<uint32_t>(created.Get(), 24) != Checksum("TextElement")
                || Read<void*>(created.Get(), 64) != owner->menu.Get())
            {
                if (created.Get() && Read<uint32_t>(created.Get(), 20) == item.id) Destroy(item.id);
                return false;
            }
            if (!Current(owner)) return false;
            Parameters scale;
            scale.Name("id", item.id);
            scale.XY("scale", xScale, 1.0f);
            if (!Properties(scale)) return false;
            item.native = std::make_shared<Weak>();
            item.native->Find(item.id);
            if (!textTable.Bind(item.native->Get())) return false;
            Color(item.id, i == owner->selected, item.enabled);
        }
        if (!Caption(titleId, heading, 120, 78, 1.2f) || !Current(owner)
            || !Caption(descriptionId, owner->items[owner->selected].description, 120, 392, 0.8f) || !Current(owner)) return false;
        Parameters focus;
        focus.Name("child_id", owner->items[owner->selected].id);
        if (!Fire("focus", privateMenu, &focus.value) || !Current(owner)) return false;
        owner->constructing = false;
        return true;
    }

    inline bool Active(const std::shared_ptr<Context>& owner, size_t index)
    {
        return Current(owner) && !Modal() && index < owner->items.size()
            && owner->items[index].native && owner->items[index].native->Get()
            && Read<uint32_t>(owner->items[index].native->Get(), 20) == owner->items[index].id
            && Read<void*>(owner->items[index].native->Get(), 64) == owner->menu.Get()
            && Read<uint8_t>(owner->menu.Get(), 332) != 0
            && Read<uint32_t>(owner->menu.Get(), 328) == owner->items[index].id;
    }
    inline bool QueueBack(const std::shared_ptr<Context>& owner)
    {
        if (!Current(owner) || Read<uint8_t>(owner->menu.Get(), 332) == 0) return false;
        auto selected = Read<uint32_t>(owner->menu.Get(), 328);
        auto alive = std::any_of(owner->items.begin(), owner->items.end(), [&](const Item& item) {
            return item.id == selected && item.native && item.native->Get()
                && Read<uint32_t>(item.native->Get(), 20) == selected
                && Read<void*>(item.native->Get(), 64) == owner->menu.Get();
        });
        if (!alive || !Queue(Action::Back, owner->serial)) return false;
        if (foreignDepth)
        {
            // Accepted Back must abort an in-flight getter/preset API burst.
            // Only logical cancellation happens here: no native UI deletion,
            // factory, restoration or vtable mutation until the outer task.
            owner->returnPending = true;
            Retire(owner);
        }
        return true;
    }
    inline char __fastcall ControlEvent(void* object, void*, Event* event)
    {
        auto pointer = Read<uintptr_t>(object);
        const ControlTable* table = pointer == textTable.Pointer() ? &textTable
            : pointer == containerTable.Pointer() ? &containerTable
            : pointer == menuTable.Pointer() ? &menuTable : nullptr;
        if (!table) return 1; // Native destruction already reset its vtable.
        if (!ready || nativeThread != GetCurrentThreadId() || !Readable(event, sizeof(Event))) return table->Delegate(object, event);
        Weak lifetime;
        lifetime.Find(Read<uint32_t>(object, 20));
        auto handled = [&]() { event->flags |= 2; return char(lifetime.Get() == object); };
        if (host && object == host->entry.Get()
            && (event->type == Checksum("pad_choose") || event->type == Checksum("pad_start")))
        {
            return Queue(Action::Open, host->serial) ? handled() : table->Delegate(object, event);
        }
        auto owner = context;
        if (!Current(owner) || owner->rebuilding || owner->constructing) return table->Delegate(object, event);
        if (object == owner->anchor.Get() || object == owner->menu.Get())
        {
            if (event->type == Checksum("pad_back") && Read<uint8_t>(owner->menu.Get(), 332))
            {
                return QueueBack(owner) ? handled() : table->Delegate(object, event);
            }
            return table->Delegate(object, event); // Original VMenu navigation/focus.
        }
        for (size_t i = 0; i < owner->items.size(); ++i)
        {
            auto& item = owner->items[i];
            if (!item.native || item.native->Get() != object) continue;
            Action action;
            if (event->type == Checksum("unfocus")) action = Action::Unfocus;
            else
            {
                if (!Active(owner, i)) return table->Delegate(object, event);
                if (event->type == Checksum("focus")) action = Action::Focus;
                else if (event->type == Checksum("pad_choose") || event->type == Checksum("pad_start")) action = Action::Accept;
                else if (event->type == Checksum("pad_left")) action = Action::Left;
                else if (event->type == Checksum("pad_right")) action = Action::Right;
                else if (event->type == Checksum("pad_back")) action = Action::Back;
                else return table->Delegate(object, event);
            }
            if (action == Action::Accept && item.kind == Kind::Back) action = Action::Back;
            if (!(action == Action::Back ? QueueBack(owner) : Queue(action, owner->serial, i))) return table->Delegate(object, event);
            // Never pass handled owned actions to423830's native script handler.
            // Only primitives are queued; accepted Back during a foreign API
            // may logically cancel Model. Native UI mutation remains deferred.
            return handled();
        }
        return table->Delegate(object, event);
    }
    inline void Entry(Structure* parameters)
    {
        if (!ready || nativeThread != GetCurrentThreadId() || context) return;
        uint32_t id = 0;
        if (!Name(parameters, "id", id)) return;
        const bool optionsCall = id == Checksum("ss_setup_options_menu") || id == Checksum("options_menu");
        uint32_t scriptName = 0;
        if (!Name(parameters, "", scriptName) || scriptName != Checksum("animate_in"))
        {
            if (optionsCall) LogOnce("Options dispatch has no inline animate_in checksum");
            return;
        }
        Weak currentAnchor;
        currentAnchor.Find(Checksum("current_menu_anchor"));
        if (!currentAnchor.Get()) { if (optionsCall) LogOnce("Options current anchor unavailable"); return; }
        auto sourceId = Read<uint32_t>(currentAnchor.Get(), 20);
        auto pause = sourceId == Checksum("options_menu");
        auto title = sourceId == Checksum("ss_setup_options_menu");
        if ((!pause && !title) || (id != sourceId && id != Checksum("current_menu_anchor")))
        { if (optionsCall) LogOnce("Options animate_in anchor identity unavailable"); return; }
        if (!ScriptAvailable("make_new_menu") || !ScriptAvailable("make_text_sub_menu_item")
            || !ScriptAvailable("nullscript") || !ScriptAvailable(title ? "main_menu_add_item" : "make_text_sub_menu_item"))
        { LogOnce("Options row helper scripts unavailable"); return; }
        Weak existing;
        existing.Find(entryId);
        if (existing.Get()) return;
        auto source = std::make_unique<Host>();
        source->menu.Find(title ? Checksum("ss_setup_options_vmenu") : Checksum("options_vmenu"));
        source->anchor.Find(sourceId);
        source->window.Find(Checksum("root_window"));
        if (!source->menu.Get() || !source->anchor.Get() || !source->window.Get()
            || Read<uint32_t>(source->menu.Get(), 24) != Checksum("VMenu")
            || Read<uintptr_t>(source->menu.Get()) != 0x58A320 || Modal())
        { LogOnce("Options native menu/window/type authority unavailable"); return; }
        Weak alias;
        alias.Find(Checksum("current_menu"));
        if (alias.Get() != source->menu.Get() || currentAnchor.Get() != source->anchor.Get())
        { LogOnce("Options current menu alias unavailable"); return; }
        auto sourceAnchor = source->anchor.Get();
        auto sourceMenu = source->menu.Get();
        auto sourceWindow = source->window.Get();
        source->serial = Serial();
        source->pause = pause;
        Parameters row;
        row.Name("id", entryId);
        auto label = Fit(RWGTAMenu::PluginText(FixSettings(), Language(), "menu.plugins"), "", title ? 190.0f : 440.0f);
        Extent extent;
        if (!Measure(label, extent)) { LogOnce("Options entry font measurement unavailable"); return; }
        row.Text("text", EscapeNativeText(label, true));
        auto available = title ? 190.0f : 440.0f;
        row.XY("scale", extent.width > available ? available / extent.width : 1.0f, 1.0f);
        row.Name("pad_choose_script", "nullscript");
        row.Flag("no_sound");
        if (!title) row.Name("parent_menu_id", Read<uint32_t>(sourceMenu, 20));
        auto issued = Run(title ? "main_menu_add_item" : "make_text_sub_menu_item", row);
        source->entry.Find(entryId);
        alias.Find(Checksum("current_menu"));
        currentAnchor.Find(Checksum("current_menu_anchor"));
        Weak window;
        window.Find(Checksum("root_window"));
        auto sameSource = source->anchor.Get() == sourceAnchor && source->menu.Get() == sourceMenu
            && source->window.Get() == sourceWindow && currentAnchor.Get() == sourceAnchor
            && alias.Get() == sourceMenu && window.Get() == sourceWindow && !Modal()
            && Read<void*>(sourceAnchor, 64) == sourceWindow && Read<void*>(sourceMenu, 64) == sourceAnchor;
        auto correctEntry = source->entry.Get() && Read<uint32_t>(source->entry.Get(), 20) == entryId
            && Read<uint32_t>(source->entry.Get(), 24) == Checksum(title ? "ContainerElement" : "TextElement")
            && Read<void*>(source->entry.Get(), 64) == sourceMenu;
        if (!issued || !sameSource || !correctEntry || !((title ? containerTable : textTable).Bind(source->entry.Get())))
        {
            // The id was absent before this synchronous helper. Remove only
            // its exact observed result; never leave a visible inert category.
            if (source->entry.Get() && Read<uint32_t>(source->entry.Get(), 20) == entryId) Destroy(entryId);
            LogOnce("native Advanced Options entry rejected");
            return;
        }
        host = std::move(source);
    }
    inline char __cdecl Source(Structure* parameters, void* script)
    {
        // Keep complete wrapper/helper dispatch above a reentrant Task alive.
        DepthScope native(nativeDepth);
        // Exact source roots' final animate-in call: append before original.
        Entry(parameters);
        return sourceHook.call<char>(parameters, script);
    }
    inline char __fastcall Broker(void* manager, void*, uint32_t type, uint32_t target, uint32_t source, Structure* data)
    {
        //427C10 continues waiting-script notifications after target virtual16.
        // A reentrant503960 must not drain/delete while that traversal resumes.
        DepthScope native(nativeDepth);
        return brokerHook.thiscall<char>(manager, type, target, source, data);
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
        bool scheduled = false;
        // Issue the waiting parent script before retiring the usable owner.
        // A constructing gap is ours only while the exact native window remains
        // current and no foreign anchor/modal has replaced it.
        if (restore && (own || gap) && owner->window.Get() && window.Get() == owner->window.Get() && !Modal())
        {
            Parameters parameters;
            if (!Spawn(owner->pause ? "create_options_menu" : "create_setup_options_menu", parameters, Checksum("wfp_thps4_return")))
            {
                LogOnce("stock Options asynchronous return rejected");
                owner->returnPending = true;
                return; // Retain context/Back and retry after the next outer task.
            }
            scheduled = true;
            if (gap && !Modal())
            {
                Weak partial, alias;
                partial.Find(privateAnchor);
                alias.Find(Checksum("current_menu_anchor"));
                if (!alias.Get() && partial.Get() && partial.Get() == owner->anchor.Get()) Destroy(privateAnchor);
            }
        }
        Retire(owner);
        if (!scheduled && owner->anchor.Get() && !Modal())
        {
            Weak privateRoot;
            privateRoot.Find(privateAnchor);
            if (privateRoot.Get() == owner->anchor.Get()) Destroy(privateAnchor);
        }
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
            || Read<uint8_t>(host->menu.Get(), 332) == 0 || Read<uint32_t>(host->menu.Get(), 328) != entryId
            || !ScriptAvailable(host->pause ? "create_options_menu" : "create_setup_options_menu")) return;
        auto sourceMenu = host->menu.Get();
        auto sourceAnchor = host->anchor.Get();
        auto sourceWindow = host->window.Get();
        auto owner = std::make_shared<Context>();
        owner->serial = Serial();
        owner->pause = host->pause;
        owner->language = Language();
        owner->model.OpenPlugins(module, owner->language, 4096);
        if (!owner->model.IsOpen()) return; // Discovery may logically cancel.
        current.Find(Checksum("current_menu"));
        anchor.Find(Checksum("current_menu_anchor"));
        window.Find(Checksum("root_window"));
        if (!host || host->serial != serial || host->menu.Get() != sourceMenu || host->anchor.Get() != sourceAnchor || host->window.Get() != sourceWindow || current.Get() != sourceMenu || anchor.Get() != sourceAnchor || window.Get() != sourceWindow || Modal()
            || Read<uint8_t>(sourceMenu, 332) == 0 || Read<uint32_t>(sourceMenu, 328) != entryId)
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
            if (!Current(owner) || Modal() || Read<uint8_t>(owner->menu.Get(), 332) == 0) return;
            if (owner->page != SIZE_MAX)
            {
                owner->model.PageShown(owner->page);
                owner->page = SIZE_MAX;
                owner->rowStart = 0;
                owner->selected = 0;
                if (!Build(owner)) Cleanup(owner, true);
            }
            else Cleanup(owner, true);
            return;
        }
        if (!Current(owner) || Modal() || action.index >= owner->items.size()) return;
        auto item = owner->items[action.index];
        if (!item.native || !item.native->Get() || Read<uint32_t>(item.native->Get(), 20) != item.id
            || Read<void*>(item.native->Get(), 64) != owner->menu.Get()) return;
        if (action.action == Action::Unfocus) { Color(item.id, false, item.enabled); return; }
        if (!Active(owner, action.index)) return;
        owner->selected = action.index;
        if (action.action == Action::Focus)
        {
            Color(item.id, true, item.enabled);
            Parameters description;
            description.Name("id", descriptionId);
            auto fitted = Fit(item.description, "", 550);
            Extent extent;
            if (!Measure(fitted, extent)) return;
            description.Text("text", EscapeNativeText(fitted, true));
            description.XY("scale", extent.width > 550 ? 440 / extent.width : 0.8f, 0.8f);
            Properties(description);
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
            owner->rowStart = 0;
            owner->selected = 0;
            owner->model.PageShown(item.index);
        }
        else if (item.kind == Kind::Previous)
        {
            if (owner->page == SIZE_MAX) owner->rootStart = owner->rootStart >= 5 ? owner->rootStart - 5 : 0;
            else owner->rowStart = owner->rowStart >= 5 ? owner->rowStart - 5 : 0;
            owner->selected = 0;
        }
        else if (item.kind == Kind::Next)
        {
            if (owner->page == SIZE_MAX) owner->rootStart += item.index;
            else owner->rowStart += item.index;
            owner->selected = 0;
        }
        else
        {
            auto rows = Foreign([&] { return owner->model.VisibleRows(owner->page); });
            if (!Active(owner, action.index) || item.index >= rows.size()
                || !item.native || item.native->Get() != owner->items[action.index].native->Get()) return;
            auto row = *rows[item.index];
            if (!Foreign([&] { return owner->model.Enabled(row, owner->pause); }) || !Active(owner, action.index)
                || !item.native || item.native->Get() != owner->items[action.index].native->Get()) return;
            if (!accept && row.kind != RWGTAMenu::RowKind::Setting && row.kind != RWGTAMenu::RowKind::Preset) return;
            Foreign([&] { return owner->model.Change(row, accept ? 0 : action.action == Action::Left ? -1 : 1, owner->pause); });
            if (!Current(owner)) return;
        }
        if (!Build(owner)) Cleanup(owner, true);
    }
    inline int __cdecl Task(void* task)
    {
        if (!nativeThread) nativeThread = GetCurrentThreadId();
        if (nativeThread != GetCurrentThreadId()) return taskHook.call<int>(task);
        int result;
        { DepthScope depth(taskDepth); result = taskHook.call<int>(task); }
        if (!ready || taskDepth || nativeDepth || draining) return result;
        DrainScope drain;
        for (size_t n = 0; !pending.empty() && n < 32; ++n)
        {
            auto action = pending.front();
            pending.pop_front();
            Act(action);
        }
        auto owner = context;
        if (owner && owner->returnPending && !Modal()) Cleanup(owner, true);
        else if (owner && !Current(owner) && (!Modal() || !Owned(owner))) Cleanup(owner, false);
        else if (owner && !Modal() && RWGTAMenu::Model::Changed() && !Build(owner)) Cleanup(owner, true);
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
        // The native controls retain event-vtable pointers. Pin before any
        // publication; dynamic host DLL unload is unsupported.
        if (!textTable.Prepare() || !containerTable.Prepare() || !menuTable.Prepare()) { LogOnce("native vtable contracts rejected"); return; }
        destructorHook = safetyhook::create_inline(reinterpret_cast<void*>(0x471800), ElementDestroyed);
        sourceHook = safetyhook::create_inline(reinterpret_cast<void*>(0x475C40), Source);
        brokerHook = safetyhook::create_inline(reinterpret_cast<void*>(0x427C10), Broker);
        taskHook = safetyhook::create_inline(reinterpret_cast<void*>(0x503960), Task);
        if (!destructorHook || !sourceHook || !brokerHook || !taskHook)
        {
            taskHook.reset(); brokerHook.reset(); sourceHook.reset(); destructorHook.reset();
            LogOnce("native hook transaction rejected");
            return;
        }
        ready = true;
    }
}
