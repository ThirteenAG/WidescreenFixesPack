# Native PC button artwork

These PNGs are the button images supplied for the console plugins. The generated
header is checked in, so building the plugins does not require Python or Pillow.

To regenerate `source/Shared/Console/PCButtonArtwork.hpp`, run
`python resources/pc-buttons/generate.py` with Pillow installed. The generator
pads textures to power-of-two dimensions and stores their original content width
so wide keycaps retain their proportions.

The generator stores each texture in its final PS2 form: 8-bit pixels followed by a
256-entry CLUT in the GS CSM1 order with the PS2 alpha range, so the plugins hand
it to the native raster code without a runtime copy. A virtual-key table maps
bindable keys and mouse buttons to their artwork; keys without artwork are shown
as text. Texture memory remains owned by the plugin throughout its lifetime.
