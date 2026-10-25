#pragma once
#include <algorithm>
#include <cmath>
#include <deque>
#include <memory>
#include <string>
#include <vector>
#include "NativeMenuContext.hpp"
#include "NativeMenuUnwind.hpp"
#include "NativeMenuText.hpp"
#include "../Shared/RWGTA/FixMenu.hpp"

namespace THPS3Menu
{
    inline constexpr uint32_t ShowEvent = uint32_t(-839833858);
    inline constexpr uint32_t PrepareEvent = 1234828114;
    inline constexpr uint32_t BackEvent = uint32_t(-1842266424);
    inline constexpr uint32_t ChooseEvent = 1880869774;
    inline constexpr uint32_t PressEvent = 281135239;
    inline constexpr uint32_t HandledEvent = 316323547;
    inline constexpr size_t RootPage = SIZE_MAX;
    inline constexpr size_t PageRows = 6;
    inline constexpr uint32_t HelpID = Checksum("wfp_thps3_help");
    enum class ItemKind { Page, Previous, Next, Back, Value };
    struct Item
    {
        ItemKind kind = ItemKind::Back;
        size_t page = 0;
        RWGTAMenu::Row row;
        uint32_t id = 0;
        void* widget = nullptr;
        std::string label;
        bool enabled = true;
        std::string suffix;
    };
    struct Pending
    {
        Command command;
        uint64_t generation;
        size_t page, start, rowStart, row;
        uint32_t selectedID;
        void* selectedWidget;
    };
    struct Created { uint32_t id, table; void* node; void* parent; };
    struct Host
    {
        Context context;
        void* root = nullptr;
        void* entry = nullptr;
        void* expectedDestroy = nullptr;
        RWGTAMenu::Model model;
        uint64_t generation = 1;
        size_t page = RootPage, start = 0, rowStart = 0;
        bool open = false, busy = false, invalid = false, modelOpening = false, nativeBackPending = false, recovering = false;
        bool baseRetiredWhilePrivate = false;
        uint32_t foreignDepth = 0;
        uint64_t enterGeneration = 0;
        std::vector<Item> items;
        std::vector<uint32_t> leaves;
        std::vector<Created> created;
        std::vector<std::unique_ptr<Controller>> controllers;
        std::deque<Pending> pending;
    };
    inline Host host;
    inline HMODULE module = nullptr;
    inline bool installed = false;

    inline std::string Language()
    {
        auto chosen = FixSettings().String("MAIN.TextLanguage");
        if (!chosen.empty() && chosen != "auto") return chosen;
        // This installed script set has no proved native locale getter.
        return "en";
    }

    inline void Log(const char* message)
    {
        std::string line = "THPS3 native menu: ";
        line += message;
        line += '\n';
        OutputDebugStringA(line.c_str());
    }

    inline void Diagnostic(const char* reason)
    {
        static std::set<std::string> seen;
        if (reason && seen.size() < 32 && seen.insert(reason).second) Log(reason);
    }

    struct Busy
    {
        bool previous;
        Busy() : previous(host.busy) { host.busy = true; }
        ~Busy() { host.busy = previous; }
    };
    struct ForeignScope
    {
        ForeignScope() { ++host.foreignDepth; }
        ~ForeignScope() { --host.foreignDepth; }
    };
    struct Snapshot
    {
        uint64_t generation = host.generation;
        bool open = host.open;
        void* root = host.root;
        bool Current(bool shown = true) const
        {
            return generation == host.generation && open == host.open && root == host.root && !host.invalid
                && host.context.PrivateCurrent(root, shown);
        }
    };
    inline bool RecoveryOwner()
    {
        const auto& context = host.context;
        return host.root && Manager() == context.manager && Readable(context.frontend, 76)
            && Read<void*>(reinterpret_cast<void*>(0x8E1FB0)) == context.frontend
            && !Read<uint32_t>(context.frontend, 60) && Read<void*>(context.frontend, 68) == context.screen
            && Read<uint32_t>(context.frontend, 64) == context.screenID && ScreenObject(context.screen, context.screenID)
            && Read<uint8_t>(context.screen, 4) && Lookup(context.manager, ParentID) == context.parent
            && Object(context.parent, 0x58EF00, 704) && Read<uint8_t>(context.parent, 92)
            && Read<uint8_t>(context.parent, 93) && Read<uint8_t>(context.parent, 94)
            && !Read<uint8_t>(context.parent, 700) && Read<uint32_t>(context.parent, 424) == RootID
            && Lookup(context.manager, RootID) == host.root && Object(host.root, 0x58F11C, 448)
            && Read<void*>(host.root, 4) == context.parent
            && Read<uint8_t>(host.root, 92) && Read<uint8_t>(host.root, 93);
    }
    inline bool RecoveryCurrent() { return RecoveryOwner() && SelectedChild(host.context.parent) == host.root; }
    inline void RepairRecoveryIndex()
    {
        if (!host.baseRetiredWhilePrivate || !RecoveryOwner() || RecoveryCurrent()) return;
        int32_t index = -1;
        auto* child = reinterpret_cast<void*(__thiscall*)(void*, uint32_t, int32_t*)>(0x4CD6C0)(host.context.parent, RootID, &index);
        if (child != host.root || index < 0 || index > 1024 || !RecoveryOwner()) return;
        // Removing the earlier base leaves the parent's cached child index stale; restore it from the ID lookup.
        *reinterpret_cast<int32_t*>(reinterpret_cast<uint8_t*>(host.context.parent) + 420) = index;
    }

