#pragma once
#include "Game.hpp"
#include "../Shared/Console/PCButtonArtwork.hpp"
namespace vcs {
void InstallButtonIcons();
// PC-scheme text for help entries that name PS2 buttons (CText::Get hook);
// nullptr keeps the game's text.
const uint16_t* ButtonText(const char* key);
// Key artwork for a binding code (Windows virtual key / mouse code); returns
// the drawn width, or 0 when the key has no artwork.
float KeyIconWidth(uint8_t key, float height);
float DrawKeyIcon(uint8_t key, float x, float y, float height, const Color& color);
}
