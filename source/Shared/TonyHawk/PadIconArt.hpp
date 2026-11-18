#pragma once
#include <array>
#include <algorithm>
#include <cstdint>
#include <cstring>

// Controller icons drawn into the rectangles of a game font's button glyphs, and the mapping from the games'
// DirectInput binding codes (1-10 axes/POV, 11+ buttons of the bundled Xidi pad) to those icons.
// Icons: 0 A, 1 B, 2 X, 3 Y, 4 LB, 5 RB, 6 LT, 7 RT, 8 Back, 9 Start, 10 LS, 11 RS, 12-15 D-pad up/down/left/right,
// 16-19 left stick directions, 20-23 right stick directions, 24-35 diagonals (left stick, right stick, D-pad),
// 36 left stick, 37 right stick, 38 D-pad. Families: 1 Xbox, 2 PlayStation, 3 Nintendo.
namespace TonyHawkPadIcons
{
    constexpr int Count = 39;

    struct Rect
    {
        int x, y, w, h;
    };

    inline uint8_t Row(char c, int row)
    {
        static constexpr std::array<std::array<uint8_t, 7>, 26> letters{ {
            { { 14, 17, 17, 31, 17, 17, 17 } }, { { 30, 17, 17, 30, 17, 17, 30 } }, { { 14, 17, 16, 16, 16, 17, 14 } },
            { { 30, 17, 17, 17, 17, 17, 30 } }, { { 31, 16, 16, 30, 16, 16, 31 } }, { { 31, 16, 16, 30, 16, 16, 16 } },
            { { 14, 17, 16, 23, 17, 17, 15 } }, { { 17, 17, 17, 31, 17, 17, 17 } }, { { 31, 4, 4, 4, 4, 4, 31 } },
            { { 7, 2, 2, 2, 18, 18, 12 } }, { { 17, 18, 20, 24, 20, 18, 17 } }, { { 16, 16, 16, 16, 16, 16, 31 } },
            { { 17, 27, 21, 21, 17, 17, 17 } }, { { 17, 25, 21, 19, 17, 17, 17 } }, { { 14, 17, 17, 17, 17, 17, 14 } },
            { { 30, 17, 17, 30, 16, 16, 16 } }, { { 14, 17, 17, 17, 21, 18, 13 } }, { { 30, 17, 17, 30, 20, 18, 17 } },
            { { 15, 16, 16, 14, 1, 1, 30 } }, { { 31, 4, 4, 4, 4, 4, 4 } }, { { 17, 17, 17, 17, 17, 17, 14 } },
            { { 17, 17, 17, 17, 17, 10, 4 } }, { { 17, 17, 17, 21, 21, 21, 10 } }, { { 17, 17, 10, 4, 10, 17, 17 } },
            { { 17, 17, 10, 4, 4, 4, 4 } }, { { 31, 1, 2, 4, 8, 16, 31 } }
        } };
        if (c >= 'A' && c <= 'Z') return letters[c - 'A'][row];
        if (c == '1') { constexpr uint8_t rows[]{ 4, 12, 4, 4, 4, 4, 14 }; return rows[row]; }
        if (c == '2') { constexpr uint8_t rows[]{ 14, 17, 1, 2, 4, 8, 31 }; return rows[row]; }
        if (c == '3') { constexpr uint8_t rows[]{ 30, 1, 1, 14, 1, 1, 30 }; return rows[row]; }
        if (c == '+') { constexpr uint8_t rows[]{ 0, 4, 4, 31, 4, 4, 0 }; return rows[row]; }
        if (c == '-') { constexpr uint8_t rows[]{ 0, 0, 0, 31, 0, 0, 0 }; return rows[row]; }
        return 0;
    }

