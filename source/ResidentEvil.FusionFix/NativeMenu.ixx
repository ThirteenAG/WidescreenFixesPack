module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/RWGTA/FixMenu.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"
#include "Shared/MenuList.hpp"
#include "Shared/RainDroplets.hpp"
#include <unordered_map>
#include <cmath>
#include <span>

export module NativeMenu;

import EngineText;
import MenuBindings;
import OptionWord;
import Window;

export namespace REMenu
{
    inline HMODULE module = nullptr;
    inline std::function<bool()> blocksInput;
    inline SafetyHookInline initHook, execHook, inputHook, destroyHook, releaseHook;
    inline SafetyHookMid firstCountHook, secondCountHook;
    inline std::map<std::pair<uintptr_t, std::string>, std::string> restartValues;

    inline void RememberRestartValues(const RWGTAMenu::Model& model)
    {
        for (const auto& page : model.Pages())
        {
            if (!page.plugin || page.plugin->module == module) continue;
            for (const auto& setting : page.plugin->file.description.settings)
                if (setting.HasValue() && setting.apply == "restart")
                    restartValues.try_emplace({ reinterpret_cast<uintptr_t>(page.plugin->module), setting.id },
                        OtherPlugins::Get(*page.plugin, setting));
        }
    }

    inline bool RestartRequired(const RWGTAMenu::Model& model)
    {
        for (const auto& [id, value] : AppliedRestartSettings())
            if (FixSettings().Get(id) != value) return true;

        for (const auto& page : model.Pages())
        {
            if (!page.plugin || page.plugin->module == module) continue;
            for (const auto& setting : page.plugin->file.description.settings)
            {
                if (!setting.HasValue() || setting.apply != "restart") continue;
                const auto initial = restartValues.find({ reinterpret_cast<uintptr_t>(page.plugin->module), setting.id });
                if (initial != restartValues.end() && initial->second != OtherPlugins::Get(*page.plugin, setting)) return true;
            }
        }
        return false;
    }

    // A line of the page: a row of the model, or Back (none)
    struct Item
    {
        std::string label, value;
        const RWGTAMenu::Row* row = nullptr;
        bool enabled = true;
    };

    // Most lines shown at once: every list (plugins, categories, settings) scrolls through them, Back among its rows.
    // With the page title above them they take the places of the game's Option list rows and the Plugins entry: eight
    // lines in the title's Option menu, six in the game's.
    constexpr size_t Slots = 8;

    // The look of the game's Option list: the text centre of a row below the top of its label picture, the text size,
    // the colours of a label and of the selected one (which pulses between two brightnesses) and the halo around them
    namespace Look
    {
        // Text node colours are stored as A B G R
        constexpr uint32_t Rgb(uint32_t r, uint32_t g, uint32_t b, uint32_t a = 255)
        {
            return (a << 24) | (b << 16) | (g << 8) | r;
        }
        constexpr float RowCentre = 20.5f;
        constexpr int Size = 28;
        constexpr int TitleSize = 28;
        constexpr int HelpSize = 22;
        constexpr uint32_t Label = Rgb(86, 92, 110);
        constexpr uint32_t LabelHalo = Rgb(120, 128, 190, 36);
        constexpr uint32_t Dim = Rgb(52, 53, 63);
        constexpr uint32_t Selected = Rgb(194, 200, 216);
        constexpr uint32_t SelectedDim = Rgb(130, 131, 166);
        constexpr uint32_t SelectedHalo = Rgb(80, 70, 220, 112);
        constexpr uint32_t SelectedHaloDim = Rgb(80, 70, 220, 64);
        constexpr uint32_t Title = Rgb(154, 154, 154);
        constexpr uint32_t Help = Rgb(255, 255, 255);
        // Name and value lines use the size and columns of the game's Video settings page: names end left of the
        // centre, values are centred right of it
        constexpr int RowSize = 20;
        constexpr float LabelRight = 559.0f;
        constexpr float LabelWidth = 320.0f;
        constexpr float ValueCentre = 765.0f;
        constexpr float ValueWidth = 250.0f;
        constexpr float Centre = 640.0f;
        constexpr float Arrows = 1000.0f;
        constexpr float HelpLine = 585.0f;
    }

    enum class Align { Left, Centre, Right };

    // A line of text drawn by the game's text nodes, with an optional halo of eight shifted copies under it
    struct Text
    {
        void* node = nullptr;
        std::vector<void*> halo;
        std::string value;
        std::array<float, 5> geometry{};
        uint32_t tint = 0, glow = 0;
        float width = 0;
        bool fitted = false, visible = false;

        void Set(const std::string& text)
        {
            if (text == value) return;
            fitted = false;
            value = text;
            REText::Set(node, value);
            for (auto copy : halo) REText::Set(copy, value);
        }

