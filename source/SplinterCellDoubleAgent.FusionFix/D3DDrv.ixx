module;

#include "stdafx.h"
#include "FixSettings.hpp"
#include <d3d9.h>
#include <D3dx9.h>
#pragma comment(lib, "D3dx9.lib")

export module D3DDrv;

import ComVars;
import Window;
import PostFX;
import Shaders;

injector::hook_back<void* (__fastcall*)(float*, void*, uintptr_t, float, float, float, float, float, float, float, float)> hbsub_10077470;
void* __fastcall VideoTopOverlay(float* _this, void* edx, uintptr_t ctx, float x, float y, float w, float h, float u1, float v1, float u2, float v2)
{
    auto width = static_cast<float>(*(int*)(ctx + 0x5A08));
    auto height = static_cast<float>(*(int*)(ctx + 0x5A0C));

    auto verticalOffset = ((height * ((width / height) / (4.0f / 3.0f))) - height) / 2.0f;
    y -= verticalOffset;

    return hbsub_10077470.fun(_this, edx, ctx, x, y, w, h, u1, v1, u2, v2);
}

void* __fastcall VideoBottomOverlay(float* _this, void* edx, uintptr_t ctx, float x, float y, float w, float h, float u1, float v1, float u2, float v2)
{
    auto width = static_cast<float>(*(int*)(ctx + 0x5A08));
    auto height = static_cast<float>(*(int*)(ctx + 0x5A0C));

    auto verticalOffset = ((height * ((width / height) / (4.0f / 3.0f))) - height) / 2.0f;
    h += verticalOffset;

    return hbsub_10077470.fun(_this, edx, ctx, x, y, w, h, u1, v1, u2, v2);
}

void* __fastcall VideoBackground(float* _this, void* edx, uintptr_t ctx, float x, float y, float w, float h, float u1, float v1, float u2, float v2)
{
    return hbsub_10077470.fun(_this, edx, ctx, x, y, w, h, u1, v1, u2, v2);
}

void* __fastcall VideoContent(float* _this, void* edx, uintptr_t ctx, float x, float y, float w, float h, float u1, float v1, float u2, float v2)
{
    return hbsub_10077470.fun(_this, edx, ctx, x, y, w, h, u1, v1, u2, v2);
}

namespace SuitLights
{
    const wchar_t* (__thiscall* GetObjectName)(void* object) = nullptr;
    ptrdiff_t DeviceOffset = 0; // UD3DRenderDevice: IDirect3DDevice9*
    bool GlowPassGoggles = false;

    bool HasColor()
    {
        return gColor >= 1 && gColor <= SuitLightColors.size();
    }

    // SCX_glow_material: Hue and Saturation bytes
    bool IsGoggles(uintptr_t material)
    {
        return *reinterpret_cast<uint8_t*>(material + 0x51) == 0x41 && *reinterpret_cast<uint8_t*>(material + 0x52) == 0x33;
    }

    // The suit's visibility light, whose hue the game switches between green and yellow
    bool IsSuitLight(uintptr_t material)
    {
        return IsGoggles(material) || wcscmp(GetObjectName(reinterpret_cast<void*>(material)), L"GlowWhite3_SC4") == 0;
    }

