module;

#include <stdafx.h>
#include "NativeMenuText.hpp"
#include "Shared/RWGTA/FixMenu.hpp"

export module NativeMenuView;

export namespace GodfatherMenu
{
    // The shown list of the Plugins menu: the plugins list, a plugin's page or a category, with Back as its last row.
    // Every list scrolls through the same row slots; the update notice shows as lines of text instead.
    struct View
    {
        enum class Kind { Row, Back, Text };
        struct Item
        {
            Kind kind = Kind::Text;
            const RWGTAMenu::Row* row{};
            bool enabled = false;
            std::wstring text;
        };

        size_t page = RWGTAMenu::Model::RootPage;
        std::string notice;
        std::vector<Item> items;

        // The rows of the page's window; false when a text can't be shown
        bool Build(RWGTAMenu::Model& model, bool paused)
        {
            if (!model.IsOpen() || page >= model.Pages().size())
                return false;
            std::vector<Item> shown;
            if (!notice.empty())
            {
                std::vector<std::wstring> lines;
                if (!MenuText::Lines(notice, lines))
                    return false;
                for (auto& line : lines)
                    shown.push_back(Item{ Kind::Text, nullptr, false, std::move(line) });
                std::wstring back;
                if (!MenuText::Convert(model.Text("menu.back"), back))
                    return false;
                shown.push_back(Item{ Kind::Back, nullptr, true, std::move(back) });
            }
            else
            {
                for (auto row : model.Shown(page, true))
                {
                    std::wstring text;
                    if (!row)
                    {
                        if (!MenuText::Convert(model.Text("menu.back"), text))
                            return false;
                        shown.push_back(Item{ Kind::Back, nullptr, true, std::move(text) });
                        continue;
                    }
                    auto label = model.Label(*row);
                    auto value = model.Value(*row);
                    if (!value.empty())
                        label += ": " + value;
                    if (!MenuText::Convert(label, text))
                        return false;
                    MenuText::Shorten(text);
                    shown.push_back(Item{ Kind::Row, row, model.Enabled(*row, paused), std::move(text) });
                }
            }
            items = std::move(shown);
            return !items.empty();
        }
    };
}
