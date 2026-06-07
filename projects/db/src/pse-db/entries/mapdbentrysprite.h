/*
  * Copyright 2020 Twilight
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
#include "../db_autoport.h"

class MapDBEntry;
class MissableDBEntry;
class SpriteDBEntry;
class QQmlEngine;
class MapsDB;

// Sprites are a tad complicated only becuase there are different kinds of
// sprites having different set of options and all rolled into one data set.
// Sprites are the 2nd most complicated data structure mainly because sprite
// data is all over the place and in so many different forms and variables
// it's actually a very messy system because it feels like this is one area
// the developers were most trying to figure out through development.
/**
 * @brief A map's sprite definition -- base class for the four sprite kinds.
 *
 * Carries the fields common to every on-map sprite (position, movement, facing,
 * text, missable link, and the resolved @ref toSprite). The @ref SpriteType
 * distinguishes NPC / ITEM / POKEMON / TRAINER; the four subclasses
 * (MapDBEntrySpriteNpc/Item/Pokemon/Trainer) override type() and add their extra
 * data. The base comment above is Twilight's note on why this is messy. See db.md.
 *
 * @see MapDBEntry (parent), SpriteData (the save-side sprite), the four subclasses.
 */
struct DB_AUTOPORT MapDBEntrySprite : public QObject
{
  Q_OBJECT
  Q_PROPERTY(int adjustedX READ adjustedX CONSTANT)     ///< X adjusted for Gen 1 placement.
  Q_PROPERTY(int adjustedY READ adjustedY CONSTANT)     ///< Y adjusted for Gen 1 placement.
  Q_PROPERTY(SpriteType type READ type CONSTANT)        ///< Which kind of sprite this is.
  Q_PROPERTY(QString getSprite READ getSprite CONSTANT) ///< Sprite name.
  Q_PROPERTY(int getX READ getX CONSTANT)               ///< Raw X.
  Q_PROPERTY(int getY READ getY CONSTANT)               ///< Raw Y.
  Q_PROPERTY(QString getMove READ getMove CONSTANT)     ///< Movement (walk/stay/...).
  Q_PROPERTY(int getText READ getText CONSTANT)         ///< Interaction text id.
  Q_PROPERTY(int getRange READ getRange CONSTANT)       ///< Wander range (mutually exclusive with face).
  Q_PROPERTY(QString getFace READ getFace CONSTANT)     ///< Static facing (mutually exclusive with range).
  Q_PROPERTY(int getMissable READ getMissable CONSTANT) ///< Missable index, if any.
  Q_PROPERTY(MissableDBEntry* getToMissable READ getToMissable CONSTANT) ///< Resolved missable.
  Q_PROPERTY(SpriteDBEntry* getToSprite READ getToSprite CONSTANT)       ///< Resolved sprite picture.
  Q_PROPERTY(MapDBEntry* getParent READ getParent CONSTANT)             ///< Owning map.

public:
  // Details on all the maps in the game

  /// The four sprite kinds (plus an ERROR sentinel for the type-less base).
  enum SpriteType
  {
    // The sprite is a simple NPC
    NPC,

    // The sprite is an item that can be obtained
    ITEM,

    // The sprite is a one-time Pokemon that can be battled and beaten
    POKEMON,

    // The sprite is a trainer which has a team that can be battled and beaten
    TRAINER,

    // There's no child class for some reason, your asking the parent who doesn't
    // know
    ERROR
  };
  Q_ENUM(SpriteType)

  // X & Y Coordinates adjusted for the gen 1 games
  int adjustedX() const; ///< @see adjustedX property.
  int adjustedY() const; ///< @see adjustedY property.
  virtual SpriteType type() const; ///< The sprite kind (overridden by subclasses).
  const QString getSprite() const; ///< @see getSprite property.
  int getX() const;                ///< @see getX property.
  int getY() const;                ///< @see getY property.
  const QString getMove() const;   ///< @see getMove property.
  int getText() const;             ///< @see getText property.
  int getRange() const;            ///< @see getRange property.
  const QString getFace() const;   ///< @see getFace property.
  int getMissable() const;         ///< @see getMissable property.
  MissableDBEntry* getToMissable() const; ///< @see getToMissable property.
  SpriteDBEntry* getToSprite() const;     ///< @see getToSprite property.
  MapDBEntry* getParent() const;          ///< @see getParent property.

public slots:
  void qmlProtect(const QQmlEngine* const engine) const; ///< Pin to C++ ownership.

protected:
  MapDBEntrySprite();                    ///< Empty entry.
  MapDBEntrySprite(const QJsonValue& data, MapDBEntry* const parent); ///< Build from JSON under @p parent.
  virtual void deepLink();               ///< Resolve sprite/missable links.
  virtual void qmlRegister() const;      ///< Register with QML.

  // Name of sprite
  QString sprite; ///< Sprite name (read via getSprite()).

  // X & Y position on map
  int x = 0; ///< Raw X.
  int y = 0; ///< Raw Y.

  // Whether the sprite is moving or remaining still
  // Walk, Stay
  // The game only uses those 2 options but if a 3rd value is present it will
  // move without collision detection
  QString move = ""; ///< Movement mode (see note).

  // Text when interacting with sprite
  int text = -1; ///< Interaction text id.

  // Is the sprite allowed room to move?
  // Or is it given a static facing position
  // Only one or the other can be filled in, not both
  int range = -1;    ///< Wander range (exclusive with face).
  QString face = ""; ///< Static facing (exclusive with range).

  // Is this sprite a missable, if so to which missable?
  int missable = -1;                     ///< Missable index, or -1.
  MissableDBEntry* toMissable = nullptr; ///< Resolved missable (deepLink).

  // To Sprite
  SpriteDBEntry* toSprite = nullptr; ///< Resolved sprite picture (deepLink).

  // Parent Map Entry
  MapDBEntry* parent = nullptr; ///< Owning map.

  friend class MapsDB;
  friend class MapDBEntry;
};
