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
#include "mapclock.h"

#include <QGuiApplication>

#include "../mvc/mapmodel.h"

MapClock::MapClock(MapModel* map, QObject* parent)
  : QObject(parent), map(map)
{
  timer.setTimerType(Qt::PreciseTimer);
  connect(&timer, &QTimer::timeout, this, &MapClock::onTick);

  if (map != nullptr) {
    // A different map, or an edit to the animation byte, can change the cadence (20 vs 21 frames)
    // or stop the animation dead (byte 0). Re-time on both.
    connect(map, &MapModel::changed, this, [this]() {
      emit animatesChanged();
      retime();
    });
  }

  // ⚠️ HEADLESS RUNS DO NOT ANIMATE. The screenshooter, the GUI suites and tst_visual_regression all
  // boot the real UI on the `offscreen` platform -- and a map that moves under them would make every
  // one of those tests flap. The renderer takes the frame as an INPUT; this is the one place that
  // moves it, so this is the one place that has to know when to sit still.
  const bool headless = (QGuiApplication::platformName() == QStringLiteral("offscreen"));

  play = !headless;
  retime();
}

// ── What the console does ─────────────────────────────────────────────────────

bool MapClock::animates() const
{
  // The animation byte (0x3522). 0 = nothing moves -- and Surf breaks, because the game leans on
  // that tile. Anything else animates.
  return map != nullptr && map->valid() && map->tileAnim() != 0;
}

int MapClock::cadence() const
{
  if (!animates())
    return 0;

  // UpdateMovingBgTiles tests BIT 0 after the water step:
  //   set   (an ODD byte)  -> counter1 is reset there  -> the cycle is 20 frames, water only
  //   clear (EVEN, non-0)  -> it runs on to frame 21   -> the cycle is 21 frames, water + flower
  //
  // So a HACK value is not chaos: an odd byte behaves like water-only, an even one like
  // water+flower. We reproduce the console rather than "correcting" the save.
  return (map->tileAnim() & 1) ? 20 : 21;
}

// ── The tick ──────────────────────────────────────────────────────────────────

void MapClock::retime()
{
  const int frames = cadence();

  if (!play || frames == 0) {
    timer.stop();
    return;
  }

  // One animation step every `frames` console frames, at 59.7275 Hz -- about three a second.
  const qreal ms = (frames * 1000.0) / (gbFrameRate * (rate > 0 ? rate : 1.0));
  timer.start(qMax(1, qRound(ms)));
}

void MapClock::onTick()
{
  step_++;
  if (map != nullptr)
    map->setFrame(step_);

  emit frameChanged();
}

// ── The knobs ─────────────────────────────────────────────────────────────────

bool MapClock::playing() const { return play; }

void MapClock::setPlaying(bool playing)
{
  if (play == playing)
    return;

  play = playing;
  emit playingChanged();
  retime();
}

qreal MapClock::speed() const { return rate; }

void MapClock::setSpeed(qreal speed)
{
  speed = qBound(0.25, speed, 4.0);
  if (qFuzzyCompare(rate, speed))
    return;

  rate = speed;
  emit speedChanged();
  retime();
}

int MapClock::frame() const { return step_; }

void MapClock::step()
{
  onTick();
}

void MapClock::reset()
{
  if (step_ == 0)
    return;

  step_ = 0;
  if (map != nullptr)
    map->setFrame(0);

  emit frameChanged();
}
