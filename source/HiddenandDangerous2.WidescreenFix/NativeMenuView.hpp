#pragma once
#include "../Shared/RWGTA/FixMenu.hpp"
#include <functional>
#include <string>
#include <vector>

namespace HD2Menu
{
    // A value is never shortened to fit a native row. The host supplies native
    // font conversion/measurement and commits this view together with widgets.
    struct View
    {
        enum class Kind { Page, Row, Previous, Next, Back };
        struct Item
        {
            Kind kind = Kind::Back;
            size_t index = 0;
            size_t continuation = 0;
            RWGTAMenu::Row row{};
            bool enabled = true;
            std::string text;
        };

        static constexpr size_t ContentRows = 2;
        size_t page = static_cast<size_t>(-1);
        size_t physical = 0;
        size_t physicalCount = 1;
        std::vector<Item> items;

        using Current = std::function<bool()>;
        using Convert = std::function<bool(const std::string&, std::string&)>;
        using Fits = std::function<bool(const std::string&)>;

        bool Prepare(RWGTAMenu::Model& model, bool pause, const Current& current,
            const Convert& convert, const Fits& fits)
        {
            if (!current() || !model.IsOpen())
                return false;
            std::vector<Item> complete;
            auto add = [&](Item item, const std::string& text)
            {
                std::string encoded;
                if (!convert(text, encoded) || !current())
                    return false;
                std::vector<std::string> lines;
                if (!Lines(encoded, current, fits, lines))
                    return false;
                for (size_t i = 0; i < lines.size(); ++i)
                {
                    auto line = item;
                    line.continuation = i;
                    line.text = std::move(lines[i]);
                    complete.push_back(std::move(line));
                }
                return true;
            };
            if (page == static_cast<size_t>(-1))
            {
                auto pages = model.Pages();
                for (size_t i = 0; i < pages.size(); ++i)
                    if (!add(Item{ Kind::Page, i }, pages[i].title))
                        return false;
            }
            else
            {
                if (page >= model.Pages().size())
                    return false;
                auto rows = model.VisibleRows(page);
                if (!current())
                    return false;
                for (size_t i = 0; i < rows.size(); ++i)
                {
                    if (!rows[i]) return false;
                    RWGTAMenu::Row row = *rows[i];
                    auto label = model.Label(row);
                    if (!current()) return false;
                    if (row.kind == RWGTAMenu::RowKind::ApplyPreset
                        || (row.kind == RWGTAMenu::RowKind::Preset && row.plugin))
                    {
                        auto scope = row.plugin ? OtherPlugins::Name(*row.plugin) : model.Text("menu.gamepreset");
                        if (!current()) return false;
                        label = scope + ": " + label;
                    }
                    auto value = model.Value(row);
                    if (!current()) return false;
                    bool enabled = model.Enabled(row, pause);
                    if (!current()) return false;
                    if (!value.empty()) label += ": " + value;
                    if (!add(Item{ Kind::Row, i, 0, row, enabled }, label))
                        return false;
                }
            }
            // Stage all foreign reads and native measurements before publishing
            // indices. A failed widget build must discard this entire view.
            if (!current() || complete.size() > 768)
                return false;
            physicalCount = (std::max)(size_t(1), (complete.size() + ContentRows - 1) / ContentRows);
            physical = (std::min)(physical, physicalCount - 1);
            std::vector<Item> shown;
            auto start = physical * ContentRows;
            auto end = (std::min)(start + ContentRows, complete.size());
            for (size_t i = start; i < end; ++i)
                shown.push_back(std::move(complete[i]));
            auto navigation = [&](Kind kind, const char* key)
            {
                std::string encoded;
                auto text = model.Text(key);
                if (!current() || !convert(text, encoded) || !current()) return false;
                bool fitsRow = fits(encoded);
                if (!current() || !fitsRow) return false;
                shown.push_back(Item{ kind, 0, 0, {}, true, std::move(encoded) });
                return true;
            };
            if (physical && !navigation(Kind::Previous, "menu.previous")) return false;
            if (physical + 1 < physicalCount && !navigation(Kind::Next, "menu.next")) return false;
            if (!navigation(Kind::Back, "menu.back") || !current()) return false;
            items = std::move(shown);
            return true;
        }

    private:
        static bool Lines(const std::string& text, const Current& current,
            const Fits& fits, std::vector<std::string>& result)
        {
            // Conversion supplies single-byte native text. Do not split a
            // multibyte codec here; unsupported encodings are refused upstream.
            if (text.size() > 8192)
                return false;
            size_t position = 0;
            while (position < text.size())
            {
                size_t accepted = 0, word = 0;
                auto maximum = (std::min)(size_t(512), text.size() - position);
                for (size_t length = 1; length <= maximum; ++length)
                {
                    bool fitsRow = fits(text.substr(position, length));
                    if (!current()) return false;
                    if (!fitsRow) break;
                    accepted = length;
                    if (text[position + length - 1] == ' ') word = length;
                }
                if (!accepted) return false;
                if (accepted < text.size() - position && word) accepted = word;
                result.push_back(text.substr(position, accepted));
                position += accepted;
                if (result.size() > 256) return false;
            }
            if (result.empty()) result.emplace_back();
            return true;
        }
    };
}
