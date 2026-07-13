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
 * @file mapsim.cpp
 * @brief Implementation of MapSim -- the NPCs wander. See mapsim.h.
 */

#include <QGuiApplication>
#include <QVariantMap>

#include <pse-common/random.h>
#include <pse-savefile/expanded/fragments/spritedata.h>

#include "./mapsim.h"
#include "./mapengine.h"
#include "../mvc/mapmodel.h"

namespace {

/// The four ways a sprite can face, as (dx, dy) and the facing byte the game would store.
struct Step { int dx; int dy; int facing; };

const Step kDown  = {  0,  1, SpriteFacing::Down  };
const Step kUp    = {  0, -1, SpriteFacing::Up    };
const Step kLeft  = { -1,  0, SpriteFacing::Left  };
const Step kRight = {  1,  0, SpriteFacing::Right };

} // namespace

MapSim::MapSim(MapModel* map, QObject* parent)
  : QObject(parent), map(map)
{
  timer.setInterval(stepMs);
  connect(&timer, &QTimer::timeout, this, &MapSim::onTick);

  // A map with nobody who can walk cannot be simulated, and the button has to know.
  if (map != nullptr)
    connect(map, &MapModel::changed, this, &MapSim::canSimulateChanged);
}

bool MapSim::playing() const { return play; }

void MapSim::setPlaying(bool playing)
{
  if (play == playing)
    return;

  // ⚠️ Never on the offscreen platform. Every headless run -- the screenshooter, the GUI suites,
  // tst_visual_regression -- must be deterministic, and this thing REWRITES THE SAVE. A test whose
  // fixture quietly wanders off is the worst kind of flap there is.
  if (playing && QGuiApplication::platformName() == QLatin1String("offscreen"))
    return;

  play = playing;

  if (play)
    timer.start();
  else
    timer.stop();

  emit playingChanged();
}

int MapSim::interval() const { return stepMs; }

void MapSim::setInterval(int ms)
{
  ms = qBound(60, ms, 4000);
  if (stepMs == ms)
    return;

  stepMs = ms;
  timer.setInterval(stepMs);
  emit intervalChanged();
}

bool MapSim::canSimulate() const
{
  if (map == nullptr)
    return false;

  // Somebody, anybody, whose movement byte 1 says WALK.
  for (const QVariant& v : map->npcList()) {
    const int slot = v.toMap().value("slot").toInt();

    for (const QVariant& f : map->npcFields(slot)) {
      const QVariantMap field = f.toMap();
      if (field.value("key").toString() == QStringLiteral("movementByte")
          && field.value("value").toInt() == SpriteMobility::Walk)
        return true;
    }
  }

  return false;
}

void MapSim::onTick()
{
  step();
}

void MapSim::step()
{
  if (map == nullptr)
    return;

  const QVariantList npcs = map->npcList();

  for (const QVariant& v : npcs) {
    const QVariantMap npc = v.toMap();
    const int slot = npc.value("slot").toInt();

    // Read this sprite's two movement bytes back out of the model, so the simulation and the
    // Details panel can never disagree about what a sprite is allowed to do.
    int movement1 = SpriteMobility::Stay;
    int movement2 = SpriteMovement::AnyDir;

    for (const QVariant& f : map->npcFields(slot)) {
      const QVariantMap field = f.toMap();
      const QString key = field.value("key").toString();

      if (key == QStringLiteral("movementByte"))
        movement1 = field.value("value").toInt();
      else if (key == QStringLiteral("rangeDirByte"))
        movement2 = field.value("value").toInt();
    }

    // MOVEMENT BYTE 1: only WALK moves. STAY means stay -- and anything else is a no-collision hack
    // value, which we leave alone rather than guess at.
    if (movement1 != SpriteMobility::Walk)
      continue;

    // MOVEMENT BYTE 2: HOW it may move.
    QVector<Step> allowed;

    switch (movement2) {
      case SpriteMovement::AnyDir:
        allowed = { kDown, kUp, kLeft, kRight };
        break;

      case SpriteMovement::UpDown:
        allowed = { kDown, kUp };
        break;

      case SpriteMovement::LeftRight:
        allowed = { kLeft, kRight };
        break;

      // The four fixed directions: it only ever walks that way. (In the game these march a sprite in
      // a straight line -- so that is what they do here.)
      case SpriteMovement::Down:  allowed = { kDown };  break;
      case SpriteMovement::Up:    allowed = { kUp };    break;
      case SpriteMovement::Left:  allowed = { kLeft };  break;
      case SpriteMovement::Right: allowed = { kRight }; break;

      // NONE, BOULDER, and anything else: it does not wander.
      default:
        break;
    }

    if (allowed.isEmpty())
      continue;

    // The game's sprites dawdle -- they stand about far more than they walk. A sprite that moved on
    // every single tick would skitter, which is not what a town looks like.
    if (Random::inst()->rangeExclusive(0, 100) < 55)
      continue;

    const Step s = allowed.at(Random::inst()->rangeExclusive(0, allowed.size()));

    const int nx = npc.value("x").toInt() + s.dx;
    const int ny = npc.value("y").toInt() + s.dy;

    // Off the map is not a place. (moveNpc clamps too, but bumping into the edge and staying put is
    // what the console does -- it does not slide along it.)
    if (nx < 0 || ny < 0 || nx >= map->blocksWide() * 2 || ny >= map->blocksHigh() * 2)
      continue;

    // Turn first, then step -- which is also the order the game does it in.
    map->setNpcField(slot, QStringLiteral("faceDir"), s.facing);
    map->moveNpc(slot, nx, ny);

    emit moved(npc.value("name").toString());
  }
}
