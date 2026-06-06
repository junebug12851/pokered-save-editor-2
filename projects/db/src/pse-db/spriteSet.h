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
#include <QString>
#include <QVector>
#include <QHash>
#include <optional>

#include <pse-common/types.h>
#include "./db_autoport.h"

struct SpriteDBEntry;
struct MapDBEntry;
class QQmlEngine;

// Outdoor sprites have to be pre-loaded into memory.
// A SpriteSet is a group of up to 11 sprites kept in memory for a given
// outdoor area. Large maps may use two sets split by coordinate.

struct DB_AUTOPORT SpriteSetDBEntry {
  SpriteSetDBEntry();
  SpriteSetDBEntry(QJsonValue& data);
  void deepLink();

  [[nodiscard]] bool isDynamic() const;
  [[nodiscard]] QVector<SpriteDBEntry*> getSprites(var8 x, var8 y) const;

  var8 ind = 0;
  QString split;

  QVector<QString>      spriteList;
  QVector<SpriteDBEntry*> toSprites;

  std::optional<var8> splitAt;
  std::optional<var8> setWN;
  std::optional<var8> setES;

  SpriteSetDBEntry* toSetWN = nullptr;
  SpriteSetDBEntry* toSetES = nullptr;

  QVector<MapDBEntry*> toMaps;
};

class DB_AUTOPORT SpriteSetDB : public QObject
{
  Q_OBJECT
  Q_PROPERTY(int getStoreSize READ getStoreSize CONSTANT)

public:
  static SpriteSetDB* inst();

  [[nodiscard]] const QVector<SpriteSetDBEntry*> getStore() const;
  [[nodiscard]] const QHash<QString, SpriteSetDBEntry*> getInd() const;
  [[nodiscard]] int getStoreSize() const;

  Q_INVOKABLE SpriteSetDBEntry* getStoreAt(int idx) const;
  Q_INVOKABLE SpriteSetDBEntry* getIndAt(const QString& key) const;

public slots:
  void load();
  void index();
  void deepLink();
  void qmlProtect(const QQmlEngine* const engine) const;

private slots:
  void qmlRegister() const;

private:
  SpriteSetDB();

  QVector<SpriteSetDBEntry*> store;
  QHash<QString, SpriteSetDBEntry*> ind;
};
