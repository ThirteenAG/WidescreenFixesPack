#pragma once

// Upscaler bridge protocol, shared by FusionFixHub.exe (x64) and the plugins of the fixes (x86 or x64).
// Copy this file and upscaler_bridge_client.h into a plugin unchanged, like the ASI Settings files; the
// originals are in the FusionFixHub repository (source/bridge).
//
// NVIDIA DLSS (NGX) and AMD FSR (FidelityFX API) only exist for 64-bit D3D12 processes. FusionFixHub.exe, in
// the game's root folder next to the ASI loader, runs them on its own D3D12 device, on the adapter the game
// renders with, when it is started with `--upscaler <name>` (no window, no network).
//
// The plugin creates a file mapping holding Shared and two auto-reset events, and starts the hub with their
// base name. Every request is: the plugin writes the parameters and the command, signals <name>.Request and
// waits for <name>.Response. The hub answers each request exactly once, and the start (with the available
// backends) without a request.
//
// GPU synchronization uses a D3D12 fence created by the hub, which the plugin opens on its own queue (e.g.
// imported as a Vulkan timeline semaphore with DXVK, or opened on the D3D12 device of D3D9on12). The plugin
// signals WaitValue once the inputs are copied into the shared textures, the hub waits for it on its queue,
// runs the upscaler and signals SignalValue, which the plugin waits for before copying the output back. The
// hub answers Evaluate after its GPU work is submitted, so the plugin never waits on the GPU for a value that
// nobody will signal.
//
// The vendor runtimes are not shipped with the fixes. The hub's download (upscalers.json in the FusionFixHub
// repository lists them) or a package built for the player puts them into the game's folder, the folder of the
// game's exe (GameDirectory below, where FusionFixHub.exe and the ASI loader are, like DXVK's vulkan.dll), and the
// hub loads them only by full path from there, after checking their vendor's signature:
//   <folder of the game's exe>\nvngx_dlss.dll
//   <folder of the game's exe>\amd_fidelityfx_loader_dx12.dll, amd_fidelityfx_upscaler_dx12.dll

#include <cstdint>
#include <cstddef>

namespace UpscalerBridge
{
    constexpr uint32_t Version = 3;
    constexpr uint32_t PathLength = 520;

    constexpr const wchar_t* HubFileName = L"FusionFixHub.exe";
    constexpr const wchar_t* ArgumentName = L"--upscaler";

    // The files of each backend's runtime, in the game's folder; a backend is offered when all of them are there
    constexpr const wchar_t* DLSSFiles[] = { L"nvngx_dlss.dll" };
    constexpr const wchar_t* FSRFiles[] = { L"amd_fidelityfx_loader_dx12.dll", L"amd_fidelityfx_upscaler_dx12.dll" };

    enum class Command : uint32_t
    {
        None,
        Configure,      // (re)create the shared textures and the upscaler for ConfigureBackend
        Evaluate,       // upscale one frame
        Shutdown,
    };

    enum class Backend : uint32_t
    {
        None,
        DLSS,
        FSR,
    };

    constexpr uint32_t BackendBit(Backend backend)
    {
        return 1u << static_cast<uint32_t>(backend);
    }

    enum class Status : uint32_t
    {
        Pending,
        Ok,
        Failed,
    };

    // Shared textures, the inputs at the render size, Output at the output size:
    // Color     DXGI_FORMAT_R16G16B16A16_FLOAT  scene, HDR unless ConfigureFlags::LowDynamicRange
    // Depth     DXGI_FORMAT_R32_FLOAT           [0, 1] depth, 1 is far unless ConfigureFlags::DepthInverted
    // Motion    DXGI_FORMAT_R16G16_FLOAT        previous - current position in texture coordinates, no jitter
    // Reactive  DXGI_FORMAT_R16_FLOAT           0 to 1, how much a pixel should follow the current frame
    // Output    DXGI_FORMAT_R16G16B16A16_FLOAT  written by the upscaler
    enum class Texture : uint32_t
    {
        Color, Depth, Motion, Reactive, Output, Count
    };

    // The inputs the plugin copies every frame
    constexpr uint32_t InputCount = static_cast<uint32_t>(Texture::Output);

    namespace ConfigureFlags
    {
        constexpr uint32_t ReactiveMask = 1 << 0;       // Reactive is written every frame and used by FSR
        constexpr uint32_t DepthInverted = 1 << 1;      // 1 is near, 0 is far
        constexpr uint32_t LowDynamicRange = 1 << 2;    // Color is in [0, 1]
        constexpr uint32_t SharedWithD3D11 = 1 << 3;    // the plugin opens the textures on a Direct3D 11 device, which can
                                                        // only open render targets with simultaneous access
    }

#pragma pack(push, 8)
    struct Shared
    {
        uint32_t Version;
        uint32_t ClientProcessId;

        // Written by the plugin before starting the hub
        uint32_t AdapterLuidLow;
        int32_t AdapterLuidHigh;
        wchar_t GameDirectory[PathLength];  // the folder of the game's exe, where FusionFixHub.exe and the vendor runtimes are
        wchar_t LogPath[PathLength];        // written by the hub, empty for no log
        char ClientName[64];                // for the log, e.g. the plugin's name

        // Written by the hub, answering the implicit start request
        uint32_t AvailableBackends;         // BackendBit() of each usable backend
        uint32_t Reserved0;
        char Message[512];

        // Request
        Command RequestCommand;
        uint32_t RequestSerial;

        // Configure
        Backend ConfigureBackend;
        uint32_t Width;                     // render size
        uint32_t Height;
        uint32_t OutputWidth;               // output size, at least the render size
        uint32_t OutputHeight;
        uint32_t DLSSPreset;                // NVSDK_NGX_DLSS_Hint_Render_Preset, 0 is the default
        uint32_t Flags;                     // ConfigureFlags
        uint32_t Reserved1;

        // Evaluate
        uint64_t WaitValue;                 // signalled by the plugin when the inputs are ready
        uint64_t SignalValue;               // signalled by the hub when the output is ready
        float JitterX;                      // pixels of the render size: how far the rendered content moved, x right,
                                            // y down (DLSS and FSR take the same value)
        float JitterY;
        float MotionScaleX;                 // converts motion vectors to pixels of the render size
        float MotionScaleY;
        float CameraNear;
        float CameraFar;
        float CameraFovY;                   // radians
        float FrameTimeMs;
        float Sharpness;                    // FSR, 0 is off
        uint32_t Reset;                     // the history does not belong to this frame (camera cut)

        // Response
        uint32_t ResponseSerial;
        Status ResponseStatus;

        // Configure results, handles already duplicated into the plugin's process
        uint64_t TextureHandles[static_cast<size_t>(Texture::Count)];
        uint64_t TextureSizes[static_cast<size_t>(Texture::Count)];
        uint64_t FenceHandle;
    };
#pragma pack(pop)

    static_assert(sizeof(wchar_t) == 2);
    static_assert(offsetof(Shared, WaitValue) % 8 == 0);
    static_assert(offsetof(Shared, TextureHandles) % 8 == 0);
    static_assert(sizeof(Shared) == 2872, "The layout must be identical in the x86 and x64 builds");

    inline const wchar_t* MappingSuffix = L".Mapping";
    inline const wchar_t* RequestSuffix = L".Request";
    inline const wchar_t* ResponseSuffix = L".Response";
}
