#include "Game.hpp"
#include "../Shared/Console/PortableIni.hpp"
#include "../Shared/Console/Text.hpp"
#include "../Shared/Console/RestartCfgPeer.hpp"
#include <cstddef>
#include <cstring>
#include <cmath>

/// Native Display menu rows. LCS describes its frontend screens with a fixed
// table of 15 entries per screen; the Display screen uses four. Plugin rows use
// action ids the game ignores, so native navigation, fonts, highlights and
// sounds handle them; hooks supply their captions, values and adjustments.
// One "Plugins" entry swaps the screen's rows in place: the list of plugins (this
// fix first, then the loaded siblings), each with its own page and Restore
// Defaults. Back (or leaving the screen) restores the original table.
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
    CutsceneFov, FastLoading, SkipIntro, ModernControls, MultiplayerRadar, DisplayOptions, GameOptions,
    FixOptions, Preset, ApplyPreset, Back, LightingOptions, VehicleLights, FixDefaults, LightingDefaults, OptionEnd };
constexpr unsigned optionCount = OptionEnd - FrameRate;
constexpr char keys[optionCount][8] = {"WF_FPS", "WF_MORE", "WF_HUD", "WF_RAD", "WF_FOV", "WF_CBOR",
    "WF_CFOV", "WF_LOAD", "WF_INTR", "WF_CTRL", "WF_MRAD", "WF_DISP", "WF_GAME", "WF_FIX",
    "WF_PRE", "WF_APLY", "WF_BACK", "WF_LGHT", "WF_VLIT", "WF_FDEF", "WF_LDEF"};
constexpr char16_t captions[optionCount][28] = {u"FRAME RATE:", u"PLUGINS", u"HUD SIZE:",
    u"RADAR SIZE:", u"FIELD OF VIEW:", u"CUTSCENE BORDERS:", u"ORIGINAL CUTSCENE FOV:", u"FAST LOADING:",
    u"SKIP INTRO (RESTART):", u"MODERN CONTROLS (RESTART):", u"MULTIPLAYER RADAR SIZE:", u"DISPLAY AND CAMERA",
    u"GAME AND CONTROLS", u"WIDESCREEN FIX", u"PRESET:", u"APPLY PRESET", u"", u"IMPROVED VEHICLE LIGHTS",
    u"VEHICLE LIGHTS (RESTART):", u"RESTORE DEFAULTS", u"RESTORE DEFAULTS"};
bool Slider(int option) { return option == HudSize || option == RadarSize || option == MultiplayerRadar || option == FieldOfView; }
bool Ours(int action) { return action >= FrameRate && action < OptionEnd; }
bool Link(int action) { return action == MoreOptions || action == DisplayOptions || action == GameOptions || action == FixOptions || action == LightingOptions || action == Back; }
// Rows that act when pressed and show their state as the value.
bool Command(int action) { return action == ApplyPreset || action == FixDefaults || action == LightingDefaults; }

Entry original[15];
char originalName[8];
bool attached, submenu, dirty, menuWasActive;
enum class Page { Native, Plugins, Fix, Display, Game, Lighting };
Page page = Page::Native;
int shownPreset = 1;
bool confirmPreset, confirmDefaults, presetRestart;
bool confirmLighting, lightingSaveFailed;
bool lightingObserved, lightingAvailable, lightingRequested;
// Requested restart CFG state only: no foreign PRX callbacks or cached pointers.
console::portable::RestartCfgPeer lightingPeer("GTALCS.ImVehLM",
    "ms0:/PSP/PLUGINS/GTALCS.PPSSPP.ImVehLM/GTALCS.PPSSPP.ImVehLM.cfg", "MAIN", "VehicleLights");
