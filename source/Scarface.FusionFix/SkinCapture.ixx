module;
#include "stdafx.h"
#include "RTTI.h"
#include "d3dvtbl.h"
#include <d3d9.h>
#include <d3dx9.h>
#include <wrl/client.h>
#include <fstream>
#include <set>

export module SkinCapture;


namespace
{
    using Microsoft::WRL::ComPtr;
    SafetyHookInline matrixHook;
    std::filesystem::path capturePath;
    std::mutex captureMutex;
    std::set<std::string> captured;
    HMODULE d3dx = nullptr;
    using SaveTexture = HRESULT(WINAPI*)(LPCWSTR, D3DXIMAGE_FILEFORMAT, IDirect3DBaseTexture9*, const PALETTEENTRY*);
    SaveTexture saveTexture = nullptr;
    bool enabled = false;
    uint64_t session = 0;
    uint32_t textureCount = 0;
    constexpr uint32_t captureLimit = 256;
    // Tags belong to the texture, so a freed/reused COM address cannot alias an old capture.
    constexpr GUID textureTag = { 0x6d8b980a, 0x3b71, 0x49cf, { 0x92, 0xbd, 0x4c, 0x87, 0x15, 0xda, 0xe1, 0x40 } };
    struct TextureTag { uint64_t session; uint32_t id; };
    struct Palette
    {
        std::array<float, 32> damage{};
        std::array<bool, 32> valid{};
    };
    thread_local Palette palette;
    thread_local bool capturing = false;

    int __fastcall SetMatrix(void* self, void*, int index, float* matrix)
    {
        // d3dExtHardwareSkinning::SetMatrix (64DB90), vtable slot 5.
        // PolySkin's prepared matrix carries averaged front/back damage in m[15].
        if (index == 0) palette = {};
        if (index >= 0 && index < int(palette.damage.size()) && matrix)
        {
            palette.damage[index] = matrix[15];
            palette.valid[index] = true;
        }
        return matrixHook.thiscall<int>(self, index, matrix);
    }

    template<class Shader> uint32_t Hash(Shader* shader)
    {
        if (!shader) return 0;
        UINT size = 0;
        if (FAILED(shader->GetFunction(nullptr, &size)) || !size || size > 1024 * 1024) return 0;
        std::vector<DWORD> code((size + 3) / 4);
        if (FAILED(shader->GetFunction(code.data(), &size))) return 0;
        return crc32(0, code.data(), size);
    }

    uint32_t CaptureTexture(IDirect3DBaseTexture9* texture)
    {
        if (!texture) return 0;
        TextureTag tag{};
        DWORD size = sizeof(tag);
        if (SUCCEEDED(texture->GetPrivateData(textureTag, &tag, &size)) && size == sizeof(tag) && tag.session == session)
            return tag.id;
        if (textureCount >= captureLimit) return UINT32_MAX;
        tag = { session, ++textureCount };
        auto file = capturePath / ("texture_" + std::to_string(tag.id) + ".dds");
        HRESULT result = saveTexture ? saveTexture(file.c_str(), D3DXIFF_DDS, texture, nullptr) : E_NOINTERFACE;
        std::ofstream log(capturePath / "textures.txt", std::ios::app);
        log << tag.id << " type=" << texture->GetType() << " saveHRESULT=" << std::hex << result;
        if (texture->GetType() == D3DRTYPE_TEXTURE)
        {
            D3DSURFACE_DESC desc{};
            if (SUCCEEDED(static_cast<IDirect3DTexture9*>(texture)->GetLevelDesc(0, &desc)))
                log << std::dec << " width=" << desc.Width << " height=" << desc.Height
                    << " format=" << std::hex << desc.Format << " pool=" << desc.Pool;
        }
        log << '\n';
        texture->SetPrivateData(textureTag, &tag, sizeof(tag), 0);
        return tag.id;
    }

