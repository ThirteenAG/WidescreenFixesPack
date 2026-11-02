#include "../Shared/Console/PatchNotice.hpp"
#include "../Shared/Console/PSP.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include "../Shared/Console/PortableCfg.hpp"
extern "C" { PSP_MODULE_INFO("TheWarriors.FusionFix", PSP_MODULE_USER, 2, 0); }
namespace {
using namespace console::portable;
inline uintptr_t Required(const char* signature, int offset = 0)
{
    const auto address = pattern.get_first(signature, offset);
    return address;
}
#include "Display.hpp"
#include "Hud.hpp"
#include "Movie.hpp"
struct Options { bool skipIntro, dualAnalog, fps60, unthrottle; } options;
int textLanguage;
constexpr const char* unsupported[] = {
    "Fusion Fix: unsupported game code; the fix is not installed.\n",
    "Fusion Fix : version du jeu non prise en charge ; correctif non installe.\n",
    "Fusion Fix: Spielversion nicht unterstuetzt; Fix nicht installiert.\n",
    "Fusion Fix: versione del gioco non supportata; correzione non installata.\n",
    "Fusion Fix: version del juego no compatible; correccion no instalada.\n"
};
void Rejected(psp_hook_status status)
{
    Unthrottle(false);
    static bool reported = false;
    if (reported)
        return;
    reported = true;
    constexpr const char* messages[] = {
        "Fusion Fix: patch validation failed (%u); affected patches disabled.\n",
        "Fusion Fix : verification du correctif echouee (%u) ; correctifs concernes desactives.\n",
        "Fusion Fix: Patch-Pruefung fehlgeschlagen (%u); betroffene Patches deaktiviert.\n",
        "Fusion Fix: verifica della correzione fallita (%u); correzioni interessate disattivate.\n",
        "Fusion Fix: fallo de validacion (%u); correcciones afectadas desactivadas.\n"
    };
    char message[160]{};
    std::snprintf(message, sizeof(message), messages[textLanguage], unsigned(status));
    PatchNotice(message);
}
SafetyMipsInline input;
SafetyMipsMid loading;
injector::hook_back<int(int, char)> load;
int Input(uintptr_t object, int controller) {
    const int result = input.call<int>(object, controller);
    SceCtrlData pad{};
    uint8_t x = 128, y = 128;
    if (sceCtrlPeekBufferPositive(&pad, 1) > 0) {
        x = pad.Rsrv[0];
        // The native camera expects Y in the opposite direction. Its own
        // inversion option remains responsible for the final camera direction.
        y = uint8_t(255 - pad.Rsrv[1]);
    }
    if (object) {
        *reinterpret_cast<uint8_t*>(object + 0x1A) = x;
        *reinterpret_cast<uint8_t*>(object + 0x1B) = y;
    }
    return result;
}
int Loading(int object, char state) { Unthrottle(true); return load.fun(object, state); }
void ReadOptions() {
    options = { inireader.ReadBoolean("MAIN", "SkipIntro", true), inireader.ReadBoolean("MAIN", "DualAnalogPatch", true),
                inireader.ReadInteger("MAIN", "FPSLimit", 30) == 60, inireader.ReadBoolean("MAIN", "UnthrottleEmuDuringLoading", true) };
    char text[64]{};
    const char* language = inireader.ReadString("MAIN", "TextLanguage", "auto", text, sizeof(text));
    constexpr const char* languages[] = { "en", "fr", "de", "it", "es" };
    for (int i = 0; i < 5; ++i)
    {
        if (!std::strcmp(language, languages[i]))
            textLanguage = i;
    }
    Display::forced = console::parse_forced_aspect(inireader.ReadString("MAIN", "ForceAspectRatio", "Auto", text, sizeof(text)));
    Display::hudConstraint = console::parse_hud_aspect(inireader.ReadString("MAIN", "HudAspectRatioConstraint", "Auto", text, sizeof(text)));
    Display::fovFactor = std::clamp(inireader.ReadFloat("MAIN", "FOVFactor", 1.0f), 0.5f, 2.0f);
}
int Install() {
    if (injector::InitializeCheckedRuntime(Rejected) != PSP_HOOK_OK) return -1;
    // Check every site before writing any code, so an unsupported executable
    // cannot leave dependent hooks only partly installed.
    constexpr const char* sites[] = {
        "10 00 A5 27 ? ? ? ? ? ? ? ? ? ? ? ? 21 28 00 00",
        "21 10 51 00 ? ? ? ? 1C 00 50 A4",
        "02 00 42 2C ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? 00 00 A2 8C",
        "00 00 B0 AF ? ? ? ? ? ? ? ? 20 00 25 8E",
        "08 00 BF AF 04 00 B1 AF ? ? ? ? 00 00 B0 AF 06 00 03 24",
        "E0 FF BD 27 ? ? 02 3C 0C 00 BF AF 00 00 B0 AF ? ? 4C E4 08 00 B2 AF 05 00 12 24",
        "5C 00 01 C6 60 00 00 C6 04 00 BF 8F 64 00 01 E6 68 00 00 E6",
        "02 00 02 46 00 00 C3 8F",
        "D0 FF BD 27 0C 00 B3 AF ? ? 13 3C 01 00 03 24 ? ? 62 8E 00 00 B0 AF FF FF 10 24",
        "94 18 80 C4 90 18 83 C4 ? ? 02 3C ? ? 44 C4 C3 18 00 46 00 00 A1 C4 08 00 A2 C4",
        "F0 FE BD 27 FF 00 4A 31 FF FF 02 24 E0 00 BE AF 21 F0 20 01",
        "B0 F8 BD 27 24 07 B5 AF FF 00 08 31 21 A8 A0 00 10 07 B0 AF FF 00 D0 30",
        "D0 FF BD 27 10 00 B0 AF 21 40 A0 00 FF 00 C6 30 14 00 BF AF 21 80 80 00 20 00 B4 E7",
        "10 FF BD 27 B8 00 B2 AF 21 90 80 00 ? ? 04 3C ? ? 84 24 D4 00 BF AF C0 00 B4 AF",
        "E0 FF BD 27 18 00 B2 AF 21 90 80 00 ? ? 04 3C ? ? 84 24 1C 00 BF AF 14 00 B1 AF",
        "50 FD BD 27 64 02 B1 AF ? ? 02 3C 80 02 BF AF 7C 02 B7 AF 78 02 B6 AF",
        "60 FE BD 27 60 01 B4 AF 21 A0 A0 00 54 01 B1 AF 74 01 BF AF 70 01 BE AF",
        "A0 FE BD 27 2C 01 B7 AF ? ? 03 3C ? ? 62 24 14 01 B1 AF 21 B8 A0 00 01 00 05 24",
        "10 01 02 24 00 00 A2 AF ? ? D4 AE ? ? ? ? 04 00 A0 AF",
    };
    for (unsigned i = 0; i < sizeof(sites) / sizeof(sites[0]); ++i)
    {
        const auto address = pattern.get_first(sites[i], 0);
        // The intro patches extend past its signature; the input hook and
        // canvas hook start before theirs. Check those derived sites too.
        const uintptr_t before = i == 1 ? 0x30C : i == 7 ? 4 : 0;
        const uintptr_t after = i == 0 ? 48 : 32;
        if (!address || pattern.get(1, sites[i], 0) || address < pattern.text_addr ||
            address - pattern.text_addr < before || pattern.text_size < after ||
            address - pattern.text_addr > pattern.text_size - after)
        {
            PatchNotice(unsupported[textLanguage]);
            return -1;
        }
    }
    if (options.skipIntro) {
        const auto intro = Required("10 00 A5 27 ? ? ? ? ? ? ? ? ? ? ? ? 21 28 00 00");
        for (unsigned offset : {12u, 28u, 44u}) injector::MakeNOP(intro + offset);
    }
    if (options.dualAnalog)
    {
        const auto branch = Required("21 10 51 00 ? ? ? ? 1C 00 50 A4", 4);
        // Keep the left stick on movement, including while the native camera
        // modifier is held. Preserve the target and the button-history delay slot.
        const auto instruction = injector::ReadMemory<uint32_t>(branch);
        injector::WriteMemory<uint32_t>(branch, 0x10000000u | (instruction & 0xFFFF));
        input = safetymips::create_inline(branch - 0x310, Input);
    }
    if (options.fps60) {
        injector::MakeNOP(Required("02 00 42 2C ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? 00 00 A2 8C", 4));
    }
    // No VFPU use in these callbacks. Saving VFPU state would restore the prefix
    // registers before returning, making PPSSPP's JIT drop its default-prefix assumption.
    Display::Install();
    Hud::Install();
    Movie::Install();
    if (options.unthrottle) {
        safetymips::Options scalar; scalar.preserve = PSP_HOOK_SAVE_FPU;
        load.fun = injector::MakeCALL(Required("00 00 B0 AF ? ? ? ? ? ? ? ? 20 00 25 8E", 4), Loading).get();
        loading = safetymips::create_mid(Required("08 00 BF AF 04 00 B1 AF ? ? ? ? 00 00 B0 AF 06 00 03 24"),
            [](SafetyMipsContext&) { Unthrottle(false); }, scalar);
    }
    return Finish();
}
}
extern "C" int module_start(SceSize, void*) {
    constexpr auto cfg = "ms0:/PSP/PLUGINS/TheWarriors.PPSSPP.FusionFix/TheWarriors.PPSSPP.FusionFix.cfg";
    constexpr auto log = "ms0:/PSP/PLUGINS/TheWarriors.PPSSPP.FusionFix/TheWarriors.PPSSPP.FusionFix.log";
    logger.SetPath(log);
    if (!Start("WARR", cfg, log)) return 0;
    IniValue defaults[] = {{"MAIN", "SkipIntro", "1"}, {"MAIN", "DualAnalogPatch", "1"}, {"MAIN", "FPSLimit", "30"},
        {"MAIN", "UnthrottleEmuDuringLoading", "1"}, {"MAIN", "ForceAspectRatio", "Auto"},
        {"MAIN", "HudAspectRatioConstraint", "Auto"}, {"MAIN", "FOVFactor", "1"}, {"MAIN", "TextLanguage", "auto"}};
    (void)CompleteCfg(cfg, defaults);
    ReadOptions();
    return Install();
}
