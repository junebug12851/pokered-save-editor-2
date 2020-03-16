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
#include "eventpokemondbentry.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValueRef>
#include <QQmlEngine>
#include <pse-common/utility.h>

#ifdef QT_DEBUG
#include <QtDebug>
#endif

#include "../util/gamedata.h"
#include "../pokemon.h"
#include "../eventpokemondb.h"

EventPokemonDBEntry::EventPokemonDBEntry() {
  qmlRegister();
}
EventPokemonDBEntry::EventPokemonDBEntry(const QJsonValue& data) {
  qmlRegister();

  // Set simple properties
  title = data["title"].toString();
  desc = data["desc"].toString();
  pokemon = data["pokemon"].toString();
  region = data["region"].toString();

  // Set simple optional proeprties
  if(data["otID"].isString())
    otId = data["otID"].toString().toInt(nullptr, 16);

  if(data["level"].isDouble())
    level = data["level"].toDouble();

  // Set Moves
  auto movesArr = data["moves"].toArray();
  for(QJsonValue moveEntry : movesArr)
    moves.append(moveEntry.toString());

  // Set OT Name
  // Can be array or string
  if(data["otName"].isArray())
  {
    auto otNameArr = data["otName"].toArray();

    for(auto otNameEntry : otNameArr)
      otName.append(otNameEntry.toString());
  }
  else
    otName.append(data["otName"].toString());

  // Set DV
  if(data["dv"].isString())
  {
    // DV Can be
    // "max"             All DV Values of 15
    // ":##:##:##:##"    Specific DV Values
    //                   Atk, Def, Spd, Sp
    auto dvVal = data["dv"].toString();

    if(dvVal == "max") {
      dvAtk = 15;
      dvDef = 15;
      dvSpd = 15;
      dvSp = 15;
    }
    else
    {
      auto dvValParts = dvVal.split(":", QString::SkipEmptyParts);
      dvAtk = dvValParts[0].toInt(nullptr, 10);
      dvDef = dvValParts[1].toInt(nullptr, 10);
      dvSpd = dvValParts[2].toInt(nullptr, 10);
      dvSp = dvValParts[3].toInt(nullptr, 10);
    }
  }
}

void EventPokemonDBEntry::deepLink()
{
  toPokemon = PokemonDB::ind.value(pokemon, nullptr);

#ifdef QT_DEBUG
  if(toPokemon == nullptr)
    qCritical() << "Event Pokemon: " << pokemon << ", could not be deep linked." ;
#endif

  if(toPokemon != nullptr)
    toPokemon->toEventMons.append(this);
}

int EventPokemonDBEntry::getDvSp() const
{
  return dvSp;
}

void EventPokemonDBEntry::qmlProtect(const QQmlEngine* const engine) const
{
  Utility::qmlProtectUtil(this, engine);
}

void EventPokemonDBEntry::qmlRegister() const
{
  static bool once = false;
  if(once)
    return;

  qmlRegisterUncreatableType<EventPokemonDBEntry>("PSE.DB.EventPokemonDBEntry", 1, 0, "EventPokemonDBEntry", "Can't instantiate in QML");
  once = true;
}

int EventPokemonDBEntry::getDvSpd() const
{
  return dvSpd;
}

int EventPokemonDBEntry::getDvDef() const
{
  return dvDef;
}

int EventPokemonDBEntry::getDvAtk() const
{
  return dvAtk;
}

const PokemonDBEntry* EventPokemonDBEntry::getToPokemon() const
{
  return toPokemon;
}

int EventPokemonDBEntry::getLevel() const
{
  return level;
}

const QVector<QString> EventPokemonDBEntry::getMoves() const
{
  return moves;
}

const QString EventPokemonDBEntry::getRegion() const
{
  return region;
}

int EventPokemonDBEntry::getOtId() const
{
  return otId;
}

const QVector<QString> EventPokemonDBEntry::getOtName() const
{
  return otName;
}

const QString EventPokemonDBEntry::getPokemon() const
{
  return pokemon;
}

const QString EventPokemonDBEntry::getDesc() const
{
  return desc;
}

const QString EventPokemonDBEntry::getTitle() const
{
  return title;
}
