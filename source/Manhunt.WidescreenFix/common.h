#pragma once

#define DEFAULT_SCREEN_WIDTH (640)
#define DEFAULT_SCREEN_HEIGHT (480)

#define DEFAULT_ASPECT_RATIO (4.0f / 3.0f)

// The original HUD keeps round things round with this width-to-height factor of the 640x480 design
#define DEFAULT_HUD_STRETCH (0.75f)

#define PI (float)M_PI
#define DEGTORAD(x) ((x) * PI / 180.0f)
#define RADTODEG(x) ((x) * 180.0f / PI)
