#pragma once
#include "Input.hpp"
#include <cstdint>
#include <cstring>

// Rebindable keyboard/mouse actions shared by the GTA Stories PS2 PC control
// schemes. Bindings are Windows virtual-key codes. Mouse buttons use the VK
// numbering (1 LMB, 2 RMB, 4 MMB, 5/6 X buttons); the unassigned VKs 0x0E/0x0F
// are the mouse wheel. Every action has two slots.
namespace console::stories {
namespace code {
constexpr uint8_t none = 0, lmb = 1, rmb = 2, mmb = 4, x1 = 5, x2 = 6, mouseMove = 7;
constexpr uint8_t back = 8, tab = 9, enter = 13, shift = 16, ctrl = 17, alt = 18, escape = 27, space = 32;
constexpr uint8_t wheelUp = 0x0E, wheelDown = 0x0F, pageUp = 33, pageDown = 34;
constexpr uint8_t left = 37, up = 38, right = 39, down = 40, del = 46;
constexpr uint8_t num0 = 96, num4 = 100, num5 = 101, num6 = 102, num8 = 104, numPlus = 107, backtick = 192;
}
enum class Group : uint8_t { OnFoot, Vehicle, Global };
enum Action : uint8_t {
    Forward, Backward, Left, Right, Sprint, Walk, Jump, Crouch, Attack, Aim, NextTarget, PrevTarget,
    EnterVehicle, NextWeapon, PrevWeapon, LookBehind, Camera, ZoomIn, ZoomOut, Phone, Mission,
    Accelerate, Brake, SteerLeft, SteerRight, Handbrake, Horn, VehicleFire, NextRadio, PrevRadio,
    LookLeft, LookRight, LeanForward, LeanBack, TurretLeft, TurretRight, TurretUp, TurretDown,
    Pause, ActionCount
};
struct ActionInfo { const char* ini; const char16_t* name; Group group; uint8_t defaults[2]; uint8_t games; };
constexpr uint8_t vcsOnly = 1, lcsOnly = 2, both = 3;
inline constexpr ActionInfo actions[ActionCount] = {
    {"Forward", u"Move forward", Group::OnFoot, {'W', 0}, both},
    {"Backward", u"Move backward", Group::OnFoot, {'S', 0}, both},
    {"Left", u"Move left", Group::OnFoot, {'A', 0}, both},
    {"Right", u"Move right", Group::OnFoot, {'D', 0}, both},
    {"Sprint", u"Sprint", Group::OnFoot, {code::shift, 0}, both},
    {"Walk", u"Walk", Group::OnFoot, {code::alt, 0}, both},
    {"Jump", u"Jump", Group::OnFoot, {code::space, 0}, both},
    {"Crouch", u"Crouch", Group::OnFoot, {code::ctrl, 0}, vcsOnly},
    {"Attack", u"Attack / fire", Group::OnFoot, {code::lmb, code::num0}, both},
    {"Aim", u"Aim / target", Group::OnFoot, {code::rmb, 0}, both},
    {"NextTarget", u"Next target", Group::OnFoot, {'E', 0}, both},
    {"PrevTarget", u"Previous target", Group::OnFoot, {'Q', 0}, both},
    {"EnterVehicle", u"Enter / exit vehicle", Group::Global, {'F', 0}, both},
    {"NextWeapon", u"Next weapon", Group::OnFoot, {code::wheelDown, 0}, both},
    {"PrevWeapon", u"Previous weapon", Group::OnFoot, {code::wheelUp, 0}, both},
    {"LookBehind", u"Look behind", Group::Global, {'C', code::mmb}, both},
    {"Camera", u"Change camera", Group::Global, {'V', 0}, both},
    {"ZoomIn", u"Zoom in", Group::OnFoot, {code::wheelUp, code::pageUp}, both},
    {"ZoomOut", u"Zoom out", Group::OnFoot, {code::wheelDown, code::pageDown}, both},
    {"Phone", u"Answer phone / action", Group::Global, {code::tab, 0}, both},
    {"Mission", u"Special mission", Group::Global, {'2', code::numPlus}, both},
    {"Accelerate", u"Accelerate", Group::Vehicle, {'W', 0}, both},
    {"Brake", u"Brake / reverse", Group::Vehicle, {'S', 0}, both},
    {"SteerLeft", u"Steer left", Group::Vehicle, {'A', 0}, both},
    {"SteerRight", u"Steer right", Group::Vehicle, {'D', 0}, both},
    {"Handbrake", u"Handbrake", Group::Vehicle, {code::space, 0}, both},
    {"Horn", u"Horn", Group::Vehicle, {'H', 0}, both},
    {"VehicleFire", u"Vehicle weapon", Group::Vehicle, {code::lmb, code::num0}, both},
    {"NextRadio", u"Next radio station", Group::Vehicle, {code::wheelUp, 0}, both},
    {"PrevRadio", u"Previous radio station", Group::Vehicle, {code::wheelDown, 0}, both},
    {"LookLeft", u"Look left / rudder left", Group::Vehicle, {'Q', 0}, both},
    {"LookRight", u"Look right / rudder right", Group::Vehicle, {'E', 0}, both},
    {"LeanForward", u"Lean forward / nose down", Group::Vehicle, {code::shift, code::num8}, both},
    {"LeanBack", u"Lean back / nose up", Group::Vehicle, {code::ctrl, code::num5}, both},
    {"TurretLeft", u"Turret left", Group::Vehicle, {code::num4, 0}, both},
    {"TurretRight", u"Turret right", Group::Vehicle, {code::num6, 0}, both},
    {"TurretUp", u"Turret up", Group::Vehicle, {code::num8, 0}, both},
    {"TurretDown", u"Turret down", Group::Vehicle, {code::num5, 0}, both},
    {"Pause", u"Pause menu", Group::Global, {code::escape, code::backtick}, both},
};

struct KeyName { uint8_t code; const char* ini; const char* text; };
inline constexpr KeyName keyNames[] = {
    {code::lmb, "LMB", "Left mouse"}, {code::rmb, "RMB", "Right mouse"}, {code::mmb, "MMB", "Middle mouse"},
    {code::x1, "MOUSE4", "Mouse 4"}, {code::x2, "MOUSE5", "Mouse 5"}, {code::mouseMove, "MOUSE", "Mouse"},
    {code::wheelUp, "WHEELUP", "Wheel up"}, {code::wheelDown, "WHEELDOWN", "Wheel down"},
    {8, "BACKSPACE", "Backspace"}, {9, "TAB", "Tab"}, {12, "CLEAR", "Clear"}, {13, "ENTER", "Enter"},
    {16, "SHIFT", "Shift"}, {17, "CTRL", "Ctrl"}, {18, "ALT", "Alt"}, {19, "PAUSE", "Pause"},
    {20, "CAPSLOCK", "Caps Lock"}, {27, "ESC", "Esc"}, {32, "SPACE", "Space"},
    {33, "PAGEUP", "Page Up"}, {34, "PAGEDOWN", "Page Down"}, {35, "END", "End"}, {36, "HOME", "Home"},
    {37, "LEFT", "Left"}, {38, "UP", "Up"}, {39, "RIGHT", "Right"}, {40, "DOWN", "Down"},
    {44, "PRINT", "Print Screen"}, {45, "INSERT", "Insert"}, {46, "DELETE", "Delete"},
    {91, "LWIN", "Left Win"}, {92, "RWIN", "Right Win"}, {93, "APPS", "Menu"},
    {96, "NUM0", "Num 0"}, {97, "NUM1", "Num 1"}, {98, "NUM2", "Num 2"}, {99, "NUM3", "Num 3"},
    {100, "NUM4", "Num 4"}, {101, "NUM5", "Num 5"}, {102, "NUM6", "Num 6"}, {103, "NUM7", "Num 7"},
    {104, "NUM8", "Num 8"}, {105, "NUM9", "Num 9"}, {106, "NUMMULTIPLY", "Num *"}, {107, "NUMPLUS", "Num +"},
    {109, "NUMMINUS", "Num -"}, {110, "NUMDECIMAL", "Num ."}, {111, "NUMDIVIDE", "Num /"},
    {144, "NUMLOCK", "Num Lock"}, {145, "SCROLLLOCK", "Scroll Lock"},
    {186, "SEMICOLON", ";"}, {187, "EQUALS", "="}, {188, "COMMA", ","}, {189, "MINUS", "-"},
    {190, "PERIOD", "."}, {191, "SLASH", "/"}, {192, "BACKTICK", "`"}, {219, "LBRACKET", "["},
    {220, "BACKSLASH", "\\"}, {221, "RBRACKET", "]"}, {222, "APOSTROPHE", "'"}, {226, "OEM102", "\\ (102)"},
};
inline const KeyName* FindName(uint8_t value) {
    for (const auto& name : keyNames) if (name.code == value) return &name;
    return nullptr;
}
// ASCII display text of a key, for keys without artwork.
template<size_t Size> void KeyText(uint8_t value, char (&out)[Size]) {
    static_assert(Size >= 12);
    out[0] = 0;
    if (!value) return;
    if ((value >= '0' && value <= '9') || (value >= 'A' && value <= 'Z')) { out[0] = char(value); out[1] = 0; return; }
    if (value >= 112 && value <= 135) {
        const unsigned n = value - 111u; unsigned i = 0;
        out[i++] = 'F'; if (n >= 10) out[i++] = char('0' + n / 10); out[i++] = char('0' + n % 10); out[i] = 0; return;
    }
    if (const auto* name = FindName(value)) {
        unsigned i = 0;
        for (; name->text[i] && i + 1 < Size; ++i) out[i] = name->text[i];
        out[i] = 0; return;
    }
    static constexpr char hex[] = "0123456789ABCDEF";
    std::memcpy(out, "Key ", 4); out[4] = hex[value >> 4]; out[5] = hex[value & 15]; out[6] = 0;
}
template<size_t Size> void KeyIni(uint8_t value, char (&out)[Size]) {
    static_assert(Size >= 12);
    out[0] = 0;
    if (!value) return;
    if ((value >= '0' && value <= '9') || (value >= 'A' && value <= 'Z')) { out[0] = char(value); out[1] = 0; return; }
    if (value >= 112 && value <= 135) { KeyText(value, out); return; }
    if (const auto* name = FindName(value)) { std::strncpy(out, name->ini, Size - 1); out[Size - 1] = 0; return; }
    static constexpr char hex[] = "0123456789ABCDEF";
    out[0] = '0'; out[1] = 'x'; out[2] = hex[value >> 4]; out[3] = hex[value & 15]; out[4] = 0;
}
inline bool SameText(const char* a, const char* b) {
    for (; *a && *b; ++a, ++b) {
        char x = *a, y = *b;
        if (x >= 'a' && x <= 'z') x = char(x - 32);
        if (y >= 'a' && y <= 'z') y = char(y - 32);
        if (x != y) return false;
    }
    return *a == *b;
}
// Parses one key name; returns 0 for empty/unknown text.
inline uint8_t ParseKey(const char* text) {
    while (*text == ' ' || *text == '\t') ++text;
    char token[16]{}; unsigned length = 0;
    while (*text && *text != ' ' && *text != '\t' && *text != ',' && length + 1 < sizeof(token)) token[length++] = *text++;
    if (!length) return 0;
    if (length == 1) {
        char c = token[0];
        if (c >= 'a' && c <= 'z') c = char(c - 32);
        if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z')) return uint8_t(c);
    }
    if ((token[0] == 'F' || token[0] == 'f') && length <= 3 && token[1] >= '1' && token[1] <= '9') {
        unsigned n = 0;
        for (unsigned i = 1; i < length; ++i) { if (token[i] < '0' || token[i] > '9') { n = 0; break; } n = n * 10 + unsigned(token[i] - '0'); }
        if (n >= 1 && n <= 24) return uint8_t(111 + n);
    }
    if (token[0] == '0' && (token[1] == 'x' || token[1] == 'X')) {
        unsigned value = 0;
        for (unsigned i = 2; i < length; ++i) {
            char c = token[i]; unsigned digit;
            if (c >= '0' && c <= '9') digit = unsigned(c - '0');
            else if (c >= 'a' && c <= 'f') digit = unsigned(c - 'a' + 10);
            else if (c >= 'A' && c <= 'F') digit = unsigned(c - 'A' + 10);
            else return 0;
            value = value * 16 + digit;
        }
        return value < 256 && value != code::mouseMove ? uint8_t(value) : 0;
    }
    for (const auto& name : keyNames) if (name.code != code::mouseMove && SameText(token, name.ini)) return name.code;
    return 0;
}

// Raw state of one binding code in a sampled frame.
inline bool CodeHeld(const Input& input, uint8_t value, bool previous = false) {
    const auto& mouse = previous ? input.previousMouse : input.mouse;
    switch (value) {
    case code::none: case code::mouseMove: return false;
    case code::lmb: return mouse.left != 0;
    case code::rmb: return mouse.right != 0;
    case code::mmb: return mouse.middle != 0;
    case code::x1: return mouse.extra1 != 0;
    case code::x2: return mouse.extra2 != 0;
    case code::wheelUp: return mouse.wheel > 0 || mouse.wheelUp;
    case code::wheelDown: return mouse.wheel < 0 || mouse.wheelDown;
    default: return (previous ? input.previousKeys[value] : input.keys[value]) != 0;
    }
}
inline bool Wheel(uint8_t value) { return value == code::wheelUp || value == code::wheelDown; }

struct Bindings {
    uint8_t keys[ActionCount][2]{};
    uint64_t dirty = 0;
    Bindings() { Reset(); }
    void Reset() {
        for (unsigned i = 0; i < ActionCount; ++i)
            if (keys[i][0] != actions[i].defaults[0] || keys[i][1] != actions[i].defaults[1]) {
                keys[i][0] = actions[i].defaults[0]; keys[i][1] = actions[i].defaults[1]; dirty |= 1ull << i;
            }
    }
    bool held(const Input& input, Action action) const {
        return CodeHeld(input, keys[action][0]) || CodeHeld(input, keys[action][1]);
    }
    bool was(const Input& input, Action action) const {
        return CodeHeld(input, keys[action][0], true) || CodeHeld(input, keys[action][1], true);
    }
    // A wheel step is an event: it is "pressed" in the frame it arrives.
    bool pressed(const Input& input, Action action) const {
        bool now = false, before = false;
        for (auto value : keys[action]) {
            if (!value) continue;
            if (Wheel(value)) { if (CodeHeld(input, value)) return true; continue; }
            now |= CodeHeld(input, value); before |= CodeHeld(input, value, true);
        }
        return now && !before;
    }
    int axis(const Input& input, Action negative, Action positive, int range) const {
        return (int(held(input, positive)) - int(held(input, negative))) * range;
    }
    uint8_t primary(Action action) const { return keys[action][0] ? keys[action][0] : keys[action][1]; }
    // Two actions conflict when both can be used in the same situation.
    static bool Conflicts(Action a, Action b) {
        const auto x = actions[a].group, y = actions[b].group;
        return x == y || x == Group::Global || y == Group::Global;
    }
    // Binds a slot. A key already used by a conflicting action is swapped
    // with this slot's previous key (or cleared when there was none).
    void bind(Action action, unsigned slot, uint8_t value, uint8_t games) {
        if (slot > 1) return;
        const uint8_t previous = keys[action][slot];
        if (previous == value) return;
        if (value && keys[action][slot ^ 1] == value) {
            keys[action][slot ^ 1] = previous;
        } else if (value) {
            for (unsigned other = 0; other < ActionCount; ++other) {
                if (other == action || !(actions[other].games & games) || !Conflicts(action, Action(other))) continue;
                for (auto& key : keys[other]) if (key == value) {
                    key = keys[other][0] == previous || keys[other][1] == previous ? 0 : previous;
                    dirty |= 1ull << other;
                }
            }
        }
        keys[action][slot] = value;
        dirty |= 1ull << action;
    }
    void clear(Action action, unsigned slot) {
        if (slot < 2 && keys[action][slot]) { keys[action][slot] = 0; dirty |= 1ull << action; }
    }
    // "W, Up" style INI value.
    template<size_t Size> void format(Action action, char (&out)[Size]) const {
        static_assert(Size >= 32);
        char a[16], b[16];
        KeyIni(keys[action][0], a); KeyIni(keys[action][1], b);
        unsigned i = 0;
        for (const char* p = a; *p; ++p) out[i++] = *p;
        if (*b) { out[i++] = ','; out[i++] = ' '; for (const char* p = b; *p; ++p) out[i++] = *p; }
        out[i] = 0;
    }
    void parse(Action action, const char* text) {
        uint8_t parsed[2]{};
        parsed[0] = ParseKey(text);
        while (*text && *text != ',') ++text;
        if (*text == ',') parsed[1] = ParseKey(text + 1);
        if (parsed[1] == parsed[0]) parsed[1] = 0;
        keys[action][0] = parsed[0]; keys[action][1] = parsed[1];
    }
};

// Captures the next key or mouse button for the bindings page. The key that
// opened the capture and the captured key must both be released first, so the
// menu never sees either one.
struct Capture {
    enum State : uint8_t { Idle, Arming, Listening, Releasing } state = Idle;
    uint8_t result = 0;
    bool cancelled = false, cleared = false;
    bool active() const { return state != Idle; }
    void start() { state = Arming; result = 0; cancelled = cleared = false; }
    static bool Anything(const Input& input) {
        for (unsigned i = 1; i < 256; ++i) if (CodeHeld(input, uint8_t(i)) && !Wheel(uint8_t(i))) return true;
        return false;
    }
    // Returns true in the frame a capture finishes (result, cancelled or cleared).
    bool update(const Input& input) {
        switch (state) {
        case Idle: return false;
        case Arming: if (!Anything(input)) state = Listening; return false;
        case Listening:
            for (unsigned i = 1; i < 256; ++i) {
                const auto value = uint8_t(i);
                // Raw left/right modifiers are folded into the generic codes.
                if (value == code::mouseMove || (value >= 160 && value <= 165)) continue;
                if (!CodeHeld(input, value) || (!Wheel(value) && CodeHeld(input, value, true))) continue;
                if (value == code::escape) cancelled = true;
                else if (value == code::del) cleared = true;
                else result = value;
                state = Wheel(value) ? Idle : Releasing;
                if (state == Idle) return true;
                break;
            }
            return false;
        case Releasing:
            if (!Anything(input)) { state = Idle; return true; }
            return false;
        }
        return false;
    }
};
}
