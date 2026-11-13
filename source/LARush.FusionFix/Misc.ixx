module;
#include <stdafx.h>
#include <d3d9.h>
#include <wincrypt.h>
#include "FixSettings.hpp"
#pragma comment(lib, "advapi32.lib")

export module Misc;
import ComVars;
import DeveloperOverlay;
import Window;
import Camera;
import Hud;
import PadLabels;

namespace IntroSkip
{
    constexpr int32_t LastLogo{ 6 };
    SafetyHookInline openHook{};
    SafetyHookInline titleHook{};
    void (__cdecl* titleContinue)() = nullptr;
    void (__cdecl* titleIntro)() = nullptr;
    int skipTitle = 0;
    std::atomic<void*> introHandle{ nullptr };
    bool skipLogos = false;

    void __cdecl TitleInit()
    {
        titleHook.ccall<void>();
        if (skipTitle == 1)
        {
            // The intro is normally reached from the title's idle sequence. Play it
            // before skipping the prompt so this content remains available.
            skipTitle = 2;
            titleIntro();
        }
        else if (skipTitle == 2)
        {
            skipTitle = 0;
            titleContinue();
        }
    }

    bool KnownIntro(const char* path)
    {
        constexpr uint8_t expected[]{ 0xBC, 0xFA, 0xFF, 0x89, 0x9D, 0xD0, 0xAA, 0x45, 0xB4, 0xE7, 0xDE, 0x6A, 0x51, 0xFE, 0x4B, 0xDB,
            0x32, 0x91, 0x4F, 0x04, 0x04, 0x68, 0x91, 0x2D, 0xC9, 0x46, 0x81, 0x55, 0x0C, 0xCF, 0x38, 0x72 };
        const auto file = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
        if (file == INVALID_HANDLE_VALUE) return false;
        HCRYPTPROV provider{}; HCRYPTHASH hash{}; bool valid = false;
        if (CryptAcquireContextW(&provider, nullptr, nullptr, PROV_RSA_AES, CRYPT_VERIFYCONTEXT) && CryptCreateHash(provider, CALG_SHA_256, 0, 0, &hash))
        {
            std::array<uint8_t, 65536> block{}; DWORD count = 0; bool ok = true;
            while (ok)
            {
                if (!ReadFile(file, block.data(), static_cast<DWORD>(block.size()), &count, nullptr)) { ok = false; break; }
                if (!count) break;
                ok = CryptHashData(hash, block.data(), count, 0) != FALSE;
            }
            std::array<uint8_t, 32> digest{}; DWORD length = static_cast<DWORD>(digest.size());
            valid = ok && CryptGetHashParam(hash, HP_HASHVAL, digest.data(), &length, 0) && length == digest.size() && std::equal(digest.begin(), digest.end(), std::begin(expected));
        }
        if (hash) CryptDestroyHash(hash);
        if (provider) CryptReleaseContext(provider, 0);
        CloseHandle(file);
        return valid;
    }

    void* __stdcall OpenMovie(const char* path, uint32_t flags)
    {
        if (path && flags == 0)
        {
            const char* name = path;
            for (auto* p = path; *p; ++p)
                if (*p == '\\' || *p == '/') name = p + 1;
            if (skipLogos && pMovies.is_initialized())
            {
                for (int32_t i{ 0 }; i <= LastLogo; ++i)
                {
                    const auto length = std::strlen(pMovies.get_ptr()[i].name);
                    if (_strnicmp(name, pMovies.get_ptr()[i].name, length) == 0 && _stricmp(name + length, ".bik") == 0)
                        return nullptr;
                }
            }
            const bool knownIntro = _stricmp(name, "intro.bik") == 0 && KnownIntro(path);
            auto result = openHook.stdcall<void*>(path, flags);
            if (knownIntro) introHandle.store(result, std::memory_order_relaxed);
            else if (result == introHandle.load(std::memory_order_relaxed)) introHandle.store(nullptr, std::memory_order_relaxed);
            return result;
        }
        auto result = openHook.stdcall<void*>(path, flags);
        if (result && result == introHandle.load(std::memory_order_relaxed))
            introHandle.store(nullptr, std::memory_order_relaxed);
        return result;
    }

