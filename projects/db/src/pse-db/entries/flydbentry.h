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
#ifndef FLYDBENTRY_H
#define FLYDBENTRY_H

#include <QObject>
#include <QString>
#include <QHash>
#include <QJsonValue>

#include "../db_autoport.h"

struct MapDBEntry;
class QQmlEngine;
class FlyDB;

// With amazing help of Quicktype!!!
// https://app.quicktype.io

// Cities you can fly to

struct DB_AUTOPORT FlyDBEntry : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString getName READ getName CONSTANT)
  Q_PROPERTY(int ind READ ind CONSTANT)
  Q_PROPERTY(MapDBEntry* toMap READ toMap CONSTANT)

public:
  const QString getName() const;
  int getInd() const;
  const MapDBEntry* getToMap() const;

public slots:
  void qmlProtect(const QQmlEngine* const engine) const;

protected:
  FlyDBEntry();
  FlyDBEntry(QJsonValue& data);

  void deepLink();
  void qmlRegister() const;

  QString name = ""; // City Name
  int ind = 0; // Index in list
  MapDBEntry* toMap = nullptr; // Deep link to associated map data

  friend class FlyDB;
};

#endif // FLYDBENTRY_H
