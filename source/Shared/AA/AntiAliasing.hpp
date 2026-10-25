#pragma once

// Temporal anti-aliasing for the Direct3D 9 games of the fixes (x86, also D3D8 games through d3d8to9): the fixes' own
// TAA, and DLAA (NVIDIA DLSS at the render size) and AMD FSR native anti-aliasing, which FusionFixHub.exe (x64, in the
// game's folder) runs through its upscaler bridge. Native resolution only: the output has the render size.
//
// - TAA works everywhere, on the system's Direct3D 9 too: a sub-pixel Halton(2, 3) jitter of the main camera's
//   projection, camera motion reprojected from the scene's depth, and a resolve with the history of the frames before.
// - DLAA and FSR need the game on DXVK (Graphics API Vulkan, graphics_api.h), FusionFixHub.exe and the vendor
//   runtimes in the game's folder, the folder of its exe (the hub downloads them). The D3D9 runtime of Windows can't share
//   textures with the hub's D3D12 device; DXVK's interop can: the shared textures and the hub's fence are imported into
//   DXVK's Vulkan device and the copies run on DXVK's queue.
//
// What a game provides (everything else is here):
// - Configure() once, with the names for the bridge.
// - InstallGraphicsApi() before the game creates its Direct3D object, with the Graphics API setting.
// - The main camera once per frame: OnCamera() with the view and the unjittered projection, which returns the jitter;
//   the game moves its projection by it (JitterProjection) before the scene is drawn. Only the main scene's view:
//   anything jittered that isn't resolved afterwards shows the jitter.
// - A depth texture of the scene in standard [0, 1] depth (0 near), e.g. the game's depth-stencil swapped for an INTZ
//   texture (ReplaceDepthStencil).
// - Render() where the scene is complete and the HUD isn't drawn yet: it reads the scene's color and writes the
//   anti-aliased scene. Its result is LDR or HDR as the game's scene there (LowDynamicRange in Configure).
// - OnLostDevice() before the device is reset, Shutdown() when the game ends.
//
// Motion vectors are the camera's only: what moves by itself (cars, people) has the motion of the world behind it, so
// the variance clip of the resolve rejects its history. Games with velocity of their own can pass it later.

#include <windows.h>
#include <d3d9.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <mutex>
#include <string>
#include "dxvk_interop.hpp"
#include "upscaler_bridge_client.h"
#include "graphics_api.h"

namespace WFPAA
{
    enum class Mode
    {
        Off, TAA, DLAA, FSR
    };

    enum class Backend : uint32_t
    {
        DLSS = static_cast<uint32_t>(UpscalerBridge::Backend::DLSS),
        FSR = static_cast<uint32_t>(UpscalerBridge::Backend::FSR),
    };

    // ---------------------------------------------------------------------------------------------------------------
    // Math

    struct Matrix
    {
        double m[4][4]{};

        static Matrix Identity()
        {
            Matrix r;
            for (int i = 0; i < 4; ++i)
                r.m[i][i] = 1.0;
            return r;
        }

        // Sixteen floats, row by row
        static Matrix From(const float* f)
        {
            Matrix r;
            for (int i = 0; i < 4; ++i)
                for (int j = 0; j < 4; ++j)
                    r.m[i][j] = f[i * 4 + j];
            return r;
        }

        void To(float* f) const
        {
            for (int i = 0; i < 4; ++i)
                for (int j = 0; j < 4; ++j)
                    f[i * 4 + j] = static_cast<float>(m[i][j]);
        }

        Matrix operator*(const Matrix& b) const
        {
            Matrix r;
            for (int i = 0; i < 4; ++i)
                for (int j = 0; j < 4; ++j)
                    r.m[i][j] = m[i][0] * b.m[0][j] + m[i][1] * b.m[1][j] + m[i][2] * b.m[2][j] + m[i][3] * b.m[3][j];
            return r;
        }

        Matrix Inverse() const
        {
            // Gauss-Jordan with partial pivoting
            Matrix a = *this;
            Matrix r = Identity();
            for (int c = 0; c < 4; ++c)
            {
                int pivot = c;
                for (int i = c + 1; i < 4; ++i)
                    if (std::abs(a.m[i][c]) > std::abs(a.m[pivot][c]))
                        pivot = i;
                if (std::abs(a.m[pivot][c]) < 1e-12)
                    return Identity();
                std::swap(a.m[c], a.m[pivot]);
                std::swap(r.m[c], r.m[pivot]);
                double inv = 1.0 / a.m[c][c];
                for (int j = 0; j < 4; ++j)
                {
                    a.m[c][j] *= inv;
                    r.m[c][j] *= inv;
                }
                for (int i = 0; i < 4; ++i)
                {
                    if (i == c)
                        continue;
                    double f = a.m[i][c];
                    for (int j = 0; j < 4; ++j)
                    {
                        a.m[i][j] -= f * a.m[c][j];
                        r.m[i][j] -= f * r.m[c][j];
                    }
                }
            }
            return r;
        }
    };

    inline float Halton(uint32_t index, uint32_t base)
    {
        float f = 1.0f;
        float r = 0.0f;
        while (index > 0)
        {
            f /= static_cast<float>(base);
            r += f * static_cast<float>(index % base);
            index /= base;
        }
        return r;
    }

    // How far the rendered content moves this frame, in pixels of the render size: x right, y down. DLSS and FSR take
    // the same value (checked in GTA IV: with the opposite sign the noise of a still image doubles).
    struct Jitter
    {
        float X = 0.0f;
        float Y = 0.0f;
    };

    // Moves a Direct3D perspective projection (row vectors, clip = v * P, w = P[2][3] * z: 1 left-handed, -1
    // right-handed) by a jitter. P[2][0] and P[2][1] add P[2][3] * z to x and y, so NDC moves by exactly the added value.
    inline void JitterProjection(float* P, const Jitter& jitter, float width, float height)
    {
        P[8] += 2.0f * jitter.X / width * P[11];
        P[9] -= 2.0f * jitter.Y / height * P[11];
    }

    struct Camera
    {
        Matrix View;                // world -> view space
        Matrix Projection;          // as the game built it, without jitter; standard [0, 1] depth
        uint32_t Width = 0;         // render size
        uint32_t Height = 0;
    };

    struct Options
    {
        std::wstring BridgeName;                // prefix of the bridge's shared objects, e.g. L"Local\\NFSCarbon.WidescreenFix.Upscaler"
        std::string ClientName;                 // for the hub's log
        bool LowDynamicRange = true;            // the scene at Render() is in [0, 1]
        float CameraCutDistance = 25.0f;        // a camera that moved farther in one frame starts a new history (world units)
        float InfiniteFarScale = 100000.0f;     // a projection without a far plane: far = near * this, for the depth math
        float CurrentWeightMin = 0.08f;         // share of the current frame in the resolve, still
        float CurrentWeightMax = 0.2f;          // and moving fast
        float CurrentWeightPerPixel = 0.1f;     // from min to max per pixel of motion
        float VarianceClipGamma = 1.25f;
        float StaticHistoryTrust = 0.5f;        // share of the unclipped history kept where nothing moves (thin detail flickers less)
        float LumaWeight = 1.0f;
        uint32_t JitterPhases = 8;              // TAA; DLAA takes at least 32, FSR 16
        uint32_t DLSSPreset = 0;                // NVSDK_NGX_DLSS_Hint_Render_Preset, 0 is the default
    };

