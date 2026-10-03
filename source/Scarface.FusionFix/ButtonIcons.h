#pragma once
#include <d3d9.h>
#include <d3dx9.h>
#include <wrl/client.h>

namespace ScarfaceButtons
{
    // Physical element order shared with XidiGetPhysicalButtonMask.
    inline constexpr const char* Names[] = {
        "Left Stick", "Left Stick", "Right Stick", "Right Stick",
        "D-Pad Up", "D-Pad Down", "D-Pad Left", "D-Pad Right",
        "LT", "RT", "A", "B", "X", "Y", "LB", "RB", "Back", "Start", "LS", "RS"
    };

    inline Microsoft::WRL::ComPtr<IDirect3DTexture9> CreateIcon(IDirect3DDevice9* device, unsigned id)
    {
        // DDS assets embedded unchanged from Max Payne 3 Fusion Fix's buttons_pc.wtd.
        constexpr unsigned resources[] = {
            4100,4100,4101,4101,4102,4103,4104,4105,4106,4107,
            4108,4109,4110,4111,4112,4113,4114,4115,4116,4117
        };
        if(id>=std::size(resources))return {};
        HMODULE module=nullptr;
        if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&CreateIcon),&module))return {};
        Microsoft::WRL::ComPtr<IDirect3DTexture9> texture;
        if(FAILED(D3DXCreateTextureFromResourceExW(device,module,MAKEINTRESOURCEW(resources[id]),
            D3DX_DEFAULT,D3DX_DEFAULT,1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,
            D3DX_FILTER_NONE,D3DX_FILTER_NONE,0,nullptr,nullptr,texture.GetAddressOf())))return {};

        // Scarface's native font glyph UVs run bottom-to-top.
        D3DSURFACE_DESC desc{};
        if(FAILED(texture->GetLevelDesc(0,&desc)))return {};
        D3DLOCKED_RECT locked{};
        if(FAILED(texture->LockRect(0,&locked,nullptr,0)))return {};
        auto pixels=static_cast<uint8_t*>(locked.pBits);
        for(unsigned y=0;y<desc.Height/2;++y)
            for(unsigned x=0;x<desc.Width*4;++x)
                std::swap(pixels[y*locked.Pitch+x],pixels[(desc.Height-1-y)*locked.Pitch+x]);
        texture->UnlockRect(0);
        return texture;
    }
}
