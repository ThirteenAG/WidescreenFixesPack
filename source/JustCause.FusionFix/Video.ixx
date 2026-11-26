module;
#include <stdafx.h>
#include <atomic>
#include <unordered_map>
#include "FixSettings.hpp"

export module Video;


export namespace JCVideo
{
    struct Quad
    {
        float x, y, halfWidth, halfHeight, rotation, u0, v0, u1, v1;
        uint8_t flipX, flipY, scaleX, padding;
    };
    static_assert(sizeof(Quad) == 40);
    struct Decoder
    {
        uint32_t width, height;
        uint64_t generation;
        bool skipLogo;
        bool decoded = false;
    };

    inline std::mutex decodersMutex;
    inline std::unordered_map<void*, Decoder> decoders;
    inline uint64_t nextGeneration = 0;

    inline void Opened(void* handle, uint32_t width, uint32_t height, bool skipLogo)
    {
        const std::lock_guard lock(decodersMutex);
        decoders.insert_or_assign(handle, Decoder{ width, height, ++nextGeneration, skipLogo });
    }

    inline void Closed(void* handle)
    {
        const std::lock_guard lock(decodersMutex);
        decoders.erase(handle);
    }

    // A skipped logo still decodes and draws its first frame, so the player ends it the way it ends a finished movie
    inline bool EndLogo(void* handle)
    {
        const std::lock_guard lock(decodersMutex);
        const auto found = decoders.find(handle);
        if (found == decoders.end() || !found->second.skipLogo)
            return false;
        return std::exchange(found->second.decoded, true);
    }

    inline std::optional<Decoder> FindDecoder(void* handle)
    {
        const std::lock_guard lock(decodersMutex);
        if (const auto found = decoders.find(handle); found != decoders.end())
            return found->second;
        return std::nullopt;
    }

    struct DrawState
    {
        Decoder decoder;
        int mode;
    };

    inline void** nativeDecoder = nullptr;
    // The player that decoded a frame last: its global can outlive it, so only a player that is playing counts
    inline std::atomic<void*> activePlayer{ nullptr };
    inline std::atomic<ULONGLONG> activeTick{ 0 };

    inline void Playing(void* player)
    {
        activePlayer.store(player);
        activeTick.store(GetTickCount64());
    }
    inline thread_local const DrawState* drawState = nullptr;
    inline std::atomic<int> requestedMode{ 0 };
    inline SafetyHookInline spriteHook, constructHook;
    inline bool seen = false, decoderReady = false;
    inline RECT picture{};
    inline std::atomic<uint32_t> outputSize{ 0 };
    struct Key
    {
        void* sprite;
        uint64_t generation;
        uint32_t size;
        int mode;
        bool operator==(const Key&) const = default;
    };
    inline thread_local Key applied{};
    inline thread_local bool geometryReady = false;

    inline bool Transform(Quad& quad, int sourceWidth, int sourceHeight, int outputWidth, int outputHeight, int mode)
    {
        if (sourceWidth <= 0 || sourceHeight <= 0 || outputWidth <= 0 || outputHeight <= 0 ||
            sourceWidth > 16384 || sourceHeight > 16384 || outputWidth > 16384 || outputHeight > 16384) return false;
        const auto frameWidth = mode == 0 ? std::min(static_cast<float>(outputWidth), outputHeight * (4.0f / 3.0f)) : static_cast<float>(outputWidth);
        const auto frameHeight = mode == 0 ? std::min(static_cast<float>(outputHeight), outputWidth * (3.0f / 4.0f)) : static_cast<float>(outputHeight);
        const auto scaleX = frameWidth / sourceWidth;
        const auto scaleY = frameHeight / sourceHeight;
        // Show the whole movie at its own aspect
        const auto scale = std::min(scaleX, scaleY);
        const auto pictureWidth = sourceWidth * scale;
        const auto pictureHeight = sourceHeight * scale;
        quad.x = outputWidth * 0.5f;
        quad.y = outputHeight * 0.5f;
        quad.halfWidth = std::min(pictureWidth, static_cast<float>(outputWidth)) * 0.5f;
        quad.halfHeight = std::min(pictureHeight, static_cast<float>(outputHeight)) * 0.5f;
        quad.scaleX = 0;
        return true;
    }

    inline bool Apply(void* unit, Quad& quad, int outputWidth, int outputHeight)
    {
        if (!unit || !drawState)
            return false;
        const auto& decoder = drawState->decoder;
        if (!Transform(quad, decoder.width, decoder.height, outputWidth, outputHeight, drawState->mode))
            return false;
        picture = { static_cast<LONG>(quad.x - quad.halfWidth), static_cast<LONG>(quad.y - quad.halfHeight),
            static_cast<LONG>(quad.x + quad.halfWidth), static_cast<LONG>(quad.y + quad.halfHeight) };
        geometryReady = true;
        return true;
    }

    // The full-screen player owns this sprite and uses its own decoder. Other movies draw through their own paths.
    inline std::optional<Decoder> DecoderForSprite(void* self)
    {
        const auto player = static_cast<const uint8_t*>(activePlayer.load());
        if (!player || GetTickCount64() - activeTick.load() > 250 || *reinterpret_cast<void* const*>(player + 164) != self)
            return std::nullopt;
        return FindDecoder(*nativeDecoder);
    }

    inline bool (*filter)(void*) = nullptr;

    inline void __fastcall Sprite(void* self, void*)
    {
        if (filter && !filter(self))
            return;
        const auto decoder = DecoderForSprite(self);
        const DrawState state{ decoder.value_or(Decoder{}), requestedMode.load() };
        if (decoder)
        {
            const Key key{ self, decoder->generation, outputSize.load(), state.mode };
            if (key != applied)
            {
                applied = key;
                geometryReady = false;
                static_cast<uint8_t*>(self)[44] |= 4;
            }
        }
        else if (applied.sprite == self)
        {
            // The movie is gone but the sprite still holds its framing: let the game rebuild its own quad
            applied = {};
            static_cast<uint8_t*>(self)[44] |= 4;
        }
        const auto previous = std::exchange(drawState, decoder ? &state : nullptr);
        spriteHook.thiscall<void>(self);
        drawState = previous;
        if (decoder && geometryReady)
            seen = true;
    }

    inline void* __fastcall Construct(void* self, void*)
    {
        if (applied.sprite == self)
            applied = {};
        return constructHook.thiscall<void*>(self);
    }

    inline void Install()
    {
        const auto refreshMode = []()
        {
            static std::mutex mutex;
            const std::lock_guard lock(mutex);
            requestedMode.store(GetFixNumber("MAIN.FMVWidescreenMode", 0));
        };
        FixSettings().OnChange("MAIN.FMVWidescreenMode", refreshMode);
        refreshMode();
        auto spriteDraw = hook::pattern("64 A1 00 00 00 00 6A FF 68 ? ? ? ? 50 64 89 25 00 00 00 00 83 EC 44 53 56 8B F1 8A 5E 2C C0 EB 02 80 E3 01");
        auto construct = hook::pattern("6A FF ? ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 51 53 56 8B F1 57 89 74 24 0C E8 ? ? ? ? 33 DB C7 06 ? ? ? ? C7 86 84 00 00 00 0F 00 00 00");
        auto constructSite = construct.get_first();
        auto spriteSite = spriteDraw.get_first();
        if (decoderReady)
        {
            constructHook = safetyhook::create_inline(constructSite, Construct);
            spriteHook = safetyhook::create_inline(spriteSite, Sprite);
        }
    }
}
