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
#include <QString>
#include <QVector>
#include <optional>
#include "../db_autoport.h"

struct PokemonDBEntry;
class QQmlEngine;
class EventPokemonDB;


struct DB_AUTOPORT EventPokemonDBEntry : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString          getTitle     READ getTitle     CONSTANT)
  Q_PROPERTY(QString          getDesc      READ getDesc      CONSTANT)
  Q_PROPERTY(QString          getPokemon   READ getPokemon   CONSTANT)
  Q_PROPERTY(QString          getRegion    READ getRegion    CONSTANT)
  Q_PROPERTY(PokemonDBEntry*  getToPokemon READ getToPokemon CONSTANT)
  Q_PROPERTY(int getOtId  READ getOtId  CONSTANT)
  Q_PROPERTY(int getDvAtk READ getDvAtk CONSTANT)
  Q_PROPERTY(int getDvDef READ getDvDef CONSTANT)
  Q_PROPERTY(int getDvSpd READ getDvSpd CONSTANT)
  Q_PROPERTY(int getDvSp  READ getDvSp  CONSTANT)
  Q_PROPERTY(int getLevel READ getLevel CONSTANT)

public:
  QString getTitle()     const;
  QString getDesc()      const;
  QString getPokemon()   const;
  QVector<QString> getOtName()  const;
  QString getRegion()    const;
  QVector<QString> getMoves()   const;
  PokemonDBEntry* getToPokemon() const;
  int getLevel()  const;
  int getOtId()   const;
  int getDvAtk()  const;
  int getDvDef()  const;
  int getDvSpd()  const;
  int getDvSp()   const;

public slots:
  void qmlProtect(const QQmlEngine* const engine) const;

protected slots:
  void qmlRegister() const;

protected:
  EventPokemonDBEntry();
  EventPokemonDBEntry(const QJsonValue& data);
  void deepLink();

  QString title = "";
  QString desc = "";
  QString pokemon = "";
  QVector<QString> otName;
  QString region = "";
  QVector<QString> moves;
  int level = -1;
  int otId = -1;
  int dvAtk = -1;
  int dvDef = -1;
  int dvSpd = -1;
  int dvSp = -1;
  PokemonDBEntry* toPokemon = nullptr;

  friend class EventPokemonDB;
};