    // ---------------------------------------------------------------------------------------------------------------
    // Graphics API

    using GraphicsApi = FusionFixGraphicsApi::Api;

    // Before the game creates its Direct3D object: points its Direct3DCreate9 imports at DXVK (vulkan.dll in the game
    // folder, a build of the hub's upscalers.json) or D3D9on12 (hidden, allowD3D9On12). Returns the API it will run on.
    inline GraphicsApi InstallGraphicsApi(int setting, bool allowD3D9On12)
    {
        return FusionFixGraphicsApi::Install(static_cast<GraphicsApi>(std::clamp(setting, 0, 2)), allowD3D9On12);
    }

    // Whether the Vulkan choice can be offered: vulkan.dll is a DXVK the hub installed
    inline bool VulkanAvailable()
    {
        static int known = -1;
        if (known < 0)
            known = FusionFixGraphicsApi::VulkanAvailable() ? 1 : 0;
        return known == 1;
    }

    inline GraphicsApi RunningApi()
    {
        return FusionFixGraphicsApi::Running();
    }

    namespace detail
    {
        inline Options& Config()
        {
            static Options options;
            return options;
        }

        // Failures, to the debugger only
        inline void Log(const char* format, ...)
        {
            char text[512];
            int prefix = snprintf(text, sizeof(text), "AntiAliasing: ");
            va_list args;
            va_start(args, format);
            vsnprintf(text + prefix, sizeof(text) - prefix - 1, format, args);
            va_end(args);
            strcat_s(text, "\n");
            OutputDebugStringA(text);
        }

        // Whether the files DLAA (DLSS) or FSR needs are in the game folder: FusionFixHub.exe and the vendor runtime.
        // Checked once; before the hub runs, this is what the menus offer.
        inline bool BridgeFilesPresent(UpscalerBridge::Backend backend)
        {
            static int known[2] = { -1, -1 };
            auto& k = known[backend == UpscalerBridge::Backend::DLSS ? 0 : 1];
            if (k < 0)
            {
                auto root = UpscalerBridge::GameRoot();
                std::error_code ec;
                k = std::filesystem::is_regular_file(UpscalerBridge::HubPath(root), ec) && UpscalerBridge::RuntimePresent(root, backend) ? 1 : 0;
            }
            return k == 1;
        }

        template <typename T>
        void SafeRelease(T*& p)
        {
            if (p)
            {
                p->Release();
                p = nullptr;
            }
        }

        inline HMODULE ThisModule()
        {
            HMODULE module = nullptr;
            GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                reinterpret_cast<LPCWSTR>(&ThisModule), &module);
            return module;
        }

        inline uint32_t AdapterVendor(IDirect3DDevice9* device)
        {
            uint32_t vendor = 0;
            IDirect3D9* d3d = nullptr;
            if (SUCCEEDED(device->GetDirect3D(&d3d)) && d3d)
            {
                D3DDEVICE_CREATION_PARAMETERS parameters{};
                D3DADAPTER_IDENTIFIER9 identifier{};
                if (SUCCEEDED(device->GetCreationParameters(&parameters)) && SUCCEEDED(d3d->GetAdapterIdentifier(parameters.AdapterOrdinal, 0, &identifier)))
                    vendor = identifier.VendorId;
                d3d->Release();
            }
            return vendor;
        }

        // -----------------------------------------------------------------------------------------------------------
        // DXVK side of the bridge: the hub's shared textures and fence imported into DXVK's device

        namespace Protocol = UpscalerBridge;

        constexpr size_t TextureCount = static_cast<size_t>(Protocol::Texture::Count);
        constexpr size_t InputCount = Protocol::InputCount;
        constexpr size_t OutputIndex = static_cast<size_t>(Protocol::Texture::Output);

        struct Vulkan
        {
            PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = nullptr;
            PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr = nullptr;
            PFN_vkGetPhysicalDeviceProperties2 vkGetPhysicalDeviceProperties2 = nullptr;
            PFN_vkGetPhysicalDeviceMemoryProperties vkGetPhysicalDeviceMemoryProperties = nullptr;
            PFN_vkCreateImage vkCreateImage = nullptr;
            PFN_vkDestroyImage vkDestroyImage = nullptr;
            PFN_vkGetImageMemoryRequirements vkGetImageMemoryRequirements = nullptr;
            PFN_vkAllocateMemory vkAllocateMemory = nullptr;
            PFN_vkFreeMemory vkFreeMemory = nullptr;
            PFN_vkBindImageMemory vkBindImageMemory = nullptr;
            PFN_vkGetMemoryWin32HandlePropertiesKHR vkGetMemoryWin32HandlePropertiesKHR = nullptr;
            PFN_vkCreateSemaphore vkCreateSemaphore = nullptr;
            PFN_vkDestroySemaphore vkDestroySemaphore = nullptr;
            PFN_vkImportSemaphoreWin32HandleKHR vkImportSemaphoreWin32HandleKHR = nullptr;
            PFN_vkCreateCommandPool vkCreateCommandPool = nullptr;
            PFN_vkAllocateCommandBuffers vkAllocateCommandBuffers = nullptr;
            PFN_vkBeginCommandBuffer vkBeginCommandBuffer = nullptr;
            PFN_vkEndCommandBuffer vkEndCommandBuffer = nullptr;
            PFN_vkResetCommandBuffer vkResetCommandBuffer = nullptr;
            PFN_vkCmdPipelineBarrier vkCmdPipelineBarrier = nullptr;
            PFN_vkCmdCopyImage vkCmdCopyImage = nullptr;
            PFN_vkQueueSubmit vkQueueSubmit = nullptr;
            PFN_vkCreateFence vkCreateFence = nullptr;
            PFN_vkWaitForFences vkWaitForFences = nullptr;
            PFN_vkResetFences vkResetFences = nullptr;

