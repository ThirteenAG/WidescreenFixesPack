#include "Draw.hpp"

namespace kr::movie
{
namespace
{
SafetyMipsMid movieUpload, movieDraw, videoCreate, videoUpdate;

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
    if ((!fmvFill && Fit() == 1 && VerticalFit() == 1) || !draw::packetBegin)
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
    // Destination in frame pixels and source rows (1/16 texel). The story videos have black bars baked
    // into rows 76-436 of their 4:3 frame; Fill shows the picture between them as large as the screen allows.
    float frameWidth = Fit(), frameHeight = VerticalFit();
    uint32_t v0 = 0, v1 = height * 16;
    if (fmvFill)
    {
        constexpr float top = 76.0f / 512.0f, bottom = 436.0f / 512.0f;
        const float picture = (4.0f / 3.0f) / (bottom - top), aspect = Aspect();
        frameWidth = std::min(1.0f, picture / aspect);
        frameHeight = std::min(1.0f, aspect / picture);
        v0 = uint32_t(float(height * 16) * top + 0.5f);
        v1 = uint32_t(float(height * 16) * bottom + 0.5f);
    }
    const uint32_t x0 = uint32_t(float(width) * (1 - frameWidth) * 0.5f + 0.5f), x1 = width - x0;
    const uint32_t y0 = uint32_t(float(height) * (1 - frameHeight) * 0.5f + 0.5f), y1 = height - y0;
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
} // namespace

void Install()
{
    if (draw::Bind())
    {
        movieUpload = safetymips::create_mid<&MovieUpload>(0x3633B0);
        movieDraw = safetymips::create_mid<&MovieDraw>(0x36AA50);
    }
    videoCreate = safetymips::create_mid<&VideoCreate>(0x255B20);
    videoUpdate = safetymips::create_mid<&VideoUpdate>(0x2557B0);
}
} // namespace kr::movie
