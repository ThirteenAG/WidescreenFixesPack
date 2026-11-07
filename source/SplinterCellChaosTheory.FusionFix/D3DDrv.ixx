module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <LEDEffects.h>
#include <d3dx9.h>
#include <atomic>
#include <stdexcept>

export module D3DDrv;

import ComVars;
import GUI;
import WidescreenHUD;
import Xidi;
import DeveloperOverlay;
import Movie;

std::atomic<float> gVisibility{ 1.0f };
export int32_t gBlacklistIndicators = 0;
export FLTColor gColor = { 0 };
export uint32_t bLightSyncRGB;

// Ops Suit Light colours, in the order of the goggles light setting (0 keeps the game's own lights)
export constexpr std::array<uint32_t, 10> SuitLightColors =
{
    0x37FF4F, // Classic Green
    0xFFB137, // Topaz
    0xFF3737, // Voron Red
    0x3766FF, // Navy
    0xFF3795, // Pink
    0xB137FF, // Violet
    0xFFFFFF, // White
    0xF3FF37, // Gold
    0xFFB137, // Amber
    0x37E0FF, // Ghost Blue
};

// Sam's suit and goggle lights are SCX_glow_materials of ESam_TEX. They draw the skin's diffuse texture, whose lenses
// are painted green, times an HSV colour: a surface pass added to the scene and a pass into the glow buffer.
// A colour multiply can't turn green texels red or blue, so with a chosen colour these draws use shaders that keep
// the texel's brightness and replace its hue.
using FGetHSVFn = float* (__cdecl*)(float* dest, uint32_t H, uint32_t S, uint32_t V);
FGetHSVFn FGetHSV = nullptr;
uint8_t*** NameEntries = nullptr;
const uint8_t* gSuitLight = nullptr;

const char* ObjectName(const uint8_t* object)
{
    auto entry = (*NameEntries)[*reinterpret_cast<const int32_t*>(object + 0x20)];
    return reinterpret_cast<const char*>(entry + 12);
}

bool IsSuitLight(const uint8_t* material)
{
    auto package = material;
    while (auto outer = *reinterpret_cast<const uint8_t* const*>(package + 0x18))
        package = outer;
    if (_stricmp(ObjectName(package), "ESam_TEX") != 0)
        return false;

    std::string name = ObjectName(material);
    std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return name.find("glow") != std::string::npos && name.find("radio") == std::string::npos;
}

void ApplyLightHsv(SafetyHookContext& regs, uintptr_t material, uintptr_t resume)
{
    if (reinterpret_cast<const uint8_t*>(material) != gSuitLight)
        return;

    const auto args = reinterpret_cast<const uint32_t*>(regs.esp);
    auto dest = reinterpret_cast<float*>(args[0]);
    if (gColor.empty())
    {
        FGetHSV(dest, args[1], args[2], args[3]);
    }
    else
    {
        const float brightness = static_cast<float>(args[3] & 0xFF) / 255.0f;
        dest[0] = gColor.R * brightness;
        dest[1] = gColor.G * brightness;
        dest[2] = gColor.B * brightness;
        dest[3] = 1.0f;
    }

    if (gBlacklistIndicators)
    {
        const float visibility = gVisibility.load(std::memory_order_relaxed);
        for (int i = 0; i < 4; i++)
            dest[i] *= visibility;
    }

    regs.eax = reinterpret_cast<uintptr_t>(dest);
    regs.eip = resume;
}

// The game's glow material shaders. Surface pass: tex t0; mul r0, t0, v0; mul_x4 r0, r0, c0.w
constexpr uint32_t SurfacePassShader[] = { 0xFFFF0101, 0x00000042, 0xB00F0000, 0x00000005, 0x800F0000, 0xB0E40000, 0x90E40000,
    0x00000005, 0x820F0000, 0x80E40000, 0xA0FF0000, 0x0000FFFF };
// Glow buffer pass: tex t0; mul r0, t0, c0
constexpr uint32_t GlowPassShader[] = { 0xFFFF0101, 0x00000042, 0xB00F0000, 0x00000005, 0x800F0000, 0xB0E40000, 0xA0E40000,
    0x0000FFFF };

