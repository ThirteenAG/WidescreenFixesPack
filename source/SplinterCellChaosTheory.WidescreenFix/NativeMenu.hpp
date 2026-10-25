#pragma once
#include "FixSettings.hpp"
#include "Settings.hpp"
#include "MenuLayout.hpp"
#include "MenuText.hpp"
#include "Controls.hpp"
#include "Display.hpp"
#include "../Shared/RWGTA/FixMenu.hpp"
#include <cmath>
#include <cwctype>
#include <functional>
#include <set>
#include <sstream>

// The Plugins page is built from the game's own menu page, list and row objects,
// so it uses the same allocation, input and rendering as the native menus.
namespace CTMenu
{
    inline SafetyHookInline titleBuildHook, pauseBuildHook, acceptHook, cancelHook, keyHook, displayHook, popHook, destructorHook, stackDestructorHook, pushHook;
    inline const wchar_t* (__cdecl* getLanguage)() = nullptr;
    inline void* (__stdcall* makePage)(int) = nullptr;
    inline void* (__fastcall* makeItem)(void*, void*, int) = nullptr;
    inline int (__fastcall* bindStyle)(void*, void*, void*, void*) = nullptr;
    inline int (__fastcall* append)(void*, void*, const wchar_t*, int) = nullptr;
    inline int (__fastcall* clear)(void*, void*) = nullptr;
    inline bool (__fastcall* select)(void*, void*, int16_t, char, char) = nullptr;
    inline unsigned (__fastcall* addChild)(void*, void*, void*) = nullptr;
    inline void* (__fastcall* getStack)(void*, void*) = nullptr;
    inline int (__cdecl* resetFocus)(void*) = nullptr;
    inline char (__fastcall* moveMouse)(void*, void*, const uint32_t*) = nullptr;
    inline HMODULE module = nullptr;
    inline bool busy = false, ready = false;
    struct BusyScope
    {
        bool previous = busy;
        BusyScope() { busy = true; }
        ~BusyScope() { busy = previous; }
    };

