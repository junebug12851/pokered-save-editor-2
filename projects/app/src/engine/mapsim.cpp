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
 * @brief `UpdateNPCSprite`, transliterated.
 *
 * This is not "sprites wander about a bit". It is the game's own per-frame state machine, instruction
 * for instruction, out of `engine/overworld/movement.asm` -- **bugs included, because the bugs are the
 * behaviour**. The complete write-up, with the assembly beside it, is in
 * notes/reference/npc-movement.md. Read that first; this file is the code, that file is the argument.
 *
 * The one thing to know before you read a line of it:
 *
 * ⚠️ **A `STAY` sprite is not standing still. It is TURNING.** It runs the whole random-direction path
 * every time it is ready, and `TryWalking` writes the new facing *before* `CanWalkOntoTile` refuses the
 * step. Professor Oak is picking a direction, turning to face it, failing to move, and setting a new
 * delay -- about once a second, forever. A simulation that merely *skips* `STAY` sprites (which the
 * first version of this file did) is a still picture, not a simulation.
 */

#include <QGuiApplication>

#include <pse-common/random.h>
#include <pse-db/tiletraitsdb.h>
#include <pse-savefile/expanded/area/areasprites.h>
#include <pse-savefile/expanded/fragments/spritedata.h>

#include "./mapsim.h"
#include "./mapengine.h"
#include "../mvc/mapmodel.h"

namespace {

// ── The game's own names ────────────────────────────────────────────────────────────────────
//
// constants/map_object_constants.asm. Spelled out here so the code below reads like the assembly it
// came from and can be diffed against it.

constexpr int WALK = 0xFE;
constexpr int STAY = 0xFF;

constexpr int ANY_DIR    = 0x00;
constexpr int UP_DOWN    = 0x01;
constexpr int LEFT_RIGHT = 0x02;
constexpr int DIR_DOWN   = 0xD0;
constexpr int DIR_UP     = 0xD1;
constexpr int DIR_LEFT   = 0xD2;
constexpr int DIR_RIGHT  = 0xD3;

/// `NPC_MOVEMENT_*` -- the direction is the **top two bits of the random byte**.
constexpr int NPC_MOVEMENT_UP    = 0x40;
constexpr int NPC_MOVEMENT_LEFT  = 0x80;
constexpr int NPC_MOVEMENT_RIGHT = 0xC0;

enum Dir { Down, Up, Left, Right };

/// `lb bc, <bit>, <facing>` in TryWalking's four branches.
int dirBit(Dir d)
{
  switch (d) {
    case Down:  return 4;
    case Up:    return 8;
    case Left:  return 2;
    case Right: return 1;
  }
  return 0;
}

int facingOf(Dir d)
{
  switch (d) {
    case Down:  return SpriteFacing::Down;
    case Up:    return SpriteFacing::Up;
    case Left:  return SpriteFacing::Left;
    case Right: return SpriteFacing::Right;
  }
  return SpriteFacing::Down;
}

int dyOf(Dir d) { return (d == Down) ? 1 : (d == Up) ? -1 : 0; }
int dxOf(Dir d) { return (d == Right) ? 1 : (d == Left) ? -1 : 0; }

/// A byte, as the Game Boy holds it: -1 is $FF.
int asByte(int v) { return v & 0xFF; }

/// A step vector byte back to a signed -1/0/+1.
int asSigned(int b) { return (b & 0x80) ? (b - 256) : b; }

/// The Game Boy's `swap a` -- the two nibbles change places.
///
/// ⚠️ It is ×16 **only while the value fits in a nibble**. Past that the high nibble wraps around into
/// the low one, and the result is nonsense -- which is exactly the point: for a sprite far from the
/// player, that nonsense is what the off-screen check rejects.
int swapNibbles(int b) { return asByte(((b & 0x0F) << 4) | ((b & 0xF0) >> 4)); }

} // namespace

MapSim::MapSim(MapModel* map, QObject* parent)
  : QObject(parent), map(map)
{
  // ⚠️ THE CONSOLE'S OWN FRAME RATE. Every counter in this file is measured in Game Boy frames --
  // the delay is `Random() & $7F` FRAMES, a step is 16 FRAMES -- so the only interval that makes any
  // of it mean what it means is 1/59.7275 s. Not 60: the difference is half a percent, and this is
  // an emulator.
  timer.setInterval(qRound(1000.0 / 59.7275));
  connect(&timer, &QTimer::timeout, this, &MapSim::onTick);

  if (map != nullptr)
    connect(map, &MapModel::changed, this, &MapSim::canSimulateChanged);
}

bool MapSim::playing() const { return play; }

