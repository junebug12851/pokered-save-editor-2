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
#include <QPixmap>
#include <QQuickImageProvider>
#include <QString>
#include <QSize>

/**
 * @brief QML image provider for individual tileset tiles ("image://...").
 *
 * QML requests `image://<provider>/<id>` and this returns a single tile (or, in
 * debug, the whole tileset) rendered by TilesetEngine. The slash-separated id
 * format is documented in-code below.
 *
 * @see TilesetEngine (does the rendering), FontPreviewProvider (the sibling provider).
 */
class TilesetProvider : public QQuickImageProvider
{
public:
  TilesetProvider();

  // <tileset>/<type>/<font>/<frame>/<tile>/<width>/<height>
  //  * <tileset> is the tileset, case-insensitive and spaces converted to
  //    underscores
  //  * <type> is the type, specifically "outdoor" or not is used here
  //  * <font> is whether to load fonts and white out certain tiles,
  //    specifically "font" or not is used here
  //  * <frame> can be any positive number, a full frame cycle completes in 8
  //    frames though so it's suggested to use multiple of 8 for smooth
  //    animation
  //  * <tile> can either be a tile number between 0-255 or "whole" which invokes
  //    a debug operation and returns the whole image. "whole" is never suggested
  //    because whole is not cached and can take a long time to build.
  /// Render the tile for @p id (format documented above). @return the pixmap.
  virtual QPixmap requestPixmap(const QString& id, QSize* size, const QSize& requestedSize) override;

  QPixmap blankImage(QSize* size, const QSize& requestedSize); ///< Fallback blank tile.
};
