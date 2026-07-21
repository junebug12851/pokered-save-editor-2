/*
  * Copyright 2020 Fairy Fox
  *
  * Licensed under the Apache License, Version 2.0 (the "License");
  * you may not use this file except in compliance with the License.
  * You may obtain a copy of the License at
  *
  *   http://www.apache.org/licenses/LICENSE-2.0
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
*/

/**
 * @file tilesetengine.cpp
 * @brief Implementation of TilesetEngine -- tileset building + wave/flower
 *        post-processing. See tilesetengine.h.
 */

#include "./tilesetengine.h"
#include <QStringList>
#include <QColor>
#include <QVector>
#include <QPainter>

QImage TilesetEngine::getTileset(QString name)
{
  // Make lowercase and replace spaces
  QString nameFixed = name.toLower().replace(" ", "_");
  return QImage(":/assets/tilesets/" + nameFixed + ".png")
      .convertToFormat(QImage::Format::Format_ARGB32);
}

QImage TilesetEngine::getFlower(int frame)
{
  // ⚠️ FIXED 2026-07-12 against the disassembly. It used to run 2, 3, 1, 1 -- an invention.
  //
  // UpdateMovingBgTiles (home/vcopy.asm) picks the flower from the WATER's own phase counter:
  //
  //     ld a, [wMovingBGTilesCounter2]
  //     and 3
  //     cp 2
  //     ld hl, FlowerTile1
  //     jr c, .copy        ; 0 or 1 -> flower1
  //     ld hl, FlowerTile2
  //     jr z, .copy        ; 2      -> flower2
  //     ld hl, FlowerTile3 ; 3      -> flower3
  //
  // So the sequence is 1, 1, 2, 3 -- flower1 shows for TWICE as long as the other two, and the
  // flower is locked in step with the water (there is no "flower only" state, and there cannot be).
  // See notes/reference/map-animation.md.
  const int subFrame = ((frame % 4) + 4) % 4;   // negative-safe

  const int ind = (subFrame < 2) ? 1
                : (subFrame == 2) ? 2
                : 3;

  return QImage(":/assets/tilesets/_flower" + QString::number(ind) + ".png")
      .convertToFormat(QImage::Format::Format_ARGB32);
}

QImage TilesetEngine::getFont()
{
  return QImage(":/assets/tilesets/_font.png")
      .convertToFormat(QImage::Format::Format_ARGB32);
}

QPixmap TilesetEngine::buildTilesetFullDebug(QString id)
{
  auto idParts = id.split("/", Qt::SkipEmptyParts);

  // Has to have all 4 parts unconditionally
  if(idParts.size() < 4) {
    // Return error red
    auto tmp = blankImage();
    tmp.fill(QColor(255, 0, 0, 0));
    return QPixmap::fromImage(tmp);
  }

  // ── Which tiles animate ─────────────────────────────────────────────────────
  //
  // THREE states, not two. This is the game's `hTileAnimations` byte, and
  // UpdateMovingBgTiles (home/vcopy.asm) reads it like this:
  //
  //   0 (indoor  / TILEANIM_NONE)         -- returns immediately. Nothing moves.
  //   1 (cave    / TILEANIM_WATER)        -- animates the WATER tile, then resets the frame
  //                                          counter before it can ever reach the flower step.
  //   2 (outdoor / TILEANIM_WATER_FLOWER) -- lets the counter run on to 21, so the FLOWER
  //                                          tile animates as well.
  //
  // So water animates in a cave, and it always did on the console. This used to be a bool
  // ("outdoor"), which lumped Cave in with Indoor and left every cave's water dead --
  // Cerulean Cave, Seafoam, the Ship. See notes/reference/tiles.md.
  const QString type = idParts.at(1).toLower();
  const bool animatesWater  = (type == "cave" || type == "outdoor");
  const bool animatesFlower = (type == "outdoor");

  bool useFont = idParts.at(2).toLower() == "font";
  int frame = idParts.at(3).toInt();

  // Get or create blank images as needed for each layer
  auto tilesetImg = getTileset(idParts.at(0));

  auto fontImg = (useFont)
      ? getFont()
      : blankImage();
  auto flowerImg = (animatesFlower)
      ? getFlower(frame)
      : blankImage();

  // Prepare tileset image
  QImage tileset = blankImage();
  //tileset.fill(QColor("white"));

  // Stack requested and retrieved tileset layers
  QPainter p(&tileset);
  p.setCompositionMode(QPainter::CompositionMode_SourceOver);
  p.drawImage(0, 0, tilesetImg);
  p.drawImage(0, 0, fontImg);
  p.drawImage(0, 0, flowerImg);

  // The wave. Cave AND outdoor -- the water tile ($14, i.e. column 4 of row 1) shifts its
  // pixels sideways, exactly as the console rotates the tile's bytes each frame.
  if(animatesWater) {
    // Copy water tile out to it's own image
    auto waterTile = tileset.copy(4 * tileWidth, 1 * tileHeight,
                                  tileWidth, tileHeight);

    // Process water tile
    waterTile = postProcessWave(waterTile, frame);

    // Copy back into the tileset
    p.drawImage(4 * tileWidth, 1 * tileHeight, waterTile);
  }

  return QPixmap::fromImage(tileset);
}

