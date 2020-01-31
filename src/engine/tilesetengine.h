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
#ifndef TILESETENGINE_H
#define TILESETENGINE_H

#include <QObject>
#include <QImage>
#include <QPixmap>
#include <QColor>
#include <QString>
#include <QCache>

class TilesetEngine : public QObject
{
  Q_OBJECT

public:
  // Case-insensitive tileset name. Spaces are auto replaced by underscores
  static QImage getTileset(QString name);

  // Flower overlay
  // Frame can be any number, a full frame cycle is 4 frames so keep it
  // divisible by 4 for smooth animation
  static QImage getFlower(int frame);

  // Font overlay frame
  static QImage getFont();

  // <tileset>/<type>/<font>/<frame>
  //  * <tileset> is the tileset, case-insensitive and spaces converted to
  //    underscores
  //  * <type> is the type, specifically "outdoor" or not is used here
  //  * <font> is whether to load fonts and white out certain tiles,
  //    specifically "font" or not is used here
  //  * <frame> can be any positive number, a full frame cycle completes in 8
  //    frames though so it's suggested to use multiple of 8 for smooth
  //    animation
  static QPixmap buildTilesetFullDebug(QString id); // Not cached, very slow, debug only
  static QVector<QPixmap> buildTileset(QString id);

  // Convert image to tiles
  static QVector<QPixmap> getTiles(QImage tilemap);

  // Empty transparent image
  static QImage blankImage();

  // Post process the wave effect in one increment
  // postProcessWave calls this x number of times depending on frame index to
  // simulate waves back and forth
  static QImage postProcessWaveOnce(QImage tile);

  // Post process the wave effect on a given tile image
  // Frame can be any number, a full frame cycle is 8 frames so keep it
  // divisible by 8 for smooth animation
  static QImage postProcessWave(QImage tile, int frame);

  static constexpr int width = 128;
  static constexpr int height = 128;
  static constexpr int tileWidth = 8;
  static constexpr int tileHeight = 8;
  static constexpr int tileWater = 0x14;

  // flower has 4 frames, water has 8 frames
  // A full frame count would be one where everything completes
  static constexpr int fullFrameCount = 8;

  // Hold a cache of 2 full frame objects
  static QCache<QString, QVector<QPixmap>> cache;
};

#endif // TILESETENGINE_H