    void Install()
    {
        auto input = hook::pattern("6A 05 23 F1 E8 ? ? ? ? 83 C4 04 85 C0 75 04 85 F6 74 ? A1 ? ? ? ? C1 E0 04 8B 88");
        auto open = hook::pattern("FF 15 ? ? ? ? 3B C3 89 06 75 ? C7 86 ? ? ? ? 02 00 00 00");

        const auto* movies = reinterpret_cast<Movie*>(*input.get_first<uintptr_t>(30) - offsetof(Movie, skipMode));
        pMovies.SetAddress(movies);
        skipLogos = FixSettings().Bool("MAIN.SkipIntro");
        skipTitle = FixSettings().Bool("MAIN.SkipPressStart");
        auto title = hook::pattern("C7 05 ? ? ? ? 01 00 00 00 C7 05 ? ? ? ? 02 00 00 00 C7 05 ? ? ? ? 00 00 00 00 C3");
        auto proceed = hook::pattern("A1 ? ? ? ? 85 C0 C7 05 ? ? ? ? 00 00 00 00 0F 84 ? ? ? ? A1 ? ? ? ? 83 F8 64 72 ? 8D 48 FF EB ? 8B C8 40 A3 ? ? ? ? 83 F8 64 C7 04 8D ? ? ? ? 0F 00 00 00");
        auto intro = hook::pattern("A1 ? ? ? ? 85 C0 C7 05 ? ? ? ? 03 00 00 00 74 ? A1 ? ? ? ? 3D BC 00 00 00");
        titleContinue = reinterpret_cast<void(__cdecl*)()>(proceed.get_first());
        titleIntro = reinterpret_cast<void(__cdecl*)()>(intro.get_first());
        titleHook = safetyhook::create_inline(title.get_first(), TitleInit);
        // Native failed-open handling advances logo states without starting their movies.
        openHook = safetyhook::create_inline(**open.get_first<void**>(2), OpenMovie);
    }
}

namespace VideoCrop
{

    bool SetVerticalRange(bool crop)
    {
        auto buffer = Vertices.get();
        if (!buffer) return false; // The game creates it later on its first movie frame.
        void* memory = nullptr;
        if (SUCCEEDED(buffer->Lock(0, 96, &memory, 0)))
        {
            const float top = crop ? 48.0f / 480.0f : 0.0f;
            const float bottom = crop ? 432.0f / 480.0f : 1.0f;
            auto vertices = static_cast<uint8_t*>(memory);
            *reinterpret_cast<float*>(vertices + 16) = top;
            *reinterpret_cast<float*>(vertices + 40) = bottom;
            *reinterpret_cast<float*>(vertices + 64) = bottom;
            *reinterpret_cast<float*>(vertices + 88) = top;
            if (SUCCEEDED(buffer->Unlock())) return crop;
        }
        // A failed lock must not leave a previous movie's range on the shared buffer.
        buffer->Release();
        Vertices = nullptr;
        return false;
    }
}

