#include "Game.hpp"
#include <cmath>

// The disc's start-up program (memory card notice, logo video, the copyright
// screen shown while the game loads) draws 4:3 pictures over the whole screen.
// Its sprites are fitted into the 4:3 frame around the screen centre; each
// decoded video picture and the copyright picture are redrawn at their own
// shape with black bars (nothing is cut off).
namespace tcny
{
    namespace
    {
        SafetyMipsMid spriteHook, splashHook;
        safetymips::GameInline<int32_t(uintptr_t, uintptr_t, int32_t)> pictureHook;
        constexpr unsigned maxWidth = 640, maxHeight = 512;
        alignas(16) uint32_t picture[maxWidth * maxHeight];

        float Aspect()
        {
            return forcedAspect > 0 ? forcedAspect : console::ps2Aspect(PCSX2Data);
        }
        // GS window coordinate (1/16 pixel) scaled around the screen centre (2048).
        uint32_t Fit(uint32_t value, float scale)
        {
            const float centred = float(int32_t(value & 0xFFFF) - 0x8000) * scale;
            return uint32_t(0x8000 + int32_t(std::lround(centred))) & 0xFFFF;
        }
        // Textured sprite (context, x0, y0, x1, y1, u0, v0, u1, ...), GS coordinates.
        void Sprite(SafetyMipsContext& regs)
        {
            const float aspect = Aspect();
            const float x = std::min(1.0f, (4.0f / 3.0f) / aspect), y = std::min(1.0f, aspect / (4.0f / 3.0f));
            if (x == 1.0f && y == 1.0f)
            {
                return;
            }
            regs.a1 = Fit(regs.a1, x);
            regs.a3 = Fit(regs.a3, x);
            regs.a2 = Fit(regs.a2, y);
            regs.a4 = Fit(regs.a4, y);
        }
        // Copyright picture (header: +2 width, +4 height, +12 pixels), 16-bit
        // pixels in rows, copied straight into the frame buffer.
        void Splash(SafetyMipsContext& regs)
        {
            const auto header = uintptr_t(regs.s0);
            const unsigned width = Read<uint16_t>(header + 2), height = Read<uint16_t>(header + 4);
            auto* image = reinterpret_cast<uint16_t*>(Read<uintptr_t>(header + 12));
            const float aspect = Aspect();
            if (!image || !width || !height || width * height * 2 > sizeof(picture) || !std::isfinite(aspect) ||
                std::fabs(aspect - 4.0f / 3.0f) < 0.0001f)
            {
                return;
            }
            const float scaleX = std::min(1.0f, (4.0f / 3.0f) / aspect), scaleY = std::min(1.0f, aspect / (4.0f / 3.0f));
            const unsigned outWidth = std::max(1u, unsigned(float(width) * scaleX + 0.5f));
            const unsigned outHeight = std::max(1u, unsigned(float(height) * scaleY + 0.5f));
            const unsigned left = (width - outWidth) / 2, top = (height - outHeight) / 2;
            auto* copy = reinterpret_cast<uint16_t*>(picture);
            std::memcpy(copy, image, width * height * 2);
            for (unsigned y = 0; y < height; ++y)
            {
                const bool row = y >= top && y < top + outHeight;
                const unsigned sy = row ? ((2 * (y - top) + 1) * height) / (2 * outHeight) : 0;
                for (unsigned x = 0; x < width; ++x)
                {
                    uint16_t color = 0;
                    if (row && x >= left && x < left + outWidth)
                    {
                        color = copy[sy * width + ((2 * (x - left) + 1) * width) / (2 * outWidth)];
                    }
                    image[y * width + x] = color;
                }
            }
        }
        // Decoded RGBA picture in 16x16 macroblocks, uploaded column by column
        // (top to bottom, then the next column).
        uint32_t& Pixel(uint32_t* image, unsigned height, unsigned x, unsigned y)
        {
            return image[((x >> 4) * (height >> 4) + (y >> 4)) * 256 + (y & 15) * 16 + (x & 15)];
        }
        int32_t Picture(uintptr_t mpeg, uintptr_t buffer, int32_t count)
        {
            const auto result = pictureHook.call(mpeg, buffer, count);
            const unsigned width = Read<uint32_t>(mpeg), height = Read<uint32_t>(mpeg + 4);
            const float aspect = Aspect();
            if (result < 0 || !buffer || !width || !height || width > maxWidth || height > maxHeight || (width & 15) ||
                (height & 15) || !std::isfinite(aspect) || std::fabs(aspect - 4.0f / 3.0f) < 0.0001f)
            {
                return result;
            }
            // The picture is a 4:3 screen stretched over the whole display; it is
            // shrunk to 4:3 again with black bars, as large as it fits.
            const float scaleX = std::min(1.0f, (4.0f / 3.0f) / aspect), scaleY = std::min(1.0f, aspect / (4.0f / 3.0f));
            const unsigned outWidth = std::max(1u, unsigned(float(width) * scaleX + 0.5f));
            const unsigned outHeight = std::max(1u, unsigned(float(height) * scaleY + 0.5f));
            const unsigned left = (width - outWidth) / 2, top = (height - outHeight) / 2;
            auto* image = reinterpret_cast<uint32_t*>(buffer);
            std::memcpy(picture, image, width * height * sizeof(uint32_t));
            for (unsigned y = 0; y < height; ++y)
            {
                const bool row = y >= top && y < top + outHeight;
                const unsigned sy = row ? ((2 * (y - top) + 1) * height) / (2 * outHeight) : 0;
                for (unsigned x = 0; x < width; ++x)
                {
                    uint32_t color = 0x80000000;
                    if (row && x >= left && x < left + outWidth)
                    {
                        color = Pixel(picture, height, ((2 * (x - left) + 1) * width) / (2 * outWidth), sy);
                    }
                    Pixel(image, height, x, y) = color;
                }
            }
            return result;
        }
    } // namespace

    void InstallLauncher(uintptr_t sprite, uintptr_t picture, uintptr_t splash)
    {
        splashHook = safetymips::create_mid<&Splash>(splash + 8);
        spriteHook = safetymips::create_mid<&Sprite>(sprite);
        pictureHook = safetymips::create_inline_game(picture, Picture);
    }
} // namespace tcny
