module;

#include <stdafx.h>
#include <atomic>
#include <ddraw.h>
#include "FixSettings.hpp"
#include "Shared/TonyHawk/Video.hpp"

export module Movies;

import ComVars;

namespace THPS2MovieLayout
{
    struct Picture
    {
        unsigned firstRow;
        unsigned height;
        TonyVideo::Box box;
    };

    using OpenFn = void* (__stdcall*)(const char*, unsigned);
    inline OpenFn OpenMovie = nullptr;
    inline unsigned BakedBars = 0;
    inline std::atomic<int> Mode{ 1 };

    inline bool IsStockMovieSize(unsigned width, unsigned height)
    {
        return (width == 320 && height == 240) || (width == 640 && height == 480);
    }

    inline void* __stdcall Open(const char* path, unsigned flags)
    {
        auto movie = OpenMovie(path, flags);
        BakedBars = 0;
        if (!movie)
            return movie;

        struct BinkInfo
        {
            unsigned width;
            unsigned height;
            unsigned frames;
        };
        BinkInfo info{};
        std::memcpy(&info, movie, sizeof(info));
        if (!IsStockMovieSize(info.width, info.height))
            return movie;

        std::string_view name(path);
        name.remove_prefix(name.find_last_of("\\/") + 1);
        // Only these stock movies have sustained black rows at both edges.
        if (info.frames == 1503 && asi_settings::text::IEquals(name, "Intro.dat"))
            BakedBars = 12;
        else if (info.frames == 266 && asi_settings::text::IEquals(name, "NSlogo.dat"))
            BakedBars = 38;
        return movie;
    }

    inline Picture Layout(float width, float height, unsigned sourceWidth, unsigned sourceHeight)
    {
        const auto mode = Mode.load(std::memory_order_relaxed);
        const unsigned firstRow = mode == 1 && IsStockMovieSize(sourceWidth, sourceHeight) ? BakedBars * sourceHeight / 240 : 0;
        const unsigned pictureHeight = sourceHeight - 2 * firstRow;
        return { firstRow, pictureHeight, TonyVideo::Layout(width, height, float(sourceWidth), float(pictureHeight), mode) };
    }

    inline void Install(HMODULE module)
    {
        ObserveFixSetting("MAIN.FMVWidescreenMode", Mode, [](const std::string& value)
        {
            return std::atoi(value.c_str());
        });
        auto open = hook::module_pattern(module, "FF 15 ? ? ? ? 85 C0 A3 ? ? ? ? 75 ? 8D 54 24 10 52 FF 15");
        OpenMovie = **open.get_first<OpenFn*>(2);
        auto site = open.get_first<uint8_t>();
        injector::MakeCALL(site, Open, true);
        injector::WriteMemory<uint8_t>(site + 5, 0x90, true);
    }
}

namespace THPS2DirectVideo
{
    using CopyFn = int(__stdcall*)(void*, void*, int, unsigned, unsigned, unsigned, unsigned);
    inline CopyFn Copy = nullptr;

    struct FrameCache
    {
        void* movie = nullptr;
        unsigned width = 0, height = 0, bytes = 0, frame = 0, flags = 0;
        std::vector<uint8_t> source, line;
        std::vector<unsigned> columns;
    };
    inline FrameCache Frame;

    inline void Fit(uint8_t* pixels, int pitch, unsigned width, unsigned height, unsigned bytes,
        unsigned sourceWidth, unsigned sourceHeight, size_t sourcePitch)
    {
        const auto picture = THPS2MovieLayout::Layout(float(width), float(height), sourceWidth, sourceHeight);
        const unsigned outWidth = std::clamp(unsigned(std::lround(picture.box.width)), 1u, width);
        const unsigned outHeight = std::clamp(unsigned(std::lround(picture.box.height)), 1u, height);
        const unsigned left = (width - outWidth) / 2, top = (height - outHeight) / 2;
        const size_t lineBytes = size_t(outWidth) * bytes;
        Frame.line.resize(lineBytes);
        Frame.columns.resize(outWidth);
        for (unsigned col = 0; col < outWidth; ++col)
            Frame.columns[col] = unsigned(uint64_t(col) * sourceWidth / outWidth) * bytes;

        for (unsigned row = 0; row < top; ++row)
            std::memset(pixels + ptrdiff_t(row) * pitch, 0, size_t(width) * bytes);
        unsigned previous = ~0u;
        for (unsigned row = 0; row < outHeight; ++row)
        {
            const unsigned sourceRow = picture.firstRow + unsigned(uint64_t(row) * picture.height / outHeight);
            if (sourceRow != previous)
            {
                const auto* input = Frame.source.data() + size_t(sourceRow) * sourcePitch;
                if (bytes == 2)
                    for (unsigned col = 0; col < outWidth; ++col) std::memcpy(Frame.line.data() + size_t(col) * 2, input + Frame.columns[col], 2);
                else if (bytes == 3)
                    for (unsigned col = 0; col < outWidth; ++col) std::memcpy(Frame.line.data() + size_t(col) * 3, input + Frame.columns[col], 3);
                else
                    for (unsigned col = 0; col < outWidth; ++col) std::memcpy(Frame.line.data() + size_t(col) * 4, input + Frame.columns[col], 4);
                previous = sourceRow;
            }
            auto* output = pixels + ptrdiff_t(top + row) * pitch;
            std::memset(output, 0, size_t(left) * bytes);
            std::memcpy(output + size_t(left) * bytes, Frame.line.data(), lineBytes);
            std::memset(output + size_t(left + outWidth) * bytes, 0, size_t(width - left - outWidth) * bytes);
        }
        for (unsigned row = top + outHeight; row < height; ++row)
            std::memset(pixels + ptrdiff_t(row) * pitch, 0, size_t(width) * bytes);
    }

