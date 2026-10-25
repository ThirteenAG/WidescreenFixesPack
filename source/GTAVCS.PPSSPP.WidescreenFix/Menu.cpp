#include "Game.hpp"
#include "../Shared/Console/PortableIni.hpp"
#include "../Shared/Console/RestartCfgPeer.hpp"
#include "../Shared/Console/Text.hpp"
#include <cstddef>
#include <cstring>
#include <cmath>

// Native Display menu extension. The game's own slider/multistate widgets are
// copied into bounded module storage; the game's navigation, fonts, highlights,
// sliders and translated on/off strings draw them. Original lists are restored
// before the game destroys or reloads the frontend pages.
// One "Plugins" row opens the list of plugins: this fix first, then the loaded
// siblings. Each plugin page has its own rows and Restore Defaults.
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

enum Option : int { FrameRate = 0x100, MoreOptions, HudSize, RadarSize, MultiplayerRadar,
    FieldOfView, CutsceneBorders, CutsceneFov, FastLoading, Preset, ApplyPreset,
    DisplayPage, GamePage, LightsPage, VehicleLights, LightsDefaults, IconsPage, IconSet, IconsDefaults,
    FixPage, FixDefaults, SkipIntro, Back, OptionEnd };
constexpr unsigned optionCount = OptionEnd - FrameRate;
constexpr char keys[optionCount][20] = {"WF_FRAME_RATE", "WF_MORE_OPTIONS", "WF_HUD_SIZE", "WF_RADAR_SIZE",
    "WF_MP_RADAR", "WF_FOV", "WF_CUT_BORDERS", "WF_CUT_FOV", "WF_FAST_LOADING", "WF_PRESET",
    "WF_APPLY_PRESET", "WF_DISPLAY", "WF_GAME", "WF_LIGHTS_PAGE", "WF_LIGHTS", "WF_LIGHTS_DEFAULTS",
    "WF_ICONS_PAGE", "WF_ICONS", "WF_ICONS_DEFAULTS", "WF_FIX_PAGE", "WF_FIX_DEFAULTS", "WF_SKIP_INTRO", "WF_BACK"};
constexpr char16_t captions[optionCount][28] = {u"FRAME RATE:", u"PLUGINS", u"HUD SIZE:",
    u"RADAR SIZE:", u"MULTIPLAYER RADAR SIZE:", u"FIELD OF VIEW:", u"CUTSCENE BORDERS:",
    u"ORIGINAL CUTSCENE FOV:", u"FAST LOADING:", u"PRESET:", u"APPLY PRESET", u"DISPLAY AND CAMERA",
    u"GAME", u"IMPROVED VEHICLE LIGHTS", u"LIGHTS (RESTART):", u"RESTORE DEFAULTS", u"GAMEPAD ICONS",
    u"ICON SET (RESTART):", u"RESTORE DEFAULTS", u"WIDESCREEN FIX", u"RESTORE DEFAULTS", u"SKIP INTRO (RESTART):", u""};
constexpr char fpsKeys[2][12] = {"WF_FPS_30", "WF_FPS_60"};
constexpr char16_t fpsText[2][4] = {u"30", u"60"};
constexpr char onOff[2][16] = {"WF_BOOL_OFF", "WF_BOOL_ON"};
constexpr char bordersKeys[4][16] = {"WF_CB_OFF", "WF_CB_LETTERBOX", "WF_CB_PILLARBOX", "WF_CB_BOTH"};
constexpr char16_t bordersText[4][12] = {u"OFF", u"LETTERBOX", u"PILLARBOX", u"BOTH"};
constexpr char16_t onOffText[2][4] = {u"OFF", u"ON"};
constexpr char presetKeys[4][16] = {"WF_CUSTOM", "WF_VANILLA", "WF_RECOMMENDED", "WF_ENHANCED"};
constexpr char16_t presetText[4][16] = {u"CUSTOM", u"VANILLA", u"RECOMMENDED", u"ENHANCED"};
constexpr char lightsValueKeys[2][16] = {"WF_LIGHTS_OFF", "WF_LIGHTS_ON"};
constexpr char loadingKeys[3][16] = {"WF_BOOL_OFF", "WF_LOAD", "WF_LOAD_FADES"};
constexpr char16_t loadingText[2][20] = {u"LOADING", u"LOADING AND FADES"};
constexpr char iconTokens[8][4] = {"PSP", "360", "XBO", "PS3", "PS4", "PS5", "SWI", "DEK"};
constexpr char iconKeys[8][16] = {"WF_ICON_PSP", "WF_ICON_360", "WF_ICON_XBO", "WF_ICON_PS3", "WF_ICON_PS4", "WF_ICON_PS5", "WF_ICON_SWI", "WF_ICON_DEK"};
constexpr char16_t iconText[8][16] = {u"ORIGINAL PSP", u"XBOX 360", u"XBOX ONE", u"PLAYSTATION 3", u"PLAYSTATION 4", u"PLAYSTATION 5", u"NINTENDO SWITCH", u"STEAM DECK"};
constexpr unsigned maxItems = 24;
bool Slider(int option) { return option == HudSize || option == RadarSize || option == MultiplayerRadar || option == FieldOfView; }
bool Action(int option) { return option == MoreOptions || option == DisplayPage || option == GamePage || option == LightsPage ||
    option == IconsPage || option == FixPage || option == ApplyPreset || option == FixDefaults || option == LightsDefaults ||
    option == IconsDefaults || option == Back; }
