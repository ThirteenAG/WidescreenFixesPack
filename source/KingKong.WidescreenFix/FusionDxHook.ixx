module;

#include "stdafx.h"
#include <d3d9.h>
#include <d3dx9.h>

export module FusionDxHook;

import ComVars;

import <map>;
import <functional>;
import <numeric>;
import <algorithm>;
import <functional>;
import <typeindex>;
import <typeinfo>;

float fMaxObjectLength = 3005.0f;
IDirect3DVertexBuffer9* g_pCurrentVB = NULL;
UINT g_CurrentOffset = 0;
UINT g_CurrentStride = 0;

namespace ShaderDumper
{
    static constexpr const char* SHADER_DUMP_DIR = "ShaderDumps";

    void EnsureDumpDirectoryExists()
    {
        std::filesystem::create_directories(SHADER_DUMP_DIR);
    }

    std::string GetShaderDumpPath(uint32_t crc32Value, const char* shaderType)
    {
        std::stringstream ss;
        ss << SHADER_DUMP_DIR << "\\"
            << std::hex << std::setfill('0') << std::setw(8) << std::uppercase << crc32Value
            << "_" << shaderType << ".bin";
        return ss.str();
    }

    void DumpVertexShader(const DWORD* pFunction)
    {
        if (!pFunction) return;

        // Get shader size by reading DWORD count
        UINT shaderSize = 0;
        const DWORD* ptr = pFunction;
        while (*ptr != 0x0000FFFF && shaderSize < 1000000) // Safety limit
        {
            shaderSize += 4;
            ptr++;
        }
        shaderSize += 4; // Include the end marker

        uint32_t crcValue = crc32(0, (uint8_t*)pFunction, shaderSize);
        std::string dumpPath = GetShaderDumpPath(crcValue, "vs");

        // Check if already dumped
        if (std::filesystem::exists(dumpPath))
            return;

        std::ofstream file(dumpPath, std::ios::binary);
        if (file.is_open())
        {
            file.write((const char*)pFunction, shaderSize);
            file.close();
        }
    }

    void DumpPixelShader(const DWORD* pFunction)
    {
        if (!pFunction) return;

        UINT shaderSize = 0;
        const DWORD* ptr = pFunction;
        while (*ptr != 0x0000FFFF && shaderSize < 1000000)
        {
            shaderSize += 4;
            ptr++;
        }
        shaderSize += 4;

        uint32_t crcValue = crc32(0, (uint8_t*)pFunction, shaderSize);
        std::string dumpPath = GetShaderDumpPath(crcValue, "ps");

        if (std::filesystem::exists(dumpPath))
            return;

        std::ofstream file(dumpPath, std::ios::binary);
        if (file.is_open())
        {
            file.write((const char*)pFunction, shaderSize);
            file.close();
        }
    }
}

class VTBL
{
private:
    virtual std::vector<const char*>& GetMethods() = 0;
public:
    const auto GetNumberOfMethods()
    {
        return GetMethods().size();
    }
    const auto GetIndex(const char* name)
    {
        auto iter = std::find(std::begin(GetMethods()), std::end(GetMethods()), name);
        if (iter != std::end(GetMethods()))
            return (int32_t)std::distance(std::begin(GetMethods()), iter);
        else
            return (int32_t)-1;
    }
    const auto GetMethod(int32_t index)
    {
        if (index < static_cast<int32_t>(GetNumberOfMethods()))
            return GetMethods()[index];
        else
            return "";
    }
};

class IDirect3D9VTBL : public VTBL
{
private:
    std::vector<const char*>& GetMethods() override
    {
        static std::vector<const char*> methodsNames{
            "QueryInterface",
            "AddRef",
            "Release",
            "RegisterSoftwareDevice",
            "GetAdapterCount",
            "GetAdapterIdentifier",
            "GetAdapterModeCount",
            "EnumAdapterModes",
            "GetAdapterDisplayMode",
            "CheckDeviceType",
            "CheckDeviceFormat",
            "CheckDeviceMultiSampleType",
            "CheckDepthStencilMatch",
            "CheckDeviceFormatConversion",
            "GetDeviceCaps",
            "GetAdapterMonitor",
            "CreateDevice",
        };
        return methodsNames;
    }
};

