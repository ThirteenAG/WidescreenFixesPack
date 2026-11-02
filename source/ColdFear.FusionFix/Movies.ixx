module;

#include <stdafx.h>
#include <d3d9.h>
#include "FixSettings.hpp"
#include <algorithm>
#include <bcrypt.h>
#include <fstream>
#include <unordered_map>

export module Movies;


export namespace ColdFearMovie
{
    struct Info
    {
        int width = 0;
        int height = 0;
        int top = 0;
        int bottom = 0;
    };

    struct Frame
    {
        float x = 0.0f;
        float y = 0.0f;
        float width = 0.0f;
        float height = 0.0f;
    };

    inline Frame Layout(int screenWidth, int screenHeight, const Info& movie, int mode)
    {
        Frame frame;
        if (screenWidth <= 0 || screenHeight <= 0 || movie.width <= 0 || movie.height <= 0)
            return frame;
        const int top = mode != 0 ? std::clamp(movie.top, 0, movie.height - 1) : 0;
        const int bottom = mode != 0 ? std::clamp(movie.bottom, top + 1, movie.height) : movie.height;
        const float boxWidth = mode != 0 ? float(screenWidth) :
            std::min(float(screenWidth), float(screenHeight) * (4.0f / 3.0f));
        const float scale = std::min(boxWidth / float(movie.width), float(screenHeight) / float(bottom - top));
        frame.width = float(movie.width) * scale;
        frame.height = float(movie.height) * scale;
        frame.x = (float(screenWidth) - frame.width) * 0.5f;
        frame.y = (float(screenHeight) - float(bottom - top) * scale) * 0.5f - float(top) * scale;
        return frame;
    }
}

export namespace ColdFearMovie
{
    struct BoundsEntry
    {
        const char* sha256;
        int width, height, top, bottom;
    };

    // Baked bars of the original movies, recognized by their complete file hash.
    inline constexpr BoundsEntry Bounds[] =
    {
        { "3339a5b52b35727e509ad6d63575dcebe72d9ec1096e50dfb1fe79b92ca1b281", 768, 576, 125, 451 },
        { "7b5d9b2d48a46fcf7bfa968d6cde55dffa086561cb39ebe718acc7fec5892517", 720, 540, 112, 424 },
        { "d2b2aedbd57f787b26949fb0802f0ba9ff9b411e4242238e02ada4649b63abc6", 720, 540, 112, 424 },
        { "9bdd5a22b3d3253dd235777ba6a16c51c76e986cd0cd8494f125300717616fb2", 720, 540, 112, 424 },
        { "9a64f353280fd58efb83b76f20e6dd7af76b3cb285e41512f5d999f4e7615453", 720, 540, 112, 426 },
        { "2b7e4ec81c59af1651b24c0de4e9e074dcd1cf2b11402f02d87b860c52e67ae8", 720, 540, 126, 416 },
        { "f50163322a67137f4751b24fdb0f66520cd9862a8a4a47ee4b52752a417c833a", 720, 540, 112, 424 },
        { "11de3f342ac361c128e25af27e9dcf8d1bb0d6cf745d5b518ae2cb116fa4a739", 720, 540, 116, 424 },
        { "40a44bfc2a2101420c545e0c75cfa46eff9c1ce836e08a8b6e9605ed5b508440", 720, 540, 113, 424 },
        { "712d4566516f65bb4fbf5b06a5b638f0fff517066cfa15393f47d80dbc0762d5", 720, 540, 116, 424 },
        { "cd6c34bb8c2e8c86e499a3bfe97ff17ad9548e3e427b9e9b7e47d7a1c160305c", 720, 540, 106, 436 },
        { "d4a300eec4ef823ce1f15c0c7287fc6088bd8f598b525a4044110395f828f22c", 720, 540, 113, 436 },
        { "ac40c66e5db34180622b3ea8b7bf0ac838232b0b087839a3c24a721ce2e4e074", 720, 540, 112, 424 },
        { "45ef9bdc8d252c604d03371bfa7572dd331e5ce807cfa4a8d330bd0589d3ddac", 720, 540, 112, 428 },
    };
}

#pragma comment(lib, "bcrypt.lib")

export namespace ColdFearMovie
{
    inline thread_local Info Current;
    inline SafetyHookInline PlayHook;

    inline std::string Hash(std::ifstream& file)
    {
        BCRYPT_ALG_HANDLE algorithm = nullptr;
        BCRYPT_HASH_HANDLE hash = nullptr;
        std::string result;
        if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) < 0)
            return result;
        if (BCryptCreateHash(algorithm, &hash, nullptr, 0, nullptr, 0, 0) >= 0)
        {
            file.clear();
            file.seekg(0);
            std::array<char, 65536> buffer{};
            bool valid = true;
            while (file)
            {
                file.read(buffer.data(), buffer.size());
                if (file.gcount() && BCryptHashData(hash, reinterpret_cast<PUCHAR>(buffer.data()), ULONG(file.gcount()), 0) < 0)
                {
                    valid = false;
                    break;
                }
            }
            std::array<uint8_t, 32> digest{};
            if (valid && file.eof() && BCryptFinishHash(hash, digest.data(), ULONG(digest.size()), 0) >= 0)
            {
                constexpr char hex[] = "0123456789abcdef";
                for (const auto byte : digest)
                {
                    result.push_back(hex[byte >> 4]);
                    result.push_back(hex[byte & 15]);
                }
            }
            BCryptDestroyHash(hash);
        }
        BCryptCloseAlgorithmProvider(algorithm, 0);
        return result;
    }

    inline Info Inspect(const std::filesystem::path& path)
    {
        static std::unordered_map<std::filesystem::path, Info> cache;
        if (const auto found = cache.find(path); found != cache.end())
            return found->second;
        Info info;
        std::ifstream file(path, std::ios::binary);
        std::array<uint32_t, 7> header{};
        if (file.read(reinterpret_cast<char*>(header.data()), sizeof(header)) &&
            (header[0] & 0x00FFFFFFu) == 0x004B4942u && header[5] > 0 && header[5] <= 16384 &&
            header[6] > 0 && header[6] <= 16384)
        {
            info.width = int(header[5]);
            info.height = int(header[6]);
            info.bottom = info.height;
            const auto name = path.filename().string();
            if (name.size() >= 4 && _strnicmp(name.c_str(), "FMV_", 4) == 0)
            {
                const auto digest = Hash(file);
                for (const auto& known : Bounds)
                {
                    if (digest == known.sha256 && info.width == known.width && info.height == known.height)
                    {
                        info.top = known.top;
                        info.bottom = known.bottom;
                        break;
                    }
                }
            }
        }
        cache.emplace(path, info);
        return info;
    }

    inline void __cdecl Play(const char* name, const char* directory, char* language, int flags, int subtitle)
    {
        struct Restore
        {
            Info previous;
            ~Restore() { Current = previous; }
        } restore{ Current };
        Current = name && directory ? Inspect(std::filesystem::path(directory) / (std::string(name) + ".bik")) : Info{};
        PlayHook.unsafe_ccall(name, directory, language, flags, subtitle);
    }

    inline void Init()
    {
        auto play = hook::pattern("55 8B EC 81 EC 88 01 00 00 8B 45 08 8B 4D 0C 56 57 50 51 8D 95 78 FE FF FF 68");
        PlayHook = safetyhook::create_inline(play.get_first(), Play);
    }
}
