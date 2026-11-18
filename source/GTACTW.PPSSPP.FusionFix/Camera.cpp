#include "Game.hpp"
#include "Shared/Console/PortableIni.hpp"
extern "C" {
#include "../../external/injector/include/psp/nanoprintf.h"
}
namespace ctw {
namespace {
injector::hook_back<int(int16_t*, int16_t)> pitch;
SafetyMipsMid height;
bool enabled, dirty;
int angle, elevation = 13464;
uint32_t previous;
// The camera toggle and view are kept in the cfg ([CAMERA]). A failed write
// (read-only or full memory stick) is retried with backoff: 1, 2, 4 ... 32 s
// at 60 calls per second, then not again this session.
unsigned retryDelay, failures;
constexpr unsigned maximumFailures = 6;
void SaveFailed() {
    if (failures < maximumFailures) ++failures;
    retryDelay = 60u << (failures - 1);
}
void Save() {
    if (failures >= maximumFailures) return;
    if (retryDelay) { --retryDelay; return; }
    IniValue values[] = {{"CAMERA", "Active", {}}, {"CAMERA", "Angle", {}}, {"CAMERA", "Elevation", {}}};
    npf_snprintf(values[0].value, sizeof(values[0].value), "%d", int(enabled));
    npf_snprintf(values[1].value, sizeof(values[1].value), "%d", angle);
    npf_snprintf(values[2].value, sizeof(values[2].value), "%d", elevation);
    static IniWriter<> writer;
    if (writer.update(cfgPath, values, 3)) { dirty = false; failures = 0; }
    else SaveFailed();
}
void Load() {
    const int active = inireader.ReadInteger("CAMERA", "Active", 0);
    const int savedAngle = inireader.ReadInteger("CAMERA", "Angle", 0);
    const int savedElevation = inireader.ReadInteger("CAMERA", "Elevation", 13464);
    if (active < 0 || active > 1 || savedAngle < -10000 || savedAngle > 0 || savedElevation < 13464 || savedElevation > 120000) return;
    enabled = active != 0; angle = savedAngle; elevation = savedElevation;
}
int Axis(uint8_t value) {
    return value < 118 ? int(value) - 118 : value > 138 ? int(value) - 138 : 0;
}
int Pitch(int16_t* matrix, int16_t native) {
    SceCtrlData pad{};
    if (GameplayCamera()) {
        sceCtrlSetSamplingCycle(0);
        sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
        if (sceCtrlPeekBufferPositive(&pad, 1) <= 0) {
            previous = 0;
            return pitch.fun(matrix, native);
        }
        const uint32_t pressed = pad.Buttons & ~previous;
        if (pressed & PSP_CTRL_RIGHT) { enabled = !enabled; dirty = true; }
        if (enabled) {
            if (pressed & PSP_CTRL_UP) { angle = 0; elevation = 13464; dirty = true; }
            const int x = Axis(pad.Rsrv[0]), y = Axis(pad.Rsrv[1]);
            if (x || y) {
                angle = angle - x * 8; elevation = elevation - y * 8;
                if (angle < -10000) angle = -10000;
                if (angle > 0) angle = 0;
                if (elevation < 13464) elevation = 13464;
                if (elevation > 120000) elevation = 120000;
                dirty = true;
            } else if (dirty) Save(); // persist once when the stick settles
            native = int16_t(-16384 - angle);
        } else if (dirty) Save();
        previous = pad.Buttons;
    } else previous = 0;
    return pitch.fun(matrix, native);
}
}
void InstallCamera() {
    if (!Option("Enable3rdPersonCamera", 1)) return;
    Load();
    injector::MakeNOP(Address<0x088704C8>()); // retain the original cinematic-camera opt-out
    pitch.fun = injector::MakeCALL(Address<0x088559F0>(), Pitch).get();
    safetymips::Options replace; replace.execute_original = false;
    height = safetymips::create_mid(Address<0x088CBFA8>(), [](SafetyMipsContext& regs) {
        regs.a0 = at<uint32_t>(regs.sp + 0xB4);
        at<uint32_t>(regs.s6 + 0x104) = regs.a1; // displaced native Y store
        if (enabled && GameplayCamera()) regs.a0 = uint32_t(elevation);
    }, replace);
}
}
