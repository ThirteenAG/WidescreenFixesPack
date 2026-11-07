#pragma once
#include "MenuText.hpp"

namespace NativeMenu
{
inline constexpr const char* cfg = "ms0:/PSP/PLUGINS/SOCOM.FireteamBravo2.PPSSPP.FusionFix/SOCOM.FireteamBravo2.PPSSPP.FusionFix.cfg";
inline IniValue values[]
{
    { "MAIN", "SkipIntro", "1" }, { "MAIN", "DualAnalogPatch", "1" },
    { "MAIN", "UnthrottleEmuDuringLoading", "1" }, { "MAIN", "FOVFactor", "1" },
    { "MAIN", "ForceAspectRatio", "Auto" }, { "MAIN", "HudAspectRatioConstraint", "Auto" },
    { "MAIN", "HudScale", "1" }, { "MAIN", "CutsceneBorders", "3" }, { "MAIN", "TextLanguage", "auto" },
    { "MAIN", "GamepadIcons", "0" }, { "MAIN", "FPSLimit", "60" },
};
inline constexpr unsigned settingCount = sizeof(values) / sizeof(values[0]);
inline constexpr Word rowLabels[]{ Word::Skip, Word::Analog, Word::Loading, Word::Fov, Word::Aspect, Word::Hudaspect, Word::Hudscale, Word::Borders, Word::Language, Word::Icons, Word::Fps };
inline constexpr Word rowHelp[]{ Word::SkipHelp, Word::AnalogHelp, Word::LoadingHelp, Word::FovHelp, Word::AspectHelp, Word::HudaspectHelp, Word::HudscaleHelp, Word::BordersHelp, Word::LanguageHelp, Word::IconsHelp, Word::FpsHelp };
inline constexpr const char* ratios[]{ "Auto", "4:3", "16:9", "21:9", "32:9" };
inline constexpr const char* languages[]{ "auto", "en", "fr", "de", "it", "es" };
inline int ticks[settingCount], systemLanguage, level, helpTopic{ -1 }, helpParent{ 1 }, preset{ 1 };
inline char customRatios[2][16];
inline float customAspects[2];
inline bool hasCustom[2];
inline bool pending[settingCount], saveError, draining;
inline uint32_t previousButtons, heldDirection, repeatAt;
inline uintptr_t header;
struct Row { char text[128]; int (*callback)(int); };
static_assert(sizeof(Row) == 132);
inline Row rows[192], parentRows[9];
inline int commands[192];
inline unsigned count, parentCount;
inline uint8_t savedHeader[36];
inline SafetyMipsInline showHook, closeHook, dispatchHook;
inline injector::hook_back<int(uintptr_t, int)> show, dispatch;
inline injector::hook_back<int(uintptr_t)> close;
inline injector::game_fn<int(uintptr_t, int)> reset;
inline uint32_t resyncButton;
inline int resyncTarget, resyncFrames;
inline void Build(bool rewind = true);
inline void Back();
inline int Selected() { return *reinterpret_cast<const int*>(header + 60); }
// The pause movie moves its selection frame only after its own up/down
// request. Select a neighbour row and let a pad press step onto the target.
inline void Resync(int target)
{
    resyncButton = 0;
    const int total = *reinterpret_cast<const uint16_t*>(header + 74);
    const int visible = *reinterpret_cast<const int*>(header + 64);
    if (total < 2 || visible < 1 || target >= total)
        return;
    const int from = target ? target - 1 : 1;
    const int slot = std::min(from, visible - 1);
    *reinterpret_cast<int*>(header + 52) = from - slot;
    *reinterpret_cast<int*>(header + 56) = slot;
    *reinterpret_cast<int*>(header + 60) = from;
    resyncButton = target ? PSP_CTRL_DOWN : PSP_CTRL_UP;
    resyncTarget = target;
    resyncFrames = 30;
}
// The movie may ignore input while a page changes: press until it has moved.
inline void Press(console::socom::Input* input)
{
    if (!resyncButton)
        return;
    if (Selected() == resyncTarget || --resyncFrames < 0)
    {
        resyncButton = 0;
        return;
    }
    if (resyncFrames & 1)
        input->pad.Buttons |= resyncButton;
    else
        input->pad.Buttons &= ~resyncButton;
    input->changed |= resyncButton;
}
inline int Language() { return ticks[8] ? ticks[8] - 1 : systemLanguage; }
inline const char* Label(Word word) { return labels[Language()][unsigned(word)]; }
inline void MenuUnavailable()
{
    static bool reported;
    if (reported) return;
    reported = true;
    static constexpr const char* messages[] = {
        "Fusion Fix: plugin menu is incomplete; menu data unsupported.\n",
        "Fusion Fix : menu du plugin incomplet ; donnees non prises en charge.\n",
        "Fusion Fix: Plugin-Menue unvollstaendig; Menue-Daten nicht unterstuetzt.\n",
        "Fusion Fix: menu plugin incompleto; dati non supportati.\n",
        "Fusion Fix: menu del plugin incompleto; datos no compatibles.\n"
    };
    PatchNotice(messages[Language()]);
}
inline bool RestartNeeded()
{
    return ticks[0] != options.skipIntro || ticks[1] != options.dualAnalog || ticks[2] != options.unthrottle;
}
inline bool Save()
{
    IniValue changed[settingCount]{};
    unsigned total{};
    for (unsigned i = 0; i < settingCount; ++i)
        if (pending[i])
            changed[total++] = values[i];
    if (!total)
        return true;
    IniWriter<> writer;
    saveError = !writer.update(cfg, changed, total);
    if (!saveError)
        std::fill(pending, pending + settingCount, false);
    return !saveError;
}
inline const char* Value(unsigned row, char (&buffer)[64])
{
    if (row < 3)
        return Label(ticks[row] ? Word::On : Word::Off);
    if (row == 3 || row == 6)
    {
        console::Text<64> text(buffer);
        text.fixed(float(ticks[row]) / 20, 2);
        return buffer;
    }
    if (row == 4 || row == 5)
        return ticks[row] == 5 ? customRatios[row - 4] : ticks[row] ? ratios[ticks[row]] : Label(Word::Auto);
    if (row == 7)
        return Label(Word(unsigned(Word::BordersOff) + unsigned(ticks[row])));
    if (row == 9)
    {
        constexpr const char* styles[]{ "", "Xbox", "PlayStation", "Nintendo" };
        return ticks[row] ? styles[ticks[row]] : Label(Word::Auto);
    }
    if (row == 10)
        return ticks[row] ? "60 FPS" : "30 FPS";
    return ticks[row] ? languages[ticks[row]] : Label(Word::Auto);
}
inline void Store(unsigned row)
{
    auto& output = values[row].value;
    if (row < 3)
        std::strcpy(output, ticks[row] ? "1" : "0");
    else if (row == 3 || row == 6)
    {
        console::Text<sizeof(output)> text(output);
        text.fixed(float(ticks[row]) / 20, 2);
    }
    else if (row == 4 || row == 5)
    {
        std::strcpy(output, ticks[row] == 5 ? customRatios[row - 4] : ratios[ticks[row]]);
    }
    else if (row == 10)
        std::strcpy(output, ticks[row] ? "60" : "30");
    else if (row == 7 || row == 9)
    {
        output[0] = char('0' + ticks[row]);
        output[1] = 0;
    }
    else
        std::strcpy(output, languages[ticks[row]]);
    pending[row] = true;
}
inline void Apply(unsigned row)
{
    constexpr float aspects[]{ 0, 4.0f / 3, 16.0f / 9, 21.0f / 9, 32.0f / 9 };
    if (row == 3)
        Geometry::fovFactor = float(ticks[row]) / 20;
    else if (row == 4 || row == 5)
    {
        // The custom ratio was parsed at start.
        const float aspect = ticks[row] == 5 ? customAspects[row - 4] : aspects[ticks[row]];
        if (row == 4)
            Geometry::forcedAspect = aspect;
        else
            Hud::constraint = aspect;
    }
    else if (row == 6)
        Hud::scale = float(ticks[row]) / 20;
    else if (row == 7)
        Geometry::borders = ticks[row];
    else if (row == 9)
        PadIcons::style = ticks[row];
    else if (row == 10)
        FrameRate::limit = ticks[row] ? 60 : 30;
}
inline void Change(unsigned row, int direction)
{
    if (row < 3 || row == 10)
        ticks[row] ^= 1;
    else if (row == 3 || row == 6)
        ticks[row] = std::clamp(ticks[row] + direction, 10, row == 3 ? 40 : 30);
    else
    {
        const int limit = row == 7 || row == 9 ? 4 : row == 8 ? 6 : hasCustom[row - 4] ? 6 : 5;
        ticks[row] = (ticks[row] + direction + limit) % limit;
    }
    Store(row);
    Apply(row);
    if ((row < 3 || row == 7 || row == 9 || row == 10) && preset != 3)
    {
        const bool vanilla = preset == 0;
        if (ticks[0] != !vanilla || ticks[1] != !vanilla || ticks[2] != !vanilla ||
            ticks[7] != (vanilla ? 1 : 3) || ticks[9] != (vanilla ? 2 : 0) || ticks[10] != !vanilla)
            preset = 3;
    }
    Save();
    Build(false);
}
inline void Preset(int value)
{
    preset = value;
    for (unsigned i = 0; i < 3; ++i)
        if (ticks[i] != (value != 0))
        {
            ticks[i] = value != 0;
            Store(i);
        }
    const int borders = value == 0 ? 1 : 3;
    if (ticks[7] != borders)
    {
        ticks[7] = borders;
        Store(7);
        Apply(7);
    }
    const int icons = value == 0 ? 2 : 0;
    if (ticks[9] != icons)
    {
        ticks[9] = icons;
        Store(9);
        Apply(9);
    }
    const int fps = value != 0;
    if (ticks[10] != fps)
    {
        ticks[10] = fps;
        Store(10);
        Apply(10);
    }
    Save();
    level = 2;
    Build();
}
inline int Select(int index)
{
    if (index < 0 || unsigned(index) >= count)
        return 0;
    const int command = commands[index];
    if (command >= 100 && command < 100 + int(settingCount))
        Change(unsigned(command - 100), 1);
    else if (command >= 200 && command < 203)
        Preset(command - 200);
    else if (command == 4)
        Back();
    else if (command)
    {
        helpParent = level;
        level = command;
        helpTopic = -1;
        Build();
    }
    return 0;
}
inline void Add(const char* text, int command)
{
    if (count >= 192)
    {
        MenuUnavailable();
        return;
    }
    console::Text<128> output(rows[count].text);
    output.append(text);
    rows[count].callback = Select;
    commands[count++] = command;
}
inline void Paragraph(const char* text)
{
    while (*text)
    {
        char line[128]{};
        unsigned bytes{}, glyphs{}, space{};
        while (text[bytes] && glyphs < 52 && bytes < 120)
        {
            const unsigned char first = uint8_t(text[bytes]);
            const unsigned length = first < 0x80 ? 1 : first < 0xE0 ? 2 : first < 0xF0 ? 3 : 4;
            if (text[bytes] == ' ')
                space = bytes;
            bytes += length;
            ++glyphs;
        }
        if (text[bytes] && space)
            bytes = space;
        std::memcpy(line, text, bytes);
        Add(line, 0);
        text += bytes;
        while (*text == ' ')
            ++text;
    }
}
inline void Build(bool rewind)
{
    count = 0;
    if (level == 1)
    {
        Add("SOCOM Fireteam Bravo 2 Fusion Fix", 2);
        Add(Label(Word::Help), 5);
    }
    else if (level == 2)
    {
        char label[128]{};
        console::Text<128> text(label);
        text.append(Label(Word::Preset));
        text.append(": ");
        text.append(Label(preset == 3 ? Word::Custom : Word(unsigned(Word::Vanilla) + unsigned(preset))));
        Add(label, 3);
        for (unsigned row = 0; row < settingCount; ++row)
        {
            char value[64]{};
            console::Text<128> line(label);
            line.append(Label(rowLabels[row]));
            line.append(": ");
            line.append(Value(row, value));
            Add(label, 100 + int(row));
        }
        Add(Label(Word::Help), 5);
        if (RestartNeeded())
            Add(Label(Word::Restart), 0);
        if (saveError)
            Add(Label(Word::SaveError), 0);
    }
    else if (level == 3)
    {
        for (int i = 0; i < 3; ++i)
            Add(Label(Word(unsigned(Word::Vanilla) + unsigned(i))), 200 + i);
    }
    else
    {
        // Help pages are text only: Back comes first so the frame starts on it.
        Add(Label(Word::Back), 4);
        if (helpTopic < 0)
        {
            Paragraph(Label(Word::PresetHelp));
            for (unsigned row = 0; row < settingCount; ++row)
            {
                Add(Label(rowLabels[row]), 0);
                Paragraph(Label(rowHelp[row]));
            }
        }
        else
        {
            Add(Label(rowLabels[helpTopic]), 0);
            Paragraph(Label(rowHelp[helpTopic]));
        }
    }
    if (level != 5)
        Add(Label(Word::Back), 4);
    *reinterpret_cast<uint8_t*>(header + 72) = 2;
    *reinterpret_cast<uint16_t*>(header + 74) = uint16_t(count);
    *reinterpret_cast<uintptr_t*>(header + 76) = uintptr_t(rows);
    *reinterpret_cast<uint32_t*>(header + 80) = sizeof(Row);
    if (rewind)
    {
        reset(header, 1);
        Resync(0);
    }
}
inline void Back()
{
    if (!Save())
    {
        Build(false);
        return;
    }
    if (level == 1)
    {
        std::memcpy(reinterpret_cast<void*>(header + 48), savedHeader, sizeof(savedHeader));
        level = 0;
        draining = true;
        Resync(int(parentCount));
    }
    else
    {
        // Help for one setting returns to that setting's row.
        const int row = level == 5 && helpParent == 2 && helpTopic >= 0 ? helpTopic + 1 : 0;
        level = level == 5 ? helpParent : level == 3 ? 2 : 1;
        Build();
        if (row)
            Resync(row);
    }
}
inline int Dispatch(uintptr_t object, int index)
{
    if (object == header)
    {
        if (level)
            return Select(index);
        if (index == int(parentCount))
        {
            std::memcpy(savedHeader, reinterpret_cast<const void*>(header + 48), sizeof(savedHeader));
            level = 1;
            Build();
            return 0;
        }
    }
    return dispatch.fun(object, index);
}
inline int Show(uintptr_t object, int enabled)
{
    const int result = show.fun(object, enabled);
    if (enabled && !level)
    {
        parentCount = *reinterpret_cast<const uint16_t*>(header + 74);
        if (parentCount > 8)
        {
            MenuUnavailable();
            return result;
        }
        const auto original = *reinterpret_cast<const Row* const*>(header + 76);
        std::memcpy(parentRows, original, parentCount * sizeof(Row));
        console::Text<128> text(parentRows[parentCount].text);
        text.append(Label(Word::Plugins));
        parentRows[parentCount].callback = nullptr;
        *reinterpret_cast<uint16_t*>(header + 74) = uint16_t(parentCount + 1);
        *reinterpret_cast<uintptr_t*>(header + 76) = uintptr_t(parentRows);
        logger.WriteF("Native pause Plugins row appended after %u original rows.\n", parentCount);
    }
    return result;
}
inline int Close(uintptr_t object)
{
    resyncButton = 0;
    if (level)
    {
        Save();
        std::memcpy(reinterpret_cast<void*>(header + 48), savedHeader, sizeof(savedHeader));
        level = 0;
        draining = true;
    }
    return close.fun(object);
}
inline void Input(console::socom::Input* input)
{
    const uint32_t buttons = input->pad.Buttons;
    uint32_t presses = buttons & ~previousButtons;
    previousButtons = buttons;
    Press(input);
    if (!level && !draining)
        return;
    const uint32_t direction = buttons & (PSP_CTRL_LEFT | PSP_CTRL_RIGHT);
    const uint32_t now = sceKernelGetSystemTimeLow();
    if (direction != heldDirection)
    {
        heldDirection = direction;
        repeatAt = now + 400000;
    }
    else if (direction && int32_t(now - repeatAt) >= 0)
    {
        presses |= direction;
        repeatAt = now + 100000;
    }
    if (level && (presses & PSP_CTRL_CIRCLE))
        Back();
    else if (level == 2 && (presses & PSP_CTRL_TRIANGLE))
    {
        helpTopic = Selected() > 0 && Selected() <= int(settingCount) ? Selected() - 1 : -1;
        helpParent = 2;
        level = 5;
        Build();
    }
    else if (level == 2 && (presses & (PSP_CTRL_LEFT | PSP_CTRL_RIGHT)))
    {
        const int row = Selected() - 1;
        if (row >= 0 && row < int(settingCount))
            Change(unsigned(row), presses & PSP_CTRL_RIGHT ? 1 : -1);
    }
    constexpr uint32_t mask = PSP_CTRL_CIRCLE | PSP_CTRL_TRIANGLE | PSP_CTRL_LEFT | PSP_CTRL_RIGHT;
    input->pad.Buttons &= ~mask;
    input->changed &= ~mask;
    if (!level && !(buttons & mask))
        draining = false;
}
inline void ReadOptions()
{
    for (unsigned i = 0; i < settingCount; ++i)
    {
        char buffer[sizeof(values[i].value)]{};
        std::strcpy(values[i].value, inireader.ReadString(values[i].section, values[i].key, values[i].value, buffer, sizeof(buffer)));
    }
    ticks[0] = options.skipIntro;
    ticks[1] = options.dualAnalog;
    ticks[2] = options.unthrottle;
    ticks[3] = int(std::round(Geometry::fovFactor * 20));
    ticks[6] = int(std::round(Hud::scale * 20));
    ticks[7] = Geometry::borders;
    ticks[9] = PadIcons::style;
    ticks[10] = FrameRate::limit > 30;
    for (unsigned row = 4; row < 6; ++row)
    {
        ticks[row] = 5;
        for (int i = 0; i < 5; ++i)
            if (!std::strcmp(values[row].value, ratios[i]))
                ticks[row] = i;
        if (ticks[row] == 5)
        {
            const float aspect = row == 4 ? Geometry::forcedAspect : Hud::constraint;
            customAspects[row - 4] = aspect;
            hasCustom[row - 4] = aspect > 0;
            std::strcpy(customRatios[row - 4], values[row].value);
            if (!hasCustom[row - 4])
                ticks[row] = 0;
        }
    }
    for (int i = 0; i < 6; ++i)
        if (!std::strcmp(values[8].value, languages[i]))
            ticks[8] = i;
    int language{};
    sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_LANGUAGE, &language);
    switch (language)
    {
        case PSP_SYSTEMPARAM_LANGUAGE_FRENCH: systemLanguage = 1; break;
        case PSP_SYSTEMPARAM_LANGUAGE_GERMAN: systemLanguage = 2; break;
        case PSP_SYSTEMPARAM_LANGUAGE_ITALIAN: systemLanguage = 3; break;
        case PSP_SYSTEMPARAM_LANGUAGE_SPANISH: systemLanguage = 4; break;
        default: systemLanguage = 0; break;
    }
    preset = ticks[0] && ticks[1] && ticks[2] && ticks[9] == 0 && ticks[10] == 1 && ticks[7] == 3 ? 1 :
        !ticks[0] && !ticks[1] && !ticks[2] && ticks[9] == 2 && ticks[10] == 0 && ticks[7] == 1 ? 0 : 3;
}
inline void Install()
{
    const auto showSite = sites::Required("E0 FF BD 27 00 00 02 34 10 00 B0 AF 14 00 B1 AF 18 00 BF AF 33 00 A0 10 25 80 80 00 ? ? ? ?");
    const auto headerSite = sites::Required("? ? 11 3C 18 00 B2 AF 1C 00 B3 AF 20 00 B4 AF 24 00 B5 AF 28 00 B6 AF 2C 00 BF AF 13 00 A0 14");
    const auto closeSite = sites::Required("E0 FF BD 27 10 00 BF AF ? ? ? ? 10 00 80 A0 10 00 BF 8F 08 00 E0 03 20 00 BD 27 D0 FF BD 27");
    const auto dispatchSite = sites::Required("E0 FF BD 27 25 30 A0 00 C0 29 06 00 80 38 06 00 21 28 A7 00 21 28 85 00 E4 00 A5 8C 10 00 BF AF");
    const auto resetSite = sites::Required("E0 FF BD 27 FF 00 A7 30 3C 00 85 8C 34 00 80 AC 38 00 80 AC 10 00 BF AF 09 00 E0 10 3C 00 80 AC");
    header = Absolute(headerSite, 0, 32);
    reset = decltype(reset)(resetSite);
    showHook = safetymips::create_inline(showSite, Show);
    show.fun = showHook.original<decltype(show.fun)>();
    closeHook = safetymips::create_inline(closeSite, Close);
    close.fun = closeHook.original<decltype(close.fun)>();
    dispatchHook = safetymips::create_inline(dispatchSite, Dispatch);
    dispatch.fun = dispatchHook.original<decltype(dispatch.fun)>();
}
}
