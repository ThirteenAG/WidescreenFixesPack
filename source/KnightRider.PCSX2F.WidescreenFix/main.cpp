#include "../../external/injector/include/ps2/runtime.hpp"
#include "../../external/injector/include/ps2/game_abi.hpp"
#include "../Shared/Console/Viewport.hpp"
#include "../Shared/Console/PS2Display.hpp"
#include <cstdio>
#include <cstring>
#include <cmath>

extern "C" {
int CompatibleCRCList[] = {static_cast<int>(0x989192FE)};
int PCSX2Data[PCSX2Data_Size] = {1};
char OSDText[OSDStringNum][OSDStringSize] = {{1}};
}

namespace {
SafetyMipsMid cameraDefaults, projectionDefaults, horizontalProjection, separateProjection;
SafetyMipsMid screenPolygon, screenTexture, cameraData, objectMatrix, clickEnd, movieUpload, movieDraw;
float Aspect() { return console::ps2Aspect(PCSX2Data); }
uint32_t Bits(float value) { uint32_t bits; std::memcpy(&bits, &value, sizeof(bits)); return bits; }
template<class T> T Read(uintptr_t address) { return *reinterpret_cast<const T*>(address); }
template<class T> void Write(uintptr_t address, T value) { *reinterpret_cast<T*>(address) = value; }

void CameraDefaults(SafetyMipsContext& regs) {
    const auto bits = Bits(Aspect());
    regs.v0 = bits & 0xFFFF0000;
    regs.v1 = bits;
}
void ProjectionDefaults(SafetyMipsContext& regs) { regs.v0 = Bits(Aspect()); }
void HorizontalProjection(SafetyMipsContext& regs) {
    const float aspect = Aspect();
    regs.f12 = console::horizontal_fov(regs.f12, 4.0f / 3.0f, aspect);
    regs.f13 = aspect;
}
void SeparateProjection(SafetyMipsContext& regs) {
    // This overload accepts horizontal and vertical angles separately.
    // 0x3EBAE0 (from 0x125350) copies the angles that 0x3EBFE0 measured back
    // from the main camera's frustum, which is already widened: keep them.
    if (regs.ra == 0x3EBB34) return;
    const float aspect = Aspect();
    // 0x3EB660 passes FOV and FOV / AspectRatio from its settings; the
    // default AspectRatio is the screen aspect (0x3EB864), so restore the
    // vertical angle of the 4:3 default.
    if (regs.ra == 0x3EB6FC && std::fabs(Read<float>(uintptr_t(regs.s3) + 8) - aspect) < 0.001f)
        regs.f13 = regs.f12 * 0.75f;
    regs.f12 = console::horizontal_fov(regs.f12, 4.0f / 3.0f, aspect);
}

// 2D: NetImmerse NiScreenPolygon (normalized vertices) and NiScreenTexture
// (pixel rectangles) are converted to GS packets by NiPS2Renderer. The
// packets are cached per object and rebuilt when the object is dirty, so the
// horizontal fit is applied once per rebuild.
float Fit() {
    const float k = (4.0f / 3.0f) / Aspect();
    return k > 0 && k < 1 ? k : 1;
}
// Horizontal anchor (0..1) for an element spanning [left, right] of the
// screen; a negative result keeps the element unchanged.
float Anchor(float left, float right) {
    if (left <= 0.01f && right >= 0.99f) return -1;  // fades and backgrounds
    return 0.5f;
}

// The 4:3 camera currently drawn widened (see CameraData).
struct WideCamera {
    bool active;
    float stretch, halfWidth;      // 1 / fit, original right/near ratio
    float location[3], direction[3], right[3];
} wide;
#ifdef WFP_PS2_DEBUG
// PINE-readable trace: magic 'KR2D', count, then {kind, left, right, top, bottom, object, n, anchor}.
uint32_t trace[2 + 8 * 512] = {0x44325254};
uint32_t calls[9] = {0x534C4143};
SafetyMipsMid probes[11];
uint32_t setters[4 + 32 * 4] = {0x53544553};
uint32_t currentCamera;
uint32_t fmvProbe[8] = {0x5F564D46};
uint32_t geoms[4 + 64 * 8] = {0x4D4F4547};
void LogObject(uint32_t data, uint32_t matrix, float radius, float depth, float side, bool stretch) {
    uint32_t* g = geoms + 4;
    unsigned i = 0;
    while (i < 64 && g[i * 8] && g[i * 8] != data) ++i;
    if (i == 64) return;
    g[i * 8] = data; g[i * 8 + 1] = currentCamera; g[i * 8 + 2] = Bits(radius); g[i * 8 + 3] = Bits(depth);
    g[i * 8 + 4] = Bits(side); g[i * 8 + 5] = Bits(wide.halfWidth); g[i * 8 + 6] = stretch; g[i * 8 + 7] = matrix;
}
uint32_t cameras[4 + 32 * 16] = {0x534D4143};
void Trace(uint32_t kind, float l, float r, float t, float b, uintptr_t object, uint32_t n, float anchor) {
    if (trace[1] >= 512) return;
    for (unsigned i = 0; i < trace[1]; ++i) if (trace[2 + 8 * i + 5] == object && trace[2 + 8 * i] == kind) return;
    uint32_t* e = trace + 2 + 8 * trace[1]++;
    e[0] = kind; e[1] = Bits(l); e[2] = Bits(r); e[3] = Bits(t); e[4] = Bits(b); e[5] = object; e[6] = n; e[7] = Bits(anchor);
}
#endif

// 0x360C5C in NiPS2Renderer::RenderScreenPolygon: f21 + f23 * x is the GS X
// of a normalized vertex x. s7 = polygon, s6 = vertex count.
void ScreenPolygon(SafetyMipsContext& regs) {
#ifdef WFP_PS2_DEBUG
    ++calls[1];
#endif
    const float k = Fit();
    const auto polygon = uintptr_t(regs.s7);
    const unsigned count = Read<uint16_t>(polygon + 12);
    const auto vertices = Read<uintptr_t>(polygon + 16);
    if (k == 1 || !count || !vertices) return;
    float left = Read<float>(vertices), right = left, top = Read<float>(vertices + 4), bottom = top;
    for (unsigned i = 1; i < count; ++i) {
        const float x = Read<float>(vertices + 12 * i), y = Read<float>(vertices + 12 * i + 4);
        if (x < left) left = x;
        if (x > right) right = x;
        if (y < top) top = y;
        if (y > bottom) bottom = y;
    }
    // Screen polygons are projected markers (objective arrows) placed by the
    // widened 3D camera: keep their position, fit their shape.
    const float anchor = Anchor(left, right) < 0 ? -1 : 0.5f * (left + right);
#ifdef WFP_PS2_DEBUG
    Trace(0x594C4F50, left, right, top, bottom, polygon, count, anchor);
#endif
    if (anchor < 0) return;
    // x' = anchor + (x - anchor) * k
    regs.f21 = regs.f21 + regs.f23 * anchor * (1 - k);
    regs.f23 = regs.f23 * k;
}

// 0x3607E0 in NiPS2Renderer::RenderScreenTexture, after the sprite packet is
// built: fp = packet, s1 = rectangle count. Each 0x28-byte record after the
// 0x20-byte header holds RGBAQ, UV, XYZ2, UV, XYZ2 (X in 12.4 with offset).
void ScreenTexture(SafetyMipsContext& regs) {
#ifdef WFP_PS2_DEBUG
    ++calls[2];
#endif
    const float k = Fit();
    const auto renderer = uintptr_t(regs.s6), packet = uintptr_t(regs.fp);
    const unsigned count = regs.s1;
    const int width = Read<int32_t>(renderer + 0x828), origin = Read<int32_t>(renderer + 0x9A4) * 16;
    if (k == 1 || !count || !packet || width <= 0) return;
    auto x = [&](unsigned i, unsigned offset) { return reinterpret_cast<uint16_t*>(packet + 0x20 + 0x28 * i + offset); };
    int left = *x(0, 0x10), right = *x(0, 0x20);
    for (unsigned i = 1; i < count; ++i) {
        if (*x(i, 0x10) < left) left = *x(i, 0x10);
        if (*x(i, 0x20) > right) right = *x(i, 0x20);
    }
    const float scale = 1.0f / (16.0f * float(width));
    const float l = float(left - origin + 4) * scale, r = float(right - origin + 4) * scale;
    const float anchor = Anchor(l, r);
#ifdef WFP_PS2_DEBUG
    Trace(0x58455454, l, r, 0, 0, uintptr_t(regs.s5), count, anchor);
#endif
    if (anchor < 0) return;
    const float pivot = float(origin) + anchor * 16.0f * float(width);
    for (unsigned i = 0; i < count; ++i)
        for (unsigned offset : {0x10u, 0x20u}) {
            const float value = pivot + (float(*x(i, offset)) - pivot) * k;
            *x(i, offset) = uint16_t(int(value + 0.5f));
        }
}
// 0x3233BC in NiCamera::Click, before renderer->SetCameraData(..., t1 =
// frustum {left, right, top, bottom, near, far}, t2 = viewport); s2 = camera.
// Menu and overlay cameras come from NIF files with a 4:3 frustum; gameplay
// cameras are already widened by the projection hooks above. Widen only 4:3
// frustums, on a copy, so the camera objects keep their stored values.
float frustum[6];
struct { uintptr_t camera; float left, right; } restore;
#ifdef WFP_PS2_DEBUG
uint32_t skipCameras[17] = {0x50494B53};
#endif
void CameraData(SafetyMipsContext& regs) {
    wide.active = false;
    const float k = Fit();
    if (k == 1) return;
    const auto camera = uintptr_t(regs.s2);
    const auto source = reinterpret_cast<const float*>(uintptr_t(regs.t1));
    // t2 (viewport) is only set in the delay slot of the call: the inset
    // viewport is changed in place and restored at the end of Click.
    auto port = reinterpret_cast<float*>(camera + 0x16C);
#ifdef WFP_PS2_DEBUG
    for (unsigned i = 1; i < 17; ++i) if (skipCameras[i] == regs.s2) {
        std::memcpy(frustum, source, sizeof(frustum));
        for (unsigned j = 0; j < 4; ++j) frustum[j] *= 3;
        regs.t1 = uint32_t(uintptr_t(frustum));
        return;
    }
#endif
    const float width = source[1] - source[0], height = source[2] - source[3];
    if (!width || !height || !(source[4] > 0)) return;
    const float portLeft = port[0], portRight = port[1];
    if (portLeft > 0.001f || portRight < 0.999f) {
        // Inset view (truck view, minimap, portraits): its frame belongs to
        // the HUD, which the widened 2D cameras keep in the centered 4:3
        // area, so map the viewport into that area too. The frustum is left
        // alone, so the view keeps its proportions.
        restore.camera = camera; restore.left = portLeft; restore.right = portRight;
        port[0] = 0.5f + (portLeft - 0.5f) * k;
        port[1] = 0.5f + (portRight - 0.5f) * k;
        return;
    }
    // 4:3 views and square normalized 2D views; other ratios (gameplay
    // cameras) are set up by the FOV helpers for the screen.
    const float ratio = std::fabs(width / height);
    if (std::fabs(ratio - 4.0f / 3.0f) > 0.01f && std::fabs(ratio - 1) > 0.01f) return;
    std::memcpy(frustum, source, sizeof(frustum));
    const float center = 0.5f * (source[0] + source[1]);
    frustum[0] = center + (source[0] - center) / k;
    frustum[1] = center + (source[1] - center) / k;
    regs.t1 = uint32_t(uintptr_t(frustum));
    // Camera world matrix rows: direction (local X), up, right (local Z), location.
    for (unsigned i = 0; i < 3; ++i) {
        wide.direction[i] = Read<float>(camera + 0x70 + 4 * i);
        wide.right[i] = Read<float>(camera + 0x90 + 4 * i);
        wide.location[i] = Read<float>(camera + 0xA0 + 4 * i);
    }
    wide.stretch = 1 / k;
    wide.halfWidth = 0.5f * std::fabs(width);  // NiFrustum is at unit distance
    wide.active = true;
#ifdef WFP_PS2_DEBUG
    currentCamera = regs.s2;
#endif
}
// Backgrounds, fades and bands that cover the whole 4:3 frustum are stretched
// along the camera's right axis so they still cover the widened frustum.
// 0x35F810 loads the object's model-to-world matrix (rows X, Y, Z, T as used
// by VU0) from a1; called from the NiPS2Renderer geometry dispatchers with the
// geometry data (model bound center at +0x10, radius at +0x1C) in s3.
float stretched[16] __attribute__((aligned(16)));
// The bound sphere test in ObjectMatrix also accepts HUD meshes that pack several
// panels into one geometry (both gameplay HUD boxes are one mesh with a
// centered bound). Check that the vertices really reach both sides of the
// 4:3 view. NiGeometryData: +8 vertex count (u16), +0x20 vertex array.
bool SpansView(uintptr_t data, const float* m) {
    const unsigned count = Read<uint16_t>(data + 8);
    const auto vertices = Read<uintptr_t>(data + 0x20);
    if (!count || count > 4096 || !vertices || vertices >= 0x02000000) return true;
    float side[4], depth[4];
    for (unsigned row = 0; row < 4; ++row) {
        side[row] = depth[row] = 0;
        for (unsigned i = 0; i < 3; ++i) {
            const float v = m[4 * row + i] - (row == 3 ? wide.location[i] : 0);
            side[row] += v * wide.right[i];
            depth[row] += v * wide.direction[i];
        }
    }
    const float limit = 0.95f * wide.halfWidth;
    bool left = false, right = false;
    const auto* v = reinterpret_cast<const float*>(vertices);
    for (unsigned n = 0; n < count && !(left && right); ++n, v += 3) {
        const float d = v[0] * depth[0] + v[1] * depth[1] + v[2] * depth[2] + depth[3];
        if (!(d > 0)) continue;
        const float s = v[0] * side[0] + v[1] * side[1] + v[2] * side[2] + side[3];
        if (s <= -limit * d) left = true;
        if (s >= limit * d) right = true;
    }
    return left && right;
}
void ObjectMatrix(SafetyMipsContext& regs) {
    if (!wide.active) return;
    const uint32_t ra = regs.ra;
    if (ra != 0x3653D4 && ra != 0x365474 && ra != 0x365514 && ra != 0x3655B4) return;
    const auto data = uintptr_t(regs.s3);
    const auto m = reinterpret_cast<const float*>(uintptr_t(regs.a1));
    if (!data || !m) return;
    float center[3], scale = 0;
    for (unsigned i = 0; i < 3; ++i) {
        center[i] = m[12 + i];
        for (unsigned j = 0; j < 3; ++j) center[i] += Read<float>(data + 0x10 + 4 * j) * m[4 * j + i];
        scale += m[i] * m[i];
    }
    const float radius = Read<float>(data + 0x1C) * std::sqrt(scale);
    float depth = 0, side = 0;
    for (unsigned i = 0; i < 3; ++i) {
        depth += (center[i] - wide.location[i]) * wide.direction[i];
        side += (center[i] - wide.location[i]) * wide.right[i];
    }
    bool background = depth > 0 && radius >= wide.halfWidth * depth + std::fabs(side);
    if (background) background = SpansView(data, m);
#ifdef WFP_PS2_DEBUG
    LogObject(regs.s3, regs.a1, radius, depth, side, background);
#endif
    if (!background) return;
    const float s = wide.stretch - 1;
    for (unsigned row = 0; row < 4; ++row) {
        const float* v = m + 4 * row;
        float* out = stretched + 4 * row;
        float along = 0;
        for (unsigned i = 0; i < 3; ++i) along += (v[i] - (row == 3 ? wide.location[i] : 0)) * wide.right[i];
        for (unsigned i = 0; i < 3; ++i) out[i] = v[i] + s * along * wide.right[i];
        out[3] = v[3];
    }
    regs.a1 = uint32_t(uintptr_t(stretched));
}
// FMV: NiPS2Renderer::UploadMovieFrame (0x363340: renderer, image, columns,
// rows of 16 px) sends the decoded frame in 16-px columns straight into the
// back buffer, so the 4:3 video fills the widened screen. During playback
// nothing else uses the depth buffer, so the frame is uploaded there instead
// and drawn back as a scaled sprite in the centered 4:3 area through GS
// context 2 (the renderer sets context 2 itself before each of its own uses).
pcsx2::GameFunction<void(uintptr_t, uint32_t)> packetBegin;  // 0x36B370: reserve n qwords after a DMA cnt tag
pcsx2::GameFunction<void(uintptr_t)> packetEnd;              // 0x36B260: close the cnt tag
struct {
    bool active;
    uintptr_t renderer;
    uint32_t width, height;
} movie;
void MovieUpload(SafetyMipsContext& regs) {
    movie.active = false;
    const float k = Fit();
    if (k == 1 || !packetBegin || !packetEnd) return;
    const auto r = uintptr_t(regs.s5);
    const uint32_t width = regs.s3 * 16, height = regs.s1 * 16;
    const uint32_t fbw = Read<uint32_t>(r + 0x998);
    const uint64_t scissor = Read<uint64_t>(r + 0x988);
    const uint32_t fbWidth = uint32_t(scissor >> 16 & 0x7FF) + 1, fbHeight = uint32_t(scissor >> 48 & 0x7FF) + 1;
    const uint32_t zbp = Read<uint32_t>(r + 0x2B8), zpsm = Read<uint32_t>(r + 0x2BC);
    // The 32-bit depth buffer has the frame buffer's size and stride.
    if (!Read<uint8_t>(r + 0x2C0) || (zpsm != 0x30 && zpsm != 0x31) || !zbp || zbp >= 0x200) return;
    if (!width || !height || width > fbWidth || height > fbHeight || width > fbw * 64 || width > 1024 || height > 1024) return;
    auto& dbp = regs.raw().gpr[30].word[1];  // fp = BITBLTBUF, DBP in bits 32-45
    dbp = (dbp & ~0x3FFFu) | (zbp * 32);
    movie = {true, r, width, height};
}
uint32_t Log2(uint32_t v) { uint32_t n = 0; while ((1u << n) < v) ++n; return n; }
// 0x36AA50 entry (closes the upload with TEXFLUSH and an end tag), a0 =
// renderer + 0x44 packet; called from 0x363420.
void MovieDraw(SafetyMipsContext& regs) {
    if (regs.ra != 0x363428 || !movie.active) return;
    movie.active = false;
    const auto r = movie.renderer, packet = uintptr_t(regs.a0);
    if (packet != r + 0x44) return;
    const uint32_t width = movie.width, height = movie.height;
    const uint64_t fbw = Read<uint32_t>(r + 0x998), psm = Read<uint32_t>(r + 0x99C);
    const uint64_t fbp = Read<uint32_t>(r + 0x990 + 4 * Read<uint32_t>(r + 0x9A0));
    const uint64_t zbp = Read<uint32_t>(r + 0x2B8), zpsm = Read<uint32_t>(r + 0x2BC);
    const uint32_t ofx = Read<uint32_t>(r + 0x9A4), ofy = Read<uint32_t>(r + 0x9A8);
    const float k = Fit();
    const uint32_t x0 = uint32_t(float(width) * (1 - k) * 0.5f + 0.5f), x1 = width - x0;
    constexpr unsigned strips = 8, count = 12 + 2 * 3 + 1 + strips * 4;
    packetBegin(packet, count + 1);
    auto* q = reinterpret_cast<uint64_t*>(Read<uintptr_t>(packet + 0x14));
    auto put = [&](uint64_t data, uint64_t reg) { q[0] = data; q[1] = reg; q += 2; };
    auto xy = [&](uint32_t x, uint32_t y) { return uint64_t((ofx + x) * 16) | uint64_t((ofy + y) * 16) << 16; };
    put(0x1000000000000000ull | count, 0xE);  // GIFtag: A+D, NLOOP = count
    put(0, 0x3F);                                                       // TEXFLUSH
    put(fbp | fbw << 16 | psm << 24, 0x4D);                             // FRAME_2
    put(zbp | zpsm << 24 | 1ull << 32, 0x4F);                           // ZBUF_2, masked
    put(0x30000, 0x48);                                                 // TEST_2: depth always
    put(0, 0x43);                                                       // ALPHA_2
    put(0, 0x4B);                                                       // FBA_2
    put(Read<uint64_t>(r + 0x988), 0x41);                               // SCISSOR_2
    put(uint64_t(ofx * 16) | uint64_t(ofy * 16) << 32, 0x19);           // XYOFFSET_2
    put(zbp * 32 | fbw << 14 | uint64_t(Log2(width)) << 26 | uint64_t(Log2(height)) << 30 | 1ull << 34 | 1ull << 35, 0x07);  // TEX0_2: CT32, RGBA, decal
    put(0x60, 0x15);                                                    // TEX1_2: bilinear
    put(5, 0x09);                                                       // CLAMP_2
    put(0x3F80000080000000ull, 0x01);                                   // RGBAQ: black
    put(0x206, 0x00);                                                   // PRIM: sprite, context 2
    put(xy(0, 0), 0x05); put(xy(x0, height), 0x05);
    put(0x206, 0x00);
    put(xy(x1, 0), 0x05); put(xy(width, height), 0x05);
    put(0x316, 0x00);                                                   // PRIM: textured sprite, UV, context 2
    for (unsigned i = 0; i < strips; ++i) {
        const uint32_t a = x0 + (x1 - x0) * i / strips, b = x0 + (x1 - x0) * (i + 1) / strips;
        const uint32_t ua = uint32_t(uint64_t(a - x0) * width * 16 / (x1 - x0)), ub = uint32_t(uint64_t(b - x0) * width * 16 / (x1 - x0));
        put(ua, 0x03); put(xy(a, 0), 0x05);
        put(ub | uint64_t(height * 16) << 16, 0x03); put(xy(b, height), 0x05);
    }
    Write<uintptr_t>(packet + 0x14, uintptr_t(q));
    Write<uint32_t>(packet + 0x10, Read<uint32_t>(packet + 0x10) - (count + 1));
    packetEnd(packet);
#ifdef WFP_PS2_DEBUG
    ++fmvProbe[6];
#endif
}
void Rejected(pcsx2_hook_status status) {
    std::snprintf(OSDText[0], OSDStringSize, "Knight Rider fix disabled: patch validation failed (%u)", unsigned(status));
}
}

