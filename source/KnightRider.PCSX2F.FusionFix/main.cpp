#include "Camera.hpp"
#include "../Shared/Console/PS2Settings.hpp"
#include "../Shared/Console/Ps2PluginConflict.hpp"
#include <cstdio>
#include <cstdlib>
PCSX2F_CFG_SETTINGS()
PCSX2F_CFG_PEER_SETTINGS()
PCSX2F_STATE_LOAD_GENERATION()

extern "C"
{
    int CompatibleCRCList[] = {static_cast<int>(0x989192FE)};
    int PCSX2Data[PCSX2Data_Size] = {1};
    char OSDText[OSDStringNum][OSDStringSize] = {{1}};
}

namespace
{
using namespace kr;
SafetyMipsMid cameraDefaults, projectionDefaults, horizontalProjection, separateProjection;
SafetyMipsMid screenPolygon, screenTexture, polygonEntry, textureEntry, cameraData, objectMatrix, clickEnd;
SafetyMipsMid endFrame, cinematicAnimation, cinematicStatic;
console::ps2settings::Value options[] = {
    {"MAIN", "ForceAspectRatio", "Auto"}, {"MAIN", "HudAspectRatioConstraint", "Auto"}, {"MAIN", "FOVFactor", "1.0"},
    {"MAIN", "CutsceneBorders", "3"}, {"MAIN", "SkipIntro", "1"}, {"MAIN", "FMVWidescreenMode", "1"}};
console::ps2settings::StateLoads stateLoads;
uint32_t refreshFrames, layout;

void ApplySettings()
{
    forcedAspect = console::parse_forced_aspect(options[0].value);
    hudAspect = console::parse_hud_aspect(options[1].value);
    char* end = nullptr;
    const float factor = std::strtof(options[2].value, &end);
    fovFactor = end != options[2].value && std::isfinite(factor) ? std::clamp(factor, 0.5f, 2.0f) : 1.0f;
    cutsceneBorders = std::clamp(std::atoi(options[3].value), 0, 3);
    skipIntro = console::ps2settings::boolean(options[4], true);
    fmvFill = console::ps2settings::boolean(options[5], true);
}
void RefreshSettings()
{
    if (console::ps2settings::read(options))
    {
        ApplySettings();
    }
}

// NiPS2Renderer::EndFrame (0x367B80): once per presented frame, menus included.
void EndFrame(SafetyMipsContext&)
{
    ++frame;
    stateLoads.poll(RefreshSettings);
    if (++refreshFrames >= 30)
    {
        refreshFrames = 0;
        RefreshSettings();
    }
    // Cached 2D packets keep the layout they were built with.
    const uint32_t current = Bits(Aspect()) ^ (Bits(HudShift()) * 31u) ^ (uint32_t(cutsceneBorders) << 4) ^ Cinematic();
    if (current != layout)
    {
        layout = current;
        hud::rebuild2D = 4000;
    }
}

void CameraData(SafetyMipsContext& regs)
{
    hud::wide.active = false;
    if (!camera::CameraData(regs))
    {
        hud::CameraData(regs);
    }
}

#ifdef WFP_PS2_DEBUG
// PINE-readable traces. cameras: magic, then per camera {camera, ra, clicks,
// view, frustum[6], viewport[4], root, last frame}. counters: magic, frame,
// main camera, then hit counts.
uint32_t cameras[4 + 48 * 16] = {0x534D4143};
uint32_t counters[32] = {0x544E5443};
SafetyMipsMid probes[16];
template <unsigned Index> void Count(SafetyMipsContext&)
{
    ++counters[Index];
    counters[16 + Index] = frame;
}
// 2D elements drawn in the subtitles view: {kind, left, right, top, bottom, count}.
uint32_t flats[4 + 32 * 6] = {0x54414C46};
void TraceFlat(uint32_t kind, uint32_t object, float l, float r, float t, float b)
{
    if (!hud::subtitles)
    {
        return;
    }
    uint32_t* e = flats + 4;
    unsigned i = 0;
    while (i < 32 && e[i * 6] && e[i * 6] != object)
    {
        ++i;
    }
    if (i == 32)
    {
        return;
    }
    e += i * 6;
    e[0] = object;
    e[1] = kind | (hud::view & 0xFF) << 8;
    e[2] = Bits(l);
    e[3] = Bits(r);
    e[4] = Bits(t);
    e[5] = Bits(b);
}
void TraceClick(SafetyMipsContext& regs)
{
    counters[1] = frame;
    counters[2] = camera::Main();
    const auto camera = uintptr_t(regs.a0);
    uint32_t* e = cameras + 4;
    unsigned i = 0;
    while (i < 48 && e[i * 16] && e[i * 16] != camera)
    {
        ++i;
    }
    if (i == 48)
    {
        return;
    }
    e += i * 16;
    e[0] = camera;
    e[1] = regs.ra;
    ++e[2];
    e[3] = uint32_t(hud::view);
    for (unsigned k = 0; k < 10; ++k)
    {
        e[4 + k] = Read<uint32_t>(camera + 0x154 + 4 * k);
    }
    e[14] = Read<uint32_t>(camera + 0x17C);
    e[15] = frame;
}
#endif

// Camera property updates of scripted cameras.
template <unsigned Index> void Scripted(SafetyMipsContext&)
{
    cinematicFrame = frame;
#ifdef WFP_PS2_DEBUG
    ++counters[Index];
    counters[16 + Index] = frame;
#endif
}

void Rejected(pcsx2_hook_status status)
{
    std::snprintf(OSDText[0], OSDStringSize, "Knight Rider fix disabled: patch validation failed (%u)",
                  unsigned(status));
}
} // namespace

