module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <d3dx9.h>
#include <array>
#include <mutex>
#include <vector>

export module ButtonIcons;

#pragma comment(lib, "d3dx9.lib")

// MAIN.GamepadIcons: the controller buttons in two textures: the prompts inside texts (etc\message\mes_tex) and the
// big action buttons of the cockpit (act_btn). PlayStation and Nintendo redraw the face buttons, shoulder buttons,
// triggers and Back/Start of both in place; the keyboard and mouse keys of the same textures are kept. Auto and Xbox
// keep the game's art.

namespace
{
    enum Button { A, B, X, Y, LB, RB, LT, RT, Back, Start, Count };

    struct Icon
    {
        Button button;
        RECT box;   // where the game's button is drawn, in the texture's full size
    };

    struct Group
    {
        RECT area;  // cleared and redrawn together (whole 4x4 blocks for compressed textures)
        std::vector<Icon> icons;
    };

    struct Atlas
    {
        const char* name;
        UINT width, height;
        std::vector<Group> groups;
        uintptr_t resource = 0;
        IDirect3DTexture9* texture = nullptr;
        std::vector<std::vector<uint8_t>> original;
        int applied = 0;
        bool failed = false;
    };

    RECT Box(LONG x, LONG y, LONG w, LONG h)
    {
        return { x, y, x + w, y + h };
    }

    std::array<Atlas, 2> Atlases{ {
        {
            "etc\\message\\mes_tex", 1024, 256,
            {
                { Box(0, 72, 160, 40), { { A, Box(2, 73, 37, 38) }, { B, Box(41, 73, 38, 38) }, { X, Box(82, 73, 38, 38) }, { Y, Box(121, 73, 38, 38) } } },
                { Box(2, 120, 76, 48), { { LT, Box(6, 122, 28, 43) }, { RT, Box(45, 123, 28, 42) } } },
                { Box(80, 122, 96, 32), { { LB, Box(82, 125, 43, 26) }, { RB, Box(131, 125, 43, 26) } } },
                { Box(178, 124, 78, 44), { { Back, Box(180, 126, 34, 40) }, { Start, Box(219, 126, 35, 40) } } },
            },
        },
        {
            "id\\jpn\\cockpit\\act_btn_360_NOMIP_BM", 768, 512,
            {
                { Box(0, 148, 256, 108), { { A, Box(0, 150, 60, 60) }, { X, Box(61, 150, 60, 60) }, { Y, Box(122, 150, 60, 60) }, { B, Box(183, 150, 60, 60) },
                    { LB, Box(1, 211, 100, 41) }, { RB, Box(109, 211, 100, 41) } } },
                { Box(256, 192, 96, 64), { { LT, Box(259, 194, 43, 61) }, { RT, Box(307, 194, 43, 61) } } },
            },
        },
    } };

    struct Picture
    {
        UINT width = 0, height = 0;
        std::vector<uint32_t> pixels;   // A8R8G8B8, cropped to the visible part
    };

    std::array<std::array<Picture, Count>, 2> Pictures;   // PlayStation, Nintendo
    std::mutex Mutex;
    HMODULE Module = nullptr;

