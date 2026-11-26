#include "Game.hpp"
#include "MenuFont.hpp"
#include "MenuBack.hpp"
#include "ModernControlText.hpp"
#include "Shared/Console/PortableIni.hpp"
#include "Shared/Console/Text.hpp"
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
    HoldCamera, FastLoading, SkipIntro, ModernControls, MultiplayerRadar, DisplayOptions, GameOptions,
    FixOptions, Preset, ApplyPreset, Back, VehicleLights, FixDefaults,
    LayoutOptions, AspectRatio, HudConstraint, PcCheats, IconSet, OptionEnd };
constexpr unsigned optionCount = OptionEnd - FrameRate;
constexpr char keys[optionCount][8] = {"WF_FPS", "WF_MORE", "WF_HUD", "WF_RAD", "WF_FOV", "WF_CBOR",
    "WF_HOLD", "WF_LOAD", "WF_INTR", "WF_CTRL", "WF_MRAD", "WF_DISP", "WF_GAME", "WF_FIX",
    "WF_PRE", "WF_APLY", "WF_BACK", "WF_VLIT", "WF_FDEF",
    "WF_LAY", "WF_ASP", "WF_HASP", "WF_PCC", "WF_ICON"};
constexpr char16_t captions[optionCount][28] = {u"FRAME RATE:", u"PLUGINS", u"HUD SIZE:",
    u"RADAR SIZE:", u"FIELD OF VIEW:", u"CUTSCENE BORDERS:", u"HOLD VEHICLE CAMERA:", u"FAST LOADING:",
    u"SKIP INTRO:", u"MODERN CONTROLS:", u"MULTIPLAYER RADAR SIZE:", u"DISPLAY AND CAMERA",
    u"GAME AND CONTROLS", u"FUSION FIX", u"PRESET:", u"APPLY PRESET", u"",
    u"VEHICLE LIGHTS:", u"RESTORE DEFAULTS", u"LAYOUT",
    u"ASPECT RATIO:", u"HUD ASPECT RATIO:", u"PC CHEATS:", u"CONTROLLER ICONS:"};
bool Slider(int option) { return option == HudSize || option == RadarSize || option == MultiplayerRadar || option == FieldOfView; }
bool Ours(int action) { return action >= FrameRate && action < OptionEnd; }
bool Link(int action) { return action == MoreOptions || action == DisplayOptions || action == GameOptions || action == FixOptions || action == LayoutOptions || action == Back; }
// Rows that act when pressed and show their state as the value.
bool Command(int action) { return action == ApplyPreset || action == FixDefaults; }

Entry original[15];
char originalName[8];
bool attached, submenu, menuWasActive;
uint32_t dirtySettings;
uint32_t lastSaveAttempt;

enum Setting : unsigned
{
    SaveRate, SaveLoading, SaveHud, SaveRadar, SaveFov, SaveBorders, SaveIntro,
    SaveModern, SaveMultiplayerRadar, SaveAspect, SaveHudAspect,
    SaveCheats, SaveIcons, SaveLanguage, SaveLights,
    SaveHold, SettingCount
};

struct CurrentSettings
{
    Settings game = settings;
    decltype(Drawing::settings) drawing = Drawing::settings;
    bool automaticAspect = autoAspect;
};

