#pragma once
#include "../../external/injector/include/ps2/game_abi.hpp"
#include "../Shared/Console/PS2Settings.hpp"
#include "../Shared/Console/Text.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iterator>

namespace burnout::menu
{
using console::ps2settings::Value;
struct Entry
{
    const char16_t* text;
    uint8_t disabled, marker;
    uint16_t spare;
};
static_assert(sizeof(Entry) == 8);
struct Language
{
    const char* aspect;
    const char* box;
    const char* scale;
    const char* fov;
    const char* movie;
    const char* more;
    const char* intro;
    const char* preset;
    const char* reload;
    const char* previous;
    const char* exit;
    const char* original;
    const char* fit;
    const char* on;
    const char* off;
    const char* custom;
    const char* vanilla;
    const char* recommended;
    const char* enhanced;
    const char16_t* displayHelp;
    const char16_t* hudHelp;
    const char16_t* sizeHelp;
    const char16_t* fovHelp;
    const char16_t* movieHelp;
    const char16_t* introHelp;
    const char16_t* presetHelp;
    const char16_t* moreHelp;
    const char16_t* reloadHelp;
    const char16_t* backHelp;
    const char16_t* pluginsHelp;
};
const Language languages[] = {{"DISPLAY ASPECT",
                               "HUD BOX",
                               "HUD SIZE",
                               "FIELD OF VIEW",
                               "MOVIES",
                               "MORE",
                               "SKIP INTRO",
                               "PRESET",
                               "RELOAD SETTINGS",
                               "PREVIOUS",
                               "BACK",
                               "ORIGINAL",
                               "FILL",
                               "ON",
                               "OFF",
                               "CUSTOM",
                               "VANILLA",
                               "RECOMMENDED",
                               "ENHANCED",
                               u"AUTO FOLLOWS PCSX2; MATCH OVERRIDES TO ITS ASPECT.",
                               u"AUTO USES SCREEN EDGES; A RATIO SETS A CENTERED BOX.",
                               u"SCALE ANCHORED DRIVING HUD ELEMENTS FROM 0.5 TO 1.5.",
                               u"DRIVING FIELD OF VIEW MULTIPLIER: 0.5 TO 2.0.",
                               u"ORIGINAL FRAME OR FIT THE WHOLE MOVIE WITHOUT CUTTING.",
                               u"SKIP THE STARTUP AND OPENING VIDEOS. RESTART REQUIRED.",
                               u"VANILLA KEEPS STARTUP; RECOMMENDED SKIPS IT AND FITS VIDEOS.",
                               u"MORE STARTUP AND PRESET OPTIONS.",
                               u"READ THE CURRENT SETTINGS FILE AGAIN.",
                               u"RETURN TO THE PREVIOUS PAGE.",
                               u"CONFIGURE THE FUSION FIX."},
                              {"FORMAT IMAGE",
                               "ZONE HUD",
                               "TAILLE HUD",
                               "CHAMP DE VISION",
                               "VIDEOS",
                               "SUITE",
                               "PASSER INTRO",
                               "PROFIL",
                               "RECHARGER",
                               "PRECEDENT",
                               "RETOUR",
                               "ORIGINAL",
                               "ADAPTER",
                               "OUI",
                               "NON",
                               "PERSONNALISE",
                               "ORIGINAL",
                               "CONSEILLE",
                               "AMELIORE",
                               u"AUTO SUIT PCSX2; SINON CHOISIR LE MEME FORMAT.",
                               u"AUTO AUX BORDS; UN FORMAT CENTRE LE HUD.",
                               u"TAILLE DU HUD DE CONDUITE: DE 0,5 A 1,5.",
                               u"CHAMP DE VISION EN CONDUITE: DE 0,5 A 2,0.",
                               u"CADRE ORIGINAL OU VIDEO ENTIERE ADAPTEE.",
                               u"PASSE LES VIDEOS DE DEMARRAGE ET D'OUVERTURE. REDEMARRAGE REQUIS.",
                               u"ORIGINAL GARDE LE DEMARRAGE; CONSEILLE LE PASSE.",
                               u"AUTRES OPTIONS ET PROFILS.",
                               u"RECHARGER LE FICHIER DE REGLAGES.",
                               u"RETOURNER A LA PAGE PRECEDENTE.",
                               u"CONFIGURER LE CORRECTIF GRAND ECRAN."},
                              {"BILDFORMAT",
                               "HUD-BEREICH",
                               "HUD-GROSSE",
                               "SICHTFELD",
                               "VIDEOS",
                               "MEHR",
                               "INTRO UBERSPRINGEN",
                               "PROFIL",
                               "NEU LADEN",
                               "ZURUCK",
                               "BEENDEN",
                               "ORIGINAL",
                               "EINPASSEN",
                               "AN",
                               "AUS",
                               "EIGENES",
                               "ORIGINAL",
                               "EMPFOHLEN",
                               "ERWEITERT",
                               u"AUTO FOLGT PCSX2; SONST GLEICHES FORMAT WAEHLEN.",
                               u"AUTO AM RAND; EIN FORMAT ZENTRIERT DAS HUD.",
                               u"GRASSE DES FAHRT-HUDS: 0,5 BIS 1,5.",
                               u"SICHTFELD BEIM FAHREN: FAKTOR 0,5 BIS 2,0.",
                               u"ORIGINAL ODER DAS GANZE VIDEO EINPASSEN.",
                               u"START- UND EROEFFNUNGSVIDEOS UEBERSPRINGEN. NEUSTART ERFORDERLICH.",
                               u"ORIGINAL BEHAELT START; EMPFOHLEN UEBERSPRINGT IHN.",
                               u"WEITERE STARTOPTIONEN UND PROFILE.",
                               u"EINSTELLUNGSDATEI ERNEUT LADEN.",
                               u"ZUR VORHERIGEN SEITE ZURUECK.",
                               u"BREITBILD-FIX EINSTELLEN."},
                              {"FORMATO",
                               "AREA HUD",
                               "DIMENSIONE HUD",
                               "CAMPO VISIVO",
                               "FILMATI",
                               "ALTRO",
                               "SALTA INTRO",
                               "PROFILO",
                               "RICARICA",
                               "INDIETRO",
                               "ESCI",
                               "ORIGINALE",
                               "ADATTA",
                               "SI",
                               "NO",
                               "PERSONALE",
                               "ORIGINALE",
                               "CONSIGLIATO",
                               "AVANZATO",
                               u"AUTO SEGUE PCSX2; ALTRIMENTI USA LO STESSO FORMATO.",
                               u"AUTO AI BORDI; UN FORMATO CENTRA L'HUD.",
                               u"DIMENSIONE HUD DI GUIDA: DA 0,5 A 1,5.",
                               u"CAMPO VISIVO DI GUIDA: FATTORE DA 0,5 A 2,0.",
                               u"ORIGINALE O ADATTA L'INTERO FILMATO.",
                               u"SALTA I FILMATI INIZIALI E D'APERTURA. RIAVVIO NECESSARIO.",
                               u"ORIGINALE MANTIENE L'AVVIO; CONSIGLIATO LO SALTA.",
                               u"ALTRE OPZIONI E PROFILI.",
                               u"RICARICA IL FILE DELLE IMPOSTAZIONI.",
                               u"TORNA ALLA PAGINA PRECEDENTE.",
                               u"CONFIGURA LA CORREZIONE WIDESCREEN."},
                              {"FORMATO",
                               "AREA HUD",
                               "TAMANO HUD",
                               "CAMPO DE VISION",
                               "VIDEOS",
                               "MAS",
                               "OMITIR INTRO",
                               "PERFIL",
                               "RECARGAR",
                               "ANTERIOR",
                               "SALIR",
                               "ORIGINAL",
                               "AJUSTAR",
                               "SI",
                               "NO",
                               "PERSONAL",
                               "ORIGINAL",
                               "RECOMENDADO",
                               "MEJORADO",
                               u"AUTO SIGUE PCSX2; SI NO, USA EL MISMO FORMATO.",
                               u"AUTO EN LOS BORDES; UN FORMATO CENTRA EL HUD.",
                               u"TAMANO DEL HUD DE CONDUCCION: DE 0,5 A 1,5.",
                               u"CAMPO DE VISION AL CONDUCIR: DE 0,5 A 2,0.",
                               u"MARCO ORIGINAL O AJUSTAR TODO EL VIDEO.",
                               u"OMITE LOS VIDEOS INICIALES Y DE APERTURA. REINICIO NECESARIO.",
                               u"ORIGINAL CONSERVA EL INICIO; RECOMENDADO LO OMITE.",
                               u"MAS OPCIONES Y PERFILES.",
                               u"RECARGA EL ARCHIVO DE AJUSTES.",
                               u"VUELVE A LA PAGINA ANTERIOR.",
                               u"CONFIGURA LA CORRECCION PANORAMICA."}};
struct ControlLanguage
{
    const char* controls;
    const char* sensitivity;
    const char* invert;
    const char* padSpeed;
    const char* padInvert;
    const char* split;
    const char16_t* controlsHelp;
    const char16_t* sensitivityHelp;
    const char16_t* invertHelp;
    const char16_t* padSpeedHelp;
    const char16_t* padInvertHelp;
    const char16_t* splitHelp;
};
const ControlLanguage controlLanguages[] = {
    {"CONTROLS", "MOUSE SENSITIVITY", "INVERT MOUSE Y", "GAMEPAD CAMERA SPEED", "INVERT GAMEPAD Y", "SIDE BY SIDE SPLIT", u"MOUSE CAMERA OPTIONS.", u"SPEED OF THE MOUSE CAMERA: 0.10 TO 5.00.", u"REVERSE VERTICAL MOUSE LOOK.", u"GAMEPAD CAMERA SPEED: 0.25 TO 4.00.", u"REVERSE VERTICAL GAMEPAD LOOK.", u"APPLIES TO THE NEXT SPLIT SCREEN RACE."},
    {"COMMANDES", "SENSIBILITE SOURIS", "INVERSER SOURIS Y", "VITESSE CAMERA MANETTE", "INVERSER MANETTE Y", "ECRAN PARTAGE COTE A COTE", u"OPTIONS DE LA CAMERA SOURIS.", u"VITESSE DE LA SOURIS: DE 0,10 A 5,00.", u"INVERSER LA CAMERA VERTICALE.", u"VITESSE CAMERA: DE 0,25 A 4,00.", u"INVERSER LA CAMERA VERTICALE.", u"ACTIF A LA PROCHAINE COURSE PARTAGEE."},
    {"STEUERUNG", "MAUSEMPFINDLICHKEIT", "MAUS Y UMKEHREN", "GAMEPAD KAMERATEMPO", "GAMEPAD Y UMKEHREN", "SPLITSCREEN SEITLICH", u"OPTIONEN FUER DIE MAUSKAMERA.", u"MAUSGESCHWINDIGKEIT: 0,10 BIS 5,00.", u"VERTIKALE MAUSANSICHT UMKEHREN.", u"KAMERATEMPO: 0,25 BIS 4,00.", u"VERTIKALE ANSICHT UMKEHREN.", u"GILT AB DEM NAECHSTEN GETEILTEN RENNEN."},
    {"CONTROLLI", "SENSIBILITA MOUSE", "INVERTI MOUSE Y", "VELOCITA CAMERA PAD", "INVERTI PAD Y", "SCHERMO DIVISO AFFIANCATO", u"OPZIONI DELLA VISUALE MOUSE.", u"VELOCITA DEL MOUSE: DA 0,10 A 5,00.", u"INVERTI LA VISUALE VERTICALE.", u"VELOCITA CAMERA: DA 0,25 A 4,00.", u"INVERTI LA VISUALE VERTICALE.", u"ATTIVO DALLA PROSSIMA GARA CONDIVISA."},
    {"CONTROLES", "SENSIBILIDAD RATON", "INVERTIR RATON Y", "VELOCIDAD CAMARA MANDO", "INVERTIR MANDO Y", "PANTALLA DIVIDIDA LATERAL", u"OPCIONES DE LA CAMARA CON RATON.", u"VELOCIDAD DEL RATON: DE 0,10 A 5,00.", u"INVERTIR LA CAMARA VERTICAL.", u"VELOCIDAD CAMARA: DE 0,25 A 4,00.", u"INVERTIR LA CAMARA VERTICAL.", u"SE APLICA EN LA PROXIMA CARRERA DIVIDIDA."},
};
Entry entries[6], mainEntries[6];
const char16_t* descriptions[6];
const char16_t* mainDescriptions[6];
char16_t labels[6][96];
const char16_t pluginsLabel[] = u"PLUGINS";
Value* values;
void (*onChange)(bool);
void (*onError)();
void (*onReload)();
int page, preset = -1;
bool refreshPending;
SafetyMipsMid listHook, descriptionHook;
safetymips::GameInline<int(uintptr_t)> updateHook;
pcsx2::GameFunction<int(uintptr_t, int)> confirm, back, up, down, left, right;
pcsx2::GameFunction<void(uintptr_t, int, int)> refresh;

constexpr unsigned textLanguageOption = 12;
constexpr unsigned skipPressStartOption = 13;
constexpr const char* languageCodes[] = {"auto", "en", "fr", "de", "it", "es"};
constexpr const char* languageNames[] = {"AUTO", "ENGLISH", "FRANCAIS", "DEUTSCH", "ITALIANO", "ESPANOL"};
constexpr unsigned languageChoices = sizeof(languageCodes) / sizeof(*languageCodes);

unsigned LanguageChoice()
{
    for (unsigned choice = 1; choice < languageChoices; ++choice)
        if (std::strcmp(values[textLanguageOption].value, languageCodes[choice]) == 0)
            return choice;
    return 0;
}

const Language& CurrentLanguage()
{
    if (const unsigned choice = LanguageChoice())
        return languages[choice - 1];

    const uint32_t language = *reinterpret_cast<const uint32_t*>(0x4E2918);
    switch (language)
    {
        case 2:
            return languages[1];
        case 3:
            return languages[2];
        case 4:
            return languages[3];
        case 6:
            return languages[4];
        default:
            return languages[0];
    }
}
void Label(unsigned row, const char* name, const char* value = nullptr)
{
    char text[96];
    if (value)
        std::snprintf(text, sizeof(text), "%s: %s", name, value);
    else
        std::snprintf(text, sizeof(text), "%s", name);
    unsigned i = 0;
    for (; i + 1 < 96 && text[i]; ++i)
        labels[row][i] = static_cast<unsigned char>(text[i]);
    labels[row][i] = 0;
    entries[row] = {labels[row], 0, 0, 0};
}
const char* Toggle(unsigned index, const Language& language)
{
    return console::ps2settings::boolean(values[index], true) ? language.on : language.off;
}
int DetectedPreset()
{
    const bool intro = console::ps2settings::boolean(values[3], true);
    const bool pressStart = console::ps2settings::boolean(values[skipPressStartOption], true);
    const int movie = std::clamp(std::atoi(values[5].value), 0, 1);
    const bool fullRate = std::clamp(std::atoi(values[11].value), 30, 60) > 30;
    if (int(intro) != movie || intro != fullRate || intro != pressStart)
        return -1;
    return intro ? (preset == 2 ? 2 : 1) : 0;
}
void Number(char* out, float value, float low, float high)
{
    if (!std::isfinite(value))
        value = 1.0f;
    int n = int(std::clamp(value, low, high) * 100.0f + 0.5f);
    std::snprintf(out, 16, "%d.%02d", n / 100, n % 100);
}
void Populate()
{
    const auto& l = CurrentLanguage();
    const auto& controls = controlLanguages[&l - languages];
    char number[16];
    if (page == 1)
    {
        Label(0, l.aspect, values[0].value);
        descriptions[0] = l.displayHelp;
        Label(1, l.box, values[1].value);
        descriptions[1] = l.hudHelp;
        Number(number, std::strtof(values[4].value, nullptr), 0.5f, 1.5f);
        Label(2, l.scale, number);
        descriptions[2] = l.sizeHelp;
        Number(number, std::strtof(values[2].value, nullptr), 0.5f, 2.0f);
        Label(3, l.fov, number);
        descriptions[3] = l.fovHelp;
        Label(4, l.movie, std::clamp(std::atoi(values[5].value), 0, 1) == 0 ? l.original : l.fit);
        descriptions[4] = l.movieHelp;
        Label(5, l.more);
        descriptions[5] = l.moreHelp;
    }
    else if (page == 2)
    {
        // The preset comes first, then what it sets
        const char* presets[] = {l.vanilla, l.recommended, l.enhanced};
        const int activePreset = DetectedPreset();
        Label(0, l.preset, activePreset < 0 ? l.custom : presets[activePreset]);
        descriptions[0] = l.presetHelp;
        Label(1, l.intro, Toggle(3, l));
        descriptions[1] = l.introHelp;
        Label(2, "FPS", std::clamp(std::atoi(values[11].value), 30, 60) > 30 ? "60" : l.original);
        static const char16_t* fpsHelp[] = {
            u"ORIGINAL OR 60 FPS. RESTART REQUIRED.",
            u"ORIGINAL OU 60 FPS. REDEMARRAGE NECESSAIRE.",
            u"ORIGINAL ODER 60 FPS. NEUSTART ERFORDERLICH.",
            u"ORIGINALE O 60 FPS. RIAVVIO NECESSARIO.",
            u"ORIGINAL O 60 FPS. REINICIO NECESARIO."};
        descriptions[2] = fpsHelp[&l - languages];
        Label(3, controls.controls);
        descriptions[3] = controls.controlsHelp;
        constexpr const char* languageLabels[] = {"LANGUAGE", "LANGUE", "SPRACHE", "LINGUA", "IDIOMA"};
        constexpr const char16_t* languageHelp[] = {
            u"AUTO USES THE GAME LANGUAGE FOR THIS MENU.",
            u"AUTO UTILISE LA LANGUE DU JEU POUR CE MENU.",
            u"AUTO NUTZT DIE SPIELSPRACHE FUER DIESES MENUE.",
            u"AUTO USA LA LINGUA DI GIOCO PER QUESTO MENU.",
            u"AUTO USA EL IDIOMA DEL JUEGO PARA ESTE MENU."};
        Label(4, languageLabels[&l - languages], languageNames[LanguageChoice()]);
        descriptions[4] = languageHelp[&l - languages];
        constexpr const char* titleLabels[] = {
            "SKIP PRESS START", "PASSER L'ECRAN START", "START UEBERSPRINGEN", "SALTA SCHERMATA START", "SALTAR PULSA START"};
        constexpr const char16_t* titleHelp[] = {
            u"CONTINUE PAST PRESS START AUTOMATICALLY. RESTART REQUIRED.",
            u"PASSER L'ECRAN START AUTOMATIQUEMENT. REDEMARRAGE NECESSAIRE.",
            u"STARTBILDSCHIRM AUTOMATISCH UEBERSPRINGEN. NEUSTART ERFORDERLICH.",
            u"PASSA OLTRE LA SCHERMATA START. RIAVVIO NECESSARIO.",
            u"OMITE LA PANTALLA PULSA START. REINICIO NECESARIO."};
        Label(5, titleLabels[&l - languages], Toggle(skipPressStartOption, l));
        descriptions[5] = titleHelp[&l - languages];
    }
    else
    {
        Number(number, std::strtof(values[6].value, nullptr), 0.1f, 5.0f);
        Label(0, controls.sensitivity, number);
        descriptions[0] = controls.sensitivityHelp;
        Label(1, controls.invert, console::ps2settings::boolean(values[7], false) ? l.on : l.off);
        descriptions[1] = controls.invertHelp;
        Number(number, std::strtof(values[8].value, nullptr), 0.25f, 4.0f);
        Label(2, controls.padSpeed, number);
        descriptions[2] = controls.padSpeedHelp;
        Label(3, controls.padInvert, console::ps2settings::boolean(values[9], false) ? l.on : l.off);
        descriptions[3] = controls.padInvertHelp;
        Label(4, l.previous);
        descriptions[4] = l.backHelp;
        Label(5, controls.split, console::ps2settings::boolean(values[10], false) ? l.on : l.off);
        descriptions[5] = controls.splitHelp;
    }
}
void InvalidateDescription()
{
    // The native ticker at 0x1E89788 rebuilds its text when the selected row
    // differs from its cached index. A translated description can change in place.
    *reinterpret_cast<int8_t*>(0x1E89788 + 0x6D) = -1;
}
void Rebuild(unsigned selected)
{
    const auto* oldHelp = descriptions[selected];
    Populate();
    if (descriptions[selected] != oldHelp)
        InvalidateDescription();
    refresh(0x1E89544, 0, 0);
}
void OptionsReloaded()
{
    refreshPending = true;
}
bool WriteValues(const unsigned* indices, const char* const* newValues, unsigned count, bool restart)
{
    const bool oldIntro = console::ps2settings::boolean(values[3], true);
    const bool oldPressStart = console::ps2settings::boolean(values[skipPressStartOption], true);
    const bool oldFullRate = std::atoi(values[11].value) > 30;
    static PCSX2FIniRequest request;
    request = {};
    request.size = sizeof(request);
    request.version = 1;
    request.operation = PCSX2F_SETTINGS_WRITE;
    request.count = count;
    for (unsigned i = 0; i < count; ++i)
    {
        auto& entry = request.entries[i];
        if (!console::ps2settings::copy(entry.section, values[indices[i]].section) ||
            !console::ps2settings::copy(entry.key, values[indices[i]].key) ||
            !console::ps2settings::copy(entry.value, newValues[i]))
            return false;
    }
    if (console::ps2settings::request(request) != PCSX2F_SETTINGS_OK)
    {
        onError();
        return false;
    }
    for (unsigned i = 0; i < count; ++i)
        console::ps2settings::copy(values[indices[i]].value, newValues[i]);
    onChange(restart && (oldIntro != console::ps2settings::boolean(values[3], true) ||
                         oldPressStart != console::ps2settings::boolean(values[skipPressStartOption], true) ||
                         oldFullRate != (std::atoi(values[11].value) > 30)));
    return true;
}
void ChangeOne(unsigned index, const char* value, bool restart = false)
{
    const unsigned indices[] = {index};
    const char* next[] = {value};
    if (WriteValues(indices, next, 1, restart))
        preset = -1;
}
void ChangeNumber(unsigned index, int direction, int minimum, int maximum)
{
    char* end = nullptr;
    float value = std::strtof(values[index].value, &end);
    if (end == values[index].value || !std::isfinite(value))
        value = 1.0f;
    value = std::clamp(value, float(minimum) / 20.0f, float(maximum) / 20.0f);
    const int step = std::clamp(int(value * 20.0f + 0.5f) + direction, minimum, maximum);
    char number[16];
    console::decimal(number, float(step) / 20.0f, 2);
    ChangeOne(index, number);
}
void Change(unsigned row, int direction)
{
    if (page == 1 && row < 2)
    {
        static const char* choices[] = {"Auto", "4:3", "16:9", "21:9", "32:9"};
        unsigned index = row;
        int current = -1;
        for (int i = 0; i < 5; ++i)
            if (!std::strcmp(values[index].value, choices[i]))
                current = i;
        current = current < 0 ? 0 : (current + direction + 5) % 5;
        ChangeOne(index, choices[current]);
    }
    else if (page == 1 && (row == 2 || row == 3))
    {
        ChangeNumber(row == 2 ? 4 : 2, direction, 10, row == 2 ? 30 : 40);
    }
    else if (page == 1 && row == 4)
    {
        ChangeOne(5, std::atoi(values[5].value) == 0 ? "1" : "0");
    }
    else if (page == 2 && row == 1)
        ChangeOne(3, console::ps2settings::boolean(values[3], true) ? "0" : "1", true);
    else if (page == 2 && row == 4)
    {
        const unsigned next = (int(LanguageChoice()) + direction + int(languageChoices)) % languageChoices;
        ChangeOne(textLanguageOption, languageCodes[next]);
    }
    else if (page == 2 && row == 5)
        ChangeOne(skipPressStartOption, console::ps2settings::boolean(values[skipPressStartOption], true) ? "0" : "1", true);
    else if (page == 3 && row == 0)
    {
        ChangeNumber(6, direction, 2, 100);
    }
    else if (page == 3 && row == 1)
        ChangeOne(7, console::ps2settings::boolean(values[7], false) ? "0" : "1");
    else if (page == 3 && row == 2)
    {
        ChangeNumber(8, direction, 5, 80);
    }
    else if (page == 3 && row == 3)
        ChangeOne(9, console::ps2settings::boolean(values[9], false) ? "0" : "1");
    else if (page == 3 && row == 5)
        ChangeOne(10, console::ps2settings::boolean(values[10], false) ? "0" : "1");
    else if (page == 2 && row == 0)
    {
        const int activePreset = DetectedPreset();
        const int next = activePreset < 0 ? 0 : (activePreset + direction + 3) % 3;
        const unsigned indices[] = {3, 5, 11, skipPressStartOption};
        const char* toggle = next ? "1" : "0";
        const char* settings[] = {toggle, toggle, next ? "60" : "30", toggle};
        if (WriteValues(indices, settings, std::size(indices), true))
            preset = next;
    }
    else if (page == 2 && row == 2)
        ChangeOne(11, std::clamp(std::atoi(values[11].value), 30, 60) > 30 ? "30" : "60", true);
}
void Close(uint16_t* selection)
{
    std::memcpy(entries, mainEntries, sizeof(entries));
    std::memcpy(descriptions, mainDescriptions, sizeof(descriptions));
    descriptions[5] = CurrentLanguage().pluginsHelp;
    *selection = 5;
    page = 0;
    InvalidateDescription();
    refresh(0x1E89544, 0, 0);
}
int Update(uintptr_t owner)
{
    auto* selected = reinterpret_cast<uint16_t*>(owner + 20);
    if (refreshPending)
    {
        refreshPending = false;
        if (page && *selected < 6)
            Rebuild(*selected);
        else if (!page)
        {
            const auto* help = CurrentLanguage().pluginsHelp;
            if (*selected == 5 && descriptions[5] != help)
                InvalidateDescription();
            descriptions[5] = help;
        }
    }
    if (!page)
    {
        if (*selected != 5 || !confirm(0x4F50C0, 0))
            return updateHook.call(owner);
        onReload();
        std::memcpy(mainEntries, entries, sizeof(entries));
        std::memcpy(mainDescriptions, descriptions, sizeof(descriptions));
        page = 1;
        *selected = 0;
        Rebuild(*selected);
        return 1;
    }
    if (back(0x4F50C0, 0))
    {
        if (page == 3 || page == 2)
        {
            const bool controlsPage = page == 3;
            page = controlsPage ? 2 : 1;
            *selected = controlsPage ? 3 : 5;
            Rebuild(*selected);
        }
        else
            Close(selected);
        return 1;
    }
    if (up(0x4F50C0, 0) && *selected > 0)
        --*selected;
    else if (down(0x4F50C0, 0) && *selected < 5)
        ++*selected;
    else
    {
        const bool decrease = left(0x4F50C0, 0) != 0;
        const bool increase = right(0x4F50C0, 0) != 0;
        const bool select = confirm(0x4F50C0, 0) != 0;
        if (select && page == 1 && *selected == 5)
        {
            page = 2;
            *selected = 0;
        }
        else if (select && page == 2 && *selected == 3)
        {
            page = 3;
            *selected = 0;
        }
        else if (select && page == 3 && *selected == 4)
        {
            *selected = 3;
            page = 2;
        }
        else if (decrease || increase || select)
            Change(*selected, decrease ? -1 : 1);
        else
            return 1;
        Rebuild(*selected);
    }
    return 1;
}
void Install(Value* settings, void (*changed)(bool), void (*error)(), void (*reload)())
{
    values = settings;
    onChange = changed;
    onError = error;
    onReload = reload;
    confirm.bind(0x22CFE0);
    back.bind(0x22CF60);
    up.bind(0x22CEE0);
    down.bind(0x22CE60);
    left.bind(0x22CDE0);
    right.bind(0x22CD60);
    refresh.bind(0x31FDB0);
    updateHook.create(0x3181E0, Update);
    safetymips::Options opts;
    opts.execute_original = false;
    listHook = safetymips::create_mid(
        0x318454,
        [](SafetyMipsContext& r)
        {
            const uintptr_t owner = uintptr_t(r.s0);
            page = 0;
            refreshPending = false;
            std::memcpy(entries, reinterpret_cast<const void*>(owner + 24), 40);
            std::memcpy(descriptions, reinterpret_cast<const void*>(owner + 64), 20);
            entries[5] = {pluginsLabel, 0, 0, 0};
            descriptions[5] = CurrentLanguage().pluginsHelp;
            *reinterpret_cast<uint16_t*>(owner + 20) = uint16_t(r.a3);
            r.a2 = uint32_t(uintptr_t(entries));
            r.v1 = 6;
        },
        opts);
    descriptionHook = safetymips::create_mid(
        0x318498,
        [](SafetyMipsContext& r)
        {
            r.a1 = uint32_t(r.s0) + 20;
            r.a2 = uint32_t(uintptr_t(descriptions));
        },
        opts);
}
} // namespace burnout::menu