    // Paints one icon (ARGB) into r; the rest of the picture is left as it is.
    inline void Paint(uint32_t* pixels, int pitch, Rect r, int icon, int family)
    {
        if (r.w < 5 || r.h < 5 || icon < 0 || icon >= Count) return;
        // Wide keyboard glyphs keep a proportional symbol inside the native advance
        const bool round = icon < 4 || icon >= 12;
        const int maxWidth = round ? r.h : r.h * 3 / 2;
        if (r.w > maxWidth) { r.x += (r.w - maxWidth) / 2; r.w = maxWidth; }
        const int maxHeight = round ? r.w : r.w * 3 / 2;
        if (r.h > maxHeight) { r.y += (r.h - maxHeight) / 2; r.h = maxHeight; }
        constexpr const char* labels[24] = { "A", "B", "X", "Y", "LB", "RB", "LT", "RT", "BK", "ST", "LS", "RS",
                                             "D", "D", "D", "D", "LS", "LS", "LS", "LS", "RS", "RS", "RS", "RS" };
        constexpr const char* nintendo[] = { "B", "A", "Y", "X" };
        constexpr const char* playstation[] = { "X", "O", "S", "T" };
        constexpr const char* psExtra[] = { "L1", "R1", "L2", "R2", "SH", "OP", "L3", "R3" };
        constexpr const char* nintendoExtra[] = { "L", "R", "ZL", "ZR", "-", "+", "LS", "RS" };
        const char* label = icon == 38 ? "D" : icon == 37 ? "RS" : icon == 36 ? "LS" :
                            icon >= 32 ? "D" : icon >= 28 ? "RS" : icon >= 24 ? "LS" :
                            icon < 4 && family == 2 ? playstation[icon] :
                            icon < 4 && family == 3 ? nintendo[icon] :
                            icon >= 4 && icon < 12 && family == 2 ? psExtra[icon - 4] :
                            icon >= 4 && icon < 12 && family == 3 ? nintendoExtra[icon - 4] : labels[icon];
        constexpr uint32_t xbox[] = { 0xff247e3d, 0xffb1262e, 0xff2368a9, 0xffa78613 };
        constexpr uint32_t ps[] = { 0xff2765aa, 0xffad333e, 0xff9d4d91, 0xff348b73 };
        const uint32_t fill = icon < 4 ? (family == 2 ? ps[icon] : family == 3 ? 0xff55585c : xbox[icon]) : 0xff3d4146;
        const int n = static_cast<int>(std::strlen(label));
        const int textWidth = n * 5 + n - 1;
        const int scale = std::min(std::max(1, (r.w - 4) / textWidth), std::max(1, (r.h - 4) / 7));
        const int ox = r.x + (r.w - textWidth * scale) / 2;
        const int oy = r.y + (r.h - 7 * scale) / 2;
        for (int y = 0; y < r.h; ++y)
        {
            for (int x = 0; x < r.w; ++x)
            {
                const float nx = (2.0f * x + 1 - r.w) / r.w, ny = (2.0f * y + 1 - r.h) / r.h;
                const float d = nx * nx + ny * ny;
                const bool inside = round ? d <= 0.93f : x > 0 && y > 0 && x < r.w - 1 && y < r.h - 1;
                if (!inside) continue;
                const bool edge = round ? d > 0.70f : x == 1 || y == 1 || x == r.w - 2 || y == r.h - 2;
                pixels[(r.y + y) * pitch + r.x + x] = edge ? 0xffe0e0e0 : fill;
            }
        }
        const bool shapes = family == 2 && icon < 4;
        for (int i = 0; i < n && !shapes; ++i)
        {
            for (int y = 0; y < 7; ++y)
            {
                for (int x = 0; x < 5; ++x)
                {
                    if (!((Row(label[i], y) >> (4 - x)) & 1)) continue;
                    for (int dy = 0; dy < scale; ++dy)
                    {
                        for (int dx = 0; dx < scale; ++dx)
                        {
                            const int px = ox + (i * 6 + x) * scale + dx, py = oy + y * scale + dy;
                            if (px >= r.x && px < r.x + r.w && py >= r.y && py < r.y + r.h)
                                pixels[py * pitch + px] = 0xffffffff;
                        }
                    }
                }
            }
        }
        if (shapes)
        {
            for (int y = 0; y < r.h; ++y)
            {
                for (int x = 0; x < r.w; ++x)
                {
                    const float nx = (2.0f * x + 1 - r.w) / r.w, ny = (2.0f * y + 1 - r.h) / r.h;
                    const float ax = nx < 0 ? -nx : nx, ay = ny < 0 ? -ny : ny;
                    bool mark = false;
                    if (icon == 0) mark = ax < 0.46f && ay < 0.46f && ((nx - ny < 0.12f && nx - ny > -0.12f) || (nx + ny < 0.12f && nx + ny > -0.12f));
                    if (icon == 1) { const float d = nx * nx + ny * ny; mark = d > 0.10f && d < 0.23f; }
                    if (icon == 2) mark = ax < 0.43f && ay < 0.43f && (ax > 0.28f || ay > 0.28f);
                    if (icon == 3) mark = ny > -0.42f && ny < 0.4f && ((ny > 0.25f && ax < 0.43f) || (ax > (ny + 0.42f) * 0.47f - 0.07f && ax < (ny + 0.42f) * 0.47f + 0.07f));
                    if (mark) pixels[(r.y + y) * pitch + r.x + x] = 0xffffffff;
                }
            }
        }
        if (icon >= 24 && icon < 36)
        {
            const int direction = (icon - 24) % 4;
            const bool right = (direction & 1) != 0, down = (direction & 2) != 0;
            for (int k = 0; k < 3; ++k)
            {
                const int px = right ? r.x + r.w - 2 - k : r.x + 1 + k;
                const int py = down ? r.y + r.h - 2 - k : r.y + 1 + k;
                pixels[py * pitch + px] = 0xffffffff;
            }
        }
        else if (icon >= 12 && icon < 24)
        {
            const int direction = (icon - 12) % 4;
            const int cx = r.x + r.w / 2, cy = r.y + r.h / 2;
            for (int k = 0; k < 3; ++k)
            {
                const int px = direction == 2 ? r.x + 1 + k : direction == 3 ? r.x + r.w - 2 - k : cx;
                const int py = direction == 0 ? r.y + 1 + k : direction == 1 ? r.y + r.h - 2 - k : cy;
                pixels[py * pitch + px] = 0xffffffff;
            }
            if (direction < 2)
            {
                const int py = direction == 0 ? r.y + 3 : r.y + r.h - 4;
                pixels[py * pitch + cx - 1] = pixels[py * pitch + cx + 1] = 0xffffffff;
            }
            else
            {
                const int px = direction == 2 ? r.x + 3 : r.x + r.w - 4;
                pixels[(cy - 1) * pitch + px] = pixels[(cy + 1) * pitch + px] = 0xffffffff;
            }
        }
    }

