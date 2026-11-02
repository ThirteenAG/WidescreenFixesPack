#include <algorithm>
#include <cmath>
#include <cstring>
extern "C"
{
#include <pspgu.h>
#include <pspge.h>
#include <pspdisplay.h>
#include <psputility_sysparam.h>
}
#include "../Shared/Console/PatchNotice.hpp"
#include "../Shared/Console/SocomInput.hpp"
#include "Sites.hpp"
#include "../Shared/Console/PortableCfg.hpp"
#include "../Shared/Console/Viewport.hpp"
#include "../Shared/Console/Text.hpp"
#include "../Shared/Console/PadButtonArtwork.hpp"
extern "C" { PSP_MODULE_INFO("FireteamBravo2.FusionFix", PSP_MODULE_USER, 2, 0); }
// The European loader (ULES00557 BOOT.BIN) overwrites the first words of the
// module loaded directly after it. Keep that range free of code.
asm(".section .text\n.globl socom2_loader_guard\nsocom2_loader_guard:\n.space 0x200\n.previous");
namespace {
using console::socom::Input;
using namespace console::portable;
#include "Geometry.hpp"
#include "Hud.hpp"
#include "Graphics.hpp"
struct Options { bool skipIntro, dualAnalog, unthrottle; } options;
#include "PadIcons.hpp"
#include "FrameRate.hpp"
#include "NativeMenu.hpp"
#include "Stick.hpp"
#include "Startup.hpp"
injector::hook_back<int(Input*, float)> update[2];
SafetyMipsMid loading;
bool installed;
void MovieComplete(uint32_t hash, uintptr_t movie)
{
    PadIcons::After(hash, movie);
    Startup::Movie(hash, movie);
}
template<unsigned Index> int Update(Input* input, float delta) {
    const int result = update[Index].fun(input, delta);
    NativeMenu::Input(input);
    if (!options.dualAnalog)
        return result;
    if (NativeMenu::level)
        input->pad.Rsrv[0] = input->pad.Rsrv[1] = 128;
    else
        Stick::Sample(input);
    const auto right = console::socom::Right(input, false);
    if (right.x || right.y) {
        *reinterpret_cast<uint8_t*>(uintptr_t(input) + 172) = 1;
        console::socom::KeepAwake(input);
    }
    return result;
}
int Exit(int status) { Unthrottle(false); return sceKernelExitDeleteThread(status); }
int Terminate(SceUID thread) { Unthrottle(false); return sceKernelTerminateDeleteThread(thread); }
// Read on the plugin's own thread. Install also runs inside the loader's
// sceKernelStartModule call, where newlib's strtol (INI parsing) intermittently
// crashed PPSSPP's JIT with the European loader.
void ReadOptions() {
    IniValue defaults[] = {{"MAIN", "SkipIntro", "1"}, {"MAIN", "DualAnalogPatch", "1"},
                           {"MAIN", "UnthrottleEmuDuringLoading", "1"},
                           {"MAIN", "FOVFactor", "1"}, {"MAIN", "ForceAspectRatio", "Auto"},
                           {"MAIN", "HudScale", "1"}, {"MAIN", "HudAspectRatioConstraint", "Auto"},
                           {"MAIN", "CutsceneBorders", "3"}, {"MAIN", "TextLanguage", "auto"},
                           {"MAIN", "GamepadIcons", "0"}, {"MAIN", "FPSLimit", "60"}};
    CompleteCfg("ms0:/PSP/PLUGINS/SOCOM.FireteamBravo2.PPSSPP.FusionFix/SOCOM.FireteamBravo2.PPSSPP.FusionFix.cfg", defaults);
    options = {inireader.ReadBoolean("MAIN", "SkipIntro", true), inireader.ReadBoolean("MAIN", "DualAnalogPatch", true),
               inireader.ReadBoolean("MAIN", "UnthrottleEmuDuringLoading", true)};
    const float fov = inireader.ReadFloat("MAIN", "FOVFactor", 1.0f);
    Geometry::fovFactor = std::clamp(fov, 0.5f, 2.0f);
    char aspect[64]{};
    Geometry::forcedAspect = console::parse_forced_aspect(inireader.ReadString("MAIN", "ForceAspectRatio", "Auto", aspect, sizeof(aspect)));
    Hud::ReadOptions();
    Geometry::borders = std::clamp(inireader.ReadInteger("MAIN", "CutsceneBorders", 3), 0, 3);
    PadIcons::ReadOptions();
    FrameRate::ReadOptions();
    NativeMenu::ReadOptions();
}
int Install() {
    if (installed || !BeginWithNotice()) return installed ? 0 : -1;
    // Validate the loaded game before writing interdependent hooks.
    constexpr const char* sitesToCheck[] = {
        "FF 00 DE 33 ? ? ? ? 25 20 20 02 25 20 C0 02 ? ? ? ? 25 28 C0 03 03 00 C4 92 03 00 80 10",
        "91 3E 06 3C 11 11 C6 34 00 80 86 44 02 63 10 46 80 3F 06 3C 00 88 86 44 43 8C 0C 46 AA 3E 06 3C",
        "90 FF BD 27 54 00 B0 AF 25 80 80 00 4C 3D 04 3C CD CC 84 34 48 00 B4 E7 00 60 84 44 A8 01 14 C6",
        "F0 FF BD 27 FF 00 A5 30 A0 01 8C E4 00 00 AC E7 C8 02 86 24 D0 02 87 24 00 00 A0 CB 20 00 65 D0",
        "F0 FF BD 27 FF 00 A5 30 A4 01 8C E4 00 00 AC E7 CC 02 86 24 D4 02 87 24 00 00 A0 CB 20 00 65 D0",
        "E0 FF BD 27 40 00 8C E4 ? ? 04 3C 10 00 BF AF ? ? ? ? ? ? 84 8C 10 00 BF 8F 08 00 E0 03",
        "? ? 06 3C 48 2D 0C E6 3C 02 A5 8C ? ? C6 24 ? ? C6 8C 6C 2D 06 AE 01 00 07 3C 25 30 A0 00",
        "? ? 14 3C 80 40 09 3C 90 00 BC E7 9C 00 B2 AF A0 00 B3 AF 00 E0 89 44 00 00 04 34 28 00 B2 27",
        "38 00 85 24 00 00 AC E4 3C 00 84 24 08 00 E0 03 00 00 8D E4 D0 FF BD 27 00 00 C8 8C 04 00 C9 8C",
        "E0 FE BD 27 08 01 B4 E7 0C 01 B6 E7 10 01 B0 AF 14 01 B1 AF 18 01 B2 AF 1C 01 BF AF 25 88 A0 00 25 80 80 00 6C 2D 04 8E",
        "00 FE BD 27 C8 01 B4 E7 CC 01 B6 E7 D0 01 B8 E7 D4 01 BA E7 D8 01 BC E7 DC 01 BE E7 E0 01 B0 AF E4 01 B1 AF",
        "? ? 11 3C 09 00 06 3C ? ? 24 8E 25 28 00 02 ? ? ? ? 00 80 C6 24 ? ? 24 8E 00 02 05 34",
        "D0 FF BD 27 20 00 B0 AF 25 80 80 00 24 00 B1 AF 28 00 BF AF ? ? ? ? 02 00 11 26 20 2F 04 8E",
        "F0 FF BD 27 00 00 BF AF ? ? 06 3C ? ? C7 24 10 00 EB 8C 21 30 80 00 00 1E 88 7C 00 5C 8C 7C",
        "10 FF BD 27 06 00 84 80 C8 00 B0 AF 25 80 A0 00 08 00 85 30 25 20 C0 00 CC 00 B1 AF D0 00 B2 AF",
        "60 FF BD 27 06 00 86 80 74 00 B1 AF 25 88 80 00 7C 00 B3 AF 08 00 C6 30 25 20 A0 00 25 98 E0 00 70 00 B0 AF 78 00 B2 AF",
        "C0 FE BD 27 28 01 B7 AF ? ? 17 3C 20 01 B5 AF 25 A8 80 00 ? ? F7 26 04 01 B4 E7 06 65 00 46",
        "C0 FF BD 27 5C 0C 85 8C 20 00 B4 E7 06 65 00 46 24 00 B0 AF 28 00 B1 AF 2C 00 B2 AF 30 00 B3 AF",
        "20 00 B0 27 9C 01 A6 8F 25 20 80 02 ? ? ? ? 25 28 00 02 00 00 00 DA 10 00 01 DA 20 00 02 DA",
        "80 FF BD 27 64 00 B1 AF 25 88 80 00 60 00 B0 AF 68 00 B2 AF 6C 00 B3 AF 70 00 B4 AF 74 00 B5 AF 78 00 BF AF 44 00 C0 10",
        "E0 FF BD 27 00 00 02 34 10 00 B0 AF 14 00 B1 AF 18 00 BF AF 33 00 A0 10 25 80 80 00 ? ? ? ?",
        "? ? 11 3C 18 00 B2 AF 1C 00 B3 AF 20 00 B4 AF 24 00 B5 AF 28 00 B6 AF 2C 00 BF AF 13 00 A0 14",
        "E0 FF BD 27 10 00 BF AF ? ? ? ? 10 00 80 A0 10 00 BF 8F 08 00 E0 03 20 00 BD 27 D0 FF BD 27",
        "E0 FF BD 27 25 30 A0 00 C0 29 06 00 80 38 06 00 21 28 A7 00 21 28 85 00 E4 00 A5 8C 10 00 BF AF",
        "E0 FF BD 27 FF 00 A7 30 3C 00 85 8C 34 00 80 AC 38 00 80 AC 10 00 BF AF 09 00 E0 10 3C 00 80 AC",
        "60 FE BD 27 7C 01 B4 AF 50 0C E2 8C FF FF 74 31 FF FF 08 31 02 00 8B 94 24 01 A8 AF 00 00 03 34",
        "02 00 06 34 ? ? 04 3C 08 00 E0 03 ? ? 86 AC",
        "25 38 00 00 ? ? ? ? 00 00 00 00 ? ? ? ? ? ? ? ? ? ? ? ? 0D 00 06 34",
        "? ? C6 24 03 00 00 10 ? ? 00 AE 02 00 04 34 ? ? 04 AE",
        "06 B3 00 46 0B 00 44 2E",
        "10 00 BF 8F 08 00 E0 03 20 00 BD 27 ? ? ? ? ? ? ? ? ? ? ? ? 10 00 BF AF ? ? ? ? 25 28 00 00 10 00 BF 8F 08 00 E0 03 20 00 BD 27 ? ? ? ? ? ? ? ? 1C 00 B3 AF",
        "01 00 06 34 10 00 B0 8F 14 00 B1 8F 18 00 BF 8F 08 00 E0 03 20 00 BD 27 ? ? ? ? 14 00 B1 AF",
        "01 00 06 34 80 BF 04 3C",
        "00 00 80 44 25 28 00 00 ? ? ? ? 25 30 00 00 10 00 B0 8F 14 00 B1 8F 18 00 BF 8F 08 00 E0 03 20 00 BD 27 ? ? ? ? 14 00 B1 AF ? ? ? ? ? ? ? ? 10 00 B0 AF 25 80 80 00 20 00 A4 24 78 00 85 8C",
        "00 00 80 44 01 00 05 34 ? ? ? ? 25 30 00 00 10 00 B0 8F 14 00 B1 8F 18 00 BF 8F 08 00 E0 03 20 00 BD 27 ? ? ? ? 25 28 80 00",
        "06 B7 00 46 ? ? ? ? ? ? ? ? ? ? ? ? 80 03 44 26",
        "1D 00 04 34 ? ? ? ? 25 28 20 02",
        "80 4F 04 3C 00 60 84 44 ? ? ? ? ? ? ? ? 00 D0 80 44",
        "25 20 00 00 25 10 00 00 24 00 B4 C7",
        "00 00 00 00 ? ? ? ? ? ? ? ? FF FF 04 24 ? ? ? ? 10 00 B0 8F",
        "60 FD BD 27 7C 02 B0 AF 25 80 80 00 80 02 B1 AF FF 00 B1 30 ? ? 04 3C ? ? 85 8C 84 02 B2 AF",
        "D0 FF BD 27 20 00 B4 AF 1C 00 B4 8C 14 00 B1 AF 04 00 86 96 1C 00 B3 AF 00 00 13 34 25 88 80 00",
        "F0 FF BD 27 B0 0C 85 8C 00 00 B0 AF 25 80 80 00 2B 20 05 00 FF 00 84 30 04 00 BF AF 06 00 80 54",
        "24 05 CD C6 50 01 B0 27 28 05 CE C6 25 20 00 02 ? ? ? ? DC 01 A5 8F 25 20 C0 02 ? ? ? ?",
        "20 05 CC C6 24 05 CD C6 28 05 CE C6 25 20 00 02 ? ? ? ? DC 01 A5 8F 25 20 C0 02 ? ? ? ?",
        "20 00 B2 27 24 05 2D C6 28 05 2E C6 34 05 25 26 ? ? ? ? 25 20 40 02 25 20 20 02 ? ? ? ?",
        "30 01 B1 27 24 05 0D C6 28 05 0E C6 34 05 05 26 ? ? ? ? 25 20 20 02 25 20 00 02 ? ? ? ?",
        "24 05 0D C6 30 01 B6 27 28 05 0E C6 34 05 05 26 ? ? ? ? 25 20 C0 02 25 20 00 02 ? ? ? ?",
        "30 00 B0 27 00 E0 84 44 2C 05 FC E6 C6 E3 00 46 ? ? ? ? 25 20 00 02 25 20 E0 02 ? ? ? ?",
        "86 63 00 46 90 00 B0 27 06 A3 00 46 25 20 00 02 ? ? ? ? D4 00 A5 8F 25 20 E0 02 ? ? ? ?",
        "20 00 B5 27 24 05 8D C6 28 05 8E C6 34 05 85 26 ? ? ? ? 25 20 A0 02 25 20 80 02 ? ? ? ?",
        "38 05 84 AE 46 A3 00 46 40 00 B2 27 34 05 85 26 ? ? ? ? 25 20 40 02 25 20 80 02 ? ? ? ?",
    };
    for (const auto* site : sitesToCheck)
    {
        if (!pattern.get_first(site, 0))
        {
            PatchRejected(PSP_HOOK_UNSUPPORTED_INSTRUCTION);
            return -1;
        }
    }
    Geometry::Install();
    Hud::Install();
    Graphics::Install();
    NativeMenu::Install();
    Startup::Install();
    PadIcons::Install();
    FrameRate::Install();
    Hud::afterMovie = MovieComplete;
    update[0].fun = injector::MakeCALL(sites::ptr_8B57BDC(), Update<0>).get();
    update[1].fun = injector::MakeCALL(sites::ptr_8815BCC(), Update<1>).get();
    if (options.dualAnalog) {
        injector::MakeCALL(sites::ptr_889D880(), console::socom::Camera);
        injector::MakeCALL(sites::ptr_889DD48(), console::socom::Camera);
        for (auto site : {sites::ptr_889D918(), sites::ptr_889D7AC(), sites::ptr_8A41C84(), sites::ptr_8A41C54()}) injector::MakeNOP(site);
    }
    if (options.unthrottle) {
        // No VFPU use: saving VFPU state would restore the prefix registers before
        // returning, making PPSSPP's JIT drop its default-prefix assumption.
        safetymips::Options scalar; scalar.preserve = PSP_HOOK_SAVE_FPU;
        loading = safetymips::create_mid(sites::ptr_8B578EC(), [](SafetyMipsContext&) { Unthrottle(true); }, scalar);
        injector::MakeCALL(sites::ptr_8B57C98(), Exit);
        injector::MakeCALL(sites::ptr_8B57834(), Terminate);
    }
    const int result = Finish(); installed = result == 0;
    if (installed)
        logger.WriteF("Camera and Flash layouts installed; manager %08X, scene %08X.\n", unsigned(uintptr_t(Geometry::manager)), unsigned(uintptr_t(Geometry::sceneCamera)));
    return result;
}
int StartModule(SceUID id, SceSize size, void* args, int* status, SceKernelSMOption* start) {
    // Patch the loaded (relocated) game before any of its threads run.
    SceKernelModuleInfo info{}; info.size = sizeof(info);
    if (sceKernelQueryModuleInfo(id, &info) >= 0 && std::strcmp(info.name, "APP_APPLICATION_NAME") == 0) {
        Attach(info); Install();
    }
    return sceKernelStartModule(id, size, args, status, start);
}
}
extern "C" int module_start(SceSize, void*) {
    logger.SetPath("ms0:/PSP/PLUGINS/SOCOM.FireteamBravo2.PPSSPP.FusionFix/SOCOM.FireteamBravo2.PPSSPP.FusionFix.log");
    constexpr auto ini = "ms0:/PSP/PLUGINS/SOCOM.FireteamBravo2.PPSSPP.FusionFix/SOCOM.FireteamBravo2.PPSSPP.FusionFix.cfg";
    constexpr auto log = "ms0:/PSP/PLUGINS/SOCOM.FireteamBravo2.PPSSPP.FusionFix/SOCOM.FireteamBravo2.PPSSPP.FusionFix.log";
    if (Start("APP_APPLICATION_NAME", ini, log)) { ReadOptions(); return Install(); }
    if (!Start("SocomPSPLoader", ini, log) || !BeginWithNotice()) return 0;
    ReadOptions();
    if (!pattern.get_first("25 40 00 00 34 00 A2 AF", 0))
    {
        PatchRejected(PSP_HOOK_UNSUPPORTED_INSTRUCTION);
        return -1;
    }
    injector::MakeCALL(sites::ptr_88041F8(), StartModule);
    return Finish();
}