// ps_2_0 replacements: hi/lo = max/min of the texel, peak = max of the colour, white = lo / 2 (the lens's hot core);
// rgb = colour * (hi - white) + white * peak, alpha = texel alpha * colour alpha. Surface pass: colour = v0, result * 4 * c0.w
constexpr uint32_t SurfacePassRecolor[] = { 0xFFFF0200, 0x05000051, 0xA00F0001, 0x3F000000, 0x40800000, 0x00000000, 0x00000000, 0x0200001F, 0x80000000,
    0x900F0000, 0x0200001F, 0x80000000, 0xB0030000, 0x0200001F, 0x90000000, 0xA00F0800, 0x03000042, 0x800F0000,
    0xB0E40000, 0xA0E40800, 0x0300000B, 0x80080001, 0x80550000, 0x80AA0000, 0x0300000B, 0x80080002, 0x80000000,
    0x80FF0001, 0x0300000A, 0x80010001, 0x80AA0000, 0x80550000, 0x0300000A, 0x80010002, 0x80000001, 0x80000000,
    0x03000005, 0x80080000, 0x80FF0000, 0x90FF0000, 0x04000004, 0x80010001, 0x80000002, 0xA1000001, 0x80FF0002,
    0x03000005, 0x80020001, 0x80000002, 0xA0000001, 0x0300000B, 0x80040001, 0x90550000, 0x90AA0000, 0x0300000B,
    0x80010002, 0x90000000, 0x80AA0001, 0x03000005, 0x80020001, 0x80550001, 0x80000002, 0x04000004, 0x80070000,
    0x90E40000, 0x80000001, 0x80550001, 0x02000001, 0x80020001, 0xA0550001, 0x03000005, 0x80010001, 0x80550001,
    0xA0FF0000, 0x03000005, 0x800F0000, 0x80E40000, 0x80000001, 0x02000001, 0x800F0800, 0x80E40000, 0x0000FFFF };
// Glow buffer pass: colour = c0
constexpr uint32_t GlowPassRecolor[] = { 0xFFFF0200, 0x05000051, 0xA00F0001, 0x3F000000, 0x00000000, 0x00000000, 0x00000000, 0x0200001F, 0x80000000,
    0xB0030000, 0x0200001F, 0x90000000, 0xA00F0800, 0x03000042, 0x800F0000, 0xB0E40000, 0xA0E40800, 0x0300000B,
    0x80080001, 0x80550000, 0x80AA0000, 0x0300000B, 0x80080002, 0x80000000, 0x80FF0001, 0x0300000A, 0x80010001,
    0x80AA0000, 0x80550000, 0x0300000A, 0x80010002, 0x80000001, 0x80000000, 0x03000005, 0x80080000, 0x80FF0000,
    0xA0FF0000, 0x04000004, 0x80010001, 0x80000002, 0xA1000001, 0x80FF0002, 0x03000005, 0x80020001, 0x80000002,
    0xA0000001, 0x0300000B, 0x80040001, 0xA0550000, 0xA0AA0000, 0x0300000B, 0x80010002, 0xA0000000, 0x80AA0001,
    0x03000005, 0x80020001, 0x80550001, 0x80000002, 0x04000004, 0x80070000, 0xA0E40000, 0x80000001, 0x80550001,
    0x02000001, 0x800F0800, 0x80E40000, 0x0000FFFF };

IDirect3DDevice9* RecolorDevice = nullptr;
IDirect3DPixelShader9* SurfacePassRecolorShader = nullptr;
IDirect3DPixelShader9* GlowPassRecolorShader = nullptr;

