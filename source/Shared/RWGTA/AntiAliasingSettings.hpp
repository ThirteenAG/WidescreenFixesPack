#pragma once

#include "AntiAliasing.hpp"

namespace RWGTA::AntiAliasing
{
    inline void Init(const Game& g, const char* sharpnessSetting, const char* graphicsAPISetting)
    {
        using namespace detail;
        game = g;
        auto& settings = *game.settings;
        pAntiAliasing = &settings.Int("GRAPHICS.AntiAliasing");
        pSharpness = &settings.Float(sharpnessSetting);

        auto name = GetThisModuleName<std::wstring>();
        name = name.substr(0, name.find_last_of(L'.'));
        WFPAA::Options options;
        options.BridgeName = L"Local\\" + name + L".Upscaler";
        options.ClientName = game.clientName;
        options.LowDynamicRange = true;
        WFPAA::TemporalAA::Configure(options);

        // Graphics API: before the game creates its Direct3D object. Vulkan is offered once the hub installed DXVK
        // (vulkan.dll in the game folder), D3D9on12 only with GRAPHICS.D3D9on12 = 1 in the file.
        static bool allowD3D9On12 = settings.Bool("GRAPHICS.D3D9on12");
        settings.Availability(graphicsAPISetting, [](std::string_view value)
        {
            if (value == "1")
                return WFPAA::VulkanAvailable();
            if (value == "2")
                return allowD3D9On12;
            return true;
        });
        if (settings.Int(graphicsAPISetting) != 0)
            WFPAA::InstallGraphicsApi(settings.Int(graphicsAPISetting), allowD3D9On12);

        // DLAA and FSR once the hub reported them (it starts with the device), TAA unless the depth can't be read or
        // the camera wasn't found
        settings.Availability("GRAPHICS.AntiAliasing", [](std::string_view value)
        {
            bool temporal = !bDepthUnusable && bCameraFound;
            if (value == "2")
                return temporal;
            if (value == "3")
                return temporal && WFPAA::TemporalAA::IsAvailable(WFPAA::Backend::DLSS);
            if (value == "4")
                return temporal && WFPAA::TemporalAA::IsAvailable(WFPAA::Backend::FSR);
            return true;
        });

        // _rwD3D8CameraBeginUpdate: the view's and the projection's RwD3D8SetTransform, and the raster extension's
        // offset just after them. III 1.0/1.1 and Vice City: cdecl (state, matrix), the camera in edi; III Steam:
        // eax = state, edx = matrix, the camera in ebx. Found here, hooked once a temporal mode is chosen.
        auto view = hook::pattern("6A 02 C7 44 24 ? 00 00 00 00 C7 44 24 ? 00 00 00 00 C7 84 24 ? ? ? ? 00 00 00 00 89 94 24 ? ? ? ? 89 84 24 ? ? ? ? C7 84 24 ? ? ? ? 00 00 80 3F E8");
        auto projection = hook::pattern("51 6A 03 D9 E0 D9 9C 24 ? ? ? ? E8 ? ? ? ? 8B 6F 60 8B 15");
        if (view.size() == 1 && projection.size() == 1)
        {
            pRasterExtOffset = *projection.get_first<int32_t*>(22);
            pViewCall = view.get_first<uint8_t>(0x36);
            pProjectionCall = projection.get_first<uint8_t>(12);
            bCameraFound = true;
        }
        else
        {
            view = hook::pattern("B8 02 00 00 00 89 95 ? ? ? ? 8D 95 ? ? ? ? E8");
            projection = hook::pattern("B8 03 00 00 00 D9 9D ? ? ? ? 8D 95 ? ? ? ? E8 ? ? ? ? 8B 73 60 8B 3D");
            if (view.size() == 1 && projection.size() == 1)
            {
                pRasterExtOffset = *projection.get_first<int32_t*>(27);
                pViewCall = view.get_first<uint8_t>(17);
                pProjectionCall = projection.get_first<uint8_t>(17);
                bSteamCamera = true;
                bCameraFound = true;
            }
        }
        settings.Refresh("GRAPHICS.AntiAliasing");

        WFP::onBeforeReset() += []()
        {
            if (pRasterExtOffset)
                game.restoreDepth(game.device8(), game.sceneCamera(), *pRasterExtOffset);
            bDepthReplaced = false;
            bJittered = false;
            WFPAA::TemporalAA::OnLostDevice();
        };

        WFP::onShutdownEvent() += []()
        {
            WFPAA::TemporalAA::Shutdown();
        };
    }
}
