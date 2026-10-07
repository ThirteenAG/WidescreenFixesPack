#include "Controls.hpp"
#include "ButtonIcons.hpp"
#include "../Shared/Console/Text.hpp"
#include <cstring>
extern "C" {
#include "../../external/injector/include/ps2/plugin_settings.h"
PCSX2F_SETTINGS_API();
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
constexpr uintptr_t textObject = 0x3D89B8;

enum Option : int16_t {
    FrameRate = 0x60, MoreOptions, HudSize, CutsceneBorders, FastLoading, PcMenu, PcControls,
    MouseSensitivity, InvertMouse, BindingsMenu, NextPage, ResetBindings, Back, Blur, OptionEnd,
    BindingRow = 0x70, BindingEnd = 0x7A
};
constexpr unsigned bindingsPerPage = BindingEnd - BindingRow;
constexpr unsigned optionCount = OptionEnd - FrameRate;
constexpr char keys[optionCount][8] = {"WF_FPS", "WF_MORE", "WF_HUD", "WF_CBOR", "WF_LOAD", "WF_PCM",
    "WF_PC", "WF_SENS", "WF_INV", "WF_BIND", "WF_NEXT", "WF_RST", "WF_BACK", "WF_BLUR"};
constexpr char captions[optionCount][28] = {"FRAME RATE:", "MORE DISPLAY OPTIONS", "HUD SIZE", "CUTSCENE BORDERS",
    "FAST LOADING", "PC CONTROLS", "KEYBOARD AND MOUSE", "MOUSE SENSITIVITY", "INVERT MOUSE Y", "KEY BINDINGS",
    "NEXT PAGE", "RESET TO DEFAULTS", "", "BLUR EFFECT"};
bool Ours(int action) { return (action >= FrameRate && action < OptionEnd) || (action >= BindingRow && action < BindingEnd); }
bool Centered(int action) {
    return action == MoreOptions || action == PcMenu || action == BindingsMenu || action == NextPage ||
        action == ResetBindings || action == Back;
}
bool Slider(int action) { return action == HudSize || action == MouseSensitivity; }

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
    bool attached = false;
    unsigned mode = 0; // 0 native rows, 1 submenu, 2 bindings
};
Extension display{displayScreen}, controls{controlsScreen};
unsigned bindingPage = 0, bindingSlot = 0, captureFrames = 0;
Act captureAction = Act::Forward;
unsigned captureSlot = 0;
bool dirty = false, menuWasActive = false;
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
void* Text() { return reinterpret_cast<void*>(*reinterpret_cast<uint32_t*>(textObject)); }

int RowAction(unsigned row) {
    const unsigned index = bindingPage * bindingsPerPage + row;
    return row < bindingsPerPage && index < available.count ? int(available.list[index]) : -1;
}
void CancelCapture() { capture = {}; }

