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
#pragma once

#include <QObject>
#include <QTimer>

class MapModel;

/**
 * @brief The map's heartbeat -- the Game Boy's own animation cadence, and nothing more.
 *
 * Exposed as `brg.mapClock`. It drives ONE number: @ref MapModel::frame, the animation STEP the
 * renderer draws. Everything about what that step looks like lives in TilesetEngine (the water's
 * rotation, the flower's three tiles); everything about WHEN it advances lives here.
 *
 * **The cadence is the console's** (`UpdateMovingBgTiles`, `home/vcopy.asm` -- see
 * notes/reference/map-animation.md). It does nothing for 19 frames, then:
 *
 *  - **water-only** tilesets (the animation byte is ODD): the counter resets on frame 20, so one
 *    animation step every **20 frames**;
 *  - **water+flower** tilesets (the byte is EVEN and non-zero): it runs on to frame 21 for the
 *    flower, so one step every **21 frames**;
 *  - **byte 0**: nothing animates at all (and Surf breaks -- the game leans on that tile).
 *
 * At the DMG's 59.7275 Hz that is about **three steps a second**. It is slow, and it is supposed
 * to be.
 *
 * ⚠️ **Determinism.** The renderer is a pure function of (save, layers, frame) -- it never reads a
 * wall clock. This class is the only thing that moves the frame on, and it **refuses to run on the
 * `offscreen` platform**, which is what every headless run uses: the screenshooter, the GUI suites,
 * and `tst_visual_regression`. A moving map must never make a test flap. (notes/plans/map-screen.md
 * -> Phase 3h)
 *
 * ⚠️ It writes **nothing** to the save. Animation is a view concern.
 */
class MapClock : public QObject
{
  Q_OBJECT

  Q_PROPERTY(bool playing READ playing WRITE setPlaying NOTIFY playingChanged)
  /// 0.5 / 1 / 2 -- half speed, the console's speed, double. (A map animator wants to slow it down.)
  Q_PROPERTY(qreal speed READ speed WRITE setSpeed NOTIFY speedChanged)
  /// The animation step being drawn. Cycles every 8 (the water's rotation is an 8-step ping-pong).
  Q_PROPERTY(int frame READ frame NOTIFY frameChanged)
  /// Does this map animate at all? (The save's animation byte, 0x3522.)
  Q_PROPERTY(bool animates READ animates NOTIFY animatesChanged)
  /// How many console frames one animation step takes here: 20, 21, or 0 when nothing animates.
  Q_PROPERTY(int cadence READ cadence NOTIFY animatesChanged)

public:
  /// The DMG's real frame rate. Not 60: the difference is 0.5%, and this is an emulator.
  static constexpr qreal gbFrameRate = 59.7275;

  explicit MapClock(MapModel* map, QObject* parent = nullptr);

  bool playing() const;
  void setPlaying(bool playing);

  qreal speed() const;
  void setSpeed(qreal speed);

  int frame() const;
  bool animates() const;
  int cadence() const;

  /// One animation step, by hand. (Paused, this is how you walk the water round its cycle.)
  Q_INVOKABLE void step();

  /// Back to frame 0 -- the still frame every screenshot and every test renders.
  Q_INVOKABLE void reset();

signals:
  void playingChanged();
  void speedChanged();
  void frameChanged();
  void animatesChanged();

private:
  void retime();     ///< The cadence changed (a different map, or the animation byte was edited).
  void onTick();

  MapModel* map = nullptr;
  QTimer timer;

  bool play = false;
  qreal rate = 1.0;
  int step_ = 0;
};
