module;

#include "stdafx.h"
#include <d3d9.h>
#include <D3dx9.h>
#pragma comment(lib, "D3dx9.lib")

export module D3DDrv;

import ComVars;
import PostFX;
import Shaders;

float* __cdecl FGetHSV(float* dest, uint8_t H, uint8_t S, uint8_t V)
{
    auto bChangeColor = ((H == 0x41 && S == 0x33) || (H == 0x5B && S == 0 && V == 0xFF) || (H == 0x2B && S == 0x40 && V == 0xFF)); //goggles || green ind || yellow ind
    if (bChangeColor)
    {
        if (gColor)
        {
            switch (gColor)
            {
                case 2:
                    dest[0] = 1.0f;
                    dest[1] = 0.0f;
                    dest[2] = 0.0f;
                    dest[3] = 1.0f;
                    break;
                case 3:
                    dest[0] = 1.0f;
                    dest[1] = 0.5f;
                    dest[2] = 0.0f;
                    dest[3] = 1.0f;
                    break;
                default:
                    dest[0] = 0.0f;
                    dest[1] = 1.0f;
                    dest[2] = 0.0f;
                    dest[3] = 1.0f;
                    break;
            }
            if (gBlacklistIndicators)
            {
                dest[0] *= gVisibility;
                dest[1] *= gVisibility;
                dest[2] *= gVisibility;
                dest[3] *= gVisibility;
            }
            return dest;
        }
    }

    float r, g, b, a = 1.0f;
    float v14 = (float)H * 6.0f * 0.00390625f;
    float v4 = floor(v14);
    float v5 = (float)(255 - S) * 0.0039215689f;
    float v6 = (float)V * 0.0039215689f;
    float v16 = (1.0f - v5) * v6;
    float v10 = (1.0f - (v5 * (v14 - v4))) * v6;
    float v7 = (1.0f - (v14 - v4)) * v5;
    float v15 = (float)V * 0.0039215689f;
    float v17 = (1.0f - v7) * v6;

    switch ((uint32_t)v4)
    {
        case 0:
            r = v15;
            g = v17;
            b = v16;
            break;
        case 1:
            r = v10;
            g = v15;
            b = v16;
            break;
        case 2:
            r = v16;
            g = v15;
            b = v17;
            break;
        case 3:
            r = v16;
            g = v10;
            b = v15;
            break;
        case 4:
            r = v17;
            g = v16;
            b = v15;
            break;
        case 5:
            r = v15;
            g = v16;
            b = v10;
            break;
        default:
            break;
    }

    dest[0] = r;
    dest[1] = g;
    dest[2] = b;
    dest[3] = a;

    if (bChangeColor && gBlacklistIndicators)
    {
        dest[0] *= gVisibility;
        dest[1] *= gVisibility;
        dest[2] *= gVisibility;
        dest[3] *= gVisibility;
    }

    return dest;
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
    CIniReader iniReader("");
    static auto nShadowMapResolutionMultiplier = std::clamp(iniReader.ReadInteger("GRAPHICS", "ShadowMapResolutionMultiplier", 0), 0, 16);
    nGlowResolutionMultiplier = std::clamp(iniReader.ReadInteger("GRAPHICS", "GlowResolutionMultiplier", 0), 0, 16);
    static auto nMirrorResolutionMultiplier = std::clamp(iniReader.ReadInteger("GRAPHICS", "MirrorResolutionMultiplier", 0), 0, 16);

    if (nShadowMapResolutionMultiplier)
    {
        const char* GShadowResolutionDegreeXrefs[] = {
            "B8 00 04 00 00 D3 F8 80 BE",
            "B8 00 04 00 00 D3 F8 8B 8E ? ? ? ? 8B 11 8D 8E",
            "B8 00 02 00 00 D3 F8 8B 8E ? ? ? ? 8B 09 6A 01",
            "B8 00 02 00 00 D3 F8 8D 8E ? ? ? ? 51 8B 8E",
            "B8 00 04 00 00 D3 F8 8B 8E ? ? ? ? 8B 11 6A 01",
            "B8 00 02 00 00 D3 F8 8B 8E ? ? ? ? 8B 09 6A 50",
            "B8 00 02 00 00 D3 F8 8D 8E ? ? ? ? 51 6A 00",
            "B8 00 02 00 00 D3 F8 8B 8E ? ? ? ? 8B 11 6A 01 50 50 51 FF 52 ? 8B 03",
            "B8 00 02 00 00 D3 F8 8B 8E ? ? ? ? 8B 11 6A 01 50 50 51 FF 52 ? 8B 1B",
            "B8 00 02 00 00 EB 11",
            "B8 00 04 00 00 D3 F8 C7 44 24",
        };

        for (auto& xref : GShadowResolutionDegreeXrefs)
        {
            auto pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), xref);
            auto v = injector::ReadMemory<uint32_t>(pattern.get_first(1), true);
            injector::WriteMemory(pattern.get_first(1), v * nShadowMapResolutionMultiplier, true);
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
    injector::WriteMemory<uint16_t>(pattern.get_first(6), 0xD285, true);     //test    edx, edx

    //SetRes
    static auto pPresentParams = *hook::module_pattern(GetModuleHandle(L"D3DDrv"), "BF ? ? ? ? F3 AB 8B 0D ? ? ? ? 39 01 8B 4D").get_first<D3DPRESENT_PARAMETERS*>(1);
    pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "B8 01 00 00 00 64 89 0D 00 00 00 00 5B 8B E5 5D C2 10 00");
    struct SetResHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.eax = 1;

            Screen.Width = pPresentParams->BackBufferWidth;
            Screen.Height = pPresentParams->BackBufferHeight;
            Screen.fWidth = static_cast<float>(Screen.Width);
            Screen.fHeight = static_cast<float>(Screen.Height);
            Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
            Screen.fHudScaleX = 1.0f / (((4.0f / 3.0f)) / (Screen.fAspectRatio));
            Screen.fHudScaleX2 = 2.0f / (600.0f * Screen.fAspectRatio);
            Screen.fTextScaleX = 600.0f * Screen.fAspectRatio;
            Screen.fHudOffset = ((4.0f / 3.0f) / Screen.fAspectRatio);
            Screen.FilmstripScaleX = static_cast<int32_t>(Screen.fWidth / (1280.0f / (368.0 * ((4.0 / 3.0) / (Screen.fAspectRatio)))));
            Screen.FilmstripOffset = static_cast<int32_t>((((Screen.fWidth / 2.0f) - ((Screen.fHeight * (4.0f / 3.0f)) / 2.0f)) * 2.0f) + ((float)Screen.FilmstripScaleX / 5.25f));

            RawInputHandler<>::OnResChange();

            CIniReader iniReader("");
            auto [DesktopResW, DesktopResH] = GetDesktopRes();
            if (Screen.Width != DesktopResW || Screen.Height != DesktopResH)
            {
                iniReader.WriteInteger("MAIN", "ResX", Screen.Width);
                iniReader.WriteInteger("MAIN", "ResY", Screen.Height);
            }
            else
            {
                iniReader.WriteInteger("MAIN", "ResX", 0);
                iniReader.WriteInteger("MAIN", "ResY", 0);
            }

            Screen.fWidescreenHudOffset = std::abs(CalculateWidescreenOffset(Screen.fWidth, Screen.fHeight, 800.0f, 600.0f));
            if (Screen.nHudWidescreenMode)
                Screen.FilmstripOffset = static_cast<int32_t>((((Screen.fWidth / 2.0f) - ((Screen.fHeight * (Screen.fAspectRatio)) / 2.0f)) * 2.0f) + ((float)Screen.FilmstripScaleX / 5.25f));

            if (Screen.fHudAspectRatioConstraint.has_value())
            {
                float value = Screen.fHudAspectRatioConstraint.value();
                if (value < 0.0f || value > (32.0f / 9.0f))
                    Screen.fWidescreenHudOffset = value;
                else
                {
                    value = ClampHudAspectRatio(value, Screen.fAspectRatio);
                    Screen.fWidescreenHudOffset = std::abs(CalculateWidescreenOffset(Screen.fHeight * value, Screen.fHeight, 800.0f, 600.0f));
                    if (Screen.nHudWidescreenMode)
                        Screen.FilmstripOffset = static_cast<int32_t>((((Screen.fWidth / 2.0f) - ((Screen.fHeight * ((Screen.fHeight * value) / Screen.fHeight)) / 2.0f)) * 2.0f) + ((float)Screen.FilmstripScaleX / 5.25f));
                }
            }

            if (pPresentParams->Windowed)
            {
                tagRECT rect;
                rect.left = (LONG)(((float)DesktopResW / 2.0f) - (Screen.fWidth / 2.0f));
                rect.top = (LONG)(((float)DesktopResH / 2.0f) - (Screen.fHeight / 2.0f));
                rect.right = (LONG)Screen.Width;
                rect.bottom = (LONG)Screen.Height;
                SetWindowLong(pPresentParams->hDeviceWindow, GWL_STYLE, GetWindowLong(pPresentParams->hDeviceWindow, GWL_STYLE) & ~WS_OVERLAPPEDWINDOW);
                SetWindowPos(pPresentParams->hDeviceWindow, NULL, rect.left, rect.top, rect.right, rect.bottom, SWP_NOACTIVATE | SWP_NOZORDER);
                SetForegroundWindow(pPresentParams->hDeviceWindow);
                SetCursor(NULL);
            }

            //Mirrors
            auto hEngine = GetModuleHandle(L"Engine");
            if (hEngine)
            {
                injector::WriteMemory(GetProcAddress(hEngine, "?MIRROR_WIDTH@@3IA"), std::clamp(640 * nMirrorResolutionMultiplier, 640, (int)Screen.Width), true);
                injector::WriteMemory(GetProcAddress(hEngine, "?MIRROR_HEIGHT@@3IA"), std::clamp(360 * nMirrorResolutionMultiplier, 360, (int)Screen.Height), true);
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
                            if (D3DXCreateTextureFromFileW(ppDevice, (LPCWSTR)(Screen.szLoadscPath.u16string().c_str()), &pTexLoadscreenCustom) == D3D_OK)
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

    //Goggles Light Color
    if (gColor || gBlacklistIndicators)
    {
        pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "FF 15 ? ? ? ? 83 C4 10 8D AE ? ? ? ? 6A 01 8B CD 8B D8 E8");
        injector::MakeNOP(pattern.get(0).get<void>(0), 6, true);
        injector::MakeCALL(pattern.get(0).get<void>(0), FGetHSV, true);

        pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "FF 15 ? ? ? ? 8B 6D 5C 0F 57 C0 83 C4 10 85 ED F3 0F 11 44 24 ? 0F 84 ? ? ? ? 8B 44 24 10 F3 0F 10 40");
        injector::MakeNOP(pattern.get(0).get<void>(0), 6, true);
        injector::MakeCALL(pattern.get(0).get<void>(0), FGetHSV, true);
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
            if (bVideoStartedFromLoadMap)
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
    static float f4 = 4.0f * 2.0f;
    pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "F3 0F 10 1D ? ? ? ? F3 0F 10 66 ? F3 0F 59 64 24 ? 8B 8C 24");
    injector::WriteMemory(pattern.get_first(4), &f4, true);

    static float f8 = 8.0f * 2.0f;
    pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "F3 0F 10 25 ? ? ? ? 0F 28 C8 F3 0F 59 0D ? ? ? ? F3 0F 58 0D ? ? ? ? F3 0F 59 C8 F3 0F 10 47");
    injector::WriteMemory(pattern.get_first(4), &f8, true);

    InitPostFX();
    InitShaders();
}
