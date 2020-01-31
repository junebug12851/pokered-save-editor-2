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

#include <QImage>

#include "./tilesetprovider.h"
#include "./tilesetengine.h"

TilesetProvider::TilesetProvider()
  : QQuickImageProvider(QQuickImageProvider::Pixmap)
{}


QPixmap TilesetProvider::requestPixmap(const QString& id, QSize* size, const QSize& requestedSize)
{
  // Check to make sure it's a properly formed request
  auto idParts = id.split("/", QString::SplitBehavior::SkipEmptyParts);

  // Has to have all 5 parts unconditionally
  if(idParts.size() < 5)
    return blankImage(size, requestedSize);

  // Whole tileset or a specific tile
  // Never whole tileset unless it's for debug purposes
  bool wholeTileset = idParts.at(4) == "whole";

  // Actual size
  QSize actualSize = QSize(wholeTileset ? TilesetEngine::width : TilesetEngine::tileWidth,
                           wholeTileset ? TilesetEngine::height : TilesetEngine::tileHeight);

  // Set actual size if asked
  if(size != nullptr)
    *size = actualSize;

  // Prepare return
  QPixmap ret;

  // Get full tileset if "whole" otherwise requested tile
  if(wholeTileset)
    ret = TilesetEngine::buildTilesetFullDebug(id);
  else
    ret = TilesetEngine::buildTileset(id).at(idParts.at(4).toInt());

  // Scale if asked to
  ret = ret.scaled((requestedSize.width() > 0) ? requestedSize.width() : actualSize.width(),
             (requestedSize.height() > 0) ? requestedSize.height() : actualSize.height());

  // Return said tile or tileset
  return ret;
}

QPixmap TilesetProvider::blankImage(QSize* size, const QSize& requestedSize)
{
  // Create an error "red" blank tile to indicate issue
  QSize actualSize = QSize(TilesetEngine::width, TilesetEngine::height);

  if(size != nullptr)
    *size = actualSize;

  auto img = QImage(TilesetEngine::width, TilesetEngine::height, QImage::Format::Format_ARGB32);
  img.fill(QColor(255, 0, 0, 0)); // Fill with error "red" color

  auto ret = QPixmap::fromImage(img);

  ret = ret.scaled((requestedSize.width() > 0) ? requestedSize.width() : actualSize.width(),
             (requestedSize.height() > 0) ? requestedSize.height() : actualSize.height());

  return ret;
}
