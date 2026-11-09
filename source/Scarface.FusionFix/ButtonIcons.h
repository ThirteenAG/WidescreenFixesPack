#pragma once
#include <d3d9.h>
#include <wrl/client.h>
#include <array>

namespace ScarfaceButtons
{
    inline unsigned Style(int choice)
    {
        constexpr std::array<unsigned, 9> styles{ 0, 0, 4, 5, 1, 2, 3, 6, 7 };
        return choice >= 0 && size_t(choice) < styles.size() ? styles[choice] : 0;
    }

    // Physical element order shared with XidiGetPhysicalButtonMask.
    // Style order matches Max Payne 3's GamepadIcons option.
    inline constexpr const char* Names[][20] = {
        {"Left Stick", "Left Stick", "Right Stick", "Right Stick",
         "D-Pad Up", "D-Pad Down", "D-Pad Left", "D-Pad Right",
         "LT", "RT", "A", "B", "X", "Y", "LB", "RB", "Back", "Start", "LS", "RS"},
        {"Left Stick", "Left Stick", "Right Stick", "Right Stick",
         "D-Pad Up", "D-Pad Down", "D-Pad Left", "D-Pad Right",
         "LT", "RT", "A", "B", "X", "Y", "LB", "RB", "View", "Menu", "LS", "RS"},
        {"Left Stick", "Left Stick", "Right Stick", "Right Stick",
         "D-Pad Up", "D-Pad Down", "D-Pad Left", "D-Pad Right",
         "L2", "R2", "Cross", "Circle", "Square", "Triangle", "L1", "R1", "Select", "Start", "L3", "R3"},
        {"Left Stick", "Left Stick", "Right Stick", "Right Stick",
         "D-Pad Up", "D-Pad Down", "D-Pad Left", "D-Pad Right",
         "L2", "R2", "Cross", "Circle", "Square", "Triangle", "L1", "R1", "Touchpad", "Options", "L3", "R3"},
        {"Left Stick", "Left Stick", "Right Stick", "Right Stick",
         "D-Pad Up", "D-Pad Down", "D-Pad Left", "D-Pad Right",
         "L2", "R2", "Cross", "Circle", "Square", "Triangle", "L1", "R1", "Touchpad", "Options", "L3", "R3"},
        {"Left Stick", "Left Stick", "Right Stick", "Right Stick",
         "D-Pad Up", "D-Pad Down", "D-Pad Left", "D-Pad Right",
         "ZL", "ZR", "B", "A", "Y", "X", "L", "R", "Minus", "Plus", "LS", "RS"},
        {"Left Stick", "Left Stick", "Right Stick", "Right Stick",
         "D-Pad Up", "D-Pad Down", "D-Pad Left", "D-Pad Right",
         "L2", "R2", "A", "B", "X", "Y", "L1", "R1", "View", "Menu", "L3", "R3"},
        {"Left Stick", "Left Stick", "Right Trackpad", "Right Trackpad",
         "D-Pad Up", "D-Pad Down", "D-Pad Left", "D-Pad Right",
         "LT", "RT", "A", "B", "X", "Y", "LB", "RB", "Back", "Start", "LS", "Right Trackpad Click"}
    };

