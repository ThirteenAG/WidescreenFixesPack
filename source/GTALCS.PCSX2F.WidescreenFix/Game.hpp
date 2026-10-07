#pragma once
#include "../../external/injector/include/ps2/runtime.hpp"
#include "../../external/injector/include/ps2/game_abi.hpp"
#include "../Shared/Console/Viewport.hpp"
#include <cstdint>

namespace lcs {
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
    bool widescreen = false, sixtyFPS = false, unthrottle = true, pcControls = false;
    float aspect = 4.0f / 3.0f, baseAspect = 4.0f / 3.0f, hudScale = 1.0f;
    float mouseSensitivity = 0.002f;
    bool invertMouse = false, cutsceneBorders = true, disableBlur = false;
};
inline Settings settings;
namespace address {
constexpr uintptr_t currentArea = 0x3D8430, vertices = 0x65FFE0;
constexpr uintptr_t wideScreenPreference = 0x3D8B8A;
}
void InstallDrawing();
void InstallHud();
void InstallFrontend();
void InstallCamera();
void InstallAim();
void UpdateViewport();
// Applies settings.disableBlur (menu toggle).
void ApplyBlur();
void ApplyFrameRate();
// True for text in the plugin's own menu caption/value buffers.
bool MenuCaption(uintptr_t text);

enum class DrawMode { None, Center, LeftBottom, RightTop, RightBottom, World, Frontend };
inline DrawMode drawMode = DrawMode::None;
inline bool textureDraw = false;
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
