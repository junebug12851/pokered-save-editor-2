/*
  * Copyright 2020 Fairy Fox
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
#include "../db_autoport.h"

class QQmlEngine;
class MapDBEntry;
class MapsDB;
class MapDBEntryWarpIn;

// List of Warps on Map that warp out to a different map
// They can only warp to a "warp-in" point
/**
 * @brief A warp-out point: a tile that warps the player to another map.
 *
 * As the note says, a warp-out can only target a "warp-in" point (@ref warp index
 * on @ref map). deepLink() resolves @ref toMap and @ref toWarp. A leaf of
 * MapDBEntry. See db.md.
 *
 * @see MapDBEntry (parent), MapDBEntryWarpIn, WarpData (the save-side warp).
 */
struct DB_AUTOPORT MapDBEntryWarpOut : public QObject {
  Q_OBJECT
  Q_PROPERTY(int getX READ getX CONSTANT)             ///< Warp-out tile X.
  Q_PROPERTY(int getY READ getY CONSTANT)             ///< Warp-out tile Y.
  Q_PROPERTY(int getWarp READ getWarp CONSTANT)       ///< Target warp-in index on the destination map.
  Q_PROPERTY(QString getMap READ getMap CONSTANT)     ///< Destination map name.
  Q_PROPERTY(bool getGlitch READ getGlitch CONSTANT)  ///< Whether this warp is unintended (glitch).
  Q_PROPERTY(MapDBEntry* getToMap READ getToMap CONSTANT) ///< Resolved destination map.
  Q_PROPERTY(MapDBEntry* getParent READ getParent CONSTANT) ///< Owning map.
  Q_PROPERTY(MapDBEntryWarpIn* getToWarp READ getToWarp CONSTANT) ///< Resolved destination warp-in.

public:
  int getX() const;             ///< @see getX property.
  int getY() const;             ///< @see getY property.
  int getWarp() const;          ///< @see getWarp property.
  const QString getMap() const; ///< @see getMap property.
  bool getGlitch() const;       ///< @see getGlitch property.
  MapDBEntry* getToMap() const; ///< @see getToMap property.
  MapDBEntry* getParent() const; ///< @see getParent property.
  MapDBEntryWarpIn* getToWarp() const; ///< @see getToWarp property.

public slots:
  void qmlProtect(const QQmlEngine* const engine) const; ///< Pin to C++ ownership.

protected:
  MapDBEntryWarpOut(); ///< Empty entry.
  MapDBEntryWarpOut(const QJsonValue& data, MapDBEntry* const parent); ///< Build from JSON under @p parent.
  void deepLink();          ///< Resolve the destination map + warp-in.
  void qmlRegister() const; ///< Register with QML.

  // X & Y location on Map
  int x = 0; ///< Warp-out tile X.
  int y = 0; ///< Warp-out tile Y.

  // Which pre-defined warp-in to warp to
  int warp = 0; ///< Target warp-in index.

  // Which map to warp to
  QString map = ""; ///< Destination map name.

  // Is this warp-out not intended to be used
  bool glitch = false; ///< Glitch/unintended warp.

  // Go to map
  MapDBEntry* toMap = nullptr; ///< Resolved destination map (deepLink).
  MapDBEntry* parent = nullptr; ///< Owning map.

  // Go to warp spot on destination map
  MapDBEntryWarpIn* toWarp = nullptr; ///< Resolved destination warp-in (deepLink).

  friend class MapDBEntry;
  friend class MapsDB;
  friend class MapDBEntryWarpIn;
};