        // x is the left edge, the centre or the right edge of the text by the alignment; y is the centre of the line
        void Fit(const std::string& text, float x, float y, int height, float limit, uint32_t color,
            Align align = Align::Left, uint32_t haloColor = 0)
        {
            const std::array<float, 5> style{ x, y, static_cast<float>(height), limit, static_cast<float>(align) };
            if (fitted && text == value && geometry == style && tint == color && glow == haloColor) return;
            REText::Place(node, x, y - height * 0.5f, height, color);
            REText::Set(node, text);
            auto size = height;
            auto measured = Read<float>(node, 216);
            if (std::isfinite(measured) && measured > limit)
            {
                size = std::max(12, static_cast<int>(std::floor(height * limit / measured)));
                REText::Place(node, x, y - size * 0.5f, size, color);
                REText::Set(node, text);
                measured = Read<float>(node, 216);
            }
            if (!std::isfinite(measured)) measured = 0;
            const float left = align == Align::Centre ? x - measured * 0.5f : align == Align::Right ? x - measured : x;
            REText::Place(node, left, y - size * 0.5f, size, color);
            const float radius = size / 18.0f;
            for (size_t i = 0; i < halo.size(); ++i)
            {
                const float angle = 0.785398f * i;
                REText::Place(halo[i], left + radius * std::cos(angle), y - size * 0.5f + radius * std::sin(angle), size, haloColor);
                REText::Set(halo[i], text);
            }
            value = text;
            geometry = style;
            tint = color;
            glow = haloColor;
            width = measured;
            fitted = true;
            Show(visible);
        }

        // A new colour on the same text and place (the pulse of the selected line)
        void Tint(uint32_t color, uint32_t haloColor)
        {
            if (!fitted || (color == tint && haloColor == glow)) return;
            const auto paint = [](void* text, uint32_t c)
            {
                Write(text, 228, c);
                Write(text, 280, Read<uint32_t>(text, 280) | 0x10000);
                Write(text, 84, Read<uint32_t>(text, 84) | 0x10000);
            };
            paint(node, color);
            for (auto copy : halo) paint(copy, haloColor);
            tint = color;
            glow = haloColor;
        }

        void Show(bool shown)
        {
            visible = shown;
            REText::Show(node, shown);
            for (auto copy : halo) REText::Show(copy, shown && (glow >> 24) != 0);
        }

        // Closing the menu clears the update flag of every child; reopening restores it only on the native items
        void Activate()
        {
            for (auto text : halo)
                if (!(Read<uint32_t>(text, 84) & 8u)) Write(text, 84, Read<uint32_t>(text, 84) | 8u);
            if (!(Read<uint32_t>(node, 84) & 8u)) Write(node, 84, Read<uint32_t>(node, 84) | 8u);
        }

        bool Ready() const
        {
            return node && std::none_of(halo.begin(), halo.end(), [](void* t) { return !t; });
        }
    };

    // The brightness of the selected line, rising and falling like the game's own selection
    inline float Pulse()
    {
        const auto t = static_cast<float>(GetTickCount64() % 1200) / 1200.0f;
        return 0.5f + 0.5f * std::cos(t * 6.2831853f);
    }

    inline uint32_t Mix(uint32_t a, uint32_t b, float t)
    {
        uint32_t out = 0;
        for (int shift = 0; shift < 32; shift += 8)
        {
            const float x = static_cast<float>((a >> shift) & 255), y = static_cast<float>((b >> shift) & 255);
            out |= static_cast<uint32_t>(std::lround(x + (y - x) * t)) << shift;
        }
        return out;
    }

    struct Host
    {
        void* gui = nullptr;
        void* root = nullptr;
        std::array<void*, 9> actors{};
        std::array<uint32_t, 9> actorFlags{};
        std::array<float, 9> rowY{};
        std::array<float, 2> entryPosition{};
        void* guideRoot = nullptr;
        std::vector<std::pair<void*, bool>> guideVisibility;
        std::string entryHelp, language;
        std::array<Text, Slots> labels, values;
        std::array<Text, 9> helpLines;
        Text entry, heading, footer, restart, above, below, left, right;
        RWGTAMenu::Model model;
        std::vector<Item> items;
        std::vector<std::string> help;
        size_t page = 0, helpStart = 0;
        MenuList::WheelNotches wheel;
        MenuList::Repeat pageUp, pageDown;
        bool homeHeld = false, endHeld = false;
        uint32_t entryIndex = 0, entryActor = 0;
        bool open = false, inHelp = false, busy = false, hidden = false;
        // The Plugins entry as a picture like the game's own entries: its selected and normal texture objects, their
        // original rectangles, and the request for the word in the entry's texture
        std::array<void*, 2> pictures{};
        std::array<std::array<float, 4>, 2> pictureRects{};
        std::shared_ptr<REWord::Request> word;
        bool wordShown = false;
    };
    inline std::unordered_map<void*, std::shared_ptr<Host>> hosts;
    inline std::weak_ptr<Host> noticeHost;

    // The selected and the normal picture of an Option list entry: the texture objects under its first object
    inline bool EntryPictures(void* actor, std::array<void*, 2>& pictures)
    {
        pictures = {};
        auto group = Read<void*>(actor, 0xEC);
        if (!group) return false;
        for (auto child = Read<void*>(group, 0x68); child; child = Read<void*>(child, 0x6C))
        {
            const auto left = Read<float>(child, 236), top = Read<float>(child, 240);
            const auto right = Read<float>(child, 244), bottom = Read<float>(child, 248);
            if (right - left != REWord::Rectangles[0][2] - REWord::Rectangles[0][0] ||
                bottom - top != REWord::Rectangles[0][3] - REWord::Rectangles[0][1]) continue;
            if (left == REWord::Rectangles[0][0]) pictures[0] = child;
            else if (left == REWord::Rectangles[1][0]) pictures[1] = child;
        }
        return pictures[0] && pictures[1];
    }

