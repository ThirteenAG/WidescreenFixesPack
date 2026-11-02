#include "../Shared/Console/PatchNotice.hpp"
#include "../Shared/Console/PSP.hpp"
#include "Sites.hpp"
#include "../Shared/Console/PortableCfg.hpp"
#include "../Shared/Console/Text.hpp"
#include "../Shared/Console/PadButtonArtwork.hpp"
#include <cstdlib>
#include <cmath>
#include <algorithm>
extern "C" {
#include <psputility_sysparam.h>
}
extern "C" { PSP_MODULE_INFO("FireteamBravo3.FusionFix", PSP_MODULE_USER, 2, 0); }
namespace {
using namespace console::portable;
#include "Stick.hpp"
namespace NativeMenu { void Input(SceCtrlData* samples, int count); bool Capturing(); }
bool RawCameraEnabled();
console::Point rightStick, leftStick;
bool CameraActive()
{
    uint32_t bits[2];
    std::memcpy(bits, &rightStick, sizeof(bits));
    return ((bits[0] | bits[1]) & 0x7FFFFFFF) != 0;
}
bool cameraRelated, installed;
uintptr_t preferences;
float fovFactor;
float* cameraParameters;
SafetyMipsInline curve;
SafetyMipsMid camera;
SafetyMipsMid projection;
float forcedAspect;
injector::hook_back<int(int, float*, float)> process[3];
injector::hook_back<void(int, float*)> movement[2];
injector::hook_back<void(float*, float)> smooth;
injector::hook_back<void(int, float*)> setParameters;
injector::hook_back<float(int)> sensitivity;
injector::hook_back<int(const float*)> movie;
int Movie(const float* matrix)
{
    constexpr float nativeAspect = 480.0f / 272.0f;
    const float aspect = forcedAspect > 0.0f ? forcedAspect : Aspect();
    const float scaleX = std::min(nativeAspect / aspect, 1.0f);
    const float scaleY = std::min(aspect / nativeAspect, 1.0f);
    if (scaleX == 1.0f && scaleY == 1.0f)
        return movie.fun(matrix);
    // The player clears to black before submitting this orthographic matrix.
    // Fit its complete canvas, including subtitle glyphs and their shadows.
    alignas(16) float adjusted[16];
    std::memcpy(adjusted, matrix, sizeof(adjusted));
    for (unsigned i = 0; i < 16; i += 4)
    {
        adjusted[i] *= scaleX;
        adjusted[i + 1] *= scaleY;
    }
    const int result = movie.fun(adjusted);
    static bool reported = false;
    if (!reported)
    {
        reported = true;
        logger.WriteF("Movie canvas fitted to aspect %.6f.\n", aspect);
    }
    return result;
}
struct CameraScope {
    bool previous;
    explicit CameraScope(bool value) : previous(cameraRelated) { cameraRelated = value; }
    ~CameraScope() { cameraRelated = previous; }
};
float Axis(uint8_t raw) {
    const int value = int(raw) - 128;
    return float(value < -127 ? -127 : value) / 127.0f;
}
console::Point RightInput(const SceCtrlData& pad)
{
    const console::Point mapped{ Axis(pad.Rsrv[0]), Axis(pad.Rsrv[1]) };
    if (!RawCameraEnabled() || mapped.x != 0 || mapped.y != 0)
        return mapped;
    float x{}, y{};
    // Stock PPSSPP publishes the host's XInput right-stick axes as Z and RZ.
    if (sceIoDevctl("kemulator:", 0x33, reinterpret_cast<void*>(11), 0, &x, sizeof(x)) < 0 ||
        sceIoDevctl("kemulator:", 0x33, reinterpret_cast<void*>(14), 0, &y, sizeof(y)) < 0)
        return {};
    return Stick::Radial(x, -y);
}
int Sample(SceCtrlData* samples, int count) {
    const int result = sceCtrlPeekBufferPositive(samples, count);
    if (result > 0 && samples) {
        NativeMenu::Input(samples, std::min(result, count));
        const auto& pad = samples[0];
        rightStick = NativeMenu::Capturing() ? console::Point{} : RightInput(pad);
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
template<unsigned Index> int Process(int object, float* output, float delta) {
    CameraScope scope(Index != 0);
    return process[Index].fun(object, output, delta);
}
template<unsigned Index> void Movement(int object, float* output) {
    movement[Index].fun(object, output);
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
#include "Hud.hpp"
#include "HudInstall.hpp"
#include "Cinematics.hpp"
#include "PadIcons.hpp"
#include "FrameRate.hpp"
// Read on the plugin's own thread: Install also runs inside the loader's
// sceKernelStartModule call, where newlib's number parsing (strtol/strtod) was
// found to intermittently crash PPSSPP's JIT (SOCOM FTB2 EU loader).
struct Options { float fov; bool skipIntro, dualAnalog, smoothing, unthrottle; } options;
bool RawCameraEnabled() { return options.dualAnalog; }
#include "Startup.hpp"
#include "NativeMenu.hpp"
void ReadOptions() {
    IniValue defaults[] = {{"MAIN", "SkipIntro", "1"}, {"MAIN", "DualAnalogPatch", "1"},
        {"MAIN", "DisableCameraSmoothing", "1"}, {"MAIN", "UnthrottleEmuDuringLoading", "1"},
        {"MAIN", "FOVFactor", "1"}, {"MAIN", "ForceAspectRatio", "Auto"},
        {"MAIN", "HudScale", "1"}, {"MAIN", "HudAspectRatioConstraint", "Auto"},
        {"MAIN", "CutsceneBorders", "3"}, {"MAIN", "TextLanguage", "auto"},
        {"MAIN", "GamepadIcons", "0"}, {"MAIN", "SkipPressStart", "1"}, {"MAIN", "FPSLimit", "60"}};
    constexpr auto cfg = "ms0:/PSP/PLUGINS/SOCOM.FireteamBravo3.PPSSPP.FusionFix/SOCOM.FireteamBravo3.PPSSPP.FusionFix.cfg";
    (void)CompleteCfg(cfg, defaults);
    char buffer[64];
    const char* text = inireader.ReadString("MAIN", "FOVFactor", "1", buffer, sizeof(buffer));
    char* end = nullptr;
    options.fov = std::strtof(text, &end);
    const bool separated = *end == ' ' || *end == '\t';
    while (*end == ' ' || *end == '\t' || *end == '\r') ++end;
    if (end == text || !std::isfinite(options.fov) || (*end && !(separated && *end == ';'))) options.fov = 1.0f;
    text = inireader.ReadString("MAIN", "ForceAspectRatio", "Auto", buffer, sizeof(buffer));
    forcedAspect = console::parse_forced_aspect(text);
    options.skipIntro = inireader.ReadBoolean("MAIN", "SkipIntro", true);
    options.dualAnalog = inireader.ReadBoolean("MAIN", "DualAnalogPatch", true);
    options.smoothing = inireader.ReadBoolean("MAIN", "DisableCameraSmoothing", true);
    options.unthrottle = inireader.ReadBoolean("MAIN", "UnthrottleEmuDuringLoading", true);
    Hud::ReadOptions();
    Cinematics::ReadOptions();
    PadIcons::style = std::clamp(inireader.ReadInteger("MAIN", "GamepadIcons", 0), 0, 3);
    Startup::skipPressStart = inireader.ReadInteger("MAIN", "SkipPressStart", 1) != 0;
    FrameRate::ReadOptions();
    NativeMenu::ReadOptions();
}
int Install() {
    if (installed || !BeginWithNotice()) return installed ? 0 : -1;
    // Check the loaded game and HUD profiles before installing any hooks.
    constexpr const char* sitesToCheck[] = {
        "? ? 04 3C 00 60 80 44 ? ? 84 24 FF FF 05 24 CC 00 04 AE F0 00 05 AE F4 00 05 AE F8 00 0C E6",
        "2C 02 B4 C7 25 20 40 00 ? ? ? ? 06 A3 00 46 ? ? ? ? 00 00 00 00 ? ? ? ? 00 00 00 00 ? ? ? ? 24 02 AC C7",
        "B0 FF BD 27 ? ? ? ? 00 60 80 44 25 20 A0 03 40 00 BF AF ? ? ? ? 00 68 85 44 ? ? ? ?",
        "C4 00 6E C6 06 A3 00 46 C8 00 6F C6 46 B3 00 46 ? ? ? ? 25 20 A0 02 ? ? ? ? 25 20 20 02 ? ? ? ? 06 F3 00 46",
        FrameRate::pacingSignature,
        "70 FD BD 27 6C 02 B1 AF 25 88 80 00 08 00 24 8E 1C 00 84 8C 50 02 B4 E7 78 02 B4 AF 88 02 BE AF 06 65 00 46 FF 00 B4 30",
        "14 00 0C C6 89 88 A5 34 00 68 85 44 24 00 04 AE 00 63 0D 46 18 00 0E C6 3E 60 0E 46",
        "41 6B 14 46 10 00 0E C6 9A 99 84 34 00 78 84 44 82 73 0F 46 01 65 14 46 10 00 0E E6",
        "08 3D 08 3C 20 00 06 26 89 88 08 35 D8 00 04 8E 00 60 88 44 25 28 00 02 ? ? ? ? 25 38 C0 00 DC 00 04 8E 0B 00 80 10",
        "08 3D 08 3C 20 00 06 26 89 88 08 35 DC 00 04 8E 00 60 88 44 25 28 00 02 ? ? ? ? 25 38 C0 00",
        "30 FF BD 27 B4 00 B0 AF B8 00 B1 AF BC 00 B2 AF C0 00 BF AF 00 3E 05 3C",
        "? ? 05 3C ? ? A5 24 00 00 80 D8 10 00 81 D8 20 00 82 D8 30 00 83 D8 ? ? A0 F8 ? ? A1 F8 ? ? A2 F8 ? ? A3 F8 80 3F 05 3C",
        "80 FF BD 27 00 00 C4 D8 10 00 C5 D8 20 00 C6 D8 30 00 C7 D8 00 00 E8 D8",
        "F0 FF BD 27 80 3F 09 3C 00 00 BF AF ? ? ? ? 00 60 89 44 00 00 BF 8F",
        "C0 FF BD 27 0C 00 8B 8C 10 00 B0 AF 14 00 B1 AF 18 00 B2 AF 1C 00 B3 AF",
        "C0 FF BD 27 00 00 AA AF 1C 00 B4 AF 28 00 B7 AF 25 A0 80 00 40 00 B7 8F",
        "0C 00 86 8C 25 28 00 02 20 00 C6 24 00 00 C7 84 04 00 C6 8C 09 F8 C0 00",
        "80 42 05 00 00 D4 09 3C 25 40 09 01 ? ? 09 3C ? ? 2A 8D 21 28 E5 00",
        "00 60 80 44 D6 01 12 34 00 00 AC E7 3C 00 04 8E 00 04 84 30",
        "F0 FF BD 27 04 00 B0 AF 25 80 80 00 00 00 B4 E7 08 00 BF AF ? ? ? ? 06 65 00 46 40 00 04 8E 2A 20 04 00 FF 00 84 30 17 00 80 14 25 28 40 00",
        "F0 FF BD 27 00 00 B0 AF 25 80 80 00 04 00 B1 AF 08 00 BF AF ? ? ? ? FF 00 B1 30 25 20 40 00",
        "40 FF BD 27 90 00 B0 AF ? ? 04 3C 10 00 B0 27 94 00 B1 AF 98 00 B2 AF 9C 00 B3 AF A0 00 B4 AF",
        "40 FF BD 27 A4 00 B0 AF ? ? 04 3C 10 00 B0 27 A8 00 B1 AF AC 00 B2 AF B0 00 B3 AF B4 00 B4 AF",
        "50 FF BD 27 90 00 B3 AF 25 98 80 00 98 00 B5 AF 25 A8 A0 00 25 20 C0 00 84 00 B0 AF 88 00 B1 AF 8C 00 B2 AF 94 00 B4 AF",
        "E0 FF BD 27 08 00 B2 AF 25 90 80 00 20 00 44 8E 14 00 B5 AF FF 00 B5 30 48 00 84 24 10 00 B4 AF FF 00 D4 30 00 00 85 84",
        "C0 FF BD 27 10 00 B0 AF 14 00 B1 AF 18 00 B2 AF 1C 00 B3 AF 20 00 B4 AF 24 00 B5 AF 28 00 B6 AF 25 80 60 01 25 88 40 01",
        "08 00 A4 27 25 28 40 00 ? ? ? ? 25 30 00 02 ? ? ? ? 08 00 A4 8F 08 00 A4 8F 0D 00 80 10 25 80 40 00",
        "D0 FF BD 27 04 00 B6 E7 08 00 B8 E7 0C 00 BA E7 10 00 BC E7 14 00 BE E7 18 00 B0 AF 86 67 00 46 25 80 A0 00 06 6F 00 46",
        "03 00 80 14 06 8D 00 46 31 00 00 10 00 00 00 00 ? ? ? ? 00 00 00 00 ? ? 04 3C ? ? 85 8C 00 12 06 3C 04 00 A7 24 9F 01 C6 24 ? ? 87 AC",
        "00 00 00 00 01 00 08 34 25 38 E8 00 2B 38 07 00 00 00 C7 A0 1C 01 AC C4 49 00 06 26 00 00 C7 90",
        "00 00 00 00 01 00 08 34 25 38 E8 00 2B 38 07 00 00 00 C7 A0 1C 01 AC C4 4A 00 06 26 00 00 C7 90",
        "01 00 08 34 24 28 E8 00 00 00 C5 A0 02 00 05 34 10 00 85 14 00 00 00 00 38 01 B6 E7 30 03 2C C6",
        "E1 3F 05 3C C0 00 0C C6 E2 E1 A5 34 C4 00 0E C6 00 68 85 44",
        "70 3F 08 3C 06 B3 00 46 00 70 88 44 25 20 00 00 08 3F 08 3C",
        "18 00 04 8E ? ? ? ? 01 00 06 34 ? ? ? ? ? ? ? ? ? ? ? ? 18 00 04 8E",
        "18 00 04 8E 01 00 06 34 ? ? ? ? ? ? ? ? ? ? ? ? 00 00 00 00",
        "00 00 00 00 ? ? ? ? 18 00 04 8E ? ? ? ? 01 00 06 34 ? ? ? ? ? ? ? ? 18 00 04 8E ? ? ? ? 01 00 06 34 ? ? ? ? ? ? ? ? 80 3F 04 3C",
        "18 00 04 8E ? ? ? ? 01 00 06 34 ? ? ? ? ? ? ? ? 80 3F 04 3C",
        "01 00 05 34 04 00 A5 8F 10 00 A4 30",
        "08 00 A4 93 00 00 4C E6",
        "25 28 A0 03 25 20 00 02 06 A3 00 46",
        "25 28 A0 03 04 00 05 8E",
        "25 28 A0 03 ? ? ? ? 00 00 00 00 04 00 04 8E",
        "25 28 00 02 03 00 15 34",
        "25 28 00 02 00 00 15 34",
        "25 40 40 00 ? ? ? ? 25 20 40 02",
        "00 00 BA E7 25 20 00 02",
        "00 00 BA C7 01 00 04 34",
        "06 A3 00 46 38 04 11 26",
        "20 01 16 26",
        "25 20 C0 02 25 20 C0 02 ? ? ? ? 06 A3 00 46",
        "87 65 00 46",
        "31 00 84 90 ? ? ? ? 02 A5 00 46",
        "01 00 05 34 ? ? ? ? 25 20 40 00 ? ? ? ? ? ? ? ? ? ? ? ? 25 28 00 00",
        "01 00 05 34 ? ? ? ? 25 20 40 00 00 00 BF 8F",
        "25 28 00 00 ? ? ? ? 25 20 40 00 ? ? ? ? ? ? ? ? ? ? ? ? 01 00 14 34",
        "F0 FF BD 27 00 00 B0 AF 04 00 B1 AF 25 80 80 00 ? ? 11 3C ? ? 24 8E 25 28 00 00 08 00 BF AF ? ? ? ? 25 30 00 00 0F 00 40 10 00 00 00 00",
        "? ? A5 24 ? ? 05 3C 25 20 00 02 ? ? ? ? ? ? A5 24 ? ? 24 8E 01 00 05 34",
    };
    for (const auto* site : sitesToCheck)
    {
        if (!pattern.get_first(site, 0))
        {
            PatchRejected(PSP_HOOK_UNSUPPORTED_INSTRUCTION);
            return -1;
        }
    }
    for (const auto& profile : Hud::profiles)
    {
        if (!pattern.get_first(profile.pattern, 0))
        {
            PatchRejected(PSP_HOOK_UNSUPPORTED_INSTRUCTION);
            return -1;
        }
    }
    fovFactor = std::clamp(options.fov, 0.5f, 2.0f);
    safetymips::Options projectionOptions;
    projectionOptions.preserve = PSP_HOOK_SAVE_FPU;
    projectionOptions.instructions = 3;
    // The camera's vertical half-angle and clipping planes stay native. Its
    // projection matrix is also consumed by world-to-screen calculations.
    projection = safetymips::create_mid(sites::Projection(), [](SafetyMipsContext& regs)
    {
        Cinematics::Camera(regs);
    }, projectionOptions);
    movie.fun = injector::MakeCALL(sites::Movie(), Movie).get();
    Hud::Install();
    Cinematics::Install();
    NativeMenu::Install();
    PadIcons::Install();
    Startup::Install();
    FrameRate::Install();
    injector::MakeCALL(sites::ptr_8A7A6F8(), Sample);
    if (options.skipIntro)
        for (auto site : {sites::ptr_88950F4(), sites::ptr_8895108(), sites::ptr_889511C(), sites::ptr_889513C()}) injector::MakeNOP(site);
    if (options.dualAnalog) {
        static SafetyMipsMid activityFirst, activitySecond, activityIdle;
        safetymips::Options activityOptions;
        activityOptions.preserve = 0;
        activityFirst = safetymips::create_mid(sites::CameraActivityFirst(), [](SafetyMipsContext& regs)
        {
            regs.t0 = CameraActive();
        }, activityOptions);
        activitySecond = safetymips::create_mid(sites::CameraActivitySecond(), [](SafetyMipsContext& regs)
        {
            regs.t0 = CameraActive();
        }, activityOptions);
        activityIdle = safetymips::create_mid(sites::CameraActivityIdle(), [](SafetyMipsContext& regs)
        {
            regs.t0 = !CameraActive();
        }, activityOptions);
        injector::WriteMemory<uint32_t>(sites::ptr_8A7A834(), 0x10000005);
        process[0].fun = injector::MakeCALL(sites::ptr_89D9978(), Process<0>).get();
        process[1].fun = injector::MakeCALL(sites::ptr_89DBB7C(), Process<1>).get();
        process[2].fun = injector::MakeCALL(sites::ptr_89DAA64(), Process<2>).get();
        movement[0].fun = injector::MakeCALL(sites::ptr_89DB5BC(), Movement<0>).get();
        movement[1].fun = injector::MakeCALL(sites::ptr_89DB5D4(), Movement<1>).get();
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
            smooth.fun = injector::MakeCALL(sites::ptr_8887758(), Smooth).get();
    }
    cameraParameters = reinterpret_cast<float*>(Absolute(sites::ptr_888773C()));
    setParameters.fun = injector::MakeCALL(sites::ptr_888774C(), Parameters).get();
    if (options.unthrottle) {
        injector::MakeCALL(sites::ptr_881AB4C(), SetEvent);
        injector::MakeCALL(sites::ptr_881AC38(), SetEvent);
        injector::MakeCALL(sites::ptr_881AD1C(), ClearEvent);
    }
    const int result = Finish(); installed = result == 0;
    if (installed)
        logger.WriteF("Camera projection installed; display aspect %.6f, FOV factor %.2f, camera type %08X.\n", forcedAspect > 0.0f ? forcedAspect : Aspect(), fovFactor, unsigned(Cinematics::multiplexor));
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
    if (!pattern.get_first("25 40 00 00 ? ? ? ? 28 00 A2 AF", 0))
    {
        PatchRejected(PSP_HOOK_UNSUPPORTED_INSTRUCTION);
        return -1;
    }
    injector::MakeCALL(sites::ptr_880438C(), StartModule);
    return Finish();
}
