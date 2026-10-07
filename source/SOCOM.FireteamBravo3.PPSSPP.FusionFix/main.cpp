#include "../Shared/Console/PSP.hpp"
#include "Sites.hpp"
extern "C" { PSP_MODULE_INFO("FireteamBravo3.FusionFix", PSP_MODULE_USER, 2, 0); }
namespace {
using namespace console::portable;
console::Point rightStick, leftStick;
bool cameraRelated, installed;
uintptr_t preferences;
float fovFactor;
float* cameraParameters;
SafetyMipsInline curve;
SafetyMipsMid camera;
injector::hook_back<int(int, float*, float)> process;
injector::hook_back<void(int, float*)> movement;
injector::hook_back<void(float*, float)> smooth;
injector::hook_back<void(int, float*)> setParameters;
injector::hook_back<float(int)> sensitivity;
struct CameraScope {
    bool previous;
    explicit CameraScope(bool value) : previous(cameraRelated) { cameraRelated = value; }
    ~CameraScope() { cameraRelated = previous; }
};
float Axis(uint8_t raw) {
    const int value = int(raw) - 128;
    return float(value < -127 ? -127 : value) / 127.0f;
}
int Sample(SceCtrlData* samples, int count) {
    const int result = sceCtrlPeekBufferPositive(samples, count);
    if (result > 0 && samples) {
        const auto& pad = samples[0];
        rightStick = {Axis(pad.Rsrv[0]), Axis(pad.Rsrv[1])};
        leftStick = {Axis(pad.Lx), Axis(pad.Ly)};
    } else { rightStick = {}; leftStick = {}; }
    return result;
}
float Response(float value, const float* table) {
    const float magnitude = console::bounded(std::fabs(value), 0, 1, 0) * 20;
    unsigned index = unsigned(magnitude);
    if (index > 19) index = 19;
    const unsigned next = index < 19 ? index + 1 : 19;
    const float fraction = console::bounded(magnitude - float(index), 0, 1, 0);
    const float result = table[index] + (table[next] - table[index]) * fraction;
    return value < 0 ? -result : result;
}
void Curve(int object, float* vertical, float* horizontal, const float* horizontalTable, const float* verticalTable) {
    console::Point input = rightStick;
    if (!cameraRelated) {
        const auto device = *reinterpret_cast<const uintptr_t*>(*reinterpret_cast<const uintptr_t*>(object) + 104);
        input = {*reinterpret_cast<const float*>(device + 280), *reinterpret_cast<const float*>(device + 284)};
    }
    *vertical = Response(-input.y, verticalTable);
    *horizontal = Response(input.x, horizontalTable);
}
int ProcessMovement(int object, float* output, float delta) {
    CameraScope scope(false); return process.fun(object, output, delta);
}
int ProcessCamera(int object, float* output, float delta) {
    CameraScope scope(true); return process.fun(object, output, delta);
}
void Movement(int object, float* output) {
    movement.fun(object, output);
    if (!cameraRelated) { output[2] = leftStick.x; output[3] = -rightStick.x; }
}
void CameraCurve(int object, float* vertical, float* horizontal, const float* horizontalTable, const float* verticalTable) {
    CameraScope scope(true); Curve(object, vertical, horizontal, horizontalTable, verticalTable);
}
void Smooth(float* values, float delta) {
    smooth.fun(values, delta);
    values[200] = 1.0f; // Default camera's native smoothing coefficient, index 2.
}
void Parameters(int destination, float* values) {
    if (fovFactor && cameraParameters) cameraParameters[1] = 22.0f * fovFactor;
    setParameters.fun(destination, values);
}
int SetEvent(SceUID event, u32 bits) { Unthrottle(true); return sceKernelSetEventFlag(event, bits); }
int ClearEvent(SceUID event, u32 bits) { Unthrottle(false); return sceKernelClearEventFlag(event, bits); }
// Read on the plugin's own thread: Install also runs inside the loader's
// sceKernelStartModule call, where newlib's number parsing (strtol/strtod) was
// found to intermittently crash PPSSPP's JIT (SOCOM FTB2 EU loader).
struct Options { float fov; bool skipIntro, dualAnalog, smoothing, unthrottle; } options;
void ReadOptions() {
    options.fov = inireader.ReadFloat("MAIN", "FOVFactor", 0);
    options.skipIntro = inireader.ReadInteger("MAIN", "SkipIntro", 1) != 0;
    options.dualAnalog = inireader.ReadInteger("MAIN", "DualAnalogPatch", 1) != 0;
    options.smoothing = inireader.ReadInteger("MAIN", "DisableCameraSmoothing", 1) != 0;
    options.unthrottle = inireader.ReadInteger("MAIN", "UnthrottleEmuDuringLoading", 1) != 0;
}
int Install() {
    if (installed || !Begin()) return installed ? 0 : -1;
    fovFactor = console::bounded(options.fov, 0, 2.5f, 0);
    if (options.skipIntro)
        for (auto site : {sites::ptr_88950F4(), sites::ptr_8895108(), sites::ptr_889511C(), sites::ptr_889513C()}) injector::MakeNOP(site);
    if (options.dualAnalog) {
        injector::MakeCALL(sites::ptr_8A7A6F8(), Sample);
        injector::WriteMemory<uint32_t>(sites::ptr_8A7A834(), 0x10000005);
        process.fun = injector::MakeCALL(sites::ptr_89D9978(), ProcessMovement).get();
        process.fun = injector::MakeCALL(sites::ptr_89DBB7C(), ProcessCamera).get();
        process.fun = injector::MakeCALL(sites::ptr_89DAA64(), ProcessCamera).get();
        movement.fun = injector::MakeCALL(sites::ptr_89DB5BC(), Movement).get();
        movement.fun = injector::MakeCALL(sites::ptr_89DB5D4(), Movement).get();
        const auto curveCall = sites::ptr_89DBD4C();
        const auto nativeCurve = injector::GetBranchDestination(curveCall).as_int();
        curve = safetymips::create_inline(nativeCurve, Curve);
        injector::MakeCALL(curveCall, CameraCurve);
        const auto sensitivityCall = sites::ptr_89DAA84();
        sensitivity.fun = injector::GetBranchDestination(sensitivityCall).get();
        preferences = Absolute(sites::ptr_89DAA90());
        // Keep binocular sensitivity native; substitute the actual camera axis
        // after its original NEG.S, without touching the other stick's movement.
        static SafetyMipsMid binocular;
        // Neither callback nor the native sensitivity getter (a leaf without VFPU
        // code) touches the VFPU. Saving VFPU state would restore the prefix
        // registers before returning, making PPSSPP's JIT drop its default-prefix
        // assumption for the whole session.
        safetymips::Options scalar; scalar.preserve = PSP_HOOK_SAVE_FPU;
        binocular = safetymips::create_mid(sensitivityCall + 8, [](SafetyMipsContext& regs) { regs.f22 = -rightStick.y; }, scalar);
        const auto cameraSite = sites::ptr_88877D4();
        camera = safetymips::create_mid(cameraSite + 48, [](SafetyMipsContext& regs) {
            const auto object = uintptr_t(regs.s0);
            if (*reinterpret_cast<const int*>(object + 1212) != 0 || *reinterpret_cast<const uint8_t*>(object + 1284)) return;
            const float gain = sensitivity.fun(int(object)) * console::bounded(regs.f20, 0, 0.2f, 1.0f / 30.0f) * 3.0f;
            float vertical = rightStick.y;
            const auto options = preferences ? *reinterpret_cast<const uintptr_t*>(preferences) : 0;
            if (options && *reinterpret_cast<const uint8_t*>(options + 49)) vertical = -vertical;
            regs.f26 = *reinterpret_cast<const float*>(object + 212) - rightStick.x * gain;
            const float pitch = console::bounded(*reinterpret_cast<const float*>(object + 208) - vertical * gain, -1.3f, 0.921875f, 0);
            *reinterpret_cast<float*>(uintptr_t(regs.sp) + 4) = pitch;
        }, scalar);
        if (options.smoothing)
            smooth.fun = injector::MakeCALL(sites::ptr_8887758(), Smooth).get();
        cameraParameters = reinterpret_cast<float*>(Absolute(sites::ptr_888773C()));
        setParameters.fun = injector::MakeCALL(sites::ptr_888774C(), Parameters).get();
    }
    if (options.unthrottle) {
        injector::MakeCALL(sites::ptr_881AB4C(), SetEvent);
        injector::MakeCALL(sites::ptr_881AC38(), SetEvent);
        injector::MakeCALL(sites::ptr_881AD1C(), ClearEvent);
    }
    const int result = Finish(); installed = result == 0;
    return result;
}
int StartModule(SceUID id, SceSize size, void* args, int* status, SceKernelSMOption* start) {
    // Patch the loaded (relocated) game before any of its threads run.
    SceKernelModuleInfo info{}; info.size = sizeof(info);
    if (sceKernelQueryModuleInfo(id, &info) >= 0 && std::strcmp(info.name, "FireTeamBravo3") == 0) {
        Attach(info); Install();
    }
    return sceKernelStartModule(id, size, args, status, start);
}
}
extern "C" int module_start(SceSize, void*) {
    constexpr auto ini = "ms0:/PSP/PLUGINS/SOCOM.FireteamBravo3.PPSSPP.FusionFix/SOCOM.FireteamBravo3.PPSSPP.FusionFix.ini";
    constexpr auto log = "ms0:/PSP/PLUGINS/SOCOM.FireteamBravo3.PPSSPP.FusionFix/SOCOM.FireteamBravo3.PPSSPP.FusionFix.log";
    if (Start("FireTeamBravo3", ini, log)) { ReadOptions(); return Install(); }
    if (!Start("SocomTacticsLoader", ini, log) || !Begin()) return 0;
    ReadOptions();
    injector::MakeCALL(sites::ptr_880438C(), StartModule);
    return Finish();
}