    template<class T> T Read(void* object, size_t offset)
    { return *reinterpret_cast<T*>(static_cast<uint8_t*>(object) + offset); }
    template<class T> void Set(void* object, size_t offset, T value)
    { ProtectedGameRef<T>{reinterpret_cast<T*>(static_cast<uint8_t*>(object) + offset)} = value; }
    inline void LogOnce(const std::string& reason)
    {
        static std::set<std::string> written;
        if (!written.insert(reason).second) return;
        OutputDebugStringA(("SCCT native menu: " + reason + "\n").c_str());
    }
    inline std::wstring Wide(const std::string& text)
    {
        auto n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), int(text.size()), nullptr, 0);
        std::wstring out(n, L'\0');
        if (n) MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), int(text.size()), out.data(), n);
        for (auto& c : out) if (c < L' ') c = L' ';
        if (out.size() > 512) { out.resize(512); if (out.back() >= 0xD800 && out.back() <= 0xDBFF) out.pop_back(); }
        return out;
    }
    inline std::string Language()
    {
        auto language = FixSettings().String("MAIN.TextLanguage");
        if (!language.empty() && language != "auto") return language;
        auto native = getLanguage ? getLanguage() : nullptr;
        if (!native) return "en";
        if (!_wcsicmp(native, L"fre") || !_wcsicmp(native, L"fra")) return "fr";
        if (!_wcsicmp(native, L"deu")) return "de";
        if (!_wcsicmp(native, L"ita")) return "it";
        if (!_wcsicmp(native, L"spa") || !_wcsicmp(native, L"esp")) return "es";
        if (!_wcsicmp(native, L"rus")) return "ru";
        return "en"; // Unreal's native "int" resources are English.
    }
    inline int Count(void* list)
    {
        if (!list) return 0;
        auto begin = Read<uintptr_t>(list, 108), end = Read<uintptr_t>(list, 112);
        if (!begin) return 0;
        return end >= begin && end - begin <= 60 * 256 && (end - begin) % 60 == 0 ? int((end - begin) / 60) : -1;
    }
    struct Host
    {
        void* controller = nullptr, * manager = nullptr, * parent = nullptr, * list = nullptr;
        int entry = -1;
        bool pause = false;
        uint64_t serial = 0;
        void* entryPage = nullptr;
        uint64_t entrySerial = 0;
    };
    inline std::map<void*, Host> hosts;
    inline uint64_t nextHostSerial = 0;
    struct ScreenState
    {
        Host host;
        void* page = nullptr, * item = nullptr, * list = nullptr, * stack = nullptr;
        size_t descriptorPage = SIZE_MAX, rootStart = 0, rowStart = 0, selected = 0;
        std::shared_ptr<CTLayout::Templates> templates;
        bool details = false;
        size_t helpIndex = 0, detailsReturn = 0, detailsPage = 0;
        std::vector<std::wstring> detailsLines;
    };
    inline std::optional<ScreenState> screen;
    struct RetiredPage { void* page; void* stack; std::shared_ptr<CTLayout::Templates> templates; };
    inline std::vector<RetiredPage> retired;
    inline RWGTAMenu::Model model;
    inline constexpr size_t RootLinks = 3, PageRows = 5;

    inline bool InStack(void* stack, void* page)
    {
        if (!stack || !page) return false;
        auto begin = Read<uintptr_t>(stack, 4), end = Read<uintptr_t>(stack, 8);
        if (!begin || end < begin || end - begin > 8 * 64 || (end - begin) % 8) return false;
        for (auto at = begin; at != end; at += 8) if (*reinterpret_cast<void**>(at) == page) return true;
        return false;
    }
    inline void* MenuStack()
    {
        auto menuTop = [](void* stack)
        {
            if (!stack) return false;
            auto begin = Read<uintptr_t>(stack, 4), end = Read<uintptr_t>(stack, 8);
            if (!begin || end <= begin || end - begin > 8 * 64 || (end - begin) % 8) return false;
            auto top = *reinterpret_cast<void**>(end - 8);
            return top && ((screen && screen->page == top) || CMenusManager::IsPointerMenuPage(Read<uint32_t>(top, 20)));
        };
        if (screen && menuTop(screen->stack)) return screen->stack;
        for (const auto& [list, host] : hosts)
        {
            auto stack = getStack(host.manager, nullptr);
            if (menuTop(stack)) return stack;
        }
        return nullptr;
    }
    inline bool WindowsMenuMouse(LPARAM position, bool move)
    {
        if (!ready || busy || !moveMouse || Screen.Width <= 0 || Screen.Height <= 0 || GetForegroundWindow() != hGameWindow) return false;
        auto stack = MenuStack();
        if (!stack) return false;
        if (move)
        {
            // Use the same logical bounds as GUI's native mouse conversion.
            // CT's native Windows path may capture relative motion; Magma's
            // verified absolute event supplies the actual menu hover/picking.
            auto delta = int16_t(((480.0f * Screen.fAspectRatio) - 640.0f) / 2.0f);
            auto x = int16_t(std::clamp(float(int16_t(LOWORD(position))) / Screen.Width, 0.0f, 1.0f) * (640 + 2 * delta) - delta);
            auto y = int16_t(std::clamp(float(int16_t(HIWORD(position))) / Screen.Height, 0.0f, 1.0f) * 480);
            uint32_t event[] = { 1, uint16_t(x) | (uint32_t(uint16_t(y)) << 16) };
            BusyScope scope;
            moveMouse(stack, nullptr, event);
        }
        return true;
    }
    inline void DestroyPage(void* page)
    {
        if (!page) return;
        auto fn = Read<void**>(page, 0)[0];
        reinterpret_cast<void* (__fastcall*)(void*, void*, char)>(fn)(page, nullptr, 1);
    }
    inline void Retire()
    {
        if (!screen) return;
        // KeyRelease resets these two indices after Accept returns. Its reset
        // helpers animate borrowed prototypes only when an index changes.
        // Set both to -1 before any package teardown can free those prototypes;
        // the post-Accept resets then touch only our still-live list fields.
        Set<int16_t>(screen->list, 170, -1);
        Set<int16_t>(screen->list, 172, -1);
        // Native hover/leave dispatchers check this active bit after invoking
        // handlers. Retirement during a handler must not animate freed source
        // prototypes when those dispatchers continue with our retained list.
        Set<uint8_t>(screen->list, 180, uint8_t(Read<uint8_t>(screen->list, 180) & ~2u));
        retired.push_back({ screen->page, screen->stack, screen->templates }); screen.reset(); model.Close();
    }
    inline void Frame(void* updatingPage)
    {
        // A click/key handler may continue using its list after Accept returns.
        // Free only on the next native page update, after input has unwound.
        if (!busy)
        {
            BusyScope scope;
            std::erase_if(retired, [updatingPage](const RetiredPage& old)
            {
                if (old.page == updatingPage || InStack(old.stack, old.page)) return false;
                DestroyPage(old.page); return true;
            });
        }
    }
    inline void Close()
    {
        if (!screen) return;
        auto manager = screen->host.manager, page = screen->page;
        BusyScope scope;
        displayHook.thiscall<void>(manager, page, char(0), char(1), char(0));
        if (screen && screen->page == page) Retire();
    }
    inline size_t Links()
    { return std::min(RootLinks, model.Pages().size() - std::min(screen->rootStart, model.Pages().size())); }
    inline void* TextElement(void* page, unsigned depth = 0)
    {
        if (!page || depth > 4) return nullptr;
        auto begin = Read<uintptr_t>(page, 8), end = Read<uintptr_t>(page, 12);
        if (!begin || end < begin || end - begin > 4 * 64 || (end - begin) % 4) return nullptr;
        for (auto at = begin; at < end; at += 4)
        {
            auto item = *reinterpret_cast<void**>(at);
            if (!item) continue;
            auto element = Read<void*>(item, 8);
            if (!element) continue;
            auto type = reinterpret_cast<int (__fastcall*)(void*, void*)>(Read<void**>(element, 0)[3])(element, nullptr);
            if (type == 8 && Read<void*>(element, 40)) return element;
            if (type == 2 || type == 3 || type == 4)
                if (auto found = TextElement(element, depth + 1)) return found;
        }
        return nullptr;
    }
    using CTText::Metrics;
    using CTText::Fit;
    inline Metrics PrototypeMetrics(void* prototype)
    {
        Metrics result;
        auto element = TextElement(prototype);
        if (!element) { LogOnce("native row text/font metrics unavailable"); return result; }
        result.font = Read<void*>(element, 40);
        auto point = Read<void*>(element, 4);
        auto vt = Read<void**>(result.font, 0);
        if (!vt[3] || !vt[4] || !vt[5] || !vt[6]) { result.font = nullptr; return result; }
        auto height = reinterpret_cast<int (__fastcall*)(void*, void*)>(vt[6])(result.font, nullptr);
        if (!point || height <= 0) { result.font = nullptr; return result; }
        auto scalable = reinterpret_cast<char (__fastcall*)(void*, void*)>(vt[5])(result.font, nullptr);
        if (scalable && height > 0) result.scale = Read<uint16_t>(point, 22) / double(height);
        result.spacing = result.scale * height * Read<int16_t>(point, 18) * 0.001;
        // Bound even a free-width prototype to the private 490-unit text span.
        // A fixed-width native text rectangle supplies a tighter limit.
        auto width = Read<int16_t>(point, 10) - Read<int16_t>(point, 8);
        if (!(Read<uint8_t>(element, 88) & 0x40) && width > 0) result.width = std::min(result.width, double(width));
        return result;
    }
    inline Metrics RowMetrics(void* list)
    {
        return PrototypeMetrics(Read<void*>(list, 44));
    }
    inline bool SettingsTextFits(void* body, double width)
    {
        auto metrics = PrototypeMetrics(Read<void*>(body, 28));
        const auto length = Read<uint32_t>(body, 60), capacity = Read<uint32_t>(body, 64);
        if (!metrics.font || length > 512 || capacity < length || capacity > 16384)
            return false;
        auto text = capacity < 8 ? reinterpret_cast<const wchar_t*>(static_cast<uint8_t*>(body) + 44) : Read<const wchar_t*>(body, 44);
        return text && metrics.Measure(std::wstring(text, length)) <= width;
    }
    inline std::wstring SettingsCaption(void* body, double width)
    {
        auto metrics = PrototypeMetrics(Read<void*>(body, 28));
        auto text = Wide(RWGTAMenu::PluginText(FixSettings(), Language(), "menu.plugins"));
        if (!metrics.font || text.empty())
            return {};
        text = CTText::Style(std::move(text), metrics, Wide(Language()).c_str());
        return metrics.Measure(text) <= width ? text : std::wstring{};
    }
    inline std::wstring Styled(const std::string& text, const Metrics& metrics)
    {
        return CTText::Style(Wide(text), metrics, Wide(Language()).c_str());
    }
    inline void RowWide(const std::wstring& wide, bool disabled)
    {
        auto index = append(screen->list, nullptr, wide.c_str(), 0);
        if (index >= 0 && index < Count(screen->list))
            Set<uint8_t>(Read<void*>(screen->list, 108), size_t(index) * 60 + 53, disabled ? 1 : 0);
    }
    inline void Row(const std::string& text, bool disabled)
    {
        auto metrics = RowMetrics(screen->list);
        RowWide(Fit(Styled(text, metrics), metrics, metrics.width), disabled);
    }
    inline bool SettingRow(const RWGTAMenu::Row& row, bool enabled, void* pageToken)
    {
        if (!screen || screen->page != pageToken) return false;
        auto labelText = model.Label(row);
        auto valueText = enabled ? model.Value(row) : model.Text("menu.unavailable");
        // A plugin API may synchronously rebuild the native source package.
        // Reacquire the font only after its page identity has been checked.
        if (!screen || screen->page != pageToken) return false;
        auto metrics = RowMetrics(screen->list);
        auto label = Styled(labelText, metrics);
        RowWide(CTText::Setting(label, Styled(valueText, metrics), metrics), !enabled);
        return true;
    }
    inline void Populate();