            bool Load(VkInstance instance, VkDevice device)
            {
                HMODULE loader = GetModuleHandleW(L"vulkan-1.dll");
                if (!loader)
                    loader = GetModuleHandleW(L"winevulkan.dll");
                if (!loader)
                    return false;

                vkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(GetProcAddress(loader, "vkGetInstanceProcAddr"));
                if (!vkGetInstanceProcAddr)
                    return false;

#define WFPAA_LOAD_INSTANCE(name) name = reinterpret_cast<PFN_##name>(vkGetInstanceProcAddr(instance, #name))
#define WFPAA_LOAD_DEVICE(name) name = reinterpret_cast<PFN_##name>(vkGetDeviceProcAddr(device, #name))
                WFPAA_LOAD_INSTANCE(vkGetDeviceProcAddr);
                WFPAA_LOAD_INSTANCE(vkGetPhysicalDeviceProperties2);
                WFPAA_LOAD_INSTANCE(vkGetPhysicalDeviceMemoryProperties);
                if (!vkGetDeviceProcAddr || !vkGetPhysicalDeviceProperties2 || !vkGetPhysicalDeviceMemoryProperties)
                    return false;

                WFPAA_LOAD_DEVICE(vkCreateImage);
                WFPAA_LOAD_DEVICE(vkDestroyImage);
                WFPAA_LOAD_DEVICE(vkGetImageMemoryRequirements);
                WFPAA_LOAD_DEVICE(vkAllocateMemory);
                WFPAA_LOAD_DEVICE(vkFreeMemory);
                WFPAA_LOAD_DEVICE(vkBindImageMemory);
                WFPAA_LOAD_DEVICE(vkGetMemoryWin32HandlePropertiesKHR);
                WFPAA_LOAD_DEVICE(vkCreateSemaphore);
                WFPAA_LOAD_DEVICE(vkDestroySemaphore);
                WFPAA_LOAD_DEVICE(vkImportSemaphoreWin32HandleKHR);
                WFPAA_LOAD_DEVICE(vkCreateCommandPool);
                WFPAA_LOAD_DEVICE(vkAllocateCommandBuffers);
                WFPAA_LOAD_DEVICE(vkBeginCommandBuffer);
                WFPAA_LOAD_DEVICE(vkEndCommandBuffer);
                WFPAA_LOAD_DEVICE(vkResetCommandBuffer);
                WFPAA_LOAD_DEVICE(vkCmdPipelineBarrier);
                WFPAA_LOAD_DEVICE(vkCmdCopyImage);
                WFPAA_LOAD_DEVICE(vkQueueSubmit);
                WFPAA_LOAD_DEVICE(vkCreateFence);
                WFPAA_LOAD_DEVICE(vkWaitForFences);
                WFPAA_LOAD_DEVICE(vkResetFences);
#undef WFPAA_LOAD_INSTANCE
#undef WFPAA_LOAD_DEVICE

                // The external memory and semaphore functions only exist when DXVK enabled the extensions
                return vkCreateImage && vkDestroyImage && vkGetImageMemoryRequirements && vkAllocateMemory && vkFreeMemory && vkBindImageMemory &&
                    vkGetMemoryWin32HandlePropertiesKHR && vkCreateSemaphore && vkDestroySemaphore && vkImportSemaphoreWin32HandleKHR &&
                    vkCreateCommandPool && vkAllocateCommandBuffers && vkBeginCommandBuffer && vkEndCommandBuffer && vkResetCommandBuffer &&
                    vkCmdPipelineBarrier && vkCmdCopyImage && vkQueueSubmit && vkCreateFence && vkWaitForFences && vkResetFences;
            }
        };

        struct GameImage
        {
            VkImage image = VK_NULL_HANDLE;
            VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
            VkFormat format = VK_FORMAT_UNDEFINED;
            VkExtent3D extent{};
        };