class IDirect3DDevice9VTBL : public VTBL
{
private:
    std::vector<const char*>& GetMethods() override
    {
        static std::vector<const char*> methodsNames{
            "QueryInterface",
            "AddRef",
            "Release",
            "TestCooperativeLevel",
            "GetAvailableTextureMem",
            "EvictManagedResources",
            "GetDirect3D",
            "GetDeviceCaps",
            "GetDisplayMode",
            "GetCreationParameters",
            "SetCursorProperties",
            "SetCursorPosition",
            "ShowCursor",
            "CreateAdditionalSwapChain",
            "GetSwapChain",
            "GetNumberOfSwapChains",
            "Reset",
            "Present",
            "GetBackBuffer",
            "GetRasterStatus",
            "SetDialogBoxMode",
            "SetGammaRamp",
            "GetGammaRamp",
            "CreateTexture",
            "CreateVolumeTexture",
            "CreateCubeTexture",
            "CreateVertexBuffer",
            "CreateIndexBuffer",
            "CreateRenderTarget",
            "CreateDepthStencilSurface",
            "UpdateSurface",
            "UpdateTexture",
            "GetRenderTargetData",
            "GetFrontBufferData",
            "StretchRect",
            "ColorFill",
            "CreateOffscreenPlainSurface",
            "SetRenderTarget",
            "GetRenderTarget",
            "SetDepthStencilSurface",
            "GetDepthStencilSurface",
            "BeginScene",
            "EndScene",
            "Clear",
            "SetTransform",
            "GetTransform",
            "MultiplyTransform",
            "SetViewport",
            "GetViewport",
            "SetMaterial",
            "GetMaterial",
            "SetLight",
            "GetLight",
            "LightEnable",
            "GetLightEnable",
            "SetClipPlane",
            "GetClipPlane",
            "SetRenderState",
            "GetRenderState",
            "CreateStateBlock",
            "BeginStateBlock",
            "EndStateBlock",
            "SetClipStatus",
            "GetClipStatus",
            "GetTexture",
            "SetTexture",
            "GetTextureStageState",
            "SetTextureStageState",
            "GetSamplerState",
            "SetSamplerState",
            "ValidateDevice",
            "SetPaletteEntries",
            "GetPaletteEntries",
            "SetCurrentTexturePalette",
            "GetCurrentTexturePalette",
            "SetScissorRect",
            "GetScissorRect",
            "SetSoftwareVertexProcessing",
            "GetSoftwareVertexProcessing",
            "SetNPatchMode",
            "GetNPatchMode",
            "DrawPrimitive",
            "DrawIndexedPrimitive",
            "DrawPrimitiveUP",
            "DrawIndexedPrimitiveUP",
            "ProcessVertices",
            "CreateVertexDeclaration",
            "SetVertexDeclaration",
            "GetVertexDeclaration",
            "SetFVF",
            "GetFVF",
            "CreateVertexShader",
            "SetVertexShader",
            "GetVertexShader",
            "SetVertexShaderConstantF",
            "GetVertexShaderConstantF",
            "SetVertexShaderConstantI",
            "GetVertexShaderConstantI",
            "SetVertexShaderConstantB",
            "GetVertexShaderConstantB",
            "SetStreamSource",
            "GetStreamSource",
            "SetStreamSourceFreq",
            "GetStreamSourceFreq",
            "SetIndices",
            "GetIndices",
            "CreatePixelShader",
            "SetPixelShader",
            "GetPixelShader",
            "SetPixelShaderConstantF",
            "GetPixelShaderConstantF",
            "SetPixelShaderConstantI",
            "GetPixelShaderConstantI",
            "SetPixelShaderConstantB",
            "GetPixelShaderConstantB",
            "DrawRectPatch",
            "DrawTriPatch",
            "DeletePatch",
            "CreateQuery",
            "SetConvolutionMonoKernel",
            "ComposeRects",
            "PresentEx",
            "GetGPUThreadPriority",
            "SetGPUThreadPriority",
            "WaitForVBlank",
            "CheckResourceResidency",
            "SetMaximumFrameLatency",
            "GetMaximumFrameLatency",
            "CheckDeviceState",
            "CreateRenderTargetEx",
            "CreateOffscreenPlainSurfaceEx",
            "CreateDepthStencilSurfaceEx",
            "ResetEx",
            "GetDisplayModeEx",
        };
        return methodsNames;
    }
};

