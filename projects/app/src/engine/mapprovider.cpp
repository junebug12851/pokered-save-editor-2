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

  // Two shapes, and the first word says which:
  //
  //   <mapInd>/<tilesetInd>/<frame>/<contrast>/<tileAnim>/<blocksetInd>
  //     the map itself. <blocksetInd> is which tileset's BLOCKS to build it from -- normally the
  //     same one the graphics come from, but the save keeps two separate pointers and is allowed
  //     to disagree with itself. -1 = the same one.
  //
  //   overlay/<mapInd>/<tilesetInd>/<layers>/<grassTile>/<c0>/<c1>/<c2>
  //     the semantic layer -- walls, grass, warps... -- as a TRANSPARENT image exactly the
  //     same size, so QML can stack it straight on top and just animate its opacity.
  //
  // The overlay is a rendered image and not a pile of QML rectangles for a plain reason:
  // Route 17 is 78 blocks tall, which is over 20,000 tiles. As delegates that would crawl;
  // as one image it scales with the zoom for free.
  if (!parts.isEmpty() && parts.at(0) == "overlay")
    return requestOverlay(parts, size, requestedSize);

  if (parts.size() < 3)
    return blankImage(size);

  const int mapInd     = parts.at(0).toInt();
  const int tilesetInd = parts.at(1).toInt();
  const int frame      = parts.at(2).toInt();
  const int contrast   = (parts.size() > 3) ? parts.at(3).toInt() : 0;

  // Which tiles animate -- the SAVE's byte, not the tileset's default. -1 = fall back.
  const int tileAnim   = (parts.size() > 4) ? parts.at(4).toInt() : -1;

  // Whose BLOCKS. -1 = the same tileset the graphics come from (the normal case).
  const int blocksetInd = (parts.size() > 5) ? parts.at(5).toInt() : -1;

  // The block filling the 3-block ring -- the SAVE's `wMapBackgroundTile`, not the map's shipped
  // one. Edit it and the edge of the world changes, which is what a console would do.
  const int borderBlock = (parts.size() > 6) ? parts.at(6).toInt() : -1;

  // parts[7] is the palette generation (a cache-buster; the filter is a global on MapEngine).
  //
  // parts[8] is the SAVE's connections -- so the ring is a **Continue-load**, not the ROM defaults.
  // Encoded `dir.toInd.src.dst.width.stride` per connection, joined by `_`; "-" (or absent) means
  // "use the map's shipped ROM connections", which is what a neighbour-map render passes. @see
  // MapModel::source, MapEngine::SaveConn.
  QVector<MapEngine::SaveConn> saveConns;
  const bool haveConns = (parts.size() > 8) && parts.at(8) != QStringLiteral("-");
  if (haveConns) {
    const QStringList encoded = parts.at(8).split('_', Qt::SkipEmptyParts);
    for (const QString& e : encoded) {
      const QStringList f = e.split('.');
      if (f.size() != 6)
        continue;
      MapEngine::SaveConn sc;
      sc.dir = f.at(0).toInt();
      sc.toInd = f.at(1).toInt();
      sc.stripSrc = f.at(2).toInt();
      sc.stripDst = f.at(3).toInt();
      sc.stripWidth = f.at(4).toInt();
      sc.width = f.at(5).toInt();
      saveConns.append(sc);
    }
  }

  // The GBC / SGB / custom colour filter, resolved for THIS map (the SGB mode colours each map in
  // its own palette). It rides in the URL as a generation counter, so the cached image refreshes
  // when the palette changes. @see MapEngine::outputPaletteFor
  QRgb outPal[4];
  MapEngine::outputPaletteFor(mapInd, tilesetInd, outPal);

  const auto buffer = MapEngine::buildOverworldMap(mapInd, borderBlock,
                                                   haveConns ? &saveConns : nullptr);
  const QImage img = MapEngine::render(buffer, tilesetInd, frame, contrast, tileAnim, blocksetInd,
                                       outPal);

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

QPixmap MapProvider::requestOverlay(const QStringList& parts, QSize* size,
                                    const QSize& requestedSize)
{
  // overlay/<mapInd>/<tilesetInd>/<layers>/<grassTile>/<c0>/<c1>/<c2>/<borderBlock>
  if (parts.size() < 4)
    return blankImage(size);

  const int mapInd     = parts.at(1).toInt();
  const int tilesetInd = parts.at(2).toInt();
  const quint32 layers = parts.at(3).toUInt();

  // The grass tile and the three counter tiles come from the SAVE, so they ride in the id --
  // which also means the overlay is re-rendered the moment either is edited, with no
  // invalidation logic to get wrong.
  MapEngine::SaveTiles save;
  save.grassTile = (parts.size() > 4) ? parts.at(4).toInt() : 0xFF;
  for (int i = 5; i < parts.size() && i < 8; i++)
    save.counters.append(parts.at(i).toInt());

  // The block the ring is filled with -- the save's `wMapBackgroundTile`. The BORDER layer paints
  // over the ring, so it has to be built from the same block the map itself is.
  const int borderBlock = (parts.size() > 8) ? parts.at(8).toInt() : -1;

  const auto buffer = MapEngine::buildOverworldMap(mapInd, borderBlock);
  const QImage img = MapEngine::overlay(buffer, tilesetInd, layers, save);

  if (img.isNull())
    return blankImage(size);

  if (size != nullptr)
    *size = img.size();

  QPixmap ret = QPixmap::fromImage(img);

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