        inline bool GetGameImage(IDirect3DTexture9* texture, GameImage& out)
        {
            if (!texture)
                return false;

            ID3D9VkInteropTexture* interop = nullptr;
            if (FAILED(texture->QueryInterface(__uuidof(ID3D9VkInteropTexture), reinterpret_cast<void**>(&interop))) || !interop)
                return false;

            VkImageCreateInfo info{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
            auto hr = interop->GetVulkanImageInfo(&out.image, &out.layout, &info);
            interop->Release();
            out.format = info.format;
            out.extent = info.extent;
            return SUCCEEDED(hr) && out.image != VK_NULL_HANDLE;
        }

        inline VkImageMemoryBarrier ImageBarrier(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, VkFlags srcAccess, VkFlags dstAccess,
            uint32_t srcQueueFamily = VK_QUEUE_FAMILY_IGNORED, uint32_t dstQueueFamily = VK_QUEUE_FAMILY_IGNORED)
        {
            VkImageMemoryBarrier barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
            barrier.srcAccessMask = srcAccess;
            barrier.dstAccessMask = dstAccess;
            barrier.oldLayout = oldLayout;
            barrier.newLayout = newLayout;
            barrier.srcQueueFamilyIndex = srcQueueFamily;
            barrier.dstQueueFamilyIndex = dstQueueFamily;
            barrier.image = image;
            barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
            return barrier;
        }

        inline VkImageCopy FullCopy(uint32_t width, uint32_t height)
        {
            VkImageCopy copy{};
            copy.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
            copy.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
            copy.extent = { width, height, 1 };
            return copy;
        }

        class DXVKBridge
        {
        public:
            LUID luid{};
            uint32_t vendorId = 0;
            uint32_t width = 0;
            uint32_t height = 0;

            ID3D9VkInteropDevice* interop = nullptr;
            VkInstance instance = nullptr;
            VkPhysicalDevice physicalDevice = nullptr;
            VkDevice device = nullptr;
            VkQueue queue = nullptr;
            uint32_t queueFamily = 0;
            Vulkan vk;

            VkCommandPool pool = VK_NULL_HANDLE;
            struct Slot
            {
                VkCommandBuffer inputs = nullptr;
                VkCommandBuffer output = nullptr;
                VkFence fence = VK_NULL_HANDLE;
                bool submitted = false;
            };
            std::array<Slot, 4> slots{};
            uint32_t slot = 0;

            struct SharedImage
            {
                VkImage image = VK_NULL_HANDLE;
                VkDeviceMemory memory = VK_NULL_HANDLE;
            };
            std::array<SharedImage, TextureCount> images{};
            VkSemaphore semaphore = VK_NULL_HANDLE;

            // Vulkan formats of the shared textures and of the game textures copied from and to them
            static constexpr VkFormat Formats[TextureCount] =
            {
                VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_R32_SFLOAT, VK_FORMAT_R16G16_SFLOAT, VK_FORMAT_R16_SFLOAT, VK_FORMAT_R16G16B16A16_SFLOAT
            };

            bool Init(IDirect3DDevice9* d3dDevice)
            {
                if (FAILED(d3dDevice->QueryInterface(__uuidof(ID3D9VkInteropDevice), reinterpret_cast<void**>(&interop))) || !interop)
                    return false;

                interop->GetVulkanHandles(&instance, &physicalDevice, &device);
                uint32_t queueIndex = 0;
                interop->GetSubmissionQueue(&queue, &queueIndex, &queueFamily);
                if (!instance || !physicalDevice || !device || !queue || !vk.Load(instance, device))
                    return false;

                VkPhysicalDeviceIDProperties id{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES };
                VkPhysicalDeviceProperties2 properties{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
                properties.pNext = &id;
                vk.vkGetPhysicalDeviceProperties2(physicalDevice, &properties);
                if (!id.deviceLUIDValid)
                    return false;
                std::memcpy(&luid, id.deviceLUID, sizeof(luid));
                std::memcpy(&vendorId, properties.properties + 8, sizeof(vendorId));

                VkCommandPoolCreateInfo poolInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
                poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
                poolInfo.queueFamilyIndex = queueFamily;
                if (vk.vkCreateCommandPool(device, &poolInfo, nullptr, &pool) != VK_SUCCESS)
                    return false;

                for (auto& s : slots)
                {
                    VkCommandBuffer buffers[2]{};
                    VkCommandBufferAllocateInfo allocate{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
                    allocate.commandPool = pool;
                    allocate.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
                    allocate.commandBufferCount = 2;
                    if (vk.vkAllocateCommandBuffers(device, &allocate, buffers) != VK_SUCCESS)
                        return false;
                    s.inputs = buffers[0];
                    s.output = buffers[1];

                    VkFenceCreateInfo fenceInfo{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
                    if (vk.vkCreateFence(device, &fenceInfo, nullptr, &s.fence) != VK_SUCCESS)
                        return false;
                }
                return true;
            }

            void ReleaseImports()
            {
                if (!device)
                    return;
                // Nothing may still be using the images: wait for the last submissions
                for (auto& s : slots)
                {
                    if (s.submitted)
                    {
                        vk.vkWaitForFences(device, 1, &s.fence, VK_TRUE, 2000000000ull);
                        vk.vkResetFences(device, 1, &s.fence);
                        s.submitted = false;
                    }
                }
                for (auto& image : images)
                {
                    if (image.image)
                        vk.vkDestroyImage(device, image.image, nullptr);
                    if (image.memory)
                        vk.vkFreeMemory(device, image.memory, nullptr);
                    image = {};
                }
                if (semaphore)
                    vk.vkDestroySemaphore(device, semaphore, nullptr);
                semaphore = VK_NULL_HANDLE;
                width = height = 0;
            }

            bool ImportImage(SharedImage& target, HANDLE handle, VkFormat format, uint32_t w, uint32_t h)
            {
                VkExternalMemoryImageCreateInfo external{ VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO };
                external.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_RESOURCE_BIT;

                VkImageCreateInfo info{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
                info.pNext = &external;
                info.imageType = VK_IMAGE_TYPE_2D;
                info.format = format;
                info.extent = { w, h, 1 };
                info.mipLevels = 1;
                info.arrayLayers = 1;
                info.samples = VK_SAMPLE_COUNT_1_BIT;
                info.tiling = VK_IMAGE_TILING_OPTIMAL;
                info.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
                info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                if (vk.vkCreateImage(device, &info, nullptr, &target.image) != VK_SUCCESS)
                    return false;

                VkMemoryRequirements requirements{};
                vk.vkGetImageMemoryRequirements(device, target.image, &requirements);

                VkMemoryWin32HandlePropertiesKHR handleProperties{ VK_STRUCTURE_TYPE_MEMORY_WIN32_HANDLE_PROPERTIES_KHR };
                if (vk.vkGetMemoryWin32HandlePropertiesKHR(device, VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_RESOURCE_BIT, handle, &handleProperties) != VK_SUCCESS)
                    return false;

                VkPhysicalDeviceMemoryProperties memoryProperties{};
                vk.vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);
                uint32_t typeIndex = UINT32_MAX;
                auto types = requirements.memoryTypeBits & handleProperties.memoryTypeBits;
                for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i)
                {
                    if ((types & (1u << i)) && (memoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
                    {
                        typeIndex = i;
                        break;
                    }
                }
                if (typeIndex == UINT32_MAX)
                    return false;

                VkMemoryDedicatedAllocateInfo dedicated{ VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO };
                dedicated.image = target.image;

                VkImportMemoryWin32HandleInfoKHR import{ VK_STRUCTURE_TYPE_IMPORT_MEMORY_WIN32_HANDLE_INFO_KHR };
                import.pNext = &dedicated;
                import.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_RESOURCE_BIT;
                import.handle = handle;

                VkMemoryAllocateInfo allocate{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
                allocate.pNext = &import;
                allocate.allocationSize = requirements.size;
                allocate.memoryTypeIndex = typeIndex;
                if (vk.vkAllocateMemory(device, &allocate, nullptr, &target.memory) != VK_SUCCESS)
                    return false;

                return vk.vkBindImageMemory(device, target.image, target.memory, 0) == VK_SUCCESS;
            }

            // Opens the shared textures and the fence of a configuration; the client closes the handles
            bool Import(Protocol::Shared& shared, uint32_t w, uint32_t h)
            {
                ReleaseImports();

                bool ok = true;
                for (size_t i = 0; i < images.size(); ++i)
                {
                    auto handle = reinterpret_cast<HANDLE>(shared.TextureHandles[i]);
                    ok = ok && handle && ImportImage(images[i], handle, Formats[i], w, h);
                }

                auto fenceHandle = reinterpret_cast<HANDLE>(shared.FenceHandle);
                if (ok && fenceHandle)
                {
                    VkSemaphoreTypeCreateInfo type{ VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO };
                    type.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
                    VkSemaphoreCreateInfo info{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
                    info.pNext = &type;
                    ok = vk.vkCreateSemaphore(device, &info, nullptr, &semaphore) == VK_SUCCESS;

                    if (ok)
                    {
                        VkImportSemaphoreWin32HandleInfoKHR import{ VK_STRUCTURE_TYPE_IMPORT_SEMAPHORE_WIN32_HANDLE_INFO_KHR };
                        import.semaphore = semaphore;
                        import.handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_D3D12_FENCE_BIT;
                        import.handle = fenceHandle;
                        ok = vk.vkImportSemaphoreWin32HandleKHR(device, &import) == VK_SUCCESS;
                    }
                }
                else
                {
                    ok = false;
                }

                if (!ok)
                {
                    ReleaseImports();
                    return false;
                }

                width = w;
                height = h;
                return true;
            }

            Slot& NextSlot()
            {
                slot = (slot + 1) % slots.size();
                auto& s = slots[slot];
                if (s.submitted)
                {
                    vk.vkWaitForFences(device, 1, &s.fence, VK_TRUE, 2000000000ull);
                    vk.vkResetFences(device, 1, &s.fence);
                    s.submitted = false;
                }
                return s;
            }

            bool Submit(VkCommandBuffer buffer, VkSemaphore waitSemaphore, uint64_t waitValue, uint64_t signalValue, VkFence fence)
            {
                VkTimelineSemaphoreSubmitInfo timeline{ VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO };
                VkSubmitInfo submit{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
                submit.pNext = &timeline;
                submit.commandBufferCount = 1;
                submit.pCommandBuffers = &buffer;

                VkFlags waitStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
                if (waitValue)
                {
                    submit.waitSemaphoreCount = 1;
                    submit.pWaitSemaphores = &waitSemaphore;
                    submit.pWaitDstStageMask = &waitStage;
                    timeline.waitSemaphoreValueCount = 1;
                    timeline.pWaitSemaphoreValues = &waitValue;
                }
                if (signalValue)
                {
                    submit.signalSemaphoreCount = 1;
                    submit.pSignalSemaphores = &semaphore;
                    timeline.signalSemaphoreValueCount = 1;
                    timeline.pSignalSemaphoreValues = &signalValue;
                }

                interop->LockSubmissionQueue();
                auto result = vk.vkQueueSubmit(queue, 1, &submit, fence);
                interop->ReleaseSubmissionQueue();
                return result == VK_SUCCESS;
            }

            // Game textures -> shared textures, then the fence reaches signalValue. Null inputs are skipped.
            bool SubmitInputs(IDirect3DTexture9* const (&inputs)[InputCount], uint64_t signalValue)
            {
                GameImage sources[InputCount];
                for (size_t i = 0; i < InputCount; ++i)
                {
                    if (!inputs[i])
                        continue;
                    if (!GetGameImage(inputs[i], sources[i]) || sources[i].format != Formats[i] || sources[i].extent.width != width || sources[i].extent.height != height)
                        return false;
                }

                auto& s = NextSlot();
                auto cmd = s.inputs;
                vk.vkResetCommandBuffer(cmd, 0);
                VkCommandBufferBeginInfo begin{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
                begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
                vk.vkBeginCommandBuffer(cmd, &begin);

                VkImageMemoryBarrier before[InputCount * 2];
                VkImageMemoryBarrier after[InputCount * 2];
                uint32_t barriers = 0;
                for (size_t i = 0; i < InputCount; ++i)
                {
                    if (!inputs[i])
                        continue;
                    auto shared = images[i].image;
                    before[barriers] = ImageBarrier(sources[i].image, sources[i].layout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT);
                    after[barriers++] = ImageBarrier(sources[i].image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, sources[i].layout, VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT);
                    before[barriers] = ImageBarrier(shared, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, VK_ACCESS_TRANSFER_WRITE_BIT);
                    // Handed over to D3D12, where it is in the common state
                    after[barriers++] = ImageBarrier(shared, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_TRANSFER_WRITE_BIT, 0, queueFamily, VK_QUEUE_FAMILY_EXTERNAL);
                }

                vk.vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, barriers, before);
                for (size_t i = 0; i < InputCount; ++i)
                {
                    if (!inputs[i])
                        continue;
                    auto copy = FullCopy(width, height);
                    vk.vkCmdCopyImage(cmd, sources[i].image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, images[i].image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);
                }
                vk.vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, barriers, after);
                vk.vkEndCommandBuffer(cmd);

                // Everything the game rendered so far must reach the queue first
                interop->FlushRenderingCommands();
                return Submit(cmd, VK_NULL_HANDLE, 0, signalValue, VK_NULL_HANDLE);
            }

            // Once the fence reaches waitValue: shared output -> game texture
            bool SubmitOutput(IDirect3DTexture9* target, uint64_t waitValue)
            {
                GameImage destination;
                if (!GetGameImage(target, destination) || destination.format != Formats[OutputIndex] ||
                    destination.extent.width != width || destination.extent.height != height)
                    return false;

                auto& s = slots[slot];
                auto output = images[OutputIndex].image;
                auto cmd = s.output;
                vk.vkResetCommandBuffer(cmd, 0);
                VkCommandBufferBeginInfo begin{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
                begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
                vk.vkBeginCommandBuffer(cmd, &begin);

                VkImageMemoryBarrier before[2] =
                {
                    ImageBarrier(output, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 0, VK_ACCESS_TRANSFER_READ_BIT, VK_QUEUE_FAMILY_EXTERNAL, queueFamily),
                    ImageBarrier(destination.image, destination.layout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT),
                };
                VkImageMemoryBarrier after[2] =
                {
                    ImageBarrier(output, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_TRANSFER_READ_BIT, 0, queueFamily, VK_QUEUE_FAMILY_EXTERNAL),
                    ImageBarrier(destination.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, destination.layout, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT),
                };

                vk.vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 2, before);
                auto copy = FullCopy(width, height);
                vk.vkCmdCopyImage(cmd, output, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, destination.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);
                vk.vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 2, after);
                vk.vkEndCommandBuffer(cmd);

                if (!Submit(cmd, semaphore, waitValue, 0, s.fence))
                    return false;
                s.submitted = true;
                return true;
            }
        };

        // -----------------------------------------------------------------------------------------------------------
        // The hub: started once the device is known, then Configure and Evaluate per frame

        struct UpscalerFrame
        {
            IDirect3DTexture9* Color = nullptr;     // A16B16G16R16F
            IDirect3DTexture9* Depth = nullptr;     // R32F, standard [0, 1] depth
            IDirect3DTexture9* Motion = nullptr;    // G16R16F, previous - current in texture coordinates
            IDirect3DTexture9* Output = nullptr;    // A16B16G16R16F
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

        class Upscaler
        {
        public:
            enum class State
            {
                Idle, Starting, Ready, Failed
            };

            std::atomic<State> state = State::Idle;
            DXVKBridge bridge;
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
                hub.Stop(true);
            }

            // Every frame: starts the hub once the device is known and DLAA or FSR is chosen (start)
            void Update(IDirect3DDevice9* device, bool start)
            {
                if (state == State::Idle)
                {
                    if (!device || !start)
                        return;
                    state = State::Failed;
                    ++generation;

                    // DXVK only: the D3D9 runtime of Windows can't share its textures with D3D12
                    if (RunningApi() != GraphicsApi::Vulkan)
                    {
                        Log("Bridge: the game doesn't run on DXVK, DLAA and FSR are off");
                        return;
                    }
                    if (!bridge.Init(device))
                    {
                        Log("Bridge: DXVK's interop interface isn't usable");
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
            bool Evaluate(Backend backend, const UpscalerFrame& frame)
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
                    shared.Flags = Config().LowDynamicRange ? Protocol::ConfigureFlags::LowDynamicRange : 0u;
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
                        Log("Bridge: the shared textures couldn't be imported into DXVK");
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

                IDirect3DTexture9* const inputs[InputCount] = { frame.Color, frame.Depth, frame.Motion, nullptr };
                if (!bridge.SubmitInputs(inputs, inputValue))
                {
                    // Nothing was submitted, the hub was not asked for this frame. Retried when the mode or size changes.
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

        inline Upscaler& TheUpscaler()
        {
            static Upscaler upscaler;
            return upscaler;
        }
    }

    // ---------------------------------------------------------------------------------------------------------------
    // Temporal anti-aliasing

    class TemporalAA
    {
        struct FrameCamera
        {
            Matrix View;
            Matrix Projection;              // without jitter
            Matrix ViewProjection;          // without jitter
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

        static inline Mode requested = Mode::Off;
        static inline float sharpness = 0.0f;
        static inline uint32_t frame = 0;               // advanced by OnCamera, once per frame
        static inline uint32_t resolveFrame = 0;        // frame of the last Render that resolved
        static inline uint32_t historyFrame = 0;        // frame the TAA history was written in
        static inline uint32_t upscalerFrame = 0;       // frame of the last DLAA or FSR frame
        static inline uint32_t historyIndex = 0;
        static inline uint32_t depthIndex = 0;          // depthTex of this frame
        static inline uint32_t depthFrame = 0;          // frame depthTex[depthIndex] was written in
        static inline FrameCamera current;
        static inline FrameCamera previous;
        static inline LARGE_INTEGER lastTime{};

        // Resources (default pool, released before a reset)
        static inline IDirect3DPixelShader9* cameraMotionPS = nullptr;
        static inline IDirect3DPixelShader9* resolvePS = nullptr;
        static inline IDirect3DPixelShader9* outputPS = nullptr;
        static inline IDirect3DPixelShader9* copyPS = nullptr;
        static inline IDirect3DPixelShader9* depthPS = nullptr;
        static inline IDirect3DStateBlock9* stateBlock = nullptr;
        static inline IDirect3DTexture9* sceneTex = nullptr;        // copy of the scene, the game's format
        static inline IDirect3DTexture9* motionTex = nullptr;       // G16R16F
        static inline IDirect3DTexture9* historyTex[2]{};           // A16B16G16R16F
        static inline IDirect3DTexture9* colorTex = nullptr;        // A16B16G16R16F, the scene for DLAA and FSR
        static inline IDirect3DTexture9* depthTex[2]{};             // R32F copies of the depth, this frame's and the last one's
        static inline IDirect3DTexture9* upscaledTex = nullptr;     // A16B16G16R16F, written by DLAA or FSR
        static inline IDirect3DTexture9* intzTex = nullptr;         // the game's depth-stencil (ReplaceDepthStencil)
        static inline uint32_t width = 0;
        static inline uint32_t height = 0;
        static inline D3DFORMAT sceneFormat = D3DFMT_UNKNOWN;
        static inline bool failed = false;                          // resources or shaders couldn't be made: nothing until a reset

        static bool LoadShader(IDirect3DDevice9* device, const wchar_t* name, IDirect3DPixelShader9*& shader)
        {
            if (shader)
                return true;
            auto module = detail::ThisModule();
            auto resource = FindResourceW(module, name, MAKEINTRESOURCEW(10));     // RT_RCDATA
            auto data = resource ? LoadResource(module, resource) : nullptr;
            auto code = data ? LockResource(data) : nullptr;
            return code && SUCCEEDED(device->CreatePixelShader(static_cast<const DWORD*>(code), &shader)) && shader;
        }

        static bool CreateTexture(IDirect3DDevice9* device, D3DFORMAT format, IDirect3DTexture9*& texture)
        {
            if (texture)
                return true;
            return SUCCEEDED(device->CreateTexture(width, height, 1, D3DUSAGE_RENDERTARGET, format, D3DPOOL_DEFAULT, &texture, nullptr)) && texture;
        }

        static void ReleaseTargets()
        {
            detail::SafeRelease(sceneTex);
            detail::SafeRelease(motionTex);
            detail::SafeRelease(historyTex[0]);
            detail::SafeRelease(historyTex[1]);
            detail::SafeRelease(colorTex);
            detail::SafeRelease(depthTex[0]);
            detail::SafeRelease(depthTex[1]);
            detail::SafeRelease(upscaledTex);
            width = height = 0;
            depthFrame = 0;
            historyFrame = 0;
            upscalerFrame = 0;
        }

        static bool EnsureResources(IDirect3DDevice9* device, uint32_t w, uint32_t h, D3DFORMAT format, bool bridge)
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

            bool ok = LoadShader(device, L"WFPAA_CAMERAMOTION", cameraMotionPS) && LoadShader(device, L"WFPAA_RESOLVE", resolvePS) &&
                LoadShader(device, L"WFPAA_OUTPUT", outputPS) && LoadShader(device, L"WFPAA_COPY", copyPS) && LoadShader(device, L"WFPAA_DEPTH", depthPS);
            ok = ok && (stateBlock || SUCCEEDED(device->CreateStateBlock(D3DSBT_ALL, &stateBlock)));
            ok = ok && CreateTexture(device, format, sceneTex) && CreateTexture(device, D3DFMT_G16R16F, motionTex) &&
                CreateTexture(device, D3DFMT_A16B16G16R16F, historyTex[0]) && CreateTexture(device, D3DFMT_A16B16G16R16F, historyTex[1]) &&
                CreateTexture(device, D3DFMT_R32F, depthTex[0]) && CreateTexture(device, D3DFMT_R32F, depthTex[1]);
            // DLAA and FSR without them fall back to TAA (RenderUpscaler)
            if (ok && bridge && !(CreateTexture(device, D3DFMT_A16B16G16R16F, colorTex) && CreateTexture(device, D3DFMT_A16B16G16R16F, upscaledTex)))
            {
                detail::SafeRelease(colorTex);
                detail::SafeRelease(upscaledTex);
            }
            if (!ok)
            {
                detail::Log("TAA: resources or shaders couldn't be created at %ux%u", w, h);
                failed = true;
                ReleaseTargets();
            }
            return ok;
        }

        struct ScreenVertex
        {
            float x, y, z, rhw;
            float u, v;
        };

        static void DrawFullscreen(IDirect3DDevice9* device, IDirect3DSurface9* target, IDirect3DPixelShader9* shader)
        {
            device->SetRenderTarget(0, target);
            device->SetPixelShader(shader);
            auto w = static_cast<float>(width);
            auto h = static_cast<float>(height);
            ScreenVertex vertices[4] =
            {
                { -0.5f,     -0.5f,     0.0f, 1.0f, 0.0f, 0.0f },
                { -0.5f,     h - 0.5f,  0.0f, 1.0f, 0.0f, 1.0f },
                { w - 0.5f,  -0.5f,     0.0f, 1.0f, 1.0f, 0.0f },
                { w - 0.5f,  h - 0.5f,  0.0f, 1.0f, 1.0f, 1.0f },
            };
            device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices, sizeof(ScreenVertex));
        }

        static void DrawFullscreen(IDirect3DDevice9* device, IDirect3DTexture9* target, IDirect3DPixelShader9* shader)
        {
            IDirect3DSurface9* surface = nullptr;
            if (FAILED(target->GetSurfaceLevel(0, &surface)) || !surface)
                return;
            DrawFullscreen(device, surface, shader);
            surface->Release();
        }

        static void SetSampler(IDirect3DDevice9* device, DWORD stage, IDirect3DBaseTexture9* texture, D3DTEXTUREFILTERTYPE filter)
        {
            device->SetTexture(stage, texture);
            device->SetSamplerState(stage, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
            device->SetSamplerState(stage, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
            device->SetSamplerState(stage, D3DSAMP_MINFILTER, filter);
            device->SetSamplerState(stage, D3DSAMP_MAGFILTER, filter);
            device->SetSamplerState(stage, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
            device->SetSamplerState(stage, D3DSAMP_SRGBTEXTURE, FALSE);
        }

        static void SetFullscreenStates(IDirect3DDevice9* device)
        {
            device->SetRenderTarget(1, nullptr);
            device->SetDepthStencilSurface(nullptr);
            device->SetVertexShader(nullptr);
            device->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1);
            device->SetRenderState(D3DRS_ZENABLE, FALSE);
            device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
            device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
            device->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
            device->SetRenderState(D3DRS_STENCILENABLE, FALSE);
            device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
            device->SetRenderState(D3DRS_COLORWRITEENABLE, 0x0F);
            device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
            device->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);
            device->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
            device->SetRenderState(D3DRS_CLIPPLANEENABLE, 0);
            device->SetRenderState(D3DRS_FOGENABLE, FALSE);
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

        static bool HistoryUsable(uint32_t since)
        {
            return since != 0 && since + 1 == frame && previous.Valid && previous.Frame + 1 == frame && !IsCameraCut();
        }

        static void RenderCameraMotion(IDirect3DDevice9* device, IDirect3DBaseTexture9* depth)
        {
            // This frame's depth as R32F: the next frame checks its motion against it, DLAA and FSR take it
            auto previousDepth = depthIndex;
            bool previousValid = depthFrame != 0 && depthFrame + 1 == frame && previous.Valid && previous.Frame + 1 == frame;
            depthIndex ^= 1u;
            SetSampler(device, 0, depth, D3DTEXF_POINT);
            DrawFullscreen(device, depthTex[depthIndex], depthPS);
            depthFrame = frame;

            // current clip space -> previous clip space, without jitter
            auto& last = previous.Valid && previous.Frame + 1 == frame ? previous : current;
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

            SetSampler(device, 1, depthTex[previousDepth], D3DTEXF_POINT);
            device->SetPixelShaderConstantF(0, constants, 7);
            DrawFullscreen(device, motionTex, cameraMotionPS);
            device->SetTexture(1, nullptr);
        }

        static void RenderOutput(IDirect3DDevice9* device, IDirect3DTexture9* source, IDirect3DSurface9* output)
        {
            float constants[4] = { 1.0f / static_cast<float>(width), 1.0f / static_cast<float>(height), sharpness, 0.0f };
            SetSampler(device, 0, source, D3DTEXF_POINT);
            device->SetPixelShaderConstantF(0, constants, 1);
            DrawFullscreen(device, output, outputPS);
        }

        static void RenderResolve(IDirect3DDevice9* device, IDirect3DBaseTexture9* depth)
        {
            auto& config = detail::Config();
            auto previousIndex = historyIndex;
            auto currentIndex = historyIndex ^ 1u;
            bool historyValid = HistoryUsable(historyFrame);

            auto w = static_cast<float>(width);
            auto h = static_cast<float>(height);
            float constants[4 * 4] =
            {
                1.0f / w, 1.0f / h, w, h,
                current.JitterPixels.X, current.JitterPixels.Y, historyValid ? 1.0f : 0.0f, config.VarianceClipGamma,
                config.CurrentWeightMin, config.CurrentWeightMax, config.CurrentWeightPerPixel, config.LumaWeight,
                config.StaticHistoryTrust, 0.1f, 0.0f, 0.0f,
            };

            SetSampler(device, 0, sceneTex, D3DTEXF_POINT);
            SetSampler(device, 1, historyTex[previousIndex], D3DTEXF_LINEAR);
            SetSampler(device, 2, motionTex, D3DTEXF_POINT);
            SetSampler(device, 3, depth, D3DTEXF_POINT);
            device->SetPixelShaderConstantF(0, constants, 4);
            DrawFullscreen(device, historyTex[currentIndex], resolvePS);
            for (DWORD s = 1; s < 4; ++s)
                device->SetTexture(s, nullptr);

            historyIndex = currentIndex;
            historyFrame = frame;
        }

        static bool RenderUpscaler(IDirect3DDevice9* device, Backend backend)
        {
            if (!colorTex || !upscaledTex)
                return false;

            // The scene in the format of the shared texture (the depth is in depthTex already)
            SetSampler(device, 0, sceneTex, D3DTEXF_POINT);
            DrawFullscreen(device, colorTex, copyPS);
            device->SetTexture(0, nullptr);

            LARGE_INTEGER now, frequency;
            QueryPerformanceCounter(&now);
            QueryPerformanceFrequency(&frequency);
            auto frameTime = lastTime.QuadPart ? static_cast<float>(static_cast<double>(now.QuadPart - lastTime.QuadPart) * 1000.0 / static_cast<double>(frequency.QuadPart)) : 16.6f;
            lastTime = now;

            detail::UpscalerFrame upscalerFrameData;
            upscalerFrameData.Color = colorTex;
            upscalerFrameData.Depth = depthTex[depthIndex];
            upscalerFrameData.Motion = motionTex;
            upscalerFrameData.Output = upscaledTex;
            upscalerFrameData.Width = width;
            upscalerFrameData.Height = height;
            upscalerFrameData.JitterPixels = current.JitterPixels;
            upscalerFrameData.CameraNear = current.Near;
            upscalerFrameData.CameraFar = current.Far;
            upscalerFrameData.CameraFovY = current.FovY;
            upscalerFrameData.FrameTimeMs = std::clamp(frameTime, 1.0f, 200.0f);
            upscalerFrameData.DLSSPreset = detail::Config().DLSSPreset;
            upscalerFrameData.Reset = !HistoryUsable(upscalerFrame);

            if (!detail::TheUpscaler().Evaluate(backend, upscalerFrameData))
                return false;
            upscalerFrame = frame;
            return true;
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
            switch (requested)
            {
            case Mode::DLAA:
                return detail::TheUpscaler().IsAvailable(Backend::DLSS) ? Mode::DLAA : Mode::TAA;
            case Mode::FSR:
                return detail::TheUpscaler().IsAvailable(Backend::FSR) ? Mode::FSR : Mode::TAA;
            default:
                return requested;
            }
        }

        // Jitter only while the frames are resolved: one that isn't (post processing failed, a screen without the
        // resolve) would show it
        static bool IsJitterActive()
        {
            return requested != Mode::Off && resolveFrame != 0 && frame - resolveFrame <= 10;
        }

        // The main camera of this frame, once per frame, with the projection as the game built it. Returns the jitter
        // to move the projection by (JitterProjection), zero while nothing is resolved.
        static Jitter OnCamera(const Camera& camera)
        {
            if (requested == Mode::Off)
            {
                current.Valid = false;
                previous.Valid = false;
                return {};
            }
            ++frame;
            if (current.Valid)
                previous = current;

            FrameCamera c;
            c.View = camera.View;
            c.Projection = camera.Projection;
            c.ViewProjection = camera.View * camera.Projection;
            c.Width = camera.Width;
            c.Height = camera.Height;
            c.Frame = frame;

            // Standard [0, 1] depth: depth = (P22 z + P32) / (P23 z), 0 at near, 1 at far. P22 = P23 has no far plane
            // (True Crime: New York City): depth = 1 + P32 / (P23 z), and a far plane InfiniteFarScale times the near
            // one stands in for it.
            auto& P = camera.Projection.m;
            if (P[2][2] != 0.0 && P[2][2] != P[2][3])
            {
                c.Near = static_cast<float>(std::abs(-P[3][2] / P[2][2]));
                c.Far = static_cast<float>(std::abs(-P[3][2] / (P[2][2] - P[2][3])));
            }
            else if (P[2][2] != 0.0)
            {
                c.Near = static_cast<float>(std::abs(-P[3][2] / P[2][2]));
                c.Far = c.Near * detail::Config().InfiniteFarScale;
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
                auto index = (frame % phases) + 1;
                c.JitterPixels.X = Halton(index, 2) - 0.5f;
                c.JitterPixels.Y = Halton(index, 3) - 0.5f;
                c.JitterNdc[0] = 2.0f * c.JitterPixels.X / static_cast<float>(c.Width);
                c.JitterNdc[1] = -2.0f * c.JitterPixels.Y / static_cast<float>(c.Height);
            }

            current = c;
            return c.JitterPixels;
        }

        // Where the scene is complete and the HUD isn't drawn yet: resolves the scene (a surface the size of the render)
        // into output (may be the same surface). depth: the scene's depth in standard [0, 1] depth. Returns whether
        // output was written. Also starts the hub once DLAA or FSR is chosen, so call it every frame; with the mode Off
        // it returns at once.
        static bool Render(IDirect3DDevice9* device, IDirect3DSurface9* scene, IDirect3DBaseTexture9* depth, IDirect3DSurface9* output)
        {
            if (!device || requested == Mode::Off)
                return false;
            detail::TheUpscaler().Update(device, requested == Mode::DLAA || requested == Mode::FSR);

            auto mode = EffectiveMode();
            if (mode == Mode::Off || !scene || !depth || !output || !current.Valid || current.Frame != frame || resolveFrame == frame)
                return false;

            D3DSURFACE_DESC desc{};
            if (FAILED(scene->GetDesc(&desc)) || desc.Width != current.Width || desc.Height != current.Height)
                return false;
            bool bridge = mode == Mode::DLAA || mode == Mode::FSR;
            if (!EnsureResources(device, desc.Width, desc.Height, desc.Format, bridge))
                return false;

            IDirect3DSurface9* sceneSurface = nullptr;
            if (FAILED(sceneTex->GetSurfaceLevel(0, &sceneSurface)) || !sceneSurface)
                return false;
            bool copied = SUCCEEDED(device->StretchRect(scene, nullptr, sceneSurface, nullptr, D3DTEXF_NONE));
            sceneSurface->Release();
            if (!copied)
                return false;

            // The game's state, render targets aside, comes back with the state block
            IDirect3DSurface9* oldTargets[2]{};
            IDirect3DSurface9* oldDepth = nullptr;
            device->GetRenderTarget(0, &oldTargets[0]);
            device->GetRenderTarget(1, &oldTargets[1]);
            device->GetDepthStencilSurface(&oldDepth);
            stateBlock->Capture();

            SetFullscreenStates(device);
            RenderCameraMotion(device, depth);

            bool upscaled = false;
            if (bridge)
            {
                upscaled = RenderUpscaler(device, mode == Mode::DLAA ? Backend::DLSS : Backend::FSR);
                if (upscaled)
                {
                    // The TAA history didn't follow these frames
                    historyFrame = 0;
                    RenderOutput(device, upscaledTex, output);
                }
            }
            if (!upscaled)
            {
                upscalerFrame = 0;
                RenderResolve(device, depth);
                RenderOutput(device, historyTex[historyIndex], output);
            }
            device->SetTexture(0, nullptr);

            device->SetRenderTarget(0, oldTargets[0]);
            device->SetRenderTarget(1, oldTargets[1]);
            device->SetDepthStencilSurface(oldDepth);
            stateBlock->Apply();
            for (auto& target : oldTargets)
                detail::SafeRelease(target);
            detail::SafeRelease(oldDepth);

            resolveFrame = frame;
            return true;
        }

        // Whether the hardware can sample the depth-stencil as a texture (INTZ). DXVK and every GPU of the last fifteen
        // years can.
        static bool IntzSupported(IDirect3DDevice9* device)
        {
            IDirect3D9* d3d = nullptr;
            if (!device || FAILED(device->GetDirect3D(&d3d)) || !d3d)
                return false;
            D3DDEVICE_CREATION_PARAMETERS parameters{};
            D3DDISPLAYMODE displayMode{};
            device->GetCreationParameters(&parameters);
            d3d->GetAdapterDisplayMode(parameters.AdapterOrdinal, &displayMode);
            bool ok = SUCCEEDED(d3d->CheckDeviceFormat(parameters.AdapterOrdinal, parameters.DeviceType, displayMode.Format,
                D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_TEXTURE, static_cast<D3DFORMAT>(MAKEFOURCC('I', 'N', 'T', 'Z'))));
            d3d->Release();
            return ok;
        }

        // For games whose scene depth isn't a texture: an INTZ texture of the same size that the game uses as its
        // depth-stencil instead of original. Returns its surface with a reference for the game (which releases it like
        // the original), or null when it can't be done (multisampled, no INTZ): the game keeps the original then.
        static IDirect3DSurface9* ReplaceDepthStencil(IDirect3DDevice9* device, IDirect3DSurface9* original)
        {
            D3DSURFACE_DESC desc{};
            if (!device || !original || FAILED(original->GetDesc(&desc)))
                return nullptr;
            if (desc.MultiSampleType != D3DMULTISAMPLE_NONE)
            {
                detail::Log("Depth: the depth-stencil is multisampled, temporal anti-aliasing needs the game's MSAA off");
                return nullptr;
            }
            if (!IntzSupported(device))
            {
                detail::Log("Depth: INTZ isn't supported");
                return nullptr;
            }
            detail::SafeRelease(intzTex);
            if (FAILED(device->CreateTexture(desc.Width, desc.Height, 1, D3DUSAGE_DEPTHSTENCIL, static_cast<D3DFORMAT>(MAKEFOURCC('I', 'N', 'T', 'Z')),
                D3DPOOL_DEFAULT, &intzTex, nullptr)) || !intzTex)
            {
                detail::Log("Depth: the INTZ texture couldn't be created (%ux%u)", desc.Width, desc.Height);
                return nullptr;
            }
            IDirect3DSurface9* surface = nullptr;
            if (FAILED(intzTex->GetSurfaceLevel(0, &surface)) || !surface)
            {
                detail::SafeRelease(intzTex);
                return nullptr;
            }
            return surface;
        }

        // The texture of ReplaceDepthStencil
        static IDirect3DTexture9* DepthTexture()
        {
            return intzTex;
        }

        // Before the device is reset
        static void OnLostDevice()
        {
            ReleaseTargets();
            detail::SafeRelease(intzTex);
            detail::SafeRelease(stateBlock);
            failed = false;
        }

        // The shaders belong to the device: after it was released for a new one, not for a reset
        static void OnDeviceReleased()
        {
            OnLostDevice();
            detail::SafeRelease(cameraMotionPS);
            detail::SafeRelease(resolvePS);
            detail::SafeRelease(outputPS);
            detail::SafeRelease(copyPS);
            detail::SafeRelease(depthPS);
        }

        // When the game ends: the hub ends with it (job object), this only lets it go
        static void Shutdown()
        {
            detail::TheUpscaler().hub.Stop(false);
        }

        // Whether DLAA (Backend::DLSS) or FSR can be offered: the hub started and reported the backend, or, while the hub
        // hasn't started yet (it starts only once one of them is chosen), the game runs on DXVK and the files are there
        static bool IsAvailable(Backend backend)
        {
            auto& upscaler = detail::TheUpscaler();
            if (upscaler.state == detail::Upscaler::State::Idle)
                return RunningApi() == GraphicsApi::Vulkan && detail::BridgeFilesPresent(static_cast<UpscalerBridge::Backend>(backend));
            return upscaler.IsAvailable(backend);
        }

        // Changes whenever IsAvailable may have changed, for settings to ask again
        static uint32_t Generation()
        {
            return detail::TheUpscaler().generation.load();
        }

        // Whether DLAA or FSR could become available at all, before the hub runs: the game will run on DXVK, and
        // FusionFixHub.exe and a vendor runtime are in the game folder
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