// Pages: 0 native rows, 1 plugin list, 6 this fix, 2/3 its display and game
// pages, 4 vehicle lights, 5 gamepad icons.
enum Mode : unsigned { Native, Plugins, DisplayMode, GameMode, LightsMode, IconsMode, FixMode };
unsigned Parent(unsigned mode) {
    return mode == DisplayMode || mode == GameMode ? FixMode : mode == Plugins ? Native : Plugins;
}

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
    CharList values[optionCount][8]{};
    unsigned mode = 0;
};
Extension extension;
bool dirty = false;
int selectedPreset = 0;
bool confirmPreset = false, confirmDefaults = false;
// A known sibling's requested restart value. Never call its code or read its
// private render/cache state; persisted CFG readback is not a live lights value.
console::portable::RestartCfgPeer lightsPeer("GTAVCS.ImVehLM",
    "ms0:/PSP/PLUGINS/GTAVCS.PPSSPP.ImVehLM/GTAVCS.PPSSPP.ImVehLM.cfg", "MAIN", "VehicleLights");
bool lightsAvailable = false, requestedLights = false, confirmLightsDefaults = false, lightsWriteRefused = false;
unsigned lightsPollDelay = 0;
bool ReadLights() {
    lightsPollDelay = 15;
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
    if (saved) {
        requestedLights = requested;
        confirmLightsDefaults = false;
    } else ReadLights();
    return saved;
}
using IconCodec = console::portable::RestartChoiceValue<8, 3>;
console::portable::RestartCfgChoicePeer<IconCodec> iconsPeer("GTAVCS.GamepadIcons",
    "ms0:/PSP/PLUGINS/GTAVCS.PPSSPP.GamepadIcons/GTAVCS.PPSSPP.GamepadIcons.cfg", "MAIN", "GamepadIcons", IconCodec{iconTokens});
bool iconsAvailable = false, confirmIconsDefaults = false, iconsWriteRefused = false;
unsigned requestedIcons = 0, iconsPollDelay = 0;
bool ReadIcons() {
    iconsPollDelay = 15;
    unsigned requested = 0;
    const bool available = iconsPeer.Read(requested);
    if (!available || available != iconsAvailable || requested != requestedIcons) confirmIconsDefaults = false;
    iconsAvailable = available;
    if (available) requestedIcons = requested;
    return available;
}
bool RequestIcons(unsigned requested) {
    if (!ReadIcons()) { iconsWriteRefused = true; return false; }
    const bool saved = iconsPeer.Write(requested);
    iconsWriteRefused = !saved;
    if (saved) { requestedIcons = requested; confirmIconsDefaults = false; }
    else ReadIcons();
    return saved;
}
// The descriptor's presets: 1 vanilla, 2 recommended (the defaults), 3 enhanced (0 = custom).
struct PresetValues { int fps, unthrottleMode; float hud, radar, multiplayerRadar, fov; bool restoreCutsceneFov; int cutsceneBorders; bool skipIntro; };
constexpr PresetValues presetValues[4] = {{},
    {0, 0, 1.0f, 1.0f, 1.0f, 1.0f, true, 1, false},
    {0, 1, 0.75f, 0.83f, 0.63f, 1.0f, true, 3, true},
    {1, 1, 0.75f, 0.83f, 0.63f, 1.0f, true, 3, true}};
