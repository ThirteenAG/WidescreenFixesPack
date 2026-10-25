#pragma once
#include "NativeMenuContracts.hpp"
#include "../Shared/RWGTA/FixMenu.hpp"
#include <deque>
#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <intrin.h>

namespace MafiaMenu
{
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
        size_t rootOffset = 0;
        bool retired = false;
        bool nativeAlive = false;
        bool detached = false;
        bool returnPending = false;
        bool rebuildPending = false;
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
    inline std::unordered_map<Page*, std::shared_ptr<Source>> sources;
    inline std::shared_ptr<Context> context;
    inline std::deque<Pending> pending;
    inline std::unordered_set<Page*> candidates;
    inline std::deque<Page*> deferredDeletes;
    inline std::unordered_set<Page*> deletionRequests;
    inline uint64_t generation = 0;
    inline unsigned int processDepth = 0, lifecycleDepth = 0, drawDepth = 0, foreignDepth = 0;
    inline bool ready = false, draining = false;
    inline HMODULE module = nullptr;
    inline SafetyHookInline initHook, processHook, destroyHook, drawHook, pageDrawHook, updateHook, pushHook, closeHook;
    inline std::array<uintptr_t, 32> buttonTable{};
    // Failures only, once each, to the debugger
    inline void Log(const char* gate)
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
    inline bool SourceAlive(const std::shared_ptr<Source>& source)
    {
        if (!source || source->retired) return false;
        auto found = sources.find(source->page);
        return found != sources.end() && found->second == source && Readable(source->page, sizeof(Page)) &&
            !source->page->closed &&
            source->page->parent == source->originParent &&
            source->page->table == source->table.data() + 1 && Contains(source->page, source->entry) && Contains(source->page, source->prototype) &&
            source->entry->parent == source->page->root;
    }
    inline bool NativePageIdentity(const std::shared_ptr<Context>& owner)
    {
        return owner && owner == context && owner->nativeAlive && Readable(owner->page, sizeof(Page)) &&
            owner->page->table == owner->table.data() + 1 && owner->page->parent == owner->returnParent;
    }
    inline bool NativeIdentity(const std::shared_ptr<Context>& owner)
    {
        return NativePageIdentity(owner) && Readable(owner->page->root, sizeof(Control));
    }
    inline bool NativeOwned(const std::shared_ptr<Context>& owner)
    {
        return NativeIdentity(owner) && SourceAlive(owner->source);
    }
    inline bool Covered(const std::shared_ptr<Context>& owner)
    {
        if (!NativeIdentity(owner)) return false;
        auto* page = Top();
        for (unsigned int count = 0; page && count < 64; ++count)
        {
            if (page == owner->page) return true;
            if (!Readable(page, sizeof(Page))) return false;
            page = page->parent;
        }
        return false;
    }
    inline bool Current(const std::shared_ptr<Context>& owner)
    {
        return NativeOwned(owner) && !owner->retired && Top() == owner->page && !owner->page->closed;
    }
    inline bool Active(const std::shared_ptr<Context>& owner, uint32_t selected)
    {
        if (!Current(owner) || !Contains(owner->page, owner->page->selected) || owner->page->selected->id != selected) return false;
        for (const auto& item : owner->items)
            if (item.native == owner->page->selected)
                return item.native->parent == owner->page->root && item.enabled;
        return false;
    }
    inline void Retire(const std::shared_ptr<Context>& owner)
    {
        if (!owner || owner->retired) return;
        owner->retired = true;
        owner->model.Close();
    }
    inline bool Queue(const std::shared_ptr<Context>& owner, uint32_t selected, int direction, bool back)
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
    inline std::optional<std::string> NativeText(const std::string& text)
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
    inline bool Fit(std::string& text, float& height, float width, unsigned int font, bool fullValue)
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
    inline bool ReadScope(const std::shared_ptr<Context>& owner)
    {
        if (!owner || owner != context || owner->retired || !SourceAlive(owner->source)) return false;
        if (owner->nativeAlive) return Current(owner);
        auto* source = owner->source->page;
        return Top() == source && source->selected == owner->source->entry && !source->closed;
    }
    template<class Call> inline auto Foreign(Call&& call)
    {
        Depth scope(foreignDepth);
        return call();
    }
    // Rows from the top of the private page to its help line: title gap, rows 0.35 apart, help line
    inline float RowsHeight(float rowHeight, size_t rows)
    {
        return rowHeight * (3.6f + static_cast<float>(rows - 1) * 1.35f);
    }
    inline size_t FittingRows(float rowHeight)
    {
        const auto scaleY = *reinterpret_cast<float*>(0x6BD88C);
        size_t rows = maxRows;
        while (rows > minRows && RowsHeight(rowHeight, rows) + 72 * scaleY > 540 * scaleY) --rows;
        return rows;
    }
    inline bool PrepareItems(const std::shared_ptr<Context>& owner)
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
            if (!current() || rows.size() > owner->rowCount - 1) return false;
            for (size_t index = 0; index < rows.size(); ++index)
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
                next.back().description = restart && owner->model.RestartRequired() ? owner->model.Text("menu.restartwarning") : owner->model.Help(row);
            }
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
    inline int __fastcall SourceCommand(Page* page, void*, int command);
    inline void InstallSource(Page* page)
    {
        if (!ready || sources.count(page) || Top() != page || !Readable(page, sizeof(Page)) || page->closed) return;
        if (page->table != reinterpret_cast<uintptr_t*>(0x62733C) || !Readable(page->parent, sizeof(Page)))
        {
            Log("source context refused");
            return;
        }
        const bool pause = page->command == 108 && page->parent->table == reinterpret_cast<uintptr_t*>(0x625E58);
        const bool title = page->command == 990110 && page->parent->table == reinterpret_cast<uintptr_t*>(0x625EB8);
        if (!pause && !title) { Log("source title/pause identity refused"); return; }
        auto* root = page->root;
        if (!Controls(root) || (root->table != reinterpret_cast<uintptr_t*>(0x625FF8) && root->table != reinterpret_cast<uintptr_t*>(0x6260F8)))
        {
            Log("source root style refused");
            return;
        }
        Control* control = nullptr;
        Control* audio = nullptr;
        Control* back = nullptr;
        const auto first = reinterpret_cast<uintptr_t>(page->begin);
        const auto last = reinterpret_cast<uintptr_t>(page->end);
        if (last < first || (last - first) % 4 || last - first > 4096 * 4 || !Readable(page->begin, last - first))
        {
            Log("source widget vector refused");
            return;
        }
        for (auto** entry = page->begin; entry < page->end; ++entry)
        {
            auto* node = *entry;
            if (!Readable(node, sizeof(Control))) { Log("source widget unreadable"); return; }
            if (node->id == entryId) { Log("source category ID collision"); return; }
            if (node->id == 100) control = node;
            if (node->id == 101) audio = node;
            if (node->id == 400) back = node;
        }
        for (auto* node : { control, audio, back })
            if (!node || node->parent != root || node->table != reinterpret_cast<uintptr_t*>(0x626900) ||
                (node->flags & 0x8030) != 0x8030 || !std::isfinite(node->width) || !std::isfinite(node->height) || node->width <= 0 || node->height <= 0)
            { Log("source button contract refused"); return; }
        const auto scaleX = *reinterpret_cast<float*>(0x6BD888);
        const auto scaleY = *reinterpret_cast<float*>(0x6BD88C);
        const auto stockEnd = std::max(control->relativeY + control->height, audio->relativeY + audio->height);
        const auto availableGap = back->relativeY - stockEnd;
        const auto margin = std::max(2.0f * scaleY, std::min(control->height * 0.1f, availableGap * 0.25f));
        auto height = std::min(back->height, availableGap - 2.0f * margin);
        const auto y = stockEnd + margin;
        const auto header = root->table == reinterpret_cast<uintptr_t*>(0x6260F8) ? 40 * scaleY : 0;
        if (!std::isfinite(scaleX) || !std::isfinite(scaleY) || scaleX <= 0 || scaleY <= 0 || !std::isfinite(y) ||
            !std::isfinite(back->relativeY) || !std::isfinite(root->height) || !std::isfinite(availableGap) || availableGap <= 0 ||
            !std::isfinite(margin) || margin <= 0 || !std::isfinite(height) || height < std::max(8.0f, 8.0f * scaleY) || y < 0 ||
            y + height + margin > back->relativeY || y + height > root->height - header)
        { Log("source category layout refused"); return; }
        auto source = std::make_shared<Source>();
        source->page = page;
        source->originParent = page->parent;
        source->prototype = back; // Native compact row keeps the private pool/footer separate.
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
        if (native->parent != root || native->id != entryId || native->table != reinterpret_cast<uintptr_t*>(0x626900) || !Append(page, native))
        {
            // Exact new object rollback spans both the parent relationship and
            // the page's destructor ownership. Never delete a borrowed node.
            if (Detach(native)) DestroyControl(native);
            Log("source partial category construction refused");
            return;
        }
        native->text = source->caption.c_str();
        reinterpret_cast<void (__thiscall*)(Control*, int)>(0x5D9220)(native, 0);
        source->entry = native;
        sources.emplace(page, source);
        page->table = source->table.data() + 1;
    }
    inline int __fastcall SourceCommand(Page* page, void*, int command)
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
    inline int __fastcall PrivateCommand(Page*, void*, int);
    inline int __fastcall PrivateKey(Page*, void*, int, int);
    inline int __fastcall PrivateSelected(Page*, void*, Control*, Control*);
    inline void Describe(const std::shared_ptr<Context>& owner, uint32_t selected);
    inline int __fastcall PrivateKeyboard(Control*, void*, int, int);
    inline bool PrepareNativePage(const std::shared_ptr<Context>& owner)
    {
        if (!NativeOwned(owner) || Top() != owner->page || owner->prepared.empty() || owner->prepared.size() > owner->rowCount) return false;
        auto* page = owner->page;
        auto* root = page->root;
        auto* prototype = owner->source->prototype;
        if (!Controls(root) || root->parent || root->table != reinterpret_cast<uintptr_t*>(0x6260F8) ||
            !Contains(owner->source->page, prototype)) { Log("private root style refused"); return false; }
        const auto scaleX = *reinterpret_cast<float*>(0x6BD888);
        const auto scaleY = *reinterpret_cast<float*>(0x6BD88C);
        if (!std::isfinite(scaleX) || !std::isfinite(scaleY) || scaleX <= 0 || scaleY <= 0 ||
            !std::isfinite(root->width) || root->width < 160 * scaleX || !std::isfinite(prototype->height) || prototype->height <= 0)
        { Log("private page geometry refused"); return false; }
        const auto rowHeight = prototype->height;
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
        auto nativeTitle = owner->preparedTitle;
        auto titleHeight = 32 * scaleY;
        if (!Fit(nativeTitle, titleHeight, root->width, 2, false)) { Log("private header font refused"); return false; }
        // All original controls remain page-owned for native destruction. They
        // are hidden only on this new private page, never on vanilla Options.
        for (auto** item = page->begin; item < page->end; ++item)
        {
            if (!Readable(*item, sizeof(Control))) { Log("private template widget refused"); return false; }
            if (*item != root) (*item)->flags &= ~(0x10u | 0x80000000u);
        }
        if (page->selected) page->selected->flags &= ~0x80000000u;
        page->selected = nullptr;
        root->height = height;
        root->relativeY = (viewportHeight - height) * 0.5f;
        reinterpret_cast<void (__thiscall*)(Control*, int)>(0x5D9220)(root, 0);
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
            if (node->parent != root || node->id != id || node->table != reinterpret_cast<uintptr_t*>(0x626900) || !Append(page, node))
            {
                if (Detach(node)) DestroyControl(node);
                return nullptr;
            }
            if (!focusable) node->flags &= ~0x8000u;
            else node->table = buttonTable.data() + 1;
            node->text = "";
            reinterpret_cast<void (__thiscall*)(Control*, int)>(0x5D9220)(node, 0);
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
    inline bool Populate(const std::shared_ptr<Context>& owner)
    {
        if (!Current(owner) || owner->prepared.empty() || owner->prepared.size() > owner->rowCount ||
            !Contains(owner->page, owner->description) || owner->description->parent != owner->page->root) return false;
        auto title = owner->preparedTitle;
        const auto scaleY = *reinterpret_cast<float*>(0x6BD88C);
        auto titleHeight = 32 * scaleY;
        if (!Fit(title, titleHeight, owner->page->root->width, 2, false)) return false;
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
            if (!Fit(item.label, labelHeights[index], owner->labelWidth, label->flags & 15, false) ||
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
        Select(owner->page, selected);
        if (!Current(owner) || !Contains(owner->page, owner->page->selected)) return false;
        Describe(owner, owner->page->selected->id);
        return Current(owner) && Contains(owner->page, owner->page->selected);
    }
    inline void RequestReturn(const std::shared_ptr<Context>& owner)
    {
        if (!owner || owner != context) return;
        owner->returnPending = true;
        Retire(owner);
        if (NativePageIdentity(owner) && Top() == owner->page && !owner->page->closed)
            Close(owner->page, -1);
    }
    inline void Describe(const std::shared_ptr<Context>& owner, uint32_t selected)
    {
        if (!Active(owner, selected) || !Contains(owner->page, owner->description)) return;
        for (const auto& item : owner->items)
        {
            if (item.native->id != selected) continue;
            auto description = item.description;
            auto height = owner->source->prototype->height;
            const auto fullPageName = item.kind == ItemKind::Page;
            if (!Fit(description, height, owner->description->width, owner->description->flags & 15, fullPageName)) return;
            owner->descriptionText = std::move(description);
            owner->description->height = height;
            owner->description->text = owner->descriptionText.c_str();
            return;
        }
    }
    inline void Open(const std::shared_ptr<Source>& source, uint64_t serial)
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
        owner->rowCount = FittingRows(source->prototype->height);
        Foreign([&] { owner->model.OpenPlugins(module, language, owner->rowCount - 1); });
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
        reinterpret_cast<void (__thiscall*)(Page*)>(0x5EA1F0)(page);
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
    inline int __fastcall PrivateKeyboard(Control* control, void*, int modifiers, int key)
    {
        auto owner = context;
        Depth scope(lifecycleDepth);
        if (owner && Active(owner, control->id) && (key == 203 || key == 205 || key == 57 || key == 28 || key == 156))
        {
            if (Queue(owner, control->id, key == 203 ? -1 : key == 205 ? 1 : 0, false)) return static_cast<int>(consumedId);
        }
        return reinterpret_cast<int (__thiscall*)(Control*, int, int)>(0x5DAF60)(control, modifiers, key);
    }
    inline int __fastcall PrivateKey(Page* page, void*, int key, int modifiers)
    {
        auto owner = context;
        Depth scope(lifecycleDepth);
        if ((key & 255) == 1 && owner && owner->page == page && Queue(owner, page->selected ? page->selected->id : 0, 0, true)) return 0;
        // Native Escape remains available if queue capacity/refusal prevents
        // logical navigation, including incomplete/retired private contexts.
        return reinterpret_cast<int (__thiscall*)(Page*, int, int)>(0x5498F0)(page, key, modifiers);
    }
    inline int __fastcall PrivateCommand(Page* page, void*, int command)
    {
        auto owner = context;
        Depth scope(lifecycleDepth);
        if (command == static_cast<int>(consumedId)) return 0;
        if (owner && owner->page == page) Queue(owner, static_cast<uint32_t>(command), 0, false);
        return 0;
    }
    inline int __fastcall PrivateSelected(Page* page, void*, Control* selected, Control*)
    {
        auto owner = context;
        Depth scope(lifecycleDepth);
        if (owner && owner->page == page && selected && Active(owner, selected->id) && pending.size() < 32)
            pending.push_back({ {}, owner, owner->serial, selected->id, 0, false, true });
        return 0;
    }
    inline void Act(const Pending& action)
    {
        auto owner = action.owner;
        if (!Current(owner) || owner->serial != action.serial) return;
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
                    owner->model.PageShown(item.index);
                    break;
                case ItemKind::Previous: owner->rootOffset = owner->rootOffset >= 4 ? owner->rootOffset - 4 : 0; break;
                case ItemKind::Next: owner->rootOffset = item.index; break;
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
    inline bool Bypass(Page* target, Page* replacement)
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
                    if (replacement) *reinterpret_cast<Page**>(0x6BD890) = replacement;
                    else
                    {
                        //5EBD6E unconditionally deletes Top at loop exit. Keep
                        //the exact last page alive until that final call.
                        target->closed = 1;
                        target->result = -1;
                        if (!*reinterpret_cast<int*>(0x6BD8A8)) *reinterpret_cast<int*>(0x6BD8A8) = -1;
                    }
                }
                return true;
            }
            if (!Readable(page, sizeof(Page))) return false;
            for (size_t index = 0; index < count; ++index) if (seen[index] == page) return false;
            seen[count] = page;
            previous = page;
            page = page->parent;
        }
        return page == nullptr;
    }
    inline void Collect()
    {
        if (processDepth || lifecycleDepth || drawDepth || foreignDepth) return;
        // Actual native deletion is leased to complete input/draw/lifecycle
        // and foreign API return, not merely to a retained C++Context.
        for (size_t remaining = deferredDeletes.size(); remaining; --remaining)
        {
            auto* page = deferredDeletes.front();
            deferredDeletes.pop_front();
            if (Top() == page && Readable(page, sizeof(Page)) && !page->parent)
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
    inline void Drain()
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
            else if (owner->rebuildPending || RWGTAMenu::Model::Changed())
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
    inline int __fastcall Loaded(Page* page, void*)
    {
        Depth scope(lifecycleDepth);
        auto result = initHook.thiscall<int>(page);
        if (ready && Readable(page, sizeof(Page)) && page->table == reinterpret_cast<uintptr_t*>(0x62733C)) candidates.emplace(page);
        return result;
    }
    inline int __fastcall Process(Page* page, void*, int dx, int dy, int buttons, int key, int keyEvent, int modifiers, int delta)
    {
        int result;
        { Depth scope(processDepth); result = processHook.thiscall<int>(page, dx, dy, buttons, key, keyEvent, modifiers, delta); }
        //5EBD27 onward reads fresh globals, world update/render; no old page
        // or control iterator is resumed after this complete native return.
        Drain();
        return result;
    }
    inline unsigned int __fastcall Draw(Control* control, void*, int flags)
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
    inline int __fastcall PageDraw(Page* page, void*, int flags)
    {
        Depth scope(drawDepth);
        return pageDrawHook.thiscall<int>(page, flags);
    }
    inline unsigned int __fastcall Update(Control* control, void*, int flags)
    {
        Depth scope(drawDepth);
        return updateHook.thiscall<unsigned int>(control, flags);
    }
    inline uintptr_t __fastcall Destroyed(Page* page, void*)
    {
        const auto caller = reinterpret_cast<uintptr_t>(_ReturnAddress());
        // Logical Model.Close/native child destruction may synchronously
        // request this same page again. The first request owns retirement.
        if (deletionRequests.count(page)) return 0;
        DeletionScope deleting(page);
        auto owner = context;
        uintptr_t result = 0;
        const bool loopFinal = caller == 0x5EBD73;
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
        const bool nativePop = caller == 0x5EAC23 && processDepth == 1 && !drawDepth && !lifecycleDepth && !foreignDepth;
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
                    if (Readable(pair.first, sizeof(Page)))
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
    inline int __stdcall Pushed(Page* page, int command)
    {
        Depth scope(lifecycleDepth);
        return pushHook.stdcall<int>(page, command);
    }
    inline int __fastcall Closed(Page* page, void*, int result)
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
    inline void Init()
    {
        if (ready) return;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&Init), &module);
        if (!Contracts()) { Log("native executable/vtable contract refused"); return; }
        if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
            reinterpret_cast<LPCWSTR>(&Init), &module)) { Log("process lifetime pin refused"); return; }
        std::memcpy(buttonTable.data(), reinterpret_cast<void*>(0x6268FC), sizeof(buttonTable));
        buttonTable[11] = reinterpret_cast<uintptr_t>(&PrivateKeyboard);
        initHook = safetyhook::create_inline(reinterpret_cast<void*>(0x5EA2E0), &Loaded);
        processHook = safetyhook::create_inline(reinterpret_cast<void*>(0x5EA8C0), &Process);
        destroyHook = safetyhook::create_inline(reinterpret_cast<void*>(0x5EA690), &Destroyed);
        drawHook = safetyhook::create_inline(reinterpret_cast<void*>(0x5E9D60), &Draw);
        pageDrawHook = safetyhook::create_inline(reinterpret_cast<void*>(0x5EA7E0), &PageDraw);
        updateHook = safetyhook::create_inline(reinterpret_cast<void*>(0x5E9DA0), &Update);
        pushHook = safetyhook::create_inline(reinterpret_cast<void*>(0x5EAD50), &Pushed);
        closeHook = safetyhook::create_inline(reinterpret_cast<void*>(0x5EAE10), &Closed);
        if (!initHook || !processHook || !destroyHook || !drawHook || !pageDrawHook || !updateHook || !pushHook || !closeHook)
        {
            closeHook.reset(); pushHook.reset(); updateHook.reset(); pageDrawHook.reset(); drawHook.reset(); destroyHook.reset(); processHook.reset(); initHook.reset();
            Log("native hook transaction refused"); return;
        }
        ready = true;
    }
}
