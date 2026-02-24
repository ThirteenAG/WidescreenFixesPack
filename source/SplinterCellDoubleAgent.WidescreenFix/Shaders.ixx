module;

#include <stdafx.h>
#include <d3d9.h>
#include "dxsdk/d3dvtbl.h"
#include <d3dx9.h>

export module Shaders;

import ComVars;

#define DUMP_SHADERS 0

struct ShaderResource
{
    uint32_t crc;
    int resourceId;
};

static ShaderResource g_shaderResources[] = {
    { 0xBB6378E1, IDR_SHADER_BB6378E1 }, // blur in cutscenes
    { 0xC081893C, IDR_SHADER_C081893C }, // Omni light type 1 brightness
};

std::filesystem::path GetShadersPath()
{
    return GetExeModulePath() / "Shaders";
}

const DWORD* LoadShaderFromResource(HMODULE hMod, int id)
{
    HRSRC hRes = FindResource(hMod, MAKEINTRESOURCE(id), RT_RCDATA);
    if (!hRes) return nullptr;
    HGLOBAL hData = LoadResource(hMod, hRes);
    if (!hData) return nullptr;
    return (const DWORD*)LockResource(hData);
}

typedef HRESULT(STDMETHODCALLTYPE* CreateDevice_t)(IDirect3D9*, UINT, D3DDEVTYPE, HWND, DWORD, D3DPRESENT_PARAMETERS*, IDirect3DDevice9**);
typedef HRESULT(STDMETHODCALLTYPE* CreatePixelShader_t)(LPDIRECT3DDEVICE9, CONST DWORD*, IDirect3DPixelShader9**);
typedef HRESULT(STDMETHODCALLTYPE* CreateVertexShader_t)(LPDIRECT3DDEVICE9, CONST DWORD*, IDirect3DVertexShader9**);

CreateDevice_t RealD3D9CreateDevice = nullptr;
CreatePixelShader_t RealCreatePixelShader = nullptr;
CreateVertexShader_t RealCreateVertexShader = nullptr;

HRESULT WINAPI Hook_CreatePixelShader(LPDIRECT3DDEVICE9 pDevice, CONST DWORD* pFunction, IDirect3DPixelShader9** ppShader)
{
    if (!pFunction)
        return RealCreatePixelShader(pDevice, pFunction, ppShader);

    // Find the length of the shader bytecode (ends with D3DPS_END = 0x0000FFFF)
    const DWORD* p = pFunction;
    size_t num_dwords = 0;
    while (*p != 0x0000FFFF)
    {
        ++num_dwords;
        ++p;
    }
    num_dwords++; // Include the D3DPS_END token
    size_t byte_length = num_dwords * sizeof(DWORD);

    #if DUMP_SHADERS
    if (std::filesystem::exists(GetShadersPath()))
    {
        uint32_t crc = crc32(0, reinterpret_cast<const void*>(pFunction), byte_length);
        std::ostringstream oss;
        oss << std::hex << std::setw(8) << std::setfill('0') << std::uppercase << crc;
        std::string name = oss.str() + ".pso";
        std::string source_name = oss.str() + ".ps";

        auto path = GetShadersPath() / name;

        if (!std::filesystem::exists(path))
        {
            std::ofstream out(path, std::ios::binary);
            if (out)
            {
                out.write(reinterpret_cast<const char*>(pFunction), byte_length);
                out.close();
            }
        }

        path = GetShadersPath() / source_name;

        if (!std::filesystem::exists(path))
        {
            std::string source;
            ID3DXBuffer* pShaderAsm = NULL;

            const HMODULE d3dx9_43 = LoadLibrary(TEXT("d3dx9_43.dll"));

            typedef HRESULT(WINAPI* PFN_D3DXDisassembleShader)(const DWORD* pShader, BOOL EnableColorCode, LPCSTR pComments, LPD3DXBUFFER* ppDisassembly);
            auto D3DXDisassembleShader = reinterpret_cast<PFN_D3DXDisassembleShader>(GetProcAddress(d3dx9_43, "D3DXDisassembleShader"));

            if (D3DXDisassembleShader)
            {
                HRESULT hr = D3DXDisassembleShader(pFunction, FALSE, NULL, &pShaderAsm);
                if (SUCCEEDED(hr) && pShaderAsm)
                {
                    source = (char*)pShaderAsm->GetBufferPointer();
                    std::ofstream out(path);
                    if (out)
                    {
                        out << source;
                        out.close();
                    }
                }
                if (pShaderAsm)
                {
                    pShaderAsm->Release();
                    pShaderAsm = nullptr;
                }
            }
            FreeLibrary(d3dx9_43);
        }
    }
    #endif

    if (std::size(g_shaderResources))
    {
        uint32_t crc = crc32(0, reinterpret_cast<const void*>(pFunction), byte_length);

        for (auto& res : g_shaderResources)
        {
            if (crc == res.crc)
            {
                HMODULE hMod = nullptr;
                GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCWSTR)&Hook_CreatePixelShader, &hMod);

                const DWORD* bytecode = LoadShaderFromResource(hMod, res.resourceId);
                if (bytecode)
                    return RealCreatePixelShader(pDevice, bytecode, ppShader);
            }
        }
    }

    return RealCreatePixelShader(pDevice, pFunction, ppShader);
}

