module;

#include <stdafx.h>
#include <d3d9.h>
#include <unordered_map>
#include "FixSettings.hpp"

export module Movies;

import ComVars;

namespace KongMovies
{
    struct Movie
    {
        // Bink's dimensions are the first two fields of its movie handle.
        const uint32_t* video;
    };

    std::atomic<int> Mode{ 1 };
    Movie** ActiveMovie = nullptr;
    RECT* SignatureRect = nullptr;
    bool SignatureD3D8 = false;
    void* (__thiscall* DrawMovieMesh)(void*, void*, int) = nullptr;

    struct FrameFormat
    {
        uint32_t height;
        int mode;
    };

    struct LetterboxedMovie
    {
        uint32_t bars;
        FrameFormat frame;
    };

    std::mutex MoviesMutex;
    std::unordered_map<const uint32_t*, LetterboxedMovie> LetterboxedMovies;
    bool SkipLogos = false;
    uint32_t* (__stdcall* NativeOpen)(const char*, uint32_t) = nullptr;
    void (__stdcall* NativeClose)(const uint32_t*) = nullptr;
    int (__stdcall* NativeCopy)(const uint32_t*, void*, int, uint32_t, uint32_t, uint32_t, uint32_t) = nullptr;

    bool MovieName(std::string_view name, std::string_view expected)
    {
        return name.size() == expected.size() && std::equal(name.begin(), name.end(), expected.begin(),
            [](unsigned char a, unsigned char b) { return std::tolower(a) == std::tolower(b); });
    }

    uint32_t BakedBars(std::string_view name, uint32_t width, uint32_t height)
    {
        if (width == 512 && height == 384 && (MovieName(name, "newgame.bik") || MovieName(name, "bonus_3.bik")))
            return 46;
        if (width == 640 && height == 480)
        {
            if (MovieName(name, "bonus_0.bik")) return 60;
            if (MovieName(name, "bonus_2.bik")) return 50;
        }
        return 0;
    }

    FrameFormat Picture(const uint32_t* video)
    {
        const std::lock_guard lock(MoviesMutex);
        const auto movie = LetterboxedMovies.find(video);
        if (movie != LetterboxedMovies.end())
            return movie->second.frame;
        return { video[1], Mode.load(std::memory_order_relaxed) };
    }

    bool BlackRow(const uint8_t* row, uint32_t width, uint32_t type, uint32_t bytes)
    {
        for (uint32_t x = 0; x < width; ++x, row += bytes)
        {
            if (bytes >= 3)
            {
                if ((row[0] | row[1] | row[2]) > 7) return false;
            }
            else if (type == 13 || type == 14)
            {
                const unsigned luma = row[type == 13 ? 0 : 1];
                const unsigned chroma = row[type == 13 ? 1 : 0];
                if (luma > 23 || chroma < 120 || chroma > 136) return false;
            }
            else
            {
                const unsigned colorMask = type == 7 ? 0x0FFF : type == 9 ? 0x7FFF : 0xFFDF;
                if (((row[1] << 8) | row[0]) & colorMask) return false;
            }
        }
        return true;
    }

    bool ExpandPicture(uint8_t* pixels, int pitch, uint32_t width, uint32_t height, uint32_t bars, uint32_t type)
    {
        if (type != 1 && type != 3 && type != 5 && type != 7 && type != 9 && type != 10 && type != 13 && type != 14)
            return false;
        const uint32_t bytes = type == 1 ? 3 : (type == 3 || type == 5) ? 4 : 2;
        if (!pixels || pitch < int(width * bytes) || bars * 2 >= height)
            return false;
        const auto row = [=](uint32_t y) { return pixels + size_t(y) * pitch; };
        for (uint32_t y = 0; y < bars; ++y)
        {
            if (!BlackRow(row(y), width, type, bytes) || !BlackRow(row(height - 1 - y), width, type, bytes))
                return false;
        }

        // Expand the picture outward in place. The native subtitle draw happens after this copy.
        const uint32_t picture = height - bars * 2;
        const auto expandRow = [&](uint32_t y)
        {
            const uint32_t source = bars + (y * (picture - 1) + (height - 1) / 2) / (height - 1);
            if (source != y)
                std::memcpy(row(y), row(source), width * bytes);
        };
        for (uint32_t y = 0; y < height / 2; ++y)
            expandRow(y);
        for (uint32_t y = height; y-- > height / 2;)
            expandRow(y);
        return true;
    }

