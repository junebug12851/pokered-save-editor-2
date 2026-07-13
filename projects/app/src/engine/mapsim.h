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

/**
 * @brief Let the town come to life -- the NPCs actually wander.
 *
 * Exposed as `brg.mapSim`. Press ▶ and every sprite the save marks as `WALK` starts moving, the way
 * it does in the game: freely, or pinned to one axis, or facing one way forever -- whatever its
 * **movement byte 2** says. A `STAY` sprite stands exactly still, because that is what `STAY` means.
 *
 * ⚠️ **IT IS DESTRUCTIVE, ON PURPOSE.** It moves the *real* sprite data in the save -- the same two
 * bytes a drag writes. There is no shadow copy and no restore.
 *
 * That was a deliberate choice (Twilight, 2026-07-13). A non-destructive preview would need a second,
 * parallel set of sprite positions that the renderer prefers over the save's, and every feature that
 * reads a sprite's position -- the canvas, the Details panel, the status bar, the tests -- would have
 * to know which of the two to believe. That is a whole shadow world, and shadow worlds are where
 * bugs live. Moving the real thing is simpler, more honest, and exactly what a *map editor* ought to
 * do when you tell it to move the sprites.
 *
 * The screen says so plainly the first time you press it, with a "don't show me this again" that is
 * **unticked** -- because a warning you have to opt back INTO is not a warning.
 *
 * ⚠️ **Paused by default.** Nothing in this app starts moving, or making noise, on its own.
 *
 * ⚠️ Like MapClock, it **refuses to run on the `offscreen` platform** -- every headless run (the
 * screenshooter, the GUI suites, `tst_visual_regression`) must be deterministic, and a save that
 * quietly rewrites itself under a test is the worst kind of flap.
 */
class MapSim : public QObject
{
  Q_OBJECT

  Q_PROPERTY(bool playing READ playing WRITE setPlaying NOTIFY playingChanged)

  /// How long a sprite waits between steps, in ms. The game is lazier than you'd think.
  Q_PROPERTY(int interval READ interval WRITE setInterval NOTIFY intervalChanged)

  /// Is there anybody on this map who CAN move? (A map of `STAY` sprites cannot be simulated, and
  /// the button says so rather than doing nothing.)
  Q_PROPERTY(bool canSimulate READ canSimulate NOTIFY canSimulateChanged)

public:
  explicit MapSim(MapModel* map, QObject* parent = nullptr);

  bool playing() const;
  void setPlaying(bool playing);

  int interval() const;
  void setInterval(int ms);

  bool canSimulate() const;

  /// One step of everybody, by hand. (Paused, this is how you nudge the town along a frame.)
  Q_INVOKABLE void step();

signals:
  void playingChanged();
  void intervalChanged();
  void canSimulateChanged();

  /// A sprite moved. The status bar says who, so the destruction is never silent.
  void moved(const QString& who);

private:
  void onTick();

  MapModel* map = nullptr;
  QTimer timer;

  bool play = false;
  int stepMs = 450;
};
