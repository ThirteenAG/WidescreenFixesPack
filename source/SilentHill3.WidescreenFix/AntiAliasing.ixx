module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "../Shared/AA/AntiAliasing.hpp"
#include "../Shared/MaxPayne/TemporalAA.hpp"

export module AntiAliasing;

// SH3's main world draws into its indexed ARGB8 target with a shared depth-stencil. Resolve that target after the
// map, characters and shadows, before the effects/UI phase. The GPU projection and view-projection are distinct
// from the CPU/culling matrices: vertex shaders read the GPU product, so SetTransform alone cannot jitter them.
namespace
{
    using Mat = std::array<float, 16>;
    ProtectedGameRef<Mat> Projection, Product;
    GameRef<Mat> View;
    ProtectedGameRef<IUnknown*> Depth;
    GameRef<IUnknown*> Device;
    float* (__cdecl* GetGpuProduct)() = nullptr;
    const int* Mode = nullptr;
    const float* Sharpness = nullptr;
    bool HooksReady = false, DepthFailed = false, InWorld = false, Jittered = false;
    std::atomic<bool> TranslatedDevice = false;
    Mat CleanProjection{}, CleanProduct{};
    WFPAA::Jitter Jitter;
    IUnknown* IntzTexture = nullptr;
    IUnknown* IntzSurface = nullptr;
    IUnknown* OriginalDepth = nullptr;
    IDirect3DSurface9* Scene = nullptr;
    IDirect3DTexture9* SceneDepth = nullptr;
    SafetyHookInline BindHook, CreateHook, MatrixHook;
    SafetyHookMid BeginHook, EndHook;

    IDirect3DDevice9* Device9()
    {
        IDirect3DDevice9* result = nullptr;
        if (Device.get() && SUCCEEDED(Device.get()->QueryInterface(__uuidof(IDirect3DDevice9), (void**)&result)))
        {
            result->Release(); // borrowed from the D3D8 device's lifetime
            return result;
        }
        return nullptr;
    }

    void ReleaseFrame()
    {
        if (Scene) Scene->Release();
        if (SceneDepth) SceneDepth->Release();
        Scene = nullptr;
        SceneDepth = nullptr;
    }

    void RestoreCamera()
    {
        if (Jittered)
        {
            Projection = CleanProjection;
            Product = CleanProduct;
            if (auto device = Device9()) device->SetTransform(D3DTS_PROJECTION, reinterpret_cast<const D3DMATRIX*>(CleanProjection.data()));
        }
        Jittered = false;
        InWorld = false;
    }

    void RestoreDepth()
    {
        RestoreCamera();
        ReleaseFrame();
        if (OriginalDepth)
        {
            if (Depth.get() == IntzSurface)
            {
                Depth.get()->Release(); // the reference installed for the game
                Depth = OriginalDepth;
                if (Device.get()) MaxPayneAA::D3D8::SetRenderTarget(Device.get(), nullptr, OriginalDepth);
            }
            else OriginalDepth->Release(); // the game already freed its replacement
        }
        OriginalDepth = nullptr;
        if (IntzSurface) IntzSurface->Release();
        if (IntzTexture) IntzTexture->Release();
        IntzSurface = nullptr;
        IntzTexture = nullptr;
        DepthFailed = false;
    }

    void InstallDepth()
    {
        auto device = Device9();
        if (!HooksReady || !device || DepthFailed || IntzSurface || !Depth.get()) return;
        MaxPayneAA::D3D8::SurfaceDesc desc{};
        if (!WFPAA::TemporalAA::IntzSupported(device) || FAILED(MaxPayneAA::D3D8::GetDesc(Depth.get(), &desc)) ||
            desc.MultiSampleType != D3DMULTISAMPLE_NONE ||
            FAILED(MaxPayneAA::D3D8::CreateTexture(Device.get(), desc.Width, desc.Height, 1, D3DUSAGE_DEPTHSTENCIL,
                static_cast<D3DFORMAT>(MAKEFOURCC('I', 'N', 'T', 'Z')), D3DPOOL_DEFAULT, &IntzTexture)) || !IntzTexture ||
            FAILED(MaxPayneAA::D3D8::GetSurfaceLevel(IntzTexture, 0, &IntzSurface)) || !IntzSurface)
        {
            RestoreDepth();
            DepthFailed = true;
            WFPAA::TemporalAA::Log("Depth: INTZ unavailable, temporal modes disabled");
            FixSettings().Refresh("GRAPHICS.AntiAliasing");
            return;
        }
        OriginalDepth = Depth.get(); // keep the game's original reference until reset/recreation
        IntzSurface->AddRef();
        Depth = IntzSurface;
        WFPAA::TemporalAA::Log("Depth: main off-screen depth replaced with INTZ");
        FixSettings().Refresh("GRAPHICS.AntiAliasing");
    }

