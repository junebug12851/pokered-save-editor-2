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

/**
 * @file mapprovider.cpp
 * @brief Implementation of MapProvider -- the whole-map QML image provider.
 *        See mapprovider.h.
 */

#include <QImage>
#include <QStringList>

#include "./mapprovider.h"
#include "./mapengine.h"

MapProvider::MapProvider()
  : QQuickImageProvider(QQuickImageProvider::Pixmap)
{}

QPixmap MapProvider::requestPixmap(const QString& id, QSize* size, const QSize& requestedSize)
{
  const auto parts = id.split("/", Qt::SkipEmptyParts);

  // <mapInd>/<tilesetInd>/<frame>/<contrast>
  if (parts.size() < 3)
    return blankImage(size);

  const int mapInd     = parts.at(0).toInt();
  const int tilesetInd = parts.at(1).toInt();
  const int frame      = parts.at(2).toInt();
  const int contrast   = (parts.size() > 3) ? parts.at(3).toInt() : 0;

  const auto buffer = MapEngine::buildOverworldMap(mapInd);
  const QImage img = MapEngine::render(buffer, tilesetInd, frame, contrast);

  // No block data (a glitch map id) -- there is nothing in ROM to draw.
  if (img.isNull())
    return blankImage(size);

  if (size != nullptr)
    *size = img.size();

  QPixmap ret = QPixmap::fromImage(img);

  // Only scale if QML explicitly asked to, and then never smooth it -- this is
  // 8x8 pixel art and interpolating it would be a lie about what the game drew.
  if (requestedSize.width() > 0 && requestedSize.height() > 0)
    ret = ret.scaled(requestedSize.width(), requestedSize.height(),
                     Qt::IgnoreAspectRatio, Qt::FastTransformation);

  return ret;
}

QPixmap MapProvider::blankImage(QSize* size)
{
  QImage img(1, 1, QImage::Format_ARGB32);
  img.fill(Qt::transparent);

  if (size != nullptr)
    *size = img.size();

  return QPixmap::fromImage(img);
}
