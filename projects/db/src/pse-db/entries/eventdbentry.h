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
#ifndef EVENTDBENTRY_H
#define EVENTDBENTRY_H

#include <QObject>
#include <QJsonValue>

#include "../db_autoport.h"

struct MapDBEntry;
class QQmlEngine;
class EventsDB;

// With amazing help of Quicktype!!!
// https://app.quicktype.io

// In-Game events, there's like a million of them, not kidding lol. Every little
// thing you do changes and moves around events

struct DB_AUTOPORT EventDBEntry : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString getName READ getName CONSTANT)
  Q_PROPERTY(int getInd READ getInd CONSTANT)
  Q_PROPERTY(int getByte READ getByte CONSTANT)
  Q_PROPERTY(int getBit READ getBit CONSTANT)
  Q_PROPERTY(int getMapsSize READ getMapsSize CONSTANT)
  Q_PROPERTY(int getToMapsSize READ getToMapsSize CONSTANT)

public:
  const QString getName() const;
  int getInd() const;
  int getByte() const;
  int getBit() const;

  const QVector<QString> getMaps() const;
  int getMapsSize() const;
  Q_INVOKABLE const QString getMapAt(int ind) const;

  const QVector<MapDBEntry*> getToMaps() const;
  int getToMapsSize() const;
  Q_INVOKABLE const MapDBEntry* getToMapAt(int ind) const;

public slots:
  void qmlProtect(const QQmlEngine* const engine) const;

protected:
  EventDBEntry();
  EventDBEntry(QJsonValue& data);

  void deepLink();
  void qmlRegister() const;

  QString name = ""; // Event name
  int ind = 0; // Internal index
  int byte = 0; // Byte in SAV file
  int bit = 0; // Bit in byte
  QVector<QString> maps; // Associated Maps
  QVector<MapDBEntry*> toMaps; // To Associated Maps

  friend class EventsDB;
};

#endif // EVENTDBENTRY_H