Entry Row(int option, int16_t y, unsigned screen, const char* name = nullptr) {
    Entry entry{};
    entry.action = int16_t(option);
    const char* key = name ? name : keys[option - FrameRate];
    for (unsigned i = 0; i + 1 < sizeof(entry.name) && key[i]; ++i) entry.name[i] = key[i];
    entry.target = uint8_t(screen);
    entry.x = 240; entry.y = y;
    entry.align = Centered(option) ? 3 : 2;
    return entry;
}
unsigned Count(const Entry* rows) { unsigned n = 0; while (n < 15 && rows[n].action) ++n; return n; }
void Write(Extension& extension) {
    Entry rows[15]{};
    unsigned used = 0;
    const auto add = [&](const Entry& entry) { if (used < 15) rows[used++] = entry; };
    if (extension.mode == 0) {
        const unsigned count = Count(extension.original);
        if (extension.index == displayScreen) {
            // Frame rate replaces the Widescreen row (the fix sets the
            // preference from the display); More display options and the
            // native Screen Position action come last.
            Entry position{}; bool hasPosition = false;
            for (unsigned i = 0; i < count; ++i) {
                const auto& entry = extension.original[i];
                if (entry.action == 29) { position = entry; hasPosition = true; continue; }
                if (entry.action == 12 && settings.widescreen) { add(Row(FrameRate, 0, displayScreen)); continue; }
                add(entry);
                if (entry.action == 12) add(Row(FrameRate, 0, displayScreen));
            }
            add(Row(MoreOptions, 0, displayScreen));
            if (hasPosition) add(position);
            int16_t y = 40;
            for (unsigned i = 0; i < used; ++i, y += 20) rows[i].y = y;
        } else {
            for (unsigned i = 0; i < count; ++i) add(extension.original[i]);
            add(Row(PcMenu, 0, controlsScreen));
            // Keep the group above the controller picture and its callouts.
            int16_t y = 24;
            for (unsigned i = 0; i < used; ++i, y += 13) rows[i].y = y;
        }
    } else if (extension.mode == 1 && extension.index == displayScreen) {
        int16_t y = 40;
        for (int option : {HudSize, CutsceneBorders, Blur, FastLoading, Back}) { add(Row(option, y, displayScreen)); y += 20; }
    } else if (extension.mode == 1) {
        int16_t y = 30;
        for (int option : {PcControls, MouseSensitivity, InvertMouse, BindingsMenu, Back}) { add(Row(option, y, controlsScreen)); y += 18; }
    } else {
        int16_t y = 26;
        static constexpr char names[bindingsPerPage][8] = {"WF_B0", "WF_B1", "WF_B2", "WF_B3", "WF_B4", "WF_B5", "WF_B6", "WF_B7", "WF_B8", "WF_B9"};
        for (unsigned i = 0; i < bindingsPerPage && RowAction(i) >= 0; ++i, y += 15)
            add(Row(BindingRow + int(i), y, controlsScreen, names[i]));
        if (BindingPages() > 1) { add(Row(NextPage, y, controlsScreen)); y += 15; }
        add(Row(ResetBindings, y, controlsScreen)); y += 15;
        add(Row(Back, y, controlsScreen));
    }
    std::memcpy(ScreenAt(extension.index).entries, rows, sizeof(rows));
}
// Switches a screen's rows and keeps the cursor on a written row.
void Show(Extension& extension, unsigned mode, int select = -1) {
    if (!extension.attached) return;
    CancelCapture();
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
    if (Count(extension.original) + 3 > 15) return;
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
    default: return 0;
    }
}
void Adjust(int option, int direction) {
    switch (option) {
    case FrameRate: settings.sixtyFPS = !settings.sixtyFPS; ApplyFrameRate(); break;
    case HudSize: {
        int percent = Value(HudSize) + (direction > 0 ? 10 : -10);
        percent = (percent + 5) / 10 * 10;
        settings.hudScale = console::bounded(percent * 0.01f, 0.5f, 1.5f, 1.0f);
        break;
    }
    case CutsceneBorders: settings.cutsceneBorders = !settings.cutsceneBorders; break;
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
    default: return;
    }
    dirty = true;
}
// Menu captions in capitals, like the native rows. (Spaces that vanished on
// wide screens came from the compressed glyph advance; see Hud.cpp.)
template<class Char> const uint16_t* Upper(const Char* text, uint16_t (&out)[48]) {
    unsigned used = 0;
    for (unsigned i = 0; text && text[i] && used + 1 < 48; ++i)
        out[used++] = uint16_t(text[i] >= 'a' && text[i] <= 'z' ? text[i] - 32 : text[i]);
    out[used] = 0;
    return out;
}
uint16_t label[48];
const uint16_t* GetText(void* text, const char* key) {
    if (key && key[0] == 'W' && key[1] == 'F' && key[2] == '_') {
        if (!std::strcmp(key, "WF_BACK")) {
            // The game's translated "Back", in the option rows' capitals.
            const auto* source = getText.call(text, "FEDS_TB");
            unsigned i = 0;
            for (; source && source[i] && i + 1 < 48; ++i)
                label[i] = source[i] >= 'a' && source[i] <= 'z' ? uint16_t(source[i] - 'a' + 'A') : source[i];
            label[i] = 0;
            return label;
        }
        for (unsigned i = 0; i < optionCount; ++i) if (!std::strcmp(key, keys[i])) return Upper(captions[i], label);
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
        append(capture.state == console::stories::Capture::Arming ? "..." : "PRESS A KEY (ESC CANCELS)");
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
    auto* game = Text();
    if (action >= BindingRow) {
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
            const float value = action == HudSize ? (Value(HudSize) - 50) * 0.01f : Value(MouseSensitivity) / 40.0f;
            DrawBars(Menu(), focused, float(entry.x + 12), float(entry.y - 1), console::bounded(value, 0.0f, 1.0f, 0.5f));
        }
    } else if (action == FrameRate) {
        text = Ascii(row, settings.sixtyFPS ? "60" : "30");
    } else if (game) {
        text = getText.call(game, Value(action) ? "FEM_ON" : "FEM_OFF");
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
    auto* extension = Owner(CurrentScreen());
    if (extension && extension->attached) switch (action) {
    case MoreOptions: Show(display, 1); break;
    case PcMenu: Show(controls, 1); break;
    case BindingsMenu: bindingPage = 0; Show(controls, 2); break;
    case NextPage: bindingPage = (bindingPage + 1) % BindingPages(); Show(controls, 2, NextPage); break;
    case ResetBindings: bindings.Reset(); break;
    case Back:
        if (extension->mode == 2) Show(*extension, 1, BindingsMenu);
        else Show(*extension, 0, extension == &display ? MoreOptions : PcMenu);
        break;
    default:
        if (action >= BindingRow) {
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
        if (action >= BindingRow) bindingSlot ^= 1;
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
        if (extension->mode == 2) Show(*extension, 1, BindingsMenu);
        else Show(*extension, 0, extension == &display ? MoreOptions : PcMenu);
        const auto saved = pad.previous.triangle;
        pad.previous.triangle = 0;
        const auto result = processInput.call(menu);
        pad.previous.triangle = saved;
        return result;
    }
    return processInput.call(menu);
}
uint64_t DrawMenus(uintptr_t menu) {
    if (!display.attached) Attach(display, "FEH_DIS", 13);
    if (!controls.attached) Attach(controls, "FEH_CON", 7);
    const int screen = *reinterpret_cast<int*>(menu + screenOffset);
    for (auto* extension : {&display, &controls})
        if (extension->mode && screen != int(extension->index)) Show(*extension, 0);
    return drawMenus.call(menu);
}
bool PlainControls() { return controls.mode == 0; }
uint64_t ControllerPicture(uintptr_t menu) { return PlainControls() ? controllerPicture.call(menu) : 0; }
uint64_t ControllerCallouts(uintptr_t menu, int flag) { return PlainControls() ? controllerCallouts.call(menu, flag) : 0; }
void Save() {
    const bool bindingsSaved = SaveBindings();
    if (!dirty && bindingsSaved) return;
    static PCSX2FIniRequest request;
    request = {};
    request.size = sizeof(request); request.version = 1;
    request.operation = PCSX2F_SETTINGS_WRITE; request.count = 8;
    auto& e = request.entries;
    const auto set = [&](unsigned i, const char* section, const char* key, const char* value) {
        std::strcpy(e[i].section, section); std::strcpy(e[i].key, key); std::strcpy(e[i].value, value);
    };
    set(0, "MAIN", "Enable60FPS", settings.sixtyFPS ? "1" : "0");
    set(1, "MAIN", "UnthrottleEmuDuringLoading", settings.unthrottle ? "1" : "0");
    set(2, "HUD", "HudScale", ""); console::format(e[2].value, "", settings.hudScale, 2);
    set(3, "DISPLAY", "CutsceneBorders", settings.cutsceneBorders ? "1" : "0");
    set(4, "CONTROLS", "PCControlScheme", settings.pcControls ? "1" : "0");
    set(5, "CONTROLS", "MouseSensitivity", ""); console::format(e[5].value, "", settings.mouseSensitivity, 4);
    set(6, "CONTROLS", "InvertMouseY", settings.invertMouse ? "1" : "0");
    set(7, "MAIN", "DisableBlur", settings.disableBlur ? "1" : "0");
    const auto status = PCSX2F_IniRequest(&request);
    if (status == PCSX2F_SETTINGS_OK && bindingsSaved) { dirty = false; OSDText[1][0] = 0; }
    else std::strcpy(OSDText[1], status == PCSX2F_SETTINGS_UNSUPPORTED
        ? "Menu settings apply this session. Update the fork and injector to save them to the INI."
        : "Could not save plugin settings to the INI. Changes still apply this session.");
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
    } else if (active && controls.mode == 2 && CurrentScreen() == int(controlsScreen) &&
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
    return inside(label, sizeof(label)) || inside(valueText, sizeof(valueText));
}
void InstallMenu() {
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
