#include "../Shared/Console/PatchNotice.hpp"
#include "../Shared/Console/PSP.hpp"
#include <cstdlib>
#include <cmath>
#include "../Shared/Console/PortableCfg.hpp"
extern "C" { PSP_MODULE_INFO("TheWarriors.FusionFix", PSP_MODULE_USER, 2, 0); }
namespace {
using namespace console::portable;
SafetyMipsInline input;
SafetyMipsMid projection, hud, loading;
injector::hook_back<int(int, char)> load;
float aspect = 512.0f / 320.0f;
bool automaticAspect;
float ReadAspect() {
    char buffer[64];
    const char* value = inireader.ReadString("MAIN", "ForceAspectRatio", "auto", buffer, sizeof(buffer));
    automaticAspect = std::strcmp(value, "auto") == 0;
    if (automaticAspect) return Aspect();
    if (!*value || std::strcmp(value, "0") == 0) return 0;
    char* end = nullptr;
    const float width = std::strtof(value, &end);
    if (end == value || *end != ':') return 512.0f / 320.0f;
    const char* heightText = end + 1;
    const float height = std::strtof(heightText, &end);
    if (end == heightText || *end || !std::isfinite(width) || !std::isfinite(height) || !(width > 0 && height > 0)) return 512.0f / 320.0f;
    return console::bounded(width / height, 0.5f, 8, 512.0f / 320.0f);
}
int Input(uintptr_t object, int controller) {
    const int result = input.call<int>(object, controller);
    if (automaticAspect) aspect = Aspect();
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
int Install() {
    if (!BeginWithNotice()) return -1;
    if (inireader.ReadBoolean("MAIN", "SkipIntro", true)) {
        const auto intro = pattern.get_first("10 00 A5 27 ? ? ? ? ? ? ? ? ? ? ? ? 21 28 00 00", 0);
        if (FeatureSites({ intro }, "The Warriors: intro skip unavailable.\n")) for (unsigned offset : {12u, 28u, 44u}) injector::MakeNOP(intro + offset);
    }
    if (inireader.ReadBoolean("MAIN", "DualAnalogPatch", true))
    {
        const auto branch = pattern.get_first("21 10 51 00 ? ? ? ? 1C 00 50 A4", 4);
        // Keep the left stick on movement, including while the native camera
        // modifier is held. Preserve the target and the button-history delay slot.
        if (FeatureSites({ branch }, "The Warriors: dual analog controls unavailable.\n") && branch >= pattern.text_addr + 0x310) {
            const auto instruction = injector::ReadMemory<uint32_t>(branch);
            const auto opcode = instruction >> 26;
            if (opcode == 4 || opcode == 5) {
                injector::WriteMemory<uint32_t>(branch, 0x10000000u | (instruction & 0xFFFF));
                input = safetymips::create_inline(branch - 0x310, Input);
            }
        }
    }
    if (inireader.ReadBoolean("MAIN", "Enable60FPS", false)) {
        const auto limiter = pattern.get_first("02 00 42 2C ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? 00 00 A2 8C", 4);
        if (FeatureSites({ limiter }, "The Warriors: frame unlock unavailable.\n")) injector::MakeNOP(limiter);
    }
    aspect = ReadAspect();
    if (aspect) {
        const auto site = pattern.get_first("94 18 C1 E7 03 03 01 46", 4);
        const auto hudSite = pattern.get_first("02 00 02 46 00 00 C3 8F", -4);
        if (FeatureSites({ site, hudSite }, "The Warriors: widescreen correction unavailable.\n")) {
            // DIV.S supplies the perspective aspect, followed by the native LUI.
            // No VFPU use in these callbacks. Saving VFPU state would restore the prefix
            // registers before returning, making PPSSPP's JIT drop its default-prefix assumption.
            safetymips::Options replace; replace.execute_original = false; replace.preserve = PSP_HOOK_SAVE_FPU;
            projection = safetymips::create_mid(site, [](SafetyMipsContext& regs) {
                if (automaticAspect) aspect = Aspect();
                regs.f12 = aspect; regs.s3 = 0x1C000000;
            }, replace);
            hud = safetymips::create_mid(hudSite,
                [](SafetyMipsContext& regs) { regs.f1 = aspect; regs.f0 *= regs.f2; }, replace);
        }
    }
    if (inireader.ReadBoolean("MAIN", "UnthrottleEmuDuringLoading", true)) {
        safetymips::Options scalar; scalar.preserve = PSP_HOOK_SAVE_FPU;
        const auto entry = pattern.get_first("00 00 B0 AF ? ? ? ? ? ? ? ? 20 00 25 8E", 4);
        const auto exit = pattern.get_first("08 00 BF AF 04 00 B1 AF ? ? ? ? 00 00 B0 AF 06 00 03 24", 0);
        if (FeatureSites({ entry, exit }, "The Warriors: loading speed control unavailable.\n")) {
            load.fun = injector::MakeCALL(entry, Loading).get();
            loading = safetymips::create_mid(exit, [](SafetyMipsContext&) { Unthrottle(false); }, scalar);
        }
    }
    return Finish();
}
}
extern "C" int module_start(SceSize, void*) {
    logger.SetPath("ms0:/PSP/PLUGINS/TheWarriors.PPSSPP.FusionFix/TheWarriors.PPSSPP.FusionFix.log");
    constexpr auto cfg = "ms0:/PSP/PLUGINS/TheWarriors.PPSSPP.FusionFix/TheWarriors.PPSSPP.FusionFix.cfg";
    if (!Start("WARR", cfg, "ms0:/PSP/PLUGINS/TheWarriors.PPSSPP.FusionFix/TheWarriors.PPSSPP.FusionFix.log")) return 0;
    // Retain the original game's initialization workaround until it can be
    // checked on the emulator; it has no effect on normal CFG contents.
    inireader.SetIniPath(cfg);
    IniValue defaults[] = {{"MAIN", "SkipIntro", "1"}, {"MAIN", "Enable60FPS", "0"}, {"MAIN", "DualAnalogPatch", "1"},
        {"MAIN", "ForceAspectRatio", "auto"}, {"MAIN", "UnthrottleEmuDuringLoading", "1"}};
    (void)CompleteCfg(cfg, defaults);
    return Install();
}