    // A texture object's rectangle in its texture, marked changed like the game's own setter does
    inline void SetRectangle(void* picture, const std::array<float, 4>& rect)
    {
        for (size_t i = 0; i < 4; ++i) Write(picture, 236 + i * 4, rect[i]);
        auto update = (*reinterpret_cast<void(__thiscall***)(void*)>(picture))[17];
        update(picture);
    }

    // The word picture, once the render thread has written it; the text line stands in when the word can't be made.
    // The entry's selection animation sets the pictures' rectangles again, so they are kept after every update.
    inline bool WordPicture(const std::shared_ptr<Host>& host)
    {
        if (!host->word || host->word->state != REWord::State::Ready) return false;
        for (size_t i = 0; i < 2; ++i)
        {
            const auto& rect = REWord::Rectangles[i];
            for (size_t k = 0; k < 4; ++k)
                if (Read<float>(host->pictures[i], 236 + k * 4) != rect[k])
                {
                    SetRectangle(host->pictures[i], rect);
                    break;
                }
        }
        host->wordShown = true;
        return true;
    }

    inline bool WordFailed(const std::shared_ptr<Host>& host)
    {
        return !host->word || host->word->state == REWord::State::Failed;
    }

    inline void RestorePictures(const std::shared_ptr<Host>& host)
    {
        if (!host->wordShown) return;
        for (size_t i = 0; i < 2; ++i) SetRectangle(host->pictures[i], host->pictureRects[i]);
        host->wordShown = false;
    }

    inline bool Current(const std::shared_ptr<Host>& host)
    {
        return host && hosts.count(host->gui) && hosts.at(host->gui) == host &&
            Read<void*>(host->gui, 244) == host->root;
    }

    inline std::string Language()
    {
        auto language = FixSettings().Get("MAIN.TextLanguage");
        if (!language.empty() && language != "auto")
            return language;
        // The language chosen in the game's options (Japanese, English, French, Spanish, German, Italian, ...)
        const auto system = gameLanguage.get();
        switch (system ? Read<uint32_t>(system, 0x28) : 1)
        {
            case 2: return "fr";
            case 3: return "es";
            case 4: return "de";
            case 5: return "it";
            default: return "en";
        }
    }

    // The Plugins row and its help follow the game's language, also when it changes while the menu exists
    inline void EntryText(const std::shared_ptr<Host>& host)
    {
        auto language = Language();
        if (language == host->language) return;
        host->language = std::move(language);
        host->model.OpenMenu(module, host->language, host->entryIndex);
        RememberRestartValues(host->model);
        host->entry.fitted = false;
        host->entry.Fit(host->model.Text("menu.plugins"), Look::Centre, host->rowY[host->entryIndex] + Look::RowCentre,
            Look::Size, 500, Look::Label, Align::Centre, Look::LabelHalo);
        if (host->pictures[0])
        {
            host->word = REWord::Submit(REWord::TextureOf(host->pictures[0]), host->model.Text("menu.plugins"));
            host->wordShown = false;
        }
        host->entryHelp = host->model.Text("menu.pluginshelp");
        host->footer.fitted = false;
    }

    // The centre of the line in place i of the list (0 is the page title, then the rows)
    inline float Line(const std::shared_ptr<Host>& host, size_t i)
    {
        const float pitch = (host->rowY[host->entryIndex] - host->rowY[0]) / host->entryIndex;
        return host->rowY[0] + pitch * i + Look::RowCentre;
    }

    // The selected line pulses like the game's own selection
    inline void Animate(const std::shared_ptr<Host>& host)
    {
        const auto t = Pulse();
        const auto color = Mix(Look::SelectedDim, Look::Selected, t);
        const auto halo = Mix(Look::SelectedHaloDim, Look::SelectedHalo, t);
        if (!host->open)
        {
            if (WordPicture(host)) return;
            const bool selected = Read<uint32_t>(host->gui, 844) == host->entryIndex;
            host->entry.Tint(selected ? color : Look::Label, selected ? halo : Look::LabelHalo);
            return;
        }
        if (host->inHelp) return;
        const auto slot = host->model.Window(host->page).Slot();
        if (slot >= host->items.size() || !host->items[slot].enabled) return;
        host->labels[slot].Tint(color, halo);
        host->values[slot].Tint(color, halo);
        host->left.Tint(color, 0);
        host->right.Tint(color, 0);
    }

