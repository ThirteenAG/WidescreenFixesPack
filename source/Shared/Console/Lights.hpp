#pragma once
#include <cstddef>
#include <cstdint>

namespace console {
struct Vector3 { float x, y, z; };
struct LightColor { uint8_t r, g, b, a; };
struct LightData {
    Vector3 position;
    float size, nativeRange, orientation;
    LightColor color;
    uint8_t blink, noDistance;
};

inline float saturate(float value) { return value > 0.0f ? (value < 1.0f ? value : 1.0f) : 0.0f; }
inline float distanceSquared(const Vector3& a, const Vector3& b) {
    float x = a.x - b.x, y = a.y - b.y, z = a.z - b.z;
    return x * x + y * y + z * z;
}
inline float nightIntensity(unsigned hour, unsigned minute) {
    unsigned time = (hour % 24) * 60 + minute % 60;
    if (time >= 19 * 60) return float(time - 19 * 60) / 300.0f;
    if (time < 3 * 60) return 1.0f;
    return saturate(float(7 * 60 - int(time)) / 240.0f);
}
inline float starIntensity(unsigned hour, unsigned minute) {
    unsigned time = (hour % 24) * 60 + minute % 60;
    if (time >= 23 * 60 || time < 5 * 60) return 1.0f;
    if (time >= 22 * 60) return float(time - 22 * 60) / 60.0f;
    return saturate(float(6 * 60 - int(time)) / 60.0f);
}
// One phase per mode, evaluated once per frame. Light count and frame rate do
// not change the duty cycle. Smooth edges avoid abrupt on/off popping.
inline float blinkIntensity(unsigned mode, uint32_t milliseconds) {
    constexpr uint16_t on[] = {0, 500, 1000, 2000, 3000, 4000, 5000, 6000};
    constexpr uint16_t off[] = {0, 500, 1000, 2000, 3000, 4000, 5000, 4000};
    if (!mode || mode >= sizeof(on) / sizeof(on[0])) return 1.0f;
    uint32_t phase = milliseconds % (on[mode] + off[mode]);
    float value = phase < on[mode] ? float(phase) / 250.0f : 1.0f - float(phase - on[mode]) / 250.0f;
    value = saturate(value);
    return value * value * (3.0f - 2.0f * value);
}
inline bool trafficLight(const LightData& light) { return light.size == 0.45f; }
inline unsigned trafficColor(const LightColor& color) {
    if (color.r >= 250 && color.g >= 100 && color.b <= 100) return 1;
    if (color.r >= 250 && color.g < 100 && color.b == 0) return 2;
    return color.r == 0 && color.g >= 250 && color.b == 0 ? 0 : 3;
}

// Module-owned index: six bytes per light plus the hash table. Buckets include
// cell coordinates, so hash collisions never duplicate or omit candidates.
template<size_t Count, size_t Buckets = 1024> class LightGrid {
    static_assert(Count < 65535 && Buckets && !(Buckets & (Buckets - 1)));
    static constexpr uint16_t end = 65535;
    static constexpr float cellSize = 256.0f;
    struct Link { uint16_t next; int16_t x, y; };
    uint16_t heads_[Buckets]{};
    Link links_[Count]{};
    static int cell(float value) {
        float scaled = value / cellSize;
        int truncated = int(scaled);
        return truncated - (scaled < float(truncated));
    }
    static size_t hash(int x, int y) {
        return (uint32_t(x) * 73856093u ^ uint32_t(y) * 19349663u) & (Buckets - 1);
    }
public:
    void initialize(const LightData (&lights)[Count]) {
        for (auto& head : heads_) head = end;
        for (size_t i = 0; i < Count; ++i) {
            int x = cell(lights[i].position.x), y = cell(lights[i].position.y);
            size_t bucket = hash(x, y);
            links_[i] = {heads_[bucket], int16_t(x), int16_t(y)};
            heads_[bucket] = uint16_t(i);
        }
    }
    template<class Visitor> void visit(const Vector3& center, float range, Visitor visitor) const {
        // Game positions are finite. A corrupted camera must not turn this
        // bounded lookup into an unbounded cell walk or a float-to-int overflow.
        if (!(range >= 0.0f && range <= 8192.0f) ||
            !(center.x > -100000.0f && center.x < 100000.0f &&
              center.y > -100000.0f && center.y < 100000.0f)) return;
        int minX = cell(center.x - range), maxX = cell(center.x + range);
        int minY = cell(center.y - range), maxY = cell(center.y + range);
        for (int y = minY; y <= maxY; ++y) {
            for (int x = minX; x <= maxX; ++x) {
                for (uint16_t i = heads_[hash(x, y)]; i != end; i = links_[i].next)
                    if (links_[i].x == x && links_[i].y == y) visitor(i);
            }
        }
    }
};

// Select nearest visible candidates, rather than whichever entries happen to
// come first in the data. The caller owns the fixed output array; no allocation.
template<class Candidate, size_t Capacity> class NearestLights {
    Candidate items_[Capacity]{};
    size_t count_ = 0, limit_ = Capacity;
    void descend(size_t parent) {
        Candidate value = items_[parent];
        for (size_t child = parent * 2 + 1; child < count_; child = parent * 2 + 1) {
            if (child + 1 < count_ && items_[child + 1].distance > items_[child].distance) ++child;
            if (items_[child].distance <= value.distance) break;
            items_[parent] = items_[child]; parent = child;
        }
        items_[parent] = value;
    }
public:
    void clear(size_t limit) { count_ = 0; limit_ = limit < Capacity ? limit : Capacity; }
    void insert(const Candidate& item) {
        if (!(item.distance >= 0.0f) || !limit_) return;
        if (count_ == limit_) {
            if (item.distance >= items_[0].distance) return;
            items_[0] = item; descend(0); return;
        }
        size_t child = count_++;
        while (child && items_[(child - 1) / 2].distance < item.distance) {
            items_[child] = items_[(child - 1) / 2]; child = (child - 1) / 2;
        }
        items_[child] = item;
    }
    const Candidate* begin() const { return items_; }
    const Candidate* end() const { return items_ + count_; }
    size_t size() const { return count_; }
};
}
