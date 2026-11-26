module;

#include <stdafx.h>
#include <minidx8/d3d8.h>

export module WaterD3D8;

import WaterCommon;

// The Direct3D 8 side of the water: the state the engine leaves on its device, the reflection target, and the
// water drawn with vertex shader 1.1 and pixel shader 1.4
namespace WaterD3D8
{
    IDirect3DTexture8* reflection = nullptr;
    IDirect3DSurface8* reflectionSurface = nullptr;
    IDirect3DSurface8* reflectionDepth = nullptr;
    IDirect3DSurface8* savedTarget = nullptr;
    IDirect3DSurface8* savedDepth = nullptr;
    IDirect3DTexture8* waves = nullptr;
    IDirect3DCubeTexture8* normalizer = nullptr;
    DWORD pixelShader = 0;
    std::vector<std::pair<WaterLayout, DWORD>> vertexShaders;
    int supported = -1;
    bool softwareShaders = false;
    bool mixed = false;

    IDirect3DDevice8* Device(void* device)
    {
        return static_cast<IDirect3DDevice8*>(device);
    }

    std::span<const DWORD> Resource(int id)
    {
        HMODULE module = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&Resource), &module);
        auto info = FindResourceW(module, MAKEINTRESOURCEW(id), RT_RCDATA);
        auto data = static_cast<const DWORD*>(LockResource(LoadResource(module, info)));
        return { data, SizeofResource(module, info) / sizeof(DWORD) };
    }

    // The shaders are assembled with the Direct3D 9 tools; Direct3D 8 takes 1.x code without the declarations
    std::vector<DWORD> WithoutDeclarations(std::span<const DWORD> code)
    {
        std::vector<DWORD> result{ code[0] };
        for (size_t i = 1; i < code.size();)
        {
            const DWORD token = code[i];
            if (token == 0x0000FFFF)
            {
                result.push_back(token);
                break;
            }
            if ((token & 0xFFFF) == 0xFFFE)
            {
                i += 1 + ((token >> 16) & 0x7FFF);
                continue;
            }
            size_t end = i + 1;
            while (end < code.size() && (code[end] & 0x80000000u))
                ++end;
            if ((token & 0xFFFF) != 31)
                result.insert(result.end(), code.begin() + i, code.begin() + end);
            i = end;
        }
        return result;
    }

    export bool Supported(void* device)
    {
        if (supported < 0)
        {
            D3DCAPS8 caps{};
            D3DDEVICE_CREATION_PARAMETERS creation{};
            Device(device)->GetDeviceCaps(&caps);
            Device(device)->GetCreationParameters(&creation);
            softwareShaders = (creation.BehaviorFlags & D3DCREATE_SOFTWARE_VERTEXPROCESSING) != 0;
            mixed = (creation.BehaviorFlags & D3DCREATE_MIXED_VERTEXPROCESSING) != 0;
            const bool vertex = softwareShaders || mixed || caps.VertexShaderVersion >= D3DVS_VERSION(1, 1);
            supported = vertex && caps.PixelShaderVersion >= D3DPS_VERSION(1, 4) && caps.MaxSimultaneousTextures >= 6
                && (caps.TextureCaps & D3DPTEXTURECAPS_CUBEMAP);
        }
        return supported > 0;
    }

    export void Transforms(void* device, float world[16], float view[16], float projection[16])
    {
        Device(device)->GetTransform(D3DTS_WORLD, reinterpret_cast<D3DMATRIX*>(world));
        Device(device)->GetTransform(D3DTS_VIEW, reinterpret_cast<D3DMATRIX*>(view));
        Device(device)->GetTransform(D3DTS_PROJECTION, reinterpret_cast<D3DMATRIX*>(projection));
    }

    // The vertex layout of the current draw, from its FVF or its declaration. The engine's own declarations of
    // position, color and one or two texture coordinates are given, d3d8to9 does not return declarations.
    export bool Layout(void* device, WaterLayout& layout, uint64_t& identity, DWORD textured, DWORD textured2)
    {
        IDirect3DVertexBuffer8* buffer = nullptr;
        UINT stride = 0;
        if (FAILED(Device(device)->GetStreamSource(0, &buffer, &stride)) || !buffer)
            return false;
        // Private data belongs to this resource even when a later allocation reuses its address.
        static constexpr GUID meshId{ 0x8647dd94, 0x7722, 0x47fb, { 0xaf, 0x79, 0xf1, 0xe9, 0x75, 0x54, 0x69, 0xb3 } };
        static uint64_t nextId = 0;
        D3DVERTEXBUFFER_DESC desc{};
        identity = 0;
        if (SUCCEEDED(buffer->GetDesc(&desc)) && !(desc.Usage & D3DUSAGE_DYNAMIC))
        {
            DWORD size = sizeof(identity);
            if (FAILED(buffer->GetPrivateData(meshId, &identity, &size)))
            {
                identity = ++nextId;
                if (FAILED(buffer->SetPrivateData(meshId, &identity, sizeof(identity), 0)))
                    identity = 0;
            }
        }
        buffer->Release();
        layout = { stride, -1, -1 };
        DWORD shader = 0;
        Device(device)->GetVertexShader(&shader);
        // Shader handles have the low bit set (d3d8to9 sets the top bit), which no FVF has
        if (!(shader & 0x80000001u))
        {
            if ((shader & D3DFVF_POSITION_MASK) != D3DFVF_XYZ)
                return false;
            int32_t offset = 12;
            if (shader & D3DFVF_NORMAL)
                offset += 12;
            if (shader & D3DFVF_PSIZE)
                offset += 4;
            if (shader & D3DFVF_DIFFUSE)
            {
                layout.color = offset;
                offset += 4;
            }
            if (shader & D3DFVF_SPECULAR)
                offset += 4;
            if (((shader & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT) && ((shader >> 16) & 3) == D3DFVF_TEXTUREFORMAT2)
                layout.uv = offset;
        }
        else if (shader == textured || shader == textured2)
        {
            layout.color = 12;
            layout.uv = 16;
        }
        else
        {
            std::array<DWORD, 64> tokens{};
            DWORD size = sizeof(tokens);
            if (FAILED(Device(device)->GetVertexShaderDeclaration(shader, tokens.data(), &size)))
                return false;
            int32_t offset = 0;
            bool stream0 = false;
            for (DWORD i = 0; i < size / sizeof(DWORD) && tokens[i] != D3DVSD_END(); ++i)
            {
                const DWORD token = tokens[i];
                const DWORD type = (token & D3DVSD_TOKENTYPEMASK) >> D3DVSD_TOKENTYPESHIFT;
                if (type == D3DVSD_TOKEN_STREAM)
                {
                    stream0 = (token & D3DVSD_STREAMNUMBERMASK) == 0;
                    offset = 0;
                }
                else if (type == D3DVSD_TOKEN_STREAMDATA && stream0)
                {
                    if (token & D3DVSD_DATALOADTYPEMASK)
                    {
                        offset += 4 * ((token & D3DVSD_SKIPCOUNTMASK) >> D3DVSD_SKIPCOUNTSHIFT);
                        continue;
                    }
                    const DWORD data = (token & D3DVSD_DATATYPEMASK) >> D3DVSD_DATATYPESHIFT;
                    const DWORD reg = token & D3DVSD_VERTEXREGMASK;
                    static constexpr int32_t sizes[] = { 4, 8, 12, 16, 4, 4, 4, 8 };
                    if (reg == D3DVSDE_POSITION && (offset != 0 || data != D3DVSDT_FLOAT3))
                        return false;
                    if (reg == D3DVSDE_DIFFUSE && data == D3DVSDT_D3DCOLOR)
                        layout.color = offset;
                    if (reg == D3DVSDE_TEXCOORD0 && data == D3DVSDT_FLOAT2)
                        layout.uv = offset;
                    offset += data < 8 ? sizes[data] : 0;
                }
            }
        }
        return layout.color >= 0 && layout.uv >= 0;
    }

    // The positions of the current draw's vertices in their object space, every one up to 4096 and evenly spread
    // beyond that
    export bool VertexBase(void* device, UINT& base)
    {
        IDirect3DIndexBuffer8* indices = nullptr;
        const auto result = Device(device)->GetIndices(&indices, &base);
        if (indices) indices->Release();
        return SUCCEEDED(result);
    }

    export bool Positions(void* device, UINT first, UINT count, std::vector<float>& positions)
    {
        IDirect3DVertexBuffer8* buffer = nullptr;
        UINT stride = 0;
        if (FAILED(Device(device)->GetStreamSource(0, &buffer, &stride)) || !buffer)
            return false;
        if (stride < 3 * sizeof(float) || !count || uint64_t(first) + count > UINT_MAX / stride)
        {
            buffer->Release();
            return false;
        }
        BYTE* data = nullptr;
        bool done = false;
        if (SUCCEEDED(buffer->Lock(first * stride, count * stride, &data, D3DLOCK_READONLY)) && data)
        {
            const UINT step = std::max(1u, count / 4096);
            positions.clear();
            for (UINT i = 0; i < count; i += step)
            {
                const auto* p = reinterpret_cast<const float*>(data + i * stride);
                positions.insert(positions.end(), { p[0], p[1], p[2] });
            }
            buffer->Unlock();
            done = std::all_of(positions.begin(), positions.end(), [](float value) { return std::isfinite(value); });
        }
        buffer->Release();
        return done;
    }

    // The first enabled directional light: direction to it and its color
    export bool Sun(void* device, float direction[3], float color[3])
    {
        for (DWORD i = 0; i < 8; ++i)
        {
            BOOL enabled = FALSE;
            D3DLIGHT8 light{};
            if (FAILED(Device(device)->GetLightEnable(i, &enabled)) || !enabled || FAILED(Device(device)->GetLight(i, &light)))
                continue;
            if (light.Type != D3DLIGHT_DIRECTIONAL)
                continue;
            const float length = std::sqrt(light.Direction.x * light.Direction.x + light.Direction.y * light.Direction.y + light.Direction.z * light.Direction.z);
            if (length < 0.0001f)
                continue;
            direction[0] = -light.Direction.x / length;
            direction[1] = -light.Direction.y / length;
            direction[2] = -light.Direction.z / length;
            color[0] = light.Diffuse.r;
            color[1] = light.Diffuse.g;
            color[2] = light.Diffuse.b;
            return true;
        }
        return false;
    }

    // How the engine combines the water texture with the vertex color: 1, 2 or 4 times their product
    export float Modulation(void* device)
    {
        DWORD operation = D3DTOP_MODULATE;
        Device(device)->GetTextureStageState(0, D3DTSS_COLOROP, &operation);
        return operation == D3DTOP_MODULATE4X ? 4.0f : (operation == D3DTOP_MODULATE2X ? 2.0f : 1.0f);
    }

    export void Fog(void* device, float& start, float& end)
    {
        DWORD value = 0;
        Device(device)->GetRenderState(D3DRS_FOGSTART, &value);
        start = std::bit_cast<float>(value);
        Device(device)->GetRenderState(D3DRS_FOGEND, &value);
        end = std::bit_cast<float>(value);
    }

    // The device is gone with everything made on it
    export void Release()
    {
        ReleaseWaterObject(savedTarget);
        ReleaseWaterObject(savedDepth);
        ReleaseWaterObject(reflectionSurface);
        ReleaseWaterObject(reflectionDepth);
        ReleaseWaterObject(reflection);
    }

    export void ReleaseDevice()
    {
        Release();
        // Shaders are created only after the managed wave texture, which still owns the native device.
        IDirect3DDevice8* owner = nullptr;
        if (waves && SUCCEEDED(waves->GetDevice(&owner)))
        {
            if (pixelShader)
                owner->DeletePixelShader(pixelShader);
            for (const auto& entry : vertexShaders)
                owner->DeleteVertexShader(entry.second);
            owner->Release();
        }
        pixelShader = 0;
        vertexShaders.clear();
        ReleaseWaterObject(waves);
        ReleaseWaterObject(normalizer);
        supported = -1;
    }

    export bool CurrentViewport(void* device, void* viewport)
    {
        return SUCCEEDED(Device(device)->GetViewport(static_cast<D3DVIEWPORT8*>(viewport)));
    }

    bool Target(IDirect3DDevice8* device, UINT width, UINT height)
    {
        if (reflection)
        {
            D3DSURFACE_DESC description{};
            reflectionSurface->GetDesc(&description);
            if (description.Width == width && description.Height == height)
                return true;
            Release();
        }
        IDirect3DSurface8* depth = nullptr;
        D3DFORMAT depthFormat = D3DFMT_D24S8;
        if (SUCCEEDED(device->GetDepthStencilSurface(&depth)) && depth)
        {
            D3DSURFACE_DESC description{};
            depth->GetDesc(&description);
            depthFormat = description.Format;
            depth->Release();
        }
        if (FAILED(device->CreateTexture(width, height, 1, D3DUSAGE_RENDERTARGET, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &reflection)))
        {
            reflection = nullptr;
            return false;
        }
        if (FAILED(reflection->GetSurfaceLevel(0, &reflectionSurface)))
        {
            Release();
            return false;
        }
        if (FAILED(device->CreateDepthStencilSurface(width, height, depthFormat, D3DMULTISAMPLE_NONE, &reflectionDepth)))
        {
            reflectionDepth = nullptr;
            Release();
            return false;
        }
        return true;
    }

    // Renders the following draws into the reflection target, cleared to the given color
    export bool BeginReflection(void* device, UINT width, UINT height, DWORD clear, const void* viewport)
    {
        auto d = Device(device);
        if (!Target(d, width, height))
            return false;
        if (FAILED(d->GetRenderTarget(&savedTarget)))
            return false;
        const auto depthResult = d->GetDepthStencilSurface(&savedDepth);
        if (FAILED(depthResult) && depthResult != D3DERR_NOTFOUND)
        {
            ReleaseWaterObject(savedTarget);
            return false;
        }
        D3DSURFACE_DESC description{};
        reflectionDepth->GetDesc(&description);
        const bool stencil = description.Format == D3DFMT_D24S8 || description.Format == D3DFMT_D15S1 || description.Format == D3DFMT_D24X4S4;
        if (FAILED(d->SetRenderTarget(reflectionSurface, reflectionDepth)) ||
            FAILED(d->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | (stencil ? D3DCLEAR_STENCIL : 0), clear, 1.0f, 0)))
        {
            d->SetRenderTarget(savedTarget, savedDepth);
            d->SetViewport(static_cast<const D3DVIEWPORT8*>(viewport));
            ReleaseWaterObject(savedDepth);
            ReleaseWaterObject(savedTarget);
            return false;
        }
        return true;
    }

    export void EndReflection(void* device, const void* viewport)
    {
        auto d = Device(device);
        d->SetRenderTarget(savedTarget, savedDepth);
        d->SetViewport(static_cast<const D3DVIEWPORT8*>(viewport));
        for (auto** surface : { &savedTarget, &savedDepth })
        {
            if (*surface)
                (*surface)->Release();
            *surface = nullptr;
        }
    }

    bool Waves(IDirect3DDevice8* device)
    {
        if (waves)
            return true;
        IDirect3DTexture8* texture = nullptr;
        if (FAILED(device->CreateTexture(WaveNormalSize, WaveNormalSize, 0, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &texture)))
            return false;
        for (UINT level = 0; level < texture->GetLevelCount(); ++level)
        {
            const auto texels = WaveNormals(WaveNormalSize, level);
            const UINT size = std::max(1u, WaveNormalSize >> level);
            D3DLOCKED_RECT locked{};
            if (FAILED(texture->LockRect(level, &locked, nullptr, 0)))
            {
                texture->Release();
                return false;
            }
            for (UINT y = 0; y < size; ++y)
                memcpy(static_cast<BYTE*>(locked.pBits) + y * locked.Pitch, &texels[y * size], size * sizeof(uint32_t));
            if (FAILED(texture->UnlockRect(level)))
            {
                texture->Release();
                return false;
            }
        }
        waves = texture;
        return true;
    }

    // A cube map whose texels are their own direction, to normalize vectors in pixel shader 1.4
    bool Normalizer(IDirect3DDevice8* device)
    {
        if (normalizer)
            return true;
        constexpr UINT size = 64;
        IDirect3DCubeTexture8* texture = nullptr;
        if (FAILED(device->CreateCubeTexture(size, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &texture)))
            return false;
        for (UINT face = 0; face < 6; ++face)
        {
            D3DLOCKED_RECT locked{};
            if (FAILED(texture->LockRect(static_cast<D3DCUBEMAP_FACES>(face), 0, &locked, nullptr, 0)))
            {
                texture->Release();
                return false;
            }
            for (UINT y = 0; y < size; ++y)
            {
                auto* row = reinterpret_cast<uint32_t*>(static_cast<BYTE*>(locked.pBits) + y * locked.Pitch);
                for (UINT x = 0; x < size; ++x)
                {
                    const float s = (x + 0.5f) / size * 2.0f - 1.0f, t = (y + 0.5f) / size * 2.0f - 1.0f;
                    float v[3];
                    switch (face)
                    {
                        case D3DCUBEMAP_FACE_POSITIVE_X: v[0] = 1.0f; v[1] = -t; v[2] = -s; break;
                        case D3DCUBEMAP_FACE_NEGATIVE_X: v[0] = -1.0f; v[1] = -t; v[2] = s; break;
                        case D3DCUBEMAP_FACE_POSITIVE_Y: v[0] = s; v[1] = 1.0f; v[2] = t; break;
                        case D3DCUBEMAP_FACE_NEGATIVE_Y: v[0] = s; v[1] = -1.0f; v[2] = -t; break;
                        case D3DCUBEMAP_FACE_POSITIVE_Z: v[0] = s; v[1] = -t; v[2] = 1.0f; break;
                        default: v[0] = -s; v[1] = -t; v[2] = -1.0f; break;
                    }
                    const float length = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
                    const auto encode = [&](float value)
                    {
                        return static_cast<uint32_t>(std::clamp(std::lround(value / length * 127.5f + 127.5f), 0l, 255l));
                    };
                    row[x] = 0xFF000000u | (encode(v[0]) << 16) | (encode(v[1]) << 8) | encode(v[2]);
                }
            }
            if (FAILED(texture->UnlockRect(static_cast<D3DCUBEMAP_FACES>(face), 0)))
            {
                texture->Release();
                return false;
            }
        }
        normalizer = texture;
        return true;
    }

    DWORD VertexShader(IDirect3DDevice8* device, const WaterLayout& layout)
    {
        for (const auto& [key, handle] : vertexShaders)
            if (key == layout)
                return handle;
        struct Element
        {
            int32_t offset;
            DWORD reg, type, size;
        };
        std::array<Element, 3> elements{ { { 0, 0, D3DVSDT_FLOAT3, 12 }, { layout.color, 1, D3DVSDT_D3DCOLOR, 4 }, { layout.uv, 2, D3DVSDT_FLOAT2, 8 } } };
        std::sort(elements.begin(), elements.end(), [](const auto& a, const auto& b) { return a.offset < b.offset; });
        std::vector<DWORD> declaration{ D3DVSD_STREAM(0) };
        int32_t offset = 0;
        for (const auto& element : elements)
        {
            if (element.offset < offset)
                return 0;
            for (int32_t skip = (element.offset - offset) / 4; skip > 0; skip -= std::min(skip, 15))
                declaration.push_back(D3DVSD_SKIP(std::min(skip, 15)));
            declaration.push_back(D3DVSD_REG(element.reg, element.type));
            offset = element.offset + element.size;
        }
        for (int32_t skip = (static_cast<int32_t>(layout.stride) - offset) / 4; skip > 0; skip -= std::min(skip, 15))
            declaration.push_back(D3DVSD_SKIP(std::min(skip, 15)));
        declaration.push_back(D3DVSD_END());
        const auto code = WithoutDeclarations(Resource(IDR_WATER_VS8));
        DWORD handle = 0;
        if (FAILED(device->CreateVertexShader(declaration.data(), code.data(), &handle, softwareShaders ? D3DUSAGE_SOFTWAREPROCESSING : 0)))
            return 0;
        vertexShaders.emplace_back(layout, handle);
        return handle;
    }

    // Draws the current water draw with the shaders; draw issues the engine's own draw call
    export bool Draw(void* device, const WaterDraw& water, const WaterLayout& layout, const std::function<HRESULT()>& draw)
    {
        auto d = Device(device);
        if (!reflection || !Waves(d) || !Normalizer(d))
            return false;
        if (!pixelShader)
        {
            const auto code = Resource(IDR_WATER_PS8);
            if (FAILED(d->CreatePixelShader(code.data(), &pixelShader)))
                pixelShader = 0;
        }
        const DWORD vertexShader = VertexShader(d, layout);
        if (!pixelShader || !vertexShader)
            return false;

        constexpr D3DTEXTURESTAGESTATETYPE states[] = { D3DTSS_ADDRESSU, D3DTSS_ADDRESSV, D3DTSS_MAGFILTER, D3DTSS_MINFILTER,
            D3DTSS_MIPFILTER, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTSS_TEXCOORDINDEX };
        std::array<std::array<DWORD, std::size(states)>, 6> savedStates{};
        std::array<IDirect3DBaseTexture8*, 6> savedTextures{};
        for (DWORD stage = 0; stage < 6; ++stage)
        {
            d->GetTexture(stage, &savedTextures[stage]);
            for (size_t i = 0; i < std::size(states); ++i)
                d->GetTextureStageState(stage, states[i], &savedStates[stage][i]);
        }
        DWORD savedVertexShader = 0, savedPixelShader = 0, savedSoftware = FALSE;
        d->GetVertexShader(&savedVertexShader);
        d->GetPixelShader(&savedPixelShader);
        if (mixed)
        {
            d->GetRenderState(D3DRS_SOFTWAREVERTEXPROCESSING, &savedSoftware);
            d->SetRenderState(D3DRS_SOFTWAREVERTEXPROCESSING, FALSE);
        }

        d->SetTexture(3, savedTextures[0]);
        d->SetTexture(0, reflection);
        d->SetTexture(1, waves);
        d->SetTexture(2, waves);
        d->SetTexture(4, normalizer);
        d->SetTexture(5, normalizer);
        for (DWORD stage = 0; stage < 6; ++stage)
        {
            const bool wrap = stage != 0 && stage < 4;
            d->SetTextureStageState(stage, D3DTSS_ADDRESSU, wrap ? (stage == 3 ? savedStates[0][0] : D3DTADDRESS_WRAP) : D3DTADDRESS_CLAMP);
            d->SetTextureStageState(stage, D3DTSS_ADDRESSV, wrap ? (stage == 3 ? savedStates[0][1] : D3DTADDRESS_WRAP) : D3DTADDRESS_CLAMP);
            d->SetTextureStageState(stage, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
            d->SetTextureStageState(stage, D3DTSS_MINFILTER, D3DTEXF_LINEAR);
            d->SetTextureStageState(stage, D3DTSS_MIPFILTER, stage == 0 || stage > 3 ? D3DTEXF_NONE : D3DTEXF_LINEAR);
            d->SetTextureStageState(stage, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
            d->SetTextureStageState(stage, D3DTSS_TEXCOORDINDEX, stage);
        }

        // vs: c0-c3 clip, c4-c6 world, c7 camera, c8-c11 reflection, c12-c15 waves, c16 numbers, c17 fog, c18 sun
        d->SetVertexShaderConstant(0, water.worldViewProj, 4);
        d->SetVertexShaderConstant(4, water.world, 3);
        d->SetVertexShaderConstant(7, water.camera, 1);
        d->SetVertexShaderConstant(8, water.reflection, 4);
        d->SetVertexShaderConstant(12, water.waves, 4);
        const float numbers[4] = { 0.0f, 1.0f, 0.01f, 2.0f };
        d->SetVertexShaderConstant(16, numbers, 1);
        d->SetVertexShaderConstant(17, water.fog, 1);
        d->SetVertexShaderConstant(18, water.sun, 1);

        // ps: c0 distortion, c1 tint, c2 fresnel scale, c3 sun highlight, c4 fresnel bias
        const float distortion[4] = { water.params[0], water.params[0], 0.0f, 0.0f };
        const float scale[4] = { water.tint[3], water.tint[3], water.tint[3], 0.0f };
        const float highlight[4] = { water.sunColor[0] * water.sun[3], water.sunColor[1] * water.sun[3], water.sunColor[2] * water.sun[3], 0.0f };
        const float bias[4] = { water.params[1], water.params[1], water.params[1], 0.0f };
        d->SetPixelShaderConstant(0, distortion, 1);
        const float tint[4] = { water.tint[0] * 0.25f, water.tint[1] * 0.25f, water.tint[2] * 0.25f, water.tint[3] };
        d->SetPixelShaderConstant(1, tint, 1);
        d->SetPixelShaderConstant(2, scale, 1);
        d->SetPixelShaderConstant(3, highlight, 1);
        d->SetPixelShaderConstant(4, bias, 1);

        d->SetVertexShader(vertexShader);
        d->SetPixelShader(pixelShader);
        const HRESULT result = draw();

        d->SetVertexShader(savedVertexShader);
        d->SetPixelShader(savedPixelShader);
        if (mixed)
            d->SetRenderState(D3DRS_SOFTWAREVERTEXPROCESSING, savedSoftware);
        for (DWORD stage = 0; stage < 6; ++stage)
        {
            d->SetTexture(stage, savedTextures[stage]);
            if (savedTextures[stage])
                savedTextures[stage]->Release();
            for (size_t i = 0; i < std::size(states); ++i)
                d->SetTextureStageState(stage, states[i], savedStates[stage][i]);
        }
        return SUCCEEDED(result);
    }
}
