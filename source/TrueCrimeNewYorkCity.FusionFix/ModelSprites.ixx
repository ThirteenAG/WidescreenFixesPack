module;

#include <stdafx.h>
#include "minidx8/d3d8.h"

export module ModelSprites;

import ComVars;


// Some city models carry a set of camera-facing sprites (model chunk 11, kept at model+0x1C by the loader). The
// console draws them in their own render pass (priority 19.5); the PC port loads the data but its pass is empty.
namespace ModelSprites
{
    struct SpriteSet
    {
        uint32_t count;
        float* centers;     // xyz per sprite
        float* sizes;       // half width, half height per sprite
        float* uvs;         // four corners per sprite (u, v)
        uint8_t* colors;    // BGRA per sprite, 0x80 = 1.0
        uint32_t texture;   // 0x4000: global texture index
        uint16_t mask;      // instance flags & mask must equal value
        uint16_t value;
        uint32_t cached;
    };

    struct Vertex
    {
        float x, y, z;
        D3DCOLOR color;
        float u, v;
    };

    GameRef<uint8_t*> pWorld;
    GameRef<float*> pProjection;
    GameRef<float*> pView;
    GameRef<uint8_t*> pGlobalTextures;
    GameRef<uint8_t*> pLights;
    GameRef<uint32_t> nFogColor;    // followed by start, end, amount
    GameRef<int32_t> nGlobalTextures;
    uint16_t (__cdecl* GetInstanceFlags)(void* instance) = nullptr;
    float* (__fastcall* GetInstanceMatrix)(void* instance, void* edx, float* out) = nullptr;

    DWORD stateBlock = 0;

    // What the state block leaves out under d3d8to9: the vertex shader handle the wrapper tracks itself and the stream
    // that DrawPrimitiveUP unbinds; the game keeps drawing from both without setting them again
    struct Bindings
    {
        DWORD vertexShader = 0;
        IDirect3DVertexBuffer8* stream = nullptr;
        UINT stride = 0;
        IDirect3DIndexBuffer8* indices = nullptr;
        UINT baseVertex = 0;
    } saved;
    std::vector<Vertex> vertices;

    IDirect3DDevice8* Device()
    {
        return reinterpret_cast<IDirect3DDevice8*>(pD3DDevice.get());
    }

    IDirect3DBaseTexture8* GetTexture(uint8_t* model, uint32_t index)
    {
        uint8_t* texture = nullptr;
        if (index & 0x4000)
        {
            index &= 0x3FFF;
            if ((int32_t)index >= nGlobalTextures.get())
                return nullptr;
            texture = pGlobalTextures.get() + 40 * index;
        }
        else
        {
            if ((int32_t)index >= *(int16_t*)(model + 0x30))
                return nullptr;
            texture = *(uint8_t**)(model + 0x10) + 40 * index;
        }
        if (!(texture[1] & 1))
            return nullptr;
        return *(IDirect3DBaseTexture8**)(texture + 28);
    }

    // Column vectors: m[0..3] x, m[4..7] y, m[8..11] z, m[12..15] translation
    void Transform(const float* m, const float* p, float* out)
    {
        out[0] = m[0] * p[0] + m[4] * p[1] + m[8] * p[2] + m[12];
        out[1] = m[1] * p[0] + m[5] * p[1] + m[9] * p[2] + m[13];
        out[2] = m[2] * p[0] + m[6] * p[1] + m[10] * p[2] + m[14];
    }

