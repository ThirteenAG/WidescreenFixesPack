#include "Game.hpp"
#include "../Shared/Console/PortableIni.hpp"
#include "../Shared/Console/Text.hpp"
#include <cstddef>
#include <cstring>

// Native Display menu extension. The game's own slider/multistate widgets are
// copied into bounded module storage; the game's navigation, fonts, highlights,
// sliders and translated on/off strings draw them. Original lists are restored
// before the game destroys or reloads the frontend pages.
namespace vcsws {
namespace {
struct CharList { char *begin, *end, *capacity; };
template<class T> struct List { T *begin, *end, *capacity; };
struct MenuItem {
    CharList name;
    int x, y, width, height;
    float scale;
    uint8_t visible, padding0[3];
    uintptr_t vtable;
    int hook, hookType;
    uint8_t render, padding1[3];
    CharList label;
    int labelX, labelY;
    uint8_t normal[4], focused[4];
    float scaleX, scaleY;
    int horizontal, vertical;
    uint16_t font, padding2;
    List<void*> actions[7];
};
struct Widget {
    MenuItem item;
    union {
        struct { float value; int minimum, maximum; uint8_t steps, padding[3]; } slider;
        struct { List<CharList> strings; uint8_t count, index, previous, padding; } toggle;
    };
};
struct MenuPage {
    CharList name; int width, height; uint8_t inputSent, padding[3]; CharList defaultItem;
    List<MenuItem*> items; MenuItem* selected; int trigger;
};
static_assert(sizeof(MenuItem) == 0xB8 && offsetof(MenuItem, hook) == 0x28 && offsetof(MenuItem, label) == 0x34 &&
    offsetof(MenuItem, horizontal) == 0x58 && offsetof(MenuItem, actions) == 0x64);
static_assert(sizeof(Widget) == 0xC8 && sizeof(MenuPage) == 0x38 && offsetof(MenuPage, items) == 0x24);

enum Option : int { FrameRate = 0x100, MoreOptions, HudSize, RadarSize, FieldOfView, CutsceneBorders,
    CutsceneFov, FastLoading, KeyboardCheats, Back, OptionEnd };
constexpr unsigned optionCount = OptionEnd - FrameRate;
constexpr char keys[optionCount][20] = {"WF_FRAME_RATE", "WF_MORE_OPTIONS", "WF_HUD_SIZE", "WF_RADAR_SIZE",
    "WF_FOV", "WF_CUT_BORDERS", "WF_CUT_FOV", "WF_FAST_LOADING", "WF_KEY_CHEATS", "WF_BACK"};
constexpr char16_t captions[optionCount][28] = {u"FRAME RATE:", u"MORE DISPLAY OPTIONS", u"HUD SIZE:",
    u"RADAR SIZE:", u"FIELD OF VIEW:", u"CUTSCENE BORDERS:", u"ORIGINAL CUTSCENE FOV:", u"FAST LOADING:",
    u"KEYBOARD CHEATS:", u""};
constexpr char fpsKeys[2][12] = {"WF_FPS_30", "WF_FPS_60"};
constexpr char16_t fpsText[2][4] = {u"30", u"60"};
constexpr char onOff[2][8] = {"FEM_OFF", "FEM_ON"};
constexpr unsigned maxItems = 24;
bool Slider(int option) { return option == HudSize || option == RadarSize || option == FieldOfView; }
bool Action(int option) { return option == MoreOptions || option == Back; }

struct Extension {
    MenuPage* page = nullptr;
    List<MenuItem*> original{};
    MenuItem* originalSelection = nullptr;
    int originalY[maxItems]{};
    MenuItem* mainItems[maxItems]{};
    MenuItem* subItems[maxItems]{};
    List<MenuItem*> mainList{}, subList{};
    MenuItem* mainSelection = nullptr;
    Widget widgets[optionCount]{};
    CharList values[optionCount][2]{};
    bool submenu = false;
};
Extension extension;
bool dirty = false;

SafetyMipsInline drawHook, inputHook, getHookHook, setHookHook, textHook, closeHook, destroyHook, defaultItemHook;
void (*playSound)(void*, uint16_t, uint32_t);
uintptr_t dmAudio;

CharList Literal(const char* value) {
    auto* begin = const_cast<char*>(value);
    const auto length = std::strlen(value);
    return {begin, begin + length, begin + length + 1};
}
template<class Return = void, class... Args> Return Virtual(MenuItem* item, unsigned slot, Args... args) {
    const auto entry = item->vtable + slot * 8;
    const auto adjust = *reinterpret_cast<const int16_t*>(entry);
    auto function = reinterpret_cast<Return (*)(void*, Args...)>(*reinterpret_cast<const uintptr_t*>(entry + 4));
    return function(reinterpret_cast<char*>(item) + adjust, args...);
}
bool Selectable(const MenuItem* item) { return item && item->visible && item->hook > 0; }
bool Owned(const MenuItem* item) {
    return item >= &extension.widgets[0].item && item < &extension.widgets[optionCount].item &&
        (reinterpret_cast<uintptr_t>(item) - reinterpret_cast<uintptr_t>(&extension.widgets[0])) % sizeof(Widget) == 0;
}
bool InPage(const MenuItem* item) {
    if (!extension.page) return false;
    for (auto p = extension.page->items.begin; p != extension.page->items.end; ++p) if (*p == item) return true;
    return false;
}
int Percent(float value) { return int(value * 100.0f + 0.5f); }
int Value(int option) {
    switch (option) {
    case FrameRate: return settings.fps != 0;
    case HudSize: return Percent(Drawing::settings.hud);
    case RadarSize: return Percent(Drawing::settings.radar);
    case FieldOfView: return Percent(settings.fov);
    case CutsceneBorders: return settings.cutsceneBorders;
    case CutsceneFov: return settings.restoreCutsceneFov;
    case FastLoading: return settings.unthrottle;
    case KeyboardCheats: return settings.pcCheats;
    default: return 0;
    }
}
void SetOption(int option, int value) {
    const float scale = console::bounded(value * 0.01f, 0.5f, 1.5f, 1.0f);
    switch (option) {
    case FrameRate: settings.fps = value != 0; ApplyFrameRate(); break;
    case HudSize: Drawing::settings.hud = scale; break;
    case RadarSize: Drawing::settings.radar = scale; break;
    case FieldOfView: settings.fov = scale; break;
    case CutsceneBorders: settings.cutsceneBorders = value != 0; break;
    case CutsceneFov: settings.restoreCutsceneFov = value != 0; break;
    case FastLoading: settings.unthrottle = value != 0; if (!settings.unthrottle) console::portable::Unthrottle(false); break;
    case KeyboardCheats: settings.pcCheats = value != 0; break;
    default: return;
    }
    dirty = true;
}
int GetHookData(MenuItem* item, void* text) {
    if (item->hook >= FrameRate && item->hook < OptionEnd) return Value(item->hook);
    return getHookHook.call<int>(item, text);
}
void SetHookData(MenuItem* item, int value) {
    if (item->hook >= FrameRate && item->hook < OptionEnd) { SetOption(item->hook, value); return; }
    setHookHook.call<void>(item, value);
}
const uint16_t* GetText(void* text, const char* key) {
    // Multistate widgets look up a copy of their value key; compare contents.
    if (key && key[0] == 'W' && key[1] == 'F' && key[2] == '_') {
        for (unsigned i = 0; i < unsigned(Back - FrameRate); ++i)
            if (!std::strcmp(key, keys[i])) return reinterpret_cast<const uint16_t*>(captions[i]);
        for (unsigned i = 0; i < 2; ++i)
            if (!std::strcmp(key, fpsKeys[i])) return reinterpret_cast<const uint16_t*>(fpsText[i]);
        if (!std::strcmp(key, "WF_BACK")) {
            // The footer's translated "back", in the option rows' capitals.
            static uint16_t back[32];
            const auto* source = textHook.call<const uint16_t*>(text, "FEM_BAK");
            unsigned i = 0;
            for (; source && source[i] && i + 1 < sizeof(back) / sizeof(back[0]); ++i)
                back[i] = source[i] >= 'a' && source[i] <= 'z' ? uint16_t(source[i] - 'a' + 'A') : source[i];
            back[i] = 0;
            return back;
        }
    }
    return textHook.call<const uint16_t*>(text, key);
}
void Refresh(Widget& widget) {
    const int option = widget.item.hook;
    if (Slider(option)) {
        const float range = float(widget.slider.maximum - widget.slider.minimum);
        widget.slider.value = console::bounded((Value(option) - widget.slider.minimum) / range, 0.0f, 1.0f, 0.5f);
    } else if (!Action(option)) {
        widget.toggle.index = widget.toggle.previous = uint8_t(Value(option));
    }
}
void Initialize(Extension& e, const Widget& slider, const Widget& toggle, const MenuItem& row) {
    for (unsigned i = 0; i < optionCount; ++i) {
        const int option = FrameRate + int(i);
        auto& widget = e.widgets[i];
        std::memcpy(&widget, Slider(option) ? &slider : &toggle, sizeof(Widget));
        widget.item.name = Literal(keys[i]);
        widget.item.label = widget.item.name;
        widget.item.hook = option;
        widget.item.hookType = 0;
        widget.item.x = row.x; widget.item.y = row.y;
        // Native actions reference the prototype's name. Input for these rows
        // is handled by the trigger hook, so no actions are shared.
        std::memset(widget.item.actions, 0, sizeof(widget.item.actions));
        if (Slider(option)) {
            widget.slider.minimum = 50; widget.slider.maximum = 150; widget.slider.steps = 10;
        } else if (Action(option)) {
            e.values[i][0] = Literal(keys[i]);
            widget.toggle.strings = {e.values[i], e.values[i] + 1, e.values[i] + 1};
            widget.toggle.count = 1;
            // Match native centred actions: one caption without a label.
            widget.item.render = 0; widget.item.x = 0; widget.item.width = 480; widget.item.horizontal = 1;
        } else {
            const bool fps = option == FrameRate;
            e.values[i][0] = Literal(fps ? fpsKeys[0] : onOff[0]);
            e.values[i][1] = Literal(fps ? fpsKeys[1] : onOff[1]);
            widget.toggle.strings = {e.values[i], e.values[i] + 2, e.values[i] + 2};
            widget.toggle.count = 2;
        }
        Refresh(widget);
    }
}
void Show(bool submenu) {
    auto& e = extension;
    auto& page = *e.page;
    if (submenu && !e.submenu) e.mainSelection = page.selected;
    e.submenu = submenu;
    page.items = submenu ? e.subList : e.mainList;
    if (!submenu) { page.selected = e.mainSelection ? e.mainSelection : e.mainItems[0]; return; }
    for (auto& widget : e.widgets) Refresh(widget);
    page.selected = &e.widgets[HudSize - FrameRate].item;
}
void Attach(MenuPage* page) {
    auto& e = extension;
    if (e.page || !page || !page->name.begin || std::strncmp(page->name.begin, "DISPLAY_PAGE", 12)) return;
    const auto count = page->items.end - page->items.begin;
    if (count <= 0 || count > int(maxItems - optionCount)) return;
    Widget *slider = nullptr, *toggle = nullptr;
    for (auto p = page->items.begin; p != page->items.end; ++p) if (*p) {
        if ((*p)->hook == 4) slider = reinterpret_cast<Widget*>(*p);   // Brightness
        if ((*p)->hook == 3) toggle = reinterpret_cast<Widget*>(*p);   // Subtitles (off/on)
    }
    if (!slider || !toggle || toggle->toggle.count != 2) return;
    e.page = page;
    e.original = page->items;
    e.originalSelection = page->selected;
    e.submenu = false; e.mainSelection = nullptr;
    Initialize(e, *slider, *toggle, toggle->item);
    unsigned main = 0, sub = 0;
    for (unsigned i = 0; i < unsigned(count); ++i) {
        auto* item = page->items.begin[i];
        e.originalY[i] = item ? item->y : 0;
        e.mainItems[main++] = item;
        // The page heading and other non-option assets stay in the submenu.
        if (item && item->hook == 0) e.subItems[sub++] = item;
    }
    e.mainItems[main++] = &e.widgets[FrameRate - FrameRate].item;
    e.mainItems[main++] = &e.widgets[MoreOptions - FrameRate].item;
    for (int option = HudSize; option <= Back; ++option) e.subItems[sub++] = &e.widgets[option - FrameRate].item;
    e.mainList = {e.mainItems, e.mainItems + main, e.mainItems + maxItems};
    e.subList = {e.subItems, e.subItems + sub, e.subItems + maxItems};
    // Fit the extended rows above the footer; native fonts and columns remain.
    int firstY = 272, rows = 0;
    for (unsigned i = 0; i < main; ++i) if (Selectable(e.mainItems[i])) {
        if (e.mainItems[i]->y < firstY) firstY = e.mainItems[i]->y;
        ++rows;
    }
    if (rows > 1) {
        const int step = (196 - firstY) / (rows - 1);
        int y = firstY;
        for (unsigned i = 0; i < main; ++i) if (Selectable(e.mainItems[i])) { e.mainItems[i]->y = y; y += step; }
    }
    int y = 54;
    for (int option = HudSize; option <= Back; ++option) { e.widgets[option - FrameRate].item.y = y; y += 19; }
    page->items = e.mainList;
}
void Detach(MenuPage* page) {
    auto& e = extension;
    if (!page || e.page != page) return;
    page->items = e.original;
    page->selected = e.originalSelection && InPage(e.originalSelection) ? e.originalSelection : page->selected;
    for (unsigned i = 0; page->items.begin + i != page->items.end && i < maxItems; ++i)
        if (page->items.begin[i]) page->items.begin[i]->y = e.originalY[i];
    // Never leave a page pointing at module-owned widgets.
    if (Owned(page->selected)) page->selected = page->items.begin != page->items.end ? page->items.begin[0] : nullptr;
    e.page = nullptr; e.submenu = false;
}
void Navigate(MenuPage& page, MenuItem* item, int direction) {
    const int count = int(page.items.end - page.items.begin);
    int index = 0;
    while (index < count && page.items.begin[index] != item) ++index;
    if (index == count) return;
    for (int remaining = count; remaining; --remaining) {
        index = (index + direction + count) % count;
        if (Selectable(page.items.begin[index])) {
            page.selected = page.items.begin[index];
            playSound(reinterpret_cast<void*>(dmAudio), 0xC5, 0);
            return;
        }
    }
}
void InputTrigger(MenuItem* item, int trigger) {
    auto& e = extension;
    if (!e.page || e.page->selected != item || !InPage(item) || item->hook <= 0) {
        inputHook.call<void>(item, trigger); return;
    }
    auto& page = *e.page;
    if (trigger == 2 || trigger == 3) { Navigate(page, item, trigger == 2 ? -1 : 1); return; }
    if (!Owned(item)) { inputHook.call<void>(item, trigger); return; }
    const int option = item->hook;
    if (trigger == 5) {
        if (e.submenu) { Show(false); playSound(reinterpret_cast<void*>(dmAudio), 0xC5, 0); return; }
        // Leave the page through a native row's cancel action.
        for (auto p = e.original.begin; p != e.original.end; ++p)
            if (Selectable(*p)) { inputHook.call<void>(*p, trigger); return; }
        return;
    }
    if (Action(option)) {
        if (trigger == 4) { Show(option == MoreOptions); playSound(reinterpret_cast<void*>(dmAudio), 0xC5, 0); }
        return;
    }
    if (trigger == 0 || trigger == 1 || (trigger == 4 && !Slider(option))) {
        // Native OnPrevMenuState/OnNextMenuState update the widget and call SetHookData.
        Virtual(item, trigger == 0 ? 12 : 11);
        playSound(reinterpret_cast<void*>(dmAudio), 0xC5, 0);
    }
}
void Save() {
    if (!dirty) return;
    using console::portable::IniValue;
    IniValue values[8] = {
        {"MAIN", "Enable60FPS", {}}, {"MAIN", "UnthrottleEmuDuringLoading", {}}, {"CONTROLS", "PCCheats", {}},
        {"HUD", "HudScale", {}}, {"RADAR", "RadarScale", {}}, {"FOV", "FOVFactor", {}},
        {"FOV", "RestoreCutsceneFOV", {}}, {"FOV", "CutsceneBorders", {}}};
    std::strcpy(values[0].value, settings.fps ? "1" : "0");
    // A configured value of 2 (also fast-forward fades) is kept while enabled.
    std::strcpy(values[1].value, !settings.unthrottle ? "0" : settings.unthrottleMode == 2 ? "2" : "1");
    std::strcpy(values[2].value, settings.pcCheats ? "1" : "0");
    console::format(values[3].value, "", Drawing::settings.hud, 2);
    console::format(values[4].value, "", Drawing::settings.radar, 2);
    console::format(values[5].value, "", settings.fov, 2);
    std::strcpy(values[6].value, settings.restoreCutsceneFov ? "1" : "0");
    std::strcpy(values[7].value, settings.cutsceneBorders ? "1" : "0");
    static console::portable::IniWriter<> writer;
    const bool saved = writer.update(iniPath, values, 8);
#ifndef NDEBUG
    logger.WriteF("Menu settings %s", saved ? "saved" : "could not be saved");
#endif
    if (saved) dirty = false;
}
void Close(void* menu, bool immediate) {
    if (extension.page && extension.submenu) Show(false);
    closeHook.call<void>(menu, immediate);
    Save();
}
// cMenuItems::SetCurrentItemByNameTrigger looks the page's XML default item up
// by name and calls through the result without a null check (PerformON,
// SetActivePage). The submenu list lacks that item, so whatever path re-enters
// the page (including a menu shutdown that bypasses Close), return to the
// main list first.
void SelectDefaultItem(MenuPage* page) {
    if (page && page == extension.page && extension.submenu) Show(false);
    defaultItemHook.call<void>(page);
}
void DestroyPage(MenuPage* page, int flags) {
    Detach(page);
    destroyHook.call<void>(page, flags);
}
void Draw(void* menu) {
    if (autoAspect) Drawing::settings.aspect = console::portable::Aspect();
    const auto manager = reinterpret_cast<uintptr_t>(menu);
    const auto begin = *reinterpret_cast<MenuPage***>(manager + 4);
    const auto end = *reinterpret_cast<MenuPage***>(manager + 8);
    if (!extension.page && begin && end > begin && end - begin <= 64)
        for (auto p = begin; p != end; ++p) Attach(*p);
    // A submenu is only valid while its page has input focus.
    if (extension.page && extension.submenu) {
        const int8_t screen = *reinterpret_cast<int8_t*>(manager + 0x1C);
        const bool tabs = *reinterpret_cast<uint8_t*>(manager + 0x1E) != 0;
        if (tabs || screen < 0 || screen >= end - begin || begin[screen] != extension.page) Show(false);
    }
    Drawing::Scope scope(Anchor::Center);
    drawHook.call<void>(menu);
}
}
void InstallMenu() {
    playSound = reinterpret_cast<void (*)(void*, uint16_t, uint32_t)>(Address<0x8A062F0>());
    dmAudio = Address<0x8BB3BB8>();
    drawHook = safetymips::create_inline(Address<0x882E518>(), Draw);
    inputHook = safetymips::create_inline(Address<0x8AC5D44>(), InputTrigger);
    getHookHook = safetymips::create_inline(Address<0x8AC6238>(), GetHookData);
    setHookHook = safetymips::create_inline(Address<0x8AC61E0>(), SetHookData);
    textHook = safetymips::create_inline(Address<0x89F6390>(), GetText);
    closeHook = safetymips::create_inline(Address<0x882DC20>(), Close);
    destroyHook = safetymips::create_inline(Address<0x8AE1CE0>(), DestroyPage);
    defaultItemHook = safetymips::create_inline(Address<0x8AE25E8>(), SelectDefaultItem);
}
}