    int __cdecl BindTarget()
    {
        // Before binding and the game's per-frame clear; with anti-aliasing off the game keeps its own depth
        if (*Mode != 0)
            InstallDepth();
        else if (OriginalDepth || IntzSurface)
            RestoreDepth();
        return BindHook.ccall<int>();
    }

    int __cdecl CreateTargets(void* error)
    {
        RestoreDepth(); // the game is about to release/recreate its surfaces
        WFPAA::TemporalAA::OnLostDevice();
        return CreateHook.ccall<int>(error);
    }

    void ApplyJitter()
    {
        CleanProjection = Projection.get();
        CleanProduct = Product.get();
        auto p = CleanProjection;
        D3DSURFACE_DESC desc{};
        if (!Scene || FAILED(Scene->GetDesc(&desc)) || !desc.Width || !desc.Height) return;
        WFPAA::JitterProjection(p.data(), Jitter, static_cast<float>(desc.Width), static_cast<float>(desc.Height));
        Mat product;
        (WFPAA::Matrix::From(View.get().data()) * WFPAA::Matrix::From(p.data())).To(product.data());
        Projection = p;
        Product = product;
        if (auto device = Device9()) device->SetTransform(D3DTS_PROJECTION, reinterpret_cast<const D3DMATRIX*>(p.data()));
        Jittered = true;
    }

    void __cdecl BuildMatrices()
    {
        MatrixHook.ccall<void>();
        if (InWorld && SceneDepth && (Jitter.X != 0.0f || Jitter.Y != 0.0f)) ApplyJitter();
    }

    void BeginWorld()
    {
        RestoreCamera();
        ReleaseFrame();
        if (!HooksReady || *Mode == 0) return;
        auto device = Device9();
        if (!device) return;
        WFPAA::TemporalAA::SetMode(static_cast<WFPAA::Mode>(std::clamp(*Mode, 0, 3)), *Sharpness / 100.0f);
        if (!IntzSurface || DepthFailed || *Mode == 0) return;
        if (FAILED(device->GetRenderTarget(0, &Scene)) || !Scene) return;
        SceneDepth = MaxPayneAA::GetDepthTexture(device);
        if (!SceneDepth) { ReleaseFrame(); return; }
        D3DSURFACE_DESC color{}, depth{};
        if (FAILED(Scene->GetDesc(&color)) || FAILED(SceneDepth->GetLevelDesc(0, &depth)) ||
            color.Width != depth.Width || color.Height != depth.Height) { ReleaseFrame(); return; }
        GetGpuProduct(); // finish the game's lazy clean matrix/culling calculation first
        WFPAA::Camera camera;
        camera.View = WFPAA::Matrix::From(View.get().data());
        camera.Projection = WFPAA::Matrix::From(Projection.get().data());
        camera.Width = color.Width;
        camera.Height = color.Height;
        Jitter = WFPAA::TemporalAA::OnCamera(camera);
        InWorld = true;
        if (Jitter.X != 0.0f || Jitter.Y != 0.0f) ApplyJitter();
    }

    void EndWorld()
    {
        if (*Mode == 0 && !Jittered && !Scene && !SceneDepth) return;
        auto device = *Mode != 0 ? Device9() : nullptr;
        if (device)
        {
            bool resolved = WFPAA::TemporalAA::Render(device, Scene, SceneDepth, Scene);
            static bool logged = false;
            if (resolved && !logged) { WFPAA::TemporalAA::Log("Resolve: main world before effects/UI"); logged = true; }
        }
        RestoreCamera();
        ReleaseFrame();
    }
}

export namespace AntiAliasing
{
    void DeviceReady(IUnknown* device)
    {
        IDirect3DDevice9* translated = nullptr;
        bool supported = device && SUCCEEDED(device->QueryInterface(__uuidof(IDirect3DDevice9), (void**)&translated));
        if (translated) translated->Release();
        TranslatedDevice = supported;
        FixSettings().Refresh("GRAPHICS.AntiAliasing");
    }