export SafetyHookInline CreateDeviceOriginal = {};
export SafetyHookInline BeginSceneOriginal = {};
export SafetyHookInline EndSceneOriginal = {};
export SafetyHookInline ResetOriginal = {};
export SafetyHookInline SetVertexShaderConstantFOriginal = {};
export SafetyHookInline SetPixelShaderConstantFOriginal = {};
export SafetyHookInline CreateTextureOriginal = {};
export SafetyHookInline SetTextureOriginal = {};
export SafetyHookInline DrawPrimitiveOriginal = {};
export SafetyHookInline SetPixelShaderOriginal = {};
export SafetyHookInline SetVertexShaderOriginal = {};
export SafetyHookInline CreatePixelShaderOriginal = {};
export SafetyHookInline CreateVertexShaderOriginal = {};
export SafetyHookInline SetStreamSourceOriginal = {};

class FusionDxHook
{
private:
    static inline std::map<const HMODULE, std::map<std::type_index, std::vector<uintptr_t*>>> deviceMethods;

    class HookWindow
    {
    public:
        HookWindow(std::wstring_view className = L"FusionDxHook", std::wstring windowName = L"FusionDxHook")
        {
            windowClass.cbSize = sizeof(WNDCLASSEX);
            windowClass.style = CS_HREDRAW | CS_VREDRAW;
            windowClass.lpfnWndProc = DefWindowProc;
            windowClass.cbClsExtra = 0;
            windowClass.cbWndExtra = 0;
            windowClass.hInstance = GetModuleHandle(NULL);
            windowClass.hIcon = NULL;
            windowClass.hCursor = NULL;
            windowClass.hbrBackground = NULL;
            windowClass.lpszMenuName = NULL;
            windowClass.lpszClassName = className.data();
            windowClass.hIconSm = NULL;
            RegisterClassExW(&windowClass);
            window = CreateWindowW(windowClass.lpszClassName, windowName.data(), WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, NULL, NULL, windowClass.hInstance, NULL);
        }
        ~HookWindow()
        {
            DestroyWindow(window);
            UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
        }
        static HWND GetHookWindow(HookWindow& hw)
        {
            return hw.window;
        }
    private:
        WNDCLASSEX windowClass{};
        HWND window{};
    };