    // The console tests alpha against 3/4 of the instance's fade: texels that pass write color and depth, the rest
    // only blend
    void Flush(IDirect3DDevice8* device, IDirect3DBaseTexture8* texture, uint32_t fade)
    {
        if (vertices.empty())
            return;

        device->SetTexture(0, texture);
        device->SetRenderState(D3DRS_ALPHAREF, 191 * fade / 15);
        device->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);
        device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
        device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, (UINT)vertices.size() / 3, vertices.data(), sizeof(Vertex));
        device->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_LESS);
        device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
        device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, (UINT)vertices.size() / 3, vertices.data(), sizeof(Vertex));
        vertices.clear();
    }

    void SetStates(IDirect3DDevice8* device)
    {
        D3DMATRIX identity{};
        identity._11 = identity._22 = identity._33 = identity._44 = 1.0f;
        device->SetTransform(D3DTS_WORLD, &identity);
        device->SetTransform(D3DTS_VIEW, reinterpret_cast<const D3DMATRIX*>(pView.get()));
        device->SetTransform(D3DTS_PROJECTION, reinterpret_cast<const D3DMATRIX*>(pProjection.get()));
        device->SetVertexShader(D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1);
        device->SetPixelShader(0);
        device->SetRenderState(D3DRS_LIGHTING, FALSE);
        device->SetRenderState(D3DRS_SPECULARENABLE, FALSE);
        device->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD);
        device->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
        device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
        device->SetRenderState(D3DRS_ZENABLE, TRUE);
        device->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
        device->SetRenderState(D3DRS_STENCILENABLE, FALSE);
        device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
        device->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
        device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
        device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
        device->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
        const uint32_t* fog = &nFogColor.get();
        const float fogStart = *(const float*)(fog + 1);
        const float fogEnd = *(const float*)(fog + 2);
        const float fogAmount = *(const float*)(fog + 3);
        if (fogAmount > 0.0f && fogEnd > fogStart)
        {
            const float end = fogStart + (fogEnd - fogStart) / fogAmount;
            device->SetRenderState(D3DRS_FOGENABLE, TRUE);
            device->SetRenderState(D3DRS_FOGCOLOR, fog[0]);
            device->SetRenderState(D3DRS_FOGTABLEMODE, D3DFOG_NONE);
            device->SetRenderState(D3DRS_FOGVERTEXMODE, D3DFOG_LINEAR);
            device->SetRenderState(D3DRS_RANGEFOGENABLE, FALSE);
            device->SetRenderState(D3DRS_FOGSTART, *(const DWORD*)&fogStart);
            device->SetRenderState(D3DRS_FOGEND, *(const DWORD*)&end);
        }
        else
            device->SetRenderState(D3DRS_FOGENABLE, FALSE);
        device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE2X);
        device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
        device->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
        device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
        device->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
        device->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
        device->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTEXF_LINEAR);
        device->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
        device->SetTextureStageState(0, D3DTSS_MIPFILTER, D3DTEXF_LINEAR);
        device->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
        device->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
        device->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
        device->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
    }

    void Render(float* camera)
    {
        uint8_t* world = pWorld.get();
        if (!world || !pProjection.get() || !pView.get())
            return;

        IDirect3DDevice8* device = Device();
        if (!device)
            return;

        // The camera's own axes in world space (camera matrix at +0x150)
        const float* cam = camera + 0x150 / 4;
        const float right[3] = { cam[0], cam[1], cam[2] };
        const float up[3] = { cam[4], cam[5], cam[6] };

        // The time of day lights: ambient, three directions, three colors
        const float* lights = (const float*)(pLights.get() + 0x1010);

        int32_t instanceCount = *(int32_t*)(world + 0x6430);
        uint8_t* models = *(uint8_t**)(world + 0x38);
        bool begun = false;
        IDirect3DBaseTexture8* batchTexture = nullptr;
        uint32_t batchFade = 0;

        // Adds one model's sprites, placed by its world matrix
        auto add = [&](uint8_t* model, SpriteSet* set, const float* matrix, uint32_t fade)
        {
            IDirect3DBaseTexture8* texture = GetTexture(model, set->texture);
            if (!texture)
                return;

            if (!begun)
            {
                begun = true;
                if (!stateBlock)
                    device->CreateStateBlock(D3DSBT_ALL, &stateBlock);
                device->CaptureStateBlock(stateBlock);
                device->GetVertexShader(&saved.vertexShader);
                device->GetStreamSource(0, &saved.stream, &saved.stride);
                device->GetIndices(&saved.indices, &saved.baseVertex);
                SetStates(device);
            }

            if (texture != batchTexture || fade != batchFade)
            {
                Flush(device, batchTexture, batchFade);
                batchTexture = texture;
                batchFade = fade;
            }

            const float scale = std::sqrt(matrix[0] * matrix[0] + matrix[1] * matrix[1] + matrix[2] * matrix[2]);
            const float rotation[9] = { matrix[0] / scale, matrix[1] / scale, matrix[2] / scale,
                                        matrix[4] / scale, matrix[5] / scale, matrix[6] / scale,
                                        matrix[8] / scale, matrix[9] / scale, matrix[10] / scale };

            for (uint32_t s = 0; s < set->count; s++)
            {
                float c[3];
                Transform(matrix, set->centers + 3 * s, c);
                const float w = set->sizes[2 * s + 0] * scale;
                const float h = set->sizes[2 * s + 1] * scale;
                const uint8_t* bgra = set->colors + 4 * s;
                // The console lights each sprite like the model's other lit vertices, with the direction from the
                // model's origin to the sprite as its normal
                const float* center = set->centers + 3 * s;
                float n[3] = { center[0], center[1], center[2] };
                const float length = std::sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
                float normal[3] = { 0.0f, 0.0f, 0.0f };
                if (length > 0.0f)
                {
                    for (int a = 0; a < 3; a++)
                        normal[a] = (rotation[a] * n[0] + rotation[3 + a] * n[1] + rotation[6 + a] * n[2]) / length;
                }
                float light[3] = { lights[0], lights[1], lights[2] };
                for (int l = 0; l < 3; l++)
                {
                    const float* dir = lights + 4 * (1 + l);
                    const float* col = lights + 4 * (4 + l);
                    const float d = -(normal[0] * dir[0] + normal[1] * dir[1] + normal[2] * dir[2]);
                    if (d > 0.0f)
                    {
                        for (int a = 0; a < 3; a++)
                            light[a] += col[a] * d;
                    }
                }
                // As the game's prelit vertex shader: lit vertex color clamped to 1, then doubled by the texture stage
                auto channel = [](uint8_t v, float l) { return (DWORD)std::clamp(v * l, 0.0f, 255.0f); };
                const DWORD alpha = std::min(255u, bgra[3] * 2u) * fade / 15;
                const D3DCOLOR color = D3DCOLOR_ARGB(alpha, channel(bgra[2], light[0]), channel(bgra[1], light[1]), channel(bgra[0], light[2]));
                const float* uv = set->uvs + 8 * s;
                // The four texture corners go to the lower left, lower right, upper left and upper right corner
                const float sx[4] = { -w, w, -w, w };
                const float sy[4] = { -h, -h, h, h };
                Vertex corner[4];
                for (int k = 0; k < 4; k++)
                {
                    corner[k].x = c[0] + right[0] * sx[k] + up[0] * sy[k];
                    corner[k].y = c[1] + right[1] * sx[k] + up[1] * sy[k];
                    corner[k].z = c[2] + right[2] * sx[k] + up[2] * sy[k];
                    corner[k].color = color;
                    corner[k].u = uv[2 * k + 0];
                    corner[k].v = uv[2 * k + 1];
                }
                vertices.push_back(corner[0]);
                vertices.push_back(corner[1]);
                vertices.push_back(corner[2]);
                vertices.push_back(corner[2]);
                vertices.push_back(corner[1]);
                vertices.push_back(corner[3]);
            }
        };

        for (int32_t i = 0; i < instanceCount; i++)
        {
            uint8_t* instance = *(uint8_t**)(world + 0x5AD4 + 8 * i);
            uint8_t* model = models + 288 * (*(uint32_t*)(instance + 4) >> 20);
            if (*(uint16_t*)(model + 0x4C) != 1)
                continue;
            SpriteSet* set = *(SpriteSet**)(model + 0x1C);
            if (!set || !set->count)
                continue;
            if ((GetInstanceFlags(instance) & set->mask) != set->value)
                continue;
            const uint32_t fade = *(uint8_t*)(instance + 20) & 0xF;
            if (!fade)
                continue;
            alignas(16) float matrix[16];
            GetInstanceMatrix(instance, nullptr, matrix);
            add(model, set, matrix, fade);
        }


        if (begun)
        {
            Flush(device, batchTexture, batchFade);
            device->SetTexture(0, nullptr);
            device->ApplyStateBlock(stateBlock);
            device->SetVertexShader(saved.vertexShader);
            device->SetStreamSource(0, saved.stream, saved.stride);
            device->SetIndices(saved.indices, saved.baseVertex);
            if (saved.stream)
                saved.stream->Release();
            if (saved.indices)
                saved.indices->Release();
            saved = {};
        }
    }

    void Release()
    {
        if (stateBlock && Device())
            Device()->DeleteStateBlock(stateBlock);
        stateBlock = 0;
    }
}