void MapSim::setPlaying(bool playing)
{
  if (play == playing)
    return;

  // ⚠️ Never on the offscreen platform. Every headless run must be deterministic, and this thing
  // REWRITES THE SAVE. A test whose fixture quietly wanders off is the worst kind of flap.
  if (playing && QGuiApplication::platformName() == QLatin1String("offscreen"))
    return;

  play = playing;

  if (play)
    timer.start();
  else
    timer.stop();

  emit playingChanged();
}

qreal MapSim::speed() const { return rate; }

void MapSim::setSpeed(qreal speed)
{
  speed = qBound(0.25, speed, 4.0);
  if (qFuzzyCompare(rate, speed))
    return;

  rate = speed;
  timer.setInterval(qMax(1, qRound(1000.0 / (59.7275 * rate))));
  emit speedChanged();
}

bool MapSim::canSimulate() const
{
  // Everybody moves -- a STAY sprite turns on the spot, which is motion, and is the whole point of
  // getting this right. So: is there anybody here at all?
  return map != nullptr && !map->npcList().isEmpty();
}

void MapSim::onTick()
{
  step();
}

// ── CanWalkOntoTile ─────────────────────────────────────────────────────────────────────────

bool MapSim::canWalkOntoTile(SpriteData* s, Dir_ d)
{
  const Dir dir = static_cast<Dir>(d);
  const int dy = dyOf(dir);
  const int dx = dxOf(dir);

  const int m1 = s->movementByte;

  // 1. Scripted movement is ALWAYS allowed.
  if (m1 < WALK)
    return true;

  // 2. Is the destination tile in the tileset's passable list? (There is no "wall" flag in the ROM --
  //    there is a list of tiles you CAN walk on, and absence from it is the wall.)
  if (!tilePassable(s->mapX - 4 + dx, s->mapY - 4 + dy))
    return false;

  // 3. ⚠️ STAY. "if $ff, no movement allowed (however, changing direction is)" -- and the facing has
  //    ALREADY been written by TryWalking, which is why Oak turns.
  if (m1 == STAY)
    return false;

  // 4/5. ⚠️ OFF THE SCREEN -- and this is the rule that surprises people.
  //
  // `yPixels`/`xPixels` are the sprite's position RELATIVE TO THE PLAYER. So these two checks do not
  // just stop a sprite walking off the edge of the screen: they stop a sprite that is ALREADY far
  // from the player from moving at all. The console has no tilemap for anywhere else -- it literally
  // cannot -- so **the game only ever animates the people near you**.
  //
  // Pallet Town's Fisherman, eight tiles below Red, does not take a single step until Red walks
  // towards him. That is not our bug; that is the cartridge, and reproducing it is the assignment.
  if (asByte(s->yPixels + 4 + dy) >= 0x80)
    return false;
  if (asByte(s->xPixels + dx) >= 0x90)
    return false;

  // 6. Another sprite in the way. `collisionData` carries a bit per blocked direction.
  if (s->collisionData & dirBit(dir))
    return false;

  // 7. ⚠️ THE DISPLACEMENT BOOKKEEPING, AND IT IS BUGGED -- ON PURPOSE, AND WE COPY IT.
  //
  //    yDisplacement starts at 8. Walk UP five times and it is 3. Now try to walk DOWN: 3 + 1 = 4,
  //    which is < 5, so it is REFUSED -- and the sprite can never come back down. The disassembly
  //    says so itself: "this line makes sprites get stuck whenever they walked upwards 5 steps".
  //
  //    And the horizontal check has a `cp $5` with NO conditional jump after it, so X is never
  //    limited at all. Also a bug. Also copied. This is the game.
  int yDisp = s->yDisp;
  int xDisp = s->xDisp;

  if (dy >= 0) {
    yDisp = asByte(yDisp + dy);
    if (yDisp < 5)
      return false;
  }
  else {
    if (yDisp == 0)
      return false;      // underflow
    yDisp -= 1;
  }

  if (dx >= 0) {
    xDisp = asByte(xDisp + dx);
    // (the `cp $5` here has no jump -- so nothing is refused. Yes, really.)
  }
  else {
    if (xDisp == 0)
      return false;      // underflow
    xDisp -= 1;
  }

  s->yDisp = yDisp;
  s->yDispChanged();

  s->xDisp = xDisp;
  s->xDispChanged();

  return true;
}