constexpr int recommendedPreset = 2;
bool PresetMatches(int preset) {
    if (preset < 1 || preset > 3) return false;
    const auto& p = presetValues[preset];
    return (settings.fps != 0) == (p.fps != 0) && (settings.unthrottle ? settings.unthrottleMode : 0) == p.unthrottleMode &&
        std::fabs(Drawing::settings.hud - p.hud) < 0.001f &&
        std::fabs(Drawing::settings.radar - p.radar) < 0.001f &&
        std::fabs(Drawing::settings.multiplayerRadar - p.multiplayerRadar) < 0.001f &&
        std::fabs(settings.fov - p.fov) < 0.001f && settings.restoreCutsceneFov == p.restoreCutsceneFov &&
        settings.cutsceneBorders == p.cutsceneBorders && settings.skipIntro == p.skipIntro;
}
int MatchingPreset() { return PresetMatches(2) ? 2 : PresetMatches(1) ? 1 : PresetMatches(3) ? 3 : 0; }

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
    case MultiplayerRadar: return Percent(Drawing::settings.multiplayerRadar);
    case FieldOfView: return Percent(settings.fov);
    case CutsceneBorders: return settings.cutsceneBorders;
    case CutsceneFov: return settings.restoreCutsceneFov;
    case FastLoading: return settings.unthrottle ? settings.unthrottleMode : 0;
    case SkipIntro: return settings.skipIntro;
    case Preset: return selectedPreset;
    case VehicleLights: return requestedLights;
    case IconSet: return int(requestedIcons);
    default: return 0;
    }
}
void SetOption(int option, int value) {
    const float scale = console::bounded(value * 0.01f, 0.1f, 2.0f, 1.0f);
    switch (option) {
    case FrameRate: settings.fps = value != 0; ApplyFrameRate(); break;
    case HudSize: Drawing::settings.hud = scale; break;
    case RadarSize: Drawing::settings.radar = scale; break;
    case MultiplayerRadar: Drawing::settings.multiplayerRadar = scale; break;
    case FieldOfView: settings.SetFovRequest(value * 0.01f); break;
    case CutsceneBorders: settings.cutsceneBorders = value >= 0 && value <= 3 ? value : 3; break;
    case CutsceneFov: settings.restoreCutsceneFov = value != 0; break;
    case SkipIntro: settings.skipIntro = value != 0; break;
    case FastLoading: settings.unthrottleMode = value; settings.unthrottle = value != 0; if (!settings.unthrottle) console::portable::Unthrottle(false); break;
    case Preset: selectedPreset = value; confirmPreset = false; return;
    case VehicleLights:
        if (value == 0 || value == 1) RequestLights(value != 0);
        else lightsWriteRefused = true;
        confirmLightsDefaults = false;
        return;
    case IconSet:
        if (value >= 0 && value < 8) RequestIcons(unsigned(value));
        else iconsWriteRefused = true;
        confirmIconsDefaults = false;
        return;
    default: return;
    }
    selectedPreset = MatchingPreset();
    confirmPreset = false; confirmDefaults = false;
    dirty = true;
}
int GetHookData(MenuItem* item, void* text) {
    if (item->hook >= FrameRate && item->hook < OptionEnd) return Value(item->hook);
    return getHookHook.call<int>(item, text);
}
void SetHookData(MenuItem* item, int value) {
    if (item->hook == VehicleLights) {
        if (!Owned(item) || !extension.page || extension.mode != LightsMode || extension.page->selected != item || !InPage(item)) return;
    }
    if (item->hook == IconSet) {
        if (!Owned(item) || !extension.page || extension.mode != IconsMode || extension.page->selected != item || !InPage(item)) return;
    }
    if (item->hook >= FrameRate && item->hook < OptionEnd) { SetOption(item->hook, value); return; }
    setHookHook.call<void>(item, value);
}
const uint16_t* GetText(void* text, const char* key) {
    // Multistate widgets look up a copy of their value key; compare contents.
    if (settings.MenuLanguage() == TextLanguage::English && key && key[0] == 'W' && key[1] == 'F' && key[2] == '_') {
        for (unsigned i = 0; i < unsigned(Back - FrameRate); ++i)
            if (!std::strcmp(key, keys[i])) {
                if (i == ApplyPreset - FrameRate && confirmPreset) return reinterpret_cast<const uint16_t*>(u"CONFIRM PRESET");
                if (i == FixDefaults - FrameRate && confirmDefaults) return reinterpret_cast<const uint16_t*>(u"CONFIRM RESTORE DEFAULTS");
                if (i == LightsDefaults - FrameRate) {
                    if (!lightsAvailable) return reinterpret_cast<const uint16_t*>(u"UNAVAILABLE");
                    if (lightsWriteRefused) return reinterpret_cast<const uint16_t*>(u"WRITE REFUSED");
                    if (confirmLightsDefaults) return reinterpret_cast<const uint16_t*>(u"CONFIRM RESTORE DEFAULTS");
                }
                if (i == IconsDefaults - FrameRate) {
                    if (!iconsAvailable) return reinterpret_cast<const uint16_t*>(u"UNAVAILABLE");
                    if (iconsWriteRefused) return reinterpret_cast<const uint16_t*>(u"WRITE REFUSED");
                    if (confirmIconsDefaults) return reinterpret_cast<const uint16_t*>(u"CONFIRM RESTORE DEFAULTS");
                }
                return reinterpret_cast<const uint16_t*>(captions[i]);
            }
        for (unsigned i = 0; i < 2; ++i)
            if (!std::strcmp(key, onOff[i])) return reinterpret_cast<const uint16_t*>(onOffText[i]);
        for (unsigned i = 0; i < 4; ++i)
            if (!std::strcmp(key, bordersKeys[i])) return reinterpret_cast<const uint16_t*>(bordersText[i]);
        for (unsigned i = 0; i < 2; ++i)
            if (!std::strcmp(key, fpsKeys[i])) return reinterpret_cast<const uint16_t*>(fpsText[i]);
        for (unsigned i = 0; i < 4; ++i)
            if (!std::strcmp(key, presetKeys[i])) return reinterpret_cast<const uint16_t*>(presetText[i]);
        for (unsigned i = 0; i < 2; ++i)
            if (!std::strcmp(key, lightsValueKeys[i])) return reinterpret_cast<const uint16_t*>(lightsAvailable ? onOffText[i] : u"UNAVAILABLE");
        for (unsigned i = 0; i < 8; ++i)
            if (!std::strcmp(key, iconKeys[i])) return reinterpret_cast<const uint16_t*>(iconsAvailable ? iconText[i] : u"UNAVAILABLE");
        for (unsigned i = 1; i < 3; ++i)
            if (!std::strcmp(key, loadingKeys[i])) return reinterpret_cast<const uint16_t*>(loadingText[i - 1]);
        if (!std::strcmp(key, "WF_BACK")) return reinterpret_cast<const uint16_t*>(u"BACK");
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
void RefreshLights() { Refresh(extension.widgets[VehicleLights - FrameRate]); }
void RefreshIcons() { Refresh(extension.widgets[IconSet - FrameRate]); }
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
            widget.slider.minimum = option == FieldOfView ? 50 : 10;
            widget.slider.maximum = 200; widget.slider.steps = 5;
        } else if (Action(option)) {
            e.values[i][0] = Literal(keys[i]);
            widget.toggle.strings = {e.values[i], e.values[i] + 1, e.values[i] + 1};
            widget.toggle.count = 1;
            // Match native centred actions: one caption without a label.
            widget.item.render = 0; widget.item.x = 0; widget.item.width = 480; widget.item.horizontal = 1;
        } else {
            const unsigned count = option == IconSet ? 8 : option == Preset || option == CutsceneBorders ? 4 : option == FastLoading ? 3 : 2;
            for (unsigned j = 0; j < count; ++j)
                e.values[i][j] = Literal(option == Preset ? presetKeys[j] : option == IconSet ? iconKeys[j] :
                    option == VehicleLights ? lightsValueKeys[j] : option == FastLoading ? loadingKeys[j] :
                    option == FrameRate ? fpsKeys[j] : option == CutsceneBorders ? bordersKeys[j] : onOff[j]);
            widget.toggle.strings = {e.values[i], e.values[i] + count, e.values[i] + count};
            widget.toggle.count = uint8_t(count);
        }
        Refresh(widget);
    }
}
void Show(unsigned mode) {
    auto& e = extension;
    if (!e.page) return;
    // Reads happen at page/action boundaries, not inside native text getters.
    if ((mode == Plugins || mode == LightsMode) && !ReadLights() && mode == LightsMode) { lightsWriteRefused = true; return; }
    if ((mode == Plugins || mode == IconsMode) && !ReadIcons() && mode == IconsMode) { iconsWriteRefused = true; return; }
    auto& page = *e.page;
    if (mode && !e.mode) { ReloadTextLanguage(); e.mainSelection = page.selected; }
    if (mode == FixMode) selectedPreset = MatchingPreset();
    e.mode = mode; confirmPreset = false; confirmDefaults = false; confirmLightsDefaults = false; lightsWriteRefused = false;
    confirmIconsDefaults = false; iconsWriteRefused = false;
    if (!mode) { page.items = e.mainList; page.selected = e.mainSelection ? e.mainSelection : e.mainItems[0]; return; }
    unsigned count = 0;
    for (auto p = e.original.begin; p != e.original.end; ++p)
        if (*p && (*p)->hook == 0) e.subItems[count++] = *p;
    constexpr int plugins[] = {FixPage, LightsPage, IconsPage, Back};
    constexpr int fix[] = {Preset, ApplyPreset, DisplayPage, GamePage, FixDefaults, Back};
    constexpr int display[] = {HudSize, RadarSize, MultiplayerRadar, FieldOfView, CutsceneBorders, CutsceneFov, Back};
    constexpr int game[] = {FrameRate, FastLoading, SkipIntro, Back};
    constexpr int lights[] = {VehicleLights, LightsDefaults, Back};
    constexpr int icons[] = {IconSet, IconsDefaults, Back};
    const int* options = game;
    unsigned rows = 4;
    switch (mode) {
    case Plugins: options = plugins; rows = 4; break;
    case FixMode: options = fix; rows = 6; break;
    case DisplayMode: options = display; rows = 7; break;
    case LightsMode: options = lights; rows = 3; break;
    case IconsMode: options = icons; rows = 3; break;
    default: break;
    }
    unsigned visibleRows = 0;
    for (unsigned i = 0; i < rows; ++i) {
        if (options[i] == LightsPage && !lightsAvailable) continue;
        if (options[i] == IconsPage && !iconsAvailable) continue;
        auto& widget = e.widgets[options[i] - FrameRate];
        Refresh(widget); widget.item.y = 54 + int(visibleRows++) * 19;
        e.subItems[count++] = &widget.item;
    }
    e.subList = {e.subItems, e.subItems + count, e.subItems + maxItems};
    page.items = e.subList; page.selected = &e.widgets[options[0] - FrameRate].item;
}
// Press once to arm, again to apply; nothing happens when the values already match.
void ApplyPresetValues(int preset, bool& confirm) {
    if (preset < 1 || preset > 3 || PresetMatches(preset)) { confirm = false; return; }
    if (!confirm) { confirm = true; return; }
    const auto& p = presetValues[preset];
    settings.fps = p.fps; ApplyFrameRate();
    settings.unthrottleMode = p.unthrottleMode; settings.unthrottle = p.unthrottleMode != 0;
    if (!settings.unthrottle) console::portable::Unthrottle(false);
    Drawing::settings.hud = p.hud; Drawing::settings.radar = p.radar; Drawing::settings.multiplayerRadar = p.multiplayerRadar;
    settings.SetFovRequest(p.fov); settings.restoreCutsceneFov = p.restoreCutsceneFov; settings.cutsceneBorders = p.cutsceneBorders;
    settings.skipIntro = p.skipIntro;
    selectedPreset = preset;
    dirty = true; confirm = false;
    for (auto& widget : extension.widgets) Refresh(widget);
}
// The siblings' only setting has the same value in every preset, so their
// pages offer Restore Defaults instead of a preset row.
void RestoreLightsDefaults() {
    if (!ReadLights() || requestedLights) { confirmLightsDefaults = false; RefreshLights(); return; }
    if (!confirmLightsDefaults) { lightsWriteRefused = false; confirmLightsDefaults = true; return; }
    RequestLights(true);
    confirmLightsDefaults = false;
    RefreshLights();
}
void RestoreIconsDefaults() {
    if (!ReadIcons() || requestedIcons == 0) { confirmIconsDefaults = false; RefreshIcons(); return; }
    if (!confirmIconsDefaults) { iconsWriteRefused = false; confirmIconsDefaults = true; return; }
    RequestIcons(0);
    confirmIconsDefaults = false; RefreshIcons();
}
void Attach(MenuPage* page) {
    auto& e = extension;
    if (e.page || !page || !page->name.begin || std::strncmp(page->name.begin, "DISPLAY_PAGE", 12)) return;
    const auto count = page->items.end - page->items.begin;
    if (count <= 0 || count >= int(maxItems)) return;
    unsigned assets = 0;
    for (auto p = page->items.begin; p != page->items.end; ++p) if (*p && (*p)->hook == 0) ++assets;
    if (assets + 7 > maxItems) return;
    Widget *slider = nullptr, *toggle = nullptr;
    for (auto p = page->items.begin; p != page->items.end; ++p) if (*p) {
        if ((*p)->hook == 4) slider = reinterpret_cast<Widget*>(*p);   // Brightness
        if ((*p)->hook == 3) toggle = reinterpret_cast<Widget*>(*p);   // Subtitles (off/on)
    }
    if (!slider || !toggle || toggle->toggle.count != 2) return;
    e.page = page;
    e.original = page->items;
    e.originalSelection = page->selected;
    e.mode = 0; e.mainSelection = nullptr;
    Initialize(e, *slider, *toggle, toggle->item);
    unsigned main = 0, sub = 0;
    for (unsigned i = 0; i < unsigned(count); ++i) {
        auto* item = page->items.begin[i];
        e.originalY[i] = item ? item->y : 0;
        e.mainItems[main++] = item;
        // The page heading and other non-option assets stay in the submenu.
        if (item && item->hook == 0) e.subItems[sub++] = item;
    }
    e.mainItems[main++] = &e.widgets[MoreOptions - FrameRate].item;
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
    e.page = nullptr; e.mode = 0;
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
    if (trigger == 2 || trigger == 3) {
        confirmPreset = false; confirmDefaults = false; confirmLightsDefaults = false; confirmIconsDefaults = false;
        Navigate(page, item, trigger == 2 ? -1 : 1); return;
    }
    if (!Owned(item)) { inputHook.call<void>(item, trigger); return; }
    const int option = item->hook;
    if (option != ApplyPreset || trigger != 4) confirmPreset = false;
    if (option != FixDefaults || trigger != 4) confirmDefaults = false;
    if (option != LightsDefaults || trigger != 4) confirmLightsDefaults = false;
    if (option != IconsDefaults || trigger != 4) confirmIconsDefaults = false;
    if (trigger == 5) {
        if (e.mode) { Show(Parent(e.mode)); playSound(reinterpret_cast<void*>(dmAudio), 0xC5, 0); return; }
        // Leave the page through a native row's cancel action.
        for (auto p = e.original.begin; p != e.original.end; ++p)
            if (Selectable(*p)) { inputHook.call<void>(*p, trigger); return; }
        return;
    }
    if (Action(option)) {
        if (trigger == 4) {
            switch (option) {
            case ApplyPreset: ApplyPresetValues(selectedPreset, confirmPreset); break;
            case FixDefaults: ApplyPresetValues(recommendedPreset, confirmDefaults); break;
            case LightsDefaults: RestoreLightsDefaults(); break;
            case IconsDefaults: RestoreIconsDefaults(); break;
            case Back: Show(Parent(e.mode)); break;
            case DisplayPage: Show(DisplayMode); break;
            case GamePage: Show(GameMode); break;
            case LightsPage: Show(LightsMode); break;
            case IconsPage: Show(IconsMode); break;
            case FixPage: Show(FixMode); break;
            default: Show(Plugins); break;
            }
            playSound(reinterpret_cast<void*>(dmAudio), 0xC5, 0);
        }
        return;
    }
    if (trigger == 0 || trigger == 1 || (trigger == 4 && !Slider(option))) {
        if (option == VehicleLights) {
            if (!ReadLights()) { lightsWriteRefused = true; RefreshLights(); return; }
            // Cycle from the fresh requested CFG state, not a cached index.
            Refresh(e.widgets[option - FrameRate]);
        }
        if (option == IconSet) {
            if (!ReadIcons()) { iconsWriteRefused = true; RefreshIcons(); return; }
            Refresh(e.widgets[option - FrameRate]);
        }
        // Native OnPrevMenuState/OnNextMenuState update the widget and call SetHookData.
        Virtual(item, trigger == 0 ? 12 : 11);
        if (option == VehicleLights) RefreshLights();
        if (option == IconSet) RefreshIcons();
        playSound(reinterpret_cast<void*>(dmAudio), 0xC5, 0);
    }
}
void Save() {
    if (!dirty) return;
    using console::portable::IniValue;
    IniValue values[9] = {
        {"MAIN", "Enable60FPS", {}}, {"MAIN", "UnthrottleEmuDuringLoading", {}}, {"MPRADAR", "RadarScale", {}},
        {"MAIN", "HudScale", {}}, {"RADAR", "RadarScale", {}}, {"MAIN", "FOVFactor", {}},
        {"FOV", "RestoreCutsceneFOV", {}}, {"MAIN", "CutsceneBorders", {}}, {"MAIN", "SkipIntro", {}}};
    std::strcpy(values[0].value, settings.fps ? "1" : "0");
    // A configured value of 2 (also fast-forward fades) is kept while enabled.
    std::strcpy(values[1].value, !settings.unthrottle ? "0" : settings.unthrottleMode == 2 ? "2" : "1");
    console::format(values[2].value, "", Drawing::settings.multiplayerRadar, 2);
    console::format(values[3].value, "", Drawing::settings.hud, 2);
    console::format(values[4].value, "", Drawing::settings.radar, 2);
    console::format(values[5].value, "", settings.fov, 2);
    std::strcpy(values[6].value, settings.restoreCutsceneFov ? "1" : "0");
    values[7].value[0] = char('0' + (settings.cutsceneBorders & 3)); values[7].value[1] = 0;
    std::strcpy(values[8].value, settings.skipIntro ? "1" : "0");
    static console::portable::IniWriter<> writer;
    const bool saved = writer.update(cfgPath, values, 9);
#ifndef NDEBUG
    logger.WriteF("Menu settings %s", saved ? "saved" : "could not be saved");
#endif
    if (saved) dirty = false;
}
void Close(void* menu, bool immediate) {
    if (extension.page && extension.mode) Show(Native);
    closeHook.call<void>(menu, immediate);
    Save();
}
// cMenuItems::SetCurrentItemByNameTrigger looks the page's XML default item up
// by name and calls through the result without a null check (PerformON,
// SetActivePage). The submenu list lacks that item, so whatever path re-enters
// the page (including a menu shutdown that bypasses Close), return to the
// main list first.
void SelectDefaultItem(MenuPage* page) {
    if (page && page == extension.page && extension.mode) Show(Native);
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
    if (extension.page && extension.mode) {
        const int8_t screen = *reinterpret_cast<int8_t*>(manager + 0x1C);
        const bool tabs = *reinterpret_cast<uint8_t*>(manager + 0x1E) != 0;
        if (tabs || screen < 0 || screen >= end - begin || begin[screen] != extension.page) Show(Native);
    }
    // A disappearing/replaced peer changes only cached value text, never the
    // native row pool under drawing. Re-entry rebuilds the scoped root link.
    if (extension.page && (extension.mode == Plugins || extension.mode == LightsMode)) {
        if (lightsPollDelay) --lightsPollDelay;
        else {
            const bool available = lightsAvailable, requested = requestedLights;
            ReadLights();
            if (available != lightsAvailable || requested != requestedLights) RefreshLights();
        }
    }
    if (extension.page && (extension.mode == Plugins || extension.mode == IconsMode)) {
        if (iconsPollDelay) --iconsPollDelay;
        else {
            const bool available = iconsAvailable; const unsigned requested = requestedIcons;
            ReadIcons();
            if (available != iconsAvailable || requested != requestedIcons) RefreshIcons();
        }
    }
    Drawing::Scope scope(Anchor::Center);
    drawHook.call<void>(menu);
}
}
void InstallMenu() {
    (void)lightsPeer.Initialize();
    (void)iconsPeer.Initialize();
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