bool ReadLighting(bool& requested) {
    const bool available = lightingPeer.Read(requested);
    if (!lightingObserved || available != lightingAvailable || (available && requested != lightingRequested)) {
        confirmLighting = false;
        lightingObserved = true;
        lightingAvailable = available;
        if (available) lightingRequested = requested;
    }
    return available;
}
bool LightingAvailable() { bool requested; return ReadLighting(requested); }
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
        if (used + 1 <= 15) {
            rows[used++] = Row(MoreOptions, y += 20, true);
        }
    } else {
        int16_t y = 46; unsigned used = 0;
        const auto add = [&](int option) { rows[used++] = Row(option, y, Link(option)); y += 18; };
        switch (page) {
        case Page::Plugins: add(FixOptions); if (LightingAvailable()) add(LightingOptions); break;
        case Page::Fix: add(Preset); add(ApplyPreset); add(DisplayOptions); add(GameOptions); add(FixDefaults); break;
        case Page::Display: add(HudSize); add(RadarSize); add(MultiplayerRadar); add(FieldOfView); add(CutsceneBorders); add(CutsceneFov); break;
        case Page::Game: add(FrameRate); add(FastLoading); add(SkipIntro); add(ModernControls); break;
        case Page::Lighting: add(VehicleLights); add(LightingDefaults); break;
        default: break;
        }
        add(Back);
    }
    std::memcpy(screen.entries, rows, sizeof(rows));
    int header = page == Page::Display ? DisplayOptions : page == Page::Game ? GameOptions : page == Page::Fix ? FixOptions : page == Page::Lighting ? LightingOptions : MoreOptions;
    std::memcpy(screen.name, sub ? keys[header - FrameRate] : originalName, sizeof(screen.name));
}
void Show(bool sub) {
    if (!attached) return;
    submenu = sub;
    if (!sub) page = Page::Native;
    else if (page == Page::Native) page = Page::Plugins;
    Write(sub);
    // Select the first submenu row, or return to "Plugins".
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
    std::memcpy(originalName, screen.name, sizeof(originalName));
    attached = true;
    Show(false);
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
    case FastLoading: return settings.unthrottle;
    case SkipIntro: return settings.skipIntro;
    case ModernControls: return settings.modernControls;
    default: return 0;
    }
}
// The descriptor's presets: 0 vanilla, 1 recommended (the defaults), 2 enhanced.
// The control scheme is the player's choice and no preset changes it.
struct PresetValues { bool fps, unthrottle, skipIntro; float hud, radar, multiplayerRadar, fov; bool restoreCutsceneFov; int cutsceneBorders; };
constexpr PresetValues presetValues[3] = {
    {false, false, false, 1.0f, 1.0f, 1.0f, 1.0f, true, 1},
    {false, true, true, 0.75f, 0.83f, 0.63f, 1.0f, true, 3},
    {false, true, true, 0.75f, 0.83f, 0.63f, 1.0f, true, 3}};
