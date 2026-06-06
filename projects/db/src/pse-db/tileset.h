/*
  * Copyright 2019 Twilight
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
#include <QVector>
#include <QString>
#include <QHash>

#include <pse-common/types.h>
#include "./db_autoport.h"

struct MapDBEntry;
class QQmlEngine;

enum class TilesetType { INDOOR = 0, CAVE, OUTDOOR };

constexpr var8 talkCount = 3;

struct DB_AUTOPORT TilesetDBEntry {
  TilesetDBEntry();
  TilesetDBEntry(QJsonValue& data);

  QString name;
  QString type;
  QString nameAlias;
  QString typeAlias;

  TilesetType typeAsEnum() const;

  var8 ind      = 0;
  var8 talk[3]  = {0, 0, 0};
  var8 grass    = 0;
  var8 bank     = 0;
  var16 blockPtr = 0;
  var16 gfxPtr   = 0;
  var16 collPtr  = 0;

  QVector<MapDBEntry*> toMaps;
};

class DB_AUTOPORT TilesetDB : public QObject
{
  Q_OBJECT
  Q_PROPERTY(int getStoreSize READ getStoreSize CONSTANT)

public:
  static TilesetDB* inst();

  [[nodiscard]] const QVector<TilesetDBEntry*> getStore() const;
  [[nodiscard]] const QHash<QString, TilesetDBEntry*> getInd() const;
  [[nodiscard]] int getStoreSize() const;

  Q_INVOKABLE TilesetDBEntry* getStoreAt(int idx) const;
  Q_INVOKABLE TilesetDBEntry* getIndAt(const QString& key) const;

public slots:
  void load();
  void index();
  void qmlProtect(const QQmlEngine* const engine) const;

private slots:
  void qmlRegister() const;

private:
  TilesetDB();

  QVector<TilesetDBEntry*> store;
  QHash<QString, TilesetDBEntry*> ind;
};