IDirect3DPixelShader9* RecolorShader(IDirect3DDevice9* device, IDirect3DPixelShader9* shader)
{
    if (device != RecolorDevice)
    {
        if (SurfacePassRecolorShader)
            SurfacePassRecolorShader->Release();
        if (GlowPassRecolorShader)
            GlowPassRecolorShader->Release();
        SurfacePassRecolorShader = nullptr;
        GlowPassRecolorShader = nullptr;
        device->CreatePixelShader(reinterpret_cast<const DWORD*>(SurfacePassRecolor), &SurfacePassRecolorShader);
        device->CreatePixelShader(reinterpret_cast<const DWORD*>(GlowPassRecolor), &GlowPassRecolorShader);
        RecolorDevice = device;
    }

    uint32_t function[16]{};
    UINT size = 0;
    if (FAILED(shader->GetFunction(nullptr, &size)) || size > sizeof(function) || FAILED(shader->GetFunction(function, &size)))
        return nullptr;
    if (size == sizeof(SurfacePassShader) && std::memcmp(function, SurfacePassShader, size) == 0)
        return SurfacePassRecolorShader;
    if (size == sizeof(GlowPassShader) && std::memcmp(function, GlowPassShader, size) == 0)
        return GlowPassRecolorShader;
    return nullptr;
}

using DrawIndexedPrimitiveFn = HRESULT(__stdcall*)(IDirect3DDevice9*, D3DPRIMITIVETYPE, INT, UINT, UINT, UINT, UINT);
DrawIndexedPrimitiveFn DrawIndexedPrimitiveOriginal = nullptr;
HRESULT __stdcall DrawIndexedPrimitiveHook(IDirect3DDevice9* device, D3DPRIMITIVETYPE type, INT baseVertex, UINT minIndex,
    UINT numVertices, UINT startIndex, UINT primitiveCount)
{
    IDirect3DPixelShader9* shader = nullptr;
    if (!gSuitLight || FAILED(device->GetPixelShader(&shader)) || !shader)
        return DrawIndexedPrimitiveOriginal(device, type, baseVertex, minIndex, numVertices, startIndex, primitiveCount);

    auto recolor = RecolorShader(device, shader);
    if (recolor)
        device->SetPixelShader(recolor);
    auto result = DrawIndexedPrimitiveOriginal(device, type, baseVertex, minIndex, numVertices, startIndex, primitiveCount);
    if (recolor)
        device->SetPixelShader(shader);
    shader->Release();
    return result;
}

// Draws a mesh section with an SCX_glow_material
SafetyHookInline shGlowMaterialDraw = {};
int __fastcall GlowMaterialDraw(uint8_t* renderer, void* edx, uint8_t* material, void* section)
{
    if (!gColor.empty() && !DrawIndexedPrimitiveOriginal)
    {
        auto device = *reinterpret_cast<IDirect3DDevice9**>(*reinterpret_cast<uint8_t**>(renderer + 0x1A4) + 0x4D3C);
        auto vtable = *reinterpret_cast<uintptr_t**>(device);
        DrawIndexedPrimitiveOriginal = reinterpret_cast<DrawIndexedPrimitiveFn>(vtable[82]);
        injector::WriteMemory(&vtable[82], &DrawIndexedPrimitiveHook, true);
    }

    gSuitLight = IsSuitLight(material) ? material : nullptr;
    auto result = shGlowMaterialDraw.unsafe_thiscall<int>(renderer, material, section);
    gSuitLight = nullptr;
    return result;
}

SafetyHookInline shsub_10C86280 = {};
int __fastcall sub_10C86280(int _this, void* edx, void* a2, int a3)
{
    static bool bOnce = false;

    if (!bOnce)
    {
        auto pDevice = *(IDirect3DDevice9**)(_this + 0x4D3C);
        IDirect3DSwapChain9* pSwapChain = nullptr;
        if (SUCCEEDED(pDevice->GetSwapChain(0, &pSwapChain)))
        {
            D3DPRESENT_PARAMETERS pp;
            if (SUCCEEDED(pSwapChain->GetPresentParameters(&pp)))
            {
                bIsWindowed = pp.Windowed;
                hGameWindow = pp.hDeviceWindow;
            }
            pSwapChain->Release();
        }
        bOnce = true;
    }

    //crashes
    //if (bIsWindowed)
    //{
    //    MSG msg;
    //    while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE))
    //    {
    //        TranslateMessage(&msg);
    //        DispatchMessageA(&msg);
    //    }
    //}

    bPlayingVideo = true;
    auto ret = shsub_10C86280.unsafe_fastcall<int>(_this, edx, a2, a3);
    bPlayingVideo = false;
    return ret;
}

