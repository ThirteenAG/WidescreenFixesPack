#pragma once
#include "../Shared/Console/Text.hpp"
#include "MenuText.hpp"
#include "MenuLayout.hpp"

namespace NativeMenu
{
inline constexpr const char* cfg = "ms0:/PSP/PLUGINS/SOCOM.FireteamBravo3.PPSSPP.FusionFix/SOCOM.FireteamBravo3.PPSSPP.FusionFix.cfg";
inline IniValue values[]
{
    { "MAIN", "SkipIntro", "1" },
    { "MAIN", "DualAnalogPatch", "1" },
    { "MAIN", "DisableCameraSmoothing", "1" },
    { "MAIN", "UnthrottleEmuDuringLoading", "1" },
    { "MAIN", "FOVFactor", "1.0" },
    { "MAIN", "ForceAspectRatio", "Auto" },
    { "MAIN", "HudAspectRatioConstraint", "Auto" },
    { "MAIN", "HudScale", "1.0" },
    { "MAIN", "CutsceneBorders", "3" },
    { "MAIN", "TextLanguage", "auto" },
    { "MAIN", "GamepadIcons", "0" },
    { "MAIN", "SkipPressStart", "1" },
    { "MAIN", "FPSLimit", "60" }
};
inline char customAspect[2][16];
inline float customRatios[2];
inline bool pending[13], hasCustomAspect[2], draining, saveError;
inline bool pluginFocused, pluginDrawFocused;
inline void Open();
inline int ticks[13], systemLanguage, level, selected, preset{ 1 };
inline bool Capturing() { return level || draining; }
inline uint32_t previousButtons, presses, heldDirection, repeatAt;
inline SafetyMipsInline layerHook, buttonHook;
inline injector::hook_back<uintptr_t(uintptr_t, uintptr_t, uintptr_t)> layer;
inline injector::hook_back<int(uintptr_t, uint8_t, uint8_t, uint8_t, int, int)> button;
inline injector::game_fn<int(const char*, int, const char*, int, int, int, int, uint32_t, int)> print;
inline injector::game_fn<uintptr_t(const char*)> font;
inline injector::game_fn<int(uintptr_t, int, const char*, int, int, int, int, uint32_t, int)> printFont;
inline injector::game_fn<uintptr_t(const char*)> findTexture;
inline injector::game_fn<int(uintptr_t, uint32_t, float, float, float, float, float, float)> sprite;
struct FooterIcon
{
    uintptr_t texture;
    float left, top, width, height, u, v;
};
inline FooterIcon icons[3];
inline constexpr Word rowLabels[]
{
    Word::Skip, Word::Analog, Word::Smoothing, Word::Loading, Word::Fov,
    Word::Aspect, Word::Hudaspect, Word::Hudscale, Word::Borders, Word::Language, Word::Icons, Word::Start, Word::Fps
};
inline constexpr Word rowHelp[]
{
    Word::SkipHelp, Word::AnalogHelp, Word::SmoothingHelp, Word::LoadingHelp, Word::FovHelp,
    Word::AspectHelp, Word::HudaspectHelp, Word::HudscaleHelp, Word::BordersHelp, Word::LanguageHelp, Word::IconsHelp, Word::StartHelp, Word::FpsHelp
};
inline constexpr const char* ratios[]{ "Auto", "4:3", "16:9", "21:9", "32:9" };
inline constexpr const char* languages[]{ "auto", "en", "fr", "de", "it", "es" };
inline int Language()
{
    return ticks[9] > 0 ? ticks[9] - 1 : systemLanguage;
}
inline const char* Label(Word key)
{
    return labels[Language()][unsigned(key)];
}
inline void ReadOptions()
{
    for (unsigned i = 0; i < 13; ++i)
    {
        char buffer[16]{};
        const auto text = inireader.ReadString("MAIN", values[i].key, values[i].value, buffer, sizeof(buffer));
        std::strncpy(values[i].value, text, sizeof(values[i].value) - 1);
    }
    ticks[0] = options.skipIntro;
    ticks[1] = options.dualAnalog;
    ticks[2] = options.smoothing;
    ticks[3] = options.unthrottle;
    ticks[4] = int(std::round(std::clamp(options.fov, 0.5f, 2.0f) * 20));
    ticks[7] = int(std::round(Hud::scale * 20));
    ticks[8] = Cinematics::borders;
    ticks[10] = PadIcons::style;
    ticks[11] = Startup::skipPressStart;
    ticks[12] = FrameRate::limit == 60 ? 60 : 30;
    ticks[9] = 0;
    for (int i = 0; i < 6; ++i)
        if (std::strcmp(values[9].value, languages[i]) == 0)
            ticks[9] = i;
    for (unsigned i = 0; i < 2; ++i)
    {
        const unsigned row = 5 + i;
        const float aspect = i ? Hud::constraint : forcedAspect;
        customRatios[i] = aspect;
        ticks[row] = aspect > 0 ? 5 : 0;
        for (unsigned j = 1; j < 5; ++j)
            if (std::fabs(aspect - console::parse_hud_aspect(ratios[j])) < 0.0001f)
                ticks[row] = int(j);
        hasCustomAspect[i] = ticks[row] == 5;
        std::strcpy(customAspect[i], values[row].value);
    }
    int language = PSP_SYSTEMPARAM_LANGUAGE_ENGLISH;
    sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_LANGUAGE, &language);
    switch (language)
    {
        case PSP_SYSTEMPARAM_LANGUAGE_FRENCH: systemLanguage = 1; break;
        case PSP_SYSTEMPARAM_LANGUAGE_GERMAN: systemLanguage = 2; break;
        case PSP_SYSTEMPARAM_LANGUAGE_ITALIAN: systemLanguage = 3; break;
        case PSP_SYSTEMPARAM_LANGUAGE_SPANISH: systemLanguage = 4; break;
        default: systemLanguage = 0; break;
    }
    const bool on = ticks[0] && ticks[1] && ticks[2] && ticks[3] && ticks[10] == 0 && ticks[11] && ticks[12] == 60 && ticks[8] == 3;
    const bool off = !ticks[0] && !ticks[1] && !ticks[2] && !ticks[3] && ticks[10] == 2 && !ticks[11] && ticks[12] == 30 && ticks[8] == 1;
    preset = on ? 1 : off ? 0 : 3;
}
inline bool Save()
{
    IniValue changed[13]{};
    unsigned count = 0;
    for (unsigned i = 0; i < 13; ++i)
        if (pending[i])
            changed[count++] = values[i];
    if (!count)
        return true;
    IniWriter<> writer;
    saveError = !writer.update(cfg, changed, count);
    if (!saveError)
        std::fill(pending, pending + 13, false);
    return !saveError;
}
inline bool RestartNeeded()
{
    return ticks[0] != options.skipIntro || ticks[1] != options.dualAnalog ||
        ticks[2] != options.smoothing || ticks[3] != options.unthrottle || ticks[11] != Startup::skipPressStart;
}
inline void Store(unsigned row)
{
    auto& output = values[row].value;
    if (row < 4 || row == 11)
        std::strcpy(output, ticks[row] ? "1" : "0");
    else if (row == 4 || row == 7)
    {
        console::Text text(output);
        text.decimal(float(ticks[row]) / 20, 2);
    }
    else if (row == 5 || row == 6)
        std::strcpy(output, ticks[row] == 5 ? customAspect[row - 5] : ratios[ticks[row]]);
    else if (row == 9)
        std::strcpy(output, languages[ticks[row]]);
    else
    {
        console::Text text(output);
        text.integer(unsigned(ticks[row]));
    }
    pending[row] = true;
}
inline void ApplyLive(unsigned row)
{
    constexpr float aspects[]{ 0, 4.0f / 3, 16.0f / 9, 21.0f / 9, 32.0f / 9 };
    if (row == 4)
        fovFactor = float(ticks[row]) / 20;
    else if (row == 5 || row == 6)
    {
        const float aspect = ticks[row] == 5 ? customRatios[row - 5] : aspects[ticks[row]];
        if (row == 5)
            forcedAspect = aspect;
        else
            Hud::constraint = aspect;
    }
    else if (row == 7)
        Hud::scale = float(ticks[row]) / 20;
    else if (row == 8)
        Cinematics::borders = ticks[row];
    else if (row == 10)
        PadIcons::style = ticks[row];
    else if (row == 12)
    {
        FrameRate::limit = ticks[row];
        if (!FrameRate::ApplyLive())
        {
            ticks[row] = FrameRate::limit;
            Store(row);
        }
    }
    else if (row == 9)
    {
        // The native loader resolves these fields in place during Layer().
        for (auto row : pluginRows)
            if (row)
                row[14] = uint32_t(uintptr_t(Label(Word::Plugins)));
    }
    else
        return;
}
inline void Change(int direction)
{
    if (selected == 3 && !ticks[1])
        return;
    if (selected == 0)
    {
        preset = (preset + direction + 3) % 3;
        for (unsigned i = 0; i < 4; ++i)
        {
            if (ticks[i] != (preset != 0))
            {
                ticks[i] = preset != 0;
                Store(i);
            }
        }
        const int borders = preset == 0 ? 1 : 3;
        if (ticks[8] != borders)
        {
            ticks[8] = borders;
            Store(8);
            ApplyLive(8);
        }
        const int icons = preset == 0 ? 2 : 0;
        if (ticks[10] != icons)
        {
            ticks[10] = icons;
            Store(10);
            ApplyLive(10);
        }
        if (ticks[11] != (preset != 0))
        {
            ticks[11] = preset != 0;
            Store(11);
        }
        const int fps = preset == 0 ? 30 : 60;
        if (ticks[12] != fps)
        {
            ticks[12] = fps;
            Store(12);
            ApplyLive(12);
        }
    }
    else
    {
        const unsigned row = unsigned(selected - 1);
        const int previous = ticks[row];
        if (row < 4 || row == 11)
            ticks[row] = !ticks[row];
        else if (row == 12)
            ticks[row] = ticks[row] == 60 ? 30 : 60;
        else if (row == 4 || row == 7)
            ticks[row] = std::clamp(ticks[row] + direction, 10, row == 4 ? 40 : 30);
        else
        {
            const int maximum = row == 8 || row == 10 ? 3 : row == 9 ? 5 : hasCustomAspect[row - 5] ? 5 : 4;
            ticks[row] = (ticks[row] + direction + maximum + 1) % (maximum + 1);
        }
        if (ticks[row] == previous)
            return;
        Store(row);
        ApplyLive(row);
        if (row < 4 || row == 8 || row == 10 || row == 11 || row == 12)
            preset = 3;
    }
    Save();
}
inline void Input(SceCtrlData* samples, int count)
{
    if (count <= 0 || !samples)
        return;
    const uint32_t buttons = samples[0].Buttons;
    presses |= buttons & ~previousButtons;
    previousButtons = buttons;
    const uint32_t direction = buttons & (PSP_CTRL_UP | PSP_CTRL_DOWN | PSP_CTRL_LEFT | PSP_CTRL_RIGHT);
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
    if (!buttons)
        draining = false;
    if (!level && pluginFocused && (presses & PSP_CTRL_CROSS))
        Open();
    if (level || draining)
        for (int i = 0; i < count; ++i)
        {
            samples[i].Buttons = 0;
            samples[i].Lx = samples[i].Ly = 128;
            samples[i].Rsrv[0] = samples[i].Rsrv[1] = 128;
        }
}
inline uintptr_t Layer(uintptr_t object, uintptr_t definition, uintptr_t strings)
{
    const auto extended = ExtendLayout(definition, strings, Label(Word::Plugins));
    const auto result = layer.fun(object, extended, strings);
    if (extended != definition)
        logger.Write("Plugins row added to native options layout.\n");
    return result;
}
inline void BadMenuArtwork()
{
    static constexpr const char* messages[] = {
        "Fusion Fix: plugin menu unavailable; button artwork invalid.\n",
        "Fusion Fix : menu du plugin indisponible ; bouton invalide.\n",
        "Fusion Fix: Plugin-Menue nicht verfuegbar; Tastensymbol ungueltig.\n",
        "Fusion Fix: menu plugin non disponibile; pulsante non valido.\n",
        "Fusion Fix: menu del plugin no disponible; boton no valido.\n"
    };
    PatchNotice(messages[Language()]);
}
inline void Open()
{
    constexpr const char* names[][2]
    {
        { "ui\\helpbut_o", "menu_circlebutton" },
        { "ui\\helpbut_x", "menu_xbutton" },
        { "ui\\helpbut_tri", "triangle_button_prompt" }
    };
    for (unsigned i = 0; i < 3; ++i)
    {
        auto& icon = icons[i];
        icon.texture = findTexture(names[i][0]);
        if (!icon.texture)
            icon.texture = findTexture(names[i][1]);
        if (!icon.texture)
        {
            static constexpr const char* messages[] = {
                "Fusion Fix: plugin menu unavailable; button artwork missing.\n",
                "Fusion Fix : menu du plugin indisponible ; bouton manquant.\n",
                "Fusion Fix: Plugin-Menue nicht verfuegbar; Tastensymbol fehlt.\n",
                "Fusion Fix: menu plugin non disponibile; pulsante mancante.\n",
                "Fusion Fix: menu del plugin no disponible; falta un boton.\n"
            };
            PatchNotice(messages[Language()]);
            return;
        }
        const auto texture = reinterpret_cast<const uintptr_t*>(icon.texture);
        const auto header = reinterpret_cast<const uint8_t*>(texture[0]);
        uint16_t width, height;
        std::memcpy(&width, header + 4, 2);
        std::memcpy(&height, header + 6, 2);
        if ((header[13] != 0 && header[13] != 1) || header[16] != 6 || !width || !height || width > 512 || height > 512)
        {
            BadMenuArtwork();
            return;
        }
        const auto box = PadIcons::NativeBox(header, reinterpret_cast<const uint8_t*>(texture[5]),
            reinterpret_cast<const uint32_t*>(texture[6]));
        if (box.right <= box.left || box.bottom <= box.top)
        {
            BadMenuArtwork();
            return;
        }
        const float scale = 12.0f / float(std::max(box.right - box.left, box.bottom - box.top));
        icon.left = box.left * scale;
        icon.top = box.top * scale;
        icon.width = box.right * scale;
        icon.height = box.bottom * scale;
        icon.u = float(box.right) / width;
        icon.v = float(box.bottom) / height;
    }
    level = 1;
    selected = 0;
    presses = 0;
    logger.Write("Native Plugins page opened.\n");
}
inline void Footer(unsigned index, float x)
{
    const auto& icon = icons[index];
    sprite(icon.texture, 0xFF808080, x - icon.left, 251 - icon.top, icon.width, icon.height, icon.u, icon.v);
}
inline int Button(uintptr_t object, uint8_t focused, uint8_t hovered, uint8_t alpha, int x, int y)
{
    const auto definition = *reinterpret_cast<const uintptr_t*>(object);
    const auto name = *reinterpret_cast<const char* const*>(definition);
    if (std::strcmp(name, buttonName) == 0 && (focused || hovered))
        pluginDrawFocused = true;
    return button.fun(object, focused, hovered, alpha, x, y);
}
inline void Print(const char* name, const char* value, int x, int y, int width, int height, uint32_t color, int alignment)
{
    alignas(16) uint8_t readable[84];
    const auto original = font(name);
    std::memcpy(readable, reinterpret_cast<const void*>(original), sizeof(readable));
    // Native menu fonts can force black ink regardless of the requested color.
    // Use an owned descriptor so the private dark page has readable text.
    readable[80] = 0;
    printFont(reinterpret_cast<uintptr_t>(readable), 0, value, x, y, width, height, color, alignment);
}
inline void Text(const char* value, int x, int y, int width, int height, uint32_t color = 0xFF141414, int alignment = 0)
{
    Print("fe_font03", value, x, y, width, height, color, alignment);
}
inline void InlineHelp(const char* value)
{
    // The full description is available on the help page; its first sentence
    // gives the selected row a concise explanation in the two-line footer.
    char sentence[384]{};
    const char* end = std::strstr(value, ". ");
    const size_t length = std::min(end ? size_t(end - value + 1) : std::strlen(value), sizeof(sentence) - 1);
    std::memcpy(sentence, value, length);
    Text(sentence, 42, 191, 394, 32);
}
inline const char* Value(unsigned row, char (&buffer)[64])
{
    if (row < 4 || row == 11)
        return Label(ticks[row] ? Word::On : Word::Off);
    if (row == 5 || row == 6)
        return ticks[row] == 5 ? customAspect[row - 5] : ticks[row] ? ratios[ticks[row]] : Label(Word::Auto);
    if (row == 8)
        return Label(Word(unsigned(Word::BordersOff) + unsigned(ticks[row])));
    if (row == 10)
    {
        constexpr const char* styles[]{ "", "Xbox", "PlayStation", "Nintendo" };
        return ticks[row] ? styles[ticks[row]] : Label(Word::Auto);
    }
    if (row == 9)
    {
        constexpr const char* languages[]{ "", "English", "Français", "Deutsch", "Italiano", "Español" };
        return ticks[row] ? languages[ticks[row]] : Label(Word::Auto);
    }
    console::Text text(buffer);
    if (row == 12)
    {
        text.integer(unsigned(ticks[row]));
        return buffer;
    }
    text.fixed(float(ticks[row]) / 20, 2);
    return buffer;
}
inline uintptr_t TraceScreen(uintptr_t manager)
{
    if (!manager)
        return 0;
    const auto root = *reinterpret_cast<const uintptr_t*>(manager + 28);
    if (!root)
        return 0;
    auto screen = *reinterpret_cast<const uintptr_t*>(root + 12);
    if (!screen)
        return 0;
    const auto stack = *reinterpret_cast<const uintptr_t*>(screen + 44);
    if (stack)
    {
        const auto count = *reinterpret_cast<const unsigned*>(stack);
        if (count > 0 && count < 64)
            screen = reinterpret_cast<const uintptr_t*>(stack)[count];
    }
    const auto name = *reinterpret_cast<const char* const*>(screen + 4);
    static const char* previousName;
    if (name && name != previousName)
    {
        previousName = name;
        logger.WriteF("Native menu screen: %s.\n", name);
    }
    return screen;
}
inline void Render(uintptr_t manager = 0)
{
    Startup::Advance(TraceScreen(manager));
    if (pluginDrawFocused && !pluginFocused)
        logger.Write("Native Plugins row focused.\n");
    pluginFocused = pluginDrawFocused;
    pluginDrawFocused = false;
    if (level || pluginFocused || draining)
    {
        SceCtrlData pad{};
        if (sceCtrlPeekBufferPositive(&pad, 1) > 0)
            Input(&pad, 1);
    }
    if (!level)
    {
        presses = 0;
        return;
    }
    uint32_t input = presses;
    presses = 0;
    if (input & PSP_CTRL_CIRCLE)
    {
        if (Save())
        {
            --level;
            if (level != 2)
                selected = 0;
            draining = level == 0;
        }
        if (!level)
            return;
        input = 0;
    }
    if (level == 1 && (input & PSP_CTRL_CROSS))
    {
        level = 2;
        selected = 0;
    }
    else if (level == 2)
    {
        if (input & PSP_CTRL_TRIANGLE)
            level = 3;
        else if (input & PSP_CTRL_UP)
            selected = (selected + 13) % 14;
        if (input & PSP_CTRL_DOWN)
            selected = (selected + 1) % 14;
        if (input & PSP_CTRL_LEFT)
            Change(-1);
        else if (input & (PSP_CTRL_RIGHT | PSP_CTRL_CROSS))
            Change(1);
    }
    Hud::scissor.fun(0, 0, 480, 272);
    Hud::Fill(0, 0, 480, 272, 0xFF000000);
    Hud::fill.fun(30, 6, 420, 235, 0xFFE8E3DB);
    Hud::fill.fun(30, 245, 420, 27, 0xFFE8E3DB);
    Text(level == 1 ? Label(Word::Plugins) : "Fusion Fix", 35, 9, 410, 18);
    if (level == 1)
    {
        Hud::fill.fun(36, 42, 408, 18, 0xFF605850);
        Text("Fireteam Bravo 3 Fusion Fix", 42, 43, 390, 18);
        Text(Label(Word::Help), 42, 201, 390, 36);
    }
    else if (level == 2)
    {
        const int first = selected / 8 * 8;
        for (int row = first; row < std::min(first + 8, 14); ++row)
        {
            const int y = 42 + (row - first) * 18;
            if (row == selected)
                Hud::fill.fun(36, y, 408, 18, 0xFF605850);
            const uint32_t color = row == 3 && !ticks[1] ? 0xFF606060 : 0xFF141414;
            Text(row ? Label(rowLabels[row - 1]) : Label(Word::Preset), 42, y + 1, 292, 17, color);
            char buffer[64]{};
            const char* value = row ? Value(unsigned(row - 1), buffer) :
                Label(preset == 3 ? Word::Custom : Word(unsigned(Word::Vanilla) + unsigned(preset)));
            Text(value, 337, y + 1, 102, 17, color, 2);
        }
        InlineHelp(selected ? Label(rowHelp[selected - 1]) : Label(Word::PresetHelp));
        char pager[16]{};
        console::Text text(pager);
        text.integer(unsigned(first / 8 + 1));
        text.append("/2");
        Text(pager, 405, 12, 35, 16, 0xFF141414, 2);
    }
    else
    {
        Text(selected ? Label(rowLabels[selected - 1]) : Label(Word::Preset), 42, 43, 394, 30);
        Text(selected ? Label(rowHelp[selected - 1]) : Label(Word::PresetHelp), 42, 77, 394, 146);
    }
    if (saveError || RestartNeeded())
        Text(Label(saveError ? Word::SaveError : Word::Restart), 36, 225, 410, 18, 0xFF003C90);
    Footer(0, 38);
    Text(Label(Word::Back), 54, 251, 105, 20);
    if (level == 2)
    {
        Footer(2, 172);
        Text(Label(Word::HelpButton), 188, 251, 100, 20);
    }
    if (level < 3)
    {
        Footer(1, 297);
        Text(Label(Word::Confirm), 313, 251, 131, 20);
    }
}
inline void Install()
{
    const auto layerSite = Hud::Required("50 FF BD 27 90 00 B3 AF 25 98 80 00 98 00 B5 AF 25 A8 A0 00 25 20 C0 00 84 00 B0 AF 88 00 B1 AF 8C 00 B2 AF 94 00 B4 AF");
    const auto buttonSite = Hud::Required("E0 FF BD 27 08 00 B2 AF 25 90 80 00 20 00 44 8E 14 00 B5 AF FF 00 B5 30 48 00 84 24 10 00 B4 AF FF 00 D4 30 00 00 85 84");
    const auto printSite = Hud::Required("C0 FF BD 27 10 00 B0 AF 14 00 B1 AF 18 00 B2 AF 1C 00 B3 AF 20 00 B4 AF 24 00 B5 AF 28 00 B6 AF 25 80 60 01 25 88 40 01");
    const auto textureSite = Hud::Required("08 00 A4 27 25 28 40 00 ? ? ? ? 25 30 00 02 ? ? ? ? 08 00 A4 8F 08 00 A4 8F 0D 00 80 10 25 80 40 00", 16);
    const auto spriteSite = Hud::Required("D0 FF BD 27 04 00 B6 E7 08 00 B8 E7 0C 00 BA E7 10 00 BC E7 14 00 BE E7 18 00 B0 AF 86 67 00 46 25 80 A0 00 06 6F 00 46");
    layerHook = safetymips::create_inline(layerSite, Layer);
    layer.fun = layerHook.original<decltype(layer.fun)>();
    buttonHook = safetymips::create_inline(buttonSite, Button);
    button.fun = buttonHook.original<decltype(button.fun)>();
    print = decltype(print)(printSite);
    font = injector::GetBranchDestination(printSite + 68).get();
    printFont = injector::GetBranchDestination(printSite + 108).get();
    findTexture = injector::GetBranchDestination(textureSite).get();
    sprite = decltype(sprite)(spriteSite);
    Hud::menuExtra = Render;
}
}
