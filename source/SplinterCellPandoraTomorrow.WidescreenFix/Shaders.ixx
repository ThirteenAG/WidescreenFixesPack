module;

#include <stdafx.h>
#include <dxsdk/dx8/d3d8.h>
#include "dxsdk/d3dvtbl.h"

export module Shaders;

import ComVars;

std::filesystem::path GetShadersPath()
{
    return GetExeModulePath() / "Shaders";
}

std::string ExtractPixelShaderFilename(const DWORD* pFunction, size_t dataSize)
{
    const char* data = reinterpret_cast<const char*>(pFunction);
    const char* fileTag = nullptr;
    for (size_t i = 0; i < dataSize; ++i)
    {
        if (data[i] == 'F' && data[i + 1] == 'I' && data[i + 2] == 'L' && data[i + 3] == 'E')
        {
            fileTag = data + i;
            break;
        }
    }
    if (!fileTag) return "";
    fileTag += 4;
    const char* end = fileTag;
    while (*end != '\0') ++end;
    std::string fullPath(fileTag, end - fileTag);
    size_t lastSlash = fullPath.find_last_of('\\');
    if (lastSlash != std::string::npos)
        return fullPath.substr(lastSlash + 1);
    return fullPath;
}

const DWORD* LoadResource(HMODULE hMod, int id)
{
    HRSRC hRes = FindResource(hMod, MAKEINTRESOURCE(id), RT_RCDATA);
    if (!hRes) return nullptr;
    HGLOBAL hData = LoadResource(hMod, hRes);
    if (!hData) return nullptr;
    return (const DWORD*)LockResource(hData);
}

typedef HRESULT(STDMETHODCALLTYPE* CreateDevice_t)(IDirect3D8*, UINT, D3DDEVTYPE, HWND, DWORD, D3DPRESENT_PARAMETERS*, IDirect3DDevice8**);
typedef HRESULT(STDMETHODCALLTYPE* CreatePixelShader_t)(LPDIRECT3DDEVICE8, CONST DWORD*, DWORD*);
typedef HRESULT(STDMETHODCALLTYPE* CreateVertexShader_t)(LPDIRECT3DDEVICE8, CONST DWORD*, CONST DWORD*, DWORD*, DWORD);

CreateDevice_t RealD3D8CreateDevice = nullptr;
CreatePixelShader_t RealCreatePixelShader = nullptr;
CreateVertexShader_t RealCreateVertexShader = nullptr;

HRESULT WINAPI Hook_CreatePixelShader(LPDIRECT3DDEVICE8 pDevice, CONST DWORD* pFunction, DWORD* pHandle)
{
    if (!pFunction)
        return RealCreatePixelShader(pDevice, pFunction, pHandle);

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

    std::string name = ExtractPixelShaderFilename(pFunction, byte_length);

    if (name.empty())
    {
        // Compute CRC32
        uint32_t crc = crc32(0, reinterpret_cast<const void*>(pFunction), byte_length);
        std::ostringstream oss;
        oss << std::hex << std::setw(8) << std::setfill('0') << std::uppercase << crc;
        name = oss.str() + ".pso";
    }

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

    std::ifstream in(path, std::ios::binary);
    std::vector<char> pNewFunction{ std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>() };

    HRESULT hr = RealCreatePixelShader(pDevice, reinterpret_cast<const DWORD*>(pNewFunction.data()), pHandle);
    if (FAILED(hr))
        return RealCreatePixelShader(pDevice, pFunction, pHandle);

    return hr;
}

HRESULT WINAPI Hook_CreateVertexShader(LPDIRECT3DDEVICE8 pDevice, CONST DWORD* pDeclaration, CONST DWORD* pFunction, DWORD* pHandle, DWORD Usage)
{
    if (!pFunction)
        return RealCreateVertexShader(pDevice, pDeclaration, pFunction, pHandle, Usage);

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

    // Compute CRC32
    uint32_t crc = crc32(0, reinterpret_cast<const void*>(pFunction), byte_length);
    std::ostringstream oss;
    oss << std::hex << std::setw(8) << std::setfill('0') << std::uppercase << crc;
    std::string name = oss.str() + ".vso";

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

    std::ifstream in(path, std::ios::binary);
    std::vector<char> pNewFunction{ std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>() };

    HRESULT hr = RealCreateVertexShader(pDevice, pDeclaration, reinterpret_cast<const DWORD*>(pNewFunction.data()), pHandle, Usage);
    if (FAILED(hr))
        return RealCreateVertexShader(pDevice, pDeclaration, pFunction, pHandle, Usage);

    return hr;
}

HRESULT WINAPI Hook_CreateDevice(IDirect3D8* d3ddev, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DDevice8** ppReturnedDeviceInterface)
{
    HRESULT hr = RealD3D8CreateDevice(d3ddev, Adapter, DeviceType, hFocusWindow, BehaviorFlags, pPresentationParameters, ppReturnedDeviceInterface);
    if (FAILED(hr))
        return hr;

    UINT_PTR* pVTable = *((UINT_PTR**)*ppReturnedDeviceInterface);

    if (pVTable[IDirect3DDevice9VTBL::CreatePixelShader] != (UINT_PTR)Hook_CreatePixelShader)
    {
        RealCreatePixelShader = (CreatePixelShader_t)pVTable[IDirect3DDevice9VTBL::CreatePixelShader];
        injector::WriteMemory(&pVTable[IDirect3DDevice9VTBL::CreatePixelShader], &Hook_CreatePixelShader, true);
    }

    if (pVTable[IDirect3DDevice9VTBL::CreateVertexShader] != (UINT_PTR)Hook_CreateVertexShader)
    {
        RealCreateVertexShader = (CreateVertexShader_t)pVTable[IDirect3DDevice9VTBL::CreateVertexShader];
        injector::WriteMemory(&pVTable[IDirect3DDevice9VTBL::CreateVertexShader], &Hook_CreateVertexShader, true);
    }

    return hr;
}

export void InitShaders()
{
    if (std::filesystem::exists(GetShadersPath()))
    {
        // Hook D3D8 creation
        auto pattern = find_module_pattern(GetModuleHandle(L"D3DDrv"), "89 83 ? ? ? ? 50");
        static auto Direct3DCreate8Hook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            auto pID3D8 = (IDirect3D8*)regs.eax;
            UINT_PTR* pVTable = (UINT_PTR*)(*((UINT_PTR*)pID3D8));
            RealD3D8CreateDevice = (CreateDevice_t)pVTable[IDirect3D8VTBL::CreateDevice];
            injector::WriteMemory(&pVTable[IDirect3D8VTBL::CreateDevice], &Hook_CreateDevice, true);
        });
    }
}