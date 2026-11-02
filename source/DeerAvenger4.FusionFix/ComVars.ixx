module;

#include <stdafx.h>

export module ComVars;

export GameRef<volatile int> game_worldState;
export GameRef<volatile int> game_demoState;
export GameRef<volatile int> game_levelState;
export GameRef<volatile int> game_transitionState;
export GameRef<volatile int> game_consoleState;
export GameRef<volatile int> game_hudBlock1;
export GameRef<volatile int> game_hudBlock2;
export GameRef<volatile int> game_menuOpen;
export GameRef<volatile int> game_menuActive;
export GameRef<volatile int> game_menuPage;
export GameRef<volatile int> game_menuSelection;
export GameRef<volatile int> game_introDone;
export GameRef<volatile int> game_matrixIndex;
export GameRef<volatile int> game_matrixDirty;
export GameRef<volatile int> game_matrixUpdate;
export GameRef<void* volatile> game_world;
export GameRef<void* volatile> game_camera;
export GameRef<void* volatile> game_engine;
export GameRef<void* volatile> game_viewportCamera;
export GameRef<volatile float> game_viewportX;
export GameRef<volatile float> game_viewportY;
export GameRef<volatile float> game_viewportWidth;
export GameRef<volatile float> game_viewportHeight;
export GameRef<volatile float> game_viewOffsetX;
export GameRef<volatile float> game_viewOffsetY;
export GameRef<volatile float> game_viewAspect;
export GameRef<volatile float> game_viewScale;
export GameRef<uint8_t> game_menuCommands;
export GameRef<uint8_t> game_matrixStack;

export GameRef<void*> cameraPrimary;
export GameRef<void*> cameraCanvas;
export GameRef<int> game_scope;
export GameRef<int> game_scripted;
export GameRef<int> game_followBlocked;
export GameRef<int> game_controlBlocked;
export GameRef<int> game_extensionOffset;
export GameRef<int> game_windowWidth;
export GameRef<int> game_windowHeight;
export GameRef<int> game_modeCount;
export GameRef<uintptr_t> game_context;
export GameRef<void*> game_selected;
export GameRef<void*> game_modes;

export namespace DeerMenu
{
    // Game globals and functions, found by pattern on first use.
    struct Addresses
    {
        // Game state flags
        uintptr_t worldState, demoState, levelState, transitionState, consoleState;
        // Nonzero while the game hides its gameplay HUD
        uintptr_t hudBlock1, hudBlock2;
        uintptr_t menuOpen, menuActive, menuPage, menuSelection, menuCommands, introDone;
        // Scene objects
        uintptr_t world, camera, engine, viewportCamera;
        // Viewport: size, offset, aspect and the matrix stack
        uintptr_t viewportX, viewportY, viewportWidth, viewportHeight;
        uintptr_t viewOffsetX, viewOffsetY, viewAspect, viewScale;
        uintptr_t matrixIndex, matrixStack, matrixDirty, matrixUpdate;
        // HUD sprite table
        uintptr_t sprites;
        // Functions
        uintptr_t title, key, menu, menuDraw, draw, frame, app;
        uintptr_t createTypeface, destroyTypeface, drawText;
    };