template<class Visitor> void VisitSavedSettings(const CurrentSettings& previous, Visitor visit)
{
    auto& drawing = Drawing::settings;
    visit(SaveRate, settings.fps, previous.game.fps);
    visit(SaveLoading, settings.unthrottle, previous.game.unthrottle);
    visit(SaveHud, drawing.hud, previous.drawing.hud);
    visit(SaveRadar, drawing.radar, previous.drawing.radar);
    visit(SaveFov, settings.fov, previous.game.fov);
    visit(SaveBorders, settings.cutsceneBorders, previous.game.cutsceneBorders);
    visit(SaveIntro, settings.skipIntro, previous.game.skipIntro);
    visit(SaveModern, settings.modernControls, previous.game.modernControls);
    visit(SaveMultiplayerRadar, drawing.multiplayerRadar, previous.drawing.multiplayerRadar);
    visit(SaveHudAspect, drawing.hudAspect, previous.drawing.hudAspect);
    visit(SaveCheats, settings.pcCheats, previous.game.pcCheats);
    visit(SaveIcons, settings.icons, previous.game.icons);
    visit(SaveLanguage, settings.textLanguage, previous.game.textLanguage);
    visit(SaveLights, settings.vehicleLights, previous.game.vehicleLights);
    visit(SaveHold, settings.holdVehicleCamera, previous.game.holdVehicleCamera);
}

void TrackChanges(const CurrentSettings& previous)
{
    VisitSavedSettings(previous, [](Setting setting, const auto& value, const auto& old)
    {
        if (value != old)
            dirtySettings |= 1u << setting;
    });
    if (autoAspect != previous.automaticAspect || (!autoAspect && Drawing::settings.aspect != previous.drawing.aspect))
        dirtySettings |= 1u << SaveAspect;
}
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
        case Page::Display: add(HudSize); add(RadarSize); add(MultiplayerRadar); add(FieldOfView); add(CutsceneBorders); add(HoldCamera); break;
        case Page::Game: add(FrameRate); add(FastLoading); add(SkipIntro); add(ModernControls); add(PcCheats); add(IconSet); add(VehicleLights); break;
        case Page::Layout: add(AspectRatio); add(HudConstraint); break;
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
    case HoldCamera: return settings.holdVehicleCamera;
    case PcCheats: return settings.pcCheats;
    case VehicleLights: return settings.vehicleLights;
    default: return 0;
    }
}
// The descriptor's presets: 0 vanilla, 1 recommended (the defaults), 2 enhanced.
struct PresetValues { bool fps, unthrottle, skipIntro; float hud, radar, multiplayerRadar, fov; int cutsceneBorders; bool vehicleLights; int icons;
    bool modernControls, holdCamera; };
constexpr PresetValues presetValues[3] = {
    {false, false, false, 1.0f, 1.0f, 1.0f, 1.0f, 1, false, 0, false, false},
    {false, true, true, 0.75f, 0.83f, 0.63f, 1.0f, 3, true, 8, true, false},
    {true, true, true, 0.75f, 0.83f, 0.63f, 1.0f, 3, true, 8, true, true}};
