/*
  * Copyright 2026 Twilight
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
#include <QString>

class AreaMap;
class AreaPlayer;
class AreaTileset;

/**
 * @brief The loaded map, ready to draw: one image URL and four rectangles.
 *
 * Exposed as `brg.map`. Watches the save's live map (AreaMap), player position
 * (AreaPlayer) and tileset (AreaTileset), and republishes them as everything the
 * map screen needs -- the `image://map/...` @ref source and the geometry of the
 * three boxes the game itself works in, all in **buffer pixels** (one screen pixel
 * per Game Boy pixel, 32 px to a block, origin at the top-left of the border ring).
 *
 * The boxes, outermost first:
 *
 * - **the buffer** (@ref imageWidth x @ref imageHeight) -- the map plus its 3-block
 *   border ring: the whole rendered image.
 * - **the map** (@ref mapX ... @ref mapH) -- where the real map sits inside that ring.
 * - **the scratch area** (@ref scratchX ...) -- the 6x5 blocks the game redraws
 *   (`wSurroundingTiles`). Always block-aligned.
 * - **the screen** (@ref screenX ...) -- the 20x18 tiles actually on the Game Boy's
 *   screen, sliding around inside the scratch area by half-block steps.
 *
 * This has to be a C++ model rather than QML bindings: the Area's children are
 * `Q_DECLARE_OPAQUE_POINTER`, so `brg.file.data.dataExpanded.area.map.*` reads as
 * `undefined` from QML (see `reference/qt-patterns.md`).
 *
 * @see MapEngine (the maths + the rendering), MapProvider (serves @ref source).
 */
class MapModel : public QObject
{
  Q_OBJECT

  Q_PROPERTY(bool valid READ valid NOTIFY changed)          ///< Does the loaded map have block data?
  Q_PROPERTY(QString source READ source NOTIFY changed)     ///< `image://map/...` URL for the whole map.
  Q_PROPERTY(QString mapName READ mapName NOTIFY changed)   ///< The loaded map's display name.
  Q_PROPERTY(QString tilesetName READ tilesetName NOTIFY changed) ///< The loaded tileset's name.

  Q_PROPERTY(int mapInd READ mapInd NOTIFY changed)         ///< Loaded map id (`wCurMap`).
  Q_PROPERTY(int tilesetInd READ tilesetInd NOTIFY changed) ///< Loaded tileset id (`wCurMapTileset`).
  Q_PROPERTY(int playerX READ playerX NOTIFY changed)       ///< Player x, in half-blocks.
  Q_PROPERTY(int playerY READ playerY NOTIFY changed)       ///< Player y, in half-blocks.

  Q_PROPERTY(int blocksWide READ blocksWide NOTIFY changed) ///< Map width, blocks.
  Q_PROPERTY(int blocksHigh READ blocksHigh NOTIFY changed) ///< Map height, blocks.
  Q_PROPERTY(int blockSize READ blockSize CONSTANT)         ///< Pixels per block (32).

  Q_PROPERTY(int imageWidth READ imageWidth NOTIFY changed)   ///< Rendered buffer width, px.
  Q_PROPERTY(int imageHeight READ imageHeight NOTIFY changed) ///< Rendered buffer height, px.

  Q_PROPERTY(int mapX READ mapX NOTIFY changed) ///< The map inside the border ring, px.
  Q_PROPERTY(int mapY READ mapY NOTIFY changed) ///< @see mapX
  Q_PROPERTY(int mapW READ mapW NOTIFY changed) ///< @see mapX
  Q_PROPERTY(int mapH READ mapH NOTIFY changed) ///< @see mapX

  Q_PROPERTY(int scratchX READ scratchX NOTIFY changed) ///< The 6x5-block scratch area, px.
  Q_PROPERTY(int scratchY READ scratchY NOTIFY changed) ///< @see scratchX
  Q_PROPERTY(int scratchW READ scratchW NOTIFY changed) ///< @see scratchX
  Q_PROPERTY(int scratchH READ scratchH NOTIFY changed) ///< @see scratchX

  Q_PROPERTY(int screenX READ screenX NOTIFY changed) ///< The 20x18-tile visible screen, px.
  Q_PROPERTY(int screenY READ screenY NOTIFY changed) ///< @see screenX
  Q_PROPERTY(int screenW READ screenW NOTIFY changed) ///< @see screenX
  Q_PROPERTY(int screenH READ screenH NOTIFY changed) ///< @see screenX

public:
  MapModel(AreaMap* map, AreaPlayer* player, AreaTileset* tileset);

  bool valid() const;
  QString source() const;
  QString mapName() const;
  QString tilesetName() const;

  int mapInd() const;
  int tilesetInd() const;
  int playerX() const;
  int playerY() const;

  int blocksWide() const;
  int blocksHigh() const;
  int blockSize() const;

  int imageWidth() const;
  int imageHeight() const;

  int mapX() const;
  int mapY() const;
  int mapW() const;
  int mapH() const;

  int scratchX() const;
  int scratchY() const;
  int scratchW() const;
  int scratchH() const;

  int screenX() const;
  int screenY() const;
  int screenW() const;
  int screenH() const;

signals:
  /// The loaded map, the tileset or the player moved -- everything above may have changed.
  void changed();

private:
  AreaMap* map = nullptr;         ///< The save's live map.
  AreaPlayer* player = nullptr;   ///< The save's live player position.
  AreaTileset* tileset = nullptr; ///< The save's live tileset.
};