    template <class... Ts>
    static inline void copyMethods(HMODULE mod, Ts&& ... inputs)
    {
        ([&]
        {
            auto vtbl_info = std::get<0>(inputs);
            auto ptr = std::get<1>(inputs);
            deviceMethods[mod][typeid(vtbl_info)].resize(vtbl_info.GetNumberOfMethods());
            std::memcpy(deviceMethods.at(mod).at(typeid(vtbl_info)).data(), ptr, vtbl_info.GetNumberOfMethods() * sizeof(uintptr_t));
        } (), ...);
    }

public:
    FusionDxHook()
    {
        CIniReader iniReader("");
        bHideUntexturedObjects = iniReader.ReadInteger("MISC", "HideUntexturedObjects", 1) != 0;
        bDisableBlur = iniReader.ReadInteger("MAIN", "DisableBlur", 1) != 0;
        bVSync = iniReader.ReadInteger("MAIN", "VSync", 1) != 0;

        CallbackHandler::RegisterCallback(L"d3d9.dll", []()
        {
            std::thread([]()
            {
                auto hD3D9 = GetModuleHandleW(L"d3d9.dll");
                if (!hD3D9) return;
                auto Direct3DCreate9 = GetProcAddress(hD3D9, "Direct3DCreate9");
                if (!Direct3DCreate9) return;
                auto Direct3D9 = ((LPDIRECT3D9(WINAPI*)(uint32_t))(Direct3DCreate9))(D3D_SDK_VERSION);
                if (!Direct3D9) return;

                auto hookWindowCtor = HookWindow(L"FusionDxHookD3D9", L"FusionDxHookD3D9");
                auto hWnd = HookWindow::GetHookWindow(hookWindowCtor);

                D3DPRESENT_PARAMETERS params;
                params.BackBufferWidth = 0;
                params.BackBufferHeight = 0;
                params.BackBufferFormat = D3DFMT_UNKNOWN;
                params.BackBufferCount = 0;
                params.MultiSampleType = D3DMULTISAMPLE_NONE;
                params.MultiSampleQuality = NULL;
                params.SwapEffect = D3DSWAPEFFECT_DISCARD;
                params.hDeviceWindow = hWnd;
                params.Windowed = 1;
                params.EnableAutoDepthStencil = 0;
                params.AutoDepthStencilFormat = D3DFMT_UNKNOWN;
                params.Flags = NULL;
                params.FullScreen_RefreshRateInHz = 0;
                params.PresentationInterval = 0;

                LPDIRECT3DDEVICE9 Device;
                if (Direct3D9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_NULLREF, hWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_DISABLE_DRIVER_MANAGEMENT, &params, &Device) >= 0)
                {
                    copyMethods(hD3D9,
                        std::forward_as_tuple(IDirect3DDevice9VTBL(), *(uintptr_t**)Device),
                        std::forward_as_tuple(IDirect3D9VTBL(), *(uintptr_t**)Direct3D9)
                    );

                    auto D3D9CreateDevice = [](LPDIRECT3D9 pDirect3D9, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DDevice9** ppReturnedDeviceInterface) -> HRESULT
                    {
                        if (bVSync && pPresentationParameters)
                        {
                            pPresentationParameters->PresentationInterval = D3DPRESENT_INTERVAL_ONE;
                            pPresentationParameters->FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
                        }
                        auto res = CreateDeviceOriginal.unsafe_stdcall<HRESULT>(pDirect3D9, Adapter, DeviceType, hFocusWindow, BehaviorFlags, pPresentationParameters, ppReturnedDeviceInterface);
                        return res;
                    };

                    auto D3D9BeginScene = [](LPDIRECT3DDEVICE9 pDevice) -> HRESULT
                    {
                        return BeginSceneOriginal.unsafe_stdcall<HRESULT>(pDevice);
                    };

                    auto D3D9EndScene = [](LPDIRECT3DDEVICE9 pDevice) -> HRESULT
                    {
                        return EndSceneOriginal.unsafe_stdcall<HRESULT>(pDevice);
                    };

                    auto D3D9Reset = [](LPDIRECT3DDEVICE9 pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters) -> HRESULT
                    {
                        // Clear vertex buffer state before reset
                        g_pCurrentVB = NULL;
                        g_CurrentOffset = 0;
                        g_CurrentStride = 0;

                        if (bVSync && pPresentationParameters)
                        {
                            pPresentationParameters->PresentationInterval = D3DPRESENT_INTERVAL_ONE;
                            pPresentationParameters->FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
                        }
                        auto res = ResetOriginal.unsafe_stdcall<HRESULT>(pDevice, pPresentationParameters);
                        return res;
                    };

                    auto D3D9SetVertexShaderConstantF = [](LPDIRECT3DDEVICE9 pDevice, UINT StartRegister, float* pConstantData, UINT Vector4fCount) -> HRESULT
                    {
                        return SetVertexShaderConstantFOriginal.unsafe_stdcall<HRESULT>(pDevice, StartRegister, pConstantData, Vector4fCount);
                    };

                    auto D3D9SetPixelShaderConstantF = [](LPDIRECT3DDEVICE9 pDevice, UINT StartRegister, float* pConstantData, UINT Vector4fCount) -> HRESULT
                    {
                        return SetPixelShaderConstantFOriginal.unsafe_stdcall<HRESULT>(pDevice, StartRegister, pConstantData, Vector4fCount);
                    };

                    auto D3D9CreateTexture = [](LPDIRECT3DDEVICE9 pDevice, UINT Width, UINT Height, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DTexture9** ppTexture, HANDLE* pSharedHandle) ->HRESULT
                    {
                        auto res = CreateTextureOriginal.unsafe_stdcall<HRESULT>(pDevice, Width, Height, Levels, Usage, Format, Pool, ppTexture, pSharedHandle);
                        return res;
                    };

                    auto D3D9SetTexture = [](LPDIRECT3DDEVICE9 pDevice, DWORD Stage, IDirect3DBaseTexture9* pTexture) -> HRESULT
                    {
                        return SetTextureOriginal.unsafe_stdcall<HRESULT>(pDevice, Stage, pTexture);
                    };

                    auto D3D9DrawPrimitive = [](LPDIRECT3DDEVICE9 pDevice, D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount) -> HRESULT
                    {
                        #if 0 //def _DEBUG
                        static bool bShaderDumpInitialized = false;
                        if (!bShaderDumpInitialized)
                        {
                            ShaderDumper::EnsureDumpDirectoryExists();
                            bShaderDumpInitialized = true;
                        }

                        // Get and dump current vertex shader
                        IDirect3DVertexShader9* pCurrentVS = nullptr;
                        if (SUCCEEDED(pDevice->GetVertexShader(&pCurrentVS)) && pCurrentVS)
                        {
                            UINT len = 0;
                            if (SUCCEEDED(pCurrentVS->GetFunction(nullptr, &len)) && len > 0)
                            {
                                std::vector<DWORD> vsData(len / 4);
                                if (SUCCEEDED(pCurrentVS->GetFunction(vsData.data(), &len)))
                                {
                                    ShaderDumper::DumpVertexShader(vsData.data());
                                }
                            }
                            pCurrentVS->Release();
                        }

                        // Get and dump current pixel shader
                        IDirect3DPixelShader9* pCurrentPS = nullptr;
                        if (SUCCEEDED(pDevice->GetPixelShader(&pCurrentPS)) && pCurrentPS)
                        {
                            UINT len = 0;
                            if (SUCCEEDED(pCurrentPS->GetFunction(nullptr, &len)) && len > 0)
                            {
                                std::vector<DWORD> psData(len / 4);
                                if (SUCCEEDED(pCurrentPS->GetFunction(psData.data(), &len)))
                                {
                                    ShaderDumper::DumpPixelShader(psData.data());
                                }
                            }
                            pCurrentPS->Release();
                        }
                        #endif

                        bool isSuspectType = (PrimitiveType == D3DPT_TRIANGLESTRIP || PrimitiveType == D3DPT_TRIANGLELIST);
                        // 367 - small grass
                        // 909 - tall grass
                        if (isSuspectType && StartVertex == 0 && PrimitiveCount > 300 && g_pCurrentVB && g_CurrentStride >= 12 && PrimitiveCount != 909 && PrimitiveCount != 367)
                        {
                            void* pData = NULL;
                            UINT numVerts;
                            if (PrimitiveType == D3DPT_TRIANGLESTRIP)
                            {
                                numVerts = PrimitiveCount + 2;
                            }
                            else // D3DPT_TRIANGLELIST
                            {
                                numVerts = PrimitiveCount * 3;
                            }
                            UINT lockOffset = g_CurrentOffset + StartVertex * g_CurrentStride;
                            UINT lockSize = numVerts * g_CurrentStride;

                            if (SUCCEEDED(g_pCurrentVB->Lock(lockOffset, lockSize, &pData, D3DLOCK_READONLY)))
                            {
                                if (pData) // Extra safety check
                                {
                                    D3DXVECTOR3* verts = (D3DXVECTOR3*)((BYTE*)pData + 0);
                                    D3DXVECTOR3 minV = verts[0];
                                    D3DXVECTOR3 maxV = verts[0];

                                    for (UINT i = 1; i < numVerts; ++i)
                                    {
                                        D3DXVec3Minimize(&minV, &minV, &verts[i]);
                                        D3DXVec3Maximize(&maxV, &maxV, &verts[i]);
                                    }

                                    D3DXVECTOR3 extent;
                                    D3DXVec3Subtract(&extent, &maxV, &minV);
                                    float length = D3DXVec3Length(&extent);
                                    g_pCurrentVB->Unlock();

                                    if (std::isinf(length) || std::isnan(length) || length > fMaxObjectLength)
                                    {
                                        return D3D_OK;
                                    }
                                }
                                else
                                {
                                    g_pCurrentVB->Unlock();
                                }
                            }
                        }

                        auto hr = DrawPrimitiveOriginal.unsafe_stdcall<HRESULT>(pDevice, PrimitiveType, StartVertex, PrimitiveCount);
                        return hr;
                    };

                    auto D3D9SetPixelShader = [](LPDIRECT3DDEVICE9 pDevice, IDirect3DPixelShader9* pShader) -> HRESULT
                    {
                        auto hr = SetPixelShaderOriginal.unsafe_stdcall<HRESULT>(pDevice, pShader);
                        return hr;
                    };

                    auto D3D9SetVertexShader = [](LPDIRECT3DDEVICE9 pDevice, IDirect3DVertexShader9* pShader) -> HRESULT
                    {
                        auto hr = SetVertexShaderOriginal.unsafe_stdcall<HRESULT>(pDevice, pShader);
                        return hr;
                    };

                    auto D3D9CreatePixelShader = [](LPDIRECT3DDEVICE9 pDevice, DWORD* pFunction, IDirect3DPixelShader9** ppShader) -> HRESULT
                    {
                        auto hr = CreatePixelShaderOriginal.unsafe_stdcall<HRESULT>(pDevice, pFunction, ppShader);

                        if (SUCCEEDED(hr) && *ppShader != nullptr)
                        {
                            static std::vector<uint8_t> pbFunc;
                            UINT len;
                            (*ppShader)->GetFunction(nullptr, &len);
                            if (pbFunc.size() < len)
                                pbFunc.resize(len);

                            (*ppShader)->GetFunction(pbFunc.data(), &len);

                            if (crc32(0, pbFunc.data(), len) == 0x15BF4BA3)
                            {
                                HMODULE hModule;
                                GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)&CreatePixelShaderOriginal, &hModule);
                                HRSRC hResource = FindResource(hModule, MAKEINTRESOURCE(IDR_BLURPS), RT_RCDATA);
                                if (hResource)
                                {
                                    HGLOBAL hGlob = LoadResource(hModule, hResource);
                                    if (hGlob)
                                    {
                                        LPVOID pData = LockResource(hGlob);
                                        DWORD size = SizeofResource(hModule, hResource);
                                        if (pData && size > 0)
                                        {
                                            LPDWORD shader_data = (LPDWORD)pData;
                                            IDirect3DPixelShader9* newShader = nullptr;
                                            HRESULT createResult = CreatePixelShaderOriginal.unsafe_stdcall<HRESULT>(pDevice, shader_data, &newShader);
                                            if (SUCCEEDED(createResult))
                                            {
                                                (*ppShader)->Release();
                                                *ppShader = newShader;
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        return hr;
                    };

                    auto D3D9CreateVertexShader = [](LPDIRECT3DDEVICE9 pDevice, DWORD* pFunction, IDirect3DVertexShader9** ppShader) -> HRESULT
                    {
                        auto hr = CreateVertexShaderOriginal.unsafe_stdcall<HRESULT>(pDevice, pFunction, ppShader);

                        if (SUCCEEDED(hr) && *ppShader != nullptr)
                        {
                            static std::vector<uint8_t> pbFunc;
                            UINT len;
                            (*ppShader)->GetFunction(nullptr, &len);
                            if (pbFunc.size() < len)
                                pbFunc.resize(len);

                            (*ppShader)->GetFunction(pbFunc.data(), &len);

                            auto crc = crc32(0, pbFunc.data(), len);
                            if (crc == 0xF5A4509D || crc == 0x16C23572)
                            {
                                HMODULE hModule;
                                GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)&CreateVertexShaderOriginal, &hModule);
                                HRSRC hResource = FindResource(hModule, MAKEINTRESOURCE(IDR_BLURVS), RT_RCDATA);
                                if (hResource)
                                {
                                    HGLOBAL hGlob = LoadResource(hModule, hResource);
                                    if (hGlob)
                                    {
                                        LPVOID pData = LockResource(hGlob);
                                        DWORD size = SizeofResource(hModule, hResource);
                                        if (pData && size > 0)
                                        {
                                            LPDWORD shader_data = (LPDWORD)pData;
                                            IDirect3DVertexShader9* newShader = nullptr;
                                            HRESULT createResult = CreateVertexShaderOriginal.unsafe_stdcall<HRESULT>(pDevice, shader_data, &newShader);
                                            if (SUCCEEDED(createResult))
                                            {
                                                (*ppShader)->Release();
                                                *ppShader = newShader;
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        return hr;
                    };

                    auto D3D9SetStreamSource = [](LPDIRECT3DDEVICE9 pDevice, UINT StreamNumber, IDirect3DVertexBuffer9* pStreamData, UINT OffsetInBytes, UINT Stride) -> HRESULT
                    {
                        if (StreamNumber == 0)
                        {
                            g_pCurrentVB = pStreamData;
                            g_CurrentOffset = OffsetInBytes;
                            g_CurrentStride = Stride;
                        }

                        auto hr = SetStreamSourceOriginal.unsafe_stdcall<HRESULT>(pDevice, StreamNumber, pStreamData, OffsetInBytes, Stride);
                        return hr;
                    };

                    static HRESULT(WINAPI * CreateDevice)(LPDIRECT3D9, UINT, D3DDEVTYPE, HWND, DWORD, D3DPRESENT_PARAMETERS*, IDirect3DDevice9**) = D3D9CreateDevice;
                    static HRESULT(WINAPI * BeginScene)(LPDIRECT3DDEVICE9) = D3D9BeginScene;
                    static HRESULT(WINAPI * EndScene)(LPDIRECT3DDEVICE9) = D3D9EndScene;
                    static HRESULT(WINAPI * Reset)(LPDIRECT3DDEVICE9, D3DPRESENT_PARAMETERS*) = D3D9Reset;
                    static HRESULT(WINAPI * SetVertexShaderConstantF)(LPDIRECT3DDEVICE9, UINT, float*, UINT) = D3D9SetVertexShaderConstantF;
                    static HRESULT(WINAPI * SetPixelShaderConstantF)(LPDIRECT3DDEVICE9, UINT, float*, UINT) = D3D9SetPixelShaderConstantF;
                    static HRESULT(WINAPI * CreateTexture)(LPDIRECT3DDEVICE9, UINT, UINT, UINT, DWORD, D3DFORMAT, D3DPOOL, IDirect3DTexture9**, HANDLE*) = D3D9CreateTexture;
                    static HRESULT(WINAPI * SetTexture)(LPDIRECT3DDEVICE9, DWORD, IDirect3DBaseTexture9*) = D3D9SetTexture;
                    static HRESULT(WINAPI * DrawPrimitive)(LPDIRECT3DDEVICE9, D3DPRIMITIVETYPE, UINT, UINT) = D3D9DrawPrimitive;
                    static HRESULT(WINAPI * SetPixelShader)(LPDIRECT3DDEVICE9, IDirect3DPixelShader9*) = D3D9SetPixelShader;
                    static HRESULT(WINAPI * SetVertexShader)(LPDIRECT3DDEVICE9, IDirect3DVertexShader9*) = D3D9SetVertexShader;
                    static HRESULT(WINAPI * CreatePixelShader)(LPDIRECT3DDEVICE9, DWORD*, IDirect3DPixelShader9**) = D3D9CreatePixelShader;
                    static HRESULT(WINAPI * CreateVertexShader)(LPDIRECT3DDEVICE9, DWORD*, IDirect3DVertexShader9**) = D3D9CreateVertexShader;
                    static HRESULT(WINAPI * SetStreamSource)(LPDIRECT3DDEVICE9, UINT, IDirect3DVertexBuffer9*, UINT, UINT) = D3D9SetStreamSource;

                    //bind(hD3D9, typeid(IDirect3DDevice9VTBL), IDirect3DDevice9VTBL().GetIndex("BeginScene"), BeginScene, BeginSceneOriginal);
                    //bind(hD3D9, typeid(IDirect3DDevice9VTBL), IDirect3DDevice9VTBL().GetIndex("EndScene"), EndScene, EndSceneOriginal);
                    //bind(hD3D9, typeid(IDirect3DDevice9VTBL), IDirect3DDevice9VTBL().GetIndex("SetVertexShaderConstantF"), SetVertexShaderConstantF, SetVertexShaderConstantFOriginal);
                    //bind(hD3D9, typeid(IDirect3DDevice9VTBL), IDirect3DDevice9VTBL().GetIndex("SetPixelShaderConstantF"), SetPixelShaderConstantF, SetPixelShaderConstantFOriginal);
                    //bind(hD3D9, typeid(IDirect3DDevice9VTBL), IDirect3DDevice9VTBL().GetIndex("CreateTexture"), CreateTexture, CreateTextureOriginal);
                    //bind(hD3D9, typeid(IDirect3DDevice9VTBL), IDirect3DDevice9VTBL().GetIndex("SetTexture"), SetTexture, SetTextureOriginal);
                    //bind(hD3D9, typeid(IDirect3DDevice9VTBL), IDirect3DDevice9VTBL().GetIndex("SetPixelShader"), SetPixelShader, SetPixelShaderOriginal);
                    //bind(hD3D9, typeid(IDirect3DDevice9VTBL), IDirect3DDevice9VTBL().GetIndex("SetVertexShader"), SetVertexShader, SetVertexShaderOriginal);

                    bind(hD3D9, typeid(IDirect3DDevice9VTBL), IDirect3DDevice9VTBL().GetIndex("Reset"), Reset, ResetOriginal);

                    if (bVSync)
                    {
                        bind(hD3D9, typeid(IDirect3D9VTBL), IDirect3D9VTBL().GetIndex("CreateDevice"), CreateDevice, CreateDeviceOriginal);
                    }

                    if (bDisableBlur)
                    {
                        bind(hD3D9, typeid(IDirect3DDevice9VTBL), IDirect3DDevice9VTBL().GetIndex("CreatePixelShader"), CreatePixelShader, CreatePixelShaderOriginal);
                        bind(hD3D9, typeid(IDirect3DDevice9VTBL), IDirect3DDevice9VTBL().GetIndex("CreateVertexShader"), CreateVertexShader, CreateVertexShaderOriginal);
                    }

                    if (bHideUntexturedObjects)
                    {
                        bind(hD3D9, typeid(IDirect3DDevice9VTBL), IDirect3DDevice9VTBL().GetIndex("DrawPrimitive"), DrawPrimitive, DrawPrimitiveOriginal);
                        bind(hD3D9, typeid(IDirect3DDevice9VTBL), IDirect3DDevice9VTBL().GetIndex("SetStreamSource"), SetStreamSource, SetStreamSourceOriginal);
                    }

                    Device->Release();
                }
                Direct3D9->Release();
            }).detach();
        });
    }

public:
    static inline void bind(HMODULE module, std::type_index type_index, uint16_t func_index, void* function, SafetyHookInline& hook)
    {
        auto target = deviceMethods.at(module).at(type_index).at(func_index);
        hook = safetyhook::create_inline(target, function);
    }
    static inline void unbind(SafetyHookInline& hook)
    {
        hook.reset();
    }
} FusionDxHook;