#include "Controls.hpp"
#include "ButtonIcons.hpp"
#include "../Shared/Console/Text.hpp"
#include <cstring>
#include "Settings.hpp"
#include "../Shared/Console/PS2Settings.hpp"
PCSX2F_CFG_PEER_SETTINGS();
extern "C" {
#include "../../external/injector/include/ps2/plugin_settings.h"
}

namespace vcs {
namespace {
struct String { char *begin, *end; uint32_t unused; char* capacity; };
template<class T> struct List { T *begin, *end; uint32_t unused; T* capacity; };
struct MenuItem {
    String name;
    int x, y, width, height;
    float scale;
    int visible;
    uintptr_t* vtable;
    int hook, hookType, render;
    String label;
    int labelX, labelY;
    Color normal, focused;
    float scaleX, scaleY;
    int horizontal, vertical;
    uint16_t font, padding;
    List<void*> actions[7];
};
struct Widget {
    MenuItem item;
    union {
        struct { float value; int minimum, maximum; uint8_t steps, padding[3]; } slider;
        struct { List<String> strings; uint8_t count, index, previous, padding; } toggle;
    };
};
struct MenuPage {
    String name; int width, height, inputSent; String defaultItem;
    List<MenuItem*> items; MenuItem* selected; int trigger, unused[2];
};
static_assert(sizeof(MenuItem) == 220 && sizeof(Widget) == 240 && sizeof(MenuPage) == 76);
// Plugin widgets use hook ids the game never assigns. Binding rows are Binding+row.
enum Option { HudSize = 0x100, FastLoading, MouseSensitivity, InvertMouse, FrameRate,
    DisplayOptions, ControlOptions, Bindings, NextBindings, Back, CutsceneBorders,
    PcControls, ResetBindings, Preset, ApplyPreset, LightsPage, VehicleLights, LightsDefaults, FixPage, FixDefaults, OptionEnd, Binding = 0x200 };
constexpr unsigned widgetCount = OptionEnd - HudSize;
constexpr unsigned maxItems = 32, maxPages = 4;
constexpr char keys[4][16] = {"WF_HUD_SIZE", "WF_FAST_LOADING", "WF_MOUSE_SPEED", "WF_INVERT_MOUSE"};
constexpr char16_t english[4][24] = {u"HUD size", u"Fast loading", u"Mouse sensitivity", u"Invert mouse Y"};
constexpr unsigned extraCount = OptionEnd - FrameRate;
constexpr char extraKeys[extraCount][20] = {"WF_FRAME_RATE", "WF_DISPLAY_OPTIONS", "WF_CONTROL_OPTIONS", "WF_BINDINGS",
    "WF_NEXT_BINDINGS", "WF_BACK", "WF_CUTSCENE_BORDERS", "WF_PC_CONTROLS", "WF_RESET_BINDINGS", "WF_PRESET", "WF_APPLY_PRESET", "WF_LIGHTS_PAGE", "WF_LIGHTS", "WF_LIGHTS_DEFAULTS",
    "WF_FIX_PAGE", "WF_FIX_DEFAULTS"};
constexpr char16_t extraText[extraCount][24] = {u"Frame rate", u"Plugins", u"PC controls", u"Key bindings",
    u"Next page", u"Back", u"Cutscene borders", u"Keyboard and mouse", u"Reset to defaults", u"Preset", u"Apply preset", u"Improved vehicle lights",
    u"Lights (restart):", u"Restore defaults", u"Widescreen fix", u"Restore defaults"};
constexpr char boolKeys[2][16] = {"WF_BOOL_OFF", "WF_BOOL_ON"};
constexpr char16_t boolText[2][4] = {u"Off", u"On"};
constexpr char bordersKeys[4][16] = {"WF_BORDERS_OFF", "WF_BORDERS_LB", "WF_BORDERS_PB", "WF_BORDERS_BOTH"};
constexpr char16_t bordersText[4][12] = {u"Off", u"Letterbox", u"Pillarbox", u"Both"};
constexpr char fpsKeys[2][12] = {"WF_FPS_30", "WF_FPS_60"};
constexpr char16_t fpsText[2][4] = {u"30", u"60"};
enum class MenuLanguage : uint8_t { Automatic, English };
struct MenuTextState {
    MenuLanguage requested = MenuLanguage::Automatic;
    MenuLanguage catalog = MenuLanguage::English;
} menuText;
MenuLanguage ParseLanguage(const char* value) {
    if (!value) return MenuLanguage::Automatic;
    while (*value == ' ' || *value == '\t') ++value;
    if (value[0] != 'e' || value[1] != 'n') return MenuLanguage::Automatic;
    value += 2;
    const bool separated = *value == ' ' || *value == '\t';
    while (*value == ' ' || *value == '\t' || *value == '\r') ++value;
    return !*value || (separated && *value == ';') ? MenuLanguage::English : MenuLanguage::Automatic;
}
void ReloadMenuLanguage() {
    // One read-only own-CFG request in static module storage, never a full Load.
    static PCSX2FIniRequest request;
    request = {}; request.size = sizeof(request); request.version = 1;
    request.operation = PCSX2F_SETTINGS_READ; request.count = 1;
    std::strcpy(request.entries[0].section, "MAIN"); std::strcpy(request.entries[0].key, "TextLanguage");
    const auto status = console::ps2settings::request(request);
    request.entries[0].value[sizeof(request.entries[0].value) - 1] = 0;
    menuText.requested = ParseLanguage(status == PCSX2F_SETTINGS_OK ? request.entries[0].value : config::values[10].value);
    // English is the only shipped host/sibling catalog; Auto falls back to it.
    switch (menuText.requested) {
    case MenuLanguage::Automatic: case MenuLanguage::English: menuText.catalog = MenuLanguage::English; break;
    }
}
bool IsAction(unsigned option) {
    return (option >= DisplayOptions && option <= Back) || option == ResetBindings || option == ApplyPreset || option == LightsPage ||
        option == LightsDefaults || option == FixPage || option == FixDefaults;
}
bool IsSlider(unsigned option) { return option == HudSize || option == MouseSensitivity; }

// Rebindable actions of this game, in menu order.
struct ActionList {
    Act list[console::stories::ActionCount]{};
    unsigned count = 0;
    ActionList() {
        for (unsigned i = 0; i < console::stories::ActionCount; ++i)
            if (console::stories::actions[i].games & game) list[count++] = Act(i);
    }
};
const ActionList available;
constexpr unsigned bindingsPerPage = 9;
unsigned BindingPages() { return (available.count + bindingsPerPage - 1) / bindingsPerPage; }
constexpr char bindingLabels[bindingsPerPage][16] = {"WF_BIND0", "WF_BIND1", "WF_BIND2", "WF_BIND3", "WF_BIND4", "WF_BIND5", "WF_BIND6", "WF_BIND7", "WF_BIND8"};
constexpr char bindingValues[bindingsPerPage][16] = {"WF_KEY0", "WF_KEY1", "WF_KEY2", "WF_KEY3", "WF_KEY4", "WF_KEY5", "WF_KEY6", "WF_KEY7", "WF_KEY8"};
constexpr unsigned actionWidgets = 12;
constexpr char presetKeys[4][16] = {"WF_CUSTOM", "WF_VANILLA", "WF_RECOMMENDED", "WF_ENHANCED"};
constexpr char16_t presetText[4][16] = {u"Custom", u"Vanilla", u"Recommended", u"Enhanced"};
constexpr char lightsKeys[2][16] = {"WF_LIGHTS_OFF", "WF_LIGHTS_ON"};
constexpr char16_t lightsText[2][4] = {u"Off", u"On"};
struct PageExtension {
    MenuPage* page = nullptr;
    List<MenuItem*> original{};
    MenuItem* originalSelection = nullptr;
    int originalY[maxItems]{};
    MenuItem* items[maxItems]{};
    Widget widgets[widgetCount]{};
    Widget hints[bindingsPerPage]{};
    String hintValues[bindingsPerPage]{}, fpsValues[2]{}, presetValues[4]{}, boolValues[2]{}, bordersValues[4]{};
    String actionValues[actionWidgets]{}, lightsValues[2]{};
    MenuItem* submenuItems[maxItems]{};
    List<MenuItem*> mainItems{};
    MenuItem* mainSelection = nullptr;
    // 0 main list, 1 plugin list, 2 bindings, 3 PC controls, 4 lights, 5 the fix's page
    unsigned mode = 0, bindingPage = 0;
    bool controls = false;
};
PageExtension pages[maxPages];
Widget *sliderPrototype = nullptr, *togglePrototype = nullptr;
PageExtension* textPage = nullptr;
bool dirty = false;
int selectedPreset = 0;
bool confirmPreset = false, confirmDefaults = false;
// Requested restart state only; the host resolves this exact loaded sibling.
console::ps2settings::RestartBoolPeer lightsPeer("GTAVCS.PCSX2F.ImVehLM.elf", "MAIN", "VehicleLights");
bool lightsAvailable = false, requestedLights = false, confirmLightsDefaults = false, lightsWriteRefused = false;
bool ReadLights() {
    bool requested = false;
    const bool available = lightsPeer.Read(requested);
    if (!available || available != lightsAvailable || requested != requestedLights) confirmLightsDefaults = false;
    lightsAvailable = available;
    if (available) requestedLights = requested;
    return available;
}
bool RequestLights(bool requested) {
    if (!ReadLights()) { lightsWriteRefused = true; return false; }
    const bool saved = lightsPeer.Write(requested);
    lightsWriteRefused = !saved;
    if (saved) { requestedLights = requested; confirmLightsDefaults = false; }
    else ReadLights();
    return saved;
}
void RefreshLights(PageExtension& extension) {
    auto& lights = extension.widgets[VehicleLights - HudSize];
    lights.toggle.index = lights.toggle.previous = uint8_t(requestedLights);
}
// The descriptor's presets: 1 vanilla, 2 recommended (the defaults), 3 enhanced (0 = custom).
// Controls, sensitivity, bindings and Skip Intro (not in this menu) are the player's choice; no preset changes them.
struct PresetValues { bool sixtyFPS, unthrottle; float hudScale; int cutsceneBorders; };
constexpr PresetValues presetValues[4] = {{}, {false, false, 1.0f, 1}, {false, true, 1.0f, 3}, {false, true, 1.0f, 3}};
constexpr int recommendedPreset = 2;
bool PresetMatches(int preset) {
    if (preset < 1 || preset > 3) return false;
    const auto& p = presetValues[preset];
    return settings.sixtyFPS == p.sixtyFPS && settings.unthrottle == p.unthrottle &&
        settings.cutsceneBorders == p.cutsceneBorders && std::fabs(settings.hudScale - p.hudScale) <= 0.001f;
}
int MatchingPreset() { return PresetMatches(2) ? 2 : PresetMatches(1) ? 1 : PresetMatches(3) ? 3 : 0; }
// Press once to arm, again to apply; nothing happens when the values already match.
bool ApplyPresetValues(int preset, bool& confirm) {
    if (preset < 1 || preset > 3 || PresetMatches(preset)) { confirm = false; return false; }
    if (!confirm) { confirm = true; return false; }
    const auto& p = presetValues[preset];
    settings.sixtyFPS = p.sixtyFPS; settings.unthrottle = p.unthrottle;
    settings.hudScale = p.hudScale; settings.cutsceneBorders = p.cutsceneBorders;
    ApplyFrameRate(); dirty = true; confirm = false;
    return true;
}
unsigned ParentMode(unsigned mode) { return mode == 2 ? 3 : mode == 3 ? 5 : mode == 4 || mode == 5 ? 1 : 0; }
unsigned renderedBindings = 0;
// Key capture of the bindings page.
unsigned bindingSlot = 0, captureFrames = 0;
Act captureAction = Act::Forward;
unsigned captureSlot = 0;
PageExtension* captureOwner = nullptr;
char16_t valueText[bindingsPerPage][48]{};
safetymips::GameInline<uint64_t(void*)> drawMenu;
safetymips::GameInline<uint64_t(MenuPage*, const char*)> parsePage;
safetymips::GameInline<void(MenuPage*, uint8_t)> destroyPage;
safetymips::GameInline<void(MenuItem*, int)> inputTrigger;
safetymips::GameInline<int(MenuItem*, void*)> getValue;
safetymips::GameInline<void(MenuItem*, int)> setValue;
safetymips::GameInline<const uint16_t*(void*, const char*)> getText;
safetymips::GameInline<void(void*, bool)> closeMenu;
safetymips::GameInline<void(MenuPage*)> setCurrentItem;

void PlaySound() {
    reinterpret_cast<void (*)(void*, uint16_t, uint32_t)>(0x2FA808)(reinterpret_cast<void*>(0x520D60), 0, 0);
}
String Literal(const char* value) {
    auto* begin = const_cast<char*>(value);
    return {begin, begin + std::strlen(value), 0, begin + std::strlen(value) + 1};
}
bool Selectable(MenuItem* item) {
    // m_bRender controls the separate label, not focus: Screen Position draws
    // its title through the multistate widget. Hook-less assets include the
    // page heading and the Screen Position editor, not ordinary option rows.
    return item && item->visible && item->hook && item->vtable &&
        reinterpret_cast<bool (*)(MenuItem*, uintptr_t)>(item->vtable[3])(item, 0x47FA50);
}
PageExtension* Owner(MenuItem* item) {
    for (auto& extension : pages) if (extension.page)
        for (auto p = extension.page->items.begin; p != extension.page->items.end; ++p)
            if (*p == item) return &extension;
    return nullptr;
}
int Value(unsigned option) {
    switch (option) {
    case HudSize: return int(settings.hudScale * 100.0f + 0.5f);
    case FastLoading: return settings.unthrottle;
    case MouseSensitivity: return int(settings.mouseSensitivity * 10000.0f + 0.5f);
    case InvertMouse: return settings.invertMouse;
    case FrameRate: return settings.sixtyFPS;
    case CutsceneBorders: return settings.cutsceneBorders;
    case PcControls: return settings.pcControls;
    case Preset: return selectedPreset;
    case VehicleLights: return requestedLights;
    default: return 0;
    }
}
int GetValue(MenuItem* item, void* text) {
    return item->hook >= HudSize ? Value(item->hook) : getValue.call(item, text);
}
void SetValue(MenuItem* item, int value) {
    if (item->hook == VehicleLights) {
        auto* extension = Owner(item);
        if (!extension || extension->mode != 4 || extension->page->selected != item ||
            item != &extension->widgets[item->hook - HudSize].item) return;
        if (value == 0 || value == 1) RequestLights(value != 0);
        else lightsWriteRefused = true;
        confirmLightsDefaults = false;
        RefreshLights(*extension);
        return;
    }
    switch (item->hook) {
    case Preset: selectedPreset = value; confirmPreset = false; return;
    case HudSize: settings.hudScale = console::bounded(value * 0.01f, 0.5f, 1.5f, 1.0f); break;
    case FastLoading: settings.unthrottle = value != 0; break;
    case MouseSensitivity: settings.mouseSensitivity = console::bounded(value * 0.0001f, 0.0001f, 0.02f, 0.002f); break;
    case InvertMouse: settings.invertMouse = value != 0; break;
    case FrameRate: settings.sixtyFPS = value != 0; ApplyFrameRate(); break;
    case CutsceneBorders: settings.cutsceneBorders = value >= 0 && value <= 3 ? value : 3; break;
    case PcControls:
        if (settings.pcControls == (value != 0)) return;
        settings.pcControls = value != 0;
        ApplyPcControls();
        break;
    default: setValue.call(item, value); return;
    }
    selectedPreset = MatchingPreset(); confirmPreset = false; confirmDefaults = false;
    dirty = true;
}
// Index into the available actions for a binding row of a page, or -1.
int RowAction(const PageExtension& extension, unsigned row) {
    const unsigned index = extension.bindingPage * bindingsPerPage + row;
    return row < bindingsPerPage && index < available.count ? int(available.list[index]) : -1;
}
void CancelCapture() {
    capture = {};
    captureOwner = nullptr;
}
bool IconRow(Act action, bool focused) {
    if (captureOwner && capture.active() && captureAction == action) return false;
    const auto* slots = bindings.keys[action];
    if (!slots[0] && !slots[1]) return false;
    if (focused && !slots[bindingSlot]) return false;
    for (unsigned i = 0; i < 2; ++i) if (slots[i] && KeyIconWidth(slots[i], 14.0f) == 0.0f) return false;
    return true;
}
const char16_t* BindingText(unsigned row, bool focused) {
    auto& out = valueText[row];
    unsigned used = 0;
    const auto append = [&](const char* text) { while (*text && used + 1 < 48) out[used++] = char16_t(*text++); };
    const auto action = Act(RowAction(*textPage, row));
    if (captureOwner && capture.active() && captureAction == action) {
        append(capture.state == console::stories::Capture::Arming ? "..." : "Press a key (Esc cancels)");
    } else {
        for (unsigned slot = 0; slot < 2; ++slot) {
            if (slot) append("  /  ");
            const bool marked = focused && slot == bindingSlot;
            if (marked) append("[");
            char name[24];
            console::stories::KeyText(bindings.keys[action][slot], name);
            append(name[0] ? name : "-");
            if (marked) append("]");
        }
    }
    out[used] = 0;
    return out;
}
const uint16_t* GetText(void* text, const char* key) {
    // Multistate widgets copy their value key before looking up its text.
    // Match our key contents; pointer identity only works for the item label.
    if (menuText.catalog == MenuLanguage::English && key && key[0] == 'W' && key[1] == 'F' && key[2] == '_') {
        for (unsigned i=0; i<4; ++i) if (!std::strcmp(key, keys[i])) return reinterpret_cast<const uint16_t*>(english[i]);
        for (unsigned i=0; i<extraCount; ++i) if (!std::strcmp(key, extraKeys[i])) {
            if (i == ApplyPreset - FrameRate && confirmPreset) return reinterpret_cast<const uint16_t*>(u"Confirm preset");
            if (i == FixDefaults - FrameRate && confirmDefaults) return reinterpret_cast<const uint16_t*>(u"Confirm defaults");
            if (i == LightsDefaults - FrameRate) {
                if (!lightsAvailable) return reinterpret_cast<const uint16_t*>(u"Unavailable");
                if (lightsWriteRefused) return reinterpret_cast<const uint16_t*>(u"Write refused");
                if (confirmLightsDefaults) return reinterpret_cast<const uint16_t*>(u"Confirm defaults");
            }
            return reinterpret_cast<const uint16_t*>(extraText[i]);
        }
        for (unsigned i=0; i<4; ++i) if (!std::strcmp(key, presetKeys[i])) return reinterpret_cast<const uint16_t*>(presetText[i]);
        for (unsigned i=0; i<2; ++i) if (!std::strcmp(key, boolKeys[i])) return reinterpret_cast<const uint16_t*>(boolText[i]);
        for (unsigned i=0; i<2; ++i) if (!std::strcmp(key, fpsKeys[i])) return reinterpret_cast<const uint16_t*>(fpsText[i]);
        for (unsigned i=0; i<4; ++i) if (!std::strcmp(key, bordersKeys[i])) return reinterpret_cast<const uint16_t*>(bordersText[i]);
        for (unsigned i=0; i<2; ++i) if (!std::strcmp(key, lightsKeys[i]))
            return reinterpret_cast<const uint16_t*>(lightsAvailable ? lightsText[i] : u"Unavailable");
        if (textPage && textPage->mode == 2) for (unsigned i=0; i<bindingsPerPage; ++i) {
            const int action = RowAction(*textPage, i);
            if (action < 0) break;
            if (!std::strcmp(key, bindingLabels[i])) return reinterpret_cast<const uint16_t*>(console::stories::actions[action].name);
            if (!std::strcmp(key, bindingValues[i])) {
                const bool focused = textPage->page->selected == &textPage->hints[i].item;
                if (IconRow(Act(action), focused)) {
                    renderedBindings |= 1u<<i;
                    static constexpr char16_t empty[]=u"";
                    return reinterpret_cast<const uint16_t*>(empty);
                }
                return reinterpret_cast<const uint16_t*>(BindingText(i, focused));
            }
        }
        static constexpr char16_t blank[]=u"";
        return reinterpret_cast<const uint16_t*>(blank);
    }
    if (const auto* rewritten = ButtonText(key)) return rewritten;
    return getText.call(text, key);
}
void InitializeWidget(Widget& widget, const Widget& prototype, unsigned option, const MenuItem& position) {
    const bool slider = IsSlider(option);
    // The native slider is 236 bytes, the multistate item 240 bytes.
    std::memcpy(&widget, &prototype, slider ? 236 : 240);
    widget.item.name = Literal(option < FrameRate ? keys[option-HudSize] : extraKeys[option-FrameRate]);
    widget.item.label = widget.item.name;
    widget.item.hook = int(option);
    widget.item.x = position.x;
    widget.item.y = position.y;
    // Actions reference the original item's name. Plugin widgets use the native
    // input hook below instead of invoking those aliased actions.
    std::memset(widget.item.actions, 0, sizeof(widget.item.actions));
    if (slider) {
        widget.slider.minimum = option == HudSize ? 50 : 1;
        widget.slider.maximum = option == HudSize ? 150 : 200;
        widget.slider.steps = prototype.slider.steps;
        widget.slider.value = float(Value(option)-widget.slider.minimum) / float(widget.slider.maximum-widget.slider.minimum);
    } else {
        widget.toggle.index = widget.toggle.previous = uint8_t(Value(option));
    }
}
Widget& WidgetFor(PageExtension& extension, unsigned option) { return extension.widgets[option - HudSize]; }
void OpenSubmenu(PageExtension& extension, unsigned mode) {
    if (!extension.mode && mode == 1) ReloadMenuLanguage();
    CancelCapture();
    // Synchronous host file requests occur only at page/input boundaries.
    if (mode == 1 || mode == 4) ReadLights();
    confirmLightsDefaults = false; lightsWriteRefused = false;
    auto& page = *extension.page;
    if (!extension.mode) extension.mainSelection = page.selected;
    if (mode == 5) selectedPreset = MatchingPreset();
    confirmPreset = false; confirmDefaults = false;
    extension.mode = mode;
    if (!mode) {
        page.items = extension.mainItems;
        page.selected = extension.mainSelection;
        return;
    }
    unsigned count = 0;
    // Reuse the native background, then render the module-owned native widgets.
    if (extension.original.begin[0] && !extension.original.begin[0]->hook)
        extension.submenuItems[count++] = extension.original.begin[0];
    unsigned row = 0;
    const int step = mode == 2 ? 15 : 17;
    const auto add = [&](MenuItem* item) { item->y = 46 + int(row++) * step; extension.submenuItems[count++] = item; };
    const auto option = [&](unsigned id) {
        auto& widget = WidgetFor(extension, id);
        if (IsSlider(id)) widget.slider.value = console::bounded(float(Value(id) - widget.slider.minimum) /
            float(widget.slider.maximum - widget.slider.minimum), 0.0f, 1.0f, 0.5f);
        else if (!IsAction(id)) widget.toggle.index = widget.toggle.previous = uint8_t(Value(id));
        add(&widget.item);
    };
    if (mode == 2) {
        for (unsigned i=0; i<bindingsPerPage && RowAction(extension, i) >= 0; ++i) {
            // Refresh the multistate value so it never indexes past its strings.
            extension.hints[i].toggle.index = extension.hints[i].toggle.previous = 0;
            add(&extension.hints[i].item);
        }
        if (BindingPages() > 1) option(NextBindings);
        option(ResetBindings);
    } else if (mode == 4) {
        option(VehicleLights); option(LightsDefaults);
    } else if (mode == 3) {
        option(PcControls); option(MouseSensitivity); option(InvertMouse); option(Bindings);
    } else if (mode == 5) {
        option(Preset); option(ApplyPreset); option(FrameRate); option(HudSize); option(FastLoading); option(CutsceneBorders); option(ControlOptions); option(FixDefaults);
    } else {
        // The plugin list: this fix first, then the loaded siblings.
        option(FixPage);
        if (lightsAvailable) option(LightsPage);
    }
    option(Back);
    page.items = {extension.submenuItems, extension.submenuItems+count, 0, extension.submenuItems+maxItems};
    page.selected = extension.submenuItems[extension.original.begin[0] && !extension.original.begin[0]->hook ? 1 : 0];
    bindingSlot = 0;
}
// Every path that leaves or re-enters a page goes back to its main list.
void RestoreAll() {
    for (auto& extension : pages) if (extension.page && extension.mode) OpenSubmenu(extension, 0);
    CancelCapture();
}
void Attach(MenuPage* page) {
    if (!settings.widescreen || !page || !page->items.begin) return;
    const auto count = page->items.end - page->items.begin;
    if (count <= 0 || count > int(maxItems-3)) return;
    Widget *brightness = nullptr, *wide = nullptr;
    for (auto p=page->items.begin; p!=page->items.end; ++p) if (*p) {
        if ((*p)->hook == 4) brightness = reinterpret_cast<Widget*>(*p);
        if ((*p)->hook == 14) wide = reinterpret_cast<Widget*>(*p);
    }
    const bool controls = page->name.begin && std::strncmp(page->name.begin, "CONTROLS_PAGE", 13) == 0;
    if (brightness && wide && wide->toggle.count == 2) {
        sliderPrototype = brightness; togglePrototype = wide;
    } else return;
    if (controls) return;
    if (!brightness || !wide) return;
    PageExtension* extension = nullptr;
    for (auto& candidate : pages) {
        if (candidate.page == page) return;
        if (!candidate.page && !extension) extension = &candidate;
    }
    if (!extension) return;
    extension->page = page;
    extension->original = page->items;
    extension->originalSelection = page->selected;
    extension->controls = controls;
    extension->mode = extension->bindingPage = 0;
    for (unsigned i=0; i<widgetCount; ++i)
        InitializeWidget(extension->widgets[i], IsSlider(HudSize+i) ? *brightness : *wide, HudSize+i, wide->item);
    for (unsigned i=0; i<2; ++i) extension->boolValues[i] = Literal(boolKeys[i]);
    // Own boolean controls must not inherit the game's translated value keys.
    for (unsigned option=HudSize; option<OptionEnd; ++option)
        if (option == FastLoading || option == InvertMouse || option == PcControls) {
            auto& widget = WidgetFor(*extension, option);
            widget.toggle.strings = {extension->boolValues,extension->boolValues+2,0,extension->boolValues+2};
            widget.toggle.count = 2;
        }
    for (unsigned i=0; i<4; ++i) extension->bordersValues[i] = Literal(bordersKeys[i]);
    auto& borders = WidgetFor(*extension, CutsceneBorders);
    borders.toggle.strings = {extension->bordersValues,extension->bordersValues+4,0,extension->bordersValues+4};
    borders.toggle.count = 4;
    extension->fpsValues[0] = Literal(fpsKeys[0]); extension->fpsValues[1] = Literal(fpsKeys[1]);
    WidgetFor(*extension, FrameRate).toggle.strings = {extension->fpsValues,extension->fpsValues+2,0,extension->fpsValues+2};
    for (unsigned i=0; i<4; ++i) extension->presetValues[i] = Literal(presetKeys[i]);
    WidgetFor(*extension, Preset).toggle.strings = {extension->presetValues,extension->presetValues+4,0,extension->presetValues+4};
    WidgetFor(*extension, Preset).toggle.count = 4;
    for (unsigned i=0; i<2; ++i) extension->lightsValues[i] = Literal(lightsKeys[i]);
    auto& lights = WidgetFor(*extension, VehicleLights);
    lights.toggle.strings = {extension->lightsValues,extension->lightsValues+2,0,extension->lightsValues+2};
    lights.toggle.count = 2;
    unsigned action = 0;
    for (unsigned option=HudSize; option<OptionEnd; ++option) if (IsAction(option)) {
        auto& value = extension->actionValues[action++];
        auto& widget = WidgetFor(*extension, option);
        value = Literal(extraKeys[option-FrameRate]);
        widget.toggle.strings = {&value,&value+1,0,&value+1};
        widget.toggle.count = 1;
        widget.toggle.index = widget.toggle.previous = 0;
        // Match the native Screen Position action: one centred caption.
        auto& item = widget.item;
        item.render = 0; item.x = 0; item.width = 480; item.horizontal = 1;
    }
    for (unsigned i=0; i<bindingsPerPage; ++i) {
        auto& hint = extension->hints[i];
        std::memcpy(&hint,wide,sizeof(hint));
        hint.item.name = hint.item.label = Literal(bindingLabels[i]);
        hint.item.hook = Binding+i;
        std::memset(hint.item.actions,0,sizeof(hint.item.actions));
        extension->hintValues[i] = Literal(bindingValues[i]);
        hint.toggle.strings = {&extension->hintValues[i],&extension->hintValues[i]+1,0,&extension->hintValues[i]+1};
        hint.toggle.count = 1; hint.toggle.index = hint.toggle.previous = 0;
    }
    // XML/default-focus actions may still refer to the old Widescreen name.
    WidgetFor(*extension, DisplayOptions).item.name = wide->item.name;
    MenuItem* screenPosition = nullptr;
    unsigned mainCount = 0;
    for (unsigned i=0; i<unsigned(count); ++i) {
        auto* item = page->items.begin[i];
        extension->originalY[i] = item ? item->y : 0;
        if (!controls && item && item->hook == 16) screenPosition = item;
        else extension->items[mainCount++] = !controls && item == &wide->item ? &WidgetFor(*extension, DisplayOptions).item : item;
    }
    if (screenPosition) extension->items[mainCount++] = screenPosition;
    page->items = {extension->items, extension->items+mainCount, 0, extension->items+maxItems};
    extension->mainItems = page->items;
    if (!controls && page->selected == &wide->item) page->selected = &WidgetFor(*extension, DisplayOptions).item;
    // Fit display rows above the game's footer. Columns, fonts, slider bars,
    // highlights and page controls continue to use the native widgets.
    int firstY = 272, rows = 0;
    for (auto p=page->items.begin; p!=page->items.end; ++p) if (Selectable(*p)) {
        if ((*p)->y < firstY) firstY = (*p)->y;
        ++rows;
    }
    if (rows > 1) {
        const int step = (198-firstY)/(rows-1);
        int y = firstY;
        for (auto p=page->items.begin; p!=page->items.end; ++p) if (Selectable(*p)) { (*p)->y = y; y += step; }
    }
}
uint64_t ParsePage(MenuPage* page, const char* filename) {
    const auto result = parsePage.call(page, filename);
    if (result) Attach(page);
    return result;
}
void DestroyPage(MenuPage* page, uint8_t flags) {
    for (auto& extension : pages) if (extension.page == page) {
        if (captureOwner == &extension) CancelCapture();
        page->items = extension.original;
        page->selected = extension.originalSelection;
        for (unsigned i=0; page->items.begin+i != page->items.end; ++i)
            if (page->items.begin[i]) page->items.begin[i]->y = extension.originalY[i];
        extension.page = nullptr;
        extension.mode = 0;
        if (sliderPrototype && Owner(&sliderPrototype->item) == nullptr) sliderPrototype = togglePrototype = nullptr;
        if (textPage == &extension) textPage = nullptr;
        break;
    }
    // Restore the engine's list before it destroys items or frees the list.
    // Cloned widgets and the extended pointer array belong only to this module.
    destroyPage.call(page, flags);
}
// cMenuItems::SetCurrentItemByNameTrigger runs when a page is (re)activated
// (SetActivePage, PerformONE, SetDefaultPageControl). It looks up the XML
// default item by name and calls through the result, so the page must show
// its main list, which keeps every native item.
void SetCurrentItem(MenuPage* page) {
    for (auto& extension : pages) if (extension.page == page && extension.mode) OpenSubmenu(extension, 0);
    setCurrentItem.call(page);
}
void InputTrigger(MenuItem* item, int trigger) {
    auto* extension = Owner(item);
    if (extension && extension->page->selected == item && item->hook) {
        auto& page = *extension->page;
        if (item->hook >= LightsPage && item->hook <= LightsDefaults) {
            if (item != &extension->widgets[item->hook - HudSize].item ||
                (item->hook == LightsPage ? extension->mode != 1 : extension->mode != 4)) return;
        }
        if (captureOwner && capture.active()) return;
        if (item->hook != ApplyPreset || trigger != 4) confirmPreset = false;
        if (item->hook != FixDefaults || trigger != 4) confirmDefaults = false;
        if (item->hook != LightsDefaults || trigger != 4) confirmLightsDefaults = false;
        if (trigger == 5 && extension->mode) { OpenSubmenu(*extension, ParentMode(extension->mode)); return; }
        if (trigger == 2 || trigger == 3) {
            const int count = int(page.items.end-page.items.begin), direction = trigger == 2 ? -1 : 1;
            int index = 0;
            while (index<count && page.items.begin[index] != item) ++index;
            for (int remaining=count; remaining; --remaining) {
                index = (index+direction+count)%count;
                if (Selectable(page.items.begin[index])) {
                    page.selected = page.items.begin[index];
                    PlaySound();
                    return;
                }
            }
            return;
        }
        if (item->hook >= Binding && extension->mode == 2) {
            const int action = RowAction(*extension, unsigned(item->hook - Binding));
            if (action < 0) return;
            if (trigger == 0 || trigger == 1) { bindingSlot ^= 1; PlaySound(); return; }
            if (trigger == 4) {
                captureAction = Act(action); captureSlot = bindingSlot; captureOwner = extension;
                captureFrames = 0; capture.start(); PlaySound();
            }
            return;
        }
        if (IsAction(item->hook)) {
            if (trigger == 4) {
                if (item->hook == DisplayOptions) OpenSubmenu(*extension,1);
                else if (item->hook == FixPage) OpenSubmenu(*extension,5);
                else if (item->hook == LightsPage) OpenSubmenu(*extension,4);
                else if (item->hook == ControlOptions) OpenSubmenu(*extension,3);
                else if (item->hook == Bindings) { extension->bindingPage = 0; OpenSubmenu(*extension,2); }
                else if (item->hook == NextBindings) {
                    extension->bindingPage = (extension->bindingPage+1)%BindingPages();
                    OpenSubmenu(*extension,2);
                    // Stay on "Next page" while paging.
                    for (auto p=page.items.begin; p!=page.items.end; ++p)
                        if (*p && (*p)->hook == NextBindings) page.selected = *p;
                } else if (item->hook == ResetBindings) bindings.Reset();
                else if (item->hook == ApplyPreset) {
                    // Reopen the page so every widget shows the applied values.
                    if (ApplyPresetValues(selectedPreset, confirmPreset)) OpenSubmenu(*extension,5);
                }
                else if (item->hook == FixDefaults) {
                    if (ApplyPresetValues(recommendedPreset, confirmDefaults)) OpenSubmenu(*extension,5);
                }
                else if (item->hook == LightsDefaults) {
                    // Restore defaults requests the recommended lights setting.
                    if (!ReadLights() || requestedLights) confirmLightsDefaults = false;
                    else if (!confirmLightsDefaults) { lightsWriteRefused = false; confirmLightsDefaults = true; }
                    else { RequestLights(true); confirmLightsDefaults = false; }
                    RefreshLights(*extension);
                }
                else if (item->hook == Back) OpenSubmenu(*extension, ParentMode(extension->mode));
                PlaySound();
                return;
            }
            if (trigger != 5) return;
            for (auto p=extension->original.begin; p!=extension->original.end; ++p)
                if (Selectable(*p)) { inputTrigger.call(*p,trigger); return; }
            return;
        }
        if (item->hook >= HudSize && item->hook < OptionEnd) {
            if (trigger == 0 || trigger == 1 || trigger == 4) {
                const bool lighting = item->hook == VehicleLights;
                if (lighting) {
                    if (extension->mode != 4 || item != &extension->widgets[item->hook - HudSize].item) return;
                    if (!ReadLights()) { lightsWriteRefused = true; RefreshLights(*extension); return; }
                    RefreshLights(*extension);
                }
                reinterpret_cast<void (*)(MenuItem*)>(item->vtable[trigger == 0 ? 13 : 12])(item);
                if (lighting) RefreshLights(*extension);
                return;
            }
            if (trigger == 5) {
                // Use the original page's Back action, including tab selection.
                for (auto p=extension->original.begin; p!=extension->original.end; ++p)
                    if (*p && (*p)->hook == 14) { inputTrigger.call(*p, trigger); return; }
            }
            return;
        }
    }
    inputTrigger.call(item, trigger);
}
// Runs once per drawn menu frame: the capture state machine and Delete to
// clear the selected slot.
void UpdateBindingsPage(PageExtension& extension) {
    if (captureOwner && captureOwner != &extension) return;
    if (capture.active()) {
        // A controller-only player can always get out: give up after ~10 s.
        if (++captureFrames > 600) { CancelCapture(); return; }
        if (!capture.update(input)) return;
        if (capture.cleared) bindings.clear(captureAction, captureSlot);
        else if (!capture.cancelled && capture.result) bindings.bind(captureAction, captureSlot, capture.result, game);
        CancelCapture();
        return;
    }
    auto* selected = extension.page->selected;
    if (selected && selected->hook >= Binding && selected->hook < Binding + int(bindingsPerPage) &&
        input.pressed(console::stories::code::del)) {
        const int action = RowAction(extension, unsigned(selected->hook - Binding));
        if (action >= 0) bindings.clear(Act(action), bindingSlot);
    }
}
void SaveSettings() {
    const bool bindingsSaved = SaveBindings();
    if (!dirty && bindingsSaved) return;
    // The host only accepts requests inside the module image (not a game stack).
    static PCSX2FIniRequest request;
    request = {};
    request.size = sizeof(request); request.version = 1;
    request.operation = PCSX2F_SETTINGS_WRITE; request.count = 7;
    auto& entries = request.entries;
    std::strcpy(entries[0].section, "MAIN"); std::strcpy(entries[0].key, "HudScale");
    console::format(entries[0].value, "", settings.hudScale, 2);
    std::strcpy(entries[1].section, "MAIN"); std::strcpy(entries[1].key, "UnthrottleEmuDuringLoading");
    std::strcpy(entries[1].value, settings.unthrottle ? "1" : "0");
    std::strcpy(entries[2].section, "MAIN"); std::strcpy(entries[2].key, "MouseSensitivity");
    console::format(entries[2].value, "", settings.mouseSensitivity / 0.002f, 2);
    std::strcpy(entries[3].section, "MAIN"); std::strcpy(entries[3].key, "InvertMouseY");
    std::strcpy(entries[3].value, settings.invertMouse ? "1" : "0");
    std::strcpy(entries[4].section, "MAIN"); std::strcpy(entries[4].key, "Enable60FPS");
    std::strcpy(entries[4].value, settings.sixtyFPS ? "1" : "0");
    std::strcpy(entries[5].section, "MAIN"); std::strcpy(entries[5].key, "CutsceneBorders");
    entries[5].value[0] = char('0' + (settings.cutsceneBorders & 3)); entries[5].value[1] = 0;
    std::strcpy(entries[6].section, "CONTROLS"); std::strcpy(entries[6].key, "PCControlScheme");
    std::strcpy(entries[6].value, settings.pcControls ? "1" : "0");
    const auto status = console::ps2settings::request(request);
    if (status == PCSX2F_SETTINGS_OK && bindingsSaved) { dirty = false; OSDText[1][0] = 0; }
    else std::strcpy(OSDText[1], status == PCSX2F_SETTINGS_UNSUPPORTED
        ? "Menu settings apply this session. Update the fork and injector to save them to the CFG."
        : "Could not save plugin settings to the CFG. Changes still apply this session.");
}
void CloseMenu(void* menu, bool immediate) {
    // Closing the pause menu from a submenu would leave the page's item list
    // without its XML default item. The next SetActivePage looks that name up
    // (cMenuItems::SetCurrentItemByNameTrigger) and calls through a null item.
    RestoreAll();
    closeMenu.call(menu, immediate);
    SaveSettings();
}
uint64_t Draw(void* menu) {
    DrawScope scope(settings.widescreen ? DrawMode::Center : DrawMode::None);
    const auto begin = *reinterpret_cast<MenuPage***>(static_cast<char*>(menu)+4);
    const auto end = *reinterpret_cast<MenuPage***>(static_cast<char*>(menu)+8);
    if (begin && end >= begin && end-begin <= 64)
        for (auto p=begin; p!=end; ++p) Attach(*p);
    textPage = nullptr;
    for (auto& extension : pages) if (extension.page && extension.mode == 2) { textPage = &extension; break; }
    if (textPage) UpdateBindingsPage(*textPage);
    else if (capture.active()) CancelCapture();
    renderedBindings=0;
    const auto result = drawMenu.call(menu);
    if(textPage) for(unsigned i=0;i<bindingsPerPage;++i) if(renderedBindings&(1u<<i)) {
        const int action=RowAction(*textPage,i);
        if(action<0)continue;
        auto& hint=textPage->hints[i].item;
        const bool focused=textPage->page->selected==&hint;
        float x=248.0f;
        for(unsigned slot=0;slot<2;++slot) {
            const uint8_t key=bindings.keys[action][slot];
            if(!key)continue;
            Color color{255,255,255,hint.normal.alpha};
            // The slot that Enter rebinds is bright; the other one is dimmed.
            if(focused && slot!=bindingSlot)color.alpha=uint8_t(color.alpha/3);
            x+=DrawKeyIcon(key,x,float(hint.y),14.0f,color)+6.0f;
        }
    }
    textPage = nullptr;
    return result;
}
}
void InstallMenu() {
    ReloadMenuLanguage();
    InstallButtonIcons();
    drawMenu = safetymips::create_inline_game(0x3B53F8, Draw);
    parsePage = safetymips::create_inline_game(0x3CE550, ParsePage);
    destroyPage = safetymips::create_inline_game(0x3CDC58, DestroyPage);
    inputTrigger = safetymips::create_inline_game(0x411E70, InputTrigger);
    getValue = safetymips::create_inline_game(0x410A40, GetValue);
    setValue = safetymips::create_inline_game(0x4109D8, SetValue);
    getText = safetymips::create_inline_game(0x3FB1A8, GetText);
    closeMenu = safetymips::create_inline_game(0x3B4FD8, CloseMenu);
    setCurrentItem = safetymips::create_inline_game(0x3CE330, SetCurrentItem);
}
}