    void BeforeReset()
    {
        TranslatedDevice = false;
        FixSettings().Refresh("GRAPHICS.AntiAliasing");
        if (!HooksReady) return;
        RestoreDepth();
        WFPAA::TemporalAA::OnLostDevice();
    }

    void Present()
    {
        static uint32_t generation = 0;
        if (generation != WFPAA::TemporalAA::Generation())
        {
            generation = WFPAA::TemporalAA::Generation();
            FixSettings().Refresh("GRAPHICS.AntiAliasing");
        }
    }

    void Shutdown()
    {
        BeforeReset();
        WFPAA::TemporalAA::Shutdown();
    }

    void Init()
    {
        auto& settings = FixSettings();
        Mode = &settings.Int("GRAPHICS.AntiAliasing");
        Sharpness = &settings.Float("GRAPHICS.Sharpness");
        auto name = GetThisModuleName<std::wstring>();
        name = name.substr(0, name.find_last_of(L'.'));
        WFPAA::Options options;
        options.BridgeName = L"Local\\" + name + L".Upscaler";
        options.ClientName = "SilentHill3.WidescreenFix";
        options.LowDynamicRange = true;
        WFPAA::TemporalAA::Configure(options);
        settings.Availability("MAIN.GraphicsAPI", [](std::string_view value) { return value != "1" || WFPAA::VulkanAvailable(); });
        WFPAA::InstallGraphicsApi(settings.Int("MAIN.GraphicsAPI"), false);
        settings.Availability("GRAPHICS.AntiAliasing", [](std::string_view value)
        {
            if (value == "0") return true;
            if (!HooksReady || !TranslatedDevice || DepthFailed) return false;
            if (value == "2") return WFPAA::TemporalAA::IsAvailable(WFPAA::Backend::DLSS);
            if (value == "3") return WFPAA::TemporalAA::IsAvailable(WFPAA::Backend::FSR);
            return true;
        });

        auto bind = hook::pattern("51 A1 ? ? ? ? 8B 04 85 ? ? ? ? 8B 08 8D 54 24 00 52 6A 00 50 FF 51 3C 85 C0 7C ? 8B 15");
        auto create = hook::pattern("51 53 56 57 33 DB 33 FF BE ? ? ? ? 8D 49 00 8B 06 3B C3 74 08");
        auto gpu = hook::pattern("FF 92 3C 01 00 00 E8 ? ? ? ? 50 68 ? ? ? ? E8 ? ? ? ? A1 ? ? ? ? 8B 10 6A 04 68");
        auto begin = hook::pattern("E8 ? ? ? ? E8 ? ? ? ? 6A 01 E8 ? ? ? ? 6A 03 E8 ? ? ? ?");
        auto end = hook::pattern("83 C4 0C E8 ? ? ? ? E8 ? ? ? ? 6A 04 E8 ? ? ? ? 6A 01 E8");
        if (bind.size() != 1 || create.size() != 1 || gpu.size() != 1 || begin.size() != 1 || end.size() != 1)
        {
            if (*Mode != 0)
                WFPAA::TemporalAA::Log("Hooks: unsupported renderer, temporal modes disabled");
            return;
        }
        auto b = bind.get_first<uint8_t>();
        Depth.SetAddress(*reinterpret_cast<uintptr_t*>(b + 32));
        Device.SetAddress(*reinterpret_cast<uintptr_t*>(b + 37));
        auto getter = injector::GetBranchDestination(gpu.get_first(6)).as_int();
        GetGpuProduct = reinterpret_cast<decltype(GetGpuProduct)>(getter);
        auto product = *reinterpret_cast<uintptr_t*>(getter + 15);
        Product.SetAddress(product);
        Projection.SetAddress(product - 0x100);
        View.SetAddress(product - 0x80);
        auto builder = injector::GetBranchDestination(getter + 9).as_int();
        BindHook = safetyhook::create_inline(b, &BindTarget);
        CreateHook = safetyhook::create_inline(create.get_first(), &CreateTargets);
        MatrixHook = safetyhook::create_inline(reinterpret_cast<void*>(builder), &BuildMatrices);
        BeginHook = safetyhook::create_mid(begin.get_first(), [](SafetyHookContext&) { BeginWorld(); });
        EndHook = safetyhook::create_mid(end.get_first(8), [](SafetyHookContext&) { EndWorld(); });
        HooksReady = BindHook && CreateHook && MatrixHook && BeginHook && EndHook;
        settings.Refresh("GRAPHICS.AntiAliasing");
    }
}