    inline const Addresses& Game()
    {
        static const Addresses addresses = []()
        {
            auto data = [](const char* pattern, ptrdiff_t operand)
            {
                return *hook::get_pattern<uintptr_t>(pattern, operand);
            };
            auto code = [](const char* pattern)
            {
                return reinterpret_cast<uintptr_t>(hook::get_pattern(pattern));
            };
            Addresses a{};
            a.worldState = data("A1 ? ? ? ? 53 48", 1);
            game_worldState.SetAddress(a.worldState);
            a.demoState = data("89 0D ? ? ? ? 55", 2);
            game_demoState.SetAddress(a.demoState);
            a.levelState = data("83 3D ? ? ? ? 03", 2);
            game_levelState.SetAddress(a.levelState);
            a.transitionState = data("39 2D ? ? ? ? 75 0A", 2);
            game_transitionState.SetAddress(a.transitionState);
            a.consoleState = data("8B 0D ? ? ? ? 85 C9 74 18", 2);
            game_consoleState.SetAddress(a.consoleState);
            a.hudBlock1 = data("39 35 ? ? ? ? 75 42", 2);
            game_hudBlock1.SetAddress(a.hudBlock1);
            a.hudBlock2 = data("A1 ? ? ? ? 74 37", 1);
            game_hudBlock2.SetAddress(a.hudBlock2);
            a.menuOpen = data("39 2D ? ? ? ? 75 09", 2);
            game_menuOpen.SetAddress(a.menuOpen);
            a.menuActive = data("A1 ? ? ? ? 85 C0 75 1C", 1);
            game_menuActive.SetAddress(a.menuActive);
            a.menuPage = data("89 3D ? ? ? ? 6A 01", 2);
            game_menuPage.SetAddress(a.menuPage);
            a.menuSelection = data("89 35 ? ? ? ? F7 D8", 2);
            game_menuSelection.SetAddress(a.menuSelection);
            a.menuCommands = data("39 35 ? ? ? ? 0F 84 ? ? ? ? 33 F6 56 56 56 6A 0E", 2);
            game_menuCommands.SetAddress(a.menuCommands);
            a.introDone = data("6A 00 6A 1A C7 05 ? ? ? ? 00 00 00 00 E8 ? ? ? ? 6A 00", 6);
            game_introDone.SetAddress(a.introDone);
            a.world = data("A1 ? ? ? ? 6A 0D", 1);
            game_world.SetAddress(a.world);
            a.camera = data("8B 2D ? ? ? ? 51", 2);
            game_camera.SetAddress(a.camera);
            a.engine = data("A1 ? ? ? ? 57 56", 1);
            game_engine.SetAddress(a.engine);
            a.viewportCamera = data("8B 1D ? ? ? ? 8B EA", 2);
            game_viewportCamera.SetAddress(a.viewportCamera);
            a.viewportX = data("D9 05 ? ? ? ? D9 C9 33 C0", 2);
            game_viewportX.SetAddress(a.viewportX);
            a.viewportY = data("D9 1D ? ? ? ? 8B 43 1C", 2);
            game_viewportY.SetAddress(a.viewportY);
            a.viewportWidth = data("8B 0D ? ? ? ? 89 44 24 38", 2);
            game_viewportWidth.SetAddress(a.viewportWidth);
            a.viewportHeight = data("D9 05 ? ? ? ? D8 74 24 70", 2);
            game_viewportHeight.SetAddress(a.viewportHeight);
            a.viewOffsetX = data("89 3D ? ? ? ? 89 35 ? ? ? ? A3 ? ? ? ?", 2);
            game_viewOffsetX.SetAddress(a.viewOffsetX);
            a.viewOffsetY = data("89 35 ? ? ? ? A3 ? ? ? ? A3 ? ? ? ? 89 2D ? ? ? ?", 2);
            game_viewOffsetY.SetAddress(a.viewOffsetY);
            a.viewAspect = data("89 2D ? ? ? ? 89 1D ? ? ? ? 83 C4 7C", 2);
            game_viewAspect.SetAddress(a.viewAspect);
            a.viewScale = data("89 1D ? ? ? ? 83 C4 7C", 2);
            game_viewScale.SetAddress(a.viewScale);
            a.matrixIndex = data("A1 ? ? ? ? 83 F8 1F", 1);
            game_matrixIndex.SetAddress(a.matrixIndex);
            a.matrixStack = data("8B 04 85 ? ? ? ? 8B E8", 3);
            game_matrixStack.SetAddress(a.matrixStack);
            a.matrixDirty = data("A3 ? ? ? ? DC CC", 1);
            game_matrixDirty.SetAddress(a.matrixDirty);
            a.matrixUpdate = data("A3 ? ? ? ? 83 C4 08 8B C3", 1);
            game_matrixUpdate.SetAddress(a.matrixUpdate);
            a.sprites = data("8D 8F ? ? ? ? 50", 2);
            a.title = code("8B 44 24 08 81 EC 9C 02 00 00");
            // The next handler has the same code; the padding after it tells them apart
            a.key = code("A1 ? ? ? ? 85 C0 74 14 8B 44 24 08 8B 4C 24 04 50 51 FF 15 ? ? ? ? 83 C4 08 C3 B8 02 00 00 00 C3 90 90 90 90 90 90 90 90 90 90 90 90 90 A1");
            a.menu = code("A1 ? ? ? ? 83 EC 50 53 55 56 57 BF 0A 00 00 00 3B C7 0F 87");
            a.menuDraw = code("A1 ? ? ? ? 83 EC 08 57");
            a.draw = code("A1 ? ? ? ? 56 57 8B 3D ? ? ? ?");
            a.frame = code("6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 81 EC 14 02 00 00");
            a.app = code("8B 44 24 04 81 EC 80 01 00 00");
            a.createTypeface = code("57 56 55 53 8B 6C 24 14 83 EC 1C 33 FF");
            a.destroyTypeface = code("8B 54 24 04 83 EC 14 85 D2");
            a.drawText = code("57 56 55 53 8B 6C 24 18 83 EC 5C");
            return a;
        }();
        return addresses;
    }
}

