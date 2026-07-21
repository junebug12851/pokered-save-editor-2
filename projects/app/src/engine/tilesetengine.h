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
#pragma once
#include <QObject>
#include <QImage>
#include <QPixmap>
#include <QColor>
#include <QString>
#include <QCache>

/**
 * @brief Static helpers that build Game Boy tileset graphics (tiles, flowers, waves).
 *
 * The rendering workhorse behind TilesetProvider and the font previews: it loads a
 * named tileset image, slices it into per-tile pixmaps, overlays animated flowers
 * and the font, and applies the animated water "wave" post-process. All static; the
 * `constexpr` values are the GB tile geometry. The id-string format for
 * buildTileset() is documented inline.
 *
 * @see TilesetProvider (the QML image provider that uses this), AreaTileset.
 */
class TilesetEngine : public QObject
{
  Q_OBJECT

public:
  /// Load a tileset image by name (case-insensitive; spaces -> underscores).
  static QImage getTileset(QString name);

  // Flower overlay
  // Frame can be any number, a full frame cycle is 4 frames so keep it
  // divisible by 4 for smooth animation
  static QImage getFlower(int frame); ///< The animated flower overlay for @p frame (see note).

  // Font overlay frame
  static QImage getFont(); ///< The font overlay image.

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
  static QVector<QPixmap> buildTileset(QString id); ///< Build the per-tile pixmaps for @p id (format above).

  // Convert image to tiles
  static QVector<QPixmap> getTiles(QImage tilemap); ///< Slice a tilemap image into per-tile pixmaps.

  // Empty transparent image
  static QImage blankImage(); ///< A blank transparent tile-sized image.

  // Post process the wave effect in one increment
  // postProcessWave calls this x number of times depending on frame index to
  // simulate waves back and forth
  static QImage postProcessWaveOnce(QImage tile); ///< One increment of the water-wave shift.

  // Post process the wave effect on a given tile image
  // Frame can be any number, a full frame cycle is 8 frames so keep it
  // divisible by 8 for smooth animation
  static QImage postProcessWave(QImage tile, int frame); ///< Apply the wave effect for @p frame (see note).

  static constexpr int width = 128;     ///< Tileset image width (px).
  static constexpr int height = 128;    ///< Tileset image height (px).
  static constexpr int tileWidth = 8;   ///< Tile width (px).
  static constexpr int tileHeight = 8;  ///< Tile height (px).
  static constexpr int tileWater = 0x14; ///< Tile id of the animated water tile.

  // flower has 4 frames, water has 8 frames
  // A full frame count would be one where everything completes
  static constexpr int fullFrameCount = 8; ///< Frames for a full animation cycle.
};
