#pragma once
#include "Game.hpp"

// GS register writes added to NiPS2Renderer's command packets. The main chain
// (renderer + 0x14) wraps GIF data in a VIF DIRECT block; the upload packet
// (renderer + 0x44) takes GIF data directly.
namespace kr::draw
{
    inline pcsx2::GameFunction<void(uintptr_t, uint32_t)> packetBegin; // 0x36B370: reserve n qwords after a DMA cnt tag
    inline pcsx2::GameFunction<void(uintptr_t)> packetEnd;             // 0x36B260: close the cnt tag
    inline pcsx2::GameFunction<void(uintptr_t)> directBegin;           // 0x36ACE0: reserve VIF DIRECT + GIFtag
    inline pcsx2::GameFunction<void(uintptr_t, uint32_t)> directEnd;   // 0x36AC60: fill VIF DIRECT + GIFtag (A+D)

    inline bool Bind()
    {
        return packetBegin.bind(0x36B370) == PCSX2_HOOK_OK && packetEnd.bind(0x36B260) == PCSX2_HOOK_OK &&
               directBegin.bind(0x36ACE0) == PCSX2_HOOK_OK && directEnd.bind(0x36AC60) == PCSX2_HOOK_OK;
    }

    struct Writer
    {
        uint64_t* q;
        void Put(uint64_t data, uint64_t reg)
        {
            q[0] = data;
            q[1] = reg;
            q += 2;
        }
    };

    struct Box
    {
        uint32_t left, top, right, bottom; // frame buffer pixels, right/bottom exclusive
    };

    // Black, untextured sprites through GS context 2 (the renderer sets
    // context 2 itself before each of its own uses), without depth writes.
    inline void Black(uintptr_t renderer, const Box* boxes, unsigned count)
    {
        if (!count || !packetBegin)
        {
            return;
        }
        const auto packet = renderer + 0x14;
        const uint32_t width = Read<uint32_t>(renderer + 0x828), height = Read<uint32_t>(renderer + 0x82C);
        const uint64_t fbw = Read<uint32_t>(renderer + 0x998), psm = Read<uint32_t>(renderer + 0x99C);
        const uint64_t fbp = Read<uint32_t>(renderer + 0x990 + 4 * Read<uint32_t>(renderer + 0x9A0));
        const uint64_t zbp = Read<uint32_t>(renderer + 0x2B8), zpsm = Read<uint32_t>(renderer + 0x2BC);
        const uint32_t ofx = Read<uint32_t>(renderer + 0x9A4), ofy = Read<uint32_t>(renderer + 0x9A8);
        if (!width || !height || width > 2048 || height > 2048)
        {
            return;
        }
        const unsigned words = 9 + 2 * count;
        packetBegin(packet, words + 2);
        directBegin(packet);
        Writer w{reinterpret_cast<uint64_t*>(Read<uintptr_t>(packet + 0x14))};
        auto xy = [&](uint32_t x, uint32_t y) { return uint64_t((ofx + x) * 16) | uint64_t((ofy + y) * 16) << 16; };
        w.Put(fbp | fbw << 16 | psm << 24, 0x4D);                                      // FRAME_2
        w.Put(zbp | zpsm << 24 | 1ull << 32, 0x4F);                                    // ZBUF_2, masked
        w.Put(0x30000, 0x48);                                                          // TEST_2: depth always
        w.Put(0, 0x43);                                                                // ALPHA_2
        w.Put(0, 0x4B);                                                                // FBA_2
        w.Put(uint64_t(width - 1) << 16 | uint64_t(height - 1) << 48, 0x41);           // SCISSOR_2
        w.Put(uint64_t(ofx * 16) | uint64_t(ofy * 16) << 32, 0x19);                    // XYOFFSET_2
        w.Put(0x3F80000080000000ull, 0x01);                                            // RGBAQ: black
        w.Put(0x206, 0x00);                                                            // PRIM: sprite, context 2
        for (unsigned i = 0; i < count; ++i)
        {
            w.Put(xy(boxes[i].left, boxes[i].top), 0x05);
            w.Put(xy(boxes[i].right, boxes[i].bottom), 0x05);
        }
        Write<uintptr_t>(packet + 0x14, uintptr_t(w.q));
        Write<uint32_t>(packet + 0x10, Read<uint32_t>(packet + 0x10) - words);
        directEnd(packet, 0x11000000); // FLUSH
        packetEnd(packet);
    }

    // Black outside the centered frame of the given relative size, drawn before
    // the frame's own picture. The bars reach one pixel into the frame, so the
    // edge column the frame's scissor leaves out stays black too.
    inline void Outside(uintptr_t renderer, float frameWidth, float frameHeight)
    {
        const uint32_t width = Read<uint32_t>(renderer + 0x828), height = Read<uint32_t>(renderer + 0x82C);
        auto bar = [](uint32_t size, float frame)
        {
            const auto outside = uint32_t(std::lround(float(size) * (1 - frame) * 0.5f));
            return outside ? std::min(outside + 1, size / 2) : 0;
        };
        const uint32_t side = bar(width, frameWidth), band = bar(height, frameHeight);
        Box boxes[4];
        unsigned count = 0;
        if (side)
        {
            boxes[count++] = {0, 0, side, height};
            boxes[count++] = {width - side, 0, width, height};
        }
        if (band)
        {
            boxes[count++] = {0, 0, width, band};
            boxes[count++] = {0, height - band, width, height};
        }
        Black(renderer, boxes, count);
    }
} // namespace kr::draw
