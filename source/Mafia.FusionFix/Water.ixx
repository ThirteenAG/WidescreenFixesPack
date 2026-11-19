module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <cmath>
#include <numbers>

export module Water;

import ComVars;
import WaterCommon;
import WaterD3D8;
import WaterD3D9;

// Reflective water. The engine renders the scene once more into a texture before the frame, through a copy of the
// camera mirrored at the water surface, with the projection's near plane moved onto the water so nothing below it
// is drawn. The water itself is then drawn with animated wave normals, a fresnel mix of the reflection and the
// game's own water texture, and a sun highlight: with shader model 1.4 on Direct3D 8, or with shader model 2 when the
// game runs through d3d8to9.
namespace Water
{
    // I3D_camera fields
    constexpr size_t CameraSize = 0x4F4;
    constexpr size_t CameraWorld = 0x10;
    constexpr size_t CameraFlags = 0xAC;
    constexpr size_t CameraRevision = 0x118;
    constexpr size_t CameraOrthographic = 0x15C;
    constexpr size_t CameraView = 0x164;
    constexpr size_t CameraProjection = 0x1A4;
    constexpr size_t CameraViewProjection = 0x1E4;
    constexpr size_t CameraNearProjection = 0x364;
    constexpr size_t CameraNearViewProjection = 0x3A4;
    constexpr size_t CameraSector = 0x3F8;
    constexpr size_t CameraSeenRevision = 0x3FC;
    // I3D_scene fields
    constexpr size_t SceneFrame = 0x13C;
    constexpr size_t SceneCallbacks = 0x144;
    constexpr size_t SceneCamera = 0x17C;
    constexpr size_t SceneClearColor = 0x220;
    constexpr size_t SceneRect = 0x23C;
    constexpr size_t SceneFadingCount = 0x290;

    // The renderer's state: set while it renders into a frame begun by an outer Render, the scene and camera being
    // rendered, the frame numbers objects are marked visible with, and its viewport
    GameRef<uint8_t> bRenderingNested;
    GameRef<uintptr_t> pRenderedScene;
    GameRef<uintptr_t> pRenderedCamera;
    GameRef<uint32_t> nRenderFrame;
    GameRef<uint32_t> nPreviousRenderFrame;
    GameRef<std::array<uint8_t, 24>> RenderViewport;
    HRESULT(__stdcall* Render)(uintptr_t driver, uintptr_t scene) = nullptr;
    // The engine's vertex declarations of position, color and one or two texture coordinates
    GameRef<uint32_t> nTexturedDeclaration;
    GameRef<uint32_t> nTextured2Declaration;

    const int* mode = nullptr;
    bool reflecting = false;
    bool reflectionReady = false;
    bool direct3D9 = false;
    uintptr_t reflectionScene = 0, reflectionCamera = 0;
    float reflectionHeight = 0.0f;
    float reflectionMatrix[16]{};
    uint32_t reflectionWidth = 0, reflectionHeightPixels = 0;
    uint32_t reflectionFrame = 0x40000000;
    uintptr_t reflectionSector = 0;
    alignas(16) std::array<uint8_t, CameraSize> mirror{};

    void* device = nullptr;
    SafetyHookInline textureCreateHook;
    SafetyHookMid frameHook, sectorHook, skyHook, textureDestroyHook;
    std::vector<uintptr_t> waterTextures;
    void* stage0 = nullptr;
    bool stage0Water = false;

    struct Surface
    {
        float height, spread;
        float bounds[4];
    };
    // The water meshes seen, with their vertices; some missions move and tilt the water to rock it
    struct Mesh
    {
        uintptr_t buffer;
        uint32_t minIndex, count;
        std::vector<float> positions;
        float world[16];
        std::optional<Surface> surface;
    };
    std::vector<Mesh> meshes;
    struct Seen
    {
        uintptr_t scene, camera;
        std::vector<Surface> drawn;
    };
    std::vector<Seen> seen;
    float sunDirection[3]{ 0.0f, 1.0f, 0.0f };
    float sunColor[3]{};
    bool sunKnown = false;

    void Multiply(const float* a, const float* b, float* out)
    {
        float r[16];
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                r[i * 4 + j] = a[i * 4] * b[j] + a[i * 4 + 1] * b[4 + j] + a[i * 4 + 2] * b[8 + j] + a[i * 4 + 3] * b[12 + j];
        memcpy(out, r, sizeof(r));
    }