    // One 4x4 DXT5 block to 32-bit BGRA texels, row by row
    inline void DecodeDxt5Block(const uint8_t* block, uint32_t out[16])
    {
        uint8_t alpha[8]{ block[0], block[1] };
        for (int i = 2; i < 8; ++i)
        {
            alpha[i] = alpha[0] > alpha[1]
                ? uint8_t(((8 - i) * alpha[0] + (i - 1) * alpha[1]) / 7)
                : i < 6 ? uint8_t(((6 - i) * alpha[0] + (i - 1) * alpha[1]) / 5) : uint8_t(i == 6 ? 0 : 255);
        }
        uint64_t alphaBits = 0;
        for (int i = 0; i < 6; ++i)
            alphaBits |= uint64_t(block[2 + i]) << (8 * i);

        auto expand = [](uint16_t c, uint32_t channel[3])
        {
            channel[0] = ((c >> 11) & 31) * 255 / 31;
            channel[1] = ((c >> 5) & 63) * 255 / 63;
            channel[2] = (c & 31) * 255 / 31;
        };
        uint16_t c0 = uint16_t(block[8] | block[9] << 8);
        uint16_t c1 = uint16_t(block[10] | block[11] << 8);
        uint32_t colors[4][3];
        expand(c0, colors[0]);
        expand(c1, colors[1]);
        for (int k = 0; k < 3; ++k)
        {
            colors[2][k] = (2 * colors[0][k] + colors[1][k]) / 3;
            colors[3][k] = (colors[0][k] + 2 * colors[1][k]) / 3;
        }
        uint32_t colorBits = uint32_t(block[12] | block[13] << 8 | block[14] << 16 | block[15] << 24);

        for (int i = 0; i < 16; ++i)
        {
            auto& c = colors[(colorBits >> (2 * i)) & 3];
            uint32_t a = alpha[(alphaBits >> (3 * i)) & 7];
            out[i] = a << 24 | c[0] << 16 | c[1] << 8 | c[2];
        }
    }

    inline Microsoft::WRL::ComPtr<IDirect3DTexture9> CreateIcon(IDirect3DDevice9* device, unsigned id, unsigned style)
    {
        // DDS assets embedded unchanged from Max Payne 3 Fusion Fix's buttons_pc.wtd (DXT5).
        constexpr unsigned resources[] =
        {
            0, 0, 1, 1, 2, 3, 4, 5, 6, 7,
            8, 9, 10, 11, 12, 13, 14, 15, 16, 17
        };
        if (id >= std::size(resources) || style >= std::size(Names))
            return {};

        HMODULE module = nullptr;
        if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&CreateIcon), &module))
            return {};
        HRSRC info = FindResourceW(module, MAKEINTRESOURCEW(4100 + style * 100 + resources[id]), MAKEINTRESOURCEW(10));
        HGLOBAL handle = info ? LoadResource(module, info) : nullptr;
        auto data = handle ? static_cast<const uint8_t*>(LockResource(handle)) : nullptr;
        DWORD size = info ? SizeofResource(module, info) : 0;
        if (!data || size < 128 || memcmp(data, "DDS ", 4) != 0 || memcmp(data + 84, "DXT5", 4) != 0)
            return {};

        uint32_t height = *reinterpret_cast<const uint32_t*>(data + 12);
        uint32_t width = *reinterpret_cast<const uint32_t*>(data + 16);
        uint32_t blocksX = (width + 3) / 4;
        uint32_t blocksY = (height + 3) / 4;
        if (!width || !height || size < 128 + blocksX * blocksY * 16)
            return {};

        Microsoft::WRL::ComPtr<IDirect3DTexture9> texture;
        if (FAILED(device->CreateTexture(width, height, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, texture.GetAddressOf(), nullptr)))
            return {};
        D3DLOCKED_RECT locked{};
        if (FAILED(texture->LockRect(0, &locked, nullptr, 0)))
            return {};

        // Scarface's native font glyph UVs run bottom-to-top, so rows are written flipped
        auto pixels = static_cast<uint8_t*>(locked.pBits);
        auto blocks = data + 128;
        for (uint32_t by = 0; by < blocksY; ++by)
        {
            for (uint32_t bx = 0; bx < blocksX; ++bx)
            {
                uint32_t texels[16];
                DecodeDxt5Block(blocks + (by * blocksX + bx) * 16, texels);
                for (uint32_t y = 0; y < 4 && by * 4 + y < height; ++y)
                {
                    auto row = reinterpret_cast<uint32_t*>(pixels + (height - 1 - (by * 4 + y)) * locked.Pitch);
                    for (uint32_t x = 0; x < 4 && bx * 4 + x < width; ++x)
                        row[bx * 4 + x] = texels[y * 4 + x];
                }
            }
        }
        texture->UnlockRect(0);
        return texture;
    }
}