// UWindowsViewport::ResizeViewport set the resolution, in fullscreen or in a window
void OnResolutionChange(int32_t width, int32_t height, bool bFullscreen)
{
    bIsWindowed = !bFullscreen;
    Screen.Width = width;
    Screen.Height = height;
    Screen.fWidth = static_cast<float>(Screen.Width);
    Screen.fHeight = static_cast<float>(Screen.Height);
    Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);

    Screen.fHUDScaleX = (1.0f / Screen.fWidth * (Screen.fHeight / 480.0f)) * 2.0f;
    dword_1120B6BC = Screen.fHUDScaleX;
    Screen.fTextScaleX = ((4.0f / 3.0f) / Screen.fAspectRatio) * 2.0f;
    Screen.fHudOffset = Screen.fTextScaleX / 2.0f;
    Screen.nHudOffsetReal = static_cast<int32_t>(((480.0f * Screen.fAspectRatio) - 640.0f) / 2.0f);
    Screen.nScopeScale = static_cast<int32_t>(Screen.fWidth * (Screen.fAspectRatio / (4.0f / 3.0f)));

    RawInputHandler<>::OnResChange();

    // The resolution is kept for the next start, a borderless window always has the monitor's
    if (bFullscreen || !bBorderless)
    {
        auto& settings = FixSettings();
        auto [DesktopResW, DesktopResH] = GetDesktopRes();
        if (Screen.Width != DesktopResW || Screen.Height != DesktopResH)
        {
            settings.Set("MAIN.ResX", std::to_string(Screen.Width));
            settings.Set("MAIN.ResY", std::to_string(Screen.Height));
        }
        else
        {
            settings.Set("MAIN.ResX", "0");
            settings.Set("MAIN.ResY", "0");
        }

        settings.Save();
    }

    Screen.fWidescreenHudOffset = -CalculateWidescreenOffset(Screen.fWidth, Screen.fHeight, 640.0f, 480.0f);
    if (Screen.fHudAspectRatioConstraint.has_value())
    {
        float value = Screen.fHudAspectRatioConstraint.value();
        if (value < 0.0f || value > (32.0f / 9.0f))
            Screen.fWidescreenHudOffset = value;
        else
        {
            value = ClampHudAspectRatio(value, Screen.fAspectRatio);
            Screen.fWidescreenHudOffset = -CalculateWidescreenOffset(Screen.fHeight * value, Screen.fHeight, 640.0f, 480.0f);
        }
    }
}