    template<class T> void Write(void* object, size_t offset, T value)
    {
        *reinterpret_cast<T*>(reinterpret_cast<uint8_t*>(object) + offset) = value;
    }
    inline void Emit(uint32_t type, uint32_t id)
    {
        Parameters params;
        Event event{ type, id, &params.value };
        reinterpret_cast<void(__thiscall*)(void*, Event*)>(0x4D2430)(host.context.manager, &event);
    }
    inline bool Queue(Command command, size_t row = 0, bool beforeShow = false)
    {
        if (host.busy || host.invalid || !host.context.PrivateCurrent(host.root, !beforeShow)) return false;
        if (command != Command::Open && !host.open) return false;
        if (command == Command::Back && host.foreignDepth)
        {
            host.model.Close();
            host.open = false;
            ++host.generation;
            host.pending.clear();
            host.nativeBackPending = true;
            return true;
        }
        if (host.pending.size() >= 32) return false;
        auto selected = Read<uint32_t>(host.root, 420);
        auto* widget = (row < host.items.size()) ? host.items[row].widget : nullptr;
        host.pending.push_back({ command, host.generation, host.page, host.start, host.rowStart, row, selected, widget });
        return true;
    }
    inline void* __fastcall ControllerDestroyed(Controller* value, void*, uint32_t)
    {
        if (!value) return nullptr;
        // This node is DLL-owned. Native deleting flags do not transfer its
        // allocator ownership. Keep both embedded links alive until unwind.
        if (!value->retired && value->native.owner && value->native.owner != host.expectedDestroy
            && value->generation == host.generation)
        {
            ++host.generation;
            host.invalid = true;
            if (!host.modelOpening) host.model.Close();
            host.open = false;
        }
        value->retired = true;
        value->native.owner = nullptr;
        return value;
    }
    inline void __fastcall ControllerInitialized(Controller* value, void*)
    {
        const uint32_t rootEvents[] = { ShowEvent, PrepareEvent, BackEvent, PressEvent };
        const uint32_t rowEvents[] = { ChooseEvent };
        auto subscribe = [&](uint32_t type)
        {
            reinterpret_cast<uint8_t(__thiscall*)(void*, uint32_t, uint8_t)>(0x4CD570)(value->native.owner, type, 1);
            if (!reinterpret_cast<uint8_t(__thiscall*)(void*, uint32_t)>(0x4CD5E0)(value->native.owner, type))
                host.invalid = true;
        };
        if (value->command == Command::Open)
            for (auto event : rootEvents) subscribe(event);
        else
            for (auto event : rowEvents) subscribe(event);
    }
    inline uint8_t __fastcall ControllerExecuted(Controller* value, void*, Event* event)
    {
        if (!value || value->retired || value->generation != host.generation || !Readable(event, sizeof(*event))) return 0;
        // Native widgets accept their own target or broadcast1. A subscribed
        // controller sees foreign-target events too, so retain that same scope
        // before any private root routing or event consumption.
        if (value->command == Command::Open && event->target != RootID && event->target != 1) return 0;
        if (value->command == Command::Enter)
        {
            if (!host.busy && !host.invalid && event->type == ChooseEvent && (event->target == EntryID || event->target == 1)
                && value->native.owner == host.entry && host.context.StockCurrent()
                && Lookup(host.context.manager, EntryID) == host.entry
                && Read<void*>(host.entry, 4) == host.context.base
                && Read<uint32_t>(host.context.base, 420) == EntryID
                && Read<uint8_t>(host.entry, 92) && Read<uint8_t>(host.entry, 93) && Read<uint8_t>(host.entry, 94))
                host.enterGeneration = host.generation;
            return 0;
        }
        if (host.recovering)
        {
            // Model ownership was lost, but this exact native page remains
            // current. Only its existing Back control can request the ordinary
            // native history escape; no old setting/preset command survives.
            if (value->command == Command::Accept && value->row < host.items.size()
                && host.items[value->row].kind == ItemKind::Back && event->type == ChooseEvent
                && (event->target == host.items[value->row].id || event->target == 1) && host.items[value->row].widget == value->native.owner
                && Read<uint32_t>(host.root, 420) == host.items[value->row].id && RecoveryCurrent())
                host.nativeBackPending = true;
            return 0; // Physical Back goes through the original native route.
        }
        if (value->command == Command::Open)
        {
            if (event->type == ShowEvent || event->type == PrepareEvent)
                Queue(Command::Open, 0, true);
            else if (event->type == BackEvent && Queue(Command::Back))
                Write(event, 0, HandledEvent);
            else if (event->type == PressEvent && host.open && event->params)
            {
                // A bounded borrowed integer lookup; no native allocation,
                // model calls or borrowed argument retention in this callback.
                auto* component = Read<Component*>(event->params);
                for (size_t count = 0; component && count < 64; ++count)
                {
                    if (!Readable(component, sizeof(*component))) break;
                    if (component->type == 1 && component->name == Checksum("press"))
                    {
                        auto selected = Read<uint32_t>(host.root, 420);
                        for (size_t row = 0; row < host.items.size(); ++row)
                            if (host.items[row].id == selected && component->value < 2)
                                if (Queue(component->value ? Command::Right : Command::Left, row))
                                    Write(event, 0, HandledEvent);
                        break;
                    }
                    component = component->next;
                }
            }
        }
        else if (event->type == ChooseEvent && (event->target == Read<uint32_t>(value->native.owner, 52) || event->target == 1)
            && Read<uint32_t>(host.root, 420) == Read<uint32_t>(value->native.owner, 52) && Read<uint8_t>(value->native.owner, 92)
            && Read<uint8_t>(value->native.owner, 93) && Read<uint8_t>(value->native.owner, 94))
            Queue(Command::Accept, value->row);
        return 0;
    }
    inline ControllerTable controllerTable{ ControllerDestroyed, ControllerInitialized, ControllerExecuted };