constexpr int recommendedPreset = 1;
bool PresetMatches(int preset) {
    if (preset < 0 || preset > 2) return false;
    const auto& p = presetValues[preset];
    return settings.unthrottle == p.unthrottle && settings.skipIntro == p.skipIntro &&
        std::fabs(Drawing::settings.hud - p.hud) < 0.0001f && std::fabs(Drawing::settings.radar - p.radar) < 0.0001f &&
        std::fabs(Drawing::settings.multiplayerRadar - p.multiplayerRadar) < 0.0001f && settings.cutsceneBorders == p.cutsceneBorders &&
        settings.vehicleLights == p.vehicleLights && settings.icons == p.icons && (settings.fps != 0) == p.fps &&
        settings.modernControls == p.modernControls && settings.holdVehicleCamera == p.holdCamera;
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
    const CurrentSettings previous;
    const auto& p = presetValues[preset];
    settings.unthrottle = p.unthrottle; settings.skipIntro = p.skipIntro;
    if (!settings.unthrottle) console::portable::Unthrottle(false);
    Drawing::settings.hud = p.hud; Drawing::settings.radar = p.radar; Drawing::settings.multiplayerRadar = p.multiplayerRadar;
    settings.cutsceneBorders = p.cutsceneBorders;
    settings.vehicleLights = p.vehicleLights;
    settings.icons = p.icons;
    settings.fps = p.fps;
    settings.modernControls = p.modernControls;
    settings.holdVehicleCamera = p.holdCamera;
    ApplyFrameRate();
    TrackChanges(previous);
    confirm = false;
    shownPreset = preset;
}
bool DefaultsMatch()
{
    return PresetMatches(recommendedPreset) && settings.fps == 0 && settings.fov == 1.0f &&
        settings.modernControls && !settings.holdVehicleCamera && !settings.pcCheats && settings.icons == 8 &&
        settings.textLanguage == TextLanguage::Auto && autoAspect && Drawing::settings.hudAspect == 0.0f;
}
void RestoreDefaults()
{
    if (DefaultsMatch()) { confirmDefaults = false; return; }
    if (!confirmDefaults) { confirmDefaults = true; return; }
    const CurrentSettings previous;
    settings = Settings{};
    autoAspect = true;
    Drawing::settings = {};
    Drawing::settings.aspect = console::portable::Aspect();
    Drawing::settings.hud = 0.75f;
    Drawing::settings.radar = 0.83f;
    Drawing::settings.multiplayerRadar = 0.63f;
    ApplyFrameRate();
    TrackChanges(previous);
    confirmDefaults = false;
    shownPreset = recommendedPreset;
}
void Adjust(int option, int direction) {
    confirmPreset = false;
    confirmDefaults = false;
    const CurrentSettings previous;
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
    case SkipIntro: settings.skipIntro = !settings.skipIntro; break;
    case ModernControls: settings.modernControls = !settings.modernControls; break;
    case HoldCamera: settings.holdVehicleCamera = !settings.holdVehicleCamera; break;
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
    case VehicleLights: settings.vehicleLights = !settings.vehicleLights; break;
    case Preset: shownPreset = shownPreset < 0 ? (direction < 0 ? 2 : 0) : (shownPreset + (direction < 0 ? 2 : 1)) % 3; return;
    case FastLoading:
        settings.unthrottle = !settings.unthrottle;
        if (!settings.unthrottle) console::portable::Unthrottle(false);
        break;
    default: return;
    }
    TrackChanges(previous);
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
    if (option == PcCheats && !CheatsAvailable())
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
void DrawValue(SafetyMipsContext& regs) {
    // a0: row action; s6: row index; s4: value text; s2/fp: value position;
    // s7: value alignment. Unknown actions skip the native value switch.
    const int action = int16_t(regs.a0);
    if (!Ours(action) || Link(action)) return;
    const auto& entry = Display().entries[unsigned(regs.s6) % 15];
    const uintptr_t text = *reinterpret_cast<uint32_t*>(Address<0x8B3607C>());
    if (!text) return;
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
    if (*reinterpret_cast<const uint8_t*>(menu + 0x131)) ResetCheatInput();
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
    case HoldCamera:
        help = u"The vehicle camera stays where the right stick turned it";
        second = u"until the stick rests for two seconds.";
        break;
    case FastLoading: help = u"Temporarily remove the emulator speed limit while loading."; break;
    case SkipIntro: help = u"Skip the opening intro on the next game start."; break;
    case ModernControls:
        help = u"Use shoulder buttons for driving, aiming and shooting.";
        second = u"Restart the game to apply the control scheme.";
        break;
    case PcCheats:
        help = u"Type PC cheat words in gameplay. See the hub.";
        if (!CheatsAvailable()) second = u"Unavailable on this game version. See the PPSSPP log.";
        break;
    case IconSet:
        help = u"Match the artwork to your PPSSPP bindings.";
        second = u"Missing or incompatible art stays original.";
        break;
    case AspectRatio: help = u"Auto follows PPSSPP. Choose a ratio to override it."; break;
    case HudConstraint: help = u"Keep the HUD in a centered box. Auto fills."; break;
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
    if (!dirtySettings) return;
    using console::portable::IniValue;
    IniValue values[SettingCount] = {
        {"MAIN", "FPSLimit", {}}, {"MAIN", "UnthrottleEmuDuringLoading", {}},
        {"MAIN", "HudScale", {}}, {"RADAR", "RadarScale", {}}, {"MAIN", "FOVFactor", {}},
        {"MAIN", "CutsceneBorders", {}},
        {"MAIN", "SkipIntro", {}}, {"CONTROLS", "ModernControlScheme", {}}, {"MPRADAR", "RadarScale", {}},
        {"MAIN", "ForceAspectRatio", {}}, {"MAIN", "HudAspectRatioConstraint", {}},
        {"CONTROLS", "PCCheats", {}}, {"MAIN", "GamepadIcons", {}}, {"MAIN", "TextLanguage", {}},
        {"MAIN", "VehicleLights", {}}, {"CONTROLS", "HoldVehicleCamera", {}}};
    std::strcpy(values[SaveRate].value, settings.fps ? "60" : "30");
    std::strcpy(values[SaveLoading].value, settings.unthrottle ? "1" : "0");
    console::decimal(values[SaveHud].value, Drawing::settings.hud, 2);
    console::decimal(values[SaveRadar].value, Drawing::settings.radar, 2);
    console::decimal(values[SaveFov].value, settings.fov, 2);
    values[SaveBorders].value[0] = char('0' + (settings.cutsceneBorders & 3)); values[SaveBorders].value[1] = 0;
    std::strcpy(values[SaveIntro].value, settings.skipIntro ? "1" : "0");
    std::strcpy(values[SaveModern].value, settings.modernControls ? "1" : "0");
    console::decimal(values[SaveMultiplayerRadar].value, Drawing::settings.multiplayerRadar, 2);
    if (autoAspect) std::strcpy(values[SaveAspect].value, "auto");
    else if (const auto index = AspectIndex(Drawing::settings.aspect)) std::strcpy(values[SaveAspect].value, aspectNames[index]);
    else console::format(values[SaveAspect].value, "", Drawing::settings.aspect, 4);
    if (Drawing::settings.hudAspect == 0.0f) std::strcpy(values[SaveHudAspect].value, "Auto");
    else if (const auto index = AspectIndex(Drawing::settings.hudAspect)) std::strcpy(values[SaveHudAspect].value, aspectNames[index]);
    else console::format(values[SaveHudAspect].value, "", Drawing::settings.hudAspect, 4);
    std::strcpy(values[SaveCheats].value, settings.pcCheats ? "1" : "0");
    std::strcpy(values[SaveIcons].value, iconSets[settings.icons]);
    std::strcpy(values[SaveLanguage].value, settings.textLanguage == TextLanguage::Auto ? "auto" : "en");
    std::strcpy(values[SaveLights].value, settings.vehicleLights ? "1" : "0");
    std::strcpy(values[SaveHold].value, settings.holdVehicleCamera ? "1" : "0");
    static console::portable::IniWriter<> writer;
    unsigned count = 0;
    for (unsigned i = 0; i < SettingCount; ++i)
        if (dirtySettings & (1u << i))
            values[count++] = values[i];

    if (writer.update(iniPath, values, count))
        dirtySettings = 0;
}
}
void TickMenu() {
    const bool active = Field<uint8_t>(activeOffset) != 0;
    if (menuWasActive && !active && submenu)
        Show(false);

    const uint32_t now = sceKernelGetSystemTimeLow();
    if (!active && dirtySettings && (menuWasActive || now - lastSaveAttempt >= 1000000u))
    {
        lastSaveAttempt = now;
        Save();
    }
    menuWasActive = active;
}

void ReadSettingsPreservingMenuEdits(void (*read)())
{
    const CurrentSettings previous;
    read();
    VisitSavedSettings(previous, [](Setting setting, auto& value, const auto& old)
    {
        if (dirtySettings & (1u << setting))
            value = old;
    });
    if (dirtySettings & (1u << SaveAspect))
    {
        autoAspect = previous.automaticAspect;
        Drawing::settings.aspect = previous.drawing.aspect;
    }
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
