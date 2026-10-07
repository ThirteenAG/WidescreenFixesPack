#pragma once
#include "PSP.hpp"
#include "Lights.hpp"

namespace console::portable {
// Native VFPU projection reads and writes an entire quad, including padding.
struct alignas(16) ProjectedPoint { float x, y, z, padding; };
struct LightRenderer {
    uintptr_t camera, clockHour, clockMinute, timer, fog, cloud, texture, screenWidth, screenHeight;
    int (*project)(const ProjectedPoint*, ProjectedPoint*, float*, float*, bool);
    void (*sprite)(float, float, float, float, float, uint8_t, uint8_t, uint8_t, int16_t, float, uint8_t);
    int (*state)(int, uintptr_t);
    void (*flush)();
    int (*signals[2])();
    unsigned zTest, zWrite, srcBlend, destBlend;
    float radius, range, nearFraction;
    unsigned limit;
    Vector3 position() const { return *reinterpret_cast<const Vector3*>(camera); }
    uintptr_t raster() const {
        auto corona = *reinterpret_cast<const uint32_t*>(texture);
        return corona ? *reinterpret_cast<const uint32_t*>(corona) : 0;
    }
    unsigned hour() const { return *reinterpret_cast<const uint8_t*>(clockHour); }
    unsigned minute() const { return *reinterpret_cast<const uint8_t*>(clockMinute); }
};

// Buffered sprites copy 4 vertices (64 bytes) and 6 indices into the game's
// per-frame GE buffer, which the game never bounds-checks: its own corona count
// was small. Commands grow up from the head and data down from the tail.
// Keep a reserve for the rest of the native frame (particles, HUD, text).
inline unsigned SpriteBudget(uintptr_t headAddress, uintptr_t tailAddress, uint32_t reserve = 96 * 1024) {
    const uint32_t head = *reinterpret_cast<const uint32_t*>(headAddress);
    const uint32_t tail = *reinterpret_cast<const uint32_t*>(tailAddress);
    if (!head || tail <= head || tail - head <= reserve) return 0;
    return (tail - head - reserve) / 96;
}
inline float coronaDepth(float z) {
    const float bias = 2.0f + 0.04f * z;
    return bias < z * 0.5f ? z - bias : z * 0.5f;
}
template<size_t Count> class StoryLights {
    const LightData (&lights_)[Count];
    LightRenderer renderer_{};
    LightGrid<Count> grid_;
    uint8_t trafficGroups_[Count]{};
    struct Visible {
        float distance;
        ProjectedPoint screen;
        float width, height;
        uint16_t index;
        uint8_t alpha;
    };
    NearestLights<Visible, 2048> visible_;
    static unsigned TrafficGroup(float x, float y) {
        if (y < 0.0f) { x = -x; y = -y; }
        return 0.5f * y - 0.8660254f * x > 0.0f &&
               -0.8660254f * y - 0.5f * x < 0.0f ? 1 : 2;
    }
public:
    explicit StoryLights(const LightData (&lights)[Count]) : lights_(lights) {}
    void initialize(const LightRenderer& renderer, bool verticallyStacked) {
        renderer_ = renderer;
        grid_.initialize(lights_);
        for (size_t i = 0; i < Count; ++i) {
            if (!trafficLight(lights_[i])) continue;
            float x, y;
            if (verticallyStacked) {
                float z = lights_[i].orientation;
                float w = __builtin_sqrtf(1.0f - z * z > 0.0f ? 1.0f - z * z : 0.0f);
                x = -2.0f * z * w; y = 1.0f - 2.0f * z * z;
            } else {
                float nearest = 1.0f;
                uint16_t pair = uint16_t(i);
                unsigned own = trafficColor(lights_[i].color);
                grid_.visit(lights_[i].position, 1.0f, [&](uint16_t candidate) {
                    const auto& other = lights_[candidate];
                    unsigned color = trafficColor(other.color);
                    if (!trafficLight(other) || other.orientation != lights_[i].orientation ||
                        (own == 0 ? color != 2 : color != 0)) return;
                    float distance = distanceSquared(other.position, lights_[i].position);
                    if (distance < nearest) { nearest = distance; pair = candidate; }
                });
                x = lights_[pair].position.y - lights_[i].position.y;
                y = lights_[i].position.x - lights_[pair].position.x;
            }
            trafficGroups_[i] = uint8_t(TrafficGroup(x, y));
        }
    }
    void learnTraffic(const ProjectedPoint* position, unsigned group) {
        if (!position || group < 1 || group > 2) return;
        Vector3 center{position->x, position->y, position->z};
        grid_.visit(center, 0.5f, [&](uint16_t i) {
            if (trafficLight(lights_[i]) && distanceSquared(lights_[i].position, center) < 0.25f)
                trafficGroups_[i] = uint8_t(group);
        });
    }
    // budget: sprites that fit in the native frame buffer (see SpriteBudget).
    void render(unsigned budget = ~0u) {
        auto& r = renderer_;
        uintptr_t raster = r.raster();
        if (!raster || !budget) return;
        Vector3 camera = r.position();
        float night = nightIntensity(r.hour(), r.minute());
        float phases[8];
        uint32_t milliseconds = *reinterpret_cast<const uint32_t*>(r.timer);
        for (unsigned i = 0; i < 8; ++i) phases[i] = blinkIntensity(i, milliseconds);
        unsigned signals[] = {3, unsigned(r.signals[0]()), unsigned(r.signals[1]())};
        float maxSquared = r.range * r.range;
        float screenWidth = float(*reinterpret_cast<const uint32_t*>(r.screenWidth));
        float screenHeight = float(*reinterpret_cast<const uint32_t*>(r.screenHeight));
        visible_.clear(r.limit < budget ? r.limit : budget);
        grid_.visit(camera, r.range, [&](uint16_t i) {
            const auto& light = lights_[i];
            float distance = distanceSquared(light.position, camera);
            // The range is horizontal: raising the camera must not remove the
            // lights below it (sizes still follow the true distance).
            const float hx = light.position.x - camera.x, hy = light.position.y - camera.y;
            const float horizontal = __builtin_sqrtf(hx * hx + hy * hy);
            if (!(horizontal * horizontal < maxSquared)) return;
            float intensity = trafficLight(light)
                ? (signals[trafficGroups_[i]] == trafficColor(light.color) ? 1.0f : 0.0f)
                : night * phases[light.blink < 8 ? light.blink : 0];
            if (!(intensity > 0.0f) || !light.color.a) return;
            float nearRange = light.nativeRange * r.nearFraction;
            if (!light.noDistance && distance <= nearRange * nearRange) return;
            ProjectedPoint position{light.position.x, light.position.y, light.position.z, 0}, screen;
            float width, height;
            if (!r.project(&position, &screen, &width, &height, false)) return;
            distance = __builtin_sqrtf(distance);
            float radius = light.noDistance ? 3.5f : 3.5f * saturate(
                (distance - nearRange) / (light.nativeRange - nearRange));
            radius *= light.size * r.radius * bounded(0.0025f * distance + 0.25f, 0.0f, 4.0f, 1.0f);
            width *= radius; height *= radius;
            if (!(screen.z > 0.0f && width > 0.0f && height > 0.0f) ||
                screen.x + width < 0.0f || screen.x - width > screenWidth ||
                screen.y + height < 0.0f || screen.y - height > screenHeight) return;
            intensity *= saturate((r.range - horizontal) / (r.range * 0.1f));
            unsigned alpha = unsigned(intensity * light.color.a);
            if (alpha) visible_.insert({distance * distance, screen, width, height, i, uint8_t(alpha)});
        });
        if (!visible_.size()) return;
        r.flush();
        r.state(r.zWrite, 0); r.state(r.zTest, 1);
        r.state(r.srcBlend, 2); r.state(r.destBlend, 2); r.state(1, raster);
        for (const auto& item : visible_) {
            const auto& color = lights_[item.index].color;
            // Coronas are flat screen sprites at the lamp's depth. Seen from above, the
            // ground or facade behind a lamp is nearly as deep, so part of the sprite
            // failed the depth test (cut in half) and, higher up, all of it. Draw them
            // slightly in front of the lamp (as native coronas do), scaled with depth.
            const float z = coronaDepth(item.screen.z);
            r.sprite(item.screen.x, item.screen.y, z, item.width, item.height,
                color.r, color.g, color.b, item.alpha, 1.0f / z, item.alpha);
        }
        r.flush();
        // Verified postconditions of CCoronas::Render, before CParticle::Render.
        r.state(r.zWrite, 1); r.state(r.zTest, 1);
        r.state(r.srcBlend, 5); r.state(r.destBlend, 6); r.state(1, 0);
    }
};

class StoryStars {
    struct Star { ProjectedPoint offset; float size; } stars_[500]{};
    LightRenderer renderer_{};
    uint32_t seed_ = 0x53544152;
    float random(float low, float high) {
        seed_ ^= seed_ << 13; seed_ ^= seed_ >> 17; seed_ ^= seed_ << 5;
        return low + (high - low) * float(seed_ & 0xFFFFFFu) / 16777216.0f;
    }
public:
    void initialize(const LightRenderer& renderer) {
        renderer_ = renderer;
        float small = bounded(inireader.ReadFloat("STARS", "SmallestStarsSize", 0.15f), 0.03f, 2.5f, 0.15f);
        float medium = bounded(inireader.ReadFloat("STARS", "MiddleStarsSize", 0.6f), small, 2.5f, small);
        float big = bounded(inireader.ReadFloat("STARS", "BiggestStarsSize", 1.2f), medium, 2.5f, medium);
        float chance = bounded(inireader.ReadFloat("STARS", "BiggestStarsChance", 20.0f), 0.0f, 100.0f, 20.0f) * 0.01f;
        for (unsigned side = 0; side < 5; ++side) {
            for (unsigned i = 0; i < 100; ++i) {
                float x = random(-95.0f, 95.0f), y = random(side == 4 ? -95.0f : -33.25f, 95.0f);
                ProjectedPoint offset;
                switch (side) {
                case 0: offset = {100.0f, -x, 10.0f + y, 0}; break;
                case 1: offset = {-100.0f, -x, 10.0f + y, 0}; break;
                case 2: offset = {-x, 100.0f, 10.0f + y, 0}; break;
                case 3: offset = {-x, -100.0f, 10.0f + y, 0}; break;
                default: offset = {x, y, 95.0f, 0}; break;
                }
                stars_[side * 100 + i] = {offset, 0.8f * random(small, random(0, 1) < chance ? big : medium)};
            }
        }
    }
    void render(unsigned budget = ~0u) {
        auto& r = renderer_;
        r.flush();
        if (!budget) return;
        float fog = *reinterpret_cast<const float*>(r.fog), cloud = *reinterpret_cast<const float*>(r.cloud);
        unsigned brightness = unsigned(255.0f * starIntensity(r.hour(), r.minute()) *
            (1.0f - saturate(fog > cloud ? fog : cloud)));
        uintptr_t raster = r.raster();
        if (!brightness || !raster) return;
        r.state(1, raster);
        Vector3 camera = r.position();
        float screenWidth = float(*reinterpret_cast<const uint32_t*>(r.screenWidth));
        float screenHeight = float(*reinterpret_cast<const uint32_t*>(r.screenHeight));
        for (const auto& star : stars_) {
            ProjectedPoint position{camera.x + star.offset.x, camera.y + star.offset.y, camera.z + star.offset.z, 0}, screen;
            float width, height;
            if (!r.project(&position, &screen, &width, &height, false) || !(screen.z > 0.0f)) continue;
            width *= star.size; height *= star.size;
            if (screen.x + width < 0.0f || screen.x - width > screenWidth ||
                screen.y + height < 0.0f || screen.y - height > screenHeight) continue;
            r.sprite(screen.x, screen.y, screen.z, width, height, uint8_t(brightness), uint8_t(brightness),
                uint8_t(brightness), 255, 1.0f / screen.z, 255);
            if (!--budget) break;
        }
        r.flush();
    }
};
}