    // Resource ids: 1000 + family * 20 + button
    bool LoadPicture(IDirect3DDevice9* device, int family, Button button, Picture& picture)
    {
        IDirect3DTexture9* texture = nullptr;
        if (FAILED(D3DXCreateTextureFromResourceExW(device, Module, MAKEINTRESOURCEW(1000 + family * 20 + button), D3DX_DEFAULT_NONPOW2,
            D3DX_DEFAULT_NONPOW2, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_SCRATCH, D3DX_FILTER_NONE, D3DX_FILTER_NONE, 0, nullptr, nullptr, &texture)))
            return false;
        D3DSURFACE_DESC desc{};
        D3DLOCKED_RECT lock{};
        bool ok = SUCCEEDED(texture->GetLevelDesc(0, &desc)) && SUCCEEDED(texture->LockRect(0, &lock, nullptr, D3DLOCK_READONLY));
        if (ok)
        {
            UINT left = desc.Width, top = desc.Height, right = 0, bottom = 0;
            for (UINT y = 0; y < desc.Height; ++y)
            {
                auto row = reinterpret_cast<const uint32_t*>(static_cast<const uint8_t*>(lock.pBits) + y * lock.Pitch);
                for (UINT x = 0; x < desc.Width; ++x)
                {
                    if ((row[x] >> 24) > 8)
                    {
                        left = std::min(left, x);
                        top = std::min(top, y);
                        right = std::max(right, x + 1);
                        bottom = std::max(bottom, y + 1);
                    }
                }
            }
            ok = right > left && bottom > top;
            if (ok)
            {
                picture.width = right - left;
                picture.height = bottom - top;
                picture.pixels.resize(picture.width * picture.height);
                for (UINT y = 0; y < picture.height; ++y)
                {
                    auto row = reinterpret_cast<const uint32_t*>(static_cast<const uint8_t*>(lock.pBits) + (top + y) * lock.Pitch);
                    std::memcpy(&picture.pixels[y * picture.width], row + left, picture.width * 4);
                }
            }
            texture->UnlockRect(0);
        }
        texture->Release();
        return ok;
    }

    // Bilinear sample of a picture at (u, v) in its pixels; channels premultiplied for the blend
    void Sample(const Picture& picture, float u, float v, float* out)
    {
        u = std::clamp(u - 0.5f, 0.0f, float(picture.width - 1));
        v = std::clamp(v - 0.5f, 0.0f, float(picture.height - 1));
        const UINT x0 = UINT(u), y0 = UINT(v);
        const UINT x1 = std::min(x0 + 1, picture.width - 1), y1 = std::min(y0 + 1, picture.height - 1);
        const float fx = u - float(x0), fy = v - float(y0);
        const uint32_t texels[4] = { picture.pixels[y0 * picture.width + x0], picture.pixels[y0 * picture.width + x1],
            picture.pixels[y1 * picture.width + x0], picture.pixels[y1 * picture.width + x1] };
        const float weights[4] = { (1 - fx) * (1 - fy), fx * (1 - fy), (1 - fx) * fy, fx * fy };
        for (int c = 0; c < 4; ++c)
            out[c] = 0.0f;
        for (int i = 0; i < 4; ++i)
        {
            const float alpha = float(texels[i] >> 24) / 255.0f;
            out[0] += weights[i] * alpha;
            for (int c = 0; c < 3; ++c)
                out[c + 1] += weights[i] * alpha * float((texels[i] >> (16 - c * 8)) & 255);
        }
    }

    // The group's area at the texture's full size: transparent, each button's picture fitted into its box
    std::vector<uint32_t> Compose(const Group& group, int family)
    {
        const LONG width = group.area.right - group.area.left, height = group.area.bottom - group.area.top;
        std::vector<uint32_t> pixels(width * height, 0);
        for (const auto& icon : group.icons)
        {
            const auto& picture = Pictures[family][icon.button];
            const float boxW = float(icon.box.right - icon.box.left), boxH = float(icon.box.bottom - icon.box.top);
            const float scale = std::min(boxW / float(picture.width), boxH / float(picture.height));
            const float w = float(picture.width) * scale, h = float(picture.height) * scale;
            const float left = float(icon.box.left - group.area.left) + (boxW - w) / 2.0f;
            const float top = float(icon.box.top - group.area.top) + (boxH - h) / 2.0f;
            for (LONG y = std::max(0L, LONG(top)); y < std::min(height, LONG(std::ceil(top + h))); ++y)
            {
                for (LONG x = std::max(0L, LONG(left)); x < std::min(width, LONG(std::ceil(left + w))); ++x)
                {
                    float texel[4];
                    Sample(picture, (float(x) + 0.5f - left) / scale, (float(y) + 0.5f - top) / scale, texel);
                    // Coverage of the pixel by the picture's edges
                    const float cover = std::clamp(std::min(float(x) + 1.0f, left + w) - std::max(float(x), left), 0.0f, 1.0f) *
                        std::clamp(std::min(float(y) + 1.0f, top + h) - std::max(float(y), top), 0.0f, 1.0f);
                    const float alpha = texel[0] * cover;
                    if (alpha <= 0.0f)
                        continue;
                    uint32_t value = uint32_t(std::lround(alpha * 255.0f)) << 24;
                    for (int c = 0; c < 3; ++c)
                        value |= uint32_t(std::clamp(std::lround(texel[c + 1] / texel[0]), 0L, 255L)) << (16 - c * 8);
                    pixels[y * width + x] = value;
                }
            }
        }
        return pixels;
    }