    inline void Attach(void* owner, Command command, size_t row = 0)
    {
        auto value = std::make_unique<Controller>();
        value->generation = host.generation;
        value->row = uint32_t(row);
        value->command = command;
        InitializeController(*value, controllerTable);
        auto* pointer = value.get();
        host.controllers.push_back(std::move(value));
        AttachController(owner, *pointer);
    }
    inline void Retire(void* node)
    {
        captions.erase(node);
        if (node == host.context.base && Manager() == host.context.manager
            && Lookup(host.context.manager, ParentID) == host.context.parent && Object(host.context.parent, 0x58EF00, 704)
            && Lookup(host.context.manager, RootID) == host.root && Object(host.root, 0x58F11C, 448)
            && Read<void*>(host.root, 4) == host.context.parent && SelectedChild(host.context.parent) == host.root
            && Read<uint32_t>(host.context.parent, 424) == RootID)
            host.baseRetiredWhilePrivate = true;
        for (auto& value : host.controllers)
        {
            if (value->native.owner != node) continue;
            value->retired = true;
            value->native.owner = nullptr;
        }
        if (node != host.expectedDestroy && (node == host.root || node == host.entry || node == host.context.base
            || node == host.context.parent || node == host.context.screen
            || std::any_of(host.items.begin(), host.items.end(), [&](const Item& item) { return item.widget == node; })))
        {
            ++host.generation;
            host.invalid = true;
            // Abort an in-flight preset burst immediately. Native node/list
            // storage stays deferred, but the shared model's generation must
            // change before another foreign plugin Set can be issued.
            if (!host.modelOpening) host.model.Close();
            host.open = false;
        }
    }
    inline void Collect()
    {
        if (Unwind::depth) return;
        for (auto& value : host.controllers)
            if (value->retired) UnlinkController(*value);
        std::erase_if(host.controllers, [](const auto& value) { return value->retired; });
    }

