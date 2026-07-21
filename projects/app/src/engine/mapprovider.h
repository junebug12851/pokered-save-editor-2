/*
  * Copyright 2026 Fairy Fox
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

#include <QPixmap>
#include <QQuickImageProvider>
#include <QSize>
#include <QString>
#include <QStringList>

/**
 * @brief QML image provider for a whole rendered map ("image://map/...").
 *
 * Two things, one provider, told apart by the first word of the id:
 *
 * - `image://map/<mapInd>/<tilesetInd>/<frame>/<contrast>/<tileAnim>` -- **the map**: its
 *   entire overworld buffer (the map plus its 3-block border ring), drawn by MapEngine at
 *   one screen pixel per Game Boy pixel (32 px per block). `tileAnim` is the **save's**
 *   animation byte (0 Indoor / 1 Cave / 2 Outdoor), not the tileset's default -- the two are
 *   allowed to disagree and the save is what a console would run.
 *
 * - `image://map/overlay/<mapInd>/<tilesetInd>/<layers>/<grassTile>/<c0>/<c1>/<c2>` -- **the
 *   semantic overlay**: walls, grass, water, warps, doors, ledges, counters, the border ring.
 *   Exactly the same size as the map and **transparent** everywhere it has nothing to say, so
 *   QML stacks it straight on top and animates its opacity. `layers` is a bit set
 *   (MapEngine::Layer); the grass and counter tiles ride in the id because they come from the
 *   save, which also means editing one re-renders the overlay with no invalidation logic to
 *   get wrong.
 *
 * Scale either in QML with `smooth: false` so it stays pixel art.
 *
 * The image is built on demand and can be large (Route 17 is 512 x 2496), so QML should let
 * `Image` cache it and only change `source` when something in the id actually changes.
 *
 * @see MapEngine (does the work), MapModel (builds the ids), TilesetProvider (sibling).
 */
class MapProvider : public QQuickImageProvider
{
public:
  MapProvider();

  /// Render whatever @p id asks for (the map, or the overlay). @return it, or a blank.
  virtual QPixmap requestPixmap(const QString& id, QSize* size, const QSize& requestedSize) override;

private:
  /// The `overlay/...` form of the id. @see requestPixmap.
  QPixmap requestOverlay(const QStringList& parts, QSize* size, const QSize& requestedSize);

  QPixmap blankImage(QSize* size); ///< 1x1 transparent fallback (no map / no data).
};