    inline int __stdcall Draw(void* movie, void* pixels, int pitch, unsigned height, unsigned x, unsigned y, unsigned flags)
    {
        const unsigned sourceWidth = MovieWidth.get(), sourceHeight = MovieHeight.get();
        const unsigned bytes = MovieSurface.get().ddpfPixelFormat.dwRGBBitCount / 8;
        const unsigned width = MovieSurface.get().dwWidth, surfaceHeight = MovieSurface.get().dwHeight;
        const bool valid = MovieRenderMode != 0 && !TextureMovie && movie && pixels && pitch && pitch != INT32_MIN &&
            (bytes == 2 || bytes == 3 || bytes == 4) && sourceWidth >= 320 && sourceHeight >= 200 &&
            sourceWidth <= 16384 && sourceHeight <= 16384 && width && width <= 16384 && surfaceHeight && surfaceHeight <= 16384 &&
            size_t(std::abs(pitch)) >= size_t(width) * bytes && uint64_t(std::abs(pitch)) * surfaceHeight <= INT32_MAX;
        if (!valid)
        {
            Frame.movie = nullptr;
            return Copy(movie, pixels, pitch, height, x, y, flags);
        }

        unsigned frame = 0;
        std::memcpy(&frame, static_cast<uint8_t*>(movie) + 12, sizeof(frame));
        const size_t sourcePitch = (size_t(sourceWidth) * bytes + 3u) & ~size_t(3);
        const bool same = Frame.movie == movie && Frame.width == sourceWidth && Frame.height == sourceHeight &&
            Frame.bytes == bytes && Frame.flags == flags && frame >= Frame.frame;
        if (!same)
            Frame.source.assign(sourcePitch * sourceHeight, 0);

        // Keep Bink's previous decoded pixels available for dirty rectangles, independent of the display surface.
        const int result = Copy(movie, Frame.source.data(), static_cast<int>(sourcePitch), sourceHeight, 0, 0, flags);
        if (result != 0)
        {
            Frame.movie = nullptr;
            return result;
        }
        Frame.movie = movie;
        Frame.width = sourceWidth;
        Frame.height = sourceHeight;
        Frame.bytes = bytes;
        Frame.frame = frame;
        Frame.flags = flags;
        Fit(static_cast<uint8_t*>(pixels), pitch, width, surfaceHeight, bytes, sourceWidth, sourceHeight, sourcePitch);
        return result;
    }

    inline void Install(HMODULE module = GetModuleHandleW(nullptr))
    {
        auto copy = hook::module_pattern(module, "8B 0D ? ? ? ? 52 8B 15 ? ? ? ? 50 51 52 FF 15 ? ? ? ? A0 ? ? ? ? 84 C0 0F 84");
        auto render = hook::module_pattern(module, "8B 44 24 04 A3 ? ? ? ? 48 0F 84 ? ? ? ? 48 0F 84 ? ? ? ? C7 05");
        const auto surfacePointer = *copy.get_first<uint8_t**>(2);
        MovieSurface.SetAddress(reinterpret_cast<DDSURFACEDESC2*>(reinterpret_cast<uintptr_t>(surfacePointer) - offsetof(DDSURFACEDESC2, lpSurface)));
        TextureMovie.SetAddress(*copy.get_first<uint8_t*>(23));
        MovieRenderMode.SetAddress(*render.get_first<int*>(5));
        Copy = **copy.get_first<CopyFn*>(18);
        if (!Copy) std::terminate();
        auto site = copy.get_first<uint8_t>(16);
        injector::MakeCALL(site, Draw, true);
        injector::WriteMemory<uint8_t>(site + 5, 0x90, true);
    }
}

export namespace THPS2Video
{
    using Vertex = THPS2MovieVertex;

    inline SafetyHookInline SetupHook, DrawHook;
    inline SafetyHookMid SplitCopyHook;
    inline uintptr_t SplitCopyEnd = 0;
    inline float Width = 640.0f, Height = 480.0f;
    inline std::array<Vertex, 8> Native{};
    inline bool Ready = false;