constexpr int recommendedPreset = 1;
bool PresetMatches(int preset) {
    if (preset < 0 || preset > 2) return false;
    const auto& p = presetValues[preset];
    return (settings.fps != 0) == p.fps && settings.unthrottle == p.unthrottle && settings.skipIntro == p.skipIntro &&
        std::fabs(Drawing::settings.hud - p.hud) < 0.0001f && std::fabs(Drawing::settings.radar - p.radar) < 0.0001f &&
        std::fabs(Drawing::settings.multiplayerRadar - p.multiplayerRadar) < 0.0001f && std::fabs(settings.fov - p.fov) < 0.0001f &&
        settings.restoreCutsceneFov == p.restoreCutsceneFov && settings.cutsceneBorders == p.cutsceneBorders;
}
int MatchingPreset() { return PresetMatches(1) ? 1 : PresetMatches(0) ? 0 : PresetMatches(2) ? 2 : -1; }
// Press once to arm, again to apply; nothing happens when the values already match.
void ApplyPresetValues(int preset, bool& confirm) {
    if (preset < 0 || preset > 2 || PresetMatches(preset)) { confirm = false; return; }
    if (!confirm) { confirm = true; return; }
    // The descriptor's preset values; the control scheme, editor/file-only
    // aspect, positions and PC cheats stay untouched.
    const auto& p = presetValues[preset];
    presetRestart = settings.skipIntro != p.skipIntro;
    settings.fps = p.fps; settings.unthrottle = p.unthrottle; settings.skipIntro = p.skipIntro;
    if (!settings.unthrottle) console::portable::Unthrottle(false);
    Drawing::settings.hud = p.hud; Drawing::settings.radar = p.radar; Drawing::settings.multiplayerRadar = p.multiplayerRadar;
    settings.fov = p.fov; settings.restoreCutsceneFov = p.restoreCutsceneFov; settings.cutsceneBorders = p.cutsceneBorders;
    ApplyFrameRate(); dirty = true; confirm = false;
    shownPreset = preset;
}
void Adjust(int option, int direction) {
    confirmPreset = false;
    confirmDefaults = false;
    presetRestart = false;
    confirmLighting = false;
    const auto step = [&](float value, float maximum) {
        // 10% steps between 50% and the maximum, aligned to the step grid.
        int percent = (Percent(value) + (direction > 0 ? 10 : -1)) / 10 * 10;
        if (direction < 0 && percent >= Percent(value)) percent -= 10;
        return console::bounded(percent * 0.01f, 0.5f, maximum, 1.0f);
    };
    switch (option) {
    case FrameRate: settings.fps = !settings.fps; ApplyFrameRate(); break;
    case HudSize: Drawing::settings.hud = step(Drawing::settings.hud, 1.5f); break;
    case RadarSize: Drawing::settings.radar = step(Drawing::settings.radar, 1.5f); break;
    case MultiplayerRadar: Drawing::settings.multiplayerRadar = step(Drawing::settings.multiplayerRadar, 1.5f); break;
    case FieldOfView: settings.fov = step(settings.fov, 2.0f); break;
    case CutsceneBorders: settings.cutsceneBorders = (settings.cutsceneBorders + (direction < 0 ? 3 : 1)) % 4; break;
    case CutsceneFov: settings.restoreCutsceneFov = !settings.restoreCutsceneFov; break;
    case SkipIntro: settings.skipIntro = !settings.skipIntro; break;
    case ModernControls: settings.modernControls = !settings.modernControls; break;
    case VehicleLights: {
        bool requested;
        if (ReadLighting(requested)) lightingSaveFailed = !lightingPeer.Write(!requested);
        return;
    }
    case Preset: shownPreset = shownPreset < 0 ? (direction < 0 ? 2 : 0) : (shownPreset + (direction < 0 ? 2 : 1)) % 3; return;
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
    if (settings.MenuLanguage() == TextLanguage::English && key && key[0] == 'W' && key[1] == 'F' && key[2] == '_') {
        for (unsigned i = 0; i < optionCount; ++i)
            if (i != Back - FrameRate && !std::strcmp(key, keys[i])) return reinterpret_cast<const uint16_t*>(captions[i]);
        if (!std::strcmp(key, "WF_BACK")) {
            static const uint16_t back[] = {'B','A','C','K',0};
            return back;
        }
    }
    return textHook.call<const uint16_t*>(text, key);
}
const uint16_t* ValueText(uintptr_t, int option) {
    static const uint16_t empty[] = {0};
    if (settings.MenuLanguage() != TextLanguage::English) return empty;
    static const uint16_t unavailable[] = {'U','N','A','V','A','I','L','A','B','L','E',0};
    static const uint16_t pressAgain[] = {'P','R','E','S','S',' ','A','G','A','I','N',0};
    static const uint16_t pressX[] = {'P','R','E','S','S',' ','X',0};
    static const uint16_t inUse[] = {'I','N',' ','U','S','E',0};
    static const uint16_t restart[] = {'R','E','S','T','A','R','T',0};
    static const uint16_t failed[] = {'S','A','V','E',' ','F','A','I','L','E','D',0};
    auto& out = values[option - FrameRate];
    const uint16_t* source = nullptr;
    char ascii[8] = {};
    if (option == FrameRate) std::strcpy(ascii, settings.fps ? "60" : "30");
    else if (option == Preset) {
        static const uint16_t names[][12] = {{'V','A','N','I','L','L','A',0}, {'R','E','C','O','M','M','E','N','D','E','D',0}, {'E','N','H','A','N','C','E','D',0}, {'C','U','S','T','O','M',0}};
        return names[shownPreset < 0 ? 3 : shownPreset];
    }
    else if (option == VehicleLights || option == LightingDefaults) {
        bool requested;
        static const uint16_t on[] = {'O','N',0}, off[] = {'O','F','F',0};
        if (!ReadLighting(requested)) { confirmLighting = false; return unavailable; }
        if (option == VehicleLights) return requested ? on : off;
        // The lights' only setting is on in every preset.
        return lightingSaveFailed ? failed : requested ? inUse : confirmLighting ? pressAgain : pressX;
    }
    else if (option == ApplyPreset) {
        return shownPreset < 0 ? empty : confirmPreset ? pressAgain : presetRestart ? restart : PresetMatches(shownPreset) ? inUse : pressX;
    }
    else if (option == FixDefaults) {
        return confirmDefaults ? pressAgain : presetRestart ? restart : PresetMatches(recommendedPreset) ? inUse : pressX;
    }
    else if (option == CutsceneBorders) {
        static const uint16_t borders[][12] = {{'O','F','F',0}, {'L','E','T','T','E','R','B','O','X',0},
            {'P','I','L','L','A','R','B','O','X',0}, {'B','O','T','H',0}};
        source = borders[settings.cutsceneBorders & 3];
    }
    else if (Slider(option)) { console::Text<8> t(ascii); t.integer(uint32_t(Value(option))); t.character('%'); }
    else {
        static const uint16_t boolean[][4] = {{'O','F','F',0}, {'O','N',0}};
        source = boolean[Value(option) != 0];
    }
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
    if (!Ours(action) || Link(action)) return;
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
    if (action != ApplyPreset) confirmPreset = false;
    if (action != FixDefaults) confirmDefaults = false;
    if (action != LightingDefaults) confirmLighting = false;
    switch (action) {
    case MoreOptions: ReloadTextLanguage(); page = Page::Plugins; Show(true); break;
    case FixOptions: page = Page::Fix; shownPreset = MatchingPreset(); presetRestart = false; Show(true); break;
    case DisplayOptions: page = Page::Display; Show(true); break;
    case GameOptions: page = Page::Game; Show(true); break;
    case LightingOptions: {
        bool requested;
        if (!ReadLighting(requested)) return;
        page = Page::Lighting; lightingSaveFailed = false; Show(true);
        break;
    }
    case LightingDefaults: {
        bool requested;
        if (!ReadLighting(requested) || requested) confirmLighting = false;
        else if (!confirmLighting) { confirmLighting = true; lightingSaveFailed = false; }
        else { lightingSaveFailed = !lightingPeer.Write(true); confirmLighting = false; }
        break;
    }
    case Back:
        if (page == Page::Plugins) Show(false);
        else { page = page == Page::Display || page == Page::Game ? Page::Fix : Page::Plugins; Show(true); }
        break;
    case ApplyPreset: ApplyPresetValues(shownPreset, confirmPreset); break;
    case FixDefaults: ApplyPresetValues(recommendedPreset, confirmDefaults); break;
    default: if (!Slider(action)) Adjust(action, 1); break;
    }
    // The native path for unknown actions toggles the row edit state; keep
    // these rows in normal navigation.
    Field<uint8_t>(editOffset) = 1;
}
void AdjustOption(uintptr_t menu, int direction) {
    if (attached && *reinterpret_cast<int*>(menu + screenOffset) == int(displayScreen) && Ours(Current().action)) {
        const int action = Current().action;
        if (Command(action)) { confirmPreset = confirmDefaults = confirmLighting = false; }
        else if (!Link(action)) Adjust(action, direction);
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
    IniValue values[10] = {
        {"MAIN", "Enable60FPS", {}}, {"MAIN", "UnthrottleEmuDuringLoading", {}},
        {"MAIN", "HudScale", {}}, {"RADAR", "RadarScale", {}}, {"MAIN", "FOVFactor", {}},
        {"FOV", "RestoreCutsceneFOV", {}}, {"MAIN", "CutsceneBorders", {}},
        {"MAIN", "SkipIntro", {}}, {"CONTROLS", "ModernControlScheme", {}}, {"MPRADAR", "RadarScale", {}}};
    std::strcpy(values[0].value, settings.fps ? "1" : "0");
    std::strcpy(values[1].value, settings.unthrottle ? "1" : "0");
    console::format(values[2].value, "", Drawing::settings.hud, 2);
    console::format(values[3].value, "", Drawing::settings.radar, 2);
    console::format(values[4].value, "", settings.fov, 2);
    std::strcpy(values[5].value, settings.restoreCutsceneFov ? "1" : "0");
    values[6].value[0] = char('0' + (settings.cutsceneBorders & 3)); values[6].value[1] = 0;
    std::strcpy(values[7].value, settings.skipIntro ? "1" : "0");
    std::strcpy(values[8].value, settings.modernControls ? "1" : "0");
    console::format(values[9].value, "", Drawing::settings.multiplayerRadar, 2);
    static console::portable::IniWriter<> writer;
    const bool saved = writer.update(iniPath, values, 10);
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
    lightingPeer.Initialize();
    textHook = safetymips::create_inline(Address<0x8913AD4>(), GetText);
    drawHook = safetymips::create_inline(Address<0x8ADFC40>(), DrawMenus);
    adjustHook = safetymips::create_inline(Address<0x8ADA240>(), AdjustOption);
    valueHook = safetymips::create_mid<&DrawValue>(Address<0x8AE09C8>());
    selectHook = safetymips::create_mid<&Select>(Address<0x8ADD3D0>());
}
}