    // Shader constant rows: row i is column i of the matrix
    void Rows(const float* m, float* out, int count)
    {
        for (int i = 0; i < count; ++i)
            for (int j = 0; j < 4; ++j)
                out[i * 4 + j] = m[j * 4 + i];
    }

    bool IsWaterName(const char* path)
    {
        const char* name = path;
        for (const char* c = path; *c; ++c)
            if (*c == '\\' || *c == '/')
                name = c + 1;
        std::string upper(name);
        std::transform(upper.begin(), upper.end(), upper.begin(), [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
        if (upper.find("VLNY") != std::string::npos || upper.find("VODA") != std::string::npos)
            return true;
        // The sea's animation frames: 00TEST00.BMP, 00TEST01.BMP and so on
        return upper.size() == 12 && upper.starts_with("00TEST") && std::isdigit(static_cast<unsigned char>(upper[6]))
            && std::isdigit(static_cast<unsigned char>(upper[7])) && upper.ends_with(".BMP");
    }

    bool IsWaterTexture(void* texture)
    {
        if (!texture)
            return false;
        for (auto itexture : waterTextures)
            if (Field<void*>(itexture, 8) == texture)
                return true;
        return false;
    }

    // ITexture::Create(name, alpha name, flags, ...) of the engine's textures
    int __stdcall TextureCreate(uintptr_t texture, const char* name, const char* alpha, int flags, uint32_t a5, uint32_t a6, uint32_t a7)
    {
        const int result = textureCreateHook.stdcall<int>(texture, name, alpha, flags, a5, a6, a7);
        std::erase(waterTextures, texture);
        if (result >= 0 && name && IsWaterName(name))
            waterTextures.push_back(texture);
        stage0Water = IsWaterTexture(stage0);
        return result;
    }

    // The device's methods can be swapped by the runtime (a Direct3D 8 reset does), so every implementation the
    // device used gets its own hook
    std::array<SafetyHookInline, 4> setTextureHooks, drawHooks;
    std::array<void*, 4> setTextureTargets{}, drawTargets{};

    template<size_t I> HRESULT __stdcall SetTexture(void* d, uint32_t stage, void* texture)
    {
        if (!stage)
        {
            stage0 = texture;
            stage0Water = !waterTextures.empty() && IsWaterTexture(texture);
        }
        return setTextureHooks[I].stdcall<HRESULT>(d, stage, texture);
    }

    // The water plane of a mesh drawn with the given world matrix: the median height of its vertices, if they are
    // close to one plane (the 10th to 90th percentile within 4 m, or a slope under 1 %) at least 8 m across. Small
    // water, like a glass or a bucket, keeps the game's look.
    std::optional<Surface> SurfaceOf(uint32_t minIndex, uint32_t count, const float world[16], uintptr_t buffer)
    {
        auto mesh = std::find_if(meshes.begin(), meshes.end(), [&](const Mesh& m) { return m.buffer == buffer && m.minIndex == minIndex && m.count == count; });
        if (mesh == meshes.end())
        {
            if (meshes.size() >= 64)
                meshes.erase(meshes.begin());
            Mesh created{ buffer, minIndex, count };
            if (!WaterD3D8::Positions(device, minIndex, count, created.positions))
                created.positions.clear();
            mesh = meshes.insert(meshes.end(), std::move(created));
        }
        else if (!memcmp(mesh->world, world, sizeof(mesh->world)))
            return mesh->surface;
        memcpy(mesh->world, world, sizeof(mesh->world));
        mesh->surface.reset();
        const size_t vertices = mesh->positions.size() / 3;
        if (!vertices)
            return std::nullopt;
        std::vector<float> heights(vertices);
        Surface surface{ 0.0f, 0.0f, { FLT_MAX, -FLT_MAX, FLT_MAX, -FLT_MAX } };
        for (size_t i = 0; i < vertices; ++i)
        {
            const float* p = &mesh->positions[i * 3];
            heights[i] = p[0] * world[1] + p[1] * world[5] + p[2] * world[9] + world[13];
            const float x = p[0] * world[0] + p[1] * world[4] + p[2] * world[8] + world[12];
            const float z = p[0] * world[2] + p[1] * world[6] + p[2] * world[10] + world[14];
            surface.bounds[0] = std::min(surface.bounds[0], x);
            surface.bounds[1] = std::max(surface.bounds[1], x);
            surface.bounds[2] = std::min(surface.bounds[2], z);
            surface.bounds[3] = std::max(surface.bounds[3], z);
        }
        std::sort(heights.begin(), heights.end());
        surface.height = heights[vertices / 2];
        surface.spread = heights[vertices * 9 / 10] - heights[vertices / 10];
        const float extent = std::max(surface.bounds[1] - surface.bounds[0], surface.bounds[3] - surface.bounds[2]);
        if (extent >= 8.0f && (surface.spread < 4.0f || surface.spread < extent * 0.01f))
            mesh->surface = surface;
        return mesh->surface;
    }

    double Seconds()
    {
        static LARGE_INTEGER frequency{}, start{};
        if (!frequency.QuadPart)
        {
            QueryPerformanceFrequency(&frequency);
            QueryPerformanceCounter(&start);
        }
        LARGE_INTEGER now{};
        QueryPerformanceCounter(&now);
        return static_cast<double>(now.QuadPart - start.QuadPart) / static_cast<double>(frequency.QuadPart);
    }

    void Constants(WaterDraw& water, const float world[16], const float view[16], const float projection[16])
    {
        float m[16];
        Multiply(world, view, m);
        Multiply(m, projection, m);
        Rows(m, &water.worldViewProj[0][0], 4);
        Rows(world, &water.world[0][0], 3);

        // Clip space of the reflection to its texture, as (u w, v w, w, w), scaled down to keep ps 1.4 in range
        const float scale = 0.01f;
        const float halfX = 0.5f / std::max(1u, reflectionWidth), halfY = 0.5f / std::max(1u, reflectionHeightPixels);
        const float texture[16] =
        {
            0.5f * scale, 0.0f, 0.0f, 0.0f,
            0.0f, -0.5f * scale, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f,
            (0.5f + halfX) * scale, (0.5f + halfY) * scale, scale, scale,
        };
        Multiply(world, reflectionMatrix, m);
        Multiply(m, texture, m);
        Rows(m, &water.reflection[0][0], 4);

        // Three wave layers drifting with the wind; tiles of 15, 6.5 and 2.6 meters
        const double time = Seconds();
        struct Layer
        {
            float size, angle, speedU, speedV;
        };
        static constexpr Layer layers[] = { { 15.0f, 0.35f, 0.011f, 0.004f }, { 6.5f, 1.25f, -0.009f, 0.016f }, { 2.6f, -0.6f, 0.027f, -0.019f } };
        for (int i = 0; i < 3; ++i)
        {
            const auto& layer = layers[i];
            const float s = 1.0f / layer.size, c = std::cos(layer.angle) * s, n = std::sin(layer.angle) * s;
            const float u = static_cast<float>(std::fmod(time * layer.speedU, 1.0)), v = static_cast<float>(std::fmod(time * layer.speedV, 1.0));
            const float rowU[4] = { c, -n, u, 0.0f }, rowV[4] = { n, c, v, 0.0f };
            memcpy(water.waves[i * 2], rowU, sizeof(rowU));
            memcpy(water.waves[i * 2 + 1], rowV, sizeof(rowV));
        }

        float start = 0.0f, end = 1.0f;
        WaterD3D8::Fog(device, start, end);
        water.fog[0] = end;
        water.fog[1] = end > start ? 1.0f / (end - start) : 0.0f;
        water.fog[2] = water.fog[3] = 0.0f;

        const float* camera = &Field<float>(pRenderedCamera, CameraWorld + 48);
        water.camera[0] = camera[0];
        water.camera[1] = camera[1];
        water.camera[2] = camera[2];
        water.camera[3] = 1.0f;

        float direction[3], color[3];
        if (WaterD3D8::Sun(device, direction, color))
        {
            memcpy(sunDirection, direction, sizeof(direction));
            memcpy(sunColor, color, sizeof(color));
            sunKnown = true;
        }
        water.sun[0] = sunDirection[0];
        water.sun[1] = sunDirection[1];
        water.sun[2] = sunDirection[2];
        water.sun[3] = sunKnown && sunDirection[1] > 0.0f ? 1.0f : 0.0f;
        water.sunColor[0] = sunColor[0];
        water.sunColor[1] = sunColor[1];
        water.sunColor[2] = sunColor[2];
        water.sunColor[3] = 0.0f;

        if (direct3D9)
        {
            const float tint[4] = { 0.95f, 0.97f, 1.0f, 0.9f };
            const float params[4] = { 0.025f, 0.04f, 1.0f, 1.0f / 300.0f };
            memcpy(water.tint, tint, sizeof(tint));
            memcpy(water.params, params, sizeof(params));
        }
        else
        {
            const float tint[4] = { 0.95f, 0.97f, 1.0f, 0.6f };
            const float params[4] = { 0.012f, 0.03f, 1.0f, 0.0f };
            memcpy(water.tint, tint, sizeof(tint));
            memcpy(water.params, params, sizeof(params));
            for (auto& c : water.sunColor)
                c *= 0.6f;
        }
        // The game's water is its texture times the vertex color, scaled like the engine's texture stage does
        const float modulation = WaterD3D8::Modulation(device);
        for (int i = 0; i < 3; ++i)
            water.tint[i] *= modulation;
    }

    // The engine's indexed draws: the water's are replaced, and skipped while the reflection is rendered
    HRESULT Draw(void* d, uint32_t minIndex, uint32_t count, const std::function<HRESULT()>& draw)
    {
        if (!stage0Water || !mode || !*mode || d != device)
            return draw();
        if (reflecting)
            return S_OK;
        const uintptr_t scene = pRenderedScene;
        WaterLayout layout;
        uintptr_t buffer = 0;
        if (!scene || !WaterD3D8::Layout(d, layout, buffer, nTexturedDeclaration, nTextured2Declaration))
            return draw();
        float world[16], view[16], projection[16];
        WaterD3D8::Transforms(d, world, view, projection);
        const auto surface = SurfaceOf(minIndex, count, world, buffer);
        if (!surface)
            return draw();
        const uintptr_t camera = pRenderedCamera;
        auto entry = std::find_if(seen.begin(), seen.end(), [&](const Seen& s) { return s.scene == scene && s.camera == camera; });
        if (entry == seen.end())
            entry = seen.insert(seen.end(), { scene, camera, {} });
        if (entry->drawn.size() < 64)
            entry->drawn.push_back(*surface);
        if (!reflectionReady || scene != reflectionScene || camera != reflectionCamera || std::abs(surface->height - reflectionHeight) > 1.5f)
            return draw();
        WaterDraw water{};
        Constants(water, world, view, projection);
        const bool done = direct3D9 ? WaterD3D9::Draw(WaterD3D9::Device(static_cast<IUnknown*>(d)), water, layout, draw)
            : WaterD3D8::Draw(d, water, layout, draw);
        return done ? S_OK : draw();
    }

    template<size_t I> HRESULT __stdcall DrawIndexedPrimitive(void* d, uint32_t type, uint32_t minIndex, uint32_t count, uint32_t start, uint32_t primitives)
    {
        return Draw(d, minIndex, count, [&]()
        {
            return drawHooks[I].stdcall<HRESULT>(d, type, minIndex, count, start, primitives);
        });
    }

    template<size_t... I> constexpr auto Hooks(std::index_sequence<I...>)
    {
        return std::pair{ std::array{ &SetTexture<I>... }, std::array{ &DrawIndexedPrimitive<I>... } };
    }

    template<class F> void Attach(void* target, std::array<void*, 4>& targets, std::array<SafetyHookInline, 4>& hooks, const std::array<F, 4>& functions)
    {
        for (size_t i = 0; i < targets.size(); ++i)
        {
            if (targets[i] == target)
                return;
            if (!targets[i])
            {
                targets[i] = target;
                hooks[i] = safetyhook::create_inline(target, functions[i]);
                return;
            }
        }
    }

    void AttachDevice()
    {
        auto current = static_cast<void*>(pD3D8Device.get());
        if (!current)
            return;
        if (current != device)
        {
            if (device)
            {
                // A new device: the old one's objects went with it
                WaterD3D8::Forget();
                WaterD3D9::Forget();
                meshes.clear();
            }
            device = current;
        }
        static constexpr auto functions = Hooks(std::make_index_sequence<4>{});
        auto table = *static_cast<void***>(device);
        Attach(table[61], setTextureTargets, setTextureHooks, functions.first);
        Attach(table[71], drawTargets, drawHooks, functions.second);
    }

    // The water plane: of the surfaces drawn the last time the scene was rendered, the one closest to the camera, the
    // larger one when equally close
    std::optional<float> Plane(uintptr_t scene, uintptr_t camera, const float* position)
    {
        auto entry = std::find_if(seen.begin(), seen.end(), [&](const Seen& s) { return s.scene == scene && s.camera == camera; });
        if (entry == seen.end() || entry->drawn.empty())
            return std::nullopt;
        const Surface* best = nullptr;
        float bestDistance = FLT_MAX, bestArea = 0.0f;
        for (const auto& surface : entry->drawn)
        {
            const float dx = std::max({ surface.bounds[0] - position[0], 0.0f, position[0] - surface.bounds[1] });
            const float dz = std::max({ surface.bounds[2] - position[2], 0.0f, position[2] - surface.bounds[3] });
            const float distance = std::sqrt(dx * dx + dz * dz);
            const float area = (surface.bounds[1] - surface.bounds[0]) * (surface.bounds[3] - surface.bounds[2]);
            if (distance < bestDistance - 0.5f || (distance < bestDistance + 0.5f && area > bestArea))
            {
                best = &surface;
                bestDistance = distance;
                bestArea = area;
            }
        }
        const float height = best->height;
        entry->drawn.clear();
        return height;
    }

    // Moves the near plane of a projection onto a view space plane, keeping x, y and w
    void Oblique(float* projection, const float plane[4])
    {
        const float sx = plane[0] > 0.0f ? 1.0f : (plane[0] < 0.0f ? -1.0f : 0.0f);
        const float sy = plane[1] > 0.0f ? 1.0f : (plane[1] < 0.0f ? -1.0f : 0.0f);
        const float q[4] = { sx / projection[0], sy / projection[5], 1.0f, (1.0f - projection[10]) / projection[14] };
        const float dot = plane[0] * q[0] + plane[1] * q[1] + plane[2] * q[2] + plane[3] * q[3];
        if (std::abs(dot) < 1e-6f)
            return;
        projection[2] = plane[0] / dot;
        projection[6] = plane[1] / dot;
        projection[10] = plane[2] / dot;
        projection[14] = plane[3] / dot;
    }

    // Renders the mirrored scene into the reflection texture; called after the engine began the frame
    void Reflect(uintptr_t driver, uintptr_t scene)
    {
        reflectionReady = false;
        if (!mode || !*mode || !scene)
            return;
        AttachDevice();
        const uintptr_t camera = Field<uintptr_t>(scene, SceneCamera);
        if (!device || !camera)
            return;
        const float* world = &Field<float>(camera, CameraWorld);
        const auto plane = Plane(scene, camera, world + 12);
        if (!plane || Field<uint8_t>(camera, CameraOrthographic) || !(Field<uint8_t>(camera, CameraFlags) & 0x20) || world[13] - *plane < 0.2f)
            return;
        const float height = *plane;

        auto device9 = WaterD3D9::Device(static_cast<IUnknown*>(device));
        direct3D9 = device9 && WaterD3D9::Supported(device9);
        if (!direct3D9 && !WaterD3D8::Supported(device))
            return;

        // The camera mirrored at the water, turned upside down so it stays a rotation: the picture is flipped,
        // which the projection of the water onto it accounts for
        const uintptr_t reflected = reinterpret_cast<uintptr_t>(mirror.data());
        memcpy(mirror.data(), reinterpret_cast<void*>(camera), CameraSize);
        float* m = &Field<float>(reflected, CameraWorld);
        m[1] = -m[1];
        m[4] = -m[4];
        m[6] = -m[6];
        m[9] = -m[9];
        m[13] = 2.0f * height - m[13];
        Field<uint8_t>(reflected, CameraFlags) |= 0x20;
        auto table = *reinterpret_cast<uintptr_t**>(reflected);
        reinterpret_cast<void(__thiscall*)(uintptr_t)>(table[0x44 / sizeof(uintptr_t)])(reflected);
        reflectionSector = Field<uintptr_t>(camera, CameraSector);
        Field<uintptr_t>(reflected, CameraSector) = reflectionSector;
        Field<uint32_t>(reflected, CameraSeenRevision) = Field<uint32_t>(reflected, CameraRevision) - 1;

        // The water plane in view space; the world matrix maps view to world, so a plane goes the other way with it
        const float clip[4] = { m[1], m[5], m[9], m[13] - height + 0.05f };
        float* view = &Field<float>(reflected, CameraView);
        for (size_t projection : { CameraProjection, CameraNearProjection })
            Oblique(&Field<float>(reflected, projection), clip);
        Multiply(view, &Field<float>(reflected, CameraProjection), &Field<float>(reflected, CameraViewProjection));
        Multiply(view, &Field<float>(reflected, CameraNearProjection), &Field<float>(reflected, CameraNearViewProjection));
        memcpy(reflectionMatrix, &Field<float>(reflected, CameraViewProjection), sizeof(reflectionMatrix));

        // The size of the frame, from the viewport the engine set
        const auto viewport = RenderViewport.get();
        const auto* size = reinterpret_cast<const uint32_t*>(viewport.data());
        const float scale = *mode >= 2 ? 1.0f : 0.5f;
        reflectionWidth = std::max(16u, static_cast<uint32_t>(size[2] * scale));
        reflectionHeightPixels = std::max(16u, static_cast<uint32_t>(size[3] * scale));

        const auto& clear = Field<std::array<float, 3>>(scene, SceneClearColor);
        const auto channel = [](float value) { return static_cast<uint32_t>(std::clamp(value, 0.0f, 1.0f) * 255.0f); };
        const uint32_t color = 0xFF000000u | (channel(clear[0]) << 16) | (channel(clear[1]) << 8) | channel(clear[2]);
        const bool begun = direct3D9 ? WaterD3D9::BeginReflection(device9, reflectionWidth, reflectionHeightPixels, color)
            : WaterD3D8::BeginReflection(device, reflectionWidth, reflectionHeightPixels, color);
        if (!begun)
            return;

        const auto rect = Field<std::array<int32_t, 4>>(scene, SceneRect);
        const uint8_t callbacks = Field<uint8_t>(scene, SceneCallbacks);
        const uint32_t fading = Field<uint32_t>(scene, SceneFadingCount);
        const uint32_t frame = Field<uint32_t>(scene, SceneFrame);
        const uint32_t current = nRenderFrame, previous = nPreviousRenderFrame;
        Field<std::array<int32_t, 4>>(scene, SceneRect) = { 0, 0, static_cast<int32_t>(reflectionWidth), static_cast<int32_t>(reflectionHeightPixels) };
        Field<uint8_t>(scene, SceneCallbacks) = callbacks & ~2;
        Field<uint32_t>(scene, SceneFadingCount) = 0;
        Field<uint32_t>(scene, SceneFrame) = reflectionFrame;
        Field<uintptr_t>(scene, SceneCamera) = reflected;
        ReflectionCamera = reflected;
        bRenderingNested = 1;
        reflecting = true;
        Render(driver, scene);
        reflecting = false;
        bRenderingNested = 0;
        ReflectionCamera = 0;
        reflectionFrame = Field<uint32_t>(scene, SceneFrame);
        Field<uintptr_t>(scene, SceneCamera) = camera;
        Field<uint32_t>(scene, SceneFrame) = frame;
        Field<uint32_t>(scene, SceneFadingCount) = fading;
        Field<uint8_t>(scene, SceneCallbacks) = callbacks;
        Field<std::array<int32_t, 4>>(scene, SceneRect) = rect;
        nRenderFrame = current;
        nPreviousRenderFrame = previous;
        pRenderedCamera = camera;
        pRenderedScene = scene;
        RenderViewport = viewport;
        if (direct3D9)
            WaterD3D9::EndReflection(device9, viewport.data());
        else
            WaterD3D8::EndReflection(device, viewport.data());
        // Objects drawn with software vertex processing keep their transformed vertices while the camera is still;
        // the reflection replaced them
        Field<uint32_t>(camera, CameraSeenRevision) = Field<uint32_t>(camera, CameraRevision) - 1;

        reflectionReady = true;
        reflectionScene = scene;
        reflectionCamera = camera;
        reflectionHeight = height;
    }
}

export void InitWater()
{
    using namespace Water;
    mode = &FixSettings().Int("GRAPHICS.Water");
    auto module = GetLS3DF();

    auto render = find_module_pattern(module, "55 8B EC 83 EC 3C 56 8B 75 0C 85 F6 57 75");
    Render = reinterpret_cast<decltype(Render)>(render.get_first());

    auto state = find_module_pattern(module, "C6 05 ? ? ? ? 01 89 35 ? ? ? ? 8B 8E 3C 01 00 00 89 0D ? ? ? ? 8B 8E 3C 01 00 00 41 89 8E 3C 01 00 00 8B C1 A3 ? ? ? ? 8B 8E 7C 01 00 00 85 C9 89 0D");
    pRenderedScene.SetAddress(*state.get_first<uintptr_t*>(9));
    nPreviousRenderFrame.SetAddress(*state.get_first<uint32_t*>(21));
    nRenderFrame.SetAddress(*state.get_first<uint32_t*>(41));
    pRenderedCamera.SetAddress(*state.get_first<uintptr_t*>(55));

    auto textured = find_module_pattern(module, "68 ? ? ? ? 6A 00 8D 54 24 ? BB 07 00 01 40");
    nTexturedDeclaration.SetAddress(*textured.get_first<uint32_t*>(1));
    auto textured2 = find_module_pattern(module, "68 ? ? ? ? 6A 00 8D 54 24 ? 52 C7 44 24 ? 00 00 00 20 89 74 24 ? C7 44 24 ? 05 00 04 40 89 5C 24 ? C7 44 24 ? 08 00 01 40");
    nTextured2Declaration.SetAddress(*textured2.get_first<uint32_t*>(1));

    auto viewport = find_module_pattern(module, "68 ? ? ? ? 8D 4D C4 51 89 55 CC 89 45 D0 FF 15");
    RenderViewport.SetAddress(*viewport.get_first<std::array<uint8_t, 24>*>(1));

    // After the sector of the camera is looked up: the mirrored camera stays in the real camera's sector
    auto sector = find_module_pattern(module, "E8 ? ? ? ? A0 ? ? ? ? 84 C0 74 04 33 FF EB");
    bRenderingNested.SetAddress(*sector.get_first<uint8_t*>(6));
    sectorHook = safetyhook::create_mid(sector.get_first(5), [](SafetyHookContext& regs)
    {
        if (reflecting)
            Field<uintptr_t>(reinterpret_cast<uintptr_t>(mirror.data()), CameraSector) = reflectionSector;
    });

    // The frame has begun: the reflection is rendered first
    auto frame = find_module_pattern(module, "8A 8E 44 01 00 00 B0 01 84 C8 74");
    frameHook = safetyhook::create_mid(frame.get_first(), [](SafetyHookContext& regs)
    {
        Reflect(Field<uintptr_t>(regs.ebp, 8), regs.esi);
    });

    // The sky is drawn only in outer frames; the reflection needs it too
    auto sky = find_module_pattern(module, "8B CE E8 ? ? ? ? A0 ? ? ? ? 84 C0 0F 85 ? ? ? ? A1 ? ? ? ? 85 C0 0F 84 ? ? ? ? 8B ? 10 02 00 00");
    skyHook = safetyhook::create_mid(sky.get_first(12), [](SafetyHookContext& regs)
    {
        if (reflecting)
            regs.eax &= ~0xFFu;
    });

    // The water's textures, by name
    auto create = find_module_pattern(module, "81 EC 60 02 00 00 53 55 8B AC 24 6C 02 00 00 8B 45 00 56 57 33 FF 55");
    textureCreateHook = safetyhook::create_inline(create.get_first(), TextureCreate);
    auto destroy = find_module_pattern(module, "6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 51 56 8B F1 89 74 24 04 C7 06 ? ? ? ? 56 C7 44 24 14 01 00 00 00 E8 ? ? ? ? 8B 46 20");
    textureDestroyHook = safetyhook::create_mid(destroy.get_first(), [](SafetyHookContext& regs)
    {
        std::erase(waterTextures, regs.ecx);
        stage0Water = IsWaterTexture(stage0);
    });

    WFP::onBeforeReset() += []()
    {
        reflectionReady = false;
        WaterD3D8::Release();
        WaterD3D9::Release();
    };
}
