#include "../Shared/Console/SocomInput.hpp"
#include "Sites.hpp"
extern "C" { PSP_MODULE_INFO("FireteamBravo2.FusionFix", PSP_MODULE_USER, 2, 0); }
// The European loader (ULES00557 BOOT.BIN) overwrites the first words of the
// module loaded directly after it. Keep that range free of code.
asm(".section .text\n.globl socom2_loader_guard\nsocom2_loader_guard:\n.space 0x200\n.previous");
namespace {
using console::socom::Input;
using namespace console::portable;
injector::hook_back<int(Input*, float)> update;
SafetyMipsMid loading;
int* introState;
bool installed;
void SkipIntro() { if (introState) *introState = 2; }
int Update(Input* input, float delta) {
    const int result = update.fun(input, delta);
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
struct Options { bool skipIntro, dualAnalog, unthrottle; } options;
void ReadOptions() {
    options = {inireader.ReadInteger("MAIN", "SkipIntro", 1) != 0, inireader.ReadInteger("MAIN", "DualAnalogPatch", 1) != 0,
               inireader.ReadInteger("MAIN", "UnthrottleEmuDuringLoading", 1) != 0};
}
int Install() {
    if (installed || !Begin()) return installed ? 0 : -1;
    const uintptr_t intro[] = {sites::ptr_886266C(), sites::ptr_88628E8(), sites::ptr_8862A08()};
    // A release without these message boxes keeps its intro; the controls still install.
    if (options.skipIntro && intro[0] && intro[1] && intro[2]) {
        introState = reinterpret_cast<int*>(Absolute(intro[0], 0, 8));
        injector::MakeCALL(intro[1], SkipIntro);
        injector::MakeCALL(intro[2], SkipIntro);
    }
    if (options.dualAnalog) {
        // hook_back checks that the two update sites retain the same original.
        update.fun = injector::MakeCALL(sites::ptr_8B57BDC(), Update).get();
        update.fun = injector::MakeCALL(sites::ptr_8815BCC(), Update).get();
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
    constexpr auto ini = "ms0:/PSP/PLUGINS/SOCOM.FireteamBravo2.PPSSPP.FusionFix/SOCOM.FireteamBravo2.PPSSPP.FusionFix.ini";
    constexpr auto log = "ms0:/PSP/PLUGINS/SOCOM.FireteamBravo2.PPSSPP.FusionFix/SOCOM.FireteamBravo2.PPSSPP.FusionFix.log";
    if (Start("APP_APPLICATION_NAME", ini, log)) { ReadOptions(); return Install(); }
    if (!Start("SocomPSPLoader", ini, log) || !Begin()) return 0;
    ReadOptions();
    injector::MakeCALL(sites::ptr_88041F8(), StartModule);
    return Finish();
}
