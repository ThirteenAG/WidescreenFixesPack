#pragma once
#include <ddraw.h>

namespace GTA2Movies
{
    using CopyFunction = int (WINAPI*)(uint32_t*, void*, int, uint32_t, uint32_t, uint32_t, uint32_t);
    using OpenFunction = void* (WINAPI*)(HWND, uint32_t, uint32_t, uint32_t);
    using BlitFunction = void (WINAPI*)(uint32_t*, RECT*, uint32_t);
    using CloseFunction = void (WINAPI*)(uint32_t*);
    inline CopyFunction OriginalCopy = nullptr;
    inline OpenFunction OriginalOpen = nullptr;
    inline BlitFunction OriginalBlit = nullptr;
    inline CloseFunction OriginalClose = nullptr;
    inline uint32_t* WindowBuffer = nullptr;
    inline IUnknown** BinkDraw = nullptr;
    inline char** Video = nullptr;

    inline RECT Fit(int width, int height, int movieWidth, int movieHeight)
    {
        auto fitted = Desktop::Fit(width, height, movieWidth, movieHeight);
        return {fitted.left, fitted.top, fitted.right, fitted.bottom};
    }

    inline int WINAPI Copy(uint32_t* movie, void* destination, int pitch, uint32_t height,
        uint32_t x, uint32_t y, uint32_t flags)
    {
        auto video = *Video;
        bool windowBuffer = WindowBuffer && destination == reinterpret_cast<void*>(WindowBuffer[5]);
        if (!movie || !video || (!windowBuffer && destination != *reinterpret_cast<void**>(video + 80)) || pitch <= 0)
            return OriginalCopy(movie, destination, pitch, height, x, y, flags);
        int width = windowBuffer ? WindowBuffer[0] : *reinterpret_cast<int*>(video + 72);
        int outputHeight = windowBuffer ? WindowBuffer[1] : *reinterpret_cast<int*>(video + 76);
        int format = flags & 15;
        int depth = windowBuffer ? (format == 0 ? 24 : format == 1 ? 32 : format >= 2 && format <= 5 ? 16 : 0)
            : *reinterpret_cast<int*>(video + 400); // Locked DDSURFACEDESC pixel depth.
        int bytes = depth / 8;
        if (width <= 0 || width > 7680 || outputHeight <= 0 || outputHeight > 4320 ||
            movie[0] == 0 || movie[0] > 8192 || movie[1] == 0 || movie[1] > 8192 ||
            (depth != 16 && depth != 24 && depth != 32) || pitch < width * bytes)
            return OriginalCopy(movie, destination, pitch, height, x, y, flags);

        // Decode into a movie-sized buffer. Never offset the decoder into a
        // surface whose pitch/height belong to a different allocation.
        if (depth == 32) flags = (flags & ~15u) | 1u;
        else if (depth == 24) flags &= ~15u;
        else if ((flags & 15u) < 2 || (flags & 15u) > 5)
            flags = (flags & ~15u) | (*reinterpret_cast<int*>(video + 100) == 6 ? 3u : 2u);
        static std::vector<uint8_t> frame;
        int sourcePitch = (movie[0] * bytes + 15) & ~15;
        frame.resize(size_t(sourcePitch) * movie[1]);
        int result = OriginalCopy(movie, frame.data(), sourcePitch, movie[1], 0, 0, flags | 0x04000000);
        if (!result) return result;
        memset(destination, 0, size_t(pitch) * outputHeight);
        auto rect = Fit(width, outputHeight, movie[0], movie[1]);
        int w = rect.right - rect.left, h = rect.bottom - rect.top;
        for (int row = 0; row < h; ++row)
        {
            auto source = frame.data() + size_t(row * movie[1] / h) * sourcePitch;
            auto output = static_cast<uint8_t*>(destination) + size_t(row + rect.top) * pitch + rect.left * bytes;
            for (int col = 0; col < w; ++col)
                memcpy(output + col * bytes, source + size_t(col * movie[0] / w) * bytes, bytes);
        }
        return result;
    }

