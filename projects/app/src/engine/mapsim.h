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

#include <QObject>
#include <QTimer>

class MapModel;
class SpriteData;

/**
 * @brief `UpdateNPCSprite` -- the game's own walking, transliterated.
 *
 * Exposed as `brg.mapSim`. Press ▶ and the map runs the console's per-frame sprite state machine, out
 * of `engine/overworld/movement.asm`, **instruction for instruction and bug for bug**. It is not an
 * impression of wandering; it is the routine. The full write-up, with the assembly beside it, is
 * notes/reference/npc-movement.md.
 *
 * ⚠️ **A `STAY` sprite is not standing still -- it is TURNING.** It runs the whole random-direction
 * path, and the facing is written *before* the collision check refuses the step. Oak turns on the spot
 * about once a second, forever. Skipping `STAY` sprites (which the first version did) is a still
 * picture, not a simulation.
 *
 * ⚠️ **It ticks at the CONSOLE'S FRAME RATE** (59.7275 Hz), because every counter in the routine is
 * measured in Game Boy frames: a step is 16 frames, a delay is `Random() & $7F` frames. At any other
 * interval none of those numbers mean what they mean.
 *
 * ⚠️ **IT IS DESTRUCTIVE, ON PURPOSE.** It writes the *real* sprite bytes -- which is exactly what the
 * console does, and it is what makes every field in the Details panel's "engine state" group visibly
 * come alive. There is no shadow copy: a parallel set of positions the renderer preferred over the
 * save's would mean every reader (the canvas, the panel, the status bar, the tests) had to know which
 * of two truths to believe, and that is where bugs live.
 *
 * ⚠️ **Paused by default**, and it refuses to run on the `offscreen` platform -- a headless test whose
 * fixture quietly rewrites itself is the worst kind of flap there is.
 */
class MapSim : public QObject
{
  Q_OBJECT

  Q_PROPERTY(bool playing READ playing WRITE setPlaying NOTIFY playingChanged)

  /// 0.25 .. 4. The console's own pace is 1. (A map author wants to slow it right down.)
  Q_PROPERTY(qreal speed READ speed WRITE setSpeed NOTIFY speedChanged)

  /// Is there anybody on this map at all? (Everybody moves -- a `STAY` sprite turns.)
  Q_PROPERTY(bool canSimulate READ canSimulate NOTIFY canSimulateChanged)

public:
  explicit MapSim(MapModel* map, QObject* parent = nullptr);

  bool playing() const;
  void setPlaying(bool playing);

  qreal speed() const;
  void setSpeed(qreal speed);

  bool canSimulate() const;

  /// ONE Game Boy frame of `UpdateNPCSprite`, for every sprite. (Paused, this is how you walk the
  /// town forward a frame at a time -- and it is what the tests drive.)
  Q_INVOKABLE void step();

signals:
  void playingChanged();
  void speedChanged();
  void canSimulateChanged();

  /// A frame in which something actually changed.
  void ticked();

private:
  /// The four directions, as the game's `TryWalking` branches name them. (An int in the header so the
  /// enum itself can stay private to the .cpp, next to the assembly it mirrors.)
  using Dir_ = int;

  void onTick();

  void tryWalking(SpriteData* s, Dir_ d);
  bool canWalkOntoTile(SpriteData* s, Dir_ d);
  bool tilePassable(int x, int y);
  void failToWalk(SpriteData* s);

  MapModel* map = nullptr;
  QTimer timer;

  bool play = false;
  qreal rate = 1.0;
};
