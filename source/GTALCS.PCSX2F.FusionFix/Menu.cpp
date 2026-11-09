#include "Controls.hpp"
#include "ButtonIcons.hpp"
#include "../Shared/Console/Text.hpp"
#include <cstring>
#include "Settings.hpp"
#include "../Shared/Console/PS2Settings.hpp"
extern "C" {
#include "../../external/injector/include/ps2/plugin_settings.h"
}

// Native frontend rows for GTA LCS PS2.
//
// The PS2 CMenuManager (0x634610) describes every frontend screen with a fixed
// table of 15 entries (0x393A30, 278 bytes per screen), like the PSP version:
// {action, GXT key, save slot, target screen, x, y, alignment}. Plugin rows use
// action ids the game's switches ignore, so native navigation, fonts,
// highlights and sounds handle them; hooks supply captions, values, input and
// the key artwork. Submenus rewrite the entries of their screen in place.
// Every way out (Back, leaving the tab, closing the menu) writes the native
// table back, and row indices always stay inside the written rows.
namespace lcs {
namespace {
struct Entry {
    int16_t action;
    char name[8];
    uint8_t saveSlot, target;
    int16_t x, y;
    uint8_t align, padding;
};
static_assert(sizeof(Entry) == 18);
struct Screen { char name[8]; Entry entries[15]; };
static_assert(sizeof(Screen) == 278);
constexpr uintptr_t screens = 0x393A30;
constexpr unsigned displayScreen = 5, controlsScreen = 3;
constexpr uintptr_t screenOffset = 0x588, rowOffset = 0x58C, editOffset = 0x590, inPageOffset = 0x5B4;

enum Option : int16_t {
    FrameRate = 0x60, MoreOptions, HudSize, CutsceneBorders, FastLoading, PcMenu, PcControls,
    MouseSensitivity, InvertMouse, BindingsMenu, NextPage, ResetBindings, Back, Blur, Preset, ApplyPreset,
    FieldOfView, HudAspect, SkipIntro, GamepadIcons, OptionEnd,
    BindingRow = 0x76, BindingEnd = 0x80,
    // Rows of the plugin list and the fix's page.
    FixOptions = 0x80, FixDefaults, VehicleLights, PluginEnd
};
constexpr unsigned bindingsPerPage = BindingEnd - BindingRow;
constexpr unsigned optionCount = OptionEnd - FrameRate;
static_assert(OptionEnd <= BindingRow && BindingEnd <= FixOptions);
constexpr char keys[optionCount][8] = {"WF_FPS", "WF_MORE", "WF_HUD", "WF_CBOR", "WF_LOAD", "WF_PCM",
    "WF_PC", "WF_SENS", "WF_INV", "WF_BIND", "WF_NEXT", "WF_RST", "WF_BACK", "WF_BLUR", "WF_PRE", "WF_APLY",
    "WF_FOV", "WF_HUDA", "WF_SKIP", "WF_ICON"};
constexpr char captions[optionCount][28] = {"FRAME RATE:", "PLUGINS", "HUD SIZE", "CUTSCENE BORDERS",
    "FAST LOADING", "PC CONTROLS", "KEYBOARD AND MOUSE", "MOUSE SENSITIVITY", "INVERT MOUSE Y", "KEY BINDINGS",
    "NEXT PAGE", "RESET TO DEFAULTS", "", "BLUR EFFECT", "PRESET", "APPLY PRESET", "FIELD OF VIEW", "HUD ASPECT RATIO",
    "SKIP INTRO", "BUTTON ICONS"};
constexpr unsigned pluginRowCount = PluginEnd - FixOptions;
constexpr char pluginKeys[pluginRowCount][8] = {"WF_FIX", "WF_FDEF", "WF_VLIT"};
constexpr char pluginCaptions[pluginRowCount][28] = {"FUSION FIX", "RESTORE DEFAULTS", "VEHICLE LIGHTS"};
// One line of help per row, shown under the menu (empty: none).
constexpr const char* optionHelp[optionCount] = {
    "Runs the game at 60 frames per second.",
    "Settings of this fix and the other plugins.",
    "Size of the HUD.",
    "Black bars in cutscenes, top and bottom or at the sides.",
    "Runs the emulator at full speed while loading.",
    "Keyboard and mouse controls and key bindings.",
    "Mouse camera and aim, keys shown on screen.",
    "Speed of the mouse camera.",
    "Inverts the vertical mouse camera.",
    "Change the key of every action. Delete clears one.",
    "Shows the next page of actions.",
    "Puts every key back to its default.",
    "",
    "The original blur filter over the picture.",
    "Choose a preset, then apply it on the row below.",
    "Sets the values of the chosen preset.",
    "Widens the gameplay view. Cutscenes keep theirs.",
    "Keeps the HUD in a centered box on wide screens.",
    "Skips the logo movies when the game starts.",
    "Button pictures shown while you play with a pad.",
};
constexpr const char* pluginHelp[pluginRowCount] = {
    "Widescreen, HUD, camera, frame rate, lights and controls.",
    "Puts this page back to its recommended values.",
    "Lit headlights, tail, brake and reverse lights.",
};
constexpr const char* iconNames[] = {"AUTO", "XBOX", "PLAYSTATION 2", "NINTENDO"};
constexpr const char* hudAspectNames[] = {"AUTO", "4:3", "16:9", "21:9", "32:9"};
constexpr float hudAspectValues[] = {0.0f, 4.0f / 3.0f, 16.0f / 9.0f, 21.0f / 9.0f, 32.0f / 9.0f};
enum class MenuLanguage : uint8_t { Automatic, English };
struct MenuCatalog {
    const char (*options)[28];
    const char (*plugin)[28];
    const char* presets[3];
    const char* borders[4];
    const char *back, *on, *off, *custom, *unavailable, *failed, *confirm, *matching, *inUse, *press, *capture;
    const char* const* help;
    const char* const* pluginHelp;
    const char* restart;
};
const MenuCatalog english{captions, pluginCaptions, {"VANILLA", "RECOMMENDED", "ENHANCED"},
    {"OFF", "LETTERBOX", "PILLARBOX", "BOTH"},
    "BACK", "ON", "OFF", "CUSTOM", "UNAVAILABLE", "SAVE FAILED", "PRESS AGAIN", "CFG MATCH", "IN USE", "PRESS X", "PRESS A KEY (ESC CANCELS)",
    optionHelp, pluginHelp, "Restart the game to apply all settings."};
const MenuCatalog& ResolveCatalog(MenuLanguage language) {
    // Only English is supplied; an undetected native locale falls back to it.
    switch (language) {
    case MenuLanguage::Automatic: case MenuLanguage::English: return english;
    }
    return english;
}
struct MenuTextState { MenuLanguage requested = MenuLanguage::Automatic; const MenuCatalog* catalog = &english; } menuText;
const MenuCatalog& Catalog() { return *menuText.catalog; }
MenuLanguage ParseLanguage(const char* value) {
    if (!value) return MenuLanguage::Automatic;
    while (*value == ' ' || *value == '\t') ++value;
    if (value[0] != 'e' || value[1] != 'n') return MenuLanguage::Automatic;
    value += 2;
    const bool separated = *value == ' ' || *value == '\t';
    while (*value == ' ' || *value == '\t' || *value == '\r') ++value;
    return !*value || (separated && *value == ';') ? MenuLanguage::English : MenuLanguage::Automatic;
}
void LoadMenuLanguage() {
    // Static module-owned storage; read one own-CFG key without default writes.
    static PCSX2FIniRequest request;
    request = {}; request.size = sizeof(request); request.version = 1;
    request.operation = PCSX2F_SETTINGS_READ; request.count = 1;
    std::strcpy(request.entries[0].section, "MAIN"); std::strcpy(request.entries[0].key, "TextLanguage");
    const auto status = console::ps2settings::request(request);
    request.entries[0].value[sizeof(request.entries[0].value) - 1] = 0;
    menuText.requested = ParseLanguage(status == PCSX2F_SETTINGS_OK ? request.entries[0].value : config::values[9].value);
    menuText.catalog = &ResolveCatalog(menuText.requested);
}
bool Ours(int action) { return (action >= FrameRate && action < OptionEnd) || (action >= BindingRow && action < BindingEnd) || (action >= FixOptions && action < PluginEnd); }
bool Centered(int action) {
    return action == MoreOptions || action == PcMenu || action == BindingsMenu || action == NextPage ||
        action == ResetBindings || action == Back || action == FixOptions;
}
bool Slider(int action) { return action == HudSize || action == MouseSensitivity || action == FieldOfView; }

struct ActionList {
    Act list[console::stories::ActionCount]{};
    unsigned count = 0;
    ActionList() {
        for (unsigned i = 0; i < console::stories::ActionCount; ++i)
            if (console::stories::actions[i].games & game) list[count++] = Act(i);
    }
};
const ActionList available;
unsigned BindingPages() { return (available.count + bindingsPerPage - 1) / bindingsPerPage; }

struct Extension {
    unsigned index;
    Entry original[15]{};
    char originalName[8]{};
    bool attached = false;
    // 0 native rows, 1 plugin list (display) or PC controls (controls screen),
    // 2 bindings, 3 PC controls (display), 5 the fix's page
    unsigned mode = 0;
};
Extension display{displayScreen}, controls{controlsScreen};
unsigned bindingPage = 0, bindingSlot = 0, captureFrames = 0;
Act captureAction = Act::Forward;
unsigned captureSlot = 0;
bool dirty = false, menuWasActive = false;
int shownPreset = -1;
bool confirmPreset = false, confirmDefaults = false;
uint16_t valueText[16][48]{};
const uint16_t emptyText[1] = {0};

safetymips::GameInline<const uint16_t*(void*, const char*)> getText;
safetymips::GameInline<uint64_t(uintptr_t)> drawMenus;
safetymips::GameInline<uint64_t(uintptr_t, int)> adjust;
safetymips::GameInline<uint64_t(uintptr_t)> processInput;
safetymips::GameInline<uint64_t(uintptr_t)> controllerPicture;
safetymips::GameInline<uint64_t(uintptr_t, int)> controllerCallouts;
SafetyMipsMid valueHook, selectHook;

Screen& ScreenAt(unsigned index) { return reinterpret_cast<Screen*>(screens)[index]; }
uintptr_t Menu() { return address::menuManager; }
template<class T> T& Field(uintptr_t offset) { return *reinterpret_cast<T*>(Menu() + offset); }
int CurrentScreen() { return Field<int>(screenOffset); }

int RowAction(unsigned row) {
    const unsigned index = bindingPage * bindingsPerPage + row;
    return row < bindingsPerPage && index < available.count ? int(available.list[index]) : -1;
}
void CancelCapture() { capture = {}; }

Entry Row(int option, int16_t y, unsigned screen, const char* name = nullptr) {
    Entry entry{};
    entry.action = int16_t(option);
    const char* key = name ? name : option >= FixOptions && option < PluginEnd ? pluginKeys[option - FixOptions] : keys[option - FrameRate];
    for (unsigned i = 0; i + 1 < sizeof(entry.name) && key[i]; ++i) entry.name[i] = key[i];
    entry.target = uint8_t(screen);
    entry.x = 240; entry.y = y;
    entry.align = Centered(option) ? 3 : 2;
    return entry;
}
unsigned Count(const Entry* rows) { unsigned n = 0; while (n < 15 && rows[n].action) ++n; return n; }
int MatchingPreset();
void Write(Extension& extension) {
    Entry rows[15]{};
    unsigned used = 0;
    const auto add = [&](const Entry& entry) { if (used < 15) rows[used++] = entry; };
    const bool onDisplay = extension.index == displayScreen;
    if (extension.mode == 0) {
        const unsigned count = Count(extension.original);
        if (onDisplay) {
            // The fix sets the widescreen preference from the display. Keep
            // one Plugins entry; frame rate lives inside it.
            Entry position{}; bool hasPosition = false;
            for (unsigned i = 0; i < count; ++i) {
                const auto& entry = extension.original[i];
                if (entry.action == 29) { position = entry; hasPosition = true; continue; }
                if (entry.action == 12) continue;
                add(entry);
            }
            add(Row(MoreOptions, 0, displayScreen));
            if (hasPosition) add(position);
            int16_t y = 40;
            for (unsigned i = 0; i < used; ++i, y += 20) rows[i].y = y;
        } else {
            for (unsigned i = 0; i < count; ++i) add(extension.original[i]);
        }
    } else if (extension.mode == 1 && onDisplay) {
        // The plugin list.
        int16_t y = 40;
        add(Row(FixOptions, y, displayScreen)); y += 18;
        add(Row(Back, y, displayScreen));
    } else if (extension.mode == 5 && onDisplay) {
        // 15 rows, the last one above the help line (y 236).
        int16_t y = 36;
        for (int option : {Preset, ApplyPreset, FrameRate, FieldOfView, HudSize, HudAspect, CutsceneBorders, Blur, FastLoading,
                           SkipIntro, VehicleLights, GamepadIcons, PcMenu, FixDefaults, Back}) { add(Row(option, y, displayScreen)); y += 13; }
    } else if (extension.mode == 1 || extension.mode == 3) {
        int16_t y = 30;
        for (int option : {PcControls, MouseSensitivity, InvertMouse, BindingsMenu, Back}) { add(Row(option, y, extension.index)); y += 18; }
    } else {
        int16_t y = 26;
        static constexpr char names[bindingsPerPage][8] = {"WF_B0", "WF_B1", "WF_B2", "WF_B3", "WF_B4", "WF_B5", "WF_B6", "WF_B7", "WF_B8", "WF_B9"};
        for (unsigned i = 0; i < bindingsPerPage && RowAction(i) >= 0; ++i, y += 15)
            add(Row(BindingRow + int(i), y, extension.index, names[i]));
        if (BindingPages() > 1) { add(Row(NextPage, y, extension.index)); y += 15; }
        add(Row(ResetBindings, y, extension.index)); y += 15;
        add(Row(Back, y, extension.index));
    }
    std::memcpy(ScreenAt(extension.index).entries, rows, sizeof(rows));
    const char* header = extension.originalName;
    switch (extension.mode) {
    case 1: header = keys[(onDisplay ? MoreOptions : PcMenu) - FrameRate]; break;
    case 2: header = keys[BindingsMenu - FrameRate]; break;
    case 3: header = keys[PcMenu - FrameRate]; break;
    case 5: header = pluginKeys[0]; break;
    default: break;
    }
    std::memcpy(ScreenAt(extension.index).name, header, 8);
}
// Switches a screen's rows and keeps the cursor on a written row.
void Show(Extension& extension, unsigned mode, int select = -1) {
    if (!extension.attached) return;
    if (extension.index == displayScreen && extension.mode == 0 && mode == 1) LoadMenuLanguage();
    CancelCapture();
    if (mode == 5) shownPreset = MatchingPreset();
    confirmPreset = false;
    confirmDefaults = false;
    extension.mode = mode;
    Write(extension);
    bindingSlot = 0;
    if (CurrentScreen() != int(extension.index)) return;
    const auto& entries = ScreenAt(extension.index).entries;
    const unsigned count = Count(entries);
    int row = 0;
    for (unsigned i = 0; select >= 0 && i < count; ++i) if (entries[i].action == select) row = int(i);
    Field<int>(rowOffset) = count ? row : 0;
    Field<uint8_t>(editOffset) = 0;
}
// Back and Triangle: one page up, with the cursor on the row that opened it.
void ShowParent(Extension& extension) {
    const bool onDisplay = &extension == &display;
    switch (extension.mode) {
    case 2: Show(extension, onDisplay ? 3 : 1, BindingsMenu); break;
    case 3: Show(extension, 5, PcMenu); break;
    case 5: Show(extension, 1, FixOptions); break;
    default: Show(extension, 0, onDisplay ? MoreOptions : PcMenu); break;
    }
}
void RestoreAll() {
    CancelCapture();
    for (auto* extension : {&display, &controls})
        if (extension->attached && extension->mode) Show(*extension, 0);
}
Extension* Owner(int screen) {
    return screen == int(displayScreen) ? &display : screen == int(controlsScreen) ? &controls : nullptr;
}
void Attach(Extension& extension, const char* name, int16_t first) {
    auto& screen = ScreenAt(extension.index);
    // Validate the native table before taking it over.
    if (std::strncmp(screen.name, name, 8) || screen.entries[0].action != first) return;
    std::memcpy(extension.original, screen.entries, sizeof(extension.original));
    std::memcpy(extension.originalName, screen.name, sizeof(extension.originalName));
    if (extension.index == displayScreen && Count(extension.original) + 1 > 15) return;
    extension.attached = true;
    Write(extension);
}

int Value(int option) {
    switch (option) {
    case FrameRate: return settings.sixtyFPS;
    case HudSize: return int(settings.hudScale * 100.0f + 0.5f);
    case CutsceneBorders: return settings.cutsceneBorders;
    case FastLoading: return settings.unthrottle;
    case PcControls: return settings.pcControls;
    case MouseSensitivity: return int(settings.mouseSensitivity * 2000.0f + 0.5f); // 0.0005 steps
    case InvertMouse: return settings.invertMouse;
    case Blur: return !settings.disableBlur;
    case FieldOfView: return int(settings.fovFactor * 100.0f + 0.5f);
    case SkipIntro: return settings.skipIntro;
    case VehicleLights: return settings.vehicleLights;
    default: return 0;
    }
}
// The descriptor's presets: 0 vanilla, 1 recommended (the defaults), 2 enhanced. Only the
// settings whose values differ between them; the others (frame rate, FOV, HUD size and shape,
// controls, sensitivity, bindings) keep the player's choice.
struct PresetValues { bool unthrottle, disableBlur, skipIntro, vehicleLights; int cutsceneBorders, gamepadIcons; };
constexpr PresetValues presetValues[3] = {{false, false, false, false, 1, 2}, {true, true, true, true, 3, 0}, {true, true, true, true, 3, 0}};
constexpr int recommendedPreset = 1;
bool PresetMatches(int preset) {
    if (preset < 0 || preset > 2) return false;
    const auto& p = presetValues[preset];
    return settings.unthrottle == p.unthrottle && settings.disableBlur == p.disableBlur &&
        settings.skipIntro == p.skipIntro && settings.vehicleLights == p.vehicleLights && settings.cutsceneBorders == p.cutsceneBorders && settings.gamepadIcons == p.gamepadIcons;
}
int MatchingPreset() { return PresetMatches(1) ? 1 : PresetMatches(0) ? 0 : PresetMatches(2) ? 2 : -1; }
// Press once to arm, again to apply; nothing happens when the values already match.
void ApplyPresetValues(int preset, bool& confirm) {
    if (preset < 0 || preset > 2 || PresetMatches(preset)) { confirm = false; return; }
    if (!confirm) { confirm = true; return; }
    // The descriptor's preset values; controls, editor-only aspect and
    // compatibility settings stay intact.
    const auto& p = presetValues[preset];
    settings.unthrottle = p.unthrottle; settings.disableBlur = p.disableBlur;
    settings.skipIntro = p.skipIntro; settings.vehicleLights = p.vehicleLights; settings.cutsceneBorders = p.cutsceneBorders; settings.gamepadIcons = p.gamepadIcons;
    ApplyBlur(); dirty = true; confirm = false;
    shownPreset = preset;
}
int HudAspectIndex() {
    for (unsigned i = 0; i < sizeof(hudAspectValues) / sizeof(*hudAspectValues); ++i)
        if (std::fabs(settings.hudAspect - hudAspectValues[i]) < 0.001f) return int(i);
    return -1;
}
void Adjust(int option, int direction) {
    confirmPreset = false;
    confirmDefaults = false;
    if (option == Preset) { shownPreset = shownPreset < 0 ? (direction < 0 ? 2 : 0) : (shownPreset + (direction < 0 ? 2 : 1)) % 3; return; }
    if (option == ApplyPreset) return;
    switch (option) {
    case FrameRate: settings.sixtyFPS = !settings.sixtyFPS; ApplyFrameRate(); break;
    case HudSize: {
        int percent = Value(HudSize) + (direction > 0 ? 10 : -10);
        percent = (percent + 5) / 10 * 10;
        settings.hudScale = console::bounded(percent * 0.01f, 0.5f, 1.5f, 1.0f);
        break;
    }
    case CutsceneBorders: settings.cutsceneBorders = (settings.cutsceneBorders + (direction < 0 ? 3 : 1)) % 4; break;
    case FastLoading: settings.unthrottle = !settings.unthrottle; break;
    case PcControls: settings.pcControls = !settings.pcControls; break;
    case MouseSensitivity: {
        int step = Value(MouseSensitivity) + (direction > 0 ? 1 : -1);
        step = step < 1 ? 1 : step > 40 ? 40 : step;
        settings.mouseSensitivity = step * 0.0005f;
        break;
    }
    case InvertMouse: settings.invertMouse = !settings.invertMouse; break;
    case Blur: settings.disableBlur = !settings.disableBlur; ApplyBlur(); break;
    case FieldOfView: {
        int percent = Value(FieldOfView) + (direction > 0 ? 5 : -5);
        percent = (percent + 2) / 5 * 5;
        settings.fovFactor = console::bounded(percent * 0.01f, 0.5f, 2.0f, 1.0f);
        break;
    }
    case HudAspect: {
        constexpr int count = int(sizeof(hudAspectValues) / sizeof(*hudAspectValues));
        int index = HudAspectIndex();
        if (index >= 0) index = (index + (direction < 0 ? count - 1 : 1)) % count;
        else {
            // A custom ratio from the CFG moves to the nearest listed one in that direction.
            index = 0;
            if (direction > 0) { for (int i = count - 1; i > 0; --i) if (hudAspectValues[i] > settings.hudAspect) index = i; }
            else { for (int i = 1; i < count; ++i) if (hudAspectValues[i] < settings.hudAspect) index = i; }
        }
        settings.hudAspect = hudAspectValues[index];
        break;
    }
    case SkipIntro: settings.skipIntro = !settings.skipIntro; break;
    case VehicleLights: settings.vehicleLights = !settings.vehicleLights; break;
    case GamepadIcons:
        settings.gamepadIcons = (settings.gamepadIcons + (direction < 0 ? 3 : 1)) % 4;
        break;
    default: return;
    }
    dirty = true;
}
// Menu captions in capitals, like the native rows. (Spaces that vanished on
// wide screens came from the compressed glyph advance; see Hud.cpp.)
template<class Char> const uint16_t* Upper(const Char* text, uint16_t (&out)[48]) {
    unsigned used = 0;
    for (unsigned i = 0; text && text[i] && used + 1 < 48; ++i)
        out[used++] = uint16_t(text[i] >= 'a' && text[i] <= 'z' ? text[i] - 32 : uint8_t(text[i]));
    out[used] = 0;
    return out;
}
uint16_t label[48];
const uint16_t* GetText(void* text, const char* key) {
    if (key && key[0] == 'W' && key[1] == 'F' && key[2] == '_') {
        if (!std::strcmp(key, "WF_BACK")) {
            return Upper(Catalog().back, label);
        }
        for (unsigned i = 0; i < optionCount; ++i) if (!std::strcmp(key, keys[i])) return Upper(Catalog().options[i], label);
        for (unsigned i = 0; i < pluginRowCount; ++i) if (!std::strcmp(key, pluginKeys[i])) return Upper(Catalog().plugin[i], label);
        if (key[3] == 'B' && key[4] >= '0' && key[4] <= '9' && !key[5]) {
            const int action = RowAction(unsigned(key[4] - '0'));
            if (action >= 0) return Upper(console::stories::actions[action].name, label);
        }
        return emptyText;
    }
    if (const auto* rewritten = ButtonText(key)) return rewritten;
    return getText.call(text, key);
}
const uint16_t* Ascii(unsigned slot, const char* text) { return Upper(text, valueText[slot % 16]); }
// Text form of a binding row: "W / UP", with the slot Enter rebinds marked.
const uint16_t* BindingText(unsigned slot, Act action, bool focused) {
    auto& out = valueText[slot % 16];
    unsigned used = 0;
    const auto append = [&](const char* text) {
        for (; *text && used + 1 < 48; ++text) out[used++] = uint16_t(*text >= 'a' && *text <= 'z' ? *text - 32 : *text);
    };
    if (capture.active() && captureAction == action) {
        append(capture.state == console::stories::Capture::Arming ? "..." : Catalog().capture);
    } else {
        for (unsigned i = 0; i < 2; ++i) {
            if (i) append("  /  ");
            const bool marked = focused && i == bindingSlot;
            if (marked) append("[");
            char name[24];
            console::stories::KeyText(bindings.keys[action][i], name);
            append(name[0] ? name : "-");
            if (marked) append("]");
        }
    }
    out[used] = 0;
    return out;
}
bool IconRow(Act action, bool focused) {
    if (capture.active() && captureAction == action) return false;
    const auto* slots = bindings.keys[action];
    if (!slots[0] && !slots[1]) return false;
    if (focused && !slots[bindingSlot]) return false;
    for (unsigned i = 0; i < 2; ++i) if (slots[i] && KeyIconWidth(slots[i], 12.0f) == 0.0f) return false;
    return true;
}
// Native frontend slider (0x3423A0): eight segments at (x, y), value 0..1.
void DrawBars(uintptr_t menu, bool focused, float x, float y, float value) {
    register uintptr_t a0 asm("a0") = menu;
    register int a1 asm("a1") = 2;
    register int a2 asm("a2") = focused;
    register float f12 asm("$f12") = x;
    register float f13 asm("$f13") = y;
    register float f14 asm("$f14") = 2.0f;
    register float f15 asm("$f15") = 15.0f;
    register float f16 asm("$f16") = 60.0f;
    register float f17 asm("$f17") = value;
    asm volatile("jal 0x3423A0; nop"
        : "+r"(a0), "+r"(a1), "+r"(a2), "+f"(f12), "+f"(f13), "+f"(f14), "+f"(f15), "+f"(f16), "+f"(f17)
        :
        : "at", "v0", "v1", "a3", "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
          "t8", "t9", "ra", "hi", "lo", "memory",
          "$f0", "$f1", "$f2", "$f3", "$f4", "$f5", "$f6", "$f7", "$f8", "$f9", "$f10", "$f11",
          "$f18", "$f19", "$f20", "$f21", "$f22", "$f23", "$f24",
          "$f25", "$f26", "$f27", "$f28", "$f29", "$f30", "$f31");
}
// Value column of one row (DrawStandardMenu 0x33ACB8, before the native value
// switch): v0 = action, sp+0x68 = row, sp+0x84 = pass (values in pass 1),
// value text in fp, its position and alignment at sp+0x74/0x78/0x7C.
void DrawValue(SafetyMipsContext& regs) {
    const int action = int16_t(regs.v0);
    if (!Ours(action) || Centered(action)) return;
    const unsigned row = *reinterpret_cast<const uint32_t*>(regs.sp + 0x68) % 15;
    const bool pass = *reinterpret_cast<const uint32_t*>(regs.sp + 0x84) != 0;
    const auto& entry = ScreenAt(unsigned(CurrentScreen()) % 64).entries[row];
    const bool focused = Field<int>(rowOffset) == int(row) && Field<uint8_t>(inPageOffset);
    const uint16_t* text = emptyText;
    if (action == FixDefaults) {
        text = Ascii(row, confirmDefaults ? Catalog().confirm : PresetMatches(recommendedPreset) ? Catalog().inUse : Catalog().press);
    } else if (action >= BindingRow && action < BindingEnd) {
        const int bound = RowAction(unsigned(action - BindingRow));
        if (bound < 0) return;
        if (IconRow(Act(bound), focused)) {
            if (pass) {
                float x = float(entry.x + 12);
                for (unsigned slot = 0; slot < 2; ++slot) {
                    const uint8_t key = bindings.keys[bound][slot];
                    if (!key) continue;
                    Color color{255, 255, 255, uint8_t(focused && slot != bindingSlot ? 80 : 255)};
                    x += DrawKeyIcon(key, x, float(entry.y) - 1.0f, 13.0f, color) + 5.0f;
                }
            }
        } else text = BindingText(row, Act(bound), focused);
    } else if (Slider(action)) {
        if (pass) {
            const float value = action == HudSize ? (Value(HudSize) - 50) * 0.01f
                : action == FieldOfView ? (Value(FieldOfView) - 50) / 150.0f : Value(MouseSensitivity) / 40.0f;
            DrawBars(Menu(), focused, float(entry.x + 12), float(entry.y - 1), console::bounded(value, 0.0f, 1.0f, 0.5f));
        }
    } else if (action == FrameRate) {
        text = Ascii(row, settings.sixtyFPS ? "60" : "30");
    } else if (action == Preset) {
        text = Ascii(row, shownPreset < 0 ? Catalog().custom : Catalog().presets[shownPreset]);
    } else if (action == ApplyPreset) {
        text = Ascii(row, shownPreset < 0 ? "" : confirmPreset ? Catalog().confirm : PresetMatches(shownPreset) ? Catalog().inUse : Catalog().press);
    } else if (action == CutsceneBorders) {
        text = Ascii(row, Catalog().borders[settings.cutsceneBorders & 3]);
    } else if (action == GamepadIcons) {
        text = Ascii(row, iconNames[settings.gamepadIcons & 3]);
    } else if (action == HudAspect) {
        const int index = HudAspectIndex();
        if (index >= 0) text = Ascii(row, hudAspectNames[index]);
        else { char ratio[16]; console::format(ratio, "", settings.hudAspect, 2); text = Ascii(row, ratio); }
    } else {
        text = Ascii(row, Value(action) ? Catalog().on : Catalog().off);
    }
    regs.fp = uint32_t(reinterpret_cast<uintptr_t>(text ? text : emptyText));
    *reinterpret_cast<int*>(regs.sp + 0x74) = entry.x + 12;
    *reinterpret_cast<int*>(regs.sp + 0x78) = entry.y;
    *reinterpret_cast<int*>(regs.sp + 0x7C) = 1;
}
// Cross on a row (ProcessUserInput 0x3411E0, before the native action switch):
// v0 = action, s1 = menu. Unknown actions only toggle the row edit flag, which
// would block up/down; preset it so the toggle clears it.
void Select(SafetyMipsContext& regs) {
    const int action = int16_t(regs.v0);
    if (!Ours(action)) return;
    if (action != ApplyPreset) confirmPreset = false;
    if (action != FixDefaults) confirmDefaults = false;
    auto* extension = Owner(CurrentScreen());
    if (extension && extension->attached) switch (action) {
    case MoreOptions: Show(display, 1); break;
    case FixOptions: if (extension == &display) Show(display, 5); break;
    case PcMenu: Show(*extension, extension == &display ? 3 : 1); break;
    case FixDefaults: ApplyPresetValues(recommendedPreset, confirmDefaults); break;
    case BindingsMenu: bindingPage = 0; Show(*extension, 2); break;
    case NextPage: bindingPage = (bindingPage + 1) % BindingPages(); Show(*extension, 2, NextPage); break;
    case ResetBindings: bindings.Reset(); break;
    case ApplyPreset: ApplyPresetValues(shownPreset, confirmPreset); break;
    case Back: ShowParent(*extension); break;
    default:
        if (action >= BindingRow && action < BindingEnd) {
            const int bound = RowAction(unsigned(action - BindingRow));
            if (bound >= 0) { captureAction = Act(bound); captureSlot = bindingSlot; captureFrames = 0; capture.start(); }
        } else if (!Slider(action)) Adjust(action, 1);
        break;
    }
    // After Show() cleared it: the native toggle then leaves the flag clear.
    *reinterpret_cast<uint8_t*>(uintptr_t(regs.s1) + editOffset) = 1;
}
const Entry& Current() {
    return ScreenAt(unsigned(CurrentScreen()) % 64).entries[unsigned(Field<int>(rowOffset)) % 15];
}
uint64_t AdjustOption(uintptr_t menu, int direction) {
    const int screen = *reinterpret_cast<int*>(menu + screenOffset);
    if (Owner(screen) && Ours(Current().action)) {
        const int action = Current().action;
        if (action >= BindingRow && action < BindingEnd) bindingSlot ^= 1;
        else if (!Centered(action)) Adjust(action, direction);
        return 0;
    }
    return adjust.call(menu, direction);
}
uint64_t ProcessInput(uintptr_t menu) {
    // A key capture owns the keyboard until every key is released.
    if (capture.active()) return 0;
    auto* extension = Owner(*reinterpret_cast<int*>(menu + screenOffset));
    auto& pad = *reinterpret_cast<Pad*>(address::pads);
    // Triangle (released) leaves a submenu for its parent, not the page.
    if (extension && extension->mode && *reinterpret_cast<uint8_t*>(menu + inPageOffset) &&
        !pad.current.triangle && pad.previous.triangle) {
        ShowParent(*extension);
        const auto saved = pad.previous.triangle;
        pad.previous.triangle = 0;
        const auto result = processInput.call(menu);
        pad.previous.triangle = saved;
        return result;
    }
    return processInput.call(menu);
}
// Help and restart lines under the plugin rows, drawn with the game's font
// the way its own frontend messages are (e.g. the missing-controller notice).
namespace font {
pcsx2::GameFunction<void(float, float)> scale;
pcsx2::GameFunction<void()> justifyOff, backgroundOff, centreOn, propOn, flush;
pcsx2::GameFunction<void(float)> centreSize;
pcsx2::GameFunction<void(uint32_t)> color;
pcsx2::GameFunction<void(int)> style, shadow;
pcsx2::GameFunction<void(float, float, const uint16_t*, int)> print;
constexpr uintptr_t details = 0x4083B0, detailsSize = 0x50;
uint16_t line[2][128];
void Install() {
    scale.bind(0x166A50); justifyOff.bind(0x166B40); backgroundOff.bind(0x166BE8); centreOn.bind(0x166B58);
    propOn.bind(0x166C98); flush.bind(0x166A30); centreSize.bind(0x166BC8); color.bind(0x166A90);
    style.bind(0x166CA8); shadow.bind(0x166E50); print.bind(0x1658F8);
}
const uint16_t* Wide(unsigned slot, const char* text) {
    unsigned used = 0;
    for (; text && *text && used + 1 < 128; ++text) line[slot][used++] = uint16_t(uint8_t(*text));
    line[slot][used] = 0;
    return line[slot];
}
void Draw(const char* help, const char* restart) {
    if ((!help || !*help) && !restart) return;
    // Leave the font state as the native frontend had it.
    uint8_t saved[detailsSize];
    std::memcpy(saved, reinterpret_cast<const void*>(details), detailsSize);
    backgroundOff(); propOn(); justifyOff(); centreOn(); centreSize(440.0f); style(1); shadow(0);
    scale(0.3834f * 0.75f, 0.71f * 0.75f);
    if (help && *help) { color(0xFFC8C8C8u); print(240.0f, 236.0f, Wide(0, help), 0); }
    if (restart) { color(0xFF4070E0u); print(240.0f, 248.0f, Wide(1, restart), 0); }
    flush();
    std::memcpy(reinterpret_cast<void*>(details), saved, detailsSize);
}
}
const char* RowHelp(const Entry& entry) {
    const int action = entry.action;
    if (action >= FrameRate && action < OptionEnd) return Catalog().help[action - FrameRate];
    if (action >= FixOptions && action < PluginEnd) return Catalog().pluginHelp[action - FixOptions];
    return nullptr;
}
uint64_t DrawMenus(uintptr_t menu) {
    if (!display.attached) Attach(display, "FEH_DIS", 13);
    if (!controls.attached) Attach(controls, "FEH_CON", 7);
    const int screen = *reinterpret_cast<int*>(menu + screenOffset);
    for (auto* extension : {&display, &controls})
        if (extension->mode && screen != int(extension->index)) Show(*extension, 0);
    const auto result = drawMenus.call(menu);
    if (Owner(screen) && Owner(screen)->attached && *reinterpret_cast<const uint8_t*>(menu + inPageOffset)) {
        const auto& entry = Current();
        const bool ours = Ours(entry.action);
        const bool restart = settings.skipIntro != settings.startedSkipIntro && Owner(screen)->mode;
        if (ours || restart) font::Draw(ours ? RowHelp(entry) : nullptr, restart ? Catalog().restart : nullptr);
    }
    return result;
}
bool PlainControls() { return controls.mode == 0; }
uint64_t ControllerPicture(uintptr_t menu) { return PlainControls() ? controllerPicture.call(menu) : 0; }
uint64_t ControllerCallouts(uintptr_t menu, int flag) { return PlainControls() ? controllerCallouts.call(menu, flag) : 0; }
void Save() {
    const bool bindingsSaved = SaveBindings();
    if (!dirty && bindingsSaved) return;
    static PCSX2FIniRequest request;
    char number[16];
    unsigned status = PCSX2F_SETTINGS_OK;
    request = {};
    const auto set = [&](const char* section, const char* key, const char* value) {
        if (request.count == PCSX2F_SETTINGS_MAX_ENTRIES) {
            const auto result = console::ps2settings::request(request);
            if (result != PCSX2F_SETTINGS_OK) status = result;
            request.count = 0;
        }
        auto& e = request.entries[request.count++];
        std::strcpy(e.section, section); std::strcpy(e.key, key); std::strcpy(e.value, value);
    };
    request.size = sizeof(request); request.version = 1; request.operation = PCSX2F_SETTINGS_WRITE;
    set("MAIN", "FPSLimit", settings.sixtyFPS ? "60" : "30");
    set("MAIN", "UnthrottleEmuDuringLoading", settings.unthrottle ? "1" : "0");
    console::decimal(number, settings.hudScale, 2); set("MAIN", "HudScale", number);
    number[0] = char('0' + (settings.cutsceneBorders & 3)); number[1] = 0; set("MAIN", "CutsceneBorders", number);
    set("CONTROLS", "PCControlScheme", settings.pcControls ? "1" : "0");
    console::decimal(number, settings.mouseSensitivity / 0.002f, 2); set("MAIN", "MouseSensitivity", number);
    set("MAIN", "InvertMouseY", settings.invertMouse ? "1" : "0");
    set("MAIN", "DisableBlur", settings.disableBlur ? "1" : "0");
    console::decimal(number, settings.fovFactor, 2); set("MAIN", "FOVFactor", number);
    const int aspect = HudAspectIndex();
    set("MAIN", "HudAspectRatioConstraint", aspect == 0 ? "Auto" : aspect > 0 ? hudAspectNames[aspect] : config::values[11].value);
    set("MAIN", "SkipIntro", settings.skipIntro ? "1" : "0");
    number[0] = char('0' + (settings.gamepadIcons & 3)); number[1] = 0; set("MAIN", "GamepadIcons", number);
    set("MAIN", "VehicleLights", settings.vehicleLights ? "1" : "0");
    if (request.count) {
        const auto result = console::ps2settings::request(request);
        if (result != PCSX2F_SETTINGS_OK) status = result;
    }
    if (status == PCSX2F_SETTINGS_OK && bindingsSaved) { dirty = false; OSDText[1][0] = 0; }
    else std::strcpy(OSDText[1], status == PCSX2F_SETTINGS_UNSUPPORTED
        ? "Menu settings apply this session. Update the fork and injector to save them to the CFG."
        : "Could not save plugin settings to the CFG. Changes still apply this session.");
}
}
void TickMenu() {
    const bool active = MenuActive();
    if (capture.active()) {
        // A controller-only player can always get out: give up after ~10 s.
        if (!active || ++captureFrames > 600) CancelCapture();
        else if (capture.update(input)) {
            if (capture.cleared) bindings.clear(captureAction, captureSlot);
            else if (!capture.cancelled && capture.result) bindings.bind(captureAction, captureSlot, capture.result, game);
            CancelCapture();
        }
    } else if (active && Owner(CurrentScreen()) && Owner(CurrentScreen())->mode == 2 &&
               Current().action >= BindingRow && Current().action < BindingEnd && input.pressed(console::stories::code::del)) {
        const int bound = RowAction(unsigned(Current().action - BindingRow));
        if (bound >= 0) bindings.clear(Act(bound), bindingSlot);
    }
    if (menuWasActive && !active) {
        RestoreAll();
        Save();
    }
    menuWasActive = active;
}
bool MenuCaption(uintptr_t text) {
    const auto inside = [text](const void* buffer, size_t bytes) {
        const auto start = reinterpret_cast<uintptr_t>(buffer);
        return text >= start && text < start + bytes;
    };
    return inside(label, sizeof(label)) || inside(valueText, sizeof(valueText)) || inside(font::line, sizeof(font::line));
}
void ReloadMenuLanguage() { LoadMenuLanguage(); }
void InstallMenu() {
    LoadMenuLanguage();
    font::Install();
    getText = safetymips::create_inline_game(0x332E68, GetText);
    drawMenus = safetymips::create_inline_game(0x339FE0, DrawMenus);
    adjust = safetymips::create_inline_game(0x342ED0, AdjustOption);
    processInput = safetymips::create_inline_game(0x340B80, ProcessInput);
    controllerPicture = safetymips::create_inline_game(0x33E190, ControllerPicture);
    controllerCallouts = safetymips::create_inline_game(0x3460A0, ControllerCallouts);
    valueHook = safetymips::create_mid<&DrawValue>(0x33ACB8);
    selectHook = safetymips::create_mid<&Select>(0x3411E0);
}
}
