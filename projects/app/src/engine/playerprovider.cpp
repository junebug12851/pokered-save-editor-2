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
 * @file playerprovider.cpp
 * @brief Implementation of PlayerProvider. See playerprovider.h.
 */

#include <QImage>
#include <QStringList>

#include "./playerprovider.h"
#include "./mapengine.h"

PlayerProvider::PlayerProvider()
  : QQuickImageProvider(QQuickImageProvider::Pixmap)
{}

QPixmap PlayerProvider::requestPixmap(const QString& id, QSize* size, const QSize& requestedSize)
{
  const auto parts = id.split("/", Qt::SkipEmptyParts);

  const int facing   = (parts.size() > 0) ? parts.at(0).toInt() : MapEngine::FacingDown;
  const int contrast = (parts.size() > 1) ? parts.at(1).toInt() : 0;

  const QImage sprite = MapEngine::playerSprite(facing, contrast);

  if (sprite.isNull()) {
    QImage blank(1, 1, QImage::Format_ARGB32);
    blank.fill(Qt::transparent);

    if (size != nullptr)
      *size = blank.size();

    return QPixmap::fromImage(blank);
  }

  if (size != nullptr)
    *size = sprite.size();

  QPixmap ret = QPixmap::fromImage(sprite);

  // Never smooth it -- 8x8 pixel art, same rule as the map.
  if (requestedSize.width() > 0 && requestedSize.height() > 0)
    ret = ret.scaled(requestedSize.width(), requestedSize.height(),
                     Qt::IgnoreAspectRatio, Qt::FastTransformation);

  return ret;
}
