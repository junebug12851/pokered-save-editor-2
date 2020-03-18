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
#ifndef MAPDBENTRYWARPOUT_H
#define MAPDBENTRYWARPOUT_H

#include <QObject>
#include <QJsonValue>
#include "../db_autoport.h"

class QQmlEngine;
class MapDBEntry;
class MapsDB;
class MapDBEntryWarpIn;

// List of Warps on Map that warp out to a different map
// They can only warp to a "warp-in" point
struct DB_AUTOPORT MapDBEntryWarpOut : public QObject {
  Q_OBJECT
  Q_PROPERTY(int getX READ getX CONSTANT)
  Q_PROPERTY(int getY READ getY CONSTANT)
  Q_PROPERTY(int getWarp READ getWarp CONSTANT)
  Q_PROPERTY(QString getMap READ getMap CONSTANT)
  Q_PROPERTY(bool getGlitch READ getGlitch CONSTANT)
  Q_PROPERTY(MapDBEntry* getToMap READ getToMap CONSTANT)
  Q_PROPERTY(MapDBEntry* getParent READ getParent CONSTANT)
  Q_PROPERTY(MapDBEntryWarpIn* getToWarp READ getToWarp CONSTANT)

public:
  int getX() const;
  int getY() const;
  int getWarp() const;
  const QString getMap() const;
  bool getGlitch() const;
  const MapDBEntry* getToMap() const;
  const MapDBEntry* getParent() const;
  const MapDBEntryWarpIn* getToWarp() const;

public slots:
  void qmlProtect(const QQmlEngine* const engine) const;

protected:
  MapDBEntryWarpOut();
  MapDBEntryWarpOut(const QJsonValue& data, MapDBEntry* const parent);
  void deepLink();
  void qmlRegister() const;

  // X & Y location on Map
  int x = 0;
  int y = 0;

  // Which pre-defined warp-in to warp to
  int warp = 0;

  // Which map to warp to
  QString map = "";

  // Is this warp-out not intended to be used
  bool glitch = false;

  // Go to map
  MapDBEntry* toMap = nullptr;
  MapDBEntry* parent = nullptr;

  // Go to warp spot on destination map
  MapDBEntryWarpIn* toWarp = nullptr;

  friend class MapDBEntry;
  friend class MapsDB;
  friend class MapDBEntryWarpIn;
};

#endif // MAPDBENTRYWARPOUT_H
