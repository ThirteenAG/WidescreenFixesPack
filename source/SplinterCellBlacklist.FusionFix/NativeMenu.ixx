module;

#include <stdafx.h>
#include <stdexcept>
#include "FixSettings.hpp"
#include <algorithm>
#include <cwctype>
#include <string>
#include <vector>
#include <cstdint>
#include <cstddef>
#include "Shared/RWGTA/FixMenu.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"
#include "Shared/MenuList.hpp"
#include <array>
#include <deque>
#include <map>
#include <memory>
#include <limits>

export module NativeMenu;

export import NativeMenuContracts;
export import NativeMenuABI;
export import MenuText;
import ComVars;

export namespace BlacklistMenu
{
    using Bridge = BlacklistMenuABI::Bridge;
    using Slot = BlacklistMenuABI::Slot;
    constexpr int32_t Advanced = 0x6000;
    constexpr int32_t RowCommand = 0x6100;
    // Rows shown at once; the list scrolls through the rest
    constexpr size_t Slots = 10;
    // Capitals in the list's row and in the line under the title
    constexpr size_t RowColumns = 30, LineColumns = 34;
    constexpr size_t RootPage = RWGTAMenu::Model::RootPage;
    inline Bridge bridge;
    inline HMODULE self = nullptr;
    inline SafetyHookInline settingsHook, performHook, backHook, destroyHook, updateHook;
    inline bool ready = false, busy = false, draining = false;
    inline unsigned updateDepth = 0;
    inline void* currentManager = nullptr;
    inline asi_settings::DescribedFile hostFile;
    inline void Log(const std::string& message)
    {
        OutputDebugStringA(("Blacklist native menu: " + message + "\n").c_str());
    }

