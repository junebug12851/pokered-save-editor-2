# Tileset Images

There are several tilesets in the game, some have a different name to the same 
tileset. To further complicate things, the tileset space is large and mostly
empty and filled with trash. Added to that, font characters overwrite or empty
out a large area of the tileset and the text engine often blocks access to 
several special purpose tiles. This is compounded by the outdoor animations
overwriting 2 tiles in specific places forming an animation and the water
animation is often different for each tileset.

To simplify things, I've created several pngs containing all the different 
tileset names even if the underlying data is the same. These pngs are the size
of the full vram area. Given the white tiles are often trash, for simplicity
I keep them white.

I've also created a font overlay which can be perfectly overlayed on top of the
tileset and is also the same size as vram. It correctly whites out the tiles 
that are innaccesible and overlays fonts on the other in the correct positions.
Much of the font data changes and can be trash but we keep things simple.

There is also an 8-frame animation that happens when the tileset is marked
as an "outdoors" tileset.

 * Tile 0x03 is replaced with an animated flower that loops 3 frames, it stops
   and holds an extra frame and then repeats.
 * Tile 0x14 is shifted 4 times to the left and then back again.

This forms a total of 8 frames and is intended for outdoors because tile 0x14 
is meant to be the water tile hence the shifting abck and forth.

outside tiles 1-7 are provided for the flower to be overlaid in the correct 
position. The water tile will have to be dynamically created.
