#pragma once

// Temporal anti-aliasing for the Direct3D 11 games of the fixes (x86): the fixes' own TAA, and DLAA (NVIDIA DLSS at the
// render size) and AMD FSR native anti-aliasing, which FusionFixHub.exe runs through its upscaler bridge. Native
// resolution only. The Direct3D 11 counterpart of AntiAliasing.hpp, whose API-independent parts it uses as they are:
// Mode, Backend, Options, Configure's log, the matrices, the Halton jitter (JitterProjection) and the shaders.
//
// - TAA works on any Direct3D 11 device.
// - DLAA and FSR need FusionFixHub.exe and the vendor runtimes in the game's folder (the folder of its exe), nothing
//   else: the game's own device opens the hub's shared textures (ID3D11Device1::OpenSharedResource1) and its shared
//   fence (ID3D11Device5::OpenSharedFence, Windows 10 1703), and the copies, the fence's signal and the wait for the
//   result run on the game's immediate context (ID3D11DeviceContext4). No DXVK.
//
// What a game provides (everything else is here):
// - TemporalAA11::Configure() once.
// - The main camera once per frame, on the thread that builds the frame: OnCamera() with the view and the unjittered
//   projection, which returns the jitter; the game moves the projection the scene is drawn with by it
//   (JitterProjection). Only the main scene's view: anything jittered that isn't resolved afterwards shows the jitter.
// - Engines that draw a frame on a render thread while the next one is built (RAGE): OnRenderedCamera() on the render
//   thread with the view and the projection of each viewport that is set for drawing. The frame whose jittered camera
//   that is becomes the one Render() resolves; a frame no viewport matched (a menu's own camera) isn't resolved.
//   Without OnRenderedCamera, Render() resolves the newest camera.
// - Render() where the scene is complete and the HUD isn't drawn yet, on the thread of the immediate context, every
//   frame whatever the mode (it also starts the hub): the scene (render size, not multisampled), the scene's
//   depth-stencil texture (standard [0, 1] depth, typeless D24S8, D32 or D32S8 with a shader resource binding) and the
//   output (may be the scene). LDR or HDR as the game's scene there (Options::LowDynamicRange).
// - OnResize() before the swap chain's buffers are resized (the cached views hold the back buffer), Shutdown() when the
//   game ends.
//
// Shaders: AntiAliasingD3D11.rc embeds shaders/d3d11/*.cso, the Direct3D 9 sources (shaders/*.hlsl) compiled with
// fxc /T ps_5_0 /E main /Gec (compatibility mode keeps tex2Dlod, sampler2D and the c registers; the c registers become
// the offsets of the $Globals constant buffer, but the textures are numbered by use, so their slots are read from each
// shader's reflection data), and shaders/d3d11/Fullscreen.hlsl with /T vs_5_0 /E main.
//
// One API per game: AntiAliasing.hpp's TemporalAA (D3D9) and TemporalAA11 have state of their own.

#include <d3d11_4.h>
#include <dxgi.h>
#include <vector>
#include "AntiAliasing.hpp"

namespace WFPAA
{
    namespace detail
    {
        // -----------------------------------------------------------------------------------------------------------
        // Direct3D 11 side of the bridge: the hub's shared textures and fence opened on the game's device

        class D3D11Bridge
        {
        public:
            LUID luid{};
            uint32_t vendorId = 0;
            uint32_t width = 0;
            uint32_t height = 0;

            ID3D11Device5* device = nullptr;
            ID3D11DeviceContext4* context = nullptr;
            std::array<ID3D11Texture2D*, TextureCount> textures{};
            ID3D11Fence* fence = nullptr;

            static constexpr DXGI_FORMAT Formats[TextureCount] =
            {
                DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_R32_FLOAT, DXGI_FORMAT_R16G16_FLOAT, DXGI_FORMAT_R16_FLOAT, DXGI_FORMAT_R16G16B16A16_FLOAT
            };

