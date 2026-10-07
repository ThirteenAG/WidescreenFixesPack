#pragma once
#include "../../external/injector/include/ps2/runtime.hpp"
#include "../../external/injector/include/ps2/game_abi.hpp"
#include "../Shared/Console/Viewport.hpp"
#include <cstdint>

namespace vcs {
using Rect = console::Rect;
struct Color { uint8_t red, green, blue, alpha; };
struct Vertex {
    int16_t u, v;
    Color color;
    int16_t x, y;
    float z;
};
static_assert(sizeof(Vertex) == 16 && offsetof(Vertex, x) == 8);
struct Sprite { void* texture; };

struct Settings {
    bool widescreen = false, skipIntro = true, sixtyFPS = false;
    bool unthrottle = true, modernControls = true, pcControls = false;
    float aspect = 4.0f / 3.0f, baseAspect = 4.0f / 3.0f, hudScale = 1.0f;
    float mouseSensitivity = 0.002f;
    bool invertMouse = false, cutsceneBorders = true;
};
inline Settings settings;

namespace address {
constexpr uintptr_t currentArea = 0x489F7C;
constexpr uintptr_t fov = 0x487484, aspect = 0x487488;
constexpr uintptr_t vertices = 0x72A510;
constexpr uintptr_t sizeFracX = 0x48A0D8, sizeFracY = 0x48A0DC;
constexpr uintptr_t wideScreenPreference = 0x2AF658;
constexpr uintptr_t camera = 0x6F44D0;
}

void InstallCamera();
void InstallAim();
void InstallDrawing();
void InstallHud();
void InstallMenu();
void ApplyFrameRate();

enum class DrawMode { None, Center, LeftBottom, RightTop, RightBottom, World };
inline DrawMode drawMode = DrawMode::None;
inline bool textureDraw = false;
inline bool worldText = false;
inline console::Point worldTextAnchor{};
struct DrawScope {
    DrawMode saved;
    explicit DrawScope(DrawMode mode) : saved(drawMode) { drawMode = mode; }
    ~DrawScope() { drawMode = saved; }
};
struct TextureScope {
    bool saved;
    explicit TextureScope(bool value) : saved(textureDraw) { textureDraw = value; }
    ~TextureScope() { textureDraw = saved; }
};
console::Transform DrawingTransform(bool physical, float centerX = 0.0f, float centerY = 0.0f);
Rect CorrectRect(Rect value, bool physical);
void CorrectVertices(Vertex* vertices, unsigned count);
}
