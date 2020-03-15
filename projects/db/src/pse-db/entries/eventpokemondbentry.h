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
#ifndef EVENTPOKEMONDBENTRY_H
#define EVENTPOKEMONDBENTRY_H

#include <QObject>
#include <QString>
#include <optional>

#include "../db_autoport.h"

struct PokemonDBEntry;
class QQmlEngine;
class EventPokemonDB;

struct DB_AUTOPORT EventPokemonDBEntry : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString getTitle READ getTitle)
  Q_PROPERTY(QString getDesc READ getDesc)
  Q_PROPERTY(QString getPokemon READ getPokemon)
  Q_PROPERTY(QVector<QString> getOtName READ getOtName)
  Q_PROPERTY(QString getRegion READ getRegion)
  Q_PROPERTY(QVector<QString> getMoves READ getMoves)
  Q_PROPERTY(PokemonDBEntry* getToPokemon READ getToPokemon)
  Q_PROPERTY(int getOtId READ getOtId)
  Q_PROPERTY(int getDvAtk READ getDvAtk)
  Q_PROPERTY(int getDvDef READ getDvDef)
  Q_PROPERTY(int getDvSpd READ getDvSpd)
  Q_PROPERTY(int getDvSp READ getDvSp)
  Q_PROPERTY(int getLevel READ getLevel)

public:
  const QString getTitle() const;
  const QString getDesc() const;
  const QString getPokemon() const;
  const QVector<QString> getOtName() const;
  const QString getRegion() const;
  const QVector<QString> getMoves() const;
  const PokemonDBEntry* getToPokemon() const;
  int getLevel() const;
  int getOtId() const;
  int getDvAtk() const;
  int getDvDef() const;
  int getDvSpd() const;
  int getDvSp() const;

public slots:
  void qmlProtect(const QQmlEngine* const engine) const;

protected slots:
  void qmlRegister() const;

protected:
  EventPokemonDBEntry();
  EventPokemonDBEntry(const QJsonValue& data);
  void deepLink();

  QString title = ""; // Event Title
  QString desc = ""; // Event Pokemon Description
  QString pokemon = ""; // Pokemon Name
  QVector<QString> otName; // Pokemon OT Name, if more than 1 it's random
  QString region = ""; // Region Code
  QVector<QString> moves; // Move list
  int level = -1; // Level, default minimum if not specified
  int otId = -1; // Pokemon OT ID, random if not specified
  int dvAtk = -1; // Pokemon DV List, random if not specified
  int dvDef = -1; // Pokemon DV List, random if not specified
  int dvSpd = -1; // Pokemon DV List, random if not specified
  int dvSp = -1; // Pokemon DV List, random if not specified

  PokemonDBEntry* toPokemon = nullptr; // Deep link to associated Pokemon

  friend class EventPokemonDB;
};

#endif // EVENTPOKEMONDBENTRY_H
