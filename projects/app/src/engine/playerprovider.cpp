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
  auto parts = id.split("/", Qt::SkipEmptyParts);

  QImage sprite;

  // ── The SILHOUETTE form: `sil/<rrggbb>/<...the ordinary id...>` ────────────────────────────
  //
  // The artwork flattened to ONE colour, alpha kept -- what the canvas stamps a sprite's outline
  // from (four copies, one pixel out in each direction). Served by the PROVIDER, deliberately:
  // the first outline used Qt Quick's MultiEffect and it never drew a pixel anywhere -- shader
  // effects don't run on the offscreen platform, and on hardware the invisible source item's
  // layer texture was never rendered either, so every sprite showed only its art's own black
  // linework (leadership, twice: *"they outline black"*, *"sprites still have black outline"*).
  // A provider image is plain pixels: it works on every backend, offscreen included, and Qt's
  // pixmap cache keys it by URL so each colour is computed once.
  QColor silColor;
  if (!parts.isEmpty() && parts.at(0) == "sil" && parts.size() > 1) {
    silColor = QColor("#" + parts.at(1));
    parts.remove(0, 2);
  }

  // Two forms, told apart by the first word:
  //   <facing>/<contrast>                            -- the player (slot 0)
  //   npc/<pictureID>/<facing>/<contrast>[/<frame>]  -- anybody else (one loose PNG per sprite)
  //
  // `frame` is the console's animFrameCounter (0-3) and is OPTIONAL: leave it off and you get the
  // standing pose, which is what every picker and every still map wants. The walk simulation passes
  // it, and that is how the legs move. @see MapEngine::npcSprite
  if (!parts.isEmpty() && parts.at(0) == "npc") {
    const int picture  = (parts.size() > 1) ? parts.at(1).toInt() : 0;
    const int facing   = (parts.size() > 2) ? parts.at(2).toInt() : MapEngine::FacingDown;
    const int contrast = (parts.size() > 3) ? parts.at(3).toInt() : 0;
    const int frame    = (parts.size() > 4) ? parts.at(4).toInt() : 0;

    sprite = MapEngine::npcSprite(picture, facing, contrast, frame);
  }
  else {
    const int facing   = (parts.size() > 0) ? parts.at(0).toInt() : MapEngine::FacingDown;
    const int contrast = (parts.size() > 1) ? parts.at(1).toInt() : 0;

    sprite = MapEngine::playerSprite(facing, contrast);
  }

  if (sprite.isNull()) {
    QImage blank(1, 1, QImage::Format_ARGB32);
    blank.fill(Qt::transparent);

    if (size != nullptr)
      *size = blank.size();

    return QPixmap::fromImage(blank);
  }

  // Flatten to the silhouette colour: every pixel that has any ink at all becomes the colour,
  // fully opaque. (Not alpha-scaled -- the outline is a mark, not a wash.)
  if (silColor.isValid()) {
    sprite = sprite.convertToFormat(QImage::Format_ARGB32);
    const QRgb ink = silColor.rgb() | 0xFF000000;
    for (int y = 0; y < sprite.height(); y++) {
      QRgb* line = reinterpret_cast<QRgb*>(sprite.scanLine(y));
      for (int x = 0; x < sprite.width(); x++)
        line[x] = (qAlpha(line[x]) > 0) ? ink : 0;
    }
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
