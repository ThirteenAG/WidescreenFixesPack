#include "Game.hpp"
#include "Lights.hpp"

namespace lcsfx {
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
using CoronaSignature = uint64_t(uint32_t, uint8_t, uint8_t, uint8_t, uint8_t, const Vector*,
    uint8_t, uint8_t, float, float, float, float, uint8_t, uint8_t, uint8_t, int32_t);
pcsx2::GameFunction<CoronaSignature> registerCorona;
pcsx2::GameCallback<CoronaSignature> trafficCorona;
pcsx2::GameCallback<void()> lightCallback;
safetymips::GameInline<void(void*)> trafficDisplay;
safetymips::GameInline<uint64_t(void*)> trafficType;
unsigned activeTrafficGroup;
bool learningTrafficGroup;

uintptr_t coronaRaster() {
    auto texture = read<uint32_t>(0x392D64);
    return texture ? read<uint32_t>(texture) : 0;
}
void BindRenderer() {
    project.bind(0x2DFD70);
    draw.bind(0x2E0048);
    renderState.bind(0x144B58);
    flush.fun = reinterpret_cast<void (*)()>(0x2DFF08);
}
// Opposite faces share a phase. These inequalities implement the native
// FindTrafficLightType 60..150 degree sectors without atan2 on every map light.
unsigned TrafficGroup(float redToGreenX, float redToGreenY) {
    float x = redToGreenY, y = -redToGreenX;
    if (y < 0.0f) { x = -x; y = -y; }
    return 0.5f * y - 0.8660254f * x > 0.0f &&
           -0.8660254f * y - 0.5f * x < 0.0f ? 1 : 2;
}
void InitializeTrafficGroups() {
    for (size_t i = 0; i < lightCount; ++i) {
        // LCS lamps are stacked vertically, so their orientation comes from the
        // IPL quaternion z component. Native forward is (-2zw, 1-2z*z).
        float z = lights[i].orientation;
        float w = __builtin_sqrtf(1.0f - z * z > 0.0f ? 1.0f - z * z : 0.0f);
        float x = -2.0f * z * w, y = 1.0f - 2.0f * z * z;
        trafficGroups[i] = uint8_t(TrafficGroup(-y, x));
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
    int32_t flag4) {
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
                          angle, pull, reflection, los, streak, flag4);
}
void RenderLights() {
    renderCoronas.fun();
    if (settings.traffic) RenderTraffic();
    if (!settings.lights) return;
    const Vector camera = cameraPosition();
    float night = console::nightIntensity(hour(), minute());
    float phases[8];
    for (unsigned mode = 0; mode < 8; ++mode)
        phases[mode] = console::blinkIntensity(mode, read<uint32_t>(0x3D9B70));
    unsigned signals[] = {3, unsigned(reinterpret_cast<uint64_t (*)()>(0x23C748)()),
                            unsigned(reinterpret_cast<uint64_t (*)()>(0x23C7A8)())};
    float maximumSquared = settings.range * settings.range;
    visible.clear(settings.limit);
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
        float nearRange = light.nativeRange * 0.5f;
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
    renderState(6, 0); renderState(4, 1); renderState(10, 1); renderState(8, 2); renderState(9, 2);
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
    // This call site follows native CCoronas::Render (0x258008), which ends
    // with raster 0, ONE/ONE blending, vertex alpha off and depth test/write on.
    renderState(1, 0); renderState(8, 2); renderState(9, 2); renderState(10, 0); renderState(6, 1); renderState(4, 1);
}

void InstallLightsInternal() {
    BindRenderer();
    // CCoronas::Render call in RenderEffects (0x1F61C8), after the world, water
    // and alpha entities, as in VCS. 0x112744 is CCoronas::RenderReflections in
    // RenderScene, before the buildings, which then painted over the coronas.
    lightCallback.bind(RenderLights);
    renderCoronas.fun = injector::MakeCALL(0x1F62F0, lightCallback.address()).get();
    if (!settings.lights) return;
    grid.initialize(lights);
    InitializeTrafficGroups();
    registerCorona.bind(0x259710);
    trafficCorona.bind(RegisterTraffic);
    trafficDisplay = safetymips::create_inline_game(0x23C838, DisplayTraffic);
    trafficType = safetymips::create_inline_game(0x23E218, FindTrafficType);
    for (uintptr_t site : {0x23CF90u, 0x23CEE8u, 0x23E164u, 0x23E0A0u})
        injector::MakeCALL(site, trafficCorona.address());
}
}
void InstallLights() { InstallLightsInternal(); }
}
