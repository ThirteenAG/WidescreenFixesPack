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
SafetyMipsMid movieUpload, movieDraw, endFrame, cinematicAnimation, cinematicStatic, videoCreate, videoUpdate;
console::ps2settings::Value options[] = {
    {"MAIN", "ForceAspectRatio", "Auto"}, {"MAIN", "HudAspectRatioConstraint", "Auto"}, {"MAIN", "FOVFactor", "1.0"},
    {"MAIN", "CutsceneBorders", "3"}, {"MAIN", "SkipIntro", "1"}};
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

// FMV: NiPS2Renderer::UploadMovieFrame (0x363340: renderer, image, columns,
// rows of 16 px) sends the decoded frame in 16-px columns straight into the
// back buffer, so the 4:3 video fills the widened screen. During playback
// nothing else uses the depth buffer, so the frame is uploaded there instead
// and drawn back as a scaled sprite through GS context 2 (the renderer sets
// context 2 itself before each of its own uses).
struct
{
    bool active;
    uintptr_t renderer;
    uint32_t width, height;
} movie;
void MovieUpload(SafetyMipsContext& regs)
{
    movie.active = false;
    if ((Fit() == 1 && VerticalFit() == 1) || !draw::packetBegin)
    {
        return;
    }
    const auto r = uintptr_t(regs.s5);
    const uint32_t width = regs.s3 * 16, height = regs.s1 * 16;
    const uint32_t fbw = Read<uint32_t>(r + 0x998);
    const uint64_t scissor = Read<uint64_t>(r + 0x988);
    const uint32_t fbWidth = uint32_t(scissor >> 16 & 0x7FF) + 1, fbHeight = uint32_t(scissor >> 48 & 0x7FF) + 1;
    const uint32_t zbp = Read<uint32_t>(r + 0x2B8), zpsm = Read<uint32_t>(r + 0x2BC);
    // The 32-bit depth buffer has the frame buffer's size and stride.
    if (!Read<uint8_t>(r + 0x2C0) || (zpsm != 0x30 && zpsm != 0x31) || !zbp || zbp >= 0x200)
    {
        return;
    }
    if (!width || !height || width > fbWidth || height > fbHeight || width > fbw * 64 || width > 1024 || height > 1024)
    {
        return;
    }
    auto& dbp = regs.raw().gpr[30].word[1]; // fp = BITBLTBUF, DBP in bits 32-45
    dbp = (dbp & ~0x3FFFu) | (zbp * 32);
    movie = {true, r, width, height};
}
uint32_t Log2(uint32_t v)
{
    uint32_t n = 0;
    while ((1u << n) < v)
    {
        ++n;
    }
    return n;
}
// 0x36AA50 entry (closes the upload with TEXFLUSH and an end tag), a0 =
// renderer + 0x44 packet; called from 0x363420.
void MovieDraw(SafetyMipsContext& regs)
{
    if (regs.ra != 0x363428 || !movie.active)
    {
        return;
    }
    movie.active = false;
    const auto r = movie.renderer, packet = uintptr_t(regs.a0);
    if (packet != r + 0x44)
    {
        return;
    }
    const uint32_t width = movie.width, height = movie.height;
    const uint64_t fbw = Read<uint32_t>(r + 0x998), psm = Read<uint32_t>(r + 0x99C);
    const uint64_t fbp = Read<uint32_t>(r + 0x990 + 4 * Read<uint32_t>(r + 0x9A0));
    const uint64_t zbp = Read<uint32_t>(r + 0x2B8), zpsm = Read<uint32_t>(r + 0x2BC);
    const uint32_t ofx = Read<uint32_t>(r + 0x9A4), ofy = Read<uint32_t>(r + 0x9A8);
    // Destination in frame pixels and source rows (1/16 texel).
    const float frameWidth = Fit(), frameHeight = VerticalFit();
    const uint32_t x0 = uint32_t(float(width) * (1 - frameWidth) * 0.5f + 0.5f), x1 = width - x0;
    const uint32_t y0 = uint32_t(float(height) * (1 - frameHeight) * 0.5f + 0.5f), y1 = height - y0;
    const uint32_t v0 = 0, v1 = height * 16;
    constexpr unsigned strips = 8, count = 12 + 3 + 1 + strips * 4;
    draw::packetBegin(packet, count + 1);
    draw::Writer w{reinterpret_cast<uint64_t*>(Read<uintptr_t>(packet + 0x14))};
    auto xy = [&](uint32_t x, uint32_t y) { return uint64_t((ofx + x) * 16) | uint64_t((ofy + y) * 16) << 16; };
    w.Put(0x1000000000000000ull | count, 0xE);                  // GIFtag: A+D, NLOOP = count
    w.Put(0, 0x3F);                                             // TEXFLUSH
    w.Put(fbp | fbw << 16 | psm << 24, 0x4D);                   // FRAME_2
    w.Put(zbp | zpsm << 24 | 1ull << 32, 0x4F);                 // ZBUF_2, masked
    w.Put(0x30000, 0x48);                                       // TEST_2: depth always
    w.Put(0, 0x43);                                             // ALPHA_2
    w.Put(0, 0x4B);                                             // FBA_2
    w.Put(Read<uint64_t>(r + 0x988), 0x41);                     // SCISSOR_2
    w.Put(uint64_t(ofx * 16) | uint64_t(ofy * 16) << 32, 0x19); // XYOFFSET_2
    w.Put(zbp * 32 | fbw << 14 | uint64_t(Log2(width)) << 26 | uint64_t(Log2(height)) << 30 | 1ull << 34 | 1ull << 35,
          0x07);                        // TEX0_2: CT32, RGBA, decal
    w.Put(0x60, 0x15);                  // TEX1_2: bilinear
    w.Put(5, 0x09);                     // CLAMP_2
    w.Put(0x3F80000080000000ull, 0x01); // RGBAQ: black
    w.Put(0x206, 0x00);                 // PRIM: sprite, context 2
    w.Put(xy(0, 0), 0x05);
    w.Put(xy(width, height), 0x05);
    w.Put(0x316, 0x00); // PRIM: textured sprite, UV, context 2
    for (unsigned i = 0; i < strips; ++i)
    {
        const uint32_t a = x0 + (x1 - x0) * i / strips, b = x0 + (x1 - x0) * (i + 1) / strips;
        const uint32_t ua = uint32_t(uint64_t(a - x0) * width * 16 / (x1 - x0)),
                       ub = uint32_t(uint64_t(b - x0) * width * 16 / (x1 - x0));
        w.Put(ua | uint64_t(v0) << 16, 0x03);
        w.Put(xy(a, y0), 0x05);
        w.Put(ub | uint64_t(v1) << 16, 0x03);
        w.Put(xy(b, y1), 0x05);
    }
    Write<uintptr_t>(packet + 0x14, uintptr_t(w.q));
    Write<uint32_t>(packet + 0x10, Read<uint32_t>(packet + 0x10) - (count + 1));
    draw::packetEnd(packet);
}

