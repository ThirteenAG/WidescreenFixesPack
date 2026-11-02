module;
#include <stdafx.h>
#include <intrin.h>
#include "FixSettings.hpp"

export module Movies;
import ComVars;


export namespace OnimushaMovies
{
    inline std::atomic<float> ContentAspect{ 4.0f / 3.0f };
    inline SafetyHookInline PlayHook, ProgressHook;
    inline const void* StartupReturn = nullptr;
    inline bool SkipStartup = false;
    inline std::atomic<bool> SkipLogo{ false };

    inline float PictureAspect(uint16_t resource)
    {
        switch (resource)
        {
            case 1233:
            case 1211:
                return 640.0f / 352.0f;
            case 1251:
            case 1239:
            case 1207:
            case 1223:
            case 1224:
            case 1234:
                return 640.0f / 384.0f;
            default:
                return 4.0f / 3.0f;
        }
    }

    inline int __fastcall Play(int movie, void*)
    {
        // The PC movie files are 640x480 with square pixels. EPILOGUE and MOV408B have a
        // 640x352 picture; the other six have a 640x384 picture. The remaining rows are baked black bars.
        const auto resource = Entries.get_ptr()[3 * movie];
        ContentAspect = PictureAspect(resource);
        const auto previous = SkipLogo.exchange(SkipStartup && _ReturnAddress() == StartupReturn);
        const auto result = PlayHook.unsafe_fastcall<int>(movie, nullptr);
        SkipLogo = previous;
        return result;
    }

    inline int __stdcall Progress(uint32_t* player)
    {
        if (SkipLogo.load(std::memory_order_relaxed))
        {
            // Use the native end state, retaining the player's setup and cleanup.
            *player = 2;
            return 1;
        }
        return ProgressHook.unsafe_stdcall<int>(player);
    }

    inline void Init()
    {
        auto play = hook::pattern("55 56 57 68 FF FF FF 00 8B F9 68 FF FF FF 00 B9 ? ? ? ? E8");
        auto entries = hook::pattern("0F B7 BE ? ? ? ? 0D 00 00 00 40 68 80 01 00 00 A3 ? ? ? ? 66 8B 86");
        SkipStartup = FixSettings().Bool("MAIN.SkipIntro");
        StartupReturn = hook::pattern("B9 13 00 00 00 E8 ? ? ? ? 6A 01 B9 ? ? ? ? E8 ? ? ? ? C6 46 01 01 C6 46 02 00").get_first(10);
        Entries.SetAddress(*entries.get_first<uint16_t*>(3));
        ProgressHook = safetyhook::create_inline(hook::pattern("83 EC 20 55 8B 6C 24 28 56 B9 ? ? ? ? 8D 75 10 E8 ? ? ? ? 85 C0 74 14").get_first(), Progress);
        PlayHook = safetyhook::create_inline(play.get_first(), Play);
    }
}
