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
#ifndef MAPDBENTRYSIGN_H
#define MAPDBENTRYSIGN_H

#include <QObject>
#include <QJsonValue>
#include "../db_autoport.h"

class MapDBEntry;
class QQmlEngine;
class MapDBEntry;

struct DB_AUTOPORT MapDBEntrySign : public QObject
{
  Q_OBJECT
  Q_PROPERTY(int getX READ getX CONSTANT)
  Q_PROPERTY(int getY READ getY CONSTANT)
  Q_PROPERTY(int getTextID READ getTextID CONSTANT)
  Q_PROPERTY(MapDBEntry* getParent READ getParent CONSTANT)

public:
  int getX() const;
  int getY() const;
  int getTextID() const;
  const MapDBEntry* getParent() const;

public slots:
  void qmlProtect(const QQmlEngine* const engine) const;

protected:
  MapDBEntrySign();
  MapDBEntrySign(const QJsonValue& data,
                 MapDBEntry* const parent);
  void qmlRegister() const;

  // X & Y location on Map
  int x = 0;
  int y = 0;

  // Which text id to display when interacting with sign
  int textID = 0;

  MapDBEntry* parent = nullptr;

  friend class MapDBEntry;
};

#endif // MAPDBENTRYSIGN_H
