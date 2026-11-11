module;

#include <stdafx.h>
#include <stdexcept>
#include "FixSettings.hpp"
#include "Shared/RWGTA/FixMenu.hpp"
#include <deque>
#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <intrin.h>

export module NativeMenu;

import ComVars;

// The fix's settings as a native page behind the game's Options (Mafia 1.2)
export namespace MafiaMenu
{
    // Mafia 1.2 addresses, resolved by Resolve() once the executable is recognised
    struct Addresses
    {
        uintptr_t select, close, push, createButton, allocate, free, measure;
        uintptr_t loaded, process, destroyed, draw, pageDraw, update, refresh, deletePage, controlKey, pageKey;
        uintptr_t loopPopReturn, finalPopReturn;
        uintptr_t* pageTable;
        uintptr_t* pauseTable;
        uintptr_t* titleTable;
        uintptr_t* panelTable;
        uintptr_t* windowTable;
        uintptr_t* buttonTable;
        float* fonts;
        const uint8_t* const* fontMaps;
    };
    Addresses& Address()
    {
        static Addresses addresses{};
        return addresses;
    }
    void Resolve()
    {
        static const bool resolved = []()
        {
            auto& a = Address();
            auto at = [](const char* signature, ptrdiff_t offset = 0)
            {
                return reinterpret_cast<uintptr_t>(hook::get_pattern(signature, offset));
            };
            auto global = [](const char* signature, ptrdiff_t offset)
            {
                return *hook::get_pattern<uintptr_t>(signature, offset);
            };
            a.select = injector::GetBranchDestination(hook::get_pattern("E8 ? ? ? ? 8B 16 8B CE FF 52 04 8A 46 20 84 C0")).as_int();
            a.close = at("56 8B F1 57 8B 7C 24 0C 8B 4E 04 85 C9 74 0B 8B 56 28");
            a.push = at("8B 4C 24 04 A1 ? ? ? ? 8B 54 24 08 89 41 04");
            // The button factory; its constructor stores the button class table
            a.createButton = at("6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 51 53 56 57 6A 40 8B FA 8B D9 E8 ? ? ? ? 8B F0 83 C4 04 89 74 24 0C 33 C0 3B F0 89 44 24 18 74 1A 57 53 8B CE E8 ? ? ? ? 8B 46 08 C7 06 ? ? ? ? 80 CC 80 89 46 08 8B C6 8B 4C 24 10 5F 5E 5B 64 89 0D 00 00 00 00 83 C4 10 C3 90 90 90 90 90 90 90 90 90 90 90 90 90 B8 74 74 75 62");
            a.buttonTable = reinterpret_cast<uintptr_t*>(global("8B 46 08 C7 06 ? ? ? ? 80 CC 80 89 46 08 8B C6 8B 4C 24 10 5F 5E 5B 64 89 0D 00 00 00 00 83 C4 10 C3 90 90 90 90 90 90 90 90 90 90 90 90 90 B8 74 74 75 62", 5));
            a.allocate = at("6A 01 FF 74 24 08 E8 ? ? ? ? 59 59 C3 E8 ? ? ? ?");
            a.free = at("FF 74 24 04 E8 ? ? ? ? 59 C3 B8 ? ? ? ?");
            a.measure = at("56 8B 74 24 08 85 F6 75 0A D9 05 ? ? ? ? 5E");
            a.loaded = at("56 8B F1 8B 56 18 8B 4E 14 8B C2 3B C2 74 10 57");
            a.process = at("83 EC 10 56 8B F1 57 33 FF 8B 46 14 89 7C 24 0C");
            a.destroyed = at("56 57 8B F9 8B 77 14 8B 47 18 3B F0 73 16 8B 0E");
            a.draw = at("53 56 57 8B F9 8B 47 08 A8 10 74 2D 8B 5C 24 10");
            a.pageDraw = at("A1 ? ? ? ? 56 57 8B F1 8B 08 6A 00 6A 03 50");
            a.update = at("53 8B 5C 24 08 56 57 8B F9 53 8B 07 FF 50 18 8B 77 34");
            a.refresh = at("56 8B F1 8B 4E 2C 85 C9 74 23 8B 01 53 57 8D 7E 18");
            a.deletePage = at("6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 83 EC 08 56 8B F1 8A 4C 24 07");
            a.controlKey = at("8B 44 24 08 83 F8 39 75 06 8B 41 04 C2 08 00 83 F8 1C");
            a.pageKey = at("80 7C 24 04 01 75 07 6A FF E8 ? ? ? ? 33 C0");
            // Return addresses of the stack loop's pop and of the final native pop
            a.loopPopReturn = at("E8 ? ? ? ? A1 ? ? ? ? C7 05 ? ? ? ? 00 00 00 00", 5);
            a.finalPopReturn = at("E8 ? ? ? ? A1 ? ? ? ? 85 C0 74 1F 8B 10", 5);
            a.pageTable = reinterpret_cast<uintptr_t*>(global("C7 06 ? ? ? ? 0D 00 00 08 00 89 46 2C EB 47", 2));
            a.pauseTable = reinterpret_cast<uintptr_t*>(global("C7 06 ? ? ? ? 8B CE EB 02 33 C9 6A 01 32 D2 C7 44 24 30 FF FF FF FF", 2));
            a.titleTable = reinterpret_cast<uintptr_t*>(global("C7 06 ? ? ? ? 0D 00 00 08 00 8B CE 89 46 2C", 2));
            a.panelTable = reinterpret_cast<uintptr_t*>(global("C7 06 ? ? ? ? 89 54 24 10 8B 48 04 41", 2));
            a.windowTable = reinterpret_cast<uintptr_t*>(global("C7 06 ? ? ? ? 8B C6 8B 4C 24 10 5F 5E 5B 64 89 0D 00 00 00 00 83 C4 10 C3 90 90 90 90 90 90 B8 64 6E 69 77", 2));
            pTopMenuPage.SetAddress(global("89 0D ? ? ? ? E8 ? ? ? ? C2 08 00", 2));
            nMenuPendingCommand.SetAddress(global("C7 05 ? ? ? ? FF FF FF FF 5E 83 C4 10", 2));
            fMenuScaleX.SetAddress(global("D9 05 ? ? ? ? D8 0D ? ? ? ? D9 5E 5C", 2));
            fMenuScaleY.SetAddress(global("D8 0D ? ? ? ? D8 6C 24 14 D9 54 24 14", 2));
            a.fonts = reinterpret_cast<float*>(global("B9 ? ? ? ? 85 C0 74 BA E8 ? ? ? ?", 1));
            a.fontMaps = reinterpret_cast<const uint8_t* const*>(global("8B 3C 85 ? ? ? ? D9 44 24 14 C1 E2 03", 3));
            nLanguage.SetAddress(global("A1 ? ? ? ? C3 E8 ? ? ? ? E9 ? ? ? ? 56", 1));
            return true;
        }();
        (void)resolved;
    }
    bool Controls(const Control* parent)
    {
        return parent && parent->end >= parent->begin && parent->capacity >= parent->end;
    }
    bool Contains(const Page* page, const Control* control)
    {
        if (!page || !control) return false;
        for (auto** item = page->begin; item < page->end; ++item)
            if (*item == control) return true;
        return false;
    }
    Page* Top() { return pTopMenuPage; }
    void Select(Page* page, uint32_t id)
    {
        reinterpret_cast<void (__thiscall*)(Page*, uint32_t)>(Address().select)(page, id);
    }
    void Close(Page* page, int result)
    {
        // Calls parent slot24 and own slot8 before marking native close state.
        reinterpret_cast<void (__thiscall*)(Page*, int)>(Address().close)(page, result);
    }
    void Push(Page* page, int command)
    {
        // Native global stack publication precedes Init/parent notification.
        reinterpret_cast<void (__stdcall*)(Page*, int)>(Address().push)(page, command);
    }
    Control* CreateButton(const Record& record, Control* parent)
    {
        //5EA370 factory dispatch proves ECX=record, EDX=current parent.
        return reinterpret_cast<Control* (__fastcall*)(const Record*, Control*)>(Address().createButton)(&record, parent);
    }
    void* Allocate(size_t length)
    {
        return reinterpret_cast<void* (__cdecl*)(size_t)>(Address().allocate)(length);
    }
    void Free(void* memory)
    {
        reinterpret_cast<void (__cdecl*)(void*)>(Address().free)(memory);
    }
    void DestroyControl(Control* control)
    {
        reinterpret_cast<void (__thiscall*)(Control*, uint32_t)>(control->table[0])(control, 1);
    }
    bool Append(Page* page, Control* control)
    {
        // Same native allocator and pointer-vector layout as5EA370/5EB7D0.
        // The factory already linked parent.children; this second vector is
        // the page's destructor ownership, not another child relationship.
        if (!page || !control) return false;
        const auto begin = reinterpret_cast<uintptr_t>(page->begin);
        const auto end = reinterpret_cast<uintptr_t>(page->end);
        const auto capacity = reinterpret_cast<uintptr_t>(page->capacity);
        if (end < begin || capacity < end || (end - begin) % 4 || (capacity - begin) % 4 ||
            capacity - begin > 4096 * sizeof(Control*) || (!begin && (end || capacity))) return false;
        const auto count = (end - begin) / sizeof(Control*);
        if (end == capacity)
        {
            const auto available = std::min<size_t>(4096, std::max<size_t>(8, count * 2));
            if (available <= count) return false;
            auto** replacement = static_cast<Control**>(Allocate(available * sizeof(Control*)));
            if (!replacement) return false;
            if (count) std::memcpy(replacement, page->begin, count * sizeof(Control*));
            Free(page->begin);
            page->begin = replacement;
            page->end = replacement + count;
            page->capacity = replacement + available;
        }
        *page->end++ = control;
        return true;
    }
    bool Detach(Control* control)
    {
        // Only the exact newly created node may be detached during rollback.
        // The native scalar destructor does not unlink it from its parent.
        if (!control) return false;
        auto* parent = control->parent;
        if (!parent) return true;
        if (!Controls(parent)) return false;
        for (auto** item = parent->begin; item < parent->end; ++item)
        {
            if (*item != control) continue;
            std::memmove(item, item + 1, static_cast<size_t>(parent->end - item - 1) * sizeof(Control*));
            --parent->end;
            control->parent = nullptr;
            return true;
        }
        return false;
    }
    float* FontBank(unsigned int font)
    {
        // Four native map slots occupy6C4078..6C4087;6C4088 begins the
        // language-map records (5F6F30), not another font pointer.
        if (font >= 4) return nullptr;
        auto* fonts = Address().fonts;
        auto* bank = fonts + 1031 * font;
        auto* maps = Address().fontMaps;
        if (!maps[font]) return nullptr;
        if (!std::isfinite(bank[2]) || bank[2] <= 0 || !std::isfinite(bank[3]) || bank[3] <= 0 ||
            !std::isfinite(bank[5])) return nullptr;
        return bank;
    }
    float Measure(const std::string& text, float height, unsigned int font)
    {
        auto* bank = FontBank(font);
        if (!bank || text.size() > 4096 || !std::isfinite(height) || height <= 0) return INFINITY;
        auto* map = Address().fontMaps[font];
        for (auto character : text)
        {
            const auto byte = static_cast<uint8_t>(character);
            const auto width = bank[4 * map[2 * byte] + 10];
            // A negative glyph width is the native blank/space sentinel;
            //6036D0 deliberately substitutes0.3em for it.
            if (!std::isfinite(width)) return INFINITY;
        }
        //603880 passes h*bank[2] to6036D0; measure uses the identical scale.
        const auto result = reinterpret_cast<double (__thiscall*)(float*, const char*, float, unsigned int, uintptr_t)>(Address().measure)(
            Address().fonts, text.c_str(), height * bank[2], font, 0);
        return std::isfinite(result) && result >= 0 && result <= std::numeric_limits<float>::max() ? static_cast<float>(result) : INFINITY;
    }
    std::string Language()
    {
        //5F70D0 returns the exact two bytes used by localized MNU filenames.
        const uint32_t language = nLanguage;
        std::string tag{static_cast<char>(language), static_cast<char>(language >> 8)};
        for (auto& character : tag)
        {
            if (character >= 'A' && character <= 'Z') character += 'a' - 'A';
            if (character < 'a' || character > 'z') return "en";
        }
        // Native5F7000 explicitly loads tables/lang_cz.def; resources usecs.
        if (tag == "cz") return "cs";
        for (const auto* supported : { "en", "de", "fr", "it", "es", "pl", "cs", "ru" })
            if (tag == supported) return tag;
        // Nonstandard native aliases require their actual DEF provenance.
        return "en";
    }
    // Optionsb's stock Back is400; native Up/Down sorts by ID, not Y.
    // Reject a collision before constructing399 so navigation stays ordered.
    constexpr uint32_t entryId = 399;
    constexpr uint32_t firstRowId = 0x7F7F8100;
    constexpr uint32_t consumedId = 0x7F7F8FFF;
    // Native rows of the private page, Back included; as many as fit the screen
    constexpr size_t minRows = 7, maxRows = 12;
    enum class ItemKind { Page, Previous, Next, Back, Setting };
    struct Item
    {
        ItemKind kind = ItemKind::Back;
        size_t index = 0;
        std::string label, value;
        bool enabled = true;
        Control* native = nullptr;
        Control* valueNative = nullptr;
        std::optional<RWGTAMenu::Row> row;
        std::string description;
    };
    struct Source
    {
        Page* page = nullptr;
        Page* originParent = nullptr;
        Control* entry = nullptr;
        Control* prototype = nullptr;
        std::string caption;
        std::array<uintptr_t, 13> table{};
        uintptr_t* original = nullptr;
        uint64_t serial = 0;
        bool retired = false;
        bool pause = false;
    };
    struct Context
    {
        std::shared_ptr<Source> source;
        RWGTAMenu::Model model;
        Page* page = nullptr;
        Page* returnParent = nullptr;
        Control* title = nullptr;
        Control* description = nullptr;
        std::array<uintptr_t, 13> table{};
        std::deque<Item> items;
        std::array<Control*, maxRows> labels{}, values{};
        size_t rowCount = minRows;
        std::string titleText, descriptionText;
        std::deque<Item> prepared;
        std::string preparedTitle;
        float labelWidth = 0;
        uint64_t serial = 0;
        int category = -1;
        size_t rootOffset = 0, rowOffset = 0;
        bool retired = false;
        bool nativeAlive = false;
        bool detached = false;
        bool returnPending = false;
        bool rebuildPending = false;
        bool help = false;
        size_t helpOffset = 0;
        std::vector<std::string> helpLines;
        std::string helpTitle, helpBack;
        Control* helpSelection = nullptr;
    };
    struct Pending
    {
        std::shared_ptr<Source> source;
        std::shared_ptr<Context> owner;
        uint64_t serial = 0;
        uint32_t selected = 0;
        int direction = 0;
        bool back = false;
        bool describe = false;
    };
    std::unordered_map<Page*, std::shared_ptr<Source>> sources;
    std::shared_ptr<Context> context;
    std::deque<Pending> pending;
    std::unordered_set<Page*> candidates;
    std::deque<Page*> deferredDeletes;
    std::unordered_set<Page*> deletionRequests;
    uint64_t generation = 0;
    unsigned int processDepth = 0, lifecycleDepth = 0, drawDepth = 0, foreignDepth = 0;
    bool ready = false, draining = false;
    std::function<bool()> blocksInput;
    std::function<float()> viewportWidth;
    std::string noticeText;
    int padKey = 0;
    HMODULE module = nullptr;
    SafetyHookInline initHook, processHook, destroyHook, drawHook, pageDrawHook, updateHook, pushHook, closeHook;
    std::array<uintptr_t, 32> buttonTable{};
    // Failures only, once each, to the debugger
    void Log(const char* gate)
    {
        static std::unordered_set<std::string> delivered;
        if (!gate || delivered.size() >= 64 || !delivered.emplace(gate).second) return;
        OutputDebugStringA(("Mafia.FusionFix menu: " + std::string(gate) + "\n").c_str());
    }
    class Depth
    {
        unsigned int& value;
    public:
        explicit Depth(unsigned int& target) : value(target) { ++value; }
        ~Depth() { --value; }
    };
    class DeletionScope
    {
        Page* page;
        bool inserted;
    public:
        explicit DeletionScope(Page* target) : page(target), inserted(deletionRequests.emplace(page).second) {}
        ~DeletionScope() { if (inserted) deletionRequests.erase(page); }
    };
    bool SourceAlive(const std::shared_ptr<Source>& source)
    {
        if (!source || source->retired) return false;
        auto found = sources.find(source->page);
        return found != sources.end() && found->second == source && source->page &&
            !source->page->closed &&
            source->page->parent == source->originParent &&
            source->page->table == source->table.data() + 1 && Contains(source->page, source->entry) && Contains(source->page, source->prototype) &&
            source->entry->parent == source->page->root;
    }
    bool NativePageIdentity(const std::shared_ptr<Context>& owner)
    {
        return owner && owner == context && owner->nativeAlive && owner->page &&
            owner->page->table == owner->table.data() + 1 && owner->page->parent == owner->returnParent;
    }
    bool NativeIdentity(const std::shared_ptr<Context>& owner)
    {
        return NativePageIdentity(owner) && owner->page->root;
    }
    bool NativeOwned(const std::shared_ptr<Context>& owner)
    {
        return NativeIdentity(owner) && SourceAlive(owner->source);
    }
    bool Covered(const std::shared_ptr<Context>& owner)
    {
        if (!NativeIdentity(owner)) return false;
        auto* page = Top();
        for (unsigned int count = 0; page && count < 64; ++count)
        {
            if (page == owner->page) return true;
            page = page->parent;
        }
        return false;
    }
    bool Current(const std::shared_ptr<Context>& owner)
    {
        return NativeOwned(owner) && !owner->retired && Top() == owner->page && !owner->page->closed;
    }
    bool Active(const std::shared_ptr<Context>& owner, uint32_t selected)
    {
        if (!Current(owner) || !Contains(owner->page, owner->page->selected) || owner->page->selected->id != selected) return false;
        if (owner->help)
            return owner->page->selected == owner->labels[owner->rowCount - 1];
        for (const auto& item : owner->items)
            if (item.native == owner->page->selected)
                return item.native->parent == owner->page->root && item.enabled;
        return false;
    }
    void Retire(const std::shared_ptr<Context>& owner)
    {
        if (!owner || owner->retired) return;
        owner->retired = true;
        owner->model.Close();
    }
    bool Queue(const std::shared_ptr<Context>& owner, uint32_t selected, int direction, bool back)
    {
        if (back && direction != 0) return false;
        if (pending.size() >= 32 || !Current(owner) || (!back && !Active(owner, selected))) return false;
        pending.push_back({ {}, owner, owner->serial, selected, direction, back });
        // Logical cancellation may abort a foreign Get/Set/preset burst, but
        // native Close/destruction stays after complete input/lifecycle unwind.
        if (back && foreignDepth)
        {
            owner->returnPending = true;
            Retire(owner);
        }
        return true;
    }
    std::optional<std::string> NativeText(const std::string& text)
    {
        if (text.size() > 4096) return std::nullopt;
        if (text.empty()) return std::string{};
        const auto count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), nullptr, 0);
        if (count <= 0) return std::nullopt;
        std::wstring wide(count, 0);
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), wide.data(), count);
        const auto bytes = WideCharToMultiByte(1252, WC_NO_BEST_FIT_CHARS, wide.data(), count, nullptr, 0, "?", nullptr);
        if (bytes <= 0) return std::nullopt;
        std::string result(bytes, 0);
        BOOL substituted = FALSE;
        WideCharToMultiByte(1252, WC_NO_BEST_FIT_CHARS, wide.data(), count, result.data(), bytes, "?", &substituted);
        if (substituted) return std::nullopt;
        for (const auto character : result)
            if (static_cast<uint8_t>(character) < 32 || character == '~') return std::nullopt;
        return result;
    }
    bool Fit(std::string& text, float& height, float width, unsigned int font, bool fullValue)
    {
        if (!std::isfinite(width) || width <= 0 || !std::isfinite(height) || height <= 0) return false;
        if (fullValue && !text.empty() && height < 8.0f) return false;
        auto measured = Measure(text, height, font);
        if (!std::isfinite(measured)) return false;
        if (measured <= width) return true;
        if (fullValue)
        {
            height *= width / measured;
            // Native control height is in rendered pixel coordinates. Refuse
            // a full value which would require less than an8-pixel text box;
            // the host retains the previous page rather than hide its value.
            return std::isfinite(height) && height >= 8.0f && Measure(text, height, font) <= width + 0.01f;
        }
        if (Measure("...", height, font) > width) return false;
        while (!text.empty() && Measure(text + "...", height, font) > width) text.pop_back();
        text += "...";
        return true;
    }
    bool ReadScope(const std::shared_ptr<Context>& owner)
    {
        if (!owner || owner != context || owner->retired || !SourceAlive(owner->source)) return false;
        if (owner->nativeAlive) return Current(owner);
        auto* source = owner->source->page;
        return Top() == source && source->selected == owner->source->entry && !source->closed;
    }
    template<class Call> auto Foreign(Call&& call)
    {
        Depth scope(foreignDepth);
        return call();
    }
    // Rows from the top of the private page to its help line: title gap, rows 0.35 apart, help line
    float RowsHeight(float rowHeight, size_t rows)
    {
        return rowHeight * (3.6f + static_cast<float>(rows - 1) * 1.35f);
    }
    size_t FittingRows(float rowHeight)
    {
        const float scaleY = fMenuScaleY;
        size_t rows = maxRows;
        while (rows > minRows && RowsHeight(rowHeight, rows) + 72 * scaleY > 540 * scaleY) --rows;
        return rows;
    }
    bool PrepareItems(const std::shared_ptr<Context>& owner)
    {
        if (!ReadScope(owner) || !owner->model.IsOpen()) return false;
        const auto serial = owner->serial;
        auto current = [&] { return ReadScope(owner) && owner->serial == serial; };
        std::deque<Item> next;
        std::string title = owner->model.Text("menu.plugins");
        const auto links = owner->rowCount - 3;
        if (owner->category < 0)
        {
            // The plugins (the fix first), plus Previous/Next/Back.
            const auto count = owner->model.Pages().size();
            if (owner->rootOffset > count) owner->rootOffset = 0;
            if (owner->rootOffset) next.push_back({ ItemKind::Previous, 0, owner->model.Text("menu.previous") });
            const auto limit = std::min(count, owner->rootOffset + links);
            for (size_t index = owner->rootOffset; index < limit; ++index)
            {
                const auto& name = owner->model.Pages()[index].title;
                next.push_back({ ItemKind::Page, index, name });
                // The row may use an ellipsis; its selected hint keeps the
                // complete page name so sibling links remain distinguishable.
                next.back().description = name;
            }
            if (limit < count) next.push_back({ ItemKind::Next, limit, owner->model.Text("menu.next") });
        }
        else
        {
            const auto category = static_cast<size_t>(owner->category);
            if (category >= owner->model.Pages().size()) return false;
            title = owner->model.Pages()[category].title;
            const auto rows = Foreign([&] { return owner->model.VisibleRows(category); });
            if (!current()) return false;
            if (owner->rowOffset >= rows.size()) owner->rowOffset = 0;
            const auto capacity = owner->rowCount - 3;
            if (owner->rowOffset)
                next.push_back({ ItemKind::Previous, owner->rowOffset > capacity ? owner->rowOffset - capacity : 0, owner->model.Text("menu.previous") });
            const auto limit = std::min(rows.size(), owner->rowOffset + capacity);
            for (size_t index = owner->rowOffset; index < limit; ++index)
            {
                // A foreign read may Close/Open the model or cover native UI.
                // Never reuse another old Row* after that boundary.
                const auto row = *rows[index];
                const auto enabled = Foreign([&] { return owner->model.Enabled(row, owner->source->pause); });
                if (!current()) return false;
                auto label = owner->model.Label(row);
                auto value = enabled ? Foreign([&] { return owner->model.Value(row); }) : owner->model.Text("menu.unavailable");
                if (!current()) return false;
                next.push_back({ ItemKind::Setting, index, std::move(label), std::move(value), enabled });
                next.back().row = row;
                // Once a changed setting needs a restart, its rows say so in the help line, in the game's own font
                const bool restart = row.kind == RWGTAMenu::RowKind::Setting && row.setting && row.setting->apply == "restart";
                next.back().description = owner->model.Help(row);
                if (restart && owner->model.RestartRequired())
                    next.back().description = owner->model.Text("menu.restartwarning") + " " + next.back().description;
            }
        }
        if (owner->category >= 0)
        {
            const auto rows = owner->model.VisibleRows(static_cast<size_t>(owner->category));
            const auto limit = owner->rowOffset + owner->rowCount - 3;
            if (limit < rows.size()) next.push_back({ ItemKind::Next, limit, owner->model.Text("menu.next") });
        }
        next.push_back({ ItemKind::Back, 0, owner->model.Text("menu.back") });
        if (owner->category < 0 && owner->model.RestartRequired())
            next.back().description = owner->model.Text("menu.restartwarning");
        if (!current() || next.empty() || next.size() > owner->rowCount) return false;
        auto nativeTitle = NativeText(title);
        if (!nativeTitle) return false;
        for (auto& item : next)
        {
            auto label = NativeText(item.label);
            auto value = NativeText(item.value);
            if (!label || !value) return false;
            item.label = std::move(*label);
            item.value = std::move(*value);
            auto description = NativeText(item.description);
            if (!description) return false;
            item.description = std::move(*description);
        }
        // Publication is separate from native mutation. Failed/covered reads
        // leave the currently owned control pool and its strings untouched.
        owner->prepared = std::move(next);
        owner->preparedTitle = std::move(*nativeTitle);
        return true;
    }
    int __fastcall SourceCommand(Page* page, void*, int command);
    void InstallSource(Page* page)
    {
        if (!ready || sources.count(page) || Top() != page || !page || page->closed) return;
        if (page->table != Address().pageTable || !page->parent)
        {
            Log("source context refused");
            return;
        }
        const bool pause = page->command == 108 && page->parent->table == Address().pauseTable;
        const bool title = page->command == 990110 && page->parent->table == Address().titleTable;
        if (!pause && !title) { Log("source title/pause identity refused"); return; }
        auto* root = page->root;
        if (!Controls(root) || (root->table != Address().panelTable && root->table != Address().windowTable))
        {
            Log("source root style refused");
            return;
        }
        Control* control = nullptr;
        Control* audio = nullptr;
        Control* back = nullptr;
        const auto first = reinterpret_cast<uintptr_t>(page->begin);
        const auto last = reinterpret_cast<uintptr_t>(page->end);
        if (last < first)
        {
            Log("source widget vector refused");
            return;
        }
        for (auto** entry = page->begin; entry < page->end; ++entry)
        {
            auto* node = *entry;
            if (!node) { Log("source widget missing"); return; }
            if (node->id == entryId) { Log("source category ID collision"); return; }
            if (node->id == 100) control = node;
            if (node->id == 101) audio = node;
            if (node->id == 400) back = node;
        }
        for (auto* node : { control, audio, back })
            if (!node || node->parent != root || node->table != Address().buttonTable ||
                (node->flags & 0x8030) != 0x8030 || !std::isfinite(node->width) || !std::isfinite(node->height) || node->width <= 0 || node->height <= 0)
            { Log("source button contract refused"); return; }
        const float scaleX = fMenuScaleX;
        const float scaleY = fMenuScaleY;
        const auto stockEnd = std::max(control->relativeY + control->height, audio->relativeY + audio->height);
        const auto availableGap = back->relativeY - stockEnd;
        const auto margin = std::max(2.0f * scaleY, std::min(control->height * 0.1f, availableGap * 0.25f));
        auto height = control->height;
        const auto y = stockEnd + margin;
        const auto header = root->table == Address().windowTable ? 40 * scaleY : 0;
        if (!std::isfinite(scaleX) || !std::isfinite(scaleY) || scaleX <= 0 || scaleY <= 0 || !std::isfinite(y) ||
            !std::isfinite(back->relativeY) || !std::isfinite(root->height) || !std::isfinite(availableGap) || availableGap <= 0 ||
            !std::isfinite(margin) || margin <= 0 || !std::isfinite(height) || height < std::max(8.0f, 8.0f * scaleY) || y < 0 ||
            y + height + margin + control->height + header > 580 * scaleY)
        { Log("source category layout refused"); return; }
        auto source = std::make_shared<Source>();
        source->page = page;
        source->originParent = page->parent;
        source->prototype = control;
        source->pause = pause;
        source->serial = ++generation;
        auto language = FixSettings().String("MAIN.TextLanguage");
        if (language.empty() || language == "auto")
            language = Language();
        FixSettings().GetTexts().Select(language);
        auto caption = NativeText(FixSettings().GetTexts().Resolve("@menu.plugins"));
        if (!caption) { Log("source category text conversion refused"); return; }
        source->caption = std::move(*caption);
        if (!Fit(source->caption, height, control->width, control->flags & 15, false))
        { Log("source category font fit refused"); return; }
        source->original = page->table;
        std::memcpy(source->table.data(), page->table - 1, sizeof(source->table));
        source->table[10] = reinterpret_cast<uintptr_t>(&SourceCommand);
        Record record{ entryId, 0x62757474, control->relativeX / scaleX, y / scaleY,
            control->width / scaleX, height / scaleY, 0, control->flags & ~0x80000000u };
        auto* native = CreateButton(record, root);
        if (!native) { Log("source category allocation refused"); return; }
        if (native->parent != root || native->id != entryId || native->table != Address().buttonTable || !Append(page, native))
        {
            // Exact new object rollback spans both the parent relationship and
            // the page's destructor ownership. Never delete a borrowed node.
            if (Detach(native)) DestroyControl(native);
            Log("source partial category construction refused");
            return;
        }
        native->text = source->caption.c_str();
        reinterpret_cast<void (__thiscall*)(Control*, int)>(Address().refresh)(native, 0);
        source->entry = native;
        back->relativeY = y + height + margin;
        back->height = control->height;
        root->height = std::max(root->height, header + back->relativeY + back->height + margin);
        root->relativeY = (600 * scaleY - root->height) * 0.5f;
        reinterpret_cast<void (__thiscall*)(Control*, int)>(Address().refresh)(root, 0);
        sources.emplace(page, source);
        page->table = source->table.data() + 1;
    }
    int __fastcall SourceCommand(Page* page, void*, int command)
    {
        auto found = sources.find(page);
        if (found == sources.end()) return 0;
        auto source = found->second;
        Depth scope(lifecycleDepth);
        if (source->retired) return 0;
        if (command == static_cast<int>(entryId) && SourceAlive(source) && Top() == page && page->selected == source->entry && pending.size() < 32)
        {
            pending.push_back({ source, {}, source->serial, entryId, 0, false });
            return 0;
        }
        return reinterpret_cast<int (__thiscall*)(Page*, int)>(source->original[9])(page, command);
    }
    int __fastcall PrivateCommand(Page*, void*, int);
    int __fastcall PrivateKey(Page*, void*, int, int);
    int __fastcall PrivateSelected(Page*, void*, Control*, Control*);
    void Describe(const std::shared_ptr<Context>& owner, uint32_t selected);
    int __fastcall PrivateKeyboard(Control*, void*, int, int);
    bool PrepareNativePage(const std::shared_ptr<Context>& owner)
    {
        if (!NativeOwned(owner) || Top() != owner->page || owner->prepared.empty() || owner->prepared.size() > owner->rowCount) return false;
        auto* page = owner->page;
        auto* root = page->root;
        auto* prototype = owner->source->prototype;
        if (!Controls(root) || root->parent || root->table != Address().windowTable ||
            !Contains(owner->source->page, prototype)) { Log("private root style refused"); return false; }
        const float scaleX = fMenuScaleX;
        const float scaleY = fMenuScaleY;
        if (!std::isfinite(scaleX) || !std::isfinite(scaleY) || scaleX <= 0 || scaleY <= 0 ||
            !std::isfinite(root->width) || root->width < 160 * scaleX || !std::isfinite(prototype->height) || prototype->height <= 0)
        { Log("private page geometry refused"); return false; }
        const auto rowHeight = std::min(prototype->height, 24 * scaleY);
        const auto gap = rowHeight * 0.35f;
        const auto clientHeight = RowsHeight(rowHeight, owner->rowCount) + 32 * scaleY;
        const auto height = clientHeight + 40 * scaleY;
        const auto viewportHeight = 600 * scaleY;
        const auto lastRowBottom = rowHeight * 2 + static_cast<float>(owner->rowCount - 1) * (rowHeight + gap);
        const auto footerTop = clientHeight - rowHeight * 1.2f;
        if (!std::isfinite(lastRowBottom) || !std::isfinite(footerTop) ||
            lastRowBottom + gap * 0.5f > footerTop || footerTop + rowHeight > clientHeight)
        { Log("private row/footer separation refused"); return false; }
        if (!std::isfinite(height) || height > viewportHeight * 0.9f) { Log("private page vertical bound refused"); return false; }
        root->width = 720 * scaleX;
        root->relativeX = ((viewportWidth ? viewportWidth() : 800 * scaleX) - root->width) * 0.5f;
        auto nativeTitle = owner->preparedTitle;
        auto titleHeight = 32 * scaleY;
        if (!Fit(nativeTitle, titleHeight, root->width, 2, true)) { Log("private header font refused"); return false; }
        // All original controls remain page-owned for native destruction. They
        // are hidden only on this new private page, never on vanilla Options.
        for (auto** item = page->begin; item < page->end; ++item)
        {
            if (!*item) { Log("private template widget missing"); return false; }
            if (*item != root) (*item)->flags &= ~(0x10u | 0x80000000u);
        }
        if (page->selected) page->selected->flags &= ~0x80000000u;
        page->selected = nullptr;
        root->height = height;
        root->relativeY = (viewportHeight - height) * 0.5f;
        reinterpret_cast<void (__thiscall*)(Control*, int)>(Address().refresh)(root, 0);
        const auto available = root->width - 40 * scaleX;
        const auto valueWidth = available * 0.38f;
        const auto labelWidth = available - valueWidth - 10 * scaleX;
        if (labelWidth < 64 * scaleX || valueWidth < 48 * scaleX) { Log("private columns refused"); return false; }
        owner->labelWidth = labelWidth;
        auto create = [&](uint32_t id, float x, float y, float width, bool focusable) -> Control*
        {
            Record record{ id, 0x62757474, x / scaleX, y / scaleY, width / scaleX, rowHeight / scaleY,
                0, (prototype->flags & ~(0x80000000u | 0x80u | 0x40u | 0x100u | 0x200u)) | 0x30u };
            auto* node = CreateButton(record, root);
            if (!node) return nullptr;
            if (node->parent != root || node->id != id || node->table != Address().buttonTable || !Append(page, node))
            {
                if (Detach(node)) DestroyControl(node);
                return nullptr;
            }
            if (!focusable) node->flags &= ~0x8000u;
            else node->table = buttonTable.data() + 1;
            node->text = "";
            reinterpret_cast<void (__thiscall*)(Control*, int)>(Address().refresh)(node, 0);
            return node;
        };
        for (size_t index = 0; index < owner->rowCount; ++index)
        {
            const auto y = rowHeight + static_cast<float>(index) * (rowHeight + gap);
            // Native hit-testing owns the whole row, including its value.
            // Text is fitted separately to the left column before rendering.
            owner->labels[index] = create(firstRowId + static_cast<uint32_t>(index), 20 * scaleX, y, available, true);
            owner->values[index] = create(firstRowId + 0x20 + static_cast<uint32_t>(index), 30 * scaleX + labelWidth, y, valueWidth, false);
            if (!owner->labels[index] || !owner->values[index]) { Log("private partial row allocation refused"); return false; }
        }
        owner->description = create(firstRowId + 0x100, 20 * scaleX, clientHeight - rowHeight * 1.2f, available, false);
        if (!owner->description) { Log("private description allocation refused"); return false; }
        owner->titleText = std::move(nativeTitle);
        root->text = owner->titleText.c_str();
        return true;
    }
    bool Populate(const std::shared_ptr<Context>& owner)
    {
        if (!Current(owner) || owner->prepared.empty() || owner->prepared.size() > owner->rowCount ||
            !Contains(owner->page, owner->description) || owner->description->parent != owner->page->root) return false;
        auto title = owner->preparedTitle;
        const float scaleY = fMenuScaleY;
        auto titleHeight = 32 * scaleY;
        if (!Fit(title, titleHeight, owner->page->root->width, 2, true)) return false;
        std::array<float, maxRows> labelHeights{}, valueHeights{};
        for (size_t index = 0; index < owner->rowCount; ++index)
        {
            auto* label = owner->labels[index];
            auto* value = owner->values[index];
            if (!Contains(owner->page, label) || !Contains(owner->page, value) || label->parent != owner->page->root || value->parent != owner->page->root)
            { Log("private control ownership refused"); return false; }
            labelHeights[index] = valueHeights[index] = owner->source->prototype->height;
            if (index >= owner->prepared.size()) continue;
            auto& item = owner->prepared[index];
            const float labelWidth = item.value.empty() ? label->width : owner->labelWidth;
            if (!Fit(item.label, labelHeights[index], labelWidth, label->flags & 15, true) ||
                !Fit(item.value, valueHeights[index], value->width, value->flags & 15, true))
            { Log("private full-value readability refused"); return false; }
            if (item.kind == ItemKind::Page)
            {
                auto hintHeight = owner->source->prototype->height;
                // Validate every complete name before publishing any rows.
                // Unsupported or unreadable names leave via normal rollback.
                if (!Fit(item.description, hintHeight, owner->description->width, owner->description->flags & 15, true))
                { Log("private full-page-name readability refused"); return false; }
            }
        }
        if (!Current(owner)) return false;
        uint32_t selected = firstRowId;
        if (Contains(owner->page, owner->page->selected)) selected = owner->page->selected->id;
        owner->serial = ++generation;
        owner->items = std::move(owner->prepared);
        owner->titleText = std::move(title);
        owner->page->root->text = owner->titleText.c_str();
        owner->descriptionText.clear();
        owner->description->text = owner->descriptionText.c_str();
        for (size_t index = 0; index < owner->rowCount; ++index)
        {
            auto* label = owner->labels[index];
            auto* value = owner->values[index];
            label->flags &= ~(0x80000000u | 0x30u);
            label->flags |= 0x8000u;
            value->flags &= ~0x30u;
            if (index >= owner->items.size()) { label->text = value->text = ""; continue; }
            auto& item = owner->items[index];
            item.native = label;
            item.valueNative = value;
            label->text = item.label.c_str();
            value->text = item.value.c_str();
            label->height = labelHeights[index];
            value->height = valueHeights[index];
            label->flags |= item.enabled ? 0x30u : 0x10u;
            value->flags |= item.enabled ? 0x30u : 0x10u;
        }
        owner->rebuildPending = false;
        RWGTAMenu::Model::Changed() = false;
        if (selected < firstRowId || selected >= firstRowId + owner->items.size()) selected = firstRowId;
        Select(owner->page, selected);
        if (!Current(owner) || !Contains(owner->page, owner->page->selected)) return false;
        Describe(owner, owner->page->selected->id);
        return Current(owner) && Contains(owner->page, owner->page->selected);
    }
    void RequestReturn(const std::shared_ptr<Context>& owner)
    {
        if (!owner || owner != context) return;
        owner->returnPending = true;
        Retire(owner);
        if (NativePageIdentity(owner) && Top() == owner->page && !owner->page->closed)
            Close(owner->page, -1);
    }
    void Describe(const std::shared_ptr<Context>& owner, uint32_t selected)
    {
        if (!Active(owner, selected) || !Contains(owner->page, owner->description)) return;
        if (owner->help) return;
        for (const auto& item : owner->items)
        {
            if (item.native->id != selected) continue;
            auto description = item.description;
            if (item.kind == ItemKind::Setting)
                description = "F1: " + description;
            auto height = owner->source->prototype->height;
            const auto fullPageName = item.kind == ItemKind::Page;
            if (!Fit(description, height, owner->description->width, owner->description->flags & 15, fullPageName)) return;
            owner->descriptionText = std::move(description);
            owner->description->height = height;
            owner->description->text = owner->descriptionText.c_str();
            return;
        }
    }
    bool Help(const std::shared_ptr<Context>& owner, int scroll = 0,
        const std::string* content = nullptr, const std::string* title = nullptr);

    bool NoticeReady()
    {
        return ready && context && Current(context) && !context->help;
    }
    void ShowNotice(const std::wstring& text)
    {
        noticeText = NativeText(asi_settings::text::Narrow(text)).value_or("");
    }
    void Notice()
    {
        auto owner = context;
        if (noticeText.empty() || !NoticeReady() || !Contains(owner->page, owner->description)) return;
        const auto title = NativeText(FixSettings().GetTexts().Resolve("@plugin.name")).value_or("Mafia Fusion Fix");
        if (Help(owner, 0, &noticeText, &title)) noticeText.clear();
    }
    bool Help(const std::shared_ptr<Context>& owner, int scroll,
        const std::string* content, const std::string* title)
    {
        if (!Current(owner) || owner->items.empty()) return false;
        if (!owner->help)
        {
            auto* selected = owner->page->selected;
            if (!selected) return false;
            auto item = std::find_if(owner->items.begin(), owner->items.end(), [selected](const Item& row)
            {
                return row.native == selected && row.kind == ItemKind::Setting;
            });
            if (!content && (item == owner->items.end() || item->description.empty())) return false;
            owner->helpTitle = title ? *title : item != owner->items.end() ? item->label : owner->titleText;
            owner->helpSelection = selected;
            owner->helpBack = NativeText(owner->model.Text("menu.back")).value_or("Back");
            owner->helpLines.clear();
            std::string line;
            const auto height = owner->source->prototype->height;
            const auto width = owner->description->width;
            const auto font = owner->description->flags & 15;
            const auto& description = content ? *content : item->description;
            for (char character : description)
            {
                if (character == '\r') continue;
                auto next = line + character;
                if (character == '\n' || (!line.empty() && Measure(next, height, font) > width))
                {
                    const auto space = character == '\n' ? std::string::npos : line.find_last_of(' ');
                    if (space != std::string::npos && space > 0)
                    {
                        owner->helpLines.push_back(line.substr(0, space));
                        line.erase(0, space + 1);
                    }
                    else
                    {
                        owner->helpLines.push_back(line);
                        line.clear();
                    }
                    if (character == '\n') continue;
                }
                if (!line.empty() || character != ' ') line += character;
            }
            if (!line.empty()) owner->helpLines.push_back(line);
            owner->help = true;
            owner->helpOffset = 0;
        }
        const auto visible = owner->rowCount - 1;
        const auto maximum = owner->helpLines.size() > visible ? owner->helpLines.size() - visible : 0;
        owner->helpOffset = static_cast<size_t>(std::clamp<int64_t>(
            static_cast<int64_t>(owner->helpOffset) + scroll, 0, static_cast<int64_t>(maximum)));
        owner->page->root->text = owner->helpTitle.c_str();
        for (size_t index = 0; index < owner->rowCount; ++index)
        {
            auto* label = owner->labels[index];
            label->flags &= ~(0x80000000u | 0x8030u);
            owner->values[index]->flags &= ~0x30u;
            if (index < visible)
            {
                const auto line = owner->helpOffset + index;
                label->text = line < owner->helpLines.size() ? owner->helpLines[line].c_str() : "";
                label->height = owner->source->prototype->height;
                label->flags |= 0x30u;
            }
            else
            {
                label->text = owner->helpBack.c_str();
                label->flags |= 0x8030u;
            }
        }
        owner->descriptionText = "F1 / Esc";
        if (maximum) owner->descriptionText += "   " + std::to_string(owner->helpOffset + 1) + "/" + std::to_string(maximum + 1);
        owner->description->text = owner->descriptionText.c_str();
        Select(owner->page, firstRowId + static_cast<uint32_t>(visible));
        return true;
    }
    void CloseHelp(const std::shared_ptr<Context>& owner)
    {
        owner->help = false;
        if (Contains(owner->page, owner->helpSelection))
            owner->page->selected = owner->helpSelection;
        if (!PrepareItems(owner) || !Populate(owner)) RequestReturn(owner);
    }
    void Open(const std::shared_ptr<Source>& source, uint64_t serial)
    {
        if (!SourceAlive(source) || source->serial != serial || Top() != source->page ||
            source->page->selected != source->entry || source->page->closed || context) return;
        auto owner = std::make_shared<Context>();
        owner->source = source;
        owner->serial = ++generation;
        context = owner;
        auto language = FixSettings().String("MAIN.TextLanguage");
        if (language.empty() || language == "auto")
            language = Language();
        owner->rowCount = FittingRows(std::min(source->prototype->height, 24 * fMenuScaleY));
        Foreign([&] { owner->model.OpenPlugins(module, language, 4096, {}, true); });
        if (!ReadScope(owner) || !PrepareItems(owner))
        {
            Retire(owner);
            if (context == owner) context.reset();
            Log("private discovery/initial preparation refused");
            return;
        }
        auto* page = static_cast<Page*>(Allocate(sizeof(Page)));
        if (!page)
        {
            Retire(owner); context.reset(); Log("private page allocation refused"); return;
        }
        std::memset(page, 0, sizeof(Page));
        reinterpret_cast<void (__thiscall*)(Page*)>(Address().deletePage)(page);
        owner->page = page;
        owner->returnParent = source->page;
        std::memcpy(owner->table.data(), source->original - 1, sizeof(owner->table));
        owner->table[8] = reinterpret_cast<uintptr_t>(&PrivateKey);
        owner->table[10] = reinterpret_cast<uintptr_t>(&PrivateCommand);
        owner->table[12] = reinterpret_cast<uintptr_t>(&PrivateSelected);
        page->table = owner->table.data() + 1;
        page->flags = source->page->flags;
        owner->nativeAlive = true;
        Push(page, entryId);
        if (!NativeOwned(owner) || Top() != page || !PrepareNativePage(owner) || !Populate(owner))
        {
            Log("private native commit refused");
            RequestReturn(owner);
            return;
        }
    }
    int __fastcall PrivateKeyboard(Control* control, void*, int modifiers, int key)
    {
        auto owner = context;
        Depth scope(lifecycleDepth);
        if (owner && Active(owner, control->id))
        {
            if (owner->help)
            {
                if (key == 59) CloseHelp(owner);
                else if (key == 200 || key == 201) Help(owner, -1);
                else if (key == 208 || key == 209) Help(owner, 1);
                else if (key == 28 || key == 57 || key == 156) CloseHelp(owner);
                return static_cast<int>(consumedId);
            }
            if (key == 59 && Help(owner)) return static_cast<int>(consumedId);
        }
        if (owner && Active(owner, control->id) && (key == 203 || key == 205 || key == 57 || key == 28 || key == 156))
        {
            if (Queue(owner, control->id, key == 203 ? -1 : key == 205 ? 1 : 0, false)) return static_cast<int>(consumedId);
        }
        return reinterpret_cast<int (__thiscall*)(Control*, int, int)>(Address().controlKey)(control, modifiers, key);
    }
    int __fastcall PrivateKey(Page* page, void*, int key, int modifiers)
    {
        auto owner = context;
        Depth scope(lifecycleDepth);
        if (owner && owner->page == page && Current(owner) && owner->help && (key & 255) == 1)
        {
            CloseHelp(owner);
            return 0;
        }
        if ((key & 255) == 1 && owner && owner->page == page && Queue(owner, page->selected ? page->selected->id : 0, 0, true)) return 0;
        // Native Escape remains available if queue capacity/refusal prevents
        // logical navigation, including incomplete/retired private contexts.
        return reinterpret_cast<int (__thiscall*)(Page*, int, int)>(Address().pageKey)(page, key, modifiers);
    }
    int __fastcall PrivateCommand(Page* page, void*, int command)
    {
        auto owner = context;
        Depth scope(lifecycleDepth);
        if (command == static_cast<int>(consumedId)) return 0;
        if (owner && owner->page == page) Queue(owner, static_cast<uint32_t>(command), 0, false);
        return 0;
    }
    int __fastcall PrivateSelected(Page* page, void*, Control* selected, Control*)
    {
        auto owner = context;
        Depth scope(lifecycleDepth);
        if (owner && owner->page == page && selected && Active(owner, selected->id) && pending.size() < 32)
            pending.push_back({ {}, owner, owner->serial, selected->id, 0, false, true });
        return 0;
    }
    void Act(const Pending& action)
    {
        auto owner = action.owner;
        if (!Current(owner) || owner->serial != action.serial) return;
        if (owner->help)
        {
            if (!action.describe) CloseHelp(owner);
            return;
        }
        if (action.back)
        {
            if (action.direction != 0) return;
            if (owner->category < 0) { RequestReturn(owner); return; }
            owner->category = -1;
        }
        else
        {
            if (!Active(owner, action.selected)) return;
            if (action.describe) { Describe(owner, action.selected); return; }
            Item item;
            bool found = false;
            for (const auto& row : owner->items)
                if (row.native->id == action.selected) { item = row; found = true; break; }
            if (!found) return;
            if (item.kind != ItemKind::Setting && action.direction != 0) return;
            switch (item.kind)
            {
                case ItemKind::Page:
                    owner->category = static_cast<int>(item.index);
                    owner->rowOffset = 0;
                    owner->model.PageShown(item.index);
                    break;
                case ItemKind::Previous:
                    if (owner->category >= 0) owner->rowOffset = item.index;
                    else owner->rootOffset = owner->rootOffset >= 4 ? owner->rootOffset - 4 : 0;
                    break;
                case ItemKind::Next:
                    if (owner->category >= 0) owner->rowOffset = item.index;
                    else owner->rootOffset = item.index;
                    break;
                case ItemKind::Back:
                    if (owner->category < 0) { RequestReturn(owner); return; }
                    owner->category = -1;
                    break;
                case ItemKind::Setting:
                {
                    if (!item.row || owner->category < 0) return;
                    const auto row = *item.row;
                    if (action.direction && row.kind != RWGTAMenu::RowKind::Setting && row.kind != RWGTAMenu::RowKind::Preset) return;
                    const auto visible = Foreign([&] { return owner->model.VisibleRows(static_cast<size_t>(owner->category)); });
                    if (!Active(owner, action.selected) || owner->serial != action.serial) return;
                    bool present = false;
                    for (auto* current : visible)
                        if (current->kind == row.kind && current->plugin == row.plugin && current->setting == row.setting) { present = true; break; }
                    if (!present) { owner->rebuildPending = true; return; }
                    const auto enabled = Foreign([&] { return owner->model.Enabled(row, owner->source->pause); });
                    if (!enabled || !Active(owner, action.selected) || owner->serial != action.serial) return;
                    Foreign([&] { owner->model.Change(row, action.direction, owner->source->pause); });
                    if (!Current(owner) || owner->serial != action.serial) return;
                    break;
                }
            }
        }
        owner->rebuildPending = true;
        if (!PrepareItems(owner))
        {
            // A real native modal preserves the logical owner until it closes.
            // Other failures return safely instead of mixing old widgets with
            // the newly selected category/root offset.
            if (Covered(owner) && Top() != owner->page) return;
            Log("private navigation preparation refused; returning to native Options");
            RequestReturn(owner);
            return;
        }
        if (!Populate(owner))
        {
            Log("private navigation commit refused; returning to native Options");
            RequestReturn(owner);
        }
        else if (Contains(owner->page, owner->page->selected)) Describe(owner, owner->page->selected->id);
    }
    bool Bypass(Page* target, Page* replacement)
    {
        // The same parent4/global6BD890 relationship as the native Pop at
        //5EAC2x; never destroy a foreign covering page during owner retirement.
        Page* previous = nullptr;
        auto* page = Top();
        std::array<Page*, 64> seen{};
        for (size_t count = 0; page && count < seen.size(); ++count)
        {
            if (page == target)
            {
                if (previous) previous->parent = replacement;
                else
                {
                    if (replacement) pTopMenuPage = replacement;
                    else
                    {
                        //5EBD6E unconditionally deletes Top at loop exit. Keep
                        //the exact last page alive until that final call.
                        target->closed = 1;
                        target->result = -1;
                        if (!nMenuPendingCommand) nMenuPendingCommand = -1;
                    }
                }
                return true;
            }
            for (size_t index = 0; index < count; ++index) if (seen[index] == page) return false;
            seen[count] = page;
            previous = page;
            page = page->parent;
        }
        return page == nullptr;
    }
    void Collect()
    {
        if (processDepth || lifecycleDepth || drawDepth || foreignDepth) return;
        // Actual native deletion is leased to complete input/draw/lifecycle
        // and foreign API return, not merely to a retained C++Context.
        for (size_t remaining = deferredDeletes.size(); remaining; --remaining)
        {
            auto* page = deferredDeletes.front();
            deferredDeletes.pop_front();
            if (Top() == page && page && !page->parent)
            {
                deferredDeletes.push_back(page); //Leased to native loop-final deletion.
                continue;
            }
            auto owner = context;
            if (owner && owner->page == page) owner->nativeAlive = false;
            { DeletionScope deleting(page); Depth scope(lifecycleDepth); destroyHook.thiscall<void>(page); }
            sources.erase(page);
        }
        auto owner = context;
        if (!owner) return;
        if (!owner->nativeAlive) { context.reset(); return; }
        if (owner->detached && NativePageIdentity(owner) && !(Top() == owner->page && !owner->page->parent))
        {
            owner->nativeAlive = false;
            { DeletionScope deleting(owner->page); Depth scope(lifecycleDepth); destroyHook.thiscall<void>(owner->page); }
            if (context == owner) context.reset();
        }
    }
    void Drain()
    {
        if (!ready || draining || processDepth || lifecycleDepth || drawDepth || foreignDepth) return;
        struct DrainingScope
        {
            bool saved = draining;
            DrainingScope() { draining = true; }
            ~DrainingScope() { draining = saved; }
        } scope;
        Collect();
        auto* sourcePage = Top();
        if (candidates.count(sourcePage))
        {
            InstallSource(sourcePage);
            if (sources.count(sourcePage)) candidates.erase(sourcePage);
        }
        auto owner = context;
        if (owner && owner->returnPending && NativePageIdentity(owner) && Top() == owner->page && !owner->page->closed)
            Close(owner->page, -1);
        auto work = std::move(pending);
        pending.clear();
        for (const auto& action : work)
        {
            if (action.source) Open(action.source, action.serial);
            else Act(action);
        }
        owner = context;
        if (owner && !owner->retired && NativeOwned(owner))
        {
            if (Top() != owner->page)
            {
                if (!Covered(owner))
                {
                    Retire(owner);
                    owner->detached = true;
                }
            }
            else if (!owner->help && (owner->rebuildPending || RWGTAMenu::Model::Changed()))
            {
                if (!PrepareItems(owner))
                {
                    if (!(Covered(owner) && Top() != owner->page)) RequestReturn(owner);
                }
                else if (!Populate(owner)) RequestReturn(owner);
            }
        }
        Collect();
    }
    int __fastcall Loaded(Page* page, void*)
    {
        Depth scope(lifecycleDepth);
        auto result = initHook.thiscall<int>(page);
        if (ready && page && page->table == Address().pageTable) candidates.emplace(page);
        return result;
    }
    int __fastcall Process(Page* page, void*, int dx, int dy, int wheel, int buttons, int key, int modifiers, int delta)
    {
        const int controller = std::exchange(padKey, 0);
        if (!key && controller) { key = controller; modifiers = 0; }
        if (blocksInput && blocksInput())
            dx = dy = wheel = buttons = key = modifiers = 0;
        int result;
        { Depth scope(processDepth); result = processHook.thiscall<int>(page, dx, dy, wheel, buttons, key, modifiers, delta); }
        //5EBD27 onward reads fresh globals, world update/render; no old page
        // or control iterator is resumed after this complete native return.
        Drain();
        return result;
    }
    unsigned int __fastcall Draw(Control* control, void*, int flags)
    {
        unsigned int result = 0;
        {
            Depth scope(drawDepth);
            auto owner = context;
            // Preserve native title/world background, but suppress the covered
            // vanilla Options window while our distinct native window draws.
            if (!(owner && Covered(owner) && SourceAlive(owner->source) && control == owner->source->page->root))
                result = drawHook.thiscall<unsigned int>(control, flags);
        }
        return result;
    }
    int __fastcall PageDraw(Page* page, void*, int flags)
    {
        Depth scope(drawDepth);
        return pageDrawHook.thiscall<int>(page, flags);
    }
    unsigned int __fastcall Update(Control* control, void*, int flags)
    {
        Depth scope(drawDepth);
        return updateHook.thiscall<unsigned int>(control, flags);
    }
    uintptr_t __fastcall Destroyed(Page* page, void*)
    {
        const auto caller = reinterpret_cast<uintptr_t>(_ReturnAddress());
        // Logical Model.Close/native child destruction may synchronously
        // request this same page again. The first request owns retirement.
        if (deletionRequests.count(page)) return 0;
        DeletionScope deleting(page);
        auto owner = context;
        uintptr_t result = 0;
        const bool loopFinal = caller == Address().loopPopReturn;
        auto queued = std::find(deferredDeletes.begin(), deferredDeletes.end(), page);
        if (queued != deferredDeletes.end())
        {
            if (!loopFinal) return 0;
            deferredDeletes.erase(queued);
        }
        bool related = candidates.count(page) != 0 || sources.count(page) != 0 ||
            (owner && (owner->page == page || owner->returnParent == page || (owner->source && owner->source->originParent == page)));
        for (const auto& pair : sources) if (pair.second->originParent == page) related = true;
        //5EAC1E is the final native Pop:its only continuation is sound/global
        // cleanup, and a single outerProcess no longer has row iterators.
        const bool nativePop = caller == Address().finalPopReturn && processDepth == 1 && !drawDepth && !lifecycleDepth && !foreignDepth;
        const bool defer = related && !nativePop &&
            ((processDepth || drawDepth || lifecycleDepth || foreignDepth) || (!loopFinal && Top() == page));
        {
            Depth scope(lifecycleDepth);
            candidates.erase(page);
            auto source = sources.find(page);
            for (auto& pair : sources)
                if (pair.second->originParent == page)
                {
                    pair.second->retired = true;
                    if (pair.first)
                    {
                        pair.first->parent = page->parent;
                        pair.first->closed = 1;
                        pair.first->result = -1;
                    }
                }
            if (owner && owner->source && owner->source->originParent == page)
            {
                Retire(owner);
                owner->returnPending = true;
                if (NativePageIdentity(owner) && Bypass(owner->page, owner->returnParent)) owner->detached = true;
            }
            if (owner && owner->source && owner->source->page == page) { owner->returnPending = true; Retire(owner); }
            if (owner && owner->nativeAlive && owner->returnParent == page && NativePageIdentity(owner))
            {
                Retire(owner);
                owner->returnPending = true;
                auto* replacement = page->parent;
                if (Bypass(owner->page, page) && Bypass(page, replacement))
                {
                    owner->page->parent = replacement;
                    owner->returnParent = replacement;
                    owner->detached = true;
                }
                else Log("forced parent teardown stack contract refused");
            }
            if (owner && owner->page == page) Retire(owner);
            if (source != sources.end()) source->second->retired = true;
            if (defer)
            {
                Bypass(page, page->parent);
                if (std::find(deferredDeletes.begin(), deferredDeletes.end(), page) == deferredDeletes.end()) deferredDeletes.push_back(page);
                Log("native deletion deferred until complete scopes return");
            }
            else
            {
                if (owner && owner->page == page) owner->nativeAlive = false;
                result = destroyHook.thiscall<uintptr_t>(page);
                sources.erase(page);
            }
        }
        Collect();
        return result;
    }
    int __stdcall Pushed(Page* page, int command)
    {
        Depth scope(lifecycleDepth);
        return pushHook.stdcall<int>(page, command);
    }
    int __fastcall Closed(Page* page, void*, int result)
    {
        Depth scope(lifecycleDepth);
        auto owner = context;
        if (owner && (owner->page == page || owner->returnParent == page || (owner->source && owner->source->page == page)))
        {
            owner->returnPending = true;
            Retire(owner);
        }
        return closeHook.thiscall<int>(page, result);
    }
    void Init()
    {
        if (ready) return;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&Init), &module);
        // The native menu is built for Mafia 1.2, recognised by its page stack pop; the operands stay in the pattern
        // because 1.1 has the same code with other addresses
        if (hook::pattern("8B 0D 90 D8 6B 00 E8 1D E9 FF FF A1 2C 3A 6C 00 C7 05 90 D8 6B 00 00 00 00 00").empty()) return;
        Resolve();
        if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
            reinterpret_cast<LPCWSTR>(&Init), &module)) { Log("process lifetime pin refused"); return; }
        std::memcpy(buttonTable.data(), Address().buttonTable - 1, sizeof(buttonTable));
        buttonTable[11] = reinterpret_cast<uintptr_t>(&PrivateKeyboard);
        initHook = safetyhook::create_inline(reinterpret_cast<void*>(Address().loaded), &Loaded);
        processHook = safetyhook::create_inline(reinterpret_cast<void*>(Address().process), &Process);
        destroyHook = safetyhook::create_inline(reinterpret_cast<void*>(Address().destroyed), &Destroyed);
        drawHook = safetyhook::create_inline(reinterpret_cast<void*>(Address().draw), &Draw);
        pageDrawHook = safetyhook::create_inline(reinterpret_cast<void*>(Address().pageDraw), &PageDraw);
        updateHook = safetyhook::create_inline(reinterpret_cast<void*>(Address().update), &Update);
        pushHook = safetyhook::create_inline(reinterpret_cast<void*>(Address().push), &Pushed);
        closeHook = safetyhook::create_inline(reinterpret_cast<void*>(Address().close), &Closed);
        ready = true;
    }
}