    bool CopyLevels(Atlas& atlas, bool save)
    {
        if (save)
            atlas.original.assign(atlas.texture->GetLevelCount(), {});
        for (DWORD level = 0; level < atlas.original.size(); ++level)
        {
            D3DSURFACE_DESC desc{};
            D3DLOCKED_RECT lock{};
            if (FAILED(atlas.texture->GetLevelDesc(level, &desc)) || FAILED(atlas.texture->LockRect(level, &lock, nullptr, save ? D3DLOCK_READONLY : 0)))
                return false;
            const bool compressed = desc.Format == D3DFMT_DXT1 || desc.Format == D3DFMT_DXT3 || desc.Format == D3DFMT_DXT5;
            const UINT rows = compressed ? (desc.Height + 3) / 4 : desc.Height;
            auto& bytes = atlas.original[level];
            if (save)
                bytes.resize(size_t(rows) * lock.Pitch);
            for (UINT row = 0; row < rows; ++row)
            {
                auto line = static_cast<uint8_t*>(lock.pBits) + row * lock.Pitch;
                if (save)
                    std::memcpy(bytes.data() + row * lock.Pitch, line, lock.Pitch);
                else
                    std::memcpy(line, bytes.data() + row * lock.Pitch, lock.Pitch);
            }
            atlas.texture->UnlockRect(level);
        }
        return true;
    }

    bool Draw(Atlas& atlas, int family)
    {
        for (DWORD level = 0; level < atlas.original.size(); ++level)
        {
            D3DSURFACE_DESC desc{};
            IDirect3DSurface9* surface = nullptr;
            if (FAILED(atlas.texture->GetLevelDesc(level, &desc)) || FAILED(atlas.texture->GetSurfaceLevel(level, &surface)))
                return false;
            const bool compressed = desc.Format == D3DFMT_DXT1 || desc.Format == D3DFMT_DXT3 || desc.Format == D3DFMT_DXT5;
            const float sx = float(desc.Width) / float(atlas.width), sy = float(desc.Height) / float(atlas.height);
            bool ok = true;
            for (const auto& group : atlas.groups)
            {
                const auto pixels = Compose(group, family);
                RECT target{ LONG(float(group.area.left) * sx), LONG(float(group.area.top) * sy),
                    LONG(std::ceil(float(group.area.right) * sx)), LONG(std::ceil(float(group.area.bottom) * sy)) };
                if (compressed)
                {
                    target.left &= ~3L;
                    target.top &= ~3L;
                    target.right = std::min(LONG(desc.Width), (target.right + 3) & ~3L);
                    target.bottom = std::min(LONG(desc.Height), (target.bottom + 3) & ~3L);
                }
                const RECT source{ 0, 0, group.area.right - group.area.left, group.area.bottom - group.area.top };
                if (FAILED(D3DXLoadSurfaceFromMemory(surface, nullptr, &target, pixels.data(), D3DFMT_A8R8G8B8, UINT(source.right * 4),
                    nullptr, &source, D3DX_FILTER_TRIANGLE, 0)))
                    ok = false;
            }
            surface->Release();
            if (!ok)
                return false;
        }
        return true;
    }

