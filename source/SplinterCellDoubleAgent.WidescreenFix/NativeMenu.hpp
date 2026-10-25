#pragma once
#include "FixSettings.hpp"
#include "NativeMenuContracts.hpp"
#include "MenuText.hpp"
#include "../Shared/RWGTA/FixMenu.hpp"

namespace DAMenu
{
    inline HMODULE engineModule = nullptr, hostModule = nullptr;
    inline void* (__stdcall* createArea)(int) = nullptr;
    inline void* (__fastcall* createNode)(void*, void*, int) = nullptr;
    inline void* (__stdcall* createFrame)(int) = nullptr;
    inline void (__fastcall* addNode)(void*, void*, void*) = nullptr;
    inline void (__fastcall* addFrame)(void*, void*, void*) = nullptr;
    inline void (__cdecl* linkNeighbors)(void*) = nullptr;
    inline void (__fastcall* areaTime)(void*, void*, unsigned int, int) = nullptr;
    inline void* (__stdcall* createHandler)(int) = nullptr;
    inline void (__fastcall* addHandler)(void*, void*, void*) = nullptr;
    inline void* (__fastcall* findNode)(void*, void*, uint32_t) = nullptr;
    inline void* (__fastcall* findPage)(void*, void*, uint32_t) = nullptr;
    inline void (__fastcall* addPackageChild)(void*, void*, void*) = nullptr;
    inline void* (__fastcall* wideAssign)(void*, void*, const wchar_t*, size_t) = nullptr;
    inline uint8_t (__fastcall* selectNode)(void*, void*) = nullptr;
    inline uint8_t (__fastcall* blurNode)(void*, void*) = nullptr;
    inline void* (__fastcall* topMenu)(void*, void*) = nullptr;
    inline void* (__fastcall* menuPackage)(void*, void*, const char*) = nullptr;
    inline void* (__fastcall* interaction)(void*, void*) = nullptr;
    inline void (__fastcall* addCursor)(void*, void*, void*) = nullptr;
    inline const wchar_t* (__fastcall* fullName)(void*, void*, wchar_t*) = nullptr;
    inline const wchar_t* (__cdecl* gameLanguage)() = nullptr;
    inline SafetyHookInline displayHook, tickHook, uninitializeHook, destroyHook, defaultsHook;
    inline bool busy = false;
    struct BusyGuard
    {
        bool previous = busy;
        BusyGuard() { busy = true; }
        ~BusyGuard() { busy = previous; }
    };
    inline RWGTAMenu::Model model;

