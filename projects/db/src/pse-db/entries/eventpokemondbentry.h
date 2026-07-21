/*
  * Copyright 2020 Fairy Fox
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


/**
 * @brief One real-world event-distribution Pokemon preset.
 *
 * QObject-getter style DB entry. Describes a distribution (@ref title / @ref desc /
 * @ref region) and the exact mon it gives -- species, level, OT id, the four DVs,
 * OT name options, and moves. deepLink() resolves @ref toPokemon. See db.md.
 *
 * @see EventPokemonDB, PokemonBox::newPokemon().
 */
struct DB_AUTOPORT EventPokemonDBEntry : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString          getTitle     READ getTitle     CONSTANT) ///< Distribution title.
  Q_PROPERTY(QString          getDesc      READ getDesc      CONSTANT) ///< Distribution description.
  Q_PROPERTY(QString          getPokemon   READ getPokemon   CONSTANT) ///< Species name.
  Q_PROPERTY(QString          getRegion    READ getRegion    CONSTANT) ///< Region it was distributed in.
  Q_PROPERTY(PokemonDBEntry*  getToPokemon READ getToPokemon CONSTANT) ///< Resolved species.
  Q_PROPERTY(int getOtId  READ getOtId  CONSTANT) ///< Original-trainer id.
  Q_PROPERTY(int getDvAtk READ getDvAtk CONSTANT) ///< Attack DV.
  Q_PROPERTY(int getDvDef READ getDvDef CONSTANT) ///< Defense DV.
  Q_PROPERTY(int getDvSpd READ getDvSpd CONSTANT) ///< Speed DV.
  Q_PROPERTY(int getDvSp  READ getDvSp  CONSTANT) ///< Special DV.
  Q_PROPERTY(int getLevel READ getLevel CONSTANT) ///< Level.

public:
  QString getTitle()     const; ///< @see getTitle property.
  QString getDesc()      const; ///< @see getDesc property.
  QString getPokemon()   const; ///< @see getPokemon property.
  QVector<QString> getOtName()  const; ///< OT-name options.
  QString getRegion()    const; ///< @see getRegion property.
  QVector<QString> getMoves()   const; ///< Move names the mon comes with.
  PokemonDBEntry* getToPokemon() const; ///< @see getToPokemon property.
  int getLevel()  const; ///< @see getLevel property.
  int getOtId()   const; ///< @see getOtId property.
  int getDvAtk()  const; ///< @see getDvAtk property.
  int getDvDef()  const; ///< @see getDvDef property.
  int getDvSpd()  const; ///< @see getDvSpd property.
  int getDvSp()   const; ///< @see getDvSp property.

public slots:
  void qmlProtect(const QQmlEngine* const engine) const; ///< Pin to C++ ownership.

protected slots:
  void qmlRegister() const; ///< Register with QML.

protected:
  EventPokemonDBEntry();                    ///< Empty entry (built by EventPokemonDB).
  EventPokemonDBEntry(const QJsonValue& data); ///< Build from a JSON value.
  void deepLink();                          ///< Resolve the species link.

  QString title = "";   ///< Backing field (read via getTitle()).
  QString desc = "";    ///< Backing field (read via getDesc()).
  QString pokemon = ""; ///< Backing field (read via getPokemon()).
  QVector<QString> otName; ///< OT-name options (read via getOtName()).
  QString region = "";  ///< Backing field (read via getRegion()).
  QVector<QString> moves; ///< Move names (read via getMoves()).
  int level = -1;       ///< Backing field (read via getLevel()).
  int otId = -1;        ///< Backing field (read via getOtId()).
  int dvAtk = -1;       ///< Backing field (read via getDvAtk()).
  int dvDef = -1;       ///< Backing field (read via getDvDef()).
  int dvSpd = -1;       ///< Backing field (read via getDvSpd()).
  int dvSp = -1;        ///< Backing field (read via getDvSp()).
  PokemonDBEntry* toPokemon = nullptr; ///< Resolved species (deepLink).

  friend class EventPokemonDB; ///< Owning DB constructs/populates entries.
};
