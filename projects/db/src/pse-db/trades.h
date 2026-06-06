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

#include <pse-common/types.h>
#include "./db_autoport.h"

struct PokemonDBEntry;
class QQmlEngine;

struct DB_AUTOPORT TradeDBEntry {
  TradeDBEntry();
  TradeDBEntry(QJsonValue& data);
  void deepLink();

  QString give;
  QString get;
  var8    textId   = 0;
  QString nickname;
  bool    unused   = false;

  PokemonDBEntry* toGive = nullptr;
  PokemonDBEntry* toGet  = nullptr;
};

class DB_AUTOPORT TradesDB : public QObject
{
  Q_OBJECT
  Q_PROPERTY(int getStoreSize READ getStoreSize CONSTANT)

public:
  static TradesDB* inst();

  [[nodiscard]] const QVector<TradeDBEntry*> getStore() const;
  [[nodiscard]] int getStoreSize() const;

  Q_INVOKABLE TradeDBEntry* getStoreAt(int idx) const;

public slots:
  void load();
  void deepLink();
  void qmlProtect(const QQmlEngine* const engine) const;

private slots:
  void qmlRegister() const;

private:
  TradesDB();

  QVector<TradeDBEntry*> store;
};