    inline int __cdecl Setup()
    {
        const auto result = SetupHook.ccall<int>();
        Ready = MovieWidth.get() > 0 && MovieHeight.get() > 0 && MovieWidth.get() <= 16384 && MovieHeight.get() <= 16384;
        if (Ready)
            std::copy_n(MovieVertices.get().data(), Native.size(), Native.begin());
        return result;
    }

    inline int __cdecl Draw()
    {
        if (!Ready)
            return DrawHook.ccall<int>();
        const auto sourceWidth = MovieWidth.get(), sourceHeight = MovieHeight.get();
        const auto picture = THPS2MovieLayout::Layout(Width, Height, sourceWidth, sourceHeight);
        const auto& box = picture.box;
        const bool split = MovieSplit.get() != 0;
        const float edge = split ? std::clamp(static_cast<float>(MovieTextureWidth.get()) / MovieWidth.get(), 0.0f, 1.0f) : 1.0f;
        const auto count = split ? 8u : 4u;
        for (size_t i = 0; i < count; ++i)
        {
            auto vertex = Native[i];
            const bool right = i % 4 == 1 || i % 4 == 2;
            const bool bottom = i % 4 >= 2;
            const float x = i < 4 ? (right ? edge : 0.0f) : (right ? 1.0f : edge);
            vertex.x = box.x + x * box.width;
            vertex.y = box.y + (bottom ? box.height : 0.0f);
            if (picture.firstRow)
            {
                const auto quad = i - i % 4;
                const float inset = (Native[quad + 2].v - Native[quad].v) * float(picture.firstRow) / sourceHeight;
                vertex.v += bottom ? -inset : inset;
            }
            MovieVertices.get()[i] = vertex;
        }
        const auto result = DrawHook.ccall<int>();
        for (size_t i = 0; i < count; ++i)
            MovieVertices.get()[i] = Native[i];
        return result;
    }

    inline void Install(float width, float height, HMODULE module = GetModuleHandleW(nullptr))
    {
        Width = width;
        Height = height;
        auto setup = hook::module_pattern(module, "A0 ? ? ? ? 83 EC 10 53 33 DB 3A C3 0F 84 ? ? ? ? A1 ? ? ? ? 8B 0D");
        auto draw = hook::module_pattern(module, "A1 ? ? ? ? 83 EC 14 85 C0 56 0F 84 ? ? ? ? 8D 44 24 08 8D 4C 24 0C");
        auto summary = hook::module_pattern(module, "A1 ? ? ? ? 68 ? ? ? ? 50 FF 15 ? ? ? ? 8B 15 ? ? ? ? 8B 0D ? ? ? ? A1");
        auto textureWidth = hook::module_pattern(module, "3B 2D ? ? ? ? 77 ? 3B 1D ? ? ? ? 0F 86");
        auto splitCopy = hook::module_pattern(module, "85 D2 8D 0C 4B 89 4D FC 74 ? 0F 6F 3D ? ? ? ? 90 8B 75 FC 8B 7D F0 8B 4D E8");
        const auto start = reinterpret_cast<uintptr_t>(draw.get_first());
        auto quad = hook::range_pattern(start, start + 0x400, "6A 00 6A 04 68 ? ? ? ? 8B 10 68 44 01 00 00 6A 06 50 FF 52 64");
        MovieSplit.SetAddress(*setup.get_first<uint8_t*>(1));
        auto movieSize = *summary.get_first<uint32_t*>(6);
        MovieWidth.SetAddress(movieSize);
        MovieHeight.SetAddress(movieSize + 1);
        MovieTextureWidth.SetAddress(*textureWidth.get_first<uint32_t*>(2));
        MovieVertices.SetAddress(*quad.get_first<Vertex*>(5));
        SetupHook = safetyhook::create_inline(setup.get_first(), Setup);
        DrawHook = safetyhook::create_inline(draw.get_first(), Draw);
        const auto copyRow = splitCopy.get_first<uint8_t>(10);
        SplitCopyEnd = reinterpret_cast<uintptr_t>(copyRow + static_cast<int8_t>(copyRow[-1]));
        SplitCopyHook = safetyhook::create_mid(copyRow, [](SafetyHookContext& regs)
        {
            // Each row's second texture can be narrower than the first.
            const auto& surface = *reinterpret_cast<const DDSURFACEDESC2*>(regs.ebp - 0x114);
            const auto* input = *reinterpret_cast<const uint16_t**>(regs.ebp - 4);
            auto* output = *reinterpret_cast<uint16_t**>(regs.ebp - 0x10);
            for (DWORD x = 0; x < surface.dwWidth; ++x)
                output[x] = input[x] | 0x8000;
            regs.eip = SplitCopyEnd;
        });
        THPS2MovieLayout::Install(module);
        THPS2DirectVideo::Install(module);
    }
}