    inline void Guide(const std::shared_ptr<Host>& host, bool hidden)
    {
        auto manager = sceneManager.get();
        auto guide = manager ? Read<void*>(manager, 208) : nullptr;
        auto root = guide ? Read<void*>(guide, 244) : nullptr;
        if (root != host->guideRoot)
        {
            host->guideVisibility.clear();
            host->guideRoot = root;
        }
        if (!root) return;
        if (hidden)
        {
            if (host->guideVisibility.empty())
                for (auto child = Read<void*>(root, 96); child; child = Read<void*>(child, 100))
                    host->guideVisibility.emplace_back(child, (Read<uint32_t>(child, 84) & 1) != 0);
            for (const auto& [node, visible] : host->guideVisibility) REText::Show(node, false);
        }
        else
        {
            for (const auto& [node, visible] : host->guideVisibility) REText::Show(node, visible);
            host->guideVisibility.clear();
        }
    }

    inline void ShowPage(const std::shared_ptr<Host>& host)
    {
        if (!Current(host)) return;
        for (auto* text : { &host->entry, &host->heading, &host->footer, &host->restart, &host->above, &host->below,
            &host->left, &host->right }) text->Activate();
        for (auto& text : host->labels) text.Activate();
        for (auto& text : host->values) text.Activate();
        for (auto& text : host->helpLines) text.Activate();
        if (host->open && !host->hidden)
        {
            for (size_t i = 0; i < host->actors.size(); ++i)
                host->actorFlags[i] = Read<uint32_t>(host->actors[i], 84);
            host->hidden = true;
        }
        if (host->open)
        {
            for (auto actor : host->actors) REText::Show(actor, false);
        }
        else if (host->hidden)
        {
            for (size_t i = 0; i < host->actors.size(); ++i)
                REText::Show(host->actors[i], (host->actorFlags[i] & 1) != 0);
            host->hidden = false;
        }
        if (!host->open)
        {
            auto actor = host->actors[host->entryActor];
            // Visibility and mouse activity are separate native flags. The entry keeps its hit region; its picture shows
            // once the word is in the texture.
            const auto flags = Read<uint32_t>(actor, 84) | 8u;
            Write(actor, 84, WordPicture(host) ? flags | 1u : flags & ~1u);
            for (size_t i = 0; i <= host->entryIndex; ++i)
            {
                const auto logical = Read<uint32_t>(host->gui, 804 + i * 4);
                if (logical >= 9) continue;
                if (Read<float>(host->actors[logical], 164) != host->rowY[i])
                {
                    Write(host->actors[logical], 164, host->rowY[i]);
                    Write(host->actors[logical], 84, Read<uint32_t>(host->actors[logical], 84) | 0x10000);
                }
            }
        }
        if (!host->open && !host->busy) EntryText(host);
        const bool entrySelected = !host->open && Read<uint32_t>(host->gui, 844) == host->entryIndex;
        Guide(host, host->open || entrySelected);
        if (!host->open)
            host->footer.Fit(entrySelected ? host->entryHelp : "", Look::Centre, Look::HelpLine, Look::HelpSize, 760, Look::Help, Align::Centre);
        host->entry.Show(!host->open && WordFailed(host));
        host->heading.Show(host->open);
        host->footer.Show(host->open || entrySelected);
        const bool restart = host->open && RestartRequired(host->model);
        if (restart)
            host->restart.Fit(host->model.Text("menu.restartwarning"), Look::Centre, Look::HelpLine + 30, Look::HelpSize, 760, Look::Help, Align::Centre);
        host->restart.Show(restart);
        const bool list = host->open && !host->inHelp;
        for (size_t i = 0; i < Slots; ++i)
        {
            const bool shown = list && i < host->items.size();
            host->labels[i].Show(shown);
            host->values[i].Show(shown && !host->values[i].value.empty());
        }
        const auto slot = host->model.Window(host->page).Slot();
        const bool arrows = list && slot < host->items.size() && host->items[slot].row && host->items[slot].enabled &&
            !RWGTAMenu::Model::Opens(*host->items[slot].row) && !host->items[slot].value.empty();
        host->left.Show(arrows);
        host->right.Show(arrows);
        auto& window = host->model.Window(host->page);
        host->above.Show(list && window.Above());
        host->below.Show(list && window.Below());
        for (size_t i = 0; i < host->helpLines.size(); ++i)
            host->helpLines[i].Show(host->open && host->inHelp && host->helpStart + i < host->help.size());
    }

    // A text node of the menu; one with a halo gets its eight copies first, so they draw under it
    inline Text MakeText(const std::shared_ptr<Host>& host, int size, bool halo = false)
    {
        Text text;
        if (halo)
            for (int i = 0; i < 8; ++i)
            {
                text.halo.push_back(REText::Append(host->gui, host->root, 1000));
                REText::Place(text.halo.back(), 0, 0, size);
                REText::Show(text.halo.back(), false);
            }
        text.node = REText::Append(host->gui, host->root, 1000);
        REText::Place(text.node, 0, 0, size);
        REText::Show(text.node, false);
        return text;
    }

