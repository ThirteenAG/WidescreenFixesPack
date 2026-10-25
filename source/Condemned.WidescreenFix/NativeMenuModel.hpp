#pragma once
#include "../Shared/RWGTA/FixMenu.hpp"
#include <cstdint>
#include <optional>
#include <utility>

// Disconnected logical preparation only. No native allocation, publication,
// callbacks or hooks. A future host must commit a Prepared view only after its
// exact native owner and traversal lease are revalidated.
namespace CondemnedMenuNative
{
    enum class ItemKind { Link, ModelRow, Previous, Next, Back };
    struct Item
    {
        ItemKind kind{ItemKind::Back};
        size_t destination{};
        RWGTAMenu::Row row{};
        std::string label, value;
        bool enabled{true};
        std::string description, requiredTail;
    };
    struct Location
    {
        std::optional<size_t> page;
        size_t first{};
        bool operator==(const Location&) const = default;
    };
    struct Prepared
    {
        uint64_t revision{};
        Location location;
        bool resetsPageState{};
        std::string title;
        std::vector<Item> items;
    };
    class Session
    {
        RWGTAMenu::Model model;
        uint64_t revision{1};
        bool pause{};
        size_t foreignDepth{};
        Location shown;
        struct ForeignScope
        {
            Session& owner;
            explicit ForeignScope(Session& value) : owner(value) { ++owner.foreignDepth; }
            ~ForeignScope() { --owner.foreignDepth; }
        };
        template<class Current> bool Alive(uint64_t token, Current& current) const
        {
            return token == revision && model.IsOpen() && current();
        }
    public:
        static constexpr size_t pageRows = 5;
        uint64_t Revision() const { return revision; }
        bool IsOpen() const { return model.IsOpen(); }
        bool RestartRequired() const { return model.RestartRequired(); }
        bool InForeignCall() const { return foreignDepth != 0; }
        std::string Text(const std::string& key) const { return model.Text(key); }
        bool NeedsRefresh() const { return RWGTAMenu::Model::Changed().load(std::memory_order_acquire); }
        void BeginRefresh() { RWGTAMenu::Model::Changed().exchange(false, std::memory_order_acq_rel); }
        const Location& Shown() const { return shown; }
        // The native host admits Back only after capacity/selected-owner checks.
        // Logical cancellation can occur during a plugin callback; native controls
        // must remain intact until every enclosing engine traversal has returned.
        void Cancel()
        {
            ++revision;
            model.Close();
        }
        template<class Current> bool Open(HMODULE host, const std::string& language, bool inPause, Current current)
        {
            Cancel();
            const auto token = revision;
            pause = inPause;
            shown = {};
            { ForeignScope scope(*this); model.OpenPlugins(host, language, pageRows); }
            if (!Alive(token, current))
            {
                if (token == revision) Cancel();
                return false;
            }
            return true;
        }
        template<class Current> std::optional<Prepared> Prepare(Location location, Current current)
        {
            const auto token = revision;
            if (!Alive(token, current)) return {};
            const bool entering = location.page && location.page != shown.page;
            auto refuse = [&]() -> std::optional<Prepared>
            {
                // PageShown resets preset confirmation. If preparation fails
                // after that logical change, require deferred native return,
                // rather than keep an old view with a mismatched confirmation.
                if (entering && token == revision) Cancel();
                return {};
            };
            Prepared result{token, location, entering, {}, {}};
            if (!location.page)
            {
                result.title = model.Text("menu.title");
                const auto& pages = model.Pages();
                if (location.first > pages.size()) return refuse();
                const auto end = (std::min)(pages.size(), location.first + pageRows);
                for (size_t i = location.first; i < end; ++i)
                    result.items.push_back({ItemKind::Link, i, {}, pages[i].title, {}, true});
                // Preserve numbered page suffixes if a long sibling name is
                // shortened. This is generated UI identity, never setting value.
                for (auto& item : result.items)
                {
                    const auto at = item.label.rfind(" (");
                    if (at != std::string::npos && item.label.ends_with(')')) item.requiredTail = item.label.substr(at);
                }
                if (location.first)
                    result.items.push_back({ItemKind::Previous, location.first > pageRows ? location.first - pageRows : 0,
                        {}, model.Text("menu.previous"), {}, true});
                if (end < pages.size())
                    result.items.push_back({ItemKind::Next, end, {}, model.Text("menu.next"), {}, true});
            }
            else
            {
                const auto page = *location.page;
                if (page >= model.Pages().size() || location.first) return refuse();
                result.title = model.Pages()[page].title;
                if (location.page != shown.page) model.PageShown(page);
                std::vector<const RWGTAMenu::Row*> visible;
                { ForeignScope scope(*this); visible = model.VisibleRows(page); }
                if (!Alive(token, current)) return refuse();
                // Copy every row before the first subsequent plugin call. A
                // read can close/reopen the model and invalidate all page rows.
                std::vector<RWGTAMenu::Row> rows;
                rows.reserve(visible.size());
                for (auto row : visible) rows.push_back(*row);
                if (rows.size() > pageRows) return refuse();
                for (const auto& row : rows)
                {
                    Item item{ItemKind::ModelRow, 0, row, {}, {}, false};
                    item.label = model.Label(row);
                    item.description = model.Help(row);
                    bool enabled{};
                    { ForeignScope scope(*this); enabled = model.Enabled(row, pause); }
                    if (!Alive(token, current)) return refuse();
                    item.enabled = enabled;
                    { ForeignScope scope(*this); item.value = enabled ? model.Value(row) : model.Text("menu.unavailable"); }
                    if (!Alive(token, current)) return refuse();
                    result.items.push_back(std::move(item));
                }
            }
            result.items.push_back({ItemKind::Back, 0, {}, model.Text("menu.back"), {}, true});
            if (!Alive(token, current)) return refuse();
            return result;
        }
        // Host calls Reject after fitting/publication refusal, before deferred
        // native Back. The old widgets then cannot dispatch stale Model actions.
        void Reject(const Prepared& candidate)
        {
            if (candidate.revision == revision && candidate.resetsPageState) Cancel();
        }
        // Called only after native geometry/text/readback has succeeded. A failed
        // root preparation preserves navigation; failed page-entry reset
        // cancels logically and requires the host's safe deferred native return.
        template<class Current> bool Commit(Prepared& candidate, Current current)
        {
            if (!Alive(candidate.revision, current)) return false;
            shown = candidate.location;
            candidate.revision = ++revision; // former physical pool actions are stale
            return true;
        }
        template<class Current> bool Change(const Prepared& displayed, size_t index, int direction, Current current)
        {
            if (!Alive(displayed.revision, current) || displayed.location != shown ||
                index >= displayed.items.size()) return false;
            const auto token = revision;
            const auto item = displayed.items[index];
            if (item.kind != ItemKind::ModelRow || !item.enabled || direction < -1 || direction > 1) return false;
            if (direction && item.row.kind != RWGTAMenu::RowKind::Setting && item.row.kind != RWGTAMenu::RowKind::Preset)
                return false;
            bool enabled{};
            { ForeignScope scope(*this); enabled = model.Enabled(item.row, pause); }
            if (!Alive(token, current) || !enabled) return false;
            bool changed{};
            { ForeignScope scope(*this); changed = model.Change(item.row, direction, pause); }
            return Alive(token, current) && changed;
        }
    };
}