/// The tile at MAP coordinates (@p x, @p y), and whether the tileset says you may stand on it.
bool MapSim::tilePassable(int x, int y)
{
  if (map == nullptr)
    return false;

  if (x < 0 || y < 0 || x >= map->blocksWide() * 2 || y >= map->blocksHigh() * 2)
    return false;

  // A map coordinate is a HALF-BLOCK -- a 2x2 patch of 8px tiles. The game checks the tile the
  // sprite would stand on, which is the bottom-left of that patch (its feet).
  const MapEngine::Buffer buf = MapEngine::buildOverworldMap(map->mapInd(), map->borderBlock());
  if (!buf.valid)
    return false;

  const int bx = MapEngine::mapBorder + x / 2;
  const int by = MapEngine::mapBorder + y / 2;

  const int block = MapEngine::blockAt(buf, bx, by);
  if (block < 0)
    return false;

  const QByteArray tiles = MapEngine::blockTileIds(map->tilesetInd(), block);
  if (tiles.size() < 16)
    return false;

  // Within the block: which of the 2x2 half-blocks, and then the sprite's FEET (the lower-left tile
  // of that quadrant) -- which is what `GetTileSpriteStandsOn` fetches.
  const int qx = (x % 2) * 2;
  const int qy = (y % 2) * 2;
  const int tile = static_cast<quint8>(tiles.at((qy + 1) * 4 + qx));

  const auto traits = TileTraitsDB::inst()->traitsOf(map->tilesetInd(),
                                                     static_cast<var8>(tile),
                                                     0xFF, {});
  return traits.testFlag(TileTraitsDB::Passable);
}

// ── .impassable ─────────────────────────────────────────────────────────────────────────────

void MapSim::failToWalk(SpriteData* s)
{
  s->movementStatus = SpriteMovementStatus::Delayed;   // 2
  s->movementStatusChanged();

  s->yStepVector = 0;
  s->yStepVectorChanged();

  s->xStepVector = 0;
  s->xStepVectorChanged();

  // ⚠️ `and $7f` -- and a rolled ZERO means a delay of 256 frames, not 0, because the counter is
  // decremented and only THEN tested. The disassembly flags it as a probable bug. It is the game.
  s->movementDelay = Random::inst()->rangeInclusive(0, 0x7F);
  s->movementDelayChanged();
}

// ── TryWalking ──────────────────────────────────────────────────────────────────────────────

void MapSim::tryWalking(SpriteData* s, Dir_ d)
{
  const Dir dir = static_cast<Dir>(d);

  // ⚠️ THE FACING IS WRITTEN FIRST -- before we ask whether the step is even legal. That single
  // ordering is why a STAY sprite turns on the spot instead of being frozen.
  s->faceDir = facingOf(dir);
  s->faceDirChanged();

  s->yStepVector = asByte(dyOf(dir));
  s->yStepVectorChanged();

  s->xStepVector = asByte(dxOf(dir));
  s->xStepVectorChanged();

  if (!canWalkOntoTile(s, d)) {
    failToWalk(s);
    return;
  }

  s->mapY = asByte(s->mapY + dyOf(dir));
  s->mapYChanged();

  s->mapX = asByte(s->mapX + dxOf(dir));
  s->mapXChanged();

  s->walkAnimationCounter = 0x10;   // 16 frames per step
  s->walkAnimationCounterChanged();

  s->movementStatus = SpriteMovementStatus::Moving;   // 3
  s->movementStatusChanged();
}

// ── UpdateNPCSprite ─────────────────────────────────────────────────────────────────────────