    inline void Populate(const std::shared_ptr<Host>& host)
    {
        if (!Current(host) || !host->open) return;
        auto& model = host->model;
        if (host->page >= model.Pages().size()) host->page = RWGTAMenu::Model::RootPage;
        std::vector<Item> items;
        const std::string title = model.Pages()[host->page].title;
        for (auto row : model.Shown(host->page, true))
        {
            if (!row)
            {
                items.push_back({ model.Text("menu.back"), "", nullptr });
                continue;
            }
            auto label = model.Label(*row);
            auto value = model.Value(*row);
            auto enabled = model.Enabled(*row, Read<uint32_t>(host->gui, 764) == 0);
            if (!Current(host)) return;
            items.push_back({ std::move(label), std::move(value), row, enabled });
        }
        host->items = std::move(items);
        const size_t selection = model.Window(host->page).Slot();
        host->heading.Fit(title, Look::Centre, Line(host, 0), Look::TitleSize, 760, Look::Title, Align::Centre);
        for (size_t i = 0; i < host->items.size(); ++i)
        {
            const auto& item = host->items[i];
            const bool selected = i == selection && item.enabled;
            const auto color = selected ? host->labels[i].tint : item.enabled ? Look::Label : Look::Dim;
            const auto halo = selected ? host->labels[i].glow : item.enabled ? Look::LabelHalo : 0;
            const auto keep = selected && (color == Look::Label || color == Look::Dim);
            const auto tint = keep ? Look::Selected : color;
            const auto glow = keep ? Look::SelectedHalo : halo;
            const auto value = item.enabled ? item.value : model.Text("menu.unavailable");
            const float y = Line(host, i + 1);
            // Lists of entries are centred like the game's Option list; a setting has its name left of the centre
            // and its value right of it, like the game's own settings pages
            if (value.empty())
                host->labels[i].Fit(item.label, Look::Centre, y, Look::Size, 760, tint, Align::Centre, glow);
            else
                host->labels[i].Fit(item.label, Look::LabelRight, y, Look::RowSize, Look::LabelWidth, tint, Align::Right, glow);
            host->values[i].Fit(value, Look::ValueCentre, y, Look::RowSize, Look::ValueWidth, tint, Align::Centre, value.empty() ? 0 : glow);
        }
        if (selection < host->items.size())
        {
            const auto& value = host->values[selection];
            const float y = Line(host, selection + 1);
            host->left.Fit("\xE3\x80\x88", Look::ValueCentre - value.width * 0.5f - 16, y, 18, 40, value.tint, Align::Right);
            host->right.Fit("\xE3\x80\x89", Look::ValueCentre + value.width * 0.5f + 16, y, 18, 40, value.tint, Align::Left);
        }
        // Rows above or below the window: small arrows right of the list, in its colour
        host->above.Fit("\xE2\x96\xB2", Look::Arrows, Line(host, 1), 16, 40, Look::Label, Align::Centre);
        host->below.Fit("\xE2\x96\xBC", Look::Arrows, Line(host, host->entryIndex), 16, 40, Look::Label, Align::Centre);
        const auto* selected = selection < host->items.size() ? host->items[selection].row : nullptr;
        const auto icons = FixSettings().Get("MAIN.GamepadIcons");
        const auto helpKey = icons == "2" ? model.Text("menu.pad.triangle") : icons == "3" ? "X" : "Y";
        host->footer.Fit(selected && !RWGTAMenu::Model::Opens(*selected) && !model.Help(*selected).empty() ?
            "F1 / " + helpKey + ": " + model.Text("menu.help") : "", Look::Centre, Look::HelpLine, Look::HelpSize, 760, Look::Help, Align::Centre);
        ShowPage(host);
    }

    inline void Close(const std::shared_ptr<Host>& host)
    {
        host->open = host->inHelp = false;
        host->model.Close();
        ShowPage(host);
    }

    // Back: the help page closes, a page returns to the list it was opened from (that row selected), the plugins list
    // closes the menu
    inline void Back(const std::shared_ptr<Host>& host)
    {
        if (host->inHelp) { host->inHelp = false; Populate(host); return; }
        const auto parent = host->model.Back(host->page);
        if (parent == SIZE_MAX) { Close(host); return; }
        host->page = parent;
        Populate(host);
    }

    // The selected line: left (-1), right (+1) or accept (0)
    inline void Change(const std::shared_ptr<Host>& host, int direction)
    {
        if (!Current(host)) return;
        const auto slot = host->model.Window(host->page).Slot();
        if (slot >= host->items.size()) return;
        const auto item = host->items[slot];
        if (!item.row)
        {
            if (!direction) Back(host);
            return;
        }
        if (RWGTAMenu::Model::Opens(*item.row))
        {
            const auto page = direction ? SIZE_MAX : host->model.Enter(*item.row);
            if (page != SIZE_MAX) host->page = page;
        }
        else
            host->model.Change(*item.row, direction, Read<uint32_t>(host->gui, 764) == 0);
        Populate(host);
    }

    inline const RWGTAMenu::Row* SelectedRow(const std::shared_ptr<Host>& host)
    {
        const auto slot = host->model.Window(host->page).Slot();
        return slot < host->items.size() ? host->items[slot].row : nullptr;
    }

    // The lines of the help page from helpStart, centred like the game's help line
    inline void HelpLines(const std::shared_ptr<Host>& host)
    {
        for (size_t i = 0; i < host->helpLines.size(); ++i)
            host->helpLines[i].Fit(host->helpStart + i < host->help.size() ? host->help[host->helpStart + i] : "",
                Look::Centre, Line(host, 1) + 30.0f * i, Look::HelpSize, 760, Look::Help, Align::Centre);
    }

