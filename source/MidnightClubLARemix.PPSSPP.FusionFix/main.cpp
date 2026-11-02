#include "../Shared/Console/PatchNotice.hpp"
#include "Game.hpp"
#include "../Shared/Console/PortableCfg.hpp"
extern "C" { PSP_MODULE_INFO("MCLARemix.FusionFix", PSP_MODULE_USER, 2, 0); }
namespace mc3 {
namespace {
int Install() {
    if (!BeginWithNotice()) return -1;
    IniValue defaults[] = {{"MAIN", "DualAnalogPatch", "1"}, {"MAIN", "ForceAspectRatio", "Auto"},
                           {"MAIN", "HudScale", "1.0"}, {"MAIN", "HudAspectRatioConstraint", "Auto"},
                           {"MAIN", "SkipIntro", "1"}, {"MAIN", "FOVFactor", "1.0"},
                           {"MAIN", "CutsceneBorders", "3"}, {"MAIN", "TextLanguage", "auto"}};
    CompleteCfg("ms0:/PSP/PLUGINS/MidnightClubLARemix.PPSSPP.FusionFix/MidnightClubLARemix.PPSSPP.FusionFix.cfg", defaults);
    skipIntro = inireader.ReadInteger("MAIN", "SkipIntro", 1) != 0;
    dualAnalog = inireader.ReadInteger("MAIN", "DualAnalogPatch", 1) != 0;
    ReadMenuOptions();
    // Resolve all game sites before any feature writes or dereferences them.
    constexpr const char* sites[] = {
        "34 01 18 C6 0E 3C 04 3C 35 FA 84 34",
        "10 00 06 A2 11 00 05 A2",
        "00 00 A0 A0 01 00 A7 A0 02 00 A6 A0",
        "?? ?? 8C C7 02 00 00 10 07 63 00 46 06 B3 00 46 07 00 40 16 80 00 0C E6",
        "FF 00 2A 31 ?? ?? 09 3C 1D 00 40 15 ?? ?? 29 25 40 52 04 00",
        "F0 FF BD 27 30 01 85 8C 00 00 B0 AF 25 80 80 00 01 00 A4 30",
        "F0 FF BD 27 ?? ?? 85 8F 00 00 BF AF 05 00 85 10",
        "0C 00 A2 8F 00 00 AC 8F FF 00 43 30",
        "00 00 8F C4 00 49 07 00 80 50 07 00 04 00 8C C4",
        "D0 FF BD 27 0C 00 B1 AF FF 00 B1 30 04 00 85 8C",
        "C0 FF BD 27 1C 00 B0 AF 25 80 80 00 20 00 B1 AF 25 88 A0 00 0C 00 04 8E",
        "F0 FF BD 27 44 00 85 8C 00 00 BF AF 01 00 A6 30 0B 00 C0 10 00 02 A5 30",
        "E0 FF BD 27 ?? ?? 05 3C 10 00 B0 AF ?? ?? B0 24 08 00 05 8E 00 CC 06 3C 00 00 A6 AC",
        "90 FF BD 27 54 00 B0 AF 25 80 80 00 ?? ?? 84 8F 7F 43 07 3C 04 00 86 8C",
        "D0 FF BD 27 0C 00 B1 AF ?? ?? 11 3C ?? ?? 31 26 08 00 25 8E 00 C9 06 3C",
        "F0 FF BD 27 04 00 B1 AF ?? ?? 11 3C ?? ?? 31 26 48 00 24 92 00 00 B0 AF 08 00 B2 AF",
        "70 FD BD 27 70 02 B0 AF 25 80 80 00 74 02 B1 AF ?? ?? 04 3C 25 88 A0 00 78 02 B2 AF",
        "30 FE BD 27 B8 01 B0 AF 25 80 80 00 C0 01 B2 AF 04 00 12 8E 25 20 40 02",
        "90 FE BD 27 ?? ?? 86 8F 4C 01 B3 AF 0C 00 D3 8C 5C 01 B7 AF 25 B8 80 00",
        "A0 FF BD 27 38 00 B3 AF 25 98 80 00 00 00 24 8D 2C 00 B0 AF 25 80 C0 00 04 00 25 8D",
        "E0 FF BD 27 00 00 B0 AF 04 00 B1 AF 25 80 80 00 25 88 A0 00 08 00 24 8E 21 28 C6 00",
        "90 02 86 90 21 00 C0 10 FF 00 A5 30 BC 02 86 8C 80 38 06 00 21 38 87 00",
    };
    for (const auto* site : sites)
    {
        if (!pattern.get_first(site, 0))
        {
            PatchRejected(PSP_HOOK_UNSUPPORTED_INSTRUCTION);
            return -1;
        }
    }
    const auto hudManager = pattern.get_first("D0 FF BD 27 0C 00 B1 AF ?? ?? 11 3C ?? ?? 31 26 08 00 25 8E 00 C9 06 3C", 0);
    constexpr auto hudElement = "04 00 A5 8C 09 F8 A0 00 21 20 86 00";
    if (!hudManager || pattern.text_size < 0x7C0 || hudManager < pattern.text_addr ||
        hudManager - pattern.text_addr > pattern.text_size - 0x7C0 ||
        !range_pattern.get_first(hudManager, 0x7C0, hudElement, 0) ||
        range_pattern.get(maxHudElementHooks, hudManager, 0x7C0, hudElement, 0))
    {
        PatchRejected(PSP_HOOK_UNSUPPORTED_INSTRUCTION);
        return -1;
    }
    InstallAspect();
    InstallHud();
    InstallCamera();
    InstallIntro();
    InstallNativeMenu();
#ifndef NDEBUG
    logger.WriteF("Patches: %u aspect %f", unsigned(injector::detail::startup.error), double(targetAspect));
#endif
    return Finish();
}
}
}
extern "C" int module_start(SceSize, void*) {
    logger.SetPath("ms0:/PSP/PLUGINS/MidnightClubLARemix.PPSSPP.FusionFix/MidnightClubLARemix.PPSSPP.FusionFix.log");
    if (!mc3::Start("MC3", "ms0:/PSP/PLUGINS/MidnightClubLARemix.PPSSPP.FusionFix/MidnightClubLARemix.PPSSPP.FusionFix.cfg",
        "ms0:/PSP/PLUGINS/MidnightClubLARemix.PPSSPP.FusionFix/MidnightClubLARemix.PPSSPP.FusionFix.log")) return 0;
    return mc3::Install();
}
