#include "Game.hpp"
#include "MenuText.hpp"
#include "../Shared/Console/PortableIni.hpp"
#include "../Shared/Console/Text.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdlib>
extern "C" {
#include <psputility_sysparam.h>
}

// Plugins page in the pause menu. The pause list is the game's own menu list:
// rows are added through its row builder and every row calls MenuAction with
// its id. The list keeps three row groups (+700 written, +704 shown); a page
// is built into the written group and the game shows it after the callback.
namespace mc3 {
namespace {
using menu::Word;
constexpr char cfg[] = "ms0:/PSP/PLUGINS/MidnightClubLARemix.PPSSPP.FusionFix/MidnightClubLARemix.PPSSPP.FusionFix.cfg";
struct Delegate { uintptr_t object; int adjustment; uintptr_t function; int argument; int kind; };
struct SavedRow { Delegate action; int id, high, flags; char text[4][96]; };
using AddRow = injector::game_fn<uintptr_t(uintptr_t, int, int, int, int, const Delegate*)>;
using SetText = injector::game_fn<void(uintptr_t, uintptr_t, int, const char*)>;
using Clear = injector::game_fn<void(uintptr_t, int)>;
SafetyMipsInline pauseHook;
uintptr_t managerSlot, callback, pageObject;
AddRow addRow;
SetText setText;
Clear clear;
SavedRow saved[16];
unsigned savedCount;

enum Page { Native, Root, Settings, Presets, HelpTopics, HelpText };
enum Id { None, Open = 100, Help, More, SettingRow = 200, PresetRow = 300, TopicRow = 400, Return = 900 };
// Settings page rows after the preset row.
enum Row { Fov, HudBox, HudSize, Borders, Skip, Analog, RowCount };
constexpr Word rowLabels[RowCount]{ Word::FOVFactor, Word::HudAspectRatioConstraint, Word::HUDScale, Word::CutsceneBorders,
                                    Word::SkipIntro, Word::DualAnalogPatch };
constexpr Word rowHelp[RowCount]{ Word::FOVFactorHelp, Word::HudAspectRatioConstraintHelp, Word::HUDScaleHelp, Word::CutsceneBordersHelp,
                                  Word::SkipIntroHelp, Word::DualAnalogPatchHelp };
constexpr const char* boxes[]{ "Auto", "4:3", "16:9", "21:9", "32:9" };
constexpr float boxAspects[]{ 0.0f, 4.0f / 3.0f, 16.0f / 9.0f, 21.0f / 9.0f, 32.0f / 9.0f };
constexpr const char* languageIds[]{ "auto", "en", "fr", "de", "it", "es" };
constexpr const char* languageNames[]{ "", "English", "Fran\347ais", "Deutsch", "Italiano", "Espa\361ol" };

Page page;
int topic, helpPage, preset, textLanguage, systemLanguage;
bool startSkip, startAnalog, saveError;
int pendingSelect = -1;
unsigned pendingCount, pendingFrames;
int rowIds[16];
unsigned rowCount;

int CurrentLanguage() { return textLanguage ? textLanguage - 1 : systemLanguage; }
const char* Label(Word word) { return menu::labels[CurrentLanguage()][unsigned(word)]; }
uintptr_t List() {
    const auto manager = at<uintptr_t>(managerSlot);
    const auto pages = at<uintptr_t>(manager + 12);
    return at<uintptr_t>(pages + 52 * 4);
}
// The pause panel fits ten rows on screen.
unsigned MaxRows(uintptr_t list) { return std::min(at<unsigned>(list + 660), 10u); }
int PresetOf() { return skipIntro != dualAnalog || cutsceneBorders != (skipIntro ? 3 : 1) ? 3 : skipIntro ? 1 : 0; }
int BoxIndex() {
    for (int i = 1; i < 5; ++i)
        if (std::fabs(hudAspect - boxAspects[i]) < 0.01f) return i;
    return 0;
}

void Save(const char* key, const char* value) {
    IniValue entry{"MAIN", key, {}};
    console::Text<sizeof(entry.value)> text(entry.value);
    text.append(value);
    IniWriter<> writer;
    saveError = !writer.update(cfg, &entry, 1);
}
void SaveFloat(const char* key, float value) {
    char buffer[16];
    console::Text<sizeof(buffer)> text(buffer);
    text.decimal(value, 2);
    Save(key, buffer);
}

void Select(uintptr_t list, unsigned group, unsigned index) {
    const auto rows = at<uintptr_t>(list + 684);
    const auto row = rows + 16 * (group * at<unsigned>(list + 660) + index);
    at<unsigned>(list + 720 + group * 16) = index;
    at<uint8_t>(list + 724 + group * 16) = 1;
    at<uint32_t>(list + 728 + group * 16) = at<uint32_t>(row);
    at<uint32_t>(list + 732 + group * 16) = at<uint32_t>(row + 4);
}

void MenuAction(uintptr_t, int action);
void Add(uintptr_t list, int id, const char* text) {
    if (rowCount >= MaxRows(list)) return;
    Delegate action{pageObject, 0, callback, id, 2};
    const auto row = addRow(list, 0, id, 0, 0, &action);
    setText(list, row, 2, text);
    rowIds[rowCount++] = id;
}
void AddValue(uintptr_t list, int id, const char* label, const char* value) {
    char line[96];
    console::Text<sizeof(line)> text(line);
    text.append(label);
    text.append(": ");
    text.append(value);
    Add(list, id, line);
}
// Help text split into lines that fit the pause panel; long texts continue
// on further pages.
constexpr unsigned helpLines = 7;
bool Paragraph(uintptr_t list, const char* text, unsigned skip = 0, unsigned lines = 16) {
    constexpr unsigned width = 34;
    unsigned line = 0;
    while (*text) {
        unsigned length = 0, cut = 0;
        while (text[length] && length < width) {
            if (text[length] == ' ') cut = length;
            ++length;
        }
        if (text[length] && cut) length = cut;
        if (line >= skip + lines || rowCount + 1 >= MaxRows(list)) return true;
        if (line++ >= skip) {
            char row[width + 1]{};
            std::memcpy(row, text, length);
            Add(list, None, row);
        }
        text += length;
        while (*text == ' ') ++text;
    }
    return false;
}

void Snapshot(uintptr_t list, unsigned group) {
    const auto count = at<unsigned>(list + 668 + group * 4);
    const auto maxRows = at<unsigned>(list + 660);
    const auto columns = std::min(at<unsigned>(list + 664), 4u);
    const auto rows = at<uintptr_t>(list + 684);
    savedCount = std::min(count, 16u);
    for (unsigned i = 0; i < savedCount; ++i) {
        auto& out = saved[i];
        out.action = at<Delegate>(list + 256 + 20 * i);
        const auto row = rows + 16 * (group * maxRows + i);
        out.id = at<int>(row); out.high = at<int>(row + 4); out.flags = at<int>(row + 12);
        const auto cells = at<uintptr_t>(row + 8);
        for (unsigned c = 0; c < 4; ++c) {
            const char* text = c < columns ? reinterpret_cast<const char*>(at<uintptr_t>(cells + c * 8)) : nullptr;
            console::Text<sizeof(out.text[c])> copy(out.text[c]);
            copy.append(text ? text : "");
        }
    }
}

// Shows the selection once the game has put the new page on screen.
void Finish(uintptr_t list, unsigned group, int selectId) {
    unsigned index = 0;
    for (unsigned i = 0; i < rowCount; ++i)
        if (rowIds[i] == selectId) index = i;
    Select(list, group, index);
    pendingSelect = int(index);
    pendingCount = rowCount;
    pendingFrames = 0;
}

void Restore(uintptr_t list, unsigned group) {
    at<unsigned>(list + 700) = group;
    clear(list, 1);
    rowCount = 0;
    for (unsigned i = 0; i < savedCount; ++i) {
        auto& old = saved[i];
        const auto row = addRow(list, 0, old.id, old.high, old.flags, &old.action);
        for (unsigned c = 0; c < 4; ++c)
            if (old.text[c][0]) setText(list, row, c, old.text[c]);
        rowIds[rowCount++] = old.id;
    }
    page = Native;
    Finish(list, group, rowIds[savedCount ? savedCount - 1 : 0]);
}

const char* Value(int row, char (&buffer)[32]) {
    console::Text<sizeof(buffer)> text(buffer);
    switch (row) {
        case Fov: text.fixed(fovFactor, 2); break;
        case HudBox: { const int box = BoxIndex(); text.append(box ? boxes[box] : Label(Word::Auto)); break; }
        case HudSize: text.integer(uint32_t(hudScale * 100.0f + 0.5f)); text.character('%'); break;
        case Borders: text.append(Label(Word(unsigned(Word::BordersOff) + unsigned(cutsceneBorders)))); break;
        case Skip: text.append(Label(skipIntro ? Word::On : Word::Off)); break;
        default: text.append(Label(dualAnalog ? Word::On : Word::Off)); break;
    }
    return buffer;
}

void Build(uintptr_t list, unsigned group, int selectId) {
    at<unsigned>(list + 700) = group;
    clear(list, 1);
    rowCount = 0;
    char value[32];
    switch (page) {
        case Root:
            Add(list, Open, "Midnight Club L.A. Remix Fusion Fix");
            Add(list, Help, Label(Word::Help));
            break;
        case Settings:
            AddValue(list, SettingRow + RowCount, Label(Word::Preset),
                     Label(preset == 3 ? Word::Custom : Word(unsigned(Word::Vanilla) + unsigned(preset))));
            for (int row = 0; row < RowCount; ++row)
                AddValue(list, SettingRow + row, Label(rowLabels[row]), Value(row, value));
            if (skipIntro != startSkip || dualAnalog != startAnalog) Paragraph(list, Label(Word::Restart));
            if (saveError) Paragraph(list, Label(Word::SaveError));
            break;
        case Presets:
            for (int i = 0; i < 3; ++i)
                Add(list, PresetRow + i, Label(Word(unsigned(Word::Vanilla) + unsigned(i))));
            break;
        case HelpTopics:
            Add(list, TopicRow + RowCount, Label(Word::Preset));
            for (int row = 0; row < RowCount; ++row)
                Add(list, TopicRow + row, Label(rowLabels[row]));
            break;
        default:
            Add(list, None, Label(topic == RowCount ? Word::Preset : rowLabels[topic]));
            if (Paragraph(list, Label(topic == RowCount ? Word::PresetHelp : rowHelp[topic]), helpPage * helpLines, helpLines))
                Add(list, More, "...");
            break;
    }
    Add(list, Return, Label(Word::Back));
    Finish(list, group, selectId);
}

void Change(int row) {
    switch (row) {
        case Fov:
            fovFactor = fovFactor >= 1.99f ? 0.5f : std::min(2.0f, std::round(fovFactor * 10.0f + 1.0f) / 10.0f);
            SaveFloat("FOVFactor", fovFactor);
            break;
        case HudBox: {
            const int box = (BoxIndex() + 1) % 5;
            hudAspect = boxAspects[box];
            Save("HudAspectRatioConstraint", boxes[box]);
            break;
        }
        case HudSize:
            hudScale = hudScale >= 1.49f ? 0.5f : std::min(1.5f, std::round(hudScale * 10.0f + 1.0f) / 10.0f);
            SaveFloat("HudScale", hudScale);
            break;
        case Borders: {
            cutsceneBorders = (cutsceneBorders + 1) % 4;
            char digit[2]{ char('0' + cutsceneBorders), 0 };
            Save("CutsceneBorders", digit);
            preset = PresetOf();
            break;
        }
        case Skip:
            skipIntro = !skipIntro;
            Save("SkipIntro", skipIntro ? "1" : "0");
            preset = PresetOf();
            break;
        default:
            dualAnalog = !dualAnalog;
            Save("DualAnalogPatch", dualAnalog ? "1" : "0");
            preset = PresetOf();
            break;
    }
}

void ApplyPreset(int value) {
    preset = value;
    skipIntro = dualAnalog = value != 0;
    Save("SkipIntro", skipIntro ? "1" : "0");
    if (!saveError) Save("DualAnalogPatch", dualAnalog ? "1" : "0");
    cutsceneBorders = value == 0 ? 1 : 3;
    const char border[]{ char('0' + cutsceneBorders), 0 };
    if (!saveError) Save("CutsceneBorders", border);
}

void MenuAction(uintptr_t, int action) {
    const auto list = List();
    const unsigned group = at<unsigned>(list + 700);
    int select = Return;
    switch (page) {
        case Native:
            Snapshot(list, at<unsigned>(list + 704));
            page = Root;
            select = Open;
            break;
        case Root:
            if (action == Return) { Restore(list, group); return; }
            if (action == Help) { page = HelpTopics; select = TopicRow + RowCount; }
            else { page = Settings; select = SettingRow + RowCount; }
            break;
        case Settings:
            if (action == Return) { page = Root; select = Open; }
            else if (action == SettingRow + RowCount) { page = Presets; select = PresetRow + (preset == 3 ? 1 : preset); }
            else if (action >= SettingRow && action < SettingRow + RowCount) { Change(action - SettingRow); select = action; }
            else select = action;
            break;
        case Presets:
            if (action >= PresetRow && action < PresetRow + 3) ApplyPreset(action - PresetRow);
            page = Settings;
            select = SettingRow + RowCount;
            break;
        case HelpTopics:
            if (action == Return) { page = Root; select = Help; break; }
            topic = action - TopicRow;
            helpPage = 0;
            page = HelpText;
            break;
        default:
            if (action == More) { ++helpPage; select = Return; }
            else if (action != Return) select = action;
            else { page = HelpTopics; select = TopicRow + topic; }
            break;
    }
    Build(list, group, select);
}

int Pause(uintptr_t object, uint8_t argument) {
    const int result = pauseHook.call<int>(object, argument);
    page = Native; pageObject = object;
    const auto list = List();
    const unsigned group = at<unsigned>(list + 700);
    const unsigned count = at<unsigned>(list + 668 + group * 4);
    if (count < MaxRows(list)) {
        Delegate action{pageObject, 0, callback, Open, 2};
        const auto row = addRow(list, 0, int(count), 0, 0, &action);
        setText(list, row, 2, Label(Word::Plugins));
    }
    return result;
}
}

void RefreshDisplayOptions()
{
    if (page != Native || saveError) return;
    static uint32_t lastRead;
    const uint32_t now = sceKernelGetSystemTimeLow();
    if (lastRead && now - lastRead < 1000000u) return;
    lastRead = now;
    const SceUID file = sceIoOpen(cfg, PSP_O_RDONLY, 0);
    if (file < 0) return;
    static char contents[8192];
    const int size = sceIoRead(file, contents, sizeof(contents) - 1);
    sceIoClose(file);
    if (size <= 0 || size == int(sizeof(contents) - 1)) return;
    contents[size] = '\0';
    char forced[64]{}, box[64]{}, fov[32]{}, scale[32]{}, borders[32]{};
    if (!rini_get_key("MAIN", "ForceAspectRatio", contents, size, forced, sizeof(forced), STRING_VAL) ||
        !rini_get_key("MAIN", "HudAspectRatioConstraint", contents, size, box, sizeof(box), STRING_VAL) ||
        !rini_get_key("MAIN", "FOVFactor", contents, size, fov, sizeof(fov), STRING_VAL) ||
        !rini_get_key("MAIN", "HudScale", contents, size, scale, sizeof(scale), STRING_VAL) ||
        !rini_get_key("MAIN", "CutsceneBorders", contents, size, borders, sizeof(borders), STRING_VAL)) return;
    char* end = nullptr;
    const float nextFov = std::strtof(fov, &end);
    if (end == fov || !std::isfinite(nextFov)) return;
    const float nextScale = std::strtof(scale, &end);
    if (end == scale || !std::isfinite(nextScale)) return;
    const long nextBorders = std::strtol(borders, &end, 10);
    if (end == borders) return;
    const float nextAspect = console::parse_forced_aspect(forced);
    automaticAspect = nextAspect <= 0.0f;
    if (!automaticAspect) targetAspect = nextAspect;
    RefreshAspect();
    hudAspect = console::parse_hud_aspect(box);
    fovFactor = std::clamp(nextFov, 0.5f, 2.0f);
    hudScale = std::clamp(nextScale, 0.5f, 1.5f);
    cutsceneBorders = int(std::clamp(nextBorders, 0l, 3l));
    preset = PresetOf();
}

// Called while a menu page draws: the game picks the selection of a new page
// from the previous one, so the intended row is selected once it shows.
void MenuFrame() {
    if (pendingSelect < 0) return;
    const auto list = List();
    const unsigned group = at<unsigned>(list + 704);
    if (at<unsigned>(list + 668 + group * 4) == pendingCount) {
        Select(list, group, unsigned(pendingSelect));
        pendingSelect = -1;
    } else if (++pendingFrames > 30) {
        pendingSelect = -1;
    }
}

void ReadMenuOptions() {
    char buffer[16]{};
    const char* language = inireader.ReadString("MAIN", "TextLanguage", "auto", buffer, sizeof(buffer));
    for (int i = 0; i < 6; ++i)
        if (language && !std::strcmp(language, languageIds[i])) textLanguage = i;
    int system = 0;
    sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_LANGUAGE, &system);
    switch (system) {
        case PSP_SYSTEMPARAM_LANGUAGE_FRENCH: systemLanguage = 1; break;
        case PSP_SYSTEMPARAM_LANGUAGE_GERMAN: systemLanguage = 2; break;
        case PSP_SYSTEMPARAM_LANGUAGE_ITALIAN: systemLanguage = 3; break;
        case PSP_SYSTEMPARAM_LANGUAGE_SPANISH: systemLanguage = 4; break;
        default: systemLanguage = 0; break;
    }
    startSkip = skipIntro;
    startAnalog = dualAnalog;
    preset = PresetOf();
}

void InstallNativeMenu() {
    const auto pause = pattern.get_first("90 FE BD 27 ?? ?? 86 8F 4C 01 B3 AF 0C 00 D3 8C 5C 01 B7 AF 25 B8 80 00", 0);
    managerSlot = gameGP + int16_t(injector::ReadMemory<uint16_t>(pause + 4));
    addRow = AddRow(pattern.get_first("A0 FF BD 27 38 00 B3 AF 25 98 80 00 00 00 24 8D 2C 00 B0 AF 25 80 C0 00 04 00 25 8D", 0));
    setText = SetText(pattern.get_first("E0 FF BD 27 00 00 B0 AF 04 00 B1 AF 25 80 80 00 25 88 A0 00 08 00 24 8E 21 28 C6 00", 0));
    clear = Clear(pattern.get_first("90 02 86 90 21 00 C0 10 FF 00 A5 30 BC 02 86 8C 80 38 06 00 21 38 87 00", 0));
    callback = injector::detail::callback_address(&MenuAction);
    pauseHook = safetymips::create_inline(pause, Pause);
}
}