extern "C" void init()
{
    static constexpr console::ps2conflict::OldPlugin oldPlugins[] = {
        {"KnightRider.PCSX2F.WidescreenFix.elf", "MAIN", "ForceAspectRatio"},
    };
    if (console::ps2conflict::Found(oldPlugins, OSDText))
        return;
    if (injector::InitializeCheckedRuntime(Rejected) != PCSX2_HOOK_OK)
    {
        return;
    }
    if (!console::ps2settings::load(options))
    {
        std::snprintf(OSDText[0], OSDStringSize,
                      "CFG settings unavailable: update the plugin injector for CFG support.");
    }
    ApplySettings();
    safetymips::Options replace;
    replace.execute_original = false;
    cameraDefaults = safetymips::create_mid<&camera::SetFovDefaults>(0x3E3600, nullptr, replace);
    projectionDefaults = safetymips::create_mid<&camera::ProjectionDefaults>(0x3EB864, nullptr, replace);
    horizontalProjection = safetymips::create_mid<&camera::HorizontalProjection>(0x3EB910);
    separateProjection = safetymips::create_mid<&camera::SeparateProjection>(0x3EBA60);
    screenPolygon = safetymips::create_mid<&hud::ScreenPolygon>(0x360C5C);
    screenTexture = safetymips::create_mid<&hud::ScreenTexture>(0x3607E0);
    polygonEntry = safetymips::create_mid(0x3608E0, [](SafetyMipsContext& regs) { hud::Rebuild(regs, 0x1C); });
    textureEntry = safetymips::create_mid(0x360380, [](SafetyMipsContext& regs) { hud::Rebuild(regs, 0x18); });
    cameraData = safetymips::create_mid<&CameraData>(0x3233BC);
    objectMatrix = safetymips::create_mid<&hud::ObjectMatrix>(0x35F810);
    // NiCamera::Click epilogue: later geometry is not drawn through a widened camera.
    clickEnd = safetymips::create_mid(0x323644, [](SafetyMipsContext&) { hud::ClickEnd(); });
    endFrame = safetymips::create_mid<&EndFrame>(0x367B80);
    // Scripted cameras: intro camera animation and static (roadside) cameras.
    cinematicAnimation = safetymips::create_mid<&Scripted<14>>(0x100220);
    cinematicStatic = safetymips::create_mid<&Scripted<15>>(0x1255F0);
    hud::Install();
    movie::Install();
#ifdef WFP_PS2_DEBUG
    hud::trace2D = &TraceFlat;
    probes[0] = safetymips::create_mid<&TraceClick>(0x323380);
    probes[1] = safetymips::create_mid<&Count<3>>(0x1264F0);  // follow outside
    probes[2] = safetymips::create_mid<&Count<4>>(0x126DC0);  // follow inside
    probes[3] = safetymips::create_mid<&Count<5>>(0x125CF0);  // follow side
    probes[4] = safetymips::create_mid<&Count<6>>(0x126080);  // follow rotate
    probes[5] = safetymips::create_mid<&Count<7>>(0x159120);  // follow vehicle
    probes[6] = safetymips::create_mid<&Count<8>>(0x3ECAF0);  // world frame
    probes[7] = safetymips::create_mid<&Count<9>>(0x152890);  // super pursuit
    probes[8] = safetymips::create_mid<&Count<10>>(0x125350); // sky box
    probes[9] = safetymips::create_mid<&Count<11>>(0x1004E0); // intro camera object
    probes[10] = safetymips::create_mid<&Count<12>>(0x124C40); // cinematic director
    probes[11] = safetymips::create_mid<&Count<13>>(0x10DB40); // static camera
#endif
    injector::FlushCaches();
}
extern "C" int main()
{
    return 0;
}
