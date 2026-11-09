#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdio>
#include "../../external/injector/include/psp/nanoprintf.h"
extern "C" {
#include <pspdisplay.h>
#include <pspgu.h>
#include <pspge.h>
#include <psputility_sysparam.h>
}
#include "../Shared/Console/PatchNotice.hpp"
#include "../Shared/Console/SocomInput.hpp"
#include "Sites.hpp"
#include "../Shared/Console/PortableCfg.hpp"
#include "../Shared/Console/Viewport.hpp"
#include "../Shared/Console/PadButtonArtwork.hpp"
#include "../Shared/Console/Text.hpp"
extern "C" { PSP_MODULE_INFO("FireteamBravo.FusionFix", PSP_MODULE_USER, 2, 0); }
namespace {
using console::socom::Input;
using namespace console::portable;
#include "Geometry.hpp"
#include "Hud.hpp"
#include "HudText.hpp"
#include "Graphics.hpp"
#include "FrameRate.hpp"
#include "PadIcons.hpp"
#include "NativeMenu.hpp"
injector::hook_back<int(Input*, float)> update;
SafetyMipsMid loading, dialogue;
int* introState;
uintptr_t introDialogue;
bool dialogueSkipped;
constexpr char cfgPath[] = "ms0:/PSP/PLUGINS/SOCOM.FireteamBravo.PPSSPP.FusionFix/SOCOM.FireteamBravo.PPSSPP.FusionFix.cfg";
void SkipIntro() { *introState = 2; }
// The callbacks below use no VFPU and call no game code. Saving VFPU state
// would also restore the prefix registers before returning to the game, which
// makes PPSSPP's JIT drop its default-prefix assumption for the whole session.
safetymips::Options Scalar(bool executeOriginal = true) {
    safetymips::Options options; options.preserve = PSP_HOOK_SAVE_FPU; options.execute_original = executeOriginal;
    return options;
}
int Update(Input* input, float delta) {
    const int result = update.fun(input, delta);
    NativeMenu::Input(input);
    console::socom::KeepAwake(input);
    return result;
}
int Exit(int status) { Unthrottle(false); return sceKernelExitDeleteThread(status); }
int Terminate(SceUID thread) { Unthrottle(false); return sceKernelTerminateDeleteThread(thread); }
int Install() {
    // The first game initializes its input/intro objects asynchronously.
    sceKernelDelayThread(120000);
    if (!BeginWithNotice()) return -1;
    IniValue defaults[] = {{"MAIN", "SkipIntro", "1"}, {"MAIN", "DualAnalogPatch", "1"},
        {"MAIN", "UnthrottleEmuDuringLoading", "1"}, {"MAIN", "FOVFactor", "1.0"},
        {"MAIN", "ForceAspectRatio", "Auto"}, {"MAIN", "CutsceneBorders", "3"},
        {"MAIN", "HudScale", "1.0"}, {"MAIN", "HudAspectRatioConstraint", "Auto"},
        {"MAIN", "FPSLimit", "60"}, {"MAIN", "TextLanguage", "auto"},
        {"MAIN", "GamepadIcons", "0"}};
    (void)CompleteCfg(cfgPath, defaults);
    Geometry::fovFactor = std::clamp(inireader.ReadFloat("MAIN", "FOVFactor", 1.0f), 0.5f, 2.0f);
    Geometry::borders = std::clamp(inireader.ReadInteger("MAIN", "CutsceneBorders", 3), 0, 3);
    char aspect[64]{};
    Geometry::forcedAspect = console::parse_forced_aspect(inireader.ReadString("MAIN", "ForceAspectRatio", "Auto", aspect, sizeof(aspect)));
    Hud::scale = std::clamp(inireader.ReadFloat("MAIN", "HudScale", 1.0f), 0.5f, 1.5f);
    char hudAspect[64]{};
    Hud::constraint = console::parse_hud_aspect(inireader.ReadString("MAIN", "HudAspectRatioConstraint", "Auto", hudAspect, sizeof(hudAspect)));
    FrameRate::limit = std::clamp(inireader.ReadInteger("MAIN", "FPSLimit", 60), 30, 60);
    PadIcons::style = std::clamp(inireader.ReadInteger("MAIN", "GamepadIcons", 0), 0, 3);
    NativeMenu::ReadOptions();
    // Validate the game code before installing interdependent hooks.
    constexpr const char* sitesToCheck[] = {
        "? ? 04 3C 01 00 05 34 ? ? ? ? ? ? 84 24 ? ? 04 3C ? ? 84 8C 00 00 12 34 7C 01 85 8C D8 00 84 8C 03 00 B1 38",
        "F0 FF BD 27 00 3F 06 3C 00 80 86 44 02 63 10 46 00 00 AC E7 00 00 A0 CB 20 00 65 D0",
        "90 FF BD 27 54 00 B0 AF 25 80 80 00 4C 3D 04 3C CD CC 84 34 48 00 B4 E7 00 60 84 44 A8 01 14 C6",
        "F0 FF BD 27 FF 00 A5 30 A0 01 8C E4 00 00 AC E7 00 00 A0 CB 20 00 65 D0",
        "F0 FF BD 27 FF 00 A5 30 A0 01 8D C4 ? ? 06 3C ? ? C6 24 C4 05 CE C4 80 3F 06 3C 00 60 86 44",
        "E0 FF BD 27 40 00 8C E4 ? ? 04 3C 10 00 BF AF ? ? ? ? ? ? 84 8C 10 00 BF 8F 08 00 E0 03",
        "? ? 04 3C ? ? 91 24 01 00 05 34 40 01 32 8E ? ? ? ? 25 20 C0 02",
        "50 FF BD 27 98 00 B2 AF ? ? 12 3C 9C 00 B3 AF ? ? 53 8E 94 00 B1 AF E0 00 91 8C 18 00 65 26",
        "E0 FF BD 27 10 00 B0 AF ? ? 10 3C ? ? 04 8E 14 00 BF AF 04 00 80 10 25 28 80 00",
        "? ? 06 3C ? ? C6 24 10 00 C5 8C 00 00 C7 8C 23 28 85 00 ? ? 04 3C ? ? 88 24 25 20 E0 00 64 01 07 8D 0B 00 E0 14",
        "25 28 80 00 ? ? 10 3C 04 00 06 3C ? ? 04 8E 14 00 BF AF ? ? ? ? 00 40 C6 24 ? ? 04 8E 00 02 05 34",
        "? ? 04 8E 01 00 84 30 07 00 80 10 ? ? 04 26 ? ? 04 3C 00 00 05 8E 25 30 40 02",
        "? ? 05 3C ? ? A5 24 05 00 85 14 ? ? 04 3C ? ? 04 3C ? ? ? ? ? ? 84 24 ? ? 04 3C ? ? ? ? ? ? 84 24 25 10 00 00",
        "? ? 04 3C 06 A3 00 46 ? ? ? ? ? ? 84 24 ? ? 11 3C ? ? 24 8E 0A 00 80 10",
        "C0 FF BD 27 1C 00 85 8C 20 00 86 90 08 00 A7 84 28 00 B4 E7 00 00 85 8C 00 A0 87 44",
        "C0 FF BD 27 2C 00 B0 AF 25 80 80 00 1C 00 04 8E 02 00 85 84 00 68 85 44 0C 00 8C C4",
        "C0 FF BD 27 2C 00 B0 AF 25 80 80 00 10 00 05 8E 28 00 B4 E7 30 00 BF AF 03 00 A0 14",
        "C0 FF BD 27 2C 00 B0 AF 10 00 90 8C 28 00 B4 E7 30 00 BF AF 03 00 00 16 00 00 85 8C",
        "30 00 AC C7 34 00 AD C7 10 00 AC E7 3C 00 AC C7 14 00 AD E7 40 00 AD C7 18 00 AC E7",
        "2C 00 A7 A3 38 00 B0 AF",
        "18 00 B1 AF 25 88 80 00 02 00 04 34",
        "25 98 40 00 10 00 50 AE ? ? ? ? 25 20 00 02 14 00 42 AE ? ? ? ? 25 20 00 02 1C 00 42 A2 ? ? ? ? 25 20 00 02 1D 00 42 A2",
        "04 00 80 10 00 00 00 00 ? ? ? ? 25 20 60 02 25 80 40 00 1F 00 00 12 25 20 60 02 ? ? ? ? 25 28 00 00",
        "A0 FF BD 27 ? ? 05 3C ? ? A5 24 ? ? A0 D8 ? ? 05 3C ? ? A5 24 01 00 A5 90 38 00 B0 AF 40 00 B2 AF",
        "D0 FF BD 27 25 38 80 00 FF FF C4 30 25 30 E0 00 FF FF 87 30 20 00 B0 AF",
        "20 FF BD 27 BC 00 B1 AF 25 88 80 00 60 00 2C C6 64 00 2D C6",
        "30 FF BD 27 AC 00 B1 AF 25 88 80 00 60 00 2C C6 64 00 2D C6",
        "? ? ? 3C ? ? ? ? 02 00 11 34 ? ? 91 14 00 00 00 00",
        "25 20 40 00 25 28 00 00 25 30 00 00 25 40 00 00 ? ? ? ? ? ? E7 24 03 00 00 10",
        "74 00 18 E6 ? ? ? ? 00 00 04 AE ? ? ? ? ? ? ? ? 68 00 04 AE 7C 00 13 26",
        "00 00 06 34 00 00 07 34 2E 00 A6 A3",
        "10 00 BF 8F 08 00 E0 03 20 00 BD 27 ? ? ? ? ? ? ? ? ? ? ? ? 10 00 BF AF ? ? ? ? 25 28 00 00 10 00 BF 8F 08 00 E0 03 20 00 BD 27 ? ? ? ? ? ? ? ? 1C 00 B3 AF",
        "01 00 06 34 10 00 B0 8F 14 00 B1 8F 18 00 BF 8F 08 00 E0 03 20 00 BD 27 ? ? ? ? 14 00 B1 AF",
        "01 00 06 34 80 BF 04 3C",
        "00 00 80 44 25 28 00 00 ? ? ? ? 25 30 00 00 10 00 B0 8F 14 00 B1 8F 18 00 BF 8F 08 00 E0 03 20 00 BD 27 ? ? ? ? 14 00 B1 AF ? ? ? ? ? ? ? ? 10 00 B0 AF 25 80 80 00 20 00 A4 24 78 00 85 8C",
        "00 00 80 44 01 00 05 34 ? ? ? ? 25 30 00 00 10 00 B0 8F 14 00 B1 8F 18 00 BF 8F 08 00 E0 03 20 00 BD 27 ? ? ? ? 25 28 80 00",
        "86 B6 00 46 ? ? ? ? ? ? ? ? ? ? ? ? 84 03 44 26",
        "1D 00 04 34 ? ? ? ? 25 28 20 02",
        "04 00 04 26 ? ? ? ? 00 00 00 00 ? ? ? ? 82 D6 00 46",
        "25 20 60 02 ? ? ? ? 00 00 00 00 ? ? ? ? 02 C6 00 46",
        "80 4F 04 3C 00 A0 84 44",
        "25 20 00 00 25 10 00 00 10 00 B4 C7",
        "10 00 B0 8F 14 00 BF 8F 08 00 E0 03 20 00 BD 27 ? ? ? ? 00 00 BF AF ? ? ? ? 00 00 00 00 ? ? ? ? BB 44 04 3C",
        "C0 05 4C C6 D4 05 45 AE 50 00 B3 27 C4 05 4D C6 C8 05 4E C6 D0 05 45 26 ? ? ? ? 25 20 60 02",
        "06 73 00 46 30 00 B2 27 CC 05 11 E6 86 7B 00 46 25 20 40 02 C6 8B 00 46 ? ? ? ? D0 00 A5 8F",
        "86 6B 00 46 90 00 B1 27 46 7B 00 46 CC 05 10 E6 25 20 20 02 C6 83 00 46 ? ? ? ? D0 00 A5 8F",
        "C0 05 8C C6 D4 05 85 AE 20 00 B5 27 C4 05 8D C6 C8 05 8E C6 D0 05 85 26 ? ? ? ? 25 20 A0 02",
        "CC 05 0F E6 50 00 B3 27 D0 05 05 26 ? ? ? ? 25 20 60 02",
        "C0 05 0C C6 C4 05 0D C6 C8 05 0E C6 64 00 B1 AF 20 00 B1 27 CC 05 0F C6 D0 05 05 26 68 00 BF AF ? ? ? ? 25 20 20 02",
        "C8 05 0E E6 CC 05 14 E6 25 20 A0 02 C6 A3 00 46 ? ? ? ? 25 28 80 02",
    };
    for (const auto* site : sitesToCheck)
    {
        if (!pattern.get_first(site, 0))
        {
            PatchRejected(PSP_HOOK_UNSUPPORTED_INSTRUCTION);
            return -1;
        }
    }
    if (!pattern.get(2, Hud::markerSignature, 44) || pattern.get(3, Hud::markerSignature, 44))
    {
        PatchRejected(PSP_HOOK_UNSUPPORTED_INSTRUCTION);
        return -1;
    }
    if (!pattern.get_first("20 00 BF 8F 08 00 E0 03 30 00 BD 27 ? ? ? ? 20 00 BF 8F 08 00 E0 03 30 00 BD 27 ? ? ? ? ? ? ? ? ? ? ? ? 20 00 B0 AF", 0) &&
        !pattern.get_first("25 20 00 02 01 00 05 34 25 30 40 00 25 40 00 00 ? ? ? ? ? ? E7 24 02 00 00 10", 0))
    {
        PatchRejected(PSP_HOOK_UNSUPPORTED_INSTRUCTION);
        return -1;
    }
    Geometry::Install();
    if (!Hud::Install())
    {
        PatchRejected(PSP_HOOK_UNSUPPORTED_INSTRUCTION);
        return -1;
    }
    HudText::Install();
    Graphics::Install();
    FrameRate::Install();
    PadIcons::Install();
    NativeMenu::Install();
    logger.WriteF("Display hooks installed; game text %08X, manager %08X, camera %08X.\n",
        unsigned(pattern.text_addr), unsigned(uintptr_t(Geometry::manager)), unsigned(uintptr_t(Geometry::sceneCamera)));
    if (inireader.ReadBoolean("MAIN", "SkipIntro", true)) {
        const uintptr_t intro[] = {sites::ptr_883BC3C(), sites::ptr_883BB24(), sites::ptr_883BBD0(), sites::Required("2C 00 A7 A3 38 00 B0 AF")};
        introState = reinterpret_cast<int*>(Absolute(intro[0]));
        introDialogue = sites::Required("18 00 B1 AF 25 88 80 00 02 00 04 34", -4);
        injector::MakeCALL(intro[1], SkipIntro);
        injector::MakeCALL(intro[2], SkipIntro);
        dialogue = safetymips::create_mid(intro[3], [](SafetyMipsContext& regs) {
            const auto object = uintptr_t(regs.a0);
            const bool skip = !dialogueSkipped && object &&
                *reinterpret_cast<const uintptr_t*>(object + 206 * 4) == introDialogue &&
                *reinterpret_cast<const int*>(object + 544 * 4) == 1;
            if (skip) dialogueSkipped = true;
            *reinterpret_cast<uint8_t*>(uintptr_t(regs.sp) + 0x2C) = skip;
            *reinterpret_cast<uint32_t*>(uintptr_t(regs.sp) + 0x38) = uint32_t(regs.s0);
        }, Scalar(false));
    }
    update.fun = injector::MakeCALL(sites::ptr_8809724(), Update).get();
    if (inireader.ReadBoolean("MAIN", "DualAnalogPatch", true)) {
        injector::MakeCALL(sites::ptr_88885DC(), console::socom::Camera);
        injector::MakeCALL(sites::ptr_8888ADC(), console::socom::Camera);
        for (auto site : {sites::ptr_8888674(), sites::ptr_8888508(), sites::ptr_89DDF9C(), sites::ptr_89DDF6C()}) injector::MakeNOP(site);
        injector::WriteMemory<uint32_t>(sites::ptr_89DE278(), 0x1000000B); // Native free-look strafe branch.
    }
    if (inireader.ReadBoolean("MAIN", "UnthrottleEmuDuringLoading", true)) {
        loading = safetymips::create_mid(sites::ptr_8A9C224(), [](SafetyMipsContext&) { Unthrottle(true); }, Scalar());
        injector::MakeCALL(sites::ptr_8A9C368(), Exit);
        injector::MakeCALL(sites::ptr_8A9C17C(), Terminate);
    }
    return Finish();
}
}
extern "C" int module_start(SceSize, void*) {
    logger.SetPath("ms0:/PSP/PLUGINS/SOCOM.FireteamBravo.PPSSPP.FusionFix/SOCOM.FireteamBravo.PPSSPP.FusionFix.log");
    return console::portable::Start("APP_APPLICATION_NAME",
        cfgPath,
        "ms0:/PSP/PLUGINS/SOCOM.FireteamBravo.PPSSPP.FusionFix/SOCOM.FireteamBravo.PPSSPP.FusionFix.log") ? Install() : 0;
}
