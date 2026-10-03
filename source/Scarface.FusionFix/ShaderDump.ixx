module;

#include "stdafx.h"
#include <d3d9.h>
#include <d3dcompiler.h>
#include "d3dvtbl.h"
#include <fstream>
#include <unordered_set>

export module ShaderDump;
import SkinCapture;

namespace
{
    SafetyHookInline vertexHook, pixelHook;
    std::filesystem::path dumpPath;
    std::unordered_set<uint64_t> dumped;
    std::mutex dumpMutex;
    HMODULE compiler = nullptr;
    using Disassemble = HRESULT(WINAPI*)(LPCVOID, SIZE_T, UINT, LPCSTR, ID3DBlob**);
    Disassemble disassemble = nullptr;
    bool enabled = false;

    template<class Shader>
    void Dump(Shader* shader, bool vertex)
    {
        if (!shader) return; // Fixed-function rendering has no shader bytecode.
        try
        {
            // GetFunction supplies the real size, including comments and END.
            UINT size = 0;
            if (FAILED(shader->GetFunction(nullptr, &size)) || !size || size > 1024 * 1024) return;
            std::vector<DWORD> code((size + sizeof(DWORD) - 1) / sizeof(DWORD));
            if (FAILED(shader->GetFunction(code.data(), &size))) return;
            const auto hash = crc32(0, code.data(), size);
            const uint64_t key = (uint64_t(vertex) << 32) | hash;
            std::lock_guard lock(dumpMutex);
            if (dumped.contains(key)) return;

            std::ostringstream name;
            name << std::hex << std::uppercase << std::setfill('0') << std::setw(8) << hash;
            const auto base = dumpPath / name.str();
            std::ofstream binary(base.wstring() + (vertex ? L".vso" : L".pso"), std::ios::binary);
            binary.write(reinterpret_cast<const char*>(code.data()), size);
            binary.close();
            if (!binary) return;

            ID3DBlob* assembly = nullptr;
            if (disassemble && SUCCEEDED(disassemble(code.data(), size, 0, nullptr, &assembly)) && assembly)
            {
                std::ofstream text(base.wstring() + (vertex ? L".vs" : L".ps"), std::ios::binary);
                text.write(static_cast<const char*>(assembly->GetBufferPointer()), assembly->GetBufferSize());
            }
            if (assembly) assembly->Release();
            dumped.insert(key);
        }
        catch (...)
        {
            // Optional diagnostics must not interrupt the game's renderer.
            OutputDebugStringA("Scarface ShaderDump: failed to write shader.\n");
        }
    }

    HRESULT WINAPI SetVertexShader(IDirect3DDevice9* device, IDirect3DVertexShader9* shader)
    {
        const auto result = vertexHook.stdcall<HRESULT>(device, shader);
        if (SUCCEEDED(result)) Dump(shader, true);
        return result;
    }

    HRESULT WINAPI SetPixelShader(IDirect3DDevice9* device, IDirect3DPixelShader9* shader)
    {
        const auto result = pixelHook.stdcall<HRESULT>(device, shader);
        if (SUCCEEDED(result)) Dump(shader, false);
        return result;
    }
}

export namespace ShaderDump
{
    bool Initialize()
    {
        CIniReader iniReader("");
        const auto mode = std::clamp(iniReader.ReadInteger("DEBUG", "DumpShaders", 0), 0, 2);
        enabled = mode != 0;
        if (!enabled) return false;
        dumpPath = GetExeModulePath() / "ShaderDumps" / "Scarface";
        std::error_code error;
        std::filesystem::create_directories(dumpPath, error);
        if (error)
        {
            OutputDebugStringA("Scarface ShaderDump: cannot create output directory.\n");
            return enabled = false;
        }
        SkinCapture::Initialize(mode == 2, dumpPath);
        compiler = LoadLibraryW(L"d3dcompiler_47.dll");
        if (compiler) disassemble = reinterpret_cast<Disassemble>(GetProcAddress(compiler, "D3DDisassemble"));
        std::ofstream info(dumpPath / "capture.txt");
        info << "Scarface Fusion Fix shader capture\n"
             << "Shaders are captured on successful D3D9 binds; menus and other scenes are included.\n"
             << "CRC32 names: .vso/.pso = bytecode, .vs/.ps = disassembly.\n"
             << "Existing files are refreshed on first use each run. Disable DEBUG/DumpShaders after capture.\n"
             << "Disassembler: " << (disassemble ? "available" : "unavailable; binary dumps still work") << '\n';
        return true;
    }

    void ObserveDevice(IDirect3DDevice9* device)
    {
        if (!enabled || !device || (vertexHook && pixelHook)) return;
        auto vtable = *reinterpret_cast<void***>(device);
        if (!vertexHook)
            vertexHook = safetyhook::create_inline(vtable[IDirect3DDevice9VTBL::SetVertexShader], SetVertexShader);
        if (!pixelHook)
            pixelHook = safetyhook::create_inline(vtable[IDirect3DDevice9VTBL::SetPixelShader], SetPixelShader);
        // Include shaders already bound when the game's device first becomes available.
        IDirect3DVertexShader9* vertex = nullptr;
        if (SUCCEEDED(device->GetVertexShader(&vertex)) && vertex)
        {
            Dump(vertex, true);
            vertex->Release();
        }
        IDirect3DPixelShader9* pixel = nullptr;
        if (SUCCEEDED(device->GetPixelShader(&pixel)) && pixel)
        {
            Dump(pixel, false);
            pixel->Release();
        }
        std::ofstream status(dumpPath / "capture.txt", std::ios::app);
        status << "Bind hooks: vertex=" << bool(vertexHook) << ", pixel=" << bool(pixelHook) << '\n';
    }

    void Shutdown()
    {
        SkinCapture::Shutdown();
        vertexHook.reset();
        pixelHook.reset();
        if (compiler) FreeLibrary(compiler);
        compiler = nullptr;
        disassemble = nullptr;
        enabled = false;
        dumped.clear();
    }
}
