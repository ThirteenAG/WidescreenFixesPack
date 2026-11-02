#pragma once

// The developer overlay draws on the Direct3D 9 device that the loader's d3d8to9 bridge creates for the game
void InitDeveloperOverlay();
void DrawDeveloperOverlay(HWND window);
bool DeveloperOverlayMessage(HWND window, UINT message, WPARAM wParam, LPARAM lParam, LRESULT& result);
