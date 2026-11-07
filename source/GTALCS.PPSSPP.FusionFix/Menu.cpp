#include "Game.hpp"
#include "MenuFont.hpp"
#include "MenuBack.hpp"
#include "ModernControlText.hpp"
#include "../Shared/Console/PortableIni.hpp"
#include "../Shared/Console/Text.hpp"
#include <cstddef>
#include <cstring>
#include <cmath>

/// Native Display menu rows. LCS describes its frontend screens with a fixed
// table of 15 entries per screen; the Display screen uses four. Plugin rows use
// action ids the game ignores, so native navigation, fonts, highlights and
// sounds handle them; hooks supply their captions, values and adjustments.
// One "Plugins" entry swaps the screen's rows in place: the list of plugins with
// this fix's page and its Restore Defaults. Back (or leaving the screen)
// restores the original table.
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
    FixOptions, Preset, ApplyPreset, Back, VehicleLights, FixDefaults,
    LayoutOptions, AspectRatio, HudConstraint, RadarX, RadarY, MultiplayerY, PcCheats, MovieMode, IconSet, OptionEnd };
constexpr unsigned optionCount = OptionEnd - FrameRate;
constexpr char keys[optionCount][8] = {"WF_FPS", "WF_MORE", "WF_HUD", "WF_RAD", "WF_FOV", "WF_CBOR",
    "WF_CFOV", "WF_LOAD", "WF_INTR", "WF_CTRL", "WF_MRAD", "WF_DISP", "WF_GAME", "WF_FIX",
    "WF_PRE", "WF_APLY", "WF_BACK", "WF_VLIT", "WF_FDEF",
    "WF_LAY", "WF_ASP", "WF_HASP", "WF_RADX", "WF_RADY", "WF_MRY", "WF_PCC", "WF_MOV", "WF_ICON"};
constexpr char16_t captions[optionCount][28] = {u"FRAME RATE:", u"PLUGINS", u"HUD SIZE:",
    u"RADAR SIZE:", u"FIELD OF VIEW:", u"CUTSCENE BORDERS:", u"ORIGINAL CUTSCENE FOV:", u"FAST LOADING:",
    u"SKIP INTRO:", u"MODERN CONTROLS:", u"MULTIPLAYER RADAR SIZE:", u"DISPLAY AND CAMERA",
    u"GAME AND CONTROLS", u"FUSION FIX", u"PRESET:", u"APPLY PRESET", u"",
    u"VEHICLE LIGHTS:", u"RESTORE DEFAULTS", u"LAYOUT",
    u"ASPECT RATIO:", u"HUD ASPECT RATIO:", u"RADAR HORIZONTAL POSITION:", u"RADAR VERTICAL POSITION:",
    u"MULTIPLAYER RADAR POSITION:", u"PC CHEATS:", u"VIDEO ASPECT RATIO:", u"CONTROLLER ICONS:"};
bool Slider(int option) { return option == HudSize || option == RadarSize || option == MultiplayerRadar || option == FieldOfView; }
bool Ours(int action) { return action >= FrameRate && action < OptionEnd; }
bool Link(int action) { return action == MoreOptions || action == DisplayOptions || action == GameOptions || action == FixOptions || action == LayoutOptions || action == Back; }
// Rows that act when pressed and show their state as the value.
bool Command(int action) { return action == ApplyPreset || action == FixDefaults; }