    inline void Help(const std::shared_ptr<Host>& host, const std::string* message = nullptr, const std::string* title = nullptr)
    {
        const auto* row = SelectedRow(host);
        if (!message && (!row || RWGTAMenu::Model::Opens(*row))) return;
        auto content = message ? *message : host->model.Help(*row);
        if (content.empty() || !Current(host)) return;
        host->help.clear();
        std::string line;
        REText::Place(host->helpLines[0].node, 0, 0, Look::HelpSize);
        // Measure complete UTF-8 words with the same native font used to draw the help page.
        for (size_t first = 0; first < content.size();)
        {
            auto last = content.find_first_of(" \r\n", first);
            if (last == std::string::npos) last = content.size();
            auto word = content.substr(first, last - first);
            auto joined = line.empty() ? word : line + " " + word;
            REText::Set(host->helpLines[0].node, joined);
            if (!line.empty() && Read<float>(host->helpLines[0].node, 216) > 760)
            {
                host->help.push_back(line);
                line = std::move(word);
            }
            else line = std::move(joined);
            if (last < content.size() && content[last] == '\n') { host->help.push_back(line); line.clear(); }
            first = last + 1;
        }
        if (!line.empty()) host->help.push_back(line);
        host->helpStart = 0;
        host->inHelp = true;
        host->heading.Fit(title ? *title : host->model.Label(*row), Look::Centre, Line(host, 0), Look::TitleSize, 760,
            Look::Title, Align::Centre);
        // Measuring used the first node directly, so its cached text is stale
        host->helpLines[0].fitted = false;
        host->helpLines[0].value.clear();
        HelpLines(host);
        host->footer.Fit(host->model.Text("menu.back"), Look::Centre, Look::HelpLine, Look::HelpSize, 760, Look::Help, Align::Centre);
        ShowPage(host);
    }

    inline bool NoticeReady()
    {
        const auto host = noticeHost.lock();
        return Current(host) && !host->inHelp && !host->busy && (!blocksInput || !blocksInput());
    }

    inline void ShowNotice(const std::wstring& message)
    {
        const auto host = noticeHost.lock();
        if (!Current(host)) return;
        if (!host->open)
        {
            host->open = true;
            host->page = RWGTAMenu::Model::RootPage;
            host->model.Window(host->page).Reset();
            Populate(host);
        }
        const auto content = asi_settings::text::Narrow(message);
        const auto title = host->model.Text("menu.plugins");
        Help(host, &content, &title);
    }

    inline bool PrepareHost(void* gui)
    {
        if (hosts.count(gui)) return Current(hosts.at(gui));
        auto root = Read<void*>(gui, 244);
        auto nodes = Read<void**>(gui, 248);
        auto manager = inputManager.get();
        if (!root || !nodes || !manager || !Read<void*>(manager, 52)) return false;
        auto host = std::make_shared<Host>();
        host->gui = gui;
        host->root = root;
        const bool extended = Read<uint32_t>(gui, 764) != 0;
        host->entryActor = extended ? 6 : 4;
        host->entryIndex = extended ? 8 : 6;
        for (size_t i = 0; i < 9; ++i)
        {
            if (Read<uint32_t>(gui, 768 + i * 4) != i + 2 || !nodes[i + 2]) return false;
            host->actors[i] = nodes[i + 2];
            host->actorFlags[i] = Read<uint32_t>(nodes[i + 2], 84);
        }
        auto previous = nodes[Read<uint32_t>(gui, 804 + (host->entryIndex - 1) * 4) + 2];
        auto first = nodes[Read<uint32_t>(gui, 804) + 2];
        const float top = Read<float>(first, 164);
        const float bottom = Read<float>(previous, 164);
        if (!std::isfinite(top) || !std::isfinite(bottom) || bottom <= top) return false;
        for (size_t i = 0; i <= host->entryIndex; ++i)
            host->rowY[i] = top + (bottom - top) * i / host->entryIndex;
        host->entryPosition = { Read<float>(previous, 160), bottom };
        auto actor = host->actors[host->entryActor];
        if (EntryPictures(actor, host->pictures))
            for (size_t i = 0; i < 2; ++i)
                for (size_t k = 0; k < 4; ++k) host->pictureRects[i][k] = Read<float>(host->pictures[i], 236 + k * 4);
        host->entry = MakeText(host, Look::Size, true);
        host->heading = MakeText(host, Look::TitleSize);
        host->footer = MakeText(host, Look::HelpSize);
        host->restart = MakeText(host, Look::HelpSize);
        for (size_t i = 0; i < Slots; ++i)
        {
            host->labels[i] = MakeText(host, Look::Size, true);
            host->values[i] = MakeText(host, Look::Size, true);
        }
        host->left = MakeText(host, 18);
        host->right = MakeText(host, 18);
        host->above = MakeText(host, 16);
        host->below = MakeText(host, 16);
        for (auto& line : host->helpLines) line = MakeText(host, Look::HelpSize);
        for (const auto* text : { &host->entry, &host->heading, &host->footer, &host->restart, &host->left, &host->right,
            &host->above, &host->below }) if (!text->Ready()) return false;
        for (const auto& group : { std::span<const Text>(host->labels), std::span<const Text>(host->values),
            std::span<const Text>(host->helpLines) })
            if (std::any_of(group.begin(), group.end(), [](const Text& t) { return !t.Ready(); })) return false;
        Write(actor, 160, host->entryPosition[0]);
        Write(actor, 164, host->entryPosition[1]);
        Write(actor, 84, Read<uint32_t>(actor, 84) | 0x10009);
        Write(gui, 804 + host->entryIndex * 4, host->entryActor);
        EntryText(host);
        hosts.emplace(gui, host);
        ShowPage(host);
        return true;
    }