QVector<QPixmap> TilesetEngine::buildTileset(QString id)
{
  // Use internal debug function to manually and slowly create it
  auto tileset = buildTilesetFullDebug(id).toImage();

  // Convert to an array of tiles
  QVector<QPixmap> ret = getTiles(tileset);

  // Return a newly built array of tiles
  return ret;
}

QVector<QPixmap> TilesetEngine::getTiles(QImage tilemap)
{
  QVector<QPixmap> ret;

  int tilesX = width / 8;
  int tilesY = height / 8;

  for(int tileY = 0; tileY < tilesY; tileY++) {
    for(int tileX = 0; tileX < tilesX; tileX++) {
      int startX = tileX * 8;
      int startY = tileY * 8;

      auto tile = tilemap.copy(startX, startY, 8, 8);
      ret.append(QPixmap::fromImage(tile));
    }
  }

  return ret;
}

QImage TilesetEngine::blankImage()
{
  auto img = QImage(width, height, QImage::Format::Format_ARGB32);
  img.fill(QColor(0, 0, 0, 0));
  return img;
}

QImage TilesetEngine::postProcessWaveOnce(QImage tile)
{
  // Holds a line of pixels as a buffer
  QVector<QColor> line;
  QImage lineImg;

  // Finished image, pre-fill white
  QImage ret = QImage(tileWidth, tileHeight, QImage::Format::Format_ARGB32);
  ret.fill(QColor("white"));

  // Loop through each line of the tile
  for(int y = 0; y < tileHeight; y++) {

    // Copy out each line
    lineImg = tile.copy(0, y, tileWidth, 1);

    // Convert it to pixels
    for(int x = 0; x < tileWidth; x++) {
      line.append(lineImg.pixelColor(x, 0));
    }

    // Convert it back to an image offset by 1
    for(int x = 0; x < tileWidth; x++) {

      // Calculate a -1 offset
      // We want to shift the row of pixels right by 1 and wrap around
      // We start at pixel #0 and move forward, the first pixel though
      // is replaced with the last pixel thus creating a left-shift wrap around

      int xOff = x - 1;
      if(xOff < 0)
        xOff = tileWidth - 1;

      ret.setPixelColor(x, y, line.at(xOff));
    }

    line.clear();
  }

  return ret;
}

QImage TilesetEngine::postProcessWave(QImage tile, int frame)
{
  // ⚠️ FIXED 2026-07-12 against the disassembly. It used to run 0,1,2,3,4,3,2,1 -- the right SHAPE
  // (a ping-pong) but the wrong offsets: the console's water swings from -1 to +3, not 0 to +4.
  //
  // What the console does, every animation step (UpdateMovingBgTiles, home/vcopy.asm):
  //
  //     ld a, [wMovingBGTilesCounter2]
  //     inc a
  //     and 7
  //     ld [wMovingBGTilesCounter2], a
  //     and 4
  //     jr nz, .left            ; counter2 bit 2 set -> rotate LEFT
  //     .right: rrca each of the tile's 16 bytes
  //     .left:  rlca each of the tile's 16 bytes
  //
  // Rotating every byte of a 2bpp tile by one bit rotates every ROW of the tile by one pixel, with
  // wraparound -- so "water has no frames; it has a rotation". Four steps right, four steps left,
  // forever. Accumulating that gives, per step (counter2 = frame % 8):
  //
  //     frame % 8:  0   1   2   3   4   5   6   7
  //     offset:     0  +1  +2  +3  +2  +1   0  -1
  //
  // (+ = rotate right.) See notes/reference/map-animation.md.
  static const int offsets[8] = { 0, 1, 2, 3, 2, 1, 0, -1 };

  const int subFrame = ((frame % 8) + 8) % 8;   // negative-safe
  const int offset = offsets[subFrame];

  QImage ret = tile;

  // A left rotation by one is a right rotation by (tileWidth - 1): the row wraps, so the two are
  // the same operation counted from the other end. Doing it that way keeps ONE primitive.
  const int steps = (offset >= 0) ? offset : (tileWidth + offset);

  for(int i = 0; i < steps; i++)
    ret = postProcessWaveOnce(ret);

  return ret;
}
