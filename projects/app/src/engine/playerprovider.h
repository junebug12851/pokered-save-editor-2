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
 * @brief QML image provider for the player's overworld sprite ("image://player/...").
 *
 * `image://player/<facing>/<contrast>` — `<facing>` is a `SPRITE_FACING_*` value
 * (0 down, 4 up, 8 left, 12 right) and `<contrast>` selects the object palette.
 *
 * Separate from MapProvider on purpose: the map image is large and worth caching, while the
 * sprite is 16×16 and changes whenever the player turns. Keeping them apart means turning
 * around doesn't redraw the whole map.
 *
 * The returned pixmap has a **transparent** colour 0, exactly as an object does on the
 * hardware. @see MapEngine::playerSprite(), `reference/sprites.md`.
 */
class PlayerProvider : public QQuickImageProvider
{
public:
  PlayerProvider();

  virtual QPixmap requestPixmap(const QString& id, QSize* size, const QSize& requestedSize) override;
};