    template<class T> T Read(void* object, size_t offset)
    { return *reinterpret_cast<T*>(static_cast<uint8_t*>(object) + offset); }
    template<class T> void Write(void* object, size_t offset, T value)
    { ProtectedGameRef<T>{reinterpret_cast<T*>(static_cast<uint8_t*>(object) + offset)} = value; }
    template<class Result, class... Args> Result Virtual(void* object, size_t slot, Args... args)
    {
        auto method = Read<void*>(Read<void*>(object, 0), slot * 4);
        return reinterpret_cast<Result (__fastcall*)(void*, void*, Args...)>(method)(object, nullptr, args...);
    }
    inline void Delete(void* object) { if (object) Virtual<void*>(object, 0, int(1)); }
    inline uint32_t Id(std::string_view text)
    {
        uint32_t crc = 0xFFFFFFFF;
        for (auto c : text)
        {
            crc ^= uint8_t(c);
            for (int i = 0; i < 8; ++i) crc = (crc >> 1) ^ ((crc & 1) ? 0xEDB88320 : 0);
        }
        return ~crc;
    }
    inline void LogOnce(const std::string& phase, void*, const std::string& reason)
    {
        static std::set<std::pair<std::string, std::string>> written;
        if (!written.emplace(phase, reason).second) return;
        auto line = "DA native menu " + phase + ": " + reason + "\n";
        OutputDebugStringA(line.c_str());
    }
    inline void* Factory()
    { return Read<void*>(engineModule, layout->factoryRva); }
    inline void* Data(void* node) { return node ? Read<void*>(node, 8) : nullptr; }
    inline int Kind(void* node) { return Data(node) ? Virtual<int>(Data(node), 3) : -1; }
    inline bool Nodes(void* area, std::vector<void*>& out)
    {
        auto start = Read<void**>(area, 12), end = Read<void**>(area, 16), limit = Read<void**>(area, 20);
        auto first = uintptr_t(start), last = uintptr_t(end), capacity = uintptr_t(limit);
        if (last < first || capacity < last || (last - first) % 4 || (last - first) / 4 > 256 || (!start && end)) return false;
        out.clear(); if (start) out.assign(start, end); return true;
    }
    inline std::string Language();
    inline DAText::Metrics TextMetrics(void* node)
    {
        DAText::Metrics metrics;
        auto data = Data(node);
        if (!data || Kind(node) != 8) return metrics;
        auto point = Read<void*>(data, 4);
        auto font = Read<void*>(data, 8);
        if (!point || !font) return metrics;
        const auto baseHeight = Read<uint8_t>(font, 73);
        const auto height = Read<uint16_t>(point, 22);
        if (!baseHeight || !height) return metrics;
        // Native Magma text uses two thirds of the requested font-height ratio.
        const auto scaled = Virtual<uint8_t>(font, 5) != 0;
        metrics.scale = scaled ? double(height) / baseHeight * (2.0 / 3.0) : 1.0;
        metrics.spacing = metrics.scale * Read<int16_t>(point, 18) * baseHeight * 0.001;
        metrics.width = Read<int16_t>(point, 10) - Read<int16_t>(point, 8) - 4.0;
        if (!std::isfinite(metrics.scale) || metrics.scale <= 0 || metrics.scale > 64 ||
            !std::isfinite(metrics.spacing) || !std::isfinite(metrics.width) || metrics.width <= 0) return {};
        metrics.font = font;
        metrics.available = [font](wchar_t c)
        {
            auto page = Read<void*>(font, 88 + (unsigned(c) >> 8) * 4);
            if (!page) return false;
            const auto glyph = Read<uint16_t>(page, (unsigned(c) & 255) * 2);
            return glyph < Read<uint16_t>(font, 80) && (glyph != 0 || c == L' ');
        };
        return metrics;
    }
    inline std::wstring Wide(const std::string& text)
    {
        if (text.size() > 65536) return {};
        int length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), int(text.size()), nullptr, 0);
        if (length <= 0) return {};
        std::wstring wide(length, L'\0');
        if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), int(text.size()), wide.data(), length)) return {};
        return wide;
    }
    inline std::wstring Styled(const std::string& text, const DAText::Metrics& metrics)
    {
        auto wide = Wide(text);
        for (auto& c : wide) if (c < 32 || c == 8232 || c == 8233) c = L' ';
        auto tag = Language();
        std::wstring locale(tag.begin(), tag.end());
        return DAText::Style(std::move(wide), metrics, locale.c_str());
    }
    inline bool CaptionWide(void* node, const std::wstring& text)
    {
        const auto metrics = TextMetrics(node);
        if (!metrics.font) return false;
        const auto fitted = DAText::Fit(text, metrics, metrics.width);
        wideAssign(static_cast<uint8_t*>(Data(node)) + 12, nullptr, fitted.c_str(), fitted.size());
        return true;
    }
    inline bool Caption(void* node, const std::string& text)
    {
        const auto metrics = TextMetrics(node);
        return metrics.font && CaptionWide(node, Styled(text, metrics));
    }
    struct Rect { int16_t left, top, right, bottom; };
    inline Rect Bounds(void* node)
    {
        // Magma stores opposite edges together, unlike Win32 RECT.
        struct NativeRect { int16_t left, right, top, bottom; };
        NativeRect rect{};
        Virtual<void*>(Data(node), 4, &rect);
        return { rect.left, rect.top, rect.right, rect.bottom };
    }
    inline void* Font(void* area, int depth = 0)
    {
        if (!area || depth > 16) return nullptr;
        std::vector<void*> nodes;
        if (!Nodes(area, nodes)) return nullptr;
        for (auto node : nodes)
        {
            auto kind = Kind(node);
            if (kind == 8 && Read<void*>(Data(node), 8)) return Read<void*>(Data(node), 8);
            if (kind == 0 || kind == 1 || kind == 2 || kind == 3 || kind == 4 || kind == 9 || kind == 10 || kind == 12)
                if (auto font = Font(Read<void*>(Data(node), 16), depth + 1)) return font;
        }
        return nullptr;
    }
    inline bool Pause(void* manager, bool& pause)
    {
        auto owner = interaction(manager, nullptr);
        if (!owner) return false;
        wchar_t buffer[1024]{};
        auto name = fullName(owner, nullptr, buffer);
        if (!name) return false;
        std::wstring_view type(name);
        if (type.starts_with(L"SCMagmaInteractionPREGame ")) { pause = false; return true; }
        if (type.starts_with(L"SCMagmaInteractionSPGame ")) { pause = true; return true; }
        return false;
    }
    inline bool Modal(void* manager)
    {
        // Native IsMessageBoxPresent filters a single message type. The
        // verified manager list covers every modal before we handle any input.
        int count = Read<int>(manager, 80);
        return count < 0 || count > 64 || count > 0;
    }
    inline std::string Language()
    {
        const auto& chosen = FixSettings().String("MAIN.TextLanguage");
        if (chosen != "auto") return chosen;
        auto value = gameLanguage ? gameLanguage() : nullptr;
        if (!value) return "en";
        if (_wcsicmp(value, L"fra") == 0) return "fr";
        if (_wcsicmp(value, L"deu") == 0) return "de";
        if (_wcsicmp(value, L"ita") == 0) return "it";
        if (_wcsicmp(value, L"esp") == 0) return "es";
        if (_wcsicmp(value, L"rus") == 0) return "ru";
        return "en";
    }
    inline void Visible(void* node, bool visible)
    {
        auto flags = Read<uint8_t>(node, 24);
        Write(node, 24, uint8_t((flags & ~1) | (visible ? 1 : 0)));
    }
    inline void Enabled(void* node, bool enabled)
    {
        auto flags = Read<uint8_t>(node, 60);
        Write(node, 60, uint8_t((flags & ~2) | (enabled ? 2 : 0)));
    }
    struct TextNode { void* node = nullptr; void* frame = nullptr; };
    inline TextNode Text(void* font, int x, int y, int width, int height, int fontHeight, int align)
    {
        TextNode result;
        auto factory = Factory();
        result.node = factory ? createNode(factory, nullptr, 8) : nullptr;
        auto frame = result.node ? createFrame(8) : nullptr;
        if (!frame) { Delete(result.node); return {}; }
        result.frame = Read<void*>(frame, 4);
        if (!Data(result.node) || !Read<void*>(Data(result.node), 4) || !result.frame || Kind(result.node) != 8)
        { Delete(frame); Delete(result.node); return {}; }
        auto data = Data(result.node);
        Write(data, 8, font);
        Write(data, 40, uint32_t((Read<uint32_t>(data, 40) & ~7) | (align & 7)));
        Write(data, 44, uint8_t(0x10));
        Write(result.frame, 8, int16_t(x)); Write(result.frame, 10, int16_t(x + width));
        Write(result.frame, 12, int16_t(y)); Write(result.frame, 14, int16_t(y + height));
        Write(result.frame, 22, int16_t(fontHeight));
        addFrame(result.node, nullptr, frame);
        return result;
    }
    inline void Color(TextNode& text, uint32_t rgba)
    {
        Write(text.frame, 4, rgba);
        if (auto current = Read<void*>(Data(text.node), 4)) Write(current, 4, rgba);
    }
    struct Button
    {
        void* node = nullptr;
        void* theme = nullptr;
        TextNode label, value;
    };
    inline Button CreateButton(void* font, uint32_t id, int x, int y, int width, int height, int fontHeight, bool value)
    {
        Button result;
        result.theme = createArea(2);
        auto factory = Factory();
        result.node = factory ? createNode(factory, nullptr, 2) : nullptr;
        auto frame = result.node ? createFrame(2) : nullptr;
        if (!result.theme || !result.node || !frame || !Read<void*>(frame, 4) || !Data(result.node) || !Read<void*>(Data(result.node), 4) || Kind(result.node) != 2)
        { Delete(frame); Delete(result.node); Delete(result.theme); return {}; }
        result.label = Text(font, 0, 0, value ? width * 62 / 100 : width, height, fontHeight, 0);
        if (value) result.value = Text(font, width * 62 / 100, 0, width - width * 62 / 100, height, fontHeight, 2);
        if (!result.label.node || (value && !result.value.node))
        {
            Delete(result.label.node); Delete(result.value.node); Delete(frame); Delete(result.node); Delete(result.theme); return {};
        }
        addNode(result.theme, nullptr, result.label.node);
        if (result.value.node) addNode(result.theme, nullptr, result.value.node);
        Write(result.node, 4, id);
        Write(Data(result.node), 16, result.theme);
        auto point = Read<void*>(frame, 4);
        Write(point, 8, int16_t(x)); Write(point, 10, int16_t(y));
        addFrame(result.node, nullptr, frame);
        areaTime(result.theme, nullptr, 0, 1);
        return result;
    }
    inline TextNode Rectangle(void* source)
    {
        if (!source || Kind(source) != 5) return {};
        auto factory = Factory();
        auto node = factory ? createNode(factory, nullptr, 5) : nullptr;
        auto frame = node ? createFrame(5) : nullptr;
        if (!node || !frame) { Delete(frame); Delete(node); return {}; }
        auto data = Data(node);
        auto point = Read<void*>(frame, 4);
        auto current = data ? Read<void*>(data, 4) : nullptr;
        auto originalData = Data(source);
        auto original = Read<void*>(originalData, 4);
        auto image = Read<void*>(originalData, 8);
        if (!image || !data || !point || !current || !original || Kind(node) != 5 ||
            Read<void*>(point, 0) != Read<void*>(original, 0) || Read<void*>(current, 0) != Read<void*>(original, 0))
        { Delete(frame); Delete(node); return {}; }
        // This native parameter is 52 bytes of color, rectangle, transforms and
        // corner colors. Keep the factory's vtable and its independent ownership.
        for (size_t offset = 4; offset < 52; offset += 4)
        {
            const auto value = Read<uint32_t>(original, offset);
            Write(point, offset, value); Write(current, offset, value);
        }
        // Native ImageUnreal destruction releases only its parameter; this
        // package-owned image handle is borrowed just like the native font.
        Write(data, 8, image);
        Write(data, 12, Read<uint8_t>(originalData, 12));
        Write(data, 16, Read<uint8_t>(originalData, 16));
        addFrame(node, nullptr, frame);
        return { node, point };
    }
    struct Item
    {
        RWGTAMenu::Row row;
        std::string label, value, help;
        int link = 0;
        bool enabled = true;
    };
    struct Host : std::enable_shared_from_this<Host>
    {
        void* manager = nullptr;
        void* parent = nullptr;
        void* package = nullptr;
        void* page = nullptr;
        Button entry;
        TextNode entryBackground;
        uint32_t entryTextColor = 0xFFA7D2A7, entrySelectionColor = 0xFFA7D2A7;
        uint32_t selectionColor = 0xFFA7D2A7, normalTextColor = 0xFFDADADA;
        TextNode title, hint, joinedHint;
        bool details = false;
        size_t helpIndex = 0, detailsPage = 0, detailsReturn = 0;
        std::vector<std::wstring> detailsLines;
        RWGTAMenu::Model joinedModel;
        std::vector<Button> joined;
        size_t joinedSelected = SIZE_MAX;
        bool joinedRefresh = false, joinedRestart = false;
        ULONGLONG joinedUpdated = 0;
        std::vector<Button> rows;
        std::vector<Item> items;
        void* savedFocus = nullptr;
        void* preceding = nullptr;
        void* following = nullptr;
        uint32_t previousNext = 0, followingPrevious = 0;
        int direction = 1;
        size_t pageIndex = SIZE_MAX, rootStart = 0, rowStart = 0, selected = 0;
        uint64_t generation = 0;
        bool pause = false, open = false, valid = true, refresh = false;
    };
    inline std::unordered_map<void*, std::shared_ptr<Host>> hosts;
    struct PendingPage { void* manager; void* page; void* owner; unsigned attempts = 0; };
    inline std::vector<PendingPage> pendingPages;
    inline PendingPage* preparingPage = nullptr;
    inline Host* active = nullptr;
    inline constexpr size_t PageRows = 8;
    struct Binding { std::weak_ptr<Host> host; size_t row; bool entry; bool joined = false; };
    inline const std::set<std::string> ControlSettings{ "MAIN.RawInputMouse", "MAIN.MouseSensitivity", "MAIN.RawInputMouseRawData" };
    inline bool Live(const Host& host)
    {
        auto at = hosts.find(host.parent);
        return host.valid && at != hosts.end() && at->second.get() == &host;
    }
    struct Snapshot
    {
        Host& host;
        uint64_t generation;
        Host* owner;
        bool open;
        explicit Snapshot(Host& value) : host(value), generation(value.generation), owner(active), open(value.open) {}
        bool Current() const { return Live(host) && host.generation == generation && active == owner && host.open == open; }
        void Advance() { generation = host.generation; owner = active; open = host.open; }
    };
    enum class Action { Open, Activate, Cancel, JoinedChange, Defaults, Details };
    struct Pending { std::weak_ptr<Host> host; Action action; size_t row; int direction; uint64_t generation; };
    inline std::vector<Pending> pending;
    inline bool Queue(const std::shared_ptr<Host>& host, Action action, size_t row = 0, int direction = 0)
    {
        if (!host || !Live(*host) || pending.size() >= 32) return false;
        pending.push_back({host, action, row, direction, host->generation}); return true;
    }
    inline std::unordered_map<void*, Binding> bindings;
    inline std::array<void*, 14> handlerTable{};
    inline void Populate(Host& host);
    inline void First(Host& host);
    inline void Open(Host& host);
    inline void Close(Host& host);
    inline void Activate(Host& host, size_t selected, int direction);
    inline bool Interactive(Host& host)
    { return Live(host) && host.open && active == &host && topMenu(host.manager, nullptr) == host.page && !Modal(host.manager); }
    inline bool JoinedInteractive(Host& host)
    { return Live(host) && !host.open && !host.joined.empty() && topMenu(host.manager, nullptr) == host.parent && !Modal(host.manager); }
    inline uint8_t __fastcall Defaults(void* controller, void*)
    {
        auto manager = Read<void*>(controller, 24);
        auto parent = topMenu(manager, nullptr);
        auto at = hosts.find(parent);
        auto host = at != hosts.end() ? at->second : std::shared_ptr<Host>{};
        auto generation = host ? host->generation : 0;
        auto result = defaultsHook.thiscall<uint8_t>(controller);
        if (result && host && Live(*host) && host->generation == generation && JoinedInteractive(*host))
            Queue(host, Action::Defaults);
        return result;
    }
    // Native focus/blur each pop one node argument (RET 4). Never invoke
    // plugin callbacks or remove engine objects while native input is unwinding.
    inline uint8_t __fastcall Focused(void* handler, void*, void*)
    {
        auto at = bindings.find(handler);
        if (at != bindings.end() && at->second.entry)
        {
            if (auto host = at->second.host.lock(); host && Live(*host))
            {
                Color(host->entry.label, host->entryBackground.node ? 0xFF000000u : 0xFFFFFFFFu);
                if (host->entryBackground.node) Color(host->entryBackground, host->entrySelectionColor);
            }
        }
        else if (at != bindings.end())
            if (auto host = at->second.host.lock(); host && Live(*host))
            {
                if (at->second.joined) { host->joinedSelected = at->second.row; host->joinedRefresh = true; }
                else
                {
                    host->selected = at->second.row;
                    if (!host->details && host->selected < host->items.size() && !host->items[host->selected].help.empty())
                        host->helpIndex = host->selected;
                    host->refresh = true;
                }
            }
        return 0;
    }
    inline uint8_t __fastcall Blurred(void* handler, void*, void*)
    {
        auto at = bindings.find(handler);
        if (at != bindings.end() && at->second.entry)
        {
            if (auto host = at->second.host.lock(); host && Live(*host))
            {
                Color(host->entry.label, host->entryTextColor);
                if (host->entryBackground.node) Color(host->entryBackground, 0xFF000000u);
            }
        }
        else if (at != bindings.end())
            if (auto host = at->second.host.lock(); host && Live(*host))
            {
                if (at->second.joined) { host->joinedSelected = SIZE_MAX; host->joinedRefresh = true; }
                else host->refresh = true;
            }
        return 0;
    }
    inline uint8_t __fastcall Accept(void* handler, void*, void*)
    {
        auto at = bindings.find(handler);
        if (at == bindings.end() || busy) return 0;
        auto binding = at->second;
        auto host = binding.host.lock();
        if (!host || !Live(*host) || Modal(host->manager)) return 0;
        if (binding.entry) return Queue(host, Action::Open);
        if (binding.joined) return JoinedInteractive(*host) && Queue(host, Action::JoinedChange, binding.row);
        return Interactive(*host) && Queue(host, Action::Activate, binding.row);
    }
    inline uint8_t __fastcall Cancel(void* handler, void*, void*)
    {
        auto at = bindings.find(handler);
        if (at == bindings.end() || at->second.entry || at->second.joined || busy) return 0;
        auto host = at->second.host.lock();
        return host && Interactive(*host) && Queue(host, Action::Cancel);
    }
    inline uint8_t __fastcall Key(void* handler, void*, void*, void* key)
    {
        auto at = bindings.find(handler);
        if (at == bindings.end() || at->second.entry || busy || !key) return 0;
        auto host = at->second.host.lock();
        if (!host || (at->second.joined ? !JoinedInteractive(*host) : !Interactive(*host))) return 0;
        auto code = Read<uint32_t>(key, 0);
        if (code == VK_F1 && !at->second.joined) return Queue(host, Action::Details);
        if (code != 37 && code != 39) return 0;
        return Queue(host, at->second.joined ? Action::JoinedChange : Action::Activate, at->second.row, code == 37 ? -1 : 1);
    }
    inline bool WindowsHelp(bool activate)
    {
        if (busy || !active) return false;
        auto at = hosts.find(active->parent);
        if (at == hosts.end() || at->second.get() != active || !Interactive(*at->second) || at->second->pageIndex == SIZE_MAX) return false;
        if (activate && !at->second->details) Queue(at->second, Action::Details);
        return true;
    }
    inline bool Bind(Host& host, Button& button, size_t row, bool entry, bool joined = false)
    {
        auto handler = createHandler(1);
        if (!handler) return false;
        Write(handler, 0, handlerTable.data());
        bindings.emplace(handler, Binding{host.shared_from_this(), row, entry, joined});
        Write(handler, 4, uint32_t(1));
        addHandler(button.node, nullptr, handler);
        return true;
    }
    inline size_t RootLinks(const Host& host)
    { return std::min(PageRows - 2, model.Pages().size() - std::min(host.rootStart, model.Pages().size())); }
    inline void Hint(Host& host)
    {
        auto retained = host.shared_from_this();
        auto hint = host.selected < host.items.size() ? host.items[host.selected].help : "";
        if (model.RestartRequired()) hint = model.Text("menu.restartwarning");
        Caption(host.hint.node, hint);
    }
    inline void Columns(Button& button, bool value)
    {
        const auto right = Read<int16_t>(button.value.frame, 10);
        const auto left = Read<int16_t>(button.label.frame, 8);
        const auto edge = value ? left + (right - left) * 62 / 100 : right;
        for (auto point : { button.label.frame, Read<void*>(Data(button.label.node), 4) })
            Write(point, 10, int16_t(edge));
        Visible(button.value.node, value);
    }
    inline size_t DetailsPages(const Host& host)
    {
        return std::max(size_t(1), (host.detailsLines.size() + 5) / 6);
    }
    inline void BeginDetails(Host& host)
    {
        auto retained = host.shared_from_this();
        Snapshot snapshot(host);
        if (!Interactive(host) || host.details || host.pageIndex == SIZE_MAX || host.helpIndex >= host.items.size()) return;
        const auto item = host.items[host.helpIndex];
        if (item.help.empty()) return;
        auto metrics = TextMetrics(host.rows.front().label.node);
        metrics.width = Read<int16_t>(host.rows.front().value.frame, 10) - Read<int16_t>(host.rows.front().label.frame, 8) - 4.0;
        if (!metrics.font || !std::isfinite(metrics.width) || metrics.width <= 0) return;
        auto title = item.label + (item.value.empty() ? "" : ": " + item.value);
        auto text = Styled(title, metrics) + L"\n" + Styled(item.help, metrics);
        auto lines = DAText::Wrap(text, metrics.width, [&](const auto& part)
        {
            const auto width = metrics.Measure(part);
            return std::isfinite(width) && (part.empty() || width > 0) ? width : INFINITY;
        });
        if (lines.empty() || !snapshot.Current()) return;
        host.details = true; host.detailsPage = 0; host.detailsReturn = host.helpIndex;
        host.detailsLines = std::move(lines);
        ++host.generation; snapshot.Advance();
        Populate(host); if (snapshot.Current()) First(host);
    }
    inline void EndDetails(Host& host)
    {
        auto retained = host.shared_from_this();
        if (!host.details || !Interactive(host)) return;
        host.details = false; host.detailsLines.clear(); host.selected = host.detailsReturn;
        ++host.generation; Snapshot snapshot(host);
        Populate(host);
        if (!snapshot.Current() || host.selected >= host.rows.size()) return;
        auto node = host.rows[host.selected].node;
        if (auto previous = Read<void*>(host.page, 36)) blurNode(previous, nullptr);
        if (!snapshot.Current()) return;
        Write(host.page, 36, node); selectNode(node, nullptr);
    }
    inline void Populate(Host& host)
    {
        auto retained = host.shared_from_this();
        Snapshot snapshot(host);
        if (!snapshot.Current() || !host.open || !model.IsOpen()) return;
        bool root = host.pageIndex == SIZE_MAX;
        std::vector<Item> items;
        auto title = root ? model.Text("menu.plugins") : model.Pages()[host.pageIndex].title;
        Caption(host.title.node, title);
        if (host.details)
        {
            const auto pages = DetailsPages(host);
            host.detailsPage = std::min(host.detailsPage, pages - 1);
            Caption(host.title.node, model.Text("menu.details") + " (" + std::to_string(host.detailsPage + 1) + "/" + std::to_string(pages) + ")");
            for (size_t i = 0; i < 6; ++i) items.push_back({ {}, "", "", "", -5, false });
            if (pages > 1) items.push_back({ {}, model.Text("menu.next"), "", "", -6 });
        }
        else if (root)
        {
            auto links = RootLinks(host);
            for (size_t i = 0; i < links; ++i) items.push_back({ {}, model.Pages()[host.rootStart + i].title, "", "", int(host.rootStart + i + 1) });
            if (host.rootStart) items.push_back({ {}, model.Text("menu.previous"), "", "", -2 });
            if (host.rootStart + links < model.Pages().size()) items.push_back({ {}, model.Text("menu.next"), "", "", -3 });
        }
        else
        {
            auto rows = model.VisibleRows(host.pageIndex);
            if (!snapshot.Current()) return;
            if (host.rowStart >= rows.size()) host.rowStart = 0;
            for (size_t i = host.rowStart; i < rows.size() && i < host.rowStart + PageRows - 3; ++i)
            {
                auto row = *rows[i];
                auto enabled = model.Enabled(row, host.pause);
                if (!snapshot.Current()) return;
                auto label = model.Label(row);
                if (!snapshot.Current()) return;
                auto value = enabled ? model.Value(row) : model.Text("menu.unavailable");
                if (!snapshot.Current()) return;
                auto help = model.Help(row);
                if (!snapshot.Current()) return;
                items.push_back({ row, std::move(label), std::move(value), std::move(help), 0, enabled });
            }
            if (host.rowStart) items.push_back({ {}, model.Text("menu.previous"), "", "", -2 });
            if (host.rowStart + PageRows - 3 < rows.size()) items.push_back({ {}, model.Text("menu.next"), "", "", -3 });
            items.push_back({ {}, model.Text("menu.details"), "", "", -4 });
        }
        if (!snapshot.Current()) return;
        items.push_back({ {}, model.Text("menu.back"), "", "", -1 });
        host.items = std::move(items);
        size_t count = host.items.size();
        host.selected = std::min(host.selected, count - 1);
        for (size_t i = 0; i < host.rows.size(); ++i)
        {
            auto& button = host.rows[i];
            Visible(button.node, i < count);
            bool enabled = i < count && host.items[i].enabled;
            Enabled(button.node, enabled);
            Columns(button, i < count && !host.items[i].value.empty());
            if (host.details && i < 6)
            {
                const auto line = host.detailsPage * 6 + i;
                CaptionWide(button.label.node, line < host.detailsLines.size() ? host.detailsLines[line] : L"");
            }
            else Caption(button.label.node, i < count ? host.items[i].label : "");
            Caption(button.value.node, i < count ? host.items[i].value : "");
            auto color = host.details && i < 6 ? host.normalTextColor : enabled ? (i == host.selected ? host.selectionColor : host.normalTextColor) : 0xFF777777u;
            Color(button.label, color); Color(button.value, color);
            auto previous = i ? i - 1 : count - 1, next = i + 1 < count ? i + 1 : 0;
            Write(button.node, 28, Read<uint32_t>(host.rows[previous].node, 4));
            Write(button.node, 32, Read<uint32_t>(host.rows[next].node, 4));
            Write(button.node, 36, Read<uint32_t>(button.node, 4));
            Write(button.node, 40, Read<uint32_t>(button.node, 4));
        }
        linkNeighbors(host.page);
        Hint(host);
    }
    inline void First(Host& host)
    {
        auto retained = host.shared_from_this();
        Snapshot snapshot(host);
        for (size_t i = 0; i < host.rows.size(); ++i)
        {
            auto node = host.rows[i].node;
            if ((Read<uint8_t>(node, 24) & 1) && (Read<uint8_t>(node, 60) & 2))
            {
                if (auto previous = Read<void*>(host.page, 36)) blurNode(previous, nullptr);
                if (!snapshot.Current()) return;
                host.selected = i; Write(host.page, 36, node); selectNode(node, nullptr); if (snapshot.Current()) Populate(host); return;
            }
        }
    }
    inline void Open(Host& host)
    {
        auto retained = host.shared_from_this();
        Snapshot snapshot(host);
        if (topMenu(host.manager, nullptr) != host.parent || Modal(host.manager) || !Pause(host.manager, host.pause)) return;
        if (active) Close(*active);
        if (active || !Live(host) || host.generation != snapshot.generation || host.open != snapshot.open) return;
        snapshot.Advance();
        host.savedFocus = Read<void*>(host.parent, 36);
        model.OpenPlugins(hostModule, Language(), 4096, host.joined.empty() ? std::set<std::string>{} : ControlSettings);
        if (!snapshot.Current()) return;
        host.pageIndex = SIZE_MAX; host.rootStart = host.rowStart = 0; host.selected = 0;
        ++host.generation;
        host.details = false; host.detailsLines.clear(); host.helpIndex = 0;
        host.open = true; active = &host; snapshot.Advance();
        Populate(host);
        if (!snapshot.Current()) return;
        displayHook.thiscall<uint8_t>(host.manager, host.page, true, true, int(0));
        if (!snapshot.Current()) return;
        First(host);
        if (!snapshot.Current()) return;
    }
    inline void Close(Host& host)
    {
        auto retained = host.shared_from_this();
        Snapshot snapshot(host);
        if (!snapshot.Current()) return;
        ++host.generation;
        host.open = false; host.details = false; host.detailsLines.clear(); snapshot.Advance();
        if (topMenu(host.manager, nullptr) == host.page)
            displayHook.thiscall<uint8_t>(host.manager, host.page, false, true, int(0));
        if (!snapshot.Current()) return;
        if (active == &host) { active = nullptr; model.Close(); snapshot.Advance(); }
        if (!snapshot.Current()) return;
        if (host.savedFocus && topMenu(host.manager, nullptr) == host.parent)
        {
            Write(host.parent, 36, host.savedFocus); selectNode(host.savedFocus, nullptr);
        }
        if (snapshot.Current()) host.savedFocus = nullptr;
    }
    inline void Activate(Host& host, size_t selected, int direction)
    {
        auto retained = host.shared_from_this();
        Snapshot snapshot(host);
        if (host.details)
        {
            if (direction || (selected < host.items.size() && host.items[selected].link == -6))
            {
                const auto pages = DetailsPages(host);
                host.detailsPage = direction < 0 ? (host.detailsPage + pages - 1) % pages : (host.detailsPage + 1) % pages;
                Populate(host); return;
            }
            if (selected < host.items.size() && host.items[selected].link == -1)
            {
                EndDetails(host); return;
            }
            return;
        }
        if (selected >= host.items.size()) return;
        auto item = host.items[selected];
        bool changedPage = false;
        if (!item.link)
        {
            bool changed = model.Change(item.row, direction, host.pause);
            if (!snapshot.Current()) return;
            if (changed) { ++host.generation; snapshot.Advance(); }
        }
        else if (!direction)
        {
            changedPage = true;
            if (item.link == -1)
            {
                if (host.pageIndex == SIZE_MAX) { Close(host); return; }
                model.PageShown(host.pageIndex); host.pageIndex = SIZE_MAX; host.rowStart = 0;
            }
            else if (item.link == -4) { BeginDetails(host); return; }
            else if (item.link == -2)
            {
                auto& start = host.pageIndex == SIZE_MAX ? host.rootStart : host.rowStart;
                const auto step = host.pageIndex == SIZE_MAX ? PageRows - 2 : PageRows - 3;
                start = start >= step ? start - step : 0;
            }
            else if (item.link == -3) (host.pageIndex == SIZE_MAX ? host.rootStart : host.rowStart) += host.pageIndex == SIZE_MAX ? PageRows - 2 : PageRows - 3;
            else { host.pageIndex = size_t(item.link - 1); host.rowStart = 0; model.PageShown(host.pageIndex); }
        }
        if (!snapshot.Current()) return;
        if (changedPage) { host.helpIndex = 0; ++host.generation; snapshot.Advance(); }
        Populate(host); if (changedPage && snapshot.Current()) First(host);
    }
    inline void* OwningPackage(void* manager, void* parent)
    {
        // PC interactions load a second P_Options in pc_only. Its actual page
        // owns B_Video/B_Control/B_Sound; the options package has a different page.
        // Require exact page identity, including native decorated package names.
        for (auto name : { "pc_only_pc", "pc_only", "options_pc", "options" })
            if (auto package = menuPackage(manager, nullptr, name); package && findPage(package, nullptr, Id("P_Options")) == parent) return package;
        return nullptr;
    }
    inline bool Fits(Rect rect, int width, int height)
    {
        return rect.left >= 0 && rect.top >= 0 && rect.right <= width && rect.bottom <= height &&
            rect.right - rect.left >= 24 && rect.bottom - rect.top >= 10;
    }
    inline bool Empty(void* parent, Rect rect, bool text = false)
    {
        std::vector<void*> nodes;
        if (!Nodes(parent, nodes)) return false;
        for (auto node : nodes)
        {
            auto kind = Kind(node);
            if (kind != 2 && kind != 3 && kind != 4 && kind != 9 && kind != 10 && kind != 12 && !(text && kind == 8)) continue;
            auto occupied = Bounds(node);
            if (rect.left < occupied.right && occupied.left < rect.right && rect.top < occupied.bottom && occupied.top < rect.bottom) return false;
        }
        return true;
    }
    inline void RefreshJoined(Host& host)
    {
        auto retained = host.shared_from_this();
        Snapshot snapshot(host);
        auto& rows = host.joinedModel.JoinedRows();
        if (!JoinedInteractive(host) || rows.size() != host.joined.size()) return;
        for (size_t i = 0; i < rows.size(); ++i)
        {
            auto label = host.joinedModel.Label(rows[i]);
            if (!snapshot.Current()) return;
            auto enabled = host.joinedModel.Enabled(rows[i], host.pause);
            if (!snapshot.Current()) return;
            auto value = enabled ? host.joinedModel.Value(rows[i]) : host.joinedModel.Text("menu.unavailable");
            if (!snapshot.Current()) return;
            auto& button = host.joined[i];
            Caption(button.label.node, label); Caption(button.value.node, value);
            Enabled(button.node, enabled);
            auto color = enabled ? (i == host.joinedSelected ? host.selectionColor : host.normalTextColor) : 0xFF777777u;
            Color(button.label, color); Color(button.value, color);
        }
        auto help = host.joinedRestart || host.joinedModel.RestartRequired() ? host.joinedModel.Text("menu.restartwarning") :
            host.joinedSelected < rows.size() ? host.joinedModel.Help(rows[host.joinedSelected]) : "";
        if (!snapshot.Current()) return;
        Caption(host.joinedHint.node, help);
        host.joinedRefresh = false;
        host.joinedUpdated = GetTickCount64();
    }
    inline bool JoinControls(Host& host, void* font, int width, int height)
    {
        if (!defaultsHook || !host.joined.empty()) return !host.joined.empty();
        auto slider = findNode(host.parent, nullptr, Id("MouseSensitivity "));
        auto label = findNode(host.parent, nullptr, Id("L_MouseSensitivity "));
        auto footer = findNode(host.parent, nullptr, Id("B_Default"));
        auto title = findNode(host.parent, nullptr, Id("L_Title"));
        auto video = findNode(host.parent, nullptr, Id("B_Video"));
        if (!slider || !label || !footer || !title || !video || Kind(slider) != 12 || Kind(label) != 8 || Kind(footer) != 2) return false;
        auto sensor = Bounds(slider), caption = Bounds(label), bottom = Bounds(footer), heading = Bounds(title), first = Bounds(video);
        if (!Fits(sensor, width, height) || !Fits(caption, width, height) || !Fits(bottom, width, height) || !Fits(heading, width, height)) return false;
        int top = std::max(sensor.bottom, caption.bottom) + 6, gap = 4;
        int available = bottom.top - top - 6;
        int rowHeight = std::min(24, (available - gap * int(ControlSettings.size() - 1)) / int(ControlSettings.size()));
        int x = caption.left, rowWidth = width - x * 2, fontHeight = std::min(20, rowHeight - 4);
        Rect hint{ int16_t(x), int16_t(heading.bottom + 4), int16_t(width - x), int16_t(first.top - 8) };
        if (rowHeight < 18 || fontHeight < 14 || rowWidth < 300 || hint.bottom - hint.top < 18 || !Fits(hint, width, height) || !Empty(host.parent, hint, true)) return false;
        std::vector<Rect> places;
        for (size_t i = 0; i < ControlSettings.size(); ++i)
        {
            int y = top + int(i) * (rowHeight + gap);
            Rect rect{ int16_t(x), int16_t(y), int16_t(x + rowWidth), int16_t(y + rowHeight) };
            if (!Fits(rect, width, height) || !Empty(host.parent, rect, true)) return false;
            places.push_back(rect);
        }
        host.joinedModel.OpenPlugins(hostModule, Language(), 4096, ControlSettings);
        if (!Live(host) || host.joinedModel.JoinedRows().size() != places.size()) { host.joinedModel.Close(); return false; }
        struct Cleanup
        {
            Host& host;
            std::vector<Button> buttons;
            TextNode hint;
            bool published = false;
            ~Cleanup()
            {
                if (published) return;
                for (auto& button : buttons) { Delete(button.node); Delete(button.theme); }
                Delete(hint.node);
                std::erase_if(bindings, [this](const auto& item) { return item.second.joined && item.second.host.lock().get() == &host; });
                host.joinedModel.Close();
            }
        } cleanup{ host };
        for (size_t i = 0; i < places.size(); ++i)
        {
            auto rect = places[i];
            auto button = CreateButton(font, Id("WFP_Mouse_" + std::to_string(i)), rect.left, rect.top, rowWidth, rowHeight, fontHeight, true);
            if (!button.node) return false;
            cleanup.buttons.push_back(button);
            if (!Bind(host, cleanup.buttons.back(), i, false, true)) return false;
        }
        cleanup.hint = Text(font, hint.left, hint.top, hint.right - hint.left, hint.bottom - hint.top, 14, 0);
        if (!cleanup.hint.node) return false;
        auto next = Read<uint32_t>(slider, 32);
        auto following = next ? findNode(host.parent, nullptr, next) : footer;
        auto followingId = following ? Read<uint32_t>(following, 4) : Read<uint32_t>(footer, 4);
        for (size_t i = 0; i < cleanup.buttons.size(); ++i)
        {
            auto& button = cleanup.buttons[i];
            Write(button.node, 28, Read<uint32_t>(i ? cleanup.buttons[i - 1].node : slider, 4));
            Write(button.node, 32, i + 1 < cleanup.buttons.size() ? Read<uint32_t>(cleanup.buttons[i + 1].node, 4) : followingId);
            Write(button.node, 36, Read<uint32_t>(button.node, 4)); Write(button.node, 40, Read<uint32_t>(button.node, 4));
            addPackageChild(host.package, nullptr, button.theme); addNode(host.parent, nullptr, button.node);
        }
        addNode(host.parent, nullptr, cleanup.hint.node);
        Write(slider, 32, Read<uint32_t>(cleanup.buttons.front().node, 4));
        if (following && Read<uint32_t>(following, 28) == Read<uint32_t>(slider, 4))
            Write(following, 28, Read<uint32_t>(cleanup.buttons.back().node, 4));
        host.joined = std::move(cleanup.buttons); host.joinedHint = cleanup.hint;
        cleanup.published = true;
        linkNeighbors(host.parent); areaTime(host.parent, nullptr, Read<unsigned int>(host.parent, 24), 0);
        RefreshJoined(host);
        return true;
    }
    inline int NativeHeight(void* node, int fallback)
    {
        if (!node || Kind(node) != 8) return fallback;
        auto point = Read<void*>(Data(node), 4);
        const auto height = point ? Read<uint16_t>(point, 22) : 0;
        return height >= 8 && height <= 64 ? height : fallback;
    }
    inline int CategoryHeight(void* button, int fallback)
    {
        auto body = Data(button);
        auto area = body ? Read<void*>(body, 16) : nullptr;
        std::vector<void*> nodes;
        if (!area || !Nodes(area, nodes)) return fallback;
        for (auto node : nodes) if (Kind(node) == 8) return NativeHeight(node, fallback);
        return fallback;
    }
    inline void AuthoredColors(void* source, TextNode& clone)
    {
        auto list = Read<void*>(source, 12);
        if (!list) return;
        auto first = Read<void**>(list, 4), last = Read<void**>(list, 8);
        const auto begin = uintptr_t(first), end = uintptr_t(last);
        if (!first || end < begin || (end - begin) % 4 || (end - begin) / 4 > 64) return;
        void* best = nullptr;
        unsigned opacity = 0;
        for (auto at = first; at != last; ++at)
        {
            auto point = Read<void*>(*at, 4);
            if (!point || Read<void*>(point, 0) != Read<void*>(clone.frame, 0)) continue;
            unsigned alpha = Read<uint32_t>(point, 4) >> 24;
            for (size_t offset = 36; offset < 52; offset += 4) alpha += Read<uint32_t>(point, offset) >> 24;
            if (!best || alpha > opacity) { best = point; opacity = alpha; }
        }
        if (!best) return;
        for (auto point : { clone.frame, Read<void*>(Data(clone.node), 4) })
        {
            Write(point, 4, Read<uint32_t>(best, 4));
            for (size_t offset = 36; offset < 52; offset += 4) Write(point, offset, Read<uint32_t>(best, offset));
        }
    }
    inline int BodyHeight(void* parent, int fallback)
    {
        std::vector<void*> nodes;
        if (!Nodes(parent, nodes)) return fallback;
        for (auto node : nodes)
        {
            if (Kind(node) != 2) continue;
            auto area = Read<void*>(Data(node), 16);
            std::vector<void*> children;
            if (!area || !Nodes(area, children)) continue;
            std::vector<void*> labels;
            for (auto child : children) if (Kind(child) == 8) labels.push_back(child);
            if (labels.size() >= 2) return NativeHeight(labels.front(), fallback);
        }
        return fallback;
    }
    struct PageLayout
    {
        Rect title, hint;
        int rowY = 0, rowWidth = 0, rowHeight = 0, fontHeight = 0, hintHeight = 0;
    };
    inline std::optional<PageLayout> Layout(Rect panel, Rect title, int fontHeight)
    {
        PageLayout result;
        result.title = title;
        result.fontHeight = fontHeight;
        result.hintHeight = std::max(12, fontHeight - 4);
        result.rowY = title.bottom + 10;
        result.rowWidth = title.right - title.left;
        const int hintBottom = panel.bottom - 8;
        const int hintTop = hintBottom - (result.hintHeight * 2 + 16);
        result.rowHeight = std::min(fontHeight + 12, (hintTop - result.rowY - 8) / int(PageRows + 1));
        result.hint = { title.left, int16_t(hintTop), title.right, int16_t(hintBottom) };
        if (fontHeight < 12 || result.rowHeight < fontHeight + 2 || result.rowWidth < 300 ||
            title.left < panel.left || title.right > panel.right || title.top < panel.top ||
            title.bottom <= title.top || result.rowY + int(PageRows + 1) * result.rowHeight > hintTop) return {};
        return result;
    }
    inline bool Build(Host& host, void* font, int width, int height)
    {
        host.page = createArea(0);
        if (!host.page) return false;
        Write(host.page, 4, Id("P_WFP_Advanced_Options"));
        std::vector<void*> nodes;
        if (!Nodes(host.parent, nodes)) { Delete(host.page); host.page = nullptr; return false; }
        auto heading = findNode(host.parent, nullptr, Id("L_Title"));
        if (!heading || Kind(heading) != 8) { Delete(host.page); host.page = nullptr; return false; }
        const auto title = Bounds(heading);
        Rect panel{};
        int largest = 0;
        size_t rectangles = 0;
        for (auto node : nodes)
        {
            if (Kind(node) != 5) continue;
            auto bounds = Bounds(node);
            if (bounds.left < 0 || bounds.top < 0 || bounds.right > width || bounds.bottom > height ||
                bounds.right <= bounds.left || bounds.bottom <= bounds.top) continue;
            const int area = (bounds.right - bounds.left) * (bounds.bottom - bounds.top);
            if (area > largest) { largest = area; panel = bounds; }
            if (++rectangles > 32) { Delete(host.page); host.page = nullptr; return false; }
            auto clone = Rectangle(node);
            if (!clone.node) { Delete(host.page); host.page = nullptr; return false; }
            AuthoredColors(node, clone);
            addNode(host.page, nullptr, clone.node);
        }
        auto plan = largest ? Layout(panel, title, BodyHeight(host.parent, std::clamp(height / 30, 14, 24))) : std::nullopt;
        if (!plan) { Delete(host.page); host.page = nullptr; return false; }
        const int fontHeight = plan->fontHeight, rowHeight = plan->rowHeight;
        const int x = title.left, rowWidth = plan->rowWidth, y = plan->rowY;
        host.title = Text(font, title.left, title.top, rowWidth, title.bottom - title.top, NativeHeight(heading, fontHeight + 4), 0);
        host.hint = Text(font, plan->hint.left, plan->hint.top, rowWidth, plan->hint.bottom - plan->hint.top, plan->hintHeight, 0);
        if (!host.title.node || !host.hint.node)
        { Delete(host.title.node); Delete(host.hint.node); Delete(host.page); host.page = nullptr; return false; }
        const auto titlePoint = Read<void*>(Data(heading), 4);
        host.selectionColor = Read<uint32_t>(titlePoint, 4) | 0xFF000000u;
        Color(host.title, host.selectionColor);
        if (auto bodyLabel = findNode(host.parent, nullptr, Id("L_MouseSensitivity ")); bodyLabel && Kind(bodyLabel) == 8)
            host.normalTextColor = Read<uint32_t>(Read<void*>(Data(bodyLabel), 4), 4) | 0xFF000000u;
        addNode(host.page, nullptr, host.title.node); addNode(host.page, nullptr, host.hint.node);
        for (size_t i = 0; i < PageRows + 1; ++i)
        {
            auto button = CreateButton(font, Id("WFP_Advanced_Row_" + std::to_string(i)), x, y + int(i) * rowHeight, rowWidth, rowHeight, fontHeight, true);
            if (!button.node || !Bind(host, button, i, false))
            {
                Delete(button.node); Delete(button.theme);
                Delete(host.page); host.page = nullptr;
                for (auto& row : host.rows) Delete(row.theme);
                std::erase_if(bindings, [&host](auto item) { return item.second.host.lock().get() == &host; }); return false;
            }
            addNode(host.page, nullptr, button.node); host.rows.push_back(button);
        }
        for (auto& row : host.rows) addPackageChild(host.package, nullptr, row.theme);
        addPackageChild(host.package, nullptr, host.page);
        areaTime(host.page, nullptr, 0, 1);
        addCursor(host.manager, nullptr, host.page);
        return true;
    }
    inline bool StyleEntry(Host& host, void* source)
    {
        std::vector<void*> children;
        if (!source || Kind(source) != 2 || !Nodes(Read<void*>(Data(source), 16), children)) return false;
        std::vector<TextNode> rectangles;
        TextNode background;
        void* caption = nullptr;
        uint32_t borderColor = 0;
        bool selectionLayer = false;
        for (auto child : children)
        {
            const auto kind = Kind(child);
            if (kind == 8) { caption = child; continue; }
            if (kind != 5) continue;
            auto rect = Bounds(child);
            const bool fill = rect.right - rect.left > 4 && rect.bottom - rect.top > 4;
            // The first image is the stock textured selection overlay. The
            // second uses the same solid image as the native border edges.
            if (fill && !selectionLayer) { selectionLayer = true; continue; }
            if (fill && background.node) continue;
            auto clone = Rectangle(child);
            if (!clone.node)
            {
                for (auto& item : rectangles) Delete(item.node);
                return false;
            }
            rectangles.push_back(clone);
            if (fill)
            {
                background = clone;
                for (auto point : { clone.frame, Read<void*>(Data(clone.node), 4) })
                    for (size_t offset = 36; offset < 52; offset += 4) Write(point, offset, uint32_t(0xFFFFFFFFu));
                Color(rectangles.back(), 0xFF000000u);
            }
            else
            {
                borderColor = Read<uint32_t>(clone.frame, 4) | 0xFF000000u;
                Color(rectangles.back(), borderColor);
            }
        }
        if (!caption || !background.node || rectangles.size() != 5 || !borderColor)
        {
            for (auto& item : rectangles) Delete(item.node);
            return false;
        }
        auto frameList = Read<void*>(host.entry.node, 12);
        auto firstFrame = frameList ? Read<void**>(frameList, 4) : nullptr;
        auto lastFrame = frameList ? Read<void**>(frameList, 8) : nullptr;
        auto sourceButtonPoint = Read<void*>(Data(source), 4);
        if (!firstFrame || lastFrame != firstFrame + 1 || !sourceButtonPoint)
        {
            for (auto& item : rectangles) Delete(item.node);
            return false;
        }
        const auto tint = Read<uint32_t>(sourceButtonPoint, 4) | 0xFF000000u;
        auto ownFramePoint = Read<void*>(*firstFrame, 4);
        auto ownCurrentPoint = Read<void*>(Data(host.entry.node), 4);
        Write(ownFramePoint, 4, tint); Write(ownCurrentPoint, 4, tint);
        const auto sourceBody = Data(caption);
        const auto sourcePoint = Read<void*>(sourceBody, 4);
        const auto labelBody = Data(host.entry.label.node);
        Write(labelBody, 8, Read<void*>(sourceBody, 8));
        Write(labelBody, 40, uint32_t((Read<uint32_t>(labelBody, 40) & ~7) | (Read<uint32_t>(sourceBody, 40) & 7)));
        for (auto point : { host.entry.label.frame, Read<void*>(labelBody, 4) })
            for (size_t offset = 8; offset < 28; offset += 4) Write(point, offset, Read<uint32_t>(sourcePoint, offset));
        host.entryTextColor = Read<uint32_t>(sourcePoint, 4) | 0xFF000000u;
        host.entrySelectionColor = host.entryTextColor;
        host.entryBackground = background;
        Color(host.entry.label, host.entryTextColor);
        for (auto& item : rectangles) addNode(host.entry.theme, nullptr, item.node);
        // All six pointers belong to this private area; put the label after its
        // backdrop, retaining native allocation/destruction of the node vector.
        auto first = Read<void**>(host.entry.theme, 12);
        for (size_t i = 0; i < rectangles.size(); ++i) Write(first, i * sizeof(void*), rectangles[i].node);
        Write(first, rectangles.size() * sizeof(void*), host.entry.label.node);
        areaTime(host.entry.theme, nullptr, 0, 1);
        return true;
    }
    inline bool Prepare(void* manager, void* parent)
    {
        if (hosts.contains(parent)) return true;
        auto package = OwningPackage(manager, parent);
        auto video = findNode(parent, nullptr, Id("B_Video")), controls = findNode(parent, nullptr, Id("B_Control")), sound = findNode(parent, nullptr, Id("B_Sound"));
        bool pause = false;
        if (!package) { LogOnce("prepare", parent, "active options page has no verified owning package"); return false; }
        if (!video || !controls || !sound)
        {
            LogOnce("prepare", parent, "native options buttons missing: video=" + std::to_string(video != nullptr) +
                " controls=" + std::to_string(controls != nullptr) + " sound=" + std::to_string(sound != nullptr));
            return false;
        }
        if (Kind(video) != 2 || Kind(controls) != 2 || Kind(sound) != 2)
        {
            LogOnce("prepare", parent, "native options button kinds: " + std::to_string(Kind(video)) + "," +
                std::to_string(Kind(controls)) + "," + std::to_string(Kind(sound)));
            return false;
        }
        if (!Pause(manager, pause))
        {
            wchar_t buffer[1024]{};
            auto owner = interaction(manager, nullptr);
            auto name = owner ? fullName(owner, nullptr, buffer) : nullptr;
            auto type = name ? std::wstring(name) : L"<unavailable>";
            LogOnce("prepare", parent, "unsupported interaction " + std::string(type.begin(), type.end()));
            return false;
        }
        auto font = Font(Read<void*>(Data(video), 16));
        int width = Read<int16_t>(package, 96), height = Read<int16_t>(package, 98);
        auto first = Bounds(video), last = Bounds(sound);
        if (first.left <= 0 || last.left <= 0) return false;
        if (!font || width < 640 || width > 4096 || height < 475 || height > 2160 || !Fits(first, width, height) || !Fits(last, width, height))
        { LogOnce("prepare", parent, "native font/package/button geometry unavailable"); return false; }
        int rowHeight = std::max(24, int(last.bottom - last.top)), gap = std::max(6, rowHeight / 3);
        int entryWidth = last.right - last.left;
        if (entryWidth < 80) return false;
        std::vector<Rect> candidates = {
            {first.left, int16_t(last.bottom + gap), int16_t(first.left + entryWidth), int16_t(last.bottom + gap + rowHeight)},
            {int16_t(last.right + gap), last.top, int16_t(last.right + gap + entryWidth), last.bottom},
            {int16_t(width / 12), int16_t(height / 10), int16_t(width / 12 + entryWidth), int16_t(height / 10 + rowHeight)}
        };
        auto at = std::find_if(candidates.begin(), candidates.end(), [&](Rect rect) { return Fits(rect, width, height) && Empty(parent, rect); });
        if (at == candidates.end()) { LogOnce("prepare", parent, "no safe native category rectangle"); return false; }
        auto host = std::make_shared<Host>(); host->manager = manager; host->parent = parent; host->package = package; host->pause = pause;
        hosts.emplace(parent, host);
        if (!Build(*host, font, width, height)) { hosts.erase(parent); LogOnce("prepare", parent, "native page/row creation failed"); return false; }
        host->entry = CreateButton(font, Id("WFP_Advanced_Options"), at->left, at->top, at->right - at->left, rowHeight, CategoryHeight(video, std::clamp(height / 30, 14, 24)), false);
        if (!host->entry.node || !Bind(*host, host->entry, 0, true))
        {
            Delete(host->entry.node); Delete(host->entry.theme);
            // The fully constructed native page is already package-owned and
            // stays hidden until its normal package teardown.
            std::erase_if(bindings, [&host](auto item) { return item.second.host.lock().get() == host.get(); });
            hosts.erase(parent);
            LogOnce("prepare", parent, "native category construction failed"); return false;
        }
        if (!StyleEntry(*host, video))
        {
            LogOnce("prepare", parent, "native category decoration unavailable");
            Color(host->entry.label, host->entryTextColor);
        }
        Caption(host->entry.label.node, RWGTAMenu::PluginText(FixSettings(), Language(), "menu.plugins"));
        host->direction = std::abs(first.top - last.top) < rowHeight ? 3 : 1;
        host->preceding = sound;
        host->previousNext = Read<uint32_t>(sound, 28 + host->direction * 4);
        host->following = findNode(parent, nullptr, host->previousNext);
        int previousDirection = host->direction == 3 ? 2 : 0;
        if (host->following) host->followingPrevious = Read<uint32_t>(host->following, 28 + previousDirection * 4);
        Write(host->entry.node, 28 + previousDirection * 4, Read<uint32_t>(sound, 4));
        Write(host->entry.node, 28 + host->direction * 4, host->previousNext ? host->previousNext : Read<uint32_t>(video, 4));
        addPackageChild(package, nullptr, host->entry.theme); addNode(parent, nullptr, host->entry.node);
        Write(sound, 28 + host->direction * 4, Read<uint32_t>(host->entry.node, 4));
        if (host->following && host->followingPrevious == Read<uint32_t>(sound, 4))
            Write(host->following, 28 + previousDirection * 4, Read<uint32_t>(host->entry.node, 4));
        linkNeighbors(parent); areaTime(parent, nullptr, Read<unsigned int>(parent, 24), 0);
        if (!JoinControls(*host, font, width, height)) LogOnce("controls", parent, "native mouse geometry/defaults unavailable; settings remain in Plugins");
        return true;
    }
    inline void Forget(Host& host)
    {
        ++host.generation; host.valid = false; host.open = false; host.details = false; host.detailsLines.clear();
        host.joinedModel.Close();
        if (active == &host) { active = nullptr; model.Close(); }
        std::erase_if(bindings, [&host](auto item) { return item.second.host.lock().get() == &host; });
    }
    inline void QueuePage(void* manager, void* page)
    {
        if (!manager || !page || hosts.contains(page) || pendingPages.size() >= 64) return;
        if (std::any_of(pendingPages.begin(), pendingPages.end(), [&](const auto& item) { return item.manager == manager && item.page == page; })) return;
        if (auto owner = interaction(manager, nullptr)) pendingPages.push_back({ manager, page, owner });
    }
    inline void ForgetPages(void* manager, void* page)
    {
        auto matches = [&](const auto& item) { return (manager && item.manager == manager) || (page && item.page == page); };
        std::erase_if(pendingPages, matches);
        if (preparingPage && matches(*preparingPage)) *preparingPage = {};
    }
    inline uint8_t __fastcall Display(void* manager, void*, void* page, bool show, bool hidePrevious, int priority)
    {
        auto result = displayHook.thiscall<uint8_t>(manager, page, show, hidePrevious, priority);
        if (!busy && result && show && page && Read<uint32_t>(page, 4) == Id("P_Options") && topMenu(manager, nullptr) == page)
            QueuePage(manager, page);
        return result;
    }
    inline void __fastcall Tick(void* object, void*, float delta)
    {
        tickHook.thiscall<void>(object, delta);
        if (busy) return;
        BusyGuard guard;
        // The controller localizes/positions Options after DisplayMenu returns.
        // Wait for its completed tick before borrowing stock styles or geometry.
        for (size_t i = 0; i < pendingPages.size();)
        {
            auto candidate = pendingPages[i];
            if (candidate.owner != object) { ++i; continue; }
            pendingPages.erase(pendingPages.begin() + i);
            if (interaction(candidate.manager, nullptr) != object || topMenu(candidate.manager, nullptr) != candidate.page) continue;
            preparingPage = &candidate;
            struct RetireAttempt { ~RetireAttempt() { preparingPage = nullptr; } } retireAttempt;
            try
            {
                if (!Prepare(candidate.manager, candidate.page) && candidate.page && ++candidate.attempts < 120)
                    pendingPages.push_back(candidate);
            }
            catch (const std::exception& error) { LogOnce("prepare", candidate.page, error.what()); }
            // A native teardown can change the queue during Prepare.
            break;
        }
        // Only here have the native mouse/key dispatch and post-Accept page
        // accesses returned. External setting/action callbacks may destroy UI.
        auto actions = std::move(pending); pending.clear();
        for (auto& action : actions)
        {
            auto host = action.host.lock();
            if (!host || !Live(*host) || action.generation != host->generation) continue;
            if (interaction(host->manager, nullptr) != object) { pending.push_back(action); continue; }
            if (action.action == Action::Defaults && JoinedInteractive(*host))
            {
                Snapshot snapshot(*host);
                auto restart = RWGTAMenu::RestoreDefaults(FixSettings(), ControlSettings);
                if (!snapshot.Current()) continue;
                host->joinedRestart |= restart; RefreshJoined(*host);
            }
            else if (action.action == Action::JoinedChange && JoinedInteractive(*host))
            {
                Snapshot snapshot(*host);
                auto& rows = host->joinedModel.JoinedRows();
                if (action.row >= rows.size()) continue;
                host->joinedModel.Change(rows[action.row], action.direction, host->pause);
                if (!snapshot.Current()) continue;
                RefreshJoined(*host);
            }
            else if (action.action == Action::Open) Open(*host);
            else if (Interactive(*host))
            {
                if (action.action == Action::Activate) Activate(*host, action.row, action.direction);
                else if (action.action == Action::Details) BeginDetails(*host);
                else if (host->details) EndDetails(*host);
                else if (host->pageIndex == SIZE_MAX) Close(*host);
                else { ++host->generation; Snapshot snapshot(*host); host->pageIndex = SIZE_MAX; host->selected = 0; Populate(*host); if (snapshot.Current()) First(*host); }
            }
        }
        std::vector<std::shared_ptr<Host>> joinedHosts;
        auto now = GetTickCount64();
        for (const auto& item : hosts)
        {
            auto& joined = *item.second;
            if (!joined.joined.empty() && interaction(joined.manager, nullptr) == object && JoinedInteractive(joined) &&
                (joined.joinedRefresh || now - joined.joinedUpdated >= 100)) joinedHosts.push_back(item.second);
        }
        for (auto& joined : joinedHosts) if (Live(*joined) && JoinedInteractive(*joined)) RefreshJoined(*joined);
        std::shared_ptr<Host> host;
        if (active) { auto at = hosts.find(active->parent); if (at != hosts.end()) host = at->second; }
        if (host && interaction(host->manager, nullptr) == object && Interactive(*host) && (host->refresh || RWGTAMenu::Model::Changed().exchange(false)))
        {
            Snapshot snapshot(*host);
            host->refresh = false; Populate(*host);
            if (!snapshot.Current()) return;
            auto node = Read<void*>(host->page, 36);
            if (!node || !(Read<uint8_t>(node, 24) & 1) || !(Read<uint8_t>(node, 60) & 2)) First(*host);
        }
    }
    inline uint8_t __fastcall Uninitialize(void* manager, void*)
    {
        BusyGuard guard;
        ForgetPages(manager, nullptr);
        for (auto it = hosts.begin(); it != hosts.end();)
        {
            if (it->second->manager != manager) { ++it; continue; }
            auto host = it->second; it = hosts.erase(it); Forget(*host);
        }
        auto result = uninitializeHook.thiscall<uint8_t>(manager);
        return result;
    }
    inline void* __fastcall Destroy(void* object, void*, uint8_t release)
    {
        ForgetPages(nullptr, object);
        for (auto it = hosts.begin(); it != hosts.end();)
        {
            if (it->second->parent != object && it->second->page != object) { ++it; continue; }
            auto host = it->second; it = hosts.erase(it); Forget(*host);
        }
        return destroyHook.thiscall<void*>(object, release);
    }
    inline void Init()
    {
        auto engine = GetModuleHandleW(L"Engine.dll"), core = GetModuleHandleW(L"Core.dll");
        if (!engine || !core || displayHook) return;
        auto nt = reinterpret_cast<IMAGE_NT_HEADERS*>(reinterpret_cast<uint8_t*>(engine) + reinterpret_cast<IMAGE_DOS_HEADER*>(engine)->e_lfanew);
        if (nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC || nt->OptionalHeader.SizeOfImage < 0xEB5310) return;
        auto ownAddFrame = NativeMenuAddFrameHook(), ownDisplay = NativeMenuDisplayHook();
        layout = Contracts::Select(engine, ownAddFrame, ownDisplay);
        if (!layout)
        {
            LogOnce("init", nullptr, std::string("unsupported Engine contracts: Steam/") +
                Contracts::Mismatch(engine, Contracts::Steam, ownAddFrame, ownDisplay) + "; Retail/" + Contracts::Mismatch(engine, Contracts::Retail, ownAddFrame, ownDisplay));
            return;
        }
        auto address = [engine](Contracts::Index index)
        { return reinterpret_cast<uint8_t*>(engine) + layout->functions[index].rva; };
        auto exported = [engine, &address](const char* name, Contracts::Index contract)
        { return GetProcAddress(engine, name) == reinterpret_cast<FARPROC>(address(contract)); };
        if (!exported("?DisplayMenu@CMenusManager@@QAE_NPAVPage@magma@@_N1W4EMENUPRIORITY@@@Z", Contracts::Display) ||
            !exported("?GetTopMenu@CMenusManager@@QAEPAVPage@magma@@XZ", Contracts::GetTop) ||
            !exported("?Uninitialize@CMenusManager@@UAE_NXZ", Contracts::Uninitialize))
        { LogOnce("init", nullptr, "Engine exported RVA/layout mismatch"); return; }
        fullName = reinterpret_cast<decltype(fullName)>(GetProcAddress(core, "?GetFullName@UObject@@QBEPBGPAG@Z"));
        if (!fullName) fullName = reinterpret_cast<decltype(fullName)>(GetProcAddress(core, "?GetFullName@UObject@@QBEPB_WPA_W@Z"));
        gameLanguage = reinterpret_cast<decltype(gameLanguage)>(GetProcAddress(core, "?GetLanguage@UObject@@SAPBGXZ"));
        if (!gameLanguage) gameLanguage = reinterpret_cast<decltype(gameLanguage)>(GetProcAddress(core, "?GetLanguage@UObject@@SAPB_WXZ"));
        if (!fullName || !gameLanguage) { LogOnce("init", nullptr, "Core name/language exports unavailable"); return; }
        engineModule = engine;
        createArea = reinterpret_cast<decltype(createArea)>(address(Contracts::CreateArea));
        createNode = reinterpret_cast<decltype(createNode)>(address(Contracts::CreateNode));
        createFrame = reinterpret_cast<decltype(createFrame)>(address(Contracts::CreateFrame));
        addNode = reinterpret_cast<decltype(addNode)>(address(Contracts::AddNode));
        addFrame = reinterpret_cast<decltype(addFrame)>(address(Contracts::AddFrame));
        linkNeighbors = reinterpret_cast<decltype(linkNeighbors)>(address(Contracts::LinkNeighbors));
        areaTime = reinterpret_cast<decltype(areaTime)>(address(Contracts::AreaTime));
        createHandler = reinterpret_cast<decltype(createHandler)>(address(Contracts::CreateHandler));
        addHandler = reinterpret_cast<decltype(addHandler)>(address(Contracts::AddHandler));
        findNode = reinterpret_cast<decltype(findNode)>(address(Contracts::FindNode));
        findPage = reinterpret_cast<decltype(findPage)>(address(Contracts::FindPage));
        addPackageChild = reinterpret_cast<decltype(addPackageChild)>(address(Contracts::AddPackageChild));
        wideAssign = reinterpret_cast<decltype(wideAssign)>(address(Contracts::WideAssign));
        selectNode = reinterpret_cast<decltype(selectNode)>(address(Contracts::SelectNode));
        blurNode = reinterpret_cast<decltype(blurNode)>(address(Contracts::BlurNode));
        topMenu = reinterpret_cast<decltype(topMenu)>(address(Contracts::GetTop));
        menuPackage = reinterpret_cast<decltype(menuPackage)>(address(Contracts::GetPackage));
        interaction = reinterpret_cast<decltype(interaction)>(address(Contracts::GetInteraction));
        addCursor = reinterpret_cast<decltype(addCursor)>(address(Contracts::AddCursor));
        auto native = reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(engine) + layout->handlerRva);
        std::copy_n(native, handlerTable.size(), handlerTable.begin());
        handlerTable[2] = reinterpret_cast<void*>(Focused); handlerTable[3] = reinterpret_cast<void*>(Blurred);
        handlerTable[4] = reinterpret_cast<void*>(Accept); handlerTable[5] = reinterpret_cast<void*>(Cancel);
        handlerTable[6] = reinterpret_cast<void*>(Key);
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&hostModule), &hostModule);
        if (!hostModule) { LogOnce("init", nullptr, "host module unavailable"); return; }
        uninitializeHook = safetyhook::create_inline(address(Contracts::Uninitialize), Uninitialize);
        destroyHook = safetyhook::create_inline(address(Contracts::PageDestroy), Destroy);
        tickHook = safetyhook::create_inline(address(Contracts::InputTick), Tick);
        if (!uninitializeHook || !destroyHook || !tickHook)
        { uninitializeHook.reset(); destroyHook.reset(); tickHook.reset(); LogOnce("init", nullptr, "lifetime/tick hook failed"); return; }
        displayHook = safetyhook::create_inline(address(Contracts::Display), Display);
        if (!displayHook) { uninitializeHook.reset(); destroyHook.reset(); tickHook.reset(); }
        if (displayHook)
        {
            auto module = GetModuleHandleW(L"EchelonMenus.dll");
            auto pattern = hook::module_pattern(module, "64 A1 00 00 00 00 6A FF 68 ? ? ? ? 50 64 89 25 00 00 00 00 83 EC 70 53 8B 1D ? ? ? ? 56 57 8B F9 8B 47 18 8B 48 08 85 C9 74 04 FF D3 EB 02 33 C0");
            if (pattern.size() == 1) defaultsHook = safetyhook::create_inline(pattern.get_first(), Defaults);
            if (!defaultsHook) LogOnce("controls", nullptr, "native defaults contract unavailable; settings remain in Plugins");
        }
        if (!displayHook) LogOnce("init", nullptr, "native display hook failed");
    }
}