void BeforePresent(SafetyHookContext& regs)
{
    UpdateXidiProfile();

    if ((CMenusManager::IsMainMenuDisplayed() && !CMenusManager::IsMenuDisplayed(Page::P_Briefing) &&
        !CMenusManager::IsMenuDisplayed(Page::P_LoadOut) && !CMenusManager::IsMenuDisplayed(Page::P_LoadOut2) &&
        !CMenusManager::IsMenuDisplayed(Page::P_CamControl) && !CMenusManager::IsMenuDisplayed(Page::P_Camera) &&
        !CMenusManager::IsMenuDisplayed(Page::P_opticcable) && !CMenusManager::IsMenuDisplayed(Page::P_MenuGameOver) &&
        (bIsInMenu && *bIsInMenu == 0)) || bQuickLoadingShown || bLoadingBarShown)
    {
        IDirect3DDevice9* pD3DDevice = (IDirect3DDevice9*)(regs.eax);

        const int W = Screen.Width;
        const int H = Screen.Height;

        constexpr float TARGET_ASPECT = 4.0f / 3.0f;

        float render_w = (float)W;
        float render_h = (float)H;

        // Fit 4:3 inside current resolution (letterbox or pillarbox)
        if ((float)W / (float)H > TARGET_ASPECT)
        {
            // Current screen is wider, pillarboxes on sides
            render_w = (float)H * TARGET_ASPECT;
        }
        else
        {
            // Current screen is taller or same, letterboxes top/bottom
            render_h = (float)W / TARGET_ASPECT;
        }

        // Center the render area
        const float x = ((float)W - render_w) * 0.5f;
        const float y = ((float)H - render_h) * 0.5f;

        const LONG left = (LONG)floor(x);
        const LONG right = (LONG)ceil(x + render_w);
        const LONG top = (LONG)floor(y);
        const LONG bottom = (LONG)ceil(y + render_h);

        const D3DCOLOR BLACK = D3DCOLOR_XRGB(0, 0, 0);

        // Left pillar
        if (left > 0)
        {
            D3DRECT r{ 0, 0, left, H };
            pD3DDevice->Clear(1, &r, D3DCLEAR_TARGET, BLACK, 1.0f, 0);
        }

        // Right pillar
        if (right < W)
        {
            D3DRECT r{ right, 0, W, H };
            pD3DDevice->Clear(1, &r, D3DCLEAR_TARGET, BLACK, 1.0f, 0);
        }

        // Top letterbox
        if (top > 0)
        {
            D3DRECT r{ 0, 0, W, top };
            pD3DDevice->Clear(1, &r, D3DCLEAR_TARGET, BLACK, 1.0f, 0);
        }

        // Bottom letterbox
        if (bottom < H)
        {
            D3DRECT r{ 0, bottom, W, H };
            pD3DDevice->Clear(1, &r, D3DCLEAR_TARGET, BLACK, 1.0f, 0);
        }

        // One-pixel safety overlap on right/bottom edges
        if (right < W)
        {
            D3DRECT r{ right - 1, 0, right, H };
            pD3DDevice->Clear(1, &r, D3DCLEAR_TARGET, BLACK, 1.0f, 0);
        }
        if (bottom < H)
        {
            D3DRECT r{ 0, bottom - 1, W, bottom };
            pD3DDevice->Clear(1, &r, D3DCLEAR_TARGET, BLACK, 1.0f, 0);
        }
    }
    DeveloperOverlay::Present(reinterpret_cast<IDirect3DDevice9*>(regs.eax));
}

