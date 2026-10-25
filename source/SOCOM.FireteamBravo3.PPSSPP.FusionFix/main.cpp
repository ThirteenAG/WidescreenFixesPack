#include "../Shared/Console/PatchNotice.hpp"
#include "../Shared/Console/PSP.hpp"
#include "Sites.hpp"
#include "../Shared/Console/PortableCfg.hpp"
#include <cstdlib>
#include <cmath>
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
void Parameters(int destination, float* values)
{
    // Only the default camera table is adjusted; aiming and binoculars use other tables.
    if (values == cameraParameters && values && fovFactor != 1.0f &&
        std::isfinite(values[1]) && values[1] > 0.0f && values[1] < 90.0f)
    {
        float adjusted[33];
        std::memcpy(adjusted, values, sizeof(adjusted));
        adjusted[1] = console::bounded(values[1] * fovFactor, 0.01f, 89.0f, values[1]);
        setParameters.fun(destination, adjusted);
    }
    else
    {
        setParameters.fun(destination, values);
    }
}
int SetEvent(SceUID event, u32 bits) { Unthrottle(true); return sceKernelSetEventFlag(event, bits); }
int ClearEvent(SceUID event, u32 bits) { Unthrottle(false); return sceKernelClearEventFlag(event, bits); }
// Read on the plugin's own thread: Install also runs inside the loader's
// sceKernelStartModule call, where newlib's number parsing (strtol/strtod) was
// found to intermittently crash PPSSPP's JIT (SOCOM FTB2 EU loader).
struct Options { float fov; bool skipIntro, dualAnalog, smoothing, unthrottle; } options;
void ReadOptions() {
    IniValue defaults[] = {{"MAIN", "SkipIntro", "1"}, {"MAIN", "DualAnalogPatch", "1"},
        {"MAIN", "DisableCameraSmoothing", "1"}, {"MAIN", "UnthrottleEmuDuringLoading", "1"},
        {"MAIN", "FOVFactor", "1"}};
    constexpr auto cfg = "ms0:/PSP/PLUGINS/SOCOM.FireteamBravo3.PPSSPP.FusionFix/SOCOM.FireteamBravo3.PPSSPP.FusionFix.cfg";
    (void)CompleteCfg(cfg, defaults);
    char buffer[64];
    const char* text = inireader.ReadString("MAIN", "FOVFactor", "1", buffer, sizeof(buffer));
    char* end = nullptr;
    options.fov = std::strtof(text, &end);
    const bool separated = *end == ' ' || *end == '\t';
    while (*end == ' ' || *end == '\t' || *end == '\r') ++end;
    if (end == text || !std::isfinite(options.fov) || (*end && !(separated && *end == ';'))) options.fov = 1.0f;
    options.skipIntro = inireader.ReadBoolean("MAIN", "SkipIntro", true);
    options.dualAnalog = inireader.ReadBoolean("MAIN", "DualAnalogPatch", true);
    options.smoothing = inireader.ReadBoolean("MAIN", "DisableCameraSmoothing", true);
    options.unthrottle = inireader.ReadBoolean("MAIN", "UnthrottleEmuDuringLoading", true);
}
int Install() {
    if (installed || !BeginWithNotice()) return installed ? 0 : -1;
    fovFactor = console::bounded(options.fov, 0.5f, 2.0f, 1.0f);
    if (options.skipIntro) {
        const uintptr_t intro[] = {sites::ptr_88950F4(), sites::ptr_8895108(), sites::ptr_889511C(), sites::ptr_889513C()};
        if (FeatureSites({ intro[0], intro[1], intro[2], intro[3] }, "SOCOM 3: intro skip unavailable.\n"))
            for (auto site : intro) injector::MakeNOP(site);
    }
    if (options.dualAnalog) {
        const uintptr_t controls[] = {sites::ptr_8A7A6F8(), sites::ptr_8A7A834(), sites::ptr_89D9978(),
            sites::ptr_89DBB7C(), sites::ptr_89DAA64(), sites::ptr_89DB5BC(), sites::ptr_89DB5D4(),
            sites::ptr_89DBD4C(), sites::ptr_89DAA84(), sites::ptr_89DAA90(), sites::ptr_88877D4(),
            sites::ptr_8887758()};
        bool available = true;
        for (unsigned i = 0; i < 12; ++i)
            if (i != 11 || options.smoothing) available = available && controls[i] != 0;
        if (available) {
            // These call sites share one saved original per callback type.
            const auto target = [](uintptr_t call) { return injector::GetBranchDestination(call).as_int(); };
            available = target(controls[7]) && target(controls[8]) && target(controls[2]) &&
                target(controls[2]) == target(controls[3]) && target(controls[2]) == target(controls[4]) &&
                target(controls[5]) && target(controls[5]) == target(controls[6]);
        }
        if (available) {
            injector::MakeCALL(controls[0], Sample);
            injector::WriteMemory<uint32_t>(controls[1], 0x10000005);
            process.fun = injector::MakeCALL(controls[2], ProcessMovement).get();
            process.fun = injector::MakeCALL(controls[3], ProcessCamera).get();
            process.fun = injector::MakeCALL(controls[4], ProcessCamera).get();
            movement.fun = injector::MakeCALL(controls[5], Movement).get();
            movement.fun = injector::MakeCALL(controls[6], Movement).get();
            const auto curveCall = controls[7];
            const auto nativeCurve = injector::GetBranchDestination(curveCall).as_int();
            curve = safetymips::create_inline(nativeCurve, Curve);
            injector::MakeCALL(curveCall, CameraCurve);
            const auto sensitivityCall = controls[8];
            sensitivity.fun = injector::GetBranchDestination(sensitivityCall).get();
            preferences = Absolute(controls[9]);
            // Keep binocular sensitivity native; substitute the actual camera axis
            // after its original NEG.S, without touching the other stick's movement.
            static SafetyMipsMid binocular;
            // Neither callback nor the native sensitivity getter (a leaf without VFPU
            // code) touches the VFPU. Saving VFPU state would restore the prefix
            // registers before returning, making PPSSPP's JIT drop its default-prefix
            // assumption for the whole session.
            safetymips::Options scalar; scalar.preserve = PSP_HOOK_SAVE_FPU;
            binocular = safetymips::create_mid(sensitivityCall + 8, [](SafetyMipsContext& regs) { regs.f22 = -rightStick.y; }, scalar);
            const auto cameraSite = controls[10];
            camera = safetymips::create_mid(cameraSite + 48, [](SafetyMipsContext& regs) {
                const auto object = uintptr_t(regs.s0);
                if (!object) return;
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
                smooth.fun = injector::MakeCALL(controls[11], Smooth).get();
        }
        else PatchNotice("SOCOM 3: dual analog controls unavailable.\n");
    }
    if (fovFactor != 1.0f)
    {
        const uintptr_t parameters = sites::ptr_888773C();
        const uintptr_t parameterCall = sites::ptr_888774C();
        if (parameters && parameterCall && injector::GetBranchDestination(parameterCall).as_int())
        {
            cameraParameters = reinterpret_cast<float*>(Absolute(parameters));
            if (cameraParameters)
                setParameters.fun = injector::MakeCALL(parameterCall, Parameters).get();
        }
    }
    if (options.unthrottle) {
        const uintptr_t load[] = {sites::ptr_881AB4C(), sites::ptr_881AC38(), sites::ptr_881AD1C()};
        if (FeatureSites({ load[0], load[1], load[2] }, "SOCOM 3: loading speed control unavailable.\n")) {
            injector::MakeCALL(load[0], SetEvent);
            injector::MakeCALL(load[1], SetEvent);
            injector::MakeCALL(load[2], ClearEvent);
        }
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
    logger.SetPath("ms0:/PSP/PLUGINS/SOCOM.FireteamBravo3.PPSSPP.FusionFix/SOCOM.FireteamBravo3.PPSSPP.FusionFix.log");
    constexpr auto cfg = "ms0:/PSP/PLUGINS/SOCOM.FireteamBravo3.PPSSPP.FusionFix/SOCOM.FireteamBravo3.PPSSPP.FusionFix.cfg";
    constexpr auto log = "ms0:/PSP/PLUGINS/SOCOM.FireteamBravo3.PPSSPP.FusionFix/SOCOM.FireteamBravo3.PPSSPP.FusionFix.log";
    if (Start("FireTeamBravo3", cfg, log)) { ReadOptions(); return Install(); }
    if (!Start("SocomTacticsLoader", cfg, log) || !BeginWithNotice()) return 0;
    ReadOptions();
    const auto startModule = sites::ptr_880438C();
    if (FeatureSites({ startModule }, "SOCOM 3: game loader hook unavailable.\n")) injector::MakeCALL(startModule, StartModule);
    return Finish();
}
