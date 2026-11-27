module;

#include <stdafx.h>
#include <d3d9.h>
#include "FixSettings.hpp"

export module Frontend;

import ComVars;
import Hud;

namespace
{
    std::atomic<bool> Cinematic{ false };
    std::atomic<int> CutsceneBorders{ 3 };
    IDirect3DDevice9** GameDevice = nullptr;
    const uint8_t* RenderingMovie = nullptr;

    bool InCutscene()
    {
        return Cinematic.load(std::memory_order_relaxed) && !*RenderingMovie;
    }

    void DrawCutsceneBorders()
    {
        if (!InCutscene())
            return;
        const int mode = CutsceneBorders.load(std::memory_order_relaxed);
        if (!mode || KingKongInMenu())
            return;

        auto* device = *GameDevice;
        D3DVIEWPORT9 viewport{};
        if (!device || FAILED(device->GetViewport(&viewport)))
            return;

        const LONG x = viewport.X;
        const LONG y = viewport.Y;
        const LONG width = viewport.Width;
        const LONG height = viewport.Height;
        const LONG side = mode & 2 ? std::max(0L, (width - MulDiv(height, 4, 3)) / 2) : 0;
        const LONG band = mode & 1 ? height / 8 : 0;
        D3DRECT rects[4];
        DWORD count = 0;
        if (side)
        {
            rects[count++] = { x, y, x + side, y + height };
            rects[count++] = { x + width - side, y, x + width, y + height };
        }
        if (band)
        {
            rects[count++] = { x + side, y, x + width - side, y + band };
            rects[count++] = { x + side, y + height - band, x + width - side, y + height };
        }
        if (count)
            device->Clear(count, rects, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
    }

    void InitCutsceneBorders(IDirect3DDevice9** device)
    {
        GameDevice = device;
        const auto refresh = []()
        {
            static std::mutex mutex;
            const std::lock_guard lock(mutex);
            CutsceneBorders.store(std::clamp(FixSettings().Int("MAIN.CutsceneBorders"), 0, 3), std::memory_order_relaxed);
        };
        FixSettings().OnChange("MAIN.CutsceneBorders", refresh);
        refresh();

        auto movie = hook::pattern("C6 05 ? ? ? ? 01 B9 ? ? ? ? E8 ? ? ? ? 88 1D ? ? ? ? E8");
        RenderingMovie = *movie.get_first<const uint8_t*>(2);

        auto frame = hook::pattern("55 8B EC 83 EC 64 68 ? ? ? ? E8 ? ? ? ? 83 C4 04 E8 ? ? ? ? E8 ? ? ? ? 83 3D ? ? ? ? 00");
        static auto BeginFrame = safetyhook::create_mid(frame.get_first(), [](SafetyHookContext&)
        {
            Cinematic.store(false, std::memory_order_relaxed);
        });
        // Jack's cinematic timeline and the final mode selected by Kong's camera.
        auto jack = hook::pattern("55 8B EC 83 EC 0C A1 ? ? ? ? 8B 48 40 89 4D FC 8B 15 ? ? ? ? 89 55 F8 6A 00 68 00 01 00 00");
        static auto JackCinematic = safetyhook::create_mid(jack.get_first(), [](SafetyHookContext&)
        {
            Cinematic.store(true, std::memory_order_relaxed);
        });
        auto kong = hook::pattern("8B 4D F8 8B 91 F4 15 00 00 89 90 60 1A 00 00 8B 45 F8 8B 4D F8 8B 91 04 16 00 00");
        static auto KongCinematic = safetyhook::create_mid(kong.get_first(3), [](SafetyHookContext& regs)
        {
            constexpr int32_t CinematicCameraMode = 9;
            if (*reinterpret_cast<const int32_t*>(regs.ecx + 0x15F4) == CinematicCameraMode)
                Cinematic.store(true, std::memory_order_relaxed);
        });

        // Draw the masks after the scene and before the interface so subtitles can use the bars.
        auto draw = hook::pattern("8B 15 ? ? ? ? 39 5A 1C 74 0A B9 ? ? ? ? E8 ? ? ? ? 6A 04 8B CE E8 ? ? ? ? 39 1D");
        static auto Draw = safetyhook::create_mid(draw.get_first(21), [](SafetyHookContext&)
        {
            DrawCutsceneBorders();
        });
    }
}

export bool KingKongInCutscene()
{
    return InCutscene();
}

// Native aspect-ratio bars outside scripted cameras.
SafetyHookInline shsub_A2A350 = {};
void __fastcall sub_A2A350(void* _this, void* edx, int a2, int a3, void* a4)
{
    if (!KingKongInCutscene() && pBarsOwner && (*reinterpret_cast<uint32_t*>(pBarsOwner + 0xCC) == 3))
        return shsub_A2A350.unsafe_fastcall(_this, edx, a2, a3, a4);
}

// The settings menu's aspect ratio option shows what it does with the fix
struct TextHookGamersEdition
{
    void operator()(injector::reg_pack& regs)
    {
        char* label = szAspectRatioLabel.get_ptr();
        if (strncmp(label, "16/9", 4) == 0)
        {
            std::memcpy(label, "Bars", 4);
        }
        else
        {
            if (strncmp(label, "4/3", 3) == 0)
            {
                std::memcpy(label, "Std", 4);
            }
        }

        regs.ecx = *(uint16_t*)(regs.edi + 6);
        regs.edx = *(uint16_t*)(regs.edi + 4);
    }
};

struct TextHookSignatureEdition
{
    void operator()(injector::reg_pack& regs)
    {
        char* label = szAspectRatioLabel.get_ptr();
        if (strncmp(label, "16/9", 4) == 0)
        {
            std::memcpy(label, "Auto", 4);
        }
        else if (strncmp(label, "4/3", 4) == 0)
        {
            std::memcpy(label, "Std", 4);
        }
        else if (strncmp(label, "4/3 ", 4) == 0)
        {
            std::memcpy(label, "Black Bands", sizeof("Black Bands"));
        }

        regs.ecx = *(uint16_t*)(regs.edi + 6);
        regs.edx = *(uint16_t*)(regs.edi + 4);
    }
};

class Frontend
{
public:
    Frontend()
    {
        WFP::onInitEvent() += []()
        {
            auto text_pattern = hook::pattern("68 ? ? ? ? E8 ? ? ? ? 50 55 56 E8 ? ? ? ? 0F B7 4F 06 0F B7 57 04"); //0x992C4C
            szAspectRatioLabel.SetAddress(*text_pattern.count(1).get(0).get<char*>(1));

            auto pattern = find_pattern("A1 ? ? ? ? 8B 88 00 08 00 00 81 C1 B4 68 06 00", "A1 ? ? ? ? 89 45 FC 8B 0D ? ? ? ? 89 0D");
            pBarsOwner.SetAddress(*pattern.get_first<uintptr_t*>(1)); //0xCBFBE0

            if (!Screen.bSignatureEdition)
            {
                pattern = hook::pattern("8B 41 10 6A 00 50 B9 ? ? ? ? E8 ? ? ? ? 8B 4C 24 ? C7 01 00 00 00 C0 C2 0C 00");
                InitCutsceneBorders(*pattern.get_first<IDirect3DDevice9**>(7) + 1);
                shsub_A2A350 = safetyhook::create_inline(pattern.get_first(), sub_A2A350);
                injector::MakeInline<TextHookGamersEdition>(text_pattern.get_first(18), text_pattern.get_first(18 + 8));
            }
            else
            {
                injector::MakeInline<TextHookSignatureEdition>(text_pattern.get_first(18), text_pattern.get_first(18 + 8));
            }
        };
    }
} Frontend;
