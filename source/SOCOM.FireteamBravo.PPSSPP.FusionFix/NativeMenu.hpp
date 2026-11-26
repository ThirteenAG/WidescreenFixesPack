#pragma once
#include "MenuText.hpp"

namespace NativeMenu
{
inline constexpr const char* cfg = "ms0:/PSP/PLUGINS/SOCOM.FireteamBravo.PPSSPP.FusionFix/SOCOM.FireteamBravo.PPSSPP.FusionFix.cfg";
inline constexpr IniValue values[] {
    {"MAIN", "SkipIntro", "1"}, {"MAIN", "DualAnalogPatch", "1"},
    {"MAIN", "UnthrottleEmuDuringLoading", "1"}, {"MAIN", "FOVFactor", "1.0"},
    {"MAIN", "ForceAspectRatio", "Auto"}, {"MAIN", "HudAspectRatioConstraint", "Auto"},
    {"MAIN", "HudScale", "1.0"}, {"MAIN", "CutsceneBorders", "3"},
    {"MAIN", "TextLanguage", "auto"}, {"MAIN", "FPSLimit", "60"},
    {"MAIN", "GamepadIcons", "0"},
};
inline constexpr unsigned settingCount = sizeof(values) / sizeof(values[0]);
inline constexpr Word names[] { Word::Skip, Word::Analog, Word::Loading, Word::Fov,
    Word::Aspect, Word::Hudaspect, Word::Hudscale, Word::Borders, Word::Language, Word::Fps };
inline constexpr Word help[] { Word::SkipHelp, Word::AnalogHelp, Word::LoadingHelp, Word::FovHelp,
    Word::AspectHelp, Word::HudaspectHelp, Word::HudscaleHelp, Word::BordersHelp, Word::LanguageHelp, Word::FpsHelp };
inline constexpr const char* ratios[] { "Auto", "4:3", "16:9", "21:9", "32:9" };
inline constexpr const char* languages[] { "auto", "en", "fr", "de", "it", "es" };
inline constexpr const char* languageNames[] { "Auto", "English", "Francais", "Deutsch", "Italiano", "Espanol" };
inline constexpr const char* descriptions[5][10] {
    {"Skip routine startup notices. Restart the emulated game after changing this setting.", "Use the right stick for camera and aim. Restart the emulated game after changing this setting.", "Unthrottle loading and restore normal speed afterwards. Restart the emulated game after changing this setting.", "Adjust the gameplay camera field of view. 1.0 uses the game's default.", "Auto follows PPSSPP's display aspect. Choose a ratio to override it.", "Auto places HUD groups at the screen edges. Choose a ratio to keep them in a centered box.", "Change the size of gameplay HUD elements around their anchors.", "Choose which borders preserve the framing of scripted cameras.", "Choose the language used in this Plugins menu.", "Choose 30 or 60 frames per second."},
    {"Passe les avis de demarrage habituels. Redemarrez le jeu emule apres ce changement.", "Utilise le stick droit pour la camera et la visee. Redemarrez le jeu emule apres ce changement.", "Accelere les chargements puis retablit la vitesse normale. Redemarrez le jeu apres ce changement.", "Modifie le champ de vision en jeu. 1,0 conserve la valeur du jeu.", "Auto suit le format de PPSSPP. Choisissez un format pour le remplacer.", "Auto place le HUD aux bords. Un format le garde dans une zone centree.", "Modifie la taille du HUD autour de ses points d'ancrage.", "Choisit les bandes qui conservent le cadrage des cameras de sequence.", "Choisit la langue de ce menu Plugins.", "Choisit 30 ou 60 images par seconde."},
    {"Ueberspringt normale Starthinweise. Starte das emulierte Spiel nach der Aenderung neu.", "Nutzt den rechten Stick fuer Kamera und Zielen. Starte das Spiel nach der Aenderung neu.", "Beschleunigt Ladezeiten und stellt danach das normale Tempo wieder her. Neustart erforderlich.", "Aendert das Sichtfeld im Spiel. 1,0 entspricht dem Standard.", "Auto folgt dem Format von PPSSPP. Ein anderes Format ueberschreibt es.", "Auto setzt das HUD an die Bildschirmraender. Ein Format begrenzt es auf eine mittige Box.", "Aendert die HUD-Groesse um ihre Ankerpunkte.", "Waehlt Balken fuer die Bildkomposition von Zwischensequenzen.", "Waehlt die Sprache dieses Plugins-Menues.", "Waehlt 30 oder 60 Bilder pro Sekunde."},
    {"Salta gli avvisi iniziali ordinari. Riavvia il gioco emulato dopo la modifica.", "Usa la levetta destra per visuale e mira. Riavvia il gioco dopo la modifica.", "Accelera i caricamenti e ripristina la velocita normale. Riavvia il gioco dopo la modifica.", "Modifica il campo visivo durante il gioco. 1,0 mantiene il valore originale.", "Auto segue il formato di PPSSPP. Scegli un formato per sostituirlo.", "Auto dispone l'HUD ai bordi. Un formato lo mantiene in un riquadro centrale.", "Modifica la dimensione dell'HUD attorno ai suoi punti di ancoraggio.", "Sceglie le bande che conservano l'inquadratura delle sequenze.", "Sceglie la lingua di questo menu Plugins.", "Sceglie 30 o 60 fotogrammi al secondo."},
    {"Omite los avisos iniciales habituales. Reinicia el juego emulado tras cambiar esta opcion.", "Usa el stick derecho para camara y mira. Reinicia el juego tras cambiar esta opcion.", "Acelera las cargas y restaura la velocidad normal. Reinicia el juego tras cambiar esta opcion.", "Ajusta el campo de vision durante la partida. 1,0 conserva el valor original.", "Auto sigue el formato de PPSSPP. Elige otro formato para sustituirlo.", "Auto coloca el HUD en los bordes. Un formato lo mantiene en una caja centrada.", "Cambia el tamano del HUD alrededor de sus puntos de anclaje.", "Elige las bandas que conservan el encuadre de las secuencias.", "Elige el idioma de este menu Plugins.", "Elige 30 o 60 fotogramas por segundo."},
};
inline constexpr const char* presetHelp[5] {
    "Vanilla selects PSP icons and 30 FPS and disables intro skip, dual analog and fast loading. Recommended and Enhanced select Auto icons and 60 FPS and enable those options. Vanilla uses Letterbox; the others use Both. Other values are kept.",
    "Vanilla choisit les icones PSP et 30 FPS, sans passage des avis, double stick ni charge rapide. Recommended et Enhanced choisissent les icones Auto et 60 FPS et les activent. Vanilla choisit Letterbox, les autres Both.",
    "Vanilla waehlt PSP-Symbole und 30 FPS und deaktiviert Hinweis-Skip, Dual Analog und schnelles Laden. Recommended und Enhanced waehlen Auto-Symbole und 60 FPS und aktivieren sie. Vanilla waehlt Letterbox, die anderen Both.",
    "Vanilla sceglie icone PSP e 30 FPS e disattiva salto avvisi, doppia levetta e caricamento rapido. Recommended e Enhanced scelgono icone Auto e 60 FPS e li attivano. Vanilla usa Letterbox, gli altri Both.",
    "Vanilla elige iconos PSP y 30 FPS y desactiva omitir avisos, doble stick y carga rapida. Recommended y Enhanced eligen iconos Auto y 60 FPS y los activan. Vanilla usa Letterbox y los demas Both.",
};
// -1 denotes the preset row; empty entries are not selectable.
inline constexpr int pages[3][5] { {-1, 3, 5, 6, 7}, {9, 4, 0, 1, 2}, {8, 10, -2, -2, -2} };
inline int ticks[settingCount]{1, 1, 1, 20, 0, 0, 20, 3, 0, 1, 0};
inline int startTicks[3]{1, 1, 1};
inline int preset{1}, systemLanguage{}, page{}, selected{1}, level{}, helpOffset{}, helpCount{};
inline int helpTopic{-2};
inline bool saveError{}, draining{}, restartNeeded{};
inline uint32_t previousButtons{};
inline uintptr_t pauseObject{};
inline char display[7][128]{}, originalText[7][128]{}, helpLines[24][64]{};
inline char customRatios[2][32]{};
inline bool customRatio[2]{};
struct SavedRow { uintptr_t label; uint8_t enabled, allowed; };
inline SavedRow saved[7]{};
inline int originalSelection{};
inline injector::hook_back<const char*(int)> originalLabel;
inline injector::hook_back<int(int)> originalAllowed;
inline injector::hook_back<int(uintptr_t)> originalConfirm;
inline injector::game_fn<void(uintptr_t)> refresh;
inline IniSnapshot settings;
enum class OptionsRead { Unavailable, Unchanged, Changed };
inline OptionsRead ReadOptions();

inline int Language() { return ticks[8] ? ticks[8] - 1 : systemLanguage; }
inline const char* Label(Word word) { return labels[Language()][unsigned(word)]; }
inline uintptr_t Row(unsigned i) { return pauseObject + 16 + i * 16; }
inline uintptr_t Font(unsigned i) { return *reinterpret_cast<const uintptr_t*>(Row(i) + 8); }
inline void FontText(uintptr_t font, const char* text)
{
    if (!font) return;
    const auto table = *reinterpret_cast<const uintptr_t*>(font);
    const auto adjust = *reinterpret_cast<const int16_t*>(table + 248);
    const injector::game_fn<void(uintptr_t, const char*, int)> set{*reinterpret_cast<const uintptr_t*>(table + 252)};
    set(font + adjust, text, 1);
}
inline void FontVisible(uintptr_t font, bool visible)
{
    if (!font) return;
    const auto table = *reinterpret_cast<const uintptr_t*>(font);
    const unsigned off = visible ? 48 : 40;
    const auto adjust = *reinterpret_cast<const int16_t*>(table + off);
    const injector::game_fn<void(uintptr_t)> call{*reinterpret_cast<const uintptr_t*>(table + off + 4)};
    call(font + adjust);
}
inline void SetRow(unsigned i, const char* text, bool active)
{
    npf_snprintf(display[i], sizeof(display[i]), "%s", text ? text : "");
    const auto row = Row(i);
    *reinterpret_cast<uintptr_t*>(row + 4) = active ? uintptr_t(display[i]) : 0;
    *reinterpret_cast<uint8_t*>(row + 12) = active;
    *reinterpret_cast<uint8_t*>(row + 13) = active;
    FontText(Font(i), display[i]); FontVisible(Font(i), active);
}
inline bool Save(const IniValue* pending, unsigned count)
{
    IniWriter<> writer;
    saveError = !writer.update(cfg, pending, count, IniUpdate::Replace, settings.Bytes());
    return !saveError;
}
inline IniValue Encode(unsigned i, int tick)
{
    IniValue entry = values[i];
    if (i == 3 || i == 6)
        console::decimal(entry.value, tick / 20.0f, 2);
    else if (i == 4 || i == 5)
        npf_snprintf(entry.value, sizeof(entry.value), "%s", ratios[tick]);
    else if (i == 8)
        npf_snprintf(entry.value, sizeof(entry.value), "%s", languages[tick]);
    else
        npf_snprintf(entry.value, sizeof(entry.value), "%d", i == 9 ? (tick ? 60 : 30) : tick);
    return entry;
}
inline void CheckRestart()
{
    restartNeeded = !std::equal(startTicks, startTicks + 3, ticks);
}
inline int MatchingPreset()
{
    if (!ticks[0] && !ticks[1] && !ticks[2] && !ticks[9] && ticks[10] == 2 && ticks[7] == 1)
        return 0;
    if (ticks[0] && ticks[1] && ticks[2] && ticks[9] && ticks[10] == 0 && ticks[7] == 3)
        return preset == 2 ? 2 : 1;
    return 3;
}
inline void Apply(unsigned i)
{
    if (i == 3) Geometry::fovFactor = ticks[i] / 20.0f;
    // The menu only offers the listed ratios.
    else if (i == 4 || i == 5)
    {
        constexpr float aspects[] { 0.0f, 4.0f / 3.0f, 16.0f / 9.0f, 21.0f / 9.0f, 32.0f / 9.0f };
        (i == 4 ? Geometry::forcedAspect : Hud::constraint) = aspects[ticks[i]];
    }
    else if (i == 6) Hud::scale = ticks[i] / 20.0f;
    else if (i == 7) Geometry::borders = ticks[i];
    else if (i == 9) FrameRate::limit = ticks[i] ? 60 : 30;
    else if (i == 10) PadIcons::style = ticks[i];
}
inline void Change(unsigned i, int direction)
{
    if (ReadOptions() == OptionsRead::Unavailable)
    {
        saveError = true;
        return;
    }
    int lo{}, hi{1};
    if (i == 3) { lo = 10; hi = 40; }
    else if (i == 4 || i == 5) hi = 4;
    else if (i == 6) { lo = 10; hi = 30; }
    else if (i == 7) hi = 3;
    else if (i == 8) hi = 5;
    else if (i == 10) hi = 3;
    const bool custom = (i == 4 || i == 5) && customRatio[i - 4];
    int next = custom ? (direction > 0 ? lo : hi) : ticks[i] + direction;
    if (next < lo) next = hi;
    if (next > hi) next = lo;
    const auto entry = Encode(i, next);
    if (!Save(&entry, 1))
        return;
    if (custom)
        customRatio[i - 4] = false;
    ticks[i] = next;
    if (i < 3 || i == 7 || i >= 9)
        preset = MatchingPreset();
    Apply(i);
    CheckRestart();
}
inline const char* Value(int i, char (&buffer)[64])
{
    if (i == -1) return preset == 0 ? Label(Word::Vanilla) : preset == 1 ? Label(Word::Recommended) : preset == 2 ? Label(Word::Enhanced) : Label(Word::Custom);
    if (i < 3) return Label(ticks[i] ? Word::On : Word::Off);
    if (i == 3 || i == 6) { npf_snprintf(buffer, sizeof(buffer), "%d.%02d", ticks[i] / 20, (ticks[i] % 20) * 5); return buffer; }
    if (i == 4 || i == 5) return customRatio[i - 4] ? customRatios[i - 4] : ticks[i] ? ratios[ticks[i]] : Label(Word::Auto);
    if (i == 7) return Label(Word(unsigned(Word::BordersOff) + unsigned(ticks[i])));
    if (i == 8) return ticks[i] ? languageNames[ticks[i]] : Label(Word::Auto);
    if (i == 10) { constexpr const char* icons[] { "Auto", "Xbox", "PlayStation", "Nintendo" }; return icons[ticks[i]]; }
    return ticks[i] ? "60 FPS" : "30 FPS";
}
inline void Preset(int direction)
{
    if (ReadOptions() == OptionsRead::Unavailable)
    {
        saveError = true;
        return;
    }
    const int next = (preset + direction + 4) % 4;
    if (next == 3)
    {
        preset = next;
        return;
    }
    const int enabled = next != 0;
    constexpr unsigned rows[]{0, 1, 2, 7, 9, 10};
    const int proposed[]{enabled, enabled, enabled, enabled ? 3 : 1, enabled, enabled ? 0 : 2};
    IniValue pending[std::size(rows)]{};
    for (unsigned i = 0; i < std::size(rows); ++i)
        pending[i] = Encode(rows[i], proposed[i]);
    if (!Save(pending, std::size(pending)))
        return;
    preset = next;
    for (unsigned i = 0; i < std::size(rows); ++i)
    {
        ticks[rows[i]] = proposed[i];
        Apply(rows[i]);
    }
    CheckRestart();
}
inline void WrapHelp()
{
    constexpr const char* iconHelp[] {
        "Auto uses Xbox art. PlayStation keeps PSP symbols. Configure controls in PPSSPP.",
        "Auto utilise les icones Xbox. PlayStation garde les symboles PSP. Configurez les commandes dans PPSSPP.",
        "Auto nutzt Xbox-Symbole. PlayStation behaelt PSP-Symbole. Steuerung in PPSSPP einstellen.",
        "Auto usa icone Xbox. PlayStation mantiene i simboli PSP. Configura i comandi in PPSSPP.",
        "Auto usa iconos Xbox. PlayStation conserva los simbolos PSP. Configura los controles en PPSSPP.",
    };
    const int language = Language();
    const char* source = helpTopic == -1 ? presetHelp[language] :
        helpTopic == 10 ? iconHelp[language] : helpTopic >= 0 ? descriptions[language][helpTopic] : Label(Word::Help);
    helpCount = 0;
    while (*source && helpCount < 24)
    {
        while (*source == ' ') ++source;
        if (!*source) break;
        size_t n = std::strlen(source); if (n > 47) n = 47;
        if (source[n] && n == 47) { size_t split = n; while (split && source[split] != ' ') --split; if (split) n = split; }
        std::memcpy(helpLines[helpCount], source, n); helpLines[helpCount][n] = 0;
        ++helpCount; source += n;
    }
    helpOffset = std::min(helpOffset, std::max(0, (helpCount - 1) / 5 * 5));
}
inline void Build()
{
    if (!pauseObject) return;
    char line[128]{};
    constexpr const char* restart[] { "RESTART", "REDEMARRER", "NEUSTART", "RIAVVIA", "REINICIAR" };
    constexpr const char* error[] { "SAVE ERROR", "ERREUR SAUVEGARDE", "SPEICHERFEHLER", "ERRORE SALVATAGGIO", "ERROR AL GUARDAR" };
    constexpr const char* navigation[] { "L/R PAGE  TRI HELP", "L/R PAGE  TRI AIDE", "L/R SEITE  TRI HILFE",
        "L/R PAGINA  TRI AIUTO", "L/R PAGINA  TRI AYUDA" };
    constexpr const char* helpNavigation[] { "L/R PAGE", "L/R PAGES", "L/R SEITE", "L/R PAGINA", "L/R PAGINA" };
    npf_snprintf(line, sizeof(line), "FUSION FIX %d/%d %s", level == 2 ? 1 + helpOffset / 5 : page + 1,
        level == 2 ? std::max(1, (helpCount + 4) / 5) : 3,
        saveError ? error[Language()] : restartNeeded ? restart[Language()] :
        level == 2 ? helpNavigation[Language()] : navigation[Language()]);
    SetRow(0, line, true);
    if (level == 2)
    {
        for (int r = 0; r < 5; ++r) SetRow(unsigned(r + 1), helpOffset + r < helpCount ? helpLines[helpOffset + r] : "", helpOffset + r < helpCount);
    }
    else
    {
        for (int r = 0; r < 5; ++r)
        {
            const int item = pages[page][r];
            if (item == -2) { SetRow(unsigned(r + 1), "", false); continue; }
            char value[64]{};
            constexpr const char* iconNames[] { "Controller icons", "Icones manette", "Controller-Symbole", "Icone controller", "Iconos del mando" };
            npf_snprintf(line, sizeof(line), "%s: %s", item == -1 ? Label(Word::Preset) : item == 10 ? iconNames[Language()] : Label(names[item]), Value(item, value));
            SetRow(unsigned(r + 1), line, true);
        }
        if (selected < 1 || selected > 5 || pages[page][selected - 1] == -2) selected = 1;
    }
    *reinterpret_cast<int*>(pauseObject + 128) = level == 2 ? 1 : selected;
    refresh(pauseObject);
}
inline void Open(uintptr_t object)
{
    pauseObject = object; originalSelection = *reinterpret_cast<int*>(object + 128);
    for (unsigned i = 0; i < 7; ++i)
    {
        const auto row = Row(i); const auto font = Font(i);
        saved[i] = { *reinterpret_cast<const uintptr_t*>(row + 4), *reinterpret_cast<const uint8_t*>(row + 12), *reinterpret_cast<const uint8_t*>(row + 13) };
        const auto text = font ? *reinterpret_cast<const char* const*>(font + 32) : nullptr;
        npf_snprintf(originalText[i], sizeof(originalText[i]), "%s", text ? text : "");
    }
    page = 0; selected = 1; level = 1; Build();
}
inline void Close()
{
    if (!pauseObject) return;
    for (unsigned i = 0; i < 7; ++i)
    {
        const auto row = Row(i);
        *reinterpret_cast<uintptr_t*>(row + 4) = saved[i].label;
        *reinterpret_cast<uint8_t*>(row + 12) = saved[i].enabled;
        *reinterpret_cast<uint8_t*>(row + 13) = saved[i].allowed;
        FontText(Font(i), originalText[i]); FontVisible(Font(i), saved[i].label && saved[i].enabled);
    }
    *reinterpret_cast<int*>(pauseObject + 128) = originalSelection;
    refresh(pauseObject); pauseObject = 0; level = 0; draining = true;
}
inline const char* PauseLabel(int i) { return i == 5 ? "PLUGINS" : originalLabel.fun(i); }
inline int PauseAllowed(int i) { return i == 5 ? 1 : originalAllowed.fun(i); }
inline int Confirm(uintptr_t object)
{
    const auto selectedRow = *reinterpret_cast<const int*>(object + 128);
    const auto id = *reinterpret_cast<const int*>(object + 16 + selectedRow * 16);
    if (id == 5) { Open(object); return 0; }
    return originalConfirm.fun(object);
}
inline void Input(console::socom::Input* input)
{
    const uint32_t buttons = input->pad.Buttons, presses = buttons & ~previousButtons;
    previousButtons = buttons;
    constexpr uint32_t mask = PSP_CTRL_UP | PSP_CTRL_DOWN | PSP_CTRL_LEFT | PSP_CTRL_RIGHT | PSP_CTRL_CROSS | PSP_CTRL_CIRCLE | PSP_CTRL_TRIANGLE | PSP_CTRL_LTRIGGER | PSP_CTRL_RTRIGGER | PSP_CTRL_START;
    if (!level && !draining) return;
    if (level == 2)
    {
        if (presses & (PSP_CTRL_CIRCLE | PSP_CTRL_START | PSP_CTRL_CROSS))
        {
            level = 1;
            Build();
        }
        else if (presses & (PSP_CTRL_UP | PSP_CTRL_LTRIGGER))
        {
            helpOffset = std::max(0, helpOffset - 5);
            Build();
        }
        else if ((presses & (PSP_CTRL_DOWN | PSP_CTRL_RTRIGGER)) && helpOffset + 5 < helpCount)
        {
            helpOffset += 5;
            Build();
        }
    }
    else if (level == 1 && (presses & (PSP_CTRL_CIRCLE | PSP_CTRL_START)))
    {
        Close();
    }
    else if (level == 1)
    {
        if (presses & (PSP_CTRL_UP | PSP_CTRL_DOWN))
        {
            const int step = presses & PSP_CTRL_DOWN ? 1 : -1;
            do { selected += step; if (selected < 1) selected = 5; if (selected > 5) selected = 1; }
            while (pages[page][selected - 1] == -2);
            Build();
        }
        if (presses & (PSP_CTRL_LTRIGGER | PSP_CTRL_RTRIGGER)) { page = (page + ((presses & PSP_CTRL_RTRIGGER) ? 1 : 2)) % 3; selected = 1; Build(); }
        if (presses & PSP_CTRL_TRIANGLE)
        {
            helpTopic = pages[page][selected - 1];
            helpOffset = 0;
            WrapHelp();
            level = 2;
            Build();
        }
        else if (presses & (PSP_CTRL_LEFT | PSP_CTRL_RIGHT))
        {
            const int step = presses & PSP_CTRL_RIGHT ? 1 : -1;
            const int item = selected <= 5 ? pages[page][selected - 1] : -2;
            if (item == -1) Preset(step); else if (item >= 0) Change(unsigned(item), step);
            Build();
        }
    }
    input->pad.Buttons &= ~mask; input->changed &= ~mask;
    if (!level && !(buttons & mask)) draining = false;
}
inline OptionsRead ReadOptions()
{
    if (!settings.Read(cfg))
        return OptionsRead::Unavailable;

    int previous[settingCount];
    char priorRatios[2][32];
    bool priorCustom[2];
    std::copy(ticks, ticks + settingCount, previous);
    std::memcpy(priorRatios, customRatios, sizeof(priorRatios));
    std::copy(customRatio, customRatio + 2, priorCustom);
    for (unsigned i = 0; i < 3; ++i)
        ticks[i] = settings.Toggle("MAIN", values[i].key, ticks[i] != 0);

    Geometry::fovFactor = std::clamp(settings.Real("MAIN", values[3].key, Geometry::fovFactor), 0.5f, 2.0f);
    Hud::scale = std::clamp(settings.Real("MAIN", values[6].key, Hud::scale), 0.5f, 1.5f);
    ticks[3] = int(std::round(Geometry::fovFactor * 20.0f));
    ticks[6] = int(std::round(Hud::scale * 20.0f));

    for (int row = 4; row <= 5; ++row)
    {
        char text[32];
        const int index = row - 4;
        if (!settings.Text("MAIN", values[row].key, text, sizeof(text)))
            continue;
        float& aspect = row == 4 ? Geometry::forcedAspect : Hud::constraint;
        aspect = row == 4 ? console::parse_forced_aspect(text) : console::parse_hud_aspect(text);
        ticks[row] = 0;
        customRatio[index] = false;
        if (aspect <= 0.0f)
            continue;
        for (int i = 1; i < 5; ++i)
        {
            if (std::strcmp(text, ratios[i]) == 0)
                ticks[row] = i;
        }
        if (!ticks[row])
        {
            customRatio[index] = true;
            npf_snprintf(customRatios[index], sizeof(customRatios[index]), "%s", text);
        }
    }

    char languageId[16];
    if (settings.Text("MAIN", values[8].key, languageId, sizeof(languageId)))
    {
        for (int i = 0; i < 6; ++i)
            if (std::strcmp(languageId, languages[i]) == 0)
                ticks[8] = i;
    }
    for (const unsigned row : {7u, 10u})
    {
        const float value = settings.Real("MAIN", values[row].key, float(ticks[row]));
        if (value >= 0.0f && value <= 3.0f && float(int(value)) == value)
            ticks[row] = int(value);
    }
    const float limit = settings.Real("MAIN", values[9].key, float(FrameRate::limit));
    if (limit == 30.0f || limit == 60.0f)
        ticks[9] = limit == 60.0f;
    Apply(7);
    Apply(9);
    Apply(10);
    if (!std::equal(previous, previous + 3, ticks) || previous[7] != ticks[7] ||
        previous[9] != ticks[9] || previous[10] != ticks[10])
        preset = MatchingPreset();
    CheckRestart();
    const bool changed = !std::equal(previous, previous + settingCount, ticks) ||
        !std::equal(priorCustom, priorCustom + 2, customRatio) ||
        (customRatio[0] && std::strcmp(priorRatios[0], customRatios[0]) != 0) ||
        (customRatio[1] && std::strcmp(priorRatios[1], customRatios[1]) != 0);
    return changed ? OptionsRead::Changed : OptionsRead::Unchanged;
}
inline void InitializeOptions()
{
    ReadOptions();
    std::copy(ticks, ticks + 3, startTicks);
    CheckRestart();
    int language{}; sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_LANGUAGE, &language);
    switch (language) { case PSP_SYSTEMPARAM_LANGUAGE_FRENCH: systemLanguage = 1; break; case PSP_SYSTEMPARAM_LANGUAGE_GERMAN: systemLanguage = 2; break; case PSP_SYSTEMPARAM_LANGUAGE_ITALIAN: systemLanguage = 3; break; case PSP_SYSTEMPARAM_LANGUAGE_SPANISH: systemLanguage = 4; break; default: systemLanguage = 0; }
}
inline void PollOptions()
{
    static uint32_t lastRead{};
    const uint32_t now = sceKernelGetSystemTimeLow();
    if (lastRead && now - lastRead < 1000000u)
        return;
    lastRead = now;
    const int previousLanguage = Language();
    if (ReadOptions() == OptionsRead::Changed && level)
    {
        if (level == 2 && previousLanguage != Language())
            WrapHelp();
        Build();
    }
}
inline void Install()
{
    const auto rows = sites::Required("25 98 40 00 10 00 50 AE ? ? ? ? 25 20 00 02 14 00 42 AE ? ? ? ? 25 20 00 02 1C 00 42 A2 ? ? ? ? 25 20 00 02 1D 00 42 A2");
    const auto confirm = sites::Required("04 00 80 10 00 00 00 00 ? ? ? ? 25 20 60 02 25 80 40 00 1F 00 00 12 25 20 60 02 ? ? ? ? 25 28 00 00");
    refresh = decltype(refresh)(sites::Required("A0 FF BD 27 ? ? 05 3C ? ? A5 24 ? ? A0 D8 ? ? 05 3C ? ? A5 24 01 00 A5 90 38 00 B0 AF 40 00 B2 AF"));
    originalLabel.fun = injector::MakeCALL(rows + 8, PauseLabel).get();
    originalAllowed.fun = injector::MakeCALL(rows + 32, PauseAllowed).get();
    originalConfirm.fun = injector::MakeCALL(confirm + 8, Confirm).get();
}
}
