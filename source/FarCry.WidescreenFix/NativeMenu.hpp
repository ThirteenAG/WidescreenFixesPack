#pragma once
#include "NativeMenuFit.hpp"
#include "NativeMenuLua.hpp"
#include "FixSettings.hpp"
#include "../Shared/RWGTA/FixMenu.hpp"
#include <deque>
#include <memory>
#include <optional>
#include <sstream>

// Supported native host. Per-architecture contracts gate transactional Init;
// all native page actions are deferred until complete traversal scopes unwind.
namespace FarCryMenu
{
    enum class ItemKind { Row, Page, Next, Previous, Back };
    struct Item
    {
        ItemKind kind = ItemKind::Back;
        RWGTAMenu::Row row{};
        size_t target{};
        std::string label, value, description;
        bool enabled = true;
        FittedText fitted;
    };
    struct Context
    {
        NativePool native;
        RWGTAMenu::Model model;
        std::string epoch, serial;
        std::vector<Item> items;
        std::optional<size_t> page;
        size_t rootOffset{}, selected{};
        bool open{}, retired{}, returnPending{}, pause{};
        double layoutScaleY{};
        uint32_t lifetime{};
    };
    struct Pending
    {
        std::string serial;
        int kind{}, index{}, key{};
        void* sender{};
    };
    inline std::shared_ptr<Context> context;
    inline std::deque<Pending> pending;
    inline HMODULE hostModule{}, gameModule{}, scriptModule{}, fontModule{}, systemModule{};
    inline unsigned nativeDepth{}, foreignDepth{};
    inline bool draining{};
    inline uint32_t generation{};
    struct Depth
    {
        unsigned& counter;
        explicit Depth(unsigned& value) : counter(value) { ++counter; }
        ~Depth() { --counter; }
    };
    inline std::string Token()
    {
        if (generation == UINT32_MAX) return {};
        return std::to_string(++generation); // Lua receives a string, never float32.
    }
    inline std::string Quote(const std::string& text)
    {
        std::string output = "\"";
        for (const unsigned char c : text)
        {
            if (c == '\\' || c == '"') output += '\\';
            if (c < 32 || c > 126) return {}; // Only verified native literals.
            output += static_cast<char>(c);
        }
        return output + '"';
    }
    inline bool Current(const std::shared_ptr<Context>& owner)
    {
        return owner && context == owner && !owner->retired &&
            owner->native.Live(gameModule, scriptModule);
    }
    inline bool Available(const std::shared_ptr<Context>& owner)
    {
        return Current(owner) && !owner->native.Modal() && (Field<unsigned char>(owner->native.ui, uiFlagsOffset) & 4);
    }
    inline void Retire(const std::shared_ptr<Context>& owner)
    {
        if (!owner || owner->retired) return;
        owner->retired = true;
        owner->model.Close(); // Logical cancellation only; no native mutations.
    }
    inline bool Global(void* script, const char* key, std::string& output)
    {
        const char* value = nullptr;
        if (!Virtual<bool>(script, 35, key, &value) || !value) return false;
        output.clear();
        for (size_t offset = 0; offset <= 64; ++offset)
        {
            if (!Readable(value + offset, 1)) return false;
            if (!value[offset]) return true;
            if (offset == 64) return false;
            output += value[offset];
        }
        return false;
    }
    inline bool Call(const std::shared_ptr<Context>& owner, const std::string& expression)
    {
        if (!Available(owner)) return false;
        Depth scope(nativeDepth);
        if (!Execute(owner->native.script, "g_WFPMenuResult=tostring(" + expression + ");") || !Available(owner)) return false;
        std::string result;
        return Global(owner->native.script, "g_WFPMenuResult", result) && result == "1";
    }
    inline bool Snapshot(const std::shared_ptr<Context>& owner, const char* required)
    {
        if (!Available(owner)) return false;
        {
            Depth scope(nativeDepth);
            if (!Execute(owner->native.script, "WFP_Snapshot();") || !Available(owner)) return false;
        }
        std::string status, pause;
        if (!Global(owner->native.script, "g_WFPMenuStatus", status) || status != required) return false;
        if (Global(owner->native.script, "g_WFPMenuPause", pause)) owner->pause = pause == "1";
        return Available(owner);
    }
    inline void* FontService(const std::shared_ptr<Context>& owner)
    {
        if (!Available(owner)) return nullptr;
        const auto system = Field<void*>(owner->native.ui, uiSystemOffset);
        if (!Table(system, systemModule, systemTableRva)) return nullptr;
        return Field<void*>(system, systemFontOffset);
    }
    inline bool Build(const std::shared_ptr<Context>& owner)
    {
        if (!Available(owner) || !owner->model.IsOpen()) return false;
        const auto oldSerial = owner->serial;
        const auto current = [&] { return Available(owner) && owner->serial == oldSerial && owner->model.IsOpen(); };
        std::vector<Item> items;
        if (owner->page && *owner->page >= owner->model.Pages().size()) return false;
        auto title = owner->page ? owner->model.Pages()[*owner->page].title : owner->model.Text("menu.title");
        if (!NativeLiteral(title)) title = owner->model.Text("menu.title");
        if (!owner->page)
        {
            const auto pages = owner->model.Pages();
            for (size_t i = owner->rootOffset; i < pages.size() && items.size() < 4; ++i)
            {
                auto label = pages[i].title;
                if (!NativeLiteral(label)) label = owner->model.Text("menu.title") + " " + std::to_string(i + 1);
                items.push_back({ItemKind::Page, {}, i, std::move(label)});
            }
            if (owner->rootOffset) items.push_back({ItemKind::Previous, {}, owner->rootOffset >= 4 ? owner->rootOffset - 4 : 0, owner->model.Text("menu.previous")});
            if (owner->rootOffset + 4 < pages.size()) items.push_back({ItemKind::Next, {}, owner->rootOffset + 4, owner->model.Text("menu.next")});
        }
        else
        {
            std::vector<const RWGTAMenu::Row*> rows;
            { Depth scope(foreignDepth); rows = owner->model.VisibleRows(*owner->page); }
            if (!current()) return false;
            for (auto pointer : rows)
            {
                if (!current() || items.size() >= 6) return false;
                const auto row = *pointer;
                Item item{ItemKind::Row, row};
                { Depth scope(foreignDepth); item.enabled = owner->model.Enabled(row, owner->pause); }
                if (!current()) return false;
                item.label = owner->model.Label(row);
                if (!current()) return false;
                if (row.kind == RWGTAMenu::RowKind::Preset || row.kind == RWGTAMenu::RowKind::ApplyPreset)
                {
                    const auto scope = row.plugin ? OtherPlugins::Name(*row.plugin) : owner->model.Text("menu.gamepreset");
                    item.label = scope + ": " + item.label;
                }
                { Depth scope(foreignDepth); item.value = owner->model.Value(row); }
                if (!current()) return false;
                if (row.plugin && row.setting) item.description = OtherPlugins::Text(*row.plugin, row.setting->description);
                items.push_back(std::move(item));
            }
        }
        items.push_back({ItemKind::Back, {}, 0, owner->model.Text("menu.back")});
        const auto service = FontService(owner);
        if (!service || !NativeLiteral(title)) return false;
        const auto scaleY = Field<double>(owner->native.ui, verticalScaleOffset);
        if (!std::isfinite(scaleY) || scaleY <= 0) return false;
        const auto minimum = static_cast<float>(std::max(8.0, std::ceil(8.0 / scaleY)));
        for (auto& item : items)
        {
            if (!current()) return false;
            if (!NativeLiteral(item.label) || !NativeLiteral(item.value))
            {
                // Unsupported native glyph/escape route is explicit. Never
                // edit a row whose complete value cannot be represented.
                item.enabled = false;
                item.label = item.row.setting && NativeLiteral(item.row.setting->key) ?
                    item.row.setting->key : owner->model.Text("menu.unavailable");
                item.value = owner->model.Text("menu.unavailable");
            }
            const auto fitted = FitRow(service, fontModule, item.label, item.value, 524, 28, 20, minimum);
            if (!fitted || (!fitted->text.empty() && fitted->extent.y * scaleY < 8)) return false; // Complete, readable values only.
            item.fitted = *fitted;
        }
        const auto heading = FitRow(service, fontModule, title, "", 524, 28, 24, 24);
        if (!heading || heading->extent.y * scaleY < 8 || !current()) return false;
        const auto serial = Token();
        if (serial.empty() || !current()) return false;
        auto selected = std::min(owner->selected, items.size() - 1);
        if (!items[selected].enabled)
            for (size_t i = 0; i < items.size(); ++i) if (items[i].enabled) { selected = i; break; }
        std::string expression = "WFP_Commit(" + Quote(serial) + "," + Quote(heading->text) + ",{ ";
        for (const auto& item : items)
            expression += "{text=" + Quote(item.fitted.text) + ",size=" + std::to_string(static_cast<int>(item.fitted.size)) +
                ",enabled=" + (item.enabled ? "1" : "0") + "},";
        expression += "}," + std::to_string(selected) + ")";
        if (!Call(owner, expression) || !current()) return false;
        owner->items = std::move(items);
        owner->serial = serial;
        owner->selected = selected;
        owner->layoutScaleY = scaleY;
        RWGTAMenu::Model::Changed() = false;
        if (owner->open && (!Call(owner, "WFP_Select(" + std::to_string(selected) + ")") ||
            !Available(owner) || owner->serial != serial || !owner->native.Selected(selected))) return false;
        return true;
    }
    inline bool Queue(Pending action)
    {
        auto owner = context;
        if (!owner) return false;
        if (action.kind == 5)
        {
            if (action.serial != owner->epoch || !owner->native.Owns(action.sender)) return false;
            owner->native.retired = true;
            Retire(owner);
            return true;
        }
        if (pending.size() >= 32 || !Available(owner)) return false;
        bool cancelForeign = false;
        if (action.kind == 0)
        {
            if (action.serial != owner->epoch || action.sender != owner->native.entry ||
                Field<void*>(owner->native.ui, focusOffset) != action.sender) return false;
        }
        else
        {
            if (!owner->open || action.serial != owner->serial || action.index < 0 ||
                static_cast<size_t>(action.index) >= owner->items.size() ||
                action.sender != owner->native.rows[action.index] || !owner->native.Selected(action.index)) return false;
            // Supported installed32 and64 GetKeyName non-GUI switch proves
            // Escape9 and arrows15..18. Never rely on GUI string key names.
            if (action.kind == 2 && action.key != 9 && (action.key < 15 || action.key > 18)) return false;
            const auto back = (action.kind == 2 && action.key == 9) ||
                (action.kind == 1 && owner->items[action.index].kind == ItemKind::Back);
            cancelForeign = back && foreignDepth;
        }
        // Admission must finish before logical cancellation. Allocation failure
        // leaves both the model and native input behavior unchanged.
        try { pending.push_back(std::move(action)); }
        catch (...) { return false; }
        if (cancelForeign)
        {
            owner->returnPending = true;
            Retire(owner); // Abort ongoing shared API burst; native Back is deferred.
        }
        return true;
    }
    inline bool NativeCurrent(const std::shared_ptr<Context>& owner)
    {
        return owner && context == owner && owner->native.Live(gameModule, scriptModule);
    }
    inline bool NativeCall(const std::shared_ptr<Context>& owner, const std::string& expression)
    {
        if (!NativeCurrent(owner) || owner->native.Modal()) return false;
        Depth scope(nativeDepth);
        if (!Execute(owner->native.script, "g_WFPMenuResult=tostring(" + expression + ");") ||
            !NativeCurrent(owner) || owner->native.Modal()) return false;
        std::string result;
        return Global(owner->native.script, "g_WFPMenuResult", result) && result == "1";
    }
    inline bool Leave(const std::shared_ptr<Context>& owner)
    {
        if (!NativeCurrent(owner)) return false;
        if (owner->native.Modal()) { owner->returnPending = true; return false; }
        if (!NativeCall(owner, "WFP_Leave()")) { owner->returnPending = true; return false; }
        owner->returnPending = false;
        owner->open = false;
        owner->retired = false;
        owner->model.Close();
        owner->items.clear();
        owner->page.reset();
        owner->selected = owner->rootOffset = 0;
        return true;
    }
    inline void Open(const std::shared_ptr<Context>& owner)
    {
        if (!Snapshot(owner, "options") || Field<void*>(owner->native.ui, focusOffset) != owner->native.entry) return;
        const auto serial = owner->serial;
        auto language = FixSettings().String("MAIN.TextLanguage");
        if (language != "en" && language != "fr" && language != "de" && language != "it" && language != "es")
            language = NativeLanguage(owner->native.script);
        { Depth scope(foreignDepth); owner->model.OpenPlugins(hostModule, language, 5); }
        if (!Available(owner) || owner->serial != serial || !owner->model.IsOpen() ||
            !Snapshot(owner, "options") || Field<void*>(owner->native.ui, focusOffset) != owner->native.entry) return;
        owner->page.reset();
        owner->selected = owner->rootOffset = 0;
        if (!Build(owner)) { owner->model.Close(); return; }
        if (!Call(owner, "WFP_Enter()") || !Snapshot(owner, "private"))
        {
            owner->returnPending = true;
            Retire(owner);
            return;
        }
        owner->open = true;
    }
    inline void Describe(const std::shared_ptr<Context>& owner, size_t index)
    {
        if (!Available(owner) || index >= owner->items.size()) return;
        auto service = FontService(owner);
        const auto scaleY = Field<double>(owner->native.ui, verticalScaleOffset);
        auto fitted = std::isfinite(scaleY) && scaleY > 0 ?
            FitRow(service, fontModule, owner->items[index].description, "", 524, 52, 14,
                static_cast<float>(std::max(8.0, std::ceil(8.0 / scaleY)))) : std::optional<FittedText>();
        if (fitted && !fitted->text.empty() && fitted->extent.y * scaleY < 8) fitted.reset();
        if (Available(owner)) Call(owner, "WFP_Describe(" + Quote(fitted ? fitted->text : std::string()) + "," +
            std::to_string(static_cast<int>(fitted ? fitted->size : 14)) + ")");
    }
    inline void Act(const std::shared_ptr<Context>& owner, const Pending& action)
    {
        if (!Available(owner) || action.serial != owner->serial || action.index < 0 ||
            static_cast<size_t>(action.index) >= owner->items.size() || !Snapshot(owner, "private") ||
            action.sender != owner->native.rows[action.index] || !owner->native.Selected(action.index)) return;
        const auto item = owner->items[action.index];
        owner->selected = static_cast<size_t>(action.index);
        if (action.kind == 3 || action.kind == 4)
        {
            owner->selected = action.index;
            Describe(owner, owner->selected);
            return;
        }
        if (action.kind == 2 && (action.key == 16 || action.key == 18))
        {
            size_t selected = action.index;
            for (size_t tries = 0; tries < owner->items.size(); ++tries)
            {
                selected = action.key == 16 ? (selected + owner->items.size() - 1) % owner->items.size() :
                    (selected + 1) % owner->items.size();
                if (owner->items[selected].enabled) break;
            }
            if (Call(owner, "WFP_Select(" + std::to_string(selected) + ")")) owner->selected = selected;
            Describe(owner, owner->selected);
            return;
        }
        const auto accept = action.kind == 1;
        const auto back = action.kind == 2 && action.key == 9;
        const auto direction = accept ? 0 : action.key == 15 ? -1 : 1;
        const auto oldPage = owner->page;
        const auto oldOffset = owner->rootOffset;
        if (back || (accept && item.kind == ItemKind::Back))
        {
            if (!owner->page) { Leave(owner); return; }
            owner->page.reset();
        }
        else if (accept && item.kind == ItemKind::Page)
        {
            if (item.target >= owner->model.Pages().size()) return;
            owner->page = item.target;
            owner->model.PageShown(item.target);
        }
        else if (accept && (item.kind == ItemKind::Next || item.kind == ItemKind::Previous)) owner->rootOffset = item.target;
        else if (item.kind == ItemKind::Row)
        {
            if (!item.enabled || (!accept && item.row.kind != RWGTAMenu::RowKind::Setting &&
                item.row.kind != RWGTAMenu::RowKind::Preset)) return;
            bool enabled;
            { Depth scope(foreignDepth); enabled = owner->model.Enabled(item.row, owner->pause); }
            if (!enabled || !Available(owner) || owner->serial != action.serial ||
                !owner->native.Selected(action.index) || !Snapshot(owner, "private")) return;
            { Depth scope(foreignDepth); owner->model.Change(item.row, direction, owner->pause); }
            if (!Available(owner) || owner->serial != action.serial || !Snapshot(owner, "private")) return;
        }
        else return;
        owner->selected = 0;
        if (!Build(owner))
        {
            owner->page = oldPage;
            owner->rootOffset = oldOffset;
            // Failed native/foreign commit never leaves old visible widgets
            // bound to new navigation. Return through the native parent route.
            owner->returnPending = true;
            Retire(owner);
            return;
        }
        Call(owner, "WFP_Select(" + std::to_string(owner->selected) + ")");
        Describe(owner, owner->selected);
    }
    inline void Drain()
    {
        if (nativeDepth || foreignDepth || draining) return;
        struct Draining { Draining() { draining = true; } ~Draining() { draining = false; } } scope;
        auto owner = context;
        if (!owner) { pending.clear(); return; }
        if (!owner->open && !owner->returnPending && !owner->retired && pending.empty()) return;
        if (!NativeCurrent(owner))
        {
            Retire(owner);
            pending.clear();
            // Missing private controls do not authorize touching them. Lua's
            // retirement path checks exact screen/table ownership and only
            // restores a still-active owned screen to the native Options.
            auto ui = owner->native.ui;
            if (Table(ui, gameModule, uiTableRva) && Readable(ui, uiBytes) &&
                Field<void*>(ui, uiScriptOffset) == owner->native.script &&
                Table(owner->native.script, scriptModule, scriptTableRva))
            {
                if (Field<void*>(ui, modalOffset)) return;
                Depth depth(nativeDepth);
                Execute(owner->native.script, "WFP_Retire(" + Quote(owner->epoch) + ");");
            }
            if (context == owner) context.reset();
            return;
        }
        if (owner->native.Modal()) return; // Hold covered owner; no Model calls.
        if (owner->open && !Field<unsigned char>(owner->native.page, screenActiveOffset))
        {
            // A stock transition can cover/leave the private page without a
            // foreign settings callback. Drop only the logical session at the
            // safe native boundary; keep the engine-owned pool reusable.
            owner->returnPending = owner->retired = owner->open = false;
            owner->model.Close(); owner->items.clear(); owner->page.reset();
            owner->selected = owner->rootOffset = 0;
            pending.clear();
            return;
        }
        if (owner->returnPending)
        {
            if (!Field<unsigned char>(owner->native.page, screenActiveOffset))
            {
                owner->returnPending = owner->retired = owner->open = false;
                owner->model.Close(); owner->items.clear(); owner->page.reset();
                owner->selected = owner->rootOffset = 0;
            }
            else Leave(owner);
            pending.clear();
            return;
        }
        while (!pending.empty())
        {
            auto action = std::move(pending.front());
            pending.pop_front();
            if (!Available(owner)) break;
            if (action.kind == 0 && action.serial == owner->epoch) Open(owner);
            else Act(owner, action);
        }
        if (Available(owner) && owner->open && Snapshot(owner, "private") && (RWGTAMenu::Model::Changed() ||
            owner->layoutScaleY != Field<double>(owner->native.ui, verticalScaleOffset)))
        {
            if (!Build(owner)) { owner->returnPending = true; Retire(owner); }
        }
    }

}