    inline int __fastcall Initialize(void* gui, void*)
    {
        if (hosts.count(gui) && Current(hosts.at(gui)))
        {
            Guide(hosts.at(gui), false);
            RestorePictures(hosts.at(gui));
        }
        hosts.erase(gui);
        return initHook.thiscall<int>(gui);
    }

    inline void* __fastcall Destroy(void* gui, void*, uint32_t flags)
    {
        RainDroplets::MenuOpen(false);
        if (hosts.count(gui) && Current(hosts.at(gui))) Guide(hosts.at(gui), false);
        hosts.erase(gui);
        return destroyHook.thiscall<void*>(gui, flags);
    }

    inline int __fastcall Release(void* gui, void*)
    {
        for (const auto& [owner, host] : hosts)
        {
            if (host->guideRoot != Read<void*>(gui, 244)) continue;
            host->guideRoot = nullptr;
            host->guideVisibility.clear();
        }
        if (hosts.count(gui) && Current(hosts.at(gui))) Guide(hosts.at(gui), false);
        hosts.erase(gui);
        return releaseHook.thiscall<int>(gui);
    }

    inline int __fastcall Exec(void* gui, void*)
    {
        if (hosts.count(gui) && !Current(hosts.at(gui))) hosts.erase(gui);
        const auto result = execHook.thiscall<int>(gui);
        // The Option menu is on the screen from its fade in to its fade out
        RainDroplets::MenuOpen(Read<uint32_t>(gui, 840) != 0);
        if (Read<uint32_t>(gui, 840) == 3)
        {
            if (PrepareHost(gui))
            {
                const auto host = hosts.at(gui);
                noticeHost = host;
                RWGTA::UpdateCheck::Tick();
                noticeHost.reset();
                if (host->open && !host->inHelp && host->model.SelectLanguage(Language()))
                    Populate(host);
                else
                    ShowPage(host);
                if (Current(host)) Animate(host);
            }
        }
        return result;
    }

