#pragma once
#include "Game.hpp"
#include "../Shared/Console/PCButtonArtwork.hpp"
namespace lcs {
// PC-scheme text for help entries that name PS2 buttons in plain words
// (CText::Get hook); nullptr keeps the game's text.
const uint16_t* ButtonText(const char* key);
// Key artwork for a binding code (Windows virtual key / mouse code), in the
// frontend's 480x272 units. Returns the drawn width, or 0 without artwork.
// A positive width overrides the artwork's proportions.
float KeyIconWidth(uint8_t key, float height);
float DrawKeyIcon(uint8_t key, float x, float y, float height, const Color& color, float width = 0.0f);
}