// Skip intro: the logo (DaviLead) and opening (intro) movies of the start-up
// sequence. CDaviVideoGameState::Update (0x2557B0) ends the movie itself in
// its states 2 and 3, as the Start button does.
uintptr_t startupVideo[2];
bool openingCreated;
void VideoCreate(SafetyMipsContext& regs)
{
    // 0x255B20 (state, path), from DaviLead (0x1371D8) and intro (0x136F90).
    if (!skipIntro)
    {
        return;
    }
    if (regs.ra == 0x1371E0)
    {
        startupVideo[0] = regs.a0;
    }
    else if (regs.ra == 0x136F98 && !openingCreated)
    {
        openingCreated = true;
        startupVideo[1] = regs.a0;
    }
}
void VideoUpdate(SafetyMipsContext& regs)
{
    for (auto& video : startupVideo)
    {
        if (video && video == regs.a0)
        {
            video = 0;
            if (Read<uint32_t>(regs.a0 + 0x10) == 0)
            {
                Write<uint32_t>(regs.a0 + 0x10, 2);
            }
        }
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
    if (draw::Bind())
    {
        movieUpload = safetymips::create_mid<&MovieUpload>(0x3633B0);
        movieDraw = safetymips::create_mid<&MovieDraw>(0x36AA50);
    }
    videoCreate = safetymips::create_mid<&VideoCreate>(0x255B20);
    videoUpdate = safetymips::create_mid<&VideoUpdate>(0x2557B0);
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