    inline int __fastcall Input(void* gui, void*)
    {
        auto found = hosts.find(gui);
        if (found == hosts.end() || !Current(found->second)) return inputHook.thiscall<int>(gui);
        const auto host = found->second;
        if (host->busy || (blocksInput && blocksInput())) return 0;
        DWORD foregroundProcess = 0;
        GetWindowThreadProcessId(GetForegroundWindow(), &foregroundProcess);
        if (foregroundProcess != GetCurrentProcessId()) return 0;
        auto manager = inputManager.get();
        auto controller = pad.get();
        if (!manager || !controller) return inputHook.thiscall<int>(gui);
        prepare(manager, 0);
        const auto buttons = pressed(manager, 0, false);
        const auto repeat = repeated(manager, 0);
        const bool click = !host->open && mouseSelection(gui);
        bool accept = (buttons & Read<uint32_t>(controller, 2924)) != 0 || click;
        auto keys = keyboard.get();
        bool escape = false;
        if (keys && Read<uint32_t>(gui, 764) == 0)
        {
            const auto key = Read<uint32_t>(keys, 2676);
            if (key < 256) escape = (Read<uint32_t>(keys, 2752 + (key >> 5) * 4) & (1u << (key & 31))) != 0;
        }
        const auto profile = Read<uint32_t>(controller, 3316);
        const auto profileOffset = 2976 + (profile < 5 ? profile * 68 : 0);
        const bool secondaryBack = !Read<uint8_t>(manager, 1057) &&
            (buttons & Read<uint32_t>(controller, profileOffset + 48)) != 0;
        const bool back = (buttons & Read<uint32_t>(controller, 2928)) != 0 || secondaryBack || escape;
        if (!host->open)
        {
            if (Read<uint32_t>(gui, 844) != host->entryIndex || !accept) return inputHook.thiscall<int>(gui);
            host->busy = true;
            host->language.clear();
            EntryText(host);
            RememberRestartValues(host->model);
            host->busy = false;
            if (!Current(host)) return 0;
            host->open = true;
            host->page = RWGTAMenu::Model::RootPage;
            host->model.Window(host->page).Reset();
            host->wheel.Clear();
            REWindow::wheelDelta = 0;
            Populate(host);
            return 0;
        }
        const bool help = (buttons & 0x1000u) != 0 ||
            (keys && (Read<uint32_t>(keys, 2752 + (VK_F1 >> 5) * 4) & (1u << (VK_F1 & 31))) != 0);
        POINT point{};
        bool mouseClick = false, mouseMoved = false;
        const bool haveMouse = MousePoint(gui, point, mouseClick, mouseMoved);
        const bool mouseBack = host->inHelp && haveMouse && mouseClick &&
            point.x >= 280 && point.x < 1000 && point.y >= Look::HelpLine - 14 && point.y < Look::HelpLine + 14;
        if (back || mouseBack || (host->inHelp && help)) { Back(host); return 0; }
        if (host->inHelp)
        {
            if ((repeat & 0x40040) && host->helpStart + host->helpLines.size() < host->help.size()) ++host->helpStart;
            if ((repeat & 0x10010) && host->helpStart) --host->helpStart;
            HelpLines(host);
            ShowPage(host);
            return 0;
        }
        auto& window = host->model.Window(host->page);
        // The mouse: the line under it while it moves (the window's slot plus its scroll); a click on an arrow scrolls
        const float pitch = Line(host, 2) - Line(host, 1);
        int step = 0;
        for (size_t i = 0; i < host->items.size(); ++i)
        {
            const float centre = Line(host, i + 1);
            const bool inside = haveMouse && point.x >= 280 && point.x < 975 &&
                point.y >= centre - pitch * 0.5f && point.y < centre + pitch * 0.5f;
            const bool clicked = inside && mouseClick;
            const bool moved = inside && mouseMoved;
            if (!clicked && !moved) continue;
            window.Point(i);
            // A click on the arrow left or right of the selected value changes it, like the game's settings pages
            const auto& value = host->values[i];
            const bool arrows = host->left.visible && window.Slot() == i;
            const float edge = value.width * 0.5f + 16;
            if (clicked && arrows && point.x < Look::ValueCentre - edge + 6 && point.x >= Look::ValueCentre - edge - 24) step = -1;
            else if (clicked && arrows && point.x >= Look::ValueCentre + edge - 6 && point.x < Look::ValueCentre + edge + 24) step = 1;
            else accept |= clicked;
            break;
        }
        // The scroll arrows right of the list
        const bool onArrows = haveMouse && mouseClick && point.x >= Look::Arrows - 25 && point.x < Look::Arrows + 25;
        if (onArrows && std::abs(point.y - Line(host, 1)) < pitch * 0.5f) window.Scroll(-1);
        if (onArrows && std::abs(point.y - Line(host, host->entryIndex)) < pitch * 0.5f) window.Scroll(1);
        if (step) Change(host, step);
        // The wheel scrolls a row per notch
        for (int notches = host->wheel.Take(REWindow::wheelDelta.exchange(0)); notches; notches += notches > 0 ? -1 : 1)
            window.Scroll(notches > 0 ? -1 : 1);
        // Up/Down at the game's menu repeat rate, wrapping like its lists; Page Up/Page Down a window, Home/End the ends
        if (repeat & 0x40040) window.Move(1);
        if (repeat & 0x10010) window.Move(-1);
        const auto held = [&](int key)
        {
            return keys && (Read<uint32_t>(keys, 2752 + (key >> 5) * 4) & (1u << (key & 31))) != 0;
        };
        const auto now = GetTickCount64();
        if (host->pageUp.Step(held(VK_PRIOR), now)) window.Page(-1);
        if (host->pageDown.Step(held(VK_NEXT), now)) window.Page(1);
        if (held(VK_HOME) && !host->homeHeld) window.Edge(-1);
        if (held(VK_END) && !host->endHeld) window.Edge(1);
        host->homeHeld = held(VK_HOME);
        host->endHeld = held(VK_END);
        if (repeat & 0x80080) Change(host, -1);
        if (repeat & 0x20020) Change(host, 1);
        if (accept) Change(host, 0);
        if (help) Help(host);
        if (!host->inHelp) Populate(host);
        return 0;
    }

    inline void Install(HMODULE ownModule)
    {
        module = ownModule;
        Bind();
        initHook = safetyhook::create_inline(initialize, Initialize);
        execHook = safetyhook::create_inline(reinterpret_cast<void*>(menuVtable[8]), Exec);
        destroyHook = safetyhook::create_inline(reinterpret_cast<void*>(menuVtable[0]), Destroy);
        releaseHook = safetyhook::create_inline(reinterpret_cast<void*>(menuVtable[36]), Release);
        inputHook = safetyhook::create_inline(processInput, Input);
        firstCountHook = safetyhook::create_mid(countInstruction + 7, [](SafetyHookContext& context)
        {
            auto gui = reinterpret_cast<void*>(originalCompiler ? context.esi : context.edi);
            if (!hosts.count(gui) || !Current(hosts.at(gui))) return;
            if (originalCompiler) ++context.ebx;
            else ++context.eax;
        });
        if (secondCountInstruction) // Current executable
            secondCountHook = safetyhook::create_mid(secondCountInstruction + 7, [](SafetyHookContext& context)
            {
                auto gui = reinterpret_cast<void*>(context.edi);
                if (hosts.count(gui) && Current(hosts.at(gui))) ++context.eax;
            });
    }
}
