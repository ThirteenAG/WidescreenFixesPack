module;

#include <stdafx.h>
#include <d3d9.h>
#include "FixSettings.hpp"

export module Movie;

namespace
{
    struct Picture
    {
        const char* file;
        uint32_t width, height;
        uint32_t top, bottom;
    };

    // The rows that hold the picture; the rest of these frames is black bars, unequal above and below.
    // The ATVI and BEENOX logos have no bars and animate across the whole frame, so they stay whole.
    constexpr Picture Pictures[] =
    {
        { "GUNINTRO.bik", 640, 480, 48, 408 },
        { "NSINTRO.bik", 640, 480, 44, 404 },
        { "ATTRACT.bik", 640, 480, 64, 392 },
    };

    using Vertices = std::array<float, 28>;

    SafetyHookInline MovieHook, DrawHook;
    uint32_t** MovieHandle = nullptr;
    void*(__stdcall* NativeOpen)(const char*, uint32_t) = nullptr;
    const Picture* CurrentPicture = nullptr;
    thread_local bool DrawingMovie = false;

    void* __stdcall OpenMovie(const char* path, uint32_t flags)
    {
        const std::string_view name{ path };
        const auto file = name.data() + name.find_last_of("\\/") + 1;
        const auto picture = std::ranges::find_if(Pictures, [file](const Picture& entry) { return !_stricmp(entry.file, file); });
        CurrentPicture = picture != std::end(Pictures) ? picture : nullptr;
        return NativeOpen(path, flags);
    }

    void PlaceQuad(Vertices& v, float x, float y, float w, float h)
    {
        v[0] = v[14] = x;
        v[7] = v[21] = x + w;
        v[1] = v[8] = y;
        v[15] = v[22] = y + h;
    }

    // The whole movie at its own aspect: Original inside the game's centred 4:3 area, Fill inside the whole screen
    void FitWhole(Vertices& v, float screenWidth, float screenHeight, float videoAspect, int mode)
    {
        const float areaWidth = mode == 0 ? std::min(screenWidth, screenHeight * 4.0f / 3.0f) : screenWidth;
        const float areaHeight = mode == 0 ? std::min(screenHeight, screenWidth * 3.0f / 4.0f) : screenHeight;
        float w = areaWidth, h = areaHeight;
        if (areaWidth / areaHeight > videoAspect) w = areaHeight * videoAspect;
        else h = areaWidth / videoAspect;
        PlaceQuad(v, (screenWidth - w) * 0.5f, (screenHeight - h) * 0.5f, w, h);
    }

    // The picture without its bars, whole and at its own aspect, as large as the screen allows
    void FitPicture(Vertices& v, float screenWidth, float screenHeight, const Picture& picture)
    {
        // Half a texel in, so that filtering doesn't mix a bar into the first and last rows
        const float top = picture.top + 0.5f, bottom = picture.bottom - 0.5f;
        const float pictureWidth = static_cast<float>(picture.width), pictureHeight = bottom - top;
        const float scale = std::min(screenWidth / pictureWidth, screenHeight / pictureHeight);
        const float w = pictureWidth * scale, h = pictureHeight * scale;
        PlaceQuad(v, (screenWidth - w) * 0.5f, (screenHeight - h) * 0.5f, w, h);
        v[6] = v[13] = top / picture.height;
        v[20] = v[27] = bottom / picture.height;
    }

    // The movie player clears the screen and draws one pre-transformed strip with the full video texture.
    HRESULT WINAPI DrawUP(IDirect3DDevice9* device, D3DPRIMITIVETYPE type, UINT count, const void* data, UINT stride)
    {
        const auto video = MovieHandle ? *MovieHandle : nullptr;
        if (!DrawingMovie || type != D3DPT_TRIANGLESTRIP || count != 2 || stride != 28 || !data || !video || !video[0] || !video[1])
            return DrawHook.stdcall<HRESULT>(device, type, count, data, stride);
        Vertices v{};
        std::memcpy(v.data(), data, sizeof(v));
        const float width = v[7], height = v[15];
        if (width <= 0.0f || height <= 0.0f)
            return DrawHook.stdcall<HRESULT>(device, type, count, data, stride);
        static const auto& mode = FixSettings().Int("MAIN.FMVWidescreenMode");
        const auto picture = CurrentPicture;
        if (mode != 0 && picture && video[0] == picture->width && video[1] == picture->height)
            FitPicture(v, width, height, *picture);
        else
            FitWhole(v, width, height, static_cast<float>(video[0]) / static_cast<float>(video[1]), mode);
        return DrawHook.stdcall<HRESULT>(device, type, count, v.data(), stride);
    }

    void __cdecl DrawMovie(void* texture, float x, float y, float width, float height, float alpha)
    {
        DrawingMovie = true;
        MovieHook.ccall<void>(texture, x, y, width, height, alpha);
        DrawingMovie = false;
    }
}

export namespace GunMovie
{
    void InstallDraw(void* drawPrimitiveUP)
    {
        DrawHook = safetyhook::create_inline(drawPrimitiveUP, DrawUP);
    }

    void Init()
    {
        auto movie = hook::pattern("83 EC 70 56 8B 74 24 78 85 F6 0F 84 ? ? ? ? 57 6A 00 E8 ? ? ? ? 68 01 00 01 00 6A 04 E8");
        auto open = hook::pattern("E8 ? ? ? ? 3B C3 A3 ? ? ? ? 0F 84 ? ? ? ? 8B 48 04 8B 10");
        MovieHandle = *open.get_first<uint32_t**>(8);
        MovieHook = safetyhook::create_inline(movie.get_first(), DrawMovie);
        NativeOpen = reinterpret_cast<decltype(NativeOpen)>(injector::GetBranchDestination(open.get_first()).as_int());
        injector::MakeCALL(open.get_first(), OpenMovie, true);
    }
}
