#include "Game.hpp"
#include "Shared/Console/PatchNotice.hpp"
#include <pspthreadman.h>
#include <cmath>
namespace essentials {
namespace {
// AEPlayerController states that frame a 3D device for fixed 2D overlays
// (hacking waves and bars, keypad and lock-pick close-ups, computer screens,
// the optic cable mask). Their cameras are authored for the native FOV.
constexpr uint32_t deviceStates[] = {
    NameCrc("s_HackInteract"), NameCrc("s_SoundHackInteract"), NameCrc("s_KeyPadInteract"),
    NameCrc("s_PickLock"), NameCrc("s_Computer"), NameCrc("s_OpticCable"),
};
static_assert(NameCrc("s_HackInteract") == 0x72988260u, "FNameCRC");
bool Game(uintptr_t address) { return address >= 0x08800000 && address < 0x0A000000 && !(address & 3); }
bool DeviceView(uintptr_t viewport) {
    // UViewport::Actor -> UObject::StateFrame -> StateNode -> Name.
    if (!Game(viewport)) return false;
    const auto controller = injector::ReadMemory<uint32_t>(viewport + 0x34);
    if (!Game(controller)) return false;
    const auto frame = injector::ReadMemory<uint32_t>(controller + 0x0C);
    if (!Game(frame)) return false;
    const auto state = injector::ReadMemory<uint32_t>(frame + 0x18);
    if (!Game(state)) return false;
    const auto name = injector::ReadMemory<uint32_t>(state + 0x20);
    for (auto device : deviceStates) if (name == device) return true;
    return false;
}
// 0 = gameplay view, 1 = device view; eased so the switch follows the
// game's own camera blend instead of popping.
float deviceBlend;
uint32_t lastFrame;
void UpdateFov(uintptr_t viewport) {
    const float target = DeviceView(viewport) ? 1.0f : 0.0f;
    const uint32_t now = sceKernelGetSystemTimeLow();
    const float elapsed = lastFrame ? float(now - lastFrame) * 1e-6f : 1.0f;
    lastFrame = now;
    const float step = elapsed >= 0.25f ? 1.0f : elapsed * 4;
    deviceBlend += (target - deviceBlend) * step;
    if (std::fabs(target - deviceBlend) < 0.002f) deviceBlend = target;
}
float CameraFov(float scripted) {
    // Gameplay: the existing expanded view (4:3 -> native PSP aspect), scaled
    // by FOVFactor. The projection-aspect hook below maps it to the display.
    const float gameplay = console::horizontal_fov(scripted, 4.0f / 3, 480.0f / 272) * fovFactor;
    if (deviceBlend <= 0) return gameplay;
    // Device close-ups: the native PSP view (FOVFactor 1, no expansion) in the
    // centered box their fixed 2D overlays (hacking waves and bars, keypad,
    // lock pick) are drawn in; wider screens add to the sides.
    const float device = console::horizontal_fov(scripted, nativeAspect, std::max(aspect, nativeAspect));
    return gameplay + (device - gameplay) * deviceBlend;
}
SafetyMipsMid camera;
SafetyMipsInline draw, projectionAspect;
uintptr_t cameraCallers[5];
void RefreshDisplaySettings()
{
    static uint32_t lastRead;
    const uint32_t now = sceKernelGetSystemTimeLow();
    if (lastRead && now - lastRead < 1000000u) return;
    lastRead = now;
    const SceUID file = sceIoOpen(inireader.iniName, PSP_O_RDONLY, 0);
    if (file < 0) return;
    static char contents[8192];
    const int size = sceIoRead(file, contents, sizeof(contents) - 1);
    sceIoClose(file);
    if (size <= 0 || size == int(sizeof(contents) - 1)) return;
    contents[size] = '\0';
    char fov[32]{}, scale[32]{}, box[64]{}, display[64]{};
    if (!rini_get_key("MAIN", "FOVFactor", contents, size, fov, sizeof(fov), STRING_VAL) ||
        !rini_get_key("MAIN", "HudScale", contents, size, scale, sizeof(scale), STRING_VAL) ||
        !rini_get_key("MAIN", "HudAspectRatioConstraint", contents, size, box, sizeof(box), STRING_VAL) ||
        !rini_get_key("MAIN", "ForceAspectRatio", contents, size, display, sizeof(display), STRING_VAL)) return;
    char* end = nullptr;
    const float nextFov = std::strtof(fov, &end);
    if (end == fov || !std::isfinite(nextFov)) return;
    const float nextScale = std::strtof(scale, &end);
    if (end == scale || !std::isfinite(nextScale)) return;
    fovFactor = console::bounded(nextFov, 0.5f, 2.0f, 1.0f);
    hudScale = console::bounded(nextScale, 0.5f, 1.5f, 1.0f);
    hudConstraint = console::parse_hud_aspect(box);
    forced = console::parse_forced_aspect(display);
}
bool CameraReturns(uintptr_t function, size_t size, uintptr_t constructor, unsigned& count) {
    if (!function || !constructor) return false;
    for (uintptr_t at = function; at < function + size; at += 4) {
        const auto instruction = injector::ReadMemory<uint32_t>(at);
        if ((instruction >> 26) == 3 &&
            (((at + 4) & 0xF0000000) | ((instruction & 0x03FFFFFF) << 2)) == constructor) {
            if (count == 5) return false;
            cameraCallers[count++] = at + 8;
        }
    }
    return true;
}
void Draw(uintptr_t engine, uintptr_t viewport, int present, void* pixels, int* size) {
    RefreshDisplaySettings();
    UpdateDisplay();
    UpdateFov(viewport);
    draw.call<void>(engine, viewport, present, pixels, size);
}
}
void InstallCamera() {
    fovFactor = console::bounded(inireader.ReadFloat("MAIN", "FOVFactor", 1), 0.5f, 2.0f, 1);
    const auto game = sites::GameDraw(), screen = sites::ScreenToWorld(), world = sites::WorldToScreen(), vectors = sites::VectorWorldToScreen();
    const auto constructor = sites::CameraConstructor();
    unsigned count = 0;
    const bool callers = CameraReturns(game, 0x500, constructor, count) &&
        CameraReturns(screen, 0x400, constructor, count) && CameraReturns(world, 0x400, constructor, count) &&
        CameraReturns(vectors, 0x140, constructor, count) && count == 5;
    constexpr auto projectionSignature = "80 3F 04 3C 08 00 E0 03 00 00 84 44";
    const auto projection = pattern.get(2, projectionSignature, 0);
    if (!callers || !projection || pattern.get(3, projectionSignature, 0)) {
        PatchRejected(PSP_HOOK_UNSUPPORTED_INSTRUCTION);
        return;
    }
    camera = safetymips::create_mid(constructor, [](SafetyMipsContext& regs) {
        for (auto caller : cameraCallers) if (regs.ra == caller) {
            // The projection keeps the vertical FOV from this horizontal FOV and
            // the display aspect (see projectionAspect). UI portals and preview
            // cameras retain their scripted FOV.
            regs.f12 = console::bounded(CameraFov(regs.f12), 1, 175, regs.f12);
            break;
        }
    });
    // The renderer asks this for its projection aspect. Match physical pixels,
    // including native WorldToScreen/ScreenToWorld matrices and culling.
    // Capture the complete leaf getter, including JR RA and its delay slot.
    projectionAspect = safetymips::create_inline(projection,
        +[]() -> float { return (480.0f / 272) / aspect; });
    draw = safetymips::create_inline(game, Draw);
}
}