    uint32_t* __stdcall Open(const char* name, uint32_t flags)
    {
        constexpr uint32_t FileHandle = 0x00800000;
        if (!name || (flags & FileHandle))
            return NativeOpen(name, flags);
        std::string_view file = name;
        if (const auto slash = file.find_last_of("\\/"); slash != file.npos)
            file.remove_prefix(slash + 1);
        if (SkipLogos)
        {
            const auto stem = file.substr(0, file.find('.'));
            for (const auto logo : { "mov1", "mov2", "mov3", "mov4" })
            {
                if (MovieName(stem, logo)) return nullptr;
            }
        }
        auto* video = NativeOpen(name, flags);
        if (video)
        {
            if (const auto bars = BakedBars(file, video[0], video[1]))
            {
                const std::lock_guard lock(MoviesMutex);
                LetterboxedMovies.insert_or_assign(video, LetterboxedMovie{ bars, { video[1], Mode.load() } });
            }
        }
        return video;
    }

    void __stdcall Close(const uint32_t* video)
    {
        {
            const std::lock_guard lock(MoviesMutex);
            LetterboxedMovies.erase(video);
        }
        NativeClose(video);
    }

    int __stdcall Copy(const uint32_t* video, void* pixels, int pitch, uint32_t height, uint32_t x, uint32_t y, uint32_t flags)
    {
        const auto result = NativeCopy(video, pixels, pitch, height, x, y, flags);
        if (result || !video || x || y || height != video[1])
            return result;
        const std::lock_guard lock(MoviesMutex);
        const auto movie = LetterboxedMovies.find(video);
        if (movie != LetterboxedMovies.end())
        {
            auto& [bars, frame] = movie->second;
            const int mode = Mode.load(std::memory_order_relaxed);
            const bool expanded = mode == 1 && ExpandPicture(static_cast<uint8_t*>(pixels), pitch, video[0], height, bars, flags & 15);
            frame = { expanded ? height - bars * 2 : height, mode };
        }
        return result;
    }

    RECT Fit(int width, int height, uint32_t videoWidth, uint32_t videoHeight, int mode)
    {
        if (width <= 0 || height <= 0 || !videoWidth || !videoHeight)
            return { 0, 0, width, height };
        const double boxWidth = mode == 0 ? std::min<double>(width, height * (4.0 / 3.0)) : width;
        const double boxHeight = mode == 0 ? std::min<double>(height, width * (3.0 / 4.0)) : height;
        const double scale = std::min(boxWidth / videoWidth, boxHeight / videoHeight);
        const auto fittedWidth = std::clamp(std::lround(videoWidth * scale), 1L, long(width));
        const auto fittedHeight = std::clamp(std::lround(videoHeight * scale), 1L, long(height));
        const auto left = (width - fittedWidth) / 2;
        const auto top = (height - fittedHeight) / 2;
        return { left, top, left + fittedWidth, top + fittedHeight };
    }

    void* __fastcall DrawGamersEdition(void* renderer, void*, void* object, int pass)
    {
        const auto* movie = *ActiveMovie;
        auto* device = static_cast<IDirect3DDevice9**>(renderer)[1];
        D3DVIEWPORT9 viewport{};
        if (!movie || !movie->video || !device || FAILED(device->GetViewport(&viewport)) || !viewport.Width || !viewport.Height)
            return DrawMovieMesh(renderer, object, pass);

        const auto pictureFormat = Picture(movie->video);
        const auto box = Fit(viewport.Width, viewport.Height, movie->video[0], pictureFormat.height, pictureFormat.mode);
        const D3DVIEWPORT9 picture{ viewport.X + DWORD(box.left), viewport.Y + DWORD(box.top),
            DWORD(box.right - box.left), DWORD(box.bottom - box.top), viewport.MinZ, viewport.MaxZ };
        if (FAILED(device->SetViewport(&picture)))
            return DrawMovieMesh(renderer, object, pass);

        const auto result = DrawMovieMesh(renderer, object, pass);
        device->SetViewport(&viewport);
        return result;
    }