HRESULT WINAPI Hook_CreateVertexShader(LPDIRECT3DDEVICE9 pDevice, CONST DWORD* pFunction, IDirect3DVertexShader9** ppShader)
{
    if (!pFunction)
        return RealCreateVertexShader(pDevice, pFunction, ppShader);

    // Find the length of the shader bytecode (ends with D3DVS_END = 0x0000FFFF)
    const DWORD* p = pFunction;
    size_t num_dwords = 0;
    while (*p != 0x0000FFFF)
    {
        ++num_dwords;
        ++p;
    }
    num_dwords++; // Include the D3DVS_END token
    size_t byte_length = num_dwords * sizeof(DWORD);

    #if DUMP_SHADERS
    if (std::filesystem::exists(GetShadersPath()))
    {
        uint32_t crc = crc32(0, reinterpret_cast<const void*>(pFunction), byte_length);
        std::ostringstream oss;
        oss << std::hex << std::setw(8) << std::setfill('0') << std::uppercase << crc;
        std::string name = oss.str() + ".vso";
        std::string source_name = oss.str() + ".vs";

        auto path = GetShadersPath() / name;

        if (!std::filesystem::exists(path))
        {
            std::ofstream out(path, std::ios::binary);
            if (out)
            {
                out.write(reinterpret_cast<const char*>(pFunction), byte_length);
                out.close();
            }
        }

        path = GetShadersPath() / source_name;

        if (!std::filesystem::exists(path))
        {
            std::string source;
            ID3DXBuffer* pShaderAsm = NULL;

            const HMODULE d3dx9_43 = LoadLibrary(TEXT("d3dx9_43.dll"));

            typedef HRESULT(WINAPI* PFN_D3DXDisassembleShader)(const DWORD* pShader, BOOL EnableColorCode, LPCSTR pComments, LPD3DXBUFFER* ppDisassembly);
            auto D3DXDisassembleShader = reinterpret_cast<PFN_D3DXDisassembleShader>(GetProcAddress(d3dx9_43, "D3DXDisassembleShader"));

            if (D3DXDisassembleShader)
            {
                HRESULT hr = D3DXDisassembleShader(pFunction, FALSE, NULL, &pShaderAsm);
                if (SUCCEEDED(hr) && pShaderAsm)
                {
                    source = (char*)pShaderAsm->GetBufferPointer();
                    std::ofstream out(path);
                    if (out)
                    {
                        out << source;
                        out.close();
                    }
                }
                if (pShaderAsm)
                {
                    pShaderAsm->Release();
                    pShaderAsm = nullptr;
                }
            }
            FreeLibrary(d3dx9_43);
        }
    }

    if (std::size(g_shaderResources))
    {
        uint32_t crc = crc32(0, reinterpret_cast<const void*>(pFunction), byte_length);

        for (auto& res : g_shaderResources)
        {
            if (crc == res.crc)
            {
                HMODULE hMod = nullptr;
                GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCWSTR)&Hook_CreateVertexShader, &hMod);

                const DWORD* bytecode = LoadShaderFromResource(hMod, res.resourceId);
                if (bytecode)
                    return RealCreateVertexShader(pDevice, bytecode, ppShader);
            }
        }
    }
    #endif

    return RealCreateVertexShader(pDevice, pFunction, ppShader);
}

HRESULT WINAPI Hook_CreateDevice(IDirect3D9* d3ddev, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DDevice9** ppReturnedDeviceInterface)
{
    HRESULT hr = RealD3D9CreateDevice(d3ddev, Adapter, DeviceType, hFocusWindow, BehaviorFlags, pPresentationParameters, ppReturnedDeviceInterface);
    if (FAILED(hr))
        return hr;

    UINT_PTR* pVTable = *((UINT_PTR**)*ppReturnedDeviceInterface);

    RealCreatePixelShader = (CreatePixelShader_t)pVTable[IDirect3DDevice9VTBL::CreatePixelShader];
    RealCreateVertexShader = (CreateVertexShader_t)pVTable[IDirect3DDevice9VTBL::CreateVertexShader];

    injector::WriteMemory(&pVTable[IDirect3DDevice9VTBL::CreatePixelShader], &Hook_CreatePixelShader, true);
    //injector::WriteMemory(&pVTable[IDirect3DDevice9VTBL::CreateVertexShader], &Hook_CreateVertexShader, true); //not used currently

    return hr;
}

export void InitShaders()
{
    // Hook D3D9 creation
    auto pattern = find_module_pattern(GetModuleHandle(L"D3DDrv"), "3B C7 89 83 ? ? ? ? 75");
    static auto Direct3DCreate9Hook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        auto pID3D9 = (IDirect3D9*)regs.eax;
        UINT_PTR* pVTable = (UINT_PTR*)(*((UINT_PTR*)pID3D9));
        RealD3D9CreateDevice = (CreateDevice_t)pVTable[IDirect3D9VTBL::CreateDevice];
        injector::WriteMemory(&pVTable[IDirect3D9VTBL::CreateDevice], &Hook_CreateDevice, true);
    });
}