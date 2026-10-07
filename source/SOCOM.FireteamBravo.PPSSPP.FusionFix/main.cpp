#include "../Shared/Console/SocomInput.hpp"
#include "Sites.hpp"
extern "C" { PSP_MODULE_INFO("FireteamBravo.FusionFix", PSP_MODULE_USER, 2, 0); }
namespace {
using console::socom::Input;
using namespace console::portable;
injector::hook_back<int(Input*, float)> update;
SafetyMipsMid dialogue, loading;
int* introState;
uintptr_t introDialogue;
bool dialogueSkipped;
void SkipIntro() { if (introState) *introState = 2; }
// The callbacks below use no VFPU and call no game code. Saving VFPU state
// would also restore the prefix registers before returning to the game, which
// makes PPSSPP's JIT drop its default-prefix assumption for the whole session.
safetymips::Options Scalar(bool executeOriginal = true) {
    safetymips::Options options; options.preserve = PSP_HOOK_SAVE_FPU; options.execute_original = executeOriginal;
    return options;
}
int Update(Input* input, float delta) {
    const int result = update.fun(input, delta);
    console::socom::KeepAwake(input);
    return result;
}
int Exit(int status) { Unthrottle(false); return sceKernelExitDeleteThread(status); }
int Terminate(SceUID thread) { Unthrottle(false); return sceKernelTerminateDeleteThread(thread); }
int Install() {
    // The first game initializes its input/intro objects asynchronously.
    sceKernelDelayThread(120000);
    if (!Begin()) return -1;
    const uintptr_t intro[] = {sites::ptr_883BC3C(), sites::ptr_883BB24(), sites::ptr_883BBD0(), sites::ptr_88577F8()};
    // A release without these message boxes keeps its intro; the controls still install.
    if (inireader.ReadInteger("MAIN", "SkipIntro", 1) && intro[0] && intro[1] && intro[2] && intro[3]) {
        introState = reinterpret_cast<int*>(Absolute(intro[0]));
        introDialogue = sites::ptr_890F6D0();
        injector::MakeCALL(intro[1], SkipIntro);
        injector::MakeCALL(intro[2], SkipIntro);
        dialogue = safetymips::create_mid(intro[3], [](SafetyMipsContext& regs) {
            const bool skip = !dialogueSkipped && introDialogue &&
                *reinterpret_cast<const uintptr_t*>(uintptr_t(regs.a0) + 206 * 4) == introDialogue;
            if (skip) dialogueSkipped = true;
            *reinterpret_cast<uint8_t*>(uintptr_t(regs.sp) + 0x2C) = skip;
            *reinterpret_cast<uint32_t*>(uintptr_t(regs.sp) + 0x38) = uint32_t(regs.s0);
        }, Scalar(false));
    }
    if (inireader.ReadInteger("MAIN", "DualAnalogPatch", 1)) {
        update.fun = injector::MakeCALL(sites::ptr_8809724(), Update).get();
        injector::MakeCALL(sites::ptr_88885DC(), console::socom::Camera);
        injector::MakeCALL(sites::ptr_8888ADC(), console::socom::Camera);
        for (auto site : {sites::ptr_8888674(), sites::ptr_8888508(), sites::ptr_89DDF9C(), sites::ptr_89DDF6C()}) injector::MakeNOP(site);
        injector::WriteMemory<uint32_t>(sites::ptr_89DE278(), 0x1000000B); // Native free-look strafe branch.
    }
    if (inireader.ReadInteger("MAIN", "UnthrottleEmuDuringLoading", 1)) {
        loading = safetymips::create_mid(sites::ptr_8A9C224(), [](SafetyMipsContext&) { Unthrottle(true); }, Scalar());
        injector::MakeCALL(sites::ptr_8A9C368(), Exit);
        injector::MakeCALL(sites::ptr_8A9C17C(), Terminate);
    }
    return Finish();
}
}
extern "C" int module_start(SceSize, void*) {
    return console::portable::Start("APP_APPLICATION_NAME",
        "ms0:/PSP/PLUGINS/SOCOM.FireteamBravo.PPSSPP.FusionFix/SOCOM.FireteamBravo.PPSSPP.FusionFix.ini",
        "ms0:/PSP/PLUGINS/SOCOM.FireteamBravo.PPSSPP.FusionFix/SOCOM.FireteamBravo.PPSSPP.FusionFix.log") ? Install() : 0;
}
