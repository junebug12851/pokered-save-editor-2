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
#ifndef MAPDBENTRYWARPIN_H
#define MAPDBENTRYWARPIN_H

#include <QObject>
#include <QJsonValue>
#include "../db_autoport.h"

class QQmlEngine;
class MapDBEntry;
class MapsDB;
class MapDBEntryWarpOut;

struct DB_AUTOPORT MapDBEntryWarpIn : public QObject {
  Q_OBJECT
  Q_PROPERTY(int getX READ getX CONSTANT)
  Q_PROPERTY(int getY READ getY CONSTANT)
  Q_PROPERTY(int getToConnectingWarpsSize READ getToConnectingWarpsSize CONSTANT)
  Q_PROPERTY(MapDBEntry* getParent READ getParent CONSTANT)

public:
  int getX() const;
  int getY() const;

  const QVector<MapDBEntryWarpOut*> getToConnectingWarps() const;
  int getToConnectingWarpsSize() const;
  Q_INVOKABLE MapDBEntryWarpOut* getToConnectingWarpsAt(const int ind) const;

  const MapDBEntry* getParent() const;

public slots:
  void qmlProtect(const QQmlEngine* const engine) const;

protected:
  MapDBEntryWarpIn();
  MapDBEntryWarpIn(const QJsonValue& data, MapDBEntry* const parent);
  void qmlRegister() const;

  // X & Y location on Map
  int x = 0;
  int y = 0;

  QVector<MapDBEntryWarpOut*> toConnectingWarps;
  MapDBEntry* parent = nullptr;

  friend class MapDBEntry;
  friend class MapsDB;
  friend class MapDBEntryWarpOut;
};

#endif // MAPDBENTRYWARPIN_H