Entry original[15];
char originalName[8];
bool attached, submenu, dirty, menuWasActive;
enum class Page { Native, Plugins, Fix, Display, Game, Layout };
Page page = Page::Native;
int shownPreset = 1;
bool confirmPreset, confirmDefaults;
bool startupSkipIntro, startupModernControls, startupVehicleLights;
uint16_t values[optionCount][16];
SafetyMipsInline textHook, drawHook, adjustHook, backHook;
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
        // The long pages (eight rows) keep Back above the help line (y 184).
        const bool tight = page == Page::Display || page == Page::Game;
        int16_t y = tight ? 40 : 46; unsigned used = 0;
        const auto add = [&](int option) { rows[used++] = Row(option, y, Link(option)); y += tight ? 16 : 18; };
        switch (page) {
        case Page::Plugins:
            add(FixOptions);
            break;
        case Page::Fix: add(Preset); add(ApplyPreset); add(DisplayOptions); add(LayoutOptions); add(GameOptions); add(FixDefaults); break;
        case Page::Display: add(HudSize); add(RadarSize); add(MultiplayerRadar); add(FieldOfView); add(CutsceneBorders); add(MovieMode); break;
        case Page::Game: add(FrameRate); add(FastLoading); add(SkipIntro); add(ModernControls); add(PcCheats); add(IconSet); add(VehicleLights); break;
        case Page::Layout: add(AspectRatio); add(HudConstraint); add(RadarX); add(RadarY); add(MultiplayerY); break;
        default: break;
        }
        add(Back);
    }
    std::memcpy(screen.entries, rows, sizeof(rows));
    int header = page == Page::Display ? DisplayOptions : page == Page::Game ? GameOptions : page == Page::Fix ? FixOptions : page == Page::Layout ? LayoutOptions : MoreOptions;
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
void BackPage()
{
    if (page == Page::Plugins) Show(false);
    else
    {
        page = page == Page::Display || page == Page::Game || page == Page::Layout ? Page::Fix : Page::Plugins;
        Show(true);
    }
}
bool NativeBack(uintptr_t pad)
{
    const bool pressed = backHook.call<bool>(pad);
    if (pressed && submenu && Field<int>(screenOffset) == int(displayScreen))
    {
        BackPage();
        return false;
    }
    return pressed;
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
    case FastLoading: return settings.unthrottle;
    case SkipIntro: return settings.skipIntro;
    case ModernControls: return settings.modernControls;
    case PcCheats: return settings.pcCheats;
    case VehicleLights: return settings.vehicleLights;
    default: return 0;
    }
}
// The descriptor's presets: 0 vanilla, 1 recommended (the defaults), 2 enhanced.
// The control scheme is the player's choice and no preset changes it.
struct PresetValues { bool fps, unthrottle, skipIntro; float hud, radar, multiplayerRadar, fov; int cutsceneBorders; bool vehicleLights; int icons; };
constexpr PresetValues presetValues[3] = {
    {false, false, false, 1.0f, 1.0f, 1.0f, 1.0f, 1, false, 0},
    {false, true, true, 0.75f, 0.83f, 0.63f, 1.0f, 3, true, 8},
    {false, true, true, 0.75f, 0.83f, 0.63f, 1.0f, 3, true, 8}};
