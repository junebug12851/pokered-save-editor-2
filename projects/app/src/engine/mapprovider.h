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

#include <QPixmap>
#include <QQuickImageProvider>
#include <QSize>
#include <QString>

/**
 * @brief QML image provider for a whole rendered map ("image://map/...").
 *
 * QML asks for `image://map/<mapInd>/<tilesetInd>/<frame>` and gets back the map's
 * entire overworld buffer -- the map plus its 3-block border ring -- drawn by
 * MapEngine at one screen pixel per Game Boy pixel (32 px per block). Scale it in
 * QML with `smooth: false` so it stays pixel art.
 *
 * The image is built on demand and can be large (Route 17 is 512 x 2496), so QML
 * should let `Image` cache it and only change `source` when the map, tileset or
 * frame actually changes.
 *
 * @see MapEngine (does the work), MapModel (builds the id), TilesetProvider (sibling).
 */
class MapProvider : public QQuickImageProvider
{
public:
  MapProvider();

  /// Render the map for @p id (`<mapInd>/<tilesetInd>/<frame>`). @return the map, or a blank.
  virtual QPixmap requestPixmap(const QString& id, QSize* size, const QSize& requestedSize) override;

private:
  QPixmap blankImage(QSize* size); ///< 1x1 transparent fallback (no map / no data).
};