    inline void* WINAPI Open(HWND window, uint32_t width, uint32_t height, uint32_t flags)
    {
        WindowBuffer = nullptr;
        RECT client{};
        RECT fitted{};
        bool fit = width && height && GetClientRect(window, &client) && client.right > 0 && client.bottom > 0;
        if (fit) fitted = Fit(client.right, client.bottom, width, height);
        // Older Bink primary buffers cannot scale. Allocate a movie output surface
        // at the fitted size and resample during decoding instead of stretching it.
        // GTA2 gives Bink an IDirectDraw4 interface, while this Bink version
        // creates surfaces with the older 108-byte DDSURFACEDESC. Use the old
        // interface during allocation; keep the renderer's interface unchanged.
        IUnknown* savedDraw = BinkDraw ? *BinkDraw : nullptr;
        IDirectDraw* compatibleDraw = nullptr;
        constexpr GUID drawIID = {0x6c14db80, 0xa733, 0x11ce, {0xa5,0x21,0x00,0x20,0xaf,0x0b,0xe5,0x60}};
        if (savedDraw && SUCCEEDED(savedDraw->QueryInterface(drawIID, reinterpret_cast<void**>(&compatibleDraw))))
            *BinkDraw = compatibleDraw;
        auto buffer = fit ? OriginalOpen(window, fitted.right - fitted.left, fitted.bottom - fitted.top,
            (flags & ~31u) | 8u) : nullptr; // RGB system-memory DirectDraw blit buffer.
        if (compatibleDraw) { *BinkDraw = savedDraw; compatibleDraw->Release(); }
        if (buffer) WindowBuffer = static_cast<uint32_t*>(buffer);
        else buffer = OriginalOpen(window, width, height, flags);
        if (Log::Enabled)
        {
            char message[192]{};
            sprintf_s(message, "GTA2 movie buffer: movie=%ux%u client=%ldx%ld fitted=%ldx%ld scaled=%d buffer=%p",
                width, height, client.right, client.bottom, fitted.right - fitted.left, fitted.bottom - fitted.top,
                WindowBuffer != nullptr, buffer);
            Log::Write(message);
        }
        if (!buffer || !width || !height) return buffer;
        auto dll = GetModuleHandleW(L"binkw32.dll");
        auto scale = reinterpret_cast<int(WINAPI*)(void*, uint32_t, uint32_t)>(GetProcAddress(dll, "_BinkBufferSetScale@12"));
        auto offset = reinterpret_cast<int(WINAPI*)(void*, int, int)>(GetProcAddress(dll, "_BinkBufferSetOffset@12"));
        if (offset && GetClientRect(window, &client) && client.right > 0 && client.bottom > 0)
        {
            auto rect = Fit(client.right, client.bottom, width, height);
            if (!WindowBuffer && (!scale || !scale(buffer, rect.right - rect.left, rect.bottom - rect.top)))
            {
                if (scale) scale(buffer, width, height);
                rect = { (client.right - int(width)) / 2, (client.bottom - int(height)) / 2, 0, 0 };
            }
            offset(buffer, rect.left, rect.top);
        }
        return buffer;
    }

    inline void WINAPI Blit(uint32_t* buffer, RECT* rectangles, uint32_t count)
    {
        if (buffer == WindowBuffer)
        {
            RECT full{0, 0, int(buffer[0]), int(buffer[1])};
            OriginalBlit(buffer, &full, 1);
        }
        else OriginalBlit(buffer, rectangles, count);
    }
    inline void WINAPI Close(uint32_t* buffer)
    {
        if (buffer == WindowBuffer) WindowBuffer = nullptr;
        OriginalClose(buffer);
    }

    inline void Install()
    {
        auto binkDraw = hook::module_pattern(GetModuleHandleW(L"binkw32.dll"),
            "8B 44 24 08 8B 08 68 ? ? ? ? 50 A3 ? ? ? ? FF 91 90 00 00 00");
        if (binkDraw.size() == 1) BinkDraw = *binkDraw.get_first<IUnknown**>(7);
        Video = *hook::pattern("A1 ? ? ? ? 83 EC 08 50 FF 15 ? ? ? ? A1").get_first<char**>(1);
        auto originals = IATHook::Replace(GetModuleHandleW(nullptr), "binkw32.dll",
            std::make_tuple("_BinkCopyToBuffer@28", Copy), std::make_tuple("_BinkBufferOpen@16", Open),
            std::make_tuple("_BinkBufferBlit@12", Blit), std::make_tuple("_BinkBufferClose@4", Close));
        if (originals.contains("_BinkCopyToBuffer@28")) OriginalCopy = reinterpret_cast<CopyFunction>(originals.at("_BinkCopyToBuffer@28").get());
        if (originals.contains("_BinkBufferOpen@16")) OriginalOpen = reinterpret_cast<OpenFunction>(originals.at("_BinkBufferOpen@16").get());
        if (originals.contains("_BinkBufferBlit@12")) OriginalBlit = reinterpret_cast<BlitFunction>(originals.at("_BinkBufferBlit@12").get());
        if (originals.contains("_BinkBufferClose@4")) OriginalClose = reinterpret_cast<CloseFunction>(originals.at("_BinkBufferClose@4").get());
    }
}
