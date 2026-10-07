#include "Game.hpp"
#include "Lights.hpp"

namespace vcsfx {
namespace {
constexpr size_t lightCount = sizeof(lights) / sizeof(lights[0]);
constexpr size_t visibleLimit = 2048;
console::LightGrid<lightCount> grid;
uint8_t trafficGroups[lightCount]{};
struct VisibleLight {
    float distance;
    Vector screen;
    float width, height;
    uint16_t index;
    uint8_t intensity;
};
console::NearestLights<VisibleLight, visibleLimit> visible;
pcsx2::GameFunction<int(Vector*, Vector*, float*, float*, bool)> project;
pcsx2::GameFunction<void(float, float, float, float, float, uint8_t, uint8_t, uint8_t, int16_t, float, uint8_t)> draw;
injector::hook_back<void()> flush, renderCoronas;
pcsx2::GameFunction<uint64_t(int, uintptr_t)> renderState;
pcsx2::GameFunction<int64_t(void*)> dmaFree;
using CoronaSignature = uint64_t(uint32_t, uint8_t, uint8_t, uint8_t, uint8_t, const Vector*,
    uint8_t, uint8_t, float, float, float, float, uint8_t, uint8_t, uint8_t, int32_t, int32_t);
pcsx2::GameFunction<CoronaSignature> registerCorona;
pcsx2::GameCallback<CoronaSignature> trafficCorona;
pcsx2::GameCallback<void()> lightCallback,starCallback;
safetymips::GameInline<void(void*)> trafficDisplay;
safetymips::GameInline<uint64_t(void*)> trafficType;
unsigned activeTrafficGroup;
bool learningTrafficGroup;

uintptr_t coronaRaster() {
    auto texture = read<uint32_t>(0x487758);
    return texture ? read<uint32_t>(texture) : 0;
}
// CSprite::FlushSpriteBuffer only drops sprites when the frame's fixed DMA
// arena runs low during CParticle::Render; elsewhere the GIF writer advances
// past the arena end unchecked. Each buffered sprite becomes six 48-byte DMA
// vertices (plus a header per flush of 96). Keep the same 192 KiB reserve as
// distant traffic for the HUD, text and menus that follow (particles check
// the arena themselves).
constexpr int64_t spriteBytes = 300, spriteReserve = 192 * 1024;
#ifdef VCSFX_DIAGNOSTICS
// Test builds only, read over PINE: lowest free arena bytes seen before coronas.
struct Diagnostics { int32_t lowestFree; uint32_t frames, limited; } diagnostics{0x7FFFFFFF, 0, 0};
#endif
unsigned SpriteBudget() {
    const int64_t free = dmaFree(reinterpret_cast<void*>(0x520F90));
#ifdef VCSFX_DIAGNOSTICS
    ++diagnostics.frames;
    if (free < diagnostics.lowestFree) diagnostics.lowestFree = int32_t(free);
    if (free <= spriteReserve + int64_t(settings.limit) * spriteBytes) ++diagnostics.limited;
#endif
    return free > spriteReserve ? unsigned((free - spriteReserve) / spriteBytes) : 0;
}
void BindRenderer() {
    dmaFree.bind(0x478628);
    project.bind(0x39FC48);
    draw.bind(0x39FF48);
    renderState.bind(0x175198);
    flush.fun = reinterpret_cast<void (*)()>(0x39FDE8);
}
// The table stores IPL orientation, not a heading in radians. Derive the
// signal direction from each red/green lamp pair. Opposite faces share a phase.
// The inequalities are the native FindTrafficLightType 60..150 degree sectors.
unsigned TrafficGroup(float redToGreenX, float redToGreenY) {
    float x = redToGreenY, y = -redToGreenX;
    if (y < 0.0f) { x = -x; y = -y; }
    return 0.5f * y - 0.8660254f * x > 0.0f &&
           -0.8660254f * y - 0.5f * x < 0.0f ? 1 : 2;
}
void InitializeTrafficGroups() {
    for (size_t i = 0; i < lightCount; ++i) {
        const auto& light = lights[i];
        if (!console::trafficLight(light)) continue;
        // Locate the same lamp's opposite colour within its small local cluster.
        float nearest = 1.0f;
        uint16_t pair = uint16_t(i);
        unsigned ownColor = console::trafficColor(light.color);
        grid.visit(light.position, 1.0f, [&](uint16_t candidate) {
            const auto& other = lights[candidate];
            if (!console::trafficLight(other) || other.orientation != light.orientation) return;
            unsigned color = console::trafficColor(other.color);
            if ((ownColor == 0 && color != 2) || (ownColor != 0 && color != 0)) return;
            float distance = console::distanceSquared(light.position, other.position);
            if (distance < nearest) { nearest = distance; pair = candidate; }
        });
        float dx = lights[pair].position.x - light.position.x;
        float dy = lights[pair].position.y - light.position.y;
        trafficGroups[i] = uint8_t(TrafficGroup(dx, dy));
    }
}
void DisplayTraffic(void* entity) {
    unsigned previous = activeTrafficGroup;
    bool previousLearning = learningTrafficGroup;
    activeTrafficGroup = 0;
    learningTrafficGroup = true;
    trafficDisplay.call(entity);
    activeTrafficGroup = previous;
    learningTrafficGroup = previousLearning;
}
uint64_t FindTrafficType(void* entity) {
    auto group = trafficType.call(entity);
    if (learningTrafficGroup) activeTrafficGroup = unsigned(group);
    return group;
}
uint64_t RegisterTraffic(uint32_t id, uint8_t r, uint8_t g, uint8_t b, uint8_t alpha,
    const Vector* position, uint8_t type, uint8_t flare, float radius, float range,
    float angle, float pull, uint8_t reflection, uint8_t los, uint8_t streak,
    int32_t flag4, int32_t flag5) {
    // Learn the exact native group when the streamed entity renders. No guessed
    // clock-based timing, and no additional game corona registrations.
    if (position && (activeTrafficGroup == 1 || activeTrafficGroup == 2)) {
        grid.visit(*position, 0.5f, [&](uint16_t candidate) {
            if (console::trafficLight(lights[candidate]) &&
                console::distanceSquared(*position, lights[candidate].position) < 0.25f)
                trafficGroups[candidate] = uint8_t(activeTrafficGroup);
        });
    }
    return registerCorona(id, r, g, b, alpha, position, type, flare, radius, range,
                          angle, pull, reflection, los, streak, flag4, flag5);
}
void RenderLights() {
    renderCoronas.fun();
    if(settings.traffic)RenderTraffic();
    if(!settings.lights)return;
    const Vector camera = cameraPosition();
    float night = console::nightIntensity(hour(), minute());
    float phases[8];
    for (unsigned mode = 0; mode < 8; ++mode)
        phases[mode] = console::blinkIntensity(mode, read<uint32_t>(0x4CD104));
    unsigned signals[] = {3, unsigned(reinterpret_cast<uint64_t (*)()>(0x313390)()),
                            unsigned(reinterpret_cast<uint64_t (*)()>(0x3133F0)())};
    float maximumSquared = settings.range * settings.range;
    const unsigned budget = SpriteBudget();
    visible.clear(settings.limit < budget ? settings.limit : budget);
    if (!budget) return;
    grid.visit(camera, settings.range, [&](uint16_t i) {
        const auto& light = lights[i];
        float distanceSquared = console::distanceSquared(camera, light.position);
        const float hx = light.position.x - camera.x, hy = light.position.y - camera.y;
        const float horizontal = __builtin_sqrtf(hx * hx + hy * hy); // range is horizontal (altitude keeps lights)
        if (!(horizontal * horizontal < maximumSquared)) return;
        bool traffic = console::trafficLight(light);
        float intensity = traffic ? (signals[trafficGroups[i]] == console::trafficColor(light.color) ? 1.0f : 0.0f)
                                  : night * phases[light.blink < 8 ? light.blink : 0];
        if (intensity <= 0.0f || !light.color.a) return;
        float nearRange = light.nativeRange / 3.0f;
        if (!light.noDistance && distanceSquared <= nearRange * nearRange) return;
        Vector position = light.position, screen;
        float width, height;
        // Our distance bound is independent of the streamed geometry far clip.
        if (!project(&position, &screen, &width, &height, false)) return;
        float distance = __builtin_sqrtf(distanceSquared);
        float radius = light.noDistance ? 3.5f : 3.5f * console::saturate(
            (distance - nearRange) / (light.nativeRange - nearRange));
        radius *= light.size * settings.radius *
            console::bounded(0.0025f * distance + 0.25f, 0.0f, 4.0f, 1.0f);
        width *= radius; height *= radius;
        if (!(width > 0.0f && height > 0.0f && screen.z > 0.0f) ||
            screen.x + width < 0.0f || screen.x - width > 640.0f ||
            screen.y + height < 0.0f || screen.y - height > 448.0f) return;
        // Fade the last ten percent of the range rather than pop at the cutoff.
        intensity *= console::saturate((settings.range - horizontal) / (settings.range * 0.1f));
        unsigned alpha = unsigned(intensity * light.color.a);
        if (alpha) visible.insert({distanceSquared, screen, width, height, i, uint8_t(alpha)});
    });
    uintptr_t raster = coronaRaster();
    if (!raster || !visible.size()) return;
    renderState(6, 0); renderState(4, 1); renderState(8, 2); renderState(9, 2);
    renderState(1, raster);
    for (const auto& item : visible) {
        const auto& color = lights[item.index].color;
        // Coronas are flat screen sprites at the lamp's depth. Seen from above, the
        // ground or facade behind a lamp is nearly as deep, so part of the sprite
        // failed the depth test (cut in half) and, higher up, all of it. Draw them
        // slightly in front of the lamp (as native coronas do), scaled with depth.
        const float bias = 2.0f + 0.04f * item.screen.z;
        const float z = bias < item.screen.z * 0.5f ? item.screen.z - bias : item.screen.z * 0.5f;
        draw(item.screen.x, item.screen.y, z, item.width, item.height,
             color.r, color.g, color.b, item.intensity, 1.0f / z, item.intensity);
    }
    flush.fun();
    // This call site follows native CCoronas::Render. Restore its verified
    // postconditions before the next world-rendering phase.
    renderState(1, 0); renderState(6, 1); renderState(4, 1);
}

struct Star { Vector offset; float size; };
Star stars[500];
uint32_t randomState = 0x56435332;
float Random(float low, float high) {
    randomState ^= randomState << 13; randomState ^= randomState >> 17; randomState ^= randomState << 5;
    return low + (high - low) * float(randomState & 0xFFFFFFu) / 16777216.0f;
}
void RenderStars() {
    flush.fun();
    float coverage = read<float>(0x4CD14C);
    float cloud = read<float>(0x4CD2C8);
    if (cloud > coverage) coverage = cloud;
    unsigned brightness = unsigned(255.0f * console::starIntensity(hour(), minute()) *
                                  (1.0f - console::saturate(coverage)));
    uintptr_t raster = coronaRaster();
    unsigned budget = brightness && raster ? SpriteBudget() : 0;
    if (!budget) return;
    // Inside CClouds::Render, before translucent clouds and world geometry.
    // Its current depth/blend state already matches the native star pass.
    renderState(1, raster);
    Vector camera = cameraPosition();
    for (const auto& star : stars) {
        Vector position{camera.x + star.offset.x, camera.y + star.offset.y, camera.z + star.offset.z};
        Vector screen; float width, height;
        if (!project(&position, &screen, &width, &height, false) || !(screen.z > 0.0f)) continue;
        width *= star.size; height *= star.size;
        if (screen.x + width < 0.0f || screen.x - width > 640.0f ||
            screen.y + height < 0.0f || screen.y - height > 448.0f) continue;
        draw(screen.x, screen.y, screen.z, width, height, uint8_t(brightness), uint8_t(brightness),
             uint8_t(brightness), 255, 1.0f / screen.z, 255);
        if (!--budget) break;
    }
    // Never leave our vertices pending for a subsequent cloud texture change.
    flush.fun();
}
}
void InstallLights() {
    BindRenderer();
    lightCallback.bind(RenderLights);
    renderCoronas.fun = injector::MakeCALL(0x21F268, lightCallback.address()).get();
    if(!settings.lights)return;
    grid.initialize(lights);
    InitializeTrafficGroups();
    registerCorona.bind(0x27DD10);
    trafficCorona.bind(RegisterTraffic);
    trafficDisplay = safetymips::create_inline_game(0x313480, DisplayTraffic);
    trafficType = safetymips::create_inline_game(0x3155C0, FindTrafficType);
    for (uintptr_t site : {0x313DFCu, 0x313E9Cu, 0x3154A0u, 0x315560u})
        injector::MakeCALL(site, trafficCorona.address());
    // These native LUI/MTC1 pairs construct 80.0f without a low half. A single
    // queued LUI immediate changes the cap to 800.0f, without a trampoline.
    injector::WriteMemory<uint32_t>(0x3EA644, 0x3C014448);
    injector::WriteMemory<uint32_t>(0x3EA690, 0x3C014448);
}
void InstallStars() {
    if (!settings.lights&&!settings.traffic) BindRenderer();
    for (unsigned side = 0; side < 5; ++side) {
        for (unsigned i = 0; i < 100; ++i) {
            float x = Random(-95.0f, 95.0f);
            float y = side == 4 ? Random(-95.0f, 95.0f) : Random(-33.25f, 95.0f);
            Vector offset;
            switch (side) {
            case 0: offset = {100.0f, -x, 10.0f + y}; break;
            case 1: offset = {-100.0f, -x, 10.0f + y}; break;
            case 2: offset = {-x, 100.0f, 10.0f + y}; break;
            case 3: offset = {-x, -100.0f, 10.0f + y}; break;
            default: offset = {x, y, 95.0f}; break;
            }
            float maximum = Random(0.0f, 1.0f) < settings.largeChance ? settings.largeStars : settings.mediumStars;
            stars[side * 100 + i] = {offset, 0.8f * Random(settings.smallStars, maximum)};
        }
    }
    starCallback.bind(RenderStars);
    injector::MakeCALL(0x18D450, starCallback.address());
}
}