    inline std::string FontText(const std::string& utf8)
    {
        if (utf8.empty()) return {};
        if (utf8.size() > 8192) return "?";
        int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.data(), int(utf8.size()), nullptr, 0);
        if (count <= 0 || count > 2048) return "?";
        std::wstring wide(size_t(count), L'\0');
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.data(), int(utf8.size()), wide.data(), count);
        std::string result(size_t(count) * 2, '\0');
        int bytes = WideCharToMultiByte(1252, 0, wide.data(), count, result.data(), int(result.size()), "?", nullptr);
        if (bytes <= 0 || bytes > 2048) return "?";
        result.resize(size_t(bytes));
        for (auto& c : result) if (static_cast<uint8_t>(c) < 32 || c == 127) c = ' ';
        return result;
    }
    inline void DestroyLeaf(uint32_t id)
    {
        auto* node = Lookup(host.context.manager, id);
        if (!node || Read<void*>(node, 4) != host.root) return;
        auto* previous = host.expectedDestroy;
        host.expectedDestroy = node;
        reinterpret_cast<void(__thiscall*)(void*, uint32_t)>(0x4D2280)(host.context.manager, id);
        host.expectedDestroy = previous;
        std::erase_if(host.created, [&](const Created& value) { return value.id == id && value.node == node; });
    }

    inline void* Create(Parameters& params, uint32_t id, uint32_t table, uint32_t parentID, void* parent)
    {
        if (Lookup(host.context.manager, id) || Lookup(host.context.manager, parentID) != parent) return nullptr;
        // The native command's CreateAttach branch dereferences an unchecked
        // factory result. Use the independently proved factory return first,
        // validate identity, then issue its ordinary parent/child attachment.
        auto* node = reinterpret_cast<void*(__thiscall*)(void*, Structure*)>(0x4D1710)(host.context.manager, &params.value);
        if (!Object(node, table, table == 0x58F074 ? 476 : 448) || Read<uint32_t>(node, 52) != id
            || Lookup(host.context.manager, id) != node) return nullptr;
        // Record publication before a callback-capable attachment. Rollback
        // can reclaim an exact registered unattached node as well as children.
        host.created.push_back({ id, table, node, parent });
        if (Lookup(host.context.manager, parentID) != parent) return nullptr;
        Parameters attachment;
        attachment.Name("parent", parentID);
        attachment.Name("child", id);
        reinterpret_cast<uint8_t(__thiscall*)(void*, uint32_t, Structure*, void*)>(0x4D1490)(host.context.manager, 0xF1E6688E, &attachment.value, nullptr);
        return Read<void*>(node, 4) == parent ? node : nullptr;
    }
    inline bool DefaultDrawer()
    {
        auto* resources = Read<void*>(reinterpret_cast<void*>(0x8F0D08));
        if (!Readable(resources, 28)) return false;
        auto* drawer = reinterpret_cast<void*(__thiscall*)(void*, uint32_t, uint32_t)>(0x4D2FA0)(resources, uint32_t(-161245377), Checksum("main"));
        if (!Readable(drawer, 28)) return false;
        // Type compatibility is established by identity with an existing stock
        // text element's drawer, not by assuming an unchecked RTTI cast.
        for (int32_t index = 0; index < 128; ++index)
        {
            auto* child = reinterpret_cast<void*(__thiscall*)(void*, int32_t, uint32_t*)>(0x4CD710)(host.context.base, index, nullptr);
            if (!child) break;
            if (Object(child, 0x58F074, 476) && Read<void*>(child, 452) == drawer
                && Readable(Read<void*>(child, 448), 84)) return true;
        }
        return false;
    }
    inline void* Text(uint32_t id, const std::string& label, uint32_t parentID, void* parent, bool enabled,
        bool heading = false, uint32_t link = 0, const std::string& suffix = {})
    {
        Parameters params;
        params.Name("type", "textmenuelement");
        params.Name("id", id);
        params.Text("text", FontText(label + suffix));
        params.Name("drawer", "main");
        // The private menu briefly has no children during a rebuild. Its
        // intended width stays fixed while stock entries use their own list.
        const float width = parentID == RootID ? 420.0f : Read<float>(parent, 76);
        if (!std::isfinite(width) || width <= 0.0f || width > 4096.0f) return nullptr;
        params.Float("w", width);
        params.Flag("lock_width");
        if (!enabled || heading) params.Flag("static");
        if (heading) params.Flag("dont_gray");
        if (link) params.Name("link", link);
        auto node = Create(params, id, 0x58F074, parentID, parent);
        if (node) captions[node] = { FontText(label), FontText(suffix) };
        return node;
    }
    inline void RefreshHelp()
    {
        auto node = Lookup(host.context.manager, HelpID);
        auto found = captions.find(node);
        if (found == captions.end()) return;
        std::string text;
        auto selected = Read<uint32_t>(host.root, 420);
        for (auto& item : host.items)
            if (item.id == selected && item.kind == ItemKind::Value) { text = host.model.Help(item.row); break; }
        if (host.model.RestartRequired()) text = host.model.Text("menu.restartwarning") + "  " + text;
        if (text.empty()) text = " ";
        text = FontText(text);
        if (found->second.label != text)
        {
            found->second.label = std::move(text);
            found->second.font = nullptr;
        }
    }
    inline void Close(bool nativeBack)
    {
        const bool owned = host.context.PrivateCurrent(host.root);
        host.model.Close();
        host.open = false;
        ++host.generation;
        host.pending.clear();
        host.nativeBackPending = false;
        for (auto& value : host.controllers)
            if (!value->retired) value->generation = host.generation;
        if (nativeBack && owned)
        {
            Busy busy;
            Emit(BackEvent, RootID);
        }
    }
    inline bool SameItem(const Item& a, const Item& b)
    {
        return a.kind == b.kind && a.page == b.page && (a.kind != ItemKind::Value
            || (a.row.kind == b.row.kind && a.row.plugin == b.row.plugin && a.row.setting == b.row.setting));
    }
    inline bool Populate()
    {
        Snapshot snapshot;
        if (!host.open || !snapshot.Current()) return false;
        std::vector<Item> desired;
        std::string heading = host.model.Text("menu.plugins");
        if (host.page == RootPage)
        {
            auto count = host.model.Pages().size();
            if (host.start >= count) host.start = 0;
            for (size_t page = host.start; page < std::min(count, host.start + PageRows); ++page)
            {
                Item item;
                item.kind = ItemKind::Page;
                item.page = page;
                item.label = host.model.Pages()[page].title;
                desired.push_back(std::move(item));
            }
            if (host.start) desired.push_back({ ItemKind::Previous, 0, {}, 0, nullptr, host.model.Text("menu.previous") });
            if (host.start + PageRows < count) desired.push_back({ ItemKind::Next, 0, {}, 0, nullptr, host.model.Text("menu.next") });
        }
        else
        {
            if (host.page >= host.model.Pages().size()) return false;
            heading = host.model.Pages()[host.page].title;
            std::vector<const RWGTAMenu::Row*> visible;
            { ForeignScope call; visible = host.model.VisibleRows(host.page); }
            if (!snapshot.Current()) return false;
            std::vector<RWGTAMenu::Row> rows;
            for (auto* row : visible) rows.push_back(*row);
            auto end = std::min(rows.size(), host.rowStart + PageRows);
            for (size_t i = host.rowStart; i < end; ++i)
            {
                const auto& row = rows[i];
                std::string value;
                { ForeignScope call; value = host.model.Value(row); }
                if (!snapshot.Current()) return false;
                bool enabled;
                { ForeignScope call; enabled = host.model.Enabled(row, host.context.pause); }
                if (!snapshot.Current()) return false;
                auto label = host.model.Label(row);
                std::string suffix;
                if (!value.empty()) suffix = ": " + value;
                if (!enabled) suffix += " [" + host.model.Text("menu.unavailable") + "]";
                desired.push_back({ ItemKind::Value, host.page, row, 0, nullptr, std::move(label), enabled, std::move(suffix) });
            }
            if (host.rowStart) desired.push_back({ ItemKind::Previous, 0, {}, 0, nullptr, host.model.Text("menu.previous") });
            if (end < rows.size()) desired.push_back({ ItemKind::Next, 0, {}, 0, nullptr, host.model.Text("menu.next") });
        }
        desired.push_back({ ItemKind::Back, 0, {}, 0, nullptr, host.model.Text("menu.back") });
        if (desired.size() > PageRows + 3) return false;
        Item selected;
        bool haveSelection = false;
        const auto selectedID = Read<uint32_t>(host.root, 420);
        for (const auto& item : host.items)
            if (item.id == selectedID) { selected = item; haveSelection = true; break; }
        Busy busy;
        ++host.generation;
        snapshot.generation = host.generation;
        host.pending.clear();
        for (auto& controller : host.controllers)
            if (!controller->retired && controller->native.owner == host.root) controller->generation = host.generation;
        auto oldLeaves = std::move(host.leaves);
        for (auto id : oldLeaves)
        {
            DestroyLeaf(id);
            if (!snapshot.Current()) return false;
        }
        host.items.clear();
        constexpr uint32_t headingID = Checksum("wfp_thps3_heading");
        auto* title = Text(headingID, heading, RootID, host.root, false, true);
        if (!title || !snapshot.Current()) { Diagnostic("private heading construction/owner refused"); return false; }
        host.leaves.push_back(headingID);
        for (size_t row = 0; row < desired.size(); ++row)
        {
            auto& item = desired[row];
            item.id = Checksum(("wfp_thps3_row_" + std::to_string(row)).c_str());
            item.widget = Text(item.id, item.label, RootID, host.root, item.enabled, false, 0, item.suffix);
            if (!item.widget || !snapshot.Current()) { Diagnostic("private item construction/owner refused"); return false; }
            host.leaves.push_back(item.id);
            Attach(item.widget, Command::Accept, row);
            if (!snapshot.Current()) return false;
        }
        auto help = Text(HelpID, " ", RootID, host.root, false, true);
        if (!help || !snapshot.Current()) { Diagnostic("private help construction/owner refused"); return false; }
        host.leaves.push_back(HelpID);
        host.items = std::move(desired);
        Emit(PrepareEvent, RootID);
        if (!snapshot.Current()) return false;
        // These children were created after the root's original Show. Prepare
        // enables them, but only Show gives them native visible/render state.
        Emit(ShowEvent, RootID);
        if (!snapshot.Current()) return false;
        uint32_t choice = 0;
        for (const auto& item : host.items)
        {
            if (!item.enabled) continue;
            if (!choice) choice = item.id;
            if (haveSelection && SameItem(item, selected)) { choice = item.id; break; }
        }
        if (choice)
            reinterpret_cast<uint8_t(__thiscall*)(void*, uint32_t)>(0x4CE2C0)(host.root, choice);
        if (!snapshot.Current()) return false;
        RefreshHelp();
        for (auto id : host.leaves)
        {
            auto* node = Lookup(host.context.manager, id);
            if (!Fit(node))
            {
                const auto reason = std::string("private row layout refused; visible=") + std::to_string(Read<uint8_t>(node, 92))
                    + ", native mode=" + std::to_string(Read<uint8_t>(node, 475));
                Diagnostic(reason.c_str());
                return false;
            }
            if (!snapshot.Current()) return false;
        }
        reinterpret_cast<void(__thiscall*)(void*)>(0x4D73A0)(host.root);
        return snapshot.Current();
    }
    inline void Open()
    {
        if (host.open || host.modelOpening || !host.context.PrivateCurrent(host.root)) return;
        Snapshot snapshot;
        {
            struct Opening
            {
                Opening() { host.modelOpening = true; }
                ~Opening() { host.modelOpening = false; }
            } opening;
            // Discovery calls foreign FindLoadedPlugins/Subscribe while the
            // shared model builds its member vector. Retire invalidates native
            // ownership immediately, but defers Close for this one operation.
            host.model.OpenPlugins(module, Language(), 4096);
        }
        if (!snapshot.Current()) { host.model.Close(); return; }
        host.open = true;
        host.page = RootPage;
        host.start = 0;
        host.rowStart = 0;
        ++host.generation;
        for (auto& value : host.controllers)
            if (!value->retired) value->generation = host.generation;
        if (!Populate()) { Diagnostic("private row publication refused; native return retained"); Close(true); }
    }
    inline void Activate(size_t index, int direction)
    {
        Snapshot snapshot;
        if (!snapshot.Current() || !host.open || index >= host.items.size()) return;
        Item item = host.items[index];
        if (!item.enabled) return;
        if (item.kind != ItemKind::Value && direction) return;
        switch (item.kind)
        {
            case ItemKind::Page:
                host.page = item.page;
                host.rowStart = 0;
                host.model.PageShown(host.page);
                break;
            case ItemKind::Previous:
                if (host.page == RootPage) host.start -= std::min(host.start, PageRows);
                else host.rowStart -= std::min(host.rowStart, PageRows);
                break;
            case ItemKind::Next:
                if (host.page == RootPage) host.start += PageRows;
                else host.rowStart += PageRows;
                break;
            case ItemKind::Back:
                if (host.page == RootPage) { Close(true); return; }
                host.model.PageShown(host.page);
                host.page = RootPage;
                host.rowStart = 0;
                break;
            case ItemKind::Value:
                { ForeignScope call; host.model.Change(item.row, direction, host.context.pause); }
                if (!snapshot.Current()) return;
                break;
        }
        if (!Populate()) Close(true);
    }

    inline void Observe()
    {
        if (!host.root || host.busy) return;
        // A transient desktop focus loss is not native owner replacement. The
        // committed owner/selected child checks below do not use screen.active
        // or parent focus bytes; input/actions still require the full predicate.
        auto& context = host.context;
        const bool identity = Manager() == context.manager && Readable(context.frontend, 76)
            && Read<void*>(reinterpret_cast<void*>(0x8E1FB0)) == context.frontend
            && !Read<uint32_t>(context.frontend, 60) && Read<void*>(context.frontend, 68) == context.screen
            && Read<uint32_t>(context.frontend, 64) == context.screenID
            && Lookup(context.manager, ParentID) == context.parent && Object(context.parent, 0x58EF00, 704)
            && Lookup(context.manager, context.baseID) == context.base
            && Lookup(context.manager, RootID) == host.root && Object(host.root, 0x58F11C, 448)
            && Read<void*>(host.root, 4) == context.parent;
        if (!identity)
        {
            ++host.generation;
            host.invalid = true;
            if (!host.modelOpening) host.model.Close();
            host.open = false;
            return;
        }
        if (host.open && (SelectedChild(context.parent) != host.root || !Read<uint8_t>(context.parent, 94))) Close(false);
    }
    inline void CleanupCreated()
    {
        if (Unwind::depth || Manager() != host.context.manager) return;
        // Do not erase the contents of a selected private root before a safe
        // native escape. This remains true behind a modal/inactive window too.
        if (Lookup(host.context.manager, RootID) == host.root && Object(host.root, 0x58F11C, 448)
            && Object(host.context.parent, 0x58EF00, 704)
            && (SelectedChild(host.context.parent) == host.root || Read<uint32_t>(host.context.parent, 424) == RootID)) return;
        // Exact registered identities only, in child-before-parent order. A
        // foreign replacement under the same checksum is never ours to delete.
        auto created = host.created;
        for (auto it = created.rbegin(); it != created.rend(); ++it)
        {
            if (Lookup(host.context.manager, it->id) != it->node
                || !Object(it->node, it->table, it->table == 0x58F074 ? 476 : 448)) continue;
            auto* parent = Read<void*>(it->node, 4);
            if (parent && parent != it->parent) continue;
            if (it->table == 0x58F11C && parent && SelectedChild(parent) == it->node) continue;
            auto* selected = Object(parent, 0x58EF00, 704) ? SelectedChild(parent) : nullptr;
            const auto selectedID = Read<uint32_t>(selected, 52);
            const bool preserveIndex = selected != it->node && Object(selected, 0x58F11C, 448)
                && Read<uint32_t>(parent, 424) == selectedID && Lookup(host.context.manager, selectedID) == selected;
            auto* previous = host.expectedDestroy;
            host.expectedDestroy = it->node;
            reinterpret_cast<void(__thiscall*)(void*, uint32_t)>(0x4D2280)(host.context.manager, it->id);
            host.expectedDestroy = previous;
            // Removing our earlier sibling must not shift the current stock
            // child's index. Preserve its exact pointer/logical ID; the native
            // pure lookup supplies the same index used by4CF380 itself.
            if (preserveIndex && Lookup(host.context.manager, ParentID) == parent && Object(parent, 0x58EF00, 704)
                && Read<uint32_t>(parent, 424) == selectedID && Lookup(host.context.manager, selectedID) == selected
                && Object(selected, 0x58F11C, 448) && Read<void*>(selected, 4) == parent)
            {
                int32_t index = -1;
                if (reinterpret_cast<void*(__thiscall*)(void*, uint32_t, int32_t*)>(0x4CD6C0)(parent, selectedID, &index) == selected
                    && index >= 0 && index <= 1024)
                    Write(parent, 420, index);
            }
        }
        std::erase_if(host.created, [](const Created& value) { return Lookup(host.context.manager, value.id) != value.node; });
    }
    inline void Prepare(const Context& context)
    {
        if (!context.StockCurrent()) return;
        if (host.root || host.entry) return;
        host.context = context;
        host.invalid = false;
        host.recovering = false;
        host.baseRetiredWhilePrivate = false;
        ++host.generation;
        if (!DefaultDrawer()) { Diagnostic("stock default drawer/font identity unavailable"); return; }
        if (!context.StockCurrent()) { Diagnostic("stock owner changed before preparation"); return; }
        if (Lookup(context.manager, RootID) || Lookup(context.manager, EntryID)) { Diagnostic("private ID already occupied"); return; }
        Busy busy;
        Parameters params;
        params.Name("type", "verticalmenu");
        params.Name("id", RootID);
        params.Float("x", 110.0f);
        params.Float("y", 60.0f);
        params.Float("w", 420.0f);
        params.Float("h", 400.0f);
        host.root = Create(params, RootID, 0x58F11C, ParentID, context.parent);
        if (!host.root || host.invalid || !context.StockCurrent())
        {
            Diagnostic("private root construction refused");
            CleanupCreated();
            host.root = host.entry = nullptr;
            return;
        }
        Attach(host.root, Command::Open);
        Emit(1533424852, RootID);
        constexpr uint32_t placeholder = Checksum("wfp_thps3_loading");
        auto* back = Text(placeholder, RWGTAMenu::PluginText(FixSettings(), Language(), "menu.back"), RootID, host.root, true);
        if (back) host.leaves.push_back(placeholder);
        if (!back || host.invalid || !context.StockCurrent())
        {
            Diagnostic("placeholder construction refused");
            CleanupCreated();
            host.root = host.entry = nullptr;
            return;
        }
        auto label = RWGTAMenu::PluginText(FixSettings(), Language(), "menu.plugins");
        host.entry = Text(EntryID, label, context.baseID, context.base, true);
        if (!host.entry || host.invalid || !context.StockCurrent())
        {
            Diagnostic("Plugins entry construction refused");
            CleanupCreated();
            host.root = host.entry = nullptr;
            return;
        }
        Attach(host.entry, Command::Enter);
        Emit(ShowEvent, EntryID);
        if (host.invalid || !context.StockCurrent() || !Fit(host.entry, Read<float>(context.base, 76)))
        {
            Diagnostic("Plugins entry fitting refused");
            CleanupCreated();
            host.root = host.entry = nullptr;
            return;
        }
        if (!host.invalid && context.StockCurrent())
            reinterpret_cast<void(__thiscall*)(void*)>(0x4D73A0)(context.base);
    }
    inline void Frame()
    {
        if (!installed || Unwind::depth) return;
        Collect();
        if (host.invalid)
        {
            RepairRecoveryIndex();
            if (RecoveryCurrent())
            {
                if (!host.recovering || host.nativeBackPending)
                {
                    host.nativeBackPending = false;
                    Busy busy;
                    Emit(BackEvent, RootID);
                }
                if (RecoveryCurrent())
                {
                    if (!host.recovering)
                    {
                        host.recovering = true;
                        for (auto& controller : host.controllers)
                            if (!controller->retired) controller->generation = host.generation;
                        uint32_t back = 0;
                        for (auto& item : host.items)
                        {
                            if (Lookup(host.context.manager, item.id) != item.widget || !Object(item.widget, 0x58F074, 476)
                                || Read<void*>(item.widget, 4) != host.root) continue;
                            if (item.kind == ItemKind::Back) back = item.id;
                            else { Write(item.widget, 95, uint8_t(1)); Write(item.widget, 93, uint8_t(0)); }
                        }
                        if (back) reinterpret_cast<uint8_t(__thiscall*)(void*, uint32_t)>(0x4CE2C0)(host.root, back);
                        Log("stock owner retired; native Back retained, settings disabled");
                    }
                    Collect();
                    return;
                }
            }
            if (!host.recovering) Close(false);
            if (Manager() != host.context.manager) host.created.clear();
            CleanupCreated();
            if (host.created.empty())
            {
                host.root = host.entry = nullptr;
                host.items.clear(); host.leaves.clear();
                host.context = {};
                host.invalid = false;
                host.recovering = false;
                host.baseRetiredWhilePrivate = false;
            }
            return;
        }
        if (host.root && !host.context.Alive())
        {
            Observe();
            return;
        }
        if (!host.root)
        {
            const char* failure = nullptr;
            auto context = Context::Discover(&failure);
            Diagnostic(failure);
            Prepare(context);
            return;
        }
        if (host.enterGeneration)
        {
            const auto generation = host.enterGeneration;
            host.enterGeneration = 0;
            if (generation == host.generation && host.context.StockCurrent()
                && Lookup(host.context.manager, EntryID) == host.entry && Object(host.entry, 0x58F074, 476)
                && Read<void*>(host.entry, 4) == host.context.base && Read<uint32_t>(host.context.base, 420) == EntryID
                && Lookup(host.context.manager, RootID) == host.root && Object(host.root, 0x58F11C, 448)
                && Read<void*>(host.root, 4) == host.context.parent)
            {
                // Navigate only after the Choose dispatch unwinds. The native
                // container owns history, visibility and focus for both pages.
                Busy busy;
                if (!reinterpret_cast<uint8_t(__thiscall*)(void*, uint32_t, uint8_t, uint8_t, uint8_t)>(0x4CF380)
                    (host.context.parent, RootID, 0, 0, 1)) Diagnostic("native Plugins navigation refused");
            }
        }
        if (!host.context.PrivateCurrent(host.root))
        {
            Observe();
            host.pending.clear();
            return;
        }
        if (host.nativeBackPending) { Close(true); return; }
        if (!host.open) Open();
        if (!host.open || !host.context.PrivateCurrent(host.root)) return;
        auto pending = std::move(host.pending);
        host.pending.clear();
        for (const auto& request : pending)
        {
            if (request.generation != host.generation || request.page != host.page || request.start != host.start || request.rowStart != host.rowStart
                || !host.context.PrivateCurrent(host.root)) continue;
            if ((request.command == Command::Accept || request.command == Command::Left || request.command == Command::Right)
                && (request.row >= host.items.size() || host.items[request.row].widget != request.selectedWidget
                    || host.items[request.row].id != request.selectedID || Read<uint32_t>(host.root, 420) != request.selectedID
                    || !Object(request.selectedWidget, 0x58F074, 476) || !Read<uint8_t>(request.selectedWidget, 94))) continue;
            switch (request.command)
            {
                case Command::Accept: Activate(request.row, 0); break;
                case Command::Left: Activate(request.row, -1); break;
                case Command::Right: Activate(request.row, 1); break;
                case Command::Back:
                    if (host.page == RootPage) Close(true);
                    else
                    {
                        host.model.PageShown(host.page);
                        host.page = RootPage;
                        host.rowStart = 0;
                        if (!Populate()) Close(true);
                    }
                    break;
                default: break;
            }
        }
        if (host.nativeBackPending) { Close(true); return; }
        if (host.open && host.context.PrivateCurrent(host.root) && RWGTAMenu::Model::Changed().exchange(false))
            if (!Populate()) Close(true);
        if (host.open && host.context.PrivateCurrent(host.root))
        {
            Snapshot snapshot;
            RefreshHelp();
            for (auto id : host.leaves)
                if (!Fit(Lookup(host.context.manager, id)) || !snapshot.Current()) { Close(true); return; }
            reinterpret_cast<void(__thiscall*)(void*)>(0x4D73A0)(host.root);
        }
        Collect();
    }
    inline void Init()
    {
        if (installed) return;
        if (!Contracts()) { Log("unsupported executable contracts"); return; }
        HMODULE pinned = nullptr;
        // Native intrusive controller nodes retain our vtable/function pointers.
        // Pin before any hook or allocation publishes them; dynamic unload is
        // unsupported for this verified host, including an explicit FreeLibrary.
        if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
            reinterpret_cast<LPCWSTR>(&Init), &pinned)) { Log("module pin failed"); return; }
        module = pinned;
        if (!Unwind::Install(Frame, Retire, Observe)) { Log("native unwind hook installation failed"); return; }
        installed = true;
    }
}