#include "Details.hpp"
    inline bool WindowsMenuHelp(bool pressed)
    {
        if (!ready || busy || !screen || screen->descriptorPage == SIZE_MAX || GetForegroundWindow() != hGameWindow) return false;
        const auto stack = MenuStack();
        if (!stack || stack != screen->stack) return false;
        const auto begin = Read<uintptr_t>(stack, 4), end = Read<uintptr_t>(stack, 8);
        if (!begin || end < begin + 8 || end - begin > 8 * 64 || (end - begin) % 8 ||
            Read<void*>(reinterpret_cast<void*>(end - 8), 0) != screen->page) return false;
        if (pressed)
        {
            BusyScope scope;
            if (screen->details) EndDetails();
            else BeginDetails();
        }
        return true;
    }
    inline void Populate()
    {
        if (!screen) return;
        if (screen->details) { PopulateDetails(); return; }
        auto pageToken = screen->page;
        auto root = screen->descriptorPage == SIZE_MAX;
        auto rows = root ? std::vector<const RWGTAMenu::Row*>{} : model.VisibleRows(screen->descriptorPage);
        if (!screen || screen->page != pageToken) return;
        auto links = root ? Links() : 0;
        clear(screen->list, nullptr);
        Row(root ? model.Text("menu.plugins") : model.Pages()[screen->descriptorPage].title, true);
        if (root)
        {
            for (size_t i = 0; i < links; ++i) Row(model.Pages()[screen->rootStart + i].title, false);
            if (screen->rootStart) Row(model.Text("menu.previous"), false);
            if (screen->rootStart + links < model.Pages().size()) Row(model.Text("menu.next"), false);
        }
        else
        {
            screen->rowStart = rows.empty() ? 0 : std::min(screen->rowStart, ((rows.size() - 1) / PageRows) * PageRows);
            auto count = std::min(PageRows, rows.size() - screen->rowStart);
            for (size_t i = 0; i < count; ++i)
            {
                auto row = rows[screen->rowStart + i];
                auto enabled = model.Enabled(*row, screen->host.pause);
                if (!screen || screen->page != pageToken) return;
                if (!SettingRow(*row, enabled, pageToken)) return;
            }
            if (screen->rowStart) Row(model.Text("menu.previous"), false);
            if (screen->rowStart + count < rows.size()) Row(model.Text("menu.next"), false);
            if (count)
            {
                if (screen->selected < count) screen->helpIndex = screen->selected;
                screen->helpIndex = std::min(screen->helpIndex, count - 1);
                Row(model.Text("menu.details"), false);
            }
            if (model.RestartRequired()) Row(model.Text("menu.restartwarning"), true);
            else if (count) Row(model.Help(*rows[screen->rowStart + screen->helpIndex]), true);
            else Row("", true);
        }
        Row(model.Text("menu.back"), false);
        auto count = Count(screen->list) - 1;
        screen->selected = std::min(screen->selected, size_t(std::max(0, count - 1)));
        auto rowData = Read<void*>(screen->list, 108);
        auto chosen = int(screen->selected + 1);
        while (chosen < Count(screen->list) - 1 && Read<uint8_t>(rowData, size_t(chosen) * 60 + 53)) ++chosen;
        screen->selected = size_t(chosen - 1);
        select(screen->list, nullptr, int16_t(chosen), 1, 1);
    }
    inline void Action(int direction)
    {
        if (!screen) return;
        auto pageToken = screen->page;
        auto selected = Read<int16_t>(screen->list, 168);
        if (selected < 1 || selected >= Count(screen->list)) return;
        if (screen->details)
        {
            if (direction) StepDetails(direction);
            else if (DetailPages() > 1 && selected == int(DetailLines + 1)) StepDetails(1);
            else EndDetails();
            return;
        }
        screen->selected = size_t(selected - 1);
        if (screen->descriptorPage != SIZE_MAX)
        {
            auto rows = model.VisibleRows(screen->descriptorPage);
            if (!screen || screen->page != pageToken) return;
            auto count = std::min(PageRows, rows.size() - std::min(screen->rowStart, rows.size()));
            if (screen->selected < count)
            {
                model.Change(*rows[screen->rowStart + screen->selected], direction, screen->host.pause);
                if (!screen || screen->page != pageToken) return;
            }
            else if (!direction)
            {
                auto at = screen->selected - count;
                if (screen->rowStart && !at) screen->rowStart -= PageRows;
                else
                {
                    if (screen->rowStart) --at;
                    const bool next = screen->rowStart + count < rows.size();
                    if (next && !at) screen->rowStart += PageRows;
                    else
                    {
                        if (next) --at;
                        if (count && !at) { BeginDetails(); return; }
                        screen->descriptorPage = SIZE_MAX; screen->rowStart = 0;
                    }
                }
                screen->selected = 0;
            }
        }
        else if (!direction)
        {
            auto links = Links(), at = screen->selected;
            if (at < links) { screen->descriptorPage = screen->rootStart + at; screen->rowStart = 0; screen->selected = 0; model.PageShown(screen->descriptorPage); }
            else
            {
                at -= links;
                if (screen->rootStart && !at) { screen->rootStart -= RootLinks; screen->selected = 0; }
                else
                {
                    if (screen->rootStart) --at;
                    if (screen->rootStart + links < model.Pages().size() && !at) { screen->rootStart += RootLinks; screen->selected = 0; }
                    else { Close(); return; }
                }
            }
        }
        Populate();
    }
    inline bool ListContract(void* list, bool ownsBindings = false)
    {
        if (!list || Count(list) < 0 || !Read<void*>(list, 4) || !Read<void*>(list, 44) || !Read<void*>(list, 40)) return false;
        // Three style blocks own their font strings but borrow package prototypes.
        // The native destructor frees +52/+72/+92, never +44/+64/+84.
        // Loaded resource lists discard those binding strings in 10D66E20;
        // constructor-created private lists keep their independently owned ones.
        // StyleBind explicitly accepts a null definition with an empty string.
        for (auto offset : { 36u, 56u, 76u })
            if ((ownsBindings && !Read<void*>(list, offset + 16)) ||
                (Read<void*>(list, offset + 8) && !Read<void*>(list, offset + 4))) return false;
        return true;
    }
    inline void Open(Host host)
    {
        if (screen) Close();
        auto source = hosts.find(host.list);
        if (source == hosts.end() || source->second.serial != host.serial ||
            (host.entryPage && !CTSettings::Lookup(host.entryPage, host.entrySerial)))
        { LogOnce("native source host changed while closing previous page"); return; }
        if (!ListContract(host.list)) { LogOnce("native list/template contract unavailable"); return; }
        auto templates = std::make_shared<CTLayout::Templates>();
        templates->pages[0] = CTLayout::Clone(Read<void*>(host.list, 44), 500);
        if (!templates->pages[0]) { LogOnce("native wider row template contract unavailable"); return; }
        auto page = makePage(0), item = makeItem(nullptr, nullptr, 10);
        if (!page || !item)
        {
            if (page) DestroyPage(page);
            if (item) DestroyPage(item);
            LogOnce("native page/item allocation failed"); return;
        }
        auto list = Read<void*>(item, 8);
        if (!list) { DestroyPage(item); DestroyPage(page); LogOnce("native list allocation failed"); return; }
        if (!ListContract(list, true) && !Read<void*>(list, 44))
        {
            // A new list has no borrowed prototypes until the styles are bound.
            for (auto offset : { 36u, 56u, 76u })
            {
                if (!Read<void*>(host.list, offset + 8)) continue;
                Set<void*>(list, offset + 4, Read<void*>(host.list, offset + 4));
                bindStyle(static_cast<uint8_t*>(list) + offset, nullptr,
                    Read<void*>(host.list, offset + 12), offset == 36 ? templates->pages[0] : Read<void*>(host.list, offset + 8));
            }
        }
        auto sourcePoint = Read<void*>(host.list, 4), targetPoint = Read<void*>(list, 4);
        Set<uint32_t>(targetPoint, 4, Read<uint32_t>(sourcePoint, 4));
        Set<uint32_t>(targetPoint, 8, uint16_t(70) | (uint32_t(110) << 16));
        Set<int8_t>(list, 164, Read<int8_t>(host.list, 164));
        Set<uint8_t>(list, 166, 12);
        Set<uint32_t>(list, 176, Read<uint32_t>(host.list, 176));
        Set<uint8_t>(list, 180, uint8_t(Read<uint8_t>(host.list, 180) | 0x42));
        Set<uint8_t>(list, 184, Read<uint8_t>(host.list, 184));
        Set<uint32_t>(item, 4, crc32(0, "WFP.AdvancedOptions.List", 24));
        Set<uint32_t>(page, 20, crc32(0, "WFP.AdvancedOptions", 19));
        addChild(page, nullptr, item); // Native page now owns wrapper/list.
        auto stack = getStack(host.manager, nullptr);
        if (!ListContract(list, true) || !stack || !RowMetrics(list).font)
        { DestroyPage(page); LogOnce("native private page style/stack contract failed"); return; }
        const auto controlsJoined = CTControls::JoinedFor(host.manager);
        const auto displayJoined = CTDisplay::JoinedFor(host.manager);
        auto joined = controlsJoined ? CTControls::Settings : std::set<std::string>{};
        if (displayJoined) joined.insert(CTDisplay::Settings.begin(), CTDisplay::Settings.end());
        model.OpenPlugins(module, Language(), 4096, joined);
        auto currentHost = hosts.find(host.list);
        if (currentHost == hosts.end() || currentHost->second.serial != host.serial ||
            controlsJoined != CTControls::JoinedFor(host.manager) ||
            displayJoined != CTDisplay::JoinedFor(host.manager) ||
            (host.entryPage && !CTSettings::Lookup(host.entryPage, host.entrySerial)))
        {
            // Plugin discovery/Subscribe/Get may synchronously unload/rebuild
            // the native source package before our page enters its stack.
            model.Close(); DestroyPage(page); LogOnce("native source host changed during model discovery"); return;
        }
        screen.emplace(); screen->host = host; screen->page = page; screen->item = item; screen->list = list; screen->stack = stack; screen->templates = std::move(templates);
        auto current = [&]
        {
            auto binding = hosts.find(host.list);
            return screen && screen->page == page && binding != hosts.end() && binding->second.serial == host.serial &&
                (!host.entryPage || CTSettings::Lookup(host.entryPage, host.entrySerial));
        };
        auto abandon = [&]
        {
            if (!screen || screen->page != page) return;
            if (InStack(screen->stack, page)) popHook.thiscall<void>(screen->stack, char(0), char(0));
            if (screen && screen->page == page) Retire();
        };
        BusyScope scope; Populate();
        if (!current()) { abandon(); return; }
        resetFocus(page);
        if (!current()) { abandon(); return; }
        displayHook.thiscall<void>(host.manager, page, char(1), char(1), char(0));
        if (!current()) { abandon(); return; }
        if (!InStack(stack, page)) { abandon(); LogOnce("native stack rejected private page"); return; }
    }
    inline void Entry(void* controller, bool pause)
    {
        if (!ready) return;
        auto list = Read<void*>(controller, pause ? 80 : 68), parent = Read<void*>(controller, 56);
        auto count = Count(list);
        if (!parent || !ListContract(list) || count != (pause ? 6 : 7))
        {
            LogOnce(pause ? "pause menu layout unsupported" : "title menu layout unsupported");
            return;
        }
        Host host{ controller, Read<void*>(controller, 20), parent, list, -1, pause };
        host.serial = ++nextHostSerial;
        hosts[list] = host;
    }

    inline int __fastcall TitleBuild(void* object, void*)
    { auto result = titleBuildHook.thiscall<int>(object); Entry(object, false); return result; }
    inline int __fastcall PauseBuild(void* object, void*)
    { auto result = pauseBuildHook.thiscall<int>(object); Entry(object, true); return result; }
    inline bool SettingsCurrent(void* page, void* manager)
    {
        auto stack = getStack(manager, nullptr);
        if (!stack) return false;
        const auto begin = Read<uintptr_t>(stack, 4), end = Read<uintptr_t>(stack, 8);
        return begin && end > begin && end - begin <= 8 * 64 && (end - begin) % 8 == 0 &&
            *reinterpret_cast<void**>(end - 8) == page;
    }
    inline void OpenSettings(void* controller, void* page)
    {
        auto manager = Read<void*>(controller, 20);
        if (!SettingsCurrent(page, manager)) return;
        std::optional<Host> source;
        for (const auto& [list, host] : hosts)
        {
            if (host.manager != manager) continue;
            if (source) { LogOnce("Settings source template is ambiguous"); return; }
            source = host;
        }
        if (!source) { LogOnce("Settings source template unavailable"); return; }
        auto entry = CTSettings::bindings.find(page);
        if (entry == CTSettings::bindings.end() || !CTSettings::Live(entry->second)) return;
        source->entryPage = page;
        source->entrySerial = entry->second.serial;
        Open(*source);
    }
    inline void RetireSettings(void* page)
    {
        CTControls::Forget(page);
        CTDisplay::Forget(page);
        if (!screen || screen->host.entryPage != page) return;
        auto privatePage = screen->page;
        BusyScope scope;
        if (InStack(screen->stack, privatePage))
            popHook.thiscall<void>(screen->stack, char(0), char(0));
        if (screen && screen->page == privatePage) Retire();
    }
    inline char __fastcall Accept(void* item, void*)
    {
        if (CTDisplay::Accept(item)) return 1;
        if (screen && item == screen->item) { BusyScope scope; Action(0); return 1; }
        for (const auto& [object, host] : hosts)
        {
            if (host.entry >= 0 && Read<void*>(item, 8) == host.list && Read<int16_t>(host.list, 168) == host.entry)
            { Open(host); return screen ? 1 : 0; }
        }
        return acceptHook.thiscall<char>(item);
    }
    inline char __fastcall Cancel(void* item, void*)
    {
        if (CTDisplay::Cancel(item)) return 1;
        if (screen && item == screen->item)
        {
            BusyScope scope;
            if (screen->details) EndDetails();
            else if (screen->descriptorPage == SIZE_MAX) Close();
            else { screen->descriptorPage = SIZE_MAX; screen->rowStart = 0; screen->selected = 0; Populate(); }
            return 1;
        }
        return cancelHook.thiscall<char>(item);
    }
    inline char __fastcall Key(void* list, void*, void* item, uint32_t* event)
    {
        if (CTControls::Key(list, item, event)) return 1;
        if (screen && list == screen->list && event && *event == VK_F1)
        { BusyScope scope; if (screen->details) EndDetails(); else BeginDetails(); return 1; }
        if (screen && list == screen->list && event && (*event == VK_LEFT || *event == VK_RIGHT))
        { BusyScope scope; Action(*event == VK_LEFT ? -1 : 1); return 1; }
        return keyHook.thiscall<char>(list, item, event);
    }
    inline char __fastcall Display(void* manager, void*, void* page, char show, char modal, char priority)
    {
        CTDisplay::Leaving(manager, page);
        // busy protects input lifetime, not the native navigation invariant:
        // foreign callbacks must dismiss ours even while Accept/hover unwinds.
        if (screen && manager == screen->host.manager && page != screen->page) Close();
        return displayHook.thiscall<char>(manager, page, show, modal, priority);
    }
    inline void __fastcall Pop(void* stack, void*, char all, char priority)
    {
        popHook.thiscall<void>(stack, all, priority);
        if (screen && stack == screen->stack && !InStack(stack, screen->page)) Retire();
    }
    inline void __fastcall Push(void* stack, void*, void* page, char modal, char priority)
    {
        // Cover direct native stack users as well as DisplayMenu. Our page is
        // always topmost until native navigation dismisses it.
        if (screen && stack == screen->stack && page != screen->page) Close();
        pushHook.thiscall<void>(stack, page, modal, priority);
    }
    inline int __fastcall PageDestroyed(void* page, void*)
    {
        CTSettings::Forget(page);
        RetireSettings(page);
        // Unlink before the source package frees the prototypes borrowed by our
        // list. The independently owned stack is destroyed before its package
        // pages (UserContext dtor); StackDestroyed drains our pages at that gate.
        std::erase_if(hosts, [page](const auto& pair) { return pair.second.parent == page; });
        if (screen && screen->host.parent == page)
        {
            auto privatePage = screen->page;
            BusyScope scope;
            if (InStack(screen->stack, screen->page))
                popHook.thiscall<void>(screen->stack, char(0), char(0));
            if (screen && screen->page == privatePage) Retire();
        }
        return destructorHook.thiscall<int>(page);
    }
    inline int __fastcall StackDestroyed(void* stack, void*)
    {
        CTSettings::ForgetStack(stack);
        auto inputUnwinding = busy;
        BusyScope scope;
        if (screen && screen->stack == stack) Retire();
        std::erase_if(retired, [stack, inputUnwinding](RetiredPage& old)
        {
            if (old.stack != stack) return false;
            // An invoked sibling action may tear down menus from Accept.
            // Keep its list alive until input unwinds, but forget the stack
            // that the native destructor is about to release.
            if (inputUnwinding) { old.stack = nullptr; return false; }
            DestroyPage(old.page); return true;
        });
        return stackDestructorHook.thiscall<int>(stack);
    }
    inline void Tick(void* page)
    {
        Frame(page);
        if (!busy && screen && page == screen->page && !screen->details)
        {
            auto selected = Read<int16_t>(screen->list, 168);
            auto moved = selected > 0 && screen->selected != size_t(selected - 1);
            if (moved) screen->selected = size_t(selected - 1);
            if (moved || RWGTAMenu::Model::Changed().exchange(false)) { BusyScope scope; Populate(); }
        }
        if (!busy) { CTControls::Tick(page); CTDisplay::Tick(page); }
    }
    inline void Init();
}