            bool Init(ID3D11Device* d3dDevice)
            {
                if (FAILED(d3dDevice->QueryInterface(IID_PPV_ARGS(&device))) || !device)
                    return false;
                ID3D11DeviceContext* immediate = nullptr;
                device->GetImmediateContext(&immediate);
                bool ok = immediate && SUCCEEDED(immediate->QueryInterface(IID_PPV_ARGS(&context))) && context;
                SafeRelease(immediate);
                if (!ok)
                    return false;

                IDXGIDevice* dxgiDevice = nullptr;
                IDXGIAdapter* adapter = nullptr;
                DXGI_ADAPTER_DESC desc{};
                ok = SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&dxgiDevice))) && SUCCEEDED(dxgiDevice->GetAdapter(&adapter)) && SUCCEEDED(adapter->GetDesc(&desc));
                SafeRelease(adapter);
                SafeRelease(dxgiDevice);
                luid = desc.AdapterLuid;
                vendorId = desc.VendorId;
                return ok;
            }

            void ReleaseImports()
            {
                for (auto& texture : textures)
                    SafeRelease(texture);
                SafeRelease(fence);
                width = height = 0;
            }

            void Release()
            {
                ReleaseImports();
                SafeRelease(context);
                SafeRelease(device);
            }

            // Opens the shared textures and the fence of a configuration; the client closes the handles
            bool Import(Protocol::Shared& shared, uint32_t w, uint32_t h)
            {
                ReleaseImports();

                for (size_t i = 0; i < textures.size(); ++i)
                {
                    auto handle = reinterpret_cast<HANDLE>(shared.TextureHandles[i]);
                    auto hr = handle ? device->OpenSharedResource1(handle, IID_PPV_ARGS(&textures[i])) : E_HANDLE;
                    if (FAILED(hr) || !textures[i])
                    {
                        Log("Bridge: shared texture %u couldn't be opened (0x%08X)", static_cast<uint32_t>(i), static_cast<uint32_t>(hr));
                        ReleaseImports();
                        return false;
                    }
                }
                auto fenceHandle = reinterpret_cast<HANDLE>(shared.FenceHandle);
                auto hr = fenceHandle ? device->OpenSharedFence(fenceHandle, IID_PPV_ARGS(&fence)) : E_HANDLE;
                if (FAILED(hr) || !fence)
                {
                    Log("Bridge: the shared fence couldn't be opened (0x%08X)", static_cast<uint32_t>(hr));
                    ReleaseImports();
                    return false;
                }

                width = w;
                height = h;
                return true;
            }

            bool Matches(ID3D11Texture2D* texture, size_t index) const
            {
                D3D11_TEXTURE2D_DESC desc{};
                texture->GetDesc(&desc);
                return desc.Format == Formats[index] && desc.Width == width && desc.Height == height && desc.SampleDesc.Count == 1;
            }

            // Game textures -> shared textures, then the fence reaches signalValue. Null inputs are skipped.
            bool SubmitInputs(ID3D11Texture2D* const (&inputs)[InputCount], uint64_t signalValue)
            {
                for (size_t i = 0; i < InputCount; ++i)
                    if (inputs[i] && !Matches(inputs[i], i))
                        return false;

                for (size_t i = 0; i < InputCount; ++i)
                    if (inputs[i])
                        context->CopyResource(textures[i], inputs[i]);
                // The hub's queue waits for this value: it must reach the GPU now
                if (FAILED(context->Signal(fence, signalValue)))
                    return false;
                context->Flush();
                return true;
            }

            // Once the fence reaches waitValue: shared output -> game texture
            bool SubmitOutput(ID3D11Texture2D* target, uint64_t waitValue)
            {
                if (!Matches(target, OutputIndex) || FAILED(context->Wait(fence, waitValue)))
                    return false;
                context->CopyResource(target, textures[OutputIndex]);
                return true;
            }
        };

        struct UpscalerFrame11
        {
            ID3D11Texture2D* Color = nullptr;       // R16G16B16A16_FLOAT
            ID3D11Texture2D* Depth = nullptr;       // R32_FLOAT, standard [0, 1] depth
            ID3D11Texture2D* Motion = nullptr;      // R16G16_FLOAT, previous - current in texture coordinates
            ID3D11Texture2D* Output = nullptr;      // R16G16B16A16_FLOAT
            uint32_t Width = 0;
            uint32_t Height = 0;
            Jitter JitterPixels;
            float CameraNear = 0.1f;
            float CameraFar = 1000.0f;
            float CameraFovY = 1.0f;
            float FrameTimeMs = 16.6f;
            uint32_t DLSSPreset = 0;
            bool Reset = false;
        };

        // The hub for a Direct3D 11 game: as Upscaler (AntiAliasing.hpp) with the D3D11 bridge
        class Upscaler11
        {
        public:
            enum class State
            {
                Idle, Starting, Ready, Failed
            };

            std::atomic<State> state = State::Idle;
            D3D11Bridge bridge;
            Protocol::Client hub;
            std::atomic<bool> dlssAvailable = false;
            std::atomic<bool> fsrAvailable = false;
            std::atomic<uint32_t> generation = 0;

            uint32_t configuredBackend = 0;
            uint32_t configuredWidth = 0;
            uint32_t configuredHeight = 0;
            uint32_t configuredPreset = 0;
            bool configureFailed = false;
            uint64_t fenceValue = 0;

            void Fail(const char* why)
            {
                Log("Bridge: %s, DLAA and FSR are off", why);
                state = State::Failed;
                dlssAvailable = false;
                fsrAvailable = false;
                ++generation;
                bridge.ReleaseImports();
                hub.Stop(true);
            }

            // Every frame: starts the hub once the device is known and DLAA or FSR is chosen (start)
            void Update(ID3D11Device* device, bool start)
            {
                if (state == State::Idle)
                {
                    if (!device || !start)
                        return;
                    state = State::Failed;
                    ++generation;

                    if (!bridge.Init(device))
                    {
                        Log("Bridge: the device has no ID3D11Device5 (Windows 10 1703 or later), DLAA and FSR are off");
                        bridge.Release();
                        return;
                    }

                    // Nothing to offer: no FusionFixHub.exe, or no runtime for this GPU (DLSS needs an NVIDIA GPU)
                    auto root = Protocol::GameRoot();
                    if (!Protocol::Present(root, bridge.vendorId == 0x10DE))
                    {
                        Log("Bridge: FusionFixHub.exe or the vendor runtimes in the game's folder are missing");
                        return;
                    }

                    auto& config = Config();
                    if (!hub.Start(root, bridge.luid, config.BridgeName, config.ClientName.c_str(), {}))
                    {
                        Log("Bridge: FusionFixHub.exe didn't start (%u)", GetLastError());
                        hub.Stop(false);
                        return;
                    }
                    state = State::Starting;
                }

                if (state == State::Starting)
                {
                    bool ok = false;
                    if (!hub.PollStarted(ok))
                        return;
                    if (!ok)
                    {
                        Fail(hub.shared && hub.shared->Message[0] ? hub.shared->Message : "the hub didn't start");
                        return;
                    }
                    dlssAvailable = hub.IsAvailable(Protocol::Backend::DLSS);
                    fsrAvailable = hub.IsAvailable(Protocol::Backend::FSR);
                    state = State::Ready;
                    ++generation;
                }
            }

            bool IsAvailable(Backend backend) const
            {
                return backend == Backend::DLSS ? dlssAvailable.load() : fsrAvailable.load();
            }

            // Frame.Color into Frame.Output; false leaves Output untouched
            bool Evaluate(Backend backend, const UpscalerFrame11& frame)
            {
                if (state != State::Ready || !IsAvailable(backend) || !frame.Color || !frame.Depth || !frame.Motion || !frame.Output)
                    return false;

                auto backendId = static_cast<uint32_t>(backend);
                bool reconfigure = configuredBackend != backendId || configuredWidth != frame.Width || configuredHeight != frame.Height ||
                    configuredPreset != frame.DLSSPreset;
                if (reconfigure)
                {
                    bridge.ReleaseImports();
                    configuredBackend = backendId;
                    configuredWidth = frame.Width;
                    configuredHeight = frame.Height;
                    configuredPreset = frame.DLSSPreset;
                    configureFailed = true;

                    auto& shared = *hub.shared;
                    shared.ConfigureBackend = static_cast<Protocol::Backend>(backendId);
                    shared.Width = frame.Width;
                    shared.Height = frame.Height;
                    // Native anti-aliasing: the output has the render size
                    shared.OutputWidth = frame.Width;
                    shared.OutputHeight = frame.Height;
                    shared.DLSSPreset = frame.DLSSPreset;
                    shared.Flags = (Config().LowDynamicRange ? Protocol::ConfigureFlags::LowDynamicRange : 0u) | Protocol::ConfigureFlags::SharedWithD3D11;
                    if (!hub.Request(Protocol::Command::Configure, 15000))
                    {
                        if (hub.Exited())
                            Fail("the hub ended");
                        else
                            Log("Bridge: %s %ux%u not configured (%s)", backend == Backend::DLSS ? "DLSS" : "FSR", frame.Width, frame.Height, shared.Message);
                        return false;
                    }
                    bool imported = bridge.Import(shared, frame.Width, frame.Height);
                    hub.CloseSharedHandles();
                    if (!imported)
                    {
                        Log("Bridge: the shared textures couldn't be opened on the game's device");
                        return false;
                    }

                    configureFailed = false;
                    fenceValue = 0;
                }
                else if (configureFailed)
                {
                    // A failed configuration is not retried every frame, only when something changes
                    return false;
                }

                auto inputValue = ++fenceValue;
                auto outputValue = ++fenceValue;

                ID3D11Texture2D* const inputs[InputCount] = { frame.Color, frame.Depth, frame.Motion, nullptr };
                if (!bridge.SubmitInputs(inputs, inputValue))
                {
                    // Nothing was signalled, the hub was not asked for this frame. Retried when the mode or size changes.
                    Log("Bridge: the inputs couldn't be copied");
                    configureFailed = true;
                    return false;
                }

                auto& shared = *hub.shared;
                shared.WaitValue = inputValue;
                shared.SignalValue = outputValue;
                shared.JitterX = frame.JitterPixels.X;
                shared.JitterY = frame.JitterPixels.Y;
                shared.MotionScaleX = static_cast<float>(frame.Width);
                shared.MotionScaleY = static_cast<float>(frame.Height);
                shared.CameraNear = frame.CameraNear;
                shared.CameraFar = frame.CameraFar;
                shared.CameraFovY = frame.CameraFovY;
                shared.FrameTimeMs = frame.FrameTimeMs;
                // The library sharpens all temporal modes alike
                shared.Sharpness = 0.0f;
                shared.Reset = frame.Reset ? 1 : 0;

                // The hub answers once its GPU work, which signals outputValue, has been submitted
                if (!hub.Request(Protocol::Command::Evaluate, 500))
                {
                    if (hub.Exited())
                        Fail("the hub ended");
                    else
                    {
                        Log("Bridge: evaluate failed (%s)", shared.Message);
                        configureFailed = true;
                    }
                    return false;
                }

                return bridge.SubmitOutput(frame.Output, outputValue);
            }
        };

        inline Upscaler11& TheUpscaler11()
        {
            static Upscaler11 upscaler;
            return upscaler;
        }

        // -----------------------------------------------------------------------------------------------------------
        // The texture and sampler slots of a compiled shader, from its reflection data (the RDEF chunk of the DXBC
        // container), without d3dcompiler

        struct ShaderBinding
        {
            std::string Name;
            int Texture = -1;
            int Sampler = -1;
        };

        inline std::vector<ShaderBinding> ReflectBindings(const void* code, size_t size)
        {
            std::vector<ShaderBinding> bindings;
            auto bytes = static_cast<const uint8_t*>(code);
            auto read = [&](size_t offset) -> uint32_t
            {
                uint32_t value = 0;
                if (offset + 4 <= size)
                    std::memcpy(&value, bytes + offset, 4);
                return value;
            };
            if (size < 32 || std::memcmp(bytes, "DXBC", 4) != 0)
                return bindings;

            auto chunks = read(28);
            for (uint32_t c = 0; c < chunks; ++c)
            {
                auto chunk = read(32 + c * 4);
                if (chunk + 8 > size || std::memcmp(bytes + chunk, "RDEF", 4) != 0)
                    continue;
                auto data = chunk + 8;
                auto count = read(data + 8);
                auto table = data + read(data + 12);
                for (uint32_t i = 0; i < count; ++i)
                {
                    // name offset, type, return type, dimension, samples, bind point, bind count, flags
                    auto entry = table + i * 32;
                    auto nameOffset = data + read(entry);
                    auto type = read(entry + 4);
                    auto point = static_cast<int>(read(entry + 20));
                    if (nameOffset >= size || (type != D3D_SIT_TEXTURE && type != D3D_SIT_SAMPLER))
                        continue;
                    std::string name(reinterpret_cast<const char*>(bytes + nameOffset), strnlen(reinterpret_cast<const char*>(bytes + nameOffset), size - nameOffset));
                    auto it = std::find_if(bindings.begin(), bindings.end(), [&](const ShaderBinding& b) { return b.Name == name; });
                    if (it == bindings.end())
                        it = bindings.insert(bindings.end(), ShaderBinding{ name });
                    (type == D3D_SIT_TEXTURE ? it->Texture : it->Sampler) = point;
                }
            }
            return bindings;
        }
    }

    // ---------------------------------------------------------------------------------------------------------------
    // Temporal anti-aliasing on Direct3D 11

    class TemporalAA11
    {
        struct FrameCamera
        {
            Matrix View;
            Matrix Projection;              // without jitter
            Matrix ViewProjection;          // without jitter
            float Rendered[32]{};           // the view and the jittered projection as the game draws with them
            Jitter JitterPixels;
            float JitterNdc[2]{};           // how far the content moved in NDC (x right, y up)
            float Near = 0.0f;
            float Far = 0.0f;
            float FovY = 0.0f;
            uint32_t Width = 0;
            uint32_t Height = 0;
            uint32_t Frame = 0;
            bool Valid = false;
        };

        struct Pass
        {
            ID3D11PixelShader* Shader = nullptr;
            std::vector<detail::ShaderBinding> Bindings;

            void Release()
            {
                detail::SafeRelease(Shader);
                Bindings.clear();
            }
        };

        struct Input
        {
            const char* Name;
            ID3D11ShaderResourceView* View;
            bool Linear;
        };

        struct Target
        {
            ID3D11Texture2D* Texture = nullptr;
            ID3D11RenderTargetView* RenderTarget = nullptr;
            ID3D11ShaderResourceView* Resource = nullptr;

            void Release()
            {
                detail::SafeRelease(Resource);
                detail::SafeRelease(RenderTarget);
                detail::SafeRelease(Texture);
            }
        };

        // Everything the passes change, put back afterwards
        struct SavedState
        {
            ID3D11RenderTargetView* RenderTargets[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT]{};
            ID3D11DepthStencilView* DepthStencil = nullptr;
            ID3D11BlendState* Blend = nullptr;
            FLOAT BlendFactor[4]{};
            UINT SampleMask = 0xFFFFFFFF;
            ID3D11DepthStencilState* DepthStencilState = nullptr;
            UINT StencilRef = 0;
            ID3D11RasterizerState* Rasterizer = nullptr;
            UINT Viewports = 0;
            D3D11_VIEWPORT ViewportList[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE]{};
            ID3D11InputLayout* InputLayout = nullptr;
            D3D11_PRIMITIVE_TOPOLOGY Topology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
            ID3D11VertexShader* VertexShader = nullptr;
            ID3D11GeometryShader* GeometryShader = nullptr;
            ID3D11HullShader* HullShader = nullptr;
            ID3D11DomainShader* DomainShader = nullptr;
            ID3D11PixelShader* PixelShader = nullptr;
            ID3D11ShaderResourceView* Resources[8]{};
            ID3D11SamplerState* Samplers[8]{};
            ID3D11Buffer* Constants = nullptr;
        };

        static constexpr size_t RecordedCount = 8;
        static constexpr UINT ConstantBytes = 8 * 16;

        // Between the thread that builds the frames and the render thread
        static inline std::mutex mutex;
        static inline std::array<FrameCamera, RecordedCount> recorded{};
        static inline uint32_t recordedNext = 0;
        static inline FrameCamera rendered;                         // the camera OnRenderedCamera found for this frame
        static inline bool renderedMatched = false;
        static inline bool renderedByViewport = false;              // the game calls OnRenderedCamera
        static inline std::atomic<Mode> requested = Mode::Off;
        static inline std::atomic<float> sharpness = 0.0f;
        static inline std::atomic<uint32_t> frame = 0;              // advanced by OnCamera, once per frame
        static inline std::atomic<uint32_t> resolvedFrame = 0;      // camera frame of the last resolve
        static inline std::atomic<uint32_t> stateGeneration = 0;
        static inline std::atomic<bool> running = false;            // Render ran on a Direct3D 11 device
        static inline std::atomic<bool> depthUsable = true;         // false: the depth is multisampled or unreadable

        // Render thread
        static inline FrameCamera current;
        static inline FrameCamera previous;                         // the camera of the last resolve
        static inline uint32_t renderCall = 0;                      // Render calls
        static inline uint32_t resolveCall = 0;                     // the call of the last resolve
        static inline uint32_t historyCall = 0;                     // the call the TAA history was written in
        static inline uint32_t upscalerCall = 0;                    // the call of the last DLAA or FSR frame
        static inline uint32_t depthCall = 0;                       // the call depthTex[depthIndex] was written in
        static inline uint32_t historyIndex = 0;
        static inline uint32_t depthIndex = 0;
        static inline LARGE_INTEGER lastTime{};

        // Resources of the device
        static inline ID3D11Device* device = nullptr;               // not held, only compared
        static inline ID3D11DeviceContext* context = nullptr;       // the immediate context, held while Render runs
        static inline ID3D11VertexShader* fullscreenVS = nullptr;
        static inline Pass cameraMotionPass, resolvePass, outputPass, copyPass, depthPass;
        static inline ID3D11SamplerState* pointSampler = nullptr;
        static inline ID3D11SamplerState* linearSampler = nullptr;
        static inline ID3D11BlendState* blendState = nullptr;
        static inline ID3D11DepthStencilState* depthStencilState = nullptr;
        static inline ID3D11RasterizerState* rasterizerState = nullptr;
        static inline ID3D11Buffer* constantBuffer = nullptr;

        // Of the render size
        static inline Target sceneTex;                              // copy of the scene, the game's format
        static inline Target motionTex;                             // R16G16_FLOAT
        static inline Target historyTex[2];                         // R16G16B16A16_FLOAT
        static inline Target depthTex[2];                           // R32_FLOAT copies of the depth, this frame's and the last one's
        static inline Target colorTex;                              // R16G16B16A16_FLOAT, the scene for DLAA and FSR
        static inline Target upscaledTex;                           // R16G16B16A16_FLOAT, written by DLAA or FSR
        static inline uint32_t width = 0;
        static inline uint32_t height = 0;
        static inline DXGI_FORMAT sceneFormat = DXGI_FORMAT_UNKNOWN;

        // Views of the game's textures, kept while they stay the same
        static inline ID3D11Texture2D* gameDepth = nullptr;
        static inline ID3D11ShaderResourceView* gameDepthView = nullptr;
        static inline ID3D11Texture2D* gameOutput = nullptr;
        static inline ID3D11RenderTargetView* gameOutputView = nullptr;

        static inline bool failed = false;                          // resources or shaders couldn't be made: nothing until a new device

        static bool LoadCode(const wchar_t* name, const void*& code, size_t& size)
        {
            auto module = detail::ThisModule();
            auto resource = FindResourceW(module, name, MAKEINTRESOURCEW(10));     // RT_RCDATA
            auto data = resource ? LoadResource(module, resource) : nullptr;
            code = data ? LockResource(data) : nullptr;
            size = resource ? SizeofResource(module, resource) : 0;
            return code && size;
        }

        static bool LoadPass(const wchar_t* name, Pass& pass)
        {
            if (pass.Shader)
                return true;
            const void* code = nullptr;
            size_t size = 0;
            if (!LoadCode(name, code, size) || FAILED(device->CreatePixelShader(code, size, nullptr, &pass.Shader)) || !pass.Shader)
                return false;
            pass.Bindings = detail::ReflectBindings(code, size);
            return true;
        }

        static bool CreateStates()
        {
            if (constantBuffer)
                return true;

            const void* code = nullptr;
            size_t size = 0;
            if (!LoadCode(L"WFPAA11_FULLSCREEN", code, size) || FAILED(device->CreateVertexShader(code, size, nullptr, &fullscreenVS)))
                return false;

            D3D11_SAMPLER_DESC sampler{};
            sampler.AddressU = sampler.AddressV = sampler.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
            sampler.ComparisonFunc = D3D11_COMPARISON_NEVER;
            sampler.MaxLOD = D3D11_FLOAT32_MAX;
            sampler.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
            if (FAILED(device->CreateSamplerState(&sampler, &pointSampler)))
                return false;
            sampler.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
            if (FAILED(device->CreateSamplerState(&sampler, &linearSampler)))
                return false;

            D3D11_BLEND_DESC blend{};
            blend.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
            D3D11_DEPTH_STENCIL_DESC depthStencil{};
            D3D11_RASTERIZER_DESC rasterizer{};
            rasterizer.FillMode = D3D11_FILL_SOLID;
            rasterizer.CullMode = D3D11_CULL_NONE;
            rasterizer.DepthClipEnable = TRUE;
            if (FAILED(device->CreateBlendState(&blend, &blendState)) || FAILED(device->CreateDepthStencilState(&depthStencil, &depthStencilState)) ||
                FAILED(device->CreateRasterizerState(&rasterizer, &rasterizerState)))
                return false;

            D3D11_BUFFER_DESC buffer{};
            buffer.ByteWidth = ConstantBytes;
            buffer.Usage = D3D11_USAGE_DYNAMIC;
            buffer.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            buffer.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            return SUCCEEDED(device->CreateBuffer(&buffer, nullptr, &constantBuffer));
        }

        static bool CreateTarget(DXGI_FORMAT format, bool renderTarget, Target& target)
        {
            if (target.Texture)
                return true;
            D3D11_TEXTURE2D_DESC desc{};
            desc.Width = width;
            desc.Height = height;
            desc.MipLevels = 1;
            desc.ArraySize = 1;
            desc.Format = format;
            desc.SampleDesc.Count = 1;
            desc.Usage = D3D11_USAGE_DEFAULT;
            desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | (renderTarget ? D3D11_BIND_RENDER_TARGET : 0);
            bool ok = SUCCEEDED(device->CreateTexture2D(&desc, nullptr, &target.Texture)) &&
                SUCCEEDED(device->CreateShaderResourceView(target.Texture, nullptr, &target.Resource)) &&
                (!renderTarget || SUCCEEDED(device->CreateRenderTargetView(target.Texture, nullptr, &target.RenderTarget)));
            if (!ok)
                target.Release();
            return ok;
        }

        static void ReleaseTargets()
        {
            sceneTex.Release();
            motionTex.Release();
            historyTex[0].Release();
            historyTex[1].Release();
            depthTex[0].Release();
            depthTex[1].Release();
            colorTex.Release();
            upscaledTex.Release();
            width = height = 0;
            sceneFormat = DXGI_FORMAT_UNKNOWN;
            depthCall = 0;
            historyCall = 0;
            upscalerCall = 0;
        }

        static void ReleaseGameViews()
        {
            detail::SafeRelease(gameDepthView);
            detail::SafeRelease(gameOutputView);
            gameDepth = nullptr;
            gameOutput = nullptr;
        }

        static void ReleaseDevice()
        {
            ReleaseTargets();
            ReleaseGameViews();
            for (auto pass : { &cameraMotionPass, &resolvePass, &outputPass, &copyPass, &depthPass })
                pass->Release();
            detail::SafeRelease(fullscreenVS);
            detail::SafeRelease(pointSampler);
            detail::SafeRelease(linearSampler);
            detail::SafeRelease(blendState);
            detail::SafeRelease(depthStencilState);
            detail::SafeRelease(rasterizerState);
            detail::SafeRelease(constantBuffer);
            device = nullptr;
            failed = false;
        }

        static bool EnsureResources(uint32_t w, uint32_t h, DXGI_FORMAT format, bool bridge)
        {
            if (failed)
                return false;
            if (w != width || h != height || format != sceneFormat)
            {
                ReleaseTargets();
                width = w;
                height = h;
                sceneFormat = format;
            }

            bool ok = CreateStates() && LoadPass(L"WFPAA11_CAMERAMOTION", cameraMotionPass) && LoadPass(L"WFPAA11_RESOLVE", resolvePass) &&
                LoadPass(L"WFPAA11_OUTPUT", outputPass) && LoadPass(L"WFPAA11_COPY", copyPass) && LoadPass(L"WFPAA11_DEPTH", depthPass);
            ok = ok && CreateTarget(format, false, sceneTex) && CreateTarget(DXGI_FORMAT_R16G16_FLOAT, true, motionTex) &&
                CreateTarget(DXGI_FORMAT_R16G16B16A16_FLOAT, true, historyTex[0]) && CreateTarget(DXGI_FORMAT_R16G16B16A16_FLOAT, true, historyTex[1]) &&
                CreateTarget(DXGI_FORMAT_R32_FLOAT, true, depthTex[0]) && CreateTarget(DXGI_FORMAT_R32_FLOAT, true, depthTex[1]);
            // DLAA and FSR without them fall back to TAA (RenderUpscaler)
            if (ok && bridge && !(CreateTarget(DXGI_FORMAT_R16G16B16A16_FLOAT, true, colorTex) && CreateTarget(DXGI_FORMAT_R16G16B16A16_FLOAT, true, upscaledTex)))
            {
                colorTex.Release();
                upscaledTex.Release();
            }
            if (!ok)
            {
                detail::Log("TAA: resources or shaders couldn't be created at %ux%u (format %u)", w, h, format);
                failed = true;
                ReleaseTargets();
            }
            return ok;
        }

        // A shader resource view of the scene's depth: typeless depth formats are read as their depth channel
        static ID3D11ShaderResourceView* DepthView(ID3D11Texture2D* depth)
        {
            if (depth == gameDepth)
                return gameDepthView;
            detail::SafeRelease(gameDepthView);
            gameDepth = depth;

            D3D11_TEXTURE2D_DESC desc{};
            depth->GetDesc(&desc);
            D3D11_SHADER_RESOURCE_VIEW_DESC view{};
            view.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            view.Texture2D.MipLevels = 1;
            switch (desc.Format)
            {
            case DXGI_FORMAT_R24G8_TYPELESS: view.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS; break;
            case DXGI_FORMAT_R32G8X24_TYPELESS: view.Format = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS; break;
            case DXGI_FORMAT_R32_TYPELESS: case DXGI_FORMAT_R32_FLOAT: view.Format = DXGI_FORMAT_R32_FLOAT; break;
            case DXGI_FORMAT_R16_TYPELESS: case DXGI_FORMAT_R16_UNORM: view.Format = DXGI_FORMAT_R16_UNORM; break;
            default: view.Format = DXGI_FORMAT_UNKNOWN; break;
            }

            bool usable = desc.SampleDesc.Count == 1 && (desc.BindFlags & D3D11_BIND_SHADER_RESOURCE) && view.Format != DXGI_FORMAT_UNKNOWN &&
                SUCCEEDED(device->CreateShaderResourceView(depth, &view, &gameDepthView));
            if (!usable)
            {
                detail::SafeRelease(gameDepthView);
                if (desc.SampleDesc.Count > 1)
                    detail::Log("Depth: the depth is multisampled, temporal anti-aliasing needs the game's MSAA off");
                else
                    detail::Log("Depth: the depth (format %u, bind flags 0x%X) can't be read", desc.Format, desc.BindFlags);
            }
            if (usable != depthUsable.exchange(usable))
                ++stateGeneration;
            return gameDepthView;
        }

        static ID3D11RenderTargetView* OutputView(ID3D11Texture2D* output)
        {
            if (output == gameOutput)
                return gameOutputView;
            detail::SafeRelease(gameOutputView);
            gameOutput = output;
            if (FAILED(device->CreateRenderTargetView(output, nullptr, &gameOutputView)))
            {
                detail::Log("Output: no render target view of the output");
                gameOutputView = nullptr;
            }
            return gameOutputView;
        }

        static void SaveState(SavedState& s)
        {
            context->OMGetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, s.RenderTargets, &s.DepthStencil);
            context->OMGetBlendState(&s.Blend, s.BlendFactor, &s.SampleMask);
            context->OMGetDepthStencilState(&s.DepthStencilState, &s.StencilRef);
            context->RSGetState(&s.Rasterizer);
            s.Viewports = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
            context->RSGetViewports(&s.Viewports, s.ViewportList);
            context->IAGetInputLayout(&s.InputLayout);
            context->IAGetPrimitiveTopology(&s.Topology);
            context->VSGetShader(&s.VertexShader, nullptr, nullptr);
            context->GSGetShader(&s.GeometryShader, nullptr, nullptr);
            context->HSGetShader(&s.HullShader, nullptr, nullptr);
            context->DSGetShader(&s.DomainShader, nullptr, nullptr);
            context->PSGetShader(&s.PixelShader, nullptr, nullptr);
            context->PSGetShaderResources(0, 8, s.Resources);
            context->PSGetSamplers(0, 8, s.Samplers);
            context->PSGetConstantBuffers(0, 1, &s.Constants);
        }

        static void RestoreState(SavedState& s)
        {
            context->OMSetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, s.RenderTargets, s.DepthStencil);
            context->OMSetBlendState(s.Blend, s.BlendFactor, s.SampleMask);
            context->OMSetDepthStencilState(s.DepthStencilState, s.StencilRef);
            context->RSSetState(s.Rasterizer);
            context->RSSetViewports(s.Viewports, s.ViewportList);
            context->IASetInputLayout(s.InputLayout);
            context->IASetPrimitiveTopology(s.Topology);
            context->VSSetShader(s.VertexShader, nullptr, 0);
            context->GSSetShader(s.GeometryShader, nullptr, 0);
            context->HSSetShader(s.HullShader, nullptr, 0);
            context->DSSetShader(s.DomainShader, nullptr, 0);
            context->PSSetShader(s.PixelShader, nullptr, 0);
            context->PSSetShaderResources(0, 8, s.Resources);
            context->PSSetSamplers(0, 8, s.Samplers);
            context->PSSetConstantBuffers(0, 1, &s.Constants);

            // The references the Get calls handed out
            for (auto& target : s.RenderTargets)
                detail::SafeRelease(target);
            detail::SafeRelease(s.DepthStencil);
            detail::SafeRelease(s.Blend);
            detail::SafeRelease(s.DepthStencilState);
            detail::SafeRelease(s.Rasterizer);
            detail::SafeRelease(s.InputLayout);
            detail::SafeRelease(s.VertexShader);
            detail::SafeRelease(s.GeometryShader);
            detail::SafeRelease(s.HullShader);
            detail::SafeRelease(s.DomainShader);
            detail::SafeRelease(s.PixelShader);
            for (auto& resource : s.Resources)
                detail::SafeRelease(resource);
            for (auto& sampler : s.Samplers)
                detail::SafeRelease(sampler);
            detail::SafeRelease(s.Constants);
        }

        static void SetFullscreenStates()
        {
            ID3D11ShaderResourceView* none[8]{};
            context->PSSetShaderResources(0, 8, none);
            context->OMSetBlendState(blendState, nullptr, 0xFFFFFFFF);
            context->OMSetDepthStencilState(depthStencilState, 0);
            context->RSSetState(rasterizerState);
            context->IASetInputLayout(nullptr);
            context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            context->VSSetShader(fullscreenVS, nullptr, 0);
            context->GSSetShader(nullptr, nullptr, 0);
            context->HSSetShader(nullptr, nullptr, 0);
            context->DSSetShader(nullptr, nullptr, 0);
            context->PSSetConstantBuffers(0, 1, &constantBuffer);
            D3D11_VIEWPORT viewport{ 0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f };
            context->RSSetViewports(1, &viewport);
        }

        // A fullscreen pass: the inputs go to the slots the shader's reflection names for them
        static void Draw(ID3D11RenderTargetView* target, const Pass& pass, std::initializer_list<Input> inputs, const float* constants = nullptr, size_t count = 0)
        {
            if (constants && count)
            {
                D3D11_MAPPED_SUBRESOURCE mapped{};
                if (SUCCEEDED(context->Map(constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
                {
                    std::memcpy(mapped.pData, constants, std::min<size_t>(count * sizeof(float), ConstantBytes));
                    context->Unmap(constantBuffer, 0);
                }
            }

            context->OMSetRenderTargets(1, &target, nullptr);
            for (auto& input : inputs)
            {
                auto it = std::find_if(pass.Bindings.begin(), pass.Bindings.end(), [&](const detail::ShaderBinding& b) { return b.Name == input.Name; });
                if (it == pass.Bindings.end())
                    continue;
                if (it->Texture >= 0)
                    context->PSSetShaderResources(it->Texture, 1, &input.View);
                if (it->Sampler >= 0)
                    context->PSSetSamplers(it->Sampler, 1, input.Linear ? &linearSampler : &pointSampler);
            }
            context->PSSetShader(pass.Shader, nullptr, 0);
            context->Draw(3, 0);

            // Unbound, so the next pass can draw into them
            ID3D11ShaderResourceView* none[8]{};
            context->PSSetShaderResources(0, 8, none);
        }

        static bool IsCameraCut()
        {
            auto currentInverse = current.View.Inverse();
            auto previousInverse = previous.View.Inverse();
            auto dx = currentInverse.m[3][0] - previousInverse.m[3][0];
            auto dy = currentInverse.m[3][1] - previousInverse.m[3][1];
            auto dz = currentInverse.m[3][2] - previousInverse.m[3][2];
            auto distance = static_cast<double>(detail::Config().CameraCutDistance);
            if (dx * dx + dy * dy + dz * dz > distance * distance)
                return true;

            // Forward vectors more than ~60 degrees apart
            auto dot = currentInverse.m[2][0] * previousInverse.m[2][0] + currentInverse.m[2][1] * previousInverse.m[2][1] +
                currentInverse.m[2][2] * previousInverse.m[2][2];
            return dot < 0.5;
        }

        // The last resolve was the call before this one, of this camera frame or the one before (a frame drawn again
        // keeps its camera, e.g. while the game is paused)
        static bool PreviousUsable()
        {
            return previous.Valid && resolveCall != 0 && resolveCall + 1 == renderCall && current.Frame - previous.Frame <= 1;
        }

        static bool HistoryUsable(uint32_t since)
        {
            return since != 0 && since + 1 == renderCall && PreviousUsable() && !IsCameraCut();
        }

        static void RenderCameraMotion(ID3D11ShaderResourceView* depth)
        {
            // This frame's depth as R32F: the next frame checks its motion against it, DLAA and FSR take it
            auto previousDepth = depthIndex;
            bool previousValid = depthCall != 0 && depthCall + 1 == renderCall && PreviousUsable();
            depthIndex ^= 1u;
            Draw(depthTex[depthIndex].RenderTarget, depthPass, { { "SceneDepth", depth, false } });
            depthCall = renderCall;

            // current clip space -> previous clip space, without jitter
            auto& last = PreviousUsable() ? previous : current;
            auto reproject = current.ViewProjection.Inverse() * last.ViewProjection;

            float constants[7 * 4]{};
            constants[0] = current.JitterNdc[0];
            constants[1] = current.JitterNdc[1];
            reproject.To(&constants[4]);
            constants[20] = current.Near;
            constants[21] = current.Far;
            constants[22] = previousValid ? 1.0f : 0.0f;
            constants[24] = 1.0f / static_cast<float>(width);
            constants[25] = 1.0f / static_cast<float>(height);

            Draw(motionTex.RenderTarget, cameraMotionPass, { { "SceneDepth", depth, false }, { "PreviousDepth", depthTex[previousDepth].Resource, false } },
                constants, std::size(constants));
        }

        static void RenderOutput(ID3D11ShaderResourceView* source, ID3D11RenderTargetView* output)
        {
            float constants[4] = { 1.0f / static_cast<float>(width), 1.0f / static_cast<float>(height), sharpness.load(), 0.0f };
            Draw(output, outputPass, { { "Source", source, false } }, constants, std::size(constants));
        }

        static void RenderResolve(ID3D11ShaderResourceView* depth)
        {
            auto& config = detail::Config();
            auto previousIndex = historyIndex;
            auto currentIndex = historyIndex ^ 1u;
            bool historyValid = HistoryUsable(historyCall);

            auto w = static_cast<float>(width);
            auto h = static_cast<float>(height);
            float constants[4 * 4] =
            {
                1.0f / w, 1.0f / h, w, h,
                current.JitterPixels.X, current.JitterPixels.Y, historyValid ? 1.0f : 0.0f, config.VarianceClipGamma,
                config.CurrentWeightMin, config.CurrentWeightMax, config.CurrentWeightPerPixel, config.LumaWeight,
                config.StaticHistoryTrust, 0.1f, 0.0f, 0.0f,
            };

            Draw(historyTex[currentIndex].RenderTarget, resolvePass,
                {
                    { "CurrentTex", sceneTex.Resource, false }, { "HistoryTex", historyTex[previousIndex].Resource, true },
                    { "MotionTex", motionTex.Resource, false }, { "DepthTex", depth, false },
                }, constants, std::size(constants));

            historyIndex = currentIndex;
            historyCall = renderCall;
        }

        static bool RenderUpscaler(Backend backend)
        {
            if (!colorTex.Texture || !upscaledTex.Texture)
                return false;

            // The scene in the format of the shared texture (the depth is in depthTex already)
            Draw(colorTex.RenderTarget, copyPass, { { "Source", sceneTex.Resource, false } });

            LARGE_INTEGER now, frequency;
            QueryPerformanceCounter(&now);
            QueryPerformanceFrequency(&frequency);
            auto frameTime = lastTime.QuadPart ? static_cast<float>(static_cast<double>(now.QuadPart - lastTime.QuadPart) * 1000.0 / static_cast<double>(frequency.QuadPart)) : 16.6f;
            lastTime = now;

            detail::UpscalerFrame11 frameData;
            frameData.Color = colorTex.Texture;
            frameData.Depth = depthTex[depthIndex].Texture;
            frameData.Motion = motionTex.Texture;
            frameData.Output = upscaledTex.Texture;
            frameData.Width = width;
            frameData.Height = height;
            frameData.JitterPixels = current.JitterPixels;
            frameData.CameraNear = current.Near;
            frameData.CameraFar = current.Far;
            frameData.CameraFovY = current.FovY;
            frameData.FrameTimeMs = std::clamp(frameTime, 1.0f, 200.0f);
            frameData.DLSSPreset = detail::Config().DLSSPreset;
            frameData.Reset = !HistoryUsable(upscalerCall);

            if (!detail::TheUpscaler11().Evaluate(backend, frameData))
                return false;
            upscalerCall = renderCall;
            return true;
        }

        static bool Matches(const float* a, const float* b, size_t count)
        {
            for (size_t i = 0; i < count; ++i)
                if (std::abs(a[i] - b[i]) > 1e-5f * std::max(1.0f, std::abs(a[i])))
                    return false;
            return true;
        }

        // The camera of the frame being drawn: the one OnRenderedCamera found, or the newest
        static bool TakeCamera()
        {
            std::lock_guard lock(mutex);
            if (renderedByViewport)
            {
                if (!renderedMatched)
                    return false;
                current = rendered;
                renderedMatched = false;
                return true;
            }
            current = recorded[(recordedNext + RecordedCount - 1) % RecordedCount];
            return current.Valid;
        }

    public:
        static void Configure(const Options& options)
        {
            detail::Config() = options;
        }

        // The mode the player chose; DLAA and FSR fall back to TAA while the bridge can't run them
        static void SetMode(Mode mode, float sharpen)
        {
            requested = mode;
            sharpness = std::clamp(sharpen, 0.0f, 1.0f);
        }

        static Mode EffectiveMode()
        {
            switch (requested.load())
            {
            case Mode::DLAA:
                return detail::TheUpscaler11().IsAvailable(Backend::DLSS) ? Mode::DLAA : Mode::TAA;
            case Mode::FSR:
                return detail::TheUpscaler11().IsAvailable(Backend::FSR) ? Mode::FSR : Mode::TAA;
            default:
                return requested.load();
            }
        }

        // Jitter only while the frames are resolved: one that isn't (post processing failed, a screen without the
        // resolve) would show it
        static bool IsJitterActive()
        {
            auto resolved = resolvedFrame.load();
            return requested.load() != Mode::Off && resolved != 0 && frame.load() - resolved <= 10;
        }

        // The main camera of this frame, once per frame, with the projection as the game built it. Returns the jitter
        // to move the projection by (JitterProjection), zero while nothing is resolved.
        static Jitter OnCamera(const Camera& camera)
        {
            if (requested.load() == Mode::Off)
                return {};
            FrameCamera c;
            c.View = camera.View;
            c.Projection = camera.Projection;
            c.ViewProjection = camera.View * camera.Projection;
            c.Width = camera.Width;
            c.Height = camera.Height;
            c.Frame = ++frame;

            // Standard [0, 1] depth: depth = (P22 z + P32) / (P23 z), 0 at near, 1 at far
            auto& P = camera.Projection.m;
            if (P[2][2] != 0.0 && P[2][2] != P[2][3])
            {
                c.Near = static_cast<float>(std::abs(-P[3][2] / P[2][2]));
                c.Far = static_cast<float>(std::abs(-P[3][2] / (P[2][2] - P[2][3])));
            }
            c.FovY = P[1][1] != 0.0 ? static_cast<float>(2.0 * std::atan(1.0 / std::abs(P[1][1]))) : 1.0f;
            c.Valid = c.Width > 0 && c.Height > 0 && c.Near > 0.0f && c.Far > c.Near && P[2][3] != 0.0;

            if (c.Valid && IsJitterActive())
            {
                // NVIDIA recommends at least 16 phases for DLSS, 32 or more preferred. One step per frame: the sign
                // and the per-frame order matter (two steps per frame put every sample in one half of the pixel).
                auto phases = std::clamp(detail::Config().JitterPhases, 2u, 128u);
                auto mode = EffectiveMode();
                if (mode == Mode::DLAA)
                    phases = std::max(phases, 32u);
                else if (mode == Mode::FSR)
                    phases = std::max(phases, 16u);
                auto index = (c.Frame % phases) + 1;
                c.JitterPixels.X = Halton(index, 2) - 0.5f;
                c.JitterPixels.Y = Halton(index, 3) - 0.5f;
                c.JitterNdc[0] = 2.0f * c.JitterPixels.X / static_cast<float>(c.Width);
                c.JitterNdc[1] = -2.0f * c.JitterPixels.Y / static_cast<float>(c.Height);
            }

            // What the game will draw with, for OnRenderedCamera
            c.View.To(c.Rendered);
            c.Projection.To(c.Rendered + 16);
            if (c.JitterPixels.X != 0.0f || c.JitterPixels.Y != 0.0f)
                JitterProjection(c.Rendered + 16, c.JitterPixels, static_cast<float>(c.Width), static_cast<float>(c.Height));

            std::lock_guard lock(mutex);
            recorded[recordedNext] = c;
            recordedNext = (recordedNext + 1) % RecordedCount;
            return c.JitterPixels;
        }

        // Render thread: a viewport set for drawing, its view and projection (sixteen floats each, row by row). When
        // they are those of a frame OnCamera recorded, that frame is the one the next Render resolves.
        static void OnRenderedCamera(const float* view, const float* projection)
        {
            std::lock_guard lock(mutex);
            renderedByViewport = true;
            for (size_t i = 1; i <= RecordedCount; ++i)
            {
                auto& c = recorded[(recordedNext + RecordedCount - i) % RecordedCount];
                if (c.Valid && Matches(c.Rendered + 16, projection, 16) && Matches(c.Rendered, view, 16))
                {
                    rendered = c;
                    renderedMatched = true;
                    return;
                }
            }
        }

        // Where the scene is complete and the HUD isn't drawn yet: resolves the scene (render size) into output (may be
        // the same texture). depth: the scene's depth-stencil texture. Returns whether output was written. Also starts
        // the hub once DLAA or FSR is chosen, so call it every frame, whatever the mode.
        static bool Render(ID3D11DeviceContext* immediate, ID3D11Texture2D* scene, ID3D11Texture2D* depth, ID3D11Texture2D* output)
        {
            if (!immediate)
                return false;

            ID3D11Device* contextDevice = nullptr;
            immediate->GetDevice(&contextDevice);
            if (!contextDevice)
                return false;
            if (contextDevice != device)
            {
                // A new device: everything is made again for it
                ReleaseDevice();
                device = contextDevice;
            }
            contextDevice->Release();

            if (!running.exchange(true))
                ++stateGeneration;
            ++renderCall;
            detail::TheUpscaler11().Update(device, requested == Mode::DLAA || requested == Mode::FSR);

            auto mode = EffectiveMode();
            // The depth is checked whatever the mode: it decides whether the temporal modes are offered (IsRunning)
            auto depthView = depth ? DepthView(depth) : nullptr;
            if (!TakeCamera() || mode == Mode::Off || !scene || !depthView || !output)
                return false;

            D3D11_TEXTURE2D_DESC desc{};
            scene->GetDesc(&desc);
            if (desc.Width != current.Width || desc.Height != current.Height || desc.SampleDesc.Count != 1 || !current.Valid)
                return false;
            bool bridge = mode == Mode::DLAA || mode == Mode::FSR;
            if (!EnsureResources(desc.Width, desc.Height, desc.Format, bridge))
                return false;

            context = immediate;
            auto outputView = OutputView(output);
            if (!outputView)
                return false;

            context->CopyResource(sceneTex.Texture, scene);

            SavedState state;
            SaveState(state);
            SetFullscreenStates();
            RenderCameraMotion(depthView);

            bool upscaled = false;
            if (bridge)
            {
                upscaled = RenderUpscaler(mode == Mode::DLAA ? Backend::DLSS : Backend::FSR);
                if (upscaled)
                {
                    // The TAA history didn't follow these frames
                    historyCall = 0;
                    SetFullscreenStates();
                    RenderOutput(upscaledTex.Resource, outputView);
                }
            }
            if (!upscaled)
            {
                upscalerCall = 0;
                RenderResolve(depthView);
                RenderOutput(historyTex[historyIndex].Resource, outputView);
            }
            RestoreState(state);

            previous = current;
            resolveCall = renderCall;
            resolvedFrame = current.Frame;
            return true;
        }

        // Before the swap chain's buffers are resized: the views of the game's textures go
        static void OnResize()
        {
            ReleaseGameViews();
        }

        // When the game ends: the hub ends with it (job object), this only lets it go
        static void Shutdown()
        {
            detail::TheUpscaler11().hub.Stop(false);
        }

        // Whether the temporal modes can run: Render ran on a Direct3D 11 device and the depth could be read
        static bool IsRunning()
        {
            return running.load() && depthUsable.load();
        }

        // Whether DLAA (Backend::DLSS) or FSR can be offered: the hub started and reported the backend, or, while the hub
        // hasn't started yet (it starts only once one of them is chosen), the files are there
        static bool IsAvailable(Backend backend)
        {
            auto& upscaler = detail::TheUpscaler11();
            if (upscaler.state == detail::Upscaler11::State::Idle)
                return detail::BridgeFilesPresent(static_cast<UpscalerBridge::Backend>(backend));
            return upscaler.IsAvailable(backend);
        }

        // Changes whenever IsRunning or IsAvailable may have changed, for settings to ask again
        static uint32_t Generation()
        {
            return detail::TheUpscaler11().generation.load() + stateGeneration.load();
        }

        // Whether DLAA or FSR could become available at all, before the hub runs: FusionFixHub.exe and a vendor
        // runtime are in the game folder
        static bool BridgePossible()
        {
            auto root = UpscalerBridge::GameRoot();
            std::error_code ec;
            return std::filesystem::is_regular_file(UpscalerBridge::HubPath(root), ec) &&
                (UpscalerBridge::RuntimePresent(root, UpscalerBridge::Backend::DLSS) || UpscalerBridge::RuntimePresent(root, UpscalerBridge::Backend::FSR));
        }

        static void Log(const char* text)
        {
            detail::Log("%s", text);
        }
    };
}
