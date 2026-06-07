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
#include <QJsonValue>
#include <QString>
#include <QHash>

#include "../db_autoport.h"

struct MapDBEntry;
struct MapDBEntrySprite;
class QQmlEngine;
class MissablesDB;

// With amazing help of Quicktype!!!
// https://app.quicktype.io

// Missable flags set in-game, a missable is simply a script and/or sprite
// that never loads (Is surpressed). Allows the game to hide things you
// shouldn't see or encounter yet or show a new map "state" after you progressed
// in the games.

// The starter you and your rival pick are both missable activated and the
// guy blocking the path in Pewter City is a missable that's hiden once you beat
// Brock.

/**
 * @brief One missable definition: a script/sprite that can be hidden or shown.
 *
 * QObject-getter style DB entry. A "missable" is the game's mechanism for
 * hiding/showing things based on progress (see the explanation above). Each entry
 * names the missable and links to the @ref toMap / @ref toMapSprite it controls
 * (resolved in deepLink). @ref defShow is its default visibility.
 *
 * @see MissablesDB, WorldMissables (the save-side flags), SpriteData::missableIndex.
 */
struct DB_AUTOPORT MissableDBEntry : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString getName READ getName CONSTANT)         ///< Missable name.
  Q_PROPERTY(int getInd READ getInd CONSTANT)               ///< Missable index (the bit it controls).
  Q_PROPERTY(QString getMap READ getMap CONSTANT)           ///< Name of the map it's on.
  Q_PROPERTY(int getSprite READ getSprite CONSTANT)         ///< Sprite index on that map.
  Q_PROPERTY(bool getDefShow READ getDefShow CONSTANT)      ///< Default visibility.
  Q_PROPERTY(MapDBEntry* getToMap READ getToMap CONSTANT)   ///< Resolved map.
  Q_PROPERTY(MapDBEntrySprite* getToMapSprite READ getToMapSprite CONSTANT) ///< Resolved map sprite.

public:
  const QString getName() const;       ///< @see getName property.
  int getInd() const;                  ///< @see getInd property.
  const QString getMap() const;        ///< @see getMap property.
  int getSprite() const;               ///< @see getSprite property.
  bool getDefShow() const;             ///< @see getDefShow property.
  MapDBEntry* getToMap() const;        ///< @see getToMap property.
  MapDBEntrySprite* getToMapSprite() const; ///< @see getToMapSprite property.

public slots:
  void qmlProtect(const QQmlEngine* const engine) const; ///< Pin to C++ ownership.

protected:
  MissableDBEntry();                ///< Empty entry (built by MissablesDB).
  MissableDBEntry(QJsonValue& data); ///< Build from a JSON value.
  void deepLink();                  ///< Resolve the map + map-sprite links.
  void qmlRegister() const;         ///< Register with QML.

  // Missable Name & Index
  QString name = ""; ///< Backing field (read via getName()).
  int ind = 0;       ///< Backing field (read via getInd()).

  // Map & Sprite on map Missable References
  QString map = ""; ///< Backing field (read via getMap()).
  int sprite = 0;    ///< Backing field (read via getSprite()).

  // Is this missable shown or hidden by default
  bool defShow = false; ///< Backing field (read via getDefShow()).

  // Deep link to associated map and sprite on map
  // There are 2 exceptions to this
  // * There's one missable that references a sprite on an incomplete map with
  //   no sprites
  // * There's one missable that references an extra sprite which isn't there
  //
  // In both cases one or both of these will be nullptr
  MapDBEntry* toMap = nullptr;            ///< Resolved map (may be null; see note above).
  MapDBEntrySprite* toMapSprite = nullptr; ///< Resolved map sprite (may be null; see note above).

  friend class MissablesDB; ///< Owning DB constructs/populates entries.
};