void MapSim::step()
{
  if (map == nullptr || map->npcSprites() == nullptr)
    return;

  AreaSprites* npcs = map->npcSprites();
  bool moved = false;

  for (int slot = 1; slot < npcs->spriteCount(); slot++) {
    SpriteData* s = npcs->spriteAt(slot);
    if (s == nullptr || s->pictureID == 0)
      continue;

    const int m1 = s->movementByte;
    const int m2 = s->getRangeDirByte();   // movement byte 2, out of wMapSpriteData

    // ── status 0: InitializeSpriteStatus ────────────────────────────────────────────────
    if (s->movementStatus == SpriteMovementStatus::UnInit) {
      s->movementStatus = SpriteMovementStatus::Ready;
      s->movementStatusChanged();

      s->imageIndex = 0xFF;
      s->imageIndexChanged();

      s->yDisp = 8;
      s->yDispChanged();

      s->xDisp = 8;
      s->xDispChanged();

      continue;
    }

    // ── status 3: UpdateSpriteInWalkingAnimation ────────────────────────────────────────
    if (s->movementStatus == SpriteMovementStatus::Moving) {
      s->intraAnimationFrameCounter = asByte(s->intraAnimationFrameCounter + 1);

      if (s->intraAnimationFrameCounter == 4) {
        s->intraAnimationFrameCounter = 0;
        s->animFrameCounter = (s->animFrameCounter + 1) & 3;   // 4 frames x 4 ticks = one step
        s->animFrameCounterChanged();
      }
      s->intraAnimationFrameCounterChanged();

      s->yPixels = asByte(s->yPixels + asSigned(s->yStepVector));
      s->yPixelsChanged();

      s->xPixels = asByte(s->xPixels + asSigned(s->xStepVector));
      s->xPixelsChanged();

      s->walkAnimationCounter = asByte(s->walkAnimationCounter - 1);
      s->walkAnimationCounterChanged();

      if (s->walkAnimationCounter != 0)
        continue;

      // The step is finished.
      if (m1 >= WALK) {
        s->movementDelay = Random::inst()->rangeInclusive(0, 0x7F);
        s->movementDelayChanged();

        s->movementStatus = SpriteMovementStatus::Delayed;
        s->movementStatusChanged();

        s->yStepVector = 0;
        s->yStepVectorChanged();

        s->xStepVector = 0;
        s->xStepVectorChanged();
      }
      else {
        s->movementStatus = SpriteMovementStatus::Ready;
        s->movementStatusChanged();
      }

      moved = true;
      continue;
    }

    // ── status 2: UpdateSpriteMovementDelay ─────────────────────────────────────────────
    if (s->movementStatus == SpriteMovementStatus::Delayed) {
      if (m1 >= WALK) {
        s->movementDelay = asByte(s->movementDelay - 1);
        s->movementDelayChanged();

        if (s->movementDelay != 0) {
          // NotYetMoving: stand still rather than hold a walk pose.
          s->animFrameCounter = 0;
          s->animFrameCounterChanged();
          continue;
        }
      }
      else {
        s->movementDelay = 0;
        s->movementDelayChanged();
      }

      s->movementStatus = SpriteMovementStatus::Ready;
      s->movementStatusChanged();

      s->animFrameCounter = 0;
      s->animFrameCounterChanged();
      continue;
    }

    // ── status 1: Ready. Roll, turn, and try to step. ───────────────────────────────────
    //
    // ⚠️ WALK **and STAY** both come down here. See the file header.
    if (m1 != WALK && m1 != STAY)
      continue;   // a scripted sprite -- not something a save editor drives

    // ── InitializeSpriteScreenPosition ──────────────────────────────────────────────────
    //
    //     ld a, [wYCoord] / ld b, a
    //     ld a, [hl]      ; MAPY
    //     sub b           ; relative to the player
    //     swap a          ; * 16
    //     sub $4          ; the 4-pixel lift
    //
    // ⚠️ TWO things to get right, and I got both wrong first time:
    //
    //   * **Do NOT take the +4 bias off `mapY`.** The game subtracts the player's *raw* coordinate
    //     from the *biased* one, and the leftover 4 is exactly what centres the sprite on screen. A
    //     sprite standing where the player stands gives 4 → 64 px → the middle. Removing the bias
    //     first put every sprite left of the player at a negative x, which as a byte is ≥ $90, which
    //     the off-screen check refuses -- so NOBODY EVER MOVED.
    //
    //   * **`swap` is a NIBBLE SWAP, not a multiply.** It is ×16 only while the value fits in a
    //     nibble; past that the high nibble wraps around into the low one. For a sprite far from the
    //     player that wrap is the whole point -- it is what makes the number nonsense, which is what
    //     makes the off-screen check reject it.
    const int dyRel = asByte(s->mapY - map->playerY());
    const int dxRel = asByte(s->mapX - map->playerX());

    s->yPixels = asByte(swapNibbles(dyRel) - 4);
    s->yPixelsChanged();

    s->xPixels = swapNibbles(dxRel);
    s->xPixelsChanged();

    const int r = Random::inst()->rangeInclusive(0, 0xFF);

    // The direction: movement byte 2 overrides the roll, or bends it onto an axis.
    Dir dir;

    if (m2 == DIR_DOWN)       dir = Down;
    else if (m2 == DIR_UP)    dir = Up;
    else if (m2 == DIR_LEFT)  dir = Left;
    else if (m2 == DIR_RIGHT) dir = Right;
    else {
      // The top TWO BITS of the random byte -- NPC_MOVEMENT_DOWN/UP/LEFT/RIGHT.
      if (r < NPC_MOVEMENT_UP)          dir = (m2 == LEFT_RIGHT) ? Left  : Down;
      else if (r < NPC_MOVEMENT_LEFT)   dir = (m2 == LEFT_RIGHT) ? Right : Up;
      else if (r < NPC_MOVEMENT_RIGHT)  dir = (m2 == UP_DOWN)    ? Up    : Left;
      else                              dir = (m2 == UP_DOWN)    ? Down  : Right;
    }

    tryWalking(s, static_cast<Dir_>(dir));
    moved = true;
  }

  if (moved) {
    npcs->spritesChanged();
    emit ticked();
  }
}