    struct PreserveBool
    {
        bool& target;
        bool previous;
        explicit PreserveBool(bool& value) : target(value), previous(value) { target = true; }
        ~PreserveBool() { target = previous; }
    };
    enum class Kind { Page, Back, Model, Text };
    struct Host;
    struct Binding
    {
        std::weak_ptr<Host> owner;
        Slot slot;
        size_t row = 0;
    };
    struct Item
    {
        int32_t command = 0;
        Kind kind = Kind::Back;
        size_t page = 0;
        RWGTAMenu::Row model;
        Binding binding;
        std::wstring label, value;
        bool enabled = true, cycler = false;
    };
    struct Host : std::enable_shared_from_this<Host>
    {
        void* scene = nullptr;
        void* tiered = nullptr;
        void* list = nullptr;
        void* manager = nullptr;
        int32_t advanced = Advanced;
        bool alive = true, open = false, details = false, renderedDetails = false, notice = false;
        MenuList::Window helpWindow;
        // The row the list had selected, and one it moved to that the window follows next update
        int32_t lastIndex = 0, follow = -1;
        // Updates since a rebuild during which the list still moves back to the row it had before, which isn't a move
        int settle = 0;
        std::wstring helpTitle;
        std::vector<std::wstring> helpLines;
        uint64_t generation = 1;
        size_t page = RootPage;
        RWGTAMenu::Model model;
        std::array<Item, 14> items;
        size_t count = 0;
        Binding accept, selected;
    };
    inline std::map<void*, std::shared_ptr<Host>> hosts;
    // Native signal dispatchers touch Slot::emitting after our callback returns.
    // Keep invalidated C++ bindings until the complete manager update unwinds.
    inline std::vector<std::shared_ptr<Host>> retired;
    inline std::weak_ptr<Host> active;
    inline MenuList::WheelNotches wheel;
    struct Pending
    {
        std::weak_ptr<Host> host;
        uint64_t generation;
        size_t page, row;
        int direction;
        bool open;
        uint32_t inputMask;
        int32_t gameMode;
    };
    inline std::deque<Pending> pending;
    inline bool Live(const Host& host)
    {
        auto it = hosts.find(host.scene);
        return host.alive && it != hosts.end() && it->second.get() == &host;
    }
    inline bool OwnsFocus(const Host& host, int32_t tier)
    {
        if (tier != 1 && tier != 12) return false;
        auto top = reinterpret_cast<void*(__cdecl*)()>(bridge.topFocus)();
        if (!top) return false;
        if (top == host.scene || top == host.tiered) return true;
        auto lists = Bridge::Read<void**>(host.tiered, 612);
        if (!lists || Bridge::Read<int32_t>(host.tiered, 616) != 13) return false;
        auto list = lists[tier];
        if (!list) return false;
        if (top == list) return true;
        auto count = Bridge::Read<int32_t>(list, 644);
        if (count < 0 || count > 14) return false;
        for (int32_t i = 0; i < count; ++i)
        {
            auto widget = Bridge::Widget(list, i);
            // Native rows cache controls of multiple kinds. Focus can remain on
            // the previous kind's child while SetMaxVisibleItems replaces it.
            if (widget && (top == widget || top == bridge.Control(widget) ||
                Bridge::Read<void*>(top, 376) == widget)) return true;
        }
        return false;
    }
    struct Snapshot
    {
        std::shared_ptr<Host> host;
        uint64_t generation;
        bool open;
        int32_t tier;
        uint32_t inputMask;
        int32_t gameMode;
        bool requireFocus;
        bool rebuilding = false;
        void* previousFocus = nullptr;
        explicit Snapshot(Host& value) : host(value.shared_from_this()), generation(value.generation), open(value.open),
            tier(Bridge::Read<int32_t>(value.tiered, 592)), inputMask(Bridge::Read<uint32_t>(value.manager, 0x5A0)), gameMode(bridge.GameMode()),
            requireFocus(value.open) {}
        bool Owner() const { return Live(*host) && host->generation == generation && host->open == open; }
        const char* Failure() const
        {
            if (!Owner()) return "owner/generation";
            if (currentManager != host->manager) return "manager";
            if (Bridge::Read<int32_t>(host->tiered, 592) != tier) return "tier";
            if (Bridge::Read<uint32_t>(host->manager, 0x5A0) != inputMask) return "input mask";
            if (bridge.GameMode() != gameMode) return "game mode";
            if (tier == 12 && open && Bridge::Read<uint8_t>(host->list, 553) != 0) return "list transition";
            if (requireFocus && !OwnsFocus(*host, tier))
            {
                auto top = reinterpret_cast<void*(__cdecl*)()>(bridge.topFocus)();
                if (!rebuilding || (top && top != previousFocus)) return "focus";
            }
            return nullptr;
        }
        bool Current() const { return Failure() == nullptr; }
    };
    inline std::wstring Wide(const std::string& value)
    {
        if (value.empty()) return {};
        auto length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()), nullptr, 0);
        if (!length) return L"?";
        std::wstring result(static_cast<size_t>(length), L'\0');
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()), result.data(), length);
        return result;
    }
    inline std::string Language();
    // The game's menus are in capitals
    inline std::wstring Upper(std::wstring value)
    {
        auto tag = Wide(Language());
        if (int size = LCMapStringEx(tag.c_str(), LCMAP_UPPERCASE, value.data(), int(value.size()), nullptr, 0, nullptr, nullptr, 0); size > 0)
        {
            std::wstring upper(size, L'\0');
            if (LCMapStringEx(tag.c_str(), LCMAP_UPPERCASE, value.data(), int(value.size()), upper.data(), size, nullptr, nullptr, 0)) return upper;
        }
        return value;
    }
    inline std::string Language()
    {
        auto language = FixSettings().Get("MAIN.TextLanguage");
        if (language == "auto")
        {
            auto native = reinterpret_cast<const char*(__cdecl*)()>(bridge.currentLanguage)();
            language = native ? std::string(native, strnlen(native, 8)) : "en";
            std::transform(language.begin(), language.end(), language.begin(), [](unsigned char c) { return char(std::tolower(c)); });
            static const std::map<std::string, std::string> tags = {
                {"int", "en"}, {"eng", "en"}, {"fre", "fr"}, {"fra", "fr"},
                {"deu", "de"}, {"ger", "de"}, {"ita", "it"}, {"spa", "es"}, {"esp", "es"}
            };
            if (auto found = tags.find(language); found != tags.end()) language = found->second;
        }
        return language;
    }
    inline bool Queue(Host& host, size_t row, int direction, bool open = false)
    {
        if (!ready || busy || !Live(host) || pending.size() >= 64 || host.manager != currentManager ||
            Bridge::Read<int32_t>(host.tiered, 592) != (open ? 1 : 12) ||
            (!open && Bridge::Read<uint8_t>(host.list, 553) != 0) || !OwnsFocus(host, open ? 1 : 12)) return false;
        pending.push_back({ host.shared_from_this(), host.generation, host.page, row, direction, open,
            Bridge::Read<uint32_t>(host.manager, 0x5A0), bridge.GameMode() });
        return true;
    }
    inline void DisconnectRows(Host& host)
    {
        for (auto& item : host.items) bridge.Disconnect(item.binding.slot);
    }
    inline uint8_t __fastcall Changed(Binding* binding, void*, Slot*, int32_t value)
    {
        auto host = binding->owner.lock();
        if (host && Live(*host) && host->open && value != 0)
            Queue(*host, binding->row, value < 0 ? -1 : 1);
        return 1;
    }
    inline uint8_t __fastcall Accepted(Binding* binding, void*, Slot*, uint8_t back)
    {
        auto host = binding->owner.lock();
        if (!host || !Live(*host) || !host->open || busy) return 1;
        if (back) Queue(*host, host->count, 0);
        else
        {
            auto selected = Bridge::Read<int32_t>(host->list, 576);
            if (selected >= 0 && static_cast<size_t>(selected) < host->count)
                Queue(*host, static_cast<size_t>(selected), 0);
        }
        return 1;
    }
    // The help line under the title: the whole help when it fits, else its first whole sentences that do (F1 shows
    // all of it), else the F1 hint
    inline std::wstring HelpLine(Host& host, const std::wstring& help)
    {
        const auto hint = Wide(host.model.Text("menu.helphint"));
        if (help.empty()) return {};
        if (MenuText::Wrap(help, LineColumns).size() == 1) return help;
        for (size_t cut = help.size(); cut > 0;)
        {
            cut = help.find_last_of(L".!?", cut - 1);
            if (cut == std::wstring::npos) break;
            if (cut + 1 < help.size() && !std::iswspace(help[cut + 1])) continue;
            const auto shown = help.substr(0, cut + 1);
            if (MenuText::Wrap(shown + L"  " + hint, LineColumns).size() == 1) return shown + L"  " + hint;
        }
        return hint;
    }
    inline void Hint(Host& host, int32_t index)
    {
        if (host.details || index < 0 || size_t(index) >= host.count) return;
        auto& item = host.items[index];
        std::wstring hint;
        if (item.kind == Kind::Model) hint = HelpLine(host, Wide(host.model.Help(item.model)));
        if (host.model.RestartRequired()) hint = Wide(host.model.Text("menu.restartwarning"));
        bridge.Caption(host.scene, 928, Upper(hint));
    }
    inline uint8_t __fastcall Selected(Binding* binding, void*, Slot*, int32_t index)
    {
        auto host = binding->owner.lock();
        if (host && Live(*host) && host->open && Bridge::Read<int32_t>(host->tiered, 592) == 12 &&
            index >= 0 && static_cast<size_t>(index) < host->count)
        {
            Snapshot snapshot(*host);
            if (!snapshot.Current()) return 1;
            Bridge::Write<int32_t>(host->scene, 1052, index);
            if (!busy)
            {
                // The window follows after this signal unwinds
                host->follow = index;
                if (!host->details) Hint(*host, index);
            }
        }
        return 1;
    }
    inline void SetBinding(Binding& binding, const std::shared_ptr<Host>& host, size_t row, void* callback)
    {
        binding.owner = host;
        binding.row = row;
        binding.slot.owner = &binding;
        binding.slot.callback = callback;
    }
    inline bool ItemRow(Host& host, Kind kind, size_t page, const RWGTAMenu::Row* model, const std::wstring& label)
    {
        Snapshot snapshot(host);
        if (host.count >= host.items.size()) return false;
        auto index = host.count++;
        auto& item = host.items[index];
        item.command = RowCommand + static_cast<int32_t>(index);
        item.kind = kind;
        item.page = page;
        if (model) item.model = *model;
        bool cycler = model && (model->kind == RWGTAMenu::RowKind::Setting || model->kind == RWGTAMenu::RowKind::Preset);
        bool enabled = true;
        std::string value;
        if (model)
        {
            enabled = host.model.Enabled(item.model, snapshot.gameMode != 11);
            if (!snapshot.Current()) return false;
            value = host.model.Value(item.model);
            if (!snapshot.Current()) return false;
        }
        auto display = Upper(label);
        if (!cycler && !value.empty()) display += L"  " + Upper(Wide(value));
        auto result = bridge.AddRow(host.list, display, &item.command, cycler ? 18 : 15);
        if (!snapshot.Current() || result <= 0)
        { Log("row creation refused at row " + std::to_string(index)); return false; }
        item.label = Upper(label);
        item.value = Upper(Wide(value));
        item.enabled = enabled;
        item.cycler = cycler;
        return true;
    }
    inline bool Configure(Host& host)
    {
        Snapshot snapshot(host);
        auto current = [&](size_t index, const char* stage)
        {
            auto failure = snapshot.Failure();
            if (!failure) return true;
            Log("row configure context lost at row " + std::to_string(index) + " after " + stage + ": " + failure);
            return false;
        };
        for (size_t index = 0; index < host.count; ++index)
        {
        auto& item = host.items[index];
        auto widget = Bridge::Widget(host.list, static_cast<int32_t>(index));
        auto record = bridge.Record(host.list, static_cast<int32_t>(index));
        if (!widget || !record || Bridge::Read<const int32_t*>(record, 64) != &item.command)
        { Log("row configure widget/record identity refused at row " + std::to_string(index)); return false; }
        if (item.kind != Kind::Model) continue;
        if (item.cycler)
        {
            auto control = bridge.Control(widget);
            if (!current(index, "control lookup")) return false;
            if (!control)
            { Log("row configure native cycler control unavailable at row " + std::to_string(index)); return false; }
            Bridge::Virtual<void>(control, 376, item.label.c_str());
            if (!current(index, "label")) return false;
            bridge.ChoiceClear(Bridge::At(control, 592));
            if (!current(index, "choice clear")) return false;
            bridge.ChoiceAdd(Bridge::At(control, 592), L"-", -1);
            if (!current(index, "previous choice")) return false;
            bridge.ChoiceAdd(Bridge::At(control, 592), item.value, 0);
            if (!current(index, "current choice")) return false;
            bridge.ChoiceAdd(Bridge::At(control, 592), L"+", 1);
            if (!current(index, "next choice")) return false;
            Bridge::Virtual<void>(control, 384, 1); // index of the current-value choice
            if (!current(index, "current value")) return false;
            SetBinding(item.binding, snapshot.host, index, reinterpret_cast<void*>(Changed));
            bridge.Connect(Bridge::At(control, 604), item.binding.slot, true);
            if (!current(index, "change callback binding")) return false;
        }
        // Native disabled styling and backing record follow ListBox::DisableItem.
        Bridge::Virtual<void>(widget, 352, item.enabled ? -1 : 3);
        if (!current(index, "enabled styling")) return false;
        Bridge::Write<int32_t>(record, 68, item.enabled ? -1 : 3);
        if (!item.enabled)
        {
            Bridge::Virtual<void>(widget, 304, 2);
            if (!current(index, "disabled styling")) return false;
        }
        }
        return true;
    }
    inline bool Populate(Host& host)
    {
        ++host.generation; // rows are rebound; old queued widget events expire
        Snapshot snapshot(host);
        PreserveBool guard(busy);
        DisconnectRows(host);
        if (!snapshot.Current()) return false;
        // Replacing the owned row pool temporarily retires its focused child.
        snapshot.previousFocus = reinterpret_cast<void*(__cdecl*)()>(bridge.topFocus)();
        snapshot.rebuilding = true;
        bridge.Clear(host.list);
        if (!snapshot.Current()) return false;
        host.count = 0;
        host.follow = -1;
        std::wstring title = Wide(host.model.Text("menu.plugins"));
        int32_t selectedIndex = 0;
        if (host.details)
        {
            title = host.helpTitle;
            std::vector<uintptr_t> keys;
            for (size_t i = 0; i < host.helpLines.size(); ++i) keys.push_back(i + 1);
            host.helpWindow.SetSlots(Slots);
            host.helpWindow.Update(keys);
            for (size_t i = host.helpWindow.First(); i < host.helpWindow.First() + host.helpWindow.Shown(); ++i)
                if (!ItemRow(host, Kind::Text, 0, nullptr, host.helpLines[i])) return false;
            selectedIndex = int32_t(host.helpWindow.Slot());
        }
        else
        {
            if (host.page >= host.model.Pages().size()) return false;
            if (host.page != RootPage) title = Wide(host.model.Pages()[host.page].title);
            // The game's own lists have no Back row: B / Esc go back a level
            for (auto row : host.model.Shown(host.page, false))
            {
                if (!snapshot.Current()) return false;
                if (!row) continue;
                auto label = Wide(host.model.Label(*row));
                if (!snapshot.Current() || !ItemRow(host, RWGTAMenu::Model::Opens(*row) ? Kind::Page : Kind::Model, 0, row, label)) return false;
            }
            selectedIndex = int32_t(host.model.Window(host.page).Slot());
        }
        if (!host.count) return false;
        // Native settings builders re-enable the list and allocate its row pool
        // through virtual SetMaxVisibleItems before touching cycler controls.
        Bridge::Write<uint8_t>(host.list, 696, 1);
        Bridge::Virtual<void>(host.list, 384, 14);
        if (auto failure = snapshot.Failure())
        { Log(std::string("row pool context lost before configure: ") + failure); return false; }
        if (!Configure(host) || !snapshot.Current()) return false;
        bridge.Caption(host.scene, 924, Upper(title));
        if (!snapshot.Current()) return false;
        bridge.Caption(host.scene, 928, host.details ? Upper(Wide(host.model.Text("menu.back"))) : std::wstring{});
        if (!snapshot.Current()) return false;
        selectedIndex = std::clamp(selectedIndex, 0, int32_t(host.count) - 1);
        // An inactive tier must not steal the stock root's focus before PushTier.
        reinterpret_cast<void(__thiscall*)(void*, int32_t, int32_t)>(bridge.selectRow)(host.list, selectedIndex, snapshot.tier == 12 ? 1 : 0);
        snapshot.rebuilding = false;
        if (!snapshot.Current()) return false;
        Bridge::Write<int32_t>(host.scene, 1052, Bridge::Read<int32_t>(host.list, 576));
        host.lastIndex = Bridge::Read<int32_t>(host.list, 576);
        host.follow = -1;
        host.settle = 3;
        Hint(host, host.lastIndex);
        return true;
    }
    // The list moved its selection (keys, pad, mouse): within its rows, and wrapping there. A step off its last row
    // (or its first) moves the window instead while there are more rows
    inline bool Follow(Host& host, int32_t index)
    {
        auto& window = host.details ? host.helpWindow : host.model.Window(host.page);
        const auto shown = int32_t(host.count), last = host.lastIndex;
        host.lastIndex = index;
        const auto first = window.First();
        if (shown >= 3 && window.Count() > size_t(shown) && last == shown - 1 && index == 0) window.Move(1);
        else if (shown >= 3 && window.Count() > size_t(shown) && last == 0 && index == shown - 1) window.Move(-1);
        else { window.Point(size_t(index)); return false; }
        return window.First() != first || int32_t(window.Slot()) != index;
    }
    // Wheel notches move the window a row each, Page Up/Down and Home/End as in every list
    inline bool Navigate(Host& host, int notches, int key)
    {
        auto& window = host.details ? host.helpWindow : host.model.Window(host.page);
        const auto index = Bridge::Read<int32_t>(host.list, 576);
        if (index >= 0 && size_t(index) < host.count) window.Point(size_t(index));
        bool changed = false;
        for (; notches; notches += notches > 0 ? -1 : 1) changed |= window.Scroll(notches > 0 ? -1 : 1);
        if (key == VK_PRIOR || key == VK_NEXT) changed |= window.Page(key == VK_NEXT ? 1 : -1);
        if (key == VK_HOME || key == VK_END) changed |= window.Edge(key == VK_END ? 1 : -1);
        return changed;
    }
    inline void Close(Host& host)
    {
        Snapshot snapshot(host);
        PreserveBool guard(busy);
        host.open = false;
        ++host.generation;
        snapshot.generation = host.generation;
        snapshot.open = false;
        host.model.Close();
        DisconnectRows(host);
        if (!snapshot.Current()) return;
        if (Bridge::Read<int32_t>(host.tiered, 592) == 12)
            bridge.PopTier(host.tiered);
        if (!snapshot.Owner()) return;
        if (Bridge::Read<int32_t>(host.tiered, 592) == 1)
        {
            auto lists = Bridge::Read<void**>(host.tiered, 612);
            if (lists && lists[1])
            {
                // The focus can stay on a row widget of our list that the help lines reused; give it back to the
                // Options row, as the game's own submenus leave it
                const auto index = Bridge::Read<int32_t>(lists[1], 576);
                if (index >= 0 && index < Bridge::Read<int32_t>(lists[1], 644))
                    reinterpret_cast<void(__thiscall*)(void*, int32_t, int32_t)>(bridge.selectRow)(lists[1], index, 1);
                if (!snapshot.Owner()) return;
                Bridge::Write<int32_t>(host.scene, 1052, Bridge::Read<int32_t>(lists[1], 576));
            }
        }
        if (active.lock() == snapshot.host)
        {
            active.reset();
            NativeMenuOpen.store(false);
        }
    }
    struct OpenRollback
    {
        std::shared_ptr<Host> host;
        uint64_t generation;
        bool complete = false;
        ~OpenRollback()
        {
            if (!complete && Live(*host) && host->open && host->generation == generation) Close(*host);
        }
    };
    inline void Open(Host& host)
    {
        Snapshot snapshot(host);
        snapshot.requireFocus = true;
        PreserveBool guard(busy);
        auto old = active.lock();
        if (old && old.get() != &host) Close(*old);
        if (!snapshot.Current() || Bridge::Read<int32_t>(host.tiered, 592) != 1) return;
        host.model.OpenMenu(self, Language(), Slots);
        if (!snapshot.Current())
        {
            if (snapshot.Owner())
            {
                if (host.open) Close(host);
                else host.model.Close();
            }
            return;
        }
        host.open = true;
        ++host.generation;
        snapshot.open = true;
        snapshot.generation = host.generation;
        OpenRollback rollback{ snapshot.host, host.generation };
        host.page = RootPage;
        host.model.Window(host.page).Reset();
        host.details = host.notice = false;
        host.helpLines.clear();
        NativeMenuWheel = 0; NativeMenuKey = 0; wheel.Clear();
        active = snapshot.host;
        ++snapshot.generation; // Populate advances once before binding rows
        ++rollback.generation;
        if (!Populate(host) || !snapshot.Current()) return;
        // Native PushTier performs focus/blur, fade and the owned history.
        // It deliberately bypasses ScenePause's twelve-slot stock preparer.
        bridge.PushTier(host.tiered, 12);
        if (!snapshot.Owner() || Bridge::Read<int32_t>(host.tiered, 616) != 13 ||
            Bridge::Read<int32_t>(host.tiered, 592) != 12) return;
        snapshot.tier = 12;
        bridge.Caption(host.scene, 924, Upper(Wide(host.model.Text("menu.plugins"))));
        rollback.complete = snapshot.Current();
        NativeMenuOpen.store(rollback.complete);
    }
    inline void Activate(Host& host, size_t index, int direction)
    {
        Snapshot snapshot(host);
        if (!host.open || Bridge::Read<int32_t>(host.tiered, 592) != 12) return;
        if (index >= host.count || host.items[index].kind == Kind::Back)
        {
            if (direction != 0) return;
            if (host.details)
            {
                if (host.notice) { Close(host); return; }
                host.details = false;
                host.helpLines.clear();
            }
            else
            {
                const auto parent = host.model.Back(host.page);
                if (parent == SIZE_MAX) { Close(host); return; }
                host.page = parent;
            }
            ++host.generation;
        }
        else
        {
            auto item = host.items[index];
            if (direction != 0 && item.kind != Kind::Model) return;
            if (!host.details) host.model.Window(host.page).Point(index);
            switch (item.kind)
            {
            case Kind::Page:
            {
                const auto page = host.model.Enter(item.model);
                if (page == SIZE_MAX) return;
                host.page = page;
                ++host.generation;
                break;
            }
            case Kind::Model:
                host.model.Change(item.model, direction, snapshot.gameMode != 11);
                if (!snapshot.Current()) return;
                break;
            default: return;
            }
        }
        snapshot.generation = host.generation; // only our navigation advances it
        if (snapshot.Current() && !Populate(host))
        {
            Log("row population refused; returning to stock Options");
            if (Live(host) && host.open) Close(host);
        }
    }
    inline std::shared_ptr<Host> NoticeHost()
    {
        if (!ready || busy || draining) return {};
        if (auto host = active.lock(); host && Live(*host) && host->open && !host->details && host->manager == currentManager && OwnsFocus(*host, 12))
            return host;
        for (const auto& [scene, host] : hosts)
            if (Live(*host) && !host->open && host->manager == currentManager &&
                Bridge::Read<int32_t>(host->tiered, 592) == 1 && OwnsFocus(*host, 1))
                return host;
        return {};
    }

    inline void ShowNotice(const std::wstring& text)
    {
        auto host = NoticeHost();
        if (!host) return;
        if (!host->open) Open(*host);
        if (!Live(*host) || !host->open || !OwnsFocus(*host, 12)) return;
        host->helpLines = MenuText::Wrap(Upper(text), RowColumns);
        host->helpTitle = Wide(RWGTAMenu::PluginText(FixSettings(), Language(), "update.title"));
        host->helpWindow.Reset();
        host->details = host->notice = true;
        if (!Populate(*host) && Live(*host) && host->open) Close(*host);
    }

    inline decltype(&ExitProcess) OriginalExit = ExitProcess;
    inline void WINAPI ExitNotice(UINT code)
    {
        RWGTA::UpdateCheck::OnExit();
        OriginalExit(code);
    }

    // F1 shows the selected row's whole help and closes it again
    inline void ShowHelp()
    {
        auto host = active.lock();
        if (!host || !Live(*host) || !host->open) return;
        Snapshot snapshot(*host);
        if (!snapshot.Current()) return;
        if (host->details)
        {
            if (host->notice) { Close(*host); return; }
            host->details = false;
            host->helpLines.clear();
            if (!Populate(*host) && Live(*host) && host->open) Close(*host);
            return;
        }
        const auto selected = Bridge::Read<int32_t>(host->list, 576);
        if (selected < 0 || size_t(selected) >= host->count) return;
        const auto item = host->items[selected];
        if (item.kind != Kind::Model) return;
        host->model.Window(host->page).Point(size_t(selected));
        auto lines = MenuText::Wrap(Upper(Wide(host->model.Help(item.model))), RowColumns);
        if (lines.empty() || !snapshot.Current()) return;
        host->helpTitle = item.label;
        host->helpLines = std::move(lines);
        host->helpWindow.Reset();
        host->details = true;
        if (!Populate(*host) && Live(*host) && host->open) Close(*host);
    }

    inline void Drain()
    {
        if (draining) return;
        PreserveBool guard(draining);
        auto events = std::move(pending);
        pending.clear();
        for (auto& event : events)
        {
            auto host = event.host.lock();
            if (!host || !Live(*host) || host->generation != event.generation ||
                host->page != event.page || host->manager != currentManager ||
                Bridge::Read<uint32_t>(host->manager, 0x5A0) != event.inputMask || bridge.GameMode() != event.gameMode) continue;
            if (!OwnsFocus(*host, event.open ? 1 : 12)) continue;
            if (event.open) Open(*host);
            else Activate(*host, event.row, event.direction);
        }
        auto host = active.lock();
        if (!host || !Live(*host) || !host->open || Bridge::Read<int32_t>(host->tiered, 592) != 12 ||
            Bridge::Read<uint8_t>(host->list, 553) != 0)
        {
            NativeMenuWheel = 0; NativeMenuKey = 0;
            return;
        }
        if (!OwnsFocus(*host, 12)) return;
        bool rebuild = RWGTAMenu::Model::Changed().exchange(false);
        const auto follow = std::exchange(host->follow, -1);
        if (host->settle > 0)
        {
            --host->settle;
            if (Bridge::Read<int32_t>(host->list, 576) != host->lastIndex && host->lastIndex >= 0 && size_t(host->lastIndex) < host->count)
                reinterpret_cast<void(__thiscall*)(void*, int32_t, int32_t)>(bridge.selectRow)(host->list, host->lastIndex, 1);
        }
        // Only the row the list has selected now counts
        else if (follow >= 0 && size_t(follow) < host->count && Bridge::Read<int32_t>(host->list, 576) == follow)
            rebuild |= Follow(*host, follow);
        const int notches = wheel.Take(NativeMenuWheel.exchange(0));
        const int key = NativeMenuKey.exchange(0);
        if (notches || key) rebuild |= Navigate(*host, notches, key);
        if (rebuild && !Populate(*host))
        {
            Log("changed row population refused; returning to stock Options");
            if (Live(*host) && host->open) Close(*host);
        }
    }
    inline void Prepare(void* scene, int32_t tier)
    {
        if (!ready || tier != 1 || busy || !currentManager) return;
        PreserveBool guard(busy);
        auto it = hosts.find(scene);
        std::shared_ptr<Host> host;
        if (it != hosts.end()) host = it->second;
        else
        {
            auto tiered = Bridge::Read<void*>(scene, 960);
            if (!tiered || Bridge::Read<int32_t>(tiered, 616) != 12 ||
                !Bridge::Read<void*>(scene, 924) || !Bridge::Read<void*>(scene, 928))
            { Log("options owner/twelve tiers/caption contract unavailable"); return; }
            host = std::make_shared<Host>();
            host->scene = scene;
            host->tiered = tiered;
            host->manager = currentManager;
            hosts[scene] = host;
            Snapshot snapshot(*host);
            bridge.AddTier(tiered);
            if (!snapshot.Current() || Bridge::Read<int32_t>(tiered, 616) != 13)
            { Log("native appended tier creation failed"); return; }
            auto lists = Bridge::Read<void**>(tiered, 612);
            if (!lists || !lists[12]) return;
            host->list = lists[12];
            // Match the native ScenePause::Init configuration of its stock tiers.
            Bridge::Write<int32_t>(host->list, 620, 0);
            Bridge::Write<uint8_t>(host->list, 729, 0);
            Bridge::Write<int32_t>(host->list, 700, 14);
            SetBinding(host->accept, host, 0, reinterpret_cast<void*>(Accepted));
            SetBinding(host->selected, host, 0, reinterpret_cast<void*>(Selected));
            bridge.Connect(Bridge::At(host->list, 604), host->accept.slot, false);
            if (!snapshot.Current()) return;
            bridge.Connect(Bridge::At(host->list, 580), host->selected.slot, true);
            if (!snapshot.Current()) return;
        }
        if (!Live(*host) || !host->list) return;
        Snapshot snapshot(*host);
        auto lists = Bridge::Read<void**>(host->tiered, 612);
        if (!lists || Bridge::Read<int32_t>(host->tiered, 616) != 13) return;
        hostFile.texts.Select(Language());
        bridge.AddRow(lists[1], Wide(hostFile.texts.Resolve("@menu.plugins")), &host->advanced, 15);
    }
    inline uint8_t __fastcall Settings11(void* scene, void*, int32_t tier)
    {
        auto result = settingsHook.thiscall<uint8_t>(scene, tier);
        Prepare(scene, tier);
        return result;
    }
    inline int32_t __stdcall Settings9(void* scene, int32_t tier)
    {
        auto result = settingsHook.stdcall<int32_t>(scene, tier);
        Prepare(scene, tier);
        return result;
    }
    inline bool OwnedCommand(Host& host, int32_t command)
    {
        auto tier = Bridge::Read<int32_t>(host.tiered, 592);
        if (tier != 1 && tier != 12) return false;
        auto lists = Bridge::Read<void**>(host.tiered, 612);
        if (!lists) return false;
        auto list = lists[tier];
        auto index = Bridge::Read<int32_t>(list, 576);
        auto record = bridge.Record(list, index);
        if (!record) return false;
        auto pointer = Bridge::Read<const int32_t*>(record, 64);
        if (command == Advanced) return tier == 1 && pointer == &host.advanced;
        auto row = command - RowCommand;
        return tier == 12 && host.open && row >= 0 && static_cast<size_t>(row) < host.count && pointer == &host.items[row].command;
    }
    inline int32_t __fastcall Perform(void* scene, void*, int32_t command)
    {
        auto it = hosts.find(scene);
        auto host = it == hosts.end() ? nullptr : it->second;
        if (host && Live(*host) && OwnedCommand(*host, command))
        {
            Queue(*host, static_cast<size_t>(command - RowCommand), 0, command == Advanced);
            return 1;
        }
        return performHook.thiscall<int32_t>(scene, command);
    }
    inline int32_t __fastcall Back(void* scene, void*)
    {
        auto it = hosts.find(scene);
        auto host = it == hosts.end() ? nullptr : it->second;
        if (host && Live(*host) && host->open && Bridge::Read<int32_t>(host->tiered, 592) == 12 &&
            Bridge::Read<uint8_t>(scene, 2665) != 0 && Queue(*host, host->count, 0))
        {
            Bridge::Write<uint8_t>(scene, 2665, 0); // consume the same native Back request
            return 1;
        }
        return backHook.thiscall<int32_t>(scene);
    }
    inline void __fastcall Destroy(void* scene, void*)
    {
        auto it = hosts.find(scene);
        if (it != hosts.end())
        {
            auto host = it->second;
            if (active.lock() == host)
            {
                active.reset();
                NativeMenuOpen.store(false);
            }
            host->alive = false;
            ++host->generation;
            host->model.Close();
            DisconnectRows(*host);
            bridge.Disconnect(host->accept.slot);
            bridge.Disconnect(host->selected.slot);
            retired.push_back(host);
            hosts.erase(it);
        }
        destroyHook.thiscall(scene);
    }
    inline uint8_t __fastcall Update(void* manager, void*, Slot* phase, float seconds)
    {
        currentManager = manager;
        ++updateDepth;
        auto result = updateHook.thiscall<uint8_t>(manager, phase, seconds);
        --updateDepth;
        if (!updateDepth && phase == Bridge::At(manager, 932))
        {
            Drain();
            if (NativeHelpRequested.exchange(false)) ShowHelp();
            RWGTA::UpdateCheck::Tick();
            retired.clear();
        }
        return result;
    }
    using Addresses = std::map<std::string, void*>;
    template<size_t N> inline Addresses Resolve(const BlacklistMenuContracts::Contract (&contracts)[N])
    {
        Addresses addresses;
        for (const auto& contract : contracts)
        {
            if (!contract.caller)
                addresses.emplace(contract.name, hook::pattern(contract.pattern).get_first<uint8_t>());
            else
                addresses.emplace(contract.name, reinterpret_cast<void*>(injector::GetBranchDestination(
                    static_cast<uint8_t*>(addresses.at(contract.caller)) + contract.callOffset).as_int()));
        }
        return addresses;
    }
    inline bool Init()
    {
        if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&Init), &self)) return false;
        auto dx9Pattern = hook::pattern(BlacklistMenuContracts::DX9[0].pattern);
        const bool dx9 = !dx9Pattern.empty(); // DX9; the alternative is DX11
        Addresses addresses;
        if (dx9)
            addresses = Resolve(BlacklistMenuContracts::DX9);
        else
            addresses = Resolve(BlacklistMenuContracts::DX11);
        if (!asi_settings::ReadModule(self, hostFile)) return false;
        // Some entries exist in one executable only.
        auto address = [&](const char* name) -> void*
        {
            auto found = addresses.find(name);
            return found != addresses.end() ? found->second : nullptr;
        };
        bridge.dx9 = dx9;
        bridge.addRow = address("AddRow"); bridge.addTier = address("AddTier"); bridge.clearList = address("ClearList");
        bridge.rehash = address("Rehash"); bridge.resizeArray = address("ResizeArray"); bridge.selectRow = address("SelectRow");
        bridge.cloneChoice = address("CloneChoice"); bridge.clearChoices = address("ClearChoices");
        bridge.connect = address("Register"); bridge.connectInt = address("RegisterInt"); bridge.disconnect = address("Disconnect");
        bridge.pushTier = address("PushTier"); bridge.popTier = address("PopTier"); bridge.historyPush = address("HistoryPush");
        bridge.setTitle = address("SetTitle"); bridge.currentLanguage = address("CurrentLanguage"); bridge.record = address("Record");
        bridge.gameMode = address("GameMode"); bridge.levelInfo = address("LevelInfo");
        bridge.topFocus = address("TopFocus");
        bridge.control = address("Control");
        if (dx9)
        {
            using namespace BlacklistMenuABI;
            clearMap9 = address("ClearMap"); hideRows9 = address("HideRows"); cloneChoice9 = bridge.cloneChoice;
            clearChoices9 = bridge.clearChoices; register9 = bridge.connect; registerInt9 = bridge.connectInt;
            popTier9 = bridge.popTier; switchTier9 = address("SwitchTier");
        }
        destroyHook = safetyhook::create_inline(address("Destroy"), Destroy);
        updateHook = safetyhook::create_inline(address("Update"), Update);
        backHook = safetyhook::create_inline(address("Back"), Back);
        performHook = safetyhook::create_inline(address("Perform"), Perform);
        settingsHook = safetyhook::create_inline(address("Settings"), dx9 ? reinterpret_cast<void*>(Settings9) : reinterpret_cast<void*>(Settings11));
        ready = true;
        RWGTA::UpdateCheck::Config update;
        update.pluginId = "SplinterCellBlacklist.FusionFix";
        update.fileVersion = rsc_FileVersion;
        update.settings = &FixSettings();
        update.playable = []() { return bool(NoticeHost()); };
        update.show = ShowNotice;
        update.format = [](const std::string& latest, bool hub)
        {
            auto line = RWGTAMenu::PluginText(FixSettings(), Language(), hub ? "update.availablehub" : "update.availablesite");
            const auto replace = [&](const std::string& token, const std::string& value)
            {
                size_t position = 0;
                while ((position = line.find(token, position)) != std::string::npos)
                {
                    line.replace(position, token.size(), value);
                    position += value.size();
                }
            };
            replace("{name}", RWGTAMenu::PluginText(FixSettings(), Language(), "plugin.name"));
            replace("{version}", latest);
            return Wide(line);
        };
        RWGTA::UpdateCheck::Init(std::move(update));
        auto exits = IATHook::Replace(GetModuleHandleW(nullptr), "KERNEL32.dll", std::forward_as_tuple("ExitProcess", ExitNotice));
        if (auto found = exits.find("ExitProcess"); found != exits.end())
            OriginalExit = reinterpret_cast<decltype(OriginalExit)>(found->second.get());
        return true;
    }
}
