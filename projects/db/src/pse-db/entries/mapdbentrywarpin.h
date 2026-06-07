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
#include "../db_autoport.h"

class QQmlEngine;
class MapDBEntry;
class MapsDB;
class MapDBEntryWarpOut;

/**
 * @brief A warp-in point: a destination spot other maps' warp-outs land on.
 *
 * Counterpart to MapDBEntryWarpOut -- a warp-out targets a warp-in by index. The
 * @ref toConnectingWarps list (surfaced via size + invokable accessor) is the set
 * of warp-outs that arrive here. A leaf of MapDBEntry. See db.md.
 *
 * @see MapDBEntry (parent), MapDBEntryWarpOut, WarpData (the save-side warp).
 */
struct DB_AUTOPORT MapDBEntryWarpIn : public QObject {
  Q_OBJECT
  Q_PROPERTY(int getX READ getX CONSTANT) ///< Warp-in tile X.
  Q_PROPERTY(int getY READ getY CONSTANT) ///< Warp-in tile Y.
  Q_PROPERTY(int getToConnectingWarpsSize READ getToConnectingWarpsSize CONSTANT) ///< Count of warp-outs arriving here.
  Q_PROPERTY(MapDBEntry* getParent READ getParent CONSTANT) ///< Owning map.

public:
  int getX() const; ///< @see getX property.
  int getY() const; ///< @see getY property.

  const QVector<MapDBEntryWarpOut*> getToConnectingWarps() const; ///< Warp-outs that land here.
  int getToConnectingWarpsSize() const;                           ///< @see getToConnectingWarpsSize property.
  Q_INVOKABLE MapDBEntryWarpOut* getToConnectingWarpsAt(const int ind) const; ///< Connecting warp @p ind (for QML).

  MapDBEntry* getParent() const; ///< @see getParent property.

public slots:
  void qmlProtect(const QQmlEngine* const engine) const; ///< Pin to C++ ownership.

protected:
  MapDBEntryWarpIn(); ///< Empty entry.
  MapDBEntryWarpIn(const QJsonValue& data, MapDBEntry* const parent); ///< Build from JSON under @p parent.
  void qmlRegister() const; ///< Register with QML.

  // X & Y location on Map
  int x = 0; ///< Warp-in tile X.
  int y = 0; ///< Warp-in tile Y.

  QVector<MapDBEntryWarpOut*> toConnectingWarps; ///< Warp-outs arriving here (filled by warp-out deepLink).
  MapDBEntry* parent = nullptr;                  ///< Owning map.

  friend class MapDBEntry;
  friend class MapsDB;
  friend class MapDBEntryWarpOut;
};
