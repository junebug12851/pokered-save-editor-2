/*
  * Copyright 2020 June Hanabi
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
#ifndef MAPDBENTRYSPRITE_H
#define MAPDBENTRYSPRITE_H

#include <QObject>
#include "../db_autoport.h"

class MapDBEntry;
class MissableDBEntry;
class SpriteDBEntry;

// Sprites are a tad complicated only becuase there are different kinds of
// sprites having different set of options and all rolled into one data set.
// Sprites are the 2nd most complicated data structure mainly because sprite
// data is all over the place and in so many different forms and variables
// it's actually a very messy system because it feels like this is one area
// the developers were most trying to figure out through development.
struct DB_AUTOPORT MapDBEntrySprite
{
  // Details on all the maps in the game

  enum class SpriteType : int
  {
    // The sprite is a simple NPC
    NPC,

    // The sprite is an item that can be obtained
    ITEM,

    // The sprite is a one-time Pokemon that can be battled and beaten
    POKEMON,

    // The sprite is a trainer which has a team that can be battled and beaten
    TRAINER,

    // There's no child class DB_AUTOPORT for some reason, your asking the parent who doesn't
    // know
    ERROR
  };

  MapDBEntrySprite();
  MapDBEntrySprite(QJsonValue& data, MapDBEntry* parent);
  virtual void deepLink();
  virtual SpriteType type();

  // Name of sprite
  QString sprite;

  // X & Y position on map
  int x = 0;
  int y = 0;

  // X & Y Coordinates adjusted for the gen 1 games
  int adjustedX();
  int adjustedY();

  // Whether the sprite is moving or remaining still
  // Walk, Stay
  // The game only uses those 2 options but if a 3rd value is present it will
  // move without collision detection
  QString move;

  // Text when interacting with sprite
  int text = 0;

  // Is the sprite allowed room to move?
  // Or is it given a static facing position
  // Only one or the other can be filled in, not both
  int range;
  QString face;

  // Is this sprite a missable, if so to which missable?
  int missable;
  MissableDBEntry* toMissable = nullptr;

  // To Sprite
  SpriteDBEntry* toSprite = nullptr;

  // Parent Map Entry
  MapDBEntry* parent = nullptr;
};

#endif // MAPDBENTRYSPRITE_H