constexpr int recommendedPreset = 1;
bool PresetMatches(int preset) {
    if (preset < 0 || preset > 2) return false;
    const auto& p = presetValues[preset];
    return settings.unthrottle == p.unthrottle && settings.skipIntro == p.skipIntro &&
        std::fabs(Drawing::settings.hud - p.hud) < 0.0001f && std::fabs(Drawing::settings.radar - p.radar) < 0.0001f &&
        std::fabs(Drawing::settings.multiplayerRadar - p.multiplayerRadar) < 0.0001f && settings.cutsceneBorders == p.cutsceneBorders &&
        settings.vehicleLights == p.vehicleLights && settings.icons == p.icons;
}
int MatchingPreset() { return PresetMatches(1) ? 1 : PresetMatches(0) ? 0 : PresetMatches(2) ? 2 : -1; }
constexpr float aspects[] = {0.0f, 4.0f / 3.0f, 16.0f / 9.0f, 21.0f / 9.0f, 32.0f / 9.0f};
constexpr const char* aspectNames[] = {"Auto", "4:3", "16:9", "21:9", "32:9"};
unsigned AspectIndex(float value)
{
    for (unsigned i = 0; i < 5; ++i)
        if (std::fabs(value - aspects[i]) < 0.001f) return i;
    return 0;
}
// Press once to arm, again to apply; nothing happens when the values already match.
void ApplyPresetValues(int preset, bool& confirm) {
    if (preset < 0 || preset > 2 || PresetMatches(preset)) { confirm = false; return; }
    if (!confirm) { confirm = true; return; }
    // Only settings that differ between presets are applied.
    const auto& p = presetValues[preset];
    settings.unthrottle = p.unthrottle; settings.skipIntro = p.skipIntro;
    if (!settings.unthrottle) console::portable::Unthrottle(false);
    Drawing::settings.hud = p.hud; Drawing::settings.radar = p.radar; Drawing::settings.multiplayerRadar = p.multiplayerRadar;
    settings.cutsceneBorders = p.cutsceneBorders;
    settings.vehicleLights = p.vehicleLights;
    settings.icons = p.icons;
    ApplyFrameRate(); dirty = true; confirm = false;
    shownPreset = preset;
}
bool DefaultsMatch()
{
    return PresetMatches(recommendedPreset) && settings.fps == 0 && settings.fov == 1.0f &&
        !settings.modernControls && !settings.pcCheats && settings.icons == 8 &&
        settings.textLanguage == TextLanguage::Auto && autoAspect && Drawing::settings.hudAspect == 0.0f &&
        Drawing::settings.radarX == 12.0f && Drawing::settings.radarY == 196.0f &&
        Drawing::settings.multiplayerRadarY == 170.0f;
}
void RestoreDefaults()
{
    if (DefaultsMatch()) { confirmDefaults = false; return; }
    if (!confirmDefaults) { confirmDefaults = true; return; }
    settings = Settings{};
    autoAspect = true;
    Drawing::settings = {};
    Drawing::settings.aspect = console::portable::Aspect();
    Drawing::settings.hud = 0.75f;
    Drawing::settings.radar = 0.83f;
    Drawing::settings.multiplayerRadar = 0.63f;
    Drawing::settings.multiplayerRadarY = 170.0f;
    ApplyFrameRate();
    dirty = true;
    confirmDefaults = false;
    shownPreset = recommendedPreset;
}
void Adjust(int option, int direction) {
    confirmPreset = false;
    confirmDefaults = false;
    const auto step = [&](float value, float minimum, float maximum)
    {
        const int percent = Percent(value);
        const int next = direction > 0 ? (percent / 5 + 1) * 5 : (percent - 1) / 5 * 5;
        return console::bounded(next * 0.01f, minimum, maximum, 1.0f);
    };
    switch (option) {
    case FrameRate: settings.fps = !settings.fps; ApplyFrameRate(); break;
    case HudSize: Drawing::settings.hud = step(Drawing::settings.hud, 0.1f, 2.0f); break;
    case RadarSize: Drawing::settings.radar = step(Drawing::settings.radar, 0.1f, 2.0f); break;
    case MultiplayerRadar: Drawing::settings.multiplayerRadar = step(Drawing::settings.multiplayerRadar, 0.1f, 2.0f); break;
    case FieldOfView: settings.fov = step(settings.fov, 0.5f, 2.0f); break;
    case CutsceneBorders: settings.cutsceneBorders = (settings.cutsceneBorders + (direction < 0 ? 3 : 1)) % 4; break;
    case MovieMode: settings.movieMode = settings.movieMode ? 0 : 1; break;
    case SkipIntro: settings.skipIntro = !settings.skipIntro; break;
    case ModernControls: settings.modernControls = !settings.modernControls; break;
    case PcCheats: settings.pcCheats = !settings.pcCheats; break;
    case IconSet:
        if (!IconsAvailable()) return;
        settings.icons = (settings.icons + (direction < 0 ? 8 : 1)) % 9;
        break;
    case AspectRatio:
    {
        const unsigned index = (AspectIndex(autoAspect ? 0.0f : Drawing::settings.aspect) + (direction < 0 ? 4 : 1)) % 5;
        autoAspect = index == 0;
        Drawing::settings.aspect = autoAspect ? console::portable::Aspect() : aspects[index];
        break;
    }
    case HudConstraint:
        Drawing::settings.hudAspect = aspects[(AspectIndex(Drawing::settings.hudAspect) + (direction < 0 ? 4 : 1)) % 5];
        break;
    case RadarX: Drawing::settings.radarX = console::bounded(Drawing::settings.radarX + direction, -480.0f, 480.0f, 12.0f); break;
    case RadarY: Drawing::settings.radarY = console::bounded(Drawing::settings.radarY + direction, -272.0f, 272.0f, 196.0f); break;
    case MultiplayerY: Drawing::settings.multiplayerRadarY = console::bounded(Drawing::settings.multiplayerRadarY + direction, -272.0f, 272.0f, 170.0f); break;
    case VehicleLights: settings.vehicleLights = !settings.vehicleLights; break;
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
    switch (ControlText(key, startupModernControls))
    {
    case ModernControlText::Right: return textHook.call<const uint16_t*>(text, "C0VELR");
    case ModernControlText::Left: return textHook.call<const uint16_t*>(text, "C0VELL");
    case ModernControlText::Circle: return textHook.call<const uint16_t*>(text, "C0PDFW");
    case ModernControlText::Cross: return textHook.call<const uint16_t*>(text, "C0VEACC");
    case ModernControlText::RightAndSquare:
    {
        static uint16_t combined[128];
        const auto right = textHook.call<const uint16_t*>(text, "C0VELR");
        const auto square = textHook.call<const uint16_t*>(text, "C0VEBRK");
        if (!right || !square) break;
        unsigned at = 0;
        for (unsigned i = 0; right[i] && at + 1 < 128; ++i) combined[at++] = right[i];
        for (const char* separator = " + "; *separator && at + 1 < 128; ++separator) combined[at++] = *separator;
        for (unsigned i = 0; square[i] && at + 1 < 128; ++i) combined[at++] = square[i];
        combined[at] = 0;
        return combined;
    }
    default: break;
    }
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
    auto& out = values[option - FrameRate];
    const uint16_t* source = nullptr;
    char ascii[8] = {};
    if ((option == MovieMode && !MoviesAvailable()) || (option == PcCheats && !CheatsAvailable()))
        return unavailable;
    if (option == IconSet)
    {
        if (!IconsAvailable() || !IconDataValid()) return unavailable;
        std::strcpy(ascii, iconSets[settings.icons]);
    }
    else if (option == AspectRatio || option == HudConstraint)
    {
        const float value = option == AspectRatio ? (autoAspect ? 0.0f : Drawing::settings.aspect) : Drawing::settings.hudAspect;
        const unsigned index = AspectIndex(value);
        if (index || value == 0.0f) std::strcpy(ascii, aspectNames[index]);
        else console::format(ascii, "", value, 2);
    }
    else if (option == RadarX || option == RadarY || option == MultiplayerY)
    {
        const float value = option == RadarX ? Drawing::settings.radarX : option == RadarY ? Drawing::settings.radarY : Drawing::settings.multiplayerRadarY;
        console::format(ascii, "", value, 0);
    }
    else if (option == FrameRate) std::strcpy(ascii, settings.fps ? "60" : "30");
    else if (option == Preset) {
        static const uint16_t names[][12] = {{'V','A','N','I','L','L','A',0}, {'R','E','C','O','M','M','E','N','D','E','D',0}, {'E','N','H','A','N','C','E','D',0}, {'C','U','S','T','O','M',0}};
        return names[shownPreset < 0 ? 3 : shownPreset];
    }
    else if (option == ApplyPreset) {
        return shownPreset < 0 ? empty : confirmPreset ? pressAgain : PresetMatches(shownPreset) ? inUse : pressX;
    }
    else if (option == FixDefaults) {
        return confirmDefaults ? pressAgain : DefaultsMatch() ? inUse : pressX;
    }
    else if (option == CutsceneBorders) {
        static const uint16_t borders[][12] = {{'O','F','F',0}, {'L','E','T','T','E','R','B','O','X',0},
            {'P','I','L','L','A','R','B','O','X',0}, {'B','O','T','H',0}};
        source = borders[settings.cutsceneBorders & 3];
    }
    else if (option == MovieMode)
    {
        static const char16_t modes[][9] = {u"ORIGINAL", u"FILL"};
        return reinterpret_cast<const uint16_t*>(modes[settings.movieMode ? 1 : 0]);
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
    const uint32_t restore = psp::enter_game();
    asm volatile("jalr %[fn]; nop"
        : "+r"(a0), "+f"(f12), "+f"(f13), "+f"(f17)
        : [fn] "r"(function)
        : "at", "v0", "v1", "a1", "a2", "a3", "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
          "t8", "t9", "ra", "hi", "lo", "memory",
          "$f0", "$f1", "$f2", "$f3", "$f4", "$f5", "$f6", "$f7", "$f8", "$f9", "$f10", "$f11",
          "$f14", "$f15", "$f16", "$f18", "$f19", "$f20", "$f21", "$f22", "$f23", "$f24",
          "$f25", "$f26", "$f27", "$f28", "$f29", "$f30", "$f31");
    psp::leave_game(restore);
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
        {
            const float minimum = action == FieldOfView ? 50.0f : 10.0f;
            DrawBars(Menu(), float(entry.x + 12), float(entry.y - 2),
                     console::bounded((Value(action) - minimum) / (200.0f - minimum), 0.0f, 1.0f, 0.5f));
        }
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
    switch (action) {
    case MoreOptions:
        ReloadTextLanguage();
        page = Page::Plugins; Show(true);
        break;
    case FixOptions: page = Page::Fix; shownPreset = MatchingPreset(); Show(true); break;
    case DisplayOptions: page = Page::Display; Show(true); break;
    case LayoutOptions: page = Page::Layout; Show(true); break;
    case GameOptions: page = Page::Game; Show(true); break;
    case Back:
        BackPage();
        break;
    case ApplyPreset: ApplyPresetValues(shownPreset, confirmPreset); break;
    case FixDefaults: RestoreDefaults(); break;
    default: if (!Slider(action)) Adjust(action, 1); break;
    }
    // The native path for unknown actions toggles the row edit state; keep
    // these rows in normal navigation.
    Field<uint8_t>(editOffset) = 1;
}
void AdjustOption(uintptr_t menu, int direction) {
    if (attached && *reinterpret_cast<int*>(menu + screenOffset) == int(displayScreen) && Ours(Current().action)) {
        const int action = Current().action;
        if (Command(action)) { confirmPreset = confirmDefaults = false; }
        else if (!Link(action)) Adjust(action, direction);
        return;
    }
    adjustHook.call<void>(menu, direction);
}
void DrawMenus(uintptr_t menu, int argument) {
    Drawing::Scope scope(Anchor::Center);
    if (autoAspect) Drawing::settings.aspect = console::portable::Aspect();
    Drawing::containUi = true;
    Attach();
    // Frontend dictionaries can load while gameplay's timing tick is paused.
    TickIcons(sceKernelGetSystemTimeLow());
    if (submenu && *reinterpret_cast<int*>(menu + screenOffset) != int(displayScreen)) Show(false);
    drawHook.call<void>(menu, argument);
    if (!submenu || Field<int>(screenOffset) != int(displayScreen) || !menu_font::available) return;
    const char16_t* help = nullptr;
    const char16_t* second = nullptr;
    switch (Current().action)
    {
    case FrameRate: help = u"Choose the original 30 FPS limit or 60 FPS."; break;
    case HudSize: help = u"Scale the HUD. The radar has separate size controls."; break;
    case RadarSize: help = u"Scale the single-player radar."; break;
    case MultiplayerRadar: help = u"Scale the radar used in multiplayer."; break;
    case FieldOfView: help = u"Adjust gameplay field of view. Higher values are wider."; break;
    case CutsceneBorders:
        help = u"Keep the original cinematic frame with black borders.";
        second = u"Off crops it to fill; Both preserves the whole shot.";
        break;
    case MovieMode:
        help = u"Original and Fill both show the whole video, uncut.";
        if (!MoviesAvailable()) second = u"Unsupported game code. Check the plugin log.";
        break;
    case FastLoading: help = u"Temporarily remove the emulator speed limit while loading."; break;
    case SkipIntro: help = u"Skip the opening intro on the next game start."; break;
    case ModernControls:
        help = u"Use shoulder buttons for driving, aiming and shooting.";
        second = u"Restart the game to apply the control scheme.";
        break;
    case PcCheats:
        help = u"Type PC cheat words in gameplay. See the hub.";
        if (!CheatsAvailable()) second = u"Unsupported game code. Check the plugin log.";
        break;
    case IconSet:
        help = u"Match the artwork to your PPSSPP bindings.";
        second = u"Missing or incompatible art stays original.";
        break;
    case AspectRatio: help = u"Auto follows PPSSPP. Choose a ratio to override it."; break;
    case HudConstraint: help = u"Keep the HUD in a centered box. Auto fills."; break;
    case RadarX: help = u"Move the single-player radar horizontally."; break;
    case RadarY: help = u"Move the radar vertically. -45 puts it on top."; break;
    case MultiplayerY: help = u"Move the multiplayer radar vertically."; break;
    case VehicleLights: help = u"Lit headlights, tail, brake and reverse lights after a restart."; break;
    case Preset: help = u"Choose Vanilla, Recommended or Enhanced, then Apply Preset."; break;
    case ApplyPreset: help = u"Press twice to apply the selected preset."; break;
    case FixDefaults: help = u"Press twice to restore all settings to their defaults."; break;
    default: break;
    }
    menu_font::Begin();
    if (help) menu_font::Text(help, 16.0f, 184.0f);
    if (second) menu_font::Text(second, 16.0f, 198.0f);
    if (settings.skipIntro != startupSkipIntro || settings.modernControls != startupModernControls ||
        settings.vehicleLights != startupVehicleLights)
        menu_font::Text(u"Restart the emulated game to apply settings.", 16.0f, 216.0f);
}
void Save() {
    if (!dirty) return;
    using console::portable::IniValue;
    IniValue values[19] = {
        {"MAIN", "FPSLimit", {}}, {"MAIN", "UnthrottleEmuDuringLoading", {}},
        {"MAIN", "HudScale", {}}, {"RADAR", "RadarScale", {}}, {"MAIN", "FOVFactor", {}},
        {"MAIN", "CutsceneBorders", {}},
        {"MAIN", "SkipIntro", {}}, {"CONTROLS", "ModernControlScheme", {}}, {"MPRADAR", "RadarScale", {}},
        {"MAIN", "ForceAspectRatio", {}}, {"MAIN", "HudAspectRatioConstraint", {}}, {"RADAR", "RadarPosX", {}},
        {"RADAR", "RadarPosY", {}}, {"MPRADAR", "RadarPosY", {}}, {"CONTROLS", "PCCheats", {}}, {"MAIN", "FMVWidescreenMode", {}}, {"MAIN", "GamepadIcons", {}}, {"MAIN", "TextLanguage", {}},
        {"MAIN", "VehicleLights", {}}};
    std::strcpy(values[0].value, settings.fps ? "60" : "30");
    std::strcpy(values[1].value, settings.unthrottle ? "1" : "0");
    console::format(values[2].value, "", Drawing::settings.hud, 2);
    console::format(values[3].value, "", Drawing::settings.radar, 2);
    console::format(values[4].value, "", settings.fov, 2);
    values[5].value[0] = char('0' + (settings.cutsceneBorders & 3)); values[5].value[1] = 0;
    std::strcpy(values[6].value, settings.skipIntro ? "1" : "0");
    std::strcpy(values[7].value, settings.modernControls ? "1" : "0");
    console::format(values[8].value, "", Drawing::settings.multiplayerRadar, 2);
    if (autoAspect) std::strcpy(values[9].value, "auto");
    else if (const auto index = AspectIndex(Drawing::settings.aspect)) std::strcpy(values[9].value, aspectNames[index]);
    else console::format(values[9].value, "", Drawing::settings.aspect, 4);
    if (Drawing::settings.hudAspect == 0.0f) std::strcpy(values[10].value, "Auto");
    else if (const auto index = AspectIndex(Drawing::settings.hudAspect)) std::strcpy(values[10].value, aspectNames[index]);
    else console::format(values[10].value, "", Drawing::settings.hudAspect, 4);
    console::format(values[11].value, "", Drawing::settings.radarX, 0);
    console::format(values[12].value, "", Drawing::settings.radarY, 0);
    console::format(values[13].value, "", Drawing::settings.multiplayerRadarY, 0);
    std::strcpy(values[14].value, settings.pcCheats ? "1" : "0");
    values[15].value[0] = char('0' + settings.movieMode); values[15].value[1] = 0;
    std::strcpy(values[16].value, iconSets[settings.icons]);
    std::strcpy(values[17].value, settings.textLanguage == TextLanguage::Auto ? "auto" : "en");
    std::strcpy(values[18].value, settings.vehicleLights ? "1" : "0");
    static console::portable::IniWriter<> writer;
    const bool saved = writer.update(iniPath, values, 19);
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
    if (!menu_back::profile.initialize({pattern.text_addr,
        reinterpret_cast<const uint8_t*>(pattern.text_addr), pattern.text_size, console::portable::gameGP}))
    {
        logger.Write("Plugins menu unavailable: unsupported Back input code. Use the sidecar configuration.\n");
        return;
    }
    startupSkipIntro = settings.skipIntro;
    startupModernControls = settings.modernControls;
    startupVehicleLights = settings.vehicleLights;
    menu_font::Initialize();
    if (!menu_font::available)
    {
        logger.Write("Plugins menu unavailable: unsupported help-text code. Use the sidecar configuration.\n");
        return;
    }
    backHook = safetymips::create_inline(menu_back::profile.get<0x08A970E0>(), NativeBack);
    textHook = safetymips::create_inline(Address<0x8913AD4>(), GetText);
    drawHook = safetymips::create_inline(Address<0x8ADFC40>(), DrawMenus);
    adjustHook = safetymips::create_inline(Address<0x8ADA240>(), AdjustOption);
    valueHook = safetymips::create_mid<&DrawValue>(Address<0x8AE09C8>());
    selectHook = safetymips::create_mid<&Select>(Address<0x8ADD3D0>());
}
}