export void InitD3DDrv()
{
    dword_1120B6BC.SetAddress(*hook::pattern("D9 1D ? ? ? ? 0F BF").count(1).get(0).get<float*>(2));
    dword_1120B6B0.SetAddress(*hook::pattern("8B 0D ? ? ? ? 85 C9 74 ? 8B 54 24 04 8B 82").count(1).get(0).get<uint32_t*>(2));
    auto pattern = hook::pattern("89 6B 58 89 7B 5C EB 0E");
    struct GetRes
    {
        void operator()(injector::reg_pack& regs)
        {
            *reinterpret_cast<uint32_t*>(regs.ebx + 0x58) = regs.ebp;
            *reinterpret_cast<uint32_t*>(regs.ebx + 0x5C) = regs.edi;
            OnResolutionChange(regs.ebp, regs.edi, true);
        }
    }; injector::MakeInline<GetRes>(pattern.get_first(0), pattern.get_first(6)); //<0x10CC622C, 0x10CC6232>

    pattern = hook::pattern("89 6B 50 89 7B 54 6A 00 68 00 10 00 00");
    struct GetResWindowed
    {
        void operator()(injector::reg_pack& regs)
        {
            *reinterpret_cast<uint32_t*>(regs.ebx + 0x50) = regs.ebp;
            *reinterpret_cast<uint32_t*>(regs.ebx + 0x54) = regs.edi;
            OnResolutionChange(regs.ebp, regs.edi, false);
        }
    }; injector::MakeInline<GetResWindowed>(pattern.get_first(0), pattern.get_first(6)); //<0x10CC623C, 0x10CC6242>

    // The movie quad receives pixel bounds and texture coordinates separately.
    pattern = hook::pattern("52 50 57 E8 ? ? ? ? 68 00 00 80 3F 68 00 00 80 3F 68 00 00 80 3F 68 00 00 80 3F 6A 2E 8B C8");
    static auto MovieFrameHook = safetyhook::create_mid(pattern.get_first(3), [](SafetyHookContext& regs)
    {
        auto movie = reinterpret_cast<const uint32_t*>(regs.esi);
        auto renderer = reinterpret_cast<const uint8_t*>(regs.edi);
        auto args = reinterpret_cast<float*>(regs.esp + sizeof(uint32_t));
        CTMovie::Rectangle rectangle{ args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7] };
        static const auto& mode = FixSettings().Int("MAIN.FMVWidescreenMode");
        if (!CTMovie::Frame(rectangle, float(*reinterpret_cast<const int*>(renderer + 0x4898)),
            float(*reinterpret_cast<const int*>(renderer + 0x489C)), float(movie[0]), float(movie[1]), mode))
            return;

        if (rectangle.left > 0.0f || rectangle.top > 0.0f)
        {
            auto device = *reinterpret_cast<IDirect3DDevice9* const*>(renderer + 0x4D3C);
            if (device)
                device->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
        }
        args[0] = rectangle.left;
        args[1] = rectangle.top;
        args[2] = rectangle.right;
        args[3] = rectangle.bottom;
        args[4] = rectangle.u0;
        args[5] = rectangle.v0;
        args[6] = rectangle.u1;
        args[7] = rectangle.v1;
    });

    pattern = hook::pattern("8B 44 24 04 81 EC 84 00 00 00 56");
    shsub_10C86280 = safetyhook::create_inline(pattern.get_first(0), sub_10C86280);

    // The game uses a 2048 shadow map by default.
    if (nShadowMapResolution && nShadowMapResolution != 2048)
    {
        auto pattern = hook::pattern("68 ? ? ? ? 68 ? ? ? ? 8B CE FF 92 ? ? ? ? 5F 5E C7 43 ? ? ? ? ? 5B C2 04 00 C7 87");
        injector::WriteMemory(pattern.get_first(1), nShadowMapResolution, true);
        injector::WriteMemory(pattern.get_first(6), nShadowMapResolution, true);

        pattern = hook::pattern("B9 00 01 00 00 89 94 24 DC 00 00 00");
        injector::WriteMemory(pattern.get_first(1), nShadowMapResolution, true);
    }

    if (!bEnableShadowFiltering)
    {
        // Disable angle-dependent filtering. Soft shadows remain less soft at wide resolutions.
        auto pattern = hook::pattern("B9 ? ? ? ? 89 94 24 DC 00 00 00");
        injector::WriteMemory(pattern.get_first(1), 0x10000, true);

        pattern = hook::pattern("BB 9A 99 99 3F");
        injector::WriteMemory<float>(pattern.get_first(1), 0.25f, true);

        pattern = hook::pattern("C7 86 C8 07 00 00 9A 99 99 BE");
        injector::WriteMemory<float>(pattern.get_first(6), -0.15f, true);
    }

    // Goggles light colour
    if (!gColor.empty() || gBlacklistIndicators || bLightSyncRGB)
    {
        pattern = hook::pattern("8B 49 20 8B 15 ? ? ? ? 8B 0C 8A");
        NameEntries = *pattern.get_first<uint8_t***>(5);

        pattern = hook::pattern("81 EC F4 00 00 00 53 55 8B E9 83 7D 50 0B 56 8B B5 A4 01 00 00");
        shGlowMaterialDraw = safetyhook::create_inline(pattern.get_first(), GlowMaterialDraw);

        // The glow material's colour: surface pass with and without a projector, glow buffer pass
        pattern = hook::pattern("33 D2 8A 53 62 8D 4C 24 ? 50 33 C0 8A 43 61 52 50 51 E8 ? ? ? ? 8B 8E ? ? ? ? 8B 11 83 C4 10 BF 01 00 00 00");
        auto projected = pattern.get_first<uint8_t>(18);
        FGetHSV = reinterpret_cast<FGetHSVFn>(injector::GetBranchDestination(projected).as_int());
        static const auto projectedResume = reinterpret_cast<uintptr_t>(projected) + 5;
        static auto projectedHook = safetyhook::create_mid(projected, [](SafetyHookContext& regs)
        {
            ApplyLightHsv(regs, regs.ebx, projectedResume);
        });

        pattern = hook::pattern("33 D2 8A 53 62 8D 4C 24 ? 50 33 C0 8A 43 61 52 50 51 E8 ? ? ? ? 8B 8E ? ? ? ? 8B 11 83 C4 10 6A 01 50 6A 14 51");
        auto surface = pattern.get_first<uint8_t>(18);
        static const auto surfaceResume = reinterpret_cast<uintptr_t>(surface) + 5;
        static auto surfaceHook = safetyhook::create_mid(surface, [](SafetyHookContext& regs)
        {
            ApplyLightHsv(regs, regs.ebx, surfaceResume);
        });

        pattern = hook::pattern("33 D2 8A 55 62 33 C0 8A 45 61 68 FF 00 00 00 8D 4C 24 ? 52 50 51 E8 ? ? ? ? 8B 45 48 83 C4 10");
        auto glow = pattern.get_first<uint8_t>(22);
        static const auto glowResume = reinterpret_cast<uintptr_t>(glow) + 5;
        static auto glowHook = safetyhook::create_mid(glow, [](SafetyHookContext& regs)
        {
            ApplyLightHsv(regs, regs.ebp, glowResume);
        });
        if (!shGlowMaterialDraw || !projectedHook || !surfaceHook || !glowHook)
            throw std::runtime_error("Light color hooks unavailable");

        pattern = hook::pattern("66 89 44 24 ? 8B 17");
        struct BlacklistIndicatorsHook
        {
            void operator()(injector::reg_pack& regs)
            {
                auto ax = *reinterpret_cast<int16_t*>(&regs.eax);
                *reinterpret_cast<int16_t*>(regs.esp + 0xC) = ax;
                auto v = ax - 446;
                if (v <= 10) v = 10;
                const float visibility = static_cast<float>(v) / 134.0f;
                gVisibility.store(gBlacklistIndicators == 2 ? visibility : 1.0f - visibility, std::memory_order_relaxed);
            }
        }; injector::MakeInline<BlacklistIndicatorsHook>(pattern.get_first()); //0x10B66B97

        if (bLightSyncRGB)
        {
            LEDEffects::Inject([]()
            {
                static float fPlayerVisibility = gVisibility.load(std::memory_order_relaxed);
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                auto gVisCmp = static_cast<float>(static_cast<int>(gVisibility.load(std::memory_order_relaxed) * 10.0f)) / 10.0f;
                auto fPlVisCmp = static_cast<float>(static_cast<int>(fPlayerVisibility * 10.0f)) / 10.0f;

                if (fPlVisCmp > gVisCmp)
                    fPlayerVisibility -= 0.05f;
                else if (fPlVisCmp < gVisCmp)
                    fPlayerVisibility += 0.05f;

                fPlayerVisibility = std::clamp(fPlayerVisibility, 0.0f, 1.0f);

                const auto color = gColor.empty() ? FLTColor(0x01FF01) : gColor;
                auto [R, G, B] = LEDEffects::RGBtoPercent(color.R * 255.0f, color.G * 255.0f, color.B * 255.0f, fPlayerVisibility);

                LEDEffects::SetLighting(R, G, B, false, false, true);
            });
        }
    }

    pattern = hook::pattern("8B 10 55 55 55 55");
    static auto FullscreenPresentHook = safetyhook::create_mid(pattern.get_first(), BeforePresent);

    pattern = hook::pattern("8B 86 3C 4D 00 00 8B 08 55 53 74 ? 8D 54 24 18 52 52 50 FF 51 44");
    static auto WindowedPresentHook = safetyhook::create_mid(pattern.get_first(6), BeforePresent);

}