    // The goggle lens texture is yellow-green with no blue, so the tint alone can't reach blue, pink, violet or white.
    // A copy that keeps each texel's brightest channel takes any colour.
    IDirect3DTexture9* NeutralTexture(IDirect3DDevice9* device)
    {
        static std::unordered_map<IDirect3DBaseTexture9*, IDirect3DTexture9*> copies;

        IDirect3DBaseTexture9* current = nullptr;
        device->GetTexture(0, &current);
        if (!current)
            return nullptr;
        current->Release();
        if (auto it = copies.find(current); it != copies.end())
            return it->second;

        IDirect3DTexture9*& neutral = copies[current];
        D3DSURFACE_DESC desc = {};
        auto texture = static_cast<IDirect3DTexture9*>(current);
        if (current->GetType() != D3DRTYPE_TEXTURE || FAILED(texture->GetLevelDesc(0, &desc)))
            return nullptr;
        if (FAILED(device->CreateTexture(desc.Width, desc.Height, texture->GetLevelCount(), 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &neutral, nullptr)))
            return nullptr;

        IDirect3DSurface9* from = nullptr;
        IDirect3DSurface9* to = nullptr;
        texture->GetSurfaceLevel(0, &from);
        neutral->GetSurfaceLevel(0, &to);
        bool copied = from && to && SUCCEEDED(D3DXLoadSurfaceFromSurface(to, nullptr, nullptr, from, nullptr, nullptr, D3DX_FILTER_NONE, 0));
        if (from)
            from->Release();
        if (to)
            to->Release();

        D3DLOCKED_RECT rect = {};
        if (copied && SUCCEEDED(neutral->LockRect(0, &rect, nullptr, 0)))
        {
            for (UINT y = 0; y < desc.Height; y++)
            {
                auto row = reinterpret_cast<uint32_t*>(static_cast<uint8_t*>(rect.pBits) + y * rect.Pitch);
                for (UINT x = 0; x < desc.Width; x++)
                {
                    const uint32_t c = row[x];
                    const uint32_t m = (std::max)({ (c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF });
                    row[x] = (c & 0xFF000000) | (m << 16) | (m << 8) | m;
                }
            }
            neutral->UnlockRect(0);
            if (neutral->GetLevelCount() > 1)
                D3DXFilterTexture(neutral, nullptr, 0, D3DX_DEFAULT);
        }
        else
        {
            neutral->Release();
            neutral = nullptr;
        }
        return neutral;
    }

    // Runs on the game's HSV result for a glow material (base pass and glow pass)
    void Apply(float* color, uintptr_t material)
    {
        if (!IsSuitLight(material))
            return;

        if (HasColor())
        {
            // Keep the game's brightness (distance amplification, glow pass), replace the colour
            const float brightness = (std::max)({ color[0], color[1], color[2] });
            const auto rgb = SuitLightColors[gColor - 1];
            color[0] = static_cast<float>((rgb >> 16) & 0xFF) / 255.0f * brightness;
            color[1] = static_cast<float>((rgb >> 8) & 0xFF) / 255.0f * brightness;
            color[2] = static_cast<float>(rgb & 0xFF) / 255.0f * brightness;
        }

        if (gBlacklistIndicators)
        {
            const float visibility = gVisibility.load(std::memory_order_relaxed);
            for (int i = 0; i < 4; i++)
                color[i] *= visibility;
        }
    }
}

int nGlowResolutionMultiplier = 0;
SafetyHookInline shsub_100B9860 = {};
void* __fastcall sub_100B9860(void* _this, void* edx, int a2, int a3, int a4, int a5, int a6)
{
    if (edx != (void*)0xC0DE)
    {
        a3 *= nGlowResolutionMultiplier;
        a4 *= nGlowResolutionMultiplier;
    }
    return shsub_100B9860.unsafe_fastcall<void*>(_this, edx, a2, a3, a4, a5, a6);
}

export void InitD3DDrv()
{

    static auto nShadowMapResolutionMultiplier = std::clamp(FixSettings().Int("GRAPHICS.ShadowMapResolutionMultiplier"), 0, 16);
    nGlowResolutionMultiplier = std::clamp(FixSettings().Int("GRAPHICS.GlowResolutionMultiplier"), 0, 16);
    static auto nMirrorResolutionMultiplier = std::clamp(FixSettings().Int("GRAPHICS.MirrorResolutionMultiplier"), 0, 16);

    if (nShadowMapResolutionMultiplier)
    {
        const std::pair<const char*, uint32_t> GShadowResolutionDegreeXrefs[] =
        {
            { "B8 00 04 00 00 D3 F8 80 BE", 1024 },
            { "B8 00 04 00 00 D3 F8 8B 8E ? ? ? ? 8B 11 8D 8E", 1024 },
            { "B8 00 02 00 00 D3 F8 8B 8E ? ? ? ? 8B 09 6A 01", 512 },
            { "B8 00 02 00 00 D3 F8 8D 8E ? ? ? ? 51 8B 8E", 512 },
            { "B8 00 04 00 00 D3 F8 8B 8E ? ? ? ? 8B 11 6A 01", 1024 },
            { "B8 00 02 00 00 D3 F8 8B 8E ? ? ? ? 8B 09 6A 50", 512 },
            { "B8 00 02 00 00 D3 F8 8D 8E ? ? ? ? 51 6A 00", 512 },
            { "B8 00 02 00 00 D3 F8 8B 8E ? ? ? ? 8B 11 6A 01 50 50 51 FF 52 ? 8B 03", 512 },
            { "B8 00 02 00 00 D3 F8 8B 8E ? ? ? ? 8B 11 6A 01 50 50 51 FF 52 ? 8B 1B", 512 },
            { "B8 00 02 00 00 EB 11", 512 },
            { "B8 00 04 00 00 D3 F8 C7 44 24", 1024 },
        };

        for (const auto& [signature, baseResolution] : GShadowResolutionDegreeXrefs)
        {
            auto pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), signature);
            injector::WriteMemory<uint32_t>(pattern.get_first<uint32_t>(1), baseResolution * nShadowMapResolutionMultiplier, true);
        }
    }

    if (nGlowResolutionMultiplier)
    {
        auto pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "6A FF 68 ? ? ? ? 64 A1 ? ? ? ? 50 64 89 25 ? ? ? ? 51 56 8B F1 33 C0");
        shsub_100B9860 = safetyhook::create_inline(pattern.get_first(), sub_100B9860);

        static injector::hook_back<void(__fastcall*)(void* _this, void* edx, int a2, int a3, int a4, int a5, int a6)> shsub_100B9860;
        auto sub_100B9860_hook = [](void* _this, void* edx, int a2, int a3, int a4, int a5, int a6)
        {
            return shsub_100B9860.fun(_this, (void*)0xC0DE, a2, a3, a4, a5, a6);
        };

        const char* xrefs_arr[] = {
            "E8 ? ? ? ? EB 02 33 C0 89 86 ? ? ? ? A1 ? ? ? ? 8B 08 8B 11 68 ? ? ? ? 83 CB FF",
            "E8 ? ? ? ? EB 02 33 C0 6A 00 89 5C 24 ? 8D 9E ? ? ? ? 53 6A 00 6A 71",
        };

        for (auto& xref : xrefs_arr)
        {
            auto pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), xref);
            shsub_100B9860.fun = injector::MakeCALL(pattern.get_first(), static_cast<void(__fastcall*)(void* _this, void* edx, int a2, int a3, int a4, int a5, int a6)>(sub_100B9860_hook), true).get();
        }

        pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "8D 4C 24 ? FF 15 ? ? ? ? 8B 54 24");
        static auto GlowResolutionHook0 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            *(float*)(regs.esp + 0x3C) = 1.0f + (1.0f / Screen.fWidth);
            *(float*)(regs.esp + 0x38) = 1.0f / Screen.fWidth;
            *(float*)(regs.esp + 0x2C) = 1.0f + (1.0f / (64.0f * (float)nGlowResolutionMultiplier));
            *(float*)(regs.esp + 0x28) = 1.0f / (64.0f * (float)nGlowResolutionMultiplier);
            *(float*)(regs.esp + 0x1C) = 1.0f + (1.0f / (128.0f * (float)nGlowResolutionMultiplier));
            *(float*)(regs.esp + 0x18) = 1.0f / (128.0f * (float)nGlowResolutionMultiplier);
            *(float*)(regs.esp + 0x0C) = 1.0f + (1.0f / (256.0f * (float)nGlowResolutionMultiplier));
            *(float*)(regs.esp + 0x08) = 1.0f / (256.0f * (float)nGlowResolutionMultiplier);
        });

        static auto increaseGlowRes = [](float* ptr)
        {
            *(int*)&ptr[1] *= nGlowResolutionMultiplier;
            //ptr[3]; // res x
            //ptr[4]; // res y

            *(int*)&ptr[10] *= nGlowResolutionMultiplier;
            ptr[12] *= (float)nGlowResolutionMultiplier;
            ptr[13] *= (float)nGlowResolutionMultiplier;

            *(int*)&ptr[19] *= nGlowResolutionMultiplier;
            ptr[21] *= (float)nGlowResolutionMultiplier;
            ptr[22] *= (float)nGlowResolutionMultiplier;

            *(int*)&ptr[28] *= nGlowResolutionMultiplier;
            ptr[30] *= (float)nGlowResolutionMultiplier;
            ptr[31] *= (float)nGlowResolutionMultiplier;

            *(int*)&ptr[37] *= nGlowResolutionMultiplier;
            ptr[39] *= (float)nGlowResolutionMultiplier;
            ptr[40] *= (float)nGlowResolutionMultiplier;
        };

        pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "50 53 57 8B CE");
        static auto GlowResolutionHook1 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            increaseGlowRes(reinterpret_cast<float*>(regs.eax));
        });

        pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "52 53 57 8B CE");
        static auto GlowResolutionHook2 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            increaseGlowRes(reinterpret_cast<float*>(regs.edx));
        });

        pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "52 F3 0F 11 84 24 ? ? ? ? F3 0F 11 84 24");
        static auto GlowResolutionHook3 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            increaseGlowRes(reinterpret_cast<float*>(regs.edx));
        });

        pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "8D 8C 24 ? ? ? ? 89 BC 24 ? ? ? ? FF 15 ? ? ? ? 8B 54 24");
        static auto VisionResolutionHook0 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            *(float*)(regs.esp + 0x3C) = 1.0f + (1.0f / Screen.fWidth);
            *(float*)(regs.esp + 0x38) = 1.0f / Screen.fWidth;
            *(float*)(regs.esp + 0x2C) = 1.0f + (1.0f / (64.0f * (float)nGlowResolutionMultiplier));
            *(float*)(regs.esp + 0x28) = 1.0f / (64.0f * (float)nGlowResolutionMultiplier);
            *(float*)(regs.esp + 0x1C) = 1.0f + (1.0f / (128.0f * (float)nGlowResolutionMultiplier));
            *(float*)(regs.esp + 0x18) = 1.0f / (128.0f * (float)nGlowResolutionMultiplier);
            *(float*)(regs.esp + 0x0C) = 1.0f + (1.0f / (256.0f * (float)nGlowResolutionMultiplier));
            *(float*)(regs.esp + 0x08) = 1.0f / (256.0f * (float)nGlowResolutionMultiplier);
        });

        pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "8B CE FF 92 ? ? ? ? 8B 8E ? ? ? ? 8B 89 ? ? ? ? E8 ? ? ? ? 8B 96");
        static auto VisionResolutionHook1 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            *(int*)(regs.esp + 0x0C) *= nGlowResolutionMultiplier;
            *(int*)(regs.esp + 0x08) *= nGlowResolutionMultiplier;
        });

        pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "8B CE FF 92 ? ? ? ? 8B 8E ? ? ? ? 8B 89 ? ? ? ? E8 ? ? ? ? 83 7D ? ? 0F 84 ? ? ? ? 8B BE ? ? ? ? F3 0F 10 05");
        static auto VisionResolutionHook2 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            *(int*)(regs.esp + 0x0C) *= nGlowResolutionMultiplier;
            *(int*)(regs.esp + 0x08) *= nGlowResolutionMultiplier;
        });

        pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "8B CE FF 92 ? ? ? ? 8B 8E ? ? ? ? 8B 89 ? ? ? ? E8 ? ? ? ? 83 7D ? ? 75");
        static auto VisionResolutionHook3 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            *(int*)(regs.esp + 0x0C) *= nGlowResolutionMultiplier;
            *(int*)(regs.esp + 0x08) *= nGlowResolutionMultiplier;
        });

        pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "8B CE FF 92 ? ? ? ? 8B 8E ? ? ? ? 8B 89 ? ? ? ? E8 ? ? ? ? 83 7D ? ? 0F 84 ? ? ? ? 8B BE ? ? ? ? 0F 57 C0");
        static auto VisionResolutionHook4 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            *(int*)(regs.esp + 0x0C) *= nGlowResolutionMultiplier;
            *(int*)(regs.esp + 0x08) *= nGlowResolutionMultiplier;
        });

        pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "8B CE FF 92 ? ? ? ? 8B 8E ? ? ? ? 8B 89 ? ? ? ? E8 ? ? ? ? 80 7C 24");
        static auto VisionResolutionHook5 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            *(int*)(regs.esp + 0x0C) *= nGlowResolutionMultiplier;
            *(int*)(regs.esp + 0x08) *= nGlowResolutionMultiplier;
        });
    }

    //Minimap aka FilmStrip
    auto pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "8B 4B 04 03 C9 85 C9 89 4C 24 10"); //?RenderFilmstrip@UD3DRenderDevice@@UAE_NXZ + 0x38C
    struct RenderFilmstrip_Hook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.ecx = Screen.FilmstripScaleX + Screen.FilmstripScaleX;
        }
    }; injector::MakeInline<RenderFilmstrip_Hook>(pattern.get_first(0));

    pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "03 D2 85 D2 89 54 24 10 DB 44 24 10"); //?RenderFilmstrip@UD3DRenderDevice@@UAE_NXZ + 0x350
    struct RenderFilmstrip_Hook2
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.edx = Screen.FilmstripOffset;
            *(int32_t*)(regs.esp + 0x10) = regs.edx;
        }
    }; injector::MakeInline<RenderFilmstrip_Hook2>(pattern.get_first(0), pattern.get_first(6));
    injector::WriteMemory<uint16_t>(pattern.get_first<uint16_t>(6), 0xD285, true);     //test    edx, edx

    if (FixSettings().Int("MAIN.WindowMode") == 2)
    {
        static SafetyHookMid borderlessSetResHook;
        const auto setRes = GetProcAddress(GetModuleHandleW(L"D3DDrv.dll"), "?SetRes@UD3DRenderDevice@@UAEHPAVUViewport@@HHH@Z");
        borderlessSetResHook = safetyhook::create_mid(setRes, [](SafetyHookContext& regs)
        {
            if (*reinterpret_cast<const int*>(regs.esp + 16))
                return;
            HWND window = hGameWindow;
            const auto viewport = *reinterpret_cast<const uintptr_t*>(regs.esp + 4);
            if (viewport)
            {
                const auto nativeWindow = *reinterpret_cast<const uintptr_t*>(viewport + 0x230);
                if (nativeWindow)
                    window = *reinterpret_cast<const HWND*>(nativeWindow + 4);
            }
            const auto [width, height] = GetGameMonitorSize(window);
            *reinterpret_cast<int*>(regs.esp + 8) = width;
            *reinterpret_cast<int*>(regs.esp + 12) = height;
        });
    }

    //SetRes
    PresentParams.SetAddress(*hook::module_pattern(GetModuleHandle(L"D3DDrv"), "BF ? ? ? ? F3 AB 8B 0D ? ? ? ? 39 01 8B 4D").get_first<D3DPRESENT_PARAMETERS*>(1));
    pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "B8 01 00 00 00 64 89 0D 00 00 00 00 5B 8B E5 5D C2 10 00");
    struct SetResHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.eax = 1;

            Screen.Width = PresentParams->BackBufferWidth;
            Screen.Height = PresentParams->BackBufferHeight;
            Screen.fWidth = static_cast<float>(Screen.Width);
            Screen.fHeight = static_cast<float>(Screen.Height);
            Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
            Screen.fHudScaleX = 1.0f / (((4.0f / 3.0f)) / (Screen.fAspectRatio));
            Screen.fHudScaleX2 = 2.0f / (600.0f * Screen.fAspectRatio);
            Screen.fTextScaleX = 600.0f * Screen.fAspectRatio;
            Screen.fHudOffset = ((4.0f / 3.0f) / Screen.fAspectRatio);
            Screen.FilmstripScaleX = static_cast<int32_t>(Screen.fWidth / (1280.0f / (368.0 * ((4.0 / 3.0) / (Screen.fAspectRatio)))));

            RawInputHandler<>::OnResChange();

            auto [DesktopResW, DesktopResH] = GetDesktopRes();
            if (Screen.Width != DesktopResW || Screen.Height != DesktopResH)
            {
                FixSettings().Set("MAIN.ResX", std::to_string(Screen.Width));
                FixSettings().Set("MAIN.ResY", std::to_string(Screen.Height));
            }
            else
            {
                FixSettings().Set("MAIN.ResX", std::to_string(0));
                FixSettings().Set("MAIN.ResY", std::to_string(0));
            }

            Screen.fWidescreenHudOffset = -CalculateWidescreenOffset(Screen.fWidth, Screen.fHeight, 800.0f, 600.0f);
            Screen.FilmstripOffset = static_cast<int32_t>((((Screen.fWidth / 2.0f) - ((Screen.fHeight * (Screen.fAspectRatio)) / 2.0f)) * 2.0f) + ((float)Screen.FilmstripScaleX / 5.25f));

            if (Screen.fHudAspectRatioConstraint.has_value())
            {
                float value = Screen.fHudAspectRatioConstraint.value();
                if (value < 0.0f || value > (32.0f / 9.0f))
                    Screen.fWidescreenHudOffset = value;
                else
                {
                    value = ClampHudAspectRatio(value, Screen.fAspectRatio);
                    Screen.fWidescreenHudOffset = -CalculateWidescreenOffset(Screen.fHeight * value, Screen.fHeight, 800.0f, 600.0f);
                    Screen.FilmstripOffset = static_cast<int32_t>((((Screen.fWidth / 2.0f) - ((Screen.fHeight * ((Screen.fHeight * value) / Screen.fHeight)) / 2.0f)) * 2.0f) + ((float)Screen.FilmstripScaleX / 5.25f));
                }
            }

            if (PresentParams->Windowed)
                PlaceGameWindow(PresentParams->hDeviceWindow, Screen.Width, Screen.Height);

            //Mirrors
            auto hEngine = GetModuleHandle(L"Engine");
            if (hEngine)
            {
                if (!MIRROR_WIDTH.is_initialized())
                {
                    MIRROR_WIDTH.SetAddress(reinterpret_cast<int*>(GetProcAddress(hEngine, "?MIRROR_WIDTH@@3IA")));
                    MIRROR_HEIGHT.SetAddress(reinterpret_cast<int*>(GetProcAddress(hEngine, "?MIRROR_HEIGHT@@3IA")));
                }
                MIRROR_WIDTH = std::clamp(640 * nMirrorResolutionMultiplier, 640, (int)Screen.Width);
                MIRROR_HEIGHT = std::clamp(360 * nMirrorResolutionMultiplier, 360, (int)Screen.Height);
            }
        }
    }; injector::MakeInline<SetResHook>(pattern.get_first(0));

    //loadscreen
    static LPDIRECT3DTEXTURE9 pTexLoadscreenCustom = nullptr;
    pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "89 BE ? ? ? ? E8 ? ? ? ? 8B 86 ? ? ? ? 8B 08 57 6A 00 50 FF 91 ? ? ? ? 6A 04 8B CB E8 ? ? ? ? 8B 6C 24 30 F6 45 54 01 74 48 83 BE"); //
    struct LoadscHook
    {
        void operator()(injector::reg_pack& regs)
        {
            if (regs.edi)
            {
                auto pTex = (IDirect3DTexture9*)regs.edi;
                if (pTex->GetType() == D3DRTYPE_TEXTURE)
                {
                    D3DSURFACE_DESC pDesc;
                    pTex->GetLevelDesc(0, &pDesc);
                    if (pDesc.Width == 2048 && pDesc.Height == 1024 && pDesc.Format == D3DFMT_DXT5 && pDesc.Usage == 0)
                    {
                        if (!pTexLoadscreenCustom)
                        {
                            IDirect3DDevice9* ppDevice = nullptr;
                            pTex->GetDevice(&ppDevice);
                            if (D3DXCreateTextureFromFileA(ppDevice, Screen.szLoadscPath.c_str(), &pTexLoadscreenCustom) == D3D_OK) // REDACTED change reverted
                                regs.edi = (uint32_t)pTexLoadscreenCustom;
                        }
                        else
                            regs.edi = (uint32_t)pTexLoadscreenCustom;
                    }
                }
            }
            *(uint32_t*)(regs.esi + 0x6EB0) = regs.edi;
        }
    }; injector::MakeInline<LoadscHook>(pattern.get_first(0), pattern.get_first(6));

    static auto DeviceResetHook = safetyhook::create_mid(GetProcAddress(GetModuleHandle(L"D3DDrv"), "?resetDevice@UD3DRenderDevice@@QAEXAAU_D3DPRESENT_PARAMETERS_@@@Z"), [](SafetyHookContext& regs)
    {
        auto SafeRelease = [](auto ppT)
        {
            if (*ppT)
            {
                (*ppT)->Release();
                *ppT = NULL;
            }
        };

        SafeRelease(&pTexLoadscreenCustom);
    });

    //Enhanced night vision NaN workaround
    pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "F3 A5 8B 90 30 02 00 00");
    injector::MakeNOP(pattern.get_first(0), 2, true);

    // Goggles and suit light colour
    if (gColor || gBlacklistIndicators)
    {
        SuitLights::GetObjectName = reinterpret_cast<decltype(SuitLights::GetObjectName)>(GetProcAddress(GetModuleHandle(L"Core"), "?GetName@UObject@@QBEPBGXZ"));
        pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "8B 86 ? ? ? ? 83 86 ? ? ? ? 01 8B 08 50 FF 91 78 01 00 00");
        SuitLights::DeviceOffset = *pattern.get_first<int32_t>(2);

        // Base pass, after FGetHSV and the base texture; the material is on the stack, esi is the render device
        pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "FF 15 ? ? ? ? 83 C4 10 8D AE ? ? ? ? 6A 01 8B CD 8B D8 E8");
        static auto SuitLightBaseHook = safetyhook::create_mid(pattern.get_first(6), [](SafetyHookContext& regs)
        {
            const auto material = *reinterpret_cast<uintptr_t*>(regs.esp + 0x114);
            SuitLights::Apply(reinterpret_cast<float*>(regs.eax), material);
            if (SuitLights::HasColor() && SuitLights::IsGoggles(material))
            {
                auto device = *reinterpret_cast<IDirect3DDevice9**>(regs.esi + SuitLights::DeviceOffset);
                if (auto neutral = SuitLights::NeutralTexture(device))
                    device->SetTexture(0, neutral);
            }
        });

        // Glow pass, after FGetHSV; the material is in ebp
        pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "FF 15 ? ? ? ? 8B 6D 5C 0F 57 C0 83 C4 10 85 ED F3 0F 11 44 24 ? 0F 84 ? ? ? ? 8B 44 24 10 F3 0F 10 40");
        static auto SuitLightGlowHook = safetyhook::create_mid(pattern.get_first(6), [](SafetyHookContext& regs)
        {
            SuitLights::Apply(reinterpret_cast<float*>(regs.eax), regs.ebp);
            SuitLights::GlowPassGoggles = SuitLights::HasColor() && SuitLights::IsGoggles(regs.ebp);
        });

        // Glow pass draw, after its base texture and effect pass; edi is the actor, +0x304 its render device
        pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "E8 ? ? ? ? 8B 13 57 8B CB FF 52 1C");
        static auto SuitLightGlowDrawHook = safetyhook::create_mid(pattern.get_first(5), [](SafetyHookContext& regs)
        {
            if (!SuitLights::GlowPassGoggles)
                return;
            SuitLights::GlowPassGoggles = false;
            auto renderDevice = *reinterpret_cast<uintptr_t*>(regs.edi + 0x304);
            auto device = *reinterpret_cast<IDirect3DDevice9**>(renderDevice + SuitLights::DeviceOffset);
            if (auto neutral = SuitLights::NeutralTexture(device))
                device->SetTexture(0, neutral);
        });
    }

    {
        pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "8B D1 F7 DA");
        struct FullScreen_RefreshRateInHzHook
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.edx = regs.ecx != 0 ? 1000 : 0;
            }
        }; injector::MakeInline<FullScreen_RefreshRateInHzHook>(pattern.get_first(0), pattern.get_first(9));
    }

    {
        pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "8B 4F ? 3B C1");
        static auto VideoSkipHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            if (bVideoStartedFromLoadMap && IsGameForeground())
            {
                #ifndef _DEBUG
                if (GetAsyncKeyState(VK_ESCAPE) & 0x8000 || GetAsyncKeyState(VK_RETURN) & 0x8000)
                    #endif
                {
                    auto& bink_Frames = *(uint32_t*)(regs.edi + 0x8);
                    auto& bink_FrameNum = *(uint32_t*)(regs.edi + 0xC);
                    bink_FrameNum = bink_Frames;
                }
            }
        });
    }

    // Brighter lights
    constexpr float fLightsMultiplier = 2.0f;

    static float f4 = 4.0f * fLightsMultiplier;
    pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "F3 0F 10 1D ? ? ? ? F3 0F 10 66 ? F3 0F 59 64 24 ? 8B 8C 24");
    injector::WriteMemory<float*>(pattern.get_first<float*>(4), &f4, true);

    static float f8 = 8.0f * fLightsMultiplier;
    pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "F3 0F 10 25 ? ? ? ? 0F 28 C8 F3 0F 59 0D ? ? ? ? F3 0F 58 0D ? ? ? ? F3 0F 59 C8 F3 0F 10 47");
    injector::WriteMemory<float*>(pattern.get_first<float*>(4), &f8, true);

    // Video Loadscreens scaling
    pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "E8 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? 6A ? 8B C8 E8 ? ? ? ? 8B 9E");
    hbsub_10077470.fun = injector::MakeCALL(pattern.get_first(0), VideoTopOverlay, true).get();

    pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "E8 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? 6A ? 8B C8 E8 ? ? ? ? 80 3D");
    hbsub_10077470.fun = injector::MakeCALL(pattern.get_first(0), VideoBottomOverlay, true).get();

    //pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "E8 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? 6A ? 8B C8 E8 ? ? ? ? 8B 4C 24");
    //hbsub_10077470.fun = injector::MakeCALL(pattern.get_first(0), VideoBackground, true).get();
    //
    //pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "E8 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? 6A ? 8B C8 E8 ? ? ? ? 8B 47");
    //hbsub_10077470.fun = injector::MakeCALL(pattern.get_first(0), VideoContent, true).get();

    InitPostFX();
    InitShaders();
}