    // Icon for one bound action. Button codes 11+ are the pad's buttons (A, B, X, Y, LB, RB, LT, RT, Back, Start,
    // LS, RS); directional actions (negative = the even one of a pair) show the stick or D-pad they are bound to.
    inline int Icon(uint32_t code, bool directional, bool negative)
    {
        if (code >= 11 && code <= 22) return static_cast<int>(code - 11);
        if (!directional) return -1;
        int direction = -1;
        if (code == 1 || code == 3 || code == 4) direction = negative ? 2 : 3;
        if (code == 2 || code == 8 || code == 5) direction = negative ? 0 : 1;
        if (direction < 0) return -1;
        return (code == 4 || code == 5 ? 12 : code == 1 || code == 2 ? 16 : 20) + direction;
    }

    // Diagonal arrow glyphs (down-right, up-left, down-left, up-right) from the up/down/left/right bindings.
    inline int Diagonal(const uint32_t* bindings, int glyph)
    {
        if (glyph < 0 || glyph > 3) return -1;
        const bool down = glyph == 0 || glyph == 2;
        const bool right = glyph == 0 || glyph == 3;
        const uint32_t vertical = bindings[down ? 1 : 0], horizontal = bindings[right ? 3 : 2];
        auto axis = [](uint32_t code) -> int
        {
            switch (code)
            {
                case 1: return 0;
                case 2: return 1;
                case 4: return 2;
                case 5: return 3;
                case 3: return 4;
                case 8: return 5;
                default: return -1;
            }
        };
        const int a = axis(vertical), b = axis(horizontal);
        if (a < 0 || b < 0 || a / 2 != b / 2 || a % 2 == b % 2) return -1;
        const int group = a / 2 == 0 ? 0 : a / 2 == 2 ? 1 : 2;
        return 24 + group * 4 + (right ? 1 : 0) + (down ? 2 : 0);
    }
}