export void RenderModelSprites(float* camera)
{
    ModelSprites::Render(camera);
}

class ModelSpritesInit
{
public:
    ModelSpritesInit()
    {
        WFP::onInitEvent() += []()
        {
            using namespace ModelSprites;

            auto pattern = hook::pattern("8B 15 ? ? ? ? 66 C7 05 ? ? ? ? 00 20 8B 82");
            pWorld.SetAddress(*pattern.get_first<uint8_t**>(2));

            pattern = hook::pattern("A1 ? ? ? ? 89 81 34 07 00 00 8B 15");
            pProjection.SetAddress(*pattern.get_first<float**>(1));
            pView.SetAddress(*pattern.get_first<float**>(13));

            pattern = hook::pattern("3B 05 ? ? ? ? 7D ? 8B 15 ? ? ? ? 8D 0C 80");
            nGlobalTextures.SetAddress(*pattern.get_first<int32_t*>(2));
            pGlobalTextures.SetAddress(*pattern.get_first<uint8_t**>(10));

            pattern = hook::pattern("8B 0D ? ? ? ? E9 ? ? ? ? CC CC CC CC CC 8B 44 24 04 8B 0D");
            pLights.SetAddress(*pattern.get_first<uint8_t**>(2));

            pattern = hook::pattern("B9 ? ? ? ? E8 ? ? ? ? 57 B9 ? ? ? ? E8 ? ? ? ? 8B CF E8");
            nFogColor.SetAddress(*pattern.get_first<uint32_t*>(1));

            pattern = hook::pattern("8B 54 24 04 8B 42 14 8B 52 08 56 8B F0 C1 EE 10 83 E6 3F");
            GetInstanceFlags = (decltype(GetInstanceFlags))pattern.get_first();

            pattern = hook::pattern("55 8B EC 83 E4 F0 81 EC 9C 00 00 00 56 8B F1 8B 4E 0C");
            GetInstanceMatrix = (decltype(GetInstanceMatrix))pattern.get_first();

            WFP::onBeforeReset() += []() { Release(); };
        };
    }
} ModelSpritesInit;
