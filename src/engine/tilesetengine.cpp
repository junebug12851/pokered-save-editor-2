/*
  * Copyright 2020 June Hanabi
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
  // Frame 0: flower2
  // Frame 1: flower3
  // Frame 2: flower1
  // Frame 3: flower1

  // returns frame 0-3 no matter frame number
  int subFrame = frame % 4;
  int ind;

  if(subFrame == 0)
    ind = 2;
  else if(subFrame == 1)
    ind = 3;
  else
    ind = 1;

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
  auto idParts = id.split("/", QString::SplitBehavior::SkipEmptyParts);

  // Has to have all 4 parts unconditionally
  if(idParts.size() < 4) {
    // Return error red
    auto tmp = blankImage();
    tmp.fill(QColor(255, 0, 0, 0));
    return QPixmap::fromImage(tmp);
  }

  // Is outdoor? and use font? Also get frame
  bool outdoorType = idParts.at(1).toLower() == "outdoor";
  bool useFont = idParts.at(2).toLower() == "font";
  int frame = idParts.at(3).toInt();

  // Get or create blank images as needed for each layer
  auto tilesetImg = getTileset(idParts.at(0));

  auto fontImg = (useFont)
      ? getFont()
      : blankImage();
  auto flowerImg = (outdoorType)
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

  // Post-Process if applicable
  if(outdoorType) {
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
  // frame #0 = 0 shift
  // frame #1 = 1 shift
  // frame #2 = 2 shift
  // frame #3 = 3 shift
  // frame #4 = 4 shift
  // frame #5 = 3 shift
  // frame #6 = 2 shift
  // frame #7 = 1 shift

  // Get frame index 0-7 no matter frame number
  int subFrame = frame % 8;

  int count = subFrame;
  if(subFrame > 4)
    count = (8 - count);

  QImage ret = tile;

  for(int i = 0; i < count; i++)
    ret = postProcessWaveOnce(ret);

  return ret;
}
