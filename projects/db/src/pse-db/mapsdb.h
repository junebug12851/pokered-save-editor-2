/*
  * Copyright 2019 June Hanabi
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
#ifndef MAP_H
#define MAP_H

#include <QObject>
#include <QVector>
#include <QHash>
#include "./db_autoport.h"

class MapDBEntry;
class QQmlEngine;
class MapSearch;

// With amazing help of Quicktype!!!
// https://app.quicktype.io

class DB_AUTOPORT MapsDB : public QObject
{
  Q_OBJECT
  Q_PROPERTY(int getStoreSize READ getStoreSize CONSTANT)
  Q_PROPERTY(MapSearch* search READ searchRaw STORED false)

public:
  // Get Instance
  static MapsDB* inst();

  // Get Properties, includes QML array helpers
  const QVector<MapDBEntry*> getStore() const;
  const QHash<QString, MapDBEntry*> getInd() const;
  int getStoreSize() const;

  MapSearch* searchRaw() const;
  const QScopedPointer<const MapSearch, QScopedPointerDeleteLater> search() const;

  // QML Methods that can't be a property or slot because they take an argument
  Q_INVOKABLE const MapDBEntry* getStoreAt(const int ind) const;
  Q_INVOKABLE const MapDBEntry* getIndAt(const QString val) const;

public slots:
  void load();
  void index();
  void deepLink();
  void qmlProtect(const QQmlEngine* const engine) const;

private slots:
  void qmlRegister() const;

private:
  MapsDB();

  QVector<MapDBEntry*> store;
  QHash<QString, MapDBEntry*> ind;
};

#endif // MAP_H
