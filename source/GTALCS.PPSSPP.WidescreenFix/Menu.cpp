#include "Game.hpp"
#include "../Shared/Console/PortableIni.hpp"
#include "../Shared/Console/Text.hpp"
#include <cstddef>
#include <cstring>

// Native Display menu rows. LCS describes its frontend screens with a fixed
// table of 15 entries per screen; the Display screen uses four. Plugin rows use
// action ids the game ignores, so native navigation, fonts, highlights and
// sounds handle them; hooks supply their captions, values and adjustments.
// "More display options" swaps the screen's rows in place and Back (or leaving
// the screen) restores the original table.
namespace lcsws {
namespace {
struct Entry {
    int16_t action;
    char name[8];
    uint8_t saveSlot, target;
    int16_t x, y;
    uint8_t align, padding;
};
struct Screen { char name[8]; Entry entries[15]; };
static_assert(sizeof(Entry) == 18 && sizeof(Screen) == 278);
constexpr unsigned displayScreen = 5;
constexpr uintptr_t screenOffset = 0x564, optionOffset = 0x568, editOffset = 0x56C, activeOffset = 0x131;

enum Option : int16_t { FrameRate = 0x60, MoreOptions, HudSize, RadarSize, FieldOfView, CutsceneBorders,
    CutsceneFov, FastLoading, Back, OptionEnd };
constexpr unsigned optionCount = OptionEnd - FrameRate;
constexpr char keys[optionCount][8] = {"WF_FPS", "WF_MORE", "WF_HUD", "WF_RAD", "WF_FOV", "WF_CBOR",
    "WF_CFOV", "WF_LOAD", "WF_BACK"};
constexpr char16_t captions[optionCount][28] = {u"FRAME RATE:", u"MORE DISPLAY OPTIONS", u"HUD SIZE:",
    u"RADAR SIZE:", u"FIELD OF VIEW:", u"CUTSCENE BORDERS:", u"ORIGINAL CUTSCENE FOV:", u"FAST LOADING:", u""};
bool Slider(int option) { return option == HudSize || option == RadarSize || option == FieldOfView; }
bool Ours(int action) { return action >= FrameRate && action < OptionEnd; }

Entry original[15];
bool attached, submenu, dirty, menuWasActive;
uint16_t values[optionCount][16];
SafetyMipsInline textHook, drawHook, adjustHook;
SafetyMipsMid valueHook, selectHook;

Screen& Display() { return reinterpret_cast<Screen*>(Address<0x8B316B4>())[displayScreen]; }
uintptr_t Menu() { return Address<0x8B8EE20>(); }
template<class T> T& Field(uintptr_t offset) { return *reinterpret_cast<T*>(Menu() + offset); }

Entry Row(int option, int16_t y, bool centered = false) {
    Entry entry{};
    entry.action = int16_t(option);
    std::memcpy(entry.name, keys[option - FrameRate], sizeof(entry.name));
    entry.target = displayScreen;
    entry.x = 240; entry.y = y;
    entry.align = centered ? 3 : 2;
    return entry;
}
void Write(bool sub) {
    auto& screen = Display();
    Entry rows[15]{};
    if (!sub) {
        std::memcpy(rows, original, sizeof(rows));
        unsigned used = 0;
        while (used < 15 && rows[used].action) ++used;
        int16_t y = used ? rows[used - 1].y : 40;
        if (used + 2 <= 15) {
            rows[used++] = Row(FrameRate, y += 20);
            rows[used++] = Row(MoreOptions, y += 20, true);
        }
    } else {
        int16_t y = 46; unsigned used = 0;
        for (int option = HudSize; option <= Back; ++option, y += 18)
            rows[used++] = Row(option, y, option == Back);
    }
    std::memcpy(screen.entries, rows, sizeof(rows));
}
void Show(bool sub) {
    if (!attached) return;
    submenu = sub;
    Write(sub);
    // Select the first submenu row, or return to "More display options".
    if (Field<int>(screenOffset) == int(displayScreen)) {
        int index = 0;
        if (!sub) while (index < 14 && Display().entries[index].action != MoreOptions) ++index;
        Field<int>(optionOffset) = index;
    }
}
void Attach() {
    if (attached) return;
    auto& screen = Display();
    // Validate the native table before taking it over (Brightness first).
    if (std::strncmp(screen.name, "FEH_DIS", 8) || screen.entries[0].action != 13) return;
    std::memcpy(original, screen.entries, sizeof(original));
    attached = true;
    Show(false);
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
    default: return 0;
    }
}
void Adjust(int option, int direction) {
    const auto step = [&](float value) {
        // 10% steps between 50% and 150%, aligned to the step grid.
        int percent = (Percent(value) + (direction > 0 ? 10 : -1)) / 10 * 10;
        if (direction < 0 && percent >= Percent(value)) percent -= 10;
        return console::bounded(percent * 0.01f, 0.5f, 1.5f, 1.0f);
    };
    switch (option) {
    case FrameRate: settings.fps = !settings.fps; ApplyFrameRate(); break;
    case HudSize: Drawing::settings.hud = step(Drawing::settings.hud); break;
    case RadarSize: Drawing::settings.radar = step(Drawing::settings.radar); break;
    case FieldOfView: settings.fov = step(settings.fov); break;
    case CutsceneBorders: settings.cutsceneBorders = !settings.cutsceneBorders; break;
    case CutsceneFov: settings.restoreCutsceneFov = !settings.restoreCutsceneFov; break;
    case FastLoading:
        settings.unthrottle = !settings.unthrottle;
        if (!settings.unthrottle) console::portable::Unthrottle(false);
        break;
    default: return;
    }
    dirty = true;
}
const Entry& Current() { return Display().entries[unsigned(Field<int>(optionOffset)) % 15]; }

const uint16_t* GetText(void* text, const char* key) {
    if (key && key[0] == 'W' && key[1] == 'F' && key[2] == '_') {
        for (unsigned i = 0; i + 1 < optionCount; ++i)
            if (!std::strcmp(key, keys[i])) return reinterpret_cast<const uint16_t*>(captions[i]);
        if (!std::strcmp(key, "WF_BACK")) {
            // The game's translated "Back", in the option rows' capitals.
            static uint16_t back[32];
            const auto* source = textHook.call<const uint16_t*>(text, "FEDS_TB");
            unsigned i = 0;
            for (; source && source[i] && i + 1 < sizeof(back) / sizeof(back[0]); ++i)
                back[i] = source[i] >= 'a' && source[i] <= 'z' ? uint16_t(source[i] - 'a' + 'A') : source[i];
            back[i] = 0;
            return back;
        }
    }
    return textHook.call<const uint16_t*>(text, key);
}
const uint16_t* ValueText(uintptr_t text, int option) {
    auto& out = values[option - FrameRate];
    const uint16_t* source = nullptr;
    char ascii[8] = {};
    if (option == FrameRate) std::strcpy(ascii, settings.fps ? "60" : "30");
    else if (Slider(option)) { console::Text<8> t(ascii); t.integer(uint32_t(Value(option))); t.character('%'); }
    else source = textHook.call<const uint16_t*>(reinterpret_cast<void*>(text), Value(option) ? "FEM_ON" : "FEM_OFF");
    unsigned i = 0;
    if (source) for (; source[i] && i + 1 < 16; ++i) out[i] = source[i];
    else for (; ascii[i] && i + 1 < 16; ++i) out[i] = uint16_t(ascii[i]);
    out[i] = 0;
    return out;
}
// Native frontend slider: 8 segments at (x, y), value 0..1 passed in $f17.
void DrawBars(uintptr_t menu, float x, float y, float value) {
    register uintptr_t a0 asm("a0") = menu;
    register float f12 asm("$f12") = x;
    register float f13 asm("$f13") = y;
    register float f17 asm("$f17") = value;
    const uintptr_t function = Address<0x8AD6FB0>();
    asm volatile("jalr %[fn]; nop"
        : "+r"(a0), "+f"(f12), "+f"(f13), "+f"(f17)
        : [fn] "r"(function)
        : "at", "v0", "v1", "a1", "a2", "a3", "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
          "t8", "t9", "ra", "hi", "lo", "memory",
          "$f0", "$f1", "$f2", "$f3", "$f4", "$f5", "$f6", "$f7", "$f8", "$f9", "$f10", "$f11",
          "$f14", "$f15", "$f16", "$f18", "$f19", "$f20", "$f21", "$f22", "$f23", "$f24",
          "$f25", "$f26", "$f27", "$f28", "$f29", "$f30", "$f31");
}
void DrawValue(SafetyMipsContext& regs) {
    // a0: row action; s6: row index; s4: value text; s2/fp: value position;
    // s7: value alignment. Unknown actions skip the native value switch.
    const int action = int16_t(regs.a0);
    if (!Ours(action) || action == MoreOptions || action == Back) return;
    const auto& entry = Display().entries[unsigned(regs.s6) % 15];
    const uintptr_t text = *reinterpret_cast<uint32_t*>(Address<0x8B3607C>());
    if (!text) return;
    if (Slider(action)) {
        // Bars like the native Brightness row; drawn once, in the text pass.
        static const uint16_t empty[1] = {0};
        regs.s4 = uint32_t(reinterpret_cast<uintptr_t>(empty));
        if (*reinterpret_cast<const int8_t*>(regs.sp + 0x68) == 1)
            DrawBars(Menu(), float(entry.x + 12), float(entry.y - 2),
                     console::bounded((Value(action) - 50) * 0.01f, 0.0f, 1.0f, 0.5f));
        return;
    }
    regs.s4 = uint32_t(reinterpret_cast<uintptr_t>(ValueText(text, action)));
    regs.s2 = uint32_t(int32_t(entry.x + 12));
    regs.fp = uint32_t(int32_t(entry.y));
    regs.s7 = 1;
}
void Select(SafetyMipsContext& regs) {
    const int action = int16_t(regs.a0);
    if (!Ours(action) || Field<int>(screenOffset) != int(displayScreen)) return;
    if (action == MoreOptions) Show(true);
    else if (action == Back) Show(false);
    else if (!Slider(action)) Adjust(action, 1);
    // The native path for unknown actions toggles the row edit state; keep
    // these rows in normal navigation.
    Field<uint8_t>(editOffset) = 1;
}
void AdjustOption(uintptr_t menu, int direction) {
    if (attached && *reinterpret_cast<int*>(menu + screenOffset) == int(displayScreen) && Ours(Current().action)) {
        if (Current().action != MoreOptions && Current().action != Back) Adjust(Current().action, direction);
        return;
    }
    adjustHook.call<void>(menu, direction);
}
void DrawMenus(uintptr_t menu, int argument) {
    Attach();
    if (submenu && *reinterpret_cast<int*>(menu + screenOffset) != int(displayScreen)) Show(false);
    drawHook.call<void>(menu, argument);
}
void Save() {
    if (!dirty) return;
    using console::portable::IniValue;
    IniValue values[7] = {
        {"MAIN", "Enable60FPS", {}}, {"MAIN", "UnthrottleEmuDuringLoading", {}},
        {"HUD", "HudScale", {}}, {"RADAR", "RadarScale", {}}, {"FOV", "FOVFactor", {}},
        {"FOV", "RestoreCutsceneFOV", {}}, {"FOV", "CutsceneBorders", {}}};
    std::strcpy(values[0].value, settings.fps ? "1" : "0");
    std::strcpy(values[1].value, settings.unthrottle ? "1" : "0");
    console::format(values[2].value, "", Drawing::settings.hud, 2);
    console::format(values[3].value, "", Drawing::settings.radar, 2);
    console::format(values[4].value, "", settings.fov, 2);
    std::strcpy(values[5].value, settings.restoreCutsceneFov ? "1" : "0");
    std::strcpy(values[6].value, settings.cutsceneBorders ? "1" : "0");
    static console::portable::IniWriter<> writer;
    const bool saved = writer.update(iniPath, values, 7);
#ifndef NDEBUG
    logger.WriteF("Menu settings %s", saved ? "saved" : "could not be saved");
#endif
    if (saved) dirty = false;
}
}
void TickMenu() {
    const bool active = Field<uint8_t>(activeOffset) != 0;
    if (menuWasActive && !active) {
        if (submenu) Show(false);
        Save();
    }
    menuWasActive = active;
}
void InstallMenu() {
    textHook = safetymips::create_inline(Address<0x8913AD4>(), GetText);
    drawHook = safetymips::create_inline(Address<0x8ADFC40>(), DrawMenus);
    adjustHook = safetymips::create_inline(Address<0x8ADA240>(), AdjustOption);
    valueHook = safetymips::create_mid<&DrawValue>(Address<0x8AE09C8>());
    selectHook = safetymips::create_mid<&Select>(Address<0x8ADD3D0>());
}
}
