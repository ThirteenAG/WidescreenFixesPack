module;
#include <stdafx.h>
#include <atomic>
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
    inline std::atomic<int> width{ 0 }, height{ 0 };
    inline std::atomic<void*> activeSprite{ nullptr }, handle{ nullptr };
    inline thread_local void* sprite = nullptr;
    inline SafetyHookInline spriteHook, constructHook;
    inline injector::hook_back<void(__fastcall*)(void*, void*)> nativeDraw;
    inline bool ready = false, seen = false, decoderReady = false;
    inline RECT picture{};
    inline std::atomic<uint32_t> outputSize{ 0 };
    struct Key
    {
        void* sprite;
        void* movie;
        uint32_t size;
        int mode;
    };
    inline Key applied{};

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
        if (!ready || !unit || !sprite || activeSprite.load() != sprite) return false;
        if (!Transform(quad, width.load(), height.load(), outputWidth, outputHeight, FixSettings().Int("MAIN.FMVWidescreenMode"))) return false;
        picture = { static_cast<LONG>(quad.x - quad.halfWidth), static_cast<LONG>(quad.y - quad.halfHeight),
            static_cast<LONG>(quad.x + quad.halfWidth), static_cast<LONG>(quad.y + quad.halfHeight) };
        return true;
    }

    // The native sprite rebuilds its quad only when marked dirty, so a new movie, framing or screen size marks it
    inline void __fastcall Draw(void* self, void* edx)
    {
        const auto current = *reinterpret_cast<uint8_t**>(static_cast<uint8_t*>(self) + 164);
        activeSprite.store(current);
        if (current)
        {
            const Key key{ current, handle.load(), outputSize.load(), FixSettings().Int("MAIN.FMVWidescreenMode") };
            if (std::memcmp(&key, &applied, sizeof(key)))
            {
                applied = key;
                current[44] |= 4;
            }
        }
        nativeDraw.fun(self, edx);
        seen = current != nullptr;
    }

    inline bool (*filter)(void*) = nullptr;

    inline void __fastcall Sprite(void* self, void*)
    {
        if (filter && !filter(self))
            return;
        const auto previous = sprite;
        sprite = self;
        spriteHook.thiscall<void>(self);
        sprite = previous;
    }

    inline void* __fastcall Construct(void* self, void*)
    {
        if (activeSprite.load() == self) activeSprite.store(nullptr);
        return constructHook.thiscall<void*>(self);
    }

    inline void Install()
    {
        auto pattern = hook::pattern("C7 40 28 05 00 00 00 EB 0B 8B 8D 54 FF FF FF E8 ? ? ? ? 8B 4D F4");
        auto spriteDraw = hook::pattern("64 A1 00 00 00 00 6A FF 68 ? ? ? ? 50 64 89 25 00 00 00 00 83 EC 44 53 56 8B F1 8A 5E 2C C0 EB 02 80 E3 01");
        auto construct = hook::pattern("6A FF ? ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 51 53 56 8B F1 57 89 74 24 0C E8 ? ? ? ? 33 DB C7 06 ? ? ? ? C7 86 84 00 00 00 0F 00 00 00");
        auto constructSite = construct.get_first();
        auto spriteSite = spriteDraw.get_first();
        auto drawSite = pattern.get_first(15);
        if (decoderReady)
        {
            constructHook = safetyhook::create_inline(constructSite, Construct);
            if (constructHook) spriteHook = safetyhook::create_inline(spriteSite, Sprite);
            if (spriteHook) nativeDraw.fun = injector::MakeCALL(drawSite, Draw).get();
            ready = nativeDraw.fun && spriteHook && constructHook;
        }
        if (!ready) OutputDebugStringA("Just Cause: movie drawing hook unavailable.\n");
    }
}