    void DrawSignatureEdition(SafetyHookContext& context)
    {
        const auto* movie = *ActiveMovie;
        const auto* display = reinterpret_cast<const int*>(SignatureD3D8 ? context.ecx : context.eax);
        if (movie && movie->video && display)
        {
            const auto picture = Picture(movie->video);
            *SignatureRect = Fit(display[20], display[21], movie->video[0], picture.height, picture.mode);
        }
    }

    void InstallPlayback()
    {
        SkipLogos = FixSettings().Get("MAIN.SkipIntro") == "1";
        auto imports = IATHook::Replace(GetModuleHandleW(nullptr), "binkw32.dll", std::forward_as_tuple("_BinkOpen@8", Open),
            std::forward_as_tuple("_BinkCopyToBuffer@28", Copy), std::forward_as_tuple("_BinkClose@4", Close));
        NativeOpen = reinterpret_cast<decltype(NativeOpen)>(imports.at("_BinkOpen@8").get());
        NativeCopy = reinterpret_cast<decltype(NativeCopy)>(imports.at("_BinkCopyToBuffer@28").get());
        NativeClose = reinterpret_cast<decltype(NativeClose)>(imports.at("_BinkClose@4").get());
    }

    void InstallFraming()
    {
        const auto refresh = []()
        {
            static std::mutex mutex;
            const std::lock_guard lock(mutex);
            Mode.store(FixSettings().Get("MAIN.FMVWidescreenMode") == "0" ? 0 : 1, std::memory_order_relaxed);
        };
        FixSettings().OnChange("MAIN.FMVWidescreenMode", refresh);
        refresh();
        if (!Screen.bSignatureEdition)
        {
            auto movie = hook::pattern("A1 ? ? ? ? 85 C0 0F 84 ? ? ? ? 8B 0D ? ? ? ? 56 51 50 E8 ? ? ? ? 83 C4 08 84 C0 A1");
            ActiveMovie = *movie.get_first<Movie**>(1);
            auto draw = hook::pattern("8B 46 14 6A 00 50 B9 ? ? ? ? E8 ? ? ? ? 6A 00 68 04 00 00 80 B9");
            DrawMovieMesh = reinterpret_cast<decltype(DrawMovieMesh)>(injector::MakeCALL(draw.get_first(11), DrawGamersEdition, true).as_int());
            return;
        }

        constexpr auto draw8 = "81 EC 84 00 00 00 53 56 57 8B D8 83 CF FF 85 DB 8B F1 75 ? BF 00 00 00 FF 8B 86 60 01 00 00";
        constexpr auto draw9d = "81 EC 84 00 00 00 53 55 56 57 8B F0 8B 84 24 98 00 00 00 33 FF 83 CB FF";
        constexpr auto draw9 = "81 EC 84 00 00 00 56 57 83 CF FF 85 DB 8B F0 75 ? BF 00 00 00 FF 8B 86 70 01 00 00";
        SignatureD3D8 = !hook::pattern(draw8).empty();
        const bool debug = !SignatureD3D8 && !hook::pattern(draw9d).empty();
        auto draw = hook::pattern(SignatureD3D8 ? draw8 : debug ? draw9d : draw9);
        auto rect = hook::pattern(debug
            ? "DB 05 ? ? ? ? C7 44 24 ? 00 00 00 00 C7 44 24 ? 0A D7 23 3C D8 25"
            : "DB 05 ? ? ? ? 68 00 00 80 3F 68 00 00 80 3F D8 25");
        SignatureRect = *rect.get_first<RECT*>(2);
        auto movie = hook::pattern(debug
            ? "A1 ? ? ? ? 85 C0 74 ? 56 8B 74 24 0C 8B 8E 70 01 00 00 51 50 E8"
            : SignatureD3D8
                ? "A1 ? ? ? ? 74 ? 8B 10 8B 40 04 8B CE 89 54 24 ? 5E 83 C4 04 E9"
                : "A1 ? ? ? ? 74 ? 8B 10 8B 58 04 8B C6 89 54 24 ? E8");
        ActiveMovie = *movie.get_first<Movie**>(1);
        static auto drawHook = safetyhook::create_mid(draw.get_first(), DrawSignatureEdition);
    }
}

class Movies
{
public:
    Movies()
    {
        WFP::onInitEvent() += []()
        {
            KongMovies::InstallFraming();
            KongMovies::InstallPlayback();
        };
    }
} Movies;