void InitializeGame()
{
    hook::pattern pattern;
    auto& settings = FixSettings();
    asi_settings::LiveLink::Start();
    Screen.nWidth = settings.Int("MAIN.ResX");
    Screen.nHeight = settings.Int("MAIN.ResY");

    if (Screen.nWidth <= 0 || Screen.nHeight <= 0 || Screen.nWidth > 16384 || Screen.nHeight > 16384)
        std::tie(Screen.nWidth, Screen.nHeight) = GetDesktopRes();
    if (Screen.nWidth <= 0 || Screen.nHeight <= 0)
        std::tie(Screen.nWidth, Screen.nHeight) = std::make_tuple(640, 480);

    LARushWindow::Init(Screen.nWidth, Screen.nHeight);
    LARushCamera::Init(Screen.nWidth, Screen.nHeight);
    LARushHud::Init();
    LARushPadLabels::Init();

    Screen.fWidth = static_cast<float>(Screen.nWidth);
    Screen.fHeight = static_cast<float>(Screen.nHeight);
    Screen.fAspectRatio = Screen.fWidth / Screen.fHeight;
    Screen.nWidth43 = static_cast<uint32_t>(Screen.fHeight * (4.0f / 3.0f));
    Screen.fWidth43 = static_cast<float>(Screen.nWidth43);
    Screen.fHudOffset = ((480.0f * Screen.fAspectRatio) - 640.0f) / 2.0f;
    Screen.fHudOffsetReal = (Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f;
    Screen.fHudScale = 1.0f / (((4.0f / 3.0f)) / (Screen.fAspectRatio));

    //Registry
    pattern = hook::pattern("E8 ? ? ? ? 6A 20 E8 ? ? ? ? 8B E8 85 ED 89 6B 38");
    struct RegHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *(uint32_t*)(regs.ecx + 0x37C) = 1;   // MovieInstalled
            *(uint32_t*)(regs.ecx + 0x380) = 0;   // IsCDROMVersion
            *(uint32_t*)(regs.ecx + 0x384) = 0;   // MovieCDNo
        }
    }; injector::MakeInline<RegHook>(pattern.get_first(0)); //0x54160C

    //Resolution
    pattern = hook::pattern("8B 44 24 04 85 C0 75 ? 8B 44 24");
    struct ResHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *(int32_t*)(regs.esp + 0x08) = Screen.nWidth;
            *(int32_t*)(regs.esp + 0x0C) = Screen.nHeight;
        }
    }; injector::MakeInline<ResHook>(pattern.get_first(6), pattern.get_first(28)); //0x53F6D6

    pattern = hook::pattern("89 BB 5C 03 00 00");
    struct ResHook2
    {
        void operator()(injector::reg_pack& regs)
        {
            *(int32_t*)(regs.ebx + 0x35C) = Screen.nWidth;
            *(int32_t*)(regs.ebx + 0x360) = Screen.nHeight;
        }
    }; injector::MakeInline<ResHook2>(pattern.get_first(0), pattern.get_first(6)); //0x53F6D6

    //Aspect Ratio
    pattern = hook::pattern("68 ? ? ? ? 8B CE E8 ? ? ? ? 51 D9 1C 24");
    injector::WriteMemory<float>(pattern.get_first<float>(1), Screen.fAspectRatio, true);
    static auto projectionAspect = safetyhook::create_mid(pattern.get_first(5), [](SafetyHookContext& context)
    {
        const auto camera = reinterpret_cast<uint8_t*>(context.esi);
        if (camera == LARushCamera::BorderedCamera)
            *reinterpret_cast<float*>(context.esp) = *reinterpret_cast<float*>(camera + 452);
    });

    pattern = hook::pattern("F3 0F 10 05 ? ? ? ? 8B C7 99");
    injector::WriteMemory<float>(*pattern.get_first<float*>(4), Screen.fAspectRatio, true);

    static auto ret_551F3B = (uint32_t)hook::get_pattern("83 C6 28 4D 75 F2 5F 5D 5E B8 01 00 00 00 5B 81 C4 98 00 00 00 C2 0C 00", 0);
    static auto ret_550D4F = reinterpret_cast<uint32_t>(hook::pattern("C3 A1 ? ? ? ? 81 EC 80 00 00 00 53 56 57 6A 03 6A 01 6A 00 8B F1 8B 08 50 FF 91").get_first());
    static auto ret_4F7EB4 = reinterpret_cast<uint32_t>(hook::pattern("8B 54 24 2C 83 C4 1C 45 83 C2 04 83 EB 0A 83 FD 03 89 54 24 10").get_first());
    pattern = hook::pattern("F3 0F 11 56 ? F3 0F 11 4E ? E8 ? ? ? ? 8B E8 85 ED");
    struct TextHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *(float*)(regs.esi + 0x0C) = regs.xmm2.f32[0];
            *(float*)(regs.esi + 0x10) = regs.xmm1.f32[0];

            auto ret = *(uint32_t*)(regs.esp + 0x28);
            if (LARushHud::DrawingMinimap)
            {
                // Laid out in the minimap's own viewport.
            }
            else if (LARushHud::DrawingSpeedometer)
            {
                LARushHud::TransformSpeedometerText(reinterpret_cast<float*>(regs.esi + 4), LARushHud::DrawingMeter);
            }
            else if (ret == ret_550D4F && *reinterpret_cast<uint32_t*>(regs.esp + 0x30) == ret_4F7EB4)
            {
                // Three digits drawn by the same upper-right meter use a nested text path.
                LARushHud::TransformSpeedometerText(reinterpret_cast<float*>(regs.esi + 4), false);
            }
            else if (ret == ret_551F3B)
            {
                *(float*)(regs.esi + 0x04) /= Screen.fHudScale;
                *(float*)(regs.esi + 0x08) /= Screen.fHudScale;
                if (LARushHud::IsMoney(regs.esi))
                    *(float*)(regs.esi + 0x04) += -1.0f + 1.0f / Screen.fHudScale + 2.0f * LARushHud::BoxLeft() / Screen.fWidth;
                else if (LARushHud::IsRace(regs.esi))
                {
                    auto& x = *reinterpret_cast<float*>(regs.esi + 0x04);
                    const float offset = 1.0f - 1.0f / Screen.fHudScale - 2.0f * LARushHud::BoxLeft() / Screen.fWidth;
                    x += x < 0.0f ? -offset : offset;
                }
            }
            D3DVIEWPORT9 viewport{};
            if (LARushHud::LowriderViewport(viewport))
                LARushHud::ConvertTextViewport(reinterpret_cast<float*>(regs.esi + 4), viewport, LARushHud::IsLowrider(regs.esi));
            else if (LARushHud::IsLowrider(regs.esi))
            {
                auto& x = *reinterpret_cast<float*>(regs.esi + 4);
                const float edge = std::max(0.0f, 1.0f - 1.0f / Screen.fHudScale - 2.0f * LARushHud::BoxLeft() / Screen.fWidth);
                x += x < 0.0f ? -edge : edge;
            }
            if (LARushHud::DrawingLowrider && ret == ret_550D4F && LARushWindow::GameState == 56)
            {
                auto& x = *reinterpret_cast<float*>(regs.esi + 4);
                const float width = *reinterpret_cast<float*>(regs.esi + 8);
                const float center = (x + width * 0.5f) / Screen.fHudScale;
                const float edge = std::max(0.0f, 1.0f - 1.0f / Screen.fHudScale - 2.0f * LARushHud::BoxLeft() / Screen.fWidth);
                x = center - width * 0.5f + (center < 0.0f ? -edge : edge);
            }
        }
    }; injector::MakeInline<TextHook>(pattern.get_first(0), pattern.get_first(10)); //0x5504BA

    static auto ret_4ED9E4 = (uint32_t)hook::get_pattern("83 C4 04 8B 44 24 10 8B 0D ? ? ? ? 40 3B C1 89 44 24 10", 0);
    static auto ret_4D25FE = (uint32_t)hook::get_pattern("83 C4 04 5F 5B 83 C4 54", 0);
    static auto ret_4F62A6 = (uint32_t)hook::get_pattern("83 C4 40 5E 5D 83 C4 6C C3", 0);
    static auto ret_4ED33B = (uint32_t)hook::get_pattern("A1 ? ? ? ? 8B 0D ? ? ? ? 83 C4 04 3B C1 74 4B D9 05", 0);
    static auto ret_4FA384 = (uint32_t)hook::get_pattern("83 C4 34 5F 5D 5B 59 C2 08 00", 0);
    static auto ret_4EF78F = (uint32_t)hook::get_pattern("8B 44 24 34 83 C4 18 40 83 C7 02 83 F8 09", 0);
    pattern = hook::pattern("0F B6 46 42 0F B7 4E 40");
    struct BackgroundGraphicsHook
    {
        void operator()(injector::reg_pack& regs)
        {
            auto ret = *(uint32_t*)(regs.esp + 0x38);
            if (LARushHud::DrawingSpeedometer)
            {
                const float scale = LARushHud::Scale();
                const float offset = 2.0f * LARushHud::BoxLeft() / Screen.fWidth;
                for (int i = 0; i < 4; ++i)
                {
                    auto& x = *reinterpret_cast<float*>(regs.esi + 0x10 + i * 4);
                    auto& y = *reinterpret_cast<float*>(regs.esi + 0x20 + i * 4);
                    x = 1.0f - offset - (1.0f - x) * scale;
                    y = -1.0f + (y + 1.0f) * scale;
                }
            }
            if (!LARushHud::DrawingMinimap && !LARushHud::DrawingSpeedometer && !LARushHud::DrawingLowrider && ret != ret_4ED9E4 && ret != ret_4D25FE && ret != ret_4F62A6 && ret != ret_4ED33B && ret != ret_4FA384 && ret != ret_4EF78F)
            {
                *(float*)(regs.esi + 0x10) /= Screen.fHudScale;
                *(float*)(regs.esi + 0x14) /= Screen.fHudScale;
                *(float*)(regs.esi + 0x18) /= Screen.fHudScale;
                *(float*)(regs.esi + 0x1C) /= Screen.fHudScale;
            }
            D3DVIEWPORT9 viewport{};
            if (LARushHud::LowriderViewport(viewport))
            {
                const float xRatio = viewport.Width / Screen.fWidth;
                const float yRatio = viewport.Height / Screen.fHeight;
                const float xCenter = (2.0f * viewport.X + viewport.Width) / Screen.fWidth - 1.0f;
                const float yCenter = 1.0f - (2.0f * viewport.Y + viewport.Height) / Screen.fHeight;
                for (int i = 0; i < 4; ++i)
                {
                    auto& x = *reinterpret_cast<float*>(regs.esi + 0x10 + i * 4);
                    auto& y = *reinterpret_cast<float*>(regs.esi + 0x20 + i * 4);
                    x = xCenter + x * xRatio;
                    y = yCenter + y * yRatio;
                }
            }
            if (LARushHud::DrawingLowrider && LARushWindow::GameState == 56)
            {
                const float inset = 2.0f * LARushHud::BoxLeft() / Screen.fWidth;
                auto vertices = reinterpret_cast<float*>(regs.esi + 0x10);
                const float center = (vertices[0] + vertices[1] + vertices[2] + vertices[3]) * 0.25f;
                for (int i = 0; i < 4; ++i)
                    vertices[i] += center < 0.0f ? inset : -inset;
            }

            *(uint16_t*)&regs.ecx = *(uint16_t*)(regs.esi + 0x40);
            *(uint8_t*)&regs.eax = *(uint8_t*)(regs.esi + 0x42);
        }
    }; injector::MakeInline<BackgroundGraphicsHook>(pattern.get_first(0), pattern.get_first(8)); //550082

    //Mouse
    static auto ret_4C74E8 = (uint32_t)hook::get_pattern("A1 ? ? ? ? 8B 0D ? ? ? ? F3 0F 10 05 ? ? ? ? 50 51 8D 54 24 40 F3 0F 11 44 24", 0);
    static auto ret_4C751D = (uint32_t)hook::get_pattern("83 C4 14 E9 ? ? ? ? 3B DF 75 43 F3 0F 10 44 24 ? F3 0F 58 44 24", 0);
    static auto ret_4F6CFC = (uint32_t)hook::get_pattern("68 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? F3 0F 10 44 24 ? F3 0F 58 44 24", 0);
    pattern = hook::pattern("D9 58 0C DD D8 DD D8 83 C4 5C C3");
    struct MouseHook
    {
        void operator()(injector::reg_pack& regs)
        {
            if (*(uint32_t*)regs.esp != ret_4C74E8 && *(uint32_t*)regs.esp != ret_4C751D && *(uint32_t*)regs.esp != ret_4F6CFC)
            {
                *(float *)(regs.eax + 0x0) /= Screen.fHudScale;
                *(float *)(regs.eax + 0x0) += Screen.fHudOffsetReal / Screen.fWidth;
                *(float *)(regs.eax + 0x8) /= Screen.fHudScale;
            }
        }
    }; injector::MakeInline<MouseHook>(pattern.get_first(10)); //4BF734
    injector::MakeRET(pattern.get_first(10 + 5));

    auto movieVertices = hook::pattern("8B 15 ? ? ? ? A1 ? ? ? ? 8B 08 6A 18 55 52 55 50");
    VideoCrop::Vertices.SetAddress(*movieVertices.get_first<IDirect3DVertexBuffer9**>(2));

    // Fit the entire decoded picture inside either the original 4:3 box or the output.
    static int movieHeight = 0;
    static int movieTop = 0;
    pattern = hook::pattern("E8 ? ? ? ? 89 44 24 60 E8 ? ? ? ? F3 0F 10 05");
    struct FMVHook
    {
        void operator()(injector::reg_pack& regs)
        {
            const auto movie = reinterpret_cast<const uint32_t*>(regs.esi);
            const float sourceWidth = static_cast<float>(movie[4]);
            float sourceHeight = static_cast<float>(movie[5]);
            const bool original = std::clamp(FixSettings().Int("MAIN.FMVWidescreenMode"), 0, 1) == 0;
            const auto intro = IntroSkip::introHandle.load(std::memory_order_relaxed);
            const bool wantCrop = !original && intro && movie[0] == reinterpret_cast<uintptr_t>(intro) && movie[4] == 640 && movie[5] == 480;
            if (VideoCrop::SetVerticalRange(wantCrop)) sourceHeight = 384.0f;
            const float movieAspect = sourceWidth > 0.0f && sourceHeight > 0.0f ? sourceWidth / sourceHeight : 4.0f / 3.0f;
            const float boxWidth = original ? std::min(Screen.fWidth, Screen.fHeight * (4.0f / 3.0f)) : Screen.fWidth;
            const float boxHeight = original ? std::min(Screen.fHeight, Screen.fWidth / (4.0f / 3.0f)) : Screen.fHeight;
            const auto width = std::max(1, static_cast<int>(std::lround(std::min(boxWidth, boxHeight * movieAspect))));
            movieHeight = std::max(1, static_cast<int>(std::lround(width / movieAspect)));
            movieTop = (Screen.nHeight - movieHeight) / 2;
            *reinterpret_cast<int32_t*>(regs.esp + 0x58) = (Screen.nWidth - width) / 2;
            regs.eax = width;
            LARushCamera::MovieFrame = true;
        }
    }; injector::MakeInline<FMVHook>(pattern.get_first(0)); //0x572C6F

    struct FMVHeightHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *reinterpret_cast<int32_t*>(regs.esp + 0x5C) = movieTop;
            regs.eax = movieHeight;
        }
    }; injector::MakeInline<FMVHeightHook>(pattern.get_first(9));

    // The scene is copied to the screen with texture coordinates on texel corners, so the left column and the top
    // row blend in the opposite edge. Sample the texel centres instead.
    pattern = hook::pattern("8B 7C 24 0C B9 18 00 00 00 8D 74 24 50 F3 A5");
    static auto sceneCopy = safetyhook::create_mid(pattern.get_first(4), [](SafetyHookContext& context)
    {
        auto vertices = reinterpret_cast<float*>(context.esp + 0x50);
        for (int i = 0; i < 4; ++i)
        {
            vertices[i * 6 + 3] += 0.5f / Screen.fWidth;
            vertices[i * 6 + 4] += 0.5f / Screen.fHeight;
        }
    });

    // Install after resolving the native return address at the same epilogue.
    LARushHud::InstallSpeedometer();

    IntroSkip::Install();
}


class Misc
{
public:
    Misc()
    {
        WFP::onInitEvent() += []()
        {
            InitializeGame();
        };
    }
} Misc;