    void Forget(Atlas& atlas)
    {
        if (atlas.texture)
            atlas.texture->Release();
        atlas.texture = nullptr;
        atlas.original.clear();
        atlas.applied = 0;
        atlas.failed = false;
    }

    IDirect3DTexture9* NativeTexture(uintptr_t resource)
    {
        const auto wrapper = *reinterpret_cast<uintptr_t*>(resource + 0xCC);
        return wrapper ? *reinterpret_cast<IDirect3DTexture9**>(wrapper + 0x10) : nullptr;
    }

    SafetyHookInline LoadHook, ReleaseHook;

    bool __fastcall Load(uintptr_t resource, void*, void* stream)
    {
        const bool result = LoadHook.unsafe_thiscall<bool>(resource, stream);
        if (result)
        {
            const char* name = reinterpret_cast<const char*>(resource + 4);
            std::lock_guard lock(Mutex);
            for (auto& atlas : Atlases)
            {
                if (_stricmp(name, atlas.name) == 0)
                {
                    Forget(atlas);
                    atlas.resource = resource;
                }
            }
        }
        return result;
    }

    void __fastcall Release(uintptr_t resource, void*)
    {
        {
            std::lock_guard lock(Mutex);
            for (auto& atlas : Atlases)
            {
                if (atlas.resource == resource)
                {
                    Forget(atlas);
                    atlas.resource = 0;
                }
            }
        }
        ReleaseHook.unsafe_thiscall(resource);
    }
}

export namespace ButtonIcons
{
    // Once per frame on the render thread
    void Frame(IDirect3DDevice9* device)
    {
        const int selected = FixSettings().Int("MAIN.GamepadIcons");
        const int family = selected == 2 || selected == 3 ? selected - 1 : 0;   // 1 PlayStation, 2 Nintendo
        std::lock_guard lock(Mutex);
        static bool pictures = false, picturesFailed = false;
        if (family && !pictures && !picturesFailed)
        {
            for (int f = 0; f < 2 && !picturesFailed; ++f)
                for (int b = 0; b < Count && !picturesFailed; ++b)
                    picturesFailed = !LoadPicture(device, f, Button(b), Pictures[f][b]);
            pictures = !picturesFailed;
        }
        for (auto& atlas : Atlases)
        {
            if (!atlas.resource)
                continue;
            const auto texture = NativeTexture(atlas.resource);
            if (texture != atlas.texture)
            {
                Forget(atlas);
                if (!texture)
                    continue;
                D3DSURFACE_DESC desc{};
                if (FAILED(texture->GetLevelDesc(0, &desc)) || desc.Pool != D3DPOOL_MANAGED)
                    continue;
                texture->AddRef();
                atlas.texture = texture;
                if (!CopyLevels(atlas, true))
                {
                    Forget(atlas);
                    continue;
                }
            }
            const int wanted = pictures ? family : 0;
            if (atlas.applied == wanted || atlas.failed)
                continue;
            if (atlas.applied && !CopyLevels(atlas, false))
                continue;
            atlas.applied = 0;
            if (wanted)
            {
                if (Draw(atlas, wanted - 1))
                    atlas.applied = wanted;
                else
                {
                    CopyLevels(atlas, false);
                    atlas.failed = true;
                }
            }
        }
    }

    void Init()
    {
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&Frame), &Module);

        // The texture resource's constructor sets its vtable: +0x20 loads it, +0x2C releases the device texture
        auto pattern = hook::pattern("C7 00 ? ? ? ? C7 40 48 10 00 00 00 89 88 CC 00 00 00");
        const auto vtable = *pattern.get_first<uintptr_t*>(2);
        LoadHook = safetyhook::create_inline(reinterpret_cast<void*>(vtable[8]), Load);
        ReleaseHook = safetyhook::create_inline(reinterpret_cast<void*>(vtable[11]), Release);
    }
}