extern "C" void init() {
    if (injector::InitializeCheckedRuntime(Rejected) != PCSX2_HOOK_OK) return;
    safetymips::Options replace;
    replace.execute_original = false;
    cameraDefaults = safetymips::create_mid<&CameraDefaults>(0x3E3600, nullptr, replace);
    projectionDefaults = safetymips::create_mid<&ProjectionDefaults>(0x3EB864, nullptr, replace);
    horizontalProjection = safetymips::create_mid<&HorizontalProjection>(0x3EB910);
    separateProjection = safetymips::create_mid<&SeparateProjection>(0x3EBA60);
    screenPolygon = safetymips::create_mid<&ScreenPolygon>(0x360C5C);
    screenTexture = safetymips::create_mid<&ScreenTexture>(0x3607E0);
    cameraData = safetymips::create_mid<&CameraData>(0x3233BC);
    objectMatrix = safetymips::create_mid<&ObjectMatrix>(0x35F810);
    if (packetBegin.bind(0x36B370) == PCSX2_HOOK_OK && packetEnd.bind(0x36B260) == PCSX2_HOOK_OK) {
        movieUpload = safetymips::create_mid<&MovieUpload>(0x3633B0);
        movieDraw = safetymips::create_mid<&MovieDraw>(0x36AA50);
    }
    // NiCamera::Click epilogue: later geometry is not drawn through a widened camera.
    clickEnd = safetymips::create_mid(0x323644, [](SafetyMipsContext&) {
        wide.active = false;
        if (!restore.camera) return;
        Write(restore.camera + 0x16C, restore.left); Write(restore.camera + 0x170, restore.right);
        restore.camera = 0;
    });
#ifdef WFP_PS2_DEBUG
    probes[0] = safetymips::create_mid(0x3608E0, [](SafetyMipsContext&) { ++calls[3]; });
    probes[1] = safetymips::create_mid(0x360380, [](SafetyMipsContext&) { ++calls[4]; });
    probes[2] = safetymips::create_mid(0x323380, [](SafetyMipsContext& regs) {
        ++calls[5]; currentCamera = 0;
        const auto camera = uintptr_t(regs.a0);
        unsigned i = 0;
        while (i < 32 && (cameras + 4)[i * 16] && (cameras + 4)[i * 16] != camera) ++i;
        if (i == 32) return;
        (cameras + 4)[i * 16] = camera; (cameras + 4)[i * 16 + 1] = regs.ra; ++(cameras + 4)[i * 16 + 2];
        for (unsigned k = 0; k < 13; ++k) (cameras + 4)[i * 16 + 3 + k] = Read<uint32_t>(camera + 0x150 + 4 * k);
    });
    probes[3] = safetymips::create_mid(0x335A90, [](SafetyMipsContext&) { ++calls[6]; });
    probes[10] = safetymips::create_mid(0x366DC0, [](SafetyMipsContext& regs) {
        uint32_t* e = setters + 4;
        unsigned i = 0;
        while (i < 32 && e[i * 4] && e[i * 4] != regs.ra) ++i;
        if (i == 32) return;
        e[i * 4] = regs.ra; ++e[i * 4 + 1];
        e[i * 4 + 2] = Read<uint32_t>(regs.t1 + 4); e[i * 4 + 3] = Read<uint32_t>(regs.t1 + 8);
    });
    probes[6] = safetymips::create_mid(0x363340, [](SafetyMipsContext& regs) {
        fmvProbe[1] = regs.a0; fmvProbe[2] = regs.a1; fmvProbe[3] = regs.a2; fmvProbe[4] = regs.a3; ++fmvProbe[5];
    });
    probes[4] = safetymips::create_mid(0x3605C0, [](SafetyMipsContext&) { ++calls[7]; });
    probes[5] = safetymips::create_mid(0x360818, [](SafetyMipsContext&) { ++calls[8]; });
#endif
    injector::FlushCaches();
}
extern "C" int main() { return 0; }