    void Capture(IDirect3DDevice9* device)
    {
        if (!enabled || capturing) return; // A texture-export helper may itself use the device.
        struct CaptureScope
        {
            CaptureScope() { capturing = true; }
            ~CaptureScope() { capturing = false; }
        } scope;
        try
        {
            ComPtr<IDirect3DVertexShader9> vertex;
            if (FAILED(device->GetVertexShader(vertex.GetAddressOf()))) return;
            const auto vs = Hash(vertex.Get());
            // Verified PC skin and skin_onebone bytecode. Ignore scenery, HUD and post effects.
            if (vs != 0xD41E262E && vs != 0xDF075E41) return;
            std::lock_guard lock(captureMutex);
            if (captured.size() >= captureLimit) return;
            ComPtr<IDirect3DPixelShader9> pixel;
            device->GetPixelShader(pixel.GetAddressOf());
            const auto ps = Hash(pixel.Get());
            std::array<uint32_t, 4> textures{};
            for (DWORD stage = 0; stage < textures.size(); ++stage)
            {
                ComPtr<IDirect3DBaseTexture9> texture;
                if (SUCCEEDED(device->GetTexture(stage, texture.GetAddressOf())))
                    textures[stage] = CaptureTexture(texture.Get());
            }
            bool damaged = false;
            for (size_t i = 0; i < palette.damage.size(); ++i)
                damaged |= palette.valid[i] && std::isfinite(palette.damage[i]) && palette.damage[i] > 0.0f;
            DWORD alphaTest = 0, alphaRef = 0, alphaFunc = 0, blend = 0, src = 0, dst = 0;
            device->GetRenderState(D3DRS_ALPHATESTENABLE, &alphaTest);
            device->GetRenderState(D3DRS_ALPHAREF, &alphaRef);
            device->GetRenderState(D3DRS_ALPHAFUNC, &alphaFunc);
            device->GetRenderState(D3DRS_ALPHABLENDENABLE, &blend);
            device->GetRenderState(D3DRS_SRCBLEND, &src);
            device->GetRenderState(D3DRS_DESTBLEND, &dst);
            std::ostringstream key;
            key << std::hex << vs << '_' << ps;
            for (auto id : textures) key << '_' << id;
            key << '_' << damaged << '_' << alphaTest << '_' << alphaRef << '_' << alphaFunc << '_' << blend << '_' << src << '_' << dst;
            if (captured.contains(key.str())) return;
            auto base = capturePath / ("draw_" + std::to_string(captured.size()));
            std::ofstream info(base.wstring() + L".txt");
            info << "VS=" << std::hex << vs << " PS=" << ps << std::dec << '\n';
            for (size_t stage = 0; stage < textures.size(); ++stage)
                info << "texture[" << stage << "]=" << textures[stage] << '\n';
            info << "alphaTest=" << alphaTest << " alphaRef=" << alphaRef << " alphaFunc=" << alphaFunc
                 << " blend=" << blend << " src=" << src << " dst=" << dst << '\n';
            info << "matrixHook=" << bool(matrixHook) << " damagePresent=" << damaged << '\n';
            for (size_t i = 0; i < palette.damage.size(); ++i)
                if (palette.valid[i]) info << "bone[" << i << "].damage=" << palette.damage[i] << '\n';
            std::array<float, 256 * 4> constants{};
            auto result = device->GetVertexShaderConstantF(0, constants.data(), 256);
            info << "VS constants HRESULT=" << std::hex << result << '\n';
            if (SUCCEEDED(result))
                std::ofstream(base.wstring() + L".vsconstants.bin", std::ios::binary).write(reinterpret_cast<const char*>(constants.data()), sizeof(constants));
            result = device->GetPixelShaderConstantF(0, constants.data(), 8);
            info << "PS constants HRESULT=" << std::hex << result << '\n';
            if (SUCCEEDED(result))
                std::ofstream(base.wstring() + L".psconstants.bin", std::ios::binary).write(reinterpret_cast<const char*>(constants.data()), 8 * 4 * sizeof(float));
            captured.insert(key.str());
        }
        catch (...)
        {
            OutputDebugStringA("Scarface SkinCapture: capture failed.\n");
        }
    }


}

export namespace SkinCapture
{
    void Initialize(bool requested, const std::filesystem::path& path)
    {
        enabled = requested;
        if (!enabled) return;
        session = (uint64_t(GetCurrentProcessId()) << 32) ^ GetTickCount64();
        capturePath = path / ("Materials_" + std::to_string(session));
        std::error_code error;
        std::filesystem::create_directories(capturePath, error);
        if (error) { enabled = false; return; }
        d3dx = LoadLibraryW(L"d3dx9_43.dll");
        if (d3dx) saveTexture = reinterpret_cast<SaveTexture>(GetProcAddress(d3dx, "D3DXSaveTextureToFileW"));
        const auto vtable = ScarfaceRTTI::FindVtable(".?AVd3dExtHardwareSkinning@pure3d@@");
        if (vtable)
            matrixHook = safetyhook::create_inline(injector::ReadMemory<void*>(vtable + 5 * sizeof(void*), true), SetMatrix);
        std::ofstream info(capturePath / "status.txt");
        info << "Character-only material capture; limit=" << captureLimit << " draws/textures.\n"
             << "Texture exporter=" << bool(saveTexture) << " matrix hook=" << bool(matrixHook) << '\n'
             << "One snapshot per shader/texture/blend combination and zero/nonzero joint damage.\n"
             << "Texture ID 0=unbound, 4294967295=capture limit. DDS preserves alpha/mipmaps.\n";
    }

    bool IsEnabled() { return enabled; }
    bool IsCapturing() { return capturing; }
    void CaptureDraw(IDirect3DDevice9* device) { Capture(device); }

    void ReportDrawHooks(bool draw, bool indexed, bool up, bool indexedUp)
    {
        if (enabled)
        {
            std::ofstream info(capturePath / "status.txt", std::ios::app);
            info << "Draw hooks=" << draw << ',' << indexed << ',' << up << ',' << indexedUp << '\n';
        }
    }

    void Shutdown()
    {
        matrixHook.reset();
        if (d3dx) FreeLibrary(d3dx);
        d3dx = nullptr; saveTexture = nullptr; enabled = false;
        captured.clear(); textureCount = 0; palette = {};
    }
}
